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

#include "netcvmenu.h"
#include "netcvdiag.h"
#include "diseqcsettingitem.h"
#include "netcvupdatemenu.h"

#include "../netcvrotor/netcvrotorservice.h"

#include <vdr/skins.h>
#include <vdr/menu.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>

#include <fstream>
#include <algorithm>

#define MENUTITLE trNOOP("Antenna settings")
#define UPDATE_DIRECTORY "/usr/share/reel/netcv/update"
#define UPDATE_FILENAME_LENGTH 17

bool isUpdateAvailable(std::string UpdateFile, std::string Firmware)
{
    std::string strUpdate;
    std::string strFirmware;
    if(!UpdateFile.size())
        return false;

    strUpdate.assign(UpdateFile, UPDATE_FILENAME_LENGTH - 7, 3);
    strFirmware.assign(Firmware, Firmware.length() - 3, 3);

    std::transform(strUpdate.begin(), strUpdate.end(), strUpdate.begin(), static_cast < int(*)(int) >(toupper));
    std::transform(strFirmware.begin(), strFirmware.end(), strFirmware.begin(), static_cast < int(*)(int) >(toupper));

    if (strUpdate > strFirmware)
        return true;
    else
        return false;
}

void GetUpdateFile(std::string *UpdateFile)
{
    std::string strBuff = "";
    std::string strBuff2 = "";

    // Get newest update file
    DIR *dp;
    struct dirent *dirp;

    if((dp = opendir(UPDATE_DIRECTORY)) != NULL)
    {
        while ((dirp = readdir(dp)) != NULL)
        {
            strBuff = std::string(dirp->d_name);
            if ((int)strBuff.length() == UPDATE_FILENAME_LENGTH)
                if (strBuff.substr(UPDATE_FILENAME_LENGTH - 4) == ".tgz")
                {
                    strBuff2.assign(strBuff, 0, UPDATE_FILENAME_LENGTH - 7);
                    if (strBuff2 == "netceiver_")
                        if (strBuff > *UpdateFile)
                            *UpdateFile = strBuff;
                }
        }
        closedir(dp);
    }
}

// class cNetCVMenu ---------------------------------------------------

cNetCVMenu::cNetCVMenu(cNetCVThread *UpdateThread, char **TmpPath)
 : cOsdMenu(tr(MENUTITLE))
{
    UpdateThread_ = UpdateThread;
    TunerMode[0] = tr("Static antenna");
    TunerMode[1] = tr("Single Cable System");
    TunerMode[2] = tr("Rotor antenna");
    AutoFocusStrings[0] = tr("off");
    AutoFocusStrings[1] = tr("fine");
    AutoFocusStrings[2] = tr("coarse");
    AutoFocusStrings[3] = tr("maximum");

    Changed_ = false;
    ExpertMenu_ = false;
    RotorEnabled_ = false;

    if (UpdateThread_->Active())
    {
        Add(new cOsdItem(tr("Updating... please wait!"), osUnknown, false));
        netcvState_ = eUpdate;
    }
    else
    {
        iNetceiverCounter_ = 0;
        netcvState_ = eInit;
        Skins.Message(mtStatus, tr("Please wait..."));
        std::string strBuff;
        std::string strBuff2;

        // Check if TmpPath is valid
        DIR *dp;

        if((dp = opendir(*TmpPath)) == NULL)
        {
            // Retry first
            free(*TmpPath);
            *TmpPath = strdup("/tmp/Netcv.XXXXXX");
            mkdtemp(*TmpPath);
            if((dp = opendir(*TmpPath)) == NULL)
                netcvState_ = eErrorTmppath;
        }
        TmpPath_ = *TmpPath;

        if(netcvState_ == eInit)
        {

            // Run netcvdiag to get informations from netceivers & tuners
            cNetCVDiag *netcvdiag = new cNetCVDiag();

            iNetceiverCounter_ = netcvdiag->GetNetceiverCount();
            if(iNetceiverCounter_ < 1)
                netcvState_ = eError;

            GetInterface();
            netcvdevices_ = new cNetCVDevices(TmpPath_, Interface_.c_str());

            if(netcvdiag->GetInformation(netcvdevices_) && netcvState_ != eError)
            {
                if(netcvdevices_->LoadSettings()==0)
                {
                    netcvState_ = eDiagnose;
                    if (netcvdevices_->Size() == 1)
                        (*(*netcvdevices_).Begin())->bShowsetting_ = true;
                    // Following line goes to crash if there is no valid netceiver.conf
                    DiseqcSettingSimple_ = new cDiseqcSettingSimple((*(*netcvdevices_).Begin())->DiseqcSetting_, (&(*(*netcvdevices_).Begin())->tuners_));
                    Set();
                }
                else
                    netcvState_ = eErrorLoad;
            }
            else
                netcvState_ = eError;
        }
    }

    // If Update is available then go to update-menu
    std::string UpdateFile="";
    GetUpdateFile(&UpdateFile);

    if(netcvdevices_->Size())
    {
        cNetCVDevice *netcvdevice = *((*netcvdevices_).Begin());
        if(isUpdateAvailable(UpdateFile, netcvdevice->GetFW()))
        {
            strncpy(curVersion_, &(*(netcvdevice->GetFW().end()-VERSION_LENGTH)), VERSION_LENGTH+1); // +1 for '\0'
            strncpy(newVersion_, &(*(UpdateFile.end()-VERSION_LENGTH-4)), VERSION_LENGTH);
            newVersion_[3] = '\0';
            if ((strcasecmp(newVersion_, "9A5") >= 0) && (strcasecmp(curVersion_, "9A5") < 0)) {
                AddSubMenu(new cNetCVUpdateMenu(UpdateThread_, netcvdevice->GetUUID(), curVersion_, newVersion_, true));
            } else
                AddSubMenu(new cNetCVUpdateMenu(UpdateThread_, netcvdevice->GetUUID(), curVersion_, newVersion_));
        }
    }
}

