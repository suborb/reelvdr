/*
 * config.c: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.c 2.22 2012/05/11 11:06:57 kls Exp $
 */

#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include "device.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "recording.h"

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

#define ChkDoublePlausibility(Variable, Default) { if (Variable < 0.00001) Variable = Default; }

#ifdef REELVDR
namespace setup
{
  using namespace std;

  string FileNameFactory(string FileType)
  {
    string configDir = cPlugin::ConfigDirectory();
    string::size_type pos = configDir.find("plugin");
    configDir.erase(pos-1);

#ifdef EXTRA_VERBOSE_DEBUG
    cout << " Config Dir : " << configDir  <<  endl;
#endif

    if (FileType == "configDirecory") // vdr config base directory
    {
#if EXTRA_VERBOSE_DEBUG
       cout << "DEBUG [setup]: ConfigDirectory   " << configDir <<   endl;
#endif
       return configDir;
    }
    if (FileType == "help") // returns symbolic link
    {
       string configFile;
       configFile = cPlugin::ConfigDirectory();
       configFile += "/setup/help/help.";
       string tmp = I18nLanguageCode(I18nCurrentLanguage());
       // if two token given we take the first one.
       string::size_type pos = tmp.find(',');
       if (pos != string::npos)
       {
         configFile += tmp.substr(0,pos);
       }
       else
       {
         configFile += tmp;
       }
       configFile += ".xml";
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " debug config file: " << configFile <<   endl;
#endif
       return configFile;
    }

    //else if (FileType == "channelsFile") // returns  channels.conf
    else if (FileType == "link") // returns symbolic link
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/channels.conf" <<  endl;
#endif
       return configDir += "/channels.conf";
    }
    else if (FileType == "channels") // returns channels dir;
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/channels" <<  endl;
#endif
       return configDir += "/channels";
    }
    else if (FileType == "setup") // returns plugins/setup dir; change to  "configDir"
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/plugins/setup" <<  endl;
#endif
       return configDir += "/plugins/setup";
    }

    configDir.append("/");
    configDir += "/channels/";
    configDir += FileType;
#ifdef EXTRA_VERBOSE_DEBUG
    cout << " Config Dir end  : " << configDir << ".conf"  <<  endl;
#endif

    return configDir += ".conf";
  }
}

#endif /* REELVDR */

// --- cSVDRPhost ------------------------------------------------------------

cSVDRPhost::cSVDRPhost(void)
{
  addr.s_addr = 0;
  mask = 0;
}

bool cSVDRPhost::Parse(const char *s)
{
  mask = 0xFFFFFFFF;
  const char *p = strchr(s, '/');
  if (p) {
     char *error = NULL;
     int m = strtoul(p + 1, &error, 10);
     if (error && *error && !isspace(*error) || m > 32)
        return false;
     *(char *)p = 0; // yes, we know it's 'const' - will be restored!
     if (m == 0)
        mask = 0;
     else {
        mask <<= (32 - m);
        mask = htonl(mask);
        }
     }
  int result = inet_aton(s, &addr);
  if (p)
     *(char *)p = '/'; // there it is again
  return result != 0 && (mask != 0 || addr.s_addr == 0);
}

bool cSVDRPhost::IsLocalhost(void)
{
  return addr.s_addr == htonl(INADDR_LOOPBACK);
}

bool cSVDRPhost::Accepts(in_addr_t Address)
{
  return (Address & mask) == (addr.s_addr & mask);
}

// --- cSatCableNumbers ------------------------------------------------------

cSatCableNumbers::cSatCableNumbers(int Size, const char *s)
{
  size = Size;
  array = MALLOC(int, size);
  FromString(s);
}

cSatCableNumbers::~cSatCableNumbers()
{
  free(array);
}

bool cSatCableNumbers::FromString(const char *s)
{
  char *t;
  int i = 0;
  const char *p = s;
  while (p && *p) {
        int n = strtol(p, &t, 10);
        if (t != p) {
           if (i < size)
              array[i++] = n;
           else {
              esyslog("ERROR: too many sat cable numbers in '%s'", s);
              return false;
              }
           }
        else {
           esyslog("ERROR: invalid sat cable number in '%s'", s);
           return false;
           }
        p = skipspace(t);
        }
  for ( ; i < size; i++)
      array[i] = 0;
  return true;
}

cString cSatCableNumbers::ToString(void)
{
  cString s("");
  for (int i = 0; i < size; i++) {
      s = cString::sprintf("%s%d ", *s, array[i]);
      }
  return s;
}

int cSatCableNumbers::FirstDeviceIndex(int DeviceIndex) const
{
  if (0 <= DeviceIndex && DeviceIndex < size) {
     if (int CableNr = array[DeviceIndex]) {
        for (int i = 0; i < size; i++) {
            if (i < DeviceIndex && array[i] == CableNr)
               return i;
            }
        }
     }
  return -1;
}

// --- cNestedItem -----------------------------------------------------------

cNestedItem::cNestedItem(const char *Text, bool WithSubItems)
{
  text = strdup(Text ? Text : "");
  subItems = WithSubItems ? new cList<cNestedItem> : NULL;
}

cNestedItem::~cNestedItem()
{
  delete subItems;
  free(text);
}

int cNestedItem::Compare(const cListObject &ListObject) const
{
  return strcasecmp(text, ((cNestedItem *)&ListObject)->text);
}

void cNestedItem::AddSubItem(cNestedItem *Item)
{
  if (!subItems)
     subItems = new cList<cNestedItem>;
  if (Item)
     subItems->Add(Item);
}

void cNestedItem::SetText(const char *Text)
{
  free(text);
  text = strdup(Text ? Text : "");
}

void cNestedItem::SetSubItems(bool On)
{
  if (On && !subItems)
     subItems = new cList<cNestedItem>;
  else if (!On && subItems) {
     delete subItems;
     subItems = NULL;
     }
}

// --- cNestedItemList -------------------------------------------------------

cNestedItemList::cNestedItemList(void)
{
  fileName = NULL;
}

cNestedItemList::~cNestedItemList()
{
  free(fileName);
}

bool cNestedItemList::Parse(FILE *f, cList<cNestedItem> *List, int &Line)
{
  char *s;
  cReadLine ReadLine;
  while ((s = ReadLine.Read(f)) != NULL) {
        Line++;
        char *p = strchr(s, '#');
        if (p)
           *p = 0;
        s = skipspace(stripspace(s));
        if (!isempty(s)) {
           p = s + strlen(s) - 1;
           if (*p == '{') {
              *p = 0;
              stripspace(s);
              cNestedItem *Item = new cNestedItem(s, true);
              List->Add(Item);
              if (!Parse(f, Item->SubItems(), Line))
                 return false;
              }
           else if (*s == '}')
              break;
           else
              List->Add(new cNestedItem(s));
           }
        }
  return true;
}

bool cNestedItemList::Write(FILE *f, cList<cNestedItem> *List, int Indent)
{
  for (cNestedItem *Item = List->First(); Item; Item = List->Next(Item)) {
      if (Item->SubItems()) {
         fprintf(f, "%*s%s {\n", Indent, "", Item->Text());
         Write(f, Item->SubItems(), Indent + 2);
         fprintf(f, "%*s}\n", Indent + 2, "");
         }
      else
         fprintf(f, "%*s%s\n", Indent, "", Item->Text());
      }
  return true;
}

void cNestedItemList::Clear(void)
{
  free(fileName);
  fileName = NULL;
  cList<cNestedItem>::Clear();
}

