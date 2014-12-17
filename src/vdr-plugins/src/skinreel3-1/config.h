/*
 * config.h: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __SKINREEL_CONFIG_H
#define __SKINREEL_CONFIG_H

#include "common.h"

#ifdef HAVE_IMAGEMAGICK
#define NUM_IMAGEEXTENSIONTEXTS 3
extern const char *imageExtensionTexts[NUM_IMAGEEXTENSIONTEXTS];
#else
#define NUM_IMAGEEXTENSIONTEXTS 0
#endif

#ifdef HAVE_FREETYPE
# include "font.h"
#endif

#include <vdr/skins.h>
#include <vdr/font.h>

#ifndef MAXFONTNAME
#define MAXFONTNAME 64
#endif
#ifndef MAXFONTSIZE
#define MAXFONTSIZE 64
#endif

static const int LargeFontSize [] = {

  20,//             FONT_OSDTITLE,
  17,//             FONT_MESSAGE,
  20,//             FONT_DATE,
  20,//             FONT_HELPKEYS,  // for Red, Green, Yellow, Blue buttons TEXT!
  20,//             FONT_CITITLE,    // Channel Info Title
  18,//             FONT_CISUBTITLE, // Channel Info Short Text below Title
  18,//             FONT_CILANGUAGE, // not used in the current implementation
  22,//             FONT_LISTITEM,   // List items, eg. channellist

  20,//             FONT_INFOTIMERHEADLINE,
  20,//             FONT_INFOTIMERTEXT,
  20,//             FONT_INFOWARNHEADLINE,
  18,//             FONT_INFOWARNTEXT,

  20,//             FONT_DETAILSTITLE, // Recording info & epg info Title
  20,//             FONT_DETAILSSUBTITLE,// Recording info & epg info subtitle/short text
  20,//             FONT_DETAILSDATE, // Recording info & epg info Date
  20,//             FONT_DETAILSTEXT, // Recording info and epg info Details - large text

  17,//             FONT_REPLAYTIMES, // replay time
  20,//             FONT_FIXED,
  20 //             FONT_LISTITEMCURRENT,
     //             FONT_NUMFONTS
};

 static const int NormalFontSize [] = {

  17,//             FONT_OSDTITLE,
  16,//             FONT_MESSAGE,
  16,//             FONT_DATE,
  16,//             FONT_HELPKEYS,
  18,//             FONT_CITITLE,
  14,//             FONT_CISUBTITLE,
  15,//             FONT_CILANGUAGE,
  20,//             FONT_LISTITEM,

  21,//             FONT_INFOTIMERHEADLINE,
  20,//             FONT_INFOTIMERTEXT,
  18,//             FONT_INFOWARNHEADLINE,
  16,//             FONT_INFOWARNTEXT,

  21,//             FONT_DETAILSTITLE,
  18,//             FONT_DETAILSSUBTITLE,
  18,//             FONT_DETAILSDATE,
  18,//             FONT_DETAILSTEXT,

  14,//             FONT_REPLAYTIMES,
  16,//             FONT_FIXED,
  20 //             FONT_LISTITEMCURRENT,
     //             FONT_NUMFONTS
};

static const int SmallFontSize [] = {

  16,//             FONT_OSDTITLE,
  15,//             FONT_MESSAGE,
  15,//             FONT_DATE,
  15,//             FONT_HELPKEYS,
  16,//             FONT_CITITLE,
  15,//             FONT_CISUBTITLE,
  15,//             FONT_CILANGUAGE,
  15,//             FONT_LISTITEM,
  15,//             FONT_INFOTIMERHEADLINE,
  14,//             FONT_INFOTIMERTEXT,
  15,//             FONT_INFOWARNHEADLINE,
  13,//             FONT_INFOWARNTEXT,

  18,//             FONT_DETAILSTITLE,
  16,//             FONT_DETAILSSUBTITLE,
  14,//             FONT_DETAILSDATE,
  13,//             FONT_DETAILSTEXT,
  14,//             FONT_REPLAYTIMES,
  15,//             FONT_FIXED,
  15 //             FONT_LISTITEMCURRENT,
     //             FONT_NUMFONTS
};


struct ReelOsdSize
{
  int x;
  int y;
  int w;
  int h;
};

#define FONT_TRUETYPE 0

enum
{
  FONT_OSDTITLE,
  FONT_MESSAGE,
  FONT_DATE,
  FONT_HELPKEYS,
  FONT_CITITLE,
  FONT_CISUBTITLE,
  FONT_CILANGUAGE,
  FONT_LISTITEM,
  FONT_INFOTIMERHEADLINE,
  FONT_INFOTIMERTEXT,
  FONT_INFOWARNHEADLINE,
  FONT_INFOWARNTEXT,
  FONT_DETAILSTITLE,
  FONT_DETAILSSUBTITLE,
  FONT_DETAILSDATE,
  FONT_DETAILSTEXT,
  FONT_REPLAYTIMES,
  FONT_FIXED,
  FONT_LISTITEMCURRENT, // it must be the last but one item of enum (before FONT_NUMFONTS) since we donot show this in the setup menu
  FONT_NUMFONTS
};

struct FontInfo
{
  int VdrId;
  char Name[MAXFONTNAME + 1];
  int Width;
  int Size;
  int Default;
};

struct FontConfig
{
  int Id;
  const char *KeyId;
  const char *KeyName;
};

#define NR_UNBUFFERED_SLOTS 2

struct CurThumb
{
  int x;
  int y;
  char path[255];
  bool blend;
  int slot; /** valid between 0 and NR_UNBUFFERED_SLOTS */
  int w; /** width */
  int h; /** height */
};

