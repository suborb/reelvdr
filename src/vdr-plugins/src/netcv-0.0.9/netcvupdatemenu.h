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
 * netcvupdatemenu.h
 *
 ***************************************************************************/

#ifndef NETCVUPDATEMENU_H
#define NETCVUPDATEMENU_H

#include <vdr/osdbase.h>

#include "netcvthread.h"

#define UPDATESDIRECTORY "/usr/share/reel/netcv/update"
#define UPDATEFILENAME_PREFIX "netceiver_"
#define UPDATEFILENAME_EXTENSION ".tgz"

enum eState { eMenu, eExit };

class cNetCVUpdateMenu : public cOsdMenu
{
    private:
        cNetCVThread *UpdateThread_;
        eState State_;
        bool urgentUpdate_;
        char *Interface_;
        const char *NetcvUUID_, *NetcvVersion_, *UpdateVersion_;

    public:
        cNetCVUpdateMenu(cNetCVThread *UpdateThread, const char *NetcvUUID, const char *NetcvVersion, const char *UpdateVersion, bool urgentUpdate = false);
        ~cNetCVUpdateMenu();

        virtual void Set();
        eOSState ProcessKey(eKeys key);
        bool CompareVersion(const char *version1, const char *version2);
        void GetInterface();
        void Restart();
};

#endif

