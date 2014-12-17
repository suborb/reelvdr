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

#include <vector>
#include <iostream>
#include <sstream>

#include <vdr/osd.h>
#include <vdr/remote.h>
#include "vdr/interface.h"
#include <vdr/debug.h>

#include "filebrowser.h"
#include "stringtools.h"
#include "stillpictureplayer.h"
#include "menufileinfo.h"
#include "convert.h"
#include "mountmanager.h"
#include "def.h"
#include "threadprovider.h"
#include "menuplaylist.h"
#include "userIf.h"
#include "filetools.h"
#include "menuBrowser.h"

//#include "i18n.h"

#include "../xinemediaplayer/services.h"

//---------listings of files and directories to be handled individually---------

// "directory", "old name", "new name", expert mode
const uint adjustableDirEntriesSize_expert = 2;
const char *const adjustableDirEntries_expert[][3] =
{
    {BASE_DIR, "hd", "Hard Drive"},
    {BASE_DIR, "dvd", "DVD"},
};


// "directory", "old name", "new name", restricted/autostart mode
const uint adjustableDirEntriesSize_restricted = 7;
const char *const adjustableDirEntries_restricted[][3] =
{
    {BASE_DIR, "hd",       trNOOP("Hard Drive")},    
    {BASE_DIR, "dvd",      trNOOP("DVD")},
    {MEDIA_DIR, "music",      trNOOP("My Music")},
    {MEDIA_DIR, "pictures",   trNOOP("My Pictures")},
    {MEDIA_DIR, "video",      trNOOP("My Videos")},
    {MEDIA_DIR, "recordings", trNOOP("My Recordings")},
    {VIDEO_DIR, "dvd",     trNOOP("My DVDs")}
};

// "directory", "old name", "new name", restricted mode
const uint adjustableDirEntriesSize_numbered = 1;
const char *const adjustableDirEntries_numbered[][3] =
{
     {BASE_DIR, "hd",       trNOOP("Hard Drive")}
}; 

// "base directory", "directory name", "pretitle for entries in this directory", restricted/autostart mode
const uint preTitleEntriesSize = 5;
const char *const preTitleEntries[][3] =
{
    {MEDIA_DIR, "music",      trNOOP("My Music")},
    {MEDIA_DIR, "pictures",   trNOOP("My Pictures")},
    {VIDEO_DIR, "dvd",     trNOOP("My DVDs")},
    {MEDIA_DIR, "video",      trNOOP("My Videos")},
    {MEDIA_DIR, "recordings", trNOOP("My Recordings")}
};

// "base directory", "directory name", "entry", alternative pretitle, restricted/autostart mode
const uint preTitleExceptionsSize = 1;
const char *const preTitleExceptions[][4] =
{
    {MEDIA_DIR, "video",  "dvd",  ""}
};

// "directory" - dirs in which hidden entries are not displayed
const uint hideHiddenEntryDirsSize= 1;
const char *const hideHiddenEntryDirs[] =
{
    BASE_DIR
};

// "directory", "name" - empty "directory" means in all directories
const uint forbiddenDirEntriesSize_expert = 2;
const char *const forbiddenDirEntries_expert[][2] =
{
    {"", "."},
    {"", "protection.fsk"},
};

// "directory", "name" - empty "directory" means in all directories
const uint forbiddenDirEntriesSize_restricted = 7;
const char *const forbiddenDirEntries_restricted[][2] =
{
    {"", "."},
    {"", "protection.fsk"},
    {MEDIA_DIR, "Channels.conf"},
    {MEDIA_DIR, "Channels.conf.tmp"},
    {MEDIA_DIR, "epg.data"},
    {MEDIA_DIR, "sys.log"},
    {MEDIA_DIR, "home"}
};

// "directory", "name", restricted/autostart mode - only these entries will appear in the specified dir
const uint allowedDirEntriesSize = 5;
const char *const allowedDirEntries[][2] =
{
    {MEDIA_DIR, ".."},
    {MEDIA_DIR, "music"},
    {MEDIA_DIR, "pictures"},
    {MEDIA_DIR, "video"},
    {MEDIA_DIR, "recordings"}
};

// "directory", "name" - entries will appear in same order
const uint positionedDirEntriesSize = 2;
const char *const positionedDirEntries[][2] =
{
    {BASE_DIR, "hd"},
    {BASE_DIR, "dvd"}
};

//---------Utilities-------------------------------

bool sortDirEntry(const cMenuDirEntryItem *item1, const cMenuDirEntryItem *item2)
{
    return GetLast(item1->GetPath()) <  GetLast(item2->GetPath());
}

bool sortFileEntryBySize(const cMenuFileItem *item1, const cMenuFileItem *item2)
{
    return item1->GetFileSize() >  item2->GetFileSize();
}

//----------cMenuFileBrowser-----------------------

cThreadProvider cMenuFileBrowser::provider_;
cPlayList cMenuFileBrowser::actualPlaylist;

cMenuFileBrowser::cMenuFileBrowser(const std::string title, cFileBrowserStatus *newstatus, cFileCache *cache, cImageConvert *convert, cThumbConvert *thumbConvert)
: cMenuFileBrowserBase(title), currentdir_(newstatus->GetStartDir()), cache_(cache), convert_(convert), thumbConvert_(thumbConvert), 
playlist_(newstatus->GetPlayList()), tmpPlaylist_(xinePlayable)
{
    printf("#####cMenuFileBrowser: currentdir_ = %s mode = %d\n", currentdir_.c_str(), newstatus->GetMode());
    SetStandardCols();
    SetHasHotkeys();
    EnableSideNote(true);

    status = newstatus;
    status->Startup();
    
    cUserIfBase::SetHistory(status->GetIfHistory());

    if(status->GetMode() == browseplaylist2)
    {
	userIf_ = cMenuBrowserBrowsePlayListIf2::Instance();
	tmpPlaylist_ = *playlist_; //copyplaylist
	tmpPlaylist_.SetType(xinePlayable);
	
 //test	
	std::vector<std::string> playlistEntries= playlist_->GetPathList();
	printf("+++++++++++++++++++++++++ playlistEntries.size = %d\n", playlistEntries.size());
	printf("++++++playlist_->GetName() = = %s\n", playlist_->GetName().c_str()); 
	printf("+++++++++++++++++++++++++ tmpPlaylist_.Size() = %d\n", tmpPlaylist_.Size());
	for(uint i= 0; i< playlistEntries.size();++i)
	{  
	    printf("playlistEntries[%d] = %s\n", i, playlistEntries[i].c_str()); 
	}
	printf("#####playlist_->CurrentIsSet() = %d\n", playlist_->CurrentIsSet());
//test   
	userIf_->EnterState(this);      
    }      
    else if(status->GetMode() == browseplaylist)
    {
	userIf_ = cMenuBrowserBrowsePlayListIf::Instance();
	userIf_->EnterState(this);	
    }
    else if(status->GetMode() == autostart)
    {
        userIf_ = cMenuBrowserAutoStartIf::Instance();
        userIf_->EnterState(this);
    }
    else if(status->GetMode() == expert)
    {
        userIf_ = cMenuBrowserStandardIf::Instance();
        userIf_->EnterState(this);
    }
    else if(status->GetMode() == restricted)
    {  
        userIf_ = cMenuBrowserRestrictedIf::Instance();
        userIf_->EnterState(this);
    } 
    else if(status->GetMode() == external)
    { 
        userIf_ = cMenuBrowserExternalIf::Instance();
        userIf_->EnterState(this);
    }
    else if(status->GetMode() == selectplaylist)
    {
        userIf_ = cMenuBrowserOpenPlaylistIf::Instance();
        userIf_->EnterState(this);
    }
    else if(status->GetMode() == saveplaylistas)
    {
	userIf_ = cMenuBrowserSavePlaylistAsIf::Instance();
        userIf_->EnterState(this);
    }  
    else if(status->GetMode() == copyplaylist)
    {
	userIf_ = cMenuBrowserBrowsePlayListAndCopyIf::Instance();
        userIf_->EnterState(this);
    }

    if(!Refresh(status->GetStartFile(), true))
    {
        //start dir cannot be opened -> use BASE_DIR instead
        currentdir_ = BASE_DIR; 
        Refresh(itemId(), true);
    }       

    //save or discard playlist?
    if(status->ClosePlayList())
    {
        cMenuBrowserBaseIf *lastIf = userIf_;
        userIf_ = cMenuBrowserShortPlaylistRequestIf::Instance();
        userIf_->EnterState(this, lastIf, osContinue);
    }

    if(status->EnterPlayList()) //???
    {
        /*if(status->GetPlayList() != playlist_.GetName())
        {
            LoadPlayList(cPlayListItem(status->GetPlayList()));
        }*/
        if(!playlist_->CurrentIsSet() || playlist_->GetCurrent().GetPath() != status->GetPlayListEntry())
        {
            playlist_->SetCurrent(status->GetPlayListEntry(), true);
        }
        //cUserIfBase::RemoveLastIf();
        EnterPlayList();
    }
}

