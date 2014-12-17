/***************************************************************************
 *   Copyright (C) 2005-2010 by Reel Multimedia                            *
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

#include "bouquets.h"

#include "MenuBouquets.h"
#include <vdr/debug.h>
#include <vdr/i18n.h>
#include <vdr/menu.h>

static const char *VERSION = "0.3";
static const char *DESCRIPTION = "bouquets plugin";

cPluginBouquets::cPluginBouquets():
view_(Default), mode_(mode_view)
{
}

cPluginBouquets::~cPluginBouquets()
{
}

const char *cPluginBouquets::Version()
{
    return VERSION;
}

const char *cPluginBouquets::Description()
{
    return DESCRIPTION;
}

bool cPluginBouquets::Initialize()
{
    return true;
}

cMenuSetupPage *cPluginBouquets::SetupMenu()
{
    return NULL;
}

bool cPluginBouquets::SetupParse(const char *Name, const char *Value)
{
    return true;
}

cOsdObject *cPluginBouquets::MainMenuAction(void)
{
    return new cMenuMyBouquets(view_, mode_);
}

bool cPluginBouquets::Service(char const *Id, void *data)
{
    if (strcmp(Id, "osChannelsHook") == 0)
    {
        struct BouquetsData
            {
                eChannellistMode Function;      /*IN*/
                cOsdMenu *pResultMenu; /*OUT*/
            } *Data;
            Data = (BouquetsData *) data;
        Data->pResultMenu = new cMenuMyBouquets(Data->Function, (eEditMode) 0);
        return true;
    }
#if APIVERSNUM >= 10716
    else if (strcmp(Id, "MenuMainHook-V1.0") == 0)
    {
        MenuMainHook_Data_V1_0 *mmh = (MenuMainHook_Data_V1_0 *)data;
        switch(mmh->Function) 
        {
            case osChannels: 
                    mmh->pResultMenu = new cMenuMyBouquets(Default, (eEditMode) 0); break;
            case osActiveBouquet:
                    mmh->pResultMenu = new cMenuMyBouquets(currentBouquet, (eEditMode) 0); break;
            case osBouquets: 
                    mmh->pResultMenu = new cMenuMyBouquets(bouquets, (eEditMode) 0); break;
            case osFavourites:
                    mmh->pResultMenu = new cMenuMyBouquets(favourites, (eEditMode) 0); break;
            case osAddFavourite:
                    mmh->pResultMenu = new cMenuMyBouquets(addToFavourites, (eEditMode) 0); break;
            default     : return false;
        }
        return true;
    }
#endif
    return false;
}

VDRPLUGINCREATOR(cPluginBouquets);      // Don't touch this!
