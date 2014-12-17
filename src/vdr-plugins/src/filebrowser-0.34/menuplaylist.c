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
 
#include <set>
#include <sys/stat.h>
#include <sys/types.h> 

#include "stringtools.h"
#include "convert.h"
#include "filecache.h"
#include "stillpictureplayer.h"
#include "filebrowser.h"
#include "playlistUserIf.h"
#include "menuplaylist.h"

#include "../xinemediaplayer/services.h"

//--------------cMenuPlayList-----------------------------

cPlayList cMenuPlayList::activePlaylist;

cMenuPlayList::cMenuPlayList(std::string title, cPlayList *playlist, cFileCache *cache, cImageConvert *convert, bool play)
: cMenuFileBrowserBase(title), playlist_(playlist), cache_(cache), convert_(convert)
{
    //printf("----cMenuPlayList::cMenuPlayList,------\n");
    SetCols(2,2,2);
    userIf_ = cMenuPlayListStandardIf::Instance();
    userIf_->EnterState(this);
    Refresh(playlist_->CurrentIsSet() ? playlist_->GetCurrent().GetId() : itemId(), true);
}

cMenuPlayList::~cMenuPlayList()
{
    Shutdown();
};

void cMenuPlayList::Shutdown()
{
    ShutdownBrowserSubMenu();
}

eOSState cMenuPlayList::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;
    state = userIf_->AnyKey(this, Key);
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
            case kFastFwd:
                state = userIf_->FastFwdKey(this);
                break;
            case kFastRew: 
                state = userIf_->FastRewKey(this);
                break;
            default:
                state = osContinue;
                break;
        }
    }

    return state;
}

eOSState cMenuPlayList::ShowFileWithImageViewer(cPlayListItem &item, bool asPlaylist)
{
    //printf("------cMenuPlayList::ShowFileWithImageViewer, item = %s-------\n", item.GetPath().c_str());
    //BuildActualPlayList(item, activePlaylist_/*, !fileBrowserSetup.playAll*/);
    activePlaylist.Clear();
    playlist_->GetPartialListOfSimilarItems(item, activePlaylist);

    status->SetStillPictureActive(true);  
    cStillPictureControl *control = new cStillPictureControl(&activePlaylist, cache_, convert_, status->GetInstance());
    cControl::Launch(control);
    return osEnd;
    //return AddFileBrowserSubMenu(new cMenuShowFile(DESCRIPTION, &activePlaylist_, cache_, convert_));
}

eOSState cMenuPlayList::ShowFileWithXine(const cPlayListItem &item, bool asPlaylist)
{
    //BuildActualPlayList(GetCurrentFileItem()->GetPlayListItem(), activePlaylist_ /*, !fileBrowserSetup.playAll*/);
    activePlaylist.Clear();
    playlist_->GetPartialListOfSimilarItems(item, activePlaylist);
    std::vector<std::string> playlistEntries;
    playlistEntries = activePlaylist.GetPathList();
    std::string tmp = std::string("file://") + item.GetPath();

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(),
        status->GetInstance(),
        true,
        playlistEntries
    };

    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
    XineStarted(true);
    return osEnd;
}

void cMenuPlayList::OpenPlayList()
{
    for(std::list<cPlayListItem>::iterator i = playlist_->list_.begin(); i != playlist_->list_.end(); ++i)
    {
        cMenuFileItem *item = new cMenuFileItem(*i, this);
        Add(item, this);
        item->Set();
    }
    if(!playlist_->IsEmpty() && !playlist_->CurrentIsSet())
    {
        playlist_->SetCurrent(*playlist_->list_.begin());
        SetCurrentToItem(*playlist_->list_.begin());
    }
}

eOSState cMenuPlayList::StandardKeyHandling(eKeys Key)
{
    if(Key == kBack && !HasSubMenu()) //this key is handled separately
    {
        return osUnknown;
    }
    else
    {
        eOSState state = cOsdMenu::ProcessKey(Key);
        return state;
    }
}

eOSState cMenuPlayList::SetAndOpenCurrent()
{
    cMenuFileItem *fileitem = GetCurrentFileItem();
    if(fileitem)
    {
        playlist_->SetCurrent(fileitem->GetPlayListItem()); 
    }
    return OpenCurrent();
}

void cMenuPlayList::ShiftCurrent(bool up)
{ 
    cMenuFileItem *fileitem = GetCurrentFileItem();
    if(fileitem)
    {
        playlist_->ShiftItem((fileitem->GetPlayListItem()), up);
        Refresh();
    }
}

