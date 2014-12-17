/*
 * VideoCD Player plugin for VDR
 * vcd.c: main
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>
#include "functions.h"
#include "i18n.h"
#include "menu.h"
#include "menucontrol.h"
#include "psdcontrol.h"
#include "psd.h"
#include "setup.h"
#include "mediaservice.h"
#include <cstring>

static const char *VERSION        = "0.7";
static const char *DESCRIPTION    = "VideoCD Player";
static const char *MAINMENUENTRY  = "VideoCD";
static const char *SETUPMENUENTRY  = "VideoCD";


class cPluginVcd : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cVcd *vcd;
  const char *option_vcd;
  cMediaService *mediaservice;
public:
  cPluginVcd(void);
  virtual ~cPluginVcd();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void);
#ifdef PLUGINENTRY
  virtual const char *MenuSetupPluginEntry(void) {
    return tr(SETUPMENUENTRY);
    }
#endif
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Service(char const *id, void *data);
  virtual bool Start(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void);
  virtual cOsdMenu *MainMenuAction(void);
  virtual cOsdMenu *MainMenuAction2(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  };

cPluginVcd::cPluginVcd(void)
{
  // Initialize any member varaiables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  vcd = NULL;
  option_vcd = NULL;
  mediaservice = NULL;
}

cPluginVcd::~cPluginVcd()
{
  // Clean up after yourself!
  delete vcd;
  delete mediaservice;
}

const char *cPluginVcd::Description(void)
{
  /*  //TB: no device name in menu
  if (option_vcd) {
     char *buf = NULL;
     asprintf(&buf, "%s (%s)", DESCRIPTION, option_vcd);
     return buf;
     }
  else
  */
     return tr(DESCRIPTION);
}

const char *cPluginVcd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return "  -v DEV,   --vcd=DEV      use DEV as the CD-ROM device (default is /dev/cdrom)\n";
}

bool cPluginVcd::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  static struct option long_options[] = {
       { "vcd", required_argument, NULL, 'v' },
       { NULL }
     };

  int c;
  while ((c = getopt_long(argc, argv, "v:", long_options, NULL)) != -1) {
     switch (c) {
        case 'v': option_vcd = optarg; break;
        default:  return false;
        }
     }
  return true;
}

bool cPluginVcd::Start(void)
{
  // Start any background activities the plugin shall perform.
  RegisterI18n(Phrases);

  mediaservice = new cMediaService(Name());
  cPlugin *manager = cPluginManager::GetPlugin(MEDIA_MANAGER_PLUGIN);
  if(manager) {
    MediaManager_Register_v1_0 reg;
    char *buf = NULL;
    asprintf(&buf,"%s%s","Media ",tr(MAINMENUENTRY));
    reg.PluginName = Name();
    reg.MainMenuEntry = buf;
    reg.Description = tr(DESCRIPTION);
    reg.mediatype = mtVcd;
    reg.shouldmount = false;
    if(manager->Service(MEDIA_MANAGER_REGISTER_ID, &reg)) {
      char *tmp = NULL;
      mediaservice->SetManager(manager);
      asprintf(&tmp,"%s %s","Super ",tr(DESCRIPTION));
      reg.Description = tmp;
      reg.mediatype = mtSvcd;
      manager->Service(MEDIA_MANAGER_REGISTER_ID, &reg);
      free(buf);
      free(tmp);
      isyslog("vcd: Successful registered");
      return true;
    }
	free(buf);
  }
  if (option_vcd)
     vcd = new cVcd(option_vcd);
  else
     vcd = new cVcd("/dev/cdrom");
  return true;
}

void cPluginVcd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

const char *cPluginVcd::MainMenuEntry(void)
{
  if(cMediaService::HaveManager())
     return NULL;
  if (VcdSetupData.HideMainMenuEntry)
     return NULL;
  if (vcd->DriveExists()) {
     if (option_vcd) {
        char *buf = NULL;
        asprintf(&buf, "%s", tr(MAINMENUENTRY));
        return buf;
        }
     else
        return tr(MAINMENUENTRY);
     }
  else
     return NULL;
}

cOsdMenu *cPluginVcd::MainMenuAction(void)
{
    vcd->medium = MEDIUM_DEVICE;
    return MainMenuAction2();
}