bool cNestedItemList::Load(const char *FileName)
{
  cList<cNestedItem>::Clear();
  if (FileName) {
     free(fileName);
     fileName = strdup(FileName);
     }
  bool result = false;
  if (fileName && access(fileName, F_OK) == 0) {
     isyslog("loading %s", fileName);
     FILE *f = fopen(fileName, "r");
     if (f) {
        int Line = 0;
        result = Parse(f, this, Line);
        fclose(f);
        }
     else {
        LOG_ERROR_STR(fileName);
        result = false;
        }
     }
  return result;
}

bool cNestedItemList::Save(void)
{
  bool result = true;
  cSafeFile f(fileName);
  if (f.Open()) {
     result = Write(f, this);
     if (!f.Close())
        result = false;
     }
  else
     result = false;
  return result;
}

// --- Folders and Commands --------------------------------------------------

cNestedItemList Folders;
cNestedItemList Commands;
cNestedItemList RecordingCommands;

// --- cSVDRPhosts -----------------------------------------------------------

cSVDRPhosts SVDRPhosts;

bool cSVDRPhosts::LocalhostOnly(void)
{
  cSVDRPhost *h = First();
  while (h) {
        if (!h->IsLocalhost())
           return false;
        h = (cSVDRPhost *)h->Next();
        }
  return true;
}

bool cSVDRPhosts::Acceptable(in_addr_t Address)
{
  cSVDRPhost *h = First();
  while (h) {
        if (h->Accepts(Address))
           return true;
        h = (cSVDRPhost *)h->Next();
        }
  return false;
}

// --- cSetupLine ------------------------------------------------------------

cSetupLine::cSetupLine(void)
{
  plugin = name = value = NULL;
}

cSetupLine::cSetupLine(const char *Name, const char *Value, const char *Plugin)
{
  name = strreplace(strdup(Name), '\n', 0);
  value = strreplace(strdup(Value), '\n', 0);
  plugin = Plugin ? strreplace(strdup(Plugin), '\n', 0) : NULL;
}

cSetupLine::~cSetupLine()
{
  free(plugin);
  free(name);
  free(value);
}

int cSetupLine::Compare(const cListObject &ListObject) const
{
  const cSetupLine *sl = (cSetupLine *)&ListObject;
  if (!plugin && !sl->plugin)
     return strcasecmp(name, sl->name);
  if (!plugin)
     return -1;
  if (!sl->plugin)
     return 1;
  int result = strcasecmp(plugin, sl->plugin);
  if (result == 0)
     result = strcasecmp(name, sl->name);
  return result;
}

bool cSetupLine::Parse(char *s)
{
  char *p = strchr(s, '=');
  if (p) {
     *p = 0;
     char *Name  = compactspace(s);
     char *Value = compactspace(p + 1);
     if (*Name) { // value may be an empty string
        p = strchr(Name, '.');
        if (p) {
           *p = 0;
           char *Plugin = compactspace(Name);
           Name = compactspace(p + 1);
           if (!(*Plugin && *Name))
              return false;
           plugin = strdup(Plugin);
           }
        name = strdup(Name);
        value = strdup(Value);
        return true;
        }
     }
  return false;
}

bool cSetupLine::Save(FILE *f)
{
  return fprintf(f, "%s%s%s = %s\n", plugin ? plugin : "", plugin ? "." : "", name, value) > 0;
}

// --- cSetup ----------------------------------------------------------------

cSetup Setup;

