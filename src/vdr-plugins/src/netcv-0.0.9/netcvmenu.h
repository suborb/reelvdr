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
 * netcvmenu.h
 *
 ***************************************************************************/

#ifndef NETCVMENU_H
#define NETCVMENU_H

#include "netcv.h"
#include "netcvdevice.h"
#include "netcvthread.h"
#include "diseqcsettingsimple.h"

#define VERSION_LENGTH 3

enum eSubItem { eDevice, eInformation, eCam, eTuner }; //, eDiseqcSetting };

class cNetCVMenu;

class cNetCVDeviceItem : public cOsdItem 
{
    private:
        cNetCVDevice *netceiver_;
        cNetCVThread *UpdateThread_;
        bool bUpdate_;
        std::string Interface_;
        std::string filename_;

    public:
        eSubItem SubItem_;

        cNetCVDeviceItem(cNetCVDevice *netceiver, eSubItem SubItem, cNetCVThread *UpdateThread = NULL, std::string filename = "", std::string Interface = "", bool update = false);
        ~cNetCVDeviceItem();

        int UpdateNetceiver();
        eOSState ProcessKey(eKeys Key);
        cNetCVDevice *GetNetceiver() { return netceiver_; };
        bool UpdateAvailable() { return bUpdate_; };
};

class cNetCVCamItem : public cOsdItem 
{
    private:
        cNetCVDevice *NetcvDevice_;
        const char *TmpPath_;
        const char *Interface_;
        std::string netceiverID_;
        NetCVCam *cam_;

    public:
        bool bShow_;

        cNetCVCamItem(const char *TmpPath, const char *Interface, NetCVCam *NetcvCam, cNetCVDevice *NetcvDevice);
        ~cNetCVCamItem();

        eOSState ProcessKey(eKeys Key);
        std::string NetceiverID() { return netceiverID_; };
};

class cNetCVTunerItem : public cOsdItem 
{
    private:
        cNetCVDevice *NetcvDevice_;
        const char *TmpPath_;
        const char *Interface_;
        std::string netceiverID_;
        cNetCVTuner *tuner_;

    public:
        bool bPower_;
        bool bShow_;

        cNetCVTunerItem(const char *TmpPath, const char *Interface, cNetCVTuner *Netcvtuner, cNetCVDevice *NetcvDevice, bool bPower);
        ~cNetCVTunerItem();

        eOSState ProcessKey(eKeys Key);
        std::string NetceiverID() { return netceiverID_; };
        std::string TunerID() { return tuner_->GetID(); };
};

class cNetCVTunerSatlistItem : public cOsdItem
{
    private:
        cDiseqcSetting *Diseqcsetting_;
        cNetCVTuners *Tuners_;
        cNetCVTuner *Tuner_;
        const char *TmpPath_;
        const char *Interface_;
        const char *NetcvUUID_;

    public:
        cNetCVTunerSatlistItem(cNetCVTuner *Tuner, const char *TmpPath, const char *Interface, cNetCVDevice *NetcvDevice);
        ~cNetCVTunerSatlistItem();

        eOSState ProcessKey(eKeys Key);
};

class cNetCVDiseqcSettingItem : public cOsdItem
{
    private:
        cDiseqcSetting *DiseqcSetting_;
        cNetCVDevice *NetcvDevice_;
        const char *TmpPath_;
        const char *Interface_;

    public:
        cNetCVDiseqcSettingItem(const char *TmpPath, const char *Interface, cNetCVDevice *NetCVDevice);
        ~cNetCVDiseqcSettingItem();
        bool bShow_;

        eOSState ProcessKey(eKeys Key);
};

class cNetCVMenu : public cOsdMenu
{
    private:
        cDiseqcSettingSimple *DiseqcSettingSimple_;
        cNetCVDevices *netcvdevices_;
        bool Changed_; // If any settings is changed, set this variable to true
        bool ExpertMenu_; // True if Expert-Menu is called
        bool RotorEnabled_;
        char *TmpPath_;
        int iNetceiverCounter_;
        cNetCVThread *UpdateThread_;
        char curVersion_[VERSION_LENGTH+1];
        char newVersion_[VERSION_LENGTH+1];
        std::string Interface_;
        bool GetInterface();
        const char* AutoFocusStrings[4];
        const char* TunerMode[3];

    public:
        cNetCVMenu(cNetCVThread *UpdateThread, char **TmpPath);
        ~cNetCVMenu();

        eNetCVState netcvState_;

        virtual void Set();
        virtual void Display();
        eOSState ProcessKey(eKeys key);
};

class cNetCVExpertMenu : public cOsdMenu
{
    private:
        cNetCVDevices *netcvdevices_;
        const char *TmpPath_;
        cNetCVThread *UpdateThread_;
        std::string UpdateFile_;
        const char *Interface_;

    public:
        cNetCVExpertMenu(cNetCVThread *UpdateThread, const char *TmpPath, eNetCVState *netcvState, cNetCVDevices *netcvdevices, const char *Interface);
        ~cNetCVExpertMenu();

        eNetCVState *netcvState_;

        void Sethelp(const char *Red = NULL, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL) { SetHelp(Red, Green, Yellow, Blue); };
        virtual void AddLine(const char *strItem, const char *strValue, int level);
        virtual void Set();
        eOSState ProcessKey(eKeys key);
};

#endif

