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
 * setupwlanmenu.cpp
 *
 ***************************************************************************/


#include "setupwlanmenu.h"
#include "setupmenu.h"
#include "menueditrefreshitems.h"
#include <string>
using namespace std;

#define MAX_LETTER_PER_LINE 48

const char* allowed_char_wep = "0123456789ABCDEF";
//const char* allowed_char_wpa = tr(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!0123456789+-_@§$%&/()=?.,;:");
//const char* allowed_char_ssid = tr(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!-0123456789+-_@§$%&/()=?.,;:");

//###########################################################################
//  cWlanDeviceItem
//###########################################################################
cWlanDeviceItem::cWlanDeviceItem(char *Interface, bool Marked, char* HardwareName)
{
    Interface_ = strdup(Interface);
    Marked_ = Marked;
    HardwareName_ = NULL;
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

    char *strBuff;
    //Marked_ = true;
    DDD("Marked %d mark %c HardwareName_ %s", Marked_, '\x87', HardwareName_);
    if(HardwareName_)
        asprintf(&strBuff, "%s%s (%s)", Marked_?"\x87 ":"  ", Interface_, HardwareName_);
    else
        asprintf(&strBuff, "%s %s", Marked_ ? "\x87" : "  ", Interface_);
    SetText(strBuff);
    free(strBuff);
}

cWlanDeviceItem::~cWlanDeviceItem()
{
    if(Interface_)
        free(Interface_);
    if(HardwareName_)
        free(HardwareName_);
}

//###########################################################################
//  cSetupWLANMenu
//###########################################################################

cSetupWLANMenu::cSetupWLANMenu(const char *title, bool installWizard) : cOsdMenu(title), InstallWiz(installWizard)
{
    FILE *process;
    char *strBuff;

    Skins.Message(mtStatus, tr("Load settings, please wait..."));

    WlanSetting = new cWLANSetting();
//    finished_text = false;
    changed = false;
    counter = 0; // UGLY Hack against UGLY twice-pressed-kOk-bug

    // List wlan-devices
    process = popen("iwconfig 2>/dev/null | grep -E 'IEEE|unassociated'", "r");
    if (process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        while(strBuff)
        {
            sscanf(strBuff, "%s", strBuff);
            Devices_.push_back(new cWlanDevice(strBuff));
            strBuff = readline.Read(process);
        }
    }
    pclose(process);

    // Load Settings
    LoadSettings();

    if(wlanEnable && Devices_.size())
        State_ = eFinish;
    else
        State_ = eDevice;

    Set();
}

cSetupWLANMenu::~cSetupWLANMenu()
{
    for(unsigned int i=0; i<Devices_.size(); ++i)
        free(Devices_.at(i));
    Devices_.clear();
}

