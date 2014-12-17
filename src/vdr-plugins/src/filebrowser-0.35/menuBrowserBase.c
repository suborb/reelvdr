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

void cMenuFileBrowserBase::MarkItem(cMenuBrowserItem *item, bool next)
{
    if(GetLast(item->GetPath()) != std::string(".."))
    {
        markedEntries_.insert(item->GetId());
        item->Mark();
        if(next && item->IsFile())
        {
            SetNextCurrent(true);
        }
    }
}

void cMenuFileBrowserBase::RemoveMark(cMenuBrowserItem *item, bool next)
{
    markedEntries_.erase(item->GetId());
    item->UnMark();
    if(next && item->IsFile())
    {
        SetNextCurrent(true);
    }
}

void cMenuFileBrowserBase::MarkAllFiles(bool onlyXinePlayable)
{
    cMenuBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetItem(i);
        SetCurrent(Get(i));
        if(item->IsFile() && !IsMarked(item) && 
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            MarkItem(item);
        }
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

bool cMenuFileBrowserBase::AllFilesAreMarked(bool onlyXinePlayable) const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetItem(i)->IsFile() && !IsMarked(GetItem(i)) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(GetItem(i))->IsXinePlayable()))
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

void cMenuFileBrowserBase::ClearMarks(bool next)
{
    while(!markedEntries_.empty())
    {
        cMenuBrowserItem *item = GetItem(*markedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetItemPos(*markedEntries_.begin())));
            RemoveMark(item, next);
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

void cMenuFileBrowserBase::ClearAllFileMarks(bool onlyXinePlayable)
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && IsMarked(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            RemoveMark(item, false);
        }
    }
    Display();
}

void cMenuFileBrowserBase::MarkUnmarkAllFiles(bool onlyXinePlayable)
{
    if(AllFilesAreMarked(onlyXinePlayable))
    {
        ClearAllFileMarks(onlyXinePlayable);
    }
    else
    {
        MarkAllFiles(onlyXinePlayable);
    }
    Display();
}

void cMenuFileBrowserBase::FlagItem(cMenuBrowserItem *item, bool next)
{
    printf("------------cMenuFileBrowserBase::FlagItem----------\n");
    if(GetLast(item->GetPath()) != std::string(".."))
    {
        printf("--------flaggedEntries_.insert---------\n");
        flaggedEntries_.insert(item->GetId());
	printf("hhhhhhhhhhh IsFlagged(item) = %d\n", IsFlagged(item));
        item->Flag();
        if(next && item->IsFile())
        {
            SetNextCurrent(true);
        }
    }
}

void cMenuFileBrowserBase::RemoveFlag(cMenuBrowserItem *item, bool next)
{
    flaggedEntries_.erase(item->GetId());
    item->UnFlag();
    if(next && item->IsFile())
    {
        SetNextCurrent(true);
    }
}

bool cMenuFileBrowserBase::IsFlagged(cMenuBrowserItem *item) const
{
    return (flaggedEntries_.find(item->GetId()) != flaggedEntries_.end());
}

bool cMenuFileBrowserBase::AllIsFlagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsFlagged(GetItem(i)))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::NothingIsFlagged() const
{
    return flaggedEntries_.empty();
}

bool cMenuFileBrowserBase::AllFilesAreFlagged(bool onlyXinePlayable) const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetItem(i)->IsFile() && !IsFlagged(GetItem(i)) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(GetItem(i))->IsXinePlayable()))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::HasFlaggedDirEntryItems() const
{
    for(std::set<itemId>::const_iterator i = flaggedEntries_.begin(); i!= flaggedEntries_.end(); ++i)
    {
        if(GetDirEntryItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cMenuFileBrowserBase::SingleDirEntryItemIsFlagged() const
{
    if(flaggedEntries_.size() == 1 &&  GetDirEntryItem(*flaggedEntries_.begin()))
    {
        return true;
    }
    return false;
}

void cMenuFileBrowserBase::FlagAllFiles(bool onlyXinePlayable)
{
    cMenuBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetItem(i);
        SetCurrent(Get(i));
        if(item->IsFile() && !IsMarked(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            FlagItem(item);
        }
    }
    Display();
}

void cMenuFileBrowserBase::ClearFlags(bool next)
{
    while(!flaggedEntries_.empty())
    {
        cMenuBrowserItem *item = GetItem(*flaggedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetItemPos(*flaggedEntries_.begin())));
            RemoveFlag(item, next);
        }
	else
	    break;
    }
    flaggedEntries_.clear();
}

void cMenuFileBrowserBase::ReFlagItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(flaggedEntries_.find(item->GetId()) != flaggedEntries_.end())
        {
            FlagItem(item);
        }
    }
}

void cMenuFileBrowserBase::FlagUnflagCurrentItem()
{
    cMenuBrowserItem *item = GetCurrentItem();
    if (item && (item->IsFile() || item->IsDir()))
    {
        if(!IsFlagged(GetCurrentItem()))
        {
            //select line
            FlagItem(GetCurrentItem());
        }
        else
        {
            //deselect line
            RemoveFlag(GetCurrentItem());
        }
        Display();
     }
}

void cMenuFileBrowserBase::ClearAllFileFlags(bool onlyXinePlayable)
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && IsFlagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            RemoveFlag(item, false);
        }
    }
    Display();
}

void cMenuFileBrowserBase::FlagUnflagAllFiles(bool onlyXinePlayable)
{
    if(AllFilesAreFlagged())
    {
        ClearAllFileFlags(onlyXinePlayable);
    }
    else
    {
        FlagAllFiles(onlyXinePlayable);
    }
    Display();
}

