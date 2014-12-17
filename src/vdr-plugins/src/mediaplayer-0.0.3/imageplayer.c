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

#include "imageplayer.h"

#include "def.h"

#include "../filebrowser/servicestructs.h"

#include "vdr/remote.h"

static const char *VERSION  = "0.0.3";


cPluginImagePlayer::cPluginImagePlayer()
{ 
}

cPluginImagePlayer::~cPluginImagePlayer()
{
}

const char *cPluginImagePlayer::Version()
{
    return VERSION;
}

const char *cPluginImagePlayer::Description()
{
    return DESCRIPTION;
}

bool cPluginImagePlayer::Initialize()
{
    return true;
}

cMenuSetupPage *cPluginImagePlayer::SetupMenu()
{
    return NULL; 
}

bool cPluginImagePlayer::SetupParse(const char *Name, const char *Value)
{
    return true;
}

cOsdObject *cPluginImagePlayer::MainMenuAction(void)
{
    std::string titlestring = tr(DESCRIPTION); 
    return PlayImage();
}

bool cPluginImagePlayer::Service(char const *id, void *data)
{

    return false;
}

const char **cPluginImagePlayer::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open imageplayer plugin.",
        NULL
    };
    return HelpPages;
}

cString cPluginImagePlayer::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{ 
    if (strcasecmp(Command, "OPEN") == 0) 
    {
       cRemote::CallPlugin("imageplayer"); 

       std::string msg = "Opening imageplayer plugin, startdir = " + std::string(Option);
       return cString(msg.c_str());
    }
    return NULL;
}

//start filebrowser in pictures directory, standard mode
cOsdObject *cPluginImagePlayer::PlayImage()
{ 
    if (cPluginManager::CallAllServices("Show All Photo Files"))
    { 
        //return NULL;
    }   

    FileBrowserSetStartDir startDirData = {
                            IMAGESDIR,
                            "",
                            -1
                            };

    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);
    return CallFileBrowser();
}

//call filebrowser
cOsdObject *cPluginImagePlayer::CallFileBrowser()
{
    printf("BLA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    FileBrowserSetCaller callerData = {
                        mediaplayer,
                        13
                        };
   
    cPluginManager::CallAllServices("Filebrowser set caller", &callerData);

    cPlugin *filebrowser = cPluginManager::GetPlugin("filebrowser");
    if(filebrowser)
    {
        return filebrowser->MainMenuAction();
    }
    return NULL;
}


VDRPLUGINCREATOR(cPluginImagePlayer); // Don't touch this!
