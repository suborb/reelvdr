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
 * netcvtuner.c 
 *
 ***************************************************************************/

#include "netcvtuner.h"
#include <vdr/menu.h>

#define PREFERENCE_MAX 15

// class cNetCVTuner ---------------------------------------------
cNetCVTuner::cNetCVTuner()
{
    bShowinfo_ = false;
}

cNetCVTuner::~cNetCVTuner()
{
}

void cNetCVTuner::SetTunerInformation(std::string tmpStdOut)
{
    int index;

    index = tmpStdOut.find("slot ", 0);
    if (index != -1){
        slotactive_ = tmpStdOut.at(index+5)=='-'?false:true; // 5 = strlen of "slot "
//        slot_ = GrepValue(tmpStdOut, index); //obsolete
    }

    index = tmpStdOut.find("type ", 0);
    if (index != -1)
        type_ = GrepValue(tmpStdOut, index);

    index = GrepIntValue(tmpStdOut, "used: ");
    if (index != -1)
        used_ = index;

    if (tmpStdOut.find("LOCK", 0) != std::string::npos)
    {
        if (tmpStdOut.find("NO LOCK", 0) == std::string::npos)
            LOCK_ = true;
        else
            LOCK_ = false;
    }

    index = tmpStdOut.find("frequency ", 0);
    if (index != -1)
        frequency_ = GrepValue(tmpStdOut, index);

    index = tmpStdOut.find("strength ", 0);
    if (index != -1)
        strength_ = GrepValue(tmpStdOut, index);

    index = tmpStdOut.find("snr ", 0);
    if (index != -1)
        snr_ = GrepValue(tmpStdOut, index);

    index = tmpStdOut.find("ber ", 0);
    if (index != -1)
        ber_ = GrepValue(tmpStdOut, index);

    index = tmpStdOut.find("unc ", 0);
    if (index != -1)
        unc_ = GrepValue(tmpStdOut, index);

    index = tmpStdOut.find("NIMCurrent ", 0);
    if (index != -1)
        nimcurrent_ = GrepValue(tmpStdOut, index);

}

std::string cNetCVTuner::GrepValue(std::string strString, int index)
{
    std::string::size_type length;
    
    index = strString.find(" ", index) + 1;
    length = strString.find(",", index) - 1;
    if (length != std::string::npos)
        length = strString.find(",", index) - index;
    strString.assign( strString, index, length);

    return strString.c_str();
}

int cNetCVTuner::GrepIntValue(std::string strString, std::string strKeyword)
{
    int index;
    std::string::size_type length;
    index = strString.find(strKeyword, 0);

    if (index == -1)
        return -1;
    else
    {
        index += strKeyword.length() + 1;
        while(strString.find(" ", index) != std::string::npos)
            ++index;
        length = strString.find(" ", index);
        if (length != std::string::npos)
            length = length - index - 1;
        strString.assign(strString, index, length);

        return atoi(strString.c_str());
    }
}

std::string cNetCVTuner::GetConvertedSlot()
{
    int slot;
    char number;
    if(slot_.size() == 3)
    {
        number = slot_.at(0);
        slot = atoi(&number) * 2;
        number = slot_.at(2);
        slot += atoi(&number);
    }
    else
        return slot_;

    return (std::string)itoa(slot);

}

std::string cNetCVTuner::GetState()
{
    int iPreference = GetPreference();
    if (iPreference > 0)
    {
        std::string strBuff = tr("Priority");
        strBuff.append(" ");
        strBuff.append(itoa(iPreference));
        return strBuff;
    }
    else if (iPreference == 0)
        return tr("normal");
    else
        return tr("disabled");
}

// class cNetCVTuners ---------------------------------------------
cNetCVTuners::cNetCVTuners()
{
}

cNetCVTuners::~cNetCVTuners()
{
}

void cNetCVTuners::SetTunerInformation(std::string tmpStdOut, unsigned int number)
{
    if(number<netcvtuner_.size())
        netcvtuner_.at(number).SetTunerInformation(tmpStdOut);
    else
        esyslog("[netcv]: %s - number (%i) out of Range\n", __PRETTY_FUNCTION__, number);
}

// class cTunerPreferenceItem -------------------------------------
cNetCVTunerPreferenceItem::cNetCVTunerPreferenceItem(cNetCVTuner *Tuner, cDiseqcSetting *Diseqcsetting)
{
    char *strBuff;
    strBuff = (char*) malloc(128*sizeof(char));
    asprintf(&strBuff, "         %s:\t%s", tr("State"), Tuner->GetState().c_str());
    SetText(strBuff);
    free(strBuff);
    Tuner_ = Tuner;
    Diseqcsetting_ = Diseqcsetting;
}

cNetCVTunerPreferenceItem::~cNetCVTunerPreferenceItem()
{
}

eOSState cNetCVTunerPreferenceItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kLeft:
        case kLeft|k_Repeat:
            if(Tuner_->GetPreference() >= 0)
            {
                Tuner_->SetPreference(Tuner_->GetPreference() - 1);
                return os_User;
            }
            else
                return osContinue;
            break;

        case kRight:
        case kRight|k_Repeat:
            if(Tuner_->GetPreference() < PREFERENCE_MAX)
            {
                Tuner_->SetPreference(Tuner_->GetPreference() + 1);
                return os_User;
            }
            else
                return osContinue;
            break;

        case kRed:
            Diseqcsetting_->SaveSetting();
            return osContinue;
            break;

        default:
            return osUnknown;
    }
}

