/*
 * config.h: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.h 2.76.1.2 2013/04/27 10:18:08 kls Exp $
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef REELVDR
#include <string>
#else
#include <string.h>
#endif /*REELVDR*/
#include <time.h>
#include <unistd.h>
#include "i18n.h"
#include "font.h"
#include "tools.h"

// VDR's own version number:

#define VDRVERSION  "2.0.2.9"
#define VDRVERSNUM   20002  // Version * 10000 + Major * 100 + Minor

// The plugin API's version number:

#define APIVERSION  "2.0.2.9"
#define APIVERSNUM   20002  // Version * 10000 + Major * 100 + Minor

// When loading plugins, VDR searches them by their APIVERSION, which
// may be smaller than VDRVERSION in case there have been no changes to
// VDR header files since the last APIVERSION. This allows compiled
// plugins to work with newer versions of the core VDR as long as no
// VDR header files have changed.

#define JUMPPLAYVERSNUM 110

// The MainMenuHook Patch's version number:
#define MAINMENUHOOKSVERSION "1.0.1"
#define MAINMENUHOOKSVERSNUM 10001  // Version * 10000 + Major * 100 + Minor

#define REMOTEINSTANTVERSION 1.0

#define MAXPRIORITY       99
#define MINPRIORITY       (-MAXPRIORITY)
#define LIVEPRIORITY      0                  // priority used when selecting a device for live viewing
#define TRANSFERPRIORITY  (LIVEPRIORITY - 1) // priority used for actual local Transfer Mode
#define IDLEPRIORITY      (MINPRIORITY - 1)  // priority of an idle device
#define MAXLIFETIME       99
#define DEFINSTRECTIME    180 // default instant recording time (minutes)

#define TIMERMACRO_TITLE    "TITLE"
#define TIMERMACRO_EPISODE  "EPISODE"

#define MINOSDWIDTH   480
#define MAXOSDWIDTH  1920
#define MINOSDHEIGHT  324
#define MAXOSDHEIGHT 1200

#define MaxFileName NAME_MAX // obsolete - use NAME_MAX directly instead!
#define MaxSkinName 16
#define MaxThemeName 16

// Basically VDR works according to the DVB standard, but there are countries/providers
// that use other standards, which in some details deviate from the DVB standard.
// This makes it necessary to handle things differently in some areas, depending on
// which standard is actually used. The following macros are used to distinguish
// these cases (make sure to adjust cMenuSetupDVB::standardComplianceTexts accordingly
// when adding a new standard):

#define STANDARD_DVB       0
#define STANDARD_ANSISCTE  1

typedef uint32_t in_addr_t; //XXX from /usr/include/netinet/in.h (apparently this is not defined on systems with glibc < 2.2)

#ifdef REELVDR
#define MAXHOSTIP 16
#define MAXHOSTNAME 64
#define MACLENGTH 18

enum eReelboxMode { eModeStandalone=0, eModeClient=1, eModeServer=2, eModeHotel=3 };
enum eReceiverType { eModeDVB = 0, eModeMcli = 1, eModeStreamdev = 2 };

void CopyToTftpRoot(const char* filename);

namespace setup
{
  extern std::string FileNameFactory(std::string FileType);
}

#endif /* REELVDR */

class cSVDRPhost : public cListObject {
private:
  struct in_addr addr;
  in_addr_t mask;
public:
  cSVDRPhost(void);
  bool Parse(const char *s);
  bool IsLocalhost(void);
  bool Accepts(in_addr_t Address);
  };

class cSatCableNumbers {
private:
  int size;
  int *array;
public:
  cSatCableNumbers(int Size, const char *s = NULL);
  ~cSatCableNumbers();
  int Size(void) const { return size; }
  int *Array(void) { return array; }
  bool FromString(const char *s);
  cString ToString(void);
  int FirstDeviceIndex(int DeviceIndex) const;
      ///< Returns the first device index (starting at 0) that uses the same
      ///< sat cable number as the device with the given DeviceIndex.
      ///< If the given device does not use the same sat cable as any other device,
      ///< or if the resulting value would be the same as DeviceIndex,
      ///< or if DeviceIndex is out of range, -1 is returned.
  };

