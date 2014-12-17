//***************************************************************************
/*
 * ipod.hpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

#ifndef __IPOD_H__
#define __IPOD_H__

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/plugin.h>
#include "itunes.hpp"
#include "status.hpp"

//***************************************************************************
// Constants
//***************************************************************************

static const char *VERSION        = "0.0.4";
static const char *DESCRIPTION    = "Apple's iPod";
static const char *MAINMENUENTRY  = "iPod";


//***************************************************************************
// Class Playlist
//***************************************************************************

class PlayList : public List, public ITunesService
{
   public:

      PlayList();
      ~PlayList();

      Song* getFirst() { return (Song*)List::getFirst();}
      Song* getNext()  { return (Song*)List::getNext();}

      const char* getPath() { return path; }
      void setPath(const char* aPath, const char* aFile);
      void setItunes(ITunes* aITunes) { iTunes = aITunes; }

      int read();
      int write();
      const char* toTrackPath(const char* aPath);
      const char* fromTrackPath(const char* aPath);

      static int cmpFct(void* arg1, void* arg2);

   protected:

      // data
      
      char path[sizePath+TB];
      ITunes* iTunes;
};


//***************************************************************************
// Class Plugin iPod
//***************************************************************************

class cPluginIpod : public cPlugin, public Cs
{
   public:

      cPluginIpod(void);
      virtual ~cPluginIpod();

      virtual const char* Version(void)         { return VERSION; }
      virtual const char* Description(void)     { return DESCRIPTION; }
      virtual const char* MainMenuEntry(void)   { return MAINMENUENTRY; }
      virtual const char* CommandLineHelp(void);
      virtual bool ProcessArgs(int argc, char *argv[]);
      virtual bool Initialize(void);
      virtual bool Start(void);
      virtual void Housekeeping(void);

      virtual cOsdObject* MainMenuAction(void);
      virtual cMenuSetupPage *SetupMenu(void);
      virtual bool SetupParse(const char *Name, const char *Value);
      virtual bool HasSetupOptions(void) { return false; }; // hidden setup menu entry
      
      int init();
      int play(PlayList* aList, cOsdMenu* podMenu);

      int mount();
      int umount();
      int isMounted();
      
      // gettings

      ITunes* getITunes()       { return itunes; }
      PlayList* getPlaylist()   { return playlist; }
      PlayList* getAutolist()   { return autolist; }

      // static stuff

      static char playlistPath[sizePath+TB];
      static char mountPoint[sizePath+TB];
      static char mountScript[sizePath+TB];
      
      // svdrp stuff
      const char **SVDRPHelpPages(void);
      cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
      
   protected:

      ITunes* itunes;
      PlayList* playlist;
      PlayList* autolist;
      char iTunesPath[sizePath+TB];
      int debug;
      PluginStatus* status;
};

//***************************************************************************
#endif //__IPOD_H__
