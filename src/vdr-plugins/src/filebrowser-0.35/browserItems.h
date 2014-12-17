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

 
#ifndef P__BROWSERITEMS_H
#define P__BROWSERITEMS_H
 
#include <string> 

#include <vdr/menuitems.h>
#include <vdr/osdbase.h>
#include <vdr/menu.h>

#include "fileType.h"
#include "dirType.h"
#include "playlist.h"

class cMenuFileBrowserBase;
class cImageConvert;

//-----------cMenuBrowserItem---------------------

class cMenuBrowserItem  
{
public:
    cMenuBrowserItem(cMenuFileBrowserBase* Menu)
    : menu(Menu), isMarked(false), isFlagged(false), isTagged(false) {}
    virtual ~cMenuBrowserItem(){}; 
    virtual bool IsFile() const {return false;}
    virtual bool IsDir() const {return false;}
    virtual bool IsEditStrItem() const {return false;}
    virtual std::string GetPath() const {return std::string();}
    virtual itemId GetId() const {return itemId();}
    virtual eOSState Open(cMenuFileBrowserBase *menu, bool asPlayListItem = false) {return osContinue;}
    virtual eOSState Play(cMenuFileBrowserBase *menu) {return osContinue;}
    virtual eOSState Update(cMenuFileBrowserBase *menu, bool currentChanched);
    virtual eOSState OpenInfo() const {return osContinue;}
    virtual void Mark(){}
    virtual void UnMark(){}
    virtual void Flag(){}
    virtual void UnFlag(){}
    virtual void Tag(){}
    virtual void UnTag(){}

protected:
    cMenuFileBrowserBase* menu;
    bool isMarked;
    bool isFlagged;
    bool isTagged;

private: 
    //forbid copying
    cMenuBrowserItem(const cMenuBrowserItem &);
    cMenuBrowserItem &operator=(const cMenuBrowserItem &);
};

//-----------cMenuBrowserEditStrItem---------------------

class cMenuBrowserEditStrItem : public cMenuBrowserItem, public cMenuEditStrItem
{
public:
    cMenuBrowserEditStrItem(cMenuFileBrowserBase* menu,const char *Name, char *Value, int Length, 
    const char *Allowed = NULL);
    /*override*/ ~cMenuBrowserEditStrItem(){};
    /*override*/ bool IsEditStrItem() const {return true;}

private: 
    //forbid copying
    cMenuBrowserEditStrItem(const cMenuBrowserEditStrItem &);
    cMenuBrowserEditStrItem &operator=(const cMenuBrowserEditStrItem &);
};

//-----------cMenuEmptyItem---------------------

class cMenuEmptyItem : public cMenuBrowserItem, public cOsdItem
{
public:
    cMenuEmptyItem(cMenuFileBrowserBase* menu);
    /*override*/ ~cMenuEmptyItem(){}

private: 
    //forbid copying
    cMenuEmptyItem(const cMenuEmptyItem &);
    cMenuEmptyItem &operator=(const cMenuEmptyItem &);
};

//-----------cMenuDirEntryItem---------------------

class cMenuDirEntryItem : public cMenuBrowserItem, public cOsdItem
{
public:
    cMenuDirEntryItem(std::string path, cMenuFileBrowserBase* menu);
    cMenuDirEntryItem(const itemId &id, cMenuFileBrowserBase* menu);
    /*override*/ ~cMenuDirEntryItem(){}
    /*override*/ std::string GetPath() const;
    /*override*/ itemId GetId() const;
    /*override*/ void Mark();
    /*override*/ void UnMark();
    /*override*/ void Flag();
    /*override*/ void UnFlag();
    /*override*/ void Tag();
    /*override*/ void UnTag();

protected:
    itemId id_;

private: 
    //forbid copying
    cMenuDirEntryItem(const cMenuDirEntryItem &);
    cMenuDirEntryItem &operator=(const cMenuDirEntryItem &);
};

//-----------cMenuDirItem------------------------------

class cMenuDirItem : public cMenuDirEntryItem
{
public:
    cMenuDirItem(std::string path, cMenuFileBrowserBase* menu);
    /*override*/ ~cMenuDirItem(){};
    /*override*/ void Set();
    /*override*/ bool IsDir() const {return true;}
    /*override*/ eOSState Open(cMenuFileBrowserBase *menu, bool asPlayListItem = false);
    /*override*/ eOSState Play(cMenuFileBrowserBase *menu);
    /*override*/ eOSState OpenInfo() const; 

protected:
    cDirType type_;

private:
    //forbid copying
    cMenuDirItem(const cMenuDirItem &);
    cMenuDirItem &operator=(const cMenuDirItem &); 
};

//-----------cMenuFileItem------------------------------

class cMenuFileItem : public cMenuDirEntryItem 
{
public:
    cMenuFileItem(std::string path, cMenuFileBrowserBase *menu);
    cMenuFileItem(cPlayListItem item, cMenuFileBrowserBase *menu);
    /*override*/ ~cMenuFileItem();
    /*override*/ void Set();
    /*override*/ bool IsFile() const {return true;}
    /*override*/ eOSState Open(cMenuFileBrowserBase *menu, bool asPlayListItem = false);
    /*override*/ eOSState Play(cMenuFileBrowserBase *menu);
    /*override*/ eOSState Update(cMenuFileBrowserBase *menu, bool currentChanched);
    /*override*/ eOSState OpenInfo() const;
    std::vector<std::string> GetFileInfoString() const; 
    cFileType GetFileType() const {return item_.GetFileType();}
    cPlayListItem GetPlayListItem() const {return item_;}
    bool IsOfSimilarType(const cMenuFileItem &item) const {return GetFileType().IsOfSimilarType(item.GetFileType());} 
    bool IsOfSimilarType(const cPlayListItem &item) const {return GetFileType().IsOfSimilarType(item.GetFileType());}
    bool IsPlayable() const {return GetFileType().IsPlayable();}
    bool IsXinePlayable() const {return GetFileType().IsXinePlayable();}
    long long GetFileSize() const;

protected:
    cPlayListItem item_;

private:
    //forbid copying   
    cMenuFileItem(const cMenuFileItem &);
    cMenuFileItem &operator=(const cMenuFileItem &);
};

//-----------cMenuDirLinkItem------------------------------

class cMenuDirLinkItem : public cMenuDirItem
{
public:
    cMenuDirLinkItem(std::string path, cMenuFileBrowserBase* menu, bool isDead = false);
    /*override*/ ~cMenuDirLinkItem(){};
    /*override*/ void Set();

private:
    bool isDead_;

    //forbid copying
    cMenuDirLinkItem(const cMenuDirItem &);
    cMenuDirLinkItem &operator=(const cMenuDirItem &);
};

//-----------cMenuFileLinkItem------------------------------

class cMenuFileLinkItem : public cMenuFileItem
{
public:
    cMenuFileLinkItem(std::string path, cMenuFileBrowserBase *menu, bool isDead = false);
    /*override*/ ~cMenuFileLinkItem(){};
    /*override*/ void Set();
private:
    bool isDead_;

    //forbid copying
    cMenuFileLinkItem(const cMenuFileItem &);
    cMenuFileLinkItem &operator=(const cMenuFileItem &);
};

#endif //P__BROWSERITEMS_H
