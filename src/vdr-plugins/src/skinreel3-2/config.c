/*
 * config.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 *  
 */

#include "common.h"
#include "config.h"
#include "tools.h"

// #ifdef USE_PLUGIN_AVARDS
// #include "services/avards.h"
// #endif

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <vdr/font.h>
#include <vdr/plugin.h>

#ifdef HAVE_IMAGEMAGICK
const char *imageExtensionTexts[NUM_IMAGEEXTENSIONTEXTS] = { "xpm", "jpg", "png" };
#endif

cReelConfig ReelConfig;

std::vector<struct ID3Tag> ID3Tags;

#ifdef HAVE_FREETYPE
# if VDRVERSNUM != 10503
cGraphtftFont FontCache;
# endif
#endif

FontConfig allFontConfig[FONT_NUMFONTS] =
{
  { FONT_OSDTITLE,          "FontOsdTitle",          "FontOsdTitleName" },
  { FONT_MESSAGE,           "FontMessage",           "FontMessageName" },
  { FONT_DATE,              "FontDate",              "FontDateName" },
  { FONT_HELPKEYS,          "FontHelpKeys",          "FontHelpKeysName" },
  { FONT_CITITLE,           "FontCiTitle",           "FontCiTitleName" },	// CI = channel info
  { FONT_CISUBTITLE,        "FontCiSubtitle",        "FontCiSubtitleName" },
  { FONT_CILANGUAGE,        "FontCiLanguage",        "FontCiLanguageName" },
  { FONT_LISTITEM,          "FontListItem",          "FontListItemName" },
  { FONT_INFOTIMERHEADLINE, "FontInfoTimerHeadline", "FontInfoTimerHeadlineName" },
  { FONT_INFOTIMERTEXT,     "FontInfoTimerText",     "FontInfoTimerTextName" },
  { FONT_INFOWARNHEADLINE,  "FontInfoWarnHeadline",  "FontInfoWarnHeadlineName" },
  { FONT_INFOWARNTEXT,      "FontInfoWarnText",      "FontInfoWarnTextName" },
  { FONT_DETAILSTITLE,      "FontDetailsTitle",      "FontDetailsTitleName" },
  { FONT_DETAILSSUBTITLE,   "FontDetailsSubtitle",   "FontDetailsSubtitleName" },
  { FONT_DETAILSDATE,       "FontDetailsDate",       "FontDetailsDateName" },
  { FONT_DETAILSTEXT,       "FontDetailsText",       "FontDetailsTextName" },
  { FONT_REPLAYTIMES,       "FontReplayTimes",       "FontReplayTimesName" },
  { FONT_FIXED,             "FontFixed",             "FontFixedName" },
  { FONT_LISTITEMCURRENT,   "FontListItemCurrent",   "FontListItemCurrentName" } // order should match with config.h enum
};

cReelConfig::cReelConfig() : showAuxInfo(1), showLogo(1), showVps(1), showSymbols(1),
                                 showSymbolsMenu(1), showSymbolsReplay(1), showSymbolsMsgs(1), showSymbolsAudio(1),
                                 showListSymbols(1), showProgressbar(1), cacheSize(50),
                                 useChannelId(0), showRemaining(0), showMarker(1),
                                 singleArea(1), singleArea8Bpp(1), numReruns(5),
                                 useSubtitleRerun(1), showTimerConflicts(1), showRecSize(2),
                                 showImages(0), resizeImages(0), showMailIcon(0),
                                 imageWidth(120), imageHeight(80), imageExtension(0), fullTitleWidth(0),
                                 useTextEffects(0), scrollDelay(50), scrollPause(1500), scrollMode(0),
                                 blinkPause(1000), scrollInfo(1), scrollListItem(1), scrollOther(1), scrollTitle(1),
                                 dynOsd(0), statusLineMode(0), showWssSymbols(0), showStatusSymbols(1), showSourceInChannelinfo(0)
{
  memset(logoDir, 0, sizeof(logoDir));
  memset(strImagesDir, 0, sizeof(strImagesDir));
  memset(strEPGImagesDir, 0, sizeof(strEPGImagesDir));
#ifdef HAVE_FREETYPE
  memset(strFontsDir, 0, sizeof(strFontsDir));
#endif



  for (int id = 0; id < FONT_NUMFONTS; id++) {
    switch (id) {
      case FONT_CISUBTITLE:
      case FONT_CILANGUAGE:
      case FONT_DETAILSSUBTITLE:
        allFonts[id].VdrId = fontSml + 1;
        allFonts[id].Default = fontSml;
        break;
      /* case FONT_FIXED:
        allFonts[id].VdrId = fontFix + 1;
        allFonts[id].Default = fontFix;
        break;
      */
      default:
        allFonts[id].VdrId = fontOsd + 1;
        allFonts[id].Default = fontOsd;
    }
    memset(allFonts[id].Name, 0, sizeof(allFonts[id].Name));
    allFonts[id].Width = 100;
    allFonts[id].Size = 26;
  }

#ifdef HAVE_FREETYPE 
   /**********************************
   * Set default fonts! these are to be overwritten by the values got from setup.conf
   * 
   * these values are not written into setup.conf unless explicitly requested by the user
   **********************************/

#define DEFAULT_TTF_NAME "lmsans10-regular.otf"
//#define DEFAULT_TTF_NAME "Fontin_Sans_B_45b.otf"
#define DEFAULT_TITLEFONT_NAME "Vera.ttf"
#define DEFAULT_FIXED_FONT_NAME "DejaVuSansMono.ttf"

  //int w=100 //width of font
  int w = 100; // Set all fonts to NormalFontSize: 
		 // the default when Settings->OSD->FontSizes = "User defined" and no fontsizes entry in setup.conf
  for (int id = 0; id < FONT_NUMFONTS; id++) 
  {

	strcpy(allFonts[id].Name, DEFAULT_TTF_NAME);
	allFonts[id].VdrId = FONT_TRUETYPE; 
	allFonts[id].Default = fontSml;
	allFonts[id].Width = w;
	allFonts[id].Size = NormalFontSize[id];

  }
  int id = FONT_OSDTITLE;
      strcpy(allFonts[id].Name, DEFAULT_TITLEFONT_NAME);
  id = FONT_FIXED;
      strcpy(allFonts[id].Name, DEFAULT_FIXED_FONT_NAME);

#endif   // HAVE_FREETYPE
}

