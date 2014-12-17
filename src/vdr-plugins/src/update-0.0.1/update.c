/*
 * mcrip.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/interface.h>
#include "setup.h"
#include "update.h"
#include "i18n.h"

cPluginUpdate::cPluginUpdate(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginUpdate::~cPluginUpdate()
{
  // Clean up after yourself!
}


const char *cPluginUpdate::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginUpdate::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginUpdate::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
} 


bool cPluginUpdate::Start(void)
{
  // Start any background activities the plugin shall perform. 
  RegisterI18n(Phrases);
  return true;
}


void cPluginUpdate::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}


const char *cPluginUpdate::MainMenuEntry(void)
{
  return UpdateSetup.Update_hidden ? 0 : tr(MAINMENUENTRY);
}

cOsdObject *cPluginUpdate::MainMenuAction(void)
{
  return new cUpdateOsd;
}  


#if APIVERSNUM >= 10331
const char **cPluginUpdate::SVDRPHelpPages(void)
{
  static const char *HelpPages[] = {
    NULL
    };
  return HelpPages;    
}

cString cPluginUpdate::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  return NULL;
}
#endif


cMenuSetupPage *cPluginUpdate::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuUpdateSetup;
} 

bool cPluginUpdate::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(Name, "Update_hidden"))              UpdateSetup.Update_hidden     = atoi(Value);
  else if (!strcasecmp(Name, "Update_noquiet"))           UpdateSetup.Update_halt       = atoi(Value);

  else return false;
    return true;
}

cMenuUpdateSetup::cMenuUpdateSetup(void)
{

  Setup();
} 

void cMenuUpdateSetup::Setup(void)
{
  int current = Current();

  Clear();

  Add(new cMenuEditBoolItem( tr("Verbose output"),                &UpdateSetup.Update_noquiet));

  SetCurrent(Get(current));
  Display();

}


void cMenuUpdateSetup::Store(void)
{
  SetupStore("Update_hidden",        UpdateSetup.Update_hidden);
  SetupStore("Update_noquiet",       UpdateSetup.Update_noquiet);
} 


eOSState cMenuUpdateSetup::ProcessKey(eKeys Key)
{
  int oldvalue = UpdateSetup.Update_remote;
  
  eOSState state = cMenuSetupPage::ProcessKey(Key);

  if( (Key != kNone) && (UpdateSetup.Update_remote != oldvalue) ) {
    Setup();
  }
      
  return state;
}      



VDRPLUGINCREATOR(cPluginUpdate); // Don't touch this!
