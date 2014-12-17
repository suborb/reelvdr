#include <stdio.h>

#include "energyoptionsmenu.h"
#include "vdrsetupclasses.h"
#include <vdr/debug.h>

//#include <fstream>
#include <iostream>
//#include <sstream>              // zeicheketten streams
//#include <string>
//#include <vector>

#define MAX_LETTER_PER_LINE 48

using std::cerr;
using std::cout;
using std::endl;

//-----------------------------------------------------------
//---------------cEnergyOptionsMenu--------------------------
//-----------------------------------------------------------

cEnergyOptionsMenu::cEnergyOptionsMenu(std::string title)
:cMenuSetupPage(), energySpareMode(0), timeTillShutdown(0), sysConfig(cSysConfig_vdr::GetInstance())
{
    data = Setup;
    const char *tmp;

    std::string ds_enable = sysConfig.GetVariable("DS_ENABLE");
    if(ds_enable == "yes")
    {
        energySpareMode = 1;
    }

    //cerr << __PRETTY_FUNCTION__ << __LINE__ << " DS_ENABLE: " << ds_enable << endl;

    if ((tmp = sysConfig.GetVariable("DS_TIMEOUT_NEW")) != NULL)
    {
        sscanf(tmp, "%d", &timeTillShutdown);
    }
    else
    {
        cerr << "DS_TIMEOUT_NEW not set.\n" << endl;
        if ((tmp = sysConfig.GetVariable("DS_TIMEOUT")) != NULL)
        {
            sscanf(tmp, "%d", &timeTillShutdown);
            timeTillShutdown = timeTillShutdown * 60;
        }
        else
            timeTillShutdown = 120;
    }

    cerr << "After sscanf, timeTillShutdown: " << timeTillShutdown << "\n" << endl;

#if APIVERSNUM < 10700
    SetCols(25, 20);
#else
    SetCols(27, 20);
#endif
    SetSection(title.c_str());
    Set();
}

eOSState cEnergyOptionsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cMenuSetupPage::ProcessKey(Key);

    if (Key == kRight || Key == kLeft || Key == kDown || Key == kUp)
    {
        Set();
    }
    else if (Key == kBack)
        return osBack;
    else if (Key == kOk)
        return osBack;

    //return state;
    return osUnknown;
}

void cEnergyOptionsMenu::Set()
{
    ShutDownModes[0] = tr("Request");
    ShutDownModes[1] = trVDR("Standby");
    ShutDownModes[2] = trVDR("Power off");

    int current = Current();

    Clear();

    Add(new cMenuEditStraItem(trVDR("Setup.Miscellaneous$Power On/Off option"),
                              &data.RequestShutDownMode, 3, ShutDownModes));
#if APIVERSNUM < 10700
    if (data.RequestShutDownMode != 2)
        Add(new cMenuEditBoolItem(trVDR("Setup.Miscellaneous$Standby Mode"),
                                  &data.StandbyOrQuickshutdown,trVDR("Standard"), trVDR("Option$Quick")));
#endif

    Add(new cOsdItem("", osUnknown, false)); // blank line

    Add(new cMenuEditBoolItem(tr("Automatic power saving"), &energySpareMode, tr("inactive"), tr("active")));
    if(energySpareMode == 1)
    {
        Add(new cMenuEditIntItem(tr(" Standby after (min)"), &data.MinUserInactivity, 0, 480, trVDR("never")));
        if (!data.StandbyOrQuickshutdown && data.RequestShutDownMode != 2)
            Add(new cMenuEditIntItem(tr(" Poweroff after (min)"), &timeTillShutdown, 0, 241, tr("immediately"),
                                                                                                       trVDR("never")));
    }

    if (current >= 0                                        // prevent crash
        && Setup.ReelboxModeTemp == eModeServer             // MultiRoom == active (Server)
        && (std::string(Get(current)->Text()).find(trVDR("Setup.Miscellaneous$Standby Mode")) == 0 // Standby-Mode selected
        && !data.StandbyOrQuickshutdown                     // Standby-Mode == Standard
        || data.RequestShutDownMode == 2))                   // Shutdown == Poweroff
    {
        Add(new cOsdItem("", osUnknown, false)); // blank line
        Add(new cOsdItem("", osUnknown, false)); // blank line
        Add(new cOsdItem(tr("Warning:"), osUnknown, false));
        AddFloatingText(tr("Will cause problems with the MultiRoom-System when Clients are active and try to access the ReelBox. "
                                                                    ), MAX_LETTER_PER_LINE);
        if (data.RequestShutDownMode == 2)                   //Shutdown == Poweroff
            AddFloatingText(tr("Therefore, do not turn off the ReelBox."), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("Should therefore remain on 'Quick'."), MAX_LETTER_PER_LINE);
    }

    SetCurrent(Get(current));
    Display();
}

void cEnergyOptionsMenu::Store()
{
    std::string ds_enable;
    if(energySpareMode == 1)
    {
        sysConfig.SetVariable("POWERSAVINGS", "yes");
        ds_enable = "yes";
        sysConfig.SetVariable("DS_ENABLE", ds_enable.c_str());
        sysConfig.SetVariable("DS_TIMEOUT_NEW", itoa (timeTillShutdown));
    }
    else
    {
        sysConfig.SetVariable("POWERSAVINGS", "no");
        ds_enable = "no";
        sysConfig.SetVariable("DS_ENABLE", "no");
        sysConfig.SetVariable("DS_TIMEOUT_NEW", "241");  //241 means "never"
    }

    sysConfig.Save();

    Setup = data;
#if APIVERSNUM >= 10716 && REELVDR
    Setup.EnergySaveOn = energySpareMode;
    Setup.StandbyTimeout = timeTillShutdown;
#endif
    Setup.Save();
}



