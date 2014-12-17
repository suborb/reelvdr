/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 additions by Tobias Bratfisch                      *
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
 * install_client.c
 *
 ***************************************************************************/

#include "install_client.h"
#include "menueditrefreshitems.h"
#include "netinfo.h"
#include <vdr/device.h>
#include <vdr/sysconfig_vdr.h>
#include <sstream>
#include <fstream>
#include <iostream>

#define CONFIG_DIRECTORY "/etc/vdr/"

//#######################################################################
// cInstallNetclient
//#######################################################################
cInstallNetclient::cInstallNetclient(std::string harddiskDevice_, bool HarddiskAvailable, bool * showMsg) : cInstallSubMenu(tr("MultiRoom"))
{
    AddFloatingText(tr("Scanning for ReelBox MultiRoom server on network. Please wait..."), 45);
    Display();
    Skins.Message(mtStatus, tr("Please wait..."));

    SetCols(2, 15);

    MultiRoom_ = MultiRoomAtStartup_ = (Setup.ReelboxMode == eModeClient);
    HarddiskAvailable_ = HarddiskAvailable;
    UUID_ = "";
    NetServerIP_ = "";
    NetServerName_ = "";
    NetserverMAC_ = "";
    harddiskDevice = harddiskDevice_;
    showMsg_ = showMsg;

    // Gather infos about Network
    cNetInfo netinfo;
    int i = 0;
    bool found = false;
    while (i < netinfo.IfNum() && !found)
    {
        if (strcmp(netinfo[i]->IfName(), "eth0") == 0)
        {
            char *ptr = netinfo[i]->IpAddress();
            int Ipaddress = ConvertIp2Int(ptr);
            free(ptr);
            ptr = netinfo[i]->NetMask();
            Netmask_ = ConvertIp2Int(ptr);
            free(ptr);
            Network_ = Ipaddress & Netmask_;
            found = true;
        }
    }

    ScanNetwork();

    /** TB: a trick to prevent the skin to clear the last picture */
    struct StructImage Image;
    Image.x = 1;
    Image.y = 1;
    Image.w = Image.h = 0;
    Image.blend = true;
    Image.slot = 0;
    strcpy(Image.path, "");
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    Image.slot = 1;
    cPluginManager::CallAllServices("setThumb", (void *)&Image);

    Set();
}

cInstallNetclient::~cInstallNetclient()
{
    vHosts_.clear();
}

void cInstallNetclient::Set()
{
    std::string strBuff;
    int current = Current();
    char currentText[256];
    if (current >= 0)
        strncpy(currentText, Get(Current())->Text(), 256);
    else
        strcpy(currentText, "");
    Clear();

    if (vHosts_.size())
    {
        if (MultiRoomAtStartup_)
            Add(new cOsdItem(tr("The MultiRoom system is already active."), osUnknown, false));
    }
    else
        AddFloatingText(tr("The MultiRoom system cannot be activated."), MAX_LETTER_PER_LINE);

    Add(new cOsdItem("", osUnknown, false));

    if (vHosts_.size())
    {
        if (MultiRoomAtStartup_)
        {
            if (vHosts_.size() == 1)
                AddFloatingText(tr("This ReelBox Avantgarde is currently configured as recording server:"), MAX_LETTER_PER_LINE);
            else
                AddFloatingText(tr("These ReelBox Avantgardes are found on the network and can be "
                                   "used as a recording server (the first one was used up to now):"), MAX_LETTER_PER_LINE);
            Add(new cOsdItem("", osUnknown, false));
            hostnameStart = Count();
            unsigned int i;
            bool showAVGNoMultiroom = false;
            for (i = 0; i < vHosts_.size(); i++)
            {
                if (!strcmp(vHosts_.at(i).Ip.c_str(), Setup.NetServerIP))
                {
                    strBuff = std::string("\t") + vHosts_.at(i).Name + "\t(" + vHosts_.at(i).Ip + ")";
                    if (vHosts_.at(i).SQLenabled == false)
                    {
                        strBuff += " *";
                        showAVGNoMultiroom = true;
                    }
                    break;
                }
            }
            Add(new cOsdItem(strBuff.c_str(), osUnknown, true), false);
            for (i = 0; i < vHosts_.size(); i++)
            {
                if (strcmp(vHosts_.at(i).Ip.c_str(), Setup.NetServerIP))
                {
                    strBuff = std::string("\t") + vHosts_.at(i).Name + "\t(" + vHosts_.at(i).Ip + ")";
                    if (vHosts_.at(i).SQLenabled == false)
                    {
                        strBuff += " *";
                        showAVGNoMultiroom = true;
                    }
                    Add(new cOsdItem(strBuff.c_str(), osUnknown, true), false);
                }
            }
            //strBuff = std::string("\t") + vHost_.at(i).Name + "\t(" + vHosts_.at(i).Ip + ")";
            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("\tDisable MultiRoom"), osUser2, true));
            if (showAVGNoMultiroom)
            {
                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(tr("Note:"), osUnknown, false));
                AddFloatingText(tr("* MultiRoom is not enabled!"), MAX_LETTER_PER_LINE);
            }
        }
        else if (vHosts_.size() == 1)
        {
            strBuff = std::string("\t") + vHosts_.at(0).Name + "\t(" + vHosts_.at(0).Ip + ")";
            if (vHosts_.at(0).SQLenabled == false)
            {
                strBuff += " *";
                AddFloatingText(tr("This ReelBox Avantgarde is found, but has MultiRoom disabled, so cannot be used as recording server:"), MAX_LETTER_PER_LINE);
                Add(new cOsdItem("", osUnknown, false));
                hostnameStart = Count();
                Add(new cOsdItem(strBuff.c_str(), osUnknown, false), true);
            }
            else
            {
                AddFloatingText(tr("This ReelBox Avantgarde is found and will be used as recording server:"), MAX_LETTER_PER_LINE);
                Add(new cOsdItem("", osUnknown, false));
                hostnameStart = Count();
                Add(new cOsdItem(strBuff.c_str(), osUnknown, true), true);
            }
            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("\tLeave MultiRoom unconfigured"), osUser2, true));
            if (vHosts_.at(0).SQLenabled == false)
            {
                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(tr("Note:"), osUnknown, false));
                AddFloatingText(tr("* MultiRoom is not enabled!"), MAX_LETTER_PER_LINE);
            }
        }
        else
        {
            AddFloatingText(tr("These ReelBox Avantgardes are found on the network and can be " "used as a recording server."), MAX_LETTER_PER_LINE);
            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("Please select one of them:"), osUnknown, false));
            hostnameStart = Count();
            strBuff = std::string("\t") + vHosts_.at(0).Name + "\t(" + vHosts_.at(0).Ip + ")";
            bool showAVGNoMultiroom = false;
            if (vHosts_.at(0).SQLenabled == false)
            {
                strBuff += " *";
                showAVGNoMultiroom = true;
            }
            Add(new cOsdItem(strBuff.c_str(), osUnknown, true), true);
            for (unsigned int i = 1; i < vHosts_.size(); ++i)
            {
                strBuff = std::string("\t") + vHosts_.at(i).Name + "\t(" + vHosts_.at(i).Ip + ")";
                if (vHosts_.at(i).SQLenabled == false)
                {
                    strBuff += " *";
                    showAVGNoMultiroom = true;
                }
                Add(new cOsdItem(strBuff.c_str(), osUnknown, true));
            }
            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("\tLeave MultiRoom unconfigured"), osUser2, true));
            if (showAVGNoMultiroom)
            {
                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(tr("Note:"), osUnknown, false));
                AddFloatingText(tr("* MultiRoom is not enabled!"), MAX_LETTER_PER_LINE);
            }
        }
    }
    else
    {
        AddFloatingText(tr("No ReelBox Avantgarde was found in the network."), MAX_LETTER_PER_LINE);
        AddFloatingText(tr("If you own an Avantgarde, turn it on and activate the MultiRoom " "System in Setup/Installation/MultiRoom."), MAX_LETTER_PER_LINE);
    }

    if (!MultiRoom_)
        SetCurrent(Get(current));
    SetHelp(tr("Rescan"), tr("Back"), tr("Skip"));
    Display();
}

