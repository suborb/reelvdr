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
 * netcvmenu.h
 *
 ***************************************************************************/

#ifndef NETCVMENUINFO_H
#define NETCVMENUINFO_H

#include <vector>
#include <vdr/osdbase.h>
#include "netcv.h"
#include "netcvmenu.h"
#include "netcvdevice.h"

class cNetCVInfoMenu;

class cNetCVInfoMenu : public cOsdMenu
{
    private:
        cNetCVDevices *netcvdevices_;
        std::string directory_;
        std::vector <std::string> files_;
        int diseqcError_;

    public:
        int netcvState_;
        cNetCVInfoMenu();
        ~cNetCVInfoMenu();

        virtual void AddLine(std::string strItem, std::string strValue, int level);
        virtual void Set();
        virtual void Display();
        eOSState ProcessKey(eKeys key);
};

#endif
