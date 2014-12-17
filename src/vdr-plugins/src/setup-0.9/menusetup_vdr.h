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

#if APIVERSNUM >= 10700

#include <vdr/menu.h>
#include <vdr/plugin.h>

#include "mainmenu.h"
#include "setupmenu.h"


#define MAXINSTANTRECTIME (24 * 60 - 1) // 23:59 hours

// --- cSetupmenuSetupOSD ---------------------------------------------------------

class cSetupmenuSetupOSD:public cMenuSetupBase
{
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
    void ExpertMenu(void);      // ExpertMenu for OSD
    void DrawExpertMenu(void);  // Draw ExpertMenu
    bool expert;

    // list of options for Setup.WantChListOnOk
    cStringList wantChOnOkModes;

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
      virtual ~ cSetupmenuSetupOSD();
    virtual eOSState ProcessKey(eKeys Key);

// Language option
    eOSState LangProcessKey(eKeys Key); // if language changed call this processkey()
};


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

#endif


// --- cSetupmenuRecording ------------------------------------------------------
class cSetupmenuRecording:public cMenuSetupBase
{
  private:
    virtual void Store(void);
    virtual void Setup(void);

    const char *PriorityTexts[3];
    int tmpprio, tmppauseprio;
    int NoAd;
    int ForceMarkad;
    const char *buf;
  public:
    cSetupmenuRecording(void);
    virtual eOSState ProcessKey(eKeys Key);
};


// --- cSetupmenuReplay ------------------------------------------------------

class cSetupmenuReplay:public cMenuSetupBase
{
  protected:
    virtual void Store(void);
  public:
      cSetupmenuReplay(void);
};


// --- cSetupmenuBackground --------------------------------------------------------
class cSetupmenuBackground:public cMenuSetupBase
{
  private:
    const char *updateChannelsTexts[3];
    const char *AddNewChannelsTexts[2];
    const char *epgScanModeTexts[3];
    int tmpUpdateChannels;
    int tmpAddNewChannels;
    int tmpEpgScanMode;
    int UpdateCheck;
    int initialVolume;          // volume to be shown in percentage
    virtual void Store(void);
    void Setup(void);
  public:
      cSetupmenuBackground(void);
    virtual eOSState ProcessKey(eKeys Key);
};


// --- cMenuSetupLiveBuffer --------------------------------------------------

class cMenuSetupLiveBuffer:public cMenuSetupBase
{
  private:
    void Setup();
    bool hasHD;
    const char *pauseKeyHandlingTexts[3];
  public:
#if USE_LIVEBUFFER
      eOSState ProcessKey(eKeys Key);
#endif
      cMenuSetupLiveBuffer(void);
};


// --- cSetupmenuSetupPlugins --------------------------------------------------
class cSetupmenuSetupPlugins:public cMenuSetupBase
{
  public:
    cSetupmenuSetupPlugins(void);
    virtual eOSState ProcessKey(eKeys Key);
};


#endif // APIVERSNUM > 10700