cReelConfig::~cReelConfig()
{
}

void cReelConfig::SetLogoDir(const char *logodirP)
{
  if (logodirP) {
    debug("cReelConfig::SetLogoDir(%s)", logodirP);
    strncpy(logoDir, logodirP, sizeof(logoDir));
    logoDirList.push_back(logodirP);
  }
}

std::string cReelConfig::GetLogoPath(const std::string& channelName, const std::string& ext)
{
      std::string logoPath;
      int i = 0;

      // look through the list of directories, and return when first found
      while (i < logoDirList.size()) {
          logoPath = logoDirList.at(i) + "/" + channelName + ext;

          if (access(logoPath.c_str(), R_OK)==0) // file exists
              return logoPath;

          ++i;
      }

      // file not found in any path, return empty string
      return std::string();
}

void cReelConfig::SetImagesDir(const char *dir)
{
  if (dir) {
    debug("cReelConfig::SetImagesDir(%s)", dir);
    strncpy(strImagesDir, dir, sizeof(strImagesDir));
  }
}

void cReelConfig::SetEPGImagesDir(const char *dir)
{
  if (dir) {
    debug("cReelConfig::SetEPGImagesDir(%s)", dir);
    strncpy(strEPGImagesDir, dir, sizeof(strEPGImagesDir));
  }
}

const char *cReelConfig::GetImageExtension(void)
{
#ifdef HAVE_IMAGEMAGICK
  return (0 <= imageExtension && imageExtension < NUM_IMAGEEXTENSIONTEXTS) ? imageExtensionTexts[imageExtension] : imageExtensionTexts[0];
#else
  return "xpm";
#endif
}

#ifdef HAVE_FREETYPE
void cReelConfig::SetFontsDir(const char *dir)
{
  if (dir) {
    debug("cReelConfig::SetFontsDir(%s)", dir);
    strncpy(strFontsDir, dir, sizeof(strFontsDir));
  }
}
#endif

