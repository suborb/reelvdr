/***************************************************************************
 *   vim: set syntax=cpp expandtab tabstop=4 shiftwidth=4:                 *
 *                                                                         *
 *   Copyright (C) 2005 by Reel Multimedia;  Author  Markus Hahn           * 
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
 *
 ***************************************************************************
 *
 *   channelscan: A plugin for VDR
 *   channelscan.c vdr-plugin interface - definitions
 *
 ***************************************************************************/


#include "service.h"
#include "csmenu.h"
#include "channelscan.h"
#include <vdr/videodir.h>
#include <vdr/device.h>
#include <vdr/remote.h>

eAutoScanStat cPluginChannelscan::AutoScanStat = AssNone;

cScanSetup ScanSetup;
bool OnlyChannelList = false;

// --- cMenuChannelscanSetup  ------------------------------------------------------

cMenuChannelscanSetup::cMenuChannelscanSetup()
{
    data_ = ScanSetup;
    serviceTypeTexts[0] = tr("TV only");
    serviceTypeTexts[1] = tr("Radio only");
    serviceTypeTexts[2] = tr("HDTV only");
    serviceTypeTexts[3] = tr("Radio + TV");
    Setup();
}

void cMenuChannelscanSetup::Setup()
{
    int current = Current();
    Clear();

    Add(new cMenuEditStraItem(tr("Servicetype"), &data_.ServiceType, 4, serviceTypeTexts));
    Add(new cMenuEditBoolItem(tr("Enable Logfile"), &data_.EnableLogfile));

    SetCurrent(Get(current));
    Display();
}

void cMenuChannelscanSetup::Store(void)
{
    ScanSetup = data_;
#ifdef REELVDR
    //SetupStore("ServiceType", "channelscan", ScanSetup.ServiceType);
    SetupStore("Logfile", "channelscan", ScanSetup.EnableLogfile);
#else
    //SetupStore("ServiceType", ScanSetup.ServiceType);
    SetupStore("Logfile", ScanSetup.EnableLogfile);
#endif
}


// --- cPluginChannelscan  ---------------------------------------------------------

cPluginChannelscan::cPluginChannelscan(void):channelDataPresent_(false)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginChannelscan::~cPluginChannelscan()
{
    // Clean up after yourself!
}

const char *cPluginChannelscan::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginChannelscan::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginChannelscan::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginChannelscan::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}

void cPluginChannelscan::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginChannelscan::MainMenuAction(void)
{

    // Perform the action when selected from the main VDR menu.
#ifdef REELVDR
    if (!AnyFreeTuners())
    {
        Skins.Message(mtError, tr("No tuners, please stop recording and retry"), 4);
        return NULL;
    }
#endif
    if (channelDataPresent_)
    {
        channelDataPresent_ = false;
        cRemote::Put(kRed);     //start channelscan
#if VDRVERSNUM < 10716
        return new cMenuChannelscan(channelData_.source, channelData_.frequency, channelData_.symbolrate, channelData_.polarization, true);
#else
        return new cMenuChannelscan(channelData_.source, channelData_.frequency, channelData_.symbolrate, channelData_.polarization);
#endif
    }
    else
        return new cMenuChannelscan();
}

cMenuSetupPage *cPluginChannelscan::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    //return new cMenuChannelscanSetup;
    return NULL;
}

bool cPluginChannelscan::Service(const char *Id, void *Data)
{
#ifdef REELVDR
    if (Id && strcmp(Id, "SelectChannelsMenu") == 0)
    {
        sSelectChannelsMenu *serviceData = (sSelectChannelsMenu *) Data;
        serviceData->pSelectMenu = new cMenuSelectChannelList();
        return true;
    }
    if (Id && strcmp(Id, "ChannelList") == 0)
    {
        sSelectChannelsList *serviceData = (sSelectChannelsList *) Data;
        serviceData->pSelectMenu = new cMenuSelectChannelList(serviceData->newTitle, serviceData->Path, serviceData->AdditionalText, serviceData->WizardMode);
        return true;
    }
#endif // REELVDR

    if (Id && strcmp(Id, "Perform channel scan") == 0)
    {
        channelData_ = *(static_cast < ChannelScanData * >(Data));
        channelDataPresent_ = true;

        cRemote::CallPlugin("channelscan");
        return true;
    }

    if (Id && strcmp(Id, "AutoScan") == 0)
    {
        //printf(" [channelscan] Id Flag set: %s  DATA %s \n", Id, (char *)Data);
        if (Data && strcmp(static_cast < const char *>(Data), "DVB-S") == 0)
            AutoScanStat = AssDvbS;
        else if (Data && strcmp(static_cast < const char *>(Data), "DVB-C") == 0)
            AutoScanStat = AssDvbC;
        else if (Data && strcmp(static_cast < const char *>(Data), "DVB-T") == 0)
            AutoScanStat = AssDvbT;
        else
        {
            esyslog("channelscan: wrong Service Data: \"%s\" ", static_cast < const char *>(Data));
            AutoScanStat = AssNone;
        }
        return true;
    }
    if (Id && strcmp(Id, "Channelscan menu Install Wizard") == 0 && Data) {
        struct DataMenu{ cOsdMenu *pmenu; };
        DataMenu *m = (DataMenu*) Data;
        m->pmenu = new cMenuChannelscan(0, 12551,22000, 'V',  /*default params*/
                                        true /*wiz. mode*/);
        return true;
    }
    return false;
}

bool cPluginChannelscan::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    /* if (!strcasecmp(Name, "ServiceType")) {
       // ServiceType is dead, don't use it! ScanSetup.ServiceType = atoi(Value);
       } else if (!strcasecmp(Name, "Logfile")) {
       ScanSetup.EnableLogfile = atoi(Value);
       if (ScanSetup.EnableLogfile > 0) {
       ScanSetup.logfile = VideoDirectory;
       ScanSetup.logfile += "/channelscan.log";
       }
       } else {
       return false;
       } 
       return true; */
    return false;
}

const char **cPluginChannelscan::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
        "LIST\n" "    Open channelscan plugin (Channel-List).",
        NULL
    };
    return HelpPages;
}

cString cPluginChannelscan::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (strcasecmp(Command, "LIST") == 0)
    {
        OnlyChannelList = true;
        cRemote::cRemote::CallPlugin("channelscan");
        cRemote::Put(kBlue);
        return "Ok";
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginChannelscan);   // Don't touch this!