cNetCVMenu::~cNetCVMenu()
{
}

void cNetCVMenu::Set()
{
    int current = Current();
    Clear();
#if REELVDR && APIVERSNUM >= 10716
    SetCols(22);
#else
    SetCols(14);
#endif /*REELVDR && APIVERSNUM >= 10716*/
    char strBuff[256];
    char type[32];

    //First check if a Rotor is enabled
    RotorEnabled_ = false;
    //Slots
    for(unsigned int i=0; i < DiseqcSettingSimple_->TunerSlots_.size(); ++i)
    {
        cNetCVTunerSlot *TunerSlot = DiseqcSettingSimple_->TunerSlots_.at(i);
        if(TunerSlot->Tuners_.size())
            //Tuners
            for(unsigned int j=0; j < TunerSlot->Tuners_.size(); ++j)
            {
                cNetCVTunerSimple *Tuner = TunerSlot->Tuners_.at(j);
                if(Tuner->Enabled_)
                {
                    if(Tuner->Mode_ == eTunerRotor)
                    RotorEnabled_ = true;
                }
                else
                    Tuner->Mode_ = eTunerDiseqc;
            }
    }

    //Slots
    for(unsigned int i=0; i < DiseqcSettingSimple_->TunerSlots_.size(); ++i)
    {
        cNetCVTunerSlot *TunerSlot = DiseqcSettingSimple_->TunerSlots_.at(i);

        if(TunerSlot->Tuners_.size())
        {
            if(!strcmp(TunerSlot->Type_, "DVB-S") || !strcmp(TunerSlot->Type_, "DVB-S2"))
                strcpy(type, tr("Satellite"));
            else if(!strcmp(TunerSlot->Type_, "DVB-C"))
                strcpy(type, tr("Cable"));
            else //if (TunerSlot->Type_ == "DVB-T")
                strcpy(type, tr("Terrestrial"));

            sprintf(strBuff, "%s %i - %s %s", tr("Slot"), i+1, type, TunerSlot->Type_);
            Add(new cOsdItem(strBuff, osUnknown, false));

            //Tuners
            for(unsigned int j=0; j < TunerSlot->Tuners_.size(); ++j)
            {
                cNetCVTunerSimple *Tuner = TunerSlot->Tuners_.at(j);
                if(TunerSlot->Tuners_.size() > 1)
                    sprintf(strBuff, "%s %c", tr("Tuner"), 'A'+j);
                else
                    strcpy(strBuff, tr("Tuner"));
                Add(new cMenuEditBoolRefreshItem(strBuff, &Tuner->Enabled_, tr("off"), tr("on")));
                if(Tuner->Diseqc_)
                {
                    if(Tuner->Enabled_)
                    {
                        strcpy(strBuff, tr("Antenna type"));
                        if(RotorEnabled_ && (Tuner->Mode_ != eTunerRotor))
                            Add(new cMenuEditStraRefreshItem(strBuff, &Tuner->Mode_, 2, TunerMode));
                        else
                            Add(new cMenuEditStraRefreshItem(strBuff, &Tuner->Mode_, 3, TunerMode));

                        if(Tuner->Mode_ == eTunerDiseqc)
                        {
                            sprintf(strBuff, "   %s", tr("Number of LNBs"));
                            Add(new cMenuEditIntRefreshItem(strBuff, &Tuner->NumberOfLNB_, 1, LNB_MAX));
                            for(int k=0; k < Tuner->NumberOfLNB_; ++k)
                            {
                                sprintf(strBuff, "   %s %c", tr("Satellite"), 'A'+k);
                                Add(new cMenuEditSrcItem(strBuff, &Tuner->Sources_.at(k)));
                            }
                        }
                        else if (Tuner->Mode_ == eTunerSingleCable)
                        {
                            // only 2 satellite position can be given in
                            // Single Cable Router EN50494 mode
                            //Tuner->NumberOfLNB_ = 2;
                            sprintf(strBuff, "   %s", tr("Number of LNBs"));
                            Add(new cMenuEditIntRefreshItem(strBuff, &Tuner->NumberOfLNB_, 1, 2));
                            for(int k=0; k < Tuner->NumberOfLNB_; ++k)
                            {
                                sprintf(strBuff, "   %s %d", tr("Satellite"), k+1);
                                Add(new cMenuEditSrcItem(strBuff, &Tuner->Sources_.at(k)));
                            }

                            sprintf(strBuff, "   %s", tr("Unique Slot Nr."));
                            Add(new cMenuEditIntItem(strBuff, &Tuner->scrProps.uniqSlotNr, 0, 7));

                            sprintf(strBuff, "   %s", tr("Downlink freq. (MHz)"));
                            Add(new cMenuEditIntItem(strBuff, &Tuner->scrProps.downlinkFreq, 900, 2200));

                            sprintf(strBuff, "   %s", tr("PIN (optional)"));
                            Add(new cMenuEditIntItem(strBuff, &Tuner->scrProps.pin, -1, 255,
                                                     tr("No pin")));

                        }
                        else if(Tuner->Mode_ == eTunerRotor)
                        {
                            sprintf(strBuff, "   %s", tr("Longitude"));
                            Add(new cMenuEditIntpItem(strBuff, &Tuner->Longitude_, 0,1800, tr("East"), tr("West")));
                            sprintf(strBuff, "   %s", tr("Latitude"));
                            Add(new cMenuEditIntpItem(strBuff, &Tuner->Latitude_, 0,1800, tr("North"), tr("South")));
                            sprintf(strBuff, "   %s", tr("Autofocus"));
                            Add(new cMenuEditStraItem(strBuff, &Tuner->AutoFocus_, 4, AutoFocusStrings));
                        }
                    }
                }
            }
            Add(new cOsdItem("", osUnknown, false));
        }
    }

    bool showRedButton = RotorEnabled_ && (cPluginManager::GetPlugin("netcvrotor") != 0);

    if(netcvState_==eDiagnose)
        SetHelp(showRedButton ? tr("Rotor") : NULL, NULL, NULL, ::Setup.ExpertOptions ? tr("Experts") : NULL);
    SetCurrent(Get(current));
    Display();
}