const cFont *cReelConfig::GetFont(int id, const cFont *pFontCur)
{
  const cFont *res = NULL;
//#ifdef HAVE_FREETYPE
//    ::Setup.UseSmallFont = 1;
//#endif
    
  
  int i;
#ifdef REELVDR
  int fontSizes = ::Setup.FontSizes;
#else
  int fontSizes = FONT_SIZE_USER;
#endif
  
  // maybe should replace loops with single assignment statements ? 
  // faster but... if the user changes to userdefined the older fontsize would still be present
  // this might be a cleaner implementation
    if (fontSizes == FONT_SIZE_NORMAL) // normal fonts
      for (i=0; i < FONT_NUMFONTS; i++) {
        strcpy(allFonts[i].Name, DEFAULT_TTF_NAME); //Name
        allFonts[i].VdrId = FONT_TRUETYPE;
        allFonts[i].Size = NormalFontSize[i];
      } 
    else if (fontSizes == FONT_SIZE_LARGE) // Large fonts
      for (i=0; i < FONT_NUMFONTS; i++) {
                strcpy(allFonts[i].Name, DEFAULT_TTF_NAME); //Name
                allFonts[i].VdrId = FONT_TRUETYPE;
                allFonts[i].Size = LargeFontSize[i];
      }
    else if (fontSizes == FONT_SIZE_SMALL) // SmallFonts
        for (i=0; i < FONT_NUMFONTS; i++) {
                strcpy(allFonts[i].Name, DEFAULT_TTF_NAME); //Name
                allFonts[i].VdrId = FONT_TRUETYPE;
                allFonts[i].Size = SmallFontSize[i];
        }

    if (fontSizes == FONT_SIZE_USER) { /** one userdefined size, so set all font sizes relative to FontOsdSize */ 
        for (i = 0; i < FONT_NUMFONTS; i++) {
             allFonts[i].Size = ::Setup.FontOsdSize + (NormalFontSize[i] - NormalFontSize[0]);
        }
    } else if (fontSizes != FONT_SIZE_USER) { /** not userdefined */
        strcpy(allFonts[FONT_FIXED].Name, DEFAULT_FIXED_FONT_NAME);

	i = FONT_LISTITEMCURRENT;
	strcpy(allFonts[i].Name, allFonts[FONT_LISTITEM].Name);
	allFonts[i].VdrId = allFonts[FONT_LISTITEM].VdrId;
	allFonts[i].Size =  allFonts[FONT_LISTITEM].Size;
	allFonts[i].Width =  allFonts[FONT_LISTITEM].Width + 5;
      
	i = FONT_OSDTITLE;
	strcpy(allFonts[i].Name, DEFAULT_TITLEFONT_NAME);
	i = FONT_FIXED;
	strcpy(allFonts[i].Name, DEFAULT_FIXED_FONT_NAME);
    }
  
  //if (::Setup.UseSmallFont == 1) { // if "Use small font" == "skin dependent"
    if (allFonts[id].VdrId == FONT_TRUETYPE) {
      if (pFontCur) // TTFs can't get patched, so it's always save to return the previous pointer
        return pFontCur;
      if (!isempty(allFonts[id].Name)) {
#ifdef HAVE_FREETYPE
        std::stringstream cachename;
#if VDRVERSNUM < 10503 && !defined(REELVDR)
        cachename << allFonts[id].Name << '_' << allFonts[id].Size << '_' << allFonts[id].Width << '_' << Setup.OSDLanguage;
        if (FontCache.Load(string(strFontsDir) + "/" + string(allFonts[id].Name), cachename.str(), allFonts[id].Size, Setup.OSDLanguage, allFonts[id].Width)) {
#else
        cachename << allFonts[id].Name << '_' << allFonts[id].Size << '_' << allFonts[id].Width << '_' << Setup.OSDLanguage;
        if (FontCache.Load(string(allFonts[id].Name), cachename.str(), allFonts[id].Size, allFonts[id].Width)) {
#endif
          res = FontCache.GetFont(cachename.str());
        } else {
          error("ERROR: ReelNG: Couldn't load font %s:%d", allFonts[id].Name, allFonts[id].Size);
        }
#else
        error("ERROR: ReelNG: Font engine not enabled at compile time!");
#endif
      }
    } else if (allFonts[id].VdrId > FONT_TRUETYPE) {
      res = cFont::GetFont((eDvbFont)(allFonts[id].VdrId - 1));
    } else {
      res = cFont::GetFont((eDvbFont)allFonts[id].VdrId);
    }
 // }

  if (res == NULL)
    res = cFont::GetFont((eDvbFont)allFonts[id].Default);

  if (res && res->Height()<32)
    return res;
  else
  {
    debug("ReelNG: font height (=%d) >= 32, falling back to fontOsd", res->Height()); // comment for logs
    debug("ReelNG: Change font size in ReelNG setup");
    return cFont::GetFont(fontOsd);
  }
}

void cReelConfig::SetFont(int id, const char *font)
{
        
  if (id >= 0 && id < FONT_NUMFONTS && font) {
    char *tmp = (char *) strrchr(font, ':');
    
    if (tmp) {
        strncpy(allFonts[id].Name, font,  (int)(tmp - font) );
        allFonts[id].Name[(int)(tmp - font)] = '\0'; // see "man strncpy"
        allFonts[id].Size = atoi(tmp + 1);
        tmp = strchr(tmp + 1, ',');
      if (tmp) {
        allFonts[id].Width = atoi(tmp + 1);
      }
    } else {
      strncpy(allFonts[id].Name, font, sizeof(allFonts[id].Name));
    }
  }
}

void cReelConfig::SetFont(int id, int vdrId)
{
  if (id >= 0 && id < FONT_NUMFONTS && vdrId >= 0) {
    allFonts[id].VdrId = vdrId;
  }
}

void cReelConfig::GetOsdSize(struct ReelOsdSize *size)
{
  if (size == NULL)
    return;

  size->y = Setup.OSDTop;
  size->x = Setup.OSDLeft;
  size->w = Setup.OSDWidth;
  size->h = Setup.OSDHeight;

#if VDRVERSNUM >= 10504
  if (dynOsd) {
    size->y = cOsd::OsdTop();
    size->x = cOsd::OsdLeft();
    size->w = cOsd::OsdWidth();
    size->h = cOsd::OsdHeight();
  }
#else    
// # ifdef USE_PLUGIN_AVARDS
//   if (dynOsd) {
//     cPlugin *p = cPluginManager::GetPlugin("avards");
//     if (p) {
//       avards_MaxOSDsize_v1_0 OSDsize;
//       if (p->Service(AVARDS_MAXOSDSIZE_SERVICE_STRING_ID, &OSDsize)) {
//         size->y = OSDsize.top;
//         size->x = OSDsize.left;
//         size->w = OSDsize.width;
//         size->h = OSDsize.height;
//       }
//     }
//   }
// # endif
#endif //VDRVERSNUM >= 10504
}
// vim:et:sw=2:ts=2:
