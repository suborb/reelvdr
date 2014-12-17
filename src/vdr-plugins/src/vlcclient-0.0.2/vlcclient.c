/*
 * vlcclient.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "i18n.h"
#include "setup.h"
#include "menu.h"
#include "vlcclient.h"

cPluginVlcClient::cPluginVlcClient(void)
{
	listThread = NULL;
}

cPluginVlcClient::~cPluginVlcClient()
{
	if (listThread)
		delete listThread;
}

const char *cPluginVlcClient::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginVlcClient::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginVlcClient::Initialize(void)
{
	listThread = new cRecordingsListThread;
  return true;
}

bool cPluginVlcClient::Start(void)
{
  // Start any background activities the plugin shall perform.
  RegisterI18n(myPhrases);
  return true;
}

void cPluginVlcClient::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginVlcClient::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginVlcClient::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cVlcClientMenu(listThread);
}

cMenuSetupPage *cPluginVlcClient::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cVlcClientMenuSetupPage;
}

bool cPluginVlcClient::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return VlcClientSetup.SetupParse(Name, Value);
}

bool cPluginVlcClient::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginVlcClient::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginVlcClient::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginVlcClient); // Don't touch this!
