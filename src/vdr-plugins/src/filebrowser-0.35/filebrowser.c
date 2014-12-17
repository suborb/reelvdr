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

#include "menuBrowser.h"
#include "setup.h"
#include "def.h"
#include "filecache.h"
#include "convert.h"
#include "servicestructs.h"
#include "stillpictureplayer.h"
#include "menuDirSelect.h"
#include "filebrowser.h"

#include "../xinemediaplayer/services.h"

#include <cstring>

static const char *VERSION  = "0.30";

//cFileBrowserStatus cPluginFileBrowser::status;
cFileBrowserStatus cPluginFileBrowser::newStatus; 
cFileBrowserInstances cPluginFileBrowser::instances;
cFileBrowserStatus cPluginFileBrowser::defStatus; 

cFileCache *cPluginFileBrowser::cache = NULL;
cImageConvert *cPluginFileBrowser::convert = NULL;
cThumbConvert *cPluginFileBrowser::thumbConvert = NULL;

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

int cPluginFileBrowser::newInstance = -1;

cPluginFileBrowser::cPluginFileBrowser()
{ 
}

cPluginFileBrowser::~cPluginFileBrowser()
{
    delete cache;
    delete convert;
}

const char *cPluginFileBrowser::Version()
{
    return VERSION;
}

const char *cPluginFileBrowser::Description()
{ 
    return DESCRIPTION;
}

bool cPluginFileBrowser::ProcessArgs(int argc, char *argv[])
{
    std::string basedir, startfile;
    if(argc >= 3 && std::string(argv[1]) == "-basedir")
    {
        basedir = argv[2];
    }
    if(argc >= 5 && std::string(argv[3]) == "-startfile")
    {
        startfile = argv[4]; 
    }
    if(basedir != "")
    { 
        newStatus.SetStartDir(basedir, startfile);
    } 
    return true;
}

bool cPluginFileBrowser::Initialize()
{

    cache = new cFileCache;
    if(!cache || !(cache->Open(TMP_CACHE, TMP_CACHE_SIZE) && cache->Open(BROWSER_CACHE, MAX_CACHE_SIZE)))
    {
        DELETENULL(cache);
    }
    convert = new cImageConvert(cache);
    thumbConvert = new cThumbConvert(cache);
    defStatus.SetStartDir(MEDIA_DIR);

    return true;
}

cMenuSetupPage *cPluginFileBrowser::SetupMenu()
{
    return new cFileBrowserSetupPage(cache);
}

bool cPluginFileBrowser::SetupParse(const char *Name, const char *Value)
{
    return fileBrowserSetup.SetupParse(Name, Value);
}

cOsdObject *cPluginFileBrowser::MainMenuAction(void)
{    
    //printf("\n-----MainMenuAction: instances.size() = %d-----\n\n", instances.instances_.size());
    //printf("\n-----newStatus.GetCaller()  = %d-----\n\n", newStatus.GetCaller());
    //printf("--------newInstance = %d--------------\n", newInstance);
    //printf("--------newStatus.id_ = %d--------------\n", newStatus.id_);
   
    std::string titlestring = tr(DESCRIPTION) + std::string(": ") + GetLastN(newStatus.GetStartDir(), 20); 
    instances.DeleteInvalid();
    bool instanceUnspecified = false;
    
    if(newInstance == -1) //no instance given
    {
        if(newStatus.GetCaller() == mediaplayer && newStatus.GetStartDir() == MUSIC_DIR && instances.GetLastMusicInst() != -1)
        {
            newInstance = instances.GetLastMusicInst();
        }
        else if(newStatus.GetCaller() == mediaplayer && newStatus.GetStartDir() == PICTURES_DIR && instances.GetLastImagesInst() != -1)
        {  
            newInstance = instances.GetLastImagesInst(); 
        }        
        else if(newStatus.GetCaller() == mediaplayer && newStatus.GetStartDir() == VIDEO_DIR && instances.GetLastMoviesInst() != -1)
        {
            newInstance = instances.GetLastMoviesInst();
        }
        else
        {
            //create a new instance
	    //printf("New Instance: newStatus.GetPlayList.Size() = %d\n", newStatus.GetPlayList()->Size()); 
	    if(newStatus.GetCaller() == xine)
	    {
	        //printf("vor newStatus.SetStartDir(defStatus.GetStartDir()) = %s\n", defStatus.GetStartDir().c_str());
		//newStatus.SetStartDir(defStatus.GetStartDir());
		//defStatus.Merge(newStatus);
		instanceUnspecified = true;
	    }
            if (newStatus.GetCaller() == normal)
	    {  
		std::string name;
		std::vector<std::string> entries;
		GetXinePlaylist(name, entries, newStatus.GetInstance());
		newStatus.SetPlayListEntries(entries);
		newStatus.GetPlayList()->SetName(name);
	    }    	    
            newStatus.Init();
	    if(!instanceUnspecified) //no new instance when called by xine
	    {
		instances.Add(newStatus);
	    }  
        }
    }

    newStatus.Init();
    cFileBrowserStatus *instance = &defStatus;  
    if(!instanceUnspecified)
    {  
	instance = (newInstance == -1) ? &instances.Last() : &instances.Get(newInstance);
    }  
    
    //printf("-----MainMenuAction: instance->GetStartDir() = %s\n", instance->GetStartDir().c_str());
    instance->Merge(newStatus);
    //printf("----2MainMenuAction: instance->GetStartDir() = %s\n", instance->GetStartDir().c_str());
    newInstance = -1;
    
    if (newStatus.GetXineControl() == xine_save_playlist)
    {
	return NULL; //do not start filebrowser when just saving a playlist
    }   
        
    newStatus.Reset();    
    if((instance->GetStartDirs()).size() > 0)
    { 
        titlestring = instance->GetExternalTitleString();
        return new cMenuDirSelect(titlestring, instance, cache, convert, thumbConvert);
    }
    else
    {   
	return new cMenuFileBrowser(titlestring, instance, cache, convert, thumbConvert);
    }
}

