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
 * install_diseqc.h
 *
 ***************************************************************************/

#ifndef INSTALL_MULTIROOM_H
#define INSTALL_MULTIROOM_H

#include "installmenu.h"

#define osLeaveUnchanged osUser4

class cInstallMultiroom:public cInstallSubMenu
{
  private:
    void Set();
    bool Save();
    
    void ConfigureAsStandalone();
    void ConfigureAsClient();
    void ConfigureAsServer();
    
    std::string hddDevice;
    bool hddAvailable;
    int enableMultiroom;
    int useAsServer;
    int noOfClients;
    
    typedef struct {
        std::string IP;
        std::string Name;
        std::string MAC;
    } server_t;
    
    server_t clientServer;


  public:
    cInstallMultiroom(std::string harddiskDevice_="", bool hddAvail=false);
    ~cInstallMultiroom();
    eOSState ProcessKey(eKeys Key);
};

#endif
