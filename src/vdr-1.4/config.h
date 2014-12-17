/*
 * config.h: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.h 1.282.1.5 2007/05/12 09:07:16 kls Exp $
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "i18n.h"
#include "tools.h"
#include "font.h"

// VDR's own version number:

#define VDRVERSION  "1.4.29"
#define VDRVERSNUM   10429  // Version * 10000 + Major * 100 + Minor

// The plugin API's version number:

#define APIVERSION  "1.4.29"
#define APIVERSNUM   10429  // Version * 10000 + Major * 100 + Minor

// When loading plugins, VDR searches them by their APIVERSION, which
// may be smaller than VDRVERSION in case there have been no changes to
// VDR header files since the last APIVERSION. This allows compiled
// plugins to work with newer versions of the core VDR as long as no
// VDR header files have changed.

#define VDRNAME  "ReelVDR"
#define VDRDIST  "UNReelVDR"
#define DISTVERSION  "1.4-testing"

#define JUMPPLAYVERSNUM 9

#define MAXPRIORITY 99
#define MAXLIFETIME 99

#define MINOSDWIDTH  480
#define MAXOSDWIDTH  672
#define MINOSDHEIGHT 324
#define MAXOSDHEIGHT 567

#define MaxFileName 256
#define MaxSkinName 16
#define MaxThemeName 16

#define MAXHOSTIP 16
#define MAXHOSTNAME 64
#define MACLENGTH 18

enum eReelboxMode { eModeStandalone=0, eModeClient=1, eModeServer=2, eModeHotel=3 };
enum eReceiverType { eModeDVB = 0, eModeMcli = 1, eModeStreamdev = 2 };

/*calls script with a filename, 
 * The script copies the file to Tftp Root for clients to download.*/
void CopyToTftpRoot(const char* filename);

class cCommand : public cListObject {
private:
  char *title;
  char *command;
  bool confirm;
  static char *result;
public:
  cCommand(void);
  virtual ~cCommand();
  bool Parse(const char *s);
  const char *Title(void) { return title; }
  bool Confirm(void) { return confirm; }
  const char *Execute(const char *Parameters = NULL);
  };

typedef uint32_t in_addr_t; //XXX from /usr/include/netinet/in.h (apparently this is not defined on systems with glibc < 2.2)

class cSVDRPhost : public cListObject {
private:
  struct in_addr addr;
  in_addr_t mask;
public:
  cSVDRPhost(void);
  bool Parse(const char *s);
  bool Accepts(in_addr_t Address);
  };

template<class T> class cConfig : public cList<T> {
protected:
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
  void SetFilename(const char *FileName)
  {
    if (FileName) {
       free(fileName);
       fileName = strdup(FileName);
    }
  }
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
                      Add(l);
                   else {
                      esyslog("ERROR: error in %s, line %d", fileName, line);
                      delete l;
                      //RC: we do not want to exit vdr just because of a simple error
                      //result = false;
                      //break;
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
  bool Save(void)
  {
    dsyslog("Saving configfile %s", fileName);


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
       }
    else
       result = false;

    /* calls a script to do the work, if save was successfull*/
#ifndef RBMINI
   if (result)
	   CopyToTftpRoot(fileName);
#endif

    return result;
  }
  };

class cCommands : public cConfig<cCommand> {};

class cSVDRPhosts : public cConfig<cSVDRPhost> {
public:
  bool Acceptable(in_addr_t Address);
  };

extern cCommands Commands;
extern cCommands RecordingCommands;
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
  void Store(const char *Name, const char *Value, const char *Plugin = NULL, bool AllowMultiple = false);
  void Store(const char *Name, int Value, const char *Plugin = NULL);
