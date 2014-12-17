/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * netcvinstallmenu.c
 *
 ***************************************************************************/

#include "netcvdiag.h"
#include "netcvinstallmenu.h"
#include "diseqcsettingitem.h"

#include <vdr/diseqc.h>
#include <vdr/plugin.h>

#include <fstream>

struct StructImage
{
    int x;
    int y;
    char path[255];
    bool blend;
    int slot;
    int w;
    int h;
};

// class cNetcvInstallMenu -----------------------------------------
cNetcvInstallMenu::cNetcvInstallMenu(char **TmpPath, const char* title) : cOsdMenu(title)
{
    NetcvDevices_ = NULL;
    NumberOfLNB_ = 2;
    NumberOfTuners_ = 1;
    DiseqcSetting_ = false;

    // Check if TmpPath is valid
    DIR *dp;

    if((dp = opendir(*TmpPath)) == NULL)
    {
        // Retry first
        free(*TmpPath);
        *TmpPath = strdup("/tmp/Netcv.XXXXXX");
        mkdtemp(*TmpPath);
        if((dp = opendir(*TmpPath)) == NULL)
            Skins.Message(mtError, tr("Couldn't create tmp-path!"));
    }
    TmpPath_ = *TmpPath;

    cNetCVDiag *netcvdiag = new cNetCVDiag();
    if(netcvdiag->GetNetceiverCount())
    {
        GetInterface();
        NetcvDevices_ = new cNetCVDevices(TmpPath_, Interface_.c_str());
        if(NetcvDevices_ && netcvdiag->GetInformation(NetcvDevices_))
        {
            bool foundLiveNetcv;
            devIt_t netcvDevice = GetFirstLiveNetCVDevice(foundLiveNetcv);

           // no live netceivers found
            if (!foundLiveNetcv) {
                esyslog("%s:%d no live netceiver found, checked %d netcv", __FILE__, __LINE__, NetcvDevices_->Size());
                Add(new cOsdItem(tr("Please check the Connection to NetCeiver!"), osUnknown, true));
                Skins.Message(mtError, tr("No NetCeiver found!"));
                Display();
                return;
            }
            
            MaxTuner_ = (*netcvDevice)->GetTuners();
            NumberOfTuners_ = MaxTuner_;

            cNetCVTuners *Tuners = &((*netcvDevice)->tuners_);
            tunerIt_t tunerIter = Tuners->Begin();

            // Check if there are Sat-Tuners
            while(tunerIter != Tuners->End())
            {
                if(((*tunerIter).GetType().find("DVB-S") == 0) || ((*tunerIter).GetType().find("DVB-S2") == 0))
                    DiseqcSetting_ = true;
                ++tunerIter;
            }

            for(int i = 0; i < 4; ++i)
                Sources_.push_back(cSource::FromString("S19.2E")); // default Astra S19.2E
            Sources_.at(1) = cSource::FromString("S13.0E"); // Default value for 2nd LNB: Hotbird 13.0E

            Set();
        }
        else
        {
            Add(new cOsdItem(tr("Internal Error! Sorry!"), osUnknown, true));
            Skins.Message(mtError, tr("Loading data from NetCeiver failed!"));
        }
    }
    else
    {
        Add(new cOsdItem(tr("Please check the Connection to NetCeiver!"), osUnknown, true));
        Skins.Message(mtError, tr("No NetCeiver found!"));
    }
    Display();
}

cNetcvInstallMenu::~cNetcvInstallMenu()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL );
}

devIt_t cNetcvInstallMenu::GetFirstLiveNetCVDevice(bool &found)
{
    devIt_t netcvDevice = NetcvDevices_->Begin();
    int i = 0;
    while (netcvDevice != NetcvDevices_->End()) {
        printf("Searching for live netceiver %d\n", i);
        if ((*netcvDevice)->GetALIVE())
            break;
        ++netcvDevice;
        ++i;
    } // while
    
    // found a live netceiver ?
    found = (netcvDevice != NetcvDevices_->End());
    
    return netcvDevice;
}

bool cNetcvInstallMenu::GetInterface()
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

void ShowBackPlateImage()
{
    struct StructImage Image;
    Image.x = 210;
    Image.y = 250;
    Image.h = 0;
    Image.w = 0;
    Image.blend = true;
    Image.slot = 0;
    
    // this page is suitable only for Avantgardes (?)
    // check for Avg and clients here ?
    
    bool isICE = (cPluginManager::GetPlugin("reelice") != NULL);
    if (isICE) {
        Image.y = 280;
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-left-Sat.png");
    }
    else
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-Sat-left.png");
    
    cPluginManager::CallAllServices("setThumb", (void*)&Image );
    Image.x = 360;
    Image.slot = 1;
    
    if (isICE) {
        Image.x = 210 + 162;
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-right-plain.png");
    }
    else
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-Sat-right.png");
    
    cPluginManager::CallAllServices("setThumb", (void*)&Image );
}

