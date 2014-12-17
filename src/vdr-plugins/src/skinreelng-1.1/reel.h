/*
 * reel.h: 'ReelNG' skin for the Video Disk Recorder
 *
 */

#ifndef __REEL_H
#define __REEL_H

#include "common.h"

#include <vdr/config.h>
#include <vdr/skins.h>
#include <vdr/skinclassic.h>

class cSkinReel : public cSkin {
private:
  cSkin *skinFallback;

public:
  cSkinReel();
  virtual const char *Description(void);
  virtual cSkinDisplayChannel *DisplayChannel(bool WithInfo);
  virtual cSkinDisplayMenu *DisplayMenu(void);
  virtual cSkinDisplayReplay *DisplayReplay(bool ModeOnly);
  virtual cSkinDisplayVolume *DisplayVolume(void);
  virtual cSkinDisplayTracks *DisplayTracks(const char *Title, int NumTracks, const char * const *Tracks);
  virtual cSkinDisplayMessage *DisplayMessage(void);
};


// interface for use texteffects (=threads)
class cSkinReelThreadedOsd {
  friend class cReelTextEffects;

public:
  virtual ~cSkinReelThreadedOsd(void) {};
  virtual void DrawTitle(const char *Title) = 0;
};

// interface for common functions
class cSkinReelBaseOsd {

protected:
  cOsd *osd;
  bool HasChannelTimerRecording(const cChannel *Channel);
  int DrawStatusSymbols(int x0, int xs, int top, int bottom, const cChannel *Channel = NULL);
  int FixWidth(int w, int bpp, bool enlarge = true);
};

// --- cSkinReelDisplayMenu -----------------------------------------------

enum { eNone, ePinProtect, eArchive, eBack, eFolder, eNewrecording, eCut, eNewCut, eArchivedCut, eRecord, eTimeractive, eSkipnextrecording, eNetwork, eDirShared, eMusic, eRunningNow };
enum eMenuType
{
  menuNormal,
  menuMenuNormal,
  menuMenuCentered,
  menuSetupNormal,
  menuSetupCentered,
  menuHelp,
  menuBouquets,
  menuImage,
  menuMusic,
  menuVideo,
  menuMultifeed
};

struct MenuTypeStruct{
  std::string Title;
  short int Type;
  bool ImgNumber;
  bool UpperCase;
};


class cSkinReelDisplayMenu : public cSkinDisplayMenu, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
private:
  const cFont *pFontList;
  const cFont *pFontOsdTitle;

  char *strTitle;
  char *strLastDate;
  char *strTheme;
  bool isMainMenu;
  bool isCenteredMenu;
  bool fShowLogo;
  bool fShowLogoDefault;
  bool fShowInfo;
  bool fSetupAreasDone;

  int xBodyLeft, xBodyRight, yBodyTop, yBodyBottom;
  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom, yTitleDecoTop, yTitleDecoBottom;
  int xFooterLeft, xFooterRight, yFooterTop, yFooterBottom;
  int xButtonsLeft, xButtonsRight, yButtonsTop, yButtonsBottom;
  int xMessageLeft, xMessageRight, yMessageTop, yMessageBottom;
  int xDateLeft, xDateRight, yDateTop, yDateBottom;
  int xLogoLeft, xLogoRight, yLogoTop, yLogoBottom;
  int xInfoLeft, xInfoRight, yInfoTop, yInfoBottom;

  int lineHeight;
  int xItemLeft;
  int xItemRight;
  int yItemTop;

  int nMessagesShown;
  int nNumImageColors;

  std::vector<MenuTypeStruct> MenuList;
  std::vector<MenuTypeStruct> MenuListWildcard;
  int MenuType;
  bool isNumberImage;
  bool withTitleImage; // tells if the Title-image is drawn; images like "Menu/Setup/Help/Music" etc

  void CreateTitleVector();
  int GetMenuType(std::string Title);
  bool UseImageNumber(std::string Title);
  bool IsUpperCase(std::string Title);
  void SetScrollbar(void);
  void SetupAreas(void);
  void SetColors(void);
  int DrawFlag(int x, int y, const tComponent *p); // not used TODO
  const char *GetPluginMainMenuName(const char *plugin);
  int ReadSizeVdr(const char *strPath);
  bool HasTabbedText(const char *s, int Tab);
  int getDateWidth(const cFont *pFontDate);

  int isMainPage; // 0 for no 1 = Menu 2 = Setup
  bool isCAPS;
  time_t old_menulist_mtime;

public:
  cSkinReelDisplayMenu();
  virtual ~cSkinReelDisplayMenu();
  virtual void Scroll(bool Up, bool Page);
  virtual int MaxItems(void);
  virtual void Clear(void);
  virtual void SetTitle(const char *Title);
  virtual void SetButtons(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
  virtual void SetMessage(eMessageType Type, const char *Text);
  virtual void SetItem(const char *Text, int Index, bool Current, bool Selectable);
  virtual void SetEvent(const cEvent *Event);
  virtual void SetRecording(const cRecording *Recording);
  virtual void SetText(const char *Text, bool FixedFont);
  virtual void SetScrollbar(int Total, int Offset);
  virtual int GetTextAreaWidth(void) const;
  virtual const cFont *GetTextAreaFont(bool FixedFont) const;
  virtual void Flush(void);
  virtual void DrawTitle(const char *Title);
  void DrawFooter();
  virtual bool DrawThumbnail(const char* filename);
  void DrawTitleImage(); // draws Image "MENU/SETUP/HELP" etc
};

// --- cSkinReelDisplayChannel --------------------------------------------


