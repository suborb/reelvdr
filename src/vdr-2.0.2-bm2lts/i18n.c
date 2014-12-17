/*
 * i18n.c: Internationalization
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: i18n.c 2.5 2012/09/01 10:53:43 kls Exp $
 */

/*
 * In case an English phrase is used in more than one context (and might need
 * different translations in other languages) it can be preceded with an
 * arbitrary string to describe its context, separated from the actual phrase
 * by a '$' character (see for instance "Button$Stop" vs. "Stop").
 * Of course this means that no English phrase may contain the '$' character!
 * If this should ever become necessary, the existing '$' would have to be
 * replaced with something different...
 */

#include "i18n.h"
#include <ctype.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include "tools.h"

// TRANSLATORS: The name of the language, as written natively
const char *LanguageName = trNOOP("LanguageName$English");
// TRANSLATORS: The 3-letter code of the language
const char *LanguageCode = trNOOP("LanguageCode$eng");

// List of known language codes with aliases.
// Actually we could list all codes from http://www.loc.gov/standards/iso639-2
// here, but that would be several hundreds - and for most of them it's unlikely
// they're ever going to be used...

const char *LanguageCodeList[] = {
  "eng,dos",
  "deu,ger",
  "slv,slo",
  "ita",
  "dut,nla,nld",
  "prt",
  "fra,fre",
  "nor",
  "fin,suo",
  "pol",
  "esl,spa",
  "ell,gre",
  "sve,swe",
  "rom,rum",
  "hun",
  "cat,cln",
  "rus",
  "srb,srp,scr,scc",
  "hrv",
  "est",
  "dan",
  "cze,ces",
  "tur",
  "ukr",
  "ara",
  NULL
  };

#if 0
/// put any strings here that should be translated but are not inside the code.
///// needed i.e. for the recording commands or the main menu inside the xml file

// TRANSLATORS:
// Menu "TV & Radio"
trNOOP("TV & Radio");
trNOOP("TV-Guide (EPG)");
trNOOP("Timer Manager");
trNOOP("Search-Timer Manager");
trNOOP("Parental Control");

// TRANSLATORS: Tooltips Menu "TV & Radio"
trNOOP("DVB channel list, TV guide, Timer & Search timer menus, Parental control");
trNOOP("Overview of all TV and radio stations: Switch, manage and edit channels");
trNOOP("Get information on TV programs and easily select broadcasts for recording");
trNOOP("View, edit and manage recording timers");
trNOOP("Find broadcasts with user defined search terms, i.e for recording TV series");
trNOOP("Deny access to TV channels, broadcasts or TV recordings by setting a PIN code");

// TRANSLATORS:
// Menu "Music & Pictures"
trNOOP("Music & Pictures");
trNOOP("Image Library");
trNOOP("Music Library");
trNOOP("Internet Radio");
trNOOP("Archive Audio-CD");

// TRANSLATORS: Tooltips Menu "Music & Pictures"
trNOOP("Music library, Picture library, Internet radio, Audio CD ripping");
trNOOP("Music library, Picture library, Internet radio");
trNOOP("Play music, manage and copy music files");
trNOOP("Play music and films from your collection");
trNOOP("Watch pictures, manage and copy picture files");
trNOOP("Listen to internet radio, manage and add radio stations");
trNOOP("Copy an Audio CD to the music library");

// TRANSLATORS:
// Menu "Films & DVD"
trNOOP("Films & DVD");
trNOOP("Movie Library");

// TRANSLATORS:
// Tooltips Menu "Films & DVD"
trNOOP("Movie library, TV recordings, Burn TV recordings, Video DVD ripping");
trNOOP("Movie library, TV recordings");
trNOOP("Watch movies, manage and copy movie files");
trNOOP("Watch TV recordings, cut, manage and copy TV recordings");
trNOOP("Recover accidentally deleted TV recordings");
trNOOP("Burn TV recordings on CD or DVD");
trNOOP("Copy a Video-DVD to the movie library");

// TRANSLATORS:
// PI epgsearch
trNOOP("Button$Functions");
trNOOP("Setup.Recording$Time Buffer at Start (min)");
trNOOP("Setup.Recording$Time Buffer at End (min)");

// TRANSLATORS:
// PI extrecmenu
trNOOP("Undelete Recordings");
trNOOP("Recording commands");

// TRANSLATORS:
// PI undelete
// PI burn
trNOOP("Write DVD - Job");
trNOOP("Write DVD - Job options");
trNOOP("Edit recording details");

// TRANSLATORS:
// PI dvdswitch
trNOOP("New folder");

// TRANSLATORS:
// Menu "Internet & Extras"
trNOOP("Internet & Extras");

