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

#include "browsermenu.h"
#include "../filebrowser/stringtools.h"
#include "../xinemediaplayer/services.h"
#include "def.h"
#include "tools.h"
#include "playlistIf.h"
#include "optionsIf.h"
#include "searchIf.h"

#include "userIf.h"

#include <time.h>
#include <iomanip>
#include <sstream>

//----------------------------------------------------------------
//--------------------------global--------------------------------
//----------------------------------------------------------------

#define MENUKEY_TIMEOUT 1000 * 30

cTimeMs iflastActivity;

bool TimeOutIf(eKeys Key)
{
    if (Key == kNone)
    {
	if (iflastActivity.TimedOut())
	{
	    return true;
	}
    }
    else
    {
	iflastActivity.Set(MENUKEY_TIMEOUT);
    }
    return false;
}

void SetIfTimeout()
{
    iflastActivity.Set(MENUKEY_TIMEOUT);
}

std::string DisplayIfEmpty(std::string, std::string);

bool cBrowserBaseIf::GetPlayerSideNoteInfo(cStringList* list, bool clearSideBanner)
{
    if (!list)
        return false;

    Xinemediaplayer_Xine_Play_Info info;
    cPluginManager::CallAllServices("Xine Get Play Info", &info);

    if (!info.isPlaying || (info.name.empty() && info.total == 0))
        return false;

#if 0
    printf("\033[7;91mClearSideBanner?%d\nisPlaying?%d\nmrl:'%s'\nname:'%s'\ntotal:elapsed %d:%d\33[0m\n",
           clearSideBanner,
           info.isPlaying,
           info.mrl.c_str(),
           info.name.c_str(),
           info.total,
           info.elapsed);
#endif
    char* tmp = NULL;

    if (clearSideBanner)
    {
        // clears the side banner!
        list->Append(strdup(""));
    }

    cFileInfo f = database_->FileInfo(info.mrl);
    
    std::string title, album, artist, genre;
    title = DisplayIfEmpty(f.title, tr("Unknown Title"));
    artist = DisplayIfEmpty(f.artist, tr("Unknown Artist"));
    album = DisplayIfEmpty(f.album, tr("Unknown Album"));
    genre = DisplayIfEmpty(f.genre, tr("Unknown Genre"));

    list->Append(strdup("STARTFROM 320"));

    list->Append(strdup("FONTCOLOUR:clrRecTitleFg"));
    
    if (info.name.empty())
        list->Append(strdup(title.c_str()));
    else
        list->Append(strdup(info.name.c_str()));
    
    list->Append(strdup("FONTCOLOUR:normal"));
    
    list->Append(strdup(album.c_str()));
    list->Append(strdup(artist.c_str()));
    list->Append(strdup(genre.c_str()));

    std::stringstream s;
    s << MakeProgressBar(info.total?(info.elapsed*100.0)/info.total:0, 50) << " ";
    s << std::setfill('0') << std::setw(2) << info.elapsed/60 ;
    s << ":" << std::setfill('0') << std::setw(2) << info.elapsed%60;
    tmp = strdup(s.str().c_str());

    list->Append(tmp);

    return true;
}


//-------------------------------------------------------------
//----------------cMenuBrowserBaseIf---------------------------
//-------------------------------------------------------------

void cBrowserBaseIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    printf("cBrowserBaseIf::EnterState, lastIf = %p\n", lastIf);
    if(lastIf)
    {
	lastIf_ = lastIf;
    }  
    database_ = search;
    menu->ClearOsd();
    SetHelp(menu);
    printf("cBrowserBaseIf::EnterState, lastIf_ = %p\n", lastIf_);
}

void cBrowserBaseIf::LeaveState(cBrowserMenu *menu)
{ 
    SetLastCurrent(menu);
}

bool cBrowserBaseIf::GetSideNoteItems(cBrowserMenu *menu, cStringList *list)
{
    if (!menu || !list)
    {
        return false;
    }

    cBrowserBaseItem *baseItem = menu->GetCurrentBaseItem();
    bool result = false;

    // does it need some side banner information ?
    if (baseItem)
    {
        result = baseItem->GetSideNoteItems(list);
    }

    // get player info, clear sidenote if no info is drawn by the current item
    bool playerInfo = cBrowserBaseIf::GetPlayerSideNoteInfo(list, !result);

    return playerInfo || result;
}

void cBrowserBaseIf::SetHelp(cBrowserMenu *menu)
{
    menu->setHelp.SetRed("");
    menu->setHelp.SetGreen("");
    menu->setHelp.SetYellow(tr("Button$Search"));
    menu->setHelp.SetBlue(trVDR("Button$Functions"));
    menu->setHelp.FlushButtons(false);
}

eOSState cBrowserBaseIf::AnyKey(cBrowserMenu *menu, eKeys key)
{
    eOSState state = menu->StandardKeyHandling(key);
    //UpdateImage(menu);
    UpdateSideNote(menu);
    return state;
}

