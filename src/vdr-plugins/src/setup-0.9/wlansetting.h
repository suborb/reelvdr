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
 * wlansetting.h
 *
 ***************************************************************************/


#ifndef WLANSETTING_H
#define WLANSETTING_H

#include <vector>

#define WEPKEY_LENGTH 27
#define WPAKEY_LENGTH 64
#define NUMBERS_OF_ENCRYTPTION_TYPE 4
#define HARDWARE_NAME_MAX_LEN 32
#define WPA_SUPPLICANT "wpa_supplicant"
#define WPA_CLIENT "wpa_cli"
#define WPA_SUPPLICANT_TESTFILE "/tmp/wpa_supplicant-test.conf"
#define WPA_TIMEOUT 15
#define DHCP_CLIENT "dhclient3"
#define DHCP_TIMEOUT 7

enum eEncryptionType { eNoEncryption, eWEP, eWPA, eWPA2 };

struct Accesspoint
{
    char *SSID;
    eEncryptionType EncryptionType;
};

class cWlanDevice
{
    private:
        char *Interface_;
        char *Mac_;
        char *HardwareName_;
        void DetectHardwarename();
        void DetectMac();

    public:
        cWlanDevice(const char *Interface);
        ~cWlanDevice();
        char* GetInterface() { return Interface_; };
        char* GetHardwareName() { return HardwareName_; };
};

class cWLANSetting
{
    private:
        char *Device_;
        char *HardwareName_;
        char *SSID_;
        char *WEPKey_;
        char *WPAKey_;
        char *IP_;
        char *Subnet_;
        char *Gateway_;
        char *DNS_;

    public:
        cWLANSetting();
        ~cWLANSetting();

        std::vector<Accesspoint*> Accesspoints_;
        int EncryptionType_;
        char** EncryptionTypeStrings_;
        int DHCP_;

        void SetWlanDevice(const char* Device);
        const char* GetWlanDevice() { return Device_; };
        void SetHardwareName(const char* HardwareName_);
        const char* GetHardwareName() { return HardwareName_; };
        int ScanSSID();
        void SetSSID(const char* SSID);
        char* GetSSID() { return SSID_; };
        void GetWlanList();
        int GetEncryptionType() { return EncryptionType_; };
        const char* GetEncryptionTypeString(int EncryptionType);
        bool SetEncryptionKey();
        char* GetWEPKey() { return WEPKey_; };
        char* GetWPAKey() { return WPAKey_; };
        bool runDHCP();
        char* GetIP() { return IP_; };
        char* GetSubnet() { return Subnet_; };
        char* GetGateway() { return Gateway_; };
        char* GetDNS() { return DNS_; };
        void terminateWpaSupplicant();
};
#endif

