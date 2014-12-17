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


#include <vdr/status.h>
#include <vdr/remote.h>

#include "def.h"
#include "browserStatus.h"
#include "playlist.h"
#include "userIfBase.h"
#include "fileType.h"
#include "filetools.h"

//------------------------------------------------
//----------cFileBrowserStatus--------------------
//------------------------------------------------

cFileBrowserStatus::cFileBrowserStatus()
: valid_(true), playListActive_(false), 
  normalExit_(false), enterPlayList_(false), closePlayList_(false), stillPictureActive_(false), calledFromMenu_(false), 
  mode_(expert), caller_(normal), xineCtrl_ (xine_browse), sortMode_(byalph), filter_(cl_unknown), id_(-1), 
  exitDir_(BASE_DIR), startDir_(BASE_DIR), startFile_(""),
  extInfo_(NULL) //, playList_(cPlayList::GetDefaultName())
{
    playlist_.reset(new cPlayList());
}

void cFileBrowserStatus::Init()
{
    if(xineCtrl_ == xine_select_playlist)
    {
	startDir_ = PLAYLIST_DEFAULT_PATH;
	if(!Filetools::DirExists(PLAYLIST_DEFAULT_PATH) && !Filetools::CreateDir(PLAYLIST_DEFAULT_PATH))
	{
	    startDir_ = MUSIC_DIR;
	}  
    }  
    if(xineCtrl_ == xine_save_playlistas)
    {
	startDir_ = PLAYLIST_DEFAULT_PATH;
    }  
    if(playlist_.get()->GetName() == "")
    {  
	playlist_.get()->SetName(cPlayList::GetDefaultName(startDir_));
    }
    SetFilter();
}

void cFileBrowserStatus::Reset()
{
    valid_ = true;
    playListActive_ = false; 
    normalExit_ = false; 
    enterPlayList_ = false; 
    closePlayList_ = false; 
    stillPictureActive_ = false;
    mode_ = expert; 
    caller_ = normal; 
    xineCtrl_ = xine_browse;
    sortMode_ = byalph; 
    filter_ = cl_unknown;
    id_ = -1;
    exitDir_ = BASE_DIR;
    startDir_ = BASE_DIR; 
    startDirs_.clear(); 
    startFile_ = "";
    playlist_.reset(new cPlayList());
    ifTypes_.clear();
}

void cFileBrowserStatus::Merge(cFileBrowserStatus &newstatus)
{
    valid_ = (newstatus.valid_ != true) ?  false : valid_; 
    playListActive_ = (newstatus.playListActive_ != false) ?  true : playListActive_;  
    normalExit_ = (newstatus.normalExit_ != false) ?  true : normalExit_; 
    enterPlayList_= (newstatus.enterPlayList_ != false) ?  true : enterPlayList_; 
    closePlayList_= (newstatus.closePlayList_ != false) ?  true : closePlayList_; 
    stillPictureActive_ = (newstatus.stillPictureActive_ != false) ?  true : stillPictureActive_; 
    mode_ = (newstatus.mode_ != expert) ?  newstatus.mode_ : mode_; 
    caller_ = (newstatus.caller_  != normal) ?  newstatus.caller_ : caller_; 
    xineCtrl_ = (newstatus.xineCtrl_ != xine_browse) ?  newstatus.xineCtrl_ : xineCtrl_ ; 
    sortMode_ = (newstatus.sortMode_ != byalph) ?  newstatus.sortMode_ : sortMode_; 
    filter_ = (newstatus.filter_ != cl_unknown) ?  newstatus.filter_ : filter_; 
    exitDir_ = (newstatus.exitDir_ != BASE_DIR) ?  newstatus.exitDir_ : exitDir_; 

    std::string defaultStartDir = GetDefStartDir(newstatus.filter_);;
    //printf("--cFileBrowserStatus::Merge, newstatus.startDir_ = %s, startDir_ = %s--\n", newstatus.startDir_.c_str(), startDir_.c_str());
    startDir_ = (newstatus.startDir_ != defaultStartDir /*BASE_DIR*/)  ?  newstatus.startDir_ : startDir_; 
    //printf("--2cFileBrowserStatus::Merge, newstatus.startDir_ = %s, startDir_ = %s--\n", newstatus.startDir_.c_str(), startDir_.c_str());
    /*if(caller_ != xine) //do not remember startdir, except when called back by xine
    {
      newstatus.startDir_ = defaultStartDir;
    } */ 
 
    startFile_ = (newstatus.startFile_ != "") ?  newstatus.startFile_ : startFile_; 
}

void cFileBrowserStatus::SetStartDir(std::string dir, std::string file)
{
    startDir_ = dir;
    startFile_ = file;
}

void cFileBrowserStatus::SetCaller(browserCaller caller)
{
    caller_ = caller;
}

void cFileBrowserStatus::SetPlayListEntries(std::vector<std::string> entries)
{
    //printf("cFileBrowserStatus::SetPlayListEntries, entries.size() = %d\n", entries.size());
    playlist_->SetPlayListEntries(entries);
}  

