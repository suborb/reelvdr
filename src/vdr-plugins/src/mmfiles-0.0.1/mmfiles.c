/*
 * mmfiles.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "filesMenu.h"
#include "thread.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Enter description for 'mmfiles' plugin";
static const char *MAINMENUENTRY  = "Mmfiles";

static cScanDirThread *scandirthread;

class cPluginMmfiles : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginMmfiles(void);
  virtual ~cPluginMmfiles();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginMmfiles::cPluginMmfiles(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginMmfiles::~cPluginMmfiles()
{
  // Clean up after yourself!
}

const char *cPluginMmfiles::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginMmfiles::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginMmfiles::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginMmfiles::Start(void)
{
  // Start any background activities the plugin shall perform.


  // Start Threads
  // 1. Scan Directories thread
  scandirthread = new cScanDirThread;
  scandirthread->Start();
  // 2. Inotify thread ?
  return true;
}

void cPluginMmfiles::Stop(void)
{
  // Stop any background activities the plugin shall perform.
  // Stop threads
  delete scandirthread;
}

void cPluginMmfiles::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginMmfiles::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginMmfiles::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginMmfiles::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.

  return new cFilesMenu;
  return NULL;
}

cMenuSetupPage *cPluginMmfiles::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginMmfiles::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginMmfiles::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginMmfiles::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginMmfiles::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginMmfiles); // Don't touch this!