eOSState cInstallNetclient::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (state)
    {
    case osUnknown:
        if (Key == kOk && (int)vHosts_.size() > Current() - hostnameStart)
        {
            for (unsigned int i = 0; i < vHosts_.size(); i++)
            {
                if (strstr(Get(Current())->Text(), vHosts_.at(i).Ip.c_str()) && vHosts_.at(i).SQLenabled == false)
                    return osContinue;
            }
            MultiRoom_ = true;
            return ProcessKeySave();
        }
        if (Key == kRed)
        {
            Skins.Message(mtStatus, tr("Please wait..."));
            cPlugin *p = cPluginManager::GetPlugin("avahi");
            if (p)
                p->Service("Avahi Restart");
            ScanNetwork();
            Set();
            return osContinue;
        }
        break;
    case osUser2:              // "No" selected
        MultiRoom_ = false;
        MultiRoomAtStartup_ = false;
        NetServerIP_ = "";
        NetServerName_ = "";
        if (Save())
            return osUser1;     // Next menu
        else
            return osContinue;
        break;
    case osContinue:
        if (!MultiRoom_)
            switch (Key)
            {
            case kRight:
            case kRight | k_Repeat:
            case kLeft:
            case kLeft | k_Repeat:
            case kDown:
            case kDown | k_Repeat:
            case kUp:
            case kUp | k_Repeat:
                Set();          // Update Info-Text
                break;
            default:
                break;
            }
        break;
    default:
        break;
    }
    return state;
}

eOSState cInstallNetclient::ProcessKeySave()
{
    if (vHosts_.size() == 0)
    {
        NetServerIP_ = "";
        NetServerName_ = "";
    }
    else
    {
        int host_index = 0;
        if (vHosts_.size() >= 2)
            for (unsigned int i = 0; i < vHosts_.size(); i++)
                if (strstr(Get(Current())->Text(), vHosts_.at(i).Ip.c_str()))
                    host_index = i;
#if 0
        if (host_index < 0 || host_index > (int)vHosts_.size())
            printf("\033[1;92m Error: Out of range \033[0m\n");
        printf("Current()->Text '%s' ", Get(Current())->Text());
        printf("vhosts.at(%i).Name '%s' ", Current() - hostnameStart, vHosts_.at(host_index).Name.c_str());
        printf("vhosts.at(%i).Ip '%s'\n", Current() - hostnameStart, vHosts_.at(host_index).Ip.c_str());
#endif
        NetServerIP_ = vHosts_.at(host_index).Ip;
        NetServerName_ = vHosts_.at(host_index).Name;
    }
    if (Save())
        return osUser1;         // Next Menu
    else
        return osContinue;
}

void cInstallNetclient::ScanNetwork()
{
    cCondWait::SleepMs(5000);

    cPlugin *p = cPluginManager::GetPlugin("avahi");
    if (p)
    {
        p->Service("Avahi ReelBox-List", &ReelBoxes);
        p->Service("Avahi MultiRoom-List", &AVGsWithDBon);
    }

    vHosts_.clear();
    for (unsigned int i = 0; i < ReelBoxes.size(); i++)
    {
        if (IsLocalNetwork(ReelBoxes.at(i).Ip.c_str()))
        {
            struct server_t server;
            server.SQLenabled = false;
            for (unsigned int j = 0; j < AVGsWithDBon.size(); j++)
                if (AVGsWithDBon.at(j).Mac == ReelBoxes.at(i).Mac && AVGsWithDBon.at(j).Ip == ReelBoxes.at(i).Ip)
                {
                    server.SQLenabled = true;
                    break;
                }
            server.Name = ReelBoxes.at(i).Name;
            server.Ip = ReelBoxes.at(i).Ip;
            vHosts_.push_back(server);
        }
    }
}

bool cInstallNetclient::IsLocalNetwork(const char *Ip)
{
    int IpInt = ConvertIp2Int(Ip);
    int Network = IpInt & Netmask_;

    if (Network == Network_)
        return true;
    else
        return false;
}

int cInstallNetclient::ConvertIp2Int(const char *Ip)
{
    int retValue = 0;
    char *index = (char *)Ip;

    for (int i = 0; i < 4; ++i)
    {
        retValue = retValue * 0x100 + atoi(index);
        index = strchr(index, '.') + 1;
    }
    return retValue;
}

bool cInstallNetclient::GetNetserverMAC()
{
    std::string command;
    char *buffer;
    char *index = NULL;
    FILE *process;

    // first ping, since arping is not available on NetClient now
    command = std::string("ping -c 1 -q ") + NetServerIP_ + " 1>/dev/null 2>/dev/null";
    SystemExec(command.c_str());

    command = std::string("arp -an ") + NetServerIP_;
    process = popen(command.c_str(), "r");
    if (process)
    {
        cReadLine readline;
        buffer = readline.Read(process);
        if (buffer)
        {
            index = strchr(buffer, ':');
            if (index && strlen(index) >= 15 && (*(index + 3) == ':') && (*(index + 6) == ':') && (*(index + 9) == ':') && (*(index + 12) == ':'))
            {
                *(index + 15) = '\0';
                index -= 2;
            }
            else
                index = NULL;
        }
        pclose(process);
    }
    else
        return false;

    if (index)
    {
        NetserverMAC_ = std::string(index);
        return true;
    }
    else
        return false;
}

