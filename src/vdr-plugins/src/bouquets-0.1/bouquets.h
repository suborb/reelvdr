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

// bouquets.h

#ifndef P__BOUQUETS_H
#define P__BOUQUETS_H

#include <vdr/plugin.h>
#include <vdr/status.h>

#include "def.h"

#if APIVERSNUM > 10700
enum eChannellistMode { Default, currentBouquet, classicList, bouquets, favourites, addToFavourites };
#endif

class cPluginBouquets:public cPlugin
{
  public:
    cPluginBouquets();
    /*override */ ~cPluginBouquets();

    /*override */ const char *Version();
    /*override */ const char *Description();

    /*override */ bool Initialize();

    /*override */ const char *MainMenuEntry(void)
    {
        return tr("Channellist");
    }
    /*override */ cOsdObject *MainMenuAction(void);
    /*override */ cMenuSetupPage *SetupMenu();
    /*override */ bool SetupParse(const char *Name, const char *Value);
    /*override */ bool HasSetupOptions(void)
    {
        return false;
    }
    bool Service(char const *Id, void *data);

  private:
    // No assigning or copying
    cPluginBouquets(const cPluginBouquets &);
    cPluginBouquets & operator=(const cPluginBouquets &);

    enum eChannellistMode view_;
    eEditMode mode_;
};

#endif // P__BOUQUETS_H