cSetup::cSetup(void)
{
  strcpy(OSDLanguage, ""); // default is taken from environment
  strcpy(OSDSkin, "sttng");
  strcpy(OSDTheme, "default");
#ifdef USE_VALIDINPUT
  ShowValidInput = 0;
#endif /* VALIDINPUT */
#ifdef USE_WAREAGLEICON
  WarEagleIcons = 1;
#endif /* WAREAGLEICON */
  PrimaryDVB = 1;
  ShowInfoOnChSwitch = 1;
  TimeoutRequChInfo = 1;
  MenuScrollPage = 1;
  MenuScrollWrap = 0;
  MenuKeyCloses = 0;
  MarkInstantRecord = 1;
  strcpy(NameInstantRecord, "TITLE EPISODE");
  InstantRecordTime = 180;
  LnbSLOF    = 11700;
  LnbFrequLo =  9750;
  LnbFrequHi = 10600;
  DiSEqC = 0;
  SetSystemTime = 0;
  TimeSource = 0;
  TimeTransponder = 0;
  StandardCompliance = STANDARD_DVB;
  MarginStart = 2;
  MarginStop = 10;
#ifdef USE_JUMPINGSECONDS
  JumpSeconds = 60;
  JumpSecondsSlow = 10;
  JumpSecondsRepeat = 300;
#endif /* JUMPINGSECONDS */
  AudioLanguages[0] = -1;
  DisplaySubtitles = 0;
#ifdef USE_TTXTSUBS
  SupportTeletext = 0;
#endif /* TTXTSUBS */
  SubtitleLanguages[0] = -1;
  SubtitleOffset = 0;
  SubtitleFgTransparency = 0;
  SubtitleBgTransparency = 0;
  EPGLanguages[0] = -1;
#ifdef USE_DDEPGENTRY
  DoubleEpgTimeDelta = 15;
  DoubleEpgAction = 0;
  MixEpgAction = 0;
  DisableVPS = 0;
#endif /* DDEPGENTRY */
  EPGScanTimeout = 5;
#ifdef REELVDR
  EPGScanMaxChannel = 0;
  EPGScanMaxDevices = 0;
  EPGScanMaxBusyDevices = 0;
  EPGScanMode = 0;
  EPGScanDailyTime = 0;
  EPGScanDailyNext = 0;
#endif
  EPGBugfixLevel = 3;
  EPGLinger = 0;
  SVDRPTimeout = 300;
  ZapTimeout = 3;
  ChannelEntryTimeout = 1000;
  DefaultPriority = 50;
  DefaultLifetime = MAXLIFETIME;
  PauseKeyHandling = 2;
  PausePriority = 10;
  PauseLifetime = 1;
  UseSubtitle = 1;
  UseVps = 0;
  VpsMargin = 120;
  RecordingDirs = 1;
  FoldersInTimerMenu = 1;
  NumberKeysForChars = 1;
  VideoDisplayFormat = 1;
  VideoFormat = 0;
  UpdateChannels = 5;
#ifdef USE_CHANNELBIND
  ChannelBindingByRid = 0;
#endif /* CHANNELBIND */
  UseDolbyDigital = 1;
  ChannelInfoPos = 0;
  ChannelInfoTime = 5;
#ifdef REELVDR
  OSDLeftP = 0.05;
  OSDTopP = 0.05;
  OSDWidthP = 0.9;
  OSDHeightP = 0.9;
  OSDLeft = 36;
  OSDTop = 29;
  OSDWidth = 648;
  OSDHeight = 518;
#else
  OSDLeftP = 0.08;
  OSDTopP = 0.08;
  OSDWidthP = 0.87;
  OSDHeightP = 0.84;
  OSDLeft = 54;
  OSDTop = 45;
  OSDWidth = 624;
  OSDHeight = 486;
#endif /*REELVDR*/
  OSDAspect = 1.0;
  OSDMessageTime = 1;
  UseSmallFont = 1;
  AntiAlias = 1;
  strcpy(FontOsd, DefaultFontOsd);
  strcpy(FontSml, DefaultFontSml);
  strcpy(FontFix, DefaultFontFix);
#ifdef REELVDR
  FontOsdSizeP = 0.031;
  FontSmlSizeP = 0.024;
  FontFixSizeP = 0.027;
  FontOsdSize = 18;
  FontSmlSize = 14;
  FontFixSize = 16;
#else
  FontOsdSizeP = 0.038;
  FontSmlSizeP = 0.035;
  FontFixSizeP = 0.031;
  FontOsdSize = 22;
  FontSmlSize = 18;
  FontFixSize = 20;
#endif
  MaxVideoFileSize = MAXVIDEOFILESIZEDEFAULT;
#ifdef USE_HARDLINKCUTTER
  MaxRecordingSize = DEFAULTRECORDINGSIZE;
  HardLinkCutter = 0;
#endif /* HARDLINKCUTTER */
  SplitEditedFiles = 0;
  DelTimeshiftRec = 0;
  MinEventTimeout = 30;
  MinUserInactivity = 300;
  NextWakeupTime = 0;
  MultiSpeedMode = 0;
  ShowReplayMode = 0;
  ShowRemainingTime = 0;
  ResumeID = 0;
#ifdef USE_JUMPPLAY
  JumpPlay = 0;
  PlayJump = 0;
  PauseLastMark = 0;
  ReloadMarks = 0;
#endif /* JUMPPLAY */
  CurrentChannel = -1;
  CurrentVolume = MAXVOLUME;
  CurrentDolby = 0;
#ifdef USE_CHANNELPROVIDE
  LocalChannelProvide = 1;
#endif /* CHANNELPROVIDE */
  InitialChannel = "";
  DeviceBondings = "";
  InitialVolume = -1;
#ifdef USE_VOLCTRL
  LRVolumeControl = 0;
  LRChannelGroups = 1;
  LRForwardRewind = 1;
#endif /* VOLCTRL */
  ChannelsWrap = 0;
  EmergencyExit = 0;
#ifdef USE_LIEMIEXT
  ShowRecDate = 1;
  ShowRecTime = 1;
  ShowRecLength = 0;
  ShowProgressBar = 0;
  MenuCmdPosition = 0;
#endif /* LIEMIEXT */
#ifdef USE_LIRCSETTINGS
  LircRepeatDelay = 350;
  LircRepeatFreq = 100;
  LircRepeatTimeout = 500;
#endif /* LIRCSETTINGS */
#ifdef USE_LNBSHARE
  VerboseLNBlog = 0;
  for (int i = 0; i < MAXDEVICES; i++) CardUsesLnbNr[i] = i + 1;
#endif /* LNBSHARE */
#ifdef USE_NOEPG
  noEPGMode = 0;
  noEPGList = NULL;
#endif /* NOEPG */
#ifdef USE_DVLVIDPREFER
  UseVidPrefer = 0; // default = disabled
  nVidPrefer = 1;
  for (int zz = 1; zz < DVLVIDPREFER_MAX; zz++) {
      VidPreferPrio[ zz ] = 50;
      VidPreferSize[ zz ] = 100;
      }
  VidPreferSize[ 0 ] = 800;
  VidPreferPrio[ 0 ] = 50;
#endif /* DVLVIDPREFER */
#ifdef USE_LIVEBUFFER
  LiveBuffer = 0;
  LiveBufferSize = 30;
  LiveBufferMaxFileSize = MAXVIDEOFILESIZEDEFAULT/2;
#endif /*USE_LIVEBUFFER*/
#ifdef REELVDR
  LiveTvOnAvg = 1;
  PreviewVideos = 0;
  ReceptionMode = eModeMcli;
  strcpy(NetServerName, "");
  strcpy(NetServerMAC, "");
  MaxMultiRoomClients = 1;
  ExpertOptions  = 1;
  OSDRandom = 0;
  OSDRemainTime = 0;
  OSDUseSymbol = 1;
  OSDScrollBarWidth = 5;
  AddNewChannels = 0;
  strcpy(NetServerIP, "");
  ReelboxModeTemp = ReelboxMode = eModeStandalone;
  RequestShutDownMode = 1;
  StandbyOrQuickshutdown = 0; // Standby = 0; QuickShutdown = 1;
  EnergySaveOn = 1;           // default off (taken from setup plugin)
  StandbyTimeout = 120;       // 2 hours (taken from setup plugin)
  UseBouquetList = 1;
  OnlyRadioChannels = 0;
  OnlyEncryptedChannels = 0;
  OnlyHDChannels = 0;
  ExpertNavi     = 1;
  WantChListOnOk = 1;
  ChannelUpDownKeyMode = 0; // 0 Normal, kChanUp and kChanDn jumps channel
                            // 1 opens bouquet / channellist
  JumpWidth = 1;
  UseZonedChannelList = false;
#endif /* REELVDR */
}

#ifdef USE_NOEPG
cSetup::~cSetup()
{
  if (noEPGList)
     free(noEPGList);
}
#endif /* NOEPG */

cSetup& cSetup::operator= (const cSetup &s)
{
  memcpy(&__BeginData__, &s.__BeginData__, (char *)&s.__EndData__ - (char *)&s.__BeginData__);
  InitialChannel = s.InitialChannel;
  DeviceBondings = s.DeviceBondings;
#ifdef USE_NOEPG
  if (noEPGList) {
     free(noEPGList);
     noEPGList = NULL;
     }
  if (s.noEPGList)
     noEPGList = strdup(s.noEPGList);
#endif /* NOEPG */
  return *this;
}

cSetupLine *cSetup::Get(const char *Name, const char *Plugin)
{
  for (cSetupLine *l = First(); l; l = Next(l)) {
      if ((l->Plugin() == NULL) == (Plugin == NULL)) {
         if ((!Plugin || strcasecmp(l->Plugin(), Plugin) == 0) && strcasecmp(l->Name(), Name) == 0)
            return l;
         }
      }
  return NULL;
}

void cSetup::Store(const char *Name, const char *Value, const char *Plugin, bool AllowMultiple)
{
  if (Name && *Name) {
     cSetupLine *l = Get(Name, Plugin);
     if (l && !AllowMultiple)
        Del(l);
     if (Value)
        Add(new cSetupLine(Name, Value, Plugin));
     }
}

void cSetup::Store(const char *Name, int Value, const char *Plugin)
{
  Store(Name, cString::sprintf("%d", Value), Plugin);
}

void cSetup::Store(const char *Name, double &Value, const char *Plugin)
{
  Store(Name, cString::sprintf("%f", Value), Plugin);
}

bool cSetup::Load(const char *FileName)
{
  if (cConfig<cSetupLine>::Load(FileName, true)) {
     bool result = true;
     for (cSetupLine *l = First(); l; l = Next(l)) {
         bool error = false;
         if (l->Plugin()) {
            cPlugin *p = cPluginManager::GetPlugin(l->Plugin());
            if (p && !p->SetupParse(l->Name(), l->Value()))
               error = true;
            }
         else {
            if (!Parse(l->Name(), l->Value()))
               error = true;
            }
         if (error) {
#if REELVDR
            isyslog("INFO: unknown (obsolete?) config parameter: %s%s%s = %s", l->Plugin() ? l->Plugin() : "", l->Plugin() ? "." : "", l->Name(), l->Value());
#else
            esyslog("ERROR: unknown config parameter: %s%s%s = %s", l->Plugin() ? l->Plugin() : "", l->Plugin() ? "." : "", l->Name(), l->Value());
#endif
            result = false;
            }
         }
     return result;
     }
  return false;
}

