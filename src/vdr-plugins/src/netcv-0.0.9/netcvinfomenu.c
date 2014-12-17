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
 * netcvmenu.c
 *
 ***************************************************************************/

#include "netcvinfomenu.h"

#include <vdr/skins.h>
#include <vdr/menu.h>
#include "netcvdiag.h"

// class cNetCVInfoMenu -----------------------------------------------
cNetCVInfoMenu::cNetCVInfoMenu() 
 : cOsdMenu(tr("NetCeiver diagnostics")) 
{
    netcvState_ = eInit;
    Skins.Message(mtStatus, tr("Please wait..."));

    // Run netcvdiag to get informations from netceivers & tuners
    cNetCVDiag *netcvdiag = new cNetCVDiag();

    int NetceiverCount = netcvdiag->GetNetceiverCount();
    if(NetceiverCount < 1)
        netcvState_ = eError;
    else
    {
        netcvdevices_ = new cNetCVDevices(NULL, NULL);

        if(netcvdiag->GetInformation(netcvdevices_) && netcvState_ != eError)
        {
            netcvState_ = eDiagnose;
            for (devIt_t devIter = netcvdevices_->Begin(); devIter != netcvdevices_->End(); ++devIter)
            {
                (*devIter)->bShowsetting_ = true;
                (*devIter)->bShowinfo_ = true;
                (*devIter)->bShowtuner_ = true;
                for (tunerIt_t tunerIter = (*devIter)->tuners_.Begin(); tunerIter != (*devIter)->tuners_.End(); ++tunerIter)
                    tunerIter->bShowinfo_ = true;
            }
            Set();
        } else
            netcvState_ = eError;
    }
}

cNetCVInfoMenu::~cNetCVInfoMenu() 
{
}

void cNetCVInfoMenu::AddLine(std::string strItem, std::string strValue, int level)
{
    std::string strBuff = "";

    for (int i=0; i < level; ++i)
        strBuff.append(" ");
    strBuff.append(strItem.c_str());
    strBuff.append(":\t");
    for (int i=1; i < level; ++i)
        strBuff.append(" ");
    strBuff.append(strValue.c_str());
    Add(new cOsdItem(strBuff.c_str(), osUnknown, false));
}

void cNetCVInfoMenu::Set()
{
    std::string strBuff;
    int current = Current();

    Clear();

    SetCols(16);

    for (devIt_t devIter = netcvdevices_->Begin(); devIter != netcvdevices_->End(); ++devIter) 
    {
        Add(new cNetCVDeviceItem(*devIter, eDevice));
        if ((*devIter)->bShowsetting_)
        {
            Add(new cNetCVDeviceItem(*devIter, eInformation));
            if ((*devIter)->bShowinfo_)
            {
                AddLine("UUID", (*devIter)->GetUUID(), 6);
                AddLine(tr("Alive"), (*devIter)->GetALIVE()?tr("Yes"):tr("No"), 6);
                AddLine(tr("Tuners"), (std::string)itoa((*devIter)->GetTuners()), 6);
                AddLine(tr("OS"), (*devIter)->GetOS(), 6);
                AddLine(tr("App"), (*devIter)->GetApp(), 6);
                AddLine(tr("Firmware"), (*devIter)->GetFW(), 6);
                AddLine(tr("Hardware"), (*devIter)->GetHW(), 6);
                AddLine(tr("Serial"), (*devIter)->GetSerial(), 6);
                //AddLine(tr("Copyright:"), (*devIter)->GetVendor());
                AddLine(tr("System uptime"), (*devIter)->GetSystemUptime(), 6);
                AddLine(tr("Process uptime"), (*devIter)->GetProcessUptime(), 6);
            }
            Add(new cNetCVDeviceItem(*devIter, eTuner));
            if ((*devIter)->bShowtuner_)
            {
                strBuff = "  ";
                strBuff.append(tr("Dish settings"));
                for (tunerIt_t tunerIter = (*devIter)->tuners_.Begin(); tunerIter != (*devIter)->tuners_.End(); ++tunerIter) 
                {
                    Add(new cNetCVTunerItem("", "", &(*tunerIter), *(&(*devIter)), false));
                    if (tunerIter->bShowinfo_)
                    {
                        if(tunerIter->GetSatlist() != "")
                            AddLine(tr("DiSEqC list"), tunerIter->GetSatlist(), 9);
                        AddLine(tr("State"), tunerIter->GetState(), 9);
                        strBuff = tunerIter->GetSlot();
                        ++strBuff.at(0);
                        ++strBuff.at(2);
                        AddLine(tr("Slot"), strBuff, 9);
                        AddLine(tr("Chip"), tunerIter->GetTuner(), 9);
                        if(tunerIter->GetBER()!="deaddead")
                        {
                            AddLine(tr("Used"), (std::string)itoa(tunerIter->GetUsed()), 9);
                            AddLine(tr("Lock"), tunerIter->GetLOCK()?tr("Yes"):tr("No"), 9);
                            AddLine(tr("Frequency"), tunerIter->GetFrequency(), 9);
                            AddLine(tr("Strenght"), tunerIter->GetStrength(), 9);
                            AddLine(tr("SNR"), tunerIter->GetSNR(), 9);
                            AddLine(tr("BER"), tunerIter->GetBER(), 9);
                            AddLine(tr("UNC"), tunerIter->GetUNC(), 9);
                            AddLine(tr("electric$Current"), tunerIter->GetNimCurrent().c_str(), 9);
                        }
                        else
                            AddLine(tr("electric$Current"), tr("Power save mode"), 9);
                    }
                }
            }
        }
    }
    SetCurrent(Get(current));
    Display();
}

void cNetCVInfoMenu::Display()
{
    switch(netcvState_)
    {
        case eInit:
            break;
        case eDiagnose:
            cOsdMenu::Display();
            break;
        default:
            break;
    }
}

eOSState cNetCVInfoMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (!HasSubMenu())
    {
        switch (state)
        {
            case os_User:
                Set();
                Display();
                break;
            default:
                break;
        }
    }
    if (Key == kRed || Key == kGreen || Key == kYellow || Key == kBlue)
        state = osContinue;

    return state;
}