void cNetcvInstallMenu::Set()
{
    int current = Current();
    Clear();
    SetCols(20);

    if(DiseqcSetting_)
    {
        char *strBuff = NULL;

        Add(new cMenuEditIntItem(tr("Number of Tuners"), &NumberOfTuners_, 1, MaxTuner_));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Please choose the satellites you can receive:"), osUnknown, false));
        Add(new cMenuEditIntRefreshItem (tr("Number of LNBs"), &NumberOfLNB_, 1, 4)); // max 4 LNBs since we don't have much space on screen
        for(int i = 0; i < NumberOfLNB_; ++i)
        {
            asprintf(&strBuff, "   LNB %i", i+1);
            Add(new cMenuEditSrcItem(strBuff, &(Sources_.at(i))));
            free(strBuff);
        }
    }
    else
    {
        Add(new cMenuEditIntItem(tr("Number of Tuners"), &NumberOfTuners_, 1, MaxTuner_));
    }

    SetHelp(NULL, tr("Back"), tr("Skip"));
    SetCurrent(Get(current));
    Display();

    // Draw Image
    ShowBackPlateImage();

}

eOSState cNetcvInstallMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(!HasSubMenu())
    {
        if(state == osUnknown)
        {
            switch(Key)
            {
                case kOk:
                    if(NetcvDevices_ && DiseqcSetting_)
                    {
                        WriteSetting();
                        bool foundLiveNetcv;
                        devIt_t netcvDevice = GetFirstLiveNetCVDevice(foundLiveNetcv);
                        if(foundLiveNetcv && (*netcvDevice)->DiseqcSetting_ 
                                && (*netcvDevice)->DiseqcSetting_->SaveSetting() == 0)
                            return osUser1; // Next step
                        Skins.Message(mtError, tr("Error saving settings"));
                    }
                    else
                        return osUser1; // Next step
                    break;
                default:
                    break;
            }
        }
        else if(state == os_User) // Refresh menu
            Set();
        Display();
    }

    return state;
}

void cNetcvInstallMenu::WriteSetting()
{
    bool found; // live netceiver found?
    devIt_t netcvDev = GetFirstLiveNetCVDevice(found);
    
    if (!found) {
        esyslog("%s:%d no live netceiver found", __FILE__ , __LINE__);
        return;
    }
    
    cDiseqcSetting *DiseqcSetting = (*netcvDev)->DiseqcSetting_;

    if(DiseqcSetting_)
    {
        eDiseqcMode DiseqcMode;
        if(NumberOfLNB_ < 5)
            DiseqcMode = eDiseqc1_0;
        else
            DiseqcMode = eDiseqc1_1;

        if(!DiseqcSetting->GetSatlistVector()->size())
            DiseqcSetting->AddSatlist(DiseqcSetting->GenerateSatlistName().c_str());

        cSatList *SatList = DiseqcSetting->GetSatlistVector()->at(0);
        SatList->SetMode(DiseqcMode);
        std::vector<cDiseqcSettingBase*>::iterator modeIter = SatList->GetModeIterator();

        switch(DiseqcMode)
        {
            case eDiseqcDisabled:
            case eDiseqcMini:
                // not implemented
                break;

            case eDiseqc1_0:
                (*modeIter)->SetNumberOfLNB(NumberOfLNB_);
                for(int i = 0; i < NumberOfLNB_; ++i)
                    (*modeIter)->GetLNBSetting(i)->iSource_ = Sources_.at(i);
                break;

            case eDiseqc1_1:
                {
                    cDiseqc1_1 *Diseqc1_1 = static_cast<cDiseqc1_1*>(*modeIter);
                    Diseqc1_1->iMultiswitch_ = 1;
                    Diseqc1_1->SetNumberOfLNB(NumberOfLNB_);
                    for(int i = 0; i < NumberOfLNB_; ++i)
                    {
                        Diseqc1_1->GetLNBSetting(i)->iSource_ = Sources_.at(i);
                        Diseqc1_1->GetLNBSetting(i)->iCascade_ = (i/4)+1;
                    }
                }
                break;

            case eDiseqc1_2:
            case eGotoX:
            case eDisiCon4:
                // not implemented
                break;
        }
    }

    // enable/disable Tuners and refer tuners to Satlist
    tunerIt_t tunerIter = (*netcvDev)->tuners_.Begin();
    for(int i=0; i < MaxTuner_; ++i)
    {
        if (tunerIter == (*netcvDev)->tuners_.End()) // no more tuners
            break;
        
        if(i<NumberOfTuners_)
            (*tunerIter).SetPreference(0);
        else
            (*tunerIter).SetPreference(-1);
        if(DiseqcSetting_)
            (*tunerIter).SetSatList(DiseqcSetting->GetSatlistVector()->at(0)->Name_);
        ++tunerIter;
    }
}

