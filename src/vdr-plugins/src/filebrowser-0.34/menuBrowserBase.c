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

#include "playlist.h"
#include "stringtools.h"
#include "userIfBase.h"
#include "menuBrowserBase.h"

//---------------------cSetHelpWrapper------------------------------

void cSetHelpWrapper::SetRed(const std::string &msg)
{
    newBtTitles[0] = msg;
}
void cSetHelpWrapper::SetGreen(const std::string &msg)
{
    newBtTitles[1] = msg;
}
void cSetHelpWrapper::SetYellow(const std::string &msg)
{
    newBtTitles[2] = msg;
}
void cSetHelpWrapper::SetBlue(const std::string &msg)
{
    newBtTitles[3] = msg;
}
void cSetHelpWrapper::SetInfo(const bool msg=false)
{
    newBtInfo = msg;
}

void cSetHelpWrapper::FlushButtons(bool forceFlush)
{
    bool flush = forceFlush;
    static eKeys ikeys[] = { kInfo, kNone };

    for (int i = 0; i < 4; i++)
    {
        if(btTitles[i] != newBtTitles[i])
        {
            btTitles[i] = newBtTitles[i];
            flush = true;
        }
    }
    if (btInfo != newBtInfo)
    {
	btInfo = newBtInfo;
	flush = true;
    }
    if(flush)
    {
        menu_->SetHelpKeys(GetStr(btTitles[0]), GetStr(btTitles[1]),GetStr(btTitles[2]), GetStr(btTitles[3])
#if APIVERSNUM > 10700
                           , btInfo ? ikeys : NULL
#endif
        );
    }
}

const char *cSetHelpWrapper::GetStr(const std::string &msg)
{
    if(msg == "")
    {
        return NULL;
    }
    else
    {
        return tr(msg.c_str());
    }
}

//----------cMenuFileBrowserBase--------------------

cFileBrowserStatus *cMenuFileBrowserBase::status = NULL;

void cMenuFileBrowserBase::MarkItem(cMenuBrowserItem *item)
{
    if(GetLast(item->GetPath()) != std::string(".."))
    {
        markedEntries_.insert(item->GetId());
        item->Mark();
        if(item->IsFile())
        {
            SetNextCurrent(true);
        }
    }
}

void cMenuFileBrowserBase::RemoveMark(cMenuBrowserItem *item)
{
    markedEntries_.erase(item->GetId());
    item->UnMark();
    if(item->IsFile())
    {
        SetNextCurrent(true);
    }
}

bool cMenuFileBrowserBase::IsMarked(cMenuBrowserItem *item) const
{
    return (markedEntries_.find(item->GetId()) != markedEntries_.end());
}

bool cMenuFileBrowserBase::AllIsMarked() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsMarked(GetItem(i)))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::NothingIsMarked() const
{
    return markedEntries_.empty();
}

bool cMenuFileBrowserBase::AllFilesAreMarked() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetItem(i)->IsFile() && !IsMarked(GetItem(i)))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::HasMarkedDirEntryItems() const
{
    for(std::set<itemId>::const_iterator i = markedEntries_.begin(); i!= markedEntries_.end(); ++i)
    {
        if(GetDirEntryItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cMenuFileBrowserBase::SingleDirEntryItemIsMarked() const
{
    if(markedEntries_.size() == 1 &&  GetDirEntryItem(*markedEntries_.begin()))
    {
        return true;
    }
    return false;
}

bool cMenuFileBrowserBase::NoFiles() const
{
    cMenuBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetItem(i);
        if(item && item->IsFile())
            return false;
    }
    return true;
}

void cMenuFileBrowserBase::MarkAllFiles()
{
    cMenuBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetItem(i);
        SetCurrent(Get(i));
        if(item->IsFile() && !IsMarked(item))
        {
            MarkItem(item);
        }
    }
}

void cMenuFileBrowserBase::ClearMarks()
{
    while(!markedEntries_.empty())
    {
        cMenuBrowserItem *item = GetItem(*markedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetItemPos(*markedEntries_.begin())));
            RemoveMark(item);
        }
	else
	    break;
    }
    markedEntries_.clear();
}

void cMenuFileBrowserBase::ReMarkItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(markedEntries_.find(item->GetId()) != markedEntries_.end())
        {
            MarkItem(item);
        }
    }
}

void cMenuFileBrowserBase::SetNextCurrent(bool up)
{
    int current = Current();
    if(up && ++current < Count())
    {
        SetCurrent(Get(current));
    }
    else if(!up && --current >= 0)
    {
        SetCurrent(Get(current));
    }
}

void cMenuFileBrowserBase::MarkUnmarkCurrentItem()
{
    cMenuBrowserItem *item = GetCurrentItem();
    if (item && (item->IsFile() || item->IsDir()))
    {
        if(!IsMarked(GetCurrentItem()))
        {
            //select line
            MarkItem(GetCurrentItem());
        }
        else
        {
            //deselect line
            RemoveMark(GetCurrentItem());
        }
        Display();
     }
}

