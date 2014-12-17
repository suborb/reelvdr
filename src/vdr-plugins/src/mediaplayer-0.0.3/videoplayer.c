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

#include "videoplayer.h"

#include "def.h"

#include "../filebrowser/servicestructs.h"

#include "vdr/remote.h"

static const char *VERSION  = "0.0.3";


cPluginVideoPlayer::cPluginVideoPlayer()
{ 
}

cPluginVideoPlayer::~cPluginVideoPlayer()
{
}

const char *cPluginVideoPlayer::Version()
{
    return VERSION;
}

const char *cPluginVideoPlayer::Description()
{
    return DESCRIPTION;
}

bool cPluginVideoPlayer::Initialize()
{
    return true;
}

cMenuSetupPage *cPluginVideoPlayer::SetupMenu()
{
    return NULL; 
}

bool cPluginVideoPlayer::SetupParse(const char *Name, const char *Value)
{
    return true;
}

cOsdObject *cPluginVideoPlayer::MainMenuAction(void)
{
    std::string titlestring = tr(DESCRIPTION); 
    return PlayVideo();
}

bool cPluginVideoPlayer::Service(char const *id, void *data)
{

    return false;
}

const char **cPluginVideoPlayer::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open audioplayer plugin.",
        NULL
    };
    return HelpPages;
}

cString cPluginVideoPlayer::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{ 
    if (strcasecmp(Command, "OPEN") == 0) 
    {
       cRemote::CallPlugin("videoplayer"); 

       std::string msg = "Opening audioplayer plugin, startdir = " + std::string(Option);
       return cString(msg.c_str());
    }
    return NULL;
}

//start filebrowser in video directory, standard mode
cOsdObject *cPluginVideoPlayer::PlayVideo()
{ 
    if (cPluginManager::CallAllServices("Show All Video Files"))
    { 
        //return NULL;
    }   

    FileBrowserSetStartDir startDirData = {
                            MOVIESDIR,
                            "",
                            -1
                            };

    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);
    return CallFileBrowser();
}

//call filebrowser
cOsdObject *cPluginVideoPlayer::CallFileBrowser()
{
    FileBrowserSetCaller callerData = {
                        mediaplayer,
                        -1
                        };
   
    cPluginManager::CallAllServices("Filebrowser set caller", &callerData);

    cPlugin *filebrowser = cPluginManager::GetPlugin("filebrowser");
    if(filebrowser)
    {
        return filebrowser->MainMenuAction();
    }
    return NULL;
}



VDRPLUGINCREATOR(cPluginVideoPlayer); // Don't touch this!
