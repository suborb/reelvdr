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
 * install_client.h
 *
 ***************************************************************************/

#ifndef INSTALL_CLIENT_H
#define INSTALL_CLIENT_H

#include "installmenu.h"
#include "../avahi/avahiservice.h"

#include <string>

struct server_t
{
    std::string Ip;
    std::string Name;
    bool SQLenabled;
};

class cInstallNetclient:public cInstallSubMenu
{
  private:
    bool MultiRoom_;
    bool MultiRoomAtStartup_;
    bool HarddiskAvailable_;
    std::string UUID_;
    std::string harddiskDevice;
    std::vector < struct ReelBox_t >ReelBoxes;
    std::vector < struct ReelBox_t >AVGsWithDBon;
    std::vector < struct server_t >vHosts_;
    int hostnameStart;          // position of the first hostname on the OSD
    std::string NetServerIP_;
    std::string NetServerName_;
    std::string NetserverMAC_;
    int Network_;
    int Netmask_;
    void Set();
    eOSState ProcessKeySave();
    bool GetNetserverMAC();
    bool Save();
    void ScanNetwork();
    bool IsLocalNetwork(const char *Ip);
    int ConvertIp2Int(const char *Ip);
    void parseAVGsSysconfig();     /** parse AVG's sysconfig downloaded to /tmp/configs and take over the settings */
    void parseAVGsPIN();     /** parse AVG's setup.conf downloaded to /tmp/configs and take over the settings for the PIN-plugin */
    void LoadMcliConfig();
    void SaveMcliTunerCount();
    void StoreMcli();
    // Tuner count: Number of tuners a netclient should use in case it connects via Mcli
    int Cable_, Satellite_, SatelliteS2_, Terrestrial_;
    // store the file into this vector
    std::vector < std::string > mclifile;
    bool *showMsg_;

  public:
    cInstallNetclient(std::string harddiskDevice, bool HarddiskAvailable, bool * showMsg);
    ~cInstallNetclient();
    eOSState ProcessKey(eKeys Key);
};
#endif
