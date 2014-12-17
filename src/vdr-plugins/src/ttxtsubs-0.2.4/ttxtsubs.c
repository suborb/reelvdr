/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/vdrttxtsubshooks.h>
#include <vdr/menuitems.h>
#include <vdr/config.h>
#include <vdr/tools.h>

#define TIMEMEASURE 0
#if TIMEMEASURE
#define TIMEVALSIMPL(tv, tv2) \
  if(tv2.tv_usec < tv.tv_usec) { \
    tv2.tv_usec += 1000000; \
    tv2.tv_sec -= 1; \
  }

#include <sys/time.h>
#include <time.h>
#endif

#include "ttxtsubsglobals.h"
#include "ttxtsubsdisplayer.h"
#include "utils.h"
#include "ttxtsubspagemenu.h"
#include "ttxtsubschannelsettings.h"
#include "ttxtsubslivereceiver.h"

#if defined(APIVERSNUM) && APIVERSNUM < 10706
#error "This version of ttxtsubs only works with vdr version >= 1.7.6!"
#endif
#if TTXTSUBSVERSNUM != 2
#error "This version of ttxtsubs requires the ttxtsubs patch version 2 to be applied to VDR!!"
#endif

static const char *VERSION        = "0.2.4";
static const char *DESCRIPTION    = trNOOP("Teletext subtitles");

cTtxtsubsConf globals;
cTtxtSubsChannelSettings TtxtSubsChannelSettings;

// ISO 639-2 language codes in VDR order
// XXX should be replaced with something that allows for other languages and for real language names!
// <http://www.avio-systems.com/dtvcc/iso639-2.txt>
// <http://www.loc.gov/standards/iso639-2/englangn_ascii.html>
const char *gLanguages[][2] = { 
  {"",""},       //None
  {"eng",""},    //English
  {"deu","ger"}, //Deutsch
  {"slv",""},    //Slovenian
  {"ita",""},    //Italian
  {"dut","nld"}, //Dutch
  {"por",""},    //Portuguese
  {"fre","fra"}, //French
  {"nor",""},    //Norwegian
  {"fin","suo"}, //Finnish
  {"pol",""},    //Polish
  {"spa","esl"}, //Spanish
  {"gre","ell"}, //Greek
  {"swe","sve"}, //Swedish
  {"ron","rum"}, //Romanian
  {"hun",""},    //Hungarian
  {"cat",""},    //Catalanian
  {"rus",""},    //Russian
  {"hrv","scr"}, //Croatian
  {"est",""},    //Estonian
  {"dan",""},    //Danish
  {"cze","ces"}, //Czech
  {"tur",""},    //Turkish
  {"und",""}     //Undefined/Manual
};
const char *gLanguageNames[] = {
  "-",
  "English",
  "Deutsch",
  "Slovenski",
  "Italiano",
  "Nederlands",
  "Português",
  "Français",
  "Norsk",
  "suomi", // this is not a typo - it's really lowercase!
  "Polski",
  "Español",
  "ÅëëçíéêÜ",
  "Svenska",
  "Romaneste",
  "Magyar",
  "Català",
  "ÀãááÚØÙ",
  "Hrvatski",
  "Eesti",
  "Dansk",
  "Èesky",
  "Türkçe",
  "Undef/Manual"
};
int gNumLanguages = sizeof(gLanguages) / sizeof(gLanguages[0]);

class cPluginTtxtsubs : public cPlugin, public cStatus, public cVDRTtxtsubsHookListener {
public:
  cPluginTtxtsubs(void);
  virtual ~cPluginTtxtsubs();
  void Reload(void) { StopTtxt(); StartTtxtPlay(0x000); }

