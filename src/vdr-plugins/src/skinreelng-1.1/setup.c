/*
 * setup.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include "i18n.h"
#include "logo.h"
#include "setup.h"
#include "tools.h"
#include <iostream>

#ifdef HAVE_FREETYPE
#include "texteffects.h"
#endif

//#define PRINT_FUNC printf("\n %s (%s:%d)\n", __PRETTY_FUNCTION__, __FILE__,__LINE__)
#define PRINT_FUNC ;

static const char *allVdrFonts[] = {
#ifdef HAVE_FREETYPE
  trNOOP("TrueType Font"),
#else
  trNOOP("No TTF support!"),
#endif
  trNOOP("Default OSD Font"),
  trNOOP("Default Fixed Size Font"),
  trNOOP("Default Small Font"),
  SKINREEL_FONTS
};

// cPluginSkinReelSetup
cPluginSkinReelSetup::cPluginSkinReelSetup(void)
{
  // create setup menu
  debug("cPluginSkinReelSetup()");

  data = ReelConfig;
  Setup();
}

void cPluginSkinReelSetup::AddCategory(const char *Title) {
  char *buffer = NULL;

  asprintf(&buffer, "---\t%s ----------------------------------------------------------------------------------", Title );

  cOsdItem *item = new cOsdItem(buffer);
  free(buffer);

  if (item) {
    item->SetSelectable(false);
    Add(item);
  }
}

void cPluginSkinReelSetup::Setup(void)
{
  // update setup display
  int current = Current();

  Clear();

  Add(new cOsdItem(tr("General")));
  Add(new cOsdItem(tr("Logos & Symbols")));
  
#ifdef HAVE_FREETYPE
  ::Setup.UseSmallFont = 1;
#endif
    
  Add(new cOsdItem(tr("Fonts")));

  SetCurrent(Get(current));
  Display();
  SetHelp(NULL, NULL, NULL, NULL);
}

void cPluginSkinReelSetup::Store(void)
{
 PRINT_FUNC;
  // store setup data
  debug("cPluginSkinReelSetup::Store()");

  ReelConfig = data;
  SetupStore("SingleArea8Bpp", ReelConfig.singleArea8Bpp);
  SetupStore("ShowAuxInfo", ReelConfig.showAuxInfo);
  SetupStore("ShowRemaining", ReelConfig.showRemaining);
  SetupStore("ShowProgressBar", ReelConfig.showProgressbar);
  SetupStore("ShowListSymbols", ReelConfig.showListSymbols);
  SetupStore("ShowSymbols", ReelConfig.showSymbols);
  SetupStore("ShowSymbolsMenu", ReelConfig.showSymbolsMenu);
  SetupStore("ShowSymbolsReplay", ReelConfig.showSymbolsReplay);
  SetupStore("ShowSymbolsMsgs", ReelConfig.showSymbolsMsgs);
  SetupStore("ShowSymbolsAudio", ReelConfig.showSymbolsAudio);
  SetupStore("ShowLogo", ReelConfig.showLogo);
  SetupStore("ShowInfo", ReelConfig.showInfo);
  SetupStore("ShowVPS", ReelConfig.showVps);
  SetupStore("ShowFlags", ReelConfig.showFlags);
  SetupStore("ShowMarker", ReelConfig.showMarker);
  SetupStore("CacheSize", ReelConfig.cacheSize);
  SetupStore("UseChannelId", ReelConfig.useChannelId);
  SetupStore("NumReruns", ReelConfig.numReruns);
  SetupStore("UseSubtitleRerun", ReelConfig.useSubtitleRerun);
  SetupStore("ShowTimerConflicts", ReelConfig.showTimerConflicts);
  SetupStore("ShowRecSize", ReelConfig.showRecSize);
  SetupStore("ShowImages", ReelConfig.showImages);
  SetupStore("ResizeImages", ReelConfig.resizeImages);
  SetupStore("ShowMailIcon", ReelConfig.showMailIcon);
  SetupStore("ImageWidth", ReelConfig.imageWidth);
  SetupStore("ImageHeight", ReelConfig.imageHeight);
  SetupStore("ImageExtension", ReelConfig.imageExtension);
  SetupStore("FullTitleWidth", ReelConfig.fullTitleWidth);
  SetupStore("UseTextEffects", ReelConfig.useTextEffects);
  SetupStore("ScrollDelay", ReelConfig.scrollDelay);
  SetupStore("ScrollPause", ReelConfig.scrollPause);
  SetupStore("ScrollMode", ReelConfig.scrollMode);
  SetupStore("BlinkPause", ReelConfig.blinkPause);
  SetupStore("ScrollInfo", ReelConfig.scrollInfo);
  SetupStore("ScrollListItem", ReelConfig.scrollListItem);
  SetupStore("ScrollOther", ReelConfig.scrollOther);
  SetupStore("ScrollTitle", ReelConfig.scrollTitle);
  SetupStore("DynOSD", ReelConfig.dynOsd);
  SetupStore("StatusLineMode", ReelConfig.statusLineMode);
  SetupStore("ShowWssSymbols", ReelConfig.showWssSymbols);
  SetupStore("ShowStatusSymbols", ReelConfig.showStatusSymbols);
  // SetupStore("FontSizes", ReelConfig.isLargeFont);
  
  ::Setup.FontSizes = 0;
  std::cout << "call Setup.Save() from " << __PRETTY_FUNCTION__ << std::endl;
  ::Setup.Save(); // vdr/config.h

/***
 * Set LISTITEMCURRENT and LISTITEM to the same font
 * except LISTITEMCURRENT is wider
 ***/
   int id1 = FONT_LISTITEM;
   int id2 = FONT_LISTITEMCURRENT;

   ReelConfig.allFonts[id2].VdrId = ReelConfig.allFonts[id1].VdrId ;
   ReelConfig.allFonts[id2].Size = ReelConfig.allFonts[id1].Size ;
   ReelConfig.allFonts[id2].Width = ReelConfig.allFonts[id1].Width + 5;
   strcpy(ReelConfig.allFonts[id2].Name, ReelConfig.allFonts[id1].Name) ;

   /** 
    * save all font sizes
    **/
  char tmp[sizeof(ReelConfig.allFonts[0].Name) + 8];
  for (int id = 0; id < FONT_NUMFONTS; id++) {
    SetupStore(allFontConfig[id].KeyId, ReelConfig.allFonts[id].VdrId);
  //  if ( ReelConfig.isLargeFont == 1 ||   ReelConfig.allFonts[id].Name[0] != 0) {
    if ( ReelConfig.allFonts[id].Name[0] != 0) {
      
     // if (ReelConfig.isLargeFont==1) 
     //  {
     //   ReelConfig.allFonts[id].Size = 19 ; // override 
     //   strcpy(ReelConfig.allFonts[id].Name,"lmsans10-regular.otf");
     // }
      snprintf(tmp, sizeof(tmp), "%s:%d,%d", ReelConfig.allFonts[id].Name, ReelConfig.allFonts[id].Size, ReelConfig.allFonts[id].Width);
      SetupStore(allFontConfig[id].KeyName, tmp);
    }
  }

  // resize logo cache
  ReelLogoCache.Resize(ReelConfig.cacheSize);
}

