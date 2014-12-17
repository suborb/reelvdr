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

#include <cstring>

static const char *VERSION  = "0.30";

//cFileBrowserStatus cPluginFileBrowser::status;
cFileBrowserStatus cPluginFileBrowser::newStatus; 
cFileBrowserInstances cPluginFileBrowser::instances;
//std::vector<cFileBrowserInstance> instances;
cFileCache *cPluginFileBrowser::cache = NULL;
cImageConvert *cPluginFileBrowser::convert = NULL;
cThumbConvert *cPluginFileBrowser::thumbConvert = NULL;
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
    std::string titlestring = tr(DESCRIPTION) + std::string(": ") + GetLastN(newStatus.GetStartDir(), 20); 
    instances.DeleteInvalid();
    if(newInstance == -1) //no instance specified
    {
        //printf("-----instances.Add(newStatus), instances.size = %d---\n", instances.instances_.size());
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
            newStatus.Init();
            //printf("-----instances.Add(newStatus)---\n");
            instances.Add(newStatus);
        }
    }
    cFileBrowserStatus *instance = (newInstance == -1) ? &instances.Last() : &instances.Get(newInstance);
    instance->Merge(newStatus);
    newInstance = -1;
    newStatus.Reset();    
    if((instance->GetStartDirs()).size() > 0)
    { 
        titlestring = instance->GetExternalTitleString();
        return new cMenuDirSelect(titlestring, instance, cache, convert, thumbConvert);
    }
    else
    {;
        return new cMenuFileBrowser(titlestring, instance, cache, convert, thumbConvert);
    }
}

bool cPluginFileBrowser::Service(char const *id, void *data)
{
    if (std::strcmp(id, "Filebrowser set startdir") == 0)
    { 
        //printf("------------Filebrowser set startdir---------------\n");
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
    else if (std::strcmp(id, "Filebrowser set caller") == 0)
    {  
        //printf("------------Filebrowser set caller---------------\n"); 
        int instance = static_cast<FileBrowserSetStartDir*>(data)->instance; 
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
        if(instance != -1 && instances.Find(instance))
        {
            instances.Get(instance).XineControl(static_cast<FileBrowserControl *>(data)->cmd); 
        }
        else
        {
            newStatus.XineControl(static_cast<FileBrowserControl *>(data)->cmd); 
        }
        return true;
    }
    else if (std::strcmp(id, "Filebrowser set playlist entry") == 0)
    {      
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

