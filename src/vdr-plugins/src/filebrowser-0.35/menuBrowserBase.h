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

 
#ifndef P__MENUBROWSERBASE_H
#define P__MENUBROWSERBASE_H
 
#include <string>
#include <set>

#include <vdr/osdbase.h>

#include "browserItems.h"
#include "browserStatus.h"

//----------cSetHelpWrapper---------------------------------

class cFileBrowserStatus;

class cSetHelpWrapper
{
    public:
    cSetHelpWrapper(cMenuFileBrowserBase *menu)
    : menu_(menu){};
    void SetRed(const std::string &msg);
    void SetGreen(const std::string &msg);
    void SetYellow(const std::string &msg);
    void SetBlue(const std::string &msg);
    void SetInfo(const bool msg);
    void FlushButtons(bool forceFlush = false);

    private:
    cMenuFileBrowserBase *menu_;
    std::string btTitles[4];
    std::string newBtTitles[4];
    bool btInfo, newBtInfo;
    const char *GetStr(const std::string &msg);
};


/*class cMarkedEntries
{
    std::set<itemId> markedEntries_;
    
    public:
      
    void MarkItem(cMenuBrowserItem *item);
    void RemoveMark(cMenuBrowserItem *item);
    void MarkAllFiles();
    bool IsMarked(cMenuBrowserItem *item) const;
    bool NothingIsMarked() const;
    bool AllIsMarked() const;
    bool AllFilesAreMarked() const; 
    bool HasMarkedDirEntryItems() const;
    bool SingleDirEntryItemIsMarked() const;
    void ClearMarks();
    void ReMarkItems();
    void MarkUnmarkCurrentItem();
    void MarkUnmarkAllFiles();    
}  */

//--------------cMenuFileBrowserBase-----------------------

class cMenuFileBrowserBase : public cOsdMenu
{
public:
    cMenuFileBrowserBase(const std::string title);   
    virtual ~cMenuFileBrowserBase(){};
    virtual void Shutdown(){};
    virtual eOSState ShowFileWithImageViewer(cPlayListItem &item, bool asPlaylist = false){ return osContinue; }
    virtual eOSState ShowFileWithXine(const cPlayListItem &item, bool asPlaylist = false, bool passInstance = true, bool showImmediately = false){ return osContinue; }
    virtual eOSState ShowDvdWithXine(const cPlayListItem &item, bool asPlaylist = false){ return osContinue; }
    virtual eOSState ShowThumbnail(cPlayListItem item, bool hide, bool currentChanged) const { return osContinue;}
    virtual eOSState ShowID3TagInfo(cPlayListItem item, bool hide, bool currentChanched){ return osContinue;}
    virtual eOSState OpenFileInfo(const cMenuFileItem &file){ return osContinue; }
    virtual eOSState EnterSubDir(std::string pathname){ return osContinue; }
    virtual eOSState LoadAndEnterPlayList(cPlayListItem item){ return osContinue; }  
    virtual eOSState InstallDebPacket(const std::string &file){ return osContinue; }
    virtual std::string GetAdjustedEntryName(std::string path) const { return path; }
    virtual std::string GetAdjustedNumberedEntryName(std::string path) const { return path; }
    virtual bool Refresh(itemId = itemId(), bool initial = false){ return true; }
#if APIVERSNUM < 10700
    void SetHelpKeys(const char *redStr, const char *greenStr, const char *yellowStr, const char *blueStr);
#else
    void SetHelpKeys(const char *redStr, const char *greenStr, const char *yellowStr, const char *blueStr, const eKeys *info);
#endif
    void XineStarted(bool withPlaylist);

