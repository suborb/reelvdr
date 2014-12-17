#ifndef DISPLAYMENU_H
#define DISPLAYMENU_H

#include <string>
#include <vector>
#include <map>

#include "reel.h"
#include "mytextscroller.h"

/** Symbols for entries in menus */
enum {
  eNone,
  eNewHD,            /**< HD over New */
  eNewEncrypted,     /**< NEW with Key */
  eNewHDEncrypted,   /**< NEW HD with Key */
  eHdEncrypted,      /**< HD icon with 'key' under it, denotes encrypted HD channel */
  eHD,               /**< HD icon */
  ePinProtect,       /**< protected by pin plugin */
  eArchive,          /**< archived VDR-recording */
  eBack,             /**< the ".."-folder */
  eFolder,           /**< the folder-icon */
  eNewrecording,     /**< icon for new recordings */
  eCut,              /**< icon for cutted recordings */
  eNewCut,           /**< new and cutted */
  eArchivedCut,      /**< a cutted archived VDR-recording */
  eRecord,           /**< currently recording */
  eTimeractive,      /**< active timer */
  eSkipnextrecording, 
  eNetwork,          /**< icon for network hosts */
  eDirShared,        /**< icon for network shares */
  eMusic,            /**< the music icon */
  eRunningNow,
  eRecordTick,        /**< both eRecord and eTimeractive to be drawn */
  eBlankTick,        /**< blank and eTimeractive to be drawn */
  eYellowButton,     /** a small yellow button */
  eCompressed
};

enum eMenuType
{
  menuNormal,
  menuMenuNormal,    /**< menu-string shown vertically on the left side, menu is oriented left */
  menuMenuCentered,  /**< setup-string shown vertically on the left side, menu is centered */
  menuSetupNormal,   /**< setup-string shown vertically on the left side, menu is oriented left */
  menuSetupCentered, /**< setup-string shown vertically on the left side, menu is centered */
  menuHelp,          /**< help-string shown vertically on the left side */
  menuBouquets,
  menuImage,         /**< image-graphics shown on the left side */
  menuMusic,         /**< menu-graphics shown on the left side */
  menuVideo,         /**< video-graphics shown on the left side */
  menuTvRadio,       /**< tvradio-graphics shown on the left side */
  menuMusicPictures, /**< musicpictures-graphics shown on the left side */
  menuFilmsDvd,      /**< */
  menuInternetExtras,/**< */
  menuOptSoftware,   /**< */
  menuSystemSettings,/**< small buttons, no numbers */
  menuPowerOff,      /**< power-off graphics shown on the left side */
  menuInternetradio, /**< small buttons, "music"-banner */
  menuMultifeed,
  menuGimmick,        /**< graphical main menu */
  menuEmptyBanner,    /**< a completely empty banner, used for showing the mp3-covers */
  menuHalfEmptyBanner /**< a half empty banner, used for displaying id3-infos, bug no mp3-covers */
};

struct MenuTypeStruct {
  std::string Title;
  short int Type;
  bool ImgNumber;
  bool UpperCase;
  bool Buttons;
  bool SmallButtons;
};

static std::vector<MenuTypeStruct> MenuList; /** The entries from the "menulist"-file */
static std::vector<MenuTypeStruct> MenuListWildcard; /** The wildcard-entries from the "menulist"-file */

/* little icons list */
enum { NO_ICON, STOP_REPLAY_ICON, STOP_RECORDING_ICON, PLAY_MEDIA_ICON, BACKGROUND_PROCESS_ICON };

class cLittleIconInfo
{
    private:
    int iconType_;
    bool selected_;

    public:
     cLittleIconInfo(int iconType, bool selected) : iconType_(iconType), selected_(selected){ };
     cLittleIconInfo() : iconType_(NO_ICON), selected_(false){ };
     // cLittleIconInfo(const cLittleIconInfo& ref) { iconType_ = ref.iconType_; selected_ = ref.selected_; printf("\033[0;92mCopy: iconType_ : %i\033[0m\n", iconType_); }
    /* cLittleIconInfo& operator=(const cLittleIconInfo& ref)
     {
        printf(" operator = (%i) ", ref.iconType_);
        if (this != &ref)
        {
            iconType_ = ref.iconType_;
            selected_ = ref.selected_;
        }
        printf(" %i\n", iconType_);
        return *this;
     }*/
     inline bool Selected()const {return selected_;}
     inline int Type()const {return iconType_;}
};

