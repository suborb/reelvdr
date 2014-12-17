/*
 * epgtimers.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/i18n.h>
#include <vdr/plugin.h>

#include "services.h"

static const char VERSION[]        = "0.0.1";
static const char DESCRIPTION[]    = trNOOP("Direct access to epgsearch's timers menu");
static const char MAINMENUENTRY[]  = trNOOP("Timer Manager");

class cPluginEpgtimers : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginEpgtimers(void);
  virtual ~cPluginEpgtimers();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; } 
  };

cPluginEpgtimers::cPluginEpgtimers(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginEpgtimers::~cPluginEpgtimers()
{
  // Clean up after yourself!
}

const char *cPluginEpgtimers::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginEpgtimers::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginEpgtimers::Initialize(void)
{
  return true;
}

bool cPluginEpgtimers::Start(void)
{
  return true;
}

void cPluginEpgtimers::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginEpgtimers::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginEpgtimers::MainMenuAction(void)
{
    cPlugin *p = cPluginManager::GetPlugin("epgsearch");
    if (p)
    {
 	Epgsearch_exttimers_v1_0* serviceData = new Epgsearch_exttimers_v1_0;
	serviceData->pTimersMenu = NULL;
	
	cOsdMenu* pMenu = NULL;
	if (p->Service("Epgsearch-exttimers-v1.0", serviceData))
	    pMenu = serviceData->pTimersMenu;
	else
	    Skins.Message(mtError, tr("This version of EPGSearch does not support this service!"));
	
	delete serviceData;
	
	if (pMenu)
	    return pMenu;	
	else
	    return NULL;
    }
    else
    {
	Skins.Message(mtError, tr("EPGSearch does not exist!"));
	return NULL;
    }

  return NULL;
}

cMenuSetupPage *cPluginEpgtimers::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginEpgtimers::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

VDRPLUGINCREATOR(cPluginEpgtimers); // Don't touch this!
