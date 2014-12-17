/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>
#include <errno.h>
#include "threadprovider.h"
#include "mountmanager.h"

#include <vdr/plugin.h>
#include <vdr/remote.h>
#include <vdr/skins.h>

#include "filetools.h"

#include "../bgprocess/service.h"
//-----------------------------------------------------------------------
//--------------------------cThreadProvider------------------------------
//-----------------------------------------------------------------------

cThreadProvider::~cThreadProvider()
{
    for(std::list<cWorkThread*>::iterator i = threads_.begin(); i != threads_.end(); i++)
    { 
         (*i)->CancelThread();
         delete *i;
    } 
    for(std::list<cTask*>::iterator i = tasks_.begin(); i != tasks_.end(); i++)
    { 
         delete *i;
    }
}

void cThreadProvider::InsertNewTask(cTask *task)  //must be allocated with "new"
{ 
     //printf("-----InsertNewTask------\n");
     Lock();
     //DeleteAllFinishedThreads(false); 
     if (nrThreadsRunning() == 0 || task->Parallel())
     {
         task->GetState() = stStarted;
         threads_.push_back(task->NewThread(this));
         if(threads_.back()->Start())      
             task->GetState() = stStarted;
         else
             task->GetState() = stFailed;
     }
     tasks_.push_back(task);
     Unlock();
}

bool cThreadProvider::GetNextFinishedTask(cTask **task) //must be released with "delete"
{
     //printf("-----GetNextFinishedTask----\n");
     Lock();
     for(std::list<cTask*>::iterator i = tasks_.begin(); i != tasks_.end(); i++)
     {
         if((*i)->GetState() == stFinished || (*i)->GetState() == stFailed)
         {
             DeleteThread((*i)->GetThread());
             *task = *i;
             tasks_.erase(i);
             Unlock();
             return true;
         }
     }
     Unlock();
     return false;
}

void cThreadProvider::StartNextThread(bool lock)
{
     //printf("-------StartNextThread------\n");
     if(lock)
         Lock();
     //DeleteAllFinishedThreads(false);
     for(std::list<cTask*>::iterator i = tasks_.begin(); i != tasks_.end(); i++)
     {
         if ((*i)->GetState() == stIdle && nrThreadsRunning() == 0)
         {
              threads_.push_back((*i)->NewThread(this));
              if(threads_.back()->Start())
                  (*i)->GetState() = stStarted;
              else
                  (*i)->GetState() = stFailed;
              break;
         } 
     }
     if(lock)
         Unlock(); 
}

void cThreadProvider::Lock()
{
    pthread_mutex_lock(&mutex_);
}

void cThreadProvider::Unlock()
{
    pthread_mutex_unlock(&mutex_);
}

void cThreadProvider::CancelAllThreads()
{
   for(std::list<cWorkThread*>::iterator i = threads_.begin(); i != threads_.end(); i++)
    { 
         (*i)->CancelThread();
    } 
}

int cThreadProvider::nrThreadsRunning() const
{
    int i = 0;
    for(std::list<cWorkThread*>::const_iterator it = threads_.begin(); it != threads_.end(); it++)
         if((*it)->GetTask()->GetState() == stStarted)
         {
             ++i;
         }
    return i;
} 

void cThreadProvider::DeleteThread(cWorkThread *thread)
{
    //printf("------DeleteThread-----\n");
    for(std::list<cWorkThread*>::iterator i = threads_.begin(); i != threads_.end(); i++)
    {
        if((*i) == thread)
        {
            delete(*i);
            threads_.erase(i);
            break;
        }
    }
}

/*void cThreadProvider::DeleteAllFinishedThreads(bool lock)
{
    if(lock)
        Lock();
    for(std::list<cWorkThread*>::iterator i = threads_.begin(); i != threads_.end(); i++)
    {
        if((*i)->Finished() || (*i)->Failed())
        {
            delete(*i);
            threads_.erase(i);
        }
     }
    if(unlock) 
        Unlock();
}*/