bool cMenuPlayList::Refresh(itemId newCurrentId, bool initial)
{
    itemId currentId = GetCurrentFileItem() ? GetCurrentFileItem()->GetId() : itemId();

    ClearMarks();
    Clear();
    OpenPlayList();

    if(initial && status->StillPictureActive())
    {
        currentId = activePlaylist.CurrentIsSet() ? activePlaylist.GetCurrent().GetId() : itemId();
        status->SetStillPictureActive(false);
    }
    if(!(newCurrentId == itemId()))
    {
        currentId = newCurrentId;
    }
    if(!(currentId == itemId()))
    { 
        SetCurrentToItem(cPlayListItem(currentId));
    }

    name_ = RemoveEnding(GetLast(playlist_->GetName()));
    SetTitle((std::string(tr("Playlist: ")) + name_).c_str());
    userIf_->SetHelp(this);
    Display();

    return true;
}

eOSState cMenuPlayList::ShowPlayListWithXine()
{
     //printf("----------------cMenuPlayList::ShowPlayListWithXine---------------------\n");
     if(!playlist_->CurrentIsSet())
     {
         cMenuFileItem *firstItem = GetFileItem(0);
         if(firstItem)
         {
             playlist_->SetCurrent(firstItem->GetPlayListItem());
         }
     }

     if(playlist_->CurrentIsSet())
     {
        return ShowFileWithXine(playlist_->GetCurrent(), true);
     }
     return osContinue;
}

void cMenuPlayList::Hide(bool hide)
{
    static int cur = 0;
    if(!hide)
    { 
        Clear();
        OpenPlayList();
        ReMarkItems();
        SetCurrent(Get(cur));
        userIf_->SetHelp(this);
    }
    else
    {
        cur = Current();
        Clear();
    }
    Display();
}

void cMenuPlayList::DeleteCurrentOrMarked()
{
    if(!markedEntries_.empty())
    {
        DeleteMarkedEntries();
    }
    else
    { 
        DeleteCurrentEntry();
    }
}

void cMenuPlayList::DeleteMarkedEntries()
{
    for(std::set<itemId>::iterator i = markedEntries_.begin(); i != markedEntries_.end(); i++)
    {
        playlist_->Remove(cPlayListItem(*i));
    }
    Refresh();
}

void cMenuPlayList::DeleteCurrentEntry()
{
    cMenuFileItem *item = GetCurrentFileItem();
    if(item)
    {
        playlist_->Remove(item->GetPlayListItem());
        SetNextCurrent(true);
        Refresh();
    }
} 

void cMenuPlayList::ClosePlayList()
{
    playlist_->Clear(status->GetExitDir());
    Refresh();
}

void cMenuPlayList::SavePlayList()
{
    playlist_->SaveToFile();
}

void cMenuPlayList::SetName(const std::string &name)
{
    playlist_->SetName(name);
}

std::string cMenuPlayList::GetName() const
{
    return playlist_->GetName();
}

std::string cMenuPlayList::GetTitleString() const
{
    return std::string(tr("Playlist: ")) + RemoveEnding(GetLast(GetName()));
}

bool cMenuPlayList::PlayListIsDirty()
{
    return playlist_->IsDirty();
}

void cMenuPlayList::SetMode(enum browseMode mode)
{
    status->SetMode(mode);
}

int cMenuPlayList::GetNextPlayableItemCyclic(cMenuFileItem *currentitem, bool all) const
{
    cMenuFileItem* nextitem;
    for (int i= Current() + 1; i < Count(); i++)
    {
        nextitem = GetFileItem(i);
        if((all && nextitem->IsPlayable()) || nextitem->IsOfSimilarType(*currentitem))
        {
            return i;
        }
    }
    for (int j = 0; j < Current(); j++)
    {
        nextitem = GetFileItem(j);
        if((all && nextitem->IsPlayable()) || nextitem->IsOfSimilarType(*currentitem))
        {
            return j;
        }
    }
    return  Current();
}

bool cMenuPlayList::GetNextPlayableItem(cMenuFileItem *currentitem, int &next, bool all) const
{
    cMenuFileItem* nextitem;
    for (int i= Current() + 1; i < Count(); i++)
    {
        nextitem = GetFileItem(i);
        if(nextitem)
        {
            if((all && nextitem->IsPlayable()) || nextitem->IsOfSimilarType(*currentitem))
            {
                next = i;
                return true;
            }
        }
    }
    return false;
}

bool cMenuPlayList::SetCurrentToNextPlayableItem(bool all)
{
    cMenuFileItem *item = GetCurrentFileItem();
    if(item)
    {
        int next = Current();
        if (GetNextPlayableItem(item, next, all))
        {
            SetCurrent(Get(next));
            return true;
        }
    }
    return false;
}

void cMenuPlayList::SetCurrentToItem(cPlayListItem currentItem)
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetFileItem(i)->GetPlayListItem() == currentItem)
        {
            SetCurrent(Get(i));
            break;
        }
    }
}

void cMenuPlayList::CopyPlayListToCD()
{ 
    playlist_->SaveToFile();
    std::string cmd =  std::string("sudo mp3_2_media.sh ") + playlist_->GetName();
    printf("-----CopyPlaylistToCD %s--------\n", cmd.c_str());
    ::SystemExec(cmd.c_str());
}