  // -- cPlugin
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void);
  virtual const char *MenuSetupPluginEntry() { return tr(DESCRIPTION); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);

  // -- cStatus
 protected:
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
  virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
  //  virtual void OsdClear(void) { ShowTtxt(); }
  //  virtual void OsdTitle(const char *Title) { HideTtxt(); }
  //  virtual void OsdStatusMessage(const char *Message) { HideTtxt(); }
  //  virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue) { HideTtxt(); }
  //  virtual void OsdCurrentItem(const char *Text) { HideTtxt(); }
  //  virtual void OsdTextItem(const char *Text, bool Scroll) { HideTtxt(); }
  //  virtual void OsdChannel(const char *Text) { HideTtxt(); }
  //  virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle) { HideTtxt(); }

  // -- cVDRTtxtsubsHookListener
  virtual void HideOSD(void) { HideTtxt(); }
  virtual void ShowOSD(void) { ShowTtxt(); }
  virtual void PlayerTeletextData(uint8_t *p, int length, bool IsPesRecording, const struct tTeletextSubtitlePage teletextSubtitlePages[], int pageCount);
  virtual int ManualPageNumber(const cChannel *channel);

  // -- internal
 private:
  void StartTtxtPlay(int page);
  void StopTtxt(void);
  void ShowTtxt(void);
  void HideTtxt(void);
  void parseLanguages(const char *val);
  void parseHIs(const char *val);

private:
  cTtxtSubsPlayer *mDispl;
  cMutex mDisplLock;;

  char mOldLanguage[4]; // language chosen from previous version
  int mOldHearingImpaired; // HI setting chosen from previous version
  cTtxtSubsLiveReceiver* mLiveReceiver;
};

class cMenuSetupTtxtsubs : public cMenuSetupPage {
 public:
  cMenuSetupTtxtsubs(cPluginTtxtsubs *ttxtsubs, int doStore = 1);
  ~cMenuSetupTtxtsubs(void);
 protected:
  virtual void Store(void);
 private:
  cPluginTtxtsubs *mTtxtsubs;
  int mLanguageNo[MAXLANGUAGES];
  int mLangHI[MAXLANGUAGES];
  int mSavedFrenchSpecial;
  int mDoStore;
  cTtxtsubsConf mConf;
};


cPluginTtxtsubs::cPluginTtxtsubs(void)
  :
  mDispl(NULL),
  mOldHearingImpaired(0),
  mLiveReceiver(0)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  
  memset(mOldLanguage, 0, 4);
  strncpy(globals.mLanguages[0][0], "unk", 4);
}

cPluginTtxtsubs::~cPluginTtxtsubs()
{
  // Clean up after yourself!
  DELETENULL(mLiveReceiver);
}

const char *cPluginTtxtsubs::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginTtxtsubs::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginTtxtsubs::Start(void)
{
  // Start any background activities the plugin shall perform.

  TtxtSubsChannelSettings.Load(AddDirectory(ConfigDirectory("ttxtsubs"),"channelsettings.dat"));
  if(!memcmp(globals.mLanguages[0][0], "unk", 3)) {
    // no language found in setup
    if(strlen(mOldLanguage)) {
      // use old setup lang, first try to find it amongst known languages
      for(int i = 0; i < gNumLanguages; i++) {
	if(!memcmp(mOldLanguage, gLanguages[i][0], 3) ||
	   !memcmp(mOldLanguage, gLanguages[i][1], 3)) {
	  strncpy(globals.mLanguages[0][0], gLanguages[i][0], 4);
	  globals.mLanguages[0][0][3] = '\0';
	  strncpy(globals.mLanguages[0][1], gLanguages[i][1], 4);
	  globals.mLanguages[0][1][3] = '\0';
	}
      }
      if(!memcmp(globals.mLanguages[0][0], "unk", 3)) {
	// not found there, just copy it
	memcpy(globals.mLanguages[0][0], mOldLanguage, 3);
	globals.mLanguages[0][0][3] = '\0';
      }
      globals.mHearingImpaireds[0][0] = mOldHearingImpaired;
      globals.mHearingImpaireds[0][1] = mOldHearingImpaired;
    } else {
      // get lang from OSD lang
      strncpy(globals.mLanguages[0][0], Setup.OSDLanguage, 4);
      globals.mLanguages[0][0][3] = '\0';
    }
  }

  //dprint("cPluginTtxtsubs::Start\n");

  HookAttach();

  return true;
}