cMenuFileBrowser::~cMenuFileBrowser()
{
    Shutdown();
}

void cMenuFileBrowser::Shutdown()
{ 
    //make sure, conversion has finished
    if(convert_ && !status->StillPictureActive())
    {
        RemoveItemsFromConversionList(convert_);
    }   
    if(thumbConvert_)
    {
        RemoveItemsFromConversionList(thumbConvert_);
    }

    //shutdown submenus of browser type in reverse order
    ShutdownBrowserSubMenu();

    //clear playlist if not active
    if(!status->PlayListIsActive()) //???
    {
        //ClearPlayList();
    }

    ShowThumbnail(cPlayListItem(), true, false); 
    ShowID3TagInfo(cPlayListItem(), true, false);
    Skins.Flush(); /** TB: hack to avoid the "clear thumbnail"-problem */

    status->Shutdown(currentdir_, GetCurrentItem() ? GetCurrentItem()->GetPath() : "",
                   /* playlist_->GetName(),*/ playlist_->CurrentIsSet() ? playlist_->GetCurrent().GetPath() : "");
}

eOSState cMenuFileBrowser::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;
    if(Key != kInfo)
    { 
        state = userIf_->AnyKey(this, Key);
    }
    RefreshIfDirty();

    if(state == osUnknown && !GetBrowserSubMenu())
    {
        switch (Key)
        {
            case kOk:
                state = userIf_->OkKey(this);
                break;
            case kBack:
                state = userIf_->BackKey(this);
                break;
            case kRed:
                state = userIf_->RedKey(this);
                break;
            case kGreen:
                state = userIf_->GreenKey(this);
                break;
            case kYellow:
                state = userIf_->YellowKey(this);
                break;
            case kBlue:
                state = userIf_->BlueKey(this);
                break;
            case kInfo:
                state = userIf_->InfoKey(this);
                break;
            default:
                state = osContinue;
                break;
        }
    }
    return state;
}

eOSState cMenuFileBrowser::ShowFileWithImageViewer(cPlayListItem &item, bool asPlaylist)
{
    //printf("---cMenuFileBrowser::ShowFileWithImageViewer item = %s asPlaylist = %d---\n", item.GetPath().c_str(), asPlaylist);
    ClearMarks();
    if(asPlaylist)
    {
        actualPlaylist.Clear();
        playlist_->GetPartialListOfSimilarItems(item, actualPlaylist);
    }
    else
    {
        actualPlaylist.Clear();
        BuildActualPlayList(item,  actualPlaylist);
        actualPlaylist.SetCurrent(item, true);
    }

    status->SetStillPictureActive(true);
    cStillPictureControl *control = new cStillPictureControl(&actualPlaylist, cache_, convert_, status->GetInstance());
    
    cRemote::CallPlugin("some none existing plugin name"); //???
    printf("blocking callplugins() from other players\n");
    cControl::Launch(control);
    return osEnd;
    //return AddFileBrowserSubMenu(new cMenuShowFile(DESCRIPTION, &actualPlaylist_, cache_, convert_));
}

eOSState cMenuFileBrowser::ShowPlayListWithXine(bool passInstance, bool showImmediately)
{
     //printf("---------cMenuFileBrowser::ShowPlayListWithXine, playlist->GetName = %s--------\n", playlist_->GetName().c_str());
     if(playlist_->CurrentIsSet())
     {
        return ShowFileWithXine(playlist_->GetCurrent(), true, passInstance, showImmediately);
     }
     else if (!playlist_->IsEmpty())
     {  
	playlist_->SetCurrent(playlist_->GetNthItem(0));
        return ShowFileWithXine(playlist_->GetCurrent(), true, passInstance, showImmediately);
     }
     else if (playlist_->IsEmpty())
     {  
        return ShowFileWithXine(cPlayListItem(), true, passInstance);
     }	
     return osContinue;
}

eOSState cMenuFileBrowser::ShowFileWithXine(const cPlayListItem &item, bool asPlaylistItem, bool passInstance, bool showImmediately)
{
    printf("----cMenuFileBrowser::ShowFileWithXine, asPlaylistItem = %d, passInstance = %d, status->GetInstance() = %d--\n", asPlaylistItem, passInstance, status->GetInstance());

    std::vector<std::string> playlistEntries; //empty list
    if(asPlaylistItem)
    {
        cPlayList partialList;
        playlist_->GetPartialListOfXinePlayableItems(item, partialList);
        playlistEntries = partialList.GetPathList();
    }
    
    /*printf("playlistEntries.size = %d\n", playlistEntries.size()); 
    for(uint i= 0; i< playlistEntries.size();++i)
    {  
      printf("playlistEntries[%d] = %s\n", i, playlistEntries[i].c_str()); 
    }*/
    
    std::string tmp;
    if(!asPlaylistItem || showImmediately)
    {  
	tmp = item.GetPath().c_str();
    }
    /*if(strstr("://", tmp.c_str()))
    {
	tmp = std::string("file://") + item.GetPath().c_str();
    }*/

    printf("file = %s\n", tmp.c_str()); 

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(),
        passInstance ? status->GetInstance() : -1,
        asPlaylistItem,
        playlist_->GetName(),
        playlistEntries
    };
    
    printf("----cMenuFileBrowser::ShowFileWithXine, currentdir_ = %s\n", currentdir_.c_str());
    status->SetStartDir(currentdir_);
    printf("----cMenuFileBrowser::ShowFileWithXine, defStatus->GetStartDir() = %s\n", status->GetStartDir().c_str());
    //cRemote::CallPlugin("some none existing plugin name"); xine osd will not pop up!
    //printf("blocking callplugins() from other players\n");
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
    XineStarted(asPlaylistItem);
    return osEnd;
}