void cSetup::StoreLanguages(const char *Name, int *Values)
{
  char buffer[I18nLanguages()->Size() * 4];
  char *q = buffer;
  for (int i = 0; i < I18nLanguages()->Size(); i++) {
      if (Values[i] < 0)
         break;
      const char *s = I18nLanguageCode(Values[i]);
      if (s) {
         if (q > buffer)
            *q++ = ' ';
         strncpy(q, s, 3);
         q += 3;
         }
      }
  *q = 0;
  Store(Name, buffer);
}

bool cSetup::ParseLanguages(const char *Value, int *Values)
{
  int n = 0;
  while (Value && *Value && n < I18nLanguages()->Size()) {
        char buffer[4];
        strn0cpy(buffer, Value, sizeof(buffer));
        int i = I18nLanguageIndex(buffer);
        if (i >= 0)
           Values[n++] = i;
        if ((Value = strchr(Value, ' ')) != NULL)
           Value++;
        }
  Values[n] = -1;
  return true;
}

bool cSetup::Parse(const char *Name, const char *Value)
{
  if      (!strcasecmp(Name, "OSDLanguage"))       { strn0cpy(OSDLanguage, Value, sizeof(OSDLanguage)); I18nSetLocale(OSDLanguage); }
#ifdef USE_LIVEBUFFER
  else if (!strcasecmp(Name, "LiveBuffer"))            LiveBuffer            = atoi(Value);
  else if (!strcasecmp(Name, "LiveBufferSize"))        LiveBufferSize        = atoi(Value);
  else if (!strcasecmp(Name, "LiveBufferMaxFileSize")) LiveBufferMaxFileSize = atoi(Value);
#endif /*USE_LIVEBUFFER*/
#ifdef REELVDR
  else if (!strcasecmp(Name, "LiveTvOnAvg"))         LiveTvOnAvg        = atoi(Value);
  else if (!strcasecmp(Name, "ReceptionMode"))       ReceptionMode      = (eReceiverType)atoi(Value);
  else if (!strcasecmp(Name, "NetServerName"))       Utf8Strn0Cpy(NetServerName, Value, MAXHOSTNAME);
  else if (!strcasecmp(Name, "NetServerMAC"))        Utf8Strn0Cpy(NetServerMAC, Value, MACLENGTH);
  else if (!strcasecmp(Name, "MaxMultiRoomClients")) MaxMultiRoomClients = atoi(Value);
  else if (!strcasecmp(Name, "ExpertOptions"))       ExpertOptions      = atoi(Value);
  else if (!strcasecmp(Name, "OSDRandom"))           OSDRandom          = atoi(Value);
  else if (!strcasecmp(Name, "OSDRemainTime"))       OSDRemainTime      = atoi(Value);
  else if (!strcasecmp(Name, "OSDUseSymbol"))        OSDUseSymbol       = atoi(Value);
  else if (!strcasecmp(Name, "OSDScrollBarWidth"))   OSDScrollBarWidth  = atoi(Value);
  else if (!strcasecmp(Name, "FontSizes"))           FontSizes          = atoi(Value);
  else if (!strcasecmp(Name, "AddNewChannels"))      AddNewChannels     = atoi(Value);
  else if (!strcasecmp(Name, "NetServerIP"))         Utf8Strn0Cpy(NetServerIP, Value, MAXHOSTIP);
  else if (!strcasecmp(Name, "ReelboxMode"))         ReelboxMode        = (eReelboxMode)atoi(Value);
  else if (!strcasecmp(Name, "RequestShutDownMode")) RequestShutDownMode= atoi(Value);
  else if (!strcasecmp(Name, "StandbyOrQuickshutdown")) StandbyOrQuickshutdown  = atoi(Value);
  else if (!strcasecmp(Name, "UseBouquetList"))      UseBouquetList     = atoi(Value);
  else if (!strcasecmp(Name, "OnlyRadioChannels"))   OnlyRadioChannels  = atoi(Value);
  else if (!strcasecmp(Name, "OnlyEncryptedChannels"))OnlyEncryptedChannels = atoi(Value);
  else if (!strcasecmp(Name, "OnlyHDChannels"))      OnlyHDChannels       = atoi(Value);
  else if (!strcasecmp(Name, "ExpertNavi"))          ExpertNavi         = atoi(Value);
  else if (!strcasecmp(Name, "WantChListOnOk"))      WantChListOnOk     = atoi(Value);
  else if (!strcasecmp(Name, "ChannelUpDownKeyMode"))  ChannelUpDownKeyMode = atoi(Value);
  else if (!strcasecmp(Name, "JumpWidth"))           JumpWidth          = atoi(Value);
  else if (!strcasecmp(Name, "UseZonedChannelList"))  UseZonedChannelList    = atoi(Value);
#endif /* REELVDR */
  else if (!strcasecmp(Name, "OSDSkin"))             Utf8Strn0Cpy(OSDSkin, Value, MaxSkinName);
  else if (!strcasecmp(Name, "OSDTheme"))            Utf8Strn0Cpy(OSDTheme, Value, MaxThemeName);
#ifdef USE_VALIDINPUT
  else if (!strcasecmp(Name, "ShowValidInput"))      ShowValidInput     = atoi(Value);
#endif /* VALIDINPUT */
#ifdef USE_WAREAGLEICON
  else if (!strcasecmp(Name, "WarEagleIcons"))       WarEagleIcons      = atoi(Value);
#endif /* WAREAGLEICON */
  else if (!strcasecmp(Name, "PrimaryDVB"))          PrimaryDVB         = atoi(Value);
  else if (!strcasecmp(Name, "ShowInfoOnChSwitch"))  ShowInfoOnChSwitch = atoi(Value);
  else if (!strcasecmp(Name, "TimeoutRequChInfo"))   TimeoutRequChInfo  = atoi(Value);
  else if (!strcasecmp(Name, "MenuScrollPage"))      MenuScrollPage     = atoi(Value);
  else if (!strcasecmp(Name, "MenuScrollWrap"))      MenuScrollWrap     = atoi(Value);
  else if (!strcasecmp(Name, "MenuKeyCloses"))       MenuKeyCloses      = atoi(Value);
  else if (!strcasecmp(Name, "MarkInstantRecord"))   MarkInstantRecord  = atoi(Value);
  else if (!strcasecmp(Name, "NameInstantRecord"))   Utf8Strn0Cpy(NameInstantRecord, Value, MaxFileName);
  else if (!strcasecmp(Name, "InstantRecordTime"))   InstantRecordTime  = atoi(Value);
  else if (!strcasecmp(Name, "LnbSLOF"))             LnbSLOF            = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequLo"))          LnbFrequLo         = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequHi"))          LnbFrequHi         = atoi(Value);
  else if (!strcasecmp(Name, "DiSEqC"))              DiSEqC             = atoi(Value);
  else if (!strcasecmp(Name, "SetSystemTime"))       SetSystemTime      = atoi(Value);
  else if (!strcasecmp(Name, "TimeSource"))          TimeSource         = cSource::FromString(Value);
  else if (!strcasecmp(Name, "TimeTransponder"))     TimeTransponder    = atoi(Value);
  else if (!strcasecmp(Name, "StandardCompliance"))  StandardCompliance = atoi(Value);
  else if (!strcasecmp(Name, "MarginStart"))         MarginStart        = atoi(Value);
  else if (!strcasecmp(Name, "MarginStop"))          MarginStop         = atoi(Value);
#ifdef USE_JUMPINGSECONDS
  else if (!strcasecmp(Name, "JumpSeconds"))         JumpSeconds        = atoi(Value);
  else if (!strcasecmp(Name, "JumpSecondsSlow"))     JumpSecondsSlow    = atoi(Value);
  else if (!strcasecmp(Name, "JumpSecondsRepeat"))   JumpSecondsRepeat  = atoi(Value);