//-----------------------------------------------------------------------
//--------------------------cTask----------------------------------------
//-----------------------------------------------------------------------

cTask::cTask(bool parallel)
: state_(stIdle),parallel_(true), thread_(NULL) 
{
}

const taskState &cTask::GetState() const
{
    return state_;
}

taskState &cTask::GetState() 
{
    return state_;
}

cWorkThread *cTask::GetThread()
{
    return thread_;
}

void cTask::Started()
{
    startTime_ = time(NULL);
}

void cTask::Finished()
{
    state_ = stFinished;
}

void cTask::Progress(){}


void cTask::InformBkgAction(std::string process, std::string description, int percentage)
{
    BgProcessData_t BgProcessData = {
                    process,
                    description,
                    startTime_,
                    percentage,
                    0, /* pid */
                    "" /*caller*/
                };
    cPluginManager::CallAllServices("bgprocess-data", &BgProcessData);
}

//--------------------------cCopyTask------------------------------------

cCopyTask::cCopyTask(const std::vector<std::string> &files,const std::string &targetDir, bool remove)
: files_(files), targetDir_(targetDir), remove_(remove), cmd_ ("cp -r "), bytesTotal_(0), bytesCopied_(0)
{
}
 
cWorkThread *cCopyTask::NewThread(cThreadProvider *provider)
{
    //printf("----cCopyTask::NewThread----\n");
    thread_ = new cCopyThread(this, provider);
    return thread_;
}

void cCopyTask::Started()
{
    //printf("----cCopyTask::Started-----\n");
    startTime_ = time(NULL);
    if(files_.size()>1)
       InformBkgAction(tr("Copying"), tr("Multiple files"), 0);
    else
       InformBkgAction(tr("Copying"), GetLast(files_[0]), 0);
}

void cCopyTask::Finished()
{
    //printf("----cCopyTask::Finished----\n"); 
    state_ = stFinished;
    if(files_.size()>1)
       InformBkgAction(tr("Copying"), tr("Multiple files"), 101);
    else
       InformBkgAction(tr("Copying"), GetLast(files_[0]), 101);
}

void cCopyTask::Progress()
{
    static int lastPercentage = 0;
    int percentage = 0;
    if(bytesTotal_ >=  bytesCopied_ && bytesTotal_ != 0)
    {
        percentage =  (100 * bytesCopied_) / bytesTotal_;
    }
    //printf("--percentage = %d---bytesCopied_ = %lld---bytesTotal_ = %lld\n", percentage, bytesCopied_, bytesTotal_);
    if(percentage != lastPercentage)
    {
        if(files_.size()>1)
           InformBkgAction(tr("Copying"), tr("Multiple files"), percentage);
        else
           InformBkgAction(tr("Copying"), GetLast(files_[0]), percentage);
        lastPercentage = percentage;
    }
}

void cCopyTask::ProcessTask()
{
    cMountManager tmpMountMgr;
    tmpMountMgr.MountAll(files_);

    bytesTotal_ = GetTotalBytes(files_, bytesTotal_);

    uint i = 0;
    while (thread_->Processing() && i < files_.size())
    { 
        if(Filetools::IsDir(files_[i]))
        {
            CopyDir(files_[i], targetDir_ + "/" + GetLast(files_[i]), bytesCopied_);
        }
        else if(Filetools::IsFile(files_[i]))
        {
            std::string destFile = targetDir_ + "/" + GetLast(files_[i]);
            CopyFile(files_[i], destFile, bytesCopied_);
        }    
        if(remove_)
        { 
            std::string cmd = "sudo nice -n 10 rm -rf " + files_[i];
            ::SystemExec(cmd.c_str()); 
        }
        i++;
    } 
    tmpMountMgr.Umount();
}

