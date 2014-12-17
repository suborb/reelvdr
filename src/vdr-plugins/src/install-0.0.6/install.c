/*
 * install.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "installmenu.h"
#include "install.h"

int startWithStep;
int _skipChannelscan;
int wasMainModeSaved;
int savedMainMode;

#if VDRVERSNUM >= 10716
bool loopMode=false;
#endif

static const char *VERSION = "0.0.6";
static const char *DESCRIPTION = trNOOP("The ReelBox installation wizard");

class cPluginInstall:public cPlugin
{
  private:
    // Add any member variables or functions you may need here.
  public:
    cPluginInstall(void);
      virtual ~ cPluginInstall();
    virtual const char *Version(void)
    {
        return VERSION;
    }
    virtual const char *Description(void)
    {
        return DESCRIPTION;
    }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual const char *MainMenuEntry(void)
    {
        return tr(MAINMENUENTRY);
    }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
    virtual bool HasSetupOptions(void)
    {
        return false;
    }
};

cPluginInstall::cPluginInstall(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    startWithStep = 0;
    wasMainModeSaved = 0;
    savedMainMode = 0;
}

cPluginInstall::~cPluginInstall()
{
    // Clean up after yourself!
}

const char *cPluginInstall::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginInstall::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginInstall::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginInstall::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}

void cPluginInstall::Stop(void)
{
    // Stop any background activities the plugin shall perform.
}

void cPluginInstall::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginInstall::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    return new cInstallMenu(startWithStep, (bool) _skipChannelscan);
}

cMenuSetupPage *cPluginInstall::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return NULL;
}

bool cPluginInstall::SetupParse(const char *Name, const char *Value)
{
    if (!strcmp(Name, "stepdone"))
    {
        startWithStep = atoi(Value);
        // atoi(Value) step we were in when the last installation was aborted/exited
        return true;
    }
    else if (!strcmp(Name, "skipchannelscan"))
    {
        _skipChannelscan = atoi(Value);
        // atoi(Value) step we were in when the last installation was aborted/exited
        return true;
    }
    else if (!strcmp(Name, "wasMainModeSaved"))
    {
        wasMainModeSaved = atoi(Value);
        return true;
    }
    else if (!strcmp(Name, "savedMainMode"))
    {
        savedMainMode = atoi(Value);
        return true;
    }
    return false;
}

bool cPluginInstall::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    if (Id) {
        if (strcmp(Id, "NetCeiver changed")==0) {
            int manual = 0;
            cPluginManager::CallAllServices("Was Tuner assignment manual?", &manual);
            
            if (manual) {
                isyslog("Not changing Tuner assignment. Tuner allocation manual.");
                return true;
            }
            
            isyslog("Auto allocating tuners for reelboxes. Tuner allocation was not manual");
            
            /* Save the tuner count to default/mcli and to limit the
               number of tuners for server given the max number of clients
            */
            
            int maxClients = Setup.MaxMultiRoomClients;
            Service("Allocate tuners for MultiRoom Server", &maxClients);
            
            return true;
        } // NetCeiver changed
        else if (strcmp(Id, "Allocate tuners for MultiRoom Server")==0) {
            int *maxClients = (int*) Data;
            
            if (maxClients && Setup.ReelboxModeTemp == eModeServer) 
                AllocateTunersForServer(*maxClients);
            
            return true;
        } // Allocate tuners for MultiRoom Server
    }

    return false;
}

const char **cPluginInstall::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    return NULL;
}

cString cPluginInstall::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    return NULL;
}

VDRPLUGINCREATOR(cPluginInstall);   // Don't touch this!
