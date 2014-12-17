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
#include <iomanip>
#include <sstream>

#include <vdr/osd.h>
#include <vdr/remote.h>
#include "vdr/interface.h"
#include <vdr/debug.h>
#include <vdr/plugin.h>

#include "browsermenu.h"
#include "userIf.h"
#include "def.h"

#include "../filebrowser/stringtools.h"

#include "convert.h"
#include "filetools.h"
#include "filecache.h"
#include "tools.h"

#include "../xinemediaplayer/services.h"


std::string dbPath = "nn3";

//----------cVolumeStatus------------------------------
cVolumeStatus::cVolumeStatus(cBrowserMenu* m) : menu(m)
{
}

bool cVolumeStatus::ClearVolume()
{
    if (volumeSet && volumeTimeout.TimedOut())
    {
        volumeSet = false;
        return true;
    }

    return false;
}

void cVolumeStatus::SetVolume(int Volume, bool Absolute)
{
    cDevice *device = cDevice::PrimaryDevice();
    if(!device) return;

    Volume = device->CurrentVolume();

    std::string volumeStr = MakeProgressBar(Volume*100.0/MAXVOLUME, 30);

    if (!volumeStr.empty()) {
        volumeStr = std::string(tr("Volume: ")) + volumeStr;
        menu->SetStatus(volumeStr.c_str());

        volumeSet = true;
        volumeTimeout.Set(2000);
    }
}

//----------cBrowserMenu-----------------------

cPlayList cBrowserMenu::playlist_;
cPlayList cBrowserMenu::favlist_(playable, FAVLIST_NAME, PLAYLIST_DEFAULT_PATH);

cBrowserMenu::cBrowserMenu(const std::string title, cMp3BrowserStatus *newstatus)
: cBrowserBase(title.c_str()), volStatus(this), search_(dbPath) 
//playlist_(newstatus->GetPlayList()), tmpPlaylist_(xinePlayable)
{
    printf("cBrowserMenu::cBrowserMenu");
    //SetStandardCols();
    //SetHasHotkeys();
    //EnableSideNote(true);

    status = newstatus;
    userIf_ = lastUserIf_ = status->GetLastIf();
    userIf_->EnterState(this, &search_, NULL, osEnd);
    //status->Startup();

    EnableSideNote(true);
}

cBrowserMenu::~cBrowserMenu()
{
    userIf_->LeaveState(this);
    status->SetLastIf(userIf_);
}

std::string MakeProgressBar(float percent, int len)
{
    if (percent < 0) return std::string();
    if (percent >= 100) percent = 100;

    len -= 2; // two chars for [ and ]
    if (len <= 0)
        return std::string();

    unsigned int p = (percent*len)/100;
    if (!p) return std::string("[") + std::string(len, ' ') + std::string("]");

    return std::string("[")
            + std::string(p, '|')
            + std::string(len-p?len-p:1, ' ')
            + std::string("]");
}

void cBrowserMenu::Display()
{
    cOsdMenu::Display();
    userIf_->UpdateSideNote(this);
}

void cBrowserMenu::ClearVolume()
{
     if (volStatus.ClearVolume())
         SetStatus(NULL);
}

void cBrowserMenu::DrawSideNote(cStringList *list)
{
    SideNote(list);
}

eOSState cBrowserMenu::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;
    if(Key != kInfo)
    { 
        state = userIf_->AnyKey(this, Key);
    }
    //RefreshIfDirty();

    if(state == osUnknown /*&& !GetBrowserSubMenu()*/)
    {
        switch (NORMALKEY(Key))
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
            case kPause:
            case kPlay:
            case kFastFwd:
            case kFastRew:
            {
                Xinemediaplayer_handle_keys Keys;
                Keys.Key = Key;
                cPluginManager::CallFirstService("Xine handle keys", &Keys);
                state = osContinue;
            }
                break;
            case kStop:
                cControl::Shutdown();
                state = osContinue;
                break;
            default:
                state = osContinue;
                break;
        }
    }

    if(userIfchanged_)
    {
	userIf_->EnterState(this, &search_, lastUserIf_);	
	userIfchanged_ = false; 
	lastUserIf_ = userIf_;
    }  
    
    if(CurrentChanged())
    {
	userIf_->UpdateImage(this);
    }  
      
    ClearVolume();  
        
    return state;
}

eOSState cBrowserMenu::StandardKeyHandling(eKeys Key)
{
    eOSState state;
    if((Key == kBack || Key == kInfo) && !HasSubMenu()) //these keys are handled separately
    {
        state = osUnknown;
    }
    else
    {
        state  = cOsdMenu::ProcessKey(Key);
    }

    return state;
}


