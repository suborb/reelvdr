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

#include "mediaplayerdvd.h"
#include "playermenu.h"

#include "def.h"

#include "vdr/remote.h"

static const char *VERSION  = "0.0.3";


cPluginMediaPlayerDvd::cPluginMediaPlayerDvd()
{
   //printf("------------cPluginMediaPlayerDvd::cPluginMediaPlayerDvd------------\n");
   //OpenDvd();
}

cPluginMediaPlayerDvd::~cPluginMediaPlayerDvd()
{
}

const char *cPluginMediaPlayerDvd::Version()
{
    return VERSION;
}

const char *cPluginMediaPlayerDvd::Description()
{
    return DESCRIPTION;
}

bool cPluginMediaPlayerDvd::Initialize()
{
    return true;
}

cMenuSetupPage *cPluginMediaPlayerDvd::SetupMenu()
{
    return NULL; 
}

bool cPluginMediaPlayerDvd::SetupParse(const char *Name, const char *Value)
{
    return true;
}

cOsdObject *cPluginMediaPlayerDvd::MainMenuAction(void)
{
    printf("------------cPluginMediaPlayerDvd::MainMenuAction------------\n");
    OpenDvd(); 
    return NULL;
}

bool cPluginMediaPlayerDvd::Service(char const *id, void *data)
{
    return false;
}

const char **cPluginMediaPlayerDvd::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open mediaplayerdvd plugin.",
        NULL
    };
    return HelpPages;
}

cString cPluginMediaPlayerDvd::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{ 
    if (strcasecmp(Command, "OPEN") == 0) 
    {
       cRemote::CallPlugin("mediaplayerdvd"); 

       std::string msg = "Opening mediaplayerdvd plugin" + std::string(Option);
       return cString(msg.c_str());
    }
    return NULL;
}

//start filebrowser in DVD-directory, expert mode
void cPluginMediaPlayerDvd::OpenDvd()
{
    struct
    {
        char const *dir;
        char const *file;
    } startDirData = { 
                         DVDDIR,
                          "" 
                     };
    struct
    {
        int mode;
        bool calledFromMediaPlayer;
    } browseModeData = { 
                           1,  //expert mode
                           true //?
                       };
    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);
    cPluginManager::CallAllServices("Filebrowser set browsemode", &browseModeData);

    cRemote::CallPlugin("filebrowser");
}

VDRPLUGINCREATOR(cPluginMediaPlayerDvd); // Don't touch this!

