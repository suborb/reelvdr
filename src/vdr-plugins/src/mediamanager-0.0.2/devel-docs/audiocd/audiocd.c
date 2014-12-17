/*
 * audiocd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "cdda_menu.h"
#include "cdda_disc.h"
#include "mediaservice.h"
#include "i18n.h"

#define DEBUG_I 1
#include "debug.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Example using mediamanager service";
static const char *MAINMENUENTRY  = "AudioCD";

class cPluginAudiocd : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cMediaService *mediaservice;
  cMediaCddaDisc *disc;
public:
  cPluginAudiocd(void);
  virtual ~cPluginAudiocd();
  virtual const char *Version(void);
  virtual const char *Description(void);
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginAudiocd::cPluginAudiocd(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  mediaservice = NULL;
  disc = NULL;
}

cPluginAudiocd::~cPluginAudiocd()
{
  // Clean up after yourself!
  delete(mediaservice);
  delete(disc);
}

const char *cPluginAudiocd::Version(void)
{
 return VERSION;
}

const char *cPluginAudiocd::Description(void)
{
 return tr(DESCRIPTION);
}

const char *cPluginAudiocd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginAudiocd::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginAudiocd::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  RegisterI18n(Phrases);
  return true;
}

bool cPluginAudiocd::Start(void)
{
  // Start any background activities the plugin shall perform.
	mediaservice = new cMediaService(Name());
	cPlugin *manager = cPluginManager::GetPlugin(MEDIA_MANAGER_PLUGIN);
	if(manager) {
		MediaManager_Register_v1_0 reg;
		reg.PluginName = Name();
		reg.MainMenuEntry = tr(MAINMENUENTRY);
		reg.Description = tr(DESCRIPTION);
		reg.mediatype = mtAudio;
		reg.shouldmount = false;
		if(manager->Service(MEDIA_MANAGER_REGISTER_ID, &reg)) {
			mediaservice->SetManager(manager);
DBG_I("[cPluginAudiocd]: Successful registered")
			return true;
		}
	}
DBG_I("[cPluginAudiocd]: This plugin requires mediamanager")
  return false;
}

void cPluginAudiocd::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginAudiocd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginAudiocd::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginAudiocd::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginAudiocd::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginAudiocd::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
	if(Data == NULL)
		return false;

	if(strcmp(Id, MEDIA_MANAGER_ACTIVATE_ID) == 0) {
		struct MediaManager_Activate_v1_0 *act = (MediaManager_Activate_v1_0*)Data;
		if(act->on) {
			disc = new cMediaCddaDisc(act->device_file);
			if(disc->Open() == 0) {
				cMediaService::SetActive(true);
				return true;
			}
		} else {
			cMediaService::SetActive(false);
			delete(disc);
			disc = NULL;
			return true;
		}
	} else if(strcmp(Id, MEDIA_MANAGER_MAINMENU_ID) == 0) {
		if(cMediaService::IsActive()) {
			MediaManager_Mainmenu_v1_0 *m = (MediaManager_Mainmenu_v1_0*)Data;
			m->mainmenu = new cMediaCddaMenu(disc);
		}
		return true;
	}
  return false;
}

const char **cPluginAudiocd::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginAudiocd::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginAudiocd); // Don't touch this!
