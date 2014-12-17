/*
 * amarokstarter.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "amarokstarter.h"
#include "amarokstarterosd.h"

static const char *MAINMENUENTRY = trNOOP("Amarok Music Player");

cPluginAmarokStarter::cPluginAmarokStarter(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginAmarokStarter::~cPluginAmarokStarter()
{
    // Clean up after yourself!
}

const char *cPluginAmarokStarter::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginAmarokStarter::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginAmarokStarter::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginAmarokStarter::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}

void cPluginAmarokStarter::Stop(void)
{
    // Stop any background activities the plugin shall perform.
}

void cPluginAmarokStarter::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginAmarokStarter::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginAmarokStarter::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

cOsdObject *cPluginAmarokStarter::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    return new cAmarokStarterOsd;
}

cMenuSetupPage *cPluginAmarokStarter::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return NULL;
}

const char *cPluginAmarokStarter::MenuSetupPluginEntry(void)
{
    return NULL;
}

const char *cPluginAmarokStarter::MainMenuEntry(void)
{
    return tr(MAINMENUENTRY);
}

bool cPluginAmarokStarter::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return false;
}

bool cPluginAmarokStarter::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginAmarokStarter::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    return NULL;
}

cString cPluginAmarokStarter::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    return NULL;
}

VDRPLUGINCREATOR(cPluginAmarokStarter); // Don't touch this!
