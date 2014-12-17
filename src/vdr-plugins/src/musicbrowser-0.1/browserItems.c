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

 
#include "../filebrowser/stringtools.h" 
#include "../xinemediaplayer/services.h"
#include <vdr/plugin.h>
#include "browsermenu.h"
// #include "convert.h"
#include <iostream>
#include <sstream>
#include <vector>

#include "browserItems.h"
#include "userIf.h"
#include "playlistIf.h"
#include "optionsIf.h"
#include "cover.h"
#include "tools.h"

std::string DisplayIfEmpty(std::string name, std::string placeHolder)
{
    return IsEmptyString(name)?placeHolder:name;
}

std::string DisplayName(const cFileInfo& file)
{
    if (!IsEmptyString(file.title))
        return file.title;

    return file.filename;
}

//-----------cBrowserBaseItem------------------------------

cBrowserBaseItem::cBrowserBaseItem(std::string name, cBrowserMenu* Menu, eOSState State)
: cOsdItem(name.c_str(), State), name_(name), menu(Menu)
{ 
}

bool cBrowserBaseItem::GetSideNoteItems(cStringList *list)
{
    return false;
}

//-----------cBrowserItem------------------------------

cBrowserItem::cBrowserItem(std::string name, cBrowserMenu* Menu)
: cBrowserBaseItem(name, Menu)
{ 
}

void cBrowserItem::Mark()
{
    isMarked = true;
    Set();
}

void cBrowserItem::UnMark()
{
    isMarked = false;
    Set();
}

void cBrowserItem::Flag()
{
    isFlagged = true;
    Set();
}

void cBrowserItem::UnFlag()
{
    isFlagged = false;
    Set();
}

void cBrowserItem::Tag()
{
    isTagged = true;
    Set();
}

void cBrowserItem::UnTag()
{
    isTagged = false;
    Set();
}

bool cBrowserItem::GetSideNoteItems(cStringList *list)
{
    if (!list)
        return false;

    list->Append(strdup(name_.c_str()));
    return true;
}

//-----------cFileItem------------------------------

cFileItem::cFileItem(cFileInfo &file, cBrowserMenu *menu)
: cBrowserItem(file.path, menu), file_(file), cover_(""), coverSize_(0), coverConversionStarted_(false)
{
    Set();
}

cFileItem::~cFileItem()
{
}

std::string cFileItem::GetPath() const
{
    return file_.path;
}  

void cFileItem::Set()
{
    /*if(isMarked)
    {
        name = std::string(">\t") + name;
    }
    if(isFlagged)
    {
        name = std::string("\x88\t") + name;
    }

    SetText(name.c_str());*/
    std::string name = DisplayName(file_);
    std::string duration = std::string(*cString::sprintf("%02lu",file_.duration/60))
            + std::string(":")
            + std::string(*cString::sprintf("%02lu",file_.duration%60));

    name = name + "\t\t" + duration;

    //printf("%s %u\n", name.c_str(), name.size());
    SetText(name.c_str(), true);
}

eOSState cFileItem::Open(cBrowserMenu *menu, bool asPlayListItem)
{
    cOsdItem* item = menu->First();

    Xinemediaplayer_Xine_Play_mrl_with_name xine;
    std::vector<std::string> &names = xine.namelistEntries;
    std::vector<std::string> &paths = xine.playlistEntries;

    while (item)
    {
        cFileItem *fileItem = dynamic_cast<cFileItem*> (item);

        item = menu->Next(item);
        if (!fileItem)
            continue;

        names.push_back(DisplayName(fileItem->file_));
        paths.push_back(fileItem->file_.path);
    }

    xine.mrl = file_.path.c_str(); // current path; play this first
    xine.instance = -5; // don't open xineosd;
    xine.playlist = true;

    cPluginManager::CallAllServices("Xine Play mrl with name", &xine);
    
    Xinemediaplayer_Set_Xine_Mode xineMode;
    xineMode.mode = XineMediaplayer_Mode_musicbrowser;
    cPluginManager::CallAllServices("Set Xine Mode", &xineMode);

    return osContinue;
}

void cFileItem::InsertInPlaylist()
{
    cPlayListItem item_(file_.path);
    //printf("cFileItem::InsertInPlaylist, item_.GetPath() = %s\n", item_.GetPath().c_str());
    menu->GetPlaylist()->Insert(item_);
}

void cFileItem::RemoveFromPlaylist()
{
    cPlayListItem item_(file_.path);
    menu->GetPlaylist()->Remove(item_);
} 

bool cFileItem::GetSideNoteItems(cStringList *list)
{
    if (!list)
        return false;
    std::string coverPath = GetConvertedCover();
    
    if (!coverPath.empty())
    {
        std::string imageString = "IMAGE:" + coverPath; 
        list->Append(strdup(imageString.c_str()));
    }

    list->Append(strdup("FONTCOLOUR:clrRecTitleFg"));
    list->Append(strdup(DisplayName(file_).c_str()));
    list->Append(strdup("FONTCOLOUR:normal"));
    if (!file_.album.empty())
        list->Append(strdup(file_.album.c_str()));
    if (!file_.artist.empty())
        list->Append(strdup(file_.artist.c_str()));
    if (!file_.genre.empty())
        list->Append(strdup(file_.genre.c_str()));

    return true;
}