struct ID3Tag {
    std::string name;
    std::string value;
};

extern std::vector<struct ID3Tag> ID3Tags;

extern FontConfig allFontConfig[FONT_NUMFONTS];

struct cReelConfig
{
private:
  char logoDir[255];
  char strImagesDir[255];
  char strEPGImagesDir[255];
#ifdef HAVE_FREETYPE
  char strFontsDir[255];
#endif
public:
  cReelConfig();
  ~cReelConfig();
  void SetLogoDir(const char *logodirP);
  char *GetLogoDir(void) { return logoDir; }
  void SetImagesDir(const char *dir);
  void SetEPGImagesDir(const char *dir);
  char *GetImagesDir(void) { return strImagesDir; }
  char *GetEPGImagesDir(void) { return strEPGImagesDir; }
#ifdef HAVE_FREETYPE
  void SetFontsDir(const char *dir);
  char *GetFontsDir(void) { return strFontsDir; }
#endif
  struct CurThumb curThumb[NR_UNBUFFERED_SLOTS];
  const char *GetImageExtension(void);
  const cFont *GetFont(int id, const cFont *pFontCur = NULL);
  void SetFont(int id, const char *font);
  void SetFont(int id, int vdrId);
  void GetOsdSize(struct ReelOsdSize *size);
  int showAuxInfo;
  int showLogo;
  int showVps;
  int showSymbols;
  int showSymbolsMenu;
  int showSymbolsReplay;
  int showSymbolsMsgs;
  int showSymbolsAudio;
  int showListSymbols;
  int showProgressbar;
  int cacheSize;
  int useChannelId;
  int showInfo;
  int showRemaining;
  int showMarker;
  int singleArea;
  int singleArea8Bpp;
  int numReruns;
  int useSubtitleRerun;
  int showTimerConflicts;
  int showRecSize;
  int showImages;
  int resizeImages;
  int showMailIcon;
  int imageWidth;
  int imageHeight;
  int imageExtension;
  int fullTitleWidth;
  int useTextEffects;
  int scrollDelay;
  int scrollPause;
  int scrollMode;
  int blinkPause;
  int scrollInfo;
  int scrollListItem;
  int scrollOther;
  int scrollTitle;
  int dynOsd;
  int statusLineMode;
  int showWssSymbols;
  int showStatusSymbols;
  int showSourceInChannelinfo;
  int pgBarType;

//  int isLargeFont;
  FontInfo allFonts[FONT_NUMFONTS];
};

extern cReelConfig ReelConfig;

#endif // __SKINREEL_CONFIG_H

// vim:et:sw=2:ts=2:
