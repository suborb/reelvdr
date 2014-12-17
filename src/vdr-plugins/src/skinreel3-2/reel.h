/*
 * reel.h: 'ReelNG' skin for the Video Disk Recorder
 *
 */

#ifndef __REEL_H
#define __REEL_H

#include "common.h"

#include <vdr/skins.h>
#include <vdr/skinsttng.h>

#if VDRVERSNUM < 10505 && !defined(REELVDR)
static const char *strWeekdays[] = {trNOOP("Sunday"), trNOOP("Monday"), trNOOP("Tuesday"), trNOOP("Wednesday"), trNOOP("Thursday"), trNOOP("Friday"), trNOOP("Saturday")};
#define WEEKDAY(n) tr(strWeekdays[n])
#else
#define WEEKDAY(n) WeekDayNameFull(n)
#endif

#define MAX_AUDIO_BITMAPS      3
#define MAX_SPEED_BITMAPS      3
#define MAX_TRICKSPEED_BITMAPS 3

typedef struct imgSize {
        int slot;
        int width;
        int height;
} imgSize_t;

#define MIN_DATEWIDTH 144
// Minimum progress bar width in channel info
#define MIN_CI_PROGRESS 124

#define TinyGap 1
#define SmallGap 1
#define Gap 2
#define BigGap 8
#define Roundness 10

#define TitleDecoGap SmallGap
#define TitleDecoGap2 0
//#define TitleDecoHeight SmallGap
#define TitleDecoHeight 1
//#define TitleDeco (TitleDecoGap + TitleDecoHeight + TitleDecoGap2)
#define TitleDeco 0
#define LogoDecoGap 1
//SmallGap
#define LogoDecoGap2 0
//Gap
#define LogoDecoWidth 0
//SmallGap
#define MarkerGap 10
#define ListProgressBarGap Gap
#define ListProgressBarBorder TinyGap
#define ListHBorder Gap
#define ScrollbarHeight 20
#define ScrollbarWidth 16

#define IMGHEADERHEIGHT 40
#define IMGFOOTERHEIGHT 40
#define IMGBUTTONSHEIGHT 25
#define IMGMENUWIDTH 150
#define IMGMENUHEIGHT 128 /** the bottom and middle images are 128px height */
#define IMGMENUTOPHEIGHT 155 /** size of the top menu pictures */
#define IMGICONWIDTH 20
#define CHANNELINFOSYMBOLWIDTH 27
#define CHANNELINFOSYMBOLHEIGHT 18
#define IMGCHANNELLOGOHEIGHT 80
#define IMGCHANNELLOGOWIDTH 80
#define IMGREPLAYLOGOHEIGHT 56
#define IMGREPLAYLOGOWIDTH 56

#define IMGNUMBERHEIGHT 20
#define MINMESSAGEHEIGHT 22

#define FILENAMEMENULIST "/etc/vdr/plugins/skinreel3/menulist"

void CutStrToPixelLength(char* , const cFont *,const int len_in_px);

cString GetFullDateTime(time_t t=0);
void SetImagePaths(cOsd *osd);
int  pgbar(const char *buff, int&i_start, int&i_end, int&progressCount);
void DrawUnbufferedImage(cOsd* osd, std::string imgName, int xLeft, int yTop, int blend, int horRep=1, int vertRep=1);
void DrawProgressBar(cOsd *osd, int x, int y, int x_filled, int x_right);
bool HasDevice(const char *deviceID);

extern cTheme Theme;

enum
{
    // image that shows MENU on the side of OSD
/*  1*/    imgMenuTitleBottom,
/*  2*/    imgMenuTitleMiddle,
/*  3*/    imgMenuTitleTop,
/*  4*/    imgMenuTitleTopX, // suffix 'X' indicates this image is extendable according to the size of the OSD

    // image that is used for Header of the OSD
/*  5*/    imgMenuHeaderRight,
/*  6*/    imgMenuHeaderCenterX,  // extendable
/*  7*/    imgMenuHeaderLeft,

    // image that is used for footer of the OSD
/*  8*/    imgMenuFooterRight,
/*  9*/    imgMenuFooterCenterX,  // extendable
/* 10*/    imgMenuFooterLeft,

