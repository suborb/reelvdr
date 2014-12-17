/*
 * vdrcd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: vdrcd.c,v 1.3 2004/12/29 00:08:20 lordjaxom Exp $
 */

#include "vdrcd.h"
#include "i18n.h"
#include "setup.h"
#include "status.h"
#include "suspend.h"

#include <vdr/plugin.h>
#include <vdr/menu.h>

#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>


#define CDROMDEV "/dev/hda"

#define DVDDIR "/media/dvd"

//menu.c:using std::ofstream;
//menu.c:using std::time;
using std::endl;
using std::string;
using std::cout;
using std::cerr;
using std::vector;

typedef std::vector<string>::const_iterator strConstItr;



extern const char *VideoDirectory;

static const char *VERSION        = "0.0.10.1";
static const char *DESCRIPTION    = "Plays identified Media Discs (for autofs)";
static const char *MAINMENUENTRY  = "Play CD/DVD";

enum eTypeMethod {
   State,
   Find
};

struct cTypelist {
  const char *Object;
  const char *PlugIn;
  bool NeedsDir;
  eTypeMethod Method;
  bool NeedsUmount;
};

#define VDRSUBDIR "2002-01-01.01.01.99.99.rec"
#define FINDCMD   "find %s -follow -type f -name '%s' 2> /dev/null"

#define trP(x,p) I18nTranslate(x, Typelist[p].PlugIn)

static struct cTypelist Typelist[] = {
    // filetype       plugin    dir?     searchmode umount
#if HAVE_XINEMEDIAPLAYER
       { "VIDEO_TS",   "xinemediaplayer",  false, State, true  },
       { "video_ts",   "xinemediaplayer",  false, State, true  },
       { "*.mpeg",     "mediaplayerdvd", false, Find,  false },
       { "vcd",        "xinemediaplayer",     false, State, true  },
       { "svcd",       "xinemediaplayer",     false, State, true  },
       { "photo_cd",   "pcd",     false, State, true  },
       { "*.mp3",      "mediaplayerdvd",     false, Find,  false },
       { "*.ogg",      "mediaplayerdvd",     false, Find,  false },
       { "*.wav",      "mediaplayerdvd",    false, Find,  true },
#else
       { "VIDEO_TS",   "dvd",     false, State, true  },
       { "video_ts",   "dvd",     false, State, true  },
       { "*.mpeg",     "filebrowser", false, Find,  false },
       { "vcd",        "vcd",     false, State, true  },
       { "svcd",       "vcd",     false, State, true  },
       { "photo_cd",   "pcd",     false, State, true  },
       { "*.mp3",      "mp3",     false, Find,  false },
       { "*.ogg",      "mp3",     false, Find,  false },
  //     { "*.wav",      "cdda",    false, Find,  true },
#endif
       { "001.vdr",    "extrecmenu", true,  Find, false },
#if 0
       { "001.vdr",    NULL,      true,  State, false },
       { "001.vdr",    NULL,      false, Find,  false },
#endif
#if HAVE_FILEBROWSER
       { "*.jpg",      "mediaplayerdvd", false, Find,  false },
       { "*.avi",      "mediaplayerdvd", false, Find,  false },
       { "*.avi-????", "mediaplayerdvd", false, Find,  false },
       { "*.mpg",      "mediaplayerdvd", false, Find,  false },
       { "*.mpeg",     "mediaplayerdvd", false, Find,  false },
#endif
#if HAVE_MPLAYER_H
       { "*.jpg",      "mplayer", false, Find,  false },
       { "*.avi",      "mplayer", false, Find,  false },
       { "*.avi-????", "mplayer", false, Find,  false },
       { "*.mpg",      "mplayer", false, Find,  false },
       { "*.mpeg",     "mplayer", false, Find,  false },
#endif
       { NULL }
     };


class cPluginVdrcd : public cPlugin
{
private:
  vector<string> Directories;
  uint DirCount;
  string mountScript;
  string dvdDir;
  string dvdDevice;
  char *tmpcdrom;
  cVdrcdStatus *status;
  int fd;

  int TestType(std::string dir, const char *file);
  void TestDvdLink();
  int DiscStatus();
  bool IsAudio();
  void ShowSplashScreen();
  void HideSplashScreen(const char *pluginName);

public:
  cPluginVdrcd(void);
  virtual ~cPluginVdrcd();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual void Housekeeping(void) { } ;
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; }
  };

cPluginVdrcd::cPluginVdrcd(void)
{
   Directories.clear();

#ifdef XEATRE
   Directories.push_back("/media/cdfs");
   Directories.push_back("/media/dvd");
#endif
}

void cPluginVdrcd::TestDvdLink()
{
  struct stat buf;
  string VideoDir = VideoDirectory;
  VideoDir.append("/dvd");
  if (lstat(VideoDir.c_str(),&buf) < 0)
  {
      DLOG ( " DBUG VDRCD  missing Link at %s  ln -s /media/dvd  %s ", VideoDir.c_str(), VideoDir.c_str());
      symlink("/media/dvd", VideoDir.c_str());
  }
  DLOG ( " DBUG VDRCD  ---------- TestDvdLink %s  ----------  ", VideoDir.c_str());
}

cPluginVdrcd::~cPluginVdrcd()
{
  char path[1024];
  if(tmpcdrom)
  {
    sprintf(path,"%s/cdrom/%s",tmpcdrom,VDRSUBDIR);
    unlink(path);
    sprintf(path,"%s/cdrom",tmpcdrom);
    rmdir(path);
    rmdir(tmpcdrom);
  }

  delete status;
}

const char *cPluginVdrcd::CommandLineHelp(void)
{
   return "  -m mountscript, --mount=<script.sh>    mount.sh script [mount.sh]\n"
          "  -c cdromDir,    --cddir=CdromDir       first or additional CD-ROM [/cdrom]\n"
          "  -d device,  --device=<device>          path to disc drive device path [/dev/cdrom] \n";
}

bool cPluginVdrcd::ProcessArgs(int argc, char *argv[])
{
  static struct option long_options[] = {
       { "cddir",   required_argument, NULL, 'c' },
       { "mount",   required_argument, NULL, 'm' },
       { "device",  required_argument, NULL, 'd' },
       { NULL }
     };

  int c;
  while ((c = getopt_long(argc, argv, "m:c:d:", long_options, NULL)) != -1)
  {
      switch (c) {
      case 'c':
         dvdDir = optarg;
         break;
      case 'm':
         mountScript = optarg;
         break;
      case 'd':
          dvdDevice = optarg;
         break;

      default:
         return false;
      }
   }

  if (dvdDir.empty())
  {
      dvdDir = DVDDIR;
  }

  if (dvdDevice.empty())
  {
      dvdDevice  = CDROMDEV;
  }
  DLOG ( " DEBUG VDRCD +++++++ Add dvdDevice \"%s\" ", dvdDir.c_str());
  Directories.push_back(dvdDir);
  return true;
}

bool cPluginVdrcd::Start(void) {
   RegisterI18n(Phrases);

   if (mountScript.empty())
      mountScript = "mount.sh";

   if (Directories.size() == 0)
      Directories.push_back("/cdrom");

   status = new cVdrcdStatus;

  return true;
}


int cPluginVdrcd::TestType(string dir, const char *file)
{
  int cnt=0;
  char *cmd = NULL;
  asprintf(&cmd, FINDCMD, dir.c_str(), file);
  DLOG ( " ----!! DEBUG find cmd: %s !! ------------------ ",cmd);

  FILE *fp = popen(cmd, "r");
  if (fp)
  {
    char *s;
    cReadLine r;
    while ((s = r.Read(fp)) != NULL) {
      DLOG ( " DEBUG VDRCD +++++++ TestType  s: %s  ",s);
      if(strcmp(s,"")!=0) cnt++;
    }
  }
  fclose(fp);
  free(cmd);

  return cnt;
}