// TRANSLATORS:
// Tooltips Menu "Internet & Extras"
trNOOP("File Manager, Web browser, ReelBlog");
trNOOP("File Manager, ReelBlog");
trNOOP("Copy, move, edit and delete files");
trNOOP("Surf the web with Firefox. / Show the Linux desktop.");
trNOOP("Read news & information on the ReelBox");

// TRANSLATORS:
// PI Filebrowser
trNOOP("Filebrowser");
trNOOP("Create directory");
trNOOP("Playlist");
trNOOP("Playlist actions");
trNOOP("Playlist Commands");
trNOOP("Filebrowser Commands");
trNOOP("Copy playlist");
trNOOP("Insert in playlist");
trNOOP("Open playlist");
trNOOP("Save playlist as");

// PI Webbrowser
// PI reelblog

// TRANSLATORS:
// Menu "Additional Software"
trNOOP("Additional Software");
trNOOP("Magazine EPG");
trNOOP("Additional software provided by third party developers");

// TRANSLATORS:
// More tooltips in the main menu
trNOOP("Play the disk in the drive");
trNOOP("Show currently active background processes");


// TRANSLATORS:
// Menu Media Manager
trNOOP("Format a DVD-RW");
trNOOP("Directory browser");

// Obsolete?
//trNOOP("Timer Timeline");
//trNOOP("Settings");
//trNOOP("Extras");

// TRANSLATORS:
// Special Menus that have a banner at the left
trNOOP("My Pictures");
trNOOP("My Music");
trNOOP("My Videos");
trNOOP("My DVDs");
trNOOP("EPG Commands: ");
trNOOP("Search actions");
trNOOP("Search entries");
trNOOP("Search templates");
trNOOP("Edit search");
trNOOP("Edit timer");
trNOOP("Edit blacklist");
trNOOP("Edit template");
trNOOP("Edit user-defined days of week");
trNOOP("found recordings");
trNOOP("Select directory");
trNOOP("Timer conflicts -");
trNOOP("Title$Childlock");
trNOOP("Games");
trNOOP("Multifeed Options");
trNOOP("Move recording");
trNOOP("Select Reelblog item");
trNOOP("SleepTimer");
trNOOP("Rename entry");
trNOOP("Rename directory");
trNOOP("Choose action");
trNOOP("Mediaplayer");
trNOOP("Channel List:");

// TRANSLATORS:
// Setup Menus with "Setup"-banner on the left
trNOOP("Title$Setup");
trNOOP("Settings");
trNOOP("Settings: System Settings");
trNOOP("Software Update");
trNOOP("Channel Lists");
trNOOP("Channel lists functions");
trNOOP("Channelscan");
trNOOP("Installation Wizard");
trNOOP("Installation");
trNOOP("Remote Control / Automation");
trNOOP("MultiRoom Setup");
trNOOP("MultiRoom - Error!");
trNOOP("Basic Settings");
trNOOP("Network Settings");
trNOOP("EPG Services");
trNOOP("Reception Settings");
trNOOP("Network");
trNOOP("Global Settings");
trNOOP("UMTS Settings");
trNOOP("Dynamic DNS");
trNOOP("GnuDIP service");
trNOOP("DynDNS service");
trNOOP("Manual Network Connections");
trNOOP("Windows Network Connection");
trNOOP("NFS Client");
trNOOP("Video Recorder "); // note the trailing blank
trNOOP("Prepare disk:");
trNOOP("Network Drives");
trNOOP("Setup - Audio/Video");
trNOOP("Timezone & clock");
trNOOP("System Information");
trNOOP("NetCeiver diagnostics");
trNOOP("NetCeiver firmware update");
trNOOP("Antenna settings");
trNOOP("Rotor Setup");
trNOOP("Streaming Clients")

//trNOOP("OSD & Language");
trNOOP("Language Settings");
trNOOP("Timezone");
trNOOP("Timezone Settings");
trNOOP("Front Panel");
trNOOP("TVTV Service");
trNOOP("International EPG");
trNOOP("Energy Options");
trNOOP("Factory defaults");
trNOOP("Setup.Miscellaneous$Power On/Off option");
trNOOP("Setup.Miscellaneous$Standby Mode");
trNOOP("Option$Quick");
//trNOOP("");

// TRANSLATORS:
//Setup OSD and Language
trNOOP("Setup.OSD$OSD Font Size");
trNOOP("FontSize$Normal");
trNOOP("Small");
trNOOP("User defined");