    // image for Icons
/* 11*/    imgIconArchive,             // char 129
/* 12*/    imgIconCut,                 // char 133
/* 13*/    imgIconFolder,              // char 130
/* 14*/    imgIconFolderUp,            // char 128
/* 15*/    imgIconFavouriteFolder,     // char 131
/* 16*/    imgIconHd,                  // char 134
           imgIconHdKey,               // 'E'; char 69
/* 17*/    imgIconPinProtect,          // char 80
/* 18*/    imgIconMessage,
/* 19*/    imgIconNewrecording,        // char 132
/* 20*/    imgIconRecord,
/* 21*/    imgIconRunningnow,          // char 136
/* 22*/    imgIconSkipnextrecording,
/* 23*/    imgIconTimer,
/* 24*/    imgIconTimeractive,         // char 135
/* 25*/    imgIconTimerpart,
/* 26*/    imgIconVps,
/* 27*/    imgIconZappingtimer,

    //Channel Info
/* 28*/    imgChannelInfoFooterCenterX,
/* 29*/    imgChannelInfoFooterLeft,
/* 30*/    imgChannelInfoFooterRight,

/* 31*/    imgChannelInfoHeaderCenterX ,
/* 32*/    imgChannelInfoHeaderLeft ,
/* 33*/    imgChannelInfoHeaderRight ,

    //Message
/* 34*/    imgMessageBarCenterX ,
/* 35*/    imgMessageBarLeft ,
/* 36*/    imgMessageBarRight ,

    //Channel Info symbols
/* 37*/    imgRecordingOn,
/* 38*/    imgRecordingOff,
/* 39*/    imgVpsOn,
/* 40*/    imgVpsOff,
/* 41*/    imgDolbyDigitalOn,
/* 42*/    imgDolbyDigitalOff,
/* 43*/    imgEncryptedOn,
/* 44*/    imgEncryptedOff,
/* 45*/    imgTeletextOn,
/* 46*/    imgTeletextOff,
/* 47*/    imgRadio,

/* 48*/    imgFreetoair,

/* 49*/    imgSignal,
/* 50*/    imgAudio,

   //Message
/* 51*/    imgIconHelpActive,
/* 52*/    imgIconHelpInactive,
/* 53*/    imgIconMessageError,
/* 54*/    imgIconMessageInfo,
/* 55*/    imgIconMessageNeutral,
/* 56*/    imgIconMessageWarning,

    //HelpButtons
/* 57*/    imgButtonRedX,
/* 58*/    imgButtonGreenX,
/* 59*/    imgButtonYellowX,
/* 60*/    imgButtonBlueX,

    //Replay
/* 61*/    imgIconControlFastFwd,
/* 62*/    imgIconControlFastFwd1,
/* 63*/    imgIconControlFastFwd2,
/* 64*/    imgIconControlFastFwd3,
/* 65*/    imgIconControlFastRew,
/* 66*/    imgIconControlFastRew1,
/* 67*/    imgIconControlFastRew2,
/* 68*/    imgIconControlFastRew3,
/* 69*/    imgIconControlPause,
/* 70*/    imgIconControlPlay,
/* 71*/    imgIconControlSkipFwd,
/* 72*/    imgIconControlSkipFwd1,
/* 73*/    imgIconControlSkipFwd2,
/* 74*/    imgIconControlSkipFwd3,
/* 75*/    imgIconControlSkipRew,
/* 76*/    imgIconControlSkipRew1,
/* 77*/    imgIconControlSkipRew2,
/* 78*/    imgIconControlSkipRew3,
/* 79*/    imgIconControlStop,

    //Channel icons
/* 80*/    imgNoChannelIcon,

/* 81*/    img43,
/* 82*/    img169,

    // The status bar with open menu
/* 83*/    imgMenuMessageLeft,
/* 84*/    imgMenuMessageRight,
/* 85*/    imgMenuMessageCenterX,

    // menu body
/* 86*/    imgMenuBodyUpperPart,
/* 87*/    imgMenuBodyLowerPart,
/* 88*/    imgMenuBodyLeftUpperPart,
/* 89*/    imgMenuBodyLeftLowerPart,
/* 90*/    imgMenuBodyRightUpperPart,
/* 91*/    imgMenuBodyRightLowerPart,

    // left part of menu body
/* 92*/    imgMenuHighlightLeftUpperPart,
/* 93*/    imgMenuHighlightLeftLowerPart,
/* 94*/    imgMenuHighlightUpperPart,
/* 95*/    imgMenuHighlightLowerPart,

/* 96*/    imgMusicBodyTop,
/* 97*/    imgMusicBodyMiddle,
/* 98*/    imgMusicBodyBottom,

/* 99*/    imgButtonInactive,
/*100*/    imgButtonActive,

/*100*/    imgButtonSmallInactive,
/*101*/    imgButtonSmallActive,

/*102*/    imgButtonBigActive,

/*103*/    imgPgBarLeft,
/*104*/    imgPgBarRight,
/*105*/    imgPgBarFilled,
/*106*/    imgPgBarEmpty,