int cPluginVdrcd::DiscStatus()
{
  int ret = -1;
  fd = open(dvdDevice.c_str(), O_RDONLY|O_NONBLOCK);
  if ((fd<0))
  {
    esyslog ("Sorry, can't open cd-device \"%s\" !", dvdDevice.c_str()); // FIXME
    return -1;
  }
  else
  {
    ret=ioctl(fd, CDROM_DRIVE_STATUS, NULL);
    switch (ret)
    {
        case CDS_NO_DISC:
             Skins.Message(mtError, tr("Please insert disk!"));
             ioctl(fd,CDROMEJECT,NULL);
             return (-1);
       case CDS_TRAY_OPEN:
            if (ioctl(fd,CDROMCLOSETRAY,NULL) == -1)
            {
                Skins.Message(mtError, tr("Please insert disk!"));
                return -1; // Slot-In doesn't have any tray to close
            } else
                return 1;

       case CDS_DRIVE_NOT_READY:
            cCondWait::SleepMs(10);
            return 1;
       case CDS_DISC_OK:
            return 0;
       default:
            return 1;
    }
  }
}

bool cPluginVdrcd::IsAudio()
{
  int ret=ioctl(fd, CDROM_DISC_STATUS, NULL);
  if (ret ==CDS_AUDIO || ret == CDS_MIXED)
  {
    isyslog (" vdrcd: Audio or Mixed CD ROM detected ");
    return true;
  }
  return false;
}

