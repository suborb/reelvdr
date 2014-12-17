/*
 * skinreelng.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include "reel.h"
#include "i18n.h"
#include "logo.h"
#include "setup.h"
#include "status.h"

#include <getopt.h>
#include <vdr/plugin.h>

#if defined(APIVERSNUM) && APIVERSNUM < 10400
#error "VDR-1.4.0 API version or greater is required!"
#endif

#if VDRVERSNUM == 10503
#warning "YOU NEED A PATCHED VDR 1.5.3 OR ReelNG WILL CRASH!"
#endif

static const char VERSION[] = "1.1";
static const char DESCRIPTION[] = trNOOP("ReelNG skin");
static const char MAINMENUENTRY[] = "ReelNG Skin"; // needed for Setup entry too!!

class cPluginSkinReel : public cPlugin {
private:
  bool fLogodirSet;
  bool fImagesDirSet;
#ifdef HAVE_FREETYPE
  bool fFontsDirSet;
#endif

public:
  cPluginSkinReel(void);
  virtual ~cPluginSkinReel();
  virtual const char *Version(void) {
    return VERSION;
  } virtual const char *Description(void) {
    return tr(DESCRIPTION);
  }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void) {
  }
  virtual cString Active(void) {
    return NULL;
  }
  virtual const char *MainMenuEntry(void) {
    return MAINMENUENTRY;
  }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

cPluginSkinReel::cPluginSkinReel(void)
{
  // initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  fLogodirSet = false;
  fImagesDirSet = false;
#ifdef HAVE_FREETYPE
  fFontsDirSet = false;
#endif
}

cPluginSkinReel::~cPluginSkinReel()
{
  // clean up after yourself!
}

const char *cPluginSkinReel::CommandLineHelp(void)
{ 
  // return a string that describes all known command line options.
  return 
#ifdef HAVE_FREETYPE
         "  -f <FONTSDIR>, --fonts=<FONTSDIR> Set directory where truetype fonts are stored\n"
#endif
         "  -i <IMAGESDIR>, --epgimages=<IMAGESDIR> Set directory where epgimages are stored\n"
         "  -l <LOGODIR>, --logodir=<LOGODIR>       Set directory where logos are stored.\n";
}

bool cPluginSkinReel::ProcessArgs(int argc, char *argv[])
{
  // implement command line argument processing here if applicable.
  static const struct option long_options[] = {
#ifdef HAVE_FREETYPE
    { "fonts", required_argument, NULL, 'f' },
#endif
    { "epgimages", required_argument, NULL, 'i' },
    { "logodir", required_argument, NULL, 'l' },
    { NULL }
  };

  fLogodirSet = false;
  int c;
  while ((c = getopt_long(argc, argv, "i:f:l:", long_options, NULL)) != -1) {
    switch (c) {
#ifdef HAVE_FREETYPE
      case 'f':
        ReelConfig.SetFontsDir(optarg);
        fFontsDirSet = true;
        break;
#endif
      case 'i':
        ReelConfig.SetImagesDir(optarg);
        fImagesDirSet = true;
        break;
      case 'l':
        ReelConfig.SetLogoDir(optarg);
        fLogodirSet = true;
        break;
      default:
        return false;
    }
  }
  if (fImagesDirSet != true)
  {
      ReelConfig.SetImagesDir("/usr/share/reel/skinreelng/Blue"); // Set default Image Directory to blue theme      
      fImagesDirSet = true;
  }
  if (fLogodirSet == false ) { ReelConfig.SetLogoDir("LogoDir Not SET"); }
  
  return true;
}

bool cPluginSkinReel::Initialize(void)
{
  // initialize any background activities the plugin shall perform.
  debug("cPluginSkinReel::Initialize()");
  return true;
}

bool cPluginSkinReel::Start(void)
{
  // start any background activities the plugin shall perform.
  debug("cPluginSkinReel::Start()");

#if VDRVERSNUM < 10507
  RegisterI18n(Phrases);
#endif

  if (!fLogodirSet) {
    // set logo directory
    //ReelConfig.SetLogoDir(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    //fLogodirSet = true;
     ReelConfig.SetLogoDir("LogoDir Not SET");
  }
  if (!fImagesDirSet) {
    // set epgimages directory
    char *dir = NULL;
    asprintf(&dir, "%s/epgimages", cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    if (dir) {
      ReelConfig.SetImagesDir(dir);
      fImagesDirSet = true;
      free(dir);
    }
  }
#ifdef HAVE_FREETYPE
  if (!fFontsDirSet) {
    // set fonts directory
    char *dir = NULL;
    asprintf(&dir, "%s/fonts", cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    if (dir) {
      ReelConfig.SetFontsDir(dir);
      fFontsDirSet = true;
      free(dir);
    }
  }
#endif

  // resize logo cache
  ReelLogoCache.Resize(ReelConfig.cacheSize);
  // create skin
  new cSkinReel;
  return true;
}

void cPluginSkinReel::Stop(void)
{
  // stop any background activities the plugin shall perform.
  debug("cPluginSkinReel::Stop()");
}

void cPluginSkinReel::Housekeeping(void)
{
  // perform any cleanup or other regular tasks.
}

cOsdObject *cPluginSkinReel::MainMenuAction(void)
{
  // perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginSkinReel::SetupMenu(void)
{
  // return a setup menu in case the plugin supports one.
  debug("cPluginSkinReel::SetupMenu()");
  return new cPluginSkinReelSetup();
}

bool cPluginSkinReel::SetupParse(const char *Name, const char *Value)
{
  // parse your own setup parameters and store their values.
//  debug("cPluginSkinReel::SetupParse()");

       if (!strcasecmp(Name, "SingleArea8Bpp"))            ReelConfig.singleArea8Bpp = atoi(Value);
  else if (!strcasecmp(Name, "ShowAuxInfo"))               ReelConfig.showAuxInfo = atoi(Value);
  else if (!strcasecmp(Name, "ShowProgressBar"))           ReelConfig.showProgressbar = atoi(Value);
  else if (!strcasecmp(Name, "ShowRemaining"))             ReelConfig.showRemaining = atoi(Value);
  else if (!strcasecmp(Name, "ShowListSymbols"))           ReelConfig.showListSymbols = atoi(Value);
  else if (!strcasecmp(Name, "ShowSymbols"))               ReelConfig.showSymbols = atoi(Value);
  else if (!strcasecmp(Name, "ShowSymbolsMenu"))           ReelConfig.showSymbolsMenu = atoi(Value);
  else if (!strcasecmp(Name, "ShowSymbolsReplay"))         ReelConfig.showSymbolsReplay = atoi(Value);
  else if (!strcasecmp(Name, "ShowSymbolsMsgs"))           ReelConfig.showSymbolsMsgs = atoi(Value);
  else if (!strcasecmp(Name, "ShowSymbolsAudio"))          ReelConfig.showSymbolsAudio = atoi(Value);
  else if (!strcasecmp(Name, "ShowLogo"))                  ReelConfig.showLogo = atoi(Value);
  else if (!strcasecmp(Name, "ShowInfo"))                  ReelConfig.showInfo = atoi(Value);
  else if (!strcasecmp(Name, "ShowMarker"))                ReelConfig.showMarker = atoi(Value);
  else if (!strcasecmp(Name, "ShowVPS"))                   ReelConfig.showVps = atoi(Value);
  else if (!strcasecmp(Name, "ShowFlags"))                 ReelConfig.showFlags = atoi(Value);
  else if (!strcasecmp(Name, "CacheSize"))                 ReelConfig.cacheSize = atoi(Value);
  else if (!strcasecmp(Name, "UseChannelId"))              ReelConfig.useChannelId = atoi(Value);
  else if (!strcasecmp(Name, "NumReruns"))                 ReelConfig.numReruns = atoi(Value);
  else if (!strcasecmp(Name, "UseSubtitleRerun"))          ReelConfig.useSubtitleRerun = atoi(Value);
  else if (!strcasecmp(Name, "ShowTimerConflicts"))        ReelConfig.showTimerConflicts = atoi(Value);
  else if (!strcasecmp(Name, "ShowRecSize"))               ReelConfig.showRecSize = atoi(Value);
  else if (!strcasecmp(Name, "ShowImages"))                ReelConfig.showImages = atoi(Value);
  else if (!strcasecmp(Name, "ResizeImages"))              ReelConfig.resizeImages = atoi(Value);
  else if (!strcasecmp(Name, "ShowMailIcon"))              ReelConfig.showMailIcon = atoi(Value);
  else if (!strcasecmp(Name, "ImageWidth"))                ReelConfig.imageWidth = atoi(Value);
  else if (!strcasecmp(Name, "ImageHeight"))               ReelConfig.imageHeight = atoi(Value);
  else if (!strcasecmp(Name, "ImageExtension"))            ReelConfig.imageExtension = atoi(Value);
  //else if (!strcasecmp(Name, "FullTitleWidth"))            ReelConfig.fullTitleWidth = atoi(Value);
  else if (!strcasecmp(Name, "UseTextEffects"))            ReelConfig.useTextEffects = atoi(Value);
  else if (!strcasecmp(Name, "ScrollDelay"))               ReelConfig.scrollDelay = atoi(Value);
  else if (!strcasecmp(Name, "ScrollPause"))               ReelConfig.scrollPause = atoi(Value);
  else if (!strcasecmp(Name, "ScrollMode"))                ReelConfig.scrollMode = atoi(Value);
  else if (!strcasecmp(Name, "BlinkPause"))                ReelConfig.blinkPause = atoi(Value);
  else if (!strcasecmp(Name, "ScrollInfo"))                ReelConfig.scrollInfo = atoi(Value);
  else if (!strcasecmp(Name, "ScrollListItem"))            ReelConfig.scrollListItem = atoi(Value);
  else if (!strcasecmp(Name, "ScrollOther"))               ReelConfig.scrollOther = atoi(Value);
  else if (!strcasecmp(Name, "ScrollTitle"))               ReelConfig.scrollTitle = atoi(Value);
  else if (!strcasecmp(Name, "FontOsdTitle"))              ReelConfig.SetFont(FONT_OSDTITLE, atoi(Value));
  else if (!strcasecmp(Name, "FontOsdTitleName"))          ReelConfig.SetFont(FONT_OSDTITLE, Value);
  else if (!strcasecmp(Name, "FontMessage"))               ReelConfig.SetFont(FONT_MESSAGE, atoi(Value));
  else if (!strcasecmp(Name, "FontMessageName"))           ReelConfig.SetFont(FONT_MESSAGE, Value);
  else if (!strcasecmp(Name, "FontDate"))                  ReelConfig.SetFont(FONT_DATE, atoi(Value));
  else if (!strcasecmp(Name, "FontDateName"))              ReelConfig.SetFont(FONT_DATE, Value);
  else if (!strcasecmp(Name, "FontHelpKeys"))              ReelConfig.SetFont(FONT_HELPKEYS, atoi(Value));
  else if (!strcasecmp(Name, "FontHelpKeysName"))          ReelConfig.SetFont(FONT_HELPKEYS, Value);
  else if (!strcasecmp(Name, "FontCiTitle"))               ReelConfig.SetFont(FONT_CITITLE, atoi(Value));
  else if (!strcasecmp(Name, "FontCiTitleName"))           ReelConfig.SetFont(FONT_CITITLE, Value);
  else if (!strcasecmp(Name, "FontCiSubtitle"))            ReelConfig.SetFont(FONT_CISUBTITLE, atoi(Value));
  else if (!strcasecmp(Name, "FontCiSubtitleName"))        ReelConfig.SetFont(FONT_CISUBTITLE, Value);
  else if (!strcasecmp(Name, "FontCiLanguage"))            ReelConfig.SetFont(FONT_CILANGUAGE, atoi(Value));
  else if (!strcasecmp(Name, "FontCiLanguageName"))        ReelConfig.SetFont(FONT_CILANGUAGE, Value);
  else if (!strcasecmp(Name, "FontListItem"))              ReelConfig.SetFont(FONT_LISTITEM, atoi(Value));
  else if (!strcasecmp(Name, "FontListItemName"))          ReelConfig.SetFont(FONT_LISTITEM, Value);
  else if (!strcasecmp(Name, "FontListItemCurrent"))       ReelConfig.SetFont(FONT_LISTITEMCURRENT, atoi(Value));
  else if (!strcasecmp(Name, "FontListItemCurrentName"))   ReelConfig.SetFont(FONT_LISTITEMCURRENT, Value);
  else if (!strcasecmp(Name, "FontInfoTimerHeadline"))     ReelConfig.SetFont(FONT_INFOTIMERHEADLINE, atoi(Value));
  else if (!strcasecmp(Name, "FontInfoTimerHeadlineName")) ReelConfig.SetFont(FONT_INFOTIMERHEADLINE, Value);
  else if (!strcasecmp(Name, "FontInfoTimerText"))         ReelConfig.SetFont(FONT_INFOTIMERTEXT, atoi(Value));
  else if (!strcasecmp(Name, "FontInfoTimerTextName"))     ReelConfig.SetFont(FONT_INFOTIMERTEXT, Value);
  else if (!strcasecmp(Name, "FontInfoWarnHeadline"))      ReelConfig.SetFont(FONT_INFOWARNHEADLINE, atoi(Value));
  else if (!strcasecmp(Name, "FontInfoWarnHeadlineName"))  ReelConfig.SetFont(FONT_INFOWARNHEADLINE, Value);
  else if (!strcasecmp(Name, "FontInfoWarnText"))          ReelConfig.SetFont(FONT_INFOWARNTEXT, atoi(Value));
  else if (!strcasecmp(Name, "FontInfoWarnTextName"))      ReelConfig.SetFont(FONT_INFOWARNTEXT, Value);
  else if (!strcasecmp(Name, "FontDetailsTitle"))          ReelConfig.SetFont(FONT_DETAILSTITLE, atoi(Value));
  else if (!strcasecmp(Name, "FontDetailsTitleName"))      ReelConfig.SetFont(FONT_DETAILSTITLE, Value);
  else if (!strcasecmp(Name, "FontDetailsSubtitle"))       ReelConfig.SetFont(FONT_DETAILSSUBTITLE, atoi(Value));
  else if (!strcasecmp(Name, "FontDetailsSubtitleName"))   ReelConfig.SetFont(FONT_DETAILSSUBTITLE, Value);
  else if (!strcasecmp(Name, "FontDetailsDate"))           ReelConfig.SetFont(FONT_DETAILSDATE, atoi(Value));
  else if (!strcasecmp(Name, "FontDetailsDateName"))       ReelConfig.SetFont(FONT_DETAILSDATE, Value);
  else if (!strcasecmp(Name, "FontDetailsText"))           ReelConfig.SetFont(FONT_DETAILSTEXT, atoi(Value));
  else if (!strcasecmp(Name, "FontDetailsTextName"))       ReelConfig.SetFont(FONT_DETAILSTEXT, Value);
  else if (!strcasecmp(Name, "FontReplayTimes"))           ReelConfig.SetFont(FONT_REPLAYTIMES, atoi(Value));
  else if (!strcasecmp(Name, "FontReplayTimesName"))       ReelConfig.SetFont(FONT_REPLAYTIMES, Value);
  else if (!strcasecmp(Name, "FontFixed"))                 ReelConfig.SetFont(FONT_FIXED, atoi(Value));
  else if (!strcasecmp(Name, "FontFixedName"))             ReelConfig.SetFont(FONT_FIXED, Value);
  else if (!strcasecmp(Name, "DynOSD"))                    ReelConfig.dynOsd = atoi(Value);
  else if (!strcasecmp(Name, "StatusLineMode"))            ReelConfig.statusLineMode = atoi(Value);
  else if (!strcasecmp(Name, "ShowWssSymbols"))            ReelConfig.showWssSymbols = atoi(Value);
  else if (!strcasecmp(Name, "ShowStatusSymbols"))         ReelConfig.showStatusSymbols = atoi(Value);
  //else if (!strcasecmp(Name, "Fontsizes" ))                ReelConfig.isLargeFont = atoi(Value);
  else return false;

  return true;
}

bool cPluginSkinReel::Service(const char *Id, void *Data)
{
  if(!strcmp(Id, "setThumb")){
	if (Data) {
                CurThumb *ptr = (struct CurThumb*)Data;                
                int slot = ptr->slot;
                strncpy((char*)ReelConfig.curThumb[slot].path, (const char*)ptr->path, 255);
                ReelConfig.curThumb[slot].x = ptr->x;
                ReelConfig.curThumb[slot].y = ptr->y;
                ReelConfig.curThumb[slot].blend = ptr->blend;
                ReelConfig.curThumb[slot].w = ptr->w;
                ReelConfig.curThumb[slot].h = ptr->h;
	} else {
                for (int i = 0; i<NR_UNBUFFERED_SLOTS; i++) {
                   strcpy((char*)ReelConfig.curThumb[i].path, "");
                   ReelConfig.curThumb[i].x = ReelConfig.curThumb[i].y = 0;
                   ReelConfig.curThumb[i].w = ReelConfig.curThumb[i].h = 0;
                }
        }
	
	//printf("SERVICE: data: %s\n", (const char*)Data);
	return true;
  }
  // handle custom service requests from other plugins
  return false;
}

const char **cPluginSkinReel::SVDRPHelpPages(void)
{
  // return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginSkinReel::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginSkinReel);    // don't touch this!
// vim:et:sw=2:ts=2:
