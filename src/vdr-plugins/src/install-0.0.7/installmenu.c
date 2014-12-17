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
 * installmenu.c
 *
 ***************************************************************************/

#include "install.h"
#include "installmenu.h"
#include "install_language.h"
#ifdef RBMINI
#  include "install_video_mini.h"
#else
#  include "install_video.h"
#  include "install_video_ice.h"
#endif
#include "install_connections.h"
#include "install_diseqc.h"
#include "install_channellist.h"
#include "install_channelscan.h"
#include "install_network.h"
#include "install_client.h"
#include "install_formathd.h"
#include "install_harddisk.h"
#include "install_multiroom.h"
#include "install_update.h"

#include <vdr/sysconfig_vdr.h>
#include <vdr/s2reel_compat.h>
#include "../avahi/avahiservice.h"
#include <vdr/device.h>
#include <vdr/config.h>
#include <vdr/interface.h>

#include <algorithm>
#include <string>
#include <sstream>

#include <sys/socket.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/sockios.h>

#define MOD_CONTROLLER "libata"


const char* DefaultNetworkDevice() 
{
    if (IsNetworkDevicePresent("eth1"))
        return "eth1";
    else
        return "eth0";
}


std::string AvailableHardDisk() {
#ifdef RBMINI
return "sdb";
#else

    cSystemDevices list = system_devices(eDeviceInternalDisk);
    cSystemDevices::iterator it = list.begin();
    printf("\n ====== \n");
    while (it != list.end()) { it->Print(); ++it; }
    printf(" ====== \n");

    if (cPluginManager::GetPlugin("reelbox")) {
        return "";
    } else if (cPluginManager::GetPlugin("reelice")) {
        // leave out "sda" (card reader), and root disk 
        std::string hdd = "";
    
        it = list.begin();
        while (it != list.end()) { 
            if (//*it->Path() != "/dev/sda" /*card reader*/ && 
                    !IsRootDisk(it->Path())
                    && it->Size() > 10000 /* 10GB in MB */
                    && it->Bus() == "ata")
                break;
            
            printf("skipping: "); it->Print();
            ++it;
        } // while
        
        if (it != list.end()) 
            hdd = it->Path();
        
        printf("\n\033[0;91mAvailable Hdd: '%s'\033[0m\n", hdd.c_str());
        return hdd;
    } // if (reelice)
    
    return "";
#endif
}

unsigned int cInstallMenu::setupState = 0;
int *cInstallMenu::ErrorState_ = NULL;
bool CheckNetceiver()
{
    bool retvalue = false;
    const char *command = "netcvdiag -a | grep ALIVE";
    FILE *process;
    char *strBuff;
    process = popen(command, "r");
    if (process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        if (strBuff)
            retvalue = true;
        pclose(process);
    }
    return retvalue;
}

bool CheckNetworkCarrier()
{                               // Checks if the LAN-Cable is plugged in eth1
    int fd;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.data = NULL;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, NETWORK_DEVICE, sizeof(ifr.ifr_name) - 1);
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t) & edata;
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)
    {
        perror("ETHTOOL_GLINK");
        return false;
    }
    close(fd);

    if (edata.data)
        return true;
    else
        return false;
}

int GetFreeMountNumber()
{
    std::vector < int >usedNumbers;
    int freeNumber = 0;
    std::stringstream buffer;
    char *buffer2;
    FILE *process;
    FILE *process2;

    // Get Numbers of used NetworkMounts
    process = popen("grep HOST /etc/default/sysconfig |grep ^MOUNT| sed -e 's/MOUNT//' | awk -F'_' '{print $1 }'", "r");
    if (process)
    {
        cReadLine readline;
        buffer2 = readline.Read(process);
        while (buffer2)
        {
            usedNumbers.push_back(atoi(buffer2));
            buffer2 = readline.Read(process);
        }
        pclose(process);
    }
    sort(usedNumbers.begin(), usedNumbers.end());

    // Checks if the mount already exists
    for (unsigned int i = 0; i < usedNumbers.size(); ++i)
    {
        buffer << "grep MOUNT" << usedNumbers.at(i) << "_NFS_HOST /etc/default/sysconfig | grep " << Setup.NetServerIP;
        process = popen(buffer.str().c_str(), "r");
        if (process)
        {
            cReadLine readline;
            buffer2 = readline.Read(process);
            if (buffer2)
            {
                buffer.str("");
                buffer << "grep MOUNT" << usedNumbers.at(i) << "_NFS_SHARENAME /etc/default/sysconfig | grep /media/hd";
                process2 = popen(buffer.str().c_str(), "r");
                if (process2)
                {
                    cReadLine readline;
                    buffer2 = readline.Read(process2);
                    if (buffer2)
                        freeNumber = -1;
                    pclose(process2);
                }
            }
            pclose(process);
            if (freeNumber == -1)
                return -1;
        }
    }

    // check for gaps
    for (unsigned int i = 0; i < usedNumbers.size(); ++i)
        if (freeNumber == usedNumbers.at(i))
            ++freeNumber;

    return freeNumber;
}

std::string GetUUID(std::string harddiskDevice)
{
    std::string UUID;
    std::string command;
    FILE *file;
    char *buffer;

    // we need only the device name not its fullpath.
    // 'sdf1' not '/dev/sdf1'
    std::string::size_type idx = harddiskDevice.find_last_of('/');
    if (idx != std::string::npos) // assume fullpath, so trim it
        harddiskDevice = harddiskDevice.substr(idx+1);

    command = std::string("ls -l /dev/disk/by-uuid/ | grep ") + harddiskDevice;
    file = popen(command.c_str(), "r");
    if (file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if (buffer)
        {
            *(strrchr(buffer, '>') - 2) = '\0';
            UUID = strrchr(buffer, ' ') + 1;
        }
        else
            UUID = "";
        pclose(file);
    }

    isyslog("%s HDD device '%s' --> UUID: '%s'", __PRETTY_FUNCTION__,
            harddiskDevice.c_str(), UUID.c_str());
    return UUID;
}

//#######################################################################
// cInstallMenu
//#######################################################################
cInstallMenu::cInstallMenu(int startWithStep, bool _skipChannelscan):cOsdMenu("Install Wizard", 30)
{
    HarddiskAvailable_ = false;
    InternalHD_ = false;
    skipChannelscan_ = false;
    chListShowMsg_ = false;
#ifndef RBMINI
    harddiskDevice = HDDEVICE;
#endif
#ifdef RBMINI
    CheckForHD();
//    InstallMenus.push_back(eStart);
    InstallMenus.push_back(eLanguage);
    InstallMenus.push_back(eVideo);
    InstallMenus.push_back(eFormatHD);
    InstallMenus.push_back(eClient);
    InstallMenus.push_back(eChannellist);
    InstallMenus.push_back(eChannelscan);
//    InstallMenus.push_back(eHarddisk);
    InstallMenus.push_back(eFinished);
#else
    CheckForHD();
//    InstallMenus.push_back(eStart);
    InstallMenus.push_back(eLanguage);
    InstallMenus.push_back(eVideo);
    
#if 0 // TODO fix hardware bug. ICE freezes when harddisk is formatted ?
    // only on ICE based boards
    if (cPluginManager::GetPlugin("reelice") && HarddiskAvailable_) 
        InstallMenus.push_back(eFormatHD);
#endif
    
    InstallMenus.push_back(eConfigMultiroom);
    InstallMenus.push_back(eConnections);
    InstallMenus.push_back(eDiseqc);
    InstallMenus.push_back(eNetwork);
    InstallMenus.push_back(eUpdate);
    InstallMenus.push_back(eChannellist);
    InstallMenus.push_back(eChannelscan);
    InstallMenus.push_back(eFinished);
#endif
    ErrorState_ = (int *)malloc(sizeof(int) * InstallMenus.size());

    // init variables
    setupState = 0;
    for (unsigned int i = 0; i < InstallMenus.size(); ++i)
    {
        ErrorState_[i] = 0;
        if (InstallMenus.at(i) == startWithStep)
            setupState = i;
    }
    GetMenu();
}

cInstallMenu::~cInstallMenu()
{
    free(ErrorState_);
}

void cInstallMenu::Display(void)
{
}

