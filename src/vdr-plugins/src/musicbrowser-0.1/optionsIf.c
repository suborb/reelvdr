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

#include "userIf.h"
#include "playlist.h"
#include "browsermenu.h"
#include "userIf.h"
#include "searchIf.h"
#include "playlistIf.h"
#include "tools.h"

#include "optionsIf.h"

//----------------------------------------------------------------
//----------------cBrowserOptionsIf---------------------------
//----------------------------------------------------------------

cBrowserOptionsIf cBrowserOptionsIf::instance;

eOSState cBrowserOptionsIf::AnyKey(cBrowserMenu *menu, eKeys Key)
{
    //printf("cMenuBrowserOptionsIf::AnyKey\n");
    eOSState state = menu->StandardKeyHandling(Key);
    UpdateSideNote(menu);
    uint num = state - osUser1; //make hot keysavailable
    if(0 <= num && num < options_.size())  
    {
	std::string option = menu->Get(num)->Text();
	printf("option = %s\n", option.c_str());
	return ExecuteOptions(menu, option);
    }  
    return osUnknown;
}

eOSState cBrowserOptionsIf::OkKey(cBrowserMenu *menu)
{
    //printf("cMenuBrowserOptionsIf::OkKey\n");
    std::string option = menu->Get(menu->Current())->Text();
    return ExecuteOptions(menu, option);
}

eOSState cBrowserOptionsIf::BackKey(cBrowserMenu *menu)
{
    return LeaveStateDefault(menu);
}

void cBrowserOptionsIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{
    //printf("cBrowserOptionsIf::EnterState, lastIf = %p\n", lastIf);
    cBrowserBaseIf::EnterState(menu, search, lastIf, defOSState);
    CreateOptions(menu);
    AddOptions(menu);
    SetCurrent(menu);
    menu->Display();
    //printf("cBrowserOptionsIf::EnterState, ,vor return, lastIf_ = %p\n", lastIf_);
}

eOSState cBrowserOptionsIf::LeaveStateDefault(cBrowserMenu *menu)
{
    //printf("cBrowserOptionsIf::LeaveStateDefault, lastIf_ = %p\n", lastIf_);
    if(lastIf_)
    {
        menu->SetIf(lastIf_, NULL);
	return defOSState_;
    }
    return osEnd;
}

void cBrowserOptionsIf::AddOptions(cBrowserMenu *menu)
{
    /*char buf[256];
    for(uint i = 0; i < options_.size(); ++i)
    {
        sprintf(buf, "%d %s", i + 1, options_[i].c_str());
        menu->Add(new cBrowserBaseItem(std::string(menu->Hk(buf)), menu, static_cast<eOSState>(osUser1 + i)));
    }*/
    for(uint i = 0; i < options_.size(); ++i)
    {
        menu->Add(new cBrowserBaseItem(options_[i], menu));
    }
}

//----------------------------------------------------------------
//----------------cBrowserStartPageIf-----------------------------
//----------------------------------------------------------------

cBrowserStartPageIf cBrowserStartPageIf::instance;

eOSState cBrowserStartPageIf::YellowKey(cBrowserMenu *menu)
{
    cSearchIf *searchIf = cSearchIf::Instance();
    menu->SetIf(searchIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::AnyKey(cBrowserMenu *menu, eKeys Key)
{
    if (NORMALKEY(Key) == kBack)
        return osBack;
    
    return menu->StandardKeyHandling(Key);
}

eOSState cBrowserStartPageIf::ExecuteOptions(cBrowserMenu *menu, std::string option)
{
    printf("option = %s\n", option.c_str());
  
    if(option == tr("Favorites"))
    {
        return ShowFavorites(menu);
    }
    else if(option == tr("Artists"))
    {
        return ShowArtists(menu);
    }
    else if(option == tr("Albums"))
    {
        return ShowAlbums(menu);
    }
    else if(option == tr("Genres"))
    {
        return ShowGenres(menu);
    }
    else if(option == tr("Playlists"))
    {
        return ShowPlaylists(menu);
    }
    else if(option == tr("All Titles"))
    {
        return ShowTitles(menu);
    }
    else if(option == tr("Ratings"))
    {
        return ShowRatings(menu);
    }
    else if(option == tr("Directories"))
    {
        return ShowDirectories(menu);
    }
    return osContinue;
}

void cBrowserStartPageIf::CreateOptions(cBrowserMenu *menu)
{
    menu->SetTitleString(tr("Start"));
    options_.clear();

    options_.push_back(tr("Favorites"));
    options_.push_back(tr("Artists"));
    options_.push_back(tr("Albums"));
    options_.push_back(tr("Genres"));
    options_.push_back(tr("All Titles"));
    options_.push_back(tr("Playlists"));
    options_.push_back(tr("[Ratings]"));
    options_.push_back(tr("[Directories]"));
}

eOSState cBrowserStartPageIf::ShowFavorites(cBrowserMenu *menu)
{
    cBrowserFavoritesIf *favoritesIf = cBrowserFavoritesIf::Instance();
    menu->SetIf(favoritesIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowArtists(cBrowserMenu *menu)
{
    cBrowserArtistIf *artistIf = cBrowserArtistIf::Instance();
    sFilters filters = {"", "", ""};
    artistIf->SetFilters(filters);
    menu->SetIf(artistIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowAlbums(cBrowserMenu *menu)
{
    cBrowserAlbumIf *albumIf = cBrowserAlbumIf::Instance();
    sFilters filters = {"", "", ""};
    albumIf->SetFilters(filters);
    menu->SetIf(albumIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowGenres(cBrowserMenu *menu)
{
    cBrowserGenreIf *genreIf = cBrowserGenreIf::Instance();
    sFilters filters = {"", "", ""};
    genreIf->SetFilters(filters);
    menu->SetIf(genreIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowPlaylists(cBrowserMenu *menu)
{
    cBrowserPlaylistFilesIf *playlistFilesIf = cBrowserPlaylistFilesIf::Instance();
    menu->SetIf(playlistFilesIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowTitles(cBrowserMenu *menu)
{
    cBrowserFilesIf *filesIf = cBrowserFilesIf::Instance();
    sFilters filters = {"", "", ""};
    filesIf->SetFilters(filters);
    menu->SetIf(filesIf, this);
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowRatings(cBrowserMenu *menu)
{
    return osContinue;
}

eOSState cBrowserStartPageIf::ShowDirectories(cBrowserMenu *menu)
{
    return osContinue;
}

//----------------------------------------------------------------
//----------------cBrowserGenreOptionsIf--------------------------
//----------------------------------------------------------------

cBrowserGenreOptionsIf cBrowserGenreOptionsIf::instance;

void cBrowserGenreOptionsIf::SetGenre(std::string genre)
{
    genre_ = genre;
}  

eOSState cBrowserGenreOptionsIf::ExecuteOptions(cBrowserMenu *menu, std::string option)
{
    //printf("cBrowserGenreOptionsIf option = %s\n", option.c_str());
  
    if(option == tr("All Titles"))
    {
        return ShowTitles(menu);
    }
    else if(option == tr("Artists"))
    {
        return ShowArtists(menu);
    }
    else if(option == tr("Albums"))
    {
        return ShowAlbums(menu);
    }
    return osContinue;
}

void cBrowserGenreOptionsIf::CreateOptions(cBrowserMenu *menu)
{
    if(IsNonEmptyStringOfSpaces(genre_))
    {
	menu->SetTitleString(tr("Unknown Genre"));
    }
    else
    {  
	menu->SetTitleString(genre_);
    }
    options_.clear();

    options_.push_back(tr("All Titles"));
    options_.push_back(tr("Artists"));
    options_.push_back(tr("Albums"));
}

eOSState cBrowserGenreOptionsIf::ShowTitles(cBrowserMenu *menu)
{
    //printf("cBrowserGenreOptionsIf::ShowTitles genre_ = %s\n", genre_.c_str());
    cBrowserFilesIf *filesIf = cBrowserFilesIf::Instance();
    sFilters filters = {genre_, "", ""};
    filesIf->SetFilters(filters);
    menu->SetIf(filesIf, this);
    return osContinue;
}

eOSState cBrowserGenreOptionsIf::ShowArtists(cBrowserMenu *menu)
{
    //printf("cBrowserStartPageIf::ShowArtists, genre_ = %s\n", genre_.c_str());
    cBrowserArtistIf *artistIf = cBrowserArtistIf::Instance();
    sFilters filters = {genre_, "", ""};
    artistIf->SetFilters(filters);
    menu->SetIf(artistIf, this);
    return osContinue;
}

eOSState cBrowserGenreOptionsIf::ShowAlbums(cBrowserMenu *menu)
{
    cBrowserAlbumIf *albumIf = cBrowserAlbumIf::Instance();
    sFilters filters = {genre_, "", ""};
    albumIf->SetFilters(filters);
    menu->SetIf(albumIf, this);
    return osContinue;
}

//----------------------------------------------------------------
//----------------cBrowserFunctionsIf--------------------------
//----------------------------------------------------------------

cBrowserFunctionsIf cBrowserFunctionsIf::instance;

eOSState cBrowserFunctionsIf::ExecuteOptions(cBrowserMenu *menu, std::string option)
{
    //printf("cBrowserGenreOptionsIf option = %s\n", option.c_str());
  
    if(option == tr("Show active playlist"))
    {
        return ShowActivePlaylist(menu);
    }
    else if(option == tr("Add current song to active playlist"))
    {
        return AddToPlaylist(menu);
    }
    else if(option == tr("New playlist"))
    {
        return ShowNewPlaylist(menu);
    }
    else if(option == tr("Show currently played songs"))
    {
        return ShowLivelist(menu);
    }
    return osContinue;
}

void cBrowserFunctionsIf::CreateOptions(cBrowserMenu *menu)
{
    menu->SetTitleString(tr("Functions"));
    options_.clear();

    options_.push_back(tr("Show active playlist"));
    options_.push_back(tr("Add current song to active playlist"));
    options_.push_back(tr("New playlist"));
    options_.push_back(tr("Show currently played songs"));
}

eOSState cBrowserFunctionsIf::ShowActivePlaylist(cBrowserMenu *menu)
{
    //printf("BrowserPlaylistIf::ShowPlaylists\n");
    cBrowserPlaylistIf *playlistIf = cBrowserPlaylistIf::Instance();
    menu->SetIf(playlistIf, lastIf_);
    return osContinue;
}

eOSState cBrowserFunctionsIf::ShowNewPlaylist(cBrowserMenu *menu)
{
    menu->GetPlaylist()->Clear();
    return ShowActivePlaylist(menu);
}

eOSState cBrowserFunctionsIf::AddToPlaylist(cBrowserMenu *menu)
{
    cBrowserIf *browserIf = dynamic_cast<cBrowserIf*>(lastIf_);
    if(browserIf)
    {
	browserIf->SetInsertPlaylist();
    }  
    return LeaveStateDefault(menu);
}

eOSState cBrowserFunctionsIf::ShowLivelist(cBrowserMenu *menu)
{
    cBrowserLivelistIf *livelistIf = cBrowserLivelistIf::Instance();
    menu->SetIf(livelistIf, lastIf_);
    return osContinue;
}


//----------------------------------------------------------------
//----------------cPlaylistFunctionsIf--------------------------
//----------------------------------------------------------------

cPlaylistFunctionsIf cPlaylistFunctionsIf::instance;

eOSState cPlaylistFunctionsIf::ExecuteOptions(cBrowserMenu *menu, std::string option)
{
    //printf("cBrowserPlaylistFunctionsIf option = %s\n", option.c_str());
  
    if(option == tr("Delete entry"))
    {
        return DeleteEntry(menu);
    }
    else if(option == tr("Save playlist"))
    {
        return SavePlaylist(menu);
    }
    else if(option == tr("Delete playlist"))
    {
        return DeletePlaylist(menu);
    }
    return osContinue;
}

void cPlaylistFunctionsIf::CreateOptions(cBrowserMenu *menu)
{
    menu->SetTitleString(tr("Playlist Functions"));
    options_.clear();

    options_.push_back(tr("Delete entry"));
    options_.push_back(tr("Save playlist"));
    options_.push_back(tr("Delete playlist"));
}

eOSState cPlaylistFunctionsIf::DeleteEntry(cBrowserMenu *menu)
{
    //printf("cPlaylistFunctionsIf::DeleteEntry item_.GetPath() = %s\n", item_.GetPath().c_str());
    playlist_->Remove(item_); 
    playlist_->SaveToFile();
    return LeaveStateDefault(menu);
}

eOSState cPlaylistFunctionsIf::SavePlaylist(cBrowserMenu *menu)
{
    playlist_->SaveToFile(); 
    return osContinue;
}

eOSState cPlaylistFunctionsIf::DeletePlaylist(cBrowserMenu *menu)
{
    playlist_->Clear(); 
    return LeaveStateDefault(menu);
}

//----------------------------------------------------------------
//----------------cBrowserArtistOptionsIf--------------------------
//----------------------------------------------------------------

/*cBrowserArtistOptionsIf cBrowserArtistOptionsIf::instance;

void cBrowserArtistOptionsIf::SetArtist(std::string artist)
{
    artist_ = artist;
}  

eOSState cBrowserArtistOptionsIf::ExecuteOptions(cBrowserMenu *menu, std::string option)
{
    printf("cBrowserArtistOptionsIf option = %s\n", option.c_str());
  
    if(option == tr("All Titles"))
    {
        return ShowTitles(menu);
    }
    else if(option == tr("Albums"))
    {
        return ShowAlbums(menu);
    }
    return osContinue;
}

void cBrowserArtistOptionsIf::CreateOptions(cBrowserMenu *menu)
{
    menu->SetTitleString(tr("Artists"));
    options_.clear();

    options_.push_back(tr("All Titles"));
    options_.push_back(tr("Albums"));
}

eOSState cBrowserArtistOptionsIf::ShowTitles(cBrowserMenu *menu)
{
    cBrowserFilesIf *filesIf = cBrowserFilesIf::Instance();
    filesIf->SetFilters("", artist_, "");
    menu->SetIf(filesIf);
    return osContinue;
}

eOSState cBrowserArtistOptionsIf::ShowAlbums(cBrowserMenu *menu)
{
    cBrowserAlbumIf *albumIf = cBrowserAlbumIf::Instance();
    albumIf->SetFilters("",artist_, "");
    menu->SetIf(albumIf);
    return osContinue;
}*/

