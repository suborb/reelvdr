#ifndef SETUP_ENERGYOPTIONSMENU_H
#define SETUP_ENERGYOPTIONSMENU_H


#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/config.h>

#include "vdrsetupclasses.h"
#include <vdr/sysconfig_vdr.h>

//-----EnergyOptionsMenu--------------------------- 

class cEnergyOptionsMenu : public cMenuSetupPage
{
  private:   
    int energySpareMode;
    int timeTillShutdown;
    const char *ShutDownModes[3];   
    cSysConfig_vdr &sysConfig;
    void Set();

  protected:
    cSetup data;
    /*override*/ void Store();

  public:
    cEnergyOptionsMenu(std::string title = "");
    ~cEnergyOptionsMenu(){};

    /*override*/ eOSState ProcessKey(eKeys key);
};

#endif

#ifndef SETUP_ENERGYOPTIONSMENU_H
#define SETUP_ENERGYOPTIONSMENU_H


#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/config.h>

#include "vdrsetupclasses.h"
#include <vdr/sysconfig_vdr.h>

//-----EnergyOptionsMenu---------------------------

class cEnergyOptionsMenu : public cMenuSetupPage
{
  private:
    int energySpareMode;
    int timeTillShutdown;
    const char *ShutDownModes[3];
    cSysConfig_vdr &sysconfig;
    void Set();

  protected:
    cSetup data;
    /*override*/ void Store();

  public:
    cEnergyOptionsMenu(std::string title = "");
    ~cEnergyOptionsMenu(){};

    /*override*/ eOSState ProcessKey(eKeys key);
};

#endif