eOSState cMenuFileBrowser::ShowDvdWithXine(const cPlayListItem &item, bool asPlaylistItem)
{
    //printf("---------cMenuFileBrowser::ShowDvdFileWithXine, asPlaylist = %d--------\n", (int)asPlaylist);
    std::vector<std::string> playlistEntries; //empty list
    if(asPlaylistItem)
    {
        cPlayList partialList;
        playlist_->GetPartialListOfSimilarItems(item, partialList);
        playlistEntries = partialList.GetPathList();
    }
    std::string tmp = item.GetPath().c_str();
    if(!strstr("dvd://", tmp.c_str()))
    {
	tmp = std::string("dvd://") + item.GetPath().c_str();
    }

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(), 
        status->GetInstance(),
        asPlaylistItem,
        playlist_->GetName(),
        playlistEntries
    };
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);

    Xinemediaplayer_Set_Xine_Mode xinemode = {
        Xinemediaplayer_Mode_filebrowser
    };
    cPluginManager::CallAllServices("Set xine mode", &xinemode);
    XineStarted(asPlaylistItem);
    return osEnd;
}

struct CurThumb
{
  int x;
  int y;
  char path[255];
  bool blend;
  int slot;
  int w;
  int h;
};

eOSState cMenuFileBrowser::ShowThumbnail(cPlayListItem item, bool hide, bool currentChanched)const
{
    //printf("ShowThumbnail, currentChanched = %d, hide = %d\n", currentChanched, hide);
    static bool dirty = true;
    static bool cleared = false;

    if(hide && !cleared)
    {
        //printf("HideThumbnail\n");
        dirty = false;
        cleared = true;
        cPluginManager::CallAllServices("setThumb", NULL);
        cleared = false;
    }
    else if(!hide)
    {
        //printf("ShowThumbnail, currentChanched = %d\n", currentChanched);
        std::string thumb;
        if(currentChanched)
        {
            dirty = true;
        }
        if(dirty && cache_ && cache_->FetchFromCache(item.GetPath(), thumb, thumbConvert_->GetEnding()))
        {
            dirty = false;
            //tell skinreelng that a new thumb is available 
            struct CurThumb curThumb;
            curThumb.x = curThumb.y = 0;
            curThumb.h = curThumb.w = 0;
            curThumb.blend = false;
            curThumb.slot = 0;
            strncpy(curThumb.path, thumb.c_str(), 255);
            cPluginManager::CallAllServices("setThumb", (void*)&curThumb);
        }
        if(dirty && currentChanched)
        { 
            //TODO: hourglass
            thumbConvert_->InsertInConversionList(item.GetPath(), true);
            thumbConvert_->StartConversion();
            cPluginManager::CallAllServices("setThumb", NULL);
            cleared = false;
        }
    }
    return osContinue;
}

eOSState cMenuFileBrowser::OpenFileInfo(const cMenuFileItem &file)
{
    return AddFileBrowserSubMenu(new cMenuFileInfo(DESCRIPTION, file));
}

eOSState cMenuFileBrowser::EnterSubDir(std::string pathname)
{
    std::string lastdir = currentdir_;
    std::string lastcurrentItem = GetCurrentItemPath();
  
    if(GetLast(pathname) == "..")
    {
        ExitDir(true);
    }
    else if (!ChangeDir(pathname))
    {
        //go back to last directory in case of error
        ChangeDir(lastdir, lastcurrentItem);
    }
    return osContinue;
}

eOSState cMenuFileBrowser::LoadAndEnterPlayList(cPlayListItem item)
{
    LoadPlayList(item);
    return EnterPlayList();	
}

//rename some entries in root dir, inefficient for larger numbers of entries!!
std::string cMenuFileBrowser::GetAdjustedEntryName(std::string path) const
{
    std::string dir = RemoveName(path);
    std::string name = GetLast(path);

    if(true)//status->GetMode() == restricted || status->GetMode() == autostart || status->GetMode() == browseplaylist)
    {
        for (uint i = 0; i < adjustableDirEntriesSize_restricted; i++)
        {
            if(adjustableDirEntries_restricted[i][0] == dir
            && adjustableDirEntries_restricted[i][1] == name)
            {
                return tr(adjustableDirEntries_restricted[i][2]);
            }
        }
    }
    else if(false)//status->GetMode() == expert)
    {
        for (uint i = 0; i < adjustableDirEntriesSize_expert; i++)
        {
            if(adjustableDirEntries_expert[i][0] == dir
            && adjustableDirEntries_expert[i][1] == name)
            {
                return tr(adjustableDirEntries_expert[i][2]);
            }
        }
    }
    return GetAdjustedNumberedEntryName(path);
}

//rename some entries in root dir, inefficient for larger numbers of entries!!
std::string cMenuFileBrowser::GetAdjustedNumberedEntryName(std::string path) const
{
    std::string dir = RemoveName(path);
    std::string name = GetLast(path);
    char buf[256];

    if(true)//status->GetMode() == restricted || status->GetMode() == autostart || status->GetMode() == browseplaylist)
    {
        for (uint i = 0; i < adjustableDirEntriesSize_numbered; i++)
        {
            if(adjustableDirEntries_numbered[i][0] == dir)
            {
                for(int j = 0; j <= 20; j++)
                {
                    sprintf(buf, "%s%d", adjustableDirEntries_numbered[i][1], j);
                    if(name == std::string(buf))
                    {
                        sprintf(buf, "%s %d", tr(adjustableDirEntries_numbered[i][2]), j);
                        return std::string(buf);
                    }
                }
            }
        }
    }
    return (name);
}

eOSState cMenuFileBrowser::EnterPlayList(bool play)
{
    std::string title = "Playlist: " + RemoveEnding(GetLast(playlist_->GetName()));
    return AddFileBrowserSubMenu(new cMenuPlayList(title.c_str(), playlist_, cache_, convert_, play));
}

eOSState cMenuFileBrowser::OpenMarked(bool &playlistEntered)
{     
    //printf("##########cMenuFileBrowser::OpenMarked############\n");   
    int oldsize = playlist_->Size();
    playlist_->Insert(markedEntries_, single);
    if(playlist_->Size() > oldsize)
    { 
        //printf("---MenuFileBrowser::OpenMarked, playlist_->Size() = %d, oldsize = %d--\n", playlist_->Size(), oldsize);
        //jump to next entry of same type as current
        if (playlist_->CurrentIsSet())
        {
            cPlayListItem lastItem = playlist_->GetCurrent();
            playlist_->SetCurrent(playlist_->GetNthItem(oldsize));
            if(!playlist_->GetCurrent().GetFileType().IsOfSimilarType(lastItem.GetFileType()))
            {
                playlist_->NextItemOfSameClass(lastItem, true);
            }
        }
        //look for next playable entry or next playable entry of filter type
        else
        {
            playlist_->SetCurrent(playlist_->GetNthItem(oldsize));
            efileclass filter = (efileclass)(GetStatus()->GetFilter()); 
            if (filter == cl_unknown)
            {
                if(!playlist_->GetCurrent().IsPlayable())
                {
                    playlist_->NextPlayableItem(true);
                }
            }
            else
            {
                if(playlist_->GetCurrent().GetFileType().GetClass() != (filter & ~CL_PLAYLIST))
                {
                    playlist_->NextItemOfClass(filter, true);
                }
            }
        }            
        if(playlist_->GetCurrent().IsPlayable())
        {
            playlistEntered = true;
            return playlist_->GetCurrent().Open(this, true);
        }
    }
    else
    {
        //just go back to current entry
        if (!playlist_->CurrentIsSet() && playlist_->Size())
        {
            playlist_->SetCurrent(playlist_->GetNthItem(0));
            efileclass filter = (efileclass)(GetStatus()->GetFilter()); 
            if (filter == cl_unknown)
            {
                if(!playlist_->GetCurrent().IsPlayable())
                {
                    playlist_->NextPlayableItem(true);
                }
            }
            else
            {
                if(playlist_->GetCurrent().GetFileType().GetClass() != (filter & ~CL_PLAYLIST))
                {
                    playlist_->NextItemOfClass(filter, true);
                }
            }
        }

        if (playlist_->CurrentIsSet() && playlist_->GetCurrent().IsPlayable())
        {
            playlistEntered = true;
            return playlist_->GetCurrent().Open(this, true); 
        }
    }
    //do nothing when playlist remains empty
    return osContinue;
}