void cPluginTtxtsubs::Stop(void)
{
  StopTtxt();
}

void cPluginTtxtsubs::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

const char *cPluginTtxtsubs::MainMenuEntry(void)
{
  bool haveChannel = Channels.GetByNumber(cDevice::ActualDevice()->CurrentChannel()) != NULL;
  switch(globals.mMainMenuEntry) {
  case 1:
    if(globals.mRealDoDisplay)
      return tr("Hide teletext subtitles");
    else
      return tr("Display teletext subtitles");
  case 2:
      return tr("deprecated");
  case 3:
    if (haveChannel) return tr("Page Selection");
  default:
    return NULL;
  }

  return NULL;
}

cOsdObject *cPluginTtxtsubs::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.

  switch(globals.mMainMenuEntry) {
  case 1:
    globals.mRealDoDisplay = globals.mRealDoDisplay ? 0 : 1;
    return NULL;
  case 3:
    return new cTtxtsubsPageMenu();
  default:
    return NULL;
  }

  return NULL;
}

cMenuSetupPage *cPluginTtxtsubs::SetupMenu(void)
{
  return new cMenuSetupTtxtsubs(this);
}

bool cPluginTtxtsubs::SetupParse(const char *Name, const char *Value)
{
  if(!strcasecmp(Name, "Display")) { globals.mDoDisplay = atoi(Value); globals.mRealDoDisplay=globals.mDoDisplay; }
  else if(!strcasecmp(Name, "Record")) globals.mDoRecord = atoi(Value);
  else if(!strcasecmp(Name, "LiveDelay")) globals.mLiveDelay = atoi(Value);
  else if(!strcasecmp(Name, "ReplayDelay")) globals.mReplayDelay = atoi(Value);
  else if(!strcasecmp(Name, "ReplayTsDelay")) globals.mReplayTsDelay = atoi(Value);
  else if(!strcasecmp(Name, "MainMenuEntry")) globals.mMainMenuEntry = atoi(Value);
  else if(!strcasecmp(Name, "FrenchSpecial")) globals.mFrenchSpecial = atoi(Value);
  else if(!strcasecmp(Name, "DvbSources")) globals.mDvbSources = atoi(Value);
  else if(!strcasecmp(Name, "FontSize")) globals.mFontSize = atoi(Value);
  else if(!strcasecmp(Name, "OutlineWidth")) globals.mOutlineWidth = atoi(Value);
  else if(!strcasecmp(Name, "Languages")) parseLanguages(Value);
  else if(!strcasecmp(Name, "HearingImpaireds")) parseHIs(Value);
  // Handle old settings
  else if(!strcasecmp(Name, "Language")) {
    strncpy(mOldLanguage, Value, 4); mOldLanguage[3] = '\0'; }
  else if(!strcasecmp(Name, "HearingImpaired")) mOldHearingImpaired = atoi(Value);
  else
    return false;
  return true;
}

void cPluginTtxtsubs::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  //dprint("cPluginTtxtsubs::ChannelSwitch(devicenr: %d, channelnr: %d) - mDispl: %x\n",  Device->DeviceNumber(), ChannelNumber, mDispl); // XXX
  if (Device->IsPrimaryDevice() && !Device->Replaying() && ChannelNumber)
  {
    StopTtxt();
    DELETENULL(mLiveReceiver);
    if (!Device->Replaying() && !Device->Transferring())
    {
      cChannel* channel = Channels.GetByNumber(ChannelNumber);
      if (channel && channel->Tpid())
      {
        mLiveReceiver = new cTtxtSubsLiveReceiver(channel, this);
        cDevice::PrimaryDevice()->AttachReceiver(mLiveReceiver);
      }
    }
    StartTtxtPlay(0x000);
  }
}

