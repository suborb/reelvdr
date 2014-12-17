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

 
#ifndef P__BROWSERBASE_H
#define P__BROWSERBASE_H
 
#include <string>
#include <set>

#include <vdr/osdbase.h>

#include "browserItems.h"
#include "browserStatus.h"

//----------cSetHelpWrapper---------------------------------

class cBrowserStatus;
class cBrowserBase;
class cBrowserBaseIf;

class cSetHelpWrapper
{
    public:
    cSetHelpWrapper(cBrowserBase *menu)
    : menu_(menu){};
    void SetRed(const std::string &msg);
    void SetGreen(const std::string &msg);
    void SetYellow(const std::string &msg);
    void SetBlue(const std::string &msg);
    void SetInfo(const bool msg);
    void FlushButtons(bool forceFlush = false);

    private:
    cBrowserBase *menu_;
    std::string btTitles[4];
    std::string newBtTitles[4];
    bool btInfo, newBtInfo;
    const char *GetStr(const std::string &msg);
};

//--------------ceBrowserBase-----------------------

class cBrowserBase : public cOsdMenu
{
public:
    cBrowserBase(const std::string title);   
    virtual ~cBrowserBase(){};
#if APIVERSNUM < 10700
    void SetHelpKeys(const char *redStr, const char *greenStr, const char *yellowStr, const char *blueStr);
#else
    void SetHelpKeys(const char *redStr, const char *greenStr, const char *yellowStr, const char *blueStr, const eKeys *info);
#endif

    void MarkItem(cBrowserItem *item, bool next = true);
    void RemoveMark(cBrowserItem *item, bool next = true);
    void MarkAllFiles();
    bool IsMarked(cBrowserItem *item) const;
    bool NothingIsMarked() const;
    bool AllIsMarked() const;
    bool AllFilesAreMarked() const; 
    bool HasMarkedFolderItems() const;
    bool SingleFolderItemIsMarked() const;
    void ClearMarks(bool next = true);
    void ReMarkItems();
    void MarkUnmarkCurrentItem();
    void ClearAllFileMarks();
    void MarkUnmarkAllFiles();
    
    void FlagItem(cBrowserItem *item, bool next = true);
    void RemoveFlag(cBrowserItem *item, bool next = true);
    void FlagAllFiles();
    bool IsFlagged(cBrowserItem *item) const;
    bool NothingIsFlagged() const;
    bool AllIsFlagged() const;
    bool AllFilesAreFlagged() const; 
    bool HasFlaggedFolderItems() const;
    bool SingleFolderItemIsFlagged() const;;
    void FlagUnflagCurrentItem();
    void ClearAllFileFlags();
    void FlagUnflagAllFiles();
    void ClearFlags(bool next = true);
    void ReFlagItems();
 
    void TagItem(cBrowserItem *item, bool next = true);
    void RemoveTag(cBrowserItem *item, bool next = true);
    void TagAllFiles();
    bool IsTagged(cBrowserItem *item) const;
    bool NothingIsTagged() const;
    bool AllIsTagged() const;
    bool AllFilesAreTagged() const; 
    bool HasTaggedFolderItems() const;
    bool SingleFolderItemIsTagged() const;
    void TagUntagCurrentItem();
    void ClearAllFileTags();
    void TagUntagAllFiles();
    void ClearTags(bool next = true);
    void ReTagItems();
    
    void MarkAllNonFlaggedFiles();
    bool HasUnMarkedOrUnFlaggedFileItems() const;
    void FlagAllTaggedFiles();
    void MarkAllNonTaggedOrFlaggedFiles();
    void TagAllFlaggedFiles();
    
    bool NoFiles() const;
    void SetNextCurrent(bool up);
        
    bool CurrentChanged();
    //bool CurrentIfChanged();
    
    //eOSState SetInfoButton(bool currentChanged);
    cBrowserItem *GetBrowserItem(int i) const;
    cFolderItem *GetFolderItem(int i) const;
    cFileItem *GetFileItem(int i) const; 
    cBrowserItem *GetBrowserItem(itemId id) const; 
    cFolderItem *GetFolderItem(itemId id) const;
    
    cFileItem *GetFileItem(itemId id) const;
    cBrowserBaseItem *GetCurrentBaseItem() const;
    cBrowserItem *GetCurrentBrowserItem() const;
    cFolderItem  *GetCurrentFolderItem() const;
    cFileItem *GetCurrentFileItem() const; 
    cFolderItem *GetSingleMarkedFolderItem() const; 
    cFileItem *GetSingleMarkedFileItem() const;
    int GetBrowserItemPos(itemId id) const;
    
    void SetCurrentItem(cOsdItem *item);
    
    //OSState OpenCurrent(bool asPlaylistItem);
    //eOSState OpenCurrentFolder();
    
    cSetHelpWrapper setHelp;

protected:
    std::set<itemId> markedEntries_;
    std::set<itemId> flaggedEntries_;
    std::set<itemId> taggedEntries_;
    int lastCurrent_;
    bool hidden; 
    
    cBrowserBaseIf *userIf_;
    cBrowserBaseIf *lastUserIf_;
    bool userIfchanged_;
};

//----------cMenuFileBrowserBase-----Inline Functions---------

inline cBrowserBase::cBrowserBase(const std::string title)
    :  cOsdMenu(title.c_str()), setHelp(this), lastCurrent_(-1), hidden (false), userIf_(0), lastUserIf_(0), userIfchanged_(false)
{
}

inline void cBrowserBase::SetHelpKeys(const char *redStr,
const char *greenStr, const char *yellowStr, const char *blueStr
#if APIVERSNUM > 10700
, const eKeys *info
#endif
)
{
#if APIVERSNUM < 10700
    SetHelp(redStr, greenStr, yellowStr, blueStr);
#else
    SetHelp(redStr, greenStr, yellowStr, blueStr, info);
#endif
}

inline cBrowserItem *cBrowserBase::GetBrowserItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cBrowserItem*>(Get(i));
    }
    return NULL;
}

inline cFileItem *cBrowserBase::GetFileItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cFileItem*>(Get(i));
    }
    return NULL;
}

inline cFolderItem *cBrowserBase::GetFolderItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cFolderItem*>(Get(i));
    }
    return NULL;
}

inline cBrowserBaseItem *cBrowserBase::GetCurrentBaseItem() const
{
    if(Count())
    {
        return dynamic_cast<cBrowserBaseItem*>(Get(Current()));
    }
    return NULL;
}

inline cBrowserItem *cBrowserBase::GetCurrentBrowserItem() const
{
    if(Count())
    {
        return dynamic_cast<cBrowserItem*>(Get(Current()));
    }
    return NULL;
}


inline cFileItem *cBrowserBase::GetCurrentFileItem() const
{
    if(Count())
    {
        return dynamic_cast<cFileItem*>(Get(Current()));
    }
    return NULL;
}

inline cFolderItem *cBrowserBase::GetCurrentFolderItem() const
{
    if(Count())
    {
        return dynamic_cast<cFolderItem*>(Get(Current()));
    }
    return NULL;
}


#endif //P__BROWSERBASE_H