void cNetCVMenu::Display()
{
    switch(netcvState_)
    {
        case eInit:
            break;
        case eDiagnose:
        case eUpdate:
            cOsdMenu::Display();
            break;
        default:
            break;
    }
}

eOSState cNetCVMenu::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    if(HadSubMenu)
    {
        if(!HasSubMenu() && ExpertMenu_) // If ExpertMenu is closed
            return osBack; // don't go back to simple setting!
        else
            return state;
    }

    if(netcvState_ == eError)
    {
        std::string strBuff;
        if(iNetceiverCounter_ < 1)
            strBuff = tr("No NetCeiver found!");
        else
            strBuff = tr("Unknown Error! Sorry!");
        Skins.Message(mtError, strBuff.c_str(), 3);
        return osBack;
    }
    else if(netcvState_ == eErrorDiseqc)
    {
        Skins.Message(mtError, tr("Internal Error: DiSEqC not working"), 3);
        return osBack;
    }
    else if(netcvState_ == eErrorLoad)
    {
        Skins.Message(mtError, tr("Internal Error: Loading netceiver.conf failed"), 3);
        return osBack;
    }
    if(netcvState_ == eDiagnose)
    {
        if((Key == kRight) || (Key == kLeft))
            Changed_ = true;
        if(state == os_User) // Refresh Menu
        {
            Set();
            Display();
            return osContinue;
        }
        if (Key == kBlue && ::Setup.ExpertOptions)
        {
            ExpertMenu_ = true;
            return AddSubMenu(new cNetCVExpertMenu(UpdateThread_, TmpPath_, &netcvState_, netcvdevices_, Interface_.c_str()));
        }
        else if(Key == kOk)
        {
            DiseqcSettingSimple_->WriteSetting();
            if((*(netcvdevices_->Begin()))->DiseqcSetting_->SaveSetting() == 0)
                return osBack;
        }
        else if(Key == kBack)
        {
            if(Changed_)
            {
                if(Interface->Confirm(tr("Save changes?")))
                {
                    DiseqcSettingSimple_->WriteSetting();
                    if((*(netcvdevices_->Begin()))->DiseqcSetting_->SaveSetting() == 0)
                        return osBack;
                    else
                        return osContinue;
                }
                else
                    return osBack;
            }
            else
                return osBack;
        }
        else if(Key == kRed)
        {
            if(RotorEnabled_ && cPluginManager::GetPlugin("netcvrotor"))
            {
                if(Changed_)
                {
                    DiseqcSettingSimple_->WriteSetting();
                    (*(netcvdevices_->Begin()))->DiseqcSetting_->SaveSetting();
                    Changed_ = false;
                }
                cPlugin *RotorPlugin = cPluginManager::GetPlugin("netcvrotor");
                if(RotorPlugin)
                {
                    netcvrotor_service_data RotorMenu;
                    RotorPlugin->Service("netcvrotor.OpenMenu", &RotorMenu);
                    return AddSubMenu(RotorMenu.menu);
                }
                Skins.Message(mtError, tr("No netcvrotor-Plugin found!"));
            }
        }
    }
    return state;
}

