/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/menu.h>
#include <vdr/remote.h>
#include "femoncfg.h"
#include "femonreceiver.h"
#include "femonosd.h"
#include "femonservice.h"
#include "femontools.h"
#include "femon.h"

#if defined(APIVERSNUM) && APIVERSNUM < 10400
#error "VDR-1.4.0 API version or greater is required!"
#endif

cPluginFemon::cPluginFemon()
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
}

cPluginFemon::~cPluginFemon()
{
  // Clean up after yourself!
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
}

const char *cPluginFemon::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginFemon::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginFemon::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginFemon::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginFemon::Stop(void)
{
  // Stop the background activities.
}

void cPluginFemon::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginFemon::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  if (cReplayControl::NowReplaying())
     Skins.Message(mtInfo, tr("Femon not available while replaying"));
  else
     return cFemonOsd::Instance(true);
  return NULL;
}

bool cPluginFemon::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if      (!strcasecmp(Name, "HideMenu"))       femonConfig.hidemenu       = atoi(Value);
  else if (!strcasecmp(Name, "DisplayMode"))    femonConfig.displaymode    = atoi(Value);
  else if (!strcasecmp(Name, "Position"))       femonConfig.position       = atoi(Value);
  else if (!strcasecmp(Name, "OSDHeight"))      femonConfig.osdheight      = atoi(Value);
  else if (!strcasecmp(Name, "OSDOffset"))      femonConfig.osdoffset      = atoi(Value);
  else if (!strcasecmp(Name, "Skin"))           femonConfig.skin = 0; //          = atoi(Value);
  else if (!strcasecmp(Name, "Theme"))          femonConfig.theme          = atoi(Value);
  else if (!strcasecmp(Name, "ShowCASystem"))   femonConfig.showcasystem   = atoi(Value);
  else if (!strcasecmp(Name, "RedLimit"))       femonConfig.redlimit       = atoi(Value);
  else if (!strcasecmp(Name, "GreenLimit"))     femonConfig.greenlimit     = atoi(Value);
  else if (!strcasecmp(Name, "UpdateInterval")) femonConfig.updateinterval = atoi(Value);
  else if (!strcasecmp(Name, "AnalStream"))     femonConfig.analyzestream  = atoi(Value);
  else if (!strcasecmp(Name, "CalcInterval"))   femonConfig.calcinterval   = atoi(Value);
  else if (!strcasecmp(Name, "UseSvdrp"))       femonConfig.usesvdrp       = atoi(Value);
  else if (!strcasecmp(Name, "ServerPort"))     femonConfig.svdrpport      = atoi(Value);
  else if (!strcasecmp(Name, "ServerIp"))       strn0cpy(femonConfig.svdrpip, Value, sizeof(femonConfig.svdrpip));
  else
    return false;
  if (femonConfig.displaymode < 0 || femonConfig.displaymode >= eFemonModeMaxNumber) femonConfig.displaymode = 0;

  return true;
}

bool cPluginFemon::Service(const char *Id, void *Data)
{
  if ((strcmp(Id,"FemonService-v1.0") == 0) && Data) {
     FemonService_v1_0 *data = (FemonService_v1_0*)Data;
     int ndx = cDevice::ActualDevice()->CardIndex();
     data->fe_name = getFrontendName(ndx);
     data->fe_status = getFrontendStatus(ndx);
     data->fe_snr = getSNR(ndx);
     data->fe_signal = getSignal(ndx);
     data->fe_ber = getBER(ndx);
     data->fe_unc = getUNC(ndx);
     data->video_bitrate = cFemonOsd::Instance() ? cFemonOsd::Instance()->GetVideoBitrate() : 0.0;
     data->audio_bitrate = cFemonOsd::Instance() ? cFemonOsd::Instance()->GetAudioBitrate() : 0.0;
     data->dolby_bitrate = cFemonOsd::Instance() ? cFemonOsd::Instance()->GetDolbyBitrate() : 0.0;
     return true;
     }

  return false;
}

