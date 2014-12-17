/*
 * timeline.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: timeline.c,v 1.17 2006/06/18 11:55:13 schmitzj Exp $
 *
 */

#include <vdr/plugin.h>
#include <vdr/device.h>
#include "checkerOsd.h"
#include "config.h"
#include "i18n.h"

static const char *VERSION        = "1.0.141";
static const char *DESCRIPTION    = trNOOP("Show timer overview and collisions");
static const char *MAINMENUENTRY  = trNOOP("Timer Timeline");

class cPluginTimeline : public cPlugin {
private:

public:
  cPluginTimeline(void);
  virtual ~cPluginTimeline();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; }
  virtual bool Service(const char *Id, void *Data);

  bool hasConflicts(void);
};

cPluginTimeline::cPluginTimeline(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginTimeline::~cPluginTimeline()
{
  // Clean up after yourself!
}

bool cPluginTimeline::Service(const char *Id, void *Data)
{
	if (strcmp(Id,"CheckTimerConflict-v1.0") == 0)
	{
		if (Data)
		{
			bool *check = (bool *)Data;
			*check = Timers.Count() ? hasConflicts() : false;
		}

		return true;
	}
	return false;
}

const char *cPluginTimeline::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginTimeline::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  if (argc==1 && argv!=NULL && !strcmp(argv[0],"timeline_command_interface") && !strcmp(argv[1],"conflicts"))
  { // yes, this is an ugly hack!
  	return hasConflicts();
  }
  return true;
}

bool cPluginTimeline::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
	RegisterI18n(tlPhrases);
  	return true;
}

bool cPluginTimeline::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginTimeline::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginTimeline::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.

	return new checkerOsd(this);
}

cMenuSetupPage *cPluginTimeline::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new timelineConfigPage();
}

bool cPluginTimeline::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
	return timelineCfg.SetupParse(Name,Value);
}

bool cPluginTimeline::hasConflicts(void)
{
	return checkerOsd::hasConflicts();
}

VDRPLUGINCREATOR(cPluginTimeline); // Don't touch this!
