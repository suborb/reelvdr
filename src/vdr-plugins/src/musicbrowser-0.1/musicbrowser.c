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

#include "vdr/remote.h"

#include "musicbrowser.h"
#include "cover.h"
//#include "servicestructs.h"
#include "def.h"
#include "browsermenu.h"

#include <getopt.h>
#include "../xinemediaplayer/services.h"

#include <cstring>

const char *VERSION  = "0.01";
const char *DESCRIPTION = trNOOP("musicbrowser");
const char *MAINMENUENTRY = trNOOP("Music Browser");
const char *MENUSETUPPLUGINENTRY = trNOOP("Music Browser");

std::string titlestring = "";

cMp3BrowserStatus cPluginMusicBrowser::status; 

void GetXinePlaylist(std::string &playlistName, std::vector<std::string> &playlistEntries, int instance)
{   
    // Data structure for service "Xine getset playlist"    
    Xinemediaplayer_Xine_GetSet_playlist data = 
    {
	false,
	instance, 
	playlistName,
	playlistEntries
    };
    cPluginManager::CallAllServices("Xine getset playlist", &data);
    
    playlistName = data.playlistName;
    playlistEntries = data.playlistEntries;
}

//int cPluginFileBrowser::newInstance = -1;

cPluginMusicBrowser::cPluginMusicBrowser()
{ 
}

cPluginMusicBrowser::~cPluginMusicBrowser()
{
    delete Cover::thumbConvert;
    delete Cover::cache;
    delete Cover::id3cache;
}

const char *cPluginMusicBrowser::Version()
{
    return VERSION;
}

const char *cPluginMusicBrowser::Description()
{ 
    return DESCRIPTION;
}

bool cPluginMusicBrowser::ProcessArgs(int argc, char *argv[])
{

    int option_index = 0;
    struct option long_options[] = {
       {"database", required_argument, 0, 'd'},
       {0, 0, 0, 0}
    };

    for (int i = 0; i < argc; ++i)
        printf("\033[7;91margv[%d] -> '%s'\033[0m\n", i, argv[i]);

    int c = -1;
    while (1)
    {
        c = getopt_long(argc, argv, "d", long_options, &option_index);

        /* no more options */
        if (c == -1)
            break;

        switch ( c )
        {
        case 0:
            break;

        case 'd':
            dbPath = optarg;
            break;

        default:
            break;
        } // switch

    } // while

    printf("dbPath after getopt '%s'\n", dbPath.c_str());
    return true;
}

bool cPluginMusicBrowser::Initialize()
{
    Cover::cache = new cTempFileCache;
    if(!Cover::cache || !(Cover::cache->Open(TMP_CACHE, TMP_CACHE_SIZE) && Cover::cache->Open(BROWSER_CACHE, MAX_CACHE_SIZE)))
    {
        DELETENULL(Cover::cache);
    }
    Cover::id3cache = new cFileCache;
    if(!Cover::id3cache || !(Cover::id3cache->Open(TMP_CACHE, TMP_CACHE_SIZE) && Cover::id3cache->Open(BROWSER_CACHE, MAX_CACHE_SIZE)))
    {
        DELETENULL(Cover::id3cache);
    }
    if(Cover::cache && Cover::id3cache)
    {  
	Cover::thumbConvert = new cThumbConvert(Cover::cache);
	Cover::thumbConvert->SetDimensions(125, 125);
	Cover::thumbConvert->SetEnding(".thumb2.png");
    }	

    return true;
}

cMenuSetupPage *cPluginMusicBrowser::SetupMenu()
{
    return NULL;
}

bool cPluginMusicBrowser::SetupParse(const char *Name, const char *Value)
{
    return true;
}

cOsdObject *cPluginMusicBrowser::MainMenuAction(void)
{    
    return new cBrowserMenu(titlestring, &status);
}

