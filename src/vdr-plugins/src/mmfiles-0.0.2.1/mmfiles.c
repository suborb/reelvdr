/*
 * mmfiles.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/remote.h>
#include "filesMenu.h"
#include "thread.h"
#include "mmfiles.h"
#include "search.h"
#include "DefaultParams.h"
#include "Enums.h"

#include "setup.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Enter description for 'mmfiles' plugin";
static const char *MAINMENUENTRY  = "Mmfiles";

 
cScanDirThread *scandirthread;

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
  /*cDirList::Instance().AddPath("/media/hd/recordings"); 
  cDirList::Instance().AddPath("/media/hd/video"); 
  cDirList::Instance().AddPath("/media/hd/music"); 
  cDirList::Instance().AddPath("/media/hd/pictures"); 
  cDirList::Instance().AddPath("/home/reel/av"); 
  */
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
  // DEFAULT PATHS to be scanned
    if (cDirList::Instance().ListSize()<=0)
    {
        cDirList::Instance().AddPath("/media/hd/music");
        cDirList::Instance().AddPath("/media/hd/video");
        cDirList::Instance().AddPath("/media/hd/recordings");
        cDirList::Instance().AddPath("/media/hd/pictures");
    }
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
  return new cMMfilesSetup;
  return NULL;
}

bool cPluginMmfiles::SetupParse(const char *Name, const char *Value)
{
    static bool cleared=false;
  // Parse your own setup parameters and store their values.
  if (Name && strncasecmp(Name, "ScanDirTree",strlen("ScanDirTree"))==0)
  {
    if(!cleared) {cDirList::Instance().ClearList(); cleared = true; }
  	cDirList::Instance().AddPath(Value);
  	return true;
  	}
  	
  return false;
}

bool cPluginMmfiles::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if (Id && strcmp(Id,"Recording List Cleared")==0)
  {
    // update all VDR recordings once again
      printf("\nSERVICE CALL \t %s\n", Id);
      return AddRecToVdr();
  }
  else if (Id && strcmp(Id, "Show All Audio Files")==0)
  {
    cDefaultParams::Instance().SetFileType(1); // See cSearchCache::doesMatchSearchParam() for numbers
    cDefaultParams::Instance().SetMenuTitle(tr("Audio"));
    cDefaultParams::Instance().SetBrowseMode(eGenreMode);

    cSearchCache::Instance().ResetSearchResult();
    //cSearchCache::Instance().SetFileType(1);
    cSearchCache::Instance().RunSearch();
    cFilesMenu::ResetBrowseMode();
    cRemote::CallPlugin("mmfiles");
  }
  else if (Id && strcmp(Id, "Show All Video Files")==0)
  {
    cDefaultParams::Instance().SetFileType(2); // See cSearchCache::doesMatchSearchParam() for numbers
    cDefaultParams::Instance().SetMenuTitle(tr("Video"));
    cDefaultParams::Instance().SetBrowseMode(eNormalMode);

    cSearchCache::Instance().ResetSearchResult();
    //cSearchCache::Instance().SetFileType(2);
    cSearchCache::Instance().RunSearch();
    cFilesMenu::ResetBrowseMode();
    cRemote::CallPlugin("mmfiles");
  }
  else if (Id && strcmp(Id, "Show All Photo Files")==0)
  {
    cDefaultParams::Instance().SetFileType(3); // See cSearchCache::doesMatchSearchParam() for numbers
    cDefaultParams::Instance().SetMenuTitle(tr("Photo"));
    cDefaultParams::Instance().SetBrowseMode(eNormalMode);

    cSearchCache::Instance().ResetSearchResult();
    //cSearchCache::Instance().SetFileType(3);
    cSearchCache::Instance().RunSearch();
    cFilesMenu::ResetBrowseMode();
    cRemote::CallPlugin("mmfiles");
  }
  else if (Id && strcmp(Id, "mmfiles set caller")==0)
  {
    
    return true;
  }
  return false;
}

const char **cPluginMmfiles::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
    static const char *HelpPages[] =
    {
        "EXIT\n"
        "    Kills all scan threads.",
        "SEARCH\n"
        "    Searches for a string.",
        NULL
    };
    return HelpPages;

}

cString cPluginMmfiles::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
    if (Command && strcasecmp(Command,"EXIT")==0)
    {
        return "(dry run)Exiting";
    }

    if (Command && strcasecmp(Command,"SEARCH")==0)
    {
        cSearchCache::Instance().SetSearchString(Option);
        cSearchCache::Instance().RunSearch();

        return "Searching";
    }

  return NULL;
}


VDRPLUGINCREATOR(cPluginMmfiles); // Don't touch this!



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//

// private
cDirList::cDirList()
{
    changed_ = false;
}

cDirList::cDirList(const cDirList& op1)
{
    *this = op1;
}

cDirList& cDirList::operator=(const cDirList& op)
{
    PathsToScanFrom = op.PathsToScanFrom;
    changed_ = true;

    return *this;
}

// public
cDirList& cDirList::Instance()
{
    static cDirList loneInstance; // the only instance of cDirList

    return loneInstance;
}


bool cDirList::AddPath(const std::string& new_path)
{
    PathsToScanFrom.push_back(new_path);
    changed_ = true;

    return true;
}




bool cDirList::RemovePath(const std::string& old_path)
{
    unsigned int idx = FindInList(old_path);

    if ( idx >= 0 && idx < ListSize() ) 
    {
        PathsToScanFrom.erase( PathsToScanFrom.begin() + idx );
        changed_ = true;

        return true;
    }

    return false;
}


unsigned int cDirList::FindInList(const std::string & path)
{
    unsigned int i = 0;

    for( i = 0; i < ListSize();  ++i)
        if (PathsToScanFrom.at(i) == path) break;

    return i;
}



bool cDirList::HasChanged() const 
{ 
    return changed_; 
}



void cDirList::ResetChanged()     
{ 
    changed_ = false; 
}



unsigned int cDirList::ListSize()const                        
{ 
    return PathsToScanFrom.size(); 
}




std::string cDirList::PathAt( unsigned int& i)const           
{ 
    return i < ListSize()? PathsToScanFrom.at(i):""; 
}