const char **cPluginFemon::SVDRPHelpPages(void)
{ 
  static const char *HelpPages[] = {
    "OPEN\n"
    "    Open femon plugin.",
    "QUIT\n"
    "    Close femon plugin.",
    "NEXT\n"
    "    Switch to next possible device.",
    "PREV\n"
    "    Switch to previous possible device.",
    "INFO\n"
    "    Print the current frontend information.",
    "NAME\n"
    "    Print the current frontend name.",
    "STAT\n"
    "    Print the current frontend status.",
    "SGNL\n"
    "    Print the current signal strength.",
    "SNRA\n"
    "    Print the current signal-to-noise ratio.",
    "BERA\n"
    "    Print the current bit error rate.",
    "UNCB\n"
    "    Print the current uncorrected blocks rate.",
    "VIBR\n"
    "    Print the actual device and current video bitrate [Mbit/s].",
    "AUBR\n"
    "    Print the actual device and current audio bitrate [kbit/s].",
    "DDBR\n"
    "    Print the actual device and current dolby bitrate [kbit/s].",
    NULL
    };
  return HelpPages;
}

cString cPluginFemon::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  if (strcasecmp(Command, "OPEN") == 0) {
     if (cReplayControl::NowReplaying()) {
        ReplyCode = 550; // Requested action not taken
        return cString("Cannot open femon plugin while replaying");
        }
     if (!cFemonOsd::Instance())
        cRemote::CallPlugin("femon");
     return cString("Opening femon plugin");
     }
  else if (strcasecmp(Command, "QUIT") == 0) {
     if (cFemonOsd::Instance())
        cRemote::Put(kBack);
     return cString("Closing femon plugin");
     }
  else if (strcasecmp(Command, "NEXT") == 0) {
     if (cFemonOsd::Instance())
        return cString::sprintf("Switching to next device: %s", cFemonOsd::Instance()->DeviceSwitch(1) ? "ok" : "failed");
     else
        return cString("Cannot switch device");
     }
  else if (strcasecmp(Command, "PREV") == 0) {
     if (cFemonOsd::Instance())
        return cString::sprintf("Switching to previous device: %s", cFemonOsd::Instance()->DeviceSwitch(-1) ? "ok" : "failed");
     else
        return cString("Cannot switch device");
     }
  else if (strcasecmp(Command, "INFO") == 0) {
     return getFrontendInfo(cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "NAME") == 0) {
     return getFrontendName(cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "STAT") == 0) {
     return getFrontendStatus(cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "SGNL") == 0) {
     int value = getSignal(cDevice::ActualDevice()->CardIndex());
     return cString::sprintf("%04X (%02d%%) on device #%d", value, value / 655, cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "SNRA") == 0) {
     int value = getSNR(cDevice::ActualDevice()->CardIndex());
     return cString::sprintf("%04X (%02d%%) on device #%d", value, value / 655, cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "BERA") == 0) {
     return cString::sprintf("%08X on device #%d", getBER(cDevice::ActualDevice()->CardIndex()), cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "UNCB") == 0) {
     return cString::sprintf("%08X on device #%d", getUNC(cDevice::ActualDevice()->CardIndex()), cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "VIBR") == 0) {
     if (cFemonOsd::Instance())
        return cString::sprintf("%s on device #%d", *getBitrateMbits(cFemonOsd::Instance()->GetVideoBitrate()), cDevice::ActualDevice()->CardIndex());
     else
        return cString::sprintf("--- Mbit/s on device #%d", cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "AUBR") == 0) {
     if (cFemonOsd::Instance())
        return cString::sprintf("%s on device #%d", *getBitrateKbits(cFemonOsd::Instance()->GetAudioBitrate()), cDevice::ActualDevice()->CardIndex());
     else
        return cString::sprintf("--- kbit/s on device #%d", cDevice::ActualDevice()->CardIndex());
     }
  else if (strcasecmp(Command, "DDBR") == 0) {
     if (cFemonOsd::Instance())
        return cString::sprintf("%s on device #%d", *getBitrateKbits(cFemonOsd::Instance()->GetDolbyBitrate()), cDevice::ActualDevice()->CardIndex());
     else
        return cString::sprintf("--- kbit/s on device #%d", cDevice::ActualDevice()->CardIndex());
     }
  return NULL;
}

cMenuFemonSetup::cMenuFemonSetup(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  dispmodes[eFemonModeBasic]       = tr("basic");
  dispmodes[eFemonModeTransponder] = tr("transponder");
  dispmodes[eFemonModeStream]      = tr("stream");
  dispmodes[eFemonModeAC3]         = tr("AC-3");

  skins[eFemonSkinClassic]         = tr("Classic");
  skins[eFemonSkinElchi]           = tr("Elchi");

  themes[eFemonThemeClassic]       = tr("Classic");
  themes[eFemonThemeElchi]         = tr("Elchi");
  themes[eFemonThemeDeepBlue]      = tr("DeepBlue");
  themes[eFemonThemeMoronimo]      = tr("Moronimo");
  themes[eFemonThemeEnigma]        = tr("Enigma");
  themes[eFemonThemeEgalsTry]      = tr("EgalsTry");
  themes[eFemonThemeDuotone]       = tr("Duotone");
  themes[eFemonThemeSilverGreen]   = tr("SilverGreen");

  data = femonConfig;
  Setup();
}

