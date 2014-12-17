/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * netcvupdatemenu.c
 *
 ***************************************************************************/

#include <vdr/interface.h>

#include "netcvupdatemenu.h"

#define MAX_LETTERS 48

// class cNetCVUpdateMenu ---------------------------------------------
cNetCVUpdateMenu::cNetCVUpdateMenu(cNetCVThread *UpdateThread, const char *NetcvUUID, const char *NetcvVersion, const char *UpdateVersion, bool urgentUpdate)
 : cOsdMenu(tr("NetCeiver firmware update")), NetcvUUID_(NetcvUUID), NetcvVersion_(NetcvVersion), UpdateVersion_(UpdateVersion)
{
    UpdateThread_ = UpdateThread;
    GetInterface();
    urgentUpdate_ = urgentUpdate;
    State_ = eMenu;
    Set();
    Display();
}

cNetCVUpdateMenu::~cNetCVUpdateMenu()
{
    if(Interface_)
        free(Interface_);
}

void cNetCVUpdateMenu::Set()
{
    SetCols(12);
    char buffer[64];
    AddFloatingText(tr("There is a new firmware version for the NetCeiver available!"), MAX_LETTERS);
    Add(new cOsdItem());
    snprintf(buffer, 64, "%s\t%s", tr("Current version:"), NetcvVersion_);
    Add(new cOsdItem(buffer));
    snprintf(buffer, 64, "%s\t%s", tr("New version:"), UpdateVersion_);
    Add(new cOsdItem(buffer));
    Add(new cOsdItem());
    if (urgentUpdate_)
        AddFloatingText(tr("If you want to use CA-modules, it is absolutely required to perform this update right now."), MAX_LETTERS);
    AddFloatingText(tr("Do you want to update the NetCeiver firmware now?"), MAX_LETTERS);
    Add(new cOsdItem());
    Add(new cOsdItem(tr("Caution:")));
    AddFloatingText(tr("Current recordings will be interrupted for a few minutes."), MAX_LETTERS);
}

eOSState cNetCVUpdateMenu::ProcessKey(eKeys Key)
{
    char *buffer;

    if(State_ == eExit)
        return osBack;

    if(UpdateThread_)
    {
        if(UpdateThread_->Active())
        {
            SetHelp(NULL);
            if(UpdateThread_->iProgress_ >= 0)
            {
                if(UpdateThread_->iProgress_ < 100)
                {
                    asprintf(&buffer, "%s%i%%", tr("Update progress: "), UpdateThread_->iProgress_);
                    Skins.Message(mtStatus, buffer);
                    free(buffer);
                }
            }
            else
                Skins.Message(mtStatus, tr("Updating... please wait!"));
        }
        else
        {
            SetHelp(tr("Update"), NULL, tr("Skip"));
            if(Key == kRed)
            {
                char *Interface = NULL;
                
                if(Interface_ && strlen(Interface_))
                    asprintf(&Interface, " -d %s", Interface_);
                asprintf(&buffer, "netcvupdate -i %s%s -X %s/%s%s%s", NetcvUUID_, Interface?Interface:"" , UPDATESDIRECTORY, UPDATEFILENAME_PREFIX, UpdateVersion_, UPDATEFILENAME_EXTENSION);
                if(Interface)
                    free(Interface);
                if(UpdateThread_->SetCommand(buffer))
                {
                    free(buffer);
                    Skins.Message(mtStatus, tr("Please wait..."));
                    if(!UpdateThread_->Start())
                    {
                        Skins.Message(mtError, tr("Update failed!"));
                        sleep(3);
                        State_ = eExit;
                    }
                }
                else
                {
                    free(buffer);
                    sleep(3);
                    Skins.Message(mtError, tr("Update failed!"));
                }
            }
            else if (Key == kBack || Key == kYellow)
                return osBack;
        }
        
        if(UpdateThread_->iProgress_ == -2)
        {
            Restart();
        }

    }
    else
    {
        Skins.Message(mtError, tr("internal error!"));
        sleep(3);
        return osBack;
    }

    return osContinue;
}

void cNetCVUpdateMenu::GetInterface()
{
    FILE *file;
    char *buffer;
    char *tmp;
    Interface_ = NULL;

    file = popen("grep NETWORK_INTERFACE= /etc/default/mcli", "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer && !Interface_)
        {
            tmp = strchr(buffer, '=') + 2; // skip both chars: ="
            *(strchr(tmp, '"')) = '\0';
            Interface_ = strdup(tmp);
            buffer = readline.Read(file);
        }
    }
    pclose(file);
}

void cNetCVUpdateMenu::Restart()
{
	Skins.Message(mtStatus, tr("Restarting NetCeiver..."));
    sleep(20);
    SystemExec("killall -HUP mcli");

//    Skins.Message(mtStatus, tr("Restarting VDR..."));
//    SystemExec("killall -HUP vdr"); // dirty hack

    Skins.Message(mtStatus, tr("Update finished"));
    sleep(3);
    State_ = eExit;
}

