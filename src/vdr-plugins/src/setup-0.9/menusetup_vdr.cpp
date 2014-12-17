/****************************************************************************
 * DESCRIPTION:
 *             Setup Menu for setting OSD and Language
 *
 *             Originally taken from vdr 1.4.7 / menu.c
 *             with many changes for reelvdr 1.4.29
 * $Id:$
 *
 * Contact:    reelbox-devel@mailings.reelbox.org
 *
 * Copyright (C) 2006 by Klaus Schmiedinger
 *               2007-1010 by Reel Multimedia GmbH
 ****************************************************************************/

#include <stdlib.h>

/*
#include "menu.h"
#include <ctype.h>
#include <sys/ioctl.h> //TB: needed to determine CAM-state
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "eitscan.h"
#if VDRVERSNUM < 10727
#include "i18n.h"
#endif
#include "interface.h"
#include "plugin.h"
#include "recording.h"
#include "remote.h"
#include "sources.h"
#include "status.h"
#include "themes.h"
#include "timers.h"
#include "transfer.h"
#include "videodir.h"
#include "diseqc.h"
#include "help.h"
#include "sysconfig_vdr.h"
#include "suspend.h"
#ifdef USEMYSQL
  #include "vdrmysql.h"
#endif
#include <sys/statvfs.h>

#include <time.h>
#include <sys/time.h>
#include <algorithm>

*/

#if APIVERSNUM > 10700

#include <vdr/debug.h>
#include <vdr/eitscan.h>
#include <vdr/plugin.h>
#include <vdr/videodir.h>

#include "mainmenu.h"
#include "menusetup_vdr.h"
#include "setupmenu.h"
#include <vdr/sysconfig_vdr.h>


// --- cSetupmenuSetupOSD ---------------------------------------------------------

/*
class cSetupmenuSetupOSD : public cMenuSetupBase {
private:
  const char *useSmallFontTexts[3];
  int osdLanguageIndex;
  const char *FontSizesTexts[3];
  const char *channelViewModeTexts[3];
  const char *ScrollBarWidthTexts[6];

  int numSkins;
  bool showSkinSelection;
  int originalSkinIndex;
  int skinIndex;
  const char **skinDescriptions;
  cThemes themes;
  int originalThemeIndex;
  int themeIndex;
  cFileNameList fontNames;
  cStringList fontOsdNames, fontSmlNames, fontFixNames;
  int fontOsdIndex, fontSmlIndex, fontFixIndex;
  virtual void Set(void);
  void ExpertMenu(void);            // ExpertMenu for OSD
  void DrawExpertMenu(void);        // Draw ExpertMenu
  bool expert;

// Language option
  int originalNumLanguages;
  int numLanguages;
  int optLanguages;
  char oldOSDLanguage[8];
  int oldOSDLanguageIndex;
  //int osdLanguageIndex;
  int ExpertOptions_old;
  bool stored;

public:
  cSetupmenuSetupOSD(void);
  virtual ~cSetupmenuSetupOSD();
  virtual eOSState ProcessKey(eKeys Key);

// Language option
  eOSState LangProcessKey(eKeys Key); // if language changed call this processkey()
  };
*/

cSetupmenuSetupOSD::cSetupmenuSetupOSD(void)
{
    data = Setup;
    //printf("\n%s\n",__PRETTY_FUNCTION__);
    osdLanguageIndex = I18nCurrentLanguage();
    numSkins = Skins.Count();
    skinIndex = originalSkinIndex = Skins.Current()->Index();
    skinDescriptions = new const char *[numSkins];
    themes.Load(Skins.Current()->Name());
    themeIndex = originalThemeIndex =
        Skins.Current()->Theme()? themes.GetThemeIndex(Skins.Current()->Theme()->
                                                       Description()) : 0;
    cFont::GetAvailableFontNames(&fontOsdNames);
    cFont::GetAvailableFontNames(&fontSmlNames);
    cFont::GetAvailableFontNames(&fontFixNames, true);
    fontOsdNames.Insert(strdup(DefaultFontOsd));
    fontSmlNames.Insert(strdup(DefaultFontSml));
    fontFixNames.Insert(strdup(DefaultFontFix));
    fontOsdIndex = std::max(0, fontOsdNames.Find(Setup.FontOsd));
    fontSmlIndex = std::max(0, fontSmlNames.Find(Setup.FontSml));
    fontFixIndex = std::max(0, fontFixNames.Find(Setup.FontFix));
    showSkinSelection = 0;
    expert = false;

    // Langugage

    for (numLanguages = 0;
         numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0;
         numLanguages++)
        ;
    originalNumLanguages = numLanguages;
    optLanguages = originalNumLanguages - 1;
    //optLanguages = 1;
    //oldOSDLanguage = Setup.OSDLanguage;
    strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
    oldOSDLanguageIndex = osdLanguageIndex = I18nCurrentLanguage();
    stored = false;
    ExpertOptions_old = data.ExpertOptions;

    wantChOnOkModes.Clear();
    wantChOnOkModes.Append(strdup(tr("channelinfo"))); // At(0) always
    wantChOnOkModes.Append(strdup(tr("Standard")));
    wantChOnOkModes.Append(strdup(tr("Favourites")));
    wantChOnOkModes.Append(strdup(tr("Favourit folders")));

    Set();
}

cSetupmenuSetupOSD::~cSetupmenuSetupOSD()
{
    //DDD("%s",__PRETTY_FUNCTION__);
    delete[]skinDescriptions;

    // Language
    if (!stored)
    {
        strn0cpy(Setup.OSDLanguage, oldOSDLanguage, 6);
        Setup.EPGLanguages[0] = oldOSDLanguageIndex;
        I18nSetLanguage(Setup.EPGLanguages[0]);
        //DrawMenu();
        //Set();
    }

}