#endif /* JUMPINGSECONDS */
  else if (!strcasecmp(Name, "AudioLanguages"))      return ParseLanguages(Value, AudioLanguages);
#ifdef USE_TTXTSUBS
  else if (!strcasecmp(Name, "DisplaySubtitles"))    { DisplaySubtitles   = atoi(Value);
                                                       SupportTeletext = DisplaySubtitles;
  }
  //else if (!strcasecmp(Name, "SupportTeletext"))     { SupportTeletext    = atoi(Value);
#else
  else if (!strcasecmp(Name, "DisplaySubtitles"))    DisplaySubtitles   = atoi(Value);
#endif /* TTXTSUBS */
  else if (!strcasecmp(Name, "SubtitleLanguages"))   return ParseLanguages(Value, SubtitleLanguages);
  else if (!strcasecmp(Name, "SubtitleOffset"))      SubtitleOffset     = atoi(Value);
  else if (!strcasecmp(Name, "SubtitleFgTransparency")) SubtitleFgTransparency = atoi(Value);
  else if (!strcasecmp(Name, "SubtitleBgTransparency")) SubtitleBgTransparency = atoi(Value);
  else if (!strcasecmp(Name, "EPGLanguages"))        return ParseLanguages(Value, EPGLanguages);
#ifdef USE_DDEPGENTRY
  else if (!strcasecmp(Name, "DoubleEpgTimeDelta"))  DoubleEpgTimeDelta = atoi(Value);
  else if (!strcasecmp(Name, "DoubleEpgAction"))     DoubleEpgAction    = atoi(Value);
  else if (!strcasecmp(Name, "MixEpgAction"))        MixEpgAction       = atoi(Value);
  else if (!strcasecmp(Name, "DisableVPS"))          DisableVPS         = atoi(Value);
#endif /* DDEPGENTRY */
  else if (!strcasecmp(Name, "EPGScanTimeout"))      EPGScanTimeout     = atoi(Value);
#ifdef REELVDR
  else if (!strcasecmp(Name, "EPGScanMaxChannel"))   EPGScanMaxChannel  = atoi(Value);
  else if (!strcasecmp(Name, "EPGScanMaxDevices"))   EPGScanMaxDevices  = atoi(Value);
  else if (!strcasecmp(Name, "EPGScanMaxBusyDevices")) EPGScanMaxBusyDevices = atoi(Value);
  else if (!strcasecmp(Name, "EPGScanMode"))         EPGScanMode        = atoi(Value);
  else if (!strcasecmp(Name, "EPGScanDailyTime"))    EPGScanDailyTime   = atoi(Value);
  else if (!strcasecmp(Name, "EPGScanDailyNext"))    EPGScanDailyNext   = atoi(Value);
#endif
  else if (!strcasecmp(Name, "EPGBugfixLevel"))      EPGBugfixLevel     = atoi(Value);
  else if (!strcasecmp(Name, "EPGLinger"))           EPGLinger          = atoi(Value);
  else if (!strcasecmp(Name, "SVDRPTimeout"))        SVDRPTimeout       = atoi(Value);
  else if (!strcasecmp(Name, "ZapTimeout"))          ZapTimeout         = atoi(Value);
  else if (!strcasecmp(Name, "ChannelEntryTimeout")) ChannelEntryTimeout= atoi(Value);
  else if (!strcasecmp(Name, "DefaultPriority"))     DefaultPriority    = atoi(Value);
  else if (!strcasecmp(Name, "DefaultLifetime"))     DefaultLifetime    = atoi(Value);
  else if (!strcasecmp(Name, "PauseKeyHandling"))    PauseKeyHandling   = atoi(Value);
  else if (!strcasecmp(Name, "PausePriority"))       PausePriority      = atoi(Value);
  else if (!strcasecmp(Name, "PauseLifetime"))       PauseLifetime      = atoi(Value);
  else if (!strcasecmp(Name, "UseSubtitle"))         UseSubtitle        = atoi(Value);
  else if (!strcasecmp(Name, "UseVps"))              UseVps             = atoi(Value);
  else if (!strcasecmp(Name, "VpsMargin"))           VpsMargin          = atoi(Value);
  else if (!strcasecmp(Name, "RecordingDirs"))       RecordingDirs      = atoi(Value);
  else if (!strcasecmp(Name, "FoldersInTimerMenu"))  FoldersInTimerMenu = atoi(Value);
  else if (!strcasecmp(Name, "NumberKeysForChars"))  NumberKeysForChars = atoi(Value);
  else if (!strcasecmp(Name, "VideoDisplayFormat"))  VideoDisplayFormat = atoi(Value);
  else if (!strcasecmp(Name, "VideoFormat"))         VideoFormat        = atoi(Value);
  else if (!strcasecmp(Name, "UpdateChannels"))      UpdateChannels     = atoi(Value);
#ifdef USE_CHANNELBIND
  else if (!strcasecmp(Name, "ChannelBindingByRid")) ChannelBindingByRid= atoi(Value);
#endif /* CHANNELBIND */
  else if (!strcasecmp(Name, "UseDolbyDigital"))     UseDolbyDigital    = atoi(Value);
  else if (!strcasecmp(Name, "ChannelInfoPos"))      ChannelInfoPos     = atoi(Value);
  else if (!strcasecmp(Name, "ChannelInfoTime"))     ChannelInfoTime    = atoi(Value);
#ifdef REELVDR
  else if (!strcasecmp(Name, "OSDLeftP"))          { OSDLeftP           = atof(Value);
                                                     ChkDoublePlausibility(OSDLeftP, 0.05); }
  else if (!strcasecmp(Name, "OSDTopP"))           { OSDTopP            = atof(Value);
                                                     ChkDoublePlausibility(OSDTopP, 0.05); }
  else if (!strcasecmp(Name, "OSDWidthP"))         { OSDWidthP          = atof(Value);
                                                     ChkDoublePlausibility(OSDWidthP, 0.9); }
  else if (!strcasecmp(Name, "OSDHeightP"))        { OSDHeightP         = atof(Value);
                                                     ChkDoublePlausibility(OSDHeightP, 0.9); }
#else
  else if (!strcasecmp(Name, "OSDLeftP"))            OSDLeftP           = atof(Value);
  else if (!strcasecmp(Name, "OSDTopP"))             OSDTopP            = atof(Value);
  else if (!strcasecmp(Name, "OSDWidthP"))         { OSDWidthP          = atof(Value); ChkDoublePlausibility(OSDWidthP, 0.87); }
  else if (!strcasecmp(Name, "OSDHeightP"))        { OSDHeightP         = atof(Value); ChkDoublePlausibility(OSDHeightP, 0.84); }
#endif
  else if (!strcasecmp(Name, "OSDLeft"))             OSDLeft            = atoi(Value);
  else if (!strcasecmp(Name, "OSDTop"))              OSDTop             = atoi(Value);
  else if (!strcasecmp(Name, "OSDWidth"))          { OSDWidth           = atoi(Value); OSDWidth &= ~0x07; } // OSD width must be a multiple of 8
  else if (!strcasecmp(Name, "OSDHeight"))           OSDHeight          = atoi(Value);
  else if (!strcasecmp(Name, "OSDAspect"))           OSDAspect          = atof(Value);
  else if (!strcasecmp(Name, "OSDMessageTime"))      OSDMessageTime     = atoi(Value);
  else if (!strcasecmp(Name, "UseSmallFont"))        UseSmallFont       = atoi(Value);
  else if (!strcasecmp(Name, "AntiAlias"))           AntiAlias          = atoi(Value);
  else if (!strcasecmp(Name, "FontOsd"))             Utf8Strn0Cpy(FontOsd, Value, MAXFONTNAME);
  else if (!strcasecmp(Name, "FontSml"))             Utf8Strn0Cpy(FontSml, Value, MAXFONTNAME);
  else if (!strcasecmp(Name, "FontFix"))             Utf8Strn0Cpy(FontFix, Value, MAXFONTNAME);
#ifdef REELVDR
  else if (!strcasecmp(Name, "FontOsdSizeP"))      { FontOsdSizeP       = atof(Value); ChkDoublePlausibility(FontOsdSizeP, 0.031); }
  else if (!strcasecmp(Name, "FontSmlSizeP"))      { FontSmlSizeP       = atof(Value); ChkDoublePlausibility(FontSmlSizeP, 0.024); }
  else if (!strcasecmp(Name, "FontFixSizeP"))      { FontFixSizeP       = atof(Value); ChkDoublePlausibility(FontFixSizeP, 0.027); }
#else
  else if (!strcasecmp(Name, "FontOsdSizeP"))      { FontOsdSizeP       = atof(Value); ChkDoublePlausibility(FontOsdSizeP, 0.038); }
  else if (!strcasecmp(Name, "FontSmlSizeP"))      { FontSmlSizeP       = atof(Value); ChkDoublePlausibility(FontSmlSizeP, 0.035); }
  else if (!strcasecmp(Name, "FontFixSizeP"))      { FontFixSizeP       = atof(Value); ChkDoublePlausibility(FontFixSizeP, 0.031); }
#endif
  else if (!strcasecmp(Name, "FontOsdSize"))         FontOsdSize        = atoi(Value);
  else if (!strcasecmp(Name, "FontSmlSize"))         FontSmlSize        = atoi(Value);
  else if (!strcasecmp(Name, "FontFixSize"))         FontFixSize        = atoi(Value);
  else if (!strcasecmp(Name, "MaxVideoFileSize"))    MaxVideoFileSize   = atoi(Value);
#ifdef USE_HARDLINKCUTTER
  else if (!strcasecmp(Name, "MaxRecordingSize"))    MaxRecordingSize   = atoi(Value);
  else if (!strcasecmp(Name, "HardLinkCutter"))      HardLinkCutter     = atoi(Value);
#endif /* HARDLINKCUTTER */
  else if (!strcasecmp(Name, "SplitEditedFiles"))    SplitEditedFiles   = atoi(Value);
  else if (!strcasecmp(Name, "DelTimeshiftRec"))     DelTimeshiftRec    = atoi(Value);
  else if (!strcasecmp(Name, "MinEventTimeout"))     MinEventTimeout    = atoi(Value);
  else if (!strcasecmp(Name, "MinUserInactivity"))   MinUserInactivity  = atoi(Value);
  else if (!strcasecmp(Name, "NextWakeupTime"))      NextWakeupTime     = atoi(Value);
  else if (!strcasecmp(Name, "MultiSpeedMode"))      MultiSpeedMode     = atoi(Value);
  else if (!strcasecmp(Name, "ShowReplayMode"))      ShowReplayMode     = atoi(Value);
  else if (!strcasecmp(Name, "ShowRemainingTime"))   ShowRemainingTime  = atoi(Value);
  else if (!strcasecmp(Name, "ResumeID"))            ResumeID           = atoi(Value);
#ifdef USE_JUMPPLAY
  else if (!strcasecmp(Name, "JumpPlay"))            JumpPlay           = atoi(Value);
  else if (!strcasecmp(Name, "PlayJump"))            PlayJump           = atoi(Value);
  else if (!strcasecmp(Name, "PauseLastMark"))       PauseLastMark      = atoi(Value);
  else if (!strcasecmp(Name, "ReloadMarks"))         ReloadMarks        = atoi(Value);
#endif /* JUMPPLAY */
  else if (!strcasecmp(Name, "CurrentChannel"))      CurrentChannel     = atoi(Value);
  else if (!strcasecmp(Name, "CurrentVolume"))       CurrentVolume      = atoi(Value);
  else if (!strcasecmp(Name, "CurrentDolby"))        CurrentDolby       = atoi(Value);
#ifdef USE_CHANNELPROVIDE
  else if (!strcasecmp(Name, "LocalChannelProvide")) LocalChannelProvide = atoi(Value);
#endif /* CHANNELPROVIDE */
  else if (!strcasecmp(Name, "InitialChannel"))      InitialChannel     = Value;
  else if (!strcasecmp(Name, "InitialVolume"))       InitialVolume      = atoi(Value);
  else if (!strcasecmp(Name, "DeviceBondings"))      DeviceBondings     = Value;

#ifdef USE_LIVEBUFFER
   else if (!strcasecmp(Name, "LiveBuffer"))            LiveBuffer            = atoi(Value);
   else if (!strcasecmp(Name, "LiveBufferSize"))        LiveBufferSize        = atoi(Value);
   else if (!strcasecmp(Name, "LiveBufferMaxFileSize")) LiveBufferMaxFileSize = atoi(Value);
#endif /*USE_LIVEBUFFER*/
#ifdef REELVDR
   else if (!strcasecmp(Name, "PreviewVideos"))       PreviewVideos      = atoi(Value);
   else if (!strcasecmp(Name, "LiveTvOnAvg"))         LiveTvOnAvg        = atoi(Value);
   else if (!strcasecmp(Name, "NetServerIP"))         Utf8Strn0Cpy(NetServerIP, Value, MAXHOSTIP);
   else if (!strcasecmp(Name, "NetServerName"))       Utf8Strn0Cpy(NetServerName, Value, MAXHOSTNAME);
   else if (!strcasecmp(Name, "NetServerMAC"))        Utf8Strn0Cpy(NetServerMAC, Value, MACLENGTH);
   else if (!strcasecmp(Name, "ReceptionMode"))       ReceptionMode      = (eReceiverType)atoi(Value);
   else if (!strcasecmp(Name, "ReelboxMode"))         ReelboxMode        = (eReelboxMode)atoi(Value);
   else if (!strcasecmp(Name, "EnergySaveOn"))         EnergySaveOn     = atoi(Value);
   else if (!strcasecmp(Name, "StandbyTimeout"))      StandbyTimeout     = atoi(Value);
#endif

#ifdef USE_VOLCTRL
  else if (!strcasecmp(Name, "LRVolumeControl"))     LRVolumeControl    = atoi(Value);
  else if (!strcasecmp(Name, "LRChannelGroups"))     LRChannelGroups    = atoi(Value);
  else if (!strcasecmp(Name, "LRForwardRewind"))     LRForwardRewind    = atoi(Value);
#endif /* VOLCTRL */
  else if (!strcasecmp(Name, "ChannelsWrap"))        ChannelsWrap       = atoi(Value);
  else if (!strcasecmp(Name, "EmergencyExit"))       EmergencyExit      = atoi(Value);
#ifdef USE_LIEMIEXT
  else if (!strcasecmp(Name, "ShowRecDate"))         ShowRecDate        = atoi(Value);
  else if (!strcasecmp(Name, "ShowRecTime"))         ShowRecTime        = atoi(Value);
  else if (!strcasecmp(Name, "ShowRecLength"))       ShowRecLength      = atoi(Value);
  else if (!strcasecmp(Name, "ShowProgressBar"))     ShowProgressBar    = atoi(Value);
  else if (!strcasecmp(Name, "MenuCmdPosition"))     MenuCmdPosition    = atoi(Value);