bool cPluginVcd::Service(char const *id, void *data)
{
  if (data == NULL)
      return false;

  if (std::strcmp(id, "VCD Play BIN/CUE") == 0)
    {
        struct BinCueData
        {
            char const *cuePath;
            cOsdMenu   *osdMenu;
        };
        BinCueData *binCueData = static_cast<BinCueData *>(data);
        vcd->medium = MEDIUM_BIN_CUE;
        vcd->SetFilePath(binCueData->cuePath);
        binCueData->osdMenu = MainMenuAction2();
        return true;
    }
    else if (strcmp(id, MEDIA_MANAGER_ACTIVATE_ID) == 0)
    {
        struct MediaManager_Activate_v1_0 *act = (MediaManager_Activate_v1_0*)data;
        if (act->on)
	{
            if (vcd == NULL)
                vcd = new cVcd(act->device_file);
            cMediaService::SetActive(true);
            return true;
        } else
	{
            cMediaService::SetActive(false);
            return true;
        }
    } else if (strcmp(id, MEDIA_MANAGER_MAINMENU_ID) == 0)
    {
         if (cMediaService::IsActive())
	 {
             MediaManager_Mainmenu_v1_0 *m = (MediaManager_Mainmenu_v1_0*)data;
             m->mainmenu = NULL;
         }
         return true;
    }
    return false;
}

/* bool cPluginVcd::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if(Data == NULL)
    return false;

  if(strcmp(Id, MEDIA_MANAGER_ACTIVATE_ID) == 0) {
    struct MediaManager_Activate_v1_0 *act = (MediaManager_Activate_v1_0*)Data;
    if(act->on) {
      if(vcd == NULL)
        vcd = new cVcd(act->device_file);
      cMediaService::SetActive(true);
      return true;
    } else {
      cMediaService::SetActive(false);
      return true;
      }
  } else if(strcmp(Id, MEDIA_MANAGER_MAINMENU_ID) == 0) {
    if(cMediaService::IsActive()) {
      MediaManager_Mainmenu_v1_0 *m = (MediaManager_Mainmenu_v1_0*)Data;
      m->mainmenu = NULL;
      }
      return true;
  }
  return false;
}
*/

cOsdMenu *cPluginVcd::MainMenuAction2(void)
{
  if (vcd->getVCD() && vcd->DiscOk()) {
     int psd_size = 0;
     if (vcd->Open()) {
        vcd->readTOC(CDROM_LBA);
        vcd->readSectorXA21(INFO_VCD_LBA, &(vcd->vcdInfo));
        vcd->readSectorXA21(ENTRIES_VCD_LBA, &(vcd->vcdEntries));
        for (int i=0; i<LOT_VCD_SIZE; i++)
           vcd->readSectorXA21(LOT_VCD_LBA+i, &(vcd->vcdLot.list_id_offset[i*CD_XA21_DATASIZE/2]));
        psd_size = max((int)U32_TO_LE(vcd->vcdInfo.psd_size), PSD_VCD_MAX_SIZE*CD_XA21_DATASIZE);
        for (int i=0; i<psd_size/CD_XA21_DATASIZE+1; i++)
           vcd->readSectorXA21(PSD_VCD_LBA+i, &(vcd->vcdPsd[i*CD_XA21_DATASIZE]));
        }
     else
        psd_size = max((int)U32_TO_LE(vcd->vcdInfo.psd_size), PSD_VCD_MAX_SIZE*CD_XA21_DATASIZE);
     if (VcdSetupData.AutostartReplay) {
        if (vcd->isLabel()  && vcd->getTracks()>0) {
           if (VcdSetupData.PlaySequenceReplay && psd_size) {
              int offs = vcd->vcdInfo.offset_multiplier * U16_TO_LE(vcd->vcdLot.list_id_offset[0]);
              union psd_vcd *psdVcd = (union psd_vcd*)(&(vcd->vcdPsd[offs]));
              if (psdVcd->header==PLAY_LIST_HEADER) {
                 __u16 item = U16_TO_LE(psdVcd->play.play_item[0]);
                 if (PLAY_ITEM_TYP(item)==piTrack)
                    cPsdVcdControl::SetTrack(PLAY_ITEM_NO(item), vcd, psdVcd);
                 else if (PLAY_ITEM_TYP(item)==piEntry)
                    cPsdVcdControl::SetEntry(PLAY_ITEM_NO(item), vcd, psdVcd);
                 else if (PLAY_ITEM_TYP(item)==piSpi)
                    cPsdSpiControl::SetItem(PLAY_ITEM_NO(item), vcd, psdVcd);
                 }
              else if (psdVcd->header==SELECTION_LIST_HEADER) {
                 __u16 item = U16_TO_LE(psdVcd->selection.play_item);
                 if (PLAY_ITEM_TYP(item)==piTrack)
                    cPsdVcdControl::SetTrack(PLAY_ITEM_NO(item), vcd, psdVcd);
                 else if (PLAY_ITEM_TYP(item)==piEntry)
                    cPsdVcdControl::SetEntry(PLAY_ITEM_NO(item), vcd, psdVcd);
                 else if (PLAY_ITEM_TYP(item)==piSpi)
                    cPsdSpiControl::SetItem(PLAY_ITEM_NO(item), vcd, psdVcd);
                 }
              dsyslog("VCD: Autoplay, PSD");
              }
           else {
              cMenuVcdControl::SetTrack(1, vcd);
              dsyslog("VCD: Autoplay, no PSD");
              return new cMenuVcd(vcd); //dirty hack to avoid null ptr exceptions in mediad
              }
           }
        else
           Skins.Message(mtInfo, tr("No VideoCD detected"));
        }
     else {
        if (VcdSetupData.PlaySequenceReplay && psd_size) {
           return new cVcdPsd(vcd);
           }
        else
           return new cMenuVcd(vcd);
        }
     }
  else
     Skins.Message(mtInfo, tr("No disc inserted"));

  return NULL;
}