void cMenuFileBrowserBase::MarkUnmarkAllFiles()
{
    if(AllFilesAreMarked())
    {
        ClearMarks();
    }
    else
    {
        MarkAllFiles();
    }
    Display();
}

bool cMenuFileBrowserBase::CurrentChanged()
{
    printf("Current = %d, lastCurrent_ = %d\n", Current(), lastCurrent_ );
    if(Count() && lastCurrent_ != Current())
    {
        lastCurrent_ = Current();
        return true;
    }
    return false;
}

eOSState cMenuFileBrowserBase::OpenCurrent(bool asPlaylistItem)
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Open(this, asPlaylistItem);
    }
    return osContinue;
}

eOSState cMenuFileBrowserBase::OpenCurrentDir()
{
    if(GetCurrentDirItem())
    {
        return GetCurrentDirItem()->Open(this);
    }
    return osContinue;
}

eOSState cMenuFileBrowserBase::UpdateCurrent(bool currentChanged)
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Update(this, currentChanged);
    }
    return osContinue;
}

eOSState cMenuFileBrowserBase::PlayCurrent()
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Play(this);
    }
    return osContinue;
}

eOSState cMenuFileBrowserBase::OpenCurrentInfo()
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->OpenInfo();
    }
    return osContinue;
}

eOSState cMenuFileBrowserBase::SetInfoButton(bool currentChanged)
{
    if(currentChanged)
    {
	cMenuFileItem *fileItem  = GetCurrentFileItem();
	if(fileItem && fileItem->GetPlayListItem().GetFileType().HasInfo())
	{  
	    setHelp.SetInfo(true);
	}
	else
	{  
	    setHelp.SetInfo(false);
	}
	setHelp.FlushButtons();
    }  
    return osContinue;
}  

cMenuBrowserItem *cMenuFileBrowserBase::GetItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cMenuBrowserItem *item = GetItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}

cMenuDirEntryItem *cMenuFileBrowserBase::GetDirEntryItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cMenuDirEntryItem *item = GetDirEntryItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}

cMenuFileItem *cMenuFileBrowserBase::GetFileItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cMenuFileItem *item = GetFileItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}

int cMenuFileBrowserBase::GetItemPos(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item && item->GetId() == id)
        {
            return i;
        }
    }
    return -1;
}

cMenuDirEntryItem *cMenuFileBrowserBase::GetSingleMarkedDirEntryItem() const
{
    if(markedEntries_.size() == 1)
    {
        return GetDirEntryItem(*markedEntries_.begin());
    }
    return NULL;
}

cMenuFileItem *cMenuFileBrowserBase::GetSingleMarkedFileItem() const
{
    if(markedEntries_.size() == 1)
    {
        return GetFileItem(*markedEntries_.begin());
    }
    return NULL;
}

void cMenuFileBrowserBase::BuildActualPlayList(const cPlayListItem &item, cPlayList &playlist, bool all)
{
    playlist.Clear();

    if(!item.GetFileType().IsPlayable())
    {
        return;
    }

    int pos = GetItemPos(item.GetPath());
    if(pos < 0)
    {
        return;
    }

    int startPos = 0;
    cMenuFileItem* fileitem;
    for (startPos = pos; startPos >= 0; --startPos)
    {
        fileitem = GetFileItem(startPos);
        if(fileitem)
        {
            if(fileitem->IsPlayable() && !fileitem->IsOfSimilarType(item) && !all)
            {
                ++startPos;
                break;
            }
        }
    }
    for (int i = startPos; i<= Count(); ++i)
    {
        fileitem = GetFileItem(i);
        if(fileitem)
        {
            if(fileitem->IsOfSimilarType(item))
            {
                playlist.Insert(*fileitem);
            }
            else if(fileitem->IsPlayable() && !all)
            {
                break;
            }
        }
    }

    playlist.SetCurrent(item);
}

void cMenuFileBrowserBase::RefreshIfDirty()
{
    if(!HasSubMenu())
    {
        if(dirty_)
        {
            if(!hidden)
            {
                Refresh(itemId(), true);
            }
            dirty_ = false;
        }
    }
    else
    {
        dirty_ = true;
    }
}

void cMenuFileBrowserBase::SetTitleString(std::string titlestring)
{
    SetTitle(titlestring.c_str());
}

void cMenuFileBrowserBase::SetMode(enum browseMode mode)
{
    status->SetMode(mode);
}

browseMode cMenuFileBrowserBase::GetMode() const
{
    return status->GetMode();
}

void cMenuFileBrowserBase::XineStarted(bool withPlaylist)
{
    status->PlayListActive(withPlaylist);
    cUserIfBase::ChangeIfType(if_xine);
}