// TRANSLATORS:
// Recording Commands
trNOOP("Mark Commercials");
trNOOP("Add Child Lock"),
trNOOP("Remove Child Lock");
trNOOP("Change title");
trNOOP("Mark as New");
trNOOP("Remove Archive Mark");
trNOOP("Remove Crop Mark");
trNOOP("Remove Archived Recording");
trNOOP("Restore Archived Recording");
trNOOP("Convert to MPEG (TS)");
trNOOP("Directory Selector:");
trNOOP("Select Media");
trNOOP("Create directory");
trNOOP("Directory commands");
trNOOP("Copy to local harddisc");
trNOOP("Generate preview");
trNOOP("Remove preview");
trNOOP("Compress recording");

// TRANSLATORS:
// PI MCLI
trNOOP("No CI-Module");
//CI/CAM Modules
trNOOP("upper slot");
trNOOP("lower slot");


// TRANSLATORS:
// Channels/Bouquets
trNOOP("Edit Channels");
trNOOP("Customize Bouquets");
trNOOP("Channellist: set filters");

// TRANSLATORS:
// PI CHANNELLIST 
trNOOP("All Channels");
trNOOP("Functions: Favourites");
trNOOP("Functions: Channel list");
trNOOP("Functions: Select favourite folder");
trNOOP("Functions: search for channel");
trNOOP("Functions: Create new Favourite folder");
trNOOP("Functions: Rename favourite folder");
trNOOP("Functions: Select ");
trNOOP("Functions: search ");
trNOOP("Functions: Create ");
trNOOP("Functions: Rename ");
trNOOP("Functions: Favourites");
trNOOP("Functions: Channel ");
trNOOP("Functions: Move Channel");
trNOOP("Sources");
trNOOP("Providers");

// PI music browser
trNOOP("Music Browser");
trNOOP("Music");

//trNOOP("");
#endif

static const char *I18nLocaleDir = LOCDIR;

static cStringList LanguageLocales;
static cStringList LanguageNames;
static cStringList LanguageCodes;

static int NumLocales = 1;
static int CurrentLanguage = 0;

static bool ContainsCode(const char *Codes, const char *Code)
{
  while (*Codes) {
        int l = 0;
        for ( ; l < 3 && Code[l]; l++) {
            if (Codes[l] != tolower(Code[l]))
               break;
            }
        if (l == 3)
           return true;
        Codes++;
        }
  return false;
}

static const char *SkipContext(const char *s)
{
  const char *p = strchr(s, '$');
  return p ? p + 1 : s;
}

static void SetEnvLanguage(const char *Locale)
{
  setenv("LANGUAGE", Locale, 1);
  extern int _nl_msg_cat_cntr;
  ++_nl_msg_cat_cntr;
}

void I18nInitialize(const char *LocaleDir)
{
  if (LocaleDir)
     I18nLocaleDir = LocaleDir;
  LanguageLocales.Append(strdup(I18N_DEFAULT_LOCALE));
  LanguageNames.Append(strdup(SkipContext(LanguageName)));
  LanguageCodes.Append(strdup(LanguageCodeList[0]));
  textdomain("vdr");
  bindtextdomain("vdr", I18nLocaleDir);
  cFileNameList Locales(I18nLocaleDir, true);
  if (Locales.Size() > 0) {
     char *OldLocale = strdup(setlocale(LC_MESSAGES, NULL));
     for (int i = 0; i < Locales.Size(); i++) {
         cString FileName = cString::sprintf("%s/%s/LC_MESSAGES/vdr.mo", I18nLocaleDir, Locales[i]);
         if (access(FileName, F_OK) == 0) { // found a locale with VDR texts
            if (NumLocales < I18N_MAX_LANGUAGES - 1) {
               SetEnvLanguage(Locales[i]);
               const char *TranslatedLanguageName = gettext(LanguageName);
               if (TranslatedLanguageName != LanguageName) {
                  NumLocales++;
                  if (strstr(OldLocale, Locales[i]) == OldLocale)
                     CurrentLanguage = LanguageLocales.Size();
                  LanguageLocales.Append(strdup(Locales[i]));
                  LanguageNames.Append(strdup(TranslatedLanguageName));
                  const char *Code = gettext(LanguageCode);
                  for (const char **lc = LanguageCodeList; *lc; lc++) {
                      if (ContainsCode(*lc, Code)) {
                         Code = *lc;
                         break;
                         }
                      }
                  LanguageCodes.Append(strdup(Code));
                  }
               }
            else {
               esyslog("ERROR: too many locales - increase I18N_MAX_LANGUAGES!");
               break;
               }
            }
         }
     SetEnvLanguage(LanguageLocales[CurrentLanguage]);
     free(OldLocale);
     dsyslog("found %d locales in %s", NumLocales - 1, I18nLocaleDir);
     }
  // Prepare any known language codes for which there was no locale:
  for (const char **lc = LanguageCodeList; *lc; lc++) {
      bool Found = false;
      for (int i = 0; i < LanguageCodes.Size(); i++) {
          if (strcmp(*lc, LanguageCodes[i]) == 0) {
             Found = true;
             break;
             }
          }
      if (!Found) {
         dsyslog("no locale for language code '%s'", *lc);
         LanguageLocales.Append(strdup(I18N_DEFAULT_LOCALE));
         LanguageNames.Append(strdup(*lc));
         LanguageCodes.Append(strdup(*lc));
         }
      }
}