bool cNetCVMenu::GetInterface()
{
    std::fstream file;
    std::string strBuff;
    int index, index2;
    bool found = false;
    const char *filename = "/etc/default/mcli";

    file.open(filename, std::ios::in);
    if(file)
    {
        getline(file, strBuff);
        while(!file.eof() && !found)
        {
            if(strBuff.find("NETWORK_INTERFACE=") == 0)
            {
                index = strBuff.find("\"", 0);
                index2 = strBuff.find("\"", ++index);

                Interface_.assign(strBuff, index, index2-index);
            }
            getline(file, strBuff);
        }
        file.close();
    }
    else
    {
        esyslog("%s:%s: Couldn't load file \"%s\"", __FILE__, __FUNCTION__, filename);
        return false;
    }

    if(found)
        return true;
    else
        return false;
}

// class cNetCVExpertMenu ---------------------------------------------
cNetCVExpertMenu::cNetCVExpertMenu(cNetCVThread *UpdateThread, const char *TmpPath, eNetCVState *netcvState, cNetCVDevices *netcvdevices, const char *Interface)
 : cOsdMenu(tr(MENUTITLE)), TmpPath_(TmpPath), Interface_(Interface)
{
    netcvState_ = netcvState;

    UpdateThread_ = UpdateThread;
    netcvdevices_ = netcvdevices;

    std::string strBuff;
    std::string strBuff2;
    UpdateFile_ = "";

    GetUpdateFile(&UpdateFile_);
    Set();
}

cNetCVExpertMenu::~cNetCVExpertMenu()
{
}

void cNetCVExpertMenu::AddLine(const char *strItem, const char *strValue, int level)
{
    std::string strBuff = "";

    for (int i=0; i < level; ++i)
        strBuff.append(" ");
    strBuff.append(strItem);
    strBuff.append(":\t");
    strBuff.append(strValue);
    Add(new cOsdItem(strBuff.c_str(), osUnknown, false));
}