template<class T> class cConfig : public cList<T> {
public:
  char *fileName;
  bool allowComments;
  void Clear(void)
  {
    free(fileName);
    fileName = NULL;
    cList<T>::Clear();
  }
public:
  cConfig(void) { fileName = NULL; }
  virtual ~cConfig() { free(fileName); }
  const char *FileName(void) { return fileName; }
  bool Load(const char *FileName = NULL, bool AllowComments = false, bool MustExist = false)
  {
    cConfig<T>::Clear();
    if (FileName) {
       free(fileName);
       fileName = strdup(FileName);
       allowComments = AllowComments;
       }
    bool result = !MustExist;
    if (fileName && access(fileName, F_OK) == 0) {
       isyslog("loading %s", fileName);
       FILE *f = fopen(fileName, "r");
       if (f) {
          char *s;
          int line = 0;
          cReadLine ReadLine;
          result = true;
          while ((s = ReadLine.Read(f)) != NULL) {
                line++;
                if (allowComments) {
                   char *p = strchr(s, '#');
                   if (p)
                      *p = 0;
                   }
                stripspace(s);
                if (!isempty(s)) {
                   T *l = new T;
                   if (l->Parse(s))
                      this->Add(l);
                   else {
                      esyslog("ERROR: error in %s, line %d", fileName, line);
                      delete l;
                      result = false;
                      }
                   }
                }
          fclose(f);
          }
       else {
          LOG_ERROR_STR(fileName);
          result = false;
          }
       }
    if (!result)
       fprintf(stderr, "vdr: error while reading '%s'\n", fileName);
    return result;
  }
  bool Save(const char* FileName = NULL)
  {
    bool result = true;
    T *l = (T *)this->First();
    cSafeFile f(fileName);
    if (f.Open()) {
       while (l) {
             if (!l->Save(f)) {
                result = false;
                break;
                }
             l = (T *)l->Next();
             }
       if (!f.Close())
          result = false;
#if REELVDR
       CopyToTftpRoot(FileName);
#endif
       }
    else {
       result = false;
       LOG_ERROR_STR(FileName);
       esyslog("could not open file %s", FileName);
    }
    return result;
  }
  };

class cNestedItem : public cListObject {
private:
  char *text;
  cList<cNestedItem> *subItems;
public:
  cNestedItem(const char *Text, bool WithSubItems = false);
  virtual ~cNestedItem();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Text(void) const { return text; }
  cList<cNestedItem> *SubItems(void) { return subItems; }
  void AddSubItem(cNestedItem *Item);
  void SetText(const char *Text);
  void SetSubItems(bool On);
  };

class cNestedItemList : public cList<cNestedItem> {
private:
  char *fileName;
  bool Parse(FILE *f, cList<cNestedItem> *List, int &Line);
  bool Write(FILE *f, cList<cNestedItem> *List, int Indent = 0);
public:
  cNestedItemList(void);
  virtual ~cNestedItemList();
  void Clear(void);
  bool Load(const char *FileName);
  bool Save(void);
  };

class cSVDRPhosts : public cConfig<cSVDRPhost> {
public:
  bool LocalhostOnly(void);
  bool Acceptable(in_addr_t Address);
  };

extern cNestedItemList Folders;
extern cNestedItemList Commands;
extern cNestedItemList RecordingCommands;
extern cSVDRPhosts SVDRPhosts;

class cSetupLine : public cListObject {
private:
  char *plugin;
  char *name;
  char *value;
public:
  cSetupLine(void);
  cSetupLine(const char *Name, const char *Value, const char *Plugin = NULL);
  virtual ~cSetupLine();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Plugin(void) { return plugin; }
  const char *Name(void) { return name; }
  const char *Value(void) { return value; }
  bool Parse(char *s);
  bool Save(FILE *f);
  };

