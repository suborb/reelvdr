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

#ifndef P__USERIF_H
#define P__USERIF_H

#include <vdr/osdbase.h>
#include <vdr/tools.h>

#include "structs.h"
#include "playlist.h"

class cBrowserMenu;
class cSearchDatabase;

//user interface for browser menu (state pattern)

//----------------------------------------------------------------
//-----------------cBrowserBaseIf-----------------------------
//----------------------------------------------------------------

class cBrowserBaseIf
{
    public:
    virtual ~cBrowserBaseIf(){}
    virtual eOSState RedKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState GreenKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState YellowKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState BlueKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState OkKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState BackKey(cBrowserMenu *menu);
    virtual eOSState InfoKey(cBrowserMenu *menu){ return osContinue; }
    virtual eOSState AnyKey(cBrowserMenu *menu, eKeys Key);
    virtual void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    virtual void LeaveState(cBrowserMenu *menu);
    virtual void SetHelp(cBrowserMenu *menu); 

    // return false if no items are to be drawn in the side banner area
    bool GetSideNoteItems(cBrowserMenu* menu, cStringList *list);
    bool GetPlayerSideNoteInfo(cStringList *list, bool clearSideBanner);
    void SetLastCurrent(cBrowserMenu *menu);
    void SetCurrent(cBrowserMenu *menu);
    cBrowserBaseIf *GetLastIf() { return lastIf_; }
    // virtual void SetMode(cMenuFileBrowser *menu, browseMode mode, bool force = false){}
    // virtual void AddExtraItems(cMenuFileBrowser *menu){}

    void UpdateSideNote(cBrowserMenu *menu);
    void UpdateImage(cBrowserMenu *menu);
    protected:
    virtual void SetMessage(){};
    cBrowserBaseIf(){};
    std::string message_;
    cBrowserBaseIf *lastIf_;
    cSearchDatabase *database_;
    int lastCurrent_;
};

//----------------------------------------------------------------
//-----------------cBrowserIf-------------------------
//----------------------------------------------------------------

class cBrowserIf : public cBrowserBaseIf
{
    public:
    void SetFilters(sFilters filters);
    /*override*/ eOSState YellowKey(cBrowserMenu *menu);
    /*override*/ eOSState BlueKey(cBrowserMenu *menu);
    /*override*/ eOSState OkKey(cBrowserMenu *menu);
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    void SetInsertPlaylist() { insertInPlaylist_ = true; }
    
    protected:
    cBrowserIf(){ insertInPlaylist_ = false; }
    virtual std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu) = 0;
    
    sFilters filters_;
    bool insertInPlaylist_;
};

//----------------------------------------------------------------
//-----------------cBrowserAlbumIf-------------------------
//----------------------------------------------------------------

class cBrowserAlbumIf : public cBrowserIf
{
    public:
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cBrowserAlbumIf *Instance(){return &instance;}

    protected:
    cBrowserAlbumIf(){}; 
    /*override*/ std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
    
    private:
    static cBrowserAlbumIf instance;
};

//----------------------------------------------------------------
//-----------------cBrowserArtistIf-------------------------
//----------------------------------------------------------------

class cBrowserArtistIf : public cBrowserIf
{
    public:
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cBrowserArtistIf *Instance(){return &instance;}

    protected:
    cBrowserArtistIf(){}; 
    /*override*/ std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
    
    private:
    static cBrowserArtistIf instance;
};

//----------------------------------------------------------------
//-----------------cBrowserGenreIf-------------------------
//----------------------------------------------------------------

class cBrowserGenreIf : public cBrowserIf
{
    public:
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);  
    static cBrowserGenreIf *Instance(){return &instance;}

    protected:
    cBrowserGenreIf(){}; 
    /*override*/ std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
    
    private:
    static cBrowserGenreIf instance;
};

//----------------------------------------------------------------
//-----------------cBrowserFilesIf-------------------------
//----------------------------------------------------------------

class cBrowserFilesIf : public cBrowserIf
{
    public:
    /*override*/ eOSState RedKey(cBrowserMenu *menu);
    /*override*/ eOSState GreenKey(cBrowserMenu *menu);
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    /*override*/ void SetHelp(cBrowserMenu *menu);
    static cBrowserFilesIf *Instance(){return &instance;}

    protected:  
    cBrowserFilesIf(){}; 
    
    private:
    static cBrowserFilesIf instance;
    std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
};

#endif //P__USERIF_H