void cNetCVExpertMenu::Set()
{
    bool UseDiseqc = false;
    std::string strBuff;
    int current = Current();

    Clear();

    SetCols(18);

    for (devIt_t devIter = netcvdevices_->Begin(); devIter != netcvdevices_->End(); ++devIter)
    {
        Add(new cNetCVDeviceItem(*devIter, eDevice));
        if ((*devIter)->bShowsetting_)
        {
            if (UpdateFile_ == "")
                Add(new cNetCVDeviceItem(*devIter, eInformation));
            else
                Add(new cNetCVDeviceItem(*devIter, eInformation, UpdateThread_, UpdateFile_, Interface_, isUpdateAvailable(UpdateFile_, (*devIter)->GetFW())));
            if ((*devIter)->bShowinfo_)
            {
                AddLine("UUID", (*devIter)->GetUUID(), 6);
                AddLine(tr("Alive"), (*devIter)->GetALIVE()?tr("Yes"):tr("No"), 6);
                AddLine(tr("Tuners"), itoa((*devIter)->GetTuners()), 6);
                AddLine(tr("OS"), (*devIter)->GetOS().c_str(), 6);
                AddLine(tr("App"), (*devIter)->GetApp().c_str(), 6);
                AddLine(tr("Firmware"), (*devIter)->GetFW().c_str(), 6);
                AddLine(tr("Hardware"), (*devIter)->GetHW().c_str(), 6);
                AddLine(tr("Serial"), (*devIter)->GetSerial().c_str(), 6);
                //AddLine(tr("Vendor"), devIter->GetVendor());
                AddLine(tr("System uptime"), (*devIter)->GetSystemUptime().c_str(), 6);
                AddLine(tr("Process uptime"), (*devIter)->GetProcessUptime().c_str(), 6);
            }

            // CAMs
            Add(new cNetCVDeviceItem(*devIter, eCam));
            if ((*devIter)->bShowcam_)
            {
                int nrAlphaCrypts = 0;
                int nrOtherCams = 0;
                for (camIt_t camIter = (*devIter)->cams_.Begin(); camIter != (*devIter)->cams_.End(); ++camIter) {
                    if(strlen((*camIter).cam_.menu_string)) {
                        if (strncmp((*camIter).cam_.menu_string, "Alpha", 5) == 0 || strncmp((*camIter).cam_.menu_string, "easy.TV", 7) == 0)
                            nrAlphaCrypts++;
                        else
                            nrOtherCams++;
                    }
                }

                bool mtdImpossible = nrAlphaCrypts > 0 && nrOtherCams > 0;
                for (camIt_t camIter = (*devIter)->cams_.Begin(); camIter != (*devIter)->cams_.End(); ++camIter)
                {
                    if(strlen((*camIter).cam_.menu_string) && strncmp((*camIter).cam_.menu_string, "Alpha", 5) == 0)
                    {
                        Add(new cNetCVCamItem(TmpPath_, Interface_, &(*camIter), *devIter));
                        {
                            if(camIter->bShowinfo_)
                            {
#if 0
                                AddLine(tr("Slot"), itoa((*camIter).cam_.slot+1), 9);
                                AddLine(tr("Status"), itoa((*camIter).cam_.status), 9);
#endif
                                strBuff = "         ";
                                if (mtdImpossible) {
                                    strBuff.append(tr("Multi-Transponder-Decryption is"));
                                    Add(new cOsdItem(strBuff.c_str()));
                                    strBuff = "         ";
                                    strBuff.append(tr("impossible because of mixed CAMs"));
                                    Add(new cOsdItem(strBuff.c_str()));
                                } else {
                                    strBuff.append(tr("Multi-Transponder"));
                                    Add(new cMenuEditIntItem(strBuff.c_str(), (int*)&((*camIter).cam_.flags), 1, 2, tr("No"), tr("Yes")));
                                }
                            }
                        }
                    }
                }
            }

            // Tuners
            Add(new cNetCVDeviceItem(*devIter, eTuner));
            if ((*devIter)->bShowtuner_)
            {
                for (tunerIt_t tunerIter = (*devIter)->tuners_.Begin(); tunerIter != (*devIter)->tuners_.End(); ++tunerIter)
                {
                    Add(new cNetCVTunerItem(TmpPath_, Interface_, &(*tunerIter), *devIter, tunerIter->GetPreference()==-1?false:true));
                    if (tunerIter->bShowinfo_)
                    {
                        if((tunerIter->GetType().find("DVB-S") == 0) || (tunerIter->GetType().find("DVB-S2") == 0))
                            Add(new cNetCVTunerSatlistItem(&(*tunerIter), TmpPath_, Interface_, *devIter));
                        //AddLine(tr("State"), tunerIter->GetState().c_str(), 9);
                        Add(new cNetCVTunerPreferenceItem(&(*tunerIter), (*devIter)->DiseqcSetting_));
                        strBuff = tunerIter->GetSlot();
                        ++strBuff.at(0);
                        ++strBuff.at(2);
                        AddLine(tr("Slot"), strBuff.c_str(), 9);
                        AddLine(tr("Chip"), tunerIter->GetTuner().c_str(), 9);
                        if(tunerIter->GetBER()!="deaddead")
                        {
                            AddLine(tr("Used"), itoa(tunerIter->GetUsed()), 9);
                            AddLine(tr("Lock"), tunerIter->GetLOCK()?tr("Yes"):tr("No"), 9);
                            AddLine(tr("Frequency"), tunerIter->GetFrequency().c_str(), 9);
                            strBuff = tunerIter->GetStrength();
                            strBuff.append("/");
                            strBuff.append(tunerIter->GetSNR());
                            AddLine(tr("Signal (strength/snr)"), strBuff.c_str(), 9);
                            strBuff = tunerIter->GetBER();
                            strBuff.append("/");
                            strBuff.append(tunerIter->GetUNC());
                            AddLine(tr("Signal (ber/unc)"), strBuff.c_str(), 9);
                            AddLine(tr("electric$Current"), tunerIter->GetNimCurrent().c_str(), 9);
                        }
                        else
                            AddLine(tr("electric$Current"), tr("Power save mode"), 9);
                    }
                }
            }

            // Check if any Sat-Tuner is available, then display antenna settings
            for (tunerIt_t tunerIter = (*devIter)->tuners_.Begin(); tunerIter != (*devIter)->tuners_.End(); ++tunerIter)
                if((tunerIter->GetType().find("DVB-S") == 0) || (tunerIter->GetType().find("DVB-S2") == 0))
                    UseDiseqc = true;
            if(UseDiseqc)
            {
                Add(new cNetCVDiseqcSettingItem(TmpPath_, Interface_, *devIter));
                if ((*devIter)->bShowDiseqcSetting_)
                    (*devIter)->DiseqcSetting_->BuildMenu(this);
            }
        }
    }
    SetCurrent(Get(current));

    // Move to previous Item if the selected Item is deleted
    if (current != Current())
        SetCurrent(Get(current-1));
    Display();
}

