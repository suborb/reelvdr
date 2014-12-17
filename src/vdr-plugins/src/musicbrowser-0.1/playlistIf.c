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
#include "userIf.h"
#include "optionsIf.h"
#include "browsermenu.h"
#include "def.h"

#include "../filebrowser/stringtools.h"

#include "playlistIf.h"


//-------------------------------------------------------------
//-----------------cBrowserPlaylistIf-----------------------
//-------------------------------------------------------------

cBrowserPlaylistIf cBrowserPlaylistIf::instance;

eOSState cBrowserPlaylistIf::OkKey(cBrowserMenu *menu)
{
    cBrowserItem* currentItem = menu->GetCurrentBrowserItem();
    if (currentItem)
    {  
        return currentItem->Open(menu, false);
    }	
    else 
    {
        cMenuEditStrItem* item = dynamic_cast<cMenuEditStrItem*> (menu->Get(menu->Current()));
        if (item) 
	{
            //playlistName_ = std::string(text);
	    playlistName_ = RemoveName(playlistName_) + std::string(text);
	    playlist_->SetName(playlistName_);

            /* redraw menu */
            int c = menu->Current();
            menu->SetItems(GetOsdItems(menu));
            menu->SetCurrentItem(menu->Get(c));
	    SetHelp(menu);
            menu->Display();
        }
    }
    return osContinue;
}

eOSState cBrowserPlaylistIf::BlueKey(cBrowserMenu *menu)
{
    cPlaylistFunctionsIf *functionsIf = cPlaylistFunctionsIf::Instance();
    cPlayListItem item;
    cFileItem *fileItem = menu->GetCurrentFileItem();
    if(fileItem)
    {
	item = fileItem->GetPlaylistItem();
    } 
    functionsIf->SetCurrentPlaylistItem(item);
    functionsIf->SetCurrentPlaylist(playlist_);
    menu->SetIf(functionsIf, this);
    return osContinue;
}

void cBrowserPlaylistIf::SetHelp(cBrowserMenu *menu)
{
    menu->setHelp.SetRed("");
    menu->setHelp.SetGreen("");
    menu->setHelp.SetYellow("");
    menu->setHelp.SetBlue(trVDR("Button$Functions"));
    menu->setHelp.FlushButtons(false);
}

void cBrowserPlaylistIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    menu->SetTitleString(tr("Playlist entries"));
    menu->SetColumns(30, 4); // "title \t\t duration"
    playlist_ = menu->GetPlaylist();
    playlistName_ = playlist_->GetName();
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}  

std::vector<cOsdItem*> cBrowserPlaylistIf::GetOsdItems(cBrowserMenu *menu)
{      
    cOsdItem *editItem = new cMenuEditStrItem(tr("Playlist"), text, 255, NULL, GetLast(playlistName_).c_str());
    cOsdItem *emptyItem = new cOsdItem("", osUnknown, false);
    
    std::vector<cOsdItem*> items;
    items.push_back(editItem);
    items.push_back(emptyItem);
    
    GetListItems(items, menu);
    return items;
}

void cBrowserPlaylistIf::GetListItems(std::vector<cOsdItem*> &items, cBrowserMenu *menu)
{    
    std::vector<std::string> pathList = playlist_->GetPathList();  
    std::vector<std::string>::iterator it = pathList.begin();
    while (it != pathList.end())
    {
        cFileInfo fileinfo = database_->FileInfo(*it);
	//cFileInfo fileinfo = { "", "", "", *it }; 
        items.push_back(new cFileItem(fileinfo, menu));
        ++it;
    }
}

//-------------------------------------------------------------
//-----------------cBrowserFavoritesIf-----------------------
//-------------------------------------------------------------

cBrowserFavoritesIf cBrowserFavoritesIf::instance;

eOSState cBrowserFavoritesIf::OkKey(cBrowserMenu *menu)
{
    return cBrowserIf::OkKey(menu);
}

void cBrowserFavoritesIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    menu->SetTitleString(tr("Favorites"));
    menu->SetColumns(35, 4); // "title \t\t duration"
    playlist_ = menu->GetFavlist();
    playlistName_ = playlist_->GetName();
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}  

std::vector<cOsdItem*> cBrowserFavoritesIf::GetOsdItems(cBrowserMenu *menu)
{      
    std::vector<cOsdItem*> items;
    GetListItems(items, menu);
    return items;
}

//-------------------------------------------------------------
//-----------------cBrowserLivelistIf-----------------------
//-------------------------------------------------------------

cBrowserLivelistIf cBrowserLivelistIf::instance;


eOSState cBrowserLivelistIf::BlueKey(cBrowserMenu *menu)
{
    return osContinue;
}

void cBrowserLivelistIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    menu->SetTitleString(tr("Live replay"));
    menu->SetColumns(35, 4); // "title \t\t duration"
    menu->LoadLivelist();
    playlist_ = menu->GetLivelist();
    playlistName_ = playlist_->GetName();
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}  

void cBrowserLivelistIf::SetHelp(cBrowserMenu *menu)
{
    menu->setHelp.SetRed("");
    menu->setHelp.SetGreen("");
    menu->setHelp.SetYellow("");
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false);
}

//-------------------------------------------------------------
//-----------------cBrowserPlaylistFilesIf-----------------------
//-------------------------------------------------------------

cBrowserPlaylistFilesIf cBrowserPlaylistFilesIf::instance;

void cBrowserPlaylistFilesIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    menu->SetTitleString(tr("Playlists"));
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
} 

std::vector<cOsdItem*> cBrowserPlaylistFilesIf::GetOsdItems(cBrowserMenu *menu)
{    
    //printf("cBrowserPlaylistIf::GetOsdItems\n");
    
    /*if (!mountMgr_.Mount(PLAYLIST_DEFAULT_PATH))
    {
        Skins.Message(mtError, tr("Cannot mount media"));
        return false;
    }*/
    
    std::string dir = PLAYLIST_DEFAULT_PATH;
    std::vector<Filetools::dirEntryInfo>  files;
    std::vector<cOsdItem*> items;
    
    //search for entries in directory 
    //DDD("----vor GetDirEntries");
    if(!Filetools::GetDirFiles(dir, files))
    {
        printf("couldn't open %s", dir.c_str());
        Skins.Message(mtError, tr("Access denied!"));
        return items;
    }  

    //Sort menu entries alphabetically
    //DDD("----cMenuFileBrowser::OpenDir() vor Sort entries");
    /*std::stable_sort(dirEntries.begin(), dirEntries.end(), sortDirEntry);
    if(status->GetSortMode() == byalph)
    {  
	std::stable_sort(fileEntries.begin(), fileEntries.end(), sortDirEntry);
    }
    else if(status->GetSortMode() == bysize)
    {  
	std::stable_sort(fileEntries.begin(), fileEntries.end(), sortFileEntryBySize);
    }*/	
    
    std::vector<Filetools::dirEntryInfo>::iterator it = files.begin();
    while (it != files.end())
    {
        items.push_back(new cPlaylistFileItem(it->name_, menu));
        ++it;
    }

    return items;
}