void cSetupWLANMenu::Set()
{
    Clear();
#if APIVERSNUM < 10700
    SetCols(12, 3);
#else
    SetCols(18, 3);
#endif

    char *strBuff;

    switch(State_)
    {
        case eDevice:
            SetHelp(NULL);
            // check if wireless card exists
            if (Devices_.size() == 1) {
                // if only one device exists, then go to wlan settings directly
                // do not offer choice of devices
                wlanEnable = true;

                WlanSetting->SetWlanDevice(Devices_.at(0)->GetInterface());
                WlanSetting->SetHardwareName(Devices_.at(0)->GetHardwareName());
                isyslog("One wlan device found. interface:'%s' Hw Name:'%s', so autoscan for networks on it",
                        Devices_.at(0)->GetInterface(),
                        Devices_.at(0)->GetHardwareName());

                Skins.Message(mtStatus, tr("Scanning networks, please wait..."));
                WlanSetting->ScanSSID();
                State_ = eSSID;
                Set();
                cOsdItem *osditem = NULL;
                for(int i=0; i<Count(); ++i)
                {
                    osditem = Get(i);
                    if(osditem && osditem->Text() && strchr(osditem->Text(), 135))
                        SetCurrent(osditem);
                }
                Display();
            } else
            if (Devices_.size()>0)
            {
                //Add(new cMenuEditBoolRefreshItem(tr("Enable WLAN"), &wlanEnable ));
                //Add(new cOsdItem("", osUnknown, false));

                // show device list if WLAN is enabled
		wlanEnable = true;
                if ( wlanEnable )
                {
                    Add(new cOsdItem(tr("Following hardware was found."), osUnknown, false));
                    Add(new cOsdItem(tr("Please choose a WLAN device:"), osUnknown, false));
                    Add(new cOsdItem("", osUnknown, false));
                    for(unsigned int i=0; i<Devices_.size(); ++i)
                    {
			//DDD("GetWlanDev: %s GetInterface: %s", WlanSetting->GetWlanDevice(), Devices_.at(i)->GetInterface());
                        if (WlanSetting->GetWlanDevice() && !strcmp(WlanSetting->GetWlanDevice(), Devices_.at(i)->GetInterface()))
                            Add(new cWlanDeviceItem(Devices_.at(i)->GetInterface(), true, Devices_.at(i)->GetHardwareName()));
                        else
                            Add(new cWlanDeviceItem(Devices_.at(i)->GetInterface(), false, Devices_.at(i)->GetHardwareName()));
                    }
                }
            }
            else
            {
                Add (new cOsdItem(tr("No wlan device found!"), osUnknown, false));
                wlanEnable = false; // Disable WLAN
            }
            break;
        case eSSID:
	    SetHelp(InstallWiz?tr("Rescan WLAN"):NULL, InstallWiz?tr("Back"):NULL, InstallWiz?tr("Skip"):tr("Rescan WLAN"), tr("Expert"));

            if(WlanSetting->GetHardwareName())
            {
                asprintf(&strBuff, "%s:\t%s", tr("Device"), WlanSetting->GetHardwareName());
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);
            }
            asprintf(&strBuff, "%s:\t%s", tr("Interface"), WlanSetting->GetWlanDevice());
            Add(new cOsdItem(strBuff, osUnknown, false));
            free(strBuff);
            Add(new cOsdItem("", osUnknown, false));

	if (WlanSetting->Accesspoints_.size() == 0 )
	    AddFloatingText(tr("Sorry, no WLAN networks found. Check the antenna, check WLAN settings and try to search again."), MAX_LETTER_PER_LINE );
	else
	{
            Add(new cOsdItem(tr("Following WLAN networks are found."), osUnknown, false));
            Add(new cOsdItem(tr("Please choose a WLAN network:"), osUnknown, false));
            Add(new cOsdItem("", osUnknown, false));
            for(unsigned int i=0; i<WlanSetting->Accesspoints_.size(); ++i)
            {
                if(strlen(WlanSetting->Accesspoints_.at(i)->SSID) > 0) // filter out wlan with empty SSID
                {
                    char icon;
                    char *strMark = NULL;

                    if(WlanSetting->GetSSID() && !strcmp(WlanSetting->Accesspoints_.at(i)->SSID, WlanSetting->GetSSID()))
                        asprintf(&strMark, "%c ", 135);

                    if(WlanSetting->Accesspoints_.at(i)->EncryptionType==eNoEncryption)
                        icon = ' '; // Nothing
                    else
                        icon = 80; // Key
                    asprintf(&strBuff, "%s%s\t%c\t%s", strMark?strMark:"", WlanSetting->Accesspoints_.at(i)->SSID, icon, WlanSetting->GetEncryptionTypeString(WlanSetting->Accesspoints_.at(i)->EncryptionType));
                    Add(new cOsdItem(strBuff));
                    if(strMark)
                        free(strMark);
                    free(strBuff);
                }
            }
	}
            break;
        case eEncryption:
            SetHelp(NULL);

            if(WlanSetting->GetHardwareName())
            {
                asprintf(&strBuff, "%s:\t%s", tr("Name"), WlanSetting->GetHardwareName());
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);
            }
            asprintf(&strBuff, "%s:\t%s", tr("Interface"), WlanSetting->GetWlanDevice());
            Add(new cOsdItem(strBuff, osUnknown, false));
            free(strBuff);

            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("Network:"), osUnknown, false));
            asprintf(&strBuff, "%s:\t%s", tr("(E)SSID"), WlanSetting->GetSSID());
            Add(new cOsdItem(strBuff, osUnknown, false));
            free(strBuff);

            Add(new cOsdItem("", osUnknown, false));
            Add(new cOsdItem(tr("Security:"), osUnknown, false));
            Add(new cMenuEditStraRefreshItem(tr("Type"), &(WlanSetting->EncryptionType_), NUMBERS_OF_ENCRYTPTION_TYPE, WlanSetting->EncryptionTypeStrings_));
            switch(WlanSetting->EncryptionType_)
            {
                case eNoEncryption:
                    break;
                case eWEP:
                    Add(new cMenuEditStrItem(tr("WEP-Key"), WlanSetting->GetWEPKey(), WEPKEY_LENGTH, allowed_char_wep));
                    break;
                case eWPA:
                    Add(new cMenuEditStrItem(tr("WPA-Key"), WlanSetting->GetWPAKey(), WPAKEY_LENGTH, trVDR(FileNameChars)));
                    break;
                case eWPA2:
                    Add(new cMenuEditStrItem(tr("WPA2-Key"), WlanSetting->GetWPAKey(), WPAKEY_LENGTH, trVDR(FileNameChars)));
                    break;
            }
            break;
        case eExpert:
            {
                if (InstallWiz)
                    SetHelp(NULL, tr("Back"), tr("Skip"), NULL);
                else
                    SetHelp(NULL);

                if(WlanSetting->GetHardwareName())
                {
                    asprintf(&strBuff, "%s:\t%s", tr("Device"), WlanSetting->GetHardwareName());
                    Add(new cOsdItem(strBuff, osUnknown, false));
                    free(strBuff);
                }
                asprintf(&strBuff, "%s:\t%s", tr("Interface"), WlanSetting->GetWlanDevice());
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);

                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(tr("Network:"), osUnknown, false));
                Add(new cMenuEditStrItem(tr("(E)SSID"), WlanSetting->GetSSID(), 31, trVDR(FileNameChars)));

                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(tr("Security:"), osUnknown, false));
                Add(new cMenuEditStraRefreshItem(tr("Type"), &(WlanSetting->EncryptionType_), NUMBERS_OF_ENCRYTPTION_TYPE, WlanSetting->EncryptionTypeStrings_));
                switch(WlanSetting->EncryptionType_)
                {
                    case eNoEncryption:
                        break;
                    case eWEP:
                        Add(new cMenuEditStrItem(tr("WEP-Key"), WlanSetting->GetWEPKey(), WEPKEY_LENGTH, allowed_char_wep));
                        break;
                    case eWPA:
                        Add(new cMenuEditStrItem(tr("WPA-Key"), WlanSetting->GetWPAKey(), WPAKEY_LENGTH, trVDR(FileNameChars)));
                        break;
                    case eWPA2:
                        Add(new cMenuEditStrItem(tr("WPA2-Key"), WlanSetting->GetWPAKey(), WPAKEY_LENGTH, trVDR(FileNameChars)));
                        break;
                }
            }
            break;
        case eFinish:
            {
                SetHelp(NULL, NULL, NULL, tr("Functions"));
#if 0
                if(finished_text)
                {
                    Add(new cOsdItem(tr("WLAN settings successfully stored"), osUnknown, false));
                    Add(new cOsdItem("", osUnknown, false));
                }
#endif
                if(WlanSetting->GetHardwareName())
                {
                    asprintf(&strBuff, "%s:\t%s", tr("Device"), WlanSetting->GetHardwareName());
                    Add(new cOsdItem(strBuff, osUnknown, false));
                    free(strBuff);
                }
                asprintf(&strBuff, "%s:\t%s", tr("Interface"), WlanSetting->GetWlanDevice());
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);
                asprintf(&strBuff, "%s:\t%s", tr("(E)SSID"), WlanSetting->GetSSID());
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);
                asprintf(&strBuff, "%s:\t%s", tr("Security"), WlanSetting->GetEncryptionTypeString(WlanSetting->GetEncryptionType()));
                Add(new cOsdItem(strBuff, osUnknown, false));
                free(strBuff);
                Add(new cMenuEditBoolRefreshItem("DHCP", &(WlanSetting->DHCP_)));
                if(WlanSetting->DHCP_)
                {
                    asprintf(&strBuff, "%s:\t%s", tr("IP Address"), WlanSetting->GetIP());
                    Add(new cOsdItem(strBuff, osUnknown, false));
                    free(strBuff);
                    asprintf(&strBuff, "%s:\t%s", tr("Subnet"), WlanSetting->GetSubnet());
                    Add(new cOsdItem(strBuff, osUnknown, false));
                    free(strBuff);
//                    asprintf(&strBuff, "%s:\t%s", tr("Gateway"), WlanSetting->GetGateway());
//                    Add(new cOsdItem(strBuff, osUnknown, false));
//                    free(strBuff);
//                    asprintf(&strBuff, "%s:\t%s", tr("DNS"), WlanSetting->GetDNS());
//                    Add(new cOsdItem(strBuff, osUnknown, false));
//                    free(strBuff);
                }
                else
                {
                    Add(new cMenuEditIpNumItem(tr("IP Address"), WlanSetting->GetIP()));
                    Add(new cMenuEditIpNumItem(tr("Subnet"), WlanSetting->GetSubnet()));
//                    Add(new cMenuEditIpNumItem(tr("Gateway"), WlanSetting->GetGateway()));
//                    Add(new cMenuEditIpNumItem(tr("DNS"), WlanSetting->GetDNS()));
                }
                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem("", osUnknown, false));
		AddFloatingText(tr("Please press OK to store the settings now.\n\n"
		                   "You can also manually set an IP address here."), MAX_LETTER_PER_LINE);
            }
            break;
    }
}