    void MarkItem(cMenuBrowserItem *item, bool next = true);
    void RemoveMark(cMenuBrowserItem *item, bool next = true);
    void MarkAllFiles(bool onlyXinePlayable = false);
    bool IsMarked(cMenuBrowserItem *item) const;
    bool NothingIsMarked() const;
    bool AllIsMarked() const;
    bool AllFilesAreMarked(bool onlyXinePlayable = false) const; 
    bool HasMarkedDirEntryItems() const;
    bool SingleDirEntryItemIsMarked() const;
    void ClearMarks(bool next = true);
    void ReMarkItems();
    void MarkUnmarkCurrentItem();
    void ClearAllFileMarks(bool onlyXinePlayable = false);
    void MarkUnmarkAllFiles(bool onlyXinePlayable = false);
    
    void FlagItem(cMenuBrowserItem *item, bool next = true);
    void RemoveFlag(cMenuBrowserItem *item, bool next = true);
    void FlagAllFiles(bool onlyXinePlayable = false);
    bool IsFlagged(cMenuBrowserItem *item) const;
    bool NothingIsFlagged() const;
    bool AllIsFlagged() const;
    bool AllFilesAreFlagged(bool onlyXinePlayable = false) const; 
    bool HasFlaggedDirEntryItems() const;
    bool SingleDirEntryItemIsFlagged() const;;
    void FlagUnflagCurrentItem();
    void ClearAllFileFlags(bool onlyXinePlayable = false);
    void FlagUnflagAllFiles(bool onlyXinePlayable = false);
    void ClearFlags(bool next = true);
    void ReFlagItems();
 
    void TagItem(cMenuBrowserItem *item, bool next = true);
    void RemoveTag(cMenuBrowserItem *item, bool next = true);
    void TagAllFiles(bool onlyXinePlayable = false);
    bool IsTagged(cMenuBrowserItem *item) const;
    bool NothingIsTagged() const;
    bool AllIsTagged() const;
    bool AllFilesAreTagged(bool onlyXinePlayable = false) const; 
    bool HasTaggedDirEntryItems() const;
    bool SingleDirEntryItemIsTagged() const;
    void TagUntagCurrentItem();
    void ClearAllFileTags(bool onlyXinePlayable = false);
    void TagUntagAllFiles(bool onlyXinePlayable = false);
    void ClearTags(bool next = true);
    void ReTagItems();
    
    void MarkAllNonFlaggedFiles(bool onlyXinePlayable = false);
    bool HasUnMarkedOrUnFlaggedFileItems(bool onlyXinePlayable = false) const;
    void FlagAllTaggedFiles(bool onlyXinePlayable = false);
    void MarkAllNonTaggedOrFlaggedFiles(bool onlyXinePlayable = false);
    void TagAllFlaggedFiles(bool onlyXinePlayable = false);
    
    bool NoFiles() const;
    void SetNextCurrent(bool up);
        
    bool CurrentChanged();
    eOSState OpenCurrent(bool asPlaylist = false);
    eOSState OpenCurrentDir();
    eOSState UpdateCurrent(bool currentChanged);
    eOSState PlayCurrent();
    eOSState OpenCurrentInfo();
    eOSState SetInfoButton(bool currentChanged);
    cMenuBrowserItem *GetItem(int i) const;
    cMenuDirEntryItem *GetDirEntryItem(int i) const;
    cMenuFileItem *GetFileItem(int i) const; 
    cMenuBrowserItem *GetItem(itemId id) const; 
    cMenuDirEntryItem *GetDirEntryItem(itemId id) const;
    cMenuFileItem *GetFileItem(itemId id) const;
    cMenuBrowserItem *GetCurrentItem() const;
    cMenuDirEntryItem  *GetCurrentDirEntryItem() const;
    cMenuFileItem *GetCurrentFileItem() const; 
    cMenuDirItem *GetCurrentDirItem() const;
    cMenuDirEntryItem *GetSingleMarkedDirEntryItem() const; 
    cMenuFileItem *GetSingleMarkedFileItem() const;
    int GetItemPos(itemId id) const;
    std::string GetCurrentItemPath() const; 
    //std::string GetCurrentDirEntryPath() const; 
    //std::string GetCurrentFileItemPath() const;
    eOSState AddFileBrowserSubMenu(cMenuFileBrowserBase *subMenu);
    cMenuFileBrowserBase *GetBrowserSubMenu();
    void ShutdownBrowserSubMenu();
    void RefreshIfDirty();
    void SetTitleString(std::string);
    void SetMode(browseMode mode);
    browseMode GetMode() const;
    cFileBrowserStatus *GetStatus() { return status; }
    
