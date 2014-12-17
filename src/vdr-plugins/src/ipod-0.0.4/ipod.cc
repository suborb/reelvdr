//***************************************************************************
/*
 * ipod.cc: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include "menu.hpp"
#include "ipod.hpp"
#include "setupmenu.hpp"
#include "vdr/remote.h"

//***************************************************************************
// Static Stuff
//***************************************************************************

char cPluginIpod::playlistPath[] = "/media/reel/music";
char cPluginIpod::mountPoint[]   = "/media/reel/music/ipod";
char cPluginIpod::mountScript[]  = "/usr/sbin/mount_ipod.sh";

//***************************************************************************
// Object
//***************************************************************************

cPluginIpod::cPluginIpod(void)
{
   itunes = new ITunes;
   autolist = new PlayList;
   playlist = new PlayList;
   status = 0;

   debug = no;
}

cPluginIpod::~cPluginIpod()
{
   if (itunes)   delete itunes;
   if (playlist) delete playlist;
   if (autolist) delete autolist;
   if (status)   delete status;
}

//***************************************************************************
// Command Line Help
//***************************************************************************

const char *cPluginIpod::CommandLineHelp(void)
{
   return NULL;
}

//***************************************************************************
// Process Arguments
//***************************************************************************

bool cPluginIpod::ProcessArgs(int argc, char *argv[])
{
   while (*argv)
   {
      if (strcmp(*argv, "-d") == 0)
      {
         debug = yes;
         Cs::setTellDevice(dDefault | dConsole);
         tell(eloAlways, "Warning: Debug mode enabled!");
      }

      if (strcmp(*argv, "-e") == 0)
      {
         if (!(*(++argv)))
            break;

         Cs::setEloquence(atoi(*argv));
         tell(eloAlways, "Info: Eloquence set to %d", eloquence);
      }

      argv++;
   }
   
   return true;
}

//***************************************************************************
// Initialize
//***************************************************************************

bool cPluginIpod::Initialize(void)
{
   return true;
}

//***************************************************************************
// Mount
//***************************************************************************

int cPluginIpod::mount()
{
   int ret;
	char* cmd;

   // is mounted ..

   if (isMounted()) 
      return success;

      //try to mount

   asprintf(&cmd, "%s %s %s", mountScript, "mount", mountPoint);
   ret = system(cmd);
   free(cmd);
	
   return WEXITSTATUS(ret) == 1 ? fail : success;
}

//***************************************************************************
// Mount
//***************************************************************************

int cPluginIpod::umount()
{
   int ret;
	char* cmd;

   if (!isMounted())
      return success;

   asprintf(&cmd, "%s %s %s", mountScript, "unmount", mountPoint);
   ret = system(cmd);
   free(cmd);

   return WEXITSTATUS(ret) == 1 ? fail : success;
}

//***************************************************************************
// Is Mounted
//***************************************************************************

int cPluginIpod::isMounted()
{
   int ret;
	char* cmd;

   // is mounted ?

   asprintf(&cmd, "%s status %s", mountScript, mountPoint);
   ret = system(cmd);
   free(cmd);

   return WEXITSTATUS(ret) == 0 ? yes : no;
}

//***************************************************************************
// Start
//***************************************************************************

bool cPluginIpod::Start(void)
{
	status = new PluginStatus;
   Cs::setTellPrefix(MainMenuEntry());

   return true;
}

//***************************************************************************
// Housekeeping
//***************************************************************************

void cPluginIpod::Housekeeping(void)
{
}

//***************************************************************************
// Init
//***************************************************************************

int cPluginIpod::init()
{
   // First Mount the iPod

   if (!debug)
   {
      if (mount() != success)
      {
         tell(eloInfo, "Mount of '%s' -> failed", mountPoint);
         return fail;
      }
      else
         tell(eloStatus, "Mount of '%s' -> success", mountPoint);
   }

   // clear already parsed itunes

   itunes->clear();

   // Parse the iTunes Database ...

   tell(eloDetail, "Parsing itunes database");

   sprintf(iTunesPath, "%s%s", mountPoint, "/iPod_Control/iTunes/iTunesDB");

   if (!debug)
      itunes->setPath(iTunesPath);
   else
      itunes->setPath("/usr/src/VDR/PLUGINS/src/ipod/tests/iTunesDB");

 	if (itunes->parse() != success)
   {
      tell(eloError, "Parsing itunes database -> failed");
      return fail;
   }
   else
      tell(eloDetail, "Parsing itunes database -> success");

   // init playlist settings now
   // playlistPath and/or itunes maybe changed ...

   autolist->setPath(playlistPath, "ipod-auto.m3u");
   autolist->setItunes(itunes);
   playlist->setPath(playlistPath, "ipod.m3u");
   playlist->setItunes(itunes);

   return success;
}

//***************************************************************************
// Main Menu Action
//***************************************************************************

cOsdObject *cPluginIpod::MainMenuAction(void)
{
   if (init() != success)
   {
      tell(eloError, "Error: ...");
      Skins.Message(mtError, tr("Ipod not found."));
      return 0;
   }
      
   // fine
   //  -> initialize the menue

   cPodMenu::init(this);
   cPodMenu* p = new cPodMenu();
   p->fill();

   return p;
}

//***************************************************************************
// Setup Menu
//***************************************************************************

cMenuSetupPage *cPluginIpod::SetupMenu(void)
{
  return new PodSetupMenu;
}

//***************************************************************************
// Setup Parse
//***************************************************************************

bool cPluginIpod::SetupParse(const char *Name, const char *Value)
{
   // Pase Setup Parameters

  if (!strcasecmp(Name, "mountPoint") )
     strcpy(mountPoint, Value);

  else if (!strcasecmp(Name, "mountScript"))
     strcpy(mountScript, Value);

  else if (!strcasecmp(Name, "playlistPath") )
     strcpy(playlistPath, Value);

  else
    return false;

  return true;
}

//***************************************************************************
// Play
//***************************************************************************

int cPluginIpod::play(PlayList* aList, cOsdMenu* podMenu)
{
   //cOsdObject* osd;
   cPlugin* p;
   //const char* last = 0;
   //const char* wanted = I18nTranslate("MP3", "mp3");
   
   aList->write();

   if (isMounted())
   {
      if (umount() != success)
      {
         tell(eloInfo, "Error: iPod busy, playback already running, stop first.");
         return fail;
      }
   }

   if (mount() != success)
   {
      tell(eloInfo, "Mount of '%s' -> failed", mountPoint);
      return fail;
   }

   tell(eloDebug, "Starting playback of %d songs", aList->getCount());

#if HAVE_MUSIC_PLUGIN
 	if (!(p = cPluginManager::GetPlugin("music")))
   {
      tell(eloError, "Error: Cannot find music plugin!");
      return fail;
   }
#endif

#if HAVE_MP3_PLUGIN
 	if (!(p = cPluginManager::GetPlugin("mp3")))
   {
      tell(eloError, "Error: Cannot find mp3 plugin!");
      return fail;
   }
#endif

#if HAVE_XINEMEDIAPLAYER_PLUGIN
 	if (!(p = cPluginManager::GetPlugin("xinemediaplayer")))
   {
      tell(eloError, "Error: Cannot find xinemediaplayer plugin!");
      return fail;
   }
#endif

#if HAVE_XINEMEDIAPLAYER_PLUGIN
    int i;
    char buf[512];
	// fill playlist
	snprintf(buf,512,  "%s/%s", playlistPath, "ipod-auto.m3u");
	p->SVDRPCommand("PLAYM3U", buf, i);
	if ( i == 550){
	snprintf(buf,512, "%s/%s", playlistPath, "ipod.m3u");
	p->SVDRPCommand("PLAYM3U", buf, i);
	}
	
	// empty playlist ipod-auto
	snprintf(buf,512,  "%s/%s", playlistPath, "ipod-auto.m3u");
	FILE *fp = fopen(buf, "w"); 
	fclose(fp);
#else
   osd = p->MainMenuAction();
   // select playlist menu

   if (strncmp(status->Title(), wanted, strlen(wanted)) != 0)
      osd->ProcessKey(kBack);

   // die auto-playlist suchen

   while (last != status->Current()) 
   {
      tell(eloDebug, "Debug: '%s'", status->Current());

      if (strcmp(status->Current(), " ipod-auto") == 0) 
      {
         tell(eloDebug, "Playlist found -> starting playpack");

         osd->ProcessKey(kOk);       // start playback
         break;
      }

      last = status->Current();
      osd->ProcessKey(kDown);
   }

   if (last == status->Current())
      tell(eloError, "Error: Playlist '%s' not found!", "ipod-auto");

   // podMenu->ProcessKey(kBack);
   // podMenu->ProcessKey(kRight);
   // podMenu->ProcessKey(kLeft);

   podMenu->Display();
   delete osd;
   osd = 0;
#endif
   return done;
}

// svdrp funktionen

const char **cPluginIpod::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = 
    {
        "OPEN\n"
        "    Open ipod plugin.",
        NULL
    };
    return HelpPages;
}


cString cPluginIpod::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)

{
    if (strcasecmp(Command, "OPEN") == 0) 
    {
        cRemote::CallPlugin("ipod"); 
	return Command;

    }
    return NULL;
}
//***************************************************************************
// VDR Internals
//***************************************************************************

VDRPLUGINCREATOR(cPluginIpod); // Don't touch this!
