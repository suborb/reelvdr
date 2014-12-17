/*
 * reelblog.c:    The reelblog plugin (based on RSS Reader plugin Version 1.0.2 for the VDR)  provides a simple OSD menu based user interface
 *                        for reading special reelblog feeds.
 *
 * Copyright (C) 2008 Heiko Westphal
*/

#include <getopt.h>
#include <vdr/config.h>
#include <vdr/plugin.h>

#include "menu.h"
#include "common.h"

#if defined(APIVERSNUM) && APIVERSNUM < 10400
#error "VDR-1.4.0 API version or greater is required!"
#endif

static const char VERSION[]       = "0.0.1";
static const char DESCRIPTION[]   = trNOOP("Show the new feeds around the Reelbox");
static const char MAINMENUENTRY[] = trNOOP("Reelblog");
static const char MENUSETUPPLUGINENTRY[] = trNOOP("Reelblog");

class cPluginReelblogReader : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginReelblogReader(void);
  virtual ~cPluginReelblogReader();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void) {}
  virtual cString Active(void) { return NULL; }
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual const char *MenuSetupPluginEntry() { return tr(MENUSETUPPLUGINENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool HasSetupOptions(void) { return false; }; // hidden setup menu entry
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

cPluginReelblogReader::cPluginReelblogReader(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginReelblogReader::~cPluginReelblogReader()
{
  // Clean up after yourself!
}

const char *cPluginReelblogReader::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginReelblogReader::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginReelblogReader::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginReelblogReader::Start(void)
{
  // Start any background activities the plugin shall perform.
  if (!ReelblogItems.Load(AddDirectory(ConfigDirectory(), "reelblog.conf")))
    Skins.Message(mtInfo, tr("ConfigFile reelblog.conf not found!"));
  return true;
}

void cPluginReelblogReader::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginReelblogReader::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginReelblogReader::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  Skins.Message(mtInfo, tr("Loading Reelblog items ..."));
  return new cReelblogItemsMenu();
}

cMenuSetupPage *cPluginReelblogReader::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginReelblogReader::SetupParse(const char *Name, const char *Value)
{
  return false;
}

bool cPluginReelblogReader::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginReelblogReader::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginReelblogReader::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginReelblogReader); // Don't touch this!
