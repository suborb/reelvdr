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

#ifndef P__MENUBROWSER_H
#define P__MENUBROWSER_H

#include <string>
#include <vector>

#include "browserItems.h"
#include "mountmanager.h"
#include "threadprovider.h"
#include "playlist.h"
#include "menuBrowserBase.h"

class cImageConvert;
class cThumbConvert;
class cFileCache;
class cMenuBrowserBaseIf;

//----------cMenuFileBrowser-----------------------

class cMenuFileBrowser : public  cMenuFileBrowserBase
{
public:
    cMenuFileBrowser(std::string title, cFileBrowserStatus *newstatus, cFileCache *cache, cImageConvert *convert, cThumbConvert *thumbConvert);
    ~cMenuFileBrowser();
    /*override*/ void Shutdown();
    /*override*/  eOSState ProcessKey(eKeys Key); 

    //functions are called from menuitems
    /*override*/ eOSState ShowThumbnail(cPlayListItem item, bool hide, bool currentChanged);
    /*override*/ eOSState ShowID3TagInfo(cPlayListItem item, bool hide, bool currentChanched);
    /*override*/ eOSState ShowFileWithImageViewer(cPlayListItem &item, bool asPlaylist = false);	
    /*override*/ eOSState ShowFileWithXine(const cPlayListItem &item, bool asPlaylist = false);
    /*override*/ eOSState ShowDvdWithXine(const cPlayListItem &item, bool asPlaylist = false);
    /*override*/ eOSState OpenFileInfo(const cMenuFileItem &file);
    /*override*/ eOSState EnterSubDir(std::string pathname);
    /*override*/ eOSState LoadAndEnterPlayList(cPlayListItem item);
    /*override*/ std::string GetAdjustedEntryName(std::string path) const;
    /*override*/ std::string GetAdjustedNumberedEntryName(std::string path) const;
    /*override*/ bool Refresh(itemId = itemId(), bool initial = false);

    //functions are called from user IF 
    eOSState OpenMarked(bool &playlistEntered);
    eOSState EnterPlayList(bool play = false); 
    eOSState ShowPlayListWithXine();
    eOSState ExitDir(bool fromItem = false);
    eOSState StandardKeyHandling(eKeys Key);
    void GetFinishedTasks();
    void DeleteMarkedEntries();
    void DeleteMarkedOrCurrent();
    void InsertCopiedEntries();
    void InsertMovedEntries();
    void CopyMarkedEntries();
    void CopyMarkedOrCurrent();
    bool RenameEntry(std::string oldname, std::string newname);
    bool CreateDirectory(std::string name); 
    bool ConvertToUTF8();
    bool BurnIso(); 
    bool MakeAndBurnIso();
    bool InsertCurrentInPlayList();
    bool InsertMarkedEntriesInPlayList();
    bool InsertMarkedOrCurrentInPlayList();
    bool HasUnfinishedTasks(); 
    void Hide(bool hide); 
    bool ChangeDir(std::string pathname, std::string currentItem = "");
    void ClearPlayList();
    void SavePlayList(std::string &path);
    void CopyPlayListToUsb();
    void CopyPlayListToDvd();
    bool PlayListIsEmpty() const;
    std::string GetTitleString() const;
    std::string GetPreTitle() const;
    std::string GetExternalActionString() const;
    FileBrowserExternalActionInfo *GetExternalActionInfo() const;
    void SetCols(int c0, int c1 = 0, int c2 = 0, int c3 = 0, int c4 = 0) { cOsdMenu::SetCols(c0, c1, c2, c3, c4); };

    cMenuBrowserBaseIf* userIf_;

private:
    std::string currentdir_;
    cFileCache *cache_;
    cImageConvert *convert_;
    cThumbConvert *thumbConvert_;
    cMountManager mountMgr_;
    std::vector<std::string> copiedEntries_;
    cPlayList *playlist_;
    static cPlayList actualPlaylist;
    static cThreadProvider provider_;

    //forbid copying
    cMenuFileBrowser(const cMenuFileBrowser &);
    cMenuFileBrowser &operator=(const cMenuFileBrowser &);

    bool OpenDir();
    void Paste(bool remove);
    void InsertItemsInConversionList(cImageConvert *convert) const;
    void RemoveItemsFromConversionList(cImageConvert *convert) const;
    void LoadPlayList(cPlayListItem item);
    void FlagItems();

    //special entry name handling
    bool IsForbiddenDirEntry(std::string path) const;
    bool IsAllowedDirEntry(std::string entry) const;
    bool IsHiddenDirEntry(std::string path) const;
    bool IsInsideFilter(cMenuFileItem &item) const;
    void RepositionDirEntries(std::vector<cMenuDirItem*> &direntries);
    void AdjustEntries(std::vector<cMenuDirItem*> &direntries, std::vector<cMenuFileItem*> &fileentries);
};

#endif //P__MENUBROWSER_H