    // Music icon
/*107*/    imgIconMusic, // char 139

    //empty banner
/*108*/    imgEmptyBannerBottom,
/*109*/    imgEmptyBannerMiddle,
/*110*/    imgEmptyBannerTop,

/*111*/    imgUnbufferedEnum=119, //Reserved for Unbuffered Images //TB: has to have ID 119
/*112*/    imgUnbufferedEnum2=120, //Reserved for Unbuffered Images //TB: has to have ID 120
};

// Identical images
enum
{
    // image that is used for Header of the OSD
    imgSetupHeaderRight = imgMenuHeaderRight,
    imgSetupHeaderCenterX = imgMenuHeaderCenterX,  // extendable
    imgSetupHeaderLeft = imgMenuHeaderLeft,

    // image that is used for footer of the OSD
    imgSetupFooterRight = imgMenuFooterRight,
    imgSetupFooterCenterX = imgMenuFooterCenterX,  // extendable
    imgSetupFooterLeft = imgMenuFooterLeft,

    // ChannelSymbol Header&Footer
    imgChannelSymbolFooterCenterX = imgChannelInfoFooterCenterX,
    imgChannelSymbolFooterLeft = imgChannelInfoFooterLeft,
    imgChannelSymbolFooterRight = imgChannelInfoFooterRight,

    imgChannelHeaderCenterX = imgMenuHeaderCenterX,
    imgChannelHeaderLeft = imgMenuHeaderLeft,
    imgChannelHeaderRight = imgMenuHeaderRight,

    imgChannelSymbolHeaderCenterX = imgMenuHeaderCenterX,
    imgChannelSymbolHeaderLeft = imgMenuHeaderLeft,
    imgChannelSymbolHeaderRight = imgMenuHeaderRight,

    // Replay Header&Footer
    imgReplayFooterCenterX = imgChannelInfoFooterCenterX,
    imgReplayFooterLeft = imgChannelInfoFooterLeft,
    imgReplayFooterRight = imgChannelInfoFooterRight,

    imgReplayHeaderCenterX = imgChannelInfoHeaderCenterX,
    imgReplayHeaderLeft = imgChannelInfoHeaderLeft,
    imgReplayHeaderRight = imgChannelInfoHeaderRight,

    // image for Menu stripe
    imgSetupTitleTopX = imgMenuTitleTopX, // extendable
    imgHelpTitleTopX = imgMenuTitleTopX, // extendable
    imgBouquetsTitleTopX = imgMenuTitleTopX, // extendable
    imgImagesTitleTopX = imgMenuTitleTopX, // extendable
    imgMusicTitleTopX = imgMenuTitleTopX, // extendable
    imgVideoTitleTopX = imgMenuTitleTopX, // extendable
    imgMultifeedTitleTopX = imgMenuTitleTopX // extendable

};

// unbuffered Images
#define uimgViaccess "viaccess.png"
#define uimgSkycrypt "skycrypt.png"
#define uimgSeca "seca.png"
#define uimgPowervu "powervu.png"
#define uimgNds "nds.png"
#define uimgNagravision "nagravision.png"
#define uimgIrdeto "irdeto.png"
#define uimgCryptoworks "cryptoworks.png"
#define uimgConax "conax.png"
#define uimgBetacrypt "betacrypt.png"
#define uimgDrecrypt "drecrypt.png"

// Set these colors are dumped into ReelNG-Blue.theme

// Background colors
THEME_CLR(Theme, clrBackground, 0xF50c1f33);
THEME_CLR(Theme, clrAltBackground, 0xEE0c1f33);
THEME_CLR(Theme, clrTitleBg, 0x00000000);
THEME_CLR(Theme, clrLogoBg, 0xD00C1F33);
THEME_CLR(Theme, clrBottomBg, 0xE51D2F7D);
THEME_CLR(Theme, clrBotProgBarBg, 0xFF0000F0);
THEME_CLR(Theme, clrBotProgBarFg, 0xFF008000);

// Text colors
THEME_CLR(Theme, clrMenuTxtFg, 0xFFEEEEEE);
THEME_CLR(Theme, clrRecDateFg, 0xEE888888);  // grey
THEME_CLR(Theme, clrTitleFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrTitleShadow, 0x00000000);
THEME_CLR(Theme, clrRecTitleFg, 0xFFFF9900); // orange