eOSState cPluginSkinReelSetup::ProcessKey(eKeys Key)
{
  bool hadSubMenu = HasSubMenu();
  //eOSState state = cMenuSetupPage::ProcessKey(Key);
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (hadSubMenu && Key == kOk)
  {
    Store();
    Skins.Message(mtInfo, tr("Changes done"),1);
  }

  if (!HasSubMenu() && (state == osUnknown || Key == kOk)) {
    if ((Key == kOk && !hadSubMenu) || Key == kBlue) {
      const char* ItemText = Get(Current())->Text();
      if (strcmp(ItemText, tr("General")) == 0)
        state = AddSubMenu(new cMenuSetupGeneral(&data));
      else if (strcmp(ItemText, tr("Logos & Symbols")) == 0)
        state = AddSubMenu(new cMenuSetupLogos(&data));
      else if (strcmp(ItemText, tr("Fonts")) == 0)
        state = AddSubMenu(new cMenuSetupFonts(&data));
    }
  }

  return state;
}

// Setup: SubMenu
cMenuSetupSubMenu::cMenuSetupSubMenu(const char* Title, cReelConfig* Data) : cOsdMenu(Title, 30)
{
  data = Data;
}

eOSState cMenuSetupSubMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
    switch (Key) {
      case kOk:
        return osBack;
      default:
        break;
    }
  }

  return state;
}

// Setup: General
cMenuSetupGeneral::cMenuSetupGeneral(cReelConfig* Data) : cMenuSetupSubMenu(tr("General"), Data)
{
  showRemainingTexts[0] = tr("elapsed");
  showRemainingTexts[1] = tr("remaining");
  showRemainingTexts[2] = tr("percent");

  showRecSizeTexts[0] = tr("never");
  showRecSizeTexts[1] = tr("use size.vdr only");
  showRecSizeTexts[2] = tr("always");

  statusLineModeTexts[0] = tr("Last line");
  statusLineModeTexts[1] = tr("Help buttons");
  statusLineModeTexts[2] = tr("Free last line");

  Set();
}

eOSState cMenuSetupGeneral::ProcessKey(eKeys Key)
{
  return cMenuSetupSubMenu::ProcessKey(Key);
}

void cMenuSetupGeneral::Set(void)
{
  int current = Current();
  Clear();

  Add(new cMenuEditBoolItem(tr("Try 8bpp single area"), &data->singleArea8Bpp));
  Add(new cMenuEditBoolItem(tr("Show info area in main menu"), &data->showInfo));
  Add(new cMenuEditBoolItem(tr("Show auxiliary information"), &data->showAuxInfo, trVDR("top"), trVDR("bottom")));
  Add(new cMenuEditStraItem(tr("Show recording's size"), &data->showRecSize, 3, showRecSizeTexts));
  //Add(new cMenuEditBoolItem(tr("Full title width"), &data->fullTitleWidth));
  Add(new cMenuEditStraItem(tr("Show remaining/elapsed time"), &data->showRemaining, 3, showRemainingTexts));
  Add(new cMenuEditBoolItem(tr("Show VPS"), &data->showVps));
  Add(new cMenuEditBoolItem(tr("Show progressbar"), &data->showProgressbar));
  Add(new cMenuEditStraItem(tr("Show messages in menu on"), &data->statusLineMode, 3, statusLineModeTexts));

  SetCurrent(Get(current));
  Display();
  SetHelp(NULL, NULL, NULL, NULL);
}

// Setup: Logos & Symbols
cMenuSetupLogos::cMenuSetupLogos(cReelConfig* Data) : cMenuSetupSubMenu(tr("Logos & Symbols"), Data)
{
#ifndef SKINREEL_NO_MENULOGO
  resizeImagesTexts[0] = tr("pixel algo");
  resizeImagesTexts[1] = tr("ratio algo");
  resizeImagesTexts[2] = tr("zoom image");
#endif

  Set();
}

eOSState cMenuSetupLogos::ProcessKey(eKeys Key)
{
  int oldShowLogo = data->showLogo;
  int oldShowSymbols = data->showSymbols;
  int oldShowStatusSymbols = data->showStatusSymbols;
  int oldShowSymbolsMenu = data->showSymbolsMenu;
  int oldShowImages = data->showImages;

  eOSState state = cMenuSetupSubMenu::ProcessKey(Key);

  if ((state == osUnknown) && (Key == kRed)) {
    Skins.Message(mtInfo, tr("Flushing channel logo cache..."));
    ReelLogoCache.Flush();
    Skins.Message(mtInfo, NULL);
    state = osContinue;
  }

  if (Key != kNone &&
      ((data->showLogo != oldShowLogo) 
       || (data->showSymbols != oldShowSymbols) 
       || (data->showStatusSymbols != oldShowStatusSymbols) 
       || (data->showSymbolsMenu != oldShowSymbolsMenu) 
       || (data->showImages != oldShowImages)
      )) {
    Set();
  }

  if (state == osUnknown) {
    switch (Key) {
      case kOk:
        return osBack;
      default:
        break;
    }
  }

  return state;
}

void cMenuSetupLogos::Set(void)
{
  int current = Current();
  Clear();

  Add(new cMenuEditBoolItem(tr("Show symbols"), &data->showSymbols));    //TODO? symbols -> icons

  if (data->showSymbols) {
#ifndef SKINREEL_NO_MENULOGO
    Add(new cMenuEditBoolItem(tr("  Show symbols in menu"), &data->showSymbolsMenu));
#endif
    Add(new cMenuEditBoolItem(tr("  Show symbols in replay"), &data->showSymbolsReplay));
    Add(new cMenuEditBoolItem(tr("  Show symbols in messages"), &data->showSymbolsMsgs));
    Add(new cMenuEditBoolItem(tr("  Show symbols in audio"), &data->showSymbolsAudio));
  }

  Add(new cMenuEditBoolItem(tr("Show symbols in lists"), &data->showListSymbols));
  Add(new cMenuEditBoolItem(tr("Show marker in lists"), &data->showMarker));
  Add(new cMenuEditBoolItem(tr("Show status symbols"), &data->showStatusSymbols));
  if (data->showStatusSymbols) {
    Add(new cMenuEditBoolItem(tr("  Show flags"), &data->showFlags));
  }

#ifndef SKINREEL_NO_MENULOGO
  if (data->showSymbols && data->showSymbolsMenu) {
    Add(new cMenuEditBoolItem(tr("Show event/recording images"), &data->showImages));
    if (data->showImages) {
#ifdef HAVE_IMAGEMAGICK
      Add(new cMenuEditStraItem(tr("  Resize images"), &data->resizeImages, 3, resizeImagesTexts));
#endif
      Add(new cMenuEditIntItem(tr("  Image width" ), &data->imageWidth, 80, 180));
     Add(new cMenuEditIntItem(tr("  Image height" ), &data->imageHeight, 80, 144));
#ifdef HAVE_IMAGEMAGICK
     Add(new cMenuEditStraItem(tr("  Image format"), &data->imageExtension, 3, imageExtensionTexts));
#endif
    }
  }
#endif //SKINREEL_NO_MENULOGO

  Add(new cMenuEditBoolItem(tr("Show channel logos"), &data->showLogo));
  if (data->showLogo) {
    Add(new cMenuEditBoolItem(tr("  Identify channel by"), &data->useChannelId, tr("name"), tr("data")));
  }
  if (data->showLogo || data->showSymbols) {
    Add(new cMenuEditIntItem(tr("Channel logo cache size"), &data->cacheSize, 0, 1000));
  }

  SetCurrent(Get(current));
  Display();
  SetHelp(tr("Button$Flush cache"), NULL, NULL, NULL);
}


