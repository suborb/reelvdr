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
 *********tr******************************************************************/

//filebrowser.h
 
#ifndef P__FILEBROWSER_H
#define P__FILEBROWSER_H

#include <string>
#include <vector>
#include <vdr/plugin.h>
#include <vdr/status.h>

#include "browserStatus.h"

static const char *DESCRIPTION = trNOOP("Filebrowser");
static const char *MAINMENUENTRY = trNOOP("File Browser");
static const char *MENUSETUPPLUGINENTRY = trNOOP("Picture viewer");

class cFileCache;
class cImageConvert;
class cThumbConvert;

class cPluginFileBrowser : public cPlugin
{
public:
    cPluginFileBrowser();
    virtual ~cPluginFileBrowser();

    /*override*/ const char *Version();
    /*override*/ const char *Description();
    /*override*/ bool ProcessArgs(int argc, char *argv[]);
    /*override*/ bool Initialize();
    /*override*/ const char *MainMenuEntry() { return tr(MAINMENUENTRY); } 
    /*override*/ const char *MenuSetupPluginEntry() { return tr(MENUSETUPPLUGINENTRY); }
    /*override*/ cOsdObject *MainMenuAction();
    /*override*/ cMenuSetupPage *SetupMenu();
    /*override*/ bool SetupParse(const char *Name, const char *Value);
    /*override*/ bool Service(char const *id, void *data);  
    /*override*/ const char **SVDRPHelpPages();
    /*override*/ cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

private:
    //static cFileBrowserStatus status;
    static cFileBrowserStatus newStatus; 
    static int newInstance;
    static cFileBrowserInstances instances;
    static cFileBrowserStatus defStatus;
    //static std::vector<cFileBrowserInstance> instances;
    static cFileCache *cache;
    static cImageConvert *convert;
    static cThumbConvert *thumbConvert;
    void CallXinewithPlaylist();

    // No assigning or copying
    cPluginFileBrowser(const cPluginFileBrowser &plugin);
    cPluginFileBrowser &operator=(const cPluginFileBrowser &plugin);
};

#endif // P__FILEBROWSER_H

