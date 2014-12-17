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
 * install_network.h
 *
 ***************************************************************************/

#ifndef INSTALL_NETWORK_H
#define INSTALL_NETWORK_H

#include "installmenu.h"

class cInstallNet:public cInstallSubMenu
{
  private:
    void Set();
    void Load();
    bool Save();
    bool CheckIP();
    bool RunDHCP();
    int NetworkState_;
    char hostname[32], domain[32], gateway[32], nameserver[32], ipaddress[32], netmask[32];
    char dhcp_gateway[32], dhcp_nameserver[32], dhcp_ipaddress[32], dhcp_netmask[32];
    char orig_hostname[32], orig_domain[32], orig_gateway[32], orig_nameserver[32], orig_ipaddress[32], orig_netmask[32];
    int use_internal, use_dhcp;
    int old_use_internal, old_use_dhcp;
    int orig_use_internal, orig_use_dhcp;
    bool wasEdited;
  public:
      cInstallNet();
     ~cInstallNet();
    eOSState ProcessKey(eKeys Key);
};

#endif