#ifdef HAVE_FREETYPE
// Setup: TTF
#if VDRVERSNUM < 10504 && !defined(REELVDR)
cMenuSetupTTF::cMenuSetupTTF(FontInfo* Data) : cOsdMenu(tr("TrueType Font"), 10)
#else // VDRVERSNUM >= 10504
cMenuSetupTTF::cMenuSetupTTF(FontInfo* Data, cStringList* fonts) : cOsdMenu(tr("TrueType Font"), 10)
#endif // VDRVERSNUM < 10504
{
  data = Data;
#if VDRVERSNUM < 10504 && !defined(REELVDR)
  availTTFs = ReelTextEffects.GetAvailTTFs();
  if (availTTFs && data) {
    nMaxTTFs = ReelTextEffects.GetNumAvailTTFs();
    nFont = 0;
    for (int i = 0; i < nMaxTTFs; i++) {
      if (availTTFs[i]) {
        if (strcmp(availTTFs[i], data->Name) == 0) {
          nFont = i;
          break;
        }
      }
    }
    nWidth = data->Width;
    nSize = data->Size;
  }
#else // VDRVERSNUM >= 10504
  if (data && fonts) {
    fontList = fonts;
    nFont = std::max(0, fontList->Find(data->Name));
    nWidth = data->Width;
    nSize = data->Size;
  }
#endif // VDRVERSNUM < 10504

  SetHelp(NULL, NULL, NULL, NULL);
  Set();
}

void cMenuSetupTTF::Set(void)
{
  int current = Current();
  Clear();

#if VDRVERSNUM < 10504 && !defined(REELVDR)
  if (availTTFs) {
    Add(new cMenuEditStraItem(tr("Name"), &nFont, nMaxTTFs, availTTFs));
#else // VDRVERSNUM >= 10504
  if (fontList->Size() > 0) {
    Add(new cMenuEditStraItem(tr("Name"), &nFont, fontList->Size(), &(*fontList)[0]));
#endif // VDRVERSNUM < 10504
    Add(new cMenuEditIntItem(tr("Size"), &nSize, 10, MAXFONTSIZE));
#if VDRVERSNUM < 10503 || VDRVERSNUM >= 10505
    //VDR 1.5.2 - 1.5.4 can't set TTF width
    Add(new cMenuEditIntItem(tr("Width"), &nWidth, 50, 150));
#endif
    SetCurrent(Get(current));
  } else {
    cOsdItem *item = new cOsdItem(tr("No TrueType fonts installed!"));

    if (item) {
      item->SetSelectable(false);
      Add(item);
    }
  }

  Display();
}

eOSState cMenuSetupTTF::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
    switch (Key) {
      case kOk:
        Store();
        state = osBack;
        break;
      default:
        break;
    }
  }

  return state;
}

void cMenuSetupTTF::Store(void)
{
 PRINT_FUNC;
#if VDRVERSNUM < 10504 && !defined(REELVDR)
  if (data && availTTFs) {
    strncpy(data->Name, availTTFs[nFont], sizeof(data->Name));
    data->Width = nWidth;
    data->Size = nSize;
  }
#else // VDRVERSNUM >= 10504
  if (data) {
    Utf8Strn0Cpy(data->Name, (*fontList)[nFont], sizeof(data->Name));
    data->Width = nWidth;
    data->Size = nSize;
  }
#endif // VDRVERSNUM < 10504
}
#endif

// Setup: Fonts
cMenuSetupFonts::cMenuSetupFonts(cReelConfig* Data) : cMenuSetupSubMenu(tr("Fonts"), Data)
{
#ifdef HAVE_FREETYPE
  allVdrFonts[0] = tr("TrueType Font");
#else
  allVdrFonts[0] = tr("No TTF support!");
#endif
  allVdrFonts[1] = tr("Default OSD Font");
  allVdrFonts[2] = tr("Default Fixed Size Font");
  allVdrFonts[3] = tr("Default Small Font");

#ifdef HAVE_FREETYPE
#if VDRVERSNUM >= 10504 || defined(REELVDR)
  cFont::GetAvailableFontNames(&fontMonoNames, true);
  cFont::GetAvailableFontNames(&fontNames);
#endif
#endif

  Set();
}

cMenuSetupFonts::~cMenuSetupFonts(void)
{
}

eOSState cMenuSetupFonts::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupSubMenu::ProcessKey(Key);
  
  if ( strcmp(Get(Current())->Text(),tr("Font Sizes")) == 0 && Key == kOk) 
    return osBack;

#ifdef HAVE_FREETYPE
  if (state == osUnknown && Key == kBlue && data->allFonts[Current()].VdrId == FONT_TRUETYPE) {
#if VDRVERSNUM < 10504 && !defined(REELVDR)
    state = AddSubMenu(new cMenuSetupTTF(&data->allFonts[Current()]));
#else // VDRVERSNUM >= 10504
    state = AddSubMenu(new cMenuSetupTTF(&data->allFonts[Current()], strncmp(Get(Current())->Text(), tr("Fixed Font"), strlen(tr("Fixed Font"))) == 0 ? &fontMonoNames : &fontNames));
#endif
  } else {
    if (!HasSubMenu() && data->allFonts[Current()].VdrId == FONT_TRUETYPE)
      SetHelp(NULL, NULL, NULL, tr("Button$Set"));
    else
      SetHelp(NULL, NULL, NULL, NULL);
  }
#endif

  return state;
}

void cMenuSetupFonts::Set(void)
{
  int current = Current();
  Clear();

  int numAvailFonts = sizeof(allVdrFonts)/sizeof(char*);
 /* static const char *test[3] = {
                         "Set Seperately",
                         "All Large",
                         "All Small"
                         };
  
  Add(new cMenuEditStraItem(tr("Font sizes"), &data->isLargeFont, 3, test));
  */
  Add(new cMenuEditStraItem(tr("OSD title"), &data->allFonts[FONT_OSDTITLE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Messages"), &data->allFonts[FONT_MESSAGE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Date"), &data->allFonts[FONT_DATE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Help keys"), &data->allFonts[FONT_HELPKEYS].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Channelinfo: title"), &data->allFonts[FONT_CITITLE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Channelinfo: subtitle"), &data->allFonts[FONT_CISUBTITLE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Channelinfo: language"), &data->allFonts[FONT_CILANGUAGE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("List items"), &data->allFonts[FONT_LISTITEM].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Info area: timers title"), &data->allFonts[FONT_INFOTIMERHEADLINE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Info area: timers text"), &data->allFonts[FONT_INFOTIMERTEXT].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Info area: warning title"), &data->allFonts[FONT_INFOWARNHEADLINE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Info area: warning text"), &data->allFonts[FONT_INFOWARNTEXT].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Details: title"), &data->allFonts[FONT_DETAILSTITLE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Details: subtitle"), &data->allFonts[FONT_DETAILSSUBTITLE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Details: date"), &data->allFonts[FONT_DETAILSDATE].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Details: text"), &data->allFonts[FONT_DETAILSTEXT].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Replay: times"), &data->allFonts[FONT_REPLAYTIMES].VdrId, numAvailFonts, allVdrFonts));
  Add(new cMenuEditStraItem(tr("Fixed Font"), &data->allFonts[FONT_FIXED].VdrId, numAvailFonts, allVdrFonts));
  //Add(new cMenuEditStraItem(tr("List items Current"), &data->allFonts[FONT_LISTITEMCURRENT].VdrId, numAvailFonts, allVdrFonts));

  SetCurrent(Get(current));
  Display();
#ifdef HAVE_FREETYPE
  if (data->allFonts[Current()].VdrId == FONT_TRUETYPE)
    SetHelp(NULL, NULL, NULL, tr("Button$Set"));
  else
#endif
    SetHelp(NULL, NULL, NULL, NULL);
}

// vim:et:sw=2:ts=2:
