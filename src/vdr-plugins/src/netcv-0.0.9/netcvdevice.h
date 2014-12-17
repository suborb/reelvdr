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
 * netcvdevice.h
 *
 ***************************************************************************/

#ifndef NETCVDEVICE_H
#define NETCVDEVICE_H

#include "netcvtuner.h"

class cNetCVDevice;

typedef std::vector<cNetCVDevice*>::iterator devIt_t;

class cNetCVDevice
{
    private:
        std::string name_;
        char *UUID_;
        bool ALIVE_;
        int tunercount_;
        std::string OS_;
        std::string App_;
        std::string FW_;
        std::string HW_;
        std::string Serial_;
        std::string Vendor_;
        int state_;
        int SystemUptime_;
        int ProcessUptime_;

        const char *TmpPath_;
        const char *Interface_;

        std::string GrepValue(std::string strString, int index);
        int GrepIntValue(std::string strString, std::string strKeyword);

    public:
        cNetCVTuners tuners_;
        cNetCVCams cams_;
        bool bShowsetting_;
        bool bShowinfo_;
        bool bShowcam_;
        bool bShowtuner_;
        bool bShowDiseqcSetting_;
        cDiseqcSetting *DiseqcSetting_;

        cNetCVDevice(const char* TmpPath, const char* Interface);
        ~cNetCVDevice();

        void SetDeviceInformation(std::string strString);
        std::string GetName() const { return name_; };
        const char* GetUUID() const { return UUID_; };
        bool GetALIVE() const { return ALIVE_; };
        int GetTuners() const { return tunercount_; };
        std::string GetOS() const { return OS_; };
        std::string GetApp() const { return App_; };
        std::string GetFW() const { return FW_; };
        std::string GetHW() const { return HW_; };
        std::string GetSerial() const { return Serial_; };
        std::string GetVendor() const { return Vendor_; };
        int GetState() const { return state_; };
        std::string GetSystemUptime() { return ConvertTime(SystemUptime_); };
        std::string GetProcessUptime() { return ConvertTime(ProcessUptime_); };

        int LoadSettings() { return DiseqcSetting_->LoadSetting(); };

        void SetTunerInformation(std::string strString, int number);
        tunerIt_t AddTuner() { return tuners_.Add(); };

        std::string ConvertTime(int iTime);
};

class cNetCVDevices
{
    private:
        std::vector<cNetCVDevice*> netcvdevice_;
        const char *TmpPath_;
        const char *Interface_;

    public:
        cNetCVDevices(const char* TmpPath, const char* Interface);
        ~cNetCVDevices();
        devIt_t Add() { netcvdevice_.push_back(new cNetCVDevice(TmpPath_, Interface_)); return netcvdevice_.end() -1; };
        devIt_t Begin() { return netcvdevice_.begin(); };
        devIt_t End() { return netcvdevice_.end(); };
        size_t Size() { return netcvdevice_.size(); };
        int LoadSettings();
};

#endif
