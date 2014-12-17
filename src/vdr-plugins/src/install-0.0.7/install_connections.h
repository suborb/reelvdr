/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 * install_connections.h
 *
 ***************************************************************************/

#ifndef INSTALL_CONNECTIONS_H
#define INSTALL_CONNECTIONS_H

#include "installmenu.h"
#include "search_netceiver.h"

class cInstallConnectionsAVG:public cInstallSubMenu
{
  private:
    void SetNoNetCeiverFound();
    void SetNetCeiverFound();
    void SetSearchingForNetCeiver();
    void Set();
    bool Save();

    SearchStatus_t status;
    bool NetCeiverFound;
    int vlan_id;
    cString vlan_state;
    
    cTimeMs wait_after_netcv_found;
    bool countdown_started; // after netceiver is found and auto jump to next step
  public:
      cInstallConnectionsAVG();
     ~cInstallConnectionsAVG();
    eOSState ProcessKey(eKeys Key);
};

#endif