void cPluginTtxtsubs::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
  //dprint("cPluginTtxtsubs::Replaying\n"); // XXX
  StopTtxt();
  if (On)
  {
    DELETENULL(mLiveReceiver);
    StartTtxtPlay(0x000);
  }
}

void cPluginTtxtsubs::PlayerTeletextData(uint8_t *p, int length, bool IsPesRecording, const struct tTeletextSubtitlePage teletextSubtitlePages[], int pageCount)
{
  mDisplLock.Lock();
  if (mDispl)
    mDispl->PES_data(p, length, IsPesRecording, teletextSubtitlePages, pageCount);
  mDisplLock.Unlock();
}

int cPluginTtxtsubs::ManualPageNumber(const cChannel *channel)
{
    cTtxtSubsChannelSetting *setting = TtxtSubsChannelSettings.Get(channel);
    if (setting && setting->PageMode() == PAGE_MODE_MANUAL)
      return setting->PageNumber();
    else
      return 0;
}

// --  internal

void cPluginTtxtsubs::StartTtxtPlay(int backup_page)
{
  //dprint("cPluginTtxtsubs::StartTtxtPlay\n");

  if(!mDispl) {
    isyslog("ttxtsubs: teletext subtitles replayer started with initial page %03x", backup_page);
    mDispl = new cTtxtSubsPlayer(backup_page);
    ShowTtxt();
  } else
    esyslog("ttxtsubs: Error: StartTtxtPlay called when already started!");
}

void cPluginTtxtsubs::StopTtxt(void)
{
  //dprint("cPluginTtxtsubs::StopTtxt\n");

  HideTtxt();
  mDisplLock.Lock();
  DELETENULL(mDispl); // takes 0.03-0.04 s
  mDisplLock.Unlock();
}

void cPluginTtxtsubs::ShowTtxt(void)
{
  if(globals.mRealDoDisplay)
    if(mDispl)
      mDispl->ShowDisplay();
}

void cPluginTtxtsubs::HideTtxt(void)
{
  if(mDispl)
    mDispl->HideDisplay();
}

void cPluginTtxtsubs::parseLanguages(const char *val) {
  size_t i;
  const char *p = val;
  const char *p2;
  size_t len;

  for(i = 0; i < (MAXLANGUAGES*2); i++) {
    if(*p == '\0')
      break; // end of string

    p2 = strchr(p, ','); // find end of entry
    if(p2) {
      len = p2 - p;
    } else { // no more , found
      len = strlen(p);
    }

    if(len) {
      size_t trlen = len;
      if(trlen > 3)
	trlen = 3;
      memcpy(globals.mLanguages[i/2][i%2], p, trlen);
      globals.mLanguages[i/2][i%2][trlen] = '\0';
    }

    p += len + 1;
  }
}

void cPluginTtxtsubs::parseHIs(const char *val)
{
  size_t i;

  for(i = 0; i < (MAXLANGUAGES*2); i++) {
    if(val[i] == '\0')
      break;
    globals.mHearingImpaireds[i/2][i%2] = val[i] != '0';
  }
}

// ----- cMenuSetupTtxtsubs -----

//#define TEST
class cMenuSetupTtxtsubsLanguages : public cMenuSetupPage {
 public:
  cMenuSetupTtxtsubsLanguages(cPluginTtxtsubs *ttxtsubs);
 protected:
  virtual void Store(void);
 private:
  cPluginTtxtsubs *mTtxtsubs;
};

cMenuSetupTtxtsubsLanguages::cMenuSetupTtxtsubsLanguages(cPluginTtxtsubs *ttxtsubs)
  :
  mTtxtsubs(ttxtsubs)
{}

void cMenuSetupTtxtsubsLanguages::Store(void)
{
  //dprint("cMenuSetupTtxtsubsLanguages::Store\n");
}

const char * mainMenuAlts[5] = {NULL, NULL, NULL, NULL, NULL};
const char * dvbSources[5];

