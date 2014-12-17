/*
 * reelcam.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <typeinfo>
#include <malloc.h>
#include <stdlib.h>

#include <vdr/plugin.h>
#include <vdr/menuitems.h>
#include <vdr/status.h>
#include <vdr/dvbdevice.h>
#include <vdr/channels.h>
#include <vdr/ci.h>

#include "reelcam.h"
#include "common.h"
#include "reel.h"
#include "observerthread.h"
#include "status.h"
#include "reelcamlink.h"
#include "i18n.h"

static const char *VERSION        = "0.0.10";
static const char *DESCRIPTION    = "a plugin switching the Reelbox-CAM-Matrix";
static const char *MAINMENUENTRY = "Common interface";

cPlugin *ReelPlugin;
int multica = 1;
int nocamlimit;

class cReelcamSetupMenu : public cMenuSetupPage {
  private:
    void Set();
  protected:
    void Store(void);
    virtual eOSState ProcessKey(eKeys Key);
  public:
    cReelcamSetupMenu(void);
};


// --- cReelCAM ---------------------------------------------------------------

cMutex cReelCAM::camLock;

bool cReelCAM::Load(const char *cfgdir)
{
  return true;
}

void cReelCAM::Save(void)
{
}

void cReelCAM::ShutDown(void)
{
}

void *cReelCAM::Init(const cDevice *dev)
{
  int n=dev->CardIndex();

  if(n<MAXDVBAPI) {
    cMutexLock lock(&camLock);

      if(typeid(*dev)==typeid(cDvbDevice)) {
        int camfd=-1;
        cCiHandler *ci=((cDevice *)dev)->CiHandler();
        if(ci) {                       /* get camDev from ciHandler */
          camfd=ci->GetCaFd();
          d(printf("reelcam %d: got ca device from ciHandler\n",n))
          }
        if(camfd<0) {                  /* last resort, try to open myself */
          d(printf("reelcam %d: try to open ca device myself\n",n))
          camfd=DvbOpen(DEV_DVB_CA,n,O_RDWR|O_NONBLOCK);
         }
        else esyslog("ERROR: can't open ca device for reelcam on card %d",n);
        }
      else esyslog("ERROR: reelcam doesn't support card %d!",n);

    }

  return false;
}

class cPluginReelcam : public cPlugin {
private:
  cRcStatus *status;
  ObserverThread *observer;
  int fd;
  // Add any member variables or functions you may need here.
public:
  cPluginReelcam(void);
  virtual ~cPluginReelcam();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  //virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; }
};

cPluginReelcam::cPluginReelcam(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  status=0;
  observer=0;
  fd=0;
}

cPluginReelcam::~cPluginReelcam()
{
  cReelCAM::ShutDown();
  delete status;
  delete observer;
  close(fd);
  // Clean up after yourself!
}

const char *cPluginReelcam::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginReelcam::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginReelcam::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  RegisterI18n(ReelcamPhrases);

#ifdef RBLITE
  if ((fd = open("/dev/reelfpga0", O_RDWR)) < 0){
    perror("error opening /dev/reelfpga0");
    return 1;
  }
#else
   cCiHandler *ci=cDevice::GetDevice(0)->CiHandler();
   if(ci)                    /* get camDev from ciHandler */
     fd=ci->GetCaFd();
#endif

  ReelPlugin=this;
  status = new cRcStatus(fd);
  observer = new ObserverThread(fd);
  observer->Start();
  return true;
}

bool cPluginReelcam::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginReelcam::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginReelcam::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginReelcam::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL; // No entry
}

cMenuSetupPage *cPluginReelcam::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  //return new cReelcamSetupMenu;
  return NULL;
}

bool cPluginReelcam::SetupParse(const char *Name, const char *Value)
{
  /* if(!strcmp(Name, "multica")){
    multica = atoi(Value);
    return true;
  } else */
  if(!strcmp(Name, "nocamlimit")){
    nocamlimit = atoi(Value);
    return true;
  }

  // Parse your own setup parameters and store their values.
  if(!*Name) {
    struct ReLink *link=(struct ReLink *)Name;
    if(!memcmp(link->magic,magic,sizeof(magic))) {
	  switch(link->op){
	case OP_PROVIDES:
	  link->num=-1;
	  link->num=status->ProvidesCa(link->dev, link->caids, link->channelNumber);
	  break;
	case OP_GETSLOT:
          link->slotOnDev=status->SlotOnDev(link->dev);
	  link->num=-1;
	  break;
	case OP_RESET:
	  link->num=status->CamReset(link->slot);
	default:
	  break;
      }
    }
    return false;
    }

  return true;
}

cReelcamSetupMenu::cReelcamSetupMenu() {
  Set();
}

void cReelcamSetupMenu::Store(void) {
  SetupStore("multica", multica);
}

void cReelcamSetupMenu::Set() {
  int current = Current();
  Clear();
  Add(new cMenuEditBoolItem(tr("Multichanneldecryption"), &multica));
  SetCurrent(Get(current));
  Display();
}

eOSState cReelcamSetupMenu::ProcessKey(eKeys Key){
  eOSState state = cOsdMenu::ProcessKey(Key);
  switch(Key){
    case kOk:
      Store();
      break;
    case kLeft:
    case kRight:
      Set();
      break;
    default:
      break;
  }
  return state;
}

VDRPLUGINCREATOR(cPluginReelcam); // Don't touch this!