void cFileItem::ConvertCover()
{
    if(!coverConversionStarted_)
    {
	Cover::ConvertCover(GetPath(), cover_, coverSize_);
	coverConversionStarted_ = true;
    }  
    //printf("cFileItem::ConvertCover(), cover_ = %s, path = %s\n", cover_.c_str(), GetPath().c_str());
}  

std::string cFileItem::GetConvertedCover()
{
    std::string defaultCover = "";
    if(cover_ == "")
    {  
	return defaultCover;
    }  
    std::string convertedCover = Cover::GetConvertedCover(cover_, coverSize_);
    //printf("cFileItem::GetConvertedCover(), convertedCover = %s\n", convertedCover.c_str());
    return !convertedCover.empty() ? convertedCover : defaultCover; 
}  

//-----------cPlaylistFileItem------------------------------

cPlaylistFileItem::cPlaylistFileItem(std::string name, cBrowserMenu *menu)
: cBrowserItem(name, menu)
{
    Set();
}

cPlaylistFileItem::~cPlaylistFileItem()
{
}

/*std::string cFileItem::GetPath() const
{
    return  id_.path_;
} */ 

void cPlaylistFileItem::Set()
{
    SetText(name_.c_str(), true);
}

eOSState cPlaylistFileItem::Open(cBrowserMenu *menu, bool asPlayListItem)
{
    menu->GetPlaylist()->LoadFromFile(name_);
    //printf("cPlaylistFileItem::Open, menu->GetPlaylist()->GetName() = %s\n", menu->GetPlaylist()->GetName().c_str()); 
    cBrowserPlaylistIf *playlistIf = cBrowserPlaylistIf::Instance();
    menu->SetIf(playlistIf);
    return osContinue;
}

//-----------cFolderItem------------------------------

cFolderItem::cFolderItem(std::string name, cBrowserMenu* menu, sFilters filters)
: cBrowserItem(name, menu), filters_(filters)
{
}

void cFolderItem::Set()
{
    /*if(isMarked)
    {
        name = std::string(">\t") + name;
    }
    if(isFlagged)
    {
        name = std::string("\x88\t") + name;
    }

    SetText(name.c_str());*/
}

eOSState cFolderItem::Open(cBrowserMenu *menu, bool asPlayListItem)
{
    return osContinue;
}

//-----------cGenreItem------------------------------

cGenreItem::cGenreItem(std::string name, cBrowserMenu* menu, sFilters filters)
: cFolderItem(name, menu, filters)
{
    Set();
} 

cGenreItem::~cGenreItem()
{
}

void cGenreItem::Set()
{
    std::string Genre = DisplayIfEmpty(name_, tr("Unknown Genre"));
    SetText(Genre.c_str(), true);
}

eOSState cGenreItem::Open(cBrowserMenu* menu, bool asPlayListItem)
{
    cBrowserGenreOptionsIf *genreOptions = cBrowserGenreOptionsIf::Instance();
    genreOptions->SetGenre(name_);
    menu->SetIf(genreOptions);
    return osContinue;
}

/*eOSState cGenreItem::Play(cBrowserMenu* menu, bool asPlayListItem)
{
    //return item_.Open(menu);
}*/

//-----------cAlbumItem------------------------------

cAlbumItem::cAlbumItem(std::string name, cBrowserMenu* menu, sFilters filters)
: cFolderItem(name, menu, filters)
{
    Set();
} 

void cAlbumItem::Set()
{
    std::string Album = DisplayIfEmpty(name_, tr("Unknown Album"));
    SetText(Album.c_str(), true);
}

eOSState cAlbumItem::Open(cBrowserMenu* menu, bool asPlayListItem)
{
    cBrowserFilesIf *filesIf = cBrowserFilesIf::Instance();
    sFilters filters = filters_;
    filters.album = name_;
    filesIf->SetFilters(filters);
    menu->SetIf(filesIf);
    return osContinue;
}

/*eOSState cAlbumItem::Play(cBrowserMenu* menu)
{
    //return item_.Open(menu);
}*/

//-----------cArtistItem------------------------------

cArtistItem::cArtistItem(std::string name, cBrowserMenu* menu, sFilters filters)
: cFolderItem(name, menu, filters)//, item_(path)
{
    Set();
} 

void cArtistItem::Set()
{	
    if(filters_.artist == name_)
    {
	SetText(tr("All Files"));
    }  
    else
    {  
	std::string Artist = DisplayIfEmpty(name_, tr("Unknown Artist"));
	SetText(Artist.c_str());
    }
}

eOSState cArtistItem::Open(cBrowserMenu* menu, bool asPlayListItem)
{   
    printf("cArtistItem::Open, filters_.artist = %s\n", filters_.artist.c_str());
    
    if(filters_.artist == name_)
    {
	cBrowserFilesIf *filesIf = cBrowserFilesIf::Instance();
	filesIf->SetFilters(filters_);
	menu->SetIf(filesIf);
	return osContinue;
    }  
    
    cBrowserAlbumIf *albumsIf = cBrowserAlbumIf::Instance();
    sFilters filters = filters_;
    filters.artist = name_;
    albumsIf->SetFilters(filters);
    menu->SetIf(albumsIf);
    return osContinue;
}

void cArtistItem::InsertInPlaylist()
{
    
} 

/*eOSState cArtistItem::Play(cBrowserMenu* menu)
{
    //return item_.Open(menu);
}*/