// Symbols
THEME_CLR(Theme, clrSymbolActive, 0xFFF4F400);
THEME_CLR(Theme, clrSymbolInactive, 0xFF707070);
THEME_CLR(Theme, clrSymbolRecord, 0xFFC40000);
THEME_CLR(Theme, clrSymbolRecordBg, 0xFFFFFFFF);
THEME_CLR(Theme, clrSymbolTimerActive, 0xFF0000C4);
THEME_CLR(Theme, clrSymbolVpsActive, 0xFFC4C400);
THEME_CLR(Theme, clrSymbolRecActive, 0xFFC40000);
THEME_CLR(Theme, clrSymbolRunActive, 0xFF00C400);

//Signal bars
THEME_CLR(Theme, clrSignalFg, 0xFFCCC400);
//THEME_CLR(Theme, clrSignalBg, 0xFF444444);
THEME_CLR(Theme, clrSignalHighFg,   0xFF116611);
THEME_CLR(Theme, clrSignalMediumFg, 0xFFCD853F);
THEME_CLR(Theme, clrSignalLowFg,    0xFFCC0000);
THEME_CLR(Theme, clrSignalBg,       0xFF8EA4E3);


// Help buttons
THEME_CLR(Theme, clrButtonRedFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrButtonRedBg, 0xE5C40000);
THEME_CLR(Theme, clrButtonGreenFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrButtonGreenBg, 0xE500C400);
THEME_CLR(Theme, clrButtonYellowFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrButtonYellowBg, 0xE5C4C400);
THEME_CLR(Theme, clrButtonBlueFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrButtonBlueBg, 0xE50000C4);

// Messages
THEME_CLR(Theme, clrMessageStatusFg, 0xFF858F99);
THEME_CLR(Theme, clrMessageStatusBg, 0xEE0C1F33);
THEME_CLR(Theme, clrMessageInfoFg, 0xFFF0F0F0);
THEME_CLR(Theme, clrMessageInfoBg, 0xEE31A000);
THEME_CLR(Theme, clrMessageWarningFg, 0xFFF0F0F0);
THEME_CLR(Theme, clrMessageWarningBg, 0xEE9CA000);
THEME_CLR(Theme, clrMessageErrorFg, 0xFFF0F0F0);
THEME_CLR(Theme, clrMessageErrorBg, 0xEEA00200);

// Volume bar
THEME_CLR(Theme, clrVolumeBar, 0xFF33CC33);
THEME_CLR(Theme, clrVolumeBarMute, 0xFFFF0000);

//Progress Bar
THEME_CLR(Theme, clrProgressBarFg, 0xFF8EA4E5);
THEME_CLR(Theme, clrProgressBarBg, 0xFF404040);

// Menu list items
THEME_CLR(Theme, clrMenuItemCurrentFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrMenuItemCurrentBg, 0xFF00AACC);
THEME_CLR(Theme, clrMenuItemSelectableFg, 0xFF858F99);
THEME_CLR(Theme, clrMenuItemNotSelectableFg, 0xEE888888);

// Replay OSD
THEME_CLR(Theme, clrReplayCurrent, 0xFFF0F0F0);
THEME_CLR(Theme, clrReplayTotal, 0xFFF0F0F0);
THEME_CLR(Theme, clrReplayModeJump, 0xFFF0F0F0);
THEME_CLR(Theme, clrReplayBarAreaBg, 0xE5DEE5FA);
THEME_CLR(Theme, clrReplayProgressSeen, 0xFF8EA4E3);
THEME_CLR(Theme, clrReplayProgressRest, 0xE5DEE5FA);
THEME_CLR(Theme, clrReplayProgressSelected, 0xFF4158BC);
THEME_CLR(Theme, clrReplayProgressMark, 0xFF4158BC);
THEME_CLR(Theme, clrReplayProgressCurrent, 0xFFFF0000);

// Needed by the reelbox-Plugin
THEME_CLR(Theme, clrWhiteText, 0xFFF0F0F0);

/*needed by reelEPG start*/
#define CLR_TRANSPARENT_NOT_0 0x00FFFFFF