eOSState cMenuFileBrowser::ExitDir(bool fromItem)
{    
    /*if(status->GetMode() == browseplaylist)
    {
        if(currentdir_ == BASE_DIR)
        {
            return osContinue; //do not exit in browseplaylist mode
        }
    }*/

    if((currentdir_ == status->GetExitDir() && !fromItem) || currentdir_ == BASE_DIR) //never go beyond BASE_DIR!
    {
        status->NormalExit();
        return osBack;
    }
 
    if(mountMgr_.IsMounted(currentdir_))
    {
        if(convert_)
        {
            RemoveItemsFromConversionList(convert_);
            convert_->WaitUntilCopied();
        }   
        if(thumbConvert_)
        {
            RemoveItemsFromConversionList(thumbConvert_);
            thumbConvert_->WaitUntilCopied();
        }
        mountMgr_.Umount();
    }
    ChangeDir(RemoveName(currentdir_), currentdir_);
    return osContinue;
}

eOSState cMenuFileBrowser::StandardKeyHandling(eKeys Key)
{
    if((Key == kBack || Key == kInfo) && !HasSubMenu()) //these keys are handled separately
    {
        return osUnknown;
    }
    else
    {
        return cOsdMenu::ProcessKey(Key);
    }
}

int cMenuFileBrowser::GetFinishedTasks()
{
    cTask *task;
    int ret = 0;
    if(provider_.GetNextFinishedTask(&task))
    {
        ret = task->ReturnVal();
        delete task;
        Refresh();
    }
    return ret;
}

void cMenuFileBrowser::DeleteMarkedEntries()
{
    std::vector<std::string> entriesToDelete;
    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++)
    {
        if(convert_ && GetFileItem(*i))
        {
            convert_->RemoveFromConversionList((*i).path_);
            if (convert_->IsInConversion((*i).path_))
                convert_->WaitUntilCopied();
        }
        entriesToDelete.push_back((*i).path_);
    }
    provider_.InsertNewTask(new cDeleteTask(entriesToDelete, currentdir_));
}

void cMenuFileBrowser::DeleteMarkedOrCurrent()
{
    if(markedEntries_.empty())
    {
        cMenuDirEntryItem *item = GetCurrentDirEntryItem();
        if(item && GetLast(item->GetPath()) != "..")
        {
            if(convert_ && item->IsFile())
            {
                if (convert_->IsInConversion(item->GetPath()))
                    convert_->WaitUntilCopied();
            }
            std::vector<std::string> entriesToDelete;
            entriesToDelete.push_back(item->GetPath());
            provider_.InsertNewTask(new cDeleteTask(entriesToDelete, currentdir_));
        }
    }
    else
    {
        DeleteMarkedEntries();
    }
}

void cMenuFileBrowser::InsertCopiedEntries()
{
    Paste(false);
}

void cMenuFileBrowser::InsertMovedEntries()
{
    Paste(true);
}

void cMenuFileBrowser::CopyMarkedEntries()
{
    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++)
    {
        copiedEntries_.push_back((*i).path_);
    }
}

void cMenuFileBrowser::CopyMarkedOrCurrent()
{
    if(markedEntries_.empty())
    {
        cMenuDirEntryItem *item = GetCurrentDirEntryItem();
        if(item && GetLast(item->GetPath()) != "..")
        {
            copiedEntries_.push_back(item->GetPath());
        }
    }
    else
    {
        CopyMarkedEntries();
    }
}

bool cMenuFileBrowser::RenameEntry(std::string oldname, std::string newname)
{
    std::string cmd = std::string("mv ") + "\"" + currentdir_  + "/" + oldname + "\" " +  "\"" + currentdir_ + "/" + newname + "\"";
    //printf("----%s-----\n", cmd.c_str());
    return !::SystemExec(cmd.c_str());
}

bool cMenuFileBrowser::CreateDirectory(std::string name)
{
    std::string cmd = std::string("mkdir ") + "\"" + currentdir_ + "/" + name + "\"";
    //printf("----%s----\n", cmd.c_str());
    return !::SystemExec(cmd.c_str());
}

bool cMenuFileBrowser::ConvertToUTF8()
{
    int res = 0, nr = 0;
    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++) {
        nr++;
        std::string cmd = std::string("convert_to_utf8.sh -f -d ") + "\"" + (*i).path_ + "\"";
        res += !::SystemExec(cmd.c_str());
        GetItem(*i)->UnMark();
        //printf("----%s----\n", cmd.c_str());
    }
    Display();
    markedEntries_.clear();
    return (res == nr);
}

bool cMenuFileBrowser::BurnIso()
{
    int res = 0, nr = 0;
    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++) {
        nr++;
        std::string cmd = std::string("growisofs -dvd-compat -Z /dev/dvd=") + "\"" + (*i).path_ + "\"";
        res += !::SystemExec(cmd.c_str());
        GetItem(*i)->UnMark();
        printf("cMenuFileBrowser::BurnIso  ----%s----\n", cmd.c_str());
    }
    Display();
    markedEntries_.clear();
    return (res == nr);
}

bool cMenuFileBrowser::MakeAndBurnIso()
{
    FileBrowserGetEntries entries;

    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++)
    {
        entries.entries.push_back((*i).path_);
    }
    //cRemote::CallPlugin("some none existing plugin name"); xine osd will not pop up!
    //printf("blocking callplugins() from other players\n");
    //printf("cPluginManager::CallAllServices: \n");
    cPluginManager::CallAllServices("reelburn get entries", &entries);
    return true;
}

bool cMenuFileBrowser::InsertCurrentInPlayList()
{
    cMenuDirEntryItem *item = GetCurrentDirEntryItem();
    if(item && GetLast(item->GetPath()) != "..")
    {
        return playlist_->Insert(item);
    }
    return playlist_->Insert(markedEntries_);
}

bool cMenuFileBrowser::InsertMarkedEntriesInPlayList()
{
    return playlist_->Insert(markedEntries_);
}

bool cMenuFileBrowser::InsertMarkedOrCurrentInPlayList()
{
    if(markedEntries_.empty())
    {
        return InsertCurrentInPlayList();
    }
    else
    {
        return InsertMarkedEntriesInPlayList();
    }
    return true;
}

bool cMenuFileBrowser::HasUnfinishedTasks() const
{
    return provider_.HasTasks();
}

