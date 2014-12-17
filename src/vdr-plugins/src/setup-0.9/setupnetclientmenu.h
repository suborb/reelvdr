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
 * setupnetclientmenu.h
 *
 ***************************************************************************/

#ifndef SETUPNETCLIENTMENU_H
#define SETUPNETCLIENTMENU_H

#include <vector>
#include "vdr/osdbase.h"
#include "vdr/interface.h"
#include "../avahi/avahiservice.h"

//enum eReceiverType { eStreamdev = 0, eMcli = 1 };

typedef struct server {
    std::string IP;
    std::string Name;
    std::string MAC;
} server_t;

typedef struct nrTuners {
    int sat;
    int satS2;
    int cable;
    int terr;
} nrTuners_t;

#define E_NETCLIENT_HOST_CONNECTION_FAILED           -1
#define E_NETCLIENT_DATABASE_LOGIN_FAILED            -2
#define E_NETCLIENT_DATABASE_TABLE_NOT_EXISTENT      -3

bool GetNetserverMAC(server_t *NetServer);

class cNetClientDatabaseChecker
{
    public:
        cNetClientDatabaseChecker();
        ~cNetClientDatabaseChecker();
        static int Check(bool DisplayMessage=false); /** returns 0 if OK, else returns error-number */
};

class cSetupNetClientMenu : public cOsdMenu
{
    private:
        std::vector<struct ReelBox_t> ReelBoxes;
        bool DatabaseAvailable_;
        bool DatabaseAvailableOld_;
        bool ExpertSettings_;
        bool HasHarddisk_;
        bool AVGChanged_; // If you selected AVG then AVGChanged is true
        int ClientMode_;
        int ClientModeOld_;
        int WakeAVG_;
        int WakeAVGOld_;
        eReceiverType ReceiverType_;
        eReceiverType ReceiverTypeOld_;
        nrTuners_t nrTuners;
        nrTuners_t nrTunersOld;
        server_t NetServer;
        std::vector<std::string> mclifile;
        void Set();
        void AddInfo();
        eOSState ProcessKey(eKeys Key);
        bool Save();
        int GetFreeMountNumber();
        std::string GetUUID();

        void LoadMcliConf(); /** loads /etc/default/mcli.conf */
        std::string currText; /** currently selected text */
        bool IsCurrent(std::string str);    
#if VDRVERSNUM >= 10716
        // Easier than patching all GetTitle...
        const char *GetTitle( void) {return Title();}
#endif
    public:
        static void parseAVGsSysconfig(bool *hasNasAsMediaDir);
        
        /** if NetserverIP not empty and hasNasAsMediaDir is false
         * MountDefaultDir() uses /media/hd/ of NetServerIP as mediadir
         */
        static void MountDefaultDir(std::string NetServerIP, int Number, bool hasNasAsMediaDir);

        static void StoreMcliConf(nrTuners_t *nrTuners, eReceiverType type); /** stores /etc/default/mcli.conf */
        static void StoreMcliPlugParams(nrTuners_t *nrTuners); /** passes the mcli-parameters to the mcli-plugin */

        cSetupNetClientMenu(const char *title);
        ~cSetupNetClientMenu();
};

class cSetupNetClientServerMenu : public cOsdMenu
{
    private:
        std::vector<struct ReelBox_t> ReelBoxes;
        int hostnameStart; // position of the first hostname on the OSD
        server_t *NetServer_;
        std::vector<server_t> vHosts_;
        int Network_;
        int Netmask_;
        bool IsLocalNetwork(const char *Ip);
        int ConvertIp2Int(const char *Ip);

    public:
        cSetupNetClientServerMenu(const char *title, server_t *NetServer);
        ~cSetupNetClientServerMenu();

        void ScanNetwork();
        void Set();
        eOSState ProcessKey(eKeys Key);
};

class cSetupNetClientMissingServerMenu : public cOsdMenu
{
    private:
        server_t NetServer;
        void Set();
        eOSState ProcessKey(eKeys Key);
        bool Save();

    public:
        cSetupNetClientMissingServerMenu(const char *title);
        ~cSetupNetClientMissingServerMenu();
};

#endif