void I18nRegister(const char *Plugin)
{
  cString Domain = cString::sprintf("vdr-%s", Plugin);
  bindtextdomain(Domain, I18nLocaleDir);
}

void I18nSetLocale(const char *Locale)
{
  if (Locale && *Locale) {
     int i = LanguageLocales.Find(Locale);
     if (i >= 0) {
        CurrentLanguage = i;
        SetEnvLanguage(Locale);
        }
     else
        dsyslog("unknown locale: '%s'", Locale);
     }
}

int I18nCurrentLanguage(void)
{
  return CurrentLanguage;
}

void I18nSetLanguage(int Language)
{
  if (Language < LanguageNames.Size()) {
     CurrentLanguage = Language;
     I18nSetLocale(I18nLocale(CurrentLanguage));
     }
}

int I18nNumLanguagesWithLocale(void)
{
  return NumLocales;
}

const cStringList *I18nLanguages(void)
{
  return &LanguageNames;
}

const char *I18nTranslate(const char *s, const char *Plugin)
{
  if (!s)
     return s;
  if (CurrentLanguage) {
     const char *t = Plugin ? dgettext(Plugin, s) : gettext(s);
     if (t != s)
        return t;
     }
  return SkipContext(s);
}

const char *I18nLocale(int Language)
{
  return 0 <= Language && Language < LanguageLocales.Size() ? LanguageLocales[Language] : NULL;
}

const char *I18nLanguageCode(int Language)
{
  return 0 <= Language && Language < LanguageCodes.Size() ? LanguageCodes[Language] : NULL;
}

int I18nLanguageIndex(const char *Code)
{
  for (int i = 0; i < LanguageCodes.Size(); i++) {
      if (ContainsCode(LanguageCodes[i], Code))
         return i;
      }
  //dsyslog("unknown language code: '%s'", Code);
  return -1;
}

const char *I18nNormalizeLanguageCode(const char *Code)
{
  for (int i = 0; i < 3; i++) {
      if (Code[i]) {
         // ETSI EN 300 468 defines language codes as consisting of three letters
         // according to ISO 639-2. This means that they are supposed to always consist
         // of exactly three letters in the range a-z - no digits, UTF-8 or other
         // funny characters. However, some broadcasters apparently don't have a
         // copy of the DVB standard (or they do, but are perhaps unable to read it),
         // so they put all sorts of non-standard stuff into the language codes,
         // like nonsense as "2ch" or "A 1" (yes, they even go as far as using
         // blanks!). Such things should go into the description of the EPG event's
         // ComponentDescriptor.
         // So, as a workaround for this broadcaster stupidity, let's ignore
         // language codes with unprintable characters...
         if (!isprint(Code[i])) {
            //dsyslog("invalid language code: '%s'", Code);
            return "???";
            }
         // ...and replace blanks with underlines (ok, this breaks the 'const'
         // of the Code parameter - but hey, it's them who started this):
         if (Code[i] == ' ')
            *((char *)&Code[i]) = '_';
         }
      else
         break;
      }
  int n = I18nLanguageIndex(Code);
  return n >= 0 ? I18nLanguageCode(n) : Code;
}

bool I18nIsPreferredLanguage(int *PreferredLanguages, const char *LanguageCode, int &OldPreference, int *Position)
{
  int pos = 1;
  bool found = false;
  while (LanguageCode) {
        int LanguageIndex = I18nLanguageIndex(LanguageCode);
        for (int i = 0; i < LanguageCodes.Size(); i++) {
            if (PreferredLanguages[i] < 0)
               break; // the language is not a preferred one
            if (PreferredLanguages[i] == LanguageIndex) {
               if (OldPreference < 0 || i < OldPreference) {
                  OldPreference = i;
                  if (Position)
                     *Position = pos;
                  found = true;
                  break;
                  }
               }
            }
        if ((LanguageCode = strchr(LanguageCode, '+')) != NULL) {
           LanguageCode++;
           pos++;
           }
        else if (pos == 1 && Position)
           *Position = 0;
        }
  if (OldPreference < 0) {
     OldPreference = LanguageCodes.Size(); // higher than the maximum possible value
     return true; // if we don't find a preferred one, we take the first one
     }
  return found;
}
