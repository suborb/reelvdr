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
#include "../filebrowser/stringtools.h"
#include "userIf.h"
#include "browserbase.h"

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

//----------cBrowserBase--------------------

void cBrowserBase::MarkItem(cBrowserItem *item, bool next)
{
    markedEntries_.insert(item->GetId());
    item->Mark();
    if(next && item->IsFileItem())
    {
	SetNextCurrent(true);
    }
}

void cBrowserBase::RemoveMark(cBrowserItem *item, bool next)
{
    markedEntries_.erase(item->GetId());
    item->UnMark();
    if(next && item->IsFileItem())
    {
        SetNextCurrent(true);
    }
}

void cBrowserBase::MarkAllFiles()
{
    cBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetBrowserItem(i);
        SetCurrent(Get(i));
        if(item->IsFileItem() && !IsMarked(item))
        {
            MarkItem(item);
        }
    }
}

bool cBrowserBase::IsMarked(cBrowserItem *item) const
{
    return (markedEntries_.find(item->GetId()) != markedEntries_.end());
}

bool cBrowserBase::AllIsMarked() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsMarked(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::NothingIsMarked() const
{
    return markedEntries_.empty();
}

bool cBrowserBase::AllFilesAreMarked() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetBrowserItem(i)->IsFileItem() && !IsMarked(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::HasMarkedFolderItems() const
{
    for(std::set<itemId>::const_iterator i = markedEntries_.begin(); i!= markedEntries_.end(); ++i)
    {
        if(GetFolderItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cBrowserBase::SingleFolderItemIsMarked() const
{
    if(markedEntries_.size() == 1 &&  GetFolderItem(*markedEntries_.begin()))
    {
        return true;
    }
    return false;
}

cFolderItem *cBrowserBase::GetSingleMarkedFolderItem() const
{
    if(markedEntries_.size() == 1)
    {
        return GetFolderItem(*markedEntries_.begin());
    }
    return NULL;
}

cFileItem *cBrowserBase::GetSingleMarkedFileItem() const
{
    if(markedEntries_.size() == 1)
    {
        return GetFileItem(*markedEntries_.begin());
    }
    return NULL;
}

void cBrowserBase::ClearMarks(bool next)
{
    while(!markedEntries_.empty())
    {
        cBrowserItem *item = GetBrowserItem(*markedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetBrowserItemPos(*markedEntries_.begin())));
            RemoveMark(item, next);
        }
	else
	    break;
    }
    markedEntries_.clear();
}

void cBrowserBase::ReMarkItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(markedEntries_.find(item->GetId()) != markedEntries_.end())
        {
            MarkItem(item);
        }
    }
}

void cBrowserBase::MarkUnmarkCurrentItem()
{
    if(!IsMarked(GetCurrentBrowserItem()))
    {
	//select line
	MarkItem(GetCurrentBrowserItem());
    }
    else
    {
	//deselect line
	RemoveMark(GetCurrentBrowserItem());
    }
    Display();
}

void cBrowserBase::ClearAllFileMarks()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && IsMarked(item))
        {
            RemoveMark(item, false);
        }
    }
    Display();
}

void cBrowserBase::MarkUnmarkAllFiles()
{
    if(AllFilesAreMarked())
    {
        ClearAllFileMarks();
    }
    else
    {
        MarkAllFiles();
    }
    Display();
}

void cBrowserBase::FlagItem(cBrowserItem *item, bool next)
{
    //printf("--------flaggedEntries_.insert---------\n");
    flaggedEntries_.insert(item->GetId());
    printf("hhhhhhhhhhh IsFlagged(item) = %d\n", IsFlagged(item));
    //item->Flag();
    if(next && item->IsFileItem())
    {
	SetNextCurrent(true);
    }
}

void cBrowserBase::RemoveFlag(cBrowserItem *item, bool next)
{
    flaggedEntries_.erase(item->GetId());
    item->UnFlag();
    if(next && item->IsFileItem())
    {
        SetNextCurrent(true);
    }
}

bool cBrowserBase::IsFlagged(cBrowserItem *item) const
{
    return (flaggedEntries_.find(item->GetId()) != flaggedEntries_.end());
}

