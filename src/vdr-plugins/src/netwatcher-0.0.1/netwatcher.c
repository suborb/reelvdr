/*
 * netwatcher.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>

#include "netwatcherthread.h"
#include "netwatchermenu.h"

extern bool wasMainMenuHookCalled;
static const char *VERSION = "0.0.1";
static const char *DESCRIPTION = "Watches the connectivity with the netceiver";
static const char *MAINMENUENTRY = "NetWatcher";

class cPluginNetWatcher:public cPlugin
{
  private:
    cNetWatcherThread * watcherThread;
    char devname[16];/** the name of the ethernet device to monitor */
    // Add any member variables or functions you may need here.
  public:
      cPluginNetWatcher(void);
      virtual ~ cPluginNetWatcher();
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
    virtual void MainThreadHook(void);
    virtual cString Active(void);
    virtual const char *MainMenuEntry(void) /*{ return NULL; } */
    {
        return MAINMENUENTRY;
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

cPluginNetWatcher::cPluginNetWatcher(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    watcherThread=NULL;
    memset(devname, 0, sizeof(devname));
}

cPluginNetWatcher::~cPluginNetWatcher()
{
    // Clean up after yourself!
    delete(watcherThread);
}

const char *cPluginNetWatcher::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return ("  --ifname <network interface>\n");
}

bool cPluginNetWatcher::ProcessArgs(int argc, char *argv[])
{
    if (argc == 3 && ((strcmp(argv[1], "--ifname") == 0) || (strcmp(argv[1], "-i") == 0)))
        strncpy((char *)&devname, argv[2], sizeof(devname)-1);
    else
        strncpy((char *)&devname, DEFAULT_DEV, sizeof(devname)-1);
    dsyslog("Netwatcher: using ethernet device \"%s\"", devname);
    return true;
}

bool cPluginNetWatcher::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    watcherThread = new cNetWatcherThread(devname);
    return true;
}

bool cPluginNetWatcher::Start(void)
{
    // Start any background activities the plugin shall perform.
    watcherThread->Start();
    return true;
}

void cPluginNetWatcher::Stop(void)
{
    // Stop any background activities the plugin shall perform.
    //watcherThread->Stop();
}

void cPluginNetWatcher::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginNetWatcher::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
   
   if (!wasMainMenuHookCalled) 
       wasMainMenuHookCalled = true;
}

cString cPluginNetWatcher::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

cOsdObject *cPluginNetWatcher::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    return new cNetWatcherMenu();
}

cMenuSetupPage *cPluginNetWatcher::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return NULL;
}

bool cPluginNetWatcher::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return false;
}

bool cPluginNetWatcher::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginNetWatcher::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    return NULL;
}

cString cPluginNetWatcher::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    return NULL;
}

VDRPLUGINCREATOR(cPluginNetWatcher);    // Don't touch this!