/** cSkinReelDisplayMenu */
class cSkinReelDisplayMenu : public cSkinDisplayMenu, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
protected:
  cMyTextScroller textScroller;
private:
  const cFont *pFontList;
  const cFont *pFontOsdTitle;

  std::string PluginName;
  std::string PluginIconFile;
  bool bPluginIconFileExists;

  char *strTitle;
  char *strLastDate;
  char *strTheme;
  bool isMainMenu;
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
  int xItemLeft, xItemRight, yItemTop; 

  int nMessagesShown;
  int nNumImageColors;

  int MenuType;
  bool isNumberImage;
  bool withTitleImage; /** "true" if the Title-image is drawn; images like "Menu/Setup/Help/Music" etc */
#if APIVERSNUM >= 10718
  bool enableSideNote;
  int xSideLeft, xSideRight;

  // fonts used in side note
  cFont *regularFont15, *regularFont13, *regularFont12, *regularFont11;
  cFont *boldFont15;
#endif


  void CreateTitleVector();
  int GetMenuType(std::string Title);
  bool UseImageNumber(std::string Title);
  bool UseButtons(std::string Title);
  bool UseSmallButtons(std::string Title);
  bool IsUpperCase(std::string Title);
  void SetScrollbar(void);
  void SetupAreas(void);
  void SetColors(void);
  const char *GetPluginMainMenuName(const char *plugin);
  const char *GetPluginName(const char *MainMenuEntry);
  bool PluginIconFileExists(const char *PluginIconFile);
  int ReadSizeVdr(const char *strPath);
  bool HasTabbedText(const char *s, int Tab);
  int getDateWidth(const cFont *pFontDate);
  void PrintID3Infos(void);

  bool DrawImage(const char *sPath, char cType /* R=raw, P=png, J=jpeg */, int x, int y, int resize_x = 0);
  int WrapText(int x, int y, const char *sText, tColor fFG, tColor fBG,
               const cFont *fFont, int lWidth, int lHeight=-1 /* no contrains*/, bool squeeze=true );

  int isMainPage; /** 0 for no 1 = Menu 2 = Setup */
  bool isButtoned; /** "true" if the menu should use big buttons */
  bool isSmallButtoned; /** "true" if the menu should use small buttons */
  bool isCAPS; /** "true" if the current entry should be converted to upper case according to the menulist-file */
  bool alreadyCleared; /** "true" if the current menu is already cleared, so no need to clear background etc */

  /* vector containing a list of mainmenu's little-icons*/
  std::map<int, cLittleIconInfo> LittleIcons;

public:
  cSkinReelDisplayMenu();
  virtual ~cSkinReelDisplayMenu();
  virtual void Scroll(bool Up, bool Page);
  virtual int MaxItems(void);
  virtual void Clear(void);
  virtual void SetTitle(const char *Title);
  virtual void SetButtons(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
  virtual void SetKeys(const eKeys *Keys);
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
  void DrawTitleImage(); /** draws Image "MENU/SETUP/HELP" etc */
  void DrawLittleIcons();

#if APIVERSNUM >= 10718 && REELVDR
  /*Side Notes*/
  virtual void EnableSideNote(bool);
  virtual void SideNote(const cStringList *strList);
  virtual void SideNote(const cEvent *Event, const cStringList* sl=NULL);
  virtual void SideNote(const cChannel* channel=NULL);
  virtual void SideNote(const cRecording *Recording);
  virtual void SideNoteIcons(const char* icons=NULL);
  virtual void ClearSideNote();
  void EmptySideNote(); // clear side note area and draw background
  void DrawReelBanner(); // draws reelavg banner
#endif /*APIVERSNUM*/

  void DrawZoomedImage(const cRecording *Recording);
};

#endif