cMenuSetupPage *cPluginVcd::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cVcdSetupMenu(vcd);
}

bool cPluginVcd::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(Name, "DriveSpeed"))
     VcdSetupData.DriveSpeed = atoi(Value);
  else if (!strcasecmp(Name, "BrokenMode"))
     VcdSetupData.BrokenMode = atoi(Value);
  else if (!strcasecmp(Name, "HideMainMenuEntry"))
     VcdSetupData.HideMainMenuEntry = atoi(Value);
  else if (!strcasecmp(Name, "PlayTracksContinuously"))
     VcdSetupData.PlayTracksContinuously = atoi(Value);
  else if (!strcasecmp(Name, "AutostartReplay"))
     VcdSetupData.AutostartReplay = atoi(Value);
  else if (!strcasecmp(Name, "PlaySequenceReplay"))
     VcdSetupData.PlaySequenceReplay = atoi(Value);
  else
     return false;
  return true;
}


// --- cMediaService --------------------------------------------------------
cPlugin *cMediaService::manager = NULL;
bool cMediaService::active = false;
bool cMediaService::replaying = false;
char *cMediaService::myname = NULL;

cMediaService::cMediaService(const char *MyName)
{
  manager = NULL;
  myname = strdup(MyName);
}

cMediaService::~cMediaService(void)
{
  manager = NULL;
  free(myname);
}

void cMediaService::SetManager(cPlugin *Manager)
{
  manager = Manager;
}

bool cMediaService::HaveManager(void)
{
  if(manager) return true;
  return false;
}

void cMediaService::SetReplaying(bool Replaying)
{
  if(manager && active) {
    MediaManager_Status_v1_0 st;
    st.set = true;
    st.flags = MEDIA_STATUS_REPLAY;
    st.isreplaying = Replaying;
    manager->Service(MEDIA_MANAGER_STATUS_ID, &st);
    replaying = Replaying;
    }
   else
     replaying = false;
}

void cMediaService::SetActive(bool Active)
{
  active = Active;
}

bool cMediaService::IsReplaying(void)
{
  return replaying;
}

bool cMediaService::IsActive(void)
{
  return active;
}

void cMediaService::EjectDisc(void)
{
  if(manager && active) {
    MediaManager_Status_v1_0 st;
    st.set = true;
    st.flags = MEDIA_STATUS_EJECTDISC;
    st.ejectdisc = true;
    manager->Service(MEDIA_MANAGER_STATUS_ID, &st);
    }
}

void cMediaService::Suspend(bool Suspend)
{
  if(manager) {
    MediaManager_Suspend_v1_0 s;
	s.PluginName = myname;
    s.suspend = Suspend;
    manager->Service(MEDIA_MANAGER_SUSPEND_ID, &s);
    }
}

VDRPLUGINCREATOR(cPluginVcd); // Don't touch this!
