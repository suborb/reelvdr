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
 * netcvdevice.c
 *
 ***************************************************************************/

#include "netcvdevice.h"

#include <stdlib.h>
#include <vdr/menu.h>

#define UUID_LENGTH 256

// class cNetCVDevice ---------------------------------------------
cNetCVDevice::cNetCVDevice(const char* TmpPath, const char* Interface)
{
    bShowsetting_ = false;
    bShowinfo_ = false;
    bShowcam_ = false;
    bShowtuner_ = false;
    bShowDiseqcSetting_ = false;
    ALIVE_ = false;
    TmpPath_ = TmpPath;
    Interface_ = Interface;

    UUID_ = (char *) malloc(UUID_LENGTH*sizeof(char));

    // Caution:
    // Create DiSEqC-Settings even if there is no Sat-Tuner
    // If you don't create DiSEqC-Settings then you have problem with SatList,
    // if it's already saved on netceiver before physically removing Sat-Tuners
    DiseqcSetting_ = new cDiseqcSetting(TmpPath_, Interface_, UUID_, &cams_, &tuners_);
}

cNetCVDevice::~cNetCVDevice()
{
    free(UUID_);
}

void cNetCVDevice::SetDeviceInformation(std::string strString)
{
    int index;
    std::string strTemp;

    index = strString.find("NetCeiver ", 0);
    if (index != -1){
        strTemp.assign( strString, index, strString.length() - 1);
        name_ = strTemp;
    }

    index = strString.find("UUID <", 0);
    if (index != -1)
        strncpy(UUID_, GrepValue(strString, index).c_str(), UUID_LENGTH);

    if (strString.find("ALIVE", 0) != std::string::npos)
        ALIVE_ = true;

    index = GrepIntValue(strString, "tuners ");
    if (index != -1)
        tunercount_ = index;

    index = strString.find("OS <", 0);
    if (index != -1)
        OS_ = GrepValue(strString, index);

    index = strString.find("App <", 0);
    if (index != -1)
        App_ = GrepValue(strString, index);

    index = strString.find("FW <", 0);
    if (index != -1)
        FW_ = GrepValue(strString, index);

    index = strString.find("HW <", 0);
    if (index != -1)
        HW_ = GrepValue(strString, index);

    index = strString.find("Serial <", 0);
    if (index != -1)
        Serial_ = GrepValue(strString, index);

    index = strString.find("Vendor <", 0);
    if (index != -1)
        Vendor_ = GrepValue(strString, index);

    index = GrepIntValue(strString, "state ");
    if (index != -1)
        state_ = index;

    index = GrepIntValue(strString, "SystemUptime ");
    if (index != -1)
        SystemUptime_ = index;

    index = GrepIntValue(strString, "ProcessUptime ");
    if (index != -1)
        ProcessUptime_ = index;
}

void cNetCVDevice::SetTunerInformation(std::string strString, int number)
{
    tuners_.SetTunerInformation(strString, number);
}

std::string cNetCVDevice::GrepValue(std::string strString, int index)
{
    std::string::size_type length;

    index = strString.find("<", index) + 1;
    length = strString.find(">", index) - index;
    strString.assign( strString, index, length);

    return strString;
}

int cNetCVDevice::GrepIntValue(std::string strString, std::string strKeyword)
{
    int index;
    std::string::size_type length;
    index = strString.find(strKeyword, 0);

    if (index == -1)
        return -1;
    else
    {
        index += strKeyword.length();
        length = strString.find(" ", index);
        if (length != std::string::npos)
            length = length - index - 1;
        strString.assign(strString, index, length);

        return atoi(strString.c_str());
    }
}

std::string cNetCVDevice::ConvertTime(int iTime)
{
    std::string strBuff;
    int seconds = iTime;
    int days = 0;
    int hours = 0;
    int minutes = 0;


    if (seconds >= 60)
    {
        minutes  = abs(seconds/60);
        seconds -= minutes*60;
        strBuff  = std::string(itoa(seconds)) + tr("sec");
    }

    if (minutes >= 60)
    {
        hours    = abs(minutes/60);
        minutes -= hours*60;
        strBuff  = std::string(itoa(minutes)) + tr("min") + " " + strBuff;
    }

    if (hours >= 24)
    {
        days     = abs(hours/24);
        hours    = hours - days*24;
    }

    if (hours > 0)
        strBuff = std::string(itoa(hours)) + tr("h") + " " + strBuff;

    if (days > 0)
        strBuff = std::string(itoa(days)) + tr("d") + " " + strBuff;

    return strBuff;
}

// class cNetCVDevices ---------------------------------------------
cNetCVDevices::cNetCVDevices(const char* TmpPath, const char* Interface)
{
    TmpPath_ = TmpPath;
    Interface_ = Interface;
}

cNetCVDevices::~cNetCVDevices()
{
}

int cNetCVDevices::LoadSettings()
{
    int RetValue=0;
    for (devIt_t devIter = netcvdevice_.begin(); devIter < netcvdevice_.end(); ++devIter)
    {
        if ((*devIter)->GetALIVE())
        {
            RetValue = (*devIter)->LoadSettings();
            if (RetValue)
                return RetValue;
        }
    }
    return 0;
}

