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


#ifndef P__BROWSERSTATUS_H
#define P__BROWSERSTATUS_H

#include <string>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#include <vdr/status.h>

#include "fileType.h"
#include "servicestructs.h"
#include "userIfBase.h"
#include "playlist.h"


//----------cFileBrowserStatus-------------------- 

class cFileBrowserStatus: public cStatus
{
    friend class cFileBrowserInstances;
public:
    cFileBrowserStatus();
    ~cFileBrowserStatus(){};
    // /*override*/ void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);

    //only called from service if/plugin
    void Init();

    void Reset();

    void Merge(cFileBrowserStatus &newstatus);
    //set the start dir and file
    void SetStartDir(std::string dir, std::string file = "");   
    //set an array of start dirs to apper on top level (extrenal mode)
    void SetStartDirs(const std::vector<std::string> &startdirs);       
    //Filebrowser will be called by caller;
    void SetCaller(browserCaller caller);
    //Service call "filebrowser xine control" has been received 
    void XineControl(xineControl cmd);   
    //Service call "Filebrowser external action" has been received 
    void SetExternalInfo(FileBrowserExternalActionInfo *info);
    //Service call "Filebrowser set playlist" has been received 
    //void SetPlayList(std::string path); 
    //Service call "Filebrowser set playlist entry" has been received 
    void SetPlayListEntry(std::string path);

    //are also called from filebrowser menus
    //browse mode has been set
    void SetMode(browseMode);
    //filebrowser has been shut down with current dir and file
    void Shutdown(std::string dir, std::string file,/* std::string playlist,*/ std::string playlistEntry);
    //main menu action has been called
    void Startup();
    //Playlist mode has been entered/exited
    void PlayListActive(bool On);
    //filebrowser has received kBack key
    void NormalExit();  
    //StillPictureMode has been entered/exited
    void SetStillPictureActive(bool On);

    bool PlayListIsActive() const;
    bool IsCalledFromSvdrp() const; 
    bool EnterPlayList() const;
    bool ClosePlayList() const;
    bool StillPictureActive() const;
    browseMode GetMode() const; 
    std::string GetExitDir() const;
    std::string GetStartDir() const;
    const std::vector<std::string>& GetStartDirs() const;
    std::string GetStartFile() const;
    browserCaller GetCaller() const; 
    int GetInstance() const;
    //std::string GetPlayList() const;
    std::string GetPlayListEntry() const;
    uint GetFilter() const;   
    cPlayList *GetPlayList();
    std::vector<eIftype> *GetIfHistory(); 

    std::string GetExternalTitleString() const;
    std::string GetExternalActionString() const;
    FileBrowserExternalActionInfo *GetExternalActionInfo();

    static std::string GetDefStartDir(int filter);

private:
    bool valid_;
    bool playListActive_;
    bool normalExit_;
    bool enterPlayList_; 
    bool closePlayList_;
    bool stillPictureActive_;
    bool calledFromMenu_;
    browseMode mode_;
    browserCaller caller_; 
    xineControl xineCtrl_;
    uint filter_;
    int id_;

    std::string exitDir_;
    std::string startDir_;
    std::vector<std::string> startDirs_;
    std::string startFile_;
    //std::string playList_;
    std::string playListEntry_;

    boost::shared_ptr<cPlayList> playlist_;
    std::vector<eIftype> ifTypes_;

    //for external mode  
    FileBrowserExternalActionInfo *extInfo_;

    void CallBackMenu();
    void SetFilter();
};

//inline
    /*inline void cFileBrowserStatus::SetPlayList(std::string path)
    {
        playList_ = path;
    }*/
    inline void cFileBrowserStatus::SetStartDirs(const std::vector<std::string> &startDirs)
    {
         startDirs_ = startDirs;
    }     
    inline void cFileBrowserStatus::SetPlayListEntry(std::string entry)
    {
        playListEntry_ = entry;
    }
    inline void cFileBrowserStatus::PlayListActive(bool On)
    {
        playListActive_ = On;
    }
    inline void cFileBrowserStatus::NormalExit()
    {
        normalExit_ = true;
    }    
    inline void cFileBrowserStatus::SetStillPictureActive(bool On)
    {
        stillPictureActive_ = On;
    }
    inline bool cFileBrowserStatus::PlayListIsActive() const
    {
        return playListActive_;
    }
    inline bool cFileBrowserStatus::EnterPlayList() const
    {
        return enterPlayList_; 
    }
    inline bool cFileBrowserStatus::ClosePlayList() const
    {
        return closePlayList_; 
    }  
    inline bool cFileBrowserStatus::StillPictureActive() const
    {
        return stillPictureActive_; 
    }
    inline browseMode cFileBrowserStatus::GetMode() const
    {
        return mode_;
    }
    inline std::string cFileBrowserStatus::GetExitDir() const
    {
        return exitDir_;
    }
    inline std::string cFileBrowserStatus::GetStartDir() const
    {
        return startDir_;
    }  
    inline const std::vector<std::string>& cFileBrowserStatus::GetStartDirs() const
    {
        return startDirs_;
    }
    inline std::string cFileBrowserStatus::GetStartFile() const
    {
        return startFile_;
    }
    inline browserCaller cFileBrowserStatus::GetCaller() const
    {
        return caller_;
    }   
    inline int cFileBrowserStatus::GetInstance() const
    {
        return id_;
    }
    inline std::string cFileBrowserStatus::GetPlayListEntry() const
    {
        return playListEntry_;
    }
    inline uint cFileBrowserStatus::GetFilter() const
    {
        return filter_;
    }
    inline cPlayList *cFileBrowserStatus::GetPlayList()
    {
        return playlist_.get();
    }   
    inline std::vector<eIftype> *cFileBrowserStatus::GetIfHistory() 
    {
        return &ifTypes_;
    }   
    inline std::string cFileBrowserStatus::GetExternalTitleString() const
    {
        return extInfo_->titleString;
    }
    inline std::string cFileBrowserStatus::GetExternalActionString() const
    {
        return extInfo_->actionString;
    }
    inline FileBrowserExternalActionInfo *cFileBrowserStatus::GetExternalActionInfo()
    {
        return extInfo_;
    }

//----------cFileBrowserInstances--------------------

class cFileBrowserInstances
{
    public:
    std::map<int, cFileBrowserStatus> instances_;
    void Add(cFileBrowserStatus &status);
    void Del(cFileBrowserStatus &status); 
    void Del(int id); 
    void DeleteInvalid();
    cFileBrowserStatus &Get(int id);
    cFileBrowserStatus &Last();
    bool Find(int id);
    int GetLastMusicInst(); 
    int GetLastImagesInst();
    int GetLastMoviesInst();
    int GetLastFilter(uint filter);
    std::vector<int> GetInstanceVec();

    private:
    int id_;
};

#endif //P__BROWSERSTATUS_H