#endif /* LIEMIEXT */
#ifdef USE_LIRCSETTINGS
  else if (!strcasecmp(Name, "LircRepeatDelay"))     LircRepeatDelay    = atoi(Value);
  else if (!strcasecmp(Name, "LircRepeatFreq"))      LircRepeatFreq     = atoi(Value);
  else if (!strcasecmp(Name, "LircRepeatTimeout"))   LircRepeatTimeout  = atoi(Value);
#endif /* LIRCSETTINGS */
#ifdef USE_NOEPG
  else if (!strcasecmp(Name, "noEPGMode"))           noEPGMode          = atoi(Value);
  else if (!strcasecmp(Name, "noEPGList")) {
     free(noEPGList);
     noEPGList = Value ? strdup(Value) : NULL;
     }
#endif /* NOEPG */
#ifdef USE_DVLVIDPREFER
  else if (strcasecmp(Name, "UseVidPrefer") == 0)    UseVidPrefer       = atoi(Value);
  else if (strcasecmp(Name, "nVidPrefer") == 0)      nVidPrefer         = atoi(Value);
  else if (strstr(Name, "VidPrefer") == Name) {
     char *x = (char *)&Name[ strlen(Name) - 1 ];
     int vN;

     if (isdigit(*x) != 0) {
        while (isdigit(*x) != 0)
              x--;
        x++;
        }

     vN = atoi(x);
     if (vN < DVLVIDPREFER_MAX) {
        if (strstr(Name, "VidPreferPrio") == Name) {
           VidPreferPrio[ vN ] = atoi(Value);
           if (VidPreferPrio[ vN ] > 99)
              VidPreferPrio[ vN ] = 99;
           }
        else if (strstr(Name, "VidPreferSize") == Name) {
           VidPreferSize[ vN ] = atoi(Value);
           }
        else
           return false;
        }
     }
#endif /* DVLVIDPREFER */
  else
#ifdef USE_LNBSHARE
  if (!strcasecmp(Name, "VerboseLNBlog")) VerboseLNBlog = atoi(Value);
  else {
    char tmp[20];
    bool result = false;
    for (int i = 1; i <= MAXDEVICES; i++) {
      sprintf(tmp, "Card%dusesLNBnr", i);
      if (!strcasecmp(Name, tmp)) {
        CardUsesLnbNr[i - 1] = atoi(Value);
        result = true;
      }
    }
     return result;
  }
#else
     return false;
#endif /* LNBSHARE */
  return true;
}