void cBrowserBaseIf::UpdateSideNote(cBrowserMenu *menu)
{
    cStringList list;
    bool drawSideNote = GetSideNoteItems(menu, &list);
    if (drawSideNote)
    {
        menu->DrawSideNote(&list);
    }
}

void cBrowserBaseIf::UpdateImage(cBrowserMenu *menu)
{
    printf("cBrowserBaseIf::UpdateImage\n");
    //if(menu->CurrentChanged())
    {  
        printf("cBrowserBaseIf::UpdateImage-----2\n");
	cFileItem *fileItem = menu->GetCurrentFileItem();
	if(fileItem)
	{
	    fileItem->ConvertCover();
	}
    }
}

eOSState cBrowserBaseIf::BackKey(cBrowserMenu *menu)
{
    printf("cBrowserBaseIf::BackKey\n");
    if(lastIf_)
    {
        printf("cBrowserBaseIf::BackKey, vor menu->SetIf(lastIf_)\n");
        menu->SetIf(lastIf_, NULL);
	return osContinue;
    }
    return osEnd;
}

void cBrowserBaseIf::SetLastCurrent(cBrowserMenu *menu)
{
    printf("cBrowserBaseIf::SetLastCurrent, current = %d\n", menu->Current());
    lastCurrent_ = menu->Current();
}

void cBrowserBaseIf::SetCurrent(cBrowserMenu *menu)
{
    printf("cBrowserBaseIf::SetCurrent, lastCurrent_ = %d\n", lastCurrent_);
    cOsdItem *currenItem = menu->Get(lastCurrent_);
    if(currenItem)
    {  
	menu->SetCurrentItem(currenItem);
    } 
}

//-------------------------------------------------------------
//-----------------cBrowserIf----------------------------------
//-------------------------------------------------------------

eOSState cBrowserIf::OkKey(cBrowserMenu *menu)
{
     cBrowserItem *currentItem = menu->GetCurrentBrowserItem();
     if(currentItem)
     {
	 return currentItem->Open(menu);
     }
     return osContinue;
}

eOSState cBrowserIf::YellowKey(cBrowserMenu *menu)
{
    cSearchIf *searchIf = cSearchIf::Instance();
    menu->SetIf(searchIf, cBrowserStartPageIf::Instance()); //always go to start page
    return osContinue;
}

eOSState cBrowserIf::BlueKey(cBrowserMenu *menu)
{
    cBrowserFunctionsIf *functionsIf = cBrowserFunctionsIf::Instance();
    menu->SetIf(functionsIf, this);
    return osContinue;
}  
  
void cBrowserIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    printf("cBrowserIf::EnterState\n");
    cBrowserBaseIf::EnterState(menu, search, lastIf, defOSState);
    
    //if(dynamic_cast<cBrowserFilesIf*>(this) == 0
    menu->SetItems(GetOsdItems(menu));
    SetCurrent(menu);
    UpdateImage(menu);
    menu->Display();
    
    if(insertInPlaylist_)
    {
        cBrowserItem *currentItem = menu->GetCurrentBrowserItem();
        if(currentItem)
	{  
	    currentItem->InsertInPlaylist();
	    menu->GetPlaylist()->SaveToFile();
	}    
	insertInPlaylist_ = false;
    }
}

void cBrowserIf::SetFilters(sFilters filters)
{
    filters_ = filters;
}

//-------------------------------------------------------------
//-----------------cBrowserAlbumIf-----------------------
//-------------------------------------------------------------

cBrowserAlbumIf cBrowserAlbumIf::instance;

void cBrowserAlbumIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    printf("cBrowserAlbumIf::EnterState\n");
    if(IsNonEmptyStringOfSpaces(filters_.artist))
    {  
	menu->SetTitleString(tr("Unknown Artist"));
    }
    else if(!filters_.artist.empty())
    {  
	menu->SetTitleString(filters_.artist);
    }
    else
    {
	menu->SetTitleString(tr("Albums"));
    }  
    
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}

std::vector<cOsdItem*> cBrowserAlbumIf::GetOsdItems(cBrowserMenu *menu)
{
      printf("cBrowserAlbumIf::GetOsdItems, filters_.artist = %s\n", filters_.artist.c_str());
      
      std::vector<cOsdItem*> albumOsdItemList;
    
      if(filters_.artist != "")
      {
	  cArtistItem* item = new cArtistItem(filters_.artist, menu, filters_);
	  albumOsdItemList.push_back(item);
      }	
    
    // list of albums
      std::vector<std::string> albumList = database_->Albums(filters_.genre,
							     filters_.artist,
							     filters_.album
							    );

      printf("Got %u number of albums\n", albumList.size());
      std::vector<std::string>::iterator it = albumList.begin();      
      while (it != albumList.end())
      {
	  cAlbumItem* item = new cAlbumItem(it->c_str(), menu, filters_);
	  albumOsdItemList.push_back(item);
	  ++it;
      }
      return albumOsdItemList;
}

//-------------------------------------------------------------
//-----------------cBrowserArtistIf-----------------------
//-------------------------------------------------------------

cBrowserArtistIf cBrowserArtistIf::instance;

void cBrowserArtistIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    printf("cBrowserArtistIf::EnterState\n");
    menu->SetTitleString(tr("Artists"));
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}
 
std::vector<cOsdItem*> cBrowserArtistIf::GetOsdItems(cBrowserMenu *menu)
{
      printf("cBrowserArtistIf::GetOsdItems\n");
      // list of artists
      std::vector<std::string> artistList = database_->Artists(filters_.genre,
							       filters_.artist
							      );

      printf("Got %u number of artists\n", artistList.size());
      std::vector<std::string>::iterator it = artistList.begin();
      std::vector<cOsdItem*> artistOsdItemList;
      while (it != artistList.end())
      {
	  cArtistItem* item = new cArtistItem(it->c_str(), menu, filters_);
	  artistOsdItemList.push_back(item);
	  ++it;
      }
      return artistOsdItemList;
}

//-------------------------------------------------------------
//-----------------cBrowserGenreIf-----------------------
//-------------------------------------------------------------

cBrowserGenreIf cBrowserGenreIf::instance;

void cBrowserGenreIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{ 
    printf("cBrowserGenreIf::EnterState\n");
    menu->SetTitleString(tr("Genres"));
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}
 
std::vector<cOsdItem*> cBrowserGenreIf::GetOsdItems(cBrowserMenu *menu)
{
      printf("cBrowserGenreIf::GetOsdItems\n");
      // list of genres
      std::vector<std::string> genreList = database_->Genres(filters_.genre
							    );

      printf("Got %u number of genres\n", genreList.size());
      std::vector<std::string>::iterator it = genreList.begin();
      std::vector<cOsdItem*> genreOsdItemList;
      while (it != genreList.end())
      {
	  cGenreItem* item = new cGenreItem(it->c_str(), menu, filters_);
	  genreOsdItemList.push_back(item);
	  ++it;
      }
      return genreOsdItemList;
}

//-------------------------------------------------------------
//-----------------cBrowserFilesIf-----------------------
//-------------------------------------------------------------

cBrowserFilesIf cBrowserFilesIf::instance;

eOSState cBrowserFilesIf::RedKey(cBrowserMenu *menu)
{
    cFileItem *fileItem = menu->GetCurrentFileItem();
    if(fileItem)
    {
	menu->GetFavlist()->Insert(fileItem->GetPlaylistItem());
	menu->GetFavlist()->SaveToFile();
    }  
    return osContinue;
}

eOSState cBrowserFilesIf::GreenKey(cBrowserMenu *menu)
{
    return osContinue;
}

void cBrowserFilesIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{    
    if(IsNonEmptyStringOfSpaces(filters_.album))
    {
	menu->SetTitleString(tr("Unknown Album"));
    }
    else if(!filters_.album.empty())
    {
	menu->SetTitleString(filters_.album);
    }
    else if(IsNonEmptyStringOfSpaces(filters_.artist))
    {
	menu->SetTitleString(tr("Unknown Artist"));
    }
    else if(!filters_.artist.empty())
    {  
	menu->SetTitleString(filters_.artist);
    }
    else if(IsNonEmptyStringOfSpaces(filters_.genre))
    {
	menu->SetTitleString(tr("Unknown Genre"));
    }
    else if(!filters_.genre.empty())
    {
	menu->SetTitleString(filters_.genre);
    }
    else
    {
	menu->SetTitleString(tr("All Titles"));
    } 

    menu->SetColumns(30, 4); // "title \t\t duration"
    printf("cBrowserFilesIf::EnterState\n");
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
    printf("cBrowserFilesIf::EnterState, vor return\n");
}  

void cBrowserFilesIf::SetHelp(cBrowserMenu *menu)
{
    menu->setHelp.SetRed(tr("Button$Favorit"));
    menu->setHelp.SetGreen(tr("Button$Playmode"));
    menu->setHelp.SetYellow(tr("Button$Search"));
    menu->setHelp.SetBlue(trVDR("Button$Functions"));
    menu->setHelp.FlushButtons(false);
}

std::vector<cOsdItem*> cBrowserFilesIf::GetOsdItems(cBrowserMenu *menu)
{    
    printf("cBrowserFilesIf::GetOsdItems\n");
    std::string titleFilter;
  
    std::vector<cFileInfo> songsList = database_->Files(filters_.genre,
							filters_.artist,
							filters_.album,
							titleFilter
							);
    printf("Got %u number of titles\n", songsList.size());

    std::vector<cOsdItem*> items;    
    std::vector<cFileInfo>::iterator it = songsList.begin();
    while (it != songsList.end())
    {
        items.push_back(new cFileItem(*it, menu));
        ++it;
    }

    return items;
}