void cSetupmenuSetupOSD::Set(void)
{
    SetCols(30);
    SetSection(tr("OSD & Language"));
    int current = Current();

    useSmallFontTexts[0] = trVDR("never");
    useSmallFontTexts[1] = trVDR("skin dependent");
    useSmallFontTexts[2] = trVDR("always");

    FontSizesTexts[FONT_SIZE_USER] = trVDR("User defined");
    FontSizesTexts[FONT_SIZE_SMALL] = trVDR("Small");
    FontSizesTexts[FONT_SIZE_NORMAL] = trVDR("FontSize$Normal");
    //FontSizesTexts[FONT_SIZE_LARGE] = trVDR("Large");

    channelViewModeTexts[0] = tr("channellist");
    channelViewModeTexts[1] = tr("current bouquet");
    channelViewModeTexts[2] = tr("bouquet list");

    Clear();

// Language
    Add(new
        cMenuEditStraItem(trVDR("Setup.OSD$Language"), &data.EPGLanguages[0],
                          I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));

    Add(new cOsdItem("", osUnknown, false));    // blank line
    Add(new
        cMenuEditBoolItem(trVDR("Setup.DVB$Display subtitles"), &data.DisplaySubtitles));
    Add(new cOsdItem("", osUnknown, false));    // blank line

// OSD Settings
    Add(new
        cMenuEditStraItem(tr("Setup.OSD$OSD color"), &themeIndex, themes.NumThemes(),
                          themes.Descriptions()));
#if APIVERSNUM > 10700
    Add(new cMenuEditStraItem(trVDR("Setup.OSD$OSD Font Size"), &data.FontSizes, sizeof(FontSizesTexts) / sizeof(char *), FontSizesTexts));
    if (data.FontSizes == FONT_SIZE_USER)
        Add(new cMenuEditPrcItem(tr("Setup.OSD$Default font size (%)"), &data.FontOsdSizeP, 0.01, 0.1, 1));
    // Small and fix will be calculated
#else
    Add(new
        cMenuEditStraItem(trVDR("Setup.OSD$OSD Font Size"), &data.FontSizes,
                          sizeof(FontSizesTexts) / sizeof(char *), FontSizesTexts));
    if (data.FontSizes == FONT_SIZE_USER)
    {
        Add(new
            cMenuEditIntItem(trVDR("Setup.OSD$OSD font size (pixel)"), &data.FontOsdSize,
                             MINFONTSIZE, MAXFONTSIZE));
        //Add(new cMenuEditIntItem( trVDR("Setup.OSD$Small font size (pixel)"),&data.FontSmlSize, MINFONTSIZE, MAXFONTSIZE));
        //Add(new cMenuEditIntItem( trVDR("Setup.OSD$Fixed font size (pixel)"),&data.FontFixSize, MINFONTSIZE, MAXFONTSIZE));
    }
#endif

    if (!strcmp(Skins.Current()->Name(), "Reel") == 0)
        Add(new
            cMenuEditStraItem(trVDR("Setup.OSD$Use small font"), &data.UseSmallFont, 3,
                              useSmallFontTexts));
    Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Scroll pages"), &data.MenuScrollPage));
    Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Scroll wraps"), &data.MenuScrollWrap));
    Add(new
        cMenuEditIntItem(trVDR("Setup.OSD$Channel info time (s)"), &data.ChannelInfoTime,
                         1, 60));
    /*
    Add(new
        cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"), &data.UseBouquetList,
                          3, channelViewModeTexts));
    */
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Random menu placement"), &data.OSDRandom));
    //TODO: fix for RB ICE
    if (cPluginManager::GetPlugin("reelbox"))
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Show preview videos"), &data.PreviewVideos));

    Add(new cOsdItem("", osUnknown, false));    // blank line

    Add(new
        cMenuEditBoolItem(tr("Setup.OSD$Show expert menu options"), &data.ExpertOptions));
    if (data.ExpertOptions)
    {
        Add(new
            cMenuEditBoolItem(tr("Setup.OSD$  Use extended navigation keys"),
                              &data.ExpertNavi));

        Add(new
            cMenuEditStraItem(tr("Setup.OSD$  Ok key in TV mode"), &data.WantChListOnOk,
                              wantChOnOkModes.Size(), &wantChOnOkModes.At(0)));
        Add(new
            cMenuEditBoolItem(tr("Setup.OSD$  Program +/- keys"),
                              &data.ChannelUpDownKeyMode, tr("Standard"),
                              tr("Channell./Bouquets")));
    }

/* sent to expertmenu
    // new items
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Ok shows"),                 &data.WantChListOnOk, trVDR("channelinfo"), trVDR("channellist")));
    Add(new cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"),  &data.UseBouquetList, 3, channelViewModeTexts));
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Channel +/- keys"),  &data.ChannelUpDownKeyMode, trVDR("Standard"), trVDR("Bouq./chan. list") ));
*/

    //Add(new cMenuEditStraItem(trVDR("Setup.OSD$Language"),               &data.OSDLanguage, I18nLanguages()->Size(), I18nLanguages()));
    //Add(new cMenuEditIntItem( trVDR("Setup.EPG$Preferred languages"),    &numLanguages, 1, I18nLanguages()->Size()));
    //for (int i = 1; i < numLanguages; i++) {
    //Add(new cMenuEditStraItem(trVDR(" Setup.EPG$Preferred language"),     &data.EPGLanguages[i], I18nLanguages()->Size(), I18nLanguages()));
    // }
    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Timeout requested channel info"), &data.TimeoutRequChInfo));
    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Menu button closes"),     &data.MenuButtonCloses));
    //Add(new cMenuEditStraItem(trVDR("Setup.OSD$Use small font"),         &data.UseSmallFont, 3, useSmallFontTexts));


    if (data.ExpertOptions)
        SetHelp(NULL, NULL, NULL, tr("Expertmenu"));
    else
        SetHelp(NULL);
    SetCurrent(Get(current));
    Display();
}