void cSetupWLANMenu::runDHCP()
{
    Skins.Message(mtStatus, tr("Running DHCP, please wait..."));
    if(!WlanSetting->runDHCP())
        Skins.Message(mtWarning, tr("Retrieving IP from DHCP failed!"));
}

eOSState cSetupWLANMenu::ProcessKey(eKeys Key)
{
    char strBuff[32];
    cOsdItem *osditem;
    eOSState state = cOsdMenu::ProcessKey(Key);

    // UGLU Hack against UGLY twice-pressed-kOk-bug
    if(counter == 0)
    {
        ++counter;
        if(state==osUnknown)
            return osContinue; //ignore it!
    }

    if(state == os_User) // Refresh Page
    {
        int current = Current();
        Set();
        if(State_ == eDevice) // Jump to selected/first item
        {
            SetCurrent(Get(5));
            cOsdItem *osditem;
            for(int i=0; i<Count(); ++i)
            {
                osditem = Get(i);
                if(osditem->Text() && strchr(osditem->Text(), 135))
                    SetCurrent(osditem);
            }
        }
        else
            SetCurrent(Get(current));
        Display();
        return osContinue;
    }
    else if(state == osUser1) // Retrieve DHCP
    {
        CloseSubMenu();

        runDHCP();
        Set();
        Display();
        return osContinue;
    }
    else if(state == osUser2) // Reset settings
    {
        CloseSubMenu();

        State_ = eDevice;
        free(WlanSetting);
        WlanSetting = new cWLANSetting();
        wlanEnable = true;
        ResetSettings();
        Set();
        Display();
        return osContinue;
    }
    else if(state == osUser3) // Turn off Wireless
    {
        // Turn off Wireless
        wlanEnable = false;
        SaveSettings();
        CloseSubMenu();
        return osBack;
    }
    else if((state == osUnknown) || (state == osBack))
    {
        if(Key == kOk)
        {
            //printf("\033[0;93m %s(%i): Got kOk %d %d\033[0m\n", __FILE__, __LINE__, Key, state);
            if ( !wlanEnable )
            {
                Skins.Message(mtStatus, tr("Storing settings, please wait..."));
                cSysConfig_vdr::GetInstance().SetVariable("WLAN", "no");
                cSysConfig_vdr::GetInstance().Save();
                SystemExec("setup-net restart");
                // WLAN == yes is stored in SaveSettings

                return osBack;
            }

            switch(State_)
            {
                case eDevice:
                    {
                        if(Current() > 0)
                        {
                            cWlanDeviceItem *wlandeviceitem = dynamic_cast<cWlanDeviceItem*>(Get(Current()));

                            if(wlandeviceitem)
                            {
                                WlanSetting->SetWlanDevice(wlandeviceitem->GetInterface());
                                WlanSetting->SetHardwareName(wlandeviceitem->GetHardwareName());

                                Skins.Message(mtStatus, tr("Scanning networks, please wait..."));
                                WlanSetting->ScanSSID();
                                State_ = eSSID;
                                Set();
                                cOsdItem *osditem;
                                for(int i=0; i<Count(); ++i)
                                {
                                    osditem = Get(i);
                                    if(osditem->Text() && strchr(osditem->Text(), 135))
                                        SetCurrent(osditem);
                                }
                                Display();
                                return osContinue;
                            }
                            else
                                return osUnknown;
                        }
                        else
                            return osUnknown;
                    }
                    break;

                case eSSID:
                    {
                        char *ssid;

                        cOsdItem *ssiditem = Get(Current());
                        strncpy(strBuff, ssiditem->Text(), 32);
                        strBuff[31]=0;

                        // cut off mark-icon if there is any
                        if(strBuff[0] == (char)135 && (strBuff[1] == ' '))
                            ssid = &(strBuff[2]);
                        else
                            ssid = strBuff;

                        // select the first tab field
                        char *tab_delimiter = strchr(ssid, '\t');
                        if ( tab_delimiter ) *tab_delimiter = '\0';

                        WlanSetting->SetSSID( ssid );
                        if(WlanSetting->GetEncryptionType() == eNoEncryption)
                        {
                            runDHCP();
//                            finished_text = true;
                            State_ = eFinish;
                            changed = true;
                        }
                        else
                        {
                            State_ = eEncryption;
                            Set();
                            SetCurrent(Get(Count()-1));
                            Display();
                            return osContinue;
                        }
                    }
                    break;

                case eEncryption:
                    {
                        bool WEP_too_short = false;
                        bool WPA_too_short = false;

                        switch(WlanSetting->GetEncryptionType())
                        {
                            case eWEP:
                                if(strlen(WlanSetting->GetWEPKey()) < 10)
                                    WEP_too_short = true;
                                break;
                            case eWPA:
                            case eWPA2:
                                if(strlen(WlanSetting->GetWPAKey()) < 8)
                                    WPA_too_short = true;
                                break;
                            default:
                                break;
                        }

                        if(WEP_too_short)
                            Skins.Message(mtWarning, tr("Keylength must be at least 10 chars!"));
                        else if(WPA_too_short)
                            Skins.Message(mtWarning, tr("Keylength must be at least 8 chars!"));
                        else
                        {
                            Skins.Message(mtStatus, tr("Please wait..."));
                            if(WlanSetting->SetEncryptionKey())
                            {
                                runDHCP();
//                                finished_text = true;
                                State_ = eFinish;
                                changed = true;
                                Set();
                                Display();
                            }
                            else
                                Skins.Message(mtWarning, tr("Authentication failed"), 3);
                        }
                        return osContinue;
                    }
                    break;

                case eExpert:
                    {
                        bool invalid_ssid = false;
                        bool WEP_too_short = false;
                        bool WPA_too_short = false;

                        if(strlen(WlanSetting->GetSSID())<1)
                            invalid_ssid = true;

                        switch(WlanSetting->GetEncryptionType())
                        {
                            case eWEP:
                                if(strlen(WlanSetting->GetWEPKey()) < 10)
                                    WEP_too_short = true;
                                break;
                            case eWPA:
                            case eWPA2:
                                if(strlen(WlanSetting->GetWPAKey()) < 8)
                                    WPA_too_short = true;
                                break;
                            default:
                                break;
                        }

                        if(invalid_ssid)
                            Skins.Message(mtWarning, tr("Please enter SSID!"));
                        else if(WEP_too_short)
                            Skins.Message(mtWarning, tr("Keylength must be at least 10 chars!"));
                        else if(WPA_too_short)
                            Skins.Message(mtWarning, tr("Keylength must be at least 8 chars!"));
                        else
                        {
                            Skins.Message(mtStatus, tr("Please wait..."));
                            if(WlanSetting->SetEncryptionKey())
                            {
                                runDHCP();
//                                finished_text = true;
                                State_ = eFinish;
                                changed = true;
                                Set();
                                Display();
                            }
                            else
                                Skins.Message(mtWarning, tr("Authentication failed"), 3);
                        }
                        return osContinue;
                    }
                    break;

                case eFinish:
                    if(Key == kOk)
                    {
                        WlanSetting->terminateWpaSupplicant();
                        SaveSettings();
                        Skins.Message(mtStatus, tr("Settings stored"), 3);
                        return osBack;
                    }
                    break;
            }
            Set();
            Display();
            return osContinue;
        }
        else if (Key == kBack)
        {
            switch(State_)
            {
                case eDevice:
                    if(changed && Interface->Confirm(tr("Store settings?")))
                        SaveSettings();
                    return osBack;
                    break;
                case eSSID:
                if (Devices_.size() == 1) { // only one wlan device available
                    return osBack;
                }
                    State_ = eDevice;
                    Set();
                    for(int i=0; i<Count(); ++i)
                    {
                        osditem = Get(i);
                        if(osditem->Text() && strchr(osditem->Text(), 135))
                            SetCurrent(osditem);
                    }
                    Display();
                    return osContinue;
                    break;
                case eEncryption:
                    State_ = eSSID;
                    Set();
                    for(int i=0; i<Count(); ++i)
                    {
                        osditem = Get(i);
                        if(osditem->Text() && strchr(osditem->Text(), 135))
                            SetCurrent(osditem);
                    }
                    Display();
                    return osContinue;
                    break;
                case eExpert:
                    State_ = eSSID;
                    Set();
                    cOsdItem *osditem;
                    for(int i=0; i<Count(); ++i)
                    {
                        osditem = Get(i);
                        if(osditem->Text() && strchr(osditem->Text(), 135))
                            SetCurrent(osditem);
                    }
                    Display();
                    return osContinue;
                case eFinish:
                    if (changed && Interface->Confirm(tr("Store settings?")))
                        SaveSettings();
                    return osBack;
                    break;
            }
            Set();
            Display();
            return osContinue;
        }
        else if(Key == kBlue)
        {
            switch(State_)
            {
                case eSSID:
                    State_ = eExpert;
                    Set();
                    Display();
                    return osContinue;
                    break;
                case eFinish:
                    AddSubMenu(new cSetupWLANMenuFunctions());
                    return osContinue;
                    break;
                default:
                    return osUnknown;
            }
        }
        else if(InstallWiz&&(Key == kRed) || !InstallWiz&&(Key==kYellow))
        {
            switch(State_)
            {
                case eSSID:
                    Skins.Message(mtStatus, tr("Scanning networks, please wait..."));
                    WlanSetting->ScanSSID();
                    Set();
                    Display();
                    break;
                default:
                    break;
            }
        } else if (InstallWiz && Key == kGreen) 
        {
            if (State_ == eExpert) { // go back to normal mode from expert
                State_ = eSSID;
                Set();
                cOsdItem *osditem;
                for(int i=0; i<Count(); ++i)
                {
                    osditem = Get(i);
                    if(osditem->Text() && strchr(osditem->Text(), 135))
                        SetCurrent(osditem);
                }
                Display();
                return osContinue;
            }
            return osUnknown;
        }
        else if (InstallWiz && Key == kYellow) return osUnknown;
    }
    return state;
}

