/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * install_diseqc.c
 *
 ***************************************************************************/

#include "install_diseqc.h"

cInstallDiseqc::cInstallDiseqc() : cInstallSubMenu(tr("Reception"))
{
    cPlugin *p = cPluginManager::GetPlugin("netcv");
    if (p)
    {
        struct
        {
            char *title;
            cOsdMenu *menu;
        } data;

        char title[64];
        snprintf(title, 63, "%s", Title());
        data.title = title;

        p->Service("Netceiver Install Wizard", &data);
        AddSubMenu(data.menu);
    }
    else
    {
        esyslog("ERROR: netcv plugin is missing!\n");
        Skins.Message(mtError, "netcv plugin is missing!");
    }
}

eOSState cInstallDiseqc::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (HasSubMenu())
    {
        if (state == osUser1)
        {
            CloseSubMenu();
            return osUser1;
        }
        else
        {
            if ((Key == kYellow) || (Key == kGreen))
                return osUnknown;
            else
                return osContinue;
        }
    }
    return state;
}

bool cInstallDiseqc::Save()
{
    return false;
}

void cInstallDiseqc::Set()
{
}