bool cMenuFileBrowser::Refresh(itemId newItemId, bool initial)
{ 
    //printf("Refresh\n");
    //DDD("\n----cMenuFileBrowser::Refresh\n");
    std::string newCurrentPath = newItemId.path_;
    std::string currentPath = GetCurrentItemPath();

    if(initial && status->StillPictureActive())
    {
        //printf("--cMenuFileBrowser::Refresh, actualPlaylist.CurrentIsSet() = %d--\n", actualPlaylist.CurrentIsSet());
        currentPath = actualPlaylist.CurrentIsSet() ? actualPlaylist.GetCurrent().GetPath() : "";
        status->SetStillPictureActive(false);
    }
    else if(newCurrentPath != "")
    {
        currentPath = newCurrentPath;
    }

    SetTitle(GetTitleString().c_str());
    ClearMarks();
    ClearFlags();
    ClearTags();
    Clear();

    //create menu entries
    //DDD("----vor OpenDir");
    bool success = OpenDir();

    //set current entry
    if(currentPath != "")
    {
        for (int i= 0; i < Count(); i++)
        {
            if(GetItem(i)->GetPath() == currentPath)
            {
                SetCurrent(Get(i));
                UpdateCurrent(true);
                break;
            }
        }
    }

    //DDD("----vor InsertItemsInConversionList");
    if(success)
    {
        //start conversion for convertable items
        if(convert_)
        {
            InsertItemsInConversionList(convert_);
            convert_->StartConversion();
        }
        if(thumbConvert_)
        {
            InsertItemsInConversionList(thumbConvert_);
            thumbConvert_->StartConversion();
        }
        MarkFlagTagItems();
    }  

    //last current becomes invalid
    lastCurrent_ = -1;

    //refresh user if 
    userIf_->SetMode(this, status->GetMode(), initial);
    userIf_->SetHelp(this);
    Display();

    //DDD("----cMenuFileBrowser::Refresh: finished\n");
    return success;
}

void cMenuFileBrowser::Hide(bool hide)
{
    //printf("Hide\n");
    static int cur = 0;
    if(!hide)
    {
        hidden = false;
        Clear();
        OpenDir();
        ReMarkItems();
        //FlagItems();
        SetCurrent(Get(cur));
        UpdateCurrent(true);
        userIf_->SetHelp(this); 
    }
    else
    {
        hidden = true;
        cur = Current();
        Clear();
	ShowThumbnail(cPlayListItem(),true,false);
        ShowID3TagInfo(cPlayListItem(),true,false);
	Skins.Flush();
    }
    Display();
}

void cMenuFileBrowser::SetStandardCols()
{
    SetCols(2, 2, 2);
}  
 
bool cMenuFileBrowser::OpenDir()
{
    //printf("\n----cMenuFileBrowser::OpenDir() currentdir_ = %s\n", currentdir_.c_str());
    //DDD("----currentdir_ = %s\n", currentdir_.c_str());
    if (!mountMgr_.Mount(currentdir_))
    {
        Skins.Message(mtError, tr("Cannot mount media"));
        return false;
    }

    //use BASEDIR if currentdir_ is empty
    std::string dir = currentdir_;
    if (dir == "")
    {
        dir = BASE_DIR;
    }

    std::vector<Filetools::dirEntryInfo>  dirs;
    std::vector<Filetools::dirEntryInfo>  files;
    std::vector<Filetools::dirEntryInfo>  dirLinks;
    std::vector<Filetools::dirEntryInfo>  fileLinks;
    
    //search for entries in directory 
    //DDD("----vor GetDirEntries");
    if(!Filetools::GetDirEntries(dir, dirs, files, dirLinks, fileLinks) || !Filetools::IsExecutableDir(dir))
    {
        printf("couldn't open %s", dir.c_str());
        Skins.Message(mtError, tr("Access denied!"));
        return false;
    }  

    std::vector<cMenuDirItem*> dirEntries;
    std::vector<cMenuFileItem*> fileEntries;
    
    //create the corresponding menu entries
    for(uint i = 0; i < dirs.size(); i++)
    {
        //DDD("----dirs[%d].name_ = %s", i, dirs[i].name_.c_str());
        dirEntries.push_back(new cMenuDirItem(dirs[i].name_, this));
    }
    for(uint i = 0; i < dirLinks.size(); i++)
    {
        //DDD("----dirLinks[%d].name_ = %s", i, dirLinks[i].name_.c_str());
        dirEntries.push_back(new cMenuDirLinkItem(dirLinks[i].name_, this));
    }
    for(uint i = 0; i < files.size(); i++)
    {
        //DDD("----files[%d].name_ = %s", i, files[i].name_.c_str());
	//printf("----files[%d].name_ = %s\n", i, files[i].name_.c_str());
        fileEntries.push_back(new cMenuFileItem(files[i].name_, this));
    }
    for(uint i = 0; i < fileLinks.size(); i++)
    {
        //DDD("----fileLinks[%d].name_ = %s", i, fileLinks[i].name_.c_str());
        fileEntries.push_back(new cMenuFileLinkItem(fileLinks[i].name_, this));
    }

    //Sort menu entries alphabetically
    //DDD("----cMenuFileBrowser::OpenDir() vor Sort entries");
    std::stable_sort(dirEntries.begin(), dirEntries.end(), sortDirEntry);
    if(status->GetSortMode() == byalph)
    {  
	std::stable_sort(fileEntries.begin(), fileEntries.end(), sortDirEntry);
    }
    else if(status->GetSortMode() == bysize)
    {  
	std::stable_sort(fileEntries.begin(), fileEntries.end(), sortFileEntryBySize);
    }	

    //Remove "forbidden" entries, resort
    //DDD("----cMenuFileBrowser::OpenDir() vor Adjust entries");
    AdjustEntries(dirEntries, fileEntries);

    //Add menu entries
    //DDD("----cMenuFileBrowser::OpenDir() vor Add entries");
    
    //Add Additional Entries from user if
    userIf_->AddExtraItems(this);
     
    uint i = 0;
    if(currentdir_ != status->GetExitDir() && currentdir_ != BASE_DIR)
    {
        for (i = 0; i < dirEntries.size(); ++i)
        {
            if(GetLast(dirEntries[i]->GetPath()) == "..")
            {
                Add(dirEntries[i]);
                dirEntries[i]->Set();
                break;
            }
        }
        Add(new cMenuEmptyItem(this));  //empty line
    }
    for (i  = 0; i < dirEntries.size(); ++i)
    {
        if(GetLast(dirEntries[i]->GetPath()) != "..")
        {
            Add(dirEntries[i]);
            dirEntries[i]->Set();
        }
    }
    for (i = 0; i < fileEntries.size(); ++i)
    {
        Add(fileEntries[i]);
        fileEntries[i]->Set();
    }

    //DDD("----cMenuFileBrowser::OpenDir():finished\n");
    return true;
}

bool cMenuFileBrowser::ChangeDir(std::string pathname, std::string currentItem)
{
    currentdir_ = pathname;
    if(convert_)
    {
        RemoveItemsFromConversionList(convert_);
    }   
    if(thumbConvert_)
    {
        RemoveItemsFromConversionList(thumbConvert_);
    }
    return Refresh(currentItem);
}

void cMenuFileBrowser::ClearPlayList()
{
    //printf("----------cMenuFileBrowser::ClearPlaylist--------\n");
    playlist_->Clear(status->GetExitDir());
}