void cSetupWLANMenu::ResetSettings()
{
    std::string command;

    changed = true;

    // terminate wpa_supplicant
    command = std::string(WPA_CLIENT) +  " terminate";
    DDD("Executing %s", command.c_str());
    SystemExec(command.c_str());

    // ifconfig down every wireless devices
    for(unsigned int i=0; i<Devices_.size(); ++i)
    {
        command = std::string("ifconfig ") + Devices_.at(i)->GetInterface() + " down";
        DDD("Executing %s", command.c_str());
        SystemExec(command.c_str());
    }
}

void cSetupWLANMenu::LoadSettings()
{
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    if(sysconfig.GetVariable("WLAN") && !strcmp(sysconfig.GetVariable("WLAN"), "yes"))
        wlanEnable = true;
    else
        wlanEnable = false;

    if(wlanEnable)
    {
        if(sysconfig.GetVariable("WLAN_LAST_DEVICE"))
            WlanSetting->SetWlanDevice(sysconfig.GetVariable("WLAN_LAST_DEVICE"));

        // Get Hardware name
        cWlanDevice *wlandevice = new cWlanDevice(WlanSetting->GetWlanDevice());
        if(wlandevice->GetHardwareName())
            WlanSetting->SetHardwareName(wlandevice->GetHardwareName());
        free(wlandevice);

        if(sysconfig.GetVariable("WLAN_DHCP") && !strcmp(sysconfig.GetVariable("WLAN_DHCP"), "yes"))
            WlanSetting->DHCP_ = true;
        else
            WlanSetting->DHCP_ = false;
        WlanSetting->SetSSID(sysconfig.GetVariable("WLAN_SSID"));

        if(WlanSetting->DHCP_ && WlanSetting->GetWlanDevice())
        {
            cNetInfo netinfo;
            int i=0;
            bool found=false;
            while(!found && i < netinfo.IfNum())
            {
                if(netinfo[i]->IfName() && !strcmp(WlanSetting->GetWlanDevice(), netinfo[i]->IfName()))
                {
                    found=true;
                    strcpy(WlanSetting->GetIP(), netinfo[i]->IpAddress());
                    strcpy(WlanSetting->GetSubnet(), netinfo[i]->NetMask());
                }
                ++i;
            }
        }
        else
        {
            if(sysconfig.GetVariable("WLAN_IP"))
                strcpy(WlanSetting->GetIP(), sysconfig.GetVariable("WLAN_IP"));
            if(sysconfig.GetVariable("WLAN_NETMASK"))
                strcpy(WlanSetting->GetSubnet(), sysconfig.GetVariable("WLAN_NETMASK"));
        }
        if(sysconfig.GetVariable("NAMESERVER"))
            strcpy(WlanSetting->GetDNS(), sysconfig.GetVariable("NAMESERVER"));
        if(sysconfig.GetVariable("GATEWAY"))
            strcpy(WlanSetting->GetGateway(), sysconfig.GetVariable("GATEWAY"));
        if(sysconfig.GetVariable("WLAN_MODUS"))
        {
            if(!strcmp(sysconfig.GetVariable("WLAN_MODUS"), "Open"))
                WlanSetting->EncryptionType_ = eNoEncryption;
            else if(!strcmp(sysconfig.GetVariable("WLAN_MODUS"), "WEP"))
                WlanSetting->EncryptionType_ = eWEP;
            else if(!strcmp(sysconfig.GetVariable("WLAN_MODUS"), "WPA"))
                WlanSetting->EncryptionType_ = eWPA;
            else if(!strcmp(sysconfig.GetVariable("WLAN_MODUS"), "WPA2"))
                WlanSetting->EncryptionType_ = eWPA2;
        }
        if(sysconfig.GetVariable("WLAN_WEP_KEY_1"))
            strcpy(WlanSetting->GetWEPKey(), sysconfig.GetVariable("WLAN_WEP_KEY_1"));
        if(sysconfig.GetVariable("PSK_PLAIN"))
            strcpy(WlanSetting->GetWPAKey(), sysconfig.GetVariable("PSK_PLAIN"));
    }
}