THEME_CLR(Theme, themeClrDateBg              , 0xF50c1f33);
THEME_CLR(Theme, themeClrDateTxt             , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrScaleBg             , 0xF50c1f33);
THEME_CLR(Theme, themeClrScaleTxt            , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrDetailLeftBg        , 0xF50c1f33);
THEME_CLR(Theme, themeClrDetailLeftTxt       , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrDetailRightBg       , 0xF50c1f33);
THEME_CLR(Theme, themeClrDetailRightTxt      , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrItemNormalBg        , 0xF00C1F33);
THEME_CLR(Theme, themeClrItemNormalTxt       , 0xFF707070);
THEME_CLR(Theme, themeClrItemSelectBg        , 0xF0355E70);
THEME_CLR(Theme, themeClrItemSelectTxt       , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrItemRecordNormalBg  , 0xF0704835);
THEME_CLR(Theme, themeClrItemRecordNormalTxt , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrItemRecordSelectBg  , 0xF07E3827);
THEME_CLR(Theme, themeClrItemRecordSelectTxt , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrItemSwitchNormalBg  , 0xF0706E35);
THEME_CLR(Theme, themeClrItemSwitchNormalTxt , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrItemSwitchSelectBg  , 0xF086841F);
THEME_CLR(Theme, themeClrItemSwitchSelectTxt , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrTxtShadow           , clrBlack);
THEME_CLR(Theme, themeClrButtonRedFg         , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrButtonGreenFg       , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrButtonYellowFg      , 0xFFF0F0F0);
THEME_CLR(Theme, themeClrButtonBlueFg        , 0xFFF0F0F0);

THEME_CLR(Theme, themeOptScaleFrame   , 1);
THEME_CLR(Theme, themeOptItemsFrame   , 1);
THEME_CLR(Theme, themeOptChannelsFrame, 1);
THEME_CLR(Theme, themeOptShowDate     , 1);
THEME_CLR(Theme, themeOptShowTime     , 0);
THEME_CLR(Theme, themeOptShowSR       , 0);
THEME_CLR(Theme, themeOptShowRN       , 0);

THEME_CLR(Theme, themeOptFrameLeft  , 0);
THEME_CLR(Theme, themeOptFrameTop   , 1);
THEME_CLR(Theme, themeOptFrameRight , 2);
THEME_CLR(Theme, themeOptFrameBottom, 1);

THEME_CLR(Theme, themeClrDateFrame         , 0);
THEME_CLR(Theme, themeClrScaleFrame        , 0);
THEME_CLR(Theme, themeClrDetailLeftFrame   , 0);
THEME_CLR(Theme, themeClrDetailRightFrame  , 0);
THEME_CLR(Theme, themeClrChannelNormalFrame, CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrChannelSelectFrame, CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrChannelsFrame     , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrChannelsBg        , 0xF00C1F33);
THEME_CLR(Theme, themeClrItemsFrame        , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrItemsBg           , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrChannelNormalBg   , 0xF00C1F33);
THEME_CLR(Theme, themeClrChannelNormalTxt  , 0xFF707070);
THEME_CLR(Theme, themeClrChannelSelectBg   , 0xF00C1F33);
THEME_CLR(Theme, themeClrChannelSelectTxt  , 0xFFEEEEEE);
THEME_CLR(Theme, themeClrTimeScaleBg       , 0xF00C1F33);
THEME_CLR(Theme, themeClrTimeScaleFg       , 0xFF303030);
THEME_CLR(Theme, themeClrTimeNowBg         , 0xF00C1F33);
THEME_CLR(Theme, themeClrTimeNowFg         , 0xF07E3827);

THEME_CLR(Theme, themeClrItemNormalFrame   , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrItemSelectFrame   , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrRecordNormalFrame , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrRecordSelectFrame , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrSwitchNormalFrame , CLR_TRANSPARENT_NOT_0);
THEME_CLR(Theme, themeClrSwitchSelectFrame , CLR_TRANSPARENT_NOT_0);

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


/** interface for use texteffects (=threads) */
class cSkinReelThreadedOsd {
  friend class cReelTextEffects;

public:
  virtual ~cSkinReelThreadedOsd(void) {};
  virtual void DrawTitle(const char *Title) = 0;
};

/** interface for common functions */
class cSkinReelBaseOsd {

protected:
  cOsd *osd;
  bool HasChannelTimerRecording(const cChannel *Channel);
  int DrawStatusSymbols(int x0, int xs, int top, int bottom, const cChannel *Channel = NULL);
  int FixWidth(int w, int bpp, bool enlarge = true);
};

#endif //__REEL_H

// vim:et:sw=2:ts=2:
