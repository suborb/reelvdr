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
 * netcvdiseqcmenu.h
 *
 ***************************************************************************/

#ifndef NETCVDISEQCMENU_H
#define NETCVDISEQCMENU_H

#include <vdr/menu.h>
#include <string>
#include <vector>

#include "netcvdevice.h"

enum eNetCVState { eInit, eError, eDiagnose, eRestart, eDone };

class cNetCVDiseqcMenu : public cOsdMenu
{
    private:
        std::string directory_;
        std::vector <std::string> files_;
        cNetCVDevices *netcvdevices_;
        devIt_t currentnetceiver_;

    public:
        cNetCVDiseqcMenu();
        ~cNetCVDiseqcMenu();

        void Sethelp(const char *Red = NULL, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL) { SetHelp(Red, Green, Yellow, Blue); };
        int netcvState_;
        void SetErrorPage(int error);
        virtual void Set();
        eOSState ProcessKey(eKeys Key);
};

class cNetCVDiseqcItem : public cOsdItem
{
    private:
        std::string uuid_;
        std::string directory_;
        std::string filename_;
        cNetCVDiseqcMenu *menu_;

    public:
        cNetCVDiseqcItem(std::string uuid, std::string directory, std::string filename, cNetCVDiseqcMenu *menu);
        ~cNetCVDiseqcItem();

        int Upload();
        void Restart(bool bRestartVDR);
        eOSState ProcessKey(eKeys Key);
};

class cNetCVCurrentDeviceItem : public cOsdItem
{
    private:
        devIt_t *currentnetceiver_;
        devIt_t firstnetceiver_;
        devIt_t lastnetceiver_;
    public:
        cNetCVCurrentDeviceItem(devIt_t *currentnetceiver, devIt_t firstnetceiver, devIt_t lastnetceiver);
        ~cNetCVCurrentDeviceItem();

        eOSState ProcessKey(eKeys Key);
};

#endif
