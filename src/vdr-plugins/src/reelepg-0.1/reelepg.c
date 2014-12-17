/*
 * reelepg.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 *
 */
#include <vdr/plugin.h>
#include "timeline.h"
#include "config.h"
#include "service.h"


cChannels *mChannelList;

static const char *VERSION        = "0.2.0";
static const char *DESCRIPTION    = "Shows the EPG info and pictures in form of a timeline or magazine view";
//static const char *MAINMENUENTRY  = "Magazine EPG";
static const char *MAINMENUENTRY  = trNOOP("TV-Guide (ReelEPG)");

class cPluginReelEPG : public cPlugin {
    public:
	cPluginReelEPG(void);
	virtual ~cPluginReelEPG();
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
	virtual bool HasSetupOptions(void){ return false; } 
	virtual bool SetupParse(const char *Name, const char *Value);

	virtual bool Service(const char *Id, void *Data = NULL);
}; // cPluginReelEPG

cPluginReelEPG::cPluginReelEPG(void) {
    DBG(DBG_FN,"cPluginReelEPG::cPluginReelEPG()");
} // cPluginReelEPG::cPluginReelEPG
cPluginReelEPG::~cPluginReelEPG() { // Clean up after yourself!
    DBG(DBG_FN,"cPluginReelEPG::~cPluginReelEPG()");
} // cPluginReelEPG::~cPluginReelEPG
const char *cPluginReelEPG::CommandLineHelp(void) { // Return a string that describes all known command line options.
    DBG(DBG_FN,"cPluginReelEPG::CommandLineHelp()");
    return gReelEPGCfg.CommandLineHelp();
} // cPluginReelEPG::CommandLineHelp
bool cPluginReelEPG::ProcessArgs(int argc, char *argv[]) {
    DBG(DBG_FN,"cPluginReelEPG::ProcessArgs(%d,...)", argc);
    return gReelEPGCfg.ProcessArgs(argc,argv);
} // cPluginReelEPG::ProcessArgs
bool cPluginReelEPG::Initialize(void) { // Initialize any background activities the plugin shall perform.
    DBG(DBG_FN,"cPluginReelEPG::Initialize()");
    return true;
} // cPluginReelEPG::Initialize
bool cPluginReelEPG::Start(void) { // Start any background activities the plugin shall perform.
    DBG(DBG_FN,"cPluginReelEPG::Start()");
    cTimeLine::AddThemeColors();
	cTimeLine::LoadFontsNG();
    cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());

    cTimeLine::mBrowseResultSet = false;  // No browse query at startup 
    cTimeLine::GetMyFavouritesResults();  // Search for favourites with epgsearch , if query is available in setup

    return true;
} // cPluginReelEPG::Start
void cPluginReelEPG::Housekeeping(void) { // Perform any cleanup or other regular tasks.
  DBG(DBG_FN,"cPluginReelEPG::Housekeeping()");
    cTimeLine::GetMyFavouritesResults();
} // cPluginReelEPG::Housekeeping
cOsdObject *cPluginReelEPG::MainMenuAction(void) { // Perform the action when selected from the main VDR menu.
    DBG(DBG_FN,"cPluginReelEPG::MainMenuAction() %s",Skins.Current()->Name());
    if(strcmp("Reel",Skins.Current()->Name())) {
        cPlugin *lPlugin = cPluginManager::GetPlugin("tvonscreen");
        if (lPlugin)
            return lPlugin->MainMenuAction();
    } // if
    return new cTimeLine(this);
} // cPluginReelEPG::MainMenuAction
bool cPluginReelEPG::Service(const char *Id, void *Data)
{
  if (strcmp(Id, "Reelepg-SetQuery-v1.0") == 0) {
     if (Data == NULL)
        return true;
     cTimeLine::SetMyFavouritesQuery((Reelepg_setquery_v1_0 *)Data);
     return true;
     }
  return false;
}
// cPluginReelEPG::Service
cMenuSetupPage *cPluginReelEPG::SetupMenu(void) { // Return a setup menu in case the plugin supports one.
    DBG(DBG_FN,"cPluginReelEPG::SetupMenu()");
    return NULL;
} // cPluginReelEPG::SetupMenu
bool cPluginReelEPG::SetupParse(const char *Name, const char *Value) { // Parse your own setup parameters and store their values.
    DBG(DBG_FN,"cPluginReelEPG::SetupParse(%s,%s)",Name,Value);
    return gReelEPGCfg.SetupParse(Name,Value);
} // cPluginReelEPG::SetupParse

VDRPLUGINCREATOR(cPluginReelEPG); // Don't touch this!