class cSetup : public cConfig<cSetupLine> {
  friend class cPlugin; // needs to be able to call Store()
private:
  void StoreLanguages(const char *Name, int *Values);
  bool ParseLanguages(const char *Value, int *Values);
  bool Parse(const char *Name, const char *Value);
#ifdef REELVDR
public:
  cSetupLine *Get(const char *Name, const char *Plugin = NULL);
private:
#else
  cSetupLine *Get(const char *Name, const char *Plugin = NULL);
#endif
  void Store(const char *Name, const char *Value, const char *Plugin = NULL, bool AllowMultiple = false);
  void Store(const char *Name, int Value, const char *Plugin = NULL);
  void Store(const char *Name, double &Value, const char *Plugin = NULL);
public:
  // Also adjust cMenuSetup (menu.c) when adding parameters here!
  int __BeginData__;
  char OSDLanguage[I18N_MAX_LOCALE_LEN];
  char OSDSkin[MaxSkinName];
  char OSDTheme[MaxThemeName];
  int WarEagleIcons;
  int PrimaryDVB;
  int ShowInfoOnChSwitch;
  int TimeoutRequChInfo;
  int MenuScrollPage;
  int MenuScrollWrap;
  int MenuKeyCloses;
  int MarkInstantRecord;
  char NameInstantRecord[NAME_MAX + 1];
  int InstantRecordTime;
  int LnbSLOF;
  int LnbFrequLo;
  int LnbFrequHi;
  int DiSEqC;
  int SetSystemTime;
  int TimeSource;
  int TimeTransponder;
  int StandardCompliance;
  int MarginStart, MarginStop;
  int JumpSeconds, JumpSecondsSlow, JumpSecondsRepeat;
  int AudioLanguages[I18N_MAX_LANGUAGES + 1];
  int DisplaySubtitles;
  int SupportTeletext;
  int SubtitleLanguages[I18N_MAX_LANGUAGES + 1];
  int SubtitleOffset;
  int SubtitleFgTransparency, SubtitleBgTransparency;
  int EPGLanguages[I18N_MAX_LANGUAGES + 1];
  int EPGScanTimeout;
  int EPGBugfixLevel;
  int EPGLinger;
  int SVDRPTimeout;
  int ZapTimeout;
  int ChannelEntryTimeout;
  int RcRepeatDelay;
  int RcRepeatDelta;
  int DefaultPriority, DefaultLifetime;
  int PausePriority, PauseLifetime;
  int PauseKeyHandling;
  int UseSubtitle;
  int UseVps;
  int VpsMargin;
  int RecordingDirs;
  int FoldersInTimerMenu;
  int AlwaysSortFoldersFirst;
  int NumberKeysForChars;
  int ColorKey0, ColorKey1, ColorKey2, ColorKey3;
  int VideoDisplayFormat;
  int VideoFormat;
  int UpdateChannels;
  int UseDolbyDigital;
  int ChannelInfoPos;
  int ChannelInfoTime;
  double OSDLeftP, OSDTopP, OSDWidthP, OSDHeightP;
  int OSDLeft, OSDTop, OSDWidth, OSDHeight;
  double OSDAspect;
  int OSDMessageTime;
  int UseSmallFont;
  int AntiAlias;
  char FontOsd[MAXFONTNAME];
  char FontSml[MAXFONTNAME];
  char FontFix[MAXFONTNAME];
  double FontOsdSizeP;
  double FontSmlSizeP;
  double FontFixSizeP;
  int FontOsdSize;
  int FontSmlSize;
  int FontFixSize;
  int MaxVideoFileSize;
  int SplitEditedFiles;
  int DelTimeshiftRec;
  int MinEventTimeout, MinUserInactivity;
  time_t NextWakeupTime;
  int MultiSpeedMode;
  int ShowReplayMode;
  int ShowRemainingTime;
  int ProgressDisplayTime;
  int PauseOnMarkSet;
  int ResumeID;
  int JumpPlay;
  int PlayJump;
  int PauseLastMark;
  int CurrentChannel;
  int CurrentVolume;
  int CurrentDolby;
  int InitialVolume;
  int ChannelsWrap;
  int ShowChannelNamesWithSource;
  int EmergencyExit;
#ifdef REELVDR
  int LiveTvOnAvg;
  int PreviewVideos;
  eReceiverType ReceptionMode;
  char NetServerName[MAXHOSTNAME];
  char NetServerMAC[MACLENGTH];
  int MaxMultiRoomClients; // Maximum number of MultiRoom clients served by Reelbox in serverMode
  int ExpertOptions;
  int OSDRandom;
  int OSDRemainTime;
  int OSDUseSymbol;
  int OSDScrollBarWidth;
  int FontSizes;
  int AddNewChannels;
  char NetServerIP[MAXHOSTIP];
  eReelboxMode ReelboxModeTemp; // 0=standalone, 1=Client, 2=Server, 3=HotelMode (for Temporary use)
  eReelboxMode ReelboxMode; // 0=standalone, 1=Client, 2=Server, 3=HotelMode
  int RequestShutDownMode;
  int StandbyOrQuickshutdown;
  int EnergySaveOn; // 1 => MinUserInactivity sends vdr into (hot)standby and then into poweroff
  int StandbyTimeout; //(in mins) after which poweroff;  effective only  when EnergySaveOn is 1, 
  int UseBouquetList;
  int OnlyRadioChannels;
  int OnlyEncryptedChannels;
  int OnlyHDChannels;
  int ExpertNavi;
  int WantChListOnOk;
  int ChannelUpDownKeyMode; // 0 Normal, jumps to next or previous channel
                            // 1 opens bouquet / channel list
  int JumpWidth;
  // should vdr allow plugin to restrict channel list to which it can zap to?
  bool UseZonedChannelList;
  int EPGScanMaxChannel;
  int EPGScanMaxDevices;
  int EPGScanMaxBusyDevices;
  int EPGScanMode;
  int EPGScanDailyTime;
  int EPGScanDailyNext;
#endif /* REELVDR */

  int __EndData__;
  cString InitialChannel;
  cString DeviceBondings;
  cSetup(void);
  cSetup& operator= (const cSetup &s);
  bool Load(const char *FileName);
  bool Save(void);
  };

#ifdef REELVDR
// Additional defines for skinreel3-pi
// FontSizes:
#define FONT_SIZE_USER 0
#define FONT_SIZE_SMALL 1
#define FONT_SIZE_NORMAL 2
#define FONT_SIZE_LARGE 3
#endif

extern cSetup Setup;

#endif //__CONFIG_H