void cInstallMenu::NextMenu(bool GoForward)
{
    bool skip = false;
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    if (GoForward)
        ++setupState;
    else
        --setupState;

    SetErrorState(0);           // Reset Error state

    switch (InstallMenus.at(setupState))
    {
    case eConnections:
        // If connection to Netceiver is ok, then skip
        if (CheckNetceiver())
            skip = true;
        break;
    case eDiseqc:
        if (!CheckNetceiver() || Setup.ReelboxMode == eModeClient )
            skip = true;
        break;
    case eChannellist:
        // Check if there is Sattuner, else skip
#ifdef RBMINI
        {
            cPlugin *p = cPluginManager::GetPlugin("avahi");
            if (p)
            {
                std::vector < ReelBox_t > ReelBoxes;
                p->Service("Avahi MultiRoom-List", &ReelBoxes);

                skipChannelscan_ = true;
                skip = false;

                if (ReelBoxes.size() > 0 && (Setup.ReelboxMode != eModeStandalone) && !chListShowMsg_)
                    skip = true;
            }
        }
#else
#ifdef DEVICE_ATTRIBUTES
        for (unsigned int i = 0; i < MAXTUNERS; i++)
        {
            cDevice *cdev = cDevice::GetDevice(i);
#if VDRVERSNUM < 10716
            if (!cdev || !cdev->HasInput())
#else
            if(!HAS_DVB_INPUT(cdev))
#endif
                continue;
            if (cdev)
            {
                uint64_t v = 0;
                cdev->GetAttribute("fe.type", &v);
                if ((fe_type_t) v != FE_DVBS2 && (fe_type_t) v != FE_QPSK)
                {
                    skipChannelscan_ = false;
                    skip = false;
                }
                break;          //TB: RC wants that only the first tuner is important
            }
            else
                break;
        }

#else
        if (!FirstSatTuner())
        {
            skipChannelscan_ = false;
            skip = true;
        }
#endif
#endif
        if (!CheckNetceiver())
            skip = true;
        
        if (Setup.ReelboxMode == eModeClient) { 
            /* TODO skip if channellists were successfully got 
             * from multiroom server */
            skipChannelscan_ = true;
            skip = true;
        }
        
        break;
    case eChannelscan:
        // Channelscan only if No Sat-Tuner or if Channellist is skipped or if box is in client mode
        if (Setup.ReelboxMode == eModeClient || skipChannelscan_ || !CheckNetceiver())
            skip = true;
        break;
    case eNetwork:
        if (!CheckNetworkCarrier())
            skip = true;
        break;
    case eFormatHD:
        // Skip if there is no builtinharddisk
        if (!HarddiskAvailable_)
            skip = true;
        else if (!InternalHD_) {
            skip = true;
            esyslog("not an internal hdd. Skip formatting '%s'", harddiskDevice.c_str());
        }
        else if (HarddiskAvailable_ && Setup.ReelboxModeTemp == 1 /*client*/) {
            skip = true; // skip in client mode, offer it only in standalone or server mode
            isyslog("In client Mode. Skip formatting '%s'", harddiskDevice.c_str());
        } else if (HDDHasMediaDevice(HDDEVICE)) {
            // if the default harddisk is already the mediadevice disk, skip format menu
            skip = true;
            printf("default hdd is has mediadevice\n");
        }
        
        if (skip) {
            std::string hd = HDDEVICE;
            printf("\033[7;91mSkip format hd menu, '%s'\033[0m\n", hd.c_str());
        }
        break;
        
    case eConfigMultiroom:
        break;
        
#if 0
    case eHarddisk:
        // Skip if no Harddisk but AVG
        if (!HarddiskAvailable_ && strlen(Setup.NetServerIP))
            skip = true;
        else if (HarddiskAvailable_ && !strlen(Setup.NetServerIP))
            skip = true;
        break;
#endif
    case eUpdate:
        // Skip if no Network
        if (CheckNetworkCarrier())
        {
            const char * network_dev = DefaultNetworkDevice();
            char eth_dev[8] = {0};
            int i = 0;
            
            /// toupper
            while (network_dev && *network_dev && i < 7) {
                eth_dev[i] = toupper(*network_dev);
                ++i;
                ++network_dev;
            } // while
            eth_dev[i] = 0;
            
            const char *value = sysconfig.GetVariable(eth_dev);
            // no setting for network found OR
            // network is disabled ==> Skip
            if (!value || strcasecmp(value, "yes") != 0)
                skip = true;
        }
        else                    // No cable plugged in Ethernet-Device
            skip = true;        // Skip Update
        break;
    default:
        break;
    }

    if (skip)
        NextMenu(GoForward);
}

