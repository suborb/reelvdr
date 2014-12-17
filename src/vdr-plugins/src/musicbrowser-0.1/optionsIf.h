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

#ifndef P__OPTIONSIF_H
#define P__OPTIONSIF_H

#include <vdr/osdbase.h>

#include "structs.h"
#include "playlist.h"

#include "userIf.h"

//----------------------------------------------------------------
//----------------cBrowserOptionsIf---------------------------
//----------------------------------------------------------------

class cBrowserOptionsIf : public cBrowserBaseIf
{
    public:  
    /*override*/ eOSState OkKey(cBrowserMenu *menu);
    /*override*/ eOSState BackKey(cBrowserMenu *menu); 
    /*override*/ eOSState AnyKey(cBrowserMenu *menu, eKeys Key);
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cBrowserOptionsIf *Instance(){return &instance;}
 
    protected:
    cBrowserOptionsIf(){}; 
    virtual void CreateOptions(cBrowserMenu *menu){};
    virtual void AddOptions(cBrowserMenu *menu);
    virtual eOSState LeaveStateDefault(cBrowserMenu *menu);
    virtual eOSState ExecuteOptions(cBrowserMenu *menu, std::string option){return osContinue;} 
    std::vector<std::string> options_;
    eOSState defOSState_;

    private:
    static cBrowserOptionsIf instance;
};

//----------------------------------------------------------------
//----------------cBrowserStartPageIf------------------------
//----------------------------------------------------------------

class cBrowserStartPageIf : public cBrowserOptionsIf
{
    public:
    static cBrowserStartPageIf *Instance(){return &instance;}
    /*override*/ eOSState YellowKey(cBrowserMenu *menu);
    protected:
    cBrowserStartPageIf(){}
    eOSState AnyKey(cBrowserMenu *menu, eKeys Key);

    /*override*/ void CreateOptions(cBrowserMenu *menu);
    /*override*/ eOSState ExecuteOptions(cBrowserMenu *menu, std::string option);

    private:
    eOSState ShowFavorites(cBrowserMenu *menu);
    eOSState ShowArtists(cBrowserMenu *menu);
    eOSState ShowAlbums(cBrowserMenu *menu);
    eOSState ShowGenres(cBrowserMenu *menu);
    eOSState ShowPlaylists(cBrowserMenu *menu);
    eOSState ShowTitles(cBrowserMenu *menu);
    eOSState ShowRatings(cBrowserMenu *menu);
    eOSState ShowDirectories(cBrowserMenu *menu);

    static cBrowserStartPageIf instance;
};

//----------------------------------------------------------------
//----------------cBrowserGenreOptionsIf-----------------------
//----------------------------------------------------------------

class cBrowserGenreOptionsIf : public cBrowserOptionsIf
{
    public:
    static cBrowserGenreOptionsIf *Instance(){return &instance;}
    void SetGenre(std::string genre);
 
    protected:
    cBrowserGenreOptionsIf(){};
    /*override*/ void CreateOptions(cBrowserMenu *menu);
    /*override*/ eOSState ExecuteOptions(cBrowserMenu *menu, std::string option);

    private:
    eOSState ShowTitles(cBrowserMenu *menu);  
    eOSState ShowArtists(cBrowserMenu *menu);
    eOSState ShowAlbums(cBrowserMenu *menu);
    
    std::string genre_;

    static cBrowserGenreOptionsIf instance;
};

//----------------------------------------------------------------
//----------------------cBrowserFunctionsIf-----------------------
//----------------------------------------------------------------

class cBrowserFunctionsIf : public cBrowserOptionsIf
{
    public:
    static cBrowserFunctionsIf *Instance(){return &instance;}
 
    protected:
    cBrowserFunctionsIf(){};
    /*override*/ void CreateOptions(cBrowserMenu *menu);
    /*override*/ eOSState ExecuteOptions(cBrowserMenu *menu, std::string option);

    private:
    eOSState ShowActivePlaylist(cBrowserMenu *menu);
    eOSState ShowNewPlaylist(cBrowserMenu *menu); 
    eOSState AddToPlaylist(cBrowserMenu *menu);
    eOSState ShowLivelist(cBrowserMenu *menu);

    static cBrowserFunctionsIf instance;
};

//----------------------------------------------------------------
//----------------------cPlaylistFunctionsIf-----------------------
//----------------------------------------------------------------

class cPlaylistFunctionsIf : public cBrowserOptionsIf
{
    public:
    static cPlaylistFunctionsIf *Instance(){return &instance;}
    void SetCurrentPlaylistItem(cPlayListItem item) { item_ = item; }
    void SetCurrentPlaylist(cPlayList *playlist) { playlist_ = playlist; }
 
    protected:
    cPlaylistFunctionsIf(){};
    /*override*/ void CreateOptions(cBrowserMenu *menu);
    /*override*/ eOSState ExecuteOptions(cBrowserMenu *menu, std::string option);

    private:
    eOSState DeleteEntry(cBrowserMenu *menu);  
    eOSState SavePlaylist(cBrowserMenu *menu);
    eOSState DeletePlaylist(cBrowserMenu *menu);
    
    cPlayListItem item_;
    cPlayList *playlist_;
    
    static cPlaylistFunctionsIf instance;
};


//----------------------------------------------------------------
//----------------cBrowserArtistOptionsIf-----------------------
//----------------------------------------------------------------

/*class cBrowserArtistOptionsIf : public cBrowserOptionsIf
{
    public:
    static cBrowserArtistOptionsIf *Instance(){return &instance;}
    void SetArtist(std::string genre);
 
    protected:
    cBrowserArtistOptionsIf(){};
    void CreateOptions(cBrowserMenu *menu);
    eOSState ExecuteOptions(cBrowserMenu *menu, std::string option);

    private:
    eOSState ShowTitles(cBrowserMenu *menu);  
    eOSState ShowAlbums(cBrowserMenu *menu);
    
    std::string artist_;

    static cBrowserArtistOptionsIf instance;
};*/



#endif //P__OPTIONSIF_H