    const char *Hk(const char *s) { return hk(s); } //make hotkeys available for user ifs

    cSetHelpWrapper setHelp;

protected:
    std::set<itemId> markedEntries_;
    std::set<itemId> flaggedEntries_;
    std::set<itemId> taggedEntries_;
    static cFileBrowserStatus *status;
    void BuildActualPlayList(const cPlayListItem &item, cPlayList &playlist, bool all = false);
    bool hidden; 
    int lastCurrent_;

private:
    bool dirty_;
    cMenuFileBrowserBase *subMenu_;
};

//----------cMenuFileBrowserBase-----Inline Functions---------

inline cMenuFileBrowserBase::cMenuFileBrowserBase(const std::string title)
    :  cOsdMenu(title.c_str()), setHelp(this), hidden(false), lastCurrent_(-1), dirty_(true), subMenu_(NULL)
{
}

inline eOSState cMenuFileBrowserBase::AddFileBrowserSubMenu(cMenuFileBrowserBase *subMenu)
{
    dirty_ = true;
    subMenu_ = subMenu;
    return AddSubMenu(subMenu);
}

inline cMenuFileBrowserBase *cMenuFileBrowserBase::GetBrowserSubMenu()
{
    if(HasSubMenu())
    {
        return subMenu_;
    }
    else
    {
        return NULL;
    }
    //return dynamic_cast<cMenuFileBrowserBase*>(subMenu);
}

inline void cMenuFileBrowserBase::ShutdownBrowserSubMenu()
{
    if(HasSubMenu())
    {
        subMenu_->Shutdown();
    } 
    /*cMenuBrowserBase *subMenu = GetBrowserSubMenu();
    if(subMenu)
    {
        subMenu->Shutdown();
    }*/
}

inline void cMenuFileBrowserBase::SetHelpKeys(const char *redStr,
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

inline cMenuBrowserItem *cMenuFileBrowserBase::GetItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cMenuBrowserItem*>(Get(i));
    }
    return NULL;
}

inline cMenuDirEntryItem *cMenuFileBrowserBase::GetDirEntryItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cMenuDirEntryItem*>(Get(i));
    }
    return NULL;
}

inline cMenuFileItem *cMenuFileBrowserBase::GetFileItem(int i) const
{
    if(Count())
    {
        return dynamic_cast<cMenuFileItem*>(Get(i));
    }
    return NULL;
}

inline cMenuBrowserItem *cMenuFileBrowserBase::GetCurrentItem() const
{
    if(Count())
    {
        return dynamic_cast<cMenuBrowserItem*>(Get(Current()));
    }
    return NULL;
}

inline cMenuDirEntryItem *cMenuFileBrowserBase::GetCurrentDirEntryItem() const
{
    if(Count())
    {
        return dynamic_cast<cMenuDirEntryItem*>(Get(Current()));
    }
    return NULL;
}

inline cMenuFileItem *cMenuFileBrowserBase::GetCurrentFileItem() const
{
    if(Count())
    {
        return dynamic_cast<cMenuFileItem*>(Get(Current()));
    }
    return NULL;
}

inline cMenuDirItem *cMenuFileBrowserBase::GetCurrentDirItem() const
{
    if(Count())
    {
        return dynamic_cast<cMenuDirItem*>(Get(Current()));
    }
    return NULL;
}

inline std::string cMenuFileBrowserBase::GetCurrentItemPath() const
{
    std::string path;
    cMenuBrowserItem *item = GetCurrentItem();
    if (item)
    {
        path = item->GetPath();
    }
    return path;
}

#endif //P__MENUBROWSERBASE_H