eOSState cNetCVExpertMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (!HasSubMenu())
    {
        switch (state)
        {
            case os_User:  // Redraw menu
                Set();
                Display();
                break;
            default:
                break;
        }
    }

    if (*netcvState_ != eRestart)
    {
        cNetCVDeviceItem *netcvitem = dynamic_cast<cNetCVDeviceItem*>(Get(Current()));
        cNetCVDiseqcSettingItem *netcvdiseqcsettingitem = dynamic_cast<cNetCVDiseqcSettingItem*>(Get(Current()));
        cNetCVTunerItem *netcvtuneritem = dynamic_cast<cNetCVTunerItem*>(Get(Current()));
        cNetCVCamItem *netcvcamitem = dynamic_cast<cNetCVCamItem*>(Get(Current()));
        cNetCVTunerSatlistItem *netcvtunersatlistitem = dynamic_cast<cNetCVTunerSatlistItem*>(Get(Current()));
        cSatlistItem *satlistitem = dynamic_cast<cSatlistItem*>(Get(Current()));
        cNetCVTunerPreferenceItem *netcvtunerpreferenceitem = dynamic_cast<cNetCVTunerPreferenceItem*>(Get(Current()));

        if (netcvitem && netcvitem->UpdateAvailable())
            SetHelp(tr("Update firmware"));
        else if (netcvdiseqcsettingitem && netcvdiseqcsettingitem->bShow_)
            SetHelp(tr("Save"), tr("Load"), tr("Clear"), tr("Add DiSEqC list"));
        else if (netcvcamitem && netcvcamitem->bShow_)
            SetHelp(tr("Save"));
        else if (netcvtuneritem && netcvtuneritem->bShow_)
        {
            if(netcvtuneritem->bPower_)
                SetHelp(tr("Save"), tr("disable"));
            else
                SetHelp(tr("Save"), tr("enable"));
        }
        else if (satlistitem)
            satlistitem->SetHelpKeys();
        else if (netcvtunersatlistitem || netcvtunerpreferenceitem)
            SetHelp(tr("Save"));
        else
            SetHelp(NULL);
    }

    if (*netcvState_ == eErrorTmppath)
    {
        Skins.Message(mtError, tr("No tmp files available."), 3);
        return osBack;
    }

    if (UpdateThread_)
        if (UpdateThread_->Active())
        {
            SetHelp(NULL);
            *netcvState_ = eUpdate;
            if (UpdateThread_->iProgress_ >= 0)
            {
                if (UpdateThread_->iProgress_ < 100)
                {
                    std::string strBuff = (tr("Update progress: "));
                    strBuff.append(itoa(UpdateThread_->iProgress_));
                    strBuff.append("%");
                    Skins.Message(mtStatus, strBuff.c_str());
                }
                else
                    Skins.Message(mtStatus, tr("Updating... please wait!"));
            }
        }
    return state;
}

// class cNetCVDeviceItem ---------------------------------------------
cNetCVDeviceItem::cNetCVDeviceItem(cNetCVDevice *netceiver, eSubItem SubItem, cNetCVThread *UpdateThread, std::string filename, std::string Interface, bool update)
{
    std::string strBuff;

    netceiver_ = netceiver;
    UpdateThread_ = UpdateThread;
    SubItem_ = SubItem;
    filename_ = filename;
    Interface_ = Interface;
    bUpdate_ = update;

    switch (SubItem)
    {
        case eDevice:
            if(netceiver_->bShowsetting_)
                strBuff = "-  ";
            else
                strBuff = "+ ";
            strBuff.append(netceiver->GetName());
            ++strBuff.at(strBuff.length()-1);
            SetText(strBuff.c_str());
            break;
        case eInformation:
            strBuff = "   ";
            if(netceiver_->bShowinfo_)
                strBuff.append("-  ");
            else
                strBuff.append("+ ");
            strBuff.append(tr("Information"));
            SetText(strBuff.c_str());
            break;
        case eCam:
            strBuff = "   ";
            if(netceiver_->bShowcam_)
                strBuff.append("-  ");
            else
                strBuff.append("+ ");
            strBuff.append(tr("CAM settings"));
            SetText(strBuff.c_str());
            break;
        case eTuner:
            strBuff = "   ";
            if(netceiver_->bShowtuner_)
                strBuff.append("-  ");
            else
                strBuff.append("+ ");
            strBuff.append(tr("Tuner settings"));
            SetText(strBuff.c_str());
            break;
        default:
            break;
    }
}

cNetCVDeviceItem::~cNetCVDeviceItem()
{
}

int cNetCVDeviceItem::UpdateNetceiver()
{
    std::string strBuff = "netcvupdate -i ";
    strBuff.append(netceiver_->GetUUID());
    if(Interface_.size())
    {
        strBuff.append(" -d ");
        strBuff.append(Interface_);
    }
    strBuff.append(" -X ");
    strBuff.append(UPDATE_DIRECTORY);
    strBuff.append("/");
    strBuff.append(filename_);

    if (UpdateThread_->SetCommand(strBuff.c_str()))
    {
        Skins.Message(mtStatus, tr("Please wait..."));
        return UpdateThread_->Start();
    }
    else
        return false;
}

