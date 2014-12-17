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
 
#ifndef P__THREADPROVIDER_H
#define P__THREADPROVIDER_H

#include <string>
#include <vector>
#include <list>

#include <vdr/thread.h>

#include "stringtools.h"

//-----------------------------------------------------------------------
//--------------------------cThreadProvider------------------------------
//-----------------------------------------------------------------------

class cTask;
class cWorkThread;

class cThreadProvider 
{
public:
    cThreadProvider(){}; 
    ~cThreadProvider();
    void InsertNewTask(cTask *task);  //must be allocated with "new"
    bool GetNextFinishedTask(cTask **task); //must be released with "delete"
    bool HasTasks(){ return !tasks_.empty(); }
    void StartNextThread(bool lock);
    void Lock();
    void Unlock();
    void CancelAllThreads();

private:
    std::list<cWorkThread*> threads_;
    std::list<cTask*> tasks_;
    pthread_mutex_t mutex_;

    int nrThreadsRunning() const;
    void DeleteThread(cWorkThread *thread);
    //void DeleteAllFinishedThreads(bool lock);
};

//-----------------------------------------------------------------------
//------------------------------cTask------------------------------------
//-----------------------------------------------------------------------

enum taskState
{
    stIdle,
    stStarted,
    stFinished,
    stFailed
};

//--------------------------cTask----------------------------------------

class cTask
{
public:
    cTask(bool parallel = true);
    virtual ~cTask(){};
    const taskState &GetState() const;
    taskState &GetState(); 
    cWorkThread *GetThread();
    virtual cWorkThread *NewThread(cThreadProvider *provider) = 0;
    int ReturnVal() {return ret_;}
    bool Parallel() const {return parallel_;}  
    virtual void Started();
    virtual void Finished();
    virtual void Progress();
    virtual void ProcessTask(){};

protected:
    taskState state_;
    bool parallel_;
    cWorkThread *thread_;
    time_t startTime_;
    int ret_;
    void InformBkgAction(std::string process, std::string description, int percentage);
};

//--------------------------cCopyTask------------------------------------

class cCopyTask : public cTask 
{ 
    friend class cCopyThread;
public:
    cCopyTask(const std::vector<std::string> &files,const std::string &targetDir, bool remove = false);
    /*override*/ ~cCopyTask(){};
    /*override*/ cWorkThread *NewThread(cThreadProvider *provider);
    /*override*/ void Started();
    /*override*/ void Finished(); 
    /*override*/ void Progress();
    /*override*/ void ProcessTask();

private:
    std::vector<std::string> files_;
    std::string targetDir_; 
    std::string cmd_;
    long long bytesTotal_; 
    long long bytesCopied_;
    bool remove_;

    void CopyFile(std::string srcFile,  std::string destFile, long long &bytesCopied);
    void CopyDir(std::string srcDir, std::string destDir, long long &bytesCopied);
    long long GetTotalBytes(std::vector<std::string> entries, long long &totalBytes);
    void TransferRightsAndOwnership(std::string srcFile, std::string destFile);
};

//--------------------------cMoveTask------------------------------------

class cMoveTask :  public cTask
{ 
    friend class cMoveThread;
public:
    cMoveTask(const std::vector<std::string> &files, const std::string &targetDir);
    /*override*/ ~cMoveTask(){};
    /*override*/ cWorkThread *NewThread(cThreadProvider *provider);
    /*override*/ void ProcessTask();
 
private:
    std::vector<std::string> files_; 
    std::string targetDir_;
    std::string cmd_;
};

//--------------------------cDeleteTask----------------------------------

class cDeleteTask :  public cTask
{ 
    friend class cDeleteThread;
public:
    cDeleteTask(const std::vector<std::string> &files, const std::string &dir);
    /*override*/ ~cDeleteTask(){};
    /*override*/ cWorkThread *NewThread(cThreadProvider *provider);
    /*override*/ void ProcessTask();

private:
    std::vector<std::string> files_;
    std::string dir_; 
    std::string cmd_;
};


//--------------------------cInstallDebPackTask----------------------------------

class cInstallDebPackTask :  public cTask
{ 
    friend class cInstallDebPackThread;
public:
    cInstallDebPackTask(const std::string &file);
    /*override*/ ~cInstallDebPackTask(){};
    /*override*/ cWorkThread *NewThread(cThreadProvider *provider);
    /*override*/ void ProcessTask();

private:
    std::string file_; 
};

//-----------------------------------------------------------------------
//--------------------------cWorkThread----------------------------------
//-----------------------------------------------------------------------

class cWorkThread : public cThread
{
public:
     cWorkThread(cTask *task, cThreadProvider *provider);
     virtual ~cWorkThread(){};
     void CancelThread();
     cTask *GetTask(){ return task_; };
     bool Processing(){ return Running(); };

protected: 
     void LockProvider(); 
     void UnlockProvider();
     cTask *task_;
     cThreadProvider *provider_;
     void Action();
};

//--------------------------cCopyThread----------------------------------

class cCopyThread : public cWorkThread
{
public:
    cCopyThread(cCopyTask *task, cThreadProvider *provider);
    /*override*/ ~cCopyThread(){};

private:
    cCopyTask *GetCopyTask();
};

//--------------------------cCopyThread----------------------------------

class cMoveThread : public cWorkThread
{
public:
    cMoveThread(cMoveTask *task, cThreadProvider *provider);
    /*override*/  ~cMoveThread(){};

private:
    cMoveTask *GetMoveTask();
};

//--------------------------cCopyThread----------------------------------

class cDeleteThread : public cWorkThread
{
public:
    cDeleteThread(cDeleteTask *task, cThreadProvider *provider);
    /*override*/  ~cDeleteThread(){};

private:
    cDeleteTask *GetDeleteTask();
};

//------------------------cInstallDebPackThread-----------------------------

class cInstallDebPackThread : public cWorkThread
{
public:
    cInstallDebPackThread(cInstallDebPackTask *task, cThreadProvider *provider);
    /*override*/  ~cInstallDebPackThread(){};

private:
    cInstallDebPackTask *GetInstallDebPackTask();
};

#endif //P__THREADPROVIDER_H
