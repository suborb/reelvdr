/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * netcvinstallmenu.h
 *
 ***************************************************************************/

#ifndef NETCVINSTALLMENU_H
#define NETCVINSTALLMENU_H

#include <vector>
#include <vdr/osdbase.h>
#include "netcvdevice.h"
#include "diseqcsetting.h"

#define LNB_MAX 16

class cNetcvInstallMenu : public cOsdMenu
{
    private:
        bool GetInterface();

        char *TmpPath_;
        std::string Interface_;
        cNetCVDevices *NetcvDevices_;
        int NumberOfLNB_;
        int NumberOfTuners_;
        int MaxTuner_;
        std::vector<int> Sources_;
        bool DiseqcSetting_;
        devIt_t GetFirstLiveNetCVDevice(bool& found); 

    public:
        cNetcvInstallMenu(char **TmpPath, const char* title);
        ~cNetcvInstallMenu();

        void Set();
        eOSState ProcessKey(eKeys Key);
        void WriteSetting();
};

#endif