eOSState cSetupmenuSetupOSD::ProcessKey(eKeys Key)
{
    bool ModifiedApperance = false;
    int oldFontSizes = data.FontSizes;
    int setupFontSizes = Setup.FontSizes; // LangProcessKey() modifies Setup...
    eOSState state = LangProcessKey(Key);       //cMenuSetupBase::ProcessKey(Key);

    // go back to normal menu from expert menu, if kBack is pressed
    if (expert && Key == kBack) {
        expert = false;
        Set();
        return osContinue;
    }
    if (Key == kBlue)
    {
        expert = true;
        ExpertMenu();
        state = osContinue;
    }
    else if (Key == kYellow)
    {
        showSkinSelection = !showSkinSelection;
        if (expert)
            DrawExpertMenu();
        //Set();
        state = osContinue;
    }
    else if (Key == kOk)
    {
        I18nSetLocale(data.OSDLanguage);
#ifdef RBLITE
        Skins.SetCurrent("Reel");
#else
        if (skinIndex != originalSkinIndex)
        {
            cSkin *Skin = Skins.Get(skinIndex);
            if (Skin)
            {
                Utf8Strn0Cpy(data.OSDSkin, Skin->Name(), sizeof(data.OSDSkin));
                Skins.SetCurrent(Skin->Name());
                ModifiedApperance = true;
            }
        }
#endif
        if (themes.NumThemes() && Skins.Current()->Theme())
        {
            // hw data.UseSmallFont=2;
            //Skins.SetCurrent("Reel");
            Skins.Current()->Theme()->Load(themes.FileName(themeIndex));
            Utf8Strn0Cpy(data.OSDTheme, themes.Name(themeIndex), sizeof(data.OSDTheme));
            ModifiedApperance |= themeIndex != originalThemeIndex;
        }
        if (data.OSDLeft != Setup.OSDLeft || data.OSDTop != Setup.OSDTop
            || data.OSDWidth != Setup.OSDWidth || data.OSDHeight != Setup.OSDHeight)
        {
            data.OSDWidth &= ~0x07;     // OSD width must be a multiple of 8
            ModifiedApperance = true;
        }
        if (data.UseSmallFont != Setup.UseSmallFont || data.AntiAlias != Setup.AntiAlias)
            ModifiedApperance = true;
        Utf8Strn0Cpy(data.FontOsd, fontOsdNames[fontOsdIndex], sizeof(data.FontOsd));
        Utf8Strn0Cpy(data.FontSml, fontSmlNames[fontSmlIndex], sizeof(data.FontSml));
        Utf8Strn0Cpy(data.FontFix, fontFixNames[fontFixIndex], sizeof(data.FontFix));
        if (strcmp(data.FontOsd, Setup.FontOsd) || data.FontOsdSize != Setup.FontOsdSize)
        {
            cFont::SetFont(fontOsd, data.FontOsd, data.FontOsdSize);
            ModifiedApperance = true;
        }
        if (strcmp(data.FontSml, Setup.FontSml) || data.FontSmlSize != Setup.FontSmlSize)
        {
            cFont::SetFont(fontSml, data.FontSml, data.FontSmlSize);
            ModifiedApperance = true;
        }
        if (strcmp(data.FontFix, Setup.FontFix) || data.FontFixSize != Setup.FontFixSize)
        {
            cFont::SetFont(fontFix, data.FontFix, data.FontFixSize);
            ModifiedApperance = true;
        }
#if APIVERSNUM > 10700
        if(setupFontSizes != data.FontSizes) {
             switch (data.FontSizes) {
                case FONT_SIZE_LARGE:
                    data.FontOsdSizeP = 0.035; // 20
                    data.FontSmlSizeP = 0.028; // 16
                    data.FontFixSizeP = 0.031; // 18
                    break;
                case FONT_SIZE_NORMAL:
                    data.FontOsdSizeP = 0.031; // 18
                    data.FontSmlSizeP = 0.024; // 14
                    data.FontFixSizeP = 0.028; // 16
                    break;
                case FONT_SIZE_SMALL:
                    data.FontOsdSizeP = 0.028; // 16
                    data.FontSmlSizeP = 0.021; // 12
                    data.FontFixSizeP = 0.024; // 14
                    break;
                case FONT_SIZE_USER:
                    data.FontSmlSizeP = data.FontOsdSizeP * 3 / 4;
                    data.FontFixSizeP = data.FontOsdSizeP * 7 / 8;
                default:
                    break;
            } // switch
            ModifiedApperance = true;
        } // if
#endif

#ifdef USE_TTXTSUBS
        data.SupportTeletext = data.DisplaySubtitles;
        // re-tune to activate/deactivate Subtitles on current channel
        if (Setup.SupportTeletext != data.SupportTeletext)
        {
            Setup = data;
            Channels.SwitchTo(cDevice::CurrentChannel());  // active subtitles immediately on current channel
        }
        else
#endif

        Setup = data;
    } // Key == kOk

    int oldSkinIndex = skinIndex;
    int oldOsdLanguageIndex = osdLanguageIndex;
    //eOSState state = LangProcessKey(Key);       //cMenuSetupBase::ProcessKey(Key);

    if (ModifiedApperance) {
        cOsdProvider::UpdateOsdSize(true);
        SetDisplayMenu();
    } // if

    if (Key == kLeft || Key == kRight)
    {
        if (skinIndex != oldSkinIndex)
        {
            cSkin *Skin = Skins.Get(skinIndex);
            if (Skin)
            {
                char *d =
                    themes.NumThemes()? strdup(themes.Descriptions()[themeIndex]) : NULL;
                themes.Load(Skin->Name());
                if (skinIndex != oldSkinIndex)
                    themeIndex = d ? themes.GetThemeIndex(d) : 0;
                free(d);
            }
            Set();
            SetHelp(NULL);      // clear HelpKey
            Clear();            // Clear OSD
            SetSection(tr("OSD Settings - Expert options"));    // Title OSD
            DrawExpertMenu();   // Draw New OSD
        }

        if (osdLanguageIndex != oldOsdLanguageIndex /* || skinIndex != oldSkinIndex */ )
        {
            strn0cpy(data.OSDLanguage, I18nLocale(osdLanguageIndex),
                     sizeof(data.OSDLanguage));
            int OriginalOSDLanguage = I18nCurrentLanguage();
            I18nSetLanguage(osdLanguageIndex);

            Set();
            I18nSetLanguage(OriginalOSDLanguage);
        }

        if (data.FontSizes != oldFontSizes)
        {
            oldFontSizes = data.FontSizes;
            Set();
        }

        if (data.ExpertOptions != ExpertOptions_old)
        {
            ExpertOptions_old = data.ExpertOptions;
            if (data.ExpertOptions == 0)
            {
                // Reset special modes to default
                data.ExpertNavi = 0;
                data.ExpertOptions = 0;
                data.WantChListOnOk = 1;
                data.ChannelUpDownKeyMode = 0;
            }
            Set();
        }


#if 0
        if (oldFontSizes != data.FontSizes)
        {
            switch (data.FontSizes)
            {
            case 1:            /* Large */
                data.FontOsdSize = 20;
                data.FontSmlSize = 16;
                data.FontFixSize = 18;
                break;
            case 2:            /* Small */
                data.FontOsdSize = 16;
                data.FontSmlSize = 12;
                data.FontFixSize = 14;
                break;
            case 3:            /* Normal */
                data.FontOsdSize = 18;
                data.FontSmlSize = 14;
                data.FontFixSize = 16;
                break;
            default:
                break;
            }
            cFont::SetFont(fontOsd, data.FontOsd, data.FontOsdSize);
            cFont::SetFont(fontSml, data.FontSml, data.FontSmlSize);
            cFont::SetFont(fontFix, data.FontFix, data.FontFixSize);
            Set();
        }
#endif
    }                           // kLeft || kRight
    else if (Key == kOk)
    {
        Clear();
        Set();
    }

    return state;
}

