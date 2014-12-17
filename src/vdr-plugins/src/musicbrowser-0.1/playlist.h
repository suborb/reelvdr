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

#include <vdr/osdbase.h>

#include "../filebrowser/filetools.h"

//#include "browserItems.h"

class cBrowserMenu;
class cBrowserItem;

class itemId
{
    public:
    itemId(std::string path = "", int num = 0);
    bool operator==(const itemId &item) const;
    bool operator<(const itemId &item) const;
    std::string path_;
    ino_t st_ino_;
    long long size;
    int num_;
};

enum playlistMode
{
    multipleUnique,
    multiple,
    single
};

enum playlistType
{
    playable,
    xinePlayable,
    image
};

//-----------cPlayListItem---------------------

class cPlayListItem //object may be copied
{
public:
    cPlayListItem(const cBrowserItem &item);
    cPlayListItem(std::string path = "", int num = 0);
    cPlayListItem(itemId id);
    ~cPlayListItem();
    itemId GetId() const; 
    std::string GetPath() const;
    int GetNum() const;
    eOSState Open(cBrowserMenu *menu, bool asPlaylist = false);
    //eOSState Update(cMenuFileBrowserBase *menu, bool currentChanged);
    //eOSState Play(cMenuFileBrowserBase *menu, bool asPlaylist = false);
    bool operator==(const cPlayListItem &item) const;
    bool operator<(const cPlayListItem &item) const;

//private:
    itemId id_;
};

class cPlayList 
{

public:
    cPlayList(playlistType type = playable, std::string name = "", std::string basedir = "");
    ~cPlayList(); 
    static std::string GetDefaultName(std::string basedir = "");
    bool CurrentIsSet() const;
    bool IsInList(const cPlayListItem &item) const;
    bool Insert(const cPlayListItem &item, playlistMode mode = single);//multipleUnique);
    bool Insert(const cBrowserItem *item, playlistMode mode = single);//multipleUnique);
    bool Insert(const std::set<cBrowserItem*> &markedEntries, playlistMode mode = single);// multipleUnique);
    //bool Insert(const std::set<itemId> &markedEntries, playlistMode mode =  multipleUnique);
    void Remove(const cPlayListItem &item);
    void Remove(const cBrowserItem *item);
    void Append(const cPlayList& newList);  
    cPlayListItem GetCurrent() const;
    cPlayListItem GetNthItem(uint n) const;
    cPlayListItem NextRandom();
    cPlayListItem NextCyclic(bool forward);
    bool Next(bool forward);
    bool Next(cPlayListItem &item, bool forward);
    bool NextPlayableItem(bool forward);
    void ShiftItem(cPlayListItem item, bool up);
    bool IsEmpty() const;
    bool IsDirty() const;
    int Size() const;
    void SetCurrent(const cPlayListItem &item, bool any = false);
    bool LoadFromFile(const cPlayListItem &item);
    bool SaveToFile(std::string fileName);
    std::string SaveToFile();
    void CopyPlayListToCD();
    void Clear(std::string basedir = "");
    std::string GetName() const;
    int GetPos(const cPlayListItem &item) const;
    void SetName(std::string name);
    std::vector<std::string> GetPathList() const;
    void SetPlayListEntries(std::vector<std::string> entries);
    void SetType(playlistType type) {type_ = type;}

private: 
    //forbid copying
    //cPlayList(const cPlayList &);
    //cPlayList &operator=(const cPlayList &);

    std::list<cPlayListItem> list_;
    std::set<cPlayListItem> set_;
    std::list<cPlayListItem>::iterator current;
    bool currentIsSet;
    playlistType type_;
    std::string basedir_;
    std::string name_;
    int uniqueNum_;
    bool isDirty_;
    bool isNew_;
    std::list<cPlayListItem>::iterator GetIterator(const cPlayListItem &item);
    cPlayListItem MakeUnique(const cPlayListItem &item);
    //bool InsertDirItemsRecursively(const std::string &path, playlistMode = multipleUnique, bool remove = false);
};

#endif //P__PLAYLIST_H
