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

 
#ifndef P__PLAYLIST_H
#define P__PLAYLIST_H
 
#include <string> 
#include <list>
#include <set>

#include "fileType.h"

class cMenuFileBrowserBase;
class cMenuPlayList;
class cMenuBrowserItem;
class cMenuDirEntryItem;
class cMenuFileItem;

class itemId
{
    public:
    itemId(std::string path = "", int num = 0);
    bool operator==(const itemId &item) const;
    bool operator<(const itemId &item) const;
    std::string path_;
    int num_;
};

//-----------cPlayListItem---------------------

class cPlayListItem //object may be copied
{
public:
    cPlayListItem(const cMenuFileItem &item);
    cPlayListItem(std::string path = "", int num = 0);
    cPlayListItem(itemId id);
    ~cPlayListItem();
    itemId GetId() const; 
    std::string GetPath() const;
    int GetNum() const;
    bool IsPlayable();
    eOSState Open(cMenuFileBrowserBase *menu, bool asPlaylist = false);
    eOSState Update(cMenuFileBrowserBase *menu, bool currentChanged);
    cFileType GetFileType() const;
    bool operator==(const cPlayListItem &item) const;
    bool operator<(const cPlayListItem &item) const;

private:
    cFileType type_;
    itemId id_;
};

class cPlayList 
{
friend class cMenuPlayList;

public:
    cPlayList(std::string basedir = "");
    ~cPlayList(); 
    static std::string GetDefaultName(std::string basedir = "");
    bool CurrentIsSet() const;
    bool IsInList(const cPlayListItem &item) const;
    bool Insert(const cPlayListItem &item, bool unique = true);
    bool Insert(const cMenuDirEntryItem *item);
    bool Insert(const std::set<cMenuBrowserItem*> &markedEntries);
    bool Insert(const std::set<itemId> &markedEntries);
    void Remove(const cPlayListItem &item);
    cPlayListItem GetCurrent() const;
    cPlayListItem GetNthItem(uint n) const;
    cPlayListItem NextRandom();
    cPlayListItem NextCyclic(bool forward);
    bool Next(bool forward);
    bool Next(cPlayListItem &item, bool forward);
    bool NextPlayableItem(bool forward);
    bool NextItemOfSameClass(const cPlayListItem &item, bool forward);
    bool NextItemOfClass(efileclass fclass, bool forward);
    void ShiftItem(cPlayListItem item, bool up);
    bool IsEmpty() const;
    bool IsDirty() const;
    int Size() const;
    void SetCurrent(const cPlayListItem &item, bool any = false);
    bool LoadFromFile(const cPlayListItem &item);
    bool SaveToFile(std::string fileName);
    bool SaveToFile();
    void Clear(std::string basedir = "");
    std::string GetName() const;
    int GetPos(const cPlayListItem &item) const;
    void SetName(std::string name);
    std::vector<std::string> GetPathList() const;
    void GetPartialListOfSimilarItems(const cPlayListItem &item, cPlayList &partialList);

private: 
    //forbid copying
    cPlayList(const cPlayList &);
    cPlayList &operator=(const cPlayList &);

    std::list<cPlayListItem> list_;
    std::set<cPlayListItem> set_;
    std::list<cPlayListItem>::iterator current;
    bool currentIsSet;
    std::string name_;
    int uniqueNum_;
    bool isDirty_;
    bool isNew_;
    std::list<cPlayListItem>::iterator GetIterator(const cPlayListItem &item);
    cPlayListItem MakeUnique(const cPlayListItem &item);
    bool InsertDirItemsRecursively(const std::string &path);
};

#endif //P__PLAYLIST_H