void cBrowserMenu::SetItems(std::vector<cOsdItem*> osdItems)
{ 
    Clear();

    printf("Got %u osditems\n", osdItems.size());
    std::vector<cOsdItem*>::iterator it = osdItems.begin();
    while (it != osdItems.end())
    {
        Add(*it);
        ++it;
    }
}

void cBrowserMenu::SetColumns(int c0, int c1, int c2, int c3, int c4)
{
    SetCols(c0, c1, c2, c3, c4);
}

void cBrowserMenu::SetIf(cBrowserBaseIf *userIf, cBrowserBaseIf *lastUserIf)
{
    printf("cBrowserMenu::SetIf, 2 args\n");
    lastUserIf_ = lastUserIf;
    SetIf(userIf);
} 

void cBrowserMenu::SetIf(cBrowserBaseIf *userIf)
{
    printf("cBrowserMenu::SetIf\n");
    userIf_->LeaveState(this);
    userIf_ = userIf;
    userIfchanged_ = true;
}  

void cBrowserMenu::LoadLivelist()
{
    // Data structure for service "Xine getset playlist"
    std::string playlistName;
    std::vector<std::string> playlistEntries;
    
    Xinemediaplayer_Xine_GetSet_playlist data = 
    {
	false,
	-5, 
	playlistName,
	playlistEntries
    };
    cPluginManager::CallAllServices("Xine getset playlist", &data);
    
    for (int i = 0; i< playlistEntries.size(); ++i)
    {  
	printf("playlistEntries[%d] = %s\n", i, playlistEntries[i].c_str()); 
    }	
    
    livelist_.SetName("livelist");
    livelist_.SetPlayListEntries(data.playlistEntries);
}  

/*eOSState cBrowserMenu::ShowPlayListWithXine(bool passInstance, bool showImmediately)
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
}*/


/*eOSState cBrowserMenu::ShowFileInPlaylist(const cPlayListItem &item, bool asPlaylistItem, bool passInstance, bool showImmediately)
{
   cPlaylist Playlist
  
  
  
}*/
  
  
eOSState cBrowserMenu::ShowFileWithXine(const cPlayListItem &item, bool asPlaylistItem, bool passInstance, bool showImmediately)
{
    //printf("----cMenuFileBrowser::ShowFileWithXine, asPlaylistItem = %d, passInstance = %d--\n", asPlaylistItem, passInstance );
    /*std::vector<std::string> playlistEntries; //empty list
    if(asPlaylistItem)
    {
	playlistEntries = playlist_->GetPathList();
    }
    
    printf("playlistEntries.size = %d\n", playlistEntries.size()); 
    for(uint i= 0; i< playlistEntries.size();++i)
    {  
      printf("playlistEntries[%d] = %s\n", i, playlistEntries[i].c_str()); 
    }
    
    std::string tmp;
    if(!asPlaylistItem || showImmediately)
    {  
	tmp = item.GetPath().c_str();
    }

    printf("file = %s\n", tmp.c_str()); 

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(),
        passInstance ,//? status->GetInstance() : -1,
        asPlaylistItem,
        playlist_->GetName(),
        playlistEntries
    };
    
    //defStatus->SetStartDir(currentdir_);
    //cRemote::CallPlugin("some none existing plugin name"); xine osd will not pop up!
    //printf("blocking callplugins() from other players\n");
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);*/
    //XineStarted(asPlaylistItem);
    return osEnd;
}


/*eOSState cMenuFileBrowser::LoadAndEnterPlayList(cPlayListItem item)
{
    LoadPlayList(item);
    return EnterPlayList();	
}*/


/*eOSState cMenuFileBrowser::EnterPlayList(bool play)
{
    std::string title = "Playlist: " + RemoveEnding(GetLast(playlist_->GetName()));
    return AddFileBrowserSubMenu(new cMenuPlayList(title.c_str(), playlist_, cache_, convert_, play));
}*/

/*eOSState cMenuFileBrowser::OpenMarked(bool &playlistEntered)
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
}*/


/*
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
*/


/*void cMenuFileBrowser::Hide(bool hide)
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
}*/

/*void cMenuFileBrowser::SetStandardCols()
{
    SetCols(2, 2, 2);
}*/  

/*void cMenuFileBrowser::ClearPlayList()
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
   } 
  
   //*playlist_ = tmpPlaylist_;
  
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
}*/  


/*void cMenuFileBrowser::InsertItemsInConversionList(cImageConvert *convert) const
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
}*/

/*void cMenuFileBrowser::LoadPlayList(cPlayListItem item)
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
}*/


//show ID3Info inside mp3 files
/*struct ID3Tag 
{
            std::string name;
            std::string value;
};*/