eOSState cSetupmenuSetupOSD::LangProcessKey(eKeys Key)
{
    bool Modified = false;      //(optLanguages+1 != originalNumLanguages || osdLanguageIndex != data.EPGLanguages[0]);

    int oldnumLanguages = optLanguages + 1;
    int oldPrefLanguage = data.EPGLanguages[0];

    eOSState state = cMenuSetupBase::ProcessKey(Key);

    if (Key != kNone)
    {
        if (optLanguages + 1 != oldnumLanguages)
        {
            Modified = true;
            for (int i = oldnumLanguages; i <= optLanguages; i++)
            {
                Modified = true;
                data.EPGLanguages[i] = 0;
                for (int l = 0; l < I18nLanguages()->Size(); l++)
                {
                    int k;
                    for (k = 0; k < oldnumLanguages; k++)
                    {
                        if (data.EPGLanguages[k] == l)
                            break;
                    }
                    if (k >= oldnumLanguages)
                    {
                        data.EPGLanguages[i] = l;
                        break;
                    }
                }
            }
            data.EPGLanguages[optLanguages + 1] = -1;
        }

        if (oldPrefLanguage != data.EPGLanguages[0])
        {
            //Modified = true;
            strn0cpy(data.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            strn0cpy(Setup.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            data.SubtitleLanguages[0] = data.EPGLanguages[0];
            data.AudioLanguages[0] = data.EPGLanguages[0];
            stored = false;
            //int OriginalOSDLanguage = I18nCurrentLanguage();
            I18nSetLanguage(data.EPGLanguages[0]);
            //DrawMenu();
            Set();

            //I18nSetLanguage(OriginalOSDLanguage);
        }

        if (!Modified)
        {
            for (int i = 1; i <= optLanguages; i++)
            {
                if (data.EPGLanguages[i] !=::Setup.EPGLanguages[i])
                {
                    Modified = true;
                    break;
                }
            }
        }

        if (Modified)
        {
            for (int i = 0; i <= I18nLanguages()->Size(); i++)
            {
                data.AudioLanguages[i] = data.EPGLanguages[i];
                data.SubtitleLanguages[i] = data.EPGLanguages[i];
                if (data.EPGLanguages[i] == -1)
                    break;
            }
            if (Key == kOk)
                cSchedules::ResetVersions();
            else
            {
                DrawExpertMenu();
                //Set();
            }
        }
    }
    if (Key == kOk)
    {
        strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
        stored = true;
    }
    DDD("returning %d", state);
    return state;
}

// --- OSD ExpertMenu Init ---------------------------------------------------------

void cSetupmenuSetupOSD::ExpertMenu(void)
{
    Set();
    SetHelp(NULL);              // clear HelpKey
    Clear();                    // Clear OSD
    SetSection(tr("OSD Settings - Expert options"));    // Title OSD
    DrawExpertMenu();           // Draw New OSD
}

// --- OSD ExpertMenu Draw ---------------------------------------------------------

void cSetupmenuSetupOSD::DrawExpertMenu(void)
{
    ScrollBarWidthTexts[0] = "5";
    ScrollBarWidthTexts[1] = "7";
    ScrollBarWidthTexts[2] = "9";
    ScrollBarWidthTexts[3] = "11";
    ScrollBarWidthTexts[4] = "13";
    ScrollBarWidthTexts[5] = "15";


    int current = Current();

    Clear();

    for (cSkin * Skin = Skins.First(); Skin; Skin = Skins.Next(Skin))
        skinDescriptions[Skin->Index()] = Skin->Description();

    cSkin *Skin = Skins.Get(skinIndex);
    if (Skin && strcmp(Skin->Name(), "Reel"))
        showSkinSelection = 1;

    if (themes.NumThemes())
        if (showSkinSelection == 1)
        {
            Add(new
                cMenuEditStraItem(trVDR("Setup.OSD$Skin"), &skinIndex, numSkins,
                                  skinDescriptions));
        }

    //Add(new cMenuEditStraItem(trVDR("Setup.OSD$Skin color"),            &themeIndex, themes.NumThemes(), themes.Descriptions()));
    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Anti-alias"),             &data.AntiAlias));

    /* font & freetype settings */
    /* don't show these buttons with skinreelng/skinreel3, these settings are done in the skin in this case */
    if (showSkinSelection == 1)
    {
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$Default font"), &fontOsdIndex,
                              fontOsdNames.Size(), &fontOsdNames[0]));
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$Small font"), &fontSmlIndex,
                              fontSmlNames.Size(), &fontSmlNames[0]));
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$Fixed font"), &fontFixIndex,
                              fontFixNames.Size(), &fontFixNames[0]));
        Add(new
            cMenuEditPrcItem(tr("Setup.OSD$Default font size (%)"), &data.FontOsdSizeP,
                             0.01, 0.1, 1));
        Add(new
            cMenuEditPrcItem(tr("Setup.OSD$Small font size (%)"), &data.FontSmlSizeP,
                             0.01, 0.1, 1));
        Add(new
            cMenuEditPrcItem(tr("Setup.OSD$Fixed font size (%)"), &data.FontFixSizeP,
                             0.01, 0.1, 1));
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$Use small font"), &data.UseSmallFont, 3,
                              useSmallFontTexts));

#if 0
        // vdr 1.4
        Add(new cMenuEditIntItem(trVDR("Setup.OSD$Left"), &data.OSDLeft, 0, MAXOSDWIDTH));
        Add(new cMenuEditIntItem(trVDR("Setup.OSD$Top"), &data.OSDTop, 0, MAXOSDHEIGHT));
        Add(new
            cMenuEditIntItem(trVDR("Setup.OSD$Width"), &data.OSDWidth, MINOSDWIDTH,
                             MAXOSDWIDTH));
        Add(new
            cMenuEditIntItem(trVDR("Setup.OSD$Height"), &data.OSDHeight, MINOSDHEIGHT,
                             MAXOSDHEIGHT));
        // vdr 1.7
        Add(new cMenuEditPrcItem(trVDR("Setup.OSD$Left (%)"), &data.OSDLeftP, 0.0, 0.5));
        Add(new cMenuEditPrcItem(trVDR("Setup.OSD$Top (%)"), &data.OSDTopP, 0.0, 0.5));
        Add(new cMenuEditPrcItem(trVDR("Setup.OSD$Width (%)"), &data.OSDWidthP, 0.5, 1.0));
        Add(new cMenuEditPrcItem(trVDR("Setup.OSD$Height (%)"), &data.OSDHeightP, 0.5, 1.0));
#endif
    }

    //Add(new cOsdItem("", osUnknown, false));    // blank line
    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Random menu placement"), &data.OSDRandom));

    //Add(new cOsdItem("", osUnknown, false)); // blank line

    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Remain Time"),              &data.OSDRemainTime));
    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Use Symbol"),         &data.OSDUseSymbol));
    //Add(new cMenuEditStraItem(tr("Setup.OSD$ScrollBar Width"),    &tmpScrollBarWidth, 6, ScrollBarWidthTexts));

    Add(new cOsdItem("", osUnknown, false));    // blank line

    Add(new
        cMenuEditIntItem(tr("Setup.OSD$Optional languages"), &optLanguages, 0,
                         I18nLanguages()->Size(), trVDR("none"), NULL));
    for (int i = 1; i <= optLanguages; i++)
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$ Optional language"),
                              &data.EPGLanguages[i], I18nLanguages()->Size(),
                              &I18nLanguages()->At(0)));

    SetCurrent(Get(current));
    Display();
}


// Languages are in OSD menu
#if 0
// --- cMenuSetupLang ---------------------------------------------------------

class cMenuSetupLang:public cMenuSetupBase
{
  private:
    int originalNumLanguages;
    int numLanguages;
    int optLanguages;
    char oldOSDLanguage[8];
    int oldOSDLanguageIndex;
    int osdLanguageIndex;
    bool stored;
    void DrawMenu(void);
  public:
    cMenuSetupLang(void);
     ~cMenuSetupLang(void);
    virtual eOSState ProcessKey(eKeys Key);
};

cMenuSetupLang::cMenuSetupLang(void)
{
    //printf("\n%s\n",__PRETTY_FUNCTION__);
    for (numLanguages = 0;
         numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0;
         numLanguages++)
        ;
    originalNumLanguages = numLanguages;
    optLanguages = originalNumLanguages - 1;
    //oldOSDLanguage = Setup.OSDLanguage;
    strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
    oldOSDLanguageIndex = osdLanguageIndex = I18nCurrentLanguage();
    stored = false;
    //SetSection(trVDR("Setup.OSD$Language"));
    //SetHelp(trVDR("Button$Scan"));
    DrawMenu();
}