void cMenuFileBrowser::SavePlayList(std::string path, bool clear)
{
    //printf("----------cMenuFileBrowser::SavePlaylist, path = %s--------\n", path.c_str());
    playlist_->SaveToFile(path);
    if(clear)
    {  
	ClearPlayList();
    }	
}

void cMenuFileBrowser::CopyPlayListToUsb() const
{
    //printf("----------cMenuFileBrowser::CopyPlaylistToUsb--------\n");
    //TODO: determine actual mount point for usb device!!!
    std::string usbdir = "/media/USB_DISK_2.0";
    std::string listdir = usbdir + "/" + RemoveEnding(GetLast(playlist_->GetName()));
    std::string cmd = std::string("mkdir ") + listdir;
    ::SystemExec(cmd.c_str());
    provider_.InsertNewTask(new cCopyTask(playlist_->GetPathList(), listdir));
}

void cMenuFileBrowser::CopyPlayListToDvd() const
{
    playlist_->SaveToFile(); //?????????????
    std::string cmd =  std::string("sudo mp3_2_media.sh ") + playlist_->GetName();
    //printf("-----CopyPlaylistToDvd %s--------\n", cmd.c_str());
    ::SystemExec(cmd.c_str());
}

bool cMenuFileBrowser::PlayListIsEmpty() const
{
    return playlist_->IsEmpty();
}

void cMenuFileBrowser::CopyPlayListToDirectory(std:: string dir) const
{
    if(dir == "")
    {
	dir = currentdir_;
    }  
    provider_.InsertNewTask(new cCopyTask(playlist_->GetPathList(), dir));
}

void cMenuFileBrowser::AdjustPlaylistToTmpPlaylist()
{
   /*printf("--------cMenuFileBrowser::AdjustPlaylistToTmpPlaylist(), tmpPlaylist_.size = %d----\n", tmpPlaylist_.Size());
   std::vector<std::string> pathList = tmpPlaylist_.GetPathList();
   
   for(uint i = 0; i<pathList.size(); i++)
   {  
      printf("-----tmpPlaylist_[%d] = %s\n", i, pathList[i].c_str()); 
   } */
  
   *playlist_ = tmpPlaylist_;
   //playlist_->MergePlayListEntries(tmpPlaylist_.GetPathList());  
} 

void cMenuFileBrowser::InsertCurrentInTmpPlaylist(bool setcurrent)
{   
    //printf("-----InsertCurrentInTmpPlaylist-------\n");
    cMenuDirEntryItem *item = GetCurrentDirEntryItem();
    if(item->IsFile() || (item->IsDir() && !(item->GetPath() == "..")))
    {  
	tmpPlaylist_.Insert(item, single);
    }	
    if(item->IsFile() && setcurrent)
    {
	tmpPlaylist_.SetCurrent(GetCurrentFileItem()->GetPlayListItem());
    }  
}  

void cMenuFileBrowser::RemoveCurrentFromTmpPlaylist()
{
    cMenuFileItem *item = GetCurrentFileItem();
    if(item)
    { 
	tmpPlaylist_.Remove(item->GetPlayListItem());
    }	  
}  

bool cMenuFileBrowser::IsInPlaylist(cMenuFileItem *item) const
{
    return playlist_->IsInList(item->GetPlayListItem());
}  
    
bool cMenuFileBrowser::IsInTmpPlaylist(cMenuFileItem *item) const
{
    return tmpPlaylist_.IsInList(item->GetPlayListItem());
}  
    
void cMenuFileBrowser::AddAllMarkedEntriesToTmpPlaylist()
{  
    tmpPlaylist_.Insert(markedEntries_, single);
}

void cMenuFileBrowser::AddAllFlaggedEntriesToTmpPlaylist()
{  
    tmpPlaylist_.Insert(flaggedEntries_, single);
}

void cMenuFileBrowser::AddAllTaggedEntriesToTmpPlaylist()
{  
    tmpPlaylist_.Insert(taggedEntries_, single);
}

void cMenuFileBrowser::RemoveAllEntriesFromTmpPlaylist()
{  
    for (int i = 0; i < Count(); i++)
    {
        cMenuFileItem *item = GetFileItem(i);
        if(item)
        {
	    tmpPlaylist_.Remove(item->GetPlayListItem());
        }
    } 
}  

eOSState cMenuFileBrowser::InstallDebPacket(const std::string &file)
{
    cMenuBrowserBaseIf *lastIf = userIf_;
    userIf_ = cMenuBrowserInstallPacketIf::Instance();
    userIf_->EnterState(this, lastIf, osContinue);
  
    provider_.InsertNewTask(new cInstallDebPackTask(file));
    
    return osContinue;
}  

std::string cMenuFileBrowser::GetTitleString() const
{
    std::string titlestring;
    if(status->GetMode() == restricted)
    {
        titlestring = GetPreTitle();
        titlestring += GetLastN(GetAdjustedEntryName(currentdir_), 20);
    }
    else if(status->GetMode() == expert || status->GetMode() == autostart)
    {
        titlestring = tr(DESCRIPTION) + std::string(": ");
        titlestring += GetLastN(currentdir_, 20);
    }
    else if(status->GetMode() == browseplaylist)
    {
        titlestring = std::string(tr("Insert in playlist")) + ": ";
        titlestring += GetLastN(currentdir_, 20);
    }
    else if(status->GetMode() == browseplaylist2)
    {
        titlestring = std::string(tr("Insert in playlist")) + ": ";
        titlestring += GetLastN(currentdir_, 20);
    }
    else if(status->GetMode() == external)
    {
        titlestring = GetLastN(currentdir_, 20) + ": ";
        titlestring += status->GetExternalTitleString();
    }
    else if(status->GetMode() == selectplaylist)
    {
        titlestring = std::string(tr("Open playlist")) + ": ";
        titlestring += GetLastN(currentdir_, 20);
    }
    else if(status->GetMode() == copyplaylist)
    {
        titlestring = std::string(tr("Copy playlist into")) + ": ";
        titlestring += GetLastN(currentdir_, 20);
    }
    else if(status->GetMode() == saveplaylistas)
    {
        titlestring = std::string(tr("Save playlist as")) + ": ";
        titlestring += GetLastN(currentdir_, 20);
    }
    return titlestring;
}  

std::string cMenuFileBrowser::GetExternalActionString() const
{
    return status->GetExternalActionString();
}

FileBrowserExternalActionInfo *cMenuFileBrowser::GetExternalActionInfo() const
{
    return status->GetExternalActionInfo();
}

std::string cMenuFileBrowser::GetPreTitle() const
{
    std::string pretitle = "";
    std::string entry = cFileBrowserStatus::GetDefStartDir(status->GetFilter());

    for (uint i = 0; i < preTitleEntriesSize; i++)
    {
         if (entry == std::string(preTitleEntries[i][0]) + "/" + preTitleEntries[i][1])
         {
              pretitle = std::string(tr(preTitleEntries[i][2])) + ": ";
              break;
         }
    }
    for (uint i = 0; i < preTitleExceptionsSize; i++)
    {
         if (entry == std::string(preTitleExceptions[i][0]) + "/" + preTitleExceptions[i][1] + "/" + preTitleExceptions[i][2])
         {
              pretitle = preTitleExceptions[i][3]; 
              break;
         }
    }
    return pretitle;
}

void cMenuFileBrowser::Paste(bool remove)
{
    if(!remove)
    {
         provider_.InsertNewTask(new cCopyTask(copiedEntries_, currentdir_));
    }
    else
    {
        std::vector<std::string> entriesToCopy, entriesToMove;
        for (uint i= 0; i< copiedEntries_.size(); i++)
        {
            if(convert_)
            {
                convert_->RemoveFromConversionList(copiedEntries_[i]);
                if(convert_->IsInConversion(copiedEntries_[i]))
                    convert_->WaitUntilCopied();
            }
            mountMgr_.Mount(copiedEntries_[i]);
            if(Filetools::IsOnSameFileSystem(copiedEntries_[i], currentdir_))
            { 
                //printf("-----entriesToMove.push_back-\n");
                entriesToMove.push_back(copiedEntries_[i]);
            }
            else
            { 
                //printf("-----entriesToCopy.push_back-\n");
                entriesToCopy.push_back(copiedEntries_[i]);
            } 
            mountMgr_.Umount();
        }
        if(!entriesToMove.empty())
        {
            provider_.InsertNewTask(new cMoveTask(entriesToMove, currentdir_));
        }
        if(! entriesToCopy.empty())
        {
            provider_.InsertNewTask(new cCopyTask(entriesToCopy, currentdir_, true));
        }
    }
    copiedEntries_.clear();
}

void cMenuFileBrowser::InsertItemsInConversionList(cImageConvert *convert) const
{
    cMenuFileItem *fileitem;
    std::set<std::string> convItems;
    for (int i = 0; i < Count(); i++)
    {
        fileitem = GetFileItem(i);
        if(fileitem && fileitem->GetFileType().IsOfSimilarType(tp_jpg))
        {
            if (!cache_->InCache(fileitem->GetPath()))
            {
                convItems.insert(fileitem->GetPath());
            }
        }
    }
    convert->InsertInConversionList(convItems);
}

void cMenuFileBrowser::RemoveItemsFromConversionList(cImageConvert *convert) const
{
    cMenuFileItem *fileitem;
    for (int i= 0; i < Count(); i++)
    {
        fileitem = GetFileItem(i);
        if(fileitem && fileitem->GetFileType().IsOfSimilarType(tp_jpg))
        {
            convert->RemoveFromConversionList(fileitem->GetPath());
        }
    }
}

void cMenuFileBrowser::LoadPlayList(cPlayListItem item)
{
    playlist_->LoadFromFile(item);
}

eOSState cMenuFileBrowser::LoadAndReplayPlayList(cPlayListItem item)
{
    LoadPlayList(item);
    return ShowPlayListWithXine(false);
}

void cMenuFileBrowser::AppendCurrentToPlaylist()
{
    if(!GetCurrentFileItem() || 
	GetCurrentFileItem()->GetPlayListItem().GetFileType().GetType() != tp_playlist)
    {  
	return;
    }
    
    cPlayList tmpPlaylist;
    tmpPlaylist.LoadFromFile(GetCurrentFileItem()->GetPlayListItem());
    playlist_->Append(tmpPlaylist);
}  

sortMode cMenuFileBrowser::GetSortMode() const
{
    return status->GetSortMode();
}  

void cMenuFileBrowser::ChangeSortMode(sortMode mode)
{
    if(mode != status->GetSortMode())
    {  
	status->SetSortMode(mode);
	Refresh();
    }
}  

void cMenuFileBrowser::MarkFlagTagItems()
{
    printf("MarkFlagTagItems(), status->GetMode() = %d, playlist_->Size() = %d, tmpPlaylist_.size() = %d\n", status->GetMode(), playlist_->Size(), tmpPlaylist_.Size());    
    if(status->GetMode() != browseplaylist2)
    {
	return;
    }  
    
    cMenuFileItem *fileitem;
    for (int i = 0; i < Count(); i++)
    {
        fileitem = GetFileItem(i);
        if(fileitem)
        {
	    printf(":%s \n", fileitem->GetPath().c_str());
	    if(playlist_->IsInList(fileitem->GetPlayListItem()))
	    {
		printf("IsIn playlist_:%s \n", fileitem->GetPath().c_str());
	    }
	    if(tmpPlaylist_.IsInList(fileitem->GetPlayListItem()))
	    {
		printf("IsIn tmplaylist_:%s \n", fileitem->GetPath().c_str());
	    } 
            if(playlist_->IsInList(fileitem->GetPlayListItem()) && tmpPlaylist_.IsInList(fileitem->GetPlayListItem()))
            {
                FlagItem(fileitem, false);
            }
            else if(playlist_->IsInList(fileitem->GetPlayListItem()) && !tmpPlaylist_.IsInList(fileitem->GetPlayListItem()))
            {
                TagItem(fileitem, false);
            }
            else if(!playlist_->IsInList(fileitem->GetPlayListItem()) && tmpPlaylist_.IsInList(fileitem->GetPlayListItem()))
            {
                MarkItem(fileitem, false);
            }
        }
    }
     printf("MarkFlagTagItems(), vor return\n");
}

bool cMenuFileBrowser::IsForbiddenDirEntry(std::string entry) const
{
    std::string dir = RemoveName(entry);
    std::string name = GetLast(entry);

    if(status->GetMode() == expert || status->GetMode() == browseplaylist || status->GetMode() == browseplaylist2)
    {
        for (uint i = 0; i < forbiddenDirEntriesSize_expert; i++)
        {
            if (std::string(forbiddenDirEntries_expert[i][1]) == name && (std::string(forbiddenDirEntries_expert[i][0]) == dir || std::string(forbiddenDirEntries_expert[i][0]) == std::string("")))
            {
                return true;
            }
        }
        return false;
    }
    else
    {
        for (uint i = 0; i < forbiddenDirEntriesSize_restricted; i++)
        {
            if (std::string(forbiddenDirEntries_restricted[i][1]) == name && (std::string(forbiddenDirEntries_restricted[i][0]) == dir || std::string(forbiddenDirEntries_restricted[i][0]) == std::string("")))
            {
                return true;
            }
        }
        return false;
    }
}

bool cMenuFileBrowser::IsAllowedDirEntry(std::string entry) const
{
    if(status->GetMode() == expert || status->GetMode() == browseplaylist || status->GetMode() == browseplaylist2)
    {
        return true;
    }
    std::string dir = RemoveName(entry);
    std::string name = GetLast(entry);
    bool foundDir = false;
    for (uint i = 0; i < allowedDirEntriesSize; i++)
    {
       if(std::string(allowedDirEntries[i][0]) == dir || std::string(allowedDirEntries[i][0]) == std::string(""))
       {
           foundDir = true;
       }
    }
    if(!foundDir)
    {
        return true;
    }
    
    for (uint i = 0; i < allowedDirEntriesSize; i++)
    {
         if (std::string(allowedDirEntries[i][1]) == name && (std::string(allowedDirEntries[i][0]) == dir || std::string(allowedDirEntries[i][0]) == std::string("")))
         {
              return true;
         }
    }
    return false;
}

bool cMenuFileBrowser::IsHiddenDirEntry(std::string entry) const
{
    std::string dir = RemoveName(entry);
    std::string name = GetLast(entry);
    for (uint i = 0; i < hideHiddenEntryDirsSize; i++)
    {
        if(dir == hideHiddenEntryDirs[i])
        {
            if(StartsWith(name, ".") && name != "..")
            {
                return true;
            }
        }
    }
    //TODO: limit to resume files, not all hidden entries
    if(StartsWith(name, ".") && name != "..") // remove .files
    {
	    return true;
    }
    return false;
}