bool cBrowserBase::AllIsFlagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsFlagged(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::NothingIsFlagged() const
{
    return flaggedEntries_.empty();
}

bool cBrowserBase::AllFilesAreFlagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetBrowserItem(i)->IsFileItem() && !IsFlagged(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::HasFlaggedFolderItems() const
{
    for(std::set<itemId>::const_iterator i = flaggedEntries_.begin(); i!= flaggedEntries_.end(); ++i)
    {
        if(GetFolderItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cBrowserBase::SingleFolderItemIsFlagged() const
{
    if(flaggedEntries_.size() == 1 &&  GetFolderItem(*flaggedEntries_.begin()))
    {
        return true;
    }
    return false;
}

void cBrowserBase::FlagAllFiles()
{
    cBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetBrowserItem(i);
        SetCurrent(Get(i));
        if(item->IsFileItem() && !IsMarked(item))
        {
            FlagItem(item);
        }
    }
    Display();
}

void cBrowserBase::ClearFlags(bool next)
{
    while(!flaggedEntries_.empty())
    {
        cBrowserItem *item = GetBrowserItem(*flaggedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetBrowserItemPos(*flaggedEntries_.begin())));
            RemoveFlag(item, next);
        }
	else
	    break;
    }
    flaggedEntries_.clear();
}

void cBrowserBase::ReFlagItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(flaggedEntries_.find(item->GetId()) != flaggedEntries_.end())
        {
            FlagItem(item);
        }
    }
}

void cBrowserBase::FlagUnflagCurrentItem()
{
    if(!IsFlagged(GetCurrentBrowserItem()))
    {
	//select line
	FlagItem(GetCurrentBrowserItem());
    }
    else
    {
	//deselect line
	RemoveFlag(GetCurrentBrowserItem());
    }
    Display();
}

void cBrowserBase::ClearAllFileFlags()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && IsFlagged(item))
        {
            RemoveFlag(item, false);
        }
    }
    Display();
}

void cBrowserBase::FlagUnflagAllFiles()
{
    if(AllFilesAreFlagged())
    {
        ClearAllFileFlags();
    }
    else
    {
        FlagAllFiles();
    }
    Display();
}

void cBrowserBase::TagItem(cBrowserItem *item, bool next)
{
    taggedEntries_.insert(item->GetId());
    item->Tag();
    if(next && item->IsFileItem())
    {
	SetNextCurrent(true);
    }
}

void cBrowserBase::RemoveTag(cBrowserItem *item, bool next)
{
    taggedEntries_.erase(item->GetId());
    item->UnTag();
    if(next && item->IsFileItem())
    {
        SetNextCurrent(true);
    }
}

bool cBrowserBase::IsTagged(cBrowserItem *item) const
{
    return (taggedEntries_.find(item->GetId()) != taggedEntries_.end());
}

bool cBrowserBase::AllIsTagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(IsTagged(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::NothingIsTagged() const
{
    return taggedEntries_.empty();
}

bool cBrowserBase::AllFilesAreTagged() const
{
    for (int i= 0; i < Count(); i++)
    {
        if(GetBrowserItem(i)->IsFileItem() && !IsTagged(GetBrowserItem(i)))
            return false;
    }
    return true;
}

bool cBrowserBase::HasTaggedFolderItems() const
{
    for(std::set<itemId>::const_iterator i = taggedEntries_.begin(); i!= taggedEntries_.end(); ++i)
    {
        if(GetFolderItem(*i))
        {
            return true;
        }
    }
    return false;
}

bool cBrowserBase::SingleFolderItemIsTagged() const
{
    if(taggedEntries_.size() == 1 &&  GetFolderItem(*taggedEntries_.begin()))
    {
        return true;
    }
    return false;
}

void cBrowserBase::TagAllFiles()
{
    cBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetBrowserItem(i);
        SetCurrent(Get(i));
        if(item->IsFileItem() && !IsMarked(item))
        {
            TagItem(item);
        }
    }
    Display();
}

void cBrowserBase::ClearTags(bool next)
{
    while(!taggedEntries_.empty())
    {
        cBrowserItem *item = GetBrowserItem(*taggedEntries_.begin());
        /* TB: after renaming the item has changed and is NULL */
        if(item)
        {
            SetCurrent(Get(GetBrowserItemPos(*taggedEntries_.begin())));
            RemoveTag(item, next);
        }
	else
	    break;
    }
    taggedEntries_.clear();
}

void cBrowserBase::ReTagItems()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(taggedEntries_.find(item->GetId()) != taggedEntries_.end())
        {
            TagItem(item);
        }
    }
}

void cBrowserBase::TagUntagCurrentItem()
{
    if(!IsTagged(GetCurrentBrowserItem()))
    {
	//select line
	TagItem(GetCurrentBrowserItem());
    }
    else
    {
	//deselect line
	RemoveTag(GetCurrentBrowserItem());
    }
    Display();
} 

