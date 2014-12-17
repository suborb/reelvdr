/*
 * director.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "arghdirector.h"
#include "directorosd.h"

#include "menudirector.h"
#include "osdproxy.h"

// --- cMenuEditStraItemTr -----------------------------------------------------

cMenuEditStraItemTr::cMenuEditStraItemTr(const char *Name, int *Value, int NumStrings, const char * const *Strings)
:cMenuEditIntItem(Name, Value, 0, NumStrings - 1)
{
  strings = Strings;
  Set();
}

void cMenuEditStraItemTr::Set(void)
{
  SetValue(tr(strings[*value]));
}



cDirectorStatus::cDirectorStatus(cPlugin* plugin)
{
	parent = plugin;
}
	
void cDirectorStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  const cChannel* Channel = Channels.GetByNumber(Device->CurrentChannel());
	if(Channel != NULL && Channel->LinkChannels() != NULL && Channel->LinkChannels()->Count() > 1)
		if(parent && ChannelNumber != 0)
		{
			//dont know why Channelnumber != 0 is necessary, but it is
			cRemote::CallPlugin(parent->Name());
			//dsyslog("cDirectorStatus::ChannelSwitch  Channelnumber: %d, Links: %d", ChannelNumber, Channel->LinkChannels()->Count());
		}
}

cPluginDirector::cPluginDirector(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  directorStatus = NULL;
}

cPluginDirector::~cPluginDirector()
{
  // Clean up after yourself!
  delete directorStatus;
}

const char *cPluginDirector::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginDirector::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginDirector::Start(void)
{
  // Start any background activities the plugin shall perform.
  if(autostart == 1)
  	directorStatus = new cDirectorStatus(this);
  // Default values for setup
  return true;
}

void cPluginDirector::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginDirector::MainMenuAction(void)
{
  if (showOldDirectorOsd == 1)
    return new cDirectorOsd(portalmode, swapKeys, displayChannelInfo, autostart, themelike);

  cMenuDirectorData* config = new cMenuDirectorData();
  config->mHidePluginEntryInMainMenu = hide;
  config->mPortalmode = portalmode;
  config->mSwapKeys = swapKeys;
  config->mSwapKeysInMenu = swapKeysInMenu;
  config->mDisplayChannelInfo = displayChannelInfo;
  config->mSwitchChannelOnItemSelect = switchChannelOnItemSelect;
  config->mHidePluginAfterChannelSwitch = hidePluginAfterChannelSwitch;
  config->mDisplayShortcutNumbers = displayShortcutNumbers;
  config->mDisplayEpgInfo = displayEpgInfo;
  config->mHideButton = allowedButtons[showPluginButton];
//config->mAutoStart = autostart;

  cOsdObject* prxy = new cOsdProxy(cMenuDirector::Create,cMenuDirector::KeyHandlerFunction, 
                       cMenuDirector::Release, tr("Multifeed Options"),
                       config, allowedButtons[showPluginButton], true);
  return prxy;
}

bool cPluginDirector::SetupParse(const char *Name, const char *Value)
{
	// Parse your own setup parameters and store their values.
	if (!strcasecmp(Name, "HideMenu"))
		hide = atoi(Value);
	else if (!strcasecmp(Name, "SwapKeys"))
		swapKeys = atoi(Value);
	else if (!strcasecmp(Name, "SwapKeysInMenu"))
		swapKeysInMenu = atoi(Value);
	else if (!strcasecmp(Name, "PortalMode"))
		portalmode = atoi(Value);
	else if (!strcasecmp(Name, "DisplayChannelInfo"))
		displayChannelInfo = atoi(Value);
	else if (!strcasecmp(Name, "SwitchChannelOnItemSelect"))
		switchChannelOnItemSelect = atoi(Value);
	else if (!strcasecmp(Name, "HidePluginAfterChannelSwitch"))
		hidePluginAfterChannelSwitch = atoi(Value);
	else if (!strcasecmp(Name, "DisplayShortcutNumbers"))
		displayShortcutNumbers= atoi(Value);
	else if (!strcasecmp(Name, "DisplayEpgInfo"))
		displayEpgInfo= atoi(Value);
	else if (!strcasecmp(Name, "ShowOldDirectorOsd"))
		showOldDirectorOsd= atoi(Value);
	else if (!strcasecmp(Name, "ShowPluginButton"))
		showPluginButton = atoi(Value);
//	else if (!strcasecmp(Name, "AutoStart"))
//		autostart = atoi(Value);
	else if (!strcasecmp(Name, "ThemeLike"))
		themelike = atoi(Value);
	return true;
}

//the setup part
class cMenuSetupDirector : public cMenuSetupPage {
 public:
  cMenuSetupDirector();
 protected:
  virtual void Store(void);
 private:
 	int new_hide;
};

cMenuSetupDirector::cMenuSetupDirector()
{
	new_hide = hide;    
//	Add(new cMenuEditBoolItem(tr("Hide main menu entry"), &hide, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Swap up and down in menu"), &swapKeysInMenu, tr("no"), tr("yes")));
	Add(new cMenuEditBoolItem(tr("Portal Mode"), &portalmode, tr("no"), tr("yes")));
	Add(new cMenuEditBoolItem(tr("Display info on channel change"), &displayChannelInfo, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Switch channel on linkchannel selection"), &switchChannelOnItemSelect, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Hide plugin after channel switch"), &hidePluginAfterChannelSwitch, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Display channel shortcut numbers"), &displayShortcutNumbers, tr("no"), tr("yes")));
	Add(new cMenuEditBoolItem(tr("Display EPG information"), &displayEpgInfo, tr("no"), tr("yes")));
	Add(new cMenuEditStraItemTr(tr("Show plugin again on button"), &showPluginButton, NOOFALLOWEDBUTTONS, allowedButtonNames));
	Add(new cMenuEditBoolItem(tr("Swap up and down"), &swapKeys, tr("no"), tr("yes")));
	Add(new cMenuEditBoolItem(tr("Show old director OSD"), &showOldDirectorOsd, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Start the plugin automatically"), &autostart, tr("no"), tr("yes")));
//	Add(new cMenuEditBoolItem(tr("Use colors from theme"), &themelike, tr("no"), tr("yes")));

}

void cMenuSetupDirector::Store(void)
{
	SetupStore("HideMenu", new_hide = hide);
	SetupStore("SwapKeys", swapKeys);
	SetupStore("SwapKeysInMenu", swapKeysInMenu);
	SetupStore("PortalMode", portalmode);
	SetupStore("DisplayChannelInfo", displayChannelInfo);
	SetupStore("SwitchChannelOnItemSelect", switchChannelOnItemSelect);
	SetupStore("DisplayShortcutNumbers", hidePluginAfterChannelSwitch);
	SetupStore("HidePluginAfterChannelSwitch", displayShortcutNumbers);
	SetupStore("DisplayEpgInfo", displayEpgInfo);
	SetupStore("ShowOldDirectorOsd", showOldDirectorOsd);
	SetupStore("ShowPluginButton", showPluginButton);
//	SetupStore("AutoStart", autostart);
	SetupStore("ThemeLike", themelike);
}

cMenuSetupPage *cPluginDirector::SetupMenu(void)
{
	
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupDirector();
}

VDRPLUGINCREATOR(cPluginDirector); // Don't touch this!
