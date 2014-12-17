/*
 * shoutcast.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "menu.h"
#include "radioStations.h"
#include "setup.h"

static const char *VERSION        = "0.0.4";
static const char *DESCRIPTION    = "Downloads playlist from 'shoutcast' server";
#if APIVERSNUM < 10700
static const char *SETUPMENUENTRY = trNOOP("Internet Radio Setup");
#endif
static const char *MAINMENUENTRY  = trNOOP("Internet Radio");

int selectedGenreNr=0;

// last entered Server URL and Name by the User
char lastServerUrl[256];
char lastServerName[128];

// holds shoutcast key and shoutcastPageSkip
cInternetRadioSetupParams internetRadioParams;

class cPluginShoutcast : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginShoutcast(void);
  virtual ~cPluginShoutcast();
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
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
#if APIVERSNUM < 10700
  virtual const char *MenuSetupPluginEntry(void) { return tr(SETUPMENUENTRY); }
#endif
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  // virtual bool HasSetupOptions(void) { return true; } // show setup entry
  virtual bool HasSetupOptions(void) { return false; } // hide setup entry
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginShoutcast::cPluginShoutcast(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!

  strncpy( lastServerUrl,  "http://", 255);
  strncpy( lastServerName, tr("New Station"), 127); 

  internetRadioParams.shoutcastKey[0] = '\0';
  internetRadioParams.shoutcastKeyEnabled = 1;
}

cPluginShoutcast::~cPluginShoutcast()
{
  // Clean up after yourself!
}

const char *cPluginShoutcast::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginShoutcast::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginShoutcast::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginShoutcast::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginShoutcast::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginShoutcast::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginShoutcast::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginShoutcast::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginShoutcast::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cShoutcastMenu;
}

cMenuSetupPage *cPluginShoutcast::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cInternetRadioSetupPage(internetRadioParams);
}

bool cPluginShoutcast::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
    if ( !Name )
        return false;

    if ( strcmp(Name, "lastServerUrl") == 0)
        strncpy( lastServerUrl, Value, 255);
    else if ( strcmp(Name, "lastServerName") == 0)
        strncpy( lastServerName, Value, 127);
    else if (strcmp(Name, "shoutcastkey") == 0)
        strncpy(internetRadioParams.shoutcastKey, Value, 17);
    else if (strcmp(Name, "shoutcastKeyEnabled") == 0)
        internetRadioParams.shoutcastKeyEnabled = atoi(Value);
    else
        return false;

    return true;
}

bool cPluginShoutcast::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if (Id && strcmp(Id, "Add New Radio Station")==0)
  {
    if (!Data) return true;

    struct server_info{
    string url;
    string name;
    };
    struct server_info *info = (server_info *) Data;
    if (info->url.length()<=0) return true; 

    //printf("###info->url = %s\n", info->url.c_str()); 
    //printf("###info->name = %s\n", info->name.c_str());

    if (info->name.length()<=0) // station name not given, use url as name
    {
        cRadioStation::Instance()->AddStation(info->url, info->url); 
        if (!cRadioStation::Instance()->Save())
            Skins.Message(mtInfo, tr("Could not save to disk"));
        else
            Skins.Message(mtInfo, tr("Radio station added to favorites"));
    }
    else
    {
        cRadioStation::Instance()->AddStation(info->name, info->url);
        if (!cRadioStation::Instance()->Save())
            Skins.Message(mtInfo, tr("Could not save to disk"));
        else
            Skins.Message(mtInfo, tr("Radio station added to favorites"));
    }

    return true;
  }
  return false;
}

const char **cPluginShoutcast::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
    static const char *HelpPages[] = {
        "showMESG msg\n"
        "    Displays message on OSD using Skins.Message()",
        NULL
    };
    return HelpPages;
}

cString cPluginShoutcast::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    //printf("(%s:%d) '%s' '%s'\n", __FILE__,__LINE__, Command, Option);
  // Process SVDRP commands this plugin implements
  if ( !Command ) return NULL;
  if (!strcasecmp(Command, "showMESG") )
  {
    if (!Option || strlen(Option)==0 )
        return "No mesg given. Nothing not displayed.";

   cShoutcastMenu::ShowMessage(tr(Option));
    return cString::sprintf("'%s' displayed", Option);
  }
  return NULL;
}

VDRPLUGINCREATOR(cPluginShoutcast); // Don't touch this!