/*void cCopyTask::ProcessTask()
{
    cMountManager tmpMountMgr;
    tmpMountMgr.MountAll(files_);

    bytesTotal_ = GetTotalBytes(files_);

    uint i = 0;
    while (thread_->Processing() && i < files_.size())
    {
        std::string file = files_[i];
        std::string cmd = cmd_ +  "\"" + file + "\" " + "\"" + targetDir_ + "\"";
        ::SystemExec(cmd.c_str());  
        i++;
        printf("---%s---\n", cmd.c_str());
    }
    tmpMountMgr.Umount();
}*/

void cCopyTask::CopyDir(std::string srcDir, std::string destDir, long long &bytesCopied)
{ 
    //printf("-------cCopyTask::CopyDir, srcDir = %s, destDir = %s-----\n", srcDir.c_str(), destDir.c_str());

    if(!Filetools::DirExists(destDir) && mkdir(destDir.c_str(), S_IRUSR|S_IWUSR|S_IXUSR) != 0)
    {
        printf("cannot create dir %s\n", destDir.c_str());
        return;
    }

    std::vector<std::string> entries;
    if(Filetools::GetDirEntries(srcDir, entries))
    {
        for(uint i = 0; i < entries.size(); ++i)
        {
            if(Filetools::IsDir(entries[i]))
            {
                if(GetLast(entries[i]) != "." && GetLast(entries[i]) != "..")
                {
                    CopyDir(entries[i], destDir + "/" + GetLast(entries[i]), bytesCopied);
                }
            }
            else if(Filetools::IsFile(entries[i]))
            {
                CopyFile(entries[i], destDir + "/" + GetLast(entries[i]), bytesCopied);
            }
        }
    }

    //Set access rights and ownership
     TransferRightsAndOwnership(srcDir, destDir);
}

void cCopyTask::CopyFile(std::string srcFile,  std::string destFile, long long &bytesCopied)
{
    //printf("-------------CopyFile %s %s--------------\n",  srcFile.c_str(),  destFile.c_str());
    int fdSrc = open(srcFile.c_str(), O_RDONLY);
    if(fdSrc == -1)
    {   
	    esyslog("Error \"%s\" while opening file \"%s\"\n", strerror(errno), srcFile.c_str());
        return;
    }

    int fdDest = open(destFile.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR );
    if(fdDest == -1)
    {
	    esyslog("Error \"%s\" while opening file \"%s\"\n", strerror(errno), destFile.c_str());
        return;
    }

    const int bufsize = 512000; //0,5 MB chunks
    uchar buffer[bufsize];
    long long bytesRead = read(fdSrc, buffer, bufsize);
    while(bytesRead > 0)
    {
        if(bytesRead != write(fdDest, buffer, bytesRead))
        {
            esyslog("Error \"%s\" while writing to file \"%s\"\n", strerror(errno), destFile.c_str());
            break;
        }
        bytesCopied += bytesRead;
        Progress();
        bytesRead = read(fdSrc, buffer, bufsize);
        cCondWait::SleepMs(10);
    }

    close(fdSrc);
    close(fdDest);

    //Set access rights and ownership
    TransferRightsAndOwnership(srcFile, destFile);
}

long long cCopyTask::GetTotalBytes(std::vector<std::string> entries, long long &totalBytes)
{
    for(uint i = 0; i < entries.size(); ++i)
    {
         if(Filetools::IsDir(entries[i]) &&  GetLast(entries[i]) != "." && GetLast(entries[i]) != "..")
         {
             std::vector<std::string> newEntries;
             if(Filetools::GetDirEntries(entries[i], newEntries))
             {
                 GetTotalBytes(newEntries, totalBytes);
             }
         }
         else if(Filetools::IsFile(entries[i]))
         {
             totalBytes += Filetools::GetFileSize(entries[i]);
         }
    }
    return totalBytes;
}

