/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 parts rewritten by Tobias Bratfisch                *
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
 * wlansetting.cpp
 *
 ***************************************************************************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>

#include <stdlib.h>
#include <string>
#include <stdio.h>
#ifdef NEWUBUNTU
#include <unistd.h>
#endif
#include <vdr/i18n.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

#include "wlansetting.h"

const char* EncryptionTypeStrings[] = { trNOOP("Open"), "WEP", "WPA", "WPA2" };


//##### class cWlanDevice ###############################################################
cWlanDevice::cWlanDevice(const char *Interface)
{
    Interface_ = strdup(Interface);
    Mac_ = NULL;
    HardwareName_ = NULL;
    DetectMac();
    DetectHardwarename();
}

cWlanDevice::~cWlanDevice()
{
    if(Interface_)
        free(Interface_);
    if(Mac_)
        free(Mac_);
    if(HardwareName_)
        free(HardwareName_);
}

void cWlanDevice::DetectMac()
{
#define CRAPPY_CODE 1
#ifdef CRAPPY_CODE
    char *strBuff;
    char *index;
    char *keyword = strdup("HWaddr");
    FILE *process;

    asprintf(&strBuff, "LANG=C ifconfig %s | grep %s", Interface_, Interface_);
    process = popen(strBuff, "r");
    free(strBuff);
    if(process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        index = strstr(strBuff, keyword);
        if(index)
        {
            index = index + strlen(keyword) + 1;
            *(strchr(index, ' ')) = '\0'; // remove spaces after Mac-Address
            Mac_ = strdup(index);
        }
        pclose(process);
    }
    free(keyword);
#else
    struct ifreq ifr;
    struct ifreq *IFR;
    struct ifconf ifc;
    char buf[1024];
    int s, i;
    bool ok = false;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s==-1) {
        esyslog("ERROR: couldn't open socket");
        return;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    ioctl(s, SIOCGIFCONF, &ifc);

    IFR = ifc.ifc_req;
    for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++) {

        strcpy(ifr.ifr_name, IFR->ifr_name);
        if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
                    //TB: TODO: here only the name has to be compared to be sure to get the wlan-device
                    ok = 1;
                    break;
                }
            }
        }
    }

    close(s);
    if (ok) {
        Mac_ = strdup(ifr.ifr_hwaddr.sa_data);
    }
#endif
}

void cWlanDevice::DetectHardwarename()
{
    bool found = false;
    char *strBuff;
    char *index;
    char *keyword = strdup("product:");
    FILE *process;

    asprintf(&strBuff, "LANG=C sudo -u reel lshw -C network | grep '*-network\\|%s \\|serial: '" , keyword);
    process = popen(strBuff, "r"); // don't call as root since it causes screen blank while scanning hardware
    free(strBuff);
    if(process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        while(!found && strBuff)
        {
            if(strstr(strBuff, "*-network"))
            {
                if(HardwareName_)
                    free(HardwareName_);
                HardwareName_ = NULL;
            }
            else if(strstr(strBuff, keyword))
            {
                if(HardwareName_)
                    free(HardwareName_);
                index = strstr(strBuff, keyword) + strlen(keyword) + 1;
                HardwareName_ = strdup(index);
            }
            else if(strstr(strBuff, "serial: "))
            {
                if(Mac_ && strcasestr(strBuff, Mac_))
                    found = true;
            }
            strBuff = readline.Read(process);
        }
        pclose(process);
    }
    if(!found && HardwareName_)
    {
        free(HardwareName_);
        HardwareName_ = NULL;
    }
}

//##### class cWLANSetting ##############################################################
cWLANSetting::cWLANSetting()
{
        Device_ = NULL;
        HardwareName_ = NULL;
        SSID_ = (char*)malloc(32*sizeof(char));
        memset( SSID_, 0, 32 * sizeof(char));
        EncryptionType_ = eNoEncryption;

        EncryptionTypeStrings_ = (char**) malloc(sizeof(EncryptionTypeStrings));
        // translate EncryptionTypes
        for(unsigned int i=0; i<(sizeof(EncryptionTypeStrings)/sizeof(char*)); ++i)
            EncryptionTypeStrings_[i] = strdup(tr(EncryptionTypeStrings[i]));

        WEPKey_ = (char*)malloc(WEPKEY_LENGTH*sizeof(char));
        memset( WEPKey_, 0, WEPKEY_LENGTH * sizeof(char));

        WPAKey_ = (char*)malloc(WPAKEY_LENGTH*sizeof(char));
        memset( WPAKey_, 0, WPAKEY_LENGTH * sizeof(char));

        DHCP_ = false;
        IP_ = (char*)malloc(16*sizeof(char));
        strcpy(IP_, "0.0.0.0");
        Subnet_ = (char*)malloc(16*sizeof(char));
        strcpy(Subnet_, "0.0.0.0");
        Gateway_ = (char*)malloc(16*sizeof(char));
        strcpy(Gateway_, "0.0.0.0");
        DNS_ = (char*)malloc(16*sizeof(char));
        strcpy(DNS_, "0.0.0.0");
}

cWLANSetting::~cWLANSetting()
{
    if(Device_)
        free(Device_);
    if(HardwareName_)
        free(HardwareName_);
    for(unsigned int i=0; i<Accesspoints_.size(); ++i)
        free(Accesspoints_.at(i)->SSID);
    Accesspoints_.clear();
    if(SSID_)
        free(SSID_);
    for(unsigned int i=0; i<(sizeof(EncryptionTypeStrings)/sizeof(char*)); ++i)
        free(EncryptionTypeStrings_[i]);
    free(EncryptionTypeStrings_);
    if(WEPKey_)
        free(WEPKey_);
    if(WPAKey_)
        free(WPAKey_);
    if(IP_)
        free(IP_);
    if(Subnet_)
        free(Subnet_);
    if(Gateway_)
        free(Gateway_);
    if(DNS_)
        free(DNS_);
}

void cWLANSetting::SetWlanDevice(const char *Device)
{
    if(Device_)
        free(Device_);
    Device_ = strdup(Device);
}

void cWLANSetting::SetHardwareName(const char *HardwareName)
{
    if(HardwareName_)
        free(HardwareName_);
    HardwareName_=NULL;

    if(HardwareName)
    {
        HardwareName_ = strndup(HardwareName, HARDWARE_NAME_MAX_LEN);

        if(strlen(HardwareName)>HARDWARE_NAME_MAX_LEN)
        {
            HardwareName_[HARDWARE_NAME_MAX_LEN-4] = ' ';
            HardwareName_[HARDWARE_NAME_MAX_LEN-3] = '.';
            HardwareName_[HARDWARE_NAME_MAX_LEN-2] = '.';
            HardwareName_[HARDWARE_NAME_MAX_LEN-1] = '.';
            HardwareName_[HARDWARE_NAME_MAX_LEN] = '\0';
        }
    }
}

int cWLANSetting::ScanSSID()
{
    std::string command;

    // first clear list of SSIDs
    for(unsigned int i=0; i<Accesspoints_.size(); ++i)
    {
        free(Accesspoints_.at(i)->SSID);
        free(Accesspoints_.at(i));
    }
    Accesspoints_.clear();

    // power on device
    command = std::string("ifconfig ") + Device_ + " up";
    SystemExec(command.c_str());

    GetWlanList();

    return 0;
}

void cWLANSetting::SetSSID(const char* SSID)
{
    unsigned int i=0;
    bool found = false;
    std::string command;
    strncpy(SSID_, SSID, 31);
    command = std::string("iwconfig ") + Device_ + " essid \"" + SSID_ + "\"";
    SystemExec(command.c_str());

    while(!found && i < Accesspoints_.size())
    {
        if(strcmp(SSID_, Accesspoints_.at(i)->SSID) == 0)
        {
            found = true;
            EncryptionType_ = Accesspoints_.at(i)->EncryptionType;
        }
        ++i;
    }
}

void cWLANSetting::GetWlanList()
{
    FILE *process;
    char *command;
    char *strBuff, *strCutter;
    bool Encryption = false;
    bool WPA = false;
    bool WPA2 = false;

    // call this since first iwlist is not always up-to-date :/
    asprintf(&command, "iwlist %s scan 1>/dev/null 2>/dev/null", Device_);
    SystemExec(command);
    free(command);

    sleep(2); // Wait till wlan device is ready for following commands

    // Get List of available wlan networks
    asprintf(&command, "iwlist %s scan | grep 'ESSID\\|IE: \\|Encryption key:'", Device_);
    process = popen(command, "r");
    free(command);
    if(process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        Accesspoint *ap;
        bool firstAP = true;
        while((strBuff))
        {
            if(strstr(strBuff, "ESSID"))
            {
                // Next AP found
                if(firstAP)
                    firstAP = false;
                else
                {
                    if(WPA2)
                        ap->EncryptionType = eWPA2;
                    else if(WPA)
                        ap->EncryptionType = eWPA;
                    else if(Encryption)
                        ap->EncryptionType = eWEP;
                    else
                        ap->EncryptionType = eNoEncryption;
                    Accesspoints_.push_back(ap); // add collected datas to wlan-list

                    // reset temp-variables
                    Encryption = false;
                    WPA = false;
                    WPA2 = false;
                }
                ap = new Accesspoint;

                strCutter = strchr(strBuff, '\"')+1;
                *(strchr(strCutter, '\"')) = '\0';
                ap->SSID = strdup(strCutter);
            }
            else if (strstr(strBuff, "Encryption key:on"))
                Encryption=true;
            else if (strstr(strBuff, "WPA2"))
                WPA2 = true;
            else if (strstr(strBuff, "WPA"))
                WPA = true;

            strBuff = readline.Read(process);
        }

        if(!firstAP) // add last AP to wlan-list
        {
            if(WPA2)
                ap->EncryptionType = eWPA2;
            else if(WPA)
                ap->EncryptionType = eWPA;
            else if(Encryption)
                ap->EncryptionType = eWEP;
            else
                ap->EncryptionType = eNoEncryption;
            Accesspoints_.push_back(ap); // add collected datas to wlan-list
        }
    }
    pclose(process);

    sleep(2); // Wait till wlan device is ready for following commands
}

