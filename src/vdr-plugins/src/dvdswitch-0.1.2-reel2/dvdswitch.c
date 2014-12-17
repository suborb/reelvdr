/*
 * dvdswitch.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <getopt.h>
#include <unistd.h>
#include <vdr/plugin.h>
#include "helpers.h"
#include "setup.h"
#include "menu.h"
//#include "dvdplugin.h"
#include "imagelist.h"
//#include "dvdlist.h"

static const char *VERSION        = "0.1.2-reel2";
static const char *DESCRIPTION    = trNOOP("Archives DVDs and plays it from Harddisk");
static const char *MAINMENUENTRY  = trNOOP("Archive Video-DVD");

bool refreshList = true;

class cPluginDvdswitch : public cPlugin {
private:
  cDVDListThread *listThread;
  bool CheckError(void);
public:
  cPluginDvdswitch(void);
  virtual ~cPluginDvdswitch();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual bool HasSetupOptions(void) { return (access("/dev/dvd", R_OK)==0); };
#ifdef PLUGINENTRY
  virtual const char *MenuSetupPluginEntry(void) {
      return tr(SETUPMENUENTRY);
  }
#endif
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void); // { return tr(MAINMENUENTRY); }
//  virtual const char *MainMenuEntry(void) { return DVDSwitchSetup.MenuName; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginDvdswitch::cPluginDvdswitch(void)
{
    listThread = NULL;
}

cPluginDvdswitch::~cPluginDvdswitch()
{
  // Clean up after yourself!
  //cDVDPlugin::Finish();
  DebugLog.End();
}

const char *cPluginDvdswitch::CommandLineHelp(void)
{
     //12345678901234567890123456789012345678901234567890123456789012345678901234567890|
  return
      "  -D LOGFILE,    --debug=LOGFILE       write Debug-Info's in LOGFILE\n"
      "  -r SCRIPT,     --readscript=SCRIPT   SCRIPT = scriptname with path for reading\n"
      "                                       DVD as a ISO image file\n"
      "  -w SCRIPT,     --writescript=SCRIPT  SCRIPT = scriptname with path to writing\n"
      "                                       selected DVD image\n"
      "  -i PATH,       --imagedir=PATH       Path to DVD-Images. This option can be set\n"
      "                                       in setup-menu (optional)\n";
}

bool cPluginDvdswitch::ProcessArgs(int argc, char *argv[])
{
  static struct option long_options[] = {
    { "debug",        required_argument, NULL, 'D' },
    { "readscript",   required_argument, NULL, 'r' },
    { "writescript",  required_argument, NULL, 'w' },
    { "imagedir",     required_argument, NULL, 'i' },
    { NULL }
  };

  int c = 0;
  optind = 1; //default for getopt

  while((c = getopt_long(argc, argv, "D:r:w:i:", long_options, NULL)) != -1)
  {
    switch(c)
    {
      case 'D':
        DebugLog.SetLogFile(optarg);
        break;
      case 'r':
        strn0cpy(DVDSwitchSetup.DVDReadScript, optarg, MaxFileName);
        break;
      case 'w':
        strn0cpy(DVDSwitchSetup.DVDWriteScript, optarg, MaxFileName);
        break;
      case 'i':
        strn0cpy(DVDSwitchSetup.ImageDir, optarg, MaxFileName);
        DVDSwitchSetup.ImageDirPerParam = true;
        break;
      default:
        esyslog("DVDSwitch: unknown Parameter: %c", c);
        break;
    }
  }

  return true;
}

bool cPluginDvdswitch::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  MYDEBUG("Plugin wird initialisiert");

  DVDSwitchSetup.SetConfDir((char*)ConfigDirectory(Name()));
  ImageList.Init();
  DVDSwitchSetup.Init();
  listThread = new cDVDListThread();

  return true;
}

bool cPluginDvdswitch::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginDvdswitch::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginDvdswitch::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

const char *cPluginDvdswitch::MainMenuEntry(void)
{
    if (access("/dev/dvd", R_OK)==0) // file exists
        return tr(MAINMENUENTRY);
    else
    {
        isyslog("[" PLUGIN_NAME "]" " - no DVD drive available - hiding main menu entry");
        return NULL;
    }
}

cOsdObject *cPluginDvdswitch::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  MYDEBUG("MainMenuAction");
  if (CheckError())
  {
    MYDEBUG("Fehler entdeckt. Keine OSDOBJECT Rueckgabe");
    return NULL;
  }
  else
    return new cMainMenu(listThread);
}

cMenuSetupPage *cPluginDvdswitch::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupDVDSwitch();
}

bool cPluginDvdswitch::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return DVDSwitchSetup.SetupParse(Name, Value);
}

bool cPluginDvdswitch::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if (strcmp(Id, "Dvdswitch perform action") == 0)
  {
      //printf("----------Dvdswitch perform action----------\n");
      struct dvdSwitchAction
      {
        char const *cmd;
      };

      if(strcmp((static_cast<dvdSwitchAction*>(Data))->cmd, "copy") == 0)
      {
          cRemote::CallPlugin("dvdswitch");
          cRemote::Put(kRed);
          cRemote::Put(kOk); // for Copy now question!!
      }
      return true;
  }
  return false;
}

const char **cPluginDvdswitch::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginDvdswitch::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

bool cPluginDvdswitch::CheckError(void)
{
  MYDEBUG("Fehlercheck");
  MYDEBUG("Check ImageDir");
  if(!DirectoryOk(DVDSwitchSetup.ImageDir))
  {
    esyslog("ImageDir '%s' not readable or does not exist", DVDSwitchSetup.ImageDir);
    OSD_ERRMSG(tr("Image Directory not readable or does not exist"));
    return true;
  }

  MYDEBUG("Check DVD Plugin");

  if(!cPluginManager::GetPlugin("xinemediaplayer"))
  {
    esyslog("DVDPlugin is not available!");
    OSD_INFOMSG(tr("DVD-Plugin not found! Function deactivated!"));
  } else
    DVDSwitchSetup.HaveXinePlugin = true;

  return false;
}

VDRPLUGINCREATOR(cPluginDvdswitch); // Don't touch this!
