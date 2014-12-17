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

#ifndef P__PLAYLISTIF_H
#define P__PLAYLISTIF_H

#include <vdr/osdbase.h>

#include "structs.h"
#include "playlist.h"

#include "userIf.h"

//----------------------------------------------------------------
//-----------------cBrowserPlaylistIf-------------------------
//----------------------------------------------------------------

class cBrowserPlaylistIf : public cBrowserIf
{
    public:
    /*override*/ eOSState OkKey(cBrowserMenu *menu);
    /*override*/ eOSState BlueKey(cBrowserMenu *menu); 
    /*override*/ void SetHelp(cBrowserMenu *menu);  
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cBrowserPlaylistIf *Instance(){return &instance;}

    protected:  
    cBrowserPlaylistIf(){}; 
    void GetListItems(std::vector<cOsdItem*> &items, cBrowserMenu *menu);
    
    std::string playlistName_;
    cPlayList *playlist_;
    
    private:
    static cBrowserPlaylistIf instance;
    std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
 
    char text[256]; // text input by user
};

//----------------------------------------------------------------
//-----------------cBrowserFavoritesIf-------------------------
//----------------------------------------------------------------

class cBrowserFavoritesIf : public cBrowserPlaylistIf
{
    public:
    /*override*/ eOSState OkKey(cBrowserMenu *menu);
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cBrowserFavoritesIf *Instance(){return &instance;}

    protected:  
    cBrowserFavoritesIf(){}; 
    
    private:
    static cBrowserFavoritesIf instance;
    std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
};

//----------------------------------------------------------------
//-----------------cBrowserLiveListIf-------------------------
//----------------------------------------------------------------

class cBrowserLivelistIf : public cBrowserFavoritesIf
{
    public:
    /*override*/ eOSState BlueKey(cBrowserMenu *menu);
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    /*override*/ void SetHelp(cBrowserMenu *menu);
    static cBrowserLivelistIf *Instance(){return &instance;}

    protected:  
    cBrowserLivelistIf(){}; 
    
    private:
    static cBrowserLivelistIf instance;
};

//----------------------------------------------------------------
//-----------------cBrowserPlaylistFilesIf-------------------------
//----------------------------------------------------------------

class cBrowserPlaylistFilesIf : public cBrowserIf
{
    public:
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);  
    static cBrowserPlaylistFilesIf *Instance(){return &instance;}

    protected:  
    cBrowserPlaylistFilesIf(){}; 
    
    private:
    static cBrowserPlaylistFilesIf instance;
    std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
};

#endif //P__PLAYLISTIF_H