void cCopyTask::TransferRightsAndOwnership(std::string srcFile, std::string destFile)
{
    mode_t mode;
    uid_t owner;
    gid_t group;
    Filetools::GetRights(srcFile, mode);
    Filetools::GetOwner(srcFile, owner, group);
    Filetools::SetRights(destFile, mode);
    Filetools::SetOwner(destFile, owner, group);
}

//--------------------------cMoveTask------------------------------------

cMoveTask::cMoveTask(const std::vector<std::string> &files, const std::string &targetDir)
: files_(files), targetDir_(targetDir), cmd_("mv ")
{
}

cWorkThread *cMoveTask::NewThread(cThreadProvider *provider)
{
    //printf("--------cMoveTask::NewThread-------\n");
    thread_ = new cMoveThread(this, provider);
    return thread_; 
}

void cMoveTask::ProcessTask()
{ 
    printf("################################cMoveTask::ProcessTask()############################\n");
    cMountManager tmpMountMgr;
    tmpMountMgr.MountAll(files_);
    uint i = 0;
    while (thread_->Processing() && i < files_.size())
    {
        std::string file = files_[i];
        std::string cmd = "nice -n 10 " + cmd_ +  "\"" + file + "\" " + "\"" + targetDir_ + "\"";
        ::SystemExec(cmd.c_str());  
        i++;
        printf("---%s---\n", cmd.c_str());
    }
    tmpMountMgr.Umount();
}
 
//--------------------------cDeleteTask----------------------------------

cDeleteTask::cDeleteTask(const std::vector<std::string> &files, const std::string &dir)
: files_(files), dir_(dir), cmd_("rm -rf ")
{
}

cWorkThread *cDeleteTask::NewThread(cThreadProvider *provider)
{
    thread_ = new cDeleteThread(this, provider);
    return thread_; 
}

void cDeleteTask::ProcessTask()
{
    cMountManager tmpMountMgr;
    tmpMountMgr.MountAll(files_);
    uint i = 0;
    while (thread_->Processing() && i < files_.size())
    {
        std::string file = files_[i]; 
        std::string cmd = "nice -n 10 " + cmd_ + "\"" + file + "\" ";
        ::SystemExec(cmd.c_str()); 
        i++; 
        printf("----%s---\n", cmd.c_str());
    }
    tmpMountMgr.Umount(); 
}

//-----------------------------------------------------------------------
//--------------------------cWorkThread----------------------------------
//-----------------------------------------------------------------------

cWorkThread::cWorkThread(cTask *task, cThreadProvider *provider)
: task_(task), provider_(provider)
{
}

void cWorkThread::LockProvider() 
{
    provider_->Lock(); 
}

void cWorkThread::UnlockProvider()
{
    provider_->Unlock();   
}

void cWorkThread::CancelThread()    
{
    Cancel(1000);
}

void cWorkThread::Action()
{
    task_->Started();
    task_->ProcessTask();
    LockProvider(); 
    task_->Finished();
    provider_->StartNextThread(false);
    UnlockProvider();
}

//--------------------------cCopyThread----------------------------------

cCopyThread::cCopyThread(cCopyTask *task, cThreadProvider *provider) 
: cWorkThread(task, provider)
{
    SetPriority(10);
}

cCopyTask *cCopyThread::GetCopyTask()
{
    return static_cast<cCopyTask*>(task_);
}

//--------------------------cMoveThread----------------------------------

cMoveThread::cMoveThread(cMoveTask *task, cThreadProvider *provider) 
: cWorkThread(task, provider)
{
    SetPriority(10);
}

cMoveTask *cMoveThread::GetMoveTask()
{
    return static_cast<cMoveTask*>(task_);
}


//--------------------------cDeleteThread----------------------------------

cDeleteThread::cDeleteThread(cDeleteTask *task, cThreadProvider *provider) 
: cWorkThread(task, provider)
{
    SetPriority(10);
}


cDeleteTask *cDeleteThread::GetDeleteTask()
{
    return static_cast<cDeleteTask*>(task_);
}