void cPluginFileBrowser::CallXinewithPlaylist()
{
    //printf("----cMenuFileBrowser::ShowFileWithXine, asPlaylist = %d----\n", (int)asPlaylistItem);
    cPlayList *playlist = newStatus.GetPlayList();
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
    
      cPlayList partialList;
      playlist->GetPartialListOfSimilarItems(item, partialList);
      playlistEntries = partialList.GetPathList();
    
      tmp = item.GetPath().c_str();
      if(strstr("://", tmp.c_str()))
      {
	 tmp = std::string("file://") + item.GetPath().c_str();
      }
    }

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        tmp.c_str(),
        newStatus.GetInstance(),
        true,
        playlist->GetName(),
        playlistEntries
    };
    
    //cRemote::CallPlugin("some none existing plugin name"); xine osd will not pop up!
    //printf("blocking callplugins() from other players\n");
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
    //XineStarted(asPlaylistItem);
    newStatus.PlayListActive(true);
}


bool cPluginFileBrowser::Service(char const *id, void *data)
{
    if (std::strcmp(id, "Filebrowser set startdir") == 0)
    { 
        printf("------------Filebrowser set startdir---------------\n");
        int instance = static_cast<FileBrowserSetStartDir*>(data)->instance;
        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).SetStartDir( static_cast<FileBrowserSetStartDir*>(data)->dir,
                            static_cast<FileBrowserSetStartDir*>(data)->file);
        }
        else
        {
            newStatus.SetStartDir( static_cast<FileBrowserSetStartDir*>(data)->dir,
                            static_cast<FileBrowserSetStartDir*>(data)->file);
        }
        return true;
    }
    else if (std::strcmp(id, "Filebrowser set playlist") == 0)
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
    else if (std::strcmp(id, "Filebrowser set playlist entries") == 0)
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
    else if (std::strcmp(id, "Filebrowser save playlist") == 0)
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
    }*/
    else if (std::strcmp(id, "Filebrowser burn playlist") == 0)
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
    }
    else if (std::strcmp(id, "Filebrowser set caller") == 0)
    {  
        int instance = static_cast<FileBrowserSetCaller*>(data)->instance; 
	//printf("--Filebrowser set caller, instance = %d, caller = %d---------------\n", instance, static_cast<FileBrowserSetCaller *>(data)->caller);
        if(instance != -1 && instances.Find(instance))
        { 
            instances.Get(instance).SetCaller(static_cast<FileBrowserSetCaller *>(data)->caller);
        }
        else
        {
            newStatus.SetCaller(static_cast<FileBrowserSetCaller *>(data)->caller);
        }
        return true;
    }
    else if (std::strcmp(id, "Filebrowser xine control") == 0)
    {
        //printf("---------Filebrowser xine control---------------\n");
        int instance = static_cast<FileBrowserControl*>(data)->instance;   
	
	if(static_cast<FileBrowserControl *>(data)->cmd == xine_save_playlist)
	{  
	    if(instance != -1 && instances.Find(instance))
	    {
		instances.Get(instance).SavePlaylist();          
	    }
	    else
	    {
		newStatus.SavePlaylist();
	    }
	}    
        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).XineControl(static_cast<FileBrowserControl *>(data)->cmd); 
        }
        else
        {
            newStatus.XineControl(static_cast<FileBrowserControl *>(data)->cmd);
	    //newStatus.XineControl(xine_save_playlistas); ///////////test!!!!!!!!!!!!!!!!!
        }
        return true;
    }
    else if (std::strcmp(id, "Filebrowser set playlist entry") == 0)
    {   
        //printf("---------Filebrowser set playlist entry---------------\n");
        int instance = static_cast<FileBrowserSetStartDir*>(data)->instance;        
        if(instance != -1 && instances.Find(instance))
        {
            std::string entry = static_cast<FileBrowserSetPlayListEntry *>(data)->entry;
            instances.Get(instance).SetPlayListEntry(entry);
        }
        else
        {
            std::string entry = static_cast<FileBrowserSetPlayListEntry *>(data)->entry;
            newStatus.SetPlayListEntry(entry);
        }
        return true;
    }    
    else if (std::strcmp(id, "Filebrowser set instance") == 0)
    {      
        int instance = static_cast<FileBrowserInstance*>(data)->instance;  
        //printf("-------Filebrowser set instance, %d--------\n", instance);      
        newInstance = instance;
        return true;
    }    
    else if (std::strcmp(id, "Filebrowser get instances") == 0)
    { 
        //printf("--------cPluginFileBrowser::Service:Filebrowser get instances--------------\n");  
        FileBrowserInstances *Data = static_cast<FileBrowserInstances*>(data);
        std::vector<int> vec = instances.GetInstanceVec();
        Data->num_instances = vec.size();
        memcpy(Data->instances, &vec[0], sizeof(int)*vec.size());  
        //printf("--Data->instances[0] = %d------Data->instances[1] = %d--\n", Data->instances[0], Data->instances[1]);
        return true;   
    }     
    else if (std::strcmp(id, "Filebrowser delete instance") == 0)
    { 
        //printf("----cPluginFileBrowser::Service:Filebrowser delete instance------\n");  
        int instance = static_cast<FileBrowserInstance*>(data)->instance;  
        instances.Del(instance);
        return true;   
    }     
    else if (std::strcmp(id, "Filebrowser get pictureplayer info") == 0)
    { 
        //printf("----cPluginFileBrowser::Service: Filebrowser get pictureplayer info------\n");  
        FileBrowserPicPlayerInfo* info = static_cast<FileBrowserPicPlayerInfo*>(data);     
        cControl *control = cControl::Control();
        info->active = reinterpret_cast<int>(dynamic_cast<cStillPictureControl*>(control));
        return true;   
    }   
    else if (std::strcmp(id, "Filebrowser external action") == 0)
    { 
        //printf("----cPluginFileBrowser::Service: Filebrowser external action------\n"); 
        FileBrowserExternalAction* extData = static_cast<FileBrowserExternalAction*>(data);     

        newStatus.SetStartDir(extData->startDir, extData->startFile);
        newStatus.SetStartDirs(extData->startDirs);
        newStatus.SetExternalInfo(extData->info);
 
        newStatus.SetCaller(plugin); 
        return true;   
    }
    return false;
}

const char **cPluginFileBrowser::SVDRPHelpPages(void)
{
    //printf("-----------cPluginFileBrowser::SVDRPHelpPages-----------\n");
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open filebrowser plugin.",
        NULL
    };
    return HelpPages;
}

cString cPluginFileBrowser::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (strcasecmp(Command, "OPEN") == 0) 
    {
        //set new start directory
        std::string option = Option;
        if(option == "")
        {
            option = BASE_DIR;
        }
        newStatus.SetStartDir(option, "");
        newStatus.SetCaller(svdrp);

        cRemote::CallPlugin("filebrowser"); 

        std::string msg = "Opening filebrowser plugin, startdir = " + option;
        return cString(msg.c_str());
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginFileBrowser); // Don't touch this!

