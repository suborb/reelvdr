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
 * setupmysqlmenu.cpp
 *
 ***************************************************************************/

#include "setupmysqlmenu.h"
#include "netinfo.h"
#include <vdr/vdrmysql.h>

cSetupMysqlMenu::cSetupMysqlMenu(const char *title) : cOsdMenu(title)
{
    if(Setup.ReelboxModeTemp == eModeServer)
        LocalDBEnabled_ = true;
    else
        LocalDBEnabled_ = false;
    Set();
}

cSetupMysqlMenu::~cSetupMysqlMenu()
{
}

void cSetupMysqlMenu::Set()
{
    char strBuff[64];
    SetCols(14);

    snprintf(strBuff, 64, "%s:%s", tr("Database status"), LocalDBEnabled_?tr("active"):tr("inactive"));
    Add(new cOsdItem(strBuff, osUnknown, false));

    Add(new cOsdItem(tr("Clear Database"), osUser1, true));
    Add(new cOsdItem(tr("Clear Table ..."), osUser2, false));
    Add(new cOsdItem(tr("Change Database Password ..."), osUser3, false));

    Display();
}

eOSState cSetupMysqlMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if(HasSubMenu())
        return state;

    switch(state)
    {
        case osUser1:
            ClearDatabase();
            return osContinue;
            break;
        default:
            return state;
    }
}

void cSetupMysqlMenu::ClearDatabase()
{
    char local_ip[16];
    cNetInfo netinfo;

    if(netinfo.IfNum()>1)
    {
        strncpy(local_ip, netinfo[1]->IpAddress(), 16);
        //printf("\033[0;93m %s(%i): Local IP= %s \033[0m\n", __FILE__, __LINE__, local_ip);

        cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINUSER, "vdr");
        if(mysql->SetServer(local_ip))
        {
            if(!mysql->ClearDatabase())
                Skins.Message(mtError, tr("Error: Clearing database failed!"));
        }
        else
            Skins.Message(mtError, tr("Error: Connecting database failed!"));
        delete mysql;
    }
    else
        Skins.Message(mtError, tr("Local IP-Address unknown!"));

}