cOsdObject *cPluginVdrcd::MainMenuAction(void)
{
  struct stat buf;
  uint type = 0;
  bool found = false;
  string path;
  string mediaDir;

  Skins.Message(mtStatus, tr("Identifying Disc"));
  ShowSplashScreen();

  // first we check medium
  int ret = 0;
  while ((ret = DiscStatus()) != 0)
  {
    // if drive is empty exit
    if (ret == -1)
    {
       Skins.Message(mtStatus, NULL);
       HideSplashScreen("");
       return NULL;
    }
  }

  Skins.Message(mtStatus, tr("Identifying Disc"));
  ShowSplashScreen();

  if (IsAudio())
  {
#if HAVE_XINEMEDIAPLAYER
     HideSplashScreen("");
     cPluginManager::CallAllServices("Xine Play AudioCD", NULL);
     Skins.Message(mtStatus, NULL);
     return NULL;
#else
     cPlugin *p = cPluginManager::GetPlugin("cdda");
     cOsdObject *osd = NULL;
     if (p)
     {
       osd = p->MainMenuAction();
       Skins.Message(mtStatus, NULL);
       HideSplashScreen("");
       return osd;
     }
#endif
  }

  for (strConstItr dir = Directories.begin(); dir != Directories.end(); ++dir)
  {
     mediaDir = *dir;
     DLOG (" DEBUG VDRCD  Scan Directory %s ", mediaDir.c_str());
     int ret = -1;

     string cmd = mountScript;
     cmd += " status ";
     cmd += mediaDir;

     ret = system(cmd.c_str());

     if (WEXITSTATUS(ret) >= 1) {
          cmd.clear();
          cmd = mountScript;
          cmd += " mount ";
          cmd += mediaDir;
          ret = system(cmd.c_str());
     }

     if (WEXITSTATUS(ret) == 0) {
         for (type = 0; Typelist[type].Object != NULL; ++type) {
            switch (Typelist[type].Method) {
            case State:
               path = mediaDir;
               path += "/";
               path += Typelist[type].Object;
               if (stat(path.c_str(), &buf) == 0)
               {
                  found = true;
               }
               break;
            case Find:
               if (TestType(mediaDir, Typelist[type].Object))
                {
                  found = true;
                }
               break;
            }
            if (found)
               break;
         }
      }

      if (!found || Typelist[type].NeedsUmount) {
          cmd.clear();
          cmd = mountScript;
          cmd += " unmount ";  // cmd.replace(cmd.find(status)
          cmd += mediaDir;
          ret = system(cmd.c_str());
          system(cmd.c_str());
      }

      if (found)
         break;
   }

   Skins.Message(mtStatus, NULL);

   if (!found)
   {
      Skins.Message(mtError, tr("Couldn't identify disc!"));
      HideSplashScreen("");
      return NULL;
   }

/*
   if (Typelist[type].PlugIn == NULL)
   {
      /// for
      const char *oldvideodir = VideoDirectory;

      if (Typelist[type].NeedsDir)
      {
         if (tmpcdrom == NULL)
         {
            tmpcdrom = tmpnam(NULL);
            mkdir(tmpcdrom,0777);
            path.clear();
            path = tmpcdrom;
            path += "/cdrom";
            mkdir(path.c_str(), 0777);
            path += "/" VDRSUBDIR;
            symlink( mediaDir.c_str(), path.c_str());
         }
         VideoDirectory = tmpcdrom;
      }
      else
         VideoDirectory = mediaDir.data();

      DLOG (" DEBUG VDRCD -- open recmenu in %s\n", VideoDirectory);

#if VDRVERSNUM >= 10311
      Recordings.Load();
#endif
      cOsdMenu *menu = new cMenuRecordings();
      VideoDirectory = oldvideodir;
      status->NumClears() = 3;

      return menu;
   }
*/

   cPlugin *p = cPluginManager::GetPlugin(Typelist[type].PlugIn);
   cOsdObject *osd = NULL;
   if (p)
   {
#if HAVE_XINEMEDIAPLAYER
      if (strcmp(Typelist[type].PlugIn, "xinemediaplayer") == 0)
      {
          if ((strcmp(Typelist[type].Object, "vcd") == 0)
           || (strcmp(Typelist[type].Object, "svcd") == 0))
          {
              HideSplashScreen("");
              cPluginManager::CallAllServices("Xine Play VideoCD", NULL);
              return NULL;
          }
          else
          {
            std::vector<std::string> playlistEntries; //empty list
            Xinemediaplayer_Xine_Play_mrl xinePlayData  =
            {
                "dvd://",
		-1,
                false,
                playlistEntries
            };
            HideSplashScreen("");
            cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
            return NULL;
          }
      }
#else
      if (strcmp(Typelist[type].PlugIn, "dvd") == 0)
      {
           osd = p->MainMenuAction();
           return osd;
      }
#endif
      if (strcmp(Typelist[type].PlugIn, "extrecmenu") == 0)
      {
           TestDvdLink();
           p->Service("archive", (void *) "mount");
           HideSplashScreen("");
           osd = p->MainMenuAction();
           if (osd)
           {
             osd->ProcessKey(kOk);
           }
           return osd;
      }

      if (strcmp(Typelist[type].PlugIn, "vcd") == 0 && VdrcdSetup.AutoStartVCD)
      {
         const char *last = status->Current();

         osd = p->MainMenuAction();
         if (osd)
         {
            osd->ProcessKey(kDown);
            if (status->Current() != last)
            {
               // VCD has more than one track, so leave the menu open
               osd->ProcessKey(kUp);
            }
            else
            {
               // Autostart CD and destroy menu
               osd->ProcessKey(kOk);
               delete osd;
               osd = NULL;
            }
         }
         HideSplashScreen("");
         return osd;
      }
#if HAVE_MPLAYER_H
      else if (strcmp(Typelist[type].PlugIn, "mplayer") == 0)
      {
         osd = p->MainMenuAction();
         // Switch to "source" mode
         osd->ProcessKey(kYellow);
         // Move to first entry
         const char *actual = status->Current();
         osd->ProcessKey(kUp);
         while (actual != status->Current())
         {
            osd->ProcessKey(kUp);
            actual = status->Current();
         }
         // Search for /cdrom-entry
         bool mplayersource = true;
         while (strstr(actual,mediaDir.c_str()) == NULL)
         {
            osd->ProcessKey(kDown);
            if (actual == status->Current())
            {
               mplayersource = false;
               break;
            }
            else
            {
               actual = status->Current();
            }
         }

         if (mplayersource)
         {
            osd->ProcessKey(kOk);

            const char *last = status->Current();
            osd->ProcessKey(kDown);
            if (status->Current() != last)
            {
               // CD has more than one avi-file, so leave the menu open
               osd->ProcessKey(kUp);
            }
            else
            {
               // Autostart CD and destroy menu
               osd->ProcessKey(kOk);

               delete osd;
               osd = NULL;
            }
         }
         else
         {
            delete osd;
            osd = NULL;
            Skins.Message(mtError, tr("Drive not present in mplayersources.conf!"));
         }
      }
#endif // HAVE_MPLAYER
#if HAVE_FILEBROWSER
      else if (strcmp(Typelist[type].PlugIn, "filebrowser") == 0)
      {
         HideSplashScreen("");
         p->Service("startdir", (void *) "/media/dvd");
         osd = p->MainMenuAction();
      }
#endif  // HAVE_FILEBROWSER
      else if (strcmp(Typelist[type].PlugIn, "mp3") == 0)
      {
         const char *wanted = trP("MP3", type);

         osd = p->MainMenuAction();
         // Switch to Browse Mode if not already done...
         if (strncmp(status->Title(), wanted, strlen(wanted)) != 0)
            osd->ProcessKey(kBack);

         // Now switch into "source" mode
         osd->ProcessKey(kGreen);

         const char *last = NULL;
         while (last != status->Current()) {
            if (strcmp(status->Current() + strlen(status->Current())
                  - mediaDir.size(), mediaDir.c_str()) == 0) {

               osd->ProcessKey(kRed);    // Select it
               osd->ProcessKey(kYellow); // Change to browse mode again

               if (VdrcdSetup.AutoStartMP3)
                  osd->ProcessKey(kYellow); // Instant Playback

               break;
            }

            last = status->Current();
            osd->ProcessKey(kDown);
         }

         delete osd;
         osd = NULL;

         if (last == status->Current())
         {
            Skins.Message(mtError, tr("Drive not present in mp3sources.conf!"));
         }
      }
      HideSplashScreen("");
      return osd;
   }
   else
   {
      Skins.Message(mtError, tr("Missing appropriate PlugIn!"));

      if (!Typelist[type].NeedsUmount) // Unmount again if nothing found
      {
          string cmd = mountScript;
          cmd += " unmount ";
          cmd += mediaDir;
          system(cmd.c_str());
      }
   }

   DLOG ( " DBUG VDRCD   MainAction End! \n");
   HideSplashScreen("");
   return osd;
}

cMenuSetupPage *cPluginVdrcd::SetupMenu(void) {
  return new cVdrcdMenuSetupPage;
}

bool cPluginVdrcd::SetupParse(const char *Name, const char *Value) {
  return VdrcdSetup.SetupParse(Name, Value);
}

void cPluginVdrcd::ShowSplashScreen()
{
   printf("\n\n\n########################ShowSplashScreen################\n\n\n");
   cControl::Shutdown(); //avoid deadlocks when replay paused
   cControl::Launch(new cSuspendCtl);
   cControl::Attach(); // make sure device is attach
}

void cPluginVdrcd::HideSplashScreen(const char *pluginName)
{
   printf("pluginMediaD::HideSplashScreen, pluginName = %s\n", pluginName);
   //Hide sreen if disc not identyfied and no plugin started that attaches new player
   if (std::string(pluginName) != "dvd" &&
       std::string(pluginName) != "vcd" &&
       std::string(pluginName) != "mplayer" &&
       std::string(pluginName) != "mp3" &&
       std::string(pluginName) != "cdda" &&
       std::string(pluginName) != "xinemediaplayer")
   {
       cControl::Shutdown();
   }
}

VDRPLUGINCREATOR(cPluginVdrcd); // Don't touch this!