cMenuSetupLang::~cMenuSetupLang(void)
{
    if (!stored)
    {
        strn0cpy(Setup.OSDLanguage, oldOSDLanguage, 6);
        Setup.EPGLanguages[0] = oldOSDLanguageIndex;
        I18nSetLanguage(Setup.EPGLanguages[0]);
        DrawMenu();
    }
}

void cMenuSetupLang::DrawMenu(void)
{
    SetCols(25);
    SetSection(trVDR("Setup.OSD$Language"));
    int current = Current();

    Clear();

    Add(new
        cMenuEditStraItem(trVDR("Setup.OSD$Language"), &data.EPGLanguages[0],
                          I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));
    Add(new
        cMenuEditIntItem(trVDR("Setup.OSD$Optional languages"), &optLanguages, 0,
                         I18nLanguages()->Size(), trVDR("none"), NULL));
    for (int i = 1; i <= optLanguages; i++)
        Add(new
            cMenuEditStraItem(trVDR("Setup.OSD$ Optional language"),
                              &data.EPGLanguages[i], I18nLanguages()->Size(),
                              &I18nLanguages()->At(0)));
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSetupLang::ProcessKey(eKeys Key)
{
    bool Modified = (optLanguages + 1 != originalNumLanguages
                     || osdLanguageIndex != data.EPGLanguages[0]);

    int oldnumLanguages = optLanguages + 1;
    int oldPrefLanguage = data.EPGLanguages[0];

    eOSState state = cMenuSetupBase::ProcessKey(Key);

    if (Key != kNone)
    {
        if (optLanguages + 1 != oldnumLanguages)
        {
            Modified = true;
            for (int i = oldnumLanguages; i <= optLanguages; i++)
            {
                Modified = true;
                data.EPGLanguages[i] = 0;
                for (int l = 0; l < I18nLanguages()->Size(); l++)
                {
                    int k;
                    for (k = 0; k < oldnumLanguages; k++)
                    {
                        if (data.EPGLanguages[k] == l)
                            break;
                    }
                    if (k >= oldnumLanguages)
                    {
                        data.EPGLanguages[i] = l;
                        break;
                    }
                }
            }
            data.EPGLanguages[optLanguages + 1] = -1;
        }

        if (oldPrefLanguage != data.EPGLanguages[0])
        {
            Modified = true;
            strn0cpy(data.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            strn0cpy(Setup.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            stored = false;
            //int OriginalOSDLanguage = I18nCurrentLanguage();
            I18nSetLanguage(data.EPGLanguages[0]);
            DrawMenu();

            //I18nSetLanguage(OriginalOSDLanguage);
        }

        if (!Modified)
        {
            for (int i = 0; i <= optLanguages; i++)
            {
                if (data.EPGLanguages[i] !=::Setup.EPGLanguages[i])
                {
                    Modified = true;
                    break;
                }
            }
        }

        if (Modified)
        {
            for (int i = 0; i <= I18nLanguages()->Size(); i++)
            {
                data.AudioLanguages[i] = data.EPGLanguages[i];
                if (data.EPGLanguages[i] == -1)
                    break;
            }
            if (Key == kOk)
                cSchedules::ResetVersions();
            else
                DrawMenu();
        }
    }
    if (Key == kOk)
    {
        strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
        stored = true;
    }
    return state;
}
#endif

// --- cMenuSetupEPG ---------------------------------------------------------

#if 0
class cMenuSetupEPG:public cMenuSetupBase
{
  private:
    //int originalNumLanguages;
    //int numLanguages;
    void Setup(void);
  public:
    cMenuSetupEPG(void);
    virtual eOSState ProcessKey(eKeys Key);
};

cMenuSetupEPG::cMenuSetupEPG(void)
{
    /*for (numLanguages = 0; numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0; numLanguages++)
       ;
       originalNumLanguages = numLanguages;
     */
    SetSection(trVDR("EPG"));
    SetHelp(trVDR("Button$Scan"));
    Setup();
}

void cMenuSetupEPG::Setup(void)
{
    int current = Current();

    Clear();
    /*
       Add(new cMenuEditIntItem( trVDR("Setup.EPG$EPG scan timeout (h)"),      &data.EPGScanTimeout));
       Add(new cMenuEditIntItem( trVDR("Setup.EPG$EPG bugfix level"),          &data.EPGBugfixLevel, 0, MAXEPGBUGFIXLEVEL));
       Add(new cMenuEditIntItem( trVDR("Setup.EPG$EPG linger time (min)"),     &data.EPGLinger, 0));
       Add(new cMenuEditBoolItem(trVDR("Setup.EPG$Set system time"),           &data.SetSystemTime));
       if (data.SetSystemTime)
       Add(new cMenuEditTranItem(trVDR("Setup.EPG$Use time from transponder"), &data.TimeTransponder, &data.TimeSource));
       Add(new cMenuEditIntItem( trVDR("Setup.EPG$Preferred languages"),       &numLanguages, 0, I18nLanguages()->Size()));
       for (int i = 0; i < numLanguages; i++)
       Add(new cMenuEditStraItem(trVDR("Setup.EPG$Preferred language"),     &data.EPGLanguages[i], I18nLanguages()->Size(), I18nLanguages()));
     */
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSetupEPG::ProcessKey(eKeys Key)
{
    /*
       if (Key == kOk) {
       bool Modified = numLanguages != originalNumLanguages;
       if (!Modified) {
       for (int i = 0; i < numLanguages; i++) {
       if (data.EPGLanguages[i] != ::Setup.EPGLanguages[i]) {
       Modified = true;
       break;
       }
       }
       }
       if (Modified)
       cSchedules::ResetVersions();
       }

       int oldnumLanguages = numLanguages;
     */
    int oldSetSystemTime = data.SetSystemTime;

    eOSState state = cMenuSetupBase::ProcessKey(Key);
    if (Key != kNone)
    {
        if (data.SetSystemTime != oldSetSystemTime)
        {
            /*for (int i = oldnumLanguages; i < numLanguages; i++) {
               data.EPGLanguages[i] = 0;
               for (int l = 0; l < I18nLanguages()->Size(); l++) {
               int k;
               for (k = 0; k < oldnumLanguages; k++) {
               if (data.EPGLanguages[k] == l)
               break;
               }
               if (k >= oldnumLanguages) {
               data.EPGLanguages[i] = l;
               break;
               }
               }
               }
               data.EPGLanguages[numLanguages] = -1;
             */
            Setup();
        }
        if (Key == kRed)
        {
            EITScanner.ForceScan();
            return osEnd;
        }
    }
    return state;
}
#endif


// --- cSetupmenuRecording ------------------------------------------------------

cSetupmenuRecording::cSetupmenuRecording(void)
{
    SetCols(30);
    SetSection(trVDR("Recording"));

    PriorityTexts[0] = tr("low");
    PriorityTexts[1] = tr("normal");
    PriorityTexts[2] = tr("high");

    tmpprio = data.DefaultPriority == 10 ? 0 : data.DefaultPriority == 99 ? 2 : 1;
    tmppauseprio = data.PausePriority == 10 ? 0 : data.PausePriority == 99 ? 2 : 1;

    // Auto No Advt
    cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");
    NoAd = 0;
    ForceMarkad = 0;
    //const char *buf ;
    buf = cSysConfig_vdr::GetInstance().GetVariable("AUTO_NOAD");
    if (buf && strncmp(buf, "yes", 3) == 0)
        NoAd = 1;

    buf = cSysConfig_vdr::GetInstance().GetVariable("FORCE_MARKAD");
    if (buf && strncmp(buf, "yes", 3) == 0)
        ForceMarkad = 1;

    Setup();
}

void cSetupmenuRecording::Setup(void)
{
    int current = Current();

    Clear();
    //Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Record Digital Audio"),         &data.UseDolbyInRecordings));
    Add(new
        cMenuEditIntItem(trVDR("Setup.Recording$Time Buffer at Start (min)"),
                         &data.MarginStart));
    Add(new
        cMenuEditIntItem(trVDR("Setup.Recording$Time Buffer at End (min)"),
                         &data.MarginStop));
    Add(new
        cMenuEditStraItem(trVDR("Setup.Recording$Default priority"), &tmpprio, 3,
                          PriorityTexts));
    Add(new
        cMenuEditIntItem(trVDR("Setup.Recording$Default Lifetime (d)"),
                         &data.DefaultLifetime, 0, MAXLIFETIME, NULL,
                         tr("unlimited")));
    Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Use VPS"), &data.UseVps));
    Add(new
        cMenuEditBoolItem(tr("Setup.Recording$Mark Instant Recording"),
                          &data.MarkInstantRecord));
    Add(new
        cMenuEditIntItem(tr("Setup.Recording$Instant Recording Time (min)"),
                         &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
    Add(new
        cMenuEditIntItem(tr("Setup.Recording$Instant Recording Lifetime"),
                         &data.PauseLifetime, 0, MAXLIFETIME, NULL, tr("unlimited")));
    Add(new
        cMenuEditStraItem(tr("Setup.Recording$Instant Recording Priority"),
                          &tmppauseprio, 3, PriorityTexts));
    //Add(new cMenuEditBoolItem(trVDR("Setup.OSD$Recording directories"),               &data.RecordingDirs));
    //Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Split edited files"),            &data.SplitEditedFiles));
    Add(new
        cMenuEditBoolItem(tr("Setup.Recording$Mark Commercials after Recording"), &NoAd));
    if (NoAd == 1)
        Add(new cMenuEditBoolItem(tr("Setup.Recording$  Prefer new 'markad'"), &ForceMarkad));

    SetCurrent(Get(current));
    SetHelp(NULL);

    Display();
};

/*
  SetSection(trVDR("Recording"));
  Add(new cMenuEditBoolItem( trVDR("Setup.Recording$Record Dolby"),         &data.UseDolbyInRecordings));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Margin at start (min)"),     &data.MarginStart));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Margin at stop (min)"),      &data.MarginStop));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Primary limit"),             &data.PrimaryLimit, 0, MAXPRIORITY));
  Add(new cMenuEditStraItem(trVDR("Setup.Recording$Default priority"),          &tmpprio, 3, PriorityTexts));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Default lifetime (d)"),      &data.DefaultLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditStraItem( trVDR("Setup.Recording$Pause priority"),           &tmppauseprio, 3, PriorityTexts));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Pause lifetime (d)"),        &data.PauseLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Use episode name"),          &data.UseSubtitle));
  Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Use VPS"),                   &data.UseVps));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$VPS margin (s)"),            &data.VpsMargin, 0));
  Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Mark instant recording"),    &data.MarkInstantRecord));
  Add(new cMenuEditStrItem( trVDR("Setup.Recording$Name instant recording"),     data.NameInstantRecord, sizeof(data.NameInstantRecord), trVDR(FileNameChars)));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Instant rec. time (min)"),   &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
  Add(new cMenuEditIntItem( trVDR("Setup.Recording$Max. video file size (MB)"), &data.MaxVideoFileSize, MINVIDEOFILESIZE, MAXVIDEOFILESIZE));
  Add(new cMenuEditBoolItem(trVDR("Setup.Recording$Split edited files"),        &data.SplitEditedFiles));
}
*/

eOSState cSetupmenuRecording::ProcessKey(eKeys Key)
{
    eOSState state = cMenuSetupBase::ProcessKey(Key);
    if (Key == kLeft || Key == kRight)
    {
        Setup();
    }
    if (Key == kNone)
	return osUnknown;
    else
	return state;
}



void cSetupmenuRecording::Store(void)
{
    data.DefaultPriority = tmpprio == 0 ? 10 : tmpprio == 1 ? 50 : 99;
    data.PausePriority = tmppauseprio == 0 ? 10 : tmppauseprio == 1 ? 50 : 99;
    cMenuSetupBase::Store();

    // Store Sysconfig
    cSysConfig_vdr::GetInstance().SetVariable("AUTO_NOAD", NoAd ? "yes" : "no");
    cSysConfig_vdr::GetInstance().SetVariable("FORCE_MARKAD", ForceMarkad ? "yes" : "no");
    cSysConfig_vdr::GetInstance().Save();

    cMenuSetupBase::Store();
};


// --- cSetupmenuReplay ------------------------------------------------------

cSetupmenuReplay::cSetupmenuReplay(void)
{
    SetCols(32);
    SetSection(trVDR("Replay"));
    Add(new
        cMenuEditBoolItem(trVDR("Setup.Replay$Multi speed mode"), &data.MultiSpeedMode));
    //Add(new cMenuEditBoolItem(trVDR("Setup.Replay$Show replay mode"), &data.ShowReplayMode));
    //Add(new cMenuEditIntItem(trVDR("Setup.Replay$Resume ID"), &data.ResumeID, 0, 99));
    Add(new
        cMenuEditIntItem(tr("Setup.Replay$Jump width for green/yellow (min)"),
                         &data.JumpWidth, 0, 10, trVDR("intelligent") ));

#ifdef USE_JUMPPLAY
    Add(new
        cMenuEditBoolItem(tr("Setup.Replay$Jump over cutting-mark"), &data.PlayJump));
/*
    RC: has no effect - fix it?
    Add(new
        cMenuEditBoolItem(tr("Setup.Replay$Resume play after jump"), &data.JumpPlay));
*/
    //Add(new cMenuEditBoolItem(tr("Setup.Replay$Pause at last mark"), &data.PauseLastMark));
    //Add(new cMenuEditBoolItem(tr("Setup.Replay$Reload marks"), &data.ReloadMarks));
#endif /* JUMPPLAY */
    Display();
}

void cSetupmenuReplay::Store(void)
{
    if (Setup.ResumeID != data.ResumeID)
        Recordings.ResetResume();
    cMenuSetupBase::Store();
}

// --- cSetupmenuBackground --------------------------------------------------------
#if 0
class cSetupmenuBackground:public cMenuSetupBase
{
  private:
    const char *updateChannelsTexts[3];
    const char *AddNewChannelsTexts[2];
    const char *ShutDownModes[3];
    int tmpUpdateChannels;
    int tmpAddNewChannels;
    int UpdateCheck;
    int initialVolume;          // volume to be shown in percentage
    virtual void Store(void);
    void Setup(void);
  public:
    cSetupmenuBackground(void);
    virtual eOSState ProcessKey(eKeys Key);
};
#endif

cSetupmenuBackground::cSetupmenuBackground(void)
{
    updateChannelsTexts[0] = trVDR("off");
    updateChannelsTexts[1] = tr("update");   // 3
    updateChannelsTexts[2] = tr("add");      // 5

    AddNewChannelsTexts[0] = tr("at the end");
    AddNewChannelsTexts[1] = tr("to bouquets");

    epgScanModeTexts[0] = trVDR("off");
    epgScanModeTexts[1] = tr("permanent");
    epgScanModeTexts[2] = tr("daily");

    //tmpUpdateChannels = (int)data.UpdateChannels / 2;
    tmpUpdateChannels = ((int)data.UpdateChannels != 0);
    tmpAddNewChannels = data.AddNewChannels;    //XXX change

    if (data.InitialVolume > 0)
        initialVolume = (int)(data.InitialVolume / 2.55);
    else
        initialVolume = data.InitialVolume;
    //printf("inital volume :%i \t data.InitialVolume :%i \n", initialVolume, data.InitialVolume);
    // Update reminder
    cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");

    UpdateCheck = 0;
    const char *buf = cSysConfig_vdr::GetInstance().GetVariable("UPDATECHECK_ENABLE");
    if (buf && strncmp(buf, "yes", 3) == 0)
        UpdateCheck = 1;

    SetCols(30);
    SetSection(tr("Background Services"));
    Setup();
}

void cSetupmenuBackground::Setup(void)
{
    int current = Current();

    Clear();

    //Add(new cMenuEditIntItem( trVDR("Setup.Miscellaneous$Min. event timeout (min)"),   &data.MinEventTimeout, 0, INT_MAX, trVDR("off")));
    //Add(new cMenuEditIntItem( trVDR("Setup.Miscellaneous$Min. user inactivity (min)"), &data.MinUserInactivity, 0, INT_MAX, trVDR("off")));
    //Add(new cMenuEditIntItem( trVDR("Setup.Miscellaneous$SVDRP timeout (s)"),          &data.SVDRPTimeout));

    Add(new
        cMenuEditIntItem(trVDR("Setup.Miscellaneous$Zap timeout (s)"), &data.ZapTimeout,
                         0, INT_MAX, trVDR("off")));

    Add(new cMenuEditBoolItem(trVDR("Setup.DVB$Update channels"), &tmpUpdateChannels ));
/*    Add(new
        cMenuEditStraItem(trVDR("Setup.DVB$Update channels"), &tmpUpdateChannels,
                          3, updateChannelsTexts));
    if (tmpUpdateChannels == 2)
        Add(new
            cMenuEditStraItem(tr("  Update mode"), &tmpAddNewChannels, 2,
                              AddNewChannelsTexts));
	*/

    Add(new
        cMenuEditChanItem(trVDR("Setup.Miscellaneous$Initial channel"),
                          &data.InitialChannel, trVDR("Setup.Miscellaneous$as before")));
    Add(new
        cMenuEditIntItem(tr("Setup.Miscellaneous$Initial Volume (%)"), &initialVolume,
                         -1, 100, trVDR("Setup.Miscellaneous$as before")));

    if (::Setup.ReelboxModeTemp == eModeStandalone
        ||::Setup.ReelboxModeTemp == eModeServer)
        {
#if 0
        Add(new
            cMenuEditIntItem(trVDR("Setup.EPG$EPG scan timeout (h)"),
                             &data.EPGScanTimeout, 0, INT_MAX, trVDR("off")));
	if (data.EPGScanTimeout != 0) {
            Add(new cMenuEditIntItem(tr("EPG$  Last channel for EPG scan"),
                             &data.EPGScanMaxChannel, 0, 5000, tr("unlimited")));
            }
#else
        Add(new cMenuEditStraItem(trVDR("EPG$EPG scan mode"), &data.EPGScanMode, 3, epgScanModeTexts));
        if(data.EPGScanMode) {
            if(data.EPGScanMode == 2)
                Add(new cMenuEditTimeItem(tr("EPG$  Start new scan after"), &data.EPGScanDailyTime));
            Add(new cMenuEditIntItem(tr("EPG$  Last channel for EPG scan"), &data.EPGScanMaxChannel, 0, 5000, tr("unlimited")));
            Add(new cMenuEditIntItem(tr("EPG$  Max concurrent transponders"), &data.EPGScanMaxDevices, 0, MAXDEVICES, tr("unlimited")));
            Add(new cMenuEditIntItem(tr("EPG$  Max busy devices"), &data.EPGScanMaxBusyDevices, 0, MAXDEVICES, tr("unlimited")));
            if(!data.EPGScanTimeout) data.EPGScanTimeout = 3;
            Add(new cMenuEditIntItem(trVDR("EPG$  Inactivity to abort live tv (h)"), &data.EPGScanTimeout, 1, INT_MAX));
        }
#endif
        }
    Add(new
        cMenuEditBoolItem(tr("Setup.Miscellaneous$Update-check on system start"),
                          &UpdateCheck));

    SetCurrent(Get(current));
    SetHelp(tr("Button$Scan EPG"));
    Display();
}

eOSState cSetupmenuBackground::ProcessKey(eKeys Key)
{
    eOSState state = cMenuSetupBase::ProcessKey(Key);
    if (Key == kRed)
    {
        EITScanner.ForceScan();
        Skins.Message(mtInfo, tr("EPG scan started in background"));
        return osContinue;
    }
    else if (Key == kLeft || Key == kRight)
    {
        Setup();
    }
    return state;
}

void cSetupmenuBackground::Store(void)
{

    //printf("\ninitial volume(user selected): %i\n", initialVolume);
    if (initialVolume > 0)
        data.InitialVolume = (int)(2.55 * (initialVolume + 0.5));       // round, since (int)(2.55*100) gives 254 probably is 254.9999
    else
        data.InitialVolume = initialVolume;

    //printf("initial volume (after percent to vdr conversion): %i\n\n", data.InitialVolume);

    if (tmpUpdateChannels == 0)
        data.UpdateChannels = 0;
    else if (tmpUpdateChannels == 1)
        data.UpdateChannels = 5;
/*        data.UpdateChannels = 3;
    else if (tmpUpdateChannels == 2)
        data.UpdateChannels = 5;
*/
    //data.AddNewChannels = tmpAddNewChannels;
    data.AddNewChannels = 1;
    data.EPGScanDailyNext = 0; // Trigger recalculation of value

    // Store Sysconfig
    //cSysConfig_vdr::GetInstance().SetVariable("AUTO_NOAD", NoAd?"yes":"no");
    cSysConfig_vdr::GetInstance().SetVariable("UPDATECHECK_ENABLE",
                                          UpdateCheck ? "yes" : "no");
    cSysConfig_vdr::GetInstance().Save();

    //
    /*
       // Check for change
       //
       const char* buf = cSysConfig_vdr::Instance().GetVariable("AUTO_NOAD");
       if (NoAd)
       {
       if ( !buf || strncmp(buf,"yes") !=0)
       {
       cSysConfig_vdr::Instance().SetVariable("AUTO_NOAD", "yes");
       cSysConfig_vdr::Instance().Save()
       }
       }
       else
       {
       if ( !buf || strncmp(buf,"no") !=0)
       {
       cSysConfig_vdr::Instance().SetVariable("AUTO_NOAD", "no");
       cSysConfig_vdr::Instance().Save()
       }
       }
     */

    cMenuSetupBase::Store();
}

// --- cMenuSetupLiveBuffer --------------------------------------------------

#if 0
class cMenuSetupLiveBuffer:public cMenuSetupBase
{
  private:
    void Setup();
#if 1
    bool hasHD;
#endif
  public:
    eOSState ProcessKey(eKeys Key);
    cMenuSetupLiveBuffer(void);
};
#endif // 0

cMenuSetupLiveBuffer::cMenuSetupLiveBuffer(void)
{
    SetSection(trVDR("Permanent Timeshift"));
    SetCols(30);

    static const char *cmd = "cat /proc/mounts |awk '{ print $2 }'|grep -q '/media/hd'";
    hasHD = !SystemExec(cmd);

    Setup();
}

void cMenuSetupLiveBuffer::Setup(void)
{
#if USE_LIVEBUFFER
    pauseKeyHandlingTexts[0] = trVDR("keine");
    pauseKeyHandlingTexts[1] = trVDR("confirm pause live video");
    pauseKeyHandlingTexts[2] = trVDR("pause live video");

    int current = Current();
    Clear();
    SetCols(25);

    if (hasHD)
    {
        Add(new cMenuEditBoolItem(trVDR("Permanent Timeshift"), &data.LiveBuffer));
        if (data.LiveBuffer)
        {
            Add(new cMenuEditIntItem(tr("Setup.LiveBuffer$Buffer (min)"),
                                 &data.LiveBufferSize, 1, 240));
        }
    }
    else
    {
        std::string buf = std::string(trVDR("Permanent Timeshift")) + ":\t" + trVDR("no");
        Add(new cOsdItem(buf.c_str(), osUnknown, false));
    }

    if (!data.LiveBuffer)
    {
	Add(new cMenuEditStraItem(trVDR("Setup.Recording$Pause key handling"),
					    &data.PauseKeyHandling, 3, pauseKeyHandlingTexts));
    }

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(trVDR("Info:"), osUnknown, false));
    if (hasHD)
    {
	AddFloatingText(tr("With Permanent Timeshift you can pause a running broadcast and proceed it later on."), 48);
    }
    else
    {
        AddFloatingText(tr("Permanent Timeshift can only be activated with a harddisk."), 48);
        if (data.LiveBuffer)    // is currently on
        {
            data.LiveBuffer = 0;
            Store();
            AddFloatingText(tr("Permanent Timeshift has been disabled."), 48);
        }
    }

    SetCurrent(Get(current));
#else
    AddFloatingText(trVDR("Your system does not support Permanent Timeshift."), 48);
#endif
    Display();
}

#if USE_LIVEBUFFER
eOSState cMenuSetupLiveBuffer::ProcessKey(eKeys Key)
{
    int oldLiveBuffer = data.LiveBuffer;
    eOSState state = cMenuSetupBase::ProcessKey(Key);

    static const char *command = "reelvdrd --livebufferdir";
    char *strBuff = NULL;
    FILE *process;
    bool dirok;

    if (Key != kNone && (data.LiveBuffer != oldLiveBuffer))
    {
        Setup();
    }


    if (Key == kOk && data.LiveBuffer == 1)
    {
        Store();
        // re-tune to activate/deactivate Live-Buffer
        Channels.SwitchTo(cDevice::CurrentChannel());  // active timeshift immediately on current channel

        process = popen(command, "r");
        if (process)
        {
            cReadLine readline;
            //DDD("strBuff = readline...");
            strBuff = readline.Read(process);
            if (strBuff != NULL)
                BufferDirectory = strdup(strBuff);
            DDD("Permanent timeshift on, BufferDirectory: %s", BufferDirectory);
        }

        dirok = !pclose(process);

    }

    if (state == osContinue)
	state = osUnknown;
    return state;
}
#endif

// --- cSetupmenuSetupPlugins -----------------------------------------------------

cSetupmenuSetupPlugins::cSetupmenuSetupPlugins(void)
{
    SetSection(tr("System Settings"));
    SetHasHotkeys();
    for (int i = 0; i<200 ; i++)
    {
        cPlugin *p = cPluginManager::GetPlugin(i);
        //if (p && p->HasSetupOptions())
        if (p)
	{
	    if (p->HasSetupOptions())
            {
                if (!p->MenuSetupPluginEntry())
		{
		    Add(new cMenuSetupPluginItem(hk(p->MainMenuEntry()), i));
    		    DDD("new plugin entry: %d %s", i, p->MainMenuEntry());
	        }
		else
		{
		    Add(new cMenuSetupPluginItem(hk(p->MenuSetupPluginEntry()), i));
    		    DDD("new plugin entry: %d %s", i, p->MenuSetupPluginEntry());
                }
            }
            else
	    {
                DDD("cSetupmenuSetupPlugins: plugin %s has no setup options", p->Name());
	    }
	}
	else
	{	// no more plugins
            DDD("found %d plugins", i);
	    break;
	}
    }
    Display();
}

eOSState cSetupmenuSetupPlugins::ProcessKey(eKeys Key)
{
    //printf("\n%s :  \t HasSubMenu()= %d\n",__PRETTY_FUNCTION__, HasSubMenu());
    eOSState state =
        HasSubMenu()? cMenuSetupBase::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);
    DDD("state: %d key: %d", state, Key);
    //should return osUnknown if no key is pressed to auto-close menu
    if (Key == kNone)
       return osUnknown;

    if (Key == kOk)
    {
        if (state == osUnknown)
        {
            cMenuSetupPluginItem *item = (cMenuSetupPluginItem *) Get(Current());
            if (item)
            {
                cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
                if (p)
                {
                    cMenuSetupPage *menu = p->SetupMenu();
                    if (menu)
                    {
                        menu->SetPlugin(p);
                        return AddSubMenu(menu);
                    }
                    Skins.Message(mtInfo, tr("This plugin has no setup parameters!"));
                }
            }
        }
        else if (state == osContinue)
        {
            //  Store();
        }
    }
    return state;
}

#endif // APIVERSNUM > 10700