class cSkinReelDisplayChannel : public cSkinDisplayChannel, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
private:
  bool fShowLogo;
  bool fWithInfo;
  char *strLastDate;
  int nMessagesShown;

  const cFont *pFontOsdTitle;
  const cFont *pFontDate;
  const cFont *pFontTitle;
  const cFont *pFontSubtitle;
  const cFont *pFontLanguage;
  const cFont *pFontMessage;

  int xLogoLeft, xLogoRight, yLogoTop, yLogoBottom, xLogoDecoLeft, xLogoDecoRight, xLogoPos, yLogoPos;
  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom, yTitleDecoTop, yTitleDecoBottom;
  int xEventNowLeft, xEventNowRight, yEventNowTop, yEventNowBottom;
  int xEventNextLeft, xEventNextRight, yEventNextTop, yEventNextBottom;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;
  int xMessageLeft, xMessageRight, yMessageTop, yMessageBottom;
  int xFirstSymbol, xDateLeft;
  int xSymbol;
#if VDRVERSNUM < 10716
  int lastAspect;
#else
  double  lastAspect;
#endif

  int xeSignalBar, xsSignalBar;

  int strBarWidth, snrBarWidth;

  bool isMessage;

  void UpdateSignal();
  void UpdateDateTime();
  void UpdateAspect();

  void DrawAreas(void);
  void DrawGroupInfo(const cChannel *Channel, int Number);
  void DrawChannelInfo(const cChannel *Channel, int Number);
  void DrawSymbols(const cChannel *Channel);
  void DrawChannelLogo(const cChannel *channel);
  cString GetChannelName(const cChannel *Channel);
  cString GetChannelNumber(const cChannel *Channel, int Number);
public:
  cSkinReelDisplayChannel(bool WithInfo);
  virtual ~ cSkinReelDisplayChannel();
  virtual void SetChannel(const cChannel *Channel, int Number);
  virtual void SetEvents(const cEvent *Present, const cEvent *Following);
  virtual void SetMessage(eMessageType Type, const char *Text);
  virtual void Flush(void);
  virtual void DrawTitle(const char *Title);
};

// --- cSkinReelDisplayReplay ---------------------------------------------

class cSkinReelDisplayReplay : public cSkinDisplayReplay, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
private:
  const cFont *pFontOsdTitle;
  const cFont *pFontReplayTimes;
  const cFont *pFontDate;
  const cFont *pFontLanguage;
  const cFont *pFontMessage;

  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom;
  int xLogoLeft, xLogoRight, yLogoTop, yLogoBottom;
  int xProgressLeft, xProgressRight, yProgressTop, yProgressBottom;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;
  int xMessageLeft, xMessageRight, yMessageTop, yMessageBottom;
  int xFirstSymbol;

  char *strLastDate;
  bool modeonly;
  int nJumpWidth;
  int nCurrentWidth;
  bool fShowSymbol;

public:
  cSkinReelDisplayReplay(bool ModeOnly);
  virtual ~ cSkinReelDisplayReplay();
  virtual void SetTitle(const char *Title);
  virtual void SetMode(bool Play, bool Forward, int Speed);
  virtual void SetProgress(int Current, int Total);
  virtual void SetCurrent(const char *Current);
  virtual void SetTotal(const char *Total);
  virtual void SetJump(const char *Jump);
  virtual void SetMessage(eMessageType Type, const char *Text);
  virtual void Flush(void);
  virtual void DrawTitle(const char *s);
};

// --- cSkinReelDisplayVolume ---------------------------------------------

class cSkinReelDisplayVolume : public cSkinDisplayVolume//, public cSkinReelBaseOsd
{ // TODO
    private:
        cOsd *osd;
        int xVolumeBackBarLeft,xIconLeft,xVolumeFrontBarLeft,xVolumeFrontBarRight,xVolumeBackBarRight;
        int yVolumeBackBarTop,yIconTop,yVolumeFrontBarTop,yVolumeBackBarBottom,yIconBottom;
      static int mute;
        int volumeBarLength;
    public:
        cSkinReelDisplayVolume();
        virtual ~cSkinReelDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        virtual void Flush();
};

// --- ReelSkinDisplayMessage ----------------------------------------------

class ReelSkinDisplayMessage : public cSkinDisplayMessage { // TODO
    private:
        cOsd *osd;
        int x0, x1, x2, x3;
        int y0, y1, y2;
        int messageBarLength;
        cFont const *font;
    public:
        ReelSkinDisplayMessage();
        virtual ~ReelSkinDisplayMessage();
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();
};

// --- cSkinReelDisplayTracks ---------------------------------------------

class cSkinReelDisplayTracks : public cSkinDisplayTracks, public cSkinReelBaseOsd { // reimplement this like ReelSkin (old)
private:
  const cFont *pFontOsdTitle;
  const cFont *pFontDate;
  const cFont *pFontListItem;

  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom, yTitleDecoTop, yTitleDecoBottom;
  int xLogoLeft, xLogoRight, xLogoDecoLeft, xLogoDecoRight, yLogoTop, yLogoBottom;
  int xListLeft, xListRight, yListTop, yListBottom, xItemLeft, xItemRight;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;

  int lineHeight;
  int currentIndex;
  bool fShowSymbol;
  time_t lastTime;
  bool subTitleMenu;

  void SetItem(const char *Text, int Index, bool Current);
public:
  cSkinReelDisplayTracks(const char *Title, int NumTracks, const char *const *Tracks);
  virtual ~cSkinReelDisplayTracks();
  virtual void SetTrack(int Index, const char *const *Tracks);
  virtual void SetAudioChannel(int AudioChannel);
  virtual void Flush(void);
};

#endif //__REEL_H

// vim:et:sw=2:ts=2:
