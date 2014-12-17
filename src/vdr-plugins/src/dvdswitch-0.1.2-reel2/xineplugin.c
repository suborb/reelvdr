#include <vdr/remote.h>
#include <vdr/plugin.h>
#include <vdr/player.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "xineplugin.h"
#include "debug.h"

#include "../xinemediaplayer/services.h"

#define ISOMOUNTPOINT "/tmp/DVDROOT"

cXinePluginThread::cXinePluginThread(char *image)
  :cThread("XinePluginThread")
{
  MYDEBUG("[dvdswitch] xinemediaplayer launcher");
  Image = image ? strdup(image) : NULL;
}

cXinePluginThread::~cXinePluginThread(void)
{
  MYDEBUG("Xine Thread beendet");
 
  if (Image)
  {
     free(Image);
  }

  Cancel();
}

void cXinePluginThread::Action(void)
{
  MYDEBUG("[dvdswitch] xinemediaplayer launcher (Action)");
  bool is_iso_image = false;
  printf("DDD: image: %s\n", Image);
  if (Image) 
  {
    if (strstr(Image, ".iso") || strstr(Image, ".ISO")) /* iso-image */
	    is_iso_image = true;

    if (is_iso_image)
    {
        char *cmd = NULL;
        asprintf (&cmd,"mkdir -p \"%s\" ", ISOMOUNTPOINT);
        int ret = SystemExec(cmd);
        free(cmd); cmd = NULL;
        asprintf (&cmd,"mount -o loop \"%s\" " ISOMOUNTPOINT  , Image);
        ret = SystemExec(cmd);
        MYDEBUG("- %s returns %d --- \n", cmd, WEXITSTATUS(ret));
        free(cmd);
    }

     std::vector<std::string> playlistEntries; //empty list
     std::string tmp = "dvd://" ISOMOUNTPOINT;

     if(!is_iso_image)
     {
        tmp = std::string("dvd://") + Image + "/VIDEO_TS";
     }   

     Xinemediaplayer_Xine_Play_mrl xinePlayData;
     xinePlayData.mrl      = tmp.c_str(); 
     xinePlayData.instance = -1;
     xinePlayData.playlist = false;
     xinePlayData.playlistEntries = playlistEntries;

    MYDEBUG("---- call XineMediaPlayer plugin --- mrl %s ---\n", xinePlayData.mrl);
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);

    cControl *control = cControl::Control();
    MYDEBUG(control ? (char*)"Control gefunden" : (char*)"Control nicht gefunden");

    while (control && Running())
    {
       cCondWait::SleepMs(100);
       control = cControl::Control();
    }
  }

  MYDEBUG("[dvdswitch] xinemediaplayer launcher (Action end)");

  cXinePlugin::CheckMount();
  cRemote::CallPlugin("dvdswitch");
  
  cXinePlugin::Exit();

}

// --- cXinePlugin ----------------------------------------------------------------

cXinePluginThread *cXinePlugin::thread = NULL;


void cXinePlugin::Start(char *image)
{
  MYDEBUG("- %s -- %s ---- \n", __PRETTY_FUNCTION__, image);
  if(!thread)
  {
    thread = new cXinePluginThread(image);
    thread->Start();
  }
}

void cXinePlugin::CheckMount(void)
{
   int ret = SystemExec("grep loop0  /proc/mounts");

   if (WEXITSTATUS(ret)==0) 
   {
	SystemExec("umount -d " ISOMOUNTPOINT);
   }

   if (WEXITSTATUS(ret)!=0) 
   {
       MYDEBUG(" umount Failed  ");
   }
}

void cXinePlugin::Exit(void)
{
  MYDEBUG( " cXinePlugin::Exit(void) ");
  if (thread) 
  {
    DELETENULL(thread);
  }
}

