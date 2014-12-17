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

#include <reel/searchapi.h>

#include "playlist.h"
#include "structs.h"

//class cImageConvert;
class cBrowserMenu;


//-----------cBrowserItem---------------------

class cBrowserBaseItem : public cOsdItem
{
public:
    cBrowserBaseItem(std::string name, cBrowserMenu* Menu, eOSState State = osUnknown);
    virtual ~cBrowserBaseItem(){}; 
    virtual bool IsFileItem() const {return false;}
    virtual bool IsFolderItem() const {return false;}
    virtual bool IsBrowserItem() const {return false;}
    virtual bool GetSideNoteItems(cStringList *list);

    
protected:    
    std::string name_;
    cBrowserMenu* menu;
    
private: 
    //forbid copying
    cBrowserBaseItem(const cBrowserBaseItem &);
    cBrowserBaseItem &operator=(const cBrowserBaseItem &);
};  

//-----------cBrowserItem---------------------

class cBrowserItem : public cBrowserBaseItem
{
public:
    cBrowserItem(std::string name, cBrowserMenu* Menu);
    virtual ~cBrowserItem(){}
    /*override*/ void Set(){}
    /*override*/ bool IsFileItem() const {return false;}
    /*override*/ bool IsFolderItem() const {return false;}
    /*override*/ bool IsBrowserItem() const {return true;}
    bool GetSideNoteItems(cStringList *list);
    std::string GetName() const {return name_;}
    std::string GetPath() const {return name_;}
    //std::string GetPath() const;
    itemId GetId() const { return itemId(GetPath()); }
    virtual eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false) {return osContinue;}
    virtual eOSState Play(cBrowserMenu *menu) {return osContinue;}
    virtual void InsertInPlaylist() {}
    //virtual eOSState Update(cMenuFileBrowserBase *menu, bool currentChanched);
    //virtual eOSState OpenInfo() const {return osContinue;}
    
    void Mark();
    void UnMark();
    void Flag();
    void UnFlag();
    void Tag();
    void UnTag();

protected:
    bool isMarked;
    bool isFlagged;
    bool isTagged;

private: 
    //forbid copying
    cBrowserItem(const cBrowserItem &);
    cBrowserItem &operator=(const cBrowserItem &);
};

//-----------cFileItem------------------------------

class cFileItem : public cBrowserItem 
{
public:
    cFileItem(cFileInfo &file, cBrowserMenu *menu);
    cFileItem(cPlayListItem item, cBrowserMenu *menu);
    /*override*/ ~cFileItem();
    /*override*/ void Set();
    /*override*/ bool IsFileItem() const {return true;}
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    /*override*/ void InsertInPlaylist();
    void RemoveFromPlaylist();
    bool GetSideNoteItems(cStringList *list);
    void ConvertCover();
    std::string GetPath() const;
    cPlayListItem GetPlaylistItem() { return cPlayListItem(GetPath()); }
    // /*override*/ eOSState Play(cMp3BrowserMenu *menu);
    // /*override*/ eOSState Update(cMp3BrowserMenu *menu, bool currentChanched);
    // /*override*/ eOSState OpenInfo() const;
    //std::vector<std::string> GetFileInfoString() const; 

private:
    cFileInfo file_;
    std::string cover_;
    uint coverSize_;
    bool coverConversionStarted_;
    std::string GetConvertedCover();

    //forbid copying   
    cFileItem(const cFileItem &);
    cFileItem &operator=(const cFileItem &);
};

//-----------cPlaylistFileItem------------------------------

class cPlaylistFileItem : public cBrowserItem 
{
public:
    cPlaylistFileItem(std::string name, cBrowserMenu *menu);
    /*override*/ ~cPlaylistFileItem();
    /*override*/ void Set();
    /*override*/ bool IsFileItem() const {return true;}
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    //std::string GetPath() const; 

private:

    //forbid copying   
    cPlaylistFileItem(const cFileItem &);
    cPlaylistFileItem &operator=(const cFileItem &);
};

//-----------cFolderItem---------------------

class cFolderItem : public cBrowserItem
{
public:
    cFolderItem(std::string name, cBrowserMenu* menu, sFilters filters);
    cFolderItem(const itemId &id, cBrowserMenu* menu);
    /*override*/ ~cFolderItem(){}
    /*override*/ void Set();
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    // /*override*/ eOSState Play(cMp3BrowserMenu *menu);
    // /*override*/ eOSState OpenInfo() const = 0; 

  protected:    
    sFilters filters_;

private: 
    //forbid copying
    cFolderItem(const cFolderItem &);
    cFolderItem &operator=(const cFolderItem &);
};

//-----------cGenreItem---------------------

class cGenreItem : public cFolderItem
{
public:
    cGenreItem(std::string path, cBrowserMenu* menu, sFilters filters);
    cGenreItem(const itemId &id, cBrowserMenu* menu);
    /*override*/ ~cGenreItem();
    /*override*/ void Set();
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    // /*override*/ void InsertInPlaylist();
    // /*override*/ eOSState Play(cMp3BrowserMenu *menu);
    // /*override*/ eOSState OpenInfo() const; 

private: 
    //forbid copying
    cGenreItem(const cGenreItem &);
    cGenreItem &operator=(const cGenreItem &);
};

//-----------cArtistItem---------------------

class cArtistItem : public cFolderItem
{
public:
    cArtistItem(std::string path, cBrowserMenu* menu, sFilters filters);
    cArtistItem(const itemId &id, cBrowserMenu* menu);
    /*override*/ ~cArtistItem(){}
    /*override*/ void Set();
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    /*override*/ void InsertInPlaylist();
    // /*override*/ eOSState Play(cMp3BrowserMenu *menu);
    // /*override*/ eOSState OpenInfo() const; 

private: 
    //forbid copying
    cArtistItem(const cArtistItem &);
    cArtistItem &operator=(const cArtistItem &);
};

//-----------cAlbumItem---------------------

class cAlbumItem : public cFolderItem
{
public:
    cAlbumItem(std::string path, cBrowserMenu* menu, sFilters filters);
    cAlbumItem(const itemId &id, cBrowserMenu* menu);
    /*override*/ ~cAlbumItem(){}
    /*override*/ void Set();
    /*override*/ eOSState Open(cBrowserMenu *menu, bool asPlayListItem = false);
    // /*override*/ eOSState Play(cMp3BrowserMenu *menu);
    // /*override*/ eOSState OpenInfo() const; 

private:
    //forbid copying
    cAlbumItem(const cAlbumItem &);
    cAlbumItem &operator=(const cAlbumItem &);
};

#endif //P__BROWSERITEMS_H