public:
  cSetupLine *Get(const char *Name, const char *Plugin = NULL);
  // Also adjust cMenuSetup (menu.c) when adding parameters here!
  int __BeginData__;
  //int OSDLanguage;
  char OSDLanguage[I18N_MAX_LOCALE_LEN];
  char OSDSkin[MaxSkinName];
  char OSDTheme[MaxThemeName];
  int PrimaryDVB;
  int ShowInfoOnChSwitch;
  int WantChListOnOk;
  int TimeoutRequChInfo;
  int MenuScrollPage;
  int MenuScrollWrap;
  int MenuButtonCloses;
  int MarkInstantRecord;
  char NameInstantRecord[MaxFileName];
  int InstantRecordTime;
  int LnbSLOF;
  int LnbFrequLo;
  int LnbFrequHi;
  int DiSEqC;
  int SetSystemTime;
  int TimeSource;
  int TimeTransponder;
  int MarginStart, MarginStop;
  int AudioLanguages[I18N_MAX_LANGUAGES + 1];
  int DisplaySubtitles;
  int SubtitleLanguages[I18N_MAX_LANGUAGES + 1];
  int SubtitleOffset;
  int SubtitleFgTransparency, SubtitleBgTransparency;
  int EPGLanguages[I18N_MAX_LANGUAGES + 1];
  int EPGScanTimeout;
  int EPGBugfixLevel;
  int EPGLinger;
  int SVDRPTimeout;
  int ZapTimeout;
  int PrimaryLimit;
  int DefaultPriority, DefaultLifetime;
  int PausePriority, PauseLifetime;
  int UseSubtitle;
  int UseVps;
  int VpsMargin;
  int RecordingDirs;
  int VideoDisplayFormat;
  int VideoFormat;
  int UpdateChannels;
  int AddNewChannels; //Balaji, in the end (default) / in side its own Bouquet - named after the channel's provider
  int UseDolbyDigital;
  int Ac3OverHdmi;
  int LiveDelay;   //RC: for bitstreamout
  int ReplayDelay; //RC: for bitstreamout
  int PCMDelay;    //RC: for bitstreamout
  int MP2Delay;    //RC: for bitstreamout
  int UseDolbyInRecordings;
  int ChannelInfoPos;
  int ChannelInfoTime;
  int OSDLeft, OSDTop, OSDWidth, OSDHeight;
  int OSDRandom;
  int ExpertNavi;      //RC for Export Buttons/Navigation
  int ExpertOptions;   //RC for Export options
  int OSDMessageTime;
  int OSDRemainTime; //AR; bool for channelInfo 
  int OSDUseSymbol; 
  int OSDScrollBarWidth;
  int UseSmallFont;
  int AntiAlias;
  char FontOsd[MAXFONTNAME];
  char FontSml[MAXFONTNAME];
  char FontFix[MAXFONTNAME];
  int FontOsdSize;
  int FontSmlSize;
  int FontFixSize;
  int FontSizes; // 0 User defined, 3 Large, 1 Small, 2 default
  int MaxVideoFileSize;
  int SplitEditedFiles;
  int MinEventTimeout, MinUserInactivity;
  int RequestShutDownMode;
  int StandbyOrQuickshutdown; // Standby = 0; QuickShutdown = 1; Either Standby or Quickshutdown
  int MultiSpeedMode;
  int ShowReplayMode;
  int ResumeID;
  int JumpPlay;
  int JumpWidth;
  int PlayJump;
  int PauseLastMark;
  int ReloadMarks;
  int CurrentChannel;
  //Start by Klaus
  int CurrentPipChannel;
  int PipIsActive;
  int PipIsRunning; 
  int PipSwitchedToTV;
  int OnlyRadioChannels;
  int OnlyEncryptedChannels;
  int OnlyHDChannels;
  //End by Klaus
  int CurrentVolume;
  int CurrentDolby;
  int InitialChannel;
  int InitialVolume;
  int LiveBuffer;
  int LiveBufferSize;
  int LiveTvOnAvg;
  int CAMEnabled;
  int UseBouquetList;
  char NetServerIP[MAXHOSTIP];
  char NetServerName[MAXHOSTNAME];
  char NetServerMAC[MACLENGTH];
  eReceiverType ReceptionMode;  // 0 = DVB, 1 = mcli-pi, 2 = streamdev-client
  eReelboxMode ReelboxModeTemp; // 0=standalone, 1=Client, 2=Server, 3=HotelMode (for Temporary use)
  eReelboxMode ReelboxMode; // 0=standalone, 1=Client, 2=Server, 3=HotelMode
  int ChannelUpDownKeyMode; // 0 Normal, jumps to next or previous channel
  			    // 1 opens bouquet / channel list
  int __EndData__;
  cSetup(void);
  cSetup& operator= (const cSetup &s);
  bool Load(const char *FileName);
  bool Save(void);
  };

extern cSetup Setup;

#endif //__CONFIG_H