void cPluginMusicBrowser::CallXinewithPlaylist()
{
    //printf("----cMenuFileBrowser::ShowFileWithXine, asPlaylist = %d----\n", (int)asPlaylistItem);
    cPlayList *playlist = status.GetPlayList();
    std::vector<std::string> playlistEntries; //empty list
    std::string tmp; //empty string
    
    if(!playlist->IsEmpty())
    {  
    
      cPlayListItem item;
      if(playlist->CurrentIsSet())
      {
	item = playlist->GetCurrent();
      }
      else
      {
	item = playlist->GetNthItem(0);
      }	
    
      //cPlayList partialList;
      //playlist->GetPartialListOfSimilarItems(item, partialList);
      playlistEntries = playlist->GetPathList();
    
      tmp = item.GetPath().c_str();
      if(strstr("://", tmp.c_str()))
      {
	 tmp = std::string("file://") + item.GetPath().c_str();
      }
    }

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(),
        0,//status.GetInstance(),
        true,
        playlist->GetName(),
        playlistEntries
    };
    
    //cRemote::CallPlugin("some none existing plugin name"); xine osd will not pop up!
    //printf("blocking callplugins() from other players\n");
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
    //XineStarted(asPlaylistItem);
    //status.PlayListActive(true);
}


bool cPluginMusicBrowser::Service(char const *id, void *data)
{
/*    if (std::strcmp(id, "mp3player set playlist") == 0)
    { 
        //printf("------------Filebrowser set playlist---------------\n"); 
        int instance = static_cast<FileBrowserSetStartDir*>(data)->instance;

        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).PlayListActive(true);
            //instances.Get(instance).status.SetPlayList( static_cast<FileBrowserSetPlayList *>(data)->file);
        }
        else
        {
            newStatus.PlayListActive(true);
            //newStatus.SetPlayList( static_cast<FileBrowserSetPlayList *>(data)->file);
        }
        return true;
    }
    else if (std::strcmp(id, "mp3player set playlist entries") == 0)
    { 
        //printf("------------Filebrowser set playlist entries---------------\n"); 
        int instance = static_cast<FileBrowserSetPlayListEntries*>(data)->instance;
	std::string name = static_cast<FileBrowserSetPlayListEntries*>(data)->name;
        std::vector<std::string> entries = static_cast<FileBrowserSetPlayListEntries*>(data)->entries;
	
        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).SetPlayListEntries(entries);
	    instances.Get(instance).GetPlayList()->SetName(name);  
        }
        else
        { 
            newStatus.SetPlayListEntries(entries);
	    newStatus.GetPlayList()->SetName(name); 
        }
        return true;
    }
    else if (std::strcmp(id, "mp3player save playlist") == 0)
    { 
        //printf("------------Filebrowser save playlist---------------\n"); 
        int instance = static_cast<FileBrowserSavePlaylist*>(data)->instance;
	
        if(instance != -1 && instances.Find(instance))
        {
            static_cast<FileBrowserSavePlaylist*>(data)->name = instances.Get(instance).SavePlaylist();          
        }
        else
        {
            static_cast<FileBrowserSavePlaylist*>(data)->name = newStatus.SavePlaylist();
	    //printf("new name = %s\n", name.c_str());
        }
        return true;
    }/*
    else if (std::strcmp(id, "Filebrowser save playlist as") == 0) //test!!!!!!!!!!!!!!
    { 
        printf("------------Filebrowser save playlist---------------\n"); 
        int instance = static_cast<FileBrowserSavePlaylist*>(data)->instance;
	
        if(instance != -1 && instances.Find(instance))
        {
            newStatus.XineControl(xine_save_playlistas);           
        }
        else
        {
	    newStatus.XineControl(xine_save_playlistas); 
	    //printf("new name = %s\n", name.c_str());
        }
        return true;
    }
    else if (std::strcmp(id, "mp3player burn playlist") == 0)
    { 
        //printf("------------Filebrowser burn playlist---------------\n"); 
        int instance = static_cast<FileBrowserInstance*>(data)->instance;
       
        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).BurnPlayList();           
        }
        else
        {
            newStatus.BurnPlayList();
           
        }
        return true;
    }*/
    return false;
}

const char **cPluginMusicBrowser::SVDRPHelpPages(void)
{
    //printf("-----------cPluginFileBrowser::SVDRPHelpPages-----------\n");
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open mp3 browser plugin.",
        NULL
    };
    return HelpPages;
}

cString cPluginMusicBrowser::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    /*if (strcasecmp(Command, "OPEN") == 0) 
    {
        //set new start directory
        std::string option = Option;
        if(option == "")
        {
            option = BASE_DIR;
        }
        newStatus.SetStartDir(option, "");
        newStatus.SetCaller(svdrp);

        cRemote::CallPlugin("mp3player); 

        std::string msg = "Opening mp3player plugin, startdir = " + option;
        return cString(msg.c_str());
    }*/
    return NULL;
}

VDRPLUGINCREATOR(cPluginMusicBrowser); // Don't touch this!