void SaveMcliTunerCount();

bool cInstallNetclient::Save()
{
    bool save = true;
    int Number = -1;

    Skins.Message(mtStatus, tr("Setting changes - please wait"));
    SaveMcliTunerCount();       // set Max tuner counts to 1, if they are not set in /etc/defaults/mcli

    if (!MultiRoom_ && HarddiskAvailable_)
    {
        UUID_ = GetUUID(harddiskDevice);
        if (!UUID_.size())
        {
            save = false;
            Skins.Message(mtError, tr("Storing failed! (UUID is missing)"));
        }
    }

    if (save)
    {
        cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

        if (MultiRoom_ && NetServerIP_.size())
        {
            //printf("\033[0;92m Use NetServer! \033[0;m\n");
            std::stringstream strBuff;

            Setup.ReelboxMode = eModeClient;    // NetClient

            // Use AVG
            strcpy(Setup.NetServerIP, NetServerIP_.c_str());
            strncpy(Setup.NetServerName, NetServerName_.c_str(), MAXHOSTNAME);
            if (GetNetserverMAC())
                strcpy(Setup.NetServerMAC, NetserverMAC_.c_str());
            else
                strcpy(Setup.NetServerMAC, "");

            //check and set ReelboxModeTemp
            bool DatabaseAvailable = false;
            cPlugin *p = cPluginManager::GetPlugin("avahi");
            if (p)
                p->Service("Avahi check NetServer", &DatabaseAvailable);
            Setup.ReelboxModeTemp = DatabaseAvailable ? eModeClient : eModeStandalone;

#if 0
            // Setup Streamdev-Client
            if (cPlugin * StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client"))
            {
                StreamdevPlugin->SetupStore("RemoteIp", NetServerIP_.c_str());
                StreamdevPlugin->SetupParse("RemoteIp", NetServerIP_.c_str());
                StreamdevPlugin->SetupStore("StartClient", 1);
                StreamdevPlugin->SetupParse("StartClient", "1");
                StreamdevPlugin->SetupStore("SyncEPG", 1);
                StreamdevPlugin->SetupParse("SyncEPG", "1");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }
#endif

            // Network Mount
            //printf("\033[0;92m Store recordings on AVG! \033[0;m\n");
            Number = 0;         //TB: GetFreeMountNumber(); ?????  // use #0 (reserved for NetClient-AVG-Mount)
            if (Number != -1)
            {
                SystemExec("/usr/sbin/setup-shares stop 0");
                // Store NFS-Shares
                strBuff << "MOUNT" << Number << "_NFS_STARTCLIENT";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_HOST";
                sysconfig.SetVariable(strBuff.str().c_str(), Setup.NetServerIP);
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_SHARENAME";
                sysconfig.SetVariable(strBuff.str().c_str(), "/media/hd");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_DISPLAYNAME";
                sysconfig.SetVariable(strBuff.str().c_str(), "Avantgarde");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_PICTURES";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_MUSIC";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_RECORDINGS";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_VIDEOS";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");

                // NFS-specified options
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_NETWORKPROTOCOL";
                sysconfig.SetVariable(strBuff.str().c_str(), "tcp");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_BLOCKSIZE";
                sysconfig.SetVariable(strBuff.str().c_str(), "8192");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_OPTIONHS";
                sysconfig.SetVariable(strBuff.str().c_str(), "soft");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_OPTIONLOCK";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_OPTIONRW";
                sysconfig.SetVariable(strBuff.str().c_str(), "yes");
                strBuff.str("");
                strBuff << "MOUNT" << Number << "_NFS_NFSVERSION";
                sysconfig.SetVariable(strBuff.str().c_str(), "3");

                // Use NFS-Mount as MediaDevice
                strBuff.str("");
                strBuff << "/media/.mount" << Number;
                sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
                sysconfig.Save();

                strBuff.str("");
                strBuff << "setup-shares start " << Number << "; setup-mediadir restart ";
                SystemExec(strBuff.str().c_str());
            }
            else
            {
                // Use NFS-Mount as MediaDevice
                strBuff.str("");
                strBuff << "/media/.mount" << Number;
                sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
                sysconfig.Save();
                strBuff.str("");
                strBuff << "setup-shares start " << Number << "; setup-mediadir restart ";
                SystemExec(strBuff.str().c_str());
            }

            // Update Timers
            Timers.Load();      // Clear Timers
#ifdef USEMYSQL
            Timers.LoadDB();    // Get Timers from DB
#endif
        }
        else
        {
            //printf("\033[0;92m No NetServer! \033[0;m\n");
            Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone;    // Standalone

            strcpy(Setup.NetServerIP, "");
            strcpy(Setup.NetServerName, "");
            strcpy(Setup.NetServerMAC, "");

            // Disable Streamdev-Client
            if (cPlugin * StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client"))
            {
                StreamdevPlugin->SetupStore("RemoteIp", "");
                StreamdevPlugin->SetupParse("RemoteIp", "");
                StreamdevPlugin->SetupStore("StartClient", 0);
                StreamdevPlugin->SetupParse("StartClient", "0");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }

            // Use local harddisk if available
            if (HarddiskAvailable_)
                sysconfig.SetVariable("MEDIADEVICE", UUID_.c_str());
            else
                sysconfig.SetVariable("MEDIADEVICE", "");
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

            Timers.Load("/etc/vdr/timers.conf");
        }

        Setup.Save();
        sysconfig.Save();

        if (MultiRoom_ && NetServerIP_.size())
        {
            // Call script here to download the config files from selected AVG server
            std::string buf = std::string("getConfigsFromAVGServer.sh ") + NetServerIP_ + " all";
            int ret = SystemExec(buf.c_str());
            if (ret == 0 || 1 /* ignore return value, always 256!*/)
            {                   // Selected Text is the IP of the server
                Channels.Load(AddDirectory(CONFIG_DIRECTORY, "channels.conf"), false, true);
                cDevice::PrimaryDevice()->SwitchChannel(1); // go to first channel, get the video running
                parseAVGsSysconfig();
                parseAVGsPIN();
                cPluginManager::CallAllServices("reload favourites list");
                SystemExec("setup-mediadir");
            }
            else
            {
                if (showMsg_)
                *showMsg_ = true;
                printf("ERROR while downloading configs from AVG\n");
                esyslog("ERROR while downloading configs from AVG");
            }
        }
        else
        {
            printf("No MultiRoom_ or no NetServer found. Not retrieving configs.\n");
        }
        Skins.Message(mtInfo, tr("Settings stored"));
    }

    return save;
}

void cInstallNetclient::parseAVGsPIN()
{
    std::ifstream ifile("/tmp/configs/setup.conf");
    char buf[256];
    int pinCode = -1, autoProtectionMode = -1, pinResetTime = -1, skipChannelSilent = -1;
    std::string pinCode_str, autoProtectionMode_str, pinResetTime_str, skipChannelSilent_str;
    buf[0] = '\0';

    while (ifile.good())
    {
        ifile.getline(buf, 256);
        if (strlen(buf) >= 4 + 3 && !strncmp(buf, "pin.", 4))
        {
            if (strlen(buf) >= 18 + 3 + 4 && !strncmp(buf + 4, "autoProtectionMode", 18))
            {
                autoProtectionMode = atoi(buf + 4 + 3 + 18);
                autoProtectionMode_str = buf + 4 + 3 + 18;
            }
            else if (strlen(buf) >= 7 + 3 + 4 && !strncmp(buf + 4, "pinCode", 7))
            {
                pinCode = atoi(buf + 4 + 3 + 7);
                pinCode_str = buf + 4 + 3 + 7;
            }
            else if (strlen(buf) >= 12 + 3 + 4 && !strncmp(buf + 4, "pinResetTime", 12))
            {
                pinResetTime = atoi(buf + 4 + 3 + 12);
                pinResetTime_str = buf + 4 + 3 + 12;
            }
            else if (strlen(buf) >= 17 + 3 + 4 && !strncmp(buf + 4, "skipChannelSilent", 17))
            {
                skipChannelSilent = atoi(buf + 4 + 3 + 17);
                skipChannelSilent_str = buf + 4 + 3 + 17;
            }
        }
    }
    ifile.close();
    //std::cout << "autoProtectionMode: " << autoProtectionMode << " pinCode: " << pinCode << " pinResetTime: " << pinResetTime << " skipChannelSilent: " << skipChannelSilent << std::endl;

    cPlugin *pinPlugin = cPluginManager::GetPlugin("pin");
    if (pinPlugin)
    {
        if (autoProtectionMode != -1 && pinCode != -1 && pinResetTime != -1 && skipChannelSilent != -1)
        {
            pinPlugin->SetupParse("pinCode", pinCode_str.c_str());
            pinPlugin->SetupStore("pinCode", pinCode);
            pinPlugin->SetupParse("autoProtectionMode", autoProtectionMode_str.c_str());
            pinPlugin->SetupStore("autoProtectionMode", autoProtectionMode);
            pinPlugin->SetupParse("pinResetTime", pinResetTime_str.c_str());
            pinPlugin->SetupStore("pinResetTime", pinResetTime);
            pinPlugin->SetupParse("skipChannelSilent", skipChannelSilent_str.c_str());
            pinPlugin->SetupStore("skipChannelSilent", skipChannelSilent);
            Setup.Save();
        }
    }
    else
    {
        std::cout << "ERROR: couldn't get the pin-plugin" << std::endl;
        esyslog("ERROR: couldn't get the pin-plugin");
    }

}


#if 1

/**
 * @brief cInstallNetclient::parseAVGsSysconfig calls parse external config service
 *  from setup plugin
 */

void cInstallNetclient::parseAVGsSysconfig()
{
    cPluginManager::CallAllServices("parse external sysconfig");
}

#else
void cInstallNetclient::parseAVGsSysconfig()
{
    std::ifstream ifile("/tmp/configs/sysconfig");
    char buf[256];
    bool vlan = false;
    std::string vlan_str;
    std::string mediadev_str, mdev_only_rec_str;
    int mountNr = -1;
    bool isNFS = false;
    bool videos = false, recordings = false, pictures = false, music = false, softlock = false, optionrw = false, optionlock = false, startclient = false;
    std::string username, password, displayname, host, sharename, protocol;
    std::stringstream strBuff;
    int blkSize = 0;
    int nfsVersion = 3;

    while (ifile.good())
    {
        ifile.getline(buf, 256);
        if (!strncmp(buf, "VLAN=", 5))
            vlan = (buf[6] == 'y');
        else if (!strncmp(buf, "VLAN_ID=", 8))
            vlan_str = buf + 8;
        else if (!strncmp(buf, "MEDIADEVICE=", 12))
            mediadev_str = buf + 12;
        else if (!strncmp(buf, "MEDIADEVICE_ONLY_RECORDING=", 28))
            mdev_only_rec_str = buf + 29;
    }
    ifile.close();

    unsigned int pos = mediadev_str.find("/media/.mount");
    if (pos != std::string::npos)
    {
        mountNr = atoi(mediadev_str.c_str() + 14);
    }
    ifile.open("/tmp/configs/sysconfig");
    while (ifile.good())
    {
        ifile.getline(buf, 256);
        if (strlen(buf) > 10 && !strncmp(buf, "MOUNT", 5) && buf[5] == '0' + mountNr)
        {
            if (!strncmp(buf + 7, "NFS", 3))
                isNFS = true;
            if (!strncmp(buf + 11, "BLOCKSIZE", 9))
            {
                blkSize = atoi(buf + 22);
            }
            else if (!strncmp(buf + 11, "DISPLAYNAME", 11))
            {
                displayname = std::string(buf + 24);
                displayname.erase(displayname.size() - 1);
            }
            else if (!strncmp(buf + 11, "HOST", 4))
            {
                host = std::string(buf + 17);
                host.erase(host.size() - 1);
            }
            else if (!strncmp(buf + 11, "MUSIC", 5))
            {
                if (!strncmp(buf + 18, "yes", 3))
                    music = true;
            }
            else if (!strncmp(buf + 11, "NETWORKPROTOCOL", 15))
            {
                protocol = std::string(buf + 28);
                protocol.erase(protocol.size() - 1);
            }
            else if (!strncmp(buf + 11, "NFSVERSION", 10))
            {
                nfsVersion = atoi(buf + 23);
            }
            else if (!strncmp(buf + 11, "OPTIONHS", 8))
            {
                if (!strncmp(buf + 21, "soft", 4))
                    softlock = true;
            }
            else if (!strncmp(buf + 11, "OPTIONLOCK", 10))
            {
                if (!strncmp(buf + 23, "yes", 3))
                    optionlock = true;
            }
            else if (!strncmp(buf + 11, "OPTIONRW", 8))
            {
                if (!strncmp(buf + 21, "yes", 3))
                    optionrw = true;
            }
            else if (!strncmp(buf + 11, "PICTURES", 8))
            {
                if (!strncmp(buf + 21, "yes", 3))
                    pictures = true;
            }
            else if (!strncmp(buf + 11, "RECORDINGS", 10))
            {
                if (!strncmp(buf + 23, "yes", 3))
                    recordings = true;
            }
            else if (!strncmp(buf + 11, "SHARENAME", 9))
            {
                sharename = std::string(buf + 22);
                sharename.erase(sharename.size() - 1);
            }
            else if (!strncmp(buf + 11, "STARTCLIENT", 10))
            {
                if (!strncmp(buf + 23, "yes", 3))
                    startclient = true;
            }
            else if (!strncmp(buf + 11, "VIDEOS", 6))
            {
                if (!strncmp(buf + 19, "yes", 3))
                    videos = true;
            }
            else if (!strncmp(buf + 11, "USERNAME", 8))
            {
                username = std::string(buf + 21);
                username.erase(username.size() - 1);
            }
            else if (!strncmp(buf + 11, "PASSWORD", 8))
            {
                password = std::string(buf + 21);
                password.erase(password.size() - 1);
            }
        }
    }
    ifile.close();
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    if (mountNr != -1)
    {
        if (isNFS)
        {
            char strBuff2[16];
            int Number = 0;
            SystemExec("/usr/sbin/setup-shares stop 0");
            // Store NFS-Shares
            strBuff << "MOUNT" << Number << "_NFS_STARTCLIENT";
            sysconfig.SetVariable(strBuff.str().c_str(), startclient ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_HOST";
            sysconfig.SetVariable(strBuff.str().c_str(), host.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_SHARENAME";
            sysconfig.SetVariable(strBuff.str().c_str(), sharename.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_DISPLAYNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), displayname.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_PICTURES";
            sysconfig.SetVariable(strBuff.str().c_str(), pictures ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_MUSIC";
            sysconfig.SetVariable(strBuff.str().c_str(), music ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_RECORDINGS";
            sysconfig.SetVariable(strBuff.str().c_str(), recordings ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_VIDEOS";
            sysconfig.SetVariable(strBuff.str().c_str(), videos ? "yes" : "no");

            // NFS-specified options
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_NETWORKPROTOCOL";
            sysconfig.SetVariable(strBuff.str().c_str(), protocol.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_BLOCKSIZE";
            snprintf(strBuff2, 16, "%i", blkSize);
            sysconfig.SetVariable(strBuff.str().c_str(), strBuff2);
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_OPTIONHS";
            sysconfig.SetVariable(strBuff.str().c_str(), softlock ? "soft" : "hard");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_OPTIONLOCK";
            sysconfig.SetVariable(strBuff.str().c_str(), optionlock ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_OPTIONRW";
            sysconfig.SetVariable(strBuff.str().c_str(), optionrw ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_NFS_NFSVERSION";
            snprintf(strBuff2, 16, "%i", nfsVersion);
            sysconfig.SetVariable(strBuff.str().c_str(), strBuff2);

            // Use NFS-Mount as MediaDevice
            strBuff.str("");
            strBuff << "/media/.mount" << Number;
            sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            sysconfig.Save();
        }
        else if (host.size() && sharename.size())
        {
            int Number = 0;
            SystemExec("/usr/sbin/setup-shares stop 0");
            // Store SMB-Shares
            strBuff << "MOUNT" << Number << "_SMB_STARTCLIENT";
            sysconfig.SetVariable(strBuff.str().c_str(), startclient ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_HOST";
            sysconfig.SetVariable(strBuff.str().c_str(), host.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_SHARENAME";
            sysconfig.SetVariable(strBuff.str().c_str(), sharename.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_DISPLAYNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), displayname.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_PICTURES";
            sysconfig.SetVariable(strBuff.str().c_str(), pictures ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_MUSIC";
            sysconfig.SetVariable(strBuff.str().c_str(), music ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_RECORDINGS";
            sysconfig.SetVariable(strBuff.str().c_str(), recordings ? "yes" : "no");
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_VIDEOS";
            sysconfig.SetVariable(strBuff.str().c_str(), videos ? "yes" : "no");

            // SMB-specified options
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_PASSWORD";
            sysconfig.SetVariable(strBuff.str().c_str(), password.c_str());
            strBuff.str("");
            strBuff << "MOUNT" << Number << "_SMB_USERNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), username.c_str());

            // Use SMB-Mount as MediaDevice
            strBuff.str("");
            strBuff << "/media/.mount" << Number;
            sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            sysconfig.Save();
        }
    }

    if (vlan)
    {
        sysconfig.SetVariable("VLAN", "yes");
        sysconfig.SetVariable("VLAN_ID", vlan_str.substr(1, 1).c_str());
        sysconfig.Save();
        SystemExec("setup-net restart");
        cCondWait::SleepMs(1000);   // wait a bit until the network is up
    }
    else
    {
        sysconfig.SetVariable("VLAN", "no");
        sysconfig.Save();
    }
}
#endif

// taken from setup-plugin
void cInstallNetclient::LoadMcliConfig()    // temporary till mcli-plugin is released
{
    char *buffer;
    std::string strBuff;
    FILE *file = NULL;

    // default
    Cable_ = Satellite_ = SatelliteS2_ = Terrestrial_ = 1;

    file = fopen("/etc/default/mcli", "r");
    mclifile.clear();

    if (file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while (buffer)
        {
            strBuff = buffer;
            if (strstr(buffer, "DVB_C_DEVICES=\"") && !strstr(buffer, "\"\""))
                Cable_ = atoi(buffer + 15);
            else if (strstr(buffer, "DVB_S_DEVICES=\"") && !strstr(buffer, "\"\""))
                Satellite_ = atoi(buffer + 15);
            else if (strstr(buffer, "DVB_S2_DEVICES=\"") && !strstr(buffer, "\"\""))
                SatelliteS2_ = atoi(buffer + 16);
            else if (strstr(buffer, "DVB_T_DEVICES=\"") && !strstr(buffer, "\"\""))
                Terrestrial_ = atoi(buffer + 15);

            mclifile.push_back(strBuff);    // collect all lines of the file
            buffer = readline.Read(file);
        }
        fclose(file);
    }

#if 0
    //Test
    for (unsigned int i = 0; i < mclifile.size(); ++i)
        printf("\033[0;93m %s \033[0m\n", mclifile.at(i).c_str());
#endif
}

// taken from setup-plugin
void cInstallNetclient::StoreMcli()
{                               // temporary till mcli-plugin is released
    if (cPlugin * McliPlugin = cPluginManager::GetPlugin("mcli"))
    {
        std::stringstream buf;
        buf << Cable_;
        McliPlugin->SetupStore("DVB-C", Cable_);
        McliPlugin->SetupParse("DVB-C", buf.str().c_str());
        buf.str("");
        buf << Satellite_;
        McliPlugin->SetupStore("DVB-S", Satellite_);
        McliPlugin->SetupParse("DVB-S", buf.str().c_str());
        buf.str("");
        buf << Terrestrial_;
        McliPlugin->SetupStore("DVB-T", Terrestrial_);
        McliPlugin->SetupParse("DVB-T", buf.str().c_str());
        buf.str("");
        buf << SatelliteS2_;
        McliPlugin->SetupStore("DVB-S2", SatelliteS2_);
        McliPlugin->SetupParse("DVB-S2", buf.str().c_str());
    }
    else
        esyslog("ERROR: couldn't get the mcli-plugin!");

    std::ifstream ifile("/etc/default/mcli");
    char buf[256];
    std::stringstream lineBuf;
    while (ifile.good())
    {
        ifile.getline(buf, 256);
        if (!strncmp(buf, "NETWORK_INTERFACE=", 18) || !strncmp(buf, "MCLI_ENABLED=", 13))
        {
            lineBuf << buf << std::endl;
        }
    }
    ifile.close();

    std::ofstream file("/etc/default/mcli", std::ios_base::trunc);

    //file << "MCLI_ENABLED=\"" << ReceiverType_==eMcli?"true":"false" << "\"" << std::endl;
    if (lineBuf.str().size())
        file << lineBuf.str();
    file << "DVB_C_DEVICES=\"" << Cable_ << "\"" << std::endl;
    file << "DVB_S_DEVICES=\"" << Satellite_ << "\"" << std::endl;
    file << "DVB_S2_DEVICES=\"" << SatelliteS2_ << "\"" << std::endl;
    file << "DVB_T_DEVICES=\"" << Terrestrial_ << "\"" << std::endl;

    file.close();

#if 0
    cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
    if (mcliPlugin)
    {
        mcliPlugin->Service("Reinit");
    }
#endif
/*
    else {
        if(ReceiverType_ == eMcli)
            SystemExec("/etc/init.d/mcli restart");
        else
            SystemExec("/etc/init.d/mcli stop");
    }
*/
}

void cInstallNetclient::SaveMcliTunerCount()
{
    LoadMcliConfig();
    StoreMcli();
}
