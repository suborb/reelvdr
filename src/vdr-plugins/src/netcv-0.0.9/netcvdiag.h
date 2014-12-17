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
 * netcvdiag.h
 *
 ***************************************************************************/

#ifndef NETCVDIAG_H
#define NETCVDIAG_H

#include <stdio.h>
#include "netcvdevice.h"

class cNetCVDiag
{
    private:
        char *buffer_;
    public:
        cNetCVDiag();
        ~cNetCVDiag();

        FILE *process_;

        bool RunProcess();
        int GetNetceiverCount();
        bool GetInformation(cNetCVDevices *netcvdevices);
        char *GetStdOut();
        int GrepIntValue(std::string strString, std::string strKeyword);
};

#endif