const char* cWLANSetting::GetEncryptionTypeString(int EncryptionType)
{
    return tr(EncryptionTypeStrings[EncryptionType]);
}

bool cWLANSetting::SetEncryptionKey()
{
    bool abort = false;
    bool successful = false;
    int counter = 0;
    std::string command;
    char *strBuff;
    FILE *process;

    switch(EncryptionType_)
    {
        case eNoEncryption:
            SystemExec("wpa_cli terminate");
            command = std::string("iwconfig ") + Device_ + " key off";
            SystemExec(command.c_str());
            return true;
            break;
        case eWEP:
            SystemExec("wpa_cli terminate");
            command = std::string("iwconfig ") + Device_ + " key " + WEPKey_;
            SystemExec(command.c_str());
            return true;
            break;
        case eWPA:
        case eWPA2:
            command = std::string("iwconfig ") + Device_ + " key off; wpa_cli terminate"; // clear WEP-Key and terminate wpa_supplicant
            SystemExec(command.c_str());

            command = std::string("echo \"ctrl_interface=/var/run/wpa_supplicant\" > ") + WPA_SUPPLICANT_TESTFILE;
            SystemExec(command.c_str());

            command = std::string("wpa_passphrase '") + SSID_ + "' '" + WPAKey_ + "' >> " + WPA_SUPPLICANT_TESTFILE;
            SystemExec(command.c_str());

            command = std::string(WPA_SUPPLICANT) + " -B -Dwext -i" + Device_ + " -c " + WPA_SUPPLICANT_TESTFILE;
            SystemExec(command.c_str());
            sleep(2);
            while(!successful && !abort && counter < WPA_TIMEOUT)
            {
                command = std::string(WPA_CLIENT) +  " status | grep wpa_state";
                process = popen(command.c_str(), "r");
                if(process)
                {
                    cReadLine readline;
                    strBuff = readline.Read(process);
                    //printf("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, strBuff);
                    if(strBuff && (strcmp(strBuff, "wpa_state=COMPLETED") == 0))
                        successful = true;
                    else
                        sleep(1);
                    pclose(process);
                    ++counter;
                }
                else
                    abort = true;
            }
            return successful;
            break;
    }

    return false; // failed
}

bool cWLANSetting::runDHCP()
{
    std::string command;

    // run DHCP-client
    command = std::string(DHCP_CLIENT) + " " + Device_;
    SystemExec(command.c_str());

    char *Trash = (char *) malloc(512 * sizeof(char));

    for(int i=0; i<DHCP_TIMEOUT; ++i)
    {
        FILE *process;
        char *strBuff;
        char *strBuff2;
        char *strCutter;

        command = std::string("ifconfig ") + Device_ + " | grep 'inet '";;
        process = popen(command.c_str(), "r");
        if(process)
        {
            cReadLine readline;
            strBuff = readline.Read(process);
            while(strBuff)
            {
                if(strstr(strBuff, "inet "))
                {
                    strCutter = strchr(strBuff, ':') + 1;
                    strBuff2 = (strchr(strCutter, ' ')) + 1;
                    *(strBuff2-1) = '\0';
                    strcpy(IP_, strCutter);
                    strCutter = strBuff2;
                    strCutter = strchr(strCutter, ':') + 1;
                    strCutter = strchr(strCutter, ':') + 1; // skip Broadcast
                    strcpy(Subnet_, strCutter);

                    DHCP_ = true;
                    pclose(process);

                    // get Info about Gateway
                    command = std::string("netstat -rn | grep UG | grep ") + Device_;
                    process = popen(command.c_str(), "r");
                    if(process)
                    {
                        cReadLine readline;
                        strBuff = readline.Read(process);
                        if(strBuff)
                            sscanf(strBuff, "%s %s", Trash, Gateway_);
                        else
                            strcpy(Gateway_, "0.0.0.0");
                    }
                    pclose(process);

                    // get Info about DNS (only 1)
                    process = popen("grep nameserver /etc/resolv.conf", "r");
                    if(process)
                    {
                        cReadLine readline;
                        strBuff = readline.Read(process);
                        if(strBuff)
                            sscanf(strBuff, "%s %s", Trash, DNS_);
                        else
                            strcpy(DNS_, "0.0.0.0");
                    }
                    pclose(process);

                    free(Trash);
                    return true; // Got IP-settings
                }
                strBuff = readline.Read(process);
            }
        }
        pclose(process);
        sleep(1);
    }

    DHCP_ = false;
    free(Trash);
    return false; // DHCP didn't work
}

void cWLANSetting::terminateWpaSupplicant()
{
    SystemExec("wpa_cli terminate");

    remove(WPA_SUPPLICANT_TESTFILE);
}
