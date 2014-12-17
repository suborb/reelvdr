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

#ifndef P__MENUPLAYLIST_H
#define P__MENUPLAYLIST_H

#include <string>

#include "browserItems.h"
#include "playlist.h"
#include "menuBrowserBase.h"
//#include "menuStrItemBase.h"
#include "playlistUserIf.h"
#include "stringtools.h"

class cImageConvert;
class cFileCache;

//-----------------cMenuPlayList----------------------

class cMenuPlayList : public cMenuFileBrowserBase
{
public:
    cMenuPlayList(std::string title, cPlayList *playlist, cFileCache *cache, cImageConvert *convert, bool play = false);
    ~cMenuPlayList();
    /*override*/ void Shutdown(); 
    /*override*/ eOSState ProcessKey(eKeys Key);
    /*override*/ eOSState OpenFileInfo(const cMenuFileItem &file){return osContinue;}
    /*override*/ eOSState ShowFileWithImageViewer(cPlayListItem &item, bool asPlaylist = false);
    /*override*/ eOSState ShowFileWithXine(const cPlayListItem &item, bool asPlaylist = false);
    /*override*/ std::string GetAdjustedEntryName(std::string path) const;
    /*override*/ bool Refresh(itemId = itemId(), bool initial = false);

    //called from user if 
    eOSState ShowPlayListWithXine();
    void Hide(bool hide);
    eOSState StandardKeyHandling(eKeys Key);
    eOSState SetAndOpenCurrent();
    void ShiftCurrent(bool up);
    void DeleteCurrentOrMarked();
    void DeleteMarkedEntries();
    void DeleteCurrentEntry();
    void ClosePlayList();
    void SavePlayList();
    void SetName(const std::string &name);
    std::string GetName() const;
    std::string GetTitleString() const;
    bool PlayListIsDirty();
    void SetMode(enum browseMode mode);
    void CopyPlayListToCD();
    void SetCols(int c0, int c1 = 0, int c2 = 0, int c3 = 0, int c4 = 0) { cOsdMenu::SetCols(c0, c1, c2, c3, c4); };

    cMenuPlayListBaseIf *userIf_;

private:
    //forbid copying
    cMenuPlayList(cMenuPlayList &);
    cMenuPlayList &operator=(const cMenuPlayList &);
    cPlayList *playlist_;
    static cPlayList activePlaylist;
    cFileCache *cache_; 
    cImageConvert *convert_;
    std::string name_;

    void OpenPlayList();
    int GetNextPlayableItemCyclic(cMenuFileItem *currentitem, bool all) const;
    bool GetNextPlayableItem(cMenuFileItem *item, int &next, bool all) const;
    bool SetCurrentToNextPlayableItem(bool all);
    void SetCurrentToItem(cPlayListItem item);
};

//---------------cMenuPlayList----Inline functions----------

inline std::string cMenuPlayList::GetAdjustedEntryName(std::string path) const
{
    return GetLast(path);
} 

#endif //P__MENUPLAYLIST_H