eOSState cNetCVDeviceItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kOk:
            {
                switch (SubItem_)
                {
                    case eDevice:
                        netceiver_->bShowsetting_ = !netceiver_->bShowsetting_;
                        break;
                    case eInformation:
                        netceiver_->bShowinfo_ = !netceiver_->bShowinfo_;
                        break;
                    case eCam:
                        netceiver_->bShowcam_ = !netceiver_->bShowcam_;
                        break;
                    case eTuner:
                        netceiver_->bShowtuner_ = !netceiver_->bShowtuner_;
                        break;
                    default:
                        break;
                }
            }
            return os_User; // Rebuild menu
            break;
        case kRed:
            if (UpdateThread_)
                if (UpdateThread_->Active())
                    return osUnknown;
            if (bUpdate_)
            {
                UpdateNetceiver();
                return osContinue;
            }
            return osUnknown;
            break;
        default:
            return osUnknown;
            break;
    }
}

// class cNetCVCamItem ----------------------------------------------
cNetCVCamItem::cNetCVCamItem(const char *TmpPath, const char *Interface, NetCVCam *netcvcam, cNetCVDevice *NetcvDevice)
{
    std::string strBuff = "      ";

    NetcvDevice_ = NetcvDevice;
    TmpPath_ = TmpPath;
    Interface_ = Interface;

    netceiverID_ = NetcvDevice->GetUUID();
    cam_ = netcvcam;
    bShow_ = cam_->bShowinfo_;
    if (cam_->bShowinfo_)
        strBuff.append("-  ");
    else
        strBuff.append("+ ");
    strBuff.append(tr("CAM"));
    strBuff.append(" ");
    strBuff.append(itoa(cam_->cam_.slot+1));
    strBuff.append(" (");
    strBuff.append(cam_->cam_.menu_string);
    strBuff.append(")");
    SetText(strBuff.c_str());
}

cNetCVCamItem::~cNetCVCamItem()
{
}

eOSState cNetCVCamItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kOk:
            cam_->bShowinfo_ = !cam_->bShowinfo_;
            return os_User;
            break;
        case kRed:
            if(cam_->bShowinfo_)
            {
                NetcvDevice_->DiseqcSetting_->SaveSetting();
                return osContinue;
            }
            else
                return osUnknown;
            break;
        default:
            return osUnknown;
            break;
    }
}

// class cNetCVTunerItem ----------------------------------------------
cNetCVTunerItem::cNetCVTunerItem(const char *TmpPath, const char *Interface, cNetCVTuner *netcvtuner, cNetCVDevice *NetcvDevice, bool bPower)
{
    std::string strBuff = "      ";

    NetcvDevice_ = NetcvDevice;
    TmpPath_ = TmpPath;
    Interface_ = Interface;

    netceiverID_ = NetcvDevice->GetUUID();
    tuner_ = netcvtuner;
    bShow_ = tuner_->bShowinfo_;
    if (tuner_->bShowinfo_)
        strBuff.append("-  ");
    else
        strBuff.append("+ ");
    strBuff.append(tuner_->GetID());
    ++strBuff.at(strBuff.length()-1);
    strBuff.append(" (");
    strBuff.append(tuner_->GetType());
    strBuff.append(")");
    SetText(strBuff.c_str());

    bPower_ = bPower;
}

cNetCVTunerItem::~cNetCVTunerItem()
{
}

eOSState cNetCVTunerItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kOk:
            tuner_->bShowinfo_ = !tuner_->bShowinfo_;
            return os_User;
            break;
        case kRed:
            if(tuner_->bShowinfo_)
            {
                NetcvDevice_->DiseqcSetting_->SaveSetting();
                return osContinue;
            }
            else
                return osUnknown;
            break;
        case kGreen:
            if(tuner_->bShowinfo_)
            {
                if(tuner_->GetPreference() == -1)
                    tuner_->SetPreference(0);
                else
                    tuner_->SetPreference(-1);
                return os_User;
            }
        default:
            return osUnknown;
            break;
    }
}

// class cNetCVTunerSatlistItem----------------------------------------
cNetCVTunerSatlistItem::cNetCVTunerSatlistItem(cNetCVTuner *Tuner, const char *TmpPath, const char *Interface, cNetCVDevice *NetcvDevice)
{
    Diseqcsetting_ = NetcvDevice->DiseqcSetting_;
    Tuners_ = &(NetcvDevice->tuners_);
    Tuner_ = Tuner;
    TmpPath_ = TmpPath;
    Interface_ = Interface;
    NetcvUUID_ = NetcvDevice->GetUUID();
    std::string strBuff("         ");
    strBuff.append(tr("DiSEqC list"));
    strBuff.append(":\t");
    cSatList *Satlist = Diseqcsetting_->GetSatlistByName(Tuner_->GetSatlist().c_str());
    if(Satlist)
        strBuff.append(Satlist->Name_);
    else
    {
        strBuff.append("(");
        strBuff.append(tr("not selected"));
        strBuff.append(")");
    }
    SetText(strBuff.c_str());
}

cNetCVTunerSatlistItem::~cNetCVTunerSatlistItem()
{
}

eOSState cNetCVTunerSatlistItem::ProcessKey(eKeys Key)
{
    std::vector<cSatList*> *SatlistVector = Diseqcsetting_->GetSatlistVector();

    if((Key == kRight) || (Key == kLeft) || (Key == (kRight|k_Repeat)) || (Key == (kLeft|k_Repeat)))
    {
        if(SatlistVector->empty())
            return osContinue;
        else
        {
            if(!Diseqcsetting_->GetSatlistByName(Tuner_->GetSatlist().c_str()))
                Tuner_->SetSatList((*(SatlistVector->begin()))->Name_);
            else
            {
                bool found=false;
                std::vector<cSatList*>::iterator satlistIter = SatlistVector->begin();

                while(!found)
                {
                    if(strcmp((*satlistIter)->Name_, Tuner_->GetSatlist().c_str()) == 0)
                        found = true;
                    else
                        ++satlistIter;
                }

                switch(Key)
                {
                    case kRight:
                    case kRight|k_Repeat:
                        if(++satlistIter == SatlistVector->end())
                            satlistIter = SatlistVector->begin();
                        break;
                    case kLeft:
                    case kLeft|k_Repeat:
                        if(satlistIter == SatlistVector->begin())
                            satlistIter = SatlistVector->end() - 1;
                        else
                            --satlistIter;
                        break;
                    default:
                        //Do nothing
                        break;
                }

                Tuner_->SetSatList((*satlistIter)->Name_);
            }
            return os_User;
        }
    }
    else if(Key == kRed)
    {
        Diseqcsetting_->SaveSetting();
        return osContinue;
    }
    return osUnknown;
}

// class cNetCVDiseqcSettingItem---------------------------------------
cNetCVDiseqcSettingItem::cNetCVDiseqcSettingItem(const char *TmpPath, const char *Interface, cNetCVDevice *NetcvDevice)
{
    TmpPath_ = TmpPath;
    Interface_ = Interface;
    DiseqcSetting_ = NetcvDevice->DiseqcSetting_;
    NetcvDevice_ = NetcvDevice;
    bShow_ = NetcvDevice_->bShowDiseqcSetting_;
    std::string strBuff;
    if (NetcvDevice_->bShowDiseqcSetting_)
        strBuff = "   -  ";
    else
        strBuff = "   + ";
    strBuff.append(tr("Dish settings"));
    SetText(strBuff.c_str());
}

cNetCVDiseqcSettingItem::~cNetCVDiseqcSettingItem()
{
}

eOSState cNetCVDiseqcSettingItem::ProcessKey(eKeys Key)
{
    switch (Key)
    {
        case kOk:
            NetcvDevice_->bShowDiseqcSetting_ = !NetcvDevice_->bShowDiseqcSetting_;
            return os_User;
            break;
        case kRed:
            if(bShow_)
            {
                DiseqcSetting_->SaveSetting();
                return osContinue;
            }
            break;
        case kGreen:
            if(bShow_)
            {
                Skins.Message(mtStatus, tr("Please wait..."));
                delete DiseqcSetting_;
                DiseqcSetting_ = new cDiseqcSetting(TmpPath_, Interface_, NetcvDevice_->GetUUID(), &(NetcvDevice_->cams_), &(NetcvDevice_->tuners_));
                int result = DiseqcSetting_->LoadSetting();
                if(result == 0)
                    Skins.Message(mtStatus, tr("Setting loaded"), 3);
                else
                {
                    std::string errNumber = tr("Loading failed! (Error: ");
                    errNumber.append(itoa(result));
                    errNumber.append(")");
                    Skins.Message(mtError, errNumber.c_str(), 3);
                }
                return os_User;
            }
            break;
        case kYellow:
            if(bShow_)
            {
                delete DiseqcSetting_;
                DiseqcSetting_ = new cDiseqcSetting(TmpPath_, Interface_, NetcvDevice_->GetUUID(), &(NetcvDevice_->cams_), &(NetcvDevice_->tuners_));
                return os_User;
            }
            break;
        case kBlue:
            if(bShow_)
            {
                DiseqcSetting_->AddSatlist(DiseqcSetting_->GenerateSatlistName().c_str());
                return os_User;
            }
        default:
            break;
    }
    return osUnknown;
}