cMenuSetupTtxtsubs::cMenuSetupTtxtsubs(cPluginTtxtsubs *ttxtsubs, int doStore)
  :
  mTtxtsubs(ttxtsubs),
  mDoStore(doStore),
  mConf(globals)
{
  //static char *mainMenuAlts[] = {"off", "Display on/off", "This menu"};
  // can't get it to store changes in file
  if(mainMenuAlts[0] == NULL) {
    mainMenuAlts[0] = trVDR("off");
    mainMenuAlts[1] = tr("Display on/off");
    mainMenuAlts[2] = tr("4:3/Letterbox (deprecated)");
    mainMenuAlts[3] = tr("Page Mode");
    mainMenuAlts[4] = NULL;

    dvbSources[0] = tr("All");
    dvbSources[1] = tr("Only DVB-S");
    dvbSources[2] = tr("Only DVB-T");
    dvbSources[3] = tr("Only DVB-C");
    dvbSources[4] = NULL;
  }
  const int numMainMenuAlts = sizeof(mainMenuAlts) / sizeof(mainMenuAlts[0]) - 1;
  const int numDvbSources = sizeof(dvbSources) / sizeof(dvbSources[0]) - 1;

  mSavedFrenchSpecial = mConf.mFrenchSpecial;

  for(int n = 0; n < MAXLANGUAGES; n++) {
    mLanguageNo[n] = -1;
    mLangHI[n] = mConf.mHearingImpaireds[n][0];

    for(int i = 0; i < gNumLanguages; i++) {
      if(!strncmp(mConf.mLanguages[n][0], gLanguages[i][0], 4) &&
	 !strncmp(mConf.mLanguages[n][1], gLanguages[i][1], 4)) {
	mLanguageNo[n] = i;
	break;
      }
    }
  }

  Add(new cMenuEditBoolItem(tr("Display Subtitles"), &mConf.mDoDisplay));
  Add(new cMenuEditBoolItem(tr("Record Subtitles"), &mConf.mDoRecord));
  Add(new cMenuEditIntItem(tr("Live Delay"), &mConf.mLiveDelay, 0, 5000));
  Add(new cMenuEditIntItem(tr("Replay Delay (PES)"), &mConf.mReplayDelay, 0, 5000));
  Add(new cMenuEditIntItem(tr("Replay Delay (TS)"), &mConf.mReplayTsDelay, 0, 5000));
  if(mConf.mMainMenuEntry < 0 || mConf.mMainMenuEntry >= numMainMenuAlts)
    mConf.mMainMenuEntry = 0;  // menu item segfaults if out of range
  //Add(new cMenuEditStraItem(tr("Main Menu Alternative"), &mConf.mMainMenuEntry,
  //			    numMainMenuAlts, mainMenuAlts));
  Add(new cMenuEditBoolItem(tr("Workaround for some French chns"),
			    &mConf.mFrenchSpecial));
  if(mConf.mDvbSources < 0 || mConf.mDvbSources >= numDvbSources)
    mConf.mDvbSources = 0;  // menu item segfaults if out of range
  Add(new cMenuEditStraItem(tr("DVB Source Selection"),
                          &mConf.mDvbSources, 4, dvbSources));
  Add(new cMenuEditIntItem(tr("Font Size (pixel)"), &mConf.mFontSize, 10, MAXFONTSIZE * 2));
  Add(new cMenuEditIntItem(tr("Font OutlineWidth (pixel)"), &mConf.mOutlineWidth, 1, 10));

  for(int n = 0; n < MAXLANGUAGES; n++) {
    char str[100];
    const char *allowedc = "abcdefghijklmnopqrstuvwxyz";

    cOsdItem *item = new cOsdItem("--------------------------------------------------------");
    item->SetSelectable(false);
    Add(item);

    sprintf(str, "%s %d", tr("Language"), n + 1);
    if(mLanguageNo[n] >= 0) {
      Add(new cMenuEditStraItem(str, &mLanguageNo[n], gNumLanguages, gLanguageNames));
    } else {
      Add(new cMenuEditStrItem(str, mConf.mLanguages[n][0], 4, allowedc));
      Add(new cMenuEditStrItem(str, mConf.mLanguages[n][1], 4, allowedc));
    }

    sprintf(str, "%s %d %s", tr("Language"), n + 1, tr("Hearing Impaired"));
    Add(new cMenuEditBoolItem(str, &(mConf.mHearingImpaireds[n][0])));
  }

#ifdef TEST
  //AddSubMenu(new cMenuSetupTtxtsubsLanguages(mTtxtsubs));
#endif
}