bool cMenuFileBrowser::IsInsideFilter(cMenuFileItem &item) const
{
    uint filter = status->GetFilter();
    if(filter & cl_unknown)
    {
        return true;
    }
    if(filter & item.GetFileType().GetClass())
    {
        return true;
    }
    return false;
}

//reposition entries in browser, inefficient!!!
void cMenuFileBrowser::RepositionDirEntries(std::vector<cMenuDirItem*> &direntries)
{
    std::vector<cMenuDirItem*> temp;

    for (uint i = 0; i < positionedDirEntriesSize; ++i)
    {
        for (std::vector<cMenuDirItem*>::iterator itd = direntries.begin(); itd < direntries.end(); )
        {
            std::string dir = RemoveName((*itd)->GetPath());
            std::string name = GetLast((*itd)->GetPath());
            if(positionedDirEntries[i][0] == dir && positionedDirEntries[i][1] == name)
            {
                temp.push_back(*itd);
                itd = direntries.erase(itd);
            }
            else
            {
                ++itd;
            }
        }
    }

    for (std::vector<cMenuDirItem*>::iterator itd = direntries.begin(); itd < direntries.end(); ++itd)
    {
        temp.push_back(*itd);
    }

    direntries = temp;
}

//adjust entry display, inefficient!!!
void cMenuFileBrowser::AdjustEntries(std::vector<cMenuDirItem*> &direntries, std::vector<cMenuFileItem*> &fileentries)
{
    //Big TODO: make it halfway efficient!
    //remove forbidden entries and hidden entries
    for (std::vector<cMenuDirItem*>::iterator itd = direntries.begin(); itd < direntries.end(); )
    {
        if (IsForbiddenDirEntry((*itd)->GetPath())
         || IsHiddenDirEntry((*itd)->GetPath())
         ||!IsAllowedDirEntry((*itd)->GetPath()))
        {
            itd = direntries.erase(itd);
        }
        else
        {
            ++itd;
        }
    }

    for (std::vector<cMenuFileItem*>::iterator itf = fileentries.begin(); itf < fileentries.end(); )
    {
        if (IsForbiddenDirEntry((*itf)->GetPath())
         || IsHiddenDirEntry((*itf)->GetPath())
         ||!IsAllowedDirEntry((*itf)->GetPath())
         ||!IsInsideFilter((**itf)))
        {
            itf = fileentries.erase(itf);
        }
        else
        {
             ++itf;
        }
    }
    //reposition entries
    RepositionDirEntries(direntries);
}     

//show ID3Info inside mp3 files
struct ID3Tag 
{
            std::string name;
            std::string value;
};

eOSState cMenuFileBrowser::ShowID3TagInfo(cPlayListItem item, bool hide, bool currentChanched)
{ 
    //printf("ShowID3TagInfo--------------\n");
    static bool dirty = true;
    static bool cleared = false;

    if(!hide)
    {      
        //printf("ShowID3TagInfo !hide--------------\n");
        std::string title, artist, album, comment, genre, year;
        if(currentChanched)
        {
            dirty = true;
        }
        if(dirty)
        {
	    //printf("ShowID3TagInfo dirty++++++++++++++++++++++++++++++++++++++++++\n");
            dirty = false;
            std::vector<struct ID3Tag> ID3Tags;
            std::vector<std::string> ID3Info = item.GetFileType().GetFileInfo(item.GetPath());

            for(uint i = 0; i < ID3Info.size(); ++i)
            {
                if(StartsWith(ID3Info[i], tr("Title")) && title.empty())
                {
                    title = GetAfterChars(ID3Info[i], ':', ' ');
                    //printf("title = %s\n", title.c_str());
                }
                else if(StartsWith(ID3Info[i], tr("Artist")) && artist.empty()) 
                {
                    artist = GetAfterChars(ID3Info[i], ':', ' ');
                    //printf("artist = %s\n", artist.c_str());
                }
                else if(StartsWith(ID3Info[i], tr("Album")) && album.empty())
                {
                    album = GetAfterChars(ID3Info[i], ':', ' ');
                    //printf("album = %s\n", album.c_str());
                }
                else if(StartsWith(ID3Info[i], tr("Comment")) && comment.empty()) 
                {
                    comment = GetAfterChars(ID3Info[i], ':', ' ');
                }    
                else if(StartsWith(ID3Info[i], tr("Genre")) && genre.empty()) 
                {
                    genre = GetAfterChars(ID3Info[i], ':', ' ');
                }   
                else if(StartsWith(ID3Info[i], tr("Year")) && year.empty()) 
                {
                    year = GetAfterChars(ID3Info[i], ':', ' ');
                    //printf("year = %s\n", year.c_str());
                }
                //else if(StartsWith(ID3Info[i], "genre:")
                //printf("ID3Info[%d] = %s\n", i, ID3Info[i].c_str());
                //if(ID3Tags[i])
            }
    
            cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
            if (skin) { 
                
                struct ID3Tag tag;
                if (!artist.empty()) {
                    tag.name = "Artist:";
                    tag.value = artist;
                    ID3Tags.push_back(tag);
                }
                if (!title.empty()) {
                    tag.name = "Title:";
                    tag.value = title;
                    ID3Tags.push_back(tag);
                }
                if (!album.empty()) {
                    tag.name = "Album:";
                    tag.value = album;
                    ID3Tags.push_back(tag);
                }
    #if 0
                if (!comment.empty()) {
                    tag.name = "Comment";
                    tag.value = comment;
                    ID3Tags.push_back(tag);
                }
    #endif
                if (!genre.empty()) {
                    tag.name = "Genre:";
                    tag.value = genre;
                    ID3Tags.push_back(tag);
                }
                //if ( year != 0 ) {
                 if ( !year.empty() ) {  
                    tag.name = "Year:";
                    /*std::stringstream s;
                    s << year;
                    tag.value = s.str();*/
                    tag.value = year;
                    ID3Tags.push_back(tag);
                }
#if 0
                if ( track != 0 ) {
                    tag.name = "Tracknumber:";
                    std::stringstream s;
                    s << track;
                    tag.value = s.str();
                    ID3Tags.push_back(tag);
                }
#endif
		
#if APIVERSNUM < 10700
                skin->Service("setId3Infos", &ID3Tags);
                Display();
#else
                //Display();
                cStringList id3tags;
                //cString tag;
                id3tags.Append(strdup("ID3Tags"));
                for (uint i=0; i < ID3Tags.size(); ++i) {
                    //tag = ID3Tags.at(i).value;
                    id3tags.Append(strdup(ID3Tags.at(i).value.c_str()));
                }
                if (ID3Tags.size())
                    SideNote(&id3tags);
                //printf("++++++++++++++++++++++Sent %d id3tags\n", id3tags.Size());
		
#endif		
                //SetTitle("laberfasel");
                ID3Tags.clear();
                
                cleared = false;
            }
        }
        else
        {
	    //printf("ShowID3TagInfo !dirty--------------\n");
            /*cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
            if (skin) 
            {
                skin->Service("setId3Infos", NULL);
            }*/
        }
    }
    else if(!cleared)
    {  
        //printf("ShowID3TagInfo !cleared--------------\n");
        cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
        if (skin) 
        {
            skin->Service("setId3Infos", NULL);
            Display();
            cleared = true;
        }
    }
    return osContinue;
}