void cMenuFemonSetup::Setup(void)
{
  int current = Current();

  Clear();
  //Add(new cMenuEditBoolItem(  tr("Hide main menu entry"),        &data.hidemenu,       tr("no"),            tr("yes")));
  Add(new cMenuEditStraItem(  tr("Default display mode"),        &data.displaymode,    eFemonModeMaxNumber, dispmodes));
  //Add(new cMenuEditStraItem(  tr("Skin"),                        &data.skin,           eFemonSkinMaxNumber, skins));
  //Add(new cMenuEditStraItem(  tr("Theme"),                       &data.theme,          eFemonThemeMaxNumber,themes));
  Add(new cMenuEditBoolItem(  tr("Position"),                    &data.position,       tr("bottom"),        tr("top")));
  Add(new cMenuEditIntItem(   tr("Height"),                      &data.osdheight,      400,                 500));
  Add(new cMenuEditIntItem(   tr("Horizontal offset"),           &data.osdoffset,      -50,                 50));
  //Add(new cMenuEditBoolItem(  tr("Show CA system"),              &data.showcasystem,   tr("no"),            tr("yes")));
  //Add(new cMenuEditIntItem(   tr("Red limit [%]"),               &data.redlimit,       1,                   50));
  //Add(new cMenuEditIntItem(   tr("Green limit [%]"),             &data.greenlimit,     51,                  100));
  //Add(new cMenuEditIntItem(   tr("OSD update interval [0.1s]"),  &data.updateinterval, 1,                   100));
  //Add(new cMenuEditBoolItem(  tr("Analyze stream"),              &data.analyzestream,  tr("no"),            tr("yes")));
  //if (femonConfig.analyzestream)
    // Add(new cMenuEditIntItem(tr("Calculation interval [0.1s]"), &data.calcinterval,   1,                   100));
  Add(new cMenuEditBoolItem(  tr("Use SVDRP service"),           &data.usesvdrp));
  if (data.usesvdrp) {
     Add(new cMenuEditIntItem(tr("SVDRP service port"),          &data.svdrpport,      1,                   65535));
     Add(new cMenuEditStrItem(tr("SVDRP service IP"),             data.svdrpip,        MaxSvdrpIp,          ".1234567890"));
     }

  SetCurrent(Get(current));
  Display();
}

void cMenuFemonSetup::Store(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  femonConfig = data;
  SetupStore("HideMenu",       femonConfig.hidemenu);
  SetupStore("DisplayMode",    femonConfig.displaymode);
  SetupStore("Skin",           femonConfig.skin);
  SetupStore("Theme",          femonConfig.theme);
  SetupStore("Position",       femonConfig.position);
  SetupStore("OSDHeight",      femonConfig.osdheight);
  SetupStore("OSDOffset",      femonConfig.osdoffset);
  SetupStore("ShowCASystem",   femonConfig.showcasystem);
  SetupStore("RedLimit",       femonConfig.redlimit);
  SetupStore("GreenLimit",     femonConfig.greenlimit);
  SetupStore("UpdateInterval", femonConfig.updateinterval);
  SetupStore("AnalStream",     femonConfig.analyzestream);
  SetupStore("CalcInterval",   femonConfig.calcinterval);
  SetupStore("UseSvdrp",       femonConfig.usesvdrp);
  SetupStore("ServerPort",     femonConfig.svdrpport);
  SetupStore("ServerIp",       femonConfig.svdrpip);
}

eOSState cMenuFemonSetup::ProcessKey(eKeys Key)
{
  int oldUsesvdrp = data.usesvdrp;
  int oldAnalyzestream = data.analyzestream;

  eOSState state = cMenuSetupPage::ProcessKey(Key);

  if (Key != kNone && (data.analyzestream != oldAnalyzestream || data.usesvdrp != oldUsesvdrp)) {
     Setup();
     }

  return state;
}

cMenuSetupPage *cPluginFemon::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuFemonSetup;
}

VDRPLUGINCREATOR(cPluginFemon); // Don't touch this!