std::string cFileBrowserStatus::SavePlaylist()
{
    return playlist_->SaveToFile();
}  
    
void cFileBrowserStatus::BurnPlayList()
{
    playlist_->CopyPlayListToCD();
}  

void cFileBrowserStatus::XineControl(xineControl cmd)
{
    xineCtrl_ = cmd; 
}

void cFileBrowserStatus::SetMode(browseMode mode)
{
    mode_ = mode;
}

void cFileBrowserStatus::SetSortMode(sortMode mode)
{
    sortMode_ = mode;
}

void cFileBrowserStatus::SetExternalInfo(FileBrowserExternalActionInfo *info)
{
    extInfo_ = info;
}

void cFileBrowserStatus::Shutdown(std::string dir, std::string file, /*std::string playlist,*/ std::string playListEntry)
{
    //printf("----cFileBrowserStatus::Shutdown: caller = %d, playListActive_= %d, mode = %d\n", caller_, playListActive_, mode_);;
    if(!(mode_ == restricted || playListActive_ || stillPictureActive_ /*|| (ifTypes_.size() && ifTypes_[ifTypes_.size() - 1] == if_xine)*/))
    { 
        //delete current instance at next startup 
	//printf("--------DELETE INVALID---------\n");
        valid_ = false;
    }

    if(normalExit_ && !calledFromMenu_ && !mode_ == browseplaylist2)
    { 
        //go back to the calling menu
        CallBackMenu();
    } 

    //printf("---Shutdown: startdir = %s\n", dir.c_str());
    SetStartDir(dir, file);
    
    enterPlayList_ = false; 
    closePlayList_ = false;
    //playListEntry_ = playListEntry;  ???
}

void cFileBrowserStatus::Startup()
{
    //printf("----cFileBrowserStatus::Startup: caller = %d, playListActive_= %d, id_ = %d\n", caller_, playListActive_, id_);

    calledFromMenu_ = (caller_ == mediaplayer || caller_ == normal || caller_ == plugin) && !stillPictureActive_;
    normalExit_ = false;

    if(caller_ == svdrp)
    {
        SetMode(autostart);
    }
    else if(caller_ == mediaplayer)
    {
        SetMode(restricted);
        exitDir_ = GetDefStartDir(filter_);
        if(ifTypes_.size() >= 1)
        {
            if(ifTypes_[ifTypes_.size()-1] == if_playlistmenu)
            {
                enterPlayList_ = true;
            }    
            else if(ifTypes_[ifTypes_.size()-1] == if_playlistbrowser)
            {
                SetMode(browseplaylist);
            }
        }
    }
    else if(caller_ == normal)
    {
        SetMode(expert);
    }
    else if(caller_ == xine)
    {
	//printf("caller = xine, xineCtrl_ =%d\n", xineCtrl_ );
        //always use passed instance
        /*if(xineCtrl_ == xine_browse && playListActive_) 
        {
            //goto browse playlist mode
	    exitDir_ = BASE_DIR; //startDir_;
            SetMode(browseplaylist);
        }*/
        if(xineCtrl_ == xine_browse)
	{
	    exitDir_ = BASE_DIR;
            SetMode(browseplaylist2);
	}  
        else if(xineCtrl_ == xine_edit && playListActive_)
        { 
            //goto playlist
            enterPlayList_ = true;
        }
        else if(xineCtrl_ == xine_exit)
        {
            //goto restricted/standard 
            if(cUserIfBase::GetPreviousIf() == if_playlistmenu && playListActive_)
            {
                enterPlayList_ = true;
            }
            else if(cUserIfBase::GetPreviousIf() == if_restricted)
            {
                if(playListActive_)
                {
                    //closePlayList_ = true;  //xine handles playlist
                }
                SetMode(restricted); 
            }
            else if(cUserIfBase::GetPreviousIf() == if_standard)
            {
                if(playListActive_)
                {
                    //closePlayList_ = true; //xine handles playlist
                }
                SetMode(expert);
            }
            else
            {
                SetMode(restricted);
            }
            playListActive_ = false;
        }
        else if(xineCtrl_ == xine_copy_playlist)
	{
	    exitDir_ = BASE_DIR;
	    SetMode(copyplaylist);
	}
	else if(xineCtrl_ == xine_select_playlist)
	{
	    exitDir_ = BASE_DIR;
	    SetMode(selectplaylist);
	}  
	else if(xineCtrl_ == xine_save_playlistas)
	{
	    exitDir_ = BASE_DIR;
	    SetMode(saveplaylistas);
	}  
    }
    else if(caller_ == plugin)
    {
        //always a new instance
        exitDir_ = startDir_;
        SetMode(external);
    }
}