eOSState cInstallMenu::GetMenu()
{
    eOSState state;

    switch (InstallMenus.at(setupState))
    {
    case eStart:
    case eLanguage:
        state = AddSubMenu(new cInstallPrimLang());
        break;
    case eVideo:
#ifdef RBMINI
        state = AddSubMenu(new cInstallVidoutMini());
#else
        if (cPluginManager::GetPlugin("reelice"))
            state = AddSubMenu(new cInstallVidoutICE());
        else
            state = AddSubMenu(new cInstallVidoutAva());
#endif
        break;
    case eConnections:
        state = AddSubMenu(new cInstallConnectionsAVG());
        break;
    case eDiseqc:
        state = AddSubMenu(new cInstallDiseqc());
        break;
    case eChannellist:
        state = AddSubMenu(new cInstallChannelList(chListShowMsg_));
        break;
    case eChannelscan:
        state = AddSubMenu(new cInstallChannelscan());
        break;
    case eNetwork:
        state = AddSubMenu(new cInstallNet());
        break;
    case eConfigMultiroom:
        state = AddSubMenu(new cInstallMultiroom(harddiskDevice, HarddiskAvailable_));
        break;
    case eClient:
        state = AddSubMenu(new cInstallNetclient(harddiskDevice, HarddiskAvailable_, &chListShowMsg_));
        break;
    case eFormatHD:
        state = AddSubMenu(new cInstallFormatHD(harddiskDevice));
        break;
#if 0
    case eHarddisk:
        state = AddSubMenu(new cInstallHarddisk(HarddiskAvailable_));
        break;
#endif
    case eUpdate:
        state = AddSubMenu(new cInstallUpdate());
        break;
    case eFinished:
#ifdef RBMINI
        {
            int FinishType = 0;
            if (!HarddiskAvailable_)
            {
                for (unsigned int i = 0; i < InstallMenus.size(); ++i)
                {
                    if ((InstallMenus.at(i) == eClient) && (ErrorState_[i]))
                        FinishType = 1;
                }
            }
            state = AddSubMenu(new cInstallFinished(FinishType));
        }
#else
        state = AddSubMenu(new cInstallFinished());
#endif
        break;
    default:
        // start from beginning
        setupState = eStart;
        state = AddSubMenu(new cInstallPrimLang());
        break;
    }

    char tempString[3];
    sprintf(tempString, "%d", InstallMenus.at(setupState));
    cPluginManager::GetPlugin("install")->SetupStore("stepdone", InstallMenus.at(setupState));
    cPluginManager::GetPlugin("install")->SetupParse("stepdone", tempString);
    Setup.Save();

    return state;
}

#if 0
static int cInstallMenu::GetErrorState(int StepNumber)
{
    for (unsigned int i = 0; i < InstallMenus.size(); ++i)
    {
        if (InstallMenus.at(i) == StepNumber)
            return = ErrorState_[StepNumber];
    }
}
#endif

eOSState cInstallMenu::ProcessKey(eKeys Key)
{
    if (Key == kBack)           // Ignore Exit-Button
        return osContinue;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HasSubMenu())
    {
        if (InstallMenus.at(setupState) == eChannellist)
        {
            if (Key == kOk)
                skipChannelscan_ = true;    // ChannelList selected => No Channelscan
            else if (Key == kYellow)
                skipChannelscan_ = false;   // ChannelList skipped => Do ChannelScan
        }

        switch (state)
        {
        case osUser1:
            // Go to next Page
            CloseSubMenu();
            if (setupState < InstallMenus.size() - 1)
                NextMenu(true);
            else
                return osEnd;   // End after Finish-Menu
            return GetMenu();
            break;
        case osUser4:
            // Go to next Page
            CloseSubMenu();
            if (setupState > 0)
                NextMenu(false);
            return GetMenu();
        case osUnknown:
            if (Key == kYellow)
            {
                // Go to next Page
                CloseSubMenu();
                if (setupState < InstallMenus.size() - 1)
                    NextMenu(true);
                else
                    return osEnd;   // End after Finish-Menu
                return GetMenu();
            }
            else if (Key == kGreen && eFinished != InstallMenus.at(setupState))
            {
                // Go to previous Page
                CloseSubMenu();
                if (setupState > 0)
                    NextMenu(false);
                return GetMenu();
            }
            break;
        default:
            // return state;
            break;
        }
    }

    return state == osUnknown ? osContinue : state; // keep the OSD open
}

