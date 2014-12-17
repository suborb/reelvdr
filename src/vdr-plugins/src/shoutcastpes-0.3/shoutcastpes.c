/*
 * shoutcastpes.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "control.h"

static const char *VERSION        = "0.0.3";
static const char *DESCRIPTION    = "Enter description for 'shoutcastpes' plugin";
static const char *MAINMENUENTRY  = "Shoutcastpes";

class cPluginShoutcastpes : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginShoutcastpes(void);
  virtual ~cPluginShoutcastpes();
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

cPluginShoutcastpes::cPluginShoutcastpes(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginShoutcastpes::~cPluginShoutcastpes()
{
  // Clean up after yourself!
}

const char *cPluginShoutcastpes::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginShoutcastpes::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginShoutcastpes::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginShoutcastpes::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginShoutcastpes::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginShoutcastpes::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginShoutcastpes::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginShoutcastpes::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginShoutcastpes::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.

  mrl = "/home/reel/dump.mp3";
  //Launch new control
  cControl::Launch(new cMyControl);
  return new cOsdMenu("Shoutcast PES");
}

cMenuSetupPage *cPluginShoutcastpes::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginShoutcastpes::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginShoutcastpes::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginShoutcastpes::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    static const char *HelpPages[] = {
        "PLAY <filename>\n"
            "    Triggers playback of 'filename'.",
        NULL
    };
    return HelpPages;
}

cString cPluginShoutcastpes::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    if(strcasecmp(Command, "PLAY")==0)
    {
        if(!Option)
        {
            ReplyCode = 550;
            return "missing option: path to file";
        }

        cControl::Shutdown();

        mrl = Option;
        cControl::Launch(new cMyControl);

        return cString::sprintf("Started playing '%s'", Option);
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginShoutcastpes); // Don't touch this!