//Has to be adjusted when main menu changes!!!
void cFileBrowserStatus::CallBackMenu()
{   
    //printf("-----cFileBrowserStatus::CallMainMenu(), filter_ = %d--------\n", filter_);
    cRemote::Put(kNone); //why needed?  
    cRemote::Put(kNone);
    cRemote::Put(kMenu);  

    if((filter_ & cl_picture) || (filter_ & cl_audio))
    {
        cRemote::Put(kDown); 
        cRemote::Put(kOk);

        if(filter_ & cl_picture)    //Music & Pictures / Picture archive
        {
            cRemote::Put(kDown);
        }
        else if(filter_ & cl_audio) //Music & Pictures / Music archive
        {
            ;
        }
    }        
    else if(filter_ & cl_video) //Videos & Dvds / video archive
    { 
        cRemote::Put(kDown); 
        cRemote::Put(kDown); 
        cRemote::Put(kOk);
    }
    else if(filter_ & cl_unknown) //Internet & Extras / File Browser
    {
        cRemote::Put(kDown); 
        cRemote::Put(kDown); 
        cRemote::Put(kDown); 
        cRemote::Put(kOk);
    }
}

void cFileBrowserStatus::SetFilter()
{
    filter_ = cl_unknown;
    if(caller_ == xine)
    {
	filter_ = cl_video|cl_audio|cl_playlist;
    }  
    else if(startDir_ == PICTURES_DIR && caller_ == mediaplayer)
    {
        filter_ = cl_picture|cl_playlist;
    }
    else if(startDir_ == MUSIC_DIR && caller_ == mediaplayer)
    {
        filter_ = cl_audio|cl_playlist;
    }
    else if(startDir_ == VIDEO_DIR && caller_ == mediaplayer)
    {
        filter_ = cl_video|cl_playlist;
    }
    else if(startDir_ == VIDEO_DIR && caller_ == mediaplayer)
    {
        filter_ = cl_video|cl_playlist;
    }
}    

std::string cFileBrowserStatus::GetDefStartDir(int filter)
{
    //printf("------cFileBrowserStatus::GetDefStartDir, filter = %d-----\n", filter);
    if(filter == (cl_picture|cl_playlist))
    {
        return PICTURES_DIR;
    }
    else if(filter == (cl_audio|cl_playlist))
    { 
        return MUSIC_DIR;
    }
    else if(filter == (cl_video|cl_playlist))
    {
        return VIDEO_DIR;
    }
    else if(filter == (cl_video|cl_audio|cl_playlist))
    {
        return BASE_DIR;//VIDEO_DIR;
    }
    else
    {
        return BASE_DIR;
    }
}

//------------------------------------------------
//----------cFileBrowserInstances-----------------
//------------------------------------------------

cFileBrowserInstances::cFileBrowserInstances()
: id_(0)
{
}  

void cFileBrowserInstances::Add(cFileBrowserStatus &status)
{
    //printf("--cFileBrowserInstances::Add, instances_.size() = %d--\n", instances_.size());
    status.id_ = id_;
    instances_[id_] = status;
    ++id_;
}

void cFileBrowserInstances::Del(cFileBrowserStatus &status)
{
    //printf("--cFileBrowserInstances::Del, instances_.size() = %d--\n", instances_.size());
    instances_.erase(status.id_);
} 

void cFileBrowserInstances::Del(int id)
{
    //printf("--cFileBrowserInstances::Del, instances_.size() = %d--\n", instances_.size());
    instances_.erase(id);
}  

void cFileBrowserInstances::DeleteInvalid()
{
    for(std::map<int, cFileBrowserStatus>::iterator i = instances_.begin(); i != instances_.end(); )
    {
        if(i->second.valid_ == false)
        {
            instances_.erase(i++);
        }
        else
        {
            i++;
        }
    }
}

bool cFileBrowserInstances::Find(int id)
{
    return(instances_.find(id) != instances_.end());
}

int cFileBrowserInstances::GetLastFilter(uint filter)
{  
    for(std::map<int, cFileBrowserStatus>::const_iterator i = instances_.begin(); i != instances_.end(); ++i)
    {
        if(i->second.filter_ == filter)
        {
            return i->first;
        }
    }
    return -1;
}    

int cFileBrowserInstances::GetLastMusicInst()
{
    return GetLastFilter(cl_audio | cl_playlist);
}
    
int cFileBrowserInstances::GetLastImagesInst()
{
    return GetLastFilter(cl_picture | cl_playlist);
}
    
int cFileBrowserInstances::GetLastMoviesInst()
{
    return GetLastFilter(cl_video | cl_playlist);
}

cFileBrowserStatus &cFileBrowserInstances::Last()
{
    return((--instances_.end())->second);
}

cFileBrowserStatus &cFileBrowserInstances::Get(int id)
{
    return instances_[id];
} 

std::vector<int> cFileBrowserInstances::GetInstanceVec()
{
    std::vector<int> vec;
    for(std::map<int, cFileBrowserStatus>::const_iterator i = instances_.begin(); i != instances_.end(); ++i)
    {
        vec.push_back(i->first);
    }
    return vec;
}