void cInstallMenu::CheckForHD()
{
    HarddiskAvailable_ = false;
    InternalHD_ = false;
#ifdef RBMINI
    char *strBuff;
    char command[32];
    int controllerID = -1;
    char dev[16] = "/dev/sdf";

    while (access(dev, F_OK) == NULL)
    {
        sprintf(command, "lsscsi | grep %s", dev);
        FILE *process = popen(command, "r");
        if (process)
        {
            cReadLine readline;
            strBuff = readline.Read(process);
            if (strBuff)
            {
                controllerID = strBuff[1] - '0';
                HarddiskAvailable_ = true;
                harddiskDevice = dev;
            }
            pclose(process);
        }

        if (HarddiskAvailable_)
        {
            sprintf(command, "lsscsi -H %i | grep %s", controllerID, MOD_CONTROLLER);
            process = popen(command, "r");
            if (process)
            {
                cReadLine readline;
                strBuff = readline.Read(process);
                if (strBuff)
                {
                    InternalHD_ = true;
                    break;
                }
                else
                    HarddiskAvailable_ = false;
            }
        }
        dev[7]++;
    }

#else
    if (!access(harddiskDevice.c_str(), F_OK)) {
        HarddiskAvailable_ = true;
        /*assume hard disk is internal*/
        InternalHD_ = true; // TODO, check if this is true
    }
    else
        HarddiskAvailable_ = false;
#endif
}

//#######################################################################
// cInstallSubMenu
//#######################################################################
cInstallSubMenu::cInstallSubMenu(const char* title):cOsdMenu("", 24)
{
    SetTitle(title); // sets title as 'Install Wizard - <title>'
    
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load(SYSCONFIGFNAME);
    SetHelp(NULL, tr("Back"), tr("Skip"), NULL);
    Display();
    
    // Flush queued keys to avoid unwanted actions
    // FIXME: Move to common functions
    for(int i=0;i<100;i++) {
      eKeys k=Interface->GetKey(0);
      if (k==kNone)
        break;
    }
}

//#######################################################################
// cInstallFinished
//#######################################################################
cInstallFinished::cInstallFinished(int MessageNumber):cOsdMenu("Done", 33)
{
    loopMode = false;           /* fallback just to be sure */

    char *title;
    asprintf(&title, "%s - %s", tr(MAINMENUENTRY), tr("Done"));
    SetTitle(title);
    free(title);

    switch (MessageNumber)
    {
    case 0:
        Add(new cOsdItem(tr("Congratulations."), osUnknown, false), false, NULL);
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("All required settings for the initial operation have"), osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("been saved."), osUnknown, false), false, NULL);
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("Reel Multimedia wishes you a lot of fun with"), osUnknown, false), false, NULL);
#ifdef RBMINI
        Add(new cOsdItem(tr("your NetClient."), osUnknown, false), false, NULL);
#else
        Add(new cOsdItem(tr("your ReelBox."), osUnknown, false), false, NULL);
#endif
        break;
    case 1:
        Add(new cOsdItem(tr("The Install prodecure wasn't completed"), osUnknown, false));
        Add(new cOsdItem(tr("successfully."), osUnknown, false));
        break;
    default:
        break;
    }

    SetHelp(NULL, NULL, tr("End"), NULL);
    Display();
}

void cInstallFinished::Display(void)
{
    cOsdMenu::Display();
}

eOSState cInstallFinished::ProcessKey(eKeys Key)
{
    // ignore back key
    if (NORMALKEY(Key) == kGreen)
    {
        Key = kNone;
    }

    eOSState result = cOsdMenu::ProcessKey(Key);
    if (Key == kOk || Key == kYellow)
    {
        Save();
        result = osUser1;
    }
    return result;
}

bool cInstallFinished::Save()
{
    // set stepdone to inactive
    char tempString[4];
    snprintf(tempString, 4, "%d", eInactive);
    cPluginManager::GetPlugin("install")->SetupStore("stepdone", eInactive);
    cPluginManager::GetPlugin("install")->SetupParse("stepdone", tempString);
    Setup.Save();

    return true;
}

void cInstallFinished::Set()
{
}