void cSetupWLANMenu::SaveSettings()
{
    Skins.Message(mtStatus, tr("Storing settings, please wait..."));
//    char *Command;
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    sysconfig.SetVariable("WLAN", wlanEnable?"yes":"no");
    sysconfig.SetVariable("WLAN_LAST_DEVICE", WlanSetting->GetWlanDevice());
    sysconfig.SetVariable("WLAN_SSID", WlanSetting->GetSSID());

    if(WlanSetting->DHCP_)
    {
        sysconfig.SetVariable("WLAN_DHCP", "yes");
    }
    else
    {
        sysconfig.SetVariable("WLAN_LAST_DEVICE", WlanSetting->GetWlanDevice());
        sysconfig.SetVariable("WLAN_DHCP", "no");
        sysconfig.SetVariable("WLAN_IP", WlanSetting->GetIP());
        sysconfig.SetVariable("WLAN_NETMASK", WlanSetting->GetSubnet());
        // sysconfig.SetVariable("NAMESERVER", WlanSetting->GetDNS());
        // SystemExec("echo search $DOMAIN > /etc/resolv.conf");
        // asprintf(&Command, "echo nameserver %s >> /etc/resolv.conf", WlanSetting->GetDNS());
        // SystemExec(Command);
        // free(Command);
        // sysconfig.SetVariable("GATEWAY", WlanSetting->GetGateway());
    }

    switch(WlanSetting->GetEncryptionType())
    {
        case eNoEncryption:
            sysconfig.SetVariable("WLAN_MODUS", "Open");
            break;
        case eWEP:
            sysconfig.SetVariable("WLAN_MODUS", "WEP");
            sysconfig.SetVariable("WLAN_WEP_KEY_1", WlanSetting->GetWEPKey());
            sysconfig.SetVariable("WLAN_WEP_KEY", "1"); // for now we only use 1 WEP-Key
            break;
        case eWPA:
        case eWPA2:
            sysconfig.SetVariable("WLAN_MODUS", "WPA");
            sysconfig.SetVariable("WPA_SSID", WlanSetting->GetSSID());
            sysconfig.SetVariable("PSK_PLAIN", WlanSetting->GetWPAKey());
            break;
    }

    // fixed values
    sysconfig.SetVariable("WLAN_MODE", "managed"); // We assume the customer uses only with wlan-router/Accesspoint
    // WPA
    sysconfig.SetVariable("SCAN_SSID", "1"); // since WPA is usually only used for wireless

    sysconfig.Save();
    SystemExec("setup-net restart");
}

//###########################################################################
//  cSetupWLANMenuFunctions
//###########################################################################

cSetupWLANMenuFunctions::cSetupWLANMenuFunctions() : cOsdMenu(tr("Wireless LAN: Functions"))
{
    Set();
    Display();
}

cSetupWLANMenuFunctions::~cSetupWLANMenuFunctions()
{
}

void cSetupWLANMenuFunctions::Set()
{
    SetHasHotkeys();
    Add(new cOsdItem(hk(tr("Retrieve DHCP")), osUser1, true));
    Add(new cOsdItem(hk(tr("Reset WLAN settings")), osUser2, true));
    Add(new cOsdItem(hk(tr("Turn WLAN off")), osUser3, true));
}