void cBrowserBase::ClearAllFileTags()
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && IsTagged(item))
        {
            RemoveTag(item, false);
        }
    }
    Display();
}

void cBrowserBase::TagUntagAllFiles()
{
    if(AllFilesAreTagged())
    {
        ClearAllFileTags();
    }
    else
    {
        TagAllFiles();
    }
    Display();
}


void cBrowserBase::MarkAllNonFlaggedFiles()
{
   for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && !IsFlagged(item))
        {
	    RemoveTag(item, false);
            MarkItem(item, false);
        }
    } 
} 

bool cBrowserBase::HasUnMarkedOrUnFlaggedFileItems() const
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && !IsMarked(item) && !IsFlagged(item))
        {
            return true;
        }
    } 
    return false;
}  

void cBrowserBase::FlagAllTaggedFiles()
{
   for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && IsTagged(item))
        {
	    RemoveTag(item, false);
            FlagItem(item, false);
        }
    } 
}  

void cBrowserBase::MarkAllNonTaggedOrFlaggedFiles()
{
   for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item->IsFileItem() && !IsFlagged(item) && !IsTagged(item))
        {
            MarkItem(item, false);
        }
    } 
} 

void cBrowserBase::TagAllFlaggedFiles()
{
  for (int i = 0; i < Count(); i++)
  {
      cBrowserItem *item = GetBrowserItem(i);
      if(item->IsFileItem() && IsFlagged(item))
      {
	  RemoveFlag(item, false);
	  TagItem(item, false);
      }
  } 
}  

bool cBrowserBase::NoFiles() const
{
    cBrowserItem* item;
    for (int i= 0; i < Count(); i++)
    {
        item = GetBrowserItem(i);
        if(item && item->IsFileItem())
            return false;
    }
    return true;
}

void cBrowserBase::SetNextCurrent(bool up)
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

bool cBrowserBase::CurrentChanged()
{
    //printf("Current = %d, lastCurrent_ = %d\n", Current(), lastCurrent_ );
    if(Count() && lastCurrent_ != Current())
    {
        lastCurrent_ = Current();
        return true;
    }
    return false;
}

/*bool cBrowserBase::CurrentIfChanged()
{
    //printf("Current = %d, lastCurrent_ = %d\n", Current(), lastCurrent_ );
    if(lastUserIf_ != userIf_)
    {
        lastUserIf_ = userIf_;
        return true;
    }
    return false;
}*/


/*eOSState cBrowserBase::OpenCurrent(bool asPlaylistItem)
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Open(this, asPlaylistItem);
    }
    return osContinue;
}

eOSState cBrowserBase::OpenCurrentFolder()
{
    if(GetCurrentFolder())
    {
        return GetCurrentFolder()->Open(this);
    }
    return osContinue;
}*/

/*eOSState cMenuFileBrowserBase::UpdateCurrent(bool currentChanged)
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Update(this, currentChanged);
    }
    return osContinue;
}*/

/*eOSState cMenuFileBrowserBase::PlayCurrent()
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->Play(this);
    }
    return osContinue;
}*/

/*eOSState cMenuFileBrowserBase::OpenCurrentInfo()
{
    if(GetCurrentItem())
    {
        return GetCurrentItem()->OpenInfo();
    }
    return osContinue;
}*/

/*eOSState cMenuFileBrowserBase::SetInfoButton(bool currentChanged)
{
    if(currentChanged)
    {
	cFileItem *fileItem  = GetCurrentFileItem();
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
}*/  

cBrowserItem *cBrowserBase::GetBrowserItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cBrowserItem *item = GetBrowserItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}


cFileItem *cBrowserBase::GetFileItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cFileItem *item = GetFileItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}

cFolderItem *cBrowserBase::GetFolderItem(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
         cFolderItem *item = GetFolderItem(i);
         if(item && item->GetId() == id)
         {
             return item;
         }
    }
    return NULL;
}

int cBrowserBase::GetBrowserItemPos(itemId id) const
{
    for (int i = 0; i < Count(); i++)
    {
        cBrowserItem *item = GetBrowserItem(i);
        if(item && item->GetId() == id)
        {
            return i;
        }
    }
    return -1;
}

void cBrowserBase::SetCurrentItem(cOsdItem *item)
{
    SetCurrent(item);
}  

/*void cMenuFileBrowserBase::RefreshIfDirty()
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
}*/
