/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 *                                                                         *
 ***************************************************************************
 *
 * install_update.c
 *
 ***************************************************************************/

#include "install_update.h"

cInstallUpdate::cInstallUpdate() : cInstallSubMenu(tr("Software"))
{
    cPlugin *PluginDpkg = cPluginManager::GetPlugin("dpkg");
    if (PluginDpkg)
    {
        struct
        {
            char *title;
            int iMode;
            cOsdMenu *pMenu;
        } data;
        char title[64];
        snprintf(title, 63, "%s", Title());

        data.title = title;
        data.iMode = (1 << 0) | (1 << 9);
        PluginDpkg->Service("dpkg-dlg-v1.0", &data);
        AddSubMenu(data.pMenu);
    }
    else
    {
        esyslog("ERROR: dpkg plugin is missing!\n");
        Skins.Message(mtError, "dpkg plugin is missing!");
    }
}

cInstallUpdate::~cInstallUpdate()
{
}

void cInstallUpdate::Set()
{
}

bool cInstallUpdate::Save()
{
    return false;
}

eOSState cInstallUpdate::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (!HasSubMenu())
    {
        if (HadSubMenu)
            return osUser1;
    }
    return state;
}
