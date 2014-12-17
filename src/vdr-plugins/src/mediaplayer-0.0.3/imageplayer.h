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

//imageplayer.h
 
#ifndef P__IMAGEPLAYER_H
#define P__IMAGEPLAYER_H

#include <string>
#include <vdr/plugin.h>
#include <vdr/status.h>

static const char *DESCRIPTION = trNOOP("Image Player");
const char *const MOUNT_SH = "/usr/sbin/mount.sh";


class cPluginImagePlayer : public cPlugin
{
public:
    cPluginImagePlayer();
    virtual ~cPluginImagePlayer();

    /*override*/ const char *Version();
    /*override*/ const char *Description();
    /*override*/ bool Initialize();
    /*override*/ const char *MainMenuEntry(void) { return tr("Image Library"); }
    /*override*/ cOsdObject *MainMenuAction(void);
    /*override*/ cMenuSetupPage *SetupMenu();
    /*override*/ bool SetupParse(const char *Name, const char *Value);
    /*override*/ bool HasSetupOptions(void) { return false; }
    /*override*/ bool Service(char const *id, void *data);  
    /*override*/ const char **SVDRPHelpPages(void);
    /*override*/ cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

private:

    // No assigning or copying
    cPluginImagePlayer(const cPluginImagePlayer &plugin);
    cPluginImagePlayer &operator=(const cPluginImagePlayer &plugin);
    cOsdObject *PlayImage();
    cOsdObject *CallFileBrowser();
};

#endif // P__MEDIAPLAYER_H