void cMenuFileBrowserBase::TagItem(cMenuBrowserItem *item, bool next)
{
    if(GetLast(item->GetPath()) != std::string(".."))
    {
        taggedEntries_.insert(item->GetId());
        item->Tag();
        if(next && item->IsFile())
        {
            SetNextCurrent(true);
        }
    }
}

void cMenuFileBrowserBase::RemoveTag(cMenuBrowserItem *item, bool next)
{
    taggedEntries_.erase(item->GetId());
    item->UnTag();
    if(next && item->IsFile())
    {
        SetNextCurrent(true);
    }
}

bool cMenuFileBrowserBase::IsTagged(cMenuBrowserItem *item) const
{
    return (taggedEntries_.find(item->GetId()) != taggedEntries_.end());
}

bool cMenuFileBrowserBase::AllIsTagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsTagged(GetItem(i)))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::NothingIsTagged() const
{
    return taggedEntries_.empty();
}

bool cMenuFileBrowserBase::AllFilesAreTagged(bool onlyXinePlayable) const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetItem(i)->IsFile() && !IsTagged(GetItem(i)) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(GetItem(i))->IsXinePlayable()))
            return false;
    }
    return true;
}

bool cMenuFileBrowserBase::HasTaggedDirEntryItems() const
{
    for(std::set<itemId>::const_iterator i = taggedEntries_.begin(); i!= taggedEntries_.end(); ++i)
    {
        if(GetDirEntryItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cMenuFileBrowserBase::SingleDirEntryItemIsTagged() const
{
    if(taggedEntries_.size() == 1 &&  GetDirEntryItem(*taggedEntries_.begin()))
    {
        return true;
    }
    return false;
}

void cMenuFileBrowserBase::TagAllFiles(bool onlyXinePlayable)
{
    cMenuBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetItem(i);
        SetCurrent(Get(i));
        if(item->IsFile() && !IsMarked(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            TagItem(item);
        }
    }
    Display();
}

void cMenuFileBrowserBase::ClearTags(bool next)
{
    while(!taggedEntries_.empty())
    {
        cMenuBrowserItem *item = GetItem(*taggedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetItemPos(*taggedEntries_.begin())));
            RemoveTag(item, next);
        }
	else
	    break;
    }
    taggedEntries_.clear();
}

void cMenuFileBrowserBase::ReTagItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(taggedEntries_.find(item->GetId()) != taggedEntries_.end())
        {
            TagItem(item);
        }
    }
}

void cMenuFileBrowserBase::TagUntagCurrentItem()
{
    cMenuBrowserItem *item = GetCurrentItem();
    if (item && (item->IsFile() || item->IsDir()))
    {
        if(!IsTagged(GetCurrentItem()))
        {
            //select line
            TagItem(GetCurrentItem());
        }
        else
        {
            //deselect line
            RemoveTag(GetCurrentItem());
        }
        Display();
     }
} 

void cMenuFileBrowserBase::ClearAllFileTags(bool onlyXinePlayable)
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && IsTagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            RemoveTag(item, false);
        }
    }
    Display();
}

void cMenuFileBrowserBase::TagUntagAllFiles(bool onlyXinePlayable)
{
    if(AllFilesAreTagged())
    {
        ClearAllFileTags(onlyXinePlayable);
    }
    else
    {
        TagAllFiles(onlyXinePlayable);
    }
    Display();
}


void cMenuFileBrowserBase::MarkAllNonFlaggedFiles(bool onlyXinePlayable)
{
   for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && !IsFlagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
	    RemoveTag(item, false);
            MarkItem(item, false);
        }
    } 
} 

bool cMenuFileBrowserBase::HasUnMarkedOrUnFlaggedFileItems(bool onlyXinePlayable) const
{
    for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && !IsMarked(item) && !IsFlagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            return true;
        }
    } 
    return false;
}  

void cMenuFileBrowserBase::FlagAllTaggedFiles(bool onlyXinePlayable)
{
   for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && IsTagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
	    RemoveTag(item, false);
            FlagItem(item, false);
        }
    } 
}  

void cMenuFileBrowserBase::MarkAllNonTaggedOrFlaggedFiles(bool onlyXinePlayable)
{
   for (int i = 0; i < Count(); i++)
    {
        cMenuBrowserItem *item = GetItem(i);
        if(item->IsFile() && !IsFlagged(item) && !IsTagged(item) &&
	(!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
        {
            MarkItem(item, false);
        }
    } 
} 

void cMenuFileBrowserBase::TagAllFlaggedFiles(bool onlyXinePlayable)
{
  for (int i = 0; i < Count(); i++)
  {
      cMenuBrowserItem *item = GetItem(i);
      if(item->IsFile() && IsFlagged(item) &&
      (!onlyXinePlayable || static_cast<cMenuFileItem *>(item)->IsXinePlayable()))
      {
	  RemoveFlag(item, false);
	  TagItem(item, false);
      }
  } 
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
    printf("cMenuFileBrowserBase::SetMode\n");
    status->SetMode(mode);
}

browseMode cMenuFileBrowserBase::GetMode() const
{
    return status->GetMode();
}

void cMenuFileBrowserBase::XineStarted(bool withPlaylist)
{
    if(status->GetMode() != browseplaylist2)
    {  
	status->PlayListActive(withPlaylist);
    }
    cUserIfBase::ChangeIfType(if_xine);
}