cMenuSetupTtxtsubs::~cMenuSetupTtxtsubs(void)
{
  if(mTtxtsubs) {
    mTtxtsubs->Reload();
  }
  if(mDoStore) {
    Store();
    Setup.Save(); // Can't get it to write to conf file, menu item disabled.
  }
}

void cMenuSetupTtxtsubs::Store(void)
{
  for(int n=0; n < MAXLANGUAGES; n++) {
    if(mLanguageNo[n] >= 0) {
      strncpy(mConf.mLanguages[n][0], gLanguages[mLanguageNo[n]][0], 4);
      mConf.mLanguages[n][0][3] = '\0';
      strncpy(mConf.mLanguages[n][1], gLanguages[mLanguageNo[n]][1], 4);
      mConf.mLanguages[n][1][3] = '\0';
    }

    mConf.mHearingImpaireds[n][1] = mConf.mHearingImpaireds[n][0];
  }

  SetupStore("Display", mConf.mDoDisplay);
  SetupStore("Record", mConf.mDoRecord);
  SetupStore("LiveDelay", mConf.mLiveDelay);
  SetupStore("ReplayDelay", mConf.mReplayDelay);
  SetupStore("ReplayTsDelay", mConf.mReplayTsDelay);
  SetupStore("FrenchSpecial", mConf.mFrenchSpecial);
  SetupStore("MainMenuEntry", mConf.mMainMenuEntry);
  SetupStore("DvbSources", mConf.mDvbSources);
  SetupStore("FontSize", mConf.mFontSize);
  SetupStore("OutlineWidth", mConf.mOutlineWidth);

  char lstr[MAXLANGUAGES*2*4 + 1];
  char histr[MAXLANGUAGES*2 + 1];
  lstr[0] = '\0';
  histr[0] = '\0';
  for(int n=0; n < MAXLANGUAGES; n++) {
    strncat(lstr, mConf.mLanguages[n][0], 3);
    strcat(lstr, ",");
    strncat(lstr, mConf.mLanguages[n][1], 3);
    if(n != (MAXLANGUAGES - 1))
      strcat(lstr, ",");
      
    strcat(histr, mConf.mHearingImpaireds[n][0] ? "1" : "0");
    strcat(histr, mConf.mHearingImpaireds[n][1] ? "1" : "0");
  }
  SetupStore("Languages", lstr);
  SetupStore("HearingImpaireds", histr);

  globals = mConf;
}


// returns the index to choise number for the given language,
// lower is more preferred. The result is times two, plus one
// if the HI wanted but not found, or -1 if no match.
// Non HI wanted but found is no match.

int cTtxtsubsConf::langChoise(const char *lang, const int HI)
{
  size_t i, j;
  int result = -1;
  
  for(i = 0; i < MAXLANGUAGES; i++) {
    for(j = 0; j < 2; j++) {
      if(!mLanguages[i][j][0])
	continue;

      if(!memcmp(lang, mLanguages[i][j], 3)) {
	if( ( HI && mHearingImpaireds[i][j] ) || 
	    ( !HI && !mHearingImpaireds[i][j] ) ) {
	  result = i*2;
	  goto x;
	}
	if( !HI && mHearingImpaireds[i][j] ) {
	  result = 1 + i*2;
	  goto x;
	}
      }
    }
  }

 x:
  return result;
}


VDRPLUGINCREATOR(cPluginTtxtsubs); // Don't touch this!
