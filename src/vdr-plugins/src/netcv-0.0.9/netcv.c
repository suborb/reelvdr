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
 * netcv.c
 *
 ***************************************************************************/

#include <fstream>
#include <string.h>

#include <vdr/plugin.h>
#include <mcliheaders.h>

#include "netcvmenu.h"
#include "netcvinstallmenu.h"
#include "netcvinfomenu.h"
#include "netcvthread.h"
#include "netcvdiag.h"
#include "netcvservice.h"

static const char *VERSION        = "0.0.8";
static const char *DESCRIPTION    = "NetCeiver plugin";
static const char *MAINMENUENTRY  = trNOOP("Antenna settings");

class cPluginNetcv : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cNetCVDevices* GetNetceivers();
  bool GetTunerList(netcv_service_tunerlist_data *TunerList);
  bool GetInterface(char **Interface);
  bool StoreRotorsetting(netcv_service_storerotor_data *RotorData);
  char *TmpPath_;
public:
  cPluginNetcv(void);
  virtual ~cPluginNetcv();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; };
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

  cNetCVThread *NetcvThread_;
  bool bRunningUpdate_;
  };

cPluginNetcv::cPluginNetcv(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  TmpPath_ = strdup("/tmp/Netcv.XXXXXX");
  mkdtemp(TmpPath_);
  NetcvThread_ = new cNetCVThread();
  //recv_init(NULL,0); //************* commented out by tvkeller, obsolete, done by mcli
}

cPluginNetcv::~cPluginNetcv()
{
  // Clean up after yourself!
  if(NetcvThread_)
      delete NetcvThread_;
  free(TmpPath_);
  //recv_exit(); //************* commented out by tvkeller, obsolete, done by mcli
}

const char *cPluginNetcv::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginNetcv::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginNetcv::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginNetcv::Start(void)
{
    // Start any background activities the plugin shall perform.
#if 0 //def RBMINI
    bool found = false;
    char *buffer;
    FILE *file = popen("netcvdiag -c", "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while(!found && buffer)
        {
            if(strstr(buffer, "Count: 0"))
            {
                // No NetCeiver found
                Channels.Load(NULL);
                found = true;
            }
            buffer = readline.Read(file);
        }
        pclose(file);
    }
#endif
    return true;
}

void cPluginNetcv::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginNetcv::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginNetcv::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginNetcv::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginNetcv::MainMenuAction(void)
{
//  // Perform the action when selected from the main VDR menu.
//  return NULL;
    return new cNetCVMenu(NetcvThread_, &TmpPath_);
}

cMenuSetupPage *cPluginNetcv::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginNetcv::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginNetcv::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if (strcmp(Id, "Netceiver update status") == 0)
  {
      if (Data != NULL)
      {
          bool **bRunningUpdate = (bool**) Data;
          bRunningUpdate_ = NetcvThread_->Active();
          *bRunningUpdate = &bRunningUpdate_;
      }
      return true;
  }
  else if (strcmp(Id, "Netceiver Install Wizard") == 0)
  {
      if (Data != NULL)
      {
          netcv_service_data *data = (netcv_service_data*) Data;
          data->menu = new cNetcvInstallMenu(&TmpPath_, data->title);
      }
      return true;
  }
  else if (strcmp(Id, "Netceiver Information") == 0)
  {
      if (Data != NULL)
      {
          netcv_service_data *data = (netcv_service_data*) Data;
          data->menu = new cNetCVInfoMenu();
      }
      return true;
  }
  else if(strcmp(Id, "Netceiver Tuner List") == 0)
  {
      if(Data != NULL)
      {
          netcv_service_tunerlist_data *TunerList = (netcv_service_tunerlist_data*) Data;
          return GetTunerList(TunerList);
      }
  }
  else if(strcmp(Id, "Netceiver Rotorsetting Store") == 0)
  {
      if(Data != NULL)
      {
          netcv_service_storerotor_data *RotorData = (netcv_service_storerotor_data*) Data;
          return StoreRotorsetting(RotorData);
      }
  }
  return false;
}

const char **cPluginNetcv::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginNetcv::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

cNetCVDevices* cPluginNetcv::GetNetceivers()
{
    cNetCVDevices *netcvdevices = NULL;
    bool TmpAvailable = true;

    if (NetcvThread_->Active())
        Skins.Message(mtError, tr("NetCeiver Firmwareupdate is running!"));
    else
    {
        cNetCVDiag *netcvdiag = new cNetCVDiag();

        if(netcvdiag->GetNetceiverCount() < 1)
            Skins.Message(mtError, tr("No NetCeiver found!"));
        else
        {
            // Check if TmpPath is valid
            DIR *dp;

            if((dp = opendir(TmpPath_)) == NULL)
            {
                // Retry first
                free(TmpPath_);
                TmpPath_ = strdup("/tmp/Netcv.XXXXXX");
                mkdtemp(TmpPath_);
                if((dp = opendir(TmpPath_)) == NULL)
                {
                    Skins.Message(mtError, tr("Creating directory tmp failed!"));
                    TmpAvailable = false;
                }
            }
            if(TmpAvailable)
            {
                char *Interface = NULL;
                if(GetInterface(&Interface))
                {
                    netcvdevices = new cNetCVDevices(TmpPath_, Interface);

                    if(netcvdiag->GetInformation(netcvdevices))
                        netcvdevices->LoadSettings();
                    else
                    {
                        delete netcvdevices;
                        netcvdevices = NULL;
                        Skins.Message(mtError, tr("Retrieving information about tuners failed!"));
                    }
                }
                else
                    Skins.Message(mtError, tr("Getting network interface failed!"));
            }
        }
    }

    return netcvdevices;
}

bool cPluginNetcv::GetTunerList(netcv_service_tunerlist_data *TunerList)
{
    cNetCVDevices *netcvdevices = GetNetceivers();
    if(netcvdevices)
    {
        int count = 0;
        for (tunerIt_t tunerIter = (*(netcvdevices->Begin()))->tuners_.Begin(); tunerIter != (*(netcvdevices->Begin()))->tuners_.End(); ++tunerIter)
        {
            int TunerType = -1;
            if(tunerIter->GetType().find("DVB-S2") == 0)
                TunerType = eSatS2tuner;
            else if(tunerIter->GetType().find("DVB-S") == 0)
                TunerType = eSattuner;
            else if(tunerIter->GetType().find("DVB-C") == 0)
                TunerType = eCabletuner;
            else if(tunerIter->GetType().find("DVB-T") == 0)
                TunerType = eTerrtuner;

            if((TunerList->SatTuner && (TunerType == eSattuner))
                    || (TunerList->CableTuner && (TunerType == eCabletuner))
                    || (TunerList->SatS2Tuner && (TunerType == eSatS2tuner))
                    || (TunerList->TerrTuner && (TunerType == eTerrtuner)))
            {
                netcv_service_tunerlist_data_tuner tuner;
                asprintf(&tuner.Name, "%s (%s)", tunerIter->GetID().c_str(), tunerIter->GetTuner().c_str());
                tuner.Tuner = count;
                tuner.Active = tunerIter->GetPreference()>=0?1:0;
                tuner.TunerType = TunerType;
                TunerList->Tuners.push_back(tuner);
            }
            ++count;
        }
        return true;
    }
    else
        return false;
}

bool cPluginNetcv::GetInterface(char **Interface)
{
    std::fstream file;
    std::string strBuff;
    size_t index, index2;
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
                index2 =strBuff.find("\"", ++index);
                if (index2 == std::string::npos)
                    return false;
                strBuff.at(index2) = '\0';

                *Interface = strdup(strBuff.substr(index).c_str());
                found = true;
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

bool cPluginNetcv::StoreRotorsetting(netcv_service_storerotor_data *RotorData)
{
    cNetCVDevice *netcvdevice = *(GetNetceivers()->Begin()); // Get first NetCeiver
    cDiseqcSetting *DiseqcSetting = netcvdevice->DiseqcSetting_; // Get DiSEqC-Settings of NetCeiver

    if(netcvdevice)
    {
        for (tunerIt_t tunerIter = netcvdevice->tuners_.Begin(); tunerIter != netcvdevice->tuners_.End(); ++tunerIter)
        {
            if((tunerIter->GetType().find("DVB-S") == 0) || (tunerIter->GetType().find("DVB-S2") == 0)) // SatTuner only
                if(DiseqcSetting->GetSatlistByName(tunerIter->GetSatlist().c_str())->GetMode() == eGotoX)
                    tunerIter->SetPreference(-1); // Disable Tuner
        }

        if(RotorData->Tuner != -1)
        {
            // Search "Rotor" Satlist, if not found then create a new one
            bool found = false;
            unsigned int SatlistNumber = 0;
            do
            {
                if(!strcmp(DiseqcSetting->GetSatlistVector()->at(SatlistNumber)->Name_, "Rotor"))
                    found = true;
                else
                    ++SatlistNumber;
            }
            while(!found && SatlistNumber < DiseqcSetting->GetSatlistVector()->size());
            if(!found)
                DiseqcSetting->AddSatlist("Rotor");

            // Get selected Tuner
            tunerIt_t tunerIter = netcvdevice->tuners_.Begin();
            for (int i=0; i<RotorData->Tuner; ++i)
                ++tunerIter;

            tunerIter->SetPreference(0); // Enable Tuner
            tunerIter->SetSatList("Rotor"); // Use Rotor-Satlist
            DiseqcSetting->GetSatlistVector()->at(SatlistNumber)->SetMode(eGotoX); // Set mode to GotoX
            // Set following settings for GotoX
            cGotoX *GotoX = static_cast<cGotoX*>(*(DiseqcSetting->GetSatlistVector()->at(SatlistNumber)->GetModeIterator()));
            GotoX->iPositionMin_ = RotorData->PositionMin;
            GotoX->iPositionMax_ = RotorData->PositionMax;
            GotoX->iLongitude_ = RotorData->Longitude;
            GotoX->iLatitude_ = RotorData->Latitude;
            GotoX->iAutoFocus_ = RotorData->Autofocus;
        }

        // Send Data to NetCeiver
        if(DiseqcSetting->SaveSetting() == 0)
            return true;
    }
    
    return false;
}

VDRPLUGINCREATOR(cPluginNetcv); // Don't touch this!