bool cSetup::Save(void)
{
  Store("OSDLanguage",        OSDLanguage);
  Store("OSDSkin",            OSDSkin);
  Store("OSDTheme",           OSDTheme);
#ifdef USE_VALIDINPUT
  Store("ShowValidInput",     ShowValidInput);
#endif /* VALIDINPUT */
#ifdef USE_WAREAGLEICON
  Store("WarEagleIcons",      WarEagleIcons);
#endif /* WAREAGLEICON */
  Store("PrimaryDVB",         PrimaryDVB);
  Store("ShowInfoOnChSwitch", ShowInfoOnChSwitch);
  Store("TimeoutRequChInfo",  TimeoutRequChInfo);
  Store("MenuScrollPage",     MenuScrollPage);
  Store("MenuScrollWrap",     MenuScrollWrap);
  Store("MenuKeyCloses",      MenuKeyCloses);
  Store("MarkInstantRecord",  MarkInstantRecord);
  Store("NameInstantRecord",  NameInstantRecord);
  Store("InstantRecordTime",  InstantRecordTime);
  Store("LnbSLOF",            LnbSLOF);
  Store("LnbFrequLo",         LnbFrequLo);
  Store("LnbFrequHi",         LnbFrequHi);
  Store("DiSEqC",             DiSEqC);
  Store("SetSystemTime",      SetSystemTime);
  Store("TimeSource",         cSource::ToString(TimeSource));
  Store("TimeTransponder",    TimeTransponder);
  Store("StandardCompliance", StandardCompliance);
  Store("MarginStart",        MarginStart);
  Store("MarginStop",         MarginStop);
#ifdef USE_JUMPINGSECONDS
  Store("JumpSeconds",        JumpSeconds);
  Store("JumpSecondsSlow",    JumpSecondsSlow);
  Store("JumpSecondsRepeat",  JumpSecondsRepeat);
#endif /* JUMPINGSECONDS */
  StoreLanguages("AudioLanguages", AudioLanguages);
  Store("DisplaySubtitles",   DisplaySubtitles);
#ifdef USE_TTXTSUBS
  Store("SupportTeletext",    SupportTeletext);
#endif /* TTXTSUBS */
  StoreLanguages("SubtitleLanguages", SubtitleLanguages);
  Store("SubtitleOffset",     SubtitleOffset);
  Store("SubtitleFgTransparency", SubtitleFgTransparency);
  Store("SubtitleBgTransparency", SubtitleBgTransparency);
  StoreLanguages("EPGLanguages", EPGLanguages);
#ifdef USE_DDEPGENTRY
  Store("DoubleEpgTimeDelta", DoubleEpgTimeDelta);
  Store("DoubleEpgAction",    DoubleEpgAction);
  Store("MixEpgAction",       MixEpgAction);
  Store("DisableVPS",         DisableVPS);
#endif /* DDEPGENTRY */
  Store("EPGScanTimeout",     EPGScanTimeout);
#ifdef REELVDR
  Store("EPGScanMaxChannel",  EPGScanMaxChannel);
  Store("EPGScanMaxDevices",  EPGScanMaxDevices);
  Store("EPGScanMaxBusyDevices", EPGScanMaxBusyDevices);
  Store("EPGScanMode",        EPGScanMode);
  Store("EPGScanDailyTime",   EPGScanDailyTime);
  Store("EPGScanDailyNext",   EPGScanDailyNext);
#endif
  Store("EPGBugfixLevel",     EPGBugfixLevel);
  Store("EPGLinger",          EPGLinger);
  Store("SVDRPTimeout",       SVDRPTimeout);
  Store("ZapTimeout",         ZapTimeout);
  Store("ChannelEntryTimeout",ChannelEntryTimeout);
  Store("DefaultPriority",    DefaultPriority);
  Store("DefaultLifetime",    DefaultLifetime);
  Store("PauseKeyHandling",   PauseKeyHandling);
  Store("PausePriority",      PausePriority);
  Store("PauseLifetime",      PauseLifetime);
  Store("UseSubtitle",        UseSubtitle);
  Store("UseVps",             UseVps);
  Store("VpsMargin",          VpsMargin);
  Store("RecordingDirs",      RecordingDirs);
  Store("FoldersInTimerMenu", FoldersInTimerMenu);
  Store("NumberKeysForChars", NumberKeysForChars);
  Store("VideoDisplayFormat", VideoDisplayFormat);
  Store("VideoFormat",        VideoFormat);
  Store("UpdateChannels",     UpdateChannels);
#ifdef USE_CHANNELBIND
  Store("ChannelBindingByRid",ChannelBindingByRid);
#endif /* CHANNELBIND */
  Store("UseDolbyDigital",    UseDolbyDigital);
  Store("ChannelInfoPos",     ChannelInfoPos);
  Store("ChannelInfoTime",    ChannelInfoTime);
  Store("OSDLeftP",           OSDLeftP);
  Store("OSDTopP",            OSDTopP);
  Store("OSDWidthP",          OSDWidthP);
  Store("OSDHeightP",         OSDHeightP);
  Store("OSDLeft",            OSDLeft);
  Store("OSDTop",             OSDTop);
  Store("OSDWidth",           OSDWidth);
  Store("OSDHeight",          OSDHeight);
  Store("OSDAspect",          OSDAspect);
  Store("OSDMessageTime",     OSDMessageTime);
  Store("UseSmallFont",       UseSmallFont);
  Store("AntiAlias",          AntiAlias);
  Store("FontOsd",            FontOsd);
  Store("FontSml",            FontSml);
  Store("FontFix",            FontFix);
  Store("FontOsdSizeP",       FontOsdSizeP);
  Store("FontSmlSizeP",       FontSmlSizeP);
  Store("FontFixSizeP",       FontFixSizeP);
  Store("FontOsdSize",        FontOsdSize);
  Store("FontSmlSize",        FontSmlSize);
  Store("FontFixSize",        FontFixSize);
  Store("MaxVideoFileSize",   MaxVideoFileSize);
  Store("SplitEditedFiles",   SplitEditedFiles);
  Store("DelTimeshiftRec",    DelTimeshiftRec);
  Store("MinEventTimeout",    MinEventTimeout);
  Store("MinUserInactivity",  MinUserInactivity);
  Store("NextWakeupTime",     NextWakeupTime);
  Store("MultiSpeedMode",     MultiSpeedMode);
  Store("ShowReplayMode",     ShowReplayMode);
  Store("ShowRemainingTime",  ShowRemainingTime);
  Store("ResumeID",           ResumeID);
#ifdef USE_JUMPPLAY
  Store("JumpPlay",           JumpPlay);
  Store("PlayJump",           PlayJump);
  Store("PauseLastMark",      PauseLastMark);
  Store("ReloadMarks",        ReloadMarks);
#endif /* JUMPPLAY */
  Store("CurrentChannel",     CurrentChannel);
  Store("CurrentVolume",      CurrentVolume);
  Store("CurrentDolby",       CurrentDolby);
#ifdef USE_CHANNELPROVIDE
  Store("LocalChannelProvide",LocalChannelProvide);
#endif /* CHANNELPROVIDE */
#ifdef USE_HARDLINKCUTTER
  Store("MaxRecordingSize",   MaxRecordingSize);
  Store("HardLinkCutter",     HardLinkCutter);
#endif /* HARDLINKCUTTER */
  Store("InitialChannel",     InitialChannel);
  Store("InitialVolume",      InitialVolume);
  Store("DeviceBondings",     DeviceBondings);
#ifdef USE_VOLCTRL
  Store("LRVolumeControl",    LRVolumeControl);
  Store("LRChannelGroups",    LRChannelGroups);
  Store("LRForwardRewind",    LRForwardRewind);
#endif /* VOLCTRL */
  Store("ChannelsWrap",       ChannelsWrap);
  Store("EmergencyExit",      EmergencyExit);
#ifdef USE_LIEMIEXT
  Store("ShowRecDate",        ShowRecDate);
  Store("ShowRecTime",        ShowRecTime);
  Store("ShowRecLength",      ShowRecLength);
  Store("ShowProgressBar",    ShowProgressBar);
  Store("MenuCmdPosition",    MenuCmdPosition);
#endif /* LIEMIEXT */
#ifdef USE_LIRCSETTINGS
  Store("LircRepeatDelay",    LircRepeatDelay);
  Store("LircRepeatFreq",     LircRepeatFreq);
  Store("LircRepeatTimeout",  LircRepeatTimeout);
#endif /* LIRCSETTINGS */
#ifdef USE_LNBSHARE
  Store("VerboseLNBlog",       VerboseLNBlog);
  char tmp[20];
  if (cDevice::NumDevices() > 1) {
     for (int i = 1; i <= cDevice::NumDevices(); i++) {
        sprintf(tmp, "Card%dusesLNBnr", i);
        Store(tmp, CardUsesLnbNr[i - 1]);
     }
  }
#endif /* LNBSHARE */
#ifdef USE_NOEPG
  Store("noEPGMode",          noEPGMode);
  Store("noEPGList",          noEPGList ? noEPGList : "");
#endif /* NOEPG */
#ifdef USE_DVLVIDPREFER
  Store ("UseVidPrefer",      UseVidPrefer);
  Store ("nVidPrefer",        nVidPrefer);

  char vidBuf[32];
  for (int zz = 0; zz < nVidPrefer; zz++) {
      sprintf(vidBuf, "VidPreferPrio%d", zz);
      Store (vidBuf, VidPreferPrio[zz]);
      sprintf(vidBuf, "VidPreferSize%d", zz);
      Store (vidBuf, VidPreferSize[zz]);
      }
#endif /* DVLVIDPREFER */
#ifdef USE_LIVEBUFFER
  Store("LiveBuffer",         LiveBuffer);
  Store("LiveBufferSize",     LiveBufferSize);
#endif  /* LIVEBUFFER */
#ifdef REELVDR
  Store("PreviewVideos",      PreviewVideos);
  Store("LiveTvOnAvg",        LiveTvOnAvg);
  Store("ReceptionMode",      ReceptionMode);
  Store("NetServerName",      NetServerName);
  Store("NetServerMAC",       NetServerMAC);
  Store("MaxMultiRoomClients",MaxMultiRoomClients);
  Store("ExpertOptions",      ExpertOptions);
  Store("OSDRandom",          OSDRandom);
  Store("OSDRemainTime",      OSDRemainTime);
  Store("OSDUseSymbol",       OSDUseSymbol);
  Store("OSDScrollBarWidth",  OSDScrollBarWidth);
  Store("FontSizes",          FontSizes);
  Store("AddNewChannels",     AddNewChannels);
  Store("NetServerIP",        NetServerIP);
  Store("ReelboxMode",        ReelboxMode);
  Store("RequestShutDownMode",RequestShutDownMode);
  Store("StandbyOrQuickshutdown", StandbyOrQuickshutdown);
  Store("EnergySaveOn", EnergySaveOn);
  Store("StandbyTimeout", StandbyTimeout);
  Store("UseBouquetList",     UseBouquetList);
  Store("OnlyRadioChannels",  OnlyRadioChannels);
  Store("OnlyEncryptedChannels", OnlyEncryptedChannels);
  Store("OnlyHDChannels", OnlyHDChannels);
  Store("ExpertNavi",         ExpertNavi);
  Store("WantChListOnOk",     WantChListOnOk);
  Store("ChannelUpDownKeyMode", ChannelUpDownKeyMode);
  Store("JumpWidth",          JumpWidth);
  Store("UseZonedChannelList", UseZonedChannelList);
#endif /* REELVDR */

  Sort();

  if (cConfig<cSetupLine>::Save()) {
     isyslog("saved setup to %s", FileName());
     return true;
     }
  return false;
}

#ifdef REELVDR
/*call script with appropriate filename and copy the file to tftp root*/
void CopyToTftpRoot(const char* path)
{
    /*strip the filename from path*/
    const char* p = strrchr(path, '/');
    if ( !p ||!*p ) return;
    ++p;

#define CMP(x) strcasecmp(x,p)
    if ( CMP("channels.conf") &&  CMP("setup.conf") && CMP("sysconfig") && CMP("diseqc.conf") && CMP("favourites.conf") )
        // interested in only one of these files
        return;

    std::string command = std::string("CopytoTftpRoot.sh ") + p;
    SystemExec(command.c_str());
}
#endif /* REELVDR */
