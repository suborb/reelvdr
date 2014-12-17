/*
 * reel.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include "reel.h"
#include "logo.h"
#include "i18n.h"
#include "status.h"
#include "texteffects.h"

#include <math.h>
#include <ctype.h>
#include <sstream>
#include <iomanip>
#include <locale.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>

#include <vdr/device.h>
#include <vdr/timers.h>
#include <vdr/menu.h>
#include <vdr/font.h>
#include <vdr/osd.h>
#include <vdr/themes.h>
#include <vdr/plugin.h>

#include <sys/ioctl.h>
#include <unistd.h>

//#include <ReelBoxDevice.h>
//#include "ReelBoxDevice.h"

#include "tools.h"

#define MAX_AUDIO_BITMAPS      3
#define MAX_SPEED_BITMAPS      3
#define MAX_TRICKSPEED_BITMAPS 3

#if VDRVERSNUM < 10505
static const char *strWeekdays[] = {trNOOP("Sunday"), trNOOP("Monday"), trNOOP("Tuesday"), trNOOP("Wednesday"), trNOOP("Thursday"), trNOOP("Friday"), trNOOP("Saturday")};
#define WEEKDAY(n) tr(strWeekdays[n])
#else
#define WEEKDAY(n) WeekDayNameFull(n)
#endif

typedef struct imgSize {
        int slot;
        int width;
        int height;
} imgSize_t;

char oldThumb[512][NR_UNBUFFERED_SLOTS];
int thumbCleared[NR_UNBUFFERED_SLOTS] = {1};
static cPlugin *rbPlugin = NULL;

int pgbar(const char *buff, int&i_start, int&i_end, int&progressCount)
{
 /*
  * "...text...[|||||   ]...text...
  */

 progressCount=0;
 int spaceCount = 0;

 int i=0;
 while (buff[i] && buff[i]!='[')
    i++; //find pbstart

 if (buff[i] != '[')
    return 0; // nothing found

 i_start = i;
 i++;
 for (; buff[i]; i++)
 {

        if (buff[i]==']')
        {
            i_end = i;
            break;
        }

         switch (buff[i])
         {
          case ' ': spaceCount++;
                    break;
          case '|': if(spaceCount>0)
                       return 0; // not a progress bar
                    progressCount++;
                    break;
          default: return 0 ; // not a progress bar
         } // end switch

 } // end for

 if(!buff[i])
    return 0; // could not find ']'

 return spaceCount+progressCount; // return total

}// end  pgbar()

static cTheme Theme;

// Set these colors are dumped into ReelNG-Blue.theme

// Background colors
THEME_CLR(Theme, clrBackground, 0xF50c1f33);
THEME_CLR(Theme, clrAltBackground, 0xEE0c1f33);
THEME_CLR(Theme, clrTitleBg, 0x00000000);
THEME_CLR(Theme, clrLogoBg, 0xE50000E3);
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
THEME_CLR(Theme, clrButtonGreenFg, 0xFF000000);
THEME_CLR(Theme, clrButtonGreenBg, 0xE500C400);
THEME_CLR(Theme, clrButtonYellowFg, 0xFF000000);
THEME_CLR(Theme, clrButtonYellowBg, 0xE5C4C400);
THEME_CLR(Theme, clrButtonBlueFg, 0xFFFFFFFF);
THEME_CLR(Theme, clrButtonBlueBg, 0xE50000C4);

// Messages
THEME_CLR(Theme, clrMessageStatusFg, 0xFF858F99);
THEME_CLR(Theme, clrMessageStatusBg, 0xEE0C1F33);
THEME_CLR(Theme, clrMessageInfoFg, 0xFF000000);
THEME_CLR(Theme, clrMessageInfoBg, 0xEE31A000);
THEME_CLR(Theme, clrMessageWarningFg, 0xFF000000);
THEME_CLR(Theme, clrMessageWarningBg, 0xEE9CA000);
THEME_CLR(Theme, clrMessageErrorFg, 0xFF000000);
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
THEME_CLR(Theme, clrReplayCurrent, 0xFF1D2F7D);
THEME_CLR(Theme, clrReplayTotal, 0xFF1D2F7D);
THEME_CLR(Theme, clrReplayModeJump, 0xFF1D2F7D);
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
/*needed by reelEPG end */

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

#define IMGHEADERHEIGHT 30
#define IMGFOOTERHEIGHT 30
#define IMGBUTTONSHEIGHT 25
#define IMGMENUWIDTH 150
#define IMGMENUHEIGHT 256
#define IMGICONWIDTH 20
#define PROGRESSBARHEIGHT  7
#define CHANNELINFOSYMBOLWIDTH 27
#define CHANNELINFOSYMBOLHEIGHT 18
#define IMGCHANNELLOGOHEIGHT 80
#define IMGCHANNELLOGOWIDTH 80
#define IMGREPLAYLOGOHEIGHT 56
#define IMGREPLAYLOGOWIDTH 56

#define IMGNUMBERHEIGHT 20
#define MINMESSAGEHEIGHT 22

#define FILENAMEMENULIST "/etc/vdr/plugins/skinreelng/menulist"

enum
{
    // image that shows MENU on the side of OSD
/*   1 */    imgMenuTitleBottom,
/*   2 */    imgMenuTitleMiddle,
/*   3 */    imgMenuTitleTopX, // suffix 'X' indicates this image is extendable according to the size of the OSD

    // image that is used for Header of the OSD
/*   4 */    imgMenuHeaderRight,
/*   5 */    imgMenuHeaderCenterX,  // extendable
/*   6 */    imgMenuHeaderLeft,

    // image that is used for footer of the OSD
/*   7 */    imgMenuFooterRight,
/*   8 */    imgMenuFooterCenterX,  // extendable
/*   9 */    imgMenuFooterLeft,

    // image that shows SETUP on the side of OSD
/*  10 */    imgSetupTitleBottom,
/*  11 */    imgSetupTitleMiddle,

    // image that shows HELP on the side of OSD
/*  12 */    imgHelpTitleBottom,
/*  13 */    imgHelpTitleMiddle,

    // image that shows BOUQUETS on the side of OSD
/*  14 */    imgBouquetsTitleBottom,
/*  15 */    imgBouquetsTitleMiddle,

    // image that shows images on the side of OSD
/*  16 */    imgImagesTitleBottom,
/*  17 */    imgImagesTitleMiddle,

    // image that shows music on the side of OSD
/*  18 */    imgMusicTitleBottom,
/*  19 */    imgMusicTitleMiddle,

    // image that shows video on the side of OSD
/*  20 */    imgVideoTitleBottom,
/*  21 */    imgVideoTitleMiddle,

    // image that shows MULTIFEED on the side of OSD
/*  22 */    imgMultifeedTitleBottom,
/*  23 */    imgMultifeedTitleMiddle,

    // image for Icons
/*  24 */    imgIconArchive,             // char 129
/*  25 */    imgIconCut,                 // char 133
/*  26 */    imgIconFolder,              // char 130
/*  27 */    imgIconFolderUp,            // char 128
/*  28 */    imgIconFavouriteFolder,     // char 131
/*  29 */    imgIconHd,                  // char 134
/*  30 */    imgIconPinProtect,          // char 80
/*  31 */    imgIconMessage,  // TODO: Not used?
/*  32 */    imgIconNewrecording,        // char 132
/*  33 */    imgIconRecord,
/*  34 */    imgIconRunningnow,          // char 136
/*  35 */    imgIconSkipnextrecording,
/*  36 */    imgIconTimer,
/*  37 */    imgIconTimeractive,         // char 135
/*  38 */    imgIconTimerpart,
/*  39 */    imgIconVps,
/*  40 */    imgIconZappingtimer,

    //Channel Info
/*  41 */    imgChannelInfoFooterCenterX,
/*  42 */    imgChannelInfoFooterLeft,
/*  43 */    imgChannelInfoFooterRight,

/*  44 */    imgChannelInfoHeaderCenterX ,
/*  45 */    imgChannelInfoHeaderLeft ,
/*  46 */    imgChannelInfoHeaderRight ,

    //Volume
/*  47 */    imgMuteOn,
/*  48 */    imgMuteOff,
/*  49 */    imgVolume,

/*  50 */    imgVolumeBackBarLeft,
/*  51 */    imgVolumeBackBarMiddleX,
/*  52 */    imgVolumeBackBarRight ,
/*  53 */    imgVolumeFrontBarActiveX ,
/*  54 */    imgVolumeFrontBarInactiveX ,

    //Message
/*  55 */    imgMessageBarCenterX ,
/*  56 */    imgMessageBarLeft ,
/*  57 */    imgMessageBarRight ,

    //Channel Info symbols
/*  58 */    imgRecordingOn,
/*  59 */    imgRecordingOff,
/*  60 */    imgDolbyDigitalOn,
/*  61 */    imgDolbyDigitalOff,
/*  62 */    imgEncryptedOn,
/*  63 */    imgEncryptedOff,
/*  64 */    imgTeletextOn,
/*  65 */    imgTeletextOff,
/*  66 */    imgRadio,  // TODO: Not used?
/*  67 */    imgFreetoair,
/*  68 */    imgSignal,
/*  69 */    imgAudio,

   //Message
/*  70 */    imgIconHelp ,
/*  71 */    imgIconMessageError ,
/*  72 */    imgIconMessageInfo ,
/*  73 */    imgIconMessageNeutral,
/*  74 */    imgIconMessageWarning,

    //HelpButtons
/*  75 */    imgButtonRedX,
/*  76 */    imgButtonGreenX,
/*  77 */    imgButtonYellowX,
/*  78 */    imgButtonBlueX,

    //Replay
/*  79 */    imgIconControlFastFwd,
/*  80 */    imgIconControlFastFwd1,
/*  81 */    imgIconControlFastFwd2,
/*  82 */    imgIconControlFastFwd3,
/*  83 */    imgIconControlFastRew,
/*  84 */    imgIconControlFastRew1,
/*  85  */    imgIconControlFastRew2,
/*  86 */    imgIconControlFastRew3,
/*  87 */    imgIconControlPause,
/*  88 */    imgIconControlPlay,
/*  89 */    imgIconControlSkipFwd,
/*  90 */    imgIconControlSkipFwd1,
/*  91 */    imgIconControlSkipFwd2,
/*  92 */    imgIconControlSkipFwd3,
/*  93 */    imgIconControlSkipRew,
/*  94 */    imgIconControlSkipRew1,
/*  95 */    imgIconControlSkipRew2,
/*  96 */    imgIconControlSkipRew3,
/*  97 */    imgIconControlStop,

    //Audio
/*  98 */    imgIconAudio,

    //Channel icons
/*  99 */    imgNoChannelIcon,

/* 100 */    imgNumber00H ,
/* 101 */    imgNumber00  ,
/* 102 */    imgNumber01H ,
/* 103 */    imgNumber01  ,
/* 104 */    imgNumber02H ,
/* 105 */    imgNumber02  ,
/* 106 */    imgNumber03H ,
/* 107 */    imgNumber03  ,
/* 108 */    imgNumber04H ,
/* 109 */    imgNumber04  ,
/* 110 */    imgNumber05H ,
/* 111 */    imgNumber05  ,
/* 112 */    imgNumber06H ,
/* 113 */    imgNumber06  ,
/* 114 */    imgNumber07H ,
/* 115 */    imgNumber07  ,
/* 116 */    imgNumber08H ,
/* 117 */    imgNumber08  ,
/* 118 */    imgNumber09H ,
/* 119 */    imgNumber09  ,

/* 120 */    imgUnbufferedEnum, //Reserved for Unbuffered Images /** TB: this one has got to have ID 119!!!! */
/* 121 */    imgUnbufferedEnum2, //Reserved for Unbuffered Images /** TB: this one has got to have ID 120!!!! */

/* 122 */    img43,
/* 123 */    img169,

    // Music icon
/* 124 */    imgIconMusic, // char 139  // TODO: Not used?

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

    imgChannelSymbolHeaderCenterX = imgChannelInfoHeaderCenterX,
    imgChannelSymbolHeaderLeft = imgChannelInfoHeaderLeft,
    imgChannelSymbolHeaderRight = imgChannelInfoHeaderRight,

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

// one should define imgPath string and m_path char array.
// imgPath is the directory underwhich all images are available
#define IMGPATH(imgEnumNum, imgName) m_path = std::string(imgPath) + "/" + imgName; \
        osd->SetImagePath(imgEnumNum, m_path.c_str())

void SetImagePaths(cOsd *osd)
{
    std::string path;
    path = std::string("/usr/share/reel/skinreelng/") + Theme.Name();
    ReelConfig.SetImagesDir(path.c_str());
    char *imgPath = strdup(ReelConfig.GetImagesDir());
    std::string m_path;

    // MENU
    IMGPATH(imgMenuTitleBottom,"menu_title_bottom.png");
    IMGPATH(imgMenuTitleMiddle,"menu_title_middle.png");
    IMGPATH(imgMenuTitleTopX  ,"menu_title_top_x.png");

    IMGPATH(imgMenuHeaderRight,"menu_header_right.png");
    IMGPATH(imgMenuHeaderCenterX,"menu_header_center_x.png");
    IMGPATH(imgMenuHeaderLeft,"menu_header_left.png");

    IMGPATH(imgMenuFooterRight,"menu_footer_right.png");
    IMGPATH(imgMenuFooterCenterX,"menu_footer_center_x.png");
    IMGPATH(imgMenuFooterLeft,"menu_footer_left.png");

    // SETUP
    IMGPATH(imgSetupTitleBottom,"setup_title_bottom.png");
    IMGPATH(imgSetupTitleMiddle,"setup_title_middle.png");

    // HELP
    IMGPATH(imgHelpTitleBottom,"help_title_bottom.png");
    IMGPATH(imgHelpTitleMiddle,"help_title_middle.png");

    // BOUQUETS
    IMGPATH(imgBouquetsTitleBottom,"bouquets_title_bottom.png");
    IMGPATH(imgBouquetsTitleMiddle,"bouquets_title_middle.png");

    // IMAGE
    IMGPATH(imgImagesTitleBottom,"images_title_bottom.png");
    IMGPATH(imgImagesTitleMiddle,"images_title_middle.png");

    // MUSIC
    IMGPATH(imgMusicTitleBottom,"music_title_bottom.png");
    IMGPATH(imgMusicTitleMiddle,"music_title_middle.png");

    // VIDEO
    IMGPATH(imgVideoTitleBottom,"video_title_bottom.png");
    IMGPATH(imgVideoTitleMiddle,"video_title_middle.png");

    // MULTIFEED
    IMGPATH(imgMultifeedTitleBottom,"multifeed_title_bottom.png");
    IMGPATH(imgMultifeedTitleMiddle,"multifeed_title_middle.png");

    // ICONS
    IMGPATH(imgIconPinProtect,"icon_pin_protect_20.png");
    IMGPATH(imgIconArchive,"icon_archive_20.png");
    IMGPATH(imgIconMusic,"icon_music_20.png");    // TODO: Not used?
    IMGPATH(imgIconCut,"icon_cut_20.png");
    IMGPATH(imgIconFolder,"icon_folder_20.png");
    IMGPATH(imgIconFolderUp,"icon_folderup_20.png");
    IMGPATH(imgIconFavouriteFolder,"icon_favouritefolder_20.png");
    IMGPATH(imgIconHd,"icon_hd_20.png");
    IMGPATH(imgIconMessage,"icon_message_30.png");   //TODO: Not used?
//    IMGPATH(imgIconMuteOff,"icon_muteoff_64.png");  // Already in Volume-Group
//    IMGPATH(imgIconMuteOn,"icon_muteon_64.png");  // Already in Volume-Group
    IMGPATH(imgIconNewrecording,"icon_newrecording_20.png");
    IMGPATH(imgIconRecord,"icon_record_20.png");
    IMGPATH(imgIconRunningnow,"icon_runningnow_20.png");
    IMGPATH(imgIconSkipnextrecording,"icon_skipnextrecording_20.png");
    IMGPATH(imgIconTimer,"icon_timer_20.png");
    IMGPATH(imgIconTimeractive,"icon_timeractive_20.png");
    IMGPATH(imgIconTimerpart,"icon_timerpart_20.png");
//    IMGPATH(imgIconVolume,"icon_volume_64.png");  // Already in Volume-Group
    IMGPATH(imgIconVps,"icon_vps_20.png");
    IMGPATH(imgIconZappingtimer,"icon_zappingtimer_20.png");

    //ChannelInfo
    IMGPATH( imgChannelInfoFooterCenterX , "channelinfo_footer_center_x.png" );
    IMGPATH( imgChannelInfoFooterLeft , "channelinfo_footer_left.png" );
    IMGPATH( imgChannelInfoFooterRight , "channelinfo_footer_right.png" );

    IMGPATH( imgChannelInfoHeaderCenterX , "channelinfo_header_center_x.png" );
    IMGPATH( imgChannelInfoHeaderLeft , "channelinfo_header_left.png" );
    IMGPATH( imgChannelInfoHeaderRight , "channelinfo_header_right.png" );

    //Volume
    IMGPATH(imgMuteOn, "icon_muteon_64.png");
    IMGPATH(imgMuteOff, "icon_muteoff_64.png");
    IMGPATH(imgVolume, "icon_volume_64.png");

    IMGPATH( imgVolumeBackBarLeft , "volume_backbar_left.png" );
    IMGPATH( imgVolumeBackBarMiddleX, "volume_backbar_middle_x.png" );
    IMGPATH( imgVolumeBackBarRight , "volume_backbar_right.png" );
    IMGPATH( imgVolumeFrontBarActiveX , "volume_frontbar_active_x.png" );
    IMGPATH( imgVolumeFrontBarInactiveX , "volume_frontbar_inactive_x.png" );

    //Message Bar
    IMGPATH( imgMessageBarCenterX , "message_bar_center_x.png" );
    IMGPATH( imgMessageBarLeft , "message_bar_left.png" );
    IMGPATH( imgMessageBarRight , "message_bar_right.png" );

    //HelpButtons
    IMGPATH(imgButtonRedX, "button_red_x.png");
    IMGPATH(imgButtonGreenX, "button_green_x.png");
    IMGPATH(imgButtonYellowX, "button_yellow_x.png");
    IMGPATH(imgButtonBlueX, "button_blue_x.png");

    //Replay
    IMGPATH(imgIconControlFastFwd,"icon_control_fwd_56.png");
    IMGPATH(imgIconControlFastFwd1,"icon_control_fwd1_56.png");
    IMGPATH(imgIconControlFastFwd2,"icon_control_fwd2_56.png");
    IMGPATH(imgIconControlFastFwd3,"icon_control_fwd3_56.png");
    IMGPATH(imgIconControlFastRew,"icon_control_rew_56.png");
    IMGPATH(imgIconControlFastRew1,"icon_control_rew1_56.png");
    IMGPATH(imgIconControlFastRew2,"icon_control_rew2_56.png");
    IMGPATH(imgIconControlFastRew3,"icon_control_rew3_56.png");
    IMGPATH(imgIconControlPause,"icon_control_pause_56.png");
    IMGPATH(imgIconControlPlay,"icon_control_play_56.png");
    IMGPATH(imgIconControlSkipFwd,"icon_control_skipfwd_56.png");
    IMGPATH(imgIconControlSkipFwd1,"icon_control_skipfwd1_56.png");
    IMGPATH(imgIconControlSkipFwd2,"icon_control_skipfwd2_56.png");
    IMGPATH(imgIconControlSkipFwd3,"icon_control_skipfwd3_56.png");
    IMGPATH(imgIconControlSkipRew,"icon_control_skiprew_56.png");
    IMGPATH(imgIconControlSkipRew1,"icon_control_skiprew1_56.png");
    IMGPATH(imgIconControlSkipRew2,"icon_control_skiprew2_56.png");
    IMGPATH(imgIconControlSkipRew3,"icon_control_skiprew3_56.png");
    IMGPATH(imgIconControlStop,"icon_control_stop_56.png");

    //Message
    IMGPATH( imgIconHelp ,"icon_message_30.png");
    IMGPATH( imgIconMessageError ,"icon_message_error_30.png");
    IMGPATH( imgIconMessageInfo ,"icon_message_info_30.png");
    IMGPATH( imgIconMessageNeutral ,"icon_message_neutral_30.png");
    IMGPATH( imgIconMessageWarning ,"icon_message_warning_30.png");

    // img numbers
    IMGPATH( imgNumber00H , "numbers_00_h.png" );
    IMGPATH( imgNumber00  , "numbers_00.png"   );
    IMGPATH( imgNumber01H , "numbers_01_h.png" );
    IMGPATH( imgNumber01  , "numbers_01.png"   );
    IMGPATH( imgNumber02H , "numbers_02_h.png" );
    IMGPATH( imgNumber02  , "numbers_02.png"   );
    IMGPATH( imgNumber03H , "numbers_03_h.png" );
    IMGPATH( imgNumber03  , "numbers_03.png"   );
    IMGPATH( imgNumber04H , "numbers_04_h.png" );
    IMGPATH( imgNumber04  , "numbers_04.png"   );
    IMGPATH( imgNumber05H , "numbers_05_h.png" );
    IMGPATH( imgNumber05  , "numbers_05.png"   );
    IMGPATH( imgNumber06H , "numbers_06_h.png" );
    IMGPATH( imgNumber06  , "numbers_06.png"   );
    IMGPATH( imgNumber07H , "numbers_07_h.png" );
    IMGPATH( imgNumber07  , "numbers_07.png"   );
    IMGPATH( imgNumber08H , "numbers_08_h.png" );
    IMGPATH( imgNumber08  , "numbers_08.png"   );
    IMGPATH( imgNumber09H , "numbers_09_h.png" );
    IMGPATH( imgNumber09  , "numbers_09.png"   );

    IMGPATH(imgSignal, "signal.png");

    IMGPATH(imgIconAudio, "icon_audio_64.png");
    //Channel Icons
    IMGPATH(imgNoChannelIcon,"channel.png");

    free (imgPath);
    imgPath = strdup("/usr/share/reel/skinreel");

    //Channel info symbols
    IMGPATH(imgRecordingOn,"recording.png");
    IMGPATH(imgRecordingOff,"recording_off.png");
    IMGPATH(imgDolbyDigitalOn,"dolbydigital.png");
    IMGPATH(imgDolbyDigitalOff,"dolbydigital_off.png");
    IMGPATH(imgEncryptedOn,"encrypted.png");
    IMGPATH(imgEncryptedOff,"encrypted_off.png");
    IMGPATH(imgTeletextOn,"teletext.png");
    IMGPATH(imgTeletextOff,"teletext_off.png");
    IMGPATH(imgRadio,"radio.png");  // TODO: Not used?
    IMGPATH(imgAudio, "audio.png");
    IMGPATH(img43, "43.png");
    IMGPATH(img169, "169.png");

    IMGPATH( imgFreetoair,"freetoair.png");

    free(imgPath);
}

inline void DrawUnbufferedImage(cOsd* osd, std::string imgName, int pictureSlot, int xLeft, int yTop, int blend, int horRep=1, int vertRep=1)
{
  //TODO checks if the last image is the same => No SetImagePath
  std::string path = ReelConfig.GetImagesDir();
  path = path + "/" + imgName;
  osd->SetImagePath(imgUnbufferedEnum+pictureSlot, path.c_str());
  osd->DrawImage(imgUnbufferedEnum+pictureSlot, xLeft, yTop, blend, horRep, vertRep);
}

cString GetFullDateTime(time_t t=0)
{
	char buffer[32];
	if (t == 0)
		time(&t);
	struct tm tm_r;
	tm *tm = localtime_r(&t, &tm_r);
	snprintf(buffer, sizeof(buffer), "%s %02d.%02d.%04d %02d:%02d", *WeekDayName(tm->tm_wday), tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
	return buffer;
}

// --- cSkinReelDisplayChannel --------------------------------------------

cSkinReelDisplayChannel::cSkinReelDisplayChannel(bool WithInfo)
{
  debug("cSkinReelDisplayChannel::cSkinReelDisplayChannel(%d)", WithInfo);

  xSymbol = 0;
  isMessage = false;
  strBarWidth = 10000; snrBarWidth = 10000;
  fWithInfo = WithInfo;
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  lastAspect = 0;

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontTitle = ReelConfig.GetFont(FONT_CITITLE);
  pFontSubtitle = ReelConfig.GetFont(FONT_CISUBTITLE);
  pFontLanguage = ReelConfig.GetFont(FONT_CILANGUAGE);
  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  fShowLogo = ReelConfig.showLogo;
  xFirstSymbol = 0;
  nMessagesShown = 0;
  strLastDate = NULL;

  int MessageHeight = std::max(pFontMessage->Height(), MINMESSAGEHEIGHT);
  int LogoSize = 0;
  if (fWithInfo)
    LogoSize = std::max(pFontTitle->Height() * 2 + pFontSubtitle->Height() * 2 + SmallGap*0, ChannelLogoHeight);
  else
    LogoSize = std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()) + TitleDecoGap + TitleDecoHeight + TitleDecoGap2 + MessageHeight + pFontLanguage->Height() + SmallGap, ChannelLogoHeight);

  LogoSize += (LogoSize % 2 ? 1 : 0);
  // title bar & logo area
  xLogoLeft = 0;
  xLogoRight = xLogoLeft + LogoSize;
  xLogoDecoLeft = xLogoRight + LogoDecoGap;
  xLogoDecoRight = xLogoDecoLeft + LogoDecoWidth;
  xTitleLeft = (fShowLogo && (!ReelConfig.fullTitleWidth || !fWithInfo) ? xLogoDecoRight + LogoDecoGap2 : xLogoLeft);
  xTitleRight = xTitleLeft + ((OsdSize.w - xTitleLeft) & ~0x07); // width must be multiple of 8
  yTitleTop = 0;
  yTitleBottom = yTitleTop + std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()), IMGHEADERHEIGHT) ;
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  yLogoTop = (fWithInfo ? yTitleDecoBottom + TitleDecoGap2 : yTitleTop);
  //yLogoBottom = yLogoTop + LogoSize;
  xLogoPos = xLogoLeft + (LogoSize - ChannelLogoHeight) / 2;
  yLogoPos = yLogoTop + (LogoSize - ChannelLogoHeight) / 2;
  if (fWithInfo) {
    // current event area
    xEventNowLeft = (fShowLogo ? xLogoDecoRight + LogoDecoGap2 : xTitleLeft);
    xEventNowRight = xTitleRight;
    yEventNowTop = yLogoTop;
    yEventNowBottom = yEventNowTop + pFontTitle->Height() + pFontSubtitle->Height();
    // next event area
    xEventNextLeft = xEventNowLeft;
    xEventNextRight = xEventNowRight;
    yEventNextTop = yEventNowBottom + SmallGap;
    yEventNextBottom = yEventNextTop + pFontTitle->Height() + pFontSubtitle->Height();
  } else {
    xEventNowLeft   = xEventNextLeft   = (fShowLogo ? xLogoDecoRight + LogoDecoGap2 : xTitleLeft);
    xEventNowRight  = xEventNextRight  = xTitleRight;
    yEventNowTop    = yEventNextTop    = yTitleDecoBottom + TitleDecoGap2;
    yEventNowBottom = yEventNextBottom = yLogoBottom - pFontLanguage->Height() - SmallGap;
  }
  yLogoBottom = yEventNextBottom;

  // progress bar area
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomTop = yEventNextBottom + SmallGap;
  yBottomBottom = yBottomTop + std::max(pFontLanguage->Height(), IMGFOOTERHEIGHT);
  // message area
  xMessageLeft = xEventNowLeft;
  xMessageRight = xTitleRight;
  yMessageTop = yLogoTop + (LogoSize - MessageHeight) / 2;
  yMessageBottom = yMessageTop + MessageHeight;
  // date area
  cString date = GetFullDateTime();
  int w = pFontDate->Width(date);
  xDateLeft = xTitleRight - Roundness - w - SmallGap;

  // create osd
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + (Setup.ChannelInfoPos ? 0 : (OsdSize.h - yBottomBottom)), Setup.OSDRandom );
  //tArea Areas[] = { {0, 0, xBottomRight - 1, yBottomBottom - 1, fShowLogo || ReelConfig.showFlags ? 8 : 4} };
  tArea Areas[] = { {0, 0, xBottomRight - 1, yBottomBottom - 1, 32} };
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayChannel: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    // clear all
    //TB: unnecessary: //osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  } else {
    debug("cSkinReelDisplayChannel: using multiple areas");
    if (fShowLogo) {
      tArea Areas[] = { {xLogoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, 4},
                        {xTitleLeft, yTitleTop, xTitleRight - 1, yTitleDecoBottom - 1, 2},
                        {xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNextBottom - 1, 4},
                        {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 4}
                      };
      eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayChannel: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    } else {
      tArea Areas[] = { {xTitleLeft, yTitleTop, xTitleRight - 1, yTitleDecoBottom - 1, 2},
                        {xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNextBottom - 1, 4},
                        {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 4}
      };
      eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayChannel: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    }
  }

  SetImagePaths(osd);

  DrawAreas();
}

cSkinReelDisplayChannel::~cSkinReelDisplayChannel()
{
  debug("cSkinReelDisplayChannel::~cSkinReelDisplayChannel()");
  free(strLastDate);
  delete osd;
}

void cSkinReelDisplayChannel::DrawAreas(void)
{
  // draw titlebar
  // Header
  osd->DrawImage(imgChannelInfoHeaderLeft, xTitleLeft, yTitleTop, false);
  osd->DrawImage(imgChannelInfoHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight-xTitleLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelInfoHeaderRight, xTitleRight - Roundness , yTitleTop, false);

  // about LOGO : Channel symbol header
  osd->DrawImage(imgChannelSymbolHeaderLeft, xLogoLeft, yTitleTop, false);
  osd->DrawImage(imgChannelSymbolHeaderCenterX, xLogoLeft + Roundness, yTitleTop, false, xLogoRight-xLogoLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelSymbolHeaderRight, xLogoRight - Roundness , yTitleTop, false);

  // draw current event area
  osd->DrawRectangle(xEventNowLeft, yEventNowTop, xEventNowRight - 1,
                       yEventNowBottom - 1, Theme.Color(clrBackground));

  if (fWithInfo) {
    // draw next event area
    osd->DrawRectangle(xEventNextLeft, yEventNextTop, xEventNextRight - 1,
                       yEventNextBottom - 1, Theme.Color(clrAltBackground));
  }

  // draw progress bar area
  // Footer
  osd->DrawImage(imgChannelInfoFooterLeft, xBottomLeft, yBottomTop, false);
  osd->DrawImage(imgChannelInfoFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelInfoFooterRight, xBottomRight - Roundness , yBottomTop, false);

  //Channel symbol
  osd->DrawImage(imgChannelSymbolFooterLeft, xLogoLeft, yBottomTop, false);
  osd->DrawImage(imgChannelSymbolFooterCenterX, xLogoLeft + Roundness, yBottomTop, false, xLogoRight-xLogoLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelSymbolFooterRight, xLogoRight - Roundness , yBottomTop, false);

  // Draw RMM Logo
  if(fShowLogo)
  {
	  // logo area
	  osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
	  osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
	  // Draw LOGO
	  osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
			  yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
  }
}

void cSkinReelDisplayChannel::DrawGroupInfo(const cChannel *Channel, int Number)
{
  DrawAreas();
  DrawTitle(GetChannelName(Channel));
}

void cSkinReelDisplayChannel::DrawTitle(const char *Title)
{
  if (Title) {
    int xName = (fShowLogo && ReelConfig.fullTitleWidth && fWithInfo ? xEventNowLeft : xTitleLeft + Roundness + pFontOsdTitle->Width("0000-") + Gap);
    // draw titlebar
    int y = yTitleTop + (yTitleBottom - yTitleTop - pFontOsdTitle->Height()) / 2;
    // draw channel group name
    osd->DrawText(xName, y, Title, Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle, xDateLeft - SmallGap - xName - 3, yTitleBottom - y);
  }
}

void cSkinReelDisplayChannel::DrawChannelInfo(const cChannel *Channel, int Number)
{
  //osd->SetAntiAliasGranularity(255,255);
  DrawAreas();

  int xNumber = xTitleLeft + Roundness;
  int xName = xNumber + pFontOsdTitle->Width("0000-") + Gap;
  if (fShowLogo && ReelConfig.fullTitleWidth && fWithInfo) {
    xNumber = xTitleLeft + Roundness;
    xName = xEventNowLeft;
  }

  // draw channel number
  osd->DrawText(xNumber, yTitleTop, GetChannelNumber(Channel, Number),
                Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                xName - xNumber - Gap, yTitleBottom - yTitleTop, taCenter);

  // draw channel name
  DrawTitle(GetChannelName(Channel));

  if (fWithInfo && ReelConfig.showStatusSymbols)
    DrawSymbols(Channel);
}

void cSkinReelDisplayChannel::DrawSymbols(const cChannel *Channel)
{
  // draw symbols
  // right edge of logo
  xSymbol = xBottomRight - Roundness;
  // bottom edge of logo
  //int ys = yBottomTop + (yBottomBottom - yBottomTop - SymbolHeight) / 2;
  int ys = yTitleTop + (yTitleBottom - yTitleTop - SymbolHeight) / 2;
  bool isvps = false;
  if (ReelConfig.showVps) {
    // check if vps
    // get schedule
    cSchedulesLock SchedulesLock;
    const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
    if (Schedules) {
      const cSchedule *Schedule = Schedules->GetSchedule(Channel);
      if (Schedule) {
        // get present event
        const cEvent *Event = Schedule->GetPresentEvent();
        // check if present event has vps
        if (Event && Event->Vps()) {
          isvps = true;
        }
      }
    }
  }

  bool rec = cRecordControls::Active();
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(rec ? imgRecordingOn:imgRecordingOff, xSymbol,ys,true);

  // draw audio symbol according to current audio channel
  int AudioMode = cDevice::PrimaryDevice()->GetAudioChannel();
  if (!(AudioMode >= 0 && AudioMode < MAX_AUDIO_BITMAPS))
    AudioMode = 0;
  xSymbol -= ( CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(imgAudio,xSymbol,ys,true);

    // draw dolby digital symbol
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(Channel->Dpid(0) ? imgDolbyDigitalOn:imgDolbyDigitalOff, xSymbol,ys,true);

  // Teletext
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(Channel->Vpid()? imgTeletextOn: imgTeletextOff, xSymbol, ys, true);

  // draw encryption symbol
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(Channel->Ca()? imgEncryptedOn: imgEncryptedOff, xSymbol, ys, true);
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);

  // Draw FREE TO AIR, NAGRAVISION in Footer

  UpdateSignal();
  UpdateAspect();
#define imgChannelInfoIconHeight 18
  int y = yBottomTop + (yBottomBottom - yBottomTop)/2 - imgChannelInfoIconHeight/2 ; // Footer
  int xs = xeSignalBar + Gap; // end of signal bar

  std::string imgCa = "";
  switch (Channel->Ca())
  {
        case 0x0000:
            /* Reserved */
//            imgCa = imgFreetoair;
            osd->DrawImage(imgFreetoair, xs, y, true);
            break;
        case 0x0100 ... 0x01FF:
            /* Canal Plus */
            imgCa = uimgSeca;
            break;
        case 0x0500 ... 0x05FF:
            /* France Telecom */
            imgCa = uimgViaccess;
            break;
        case 0x0600 ... 0x06FF:
            /* Irdeto */
            imgCa = uimgIrdeto;
            break;
        case 0x0900 ... 0x09FF:
            /* News Datacom */
            imgCa = uimgNds;
            break;
        case 0x0B00 ... 0x0BFF:
            /* Norwegian Telekom */
            imgCa = uimgConax;
            break;
        case 0x0D00 ... 0x0DFF:
            /* Philips */
            imgCa = uimgCryptoworks;
            break;
        case 0x0E00 ... 0x0EFF:
            /* Scientific Atlanta */
            imgCa = uimgPowervu;
            break;
        case 0x1200 ... 0x12FF:
            /* BellVu Express */
            imgCa = uimgNagravision;
            break;
        case 0x1700 ... 0x17FF:
            /* BetaTechnik */
            imgCa = uimgBetacrypt;
            break;
        case 0x1800 ... 0x18FF:
            /* Kudelski SA */
            imgCa = uimgNagravision;
            break;
        case 0x4A60 ... 0x4A6F:
            /* @Sky */
            imgCa = uimgSkycrypt;
            break;
        default:
            imgCa = "";
            break;
  }

  if(imgCa != "")
    DrawUnbufferedImage(osd, imgCa, 0, xs, y, true);

  //xFirstSymbol = DrawStatusSymbols(xBottomLeft + Roundness + MIN_CI_PROGRESS + Gap, xs, yBottomTop, yBottomBottom, Channel) - Gap;
  xFirstSymbol = DrawStatusSymbols(xBottomLeft + Roundness + MIN_CI_PROGRESS + Gap, xSymbol, yTitleTop, yTitleBottom, Channel) - Gap;
}

cString cSkinReelDisplayChannel::GetChannelName(const cChannel *Channel)
{
  std::string buffer;
  // check if channel exists
  if (Channel)
    buffer = Channel->Name();
  else
    buffer = tr("*** Invalid Channel ***");
  return buffer.c_str();
}

cString cSkinReelDisplayChannel::GetChannelNumber(const cChannel *Channel, int Number)
{
  static char buffer[256];
  // check if real channel exists
  if (Channel && !Channel->GroupSep()) {
    snprintf(buffer, sizeof(buffer), "%02d%s", Channel->Number(), Number ? "-" : "");
  } else if (Number) {
    // no channel but number
    snprintf(buffer, sizeof(buffer), "%d-", Number);
  } else {
    // no channel and no number
    strcpy(buffer, " ");
  }
  return buffer;
}

void cSkinReelDisplayChannel::DrawChannelLogo(const cChannel *channel)
{
 if (!fShowLogo)
   return; // no logo to be shown

 // logo area
 //osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground)); // the real logo background
 //osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));

 if (!channel)  // Draw RMM logo if no channel/ no logo/no logoDir is set
 {
	 // logo area
	 osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground)); // the real logo background
	 osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
        //RMM Logo
	 osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
			 yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
	 dsyslog("Drew RMM Logo since *channel== %#x && fShowLogo == %i \n", (unsigned int)channel, fShowLogo);
	 return ;
 }

 std::string logoPath = std::string(ReelConfig.GetLogoDir()) + "/" + channel->Name() + ".png";

 //check for png file
 if ( access(logoPath.c_str(),R_OK) == 0 ) // file can be read
 {
	 // logo area
	 osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrLogoBg)); // the real logo background
	 osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrLogoBg));
	 // draw logo
	 osd->SetImagePath(imgUnbufferedEnum, logoPath.c_str());
	 osd->DrawImage(imgUnbufferedEnum, xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
			 yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
	 std::cout << "(" << __FILE__ << ":" << __LINE__ << ") Drew " << logoPath << std::endl;
 } else {
	 // logo area
	 osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground)); // the real logo background
	 osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
	 //RMM Logo
	 osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
			 yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
	// std::cout << "(" << __FILE__ << ":" << __LINE << ")" << " Cannot read channel logo " << logoPath << std::endl;
	isyslog("(%s:%i) ERROR Cannot read CHANNEL LOGO %s\n",__FILE__,__LINE__,logoPath.c_str());
 }

} //end DrawChannelLogo


void cSkinReelDisplayChannel::SetChannel(const cChannel *Channel, int Number)
{
  debug("cSkinReelDisplayChannel::SetChannel()");
  //printf("cSkinReelDisplayChannel::SetChannel(%#x (%i),%i)\n",Channel, Channel? Channel->Number():-1,Number);

  xFirstSymbol = 0;
  free(strLastDate);
  strLastDate = NULL;

  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);

  if (Channel) {
    if (Channel->GroupSep())
      DrawGroupInfo(Channel, Number);
    else
      DrawChannelInfo(Channel, Number);
  }

  DrawChannelLogo(Channel);
}

void cSkinReelDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following)
{
  //TODO: update symbols
  debug("cSkinReelDisplayChannel::SetEvents(%p, %p)", Present, Following);

  if (!fWithInfo)
    return;

  // Draw the event area again, since DrawText does not clear the already drawn text but blends over it!
  // taken from DrawAreas()

  // draw current event area
  osd->DrawRectangle(xEventNowLeft, yEventNowTop, xEventNowRight - 1,
          yEventNowBottom - 1, Theme.Color(clrBackground));
  // draw next event area
  osd->DrawRectangle(xEventNextLeft, yEventNextTop, xEventNextRight - 1,
          yEventNextBottom - 1, Theme.Color(clrAltBackground));

  int xTimeLeft = xEventNowLeft + Gap;
  int xTimeWidth = pFontTitle->Width("00:00");
  int lineHeightTitle = pFontTitle->Height();
  int lineHeightSubtitle = pFontSubtitle->Height();

  // check epg datas
  const cEvent *e = Present;    // Current event
  if (e) {
    std::stringstream sLen;
    std::stringstream sNow;
    int total = e->Duration();
    if (total >= 120) {
       sLen << total / 60 << " " << tr("mins");
    } else {
       sLen << total / 60 << " " << tr("min");
    }
    int now = (time(NULL) - e->StartTime());
    if ((now < total) && ((now / 60) > 0)) {
      switch (ReelConfig.showRemaining) {
        case 0:
          sNow << "+" << (int)(now / 60) << "'";
          break;
        case 1:
          sNow << "-" << (int)ceil((total - now) / 60.0) << "'";
          break;
        case 2:
          sNow << lrint((ceil((float)now) / total * 100.0)) << "%";
          break;
        default:
          error("Invalid value for ShowRemaining: %d", ReelConfig.showRemaining);
      }
    }

    int xDurationLeft = xEventNowRight - Gap - std::max(pFontTitle->Width(sLen.str().c_str()), pFontSubtitle->Width(sNow.str().c_str()));
    int xDurationWidth = xEventNowRight - Gap - xDurationLeft;
    int xTextLeft = xTimeLeft + xTimeWidth + BigGap;
    int xTextWidth = xDurationLeft - xTextLeft - BigGap;

    // draw start time
    osd->DrawText(xTimeLeft, yEventNowTop, e->GetTimeString(),
                  Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrBackground)*/,
                  pFontTitle, xTimeWidth);
    // draw title
    osd->DrawText(xTextLeft, yEventNowTop, e->Title(),
                    Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrBackground)*/,
                    pFontTitle, xTextWidth);

    // draw duration
    osd->DrawText(xDurationLeft, yEventNowTop, sLen.str().c_str(),
                  Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrBackground)*/,
                  pFontTitle, xDurationWidth, lineHeightTitle, taRight);
    osd->DrawText(xTextLeft, yEventNowTop + lineHeightTitle, e->ShortText(),
                    Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent /*Theme.Color(clrBackground)*/, pFontSubtitle, xTextWidth);

    // draw duration
    if ((now < total) && ((now / 60) > 0)) {
      osd->DrawText(xDurationLeft, yEventNowTop + lineHeightTitle, sNow.str().c_str(),
                    Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent /*Theme.Color(clrBackground)*/, pFontSubtitle, xDurationWidth,
                    lineHeightSubtitle, taRight);
    }
    // draw timebar

    int xBarLen = pFontTitle->Width("0000") ; //- SmallGap;
    int xBarLeft  = xTimeLeft + SmallGap;
    int yBarThickness = Gap;
    int yBarTop = yEventNowTop + lineHeightTitle + Gap;//*1.5- yBarThickness/2; // center it in the center :)
    int x = 0;

    if (total)
       x = int(xBarLeft + xBarLen *(float)(now)/total - TinyGap);

    x = std::min(x, xBarLeft + xBarLen);

    // draw progressbar
    osd->DrawRectangle(xBarLeft, yBarTop , xBarLeft+xBarLen, yBarTop+ yBarThickness, Theme.Color(clrProgressBarBg));//0x00222220);
    osd->DrawRectangle(xBarLeft, yBarTop , x, yBarTop+ yBarThickness, Theme.Color(clrProgressBarFg));
  }

  e = Following;                // Next event
  if (e) {
    std::stringstream sLen;
    std::stringstream sNext;

    if ((int)(e->Duration()/60) > 1) {
       sLen << e->Duration() / 60 << " " << tr("mins");
    } else {
       sLen << e->Duration() / 60 << " " << tr("min");
    }
    sNext << "-" << (int)ceil((e->StartTime() - time(NULL)) / 60.0) << "'";

    int xDurationLeft = xEventNowRight - Gap - pFontTitle->Width(sLen.str().c_str());
    int xDurationWidth = xEventNowRight - Gap - xDurationLeft;
    int xTextLeft = xTimeLeft + xTimeWidth + BigGap;
    int xTextWidth = xDurationLeft - xTextLeft - BigGap;

    // draw start time
    osd->DrawText(xTimeLeft, yEventNextTop, e->GetTimeString(),
                  Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                  pFontTitle, xTimeWidth);
    // draw title
    osd->DrawText(xTextLeft, yEventNextTop, e->Title(),
                  Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                  pFontTitle, xTextWidth);
    // draw duration
    osd->DrawText(xDurationLeft, yEventNextTop, sLen.str().c_str(),
                  Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                  pFontTitle, xDurationWidth, lineHeightTitle, taRight);
    // draw remaining time
    osd->DrawText(xDurationLeft, yEventNextTop + lineHeightTitle, sNext.str().c_str(),
                    Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent /*Theme.Color(clrBackground)*/, pFontSubtitle, xDurationWidth,
                    lineHeightSubtitle, taRight);

   // draw shorttext
    osd->DrawText(xTextLeft, yEventNextTop + lineHeightTitle, e->ShortText(),
                  Theme.Color(clrMenuItemNotSelectableFg),
                  clrTransparent /*Theme.Color(clrAltBackground)*/, pFontSubtitle, xTextWidth);
  }

}

void cSkinReelDisplayChannel::SetMessage(eMessageType Type, const char *Text)
{
  // cFont const *font = cFont::GetFont(fontSml);
  tColor  colorfg = Theme.Color(clrTitleFg);
  int imgMessageIcon;

  int x0 = xBottomLeft;
  int x1 = x0 + Roundness;
  int x2 = xBottomRight - Roundness;

  int y0 = yBottomTop;
  int y1 = y0 + IMGFOOTERHEIGHT; // height of footer

  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  switch(Type) {
    case mtStatus:
      imgMessageIcon = imgIconMessageInfo;
      break;
    case mtWarning:
      imgMessageIcon = imgIconMessageWarning;
      break;
    case mtError:
      imgMessageIcon = imgIconMessageError;
      break;
    default:
      imgMessageIcon = imgIconMessageNeutral;
      break;
  }
  colorfg = 0xFFFFFFFF;

  if (Text) {
   isMessage = true; // prevents signal and date being drawn in Flush()
   strLastDate = NULL;

   // Draw Footer
   osd->DrawImage(imgChannelInfoFooterLeft, xBottomLeft, yBottomTop, false);
   osd->DrawImage(imgChannelInfoFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
   osd->DrawImage(imgChannelInfoFooterRight, xBottomRight - Roundness , yBottomTop, false);

   osd->DrawImage(imgMessageIcon, x1,y0, true);
   #define messageIconW 30
   //display message
   osd->DrawText(x1 + messageIconW, y0 + (y1 - y0)/2 - pFontMessage->Height()/2, Text, colorfg, clrTransparent, pFontMessage, x2 - x1 - messageIconW , y1 - y0, taCenter);

  } else {
    if(isMessage == true) // Clean the  Footer
    {
       osd->DrawImage(imgChannelInfoFooterLeft, xBottomLeft, yBottomTop, false);
       osd->DrawImage(imgChannelInfoFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
       osd->DrawImage(imgChannelInfoFooterRight, xBottomRight - Roundness , yBottomTop, false);
    }
    isMessage = false;
  }
}

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

void GetSignal(int &str, int &snr, fe_status_t &status)
{
    str = 0;
    snr = 0;
    int const cardIndex = cDevice::ActualDevice()->CardIndex();
    static char dev[256];

    std::sprintf(dev, FRONTEND_DEVICE, cardIndex, 0);
    int fe = open(dev, O_RDONLY | O_NONBLOCK);
    if (fe < 0)
        return;
    ::ioctl(fe, FE_READ_SIGNAL_STRENGTH, &str);
    ::ioctl(fe, FE_READ_SNR, &snr);
#ifdef ROTORTUNER_INFO
    // TB: fe_status is only needed when using rotor-info
    ::ioctl(fe, FE_READ_STATUS, &status);
#endif
    close(fe);
}

void cSkinReelDisplayChannel::UpdateSignal()
{
 int str, snr;
 fe_status_t status;
 GetSignal(str, snr, status);

#ifdef ROTORTUNER_INFO
 char txt[256];

 // GA: Demo only, this needs to be done much better...
 if ((status>>12)&7) { // !=0 -> special netceiver status delivered
   int rotorstat=(status>>8)&7;  // 0: rotor not moving, 1: move to cached position, 2: move to unknown pos, 3: autofocus
   char *txt0="";
   char *txt1="Rotor";
   char *txt2="RotorS";
   char *txt3="RotorAF";

   sprintf(txt,"T%i %s",(status>>12)&7,rotorstat==1?txt1:(rotorstat==2?txt2:(rotorstat==3?txt3:txt0)));
   osd->DrawImage(imgChannelInfoFooterCenterX, xDateLeft-120, yBottomTop, false, 120,1);
   int w = std::min(pFontDate->Width(txt), xBottomRight - xDateLeft - Roundness );
   osd->DrawText(xDateLeft-120, yBottomTop + (yBottomBottom - yBottomTop)/2 - pFontDate->Height()/2, txt, Theme.Color(clrTitleFg),
	clrTransparent, pFontDate, w, yBottomBottom - yBottomTop);
 }
#endif

 int y = yBottomTop + (yBottomBottom - yBottomTop)/2;
 xsSignalBar = xTitleLeft + Gap + Roundness ;

 osd->DrawImage(imgSignal, xsSignalBar, y - CHANNELINFOSYMBOLHEIGHT/2, true ); //
 xsSignalBar += 72 + SmallGap;  // 72 is imgSignal length

 int bw = 45;
 xeSignalBar = xsSignalBar + bw;

 str = str * bw / 0xFFFF;
 snr = snr * bw / 0xFFFF;

 if (1 || str != strBarWidth || snr != snrBarWidth) //TODO: remove 1 and check why is signalbar missing sometimes
   {
    strBarWidth = str;
    snrBarWidth = snr;

    int h = 4;
    int yStr = int(y - 1.75*h);
    int ySnr = int(y + h*.75);

    // Draw Background
    osd->DrawRectangle(xsSignalBar, yStr, xeSignalBar - 1, yStr + h , Theme.Color(clrSignalBg));
    osd->DrawRectangle(xsSignalBar, ySnr, xeSignalBar - 1, ySnr + h , Theme.Color(clrSignalBg));
    // Draw Foreground

    int signalFgColor = Theme.Color(clrSignalHighFg);

    // medium signal : ORANGE
    if (str < 0.6*bw && str > 0.5 *bw)
        signalFgColor = Theme.Color(clrSignalMediumFg);
    else if (str>=0 && str <= 0.5 * bw)
        signalFgColor = Theme.Color(clrSignalLowFg);

    if (str)
    {
        	osd->DrawRectangle(xsSignalBar, yStr , xsSignalBar + str - 1, yStr + h , signalFgColor);
    /*
    	if (str>=bw/2)
    	{
        	//osd->DrawRectangle(xsSignalBar, yStr , xsSignalBar + str - 1, yStr + h - 1, Theme.Color(clrSignalFg));

        	// Yellow bar for the first 50%
        	osd->DrawRectangle(xsSignalBar, yStr , xsSignalBar + (str<bw/2? str-1:bw/2) , yStr + h - 1, clrYellow);

        	// in Green : the greater half
        	osd->DrawRectangle(xsSignalBar+bw/2, yStr , xsSignalBar + str - 1 , yStr + h - 1, clrGreen);
	} else {
        	// Red bar for signal less then 50%
        	osd->DrawRectangle(xsSignalBar, yStr , xsSignalBar + (str<bw/2? str-1:bw/2) , yStr + h - 1, clrRed);
    	}
        */
    } // str

    signalFgColor = Theme.Color(clrSignalHighFg);

    // medium signal : ORANGE
    if (snr < 0.6*bw && snr > 0.5 *bw)
        signalFgColor = Theme.Color(clrSignalMediumFg);
    // low signal : RED
    else if (snr>=0 && snr <= 0.5 * bw)
        signalFgColor = Theme.Color(clrSignalLowFg);

    if (snr)
    {
        osd->DrawRectangle(xsSignalBar, ySnr , xsSignalBar + snr - 1, ySnr + h , signalFgColor);
    /*
	if (snr>=bw/2)
	{
        	//osd->DrawRectangle(xsSignalBar, ySnr , xsSignalBar + snr - 1, ySnr + h - 1, Theme.Color(clrSignalFg));

        	// Yellow bar for the first 50%
        	osd->DrawRectangle(xsSignalBar, ySnr , xsSignalBar + (snr<bw/2? snr-1:bw/2) , ySnr + h - 1, clrYellow);

        	// in Green : the greater half
		osd->DrawRectangle(xsSignalBar+bw/2, ySnr , xsSignalBar + snr - 1 , ySnr + h - 1, clrGreen);
	} else {
        	// Red bar for signal less then 50%
        	osd->DrawRectangle(xsSignalBar, ySnr , xsSignalBar + (snr<bw/2? snr-1:bw/2) , ySnr + h - 1, clrRed);
	}
        */
    }// snr
   }// if 1
}

void cSkinReelDisplayChannel::UpdateDateTime(void)
{
  cString date = GetFullDateTime();
  if ((strLastDate == NULL) || strcmp(strLastDate, (const char*)date) != 0) {
    free(strLastDate);
    strLastDate = strdup((const char*)date);
    // update date string
    int w = std::min(pFontDate->Width(date), xBottomRight - xDateLeft - Roundness );
    // clear the Date by drawing the footer
    osd->DrawImage(imgChannelInfoFooterCenterX, xDateLeft, yBottomTop, false, xBottomRight - xDateLeft - Roundness,1);
    osd->DrawImage(imgChannelInfoFooterRight, xBottomRight - Roundness , yBottomTop, false);

    osd->DrawText(xDateLeft, yBottomTop + (yBottomBottom - yBottomTop)/2 - pFontDate->Height()/2, date, Theme.Color(clrTitleFg), /*Theme.Color(clrBottomBg)*/clrTransparent, pFontDate, w, yBottomBottom - yBottomTop-10);
  }
}

void cSkinReelDisplayChannel::UpdateAspect()
{
  // Aspect Ratio
  enum { aspectnone, aspect43, aspect169 };
  int ys = yTitleTop + (yTitleBottom - yTitleTop - SymbolHeight) / 2;

  //clear
  osd->DrawImage(imgChannelInfoHeaderCenterX, xSymbol, yTitleTop, false, CHANNELINFOSYMBOLWIDTH, 1);

#if VDRVERSNUM < 10716
  lastAspect = cDevice::PrimaryDevice()->GetAspectRatio();

  switch(lastAspect) {
    case aspect43:
      osd->DrawImage(img43, xSymbol, ys, true);
      break;
    case aspect169:
      osd->DrawImage(img169, xSymbol, ys, true);
      break;
    case aspectnone:
    default:
      break;
  }
#else
  int h,w;
  cDevice::PrimaryDevice()->GetVideoSize(w,h,lastAspect);
  if(4.0/3.0 == lastAspect)
    osd->DrawImage(img43, xSymbol, ys, true);
  else if(16.0/9.0 == lastAspect)
    osd->DrawImage(img169, xSymbol, ys, true);
#endif
}

void cSkinReelDisplayChannel::Flush(void)
{
  debug("cSkinReelDisplayChannel::Flush()");

  if(!isMessage)
  {
    UpdateDateTime();
    UpdateSignal();
#if VDRVERSNUM < 10716
    if ((xSymbol != 0) && (cDevice::PrimaryDevice()->GetAspectRatio() != lastAspect))
      UpdateAspect();
#else
    int h,w;
    double a;
    cDevice::PrimaryDevice()->GetVideoSize(w,h,a);
    if ((xSymbol != 0) && (a != lastAspect))
      UpdateAspect();
#endif
  }
  osd->Flush();

}

// --- cSkinReelDisplayMenu -----------------------------------------------

cSkinReelDisplayMenu::cSkinReelDisplayMenu(void)
{
  debug("%s", __PRETTY_FUNCTION__);

  for(int i=0; i<NR_UNBUFFERED_SLOTS; i++) {
    strcpy(ReelConfig.curThumb[i].path, "");
    thumbCleared[i] = 1;
    ReelConfig.curThumb[i].x = ReelConfig.curThumb[i].y = ReelConfig.curThumb[i].w = ReelConfig.curThumb[i].h = 0;
  }

  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  withTitleImage = false;
  old_menulist_mtime = 0;

  fSetupAreasDone = false;
  setlocale(LC_TIME, tr("en_US"));
  osd = NULL;

  pFontList = ReelConfig.GetFont(FONT_LISTITEM);
  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);

  const cFont *pFontDate = ReelConfig.GetFont(FONT_DATE);
  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);

  isMainPage = 0;
  isCAPS = false;
  strTitle = NULL;
  strLastDate = NULL;
  strTheme = strdup(Theme.Name());
  isMainMenu = false;
  isCenteredMenu = false;
  fShowLogo = false;
#ifdef SKINREEL_NO_MENULOGO
  fShowLogoDefault = false;
#else
  fShowLogoDefault = ReelConfig.showSymbols && (ReelConfig.showSymbolsMenu || ReelConfig.showImages);
#endif
  fShowInfo = false;
  nMessagesShown = 0;
  nNumImageColors = 2;

  int LogoHeight = std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()) + TitleDeco + pFontDetailsTitle->Height() + Gap + pFontDetailsSubtitle->Height(),
                            std::max(3 * pFontDate->Height(), (ReelConfig.showImages ? std::max(ReelConfig.imageHeight, IconHeight) : IconHeight)));
  int LogoWidth = ReelConfig.showImages ? std::max(IconWidth, ReelConfig.imageWidth) : IconWidth;
  int RightColWidth = 0;
  cString date = GetFullDateTime();
  if (fShowLogoDefault) {
    int nMainDateWidth = getDateWidth(pFontDate) + SmallGap + LogoWidth;
    int nSubDateWidth = pFontDate->Width(date);
    RightColWidth = (SmallGap + Gap + std::max(nMainDateWidth, nSubDateWidth) + Gap) & ~0x07; // must be multiple of 8
  } else {
    RightColWidth = (SmallGap + Gap + std::max(MIN_DATEWIDTH + LogoWidth, pFontDate->Width(date)) + Gap) & ~0x07; // must be multiple of 8
  }

  static const cFont *pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);
  int MessageHeight =  std::max(pFontMessage->Height(), MINMESSAGEHEIGHT);

  // title bar
  xTitleLeft = 0;
  xTitleRight = OsdSize.w - RightColWidth;
  yTitleTop = 0;
  yTitleBottom = std::max( std::max(pFontOsdTitle->Height(), pFontDate->Height()), IMGHEADERHEIGHT);
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  // Footer area
  xFooterLeft = xTitleLeft;
  xFooterRight = OsdSize.w;
  yFooterTop = OsdSize.h - IMGFOOTERHEIGHT;
  yFooterBottom = OsdSize.h;
  // help buttons
  xButtonsLeft = xTitleLeft + IMGMENUWIDTH;
  xButtonsRight = OsdSize.w - Roundness ;
  yButtonsTop = yFooterTop + 1 + (IMGFOOTERHEIGHT - IMGBUTTONSHEIGHT) / 2;
  yButtonsBottom =  yButtonsTop + IMGBUTTONSHEIGHT;

  // content area with items
  // Body
  xBodyLeft = xTitleLeft;
  xBodyRight = xTitleRight;
  yBodyTop = yTitleBottom + Gap;
  // message area
  xMessageLeft = xBodyLeft;
  xMessageRight = OsdSize.w;
  // on help buttons
  yMessageBottom = yFooterTop - Gap;
  yMessageTop = yMessageBottom - MessageHeight;
  yBodyBottom = yMessageTop - Gap;
  // logo box
  xLogoLeft = OsdSize.w - LogoWidth;
  xLogoRight = OsdSize.w;
  yLogoTop = yTitleTop;
  yLogoBottom = yLogoTop + LogoHeight + SmallGap;
  // info box
  xInfoLeft = OsdSize.w - RightColWidth;
  xInfoRight = OsdSize.w;
  yInfoTop = yLogoBottom;
  yInfoBottom = yBodyBottom;
  // date box
  xDateLeft = xTitleRight;
  xDateRight = OsdSize.w;
  yDateTop = yTitleTop;
  yDateBottom = yLogoBottom;

  // create osd
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y,Setup.OSDRandom);

  tArea Areas[] = { {xTitleLeft,   yTitleTop, xMessageRight, yButtonsBottom, 32} };
  if (ReelConfig.singleArea8Bpp && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayMenu: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    nNumImageColors = 230; //TODO: find correct number of colors
  } else {
    debug("cSkinReelDisplayMenu: using multiple areas");
    tArea Areas[] = { {xTitleLeft,   yTitleTop, xTitleRight - 1, yTitleDecoBottom - 1, 2}, //title area
                      {xBodyLeft,    yBodyTop, xBodyRight - 1, yInfoTop + pFontDetailsDate->Height() - 1, 2}, //body area (beside date/logo/symbols area)
                      {xDateLeft,    yDateTop, xLogoRight - 1, yInfoTop - 1, 4}, //date/logo area
                      {xInfoLeft,    yInfoTop, xInfoRight - 1, yInfoTop + pFontDetailsDate->Height() - 1, 4}, //area for symbols in event/recording info
                      {xBodyLeft,    yInfoTop + pFontDetailsDate->Height(), xInfoRight - 1, (ReelConfig.statusLineMode == 1 ? yBodyBottom : yMessageTop) - 1, 2}, // body/info area (below symbols area)
                      {xMessageLeft, yMessageTop, xButtonsRight - 1, yButtonsBottom - 1, 4} //buttons/message area
    };

    eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
    if (rc == oeOk)
      osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    else {
      error("cSkinReelDisplayMenu: CanHandleAreas() [1] returned %d", rc);
      delete osd;
      osd = NULL;
      throw 1;
      return;
    }

    nNumImageColors = 13; // "16 available colors" - "clrTransparent" - "clrLogoBg" - "clrMenuTxtFg"
  }

  lineHeight = std::max(pFontList->Height() , 35); // change
  xItemLeft = xBodyLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);
  xItemRight = (fShowLogo || fShowInfo ? xBodyRight : xInfoRight) - ScrollbarWidth - ListHBorder;
  int numItems = MaxItems();
  yItemTop = yBodyTop + ((ReelConfig.statusLineMode == 2 ? yMessageTop : yBodyBottom) - yBodyTop - numItems * lineHeight) / 2;

  SetImagePaths(osd);
  CreateTitleVector();
}

void cSkinReelDisplayMenu::SetColors(void)
{
  debug("%s", __PRETTY_FUNCTION__);
  debug("cSkinReelDisplayMenu::SetColors()");
  return;

  if (osd->GetBitmap(1) == NULL) { //single area
    cBitmap *bitmap = osd->GetBitmap(0);
    if (bitmap) {
      bitmap->Reset();
      bitmap->SetColor( 0, clrTransparent);
      bitmap->SetColor( 1, Theme.Color(clrTitleBg));
      bitmap->SetColor( 2, Theme.Color(clrTitleFg));
      bitmap->SetColor( 3, Theme.Color(clrTitleShadow));
      bitmap->SetColor( 4, Theme.Color(clrLogoBg));
      bitmap->SetColor( 5, Theme.Color(clrBackground));
      bitmap->SetColor( 6, Theme.Color(clrAltBackground));
      bitmap->SetColor( 7, Theme.Color(clrMenuTxtFg));
      bitmap->SetColor( 8, Theme.Color(clrBottomBg));
      bitmap->SetColor( 9, Theme.Color(clrButtonRedBg));
      bitmap->SetColor(10, Theme.Color(clrButtonRedFg));
      bitmap->SetColor(11, Theme.Color(clrButtonGreenBg));
      bitmap->SetColor(12, Theme.Color(clrButtonGreenFg));
      bitmap->SetColor(13, Theme.Color(clrButtonYellowBg));
      bitmap->SetColor(14, Theme.Color(clrButtonYellowFg));
      bitmap->SetColor(15, Theme.Color(clrButtonBlueBg));
      bitmap->SetColor(16, Theme.Color(clrButtonBlueFg));
    }
    return;
  }

  cBitmap *bitmap = osd->GetBitmap(0);
  if (bitmap) { //title area
    bitmap->Reset();
    bitmap->SetColor(0, clrTransparent);
    bitmap->SetColor(1, Theme.Color(clrTitleBg));
    bitmap->SetColor(2, Theme.Color(clrTitleFg));
    bitmap->SetColor(3, Theme.Color(clrTitleShadow));
  }
  bitmap = osd->GetBitmap(1);
  if (bitmap) { //body area (beside date/logo/ area)
    bitmap->Reset();
    bitmap->SetColor(0, Theme.Color(clrBackground));
    bitmap->SetColor(1, Theme.Color(clrAltBackground));
    bitmap->SetColor(2, Theme.Color(clrMenuItemSelectableFg));
    bitmap->SetColor(3, Theme.Color(clrMenuItemNotSelectableFg));
  }
  bitmap = osd->GetBitmap(2);
  if (bitmap) { //date/logo area
    bitmap->Reset();
    if (fShowLogo) {
      bitmap->SetColor(0, clrTransparent);
      bitmap->SetColor(1, Theme.Color(clrLogoBg));
      bitmap->SetColor(2, Theme.Color(clrMenuTxtFg));
    } else {
      bitmap->SetColor(0, Theme.Color(clrBackground));
      bitmap->SetColor(1, Theme.Color(clrAltBackground));
      bitmap->SetColor(2, Theme.Color(clrMenuItemSelectableFg));
      bitmap->SetColor(3, Theme.Color(clrMenuItemNotSelectableFg));
      bitmap->SetColor(4, clrTransparent);
      bitmap->SetColor(5, Theme.Color(clrLogoBg));
      bitmap->SetColor(6, Theme.Color(clrMenuTxtFg));
    }
  }
  bitmap = osd->GetBitmap(3);
  if (bitmap) { //area for symbols in event/recording info
    bitmap->Reset();
    bitmap->SetColor(0, Theme.Color(clrBackground));
    bitmap->SetColor(1, Theme.Color(clrAltBackground));
    bitmap->SetColor(2, Theme.Color(clrMenuItemSelectableFg));
    bitmap->SetColor(3, Theme.Color(clrMenuItemNotSelectableFg));
  }
  bitmap = osd->GetBitmap(4);
  if (bitmap) { //body/info area (below symbols area)
    bitmap->Reset();
    bitmap->SetColor(0, Theme.Color(clrBackground));
    bitmap->SetColor(1, Theme.Color(clrAltBackground));
    bitmap->SetColor(2, Theme.Color(clrMenuItemSelectableFg));
    bitmap->SetColor(3, Theme.Color(clrMenuItemNotSelectableFg));
  }
  bitmap = osd->GetBitmap(5);
  if (bitmap) { //buttons/message area
    bitmap->Reset();
    bitmap->SetColor( 0, Theme.Color(clrBackground));
    bitmap->SetColor( 1, Theme.Color(clrAltBackground));
    bitmap->SetColor( 2, Theme.Color(clrMenuItemSelectableFg));
    bitmap->SetColor( 3, Theme.Color(clrMenuItemNotSelectableFg));
    bitmap->SetColor( 4, Theme.Color(clrBottomBg));
    bitmap->SetColor( 5, Theme.Color(clrButtonRedBg));
    bitmap->SetColor( 6, Theme.Color(clrButtonRedFg));
    bitmap->SetColor( 7, Theme.Color(clrButtonGreenBg));
    bitmap->SetColor( 8, Theme.Color(clrButtonGreenFg));
    bitmap->SetColor( 9, Theme.Color(clrButtonYellowBg));
    bitmap->SetColor(10, Theme.Color(clrButtonYellowFg));
    bitmap->SetColor(11, Theme.Color(clrButtonBlueBg));
    bitmap->SetColor(12, Theme.Color(clrButtonBlueFg));
    bitmap->SetColor(13, clrTransparent);
    // color 14 reserved for "clrMessageStatusFg + 2 * Type"
    // color 15 reserved for "clrMessageStatusBg + 2 * Type"
  }
}

void cSkinReelDisplayMenu::SetupAreas(void)
{
  debug("%s", __PRETTY_FUNCTION__);
  //Must be TE_LOCKed by caller

  debug("cSkinReelDisplayMenu::SetupAreas() %d %d %d", isMainMenu, fShowLogo, fShowInfo);
  fSetupAreasDone = true;
  SetColors();

  xItemRight = (fShowLogo || fShowInfo ? xBodyRight : xInfoRight) - ScrollbarWidth - ListHBorder;

  DrawTitle(strTitle);

  // draw date area
  osd->DrawImage(imgMenuHeaderCenterX, xDateLeft, 0, xDateRight - Roundness, false);

  // draw body area
  osd->DrawRectangle(xBodyLeft, yBodyTop, xBodyRight - 1, yBodyBottom - 1, Theme.Color(clrBackground));

  // draw footer
  //  DrawFooter(); //FLOH: This causes the buttons to disapear; Do we really need this in this function?

  // draw info area
  if (fShowInfo) {
#ifdef CLEAR_BUG_WORKAROUND
    osd->DrawRectangle(xInfoLeft, fShowLogo ? yInfoTop : yBodyTop, xInfoRight - 2, yInfoBottom - 1, Theme.Color(clrAltBackground));
#else
    osd->DrawRectangle(xInfoLeft, fShowLogo ? yInfoTop : yBodyTop, xInfoRight - 1, yInfoBottom - 1, Theme.Color(clrAltBackground));
#endif

    int x = xInfoLeft + Gap;
    int y = fShowLogo ? yInfoTop : yBodyTop;
    int w = xInfoRight - x;
#ifdef CLEAR_BUG_WORKAROUND
    w--;
#endif
    int yMaxHeight = yInfoBottom;

    if (Timers.GetNextActiveTimer()) {

      const cFont *pFontInfoTimerHeadline = ReelConfig.GetFont(FONT_INFOTIMERHEADLINE);
      const cFont *pFontInfoTimerText = ReelConfig.GetFont(FONT_INFOTIMERTEXT);
      int h = pFontInfoTimerHeadline->Height();
      // Show next active timers
      y += h / 2;
      osd->DrawText(x, y, tr("TIMERS"), Theme.Color(clrMenuItemSelectableFg), clrTransparent /* Theme.Color(clrAltBackground) */,
                    pFontInfoTimerHeadline, w, pFontInfoTimerHeadline->Height(), taCenter);
      y += h + h / 2;
      ReelStatus.UpdateActiveTimers();

      h = pFontInfoTimerText->Height();
      for (tTimer *timer = ReelStatus.mTimers.First(); timer; timer = ReelStatus.mTimers.Next(timer)) {
        if (y + 2 * h + SmallGap + 1 + h / 2 > yMaxHeight)
          break; // don't overwrite warning or help buttons

        std::stringstream info;
        if (timer->isRecording) {
          info << "- " << *TimeString(timer->stopTime) << " / " << timer->channelName.c_str();
        } else {
          info << timer->startDay << ". " << *TimeString(timer->startTime) << " / " << timer->channelName.c_str();
        }
        osd->DrawText(x, y, info.str().c_str(), Theme.Color(clrMenuItemSelectableFg), clrTransparent /* Theme.Color(clrAltBackground)*/, pFontInfoTimerText, w, h);
        y += h;
        osd->DrawRectangle(x, y + SmallGap, xInfoRight - Gap - 1, y + SmallGap + 1, Theme.Color(clrMenuItemSelectableFg));
        y += SmallGap + 1 + h / 2;
      }
    }

#ifdef CLEAR_BUG_WORKAROUND
    osd->DrawRectangle(xInfoRight - 1, (fShowLogo ? yInfoTop : yBodyTop) - SmallGap, xInfoRight - 1, yInfoBottom - 1, Theme.Color(clrMenuItemNotSelectableFg));
#endif
  } else { // !fShowInfo
    osd->DrawRectangle(xInfoLeft, fShowLogo ? yInfoTop : yBodyTop, xInfoRight - 1, yInfoBottom - 1, Theme.Color(clrBackground));
  }
}

cSkinReelDisplayMenu::~cSkinReelDisplayMenu()
{
  debug("%s", __PRETTY_FUNCTION__);

  free(strTheme);
  free(strTitle);
  free(strLastDate);
  delete osd;
}

void cSkinReelDisplayMenu::CreateTitleVector()
{
  debug("%s", __PRETTY_FUNCTION__);
  std::fstream file;
  MenuTypeStruct tempMenuType;
  std::string strBuff;
  int index;
  struct stat fstats;

  /* only reread file if it has changed */
  if(stat(FILENAMEMENULIST, &fstats)==0 && fstats.st_mtime != old_menulist_mtime){
   old_menulist_mtime = fstats.st_mtime;

   file.open(FILENAMEMENULIST, std::ios::in);

   if(file)
   {
    getline(file, strBuff);
    while (!file.eof())
    {
      tempMenuType.Title = "";
      tempMenuType.Type = menuNormal;
      tempMenuType.ImgNumber = false;
      tempMenuType.UpperCase = false;

      if(strBuff.find("#") != 0) //Skip the comments
        if(strBuff.find(";") != std::string::npos)//-1) //Skip the faulty lines (missing ;-seperator)
        {
          index = strBuff.find(";", 0); //find the separator

          tempMenuType.Title.assign(strBuff, 0, index);
          strBuff = strBuff.substr(index+1).c_str();

          index = strBuff.find("#", 0); //find the comment, which will be removed
          if (index != -1)
            strBuff.resize(index); //cutoff the comment

          if (strBuff.find(";imgnum", 0) != std::string::npos)//-1)
            tempMenuType.ImgNumber = true;
          else
            tempMenuType.ImgNumber = false;

          if (strBuff.find(";uppercase", 0) != std::string::npos)//-1)
            tempMenuType.UpperCase = true;
          else
            tempMenuType.UpperCase = false;

          if (strBuff.find("menunormal") == 0)
            tempMenuType.Type = menuMenuNormal;
          else if (strBuff.find("menucentered") == 0)
            tempMenuType.Type = menuMenuCentered;
          else if (strBuff.find("setupnormal") == 0)
            tempMenuType.Type = menuSetupNormal;
          else if (strBuff.find("setupcentered") == 0)
            tempMenuType.Type = menuSetupCentered;
          else if (strBuff.find("help") == 0)
            tempMenuType.Type = menuHelp;
          else if (strBuff.find("bouquets") == 0)
            tempMenuType.Type = menuBouquets;
          else if (strBuff.find("image") == 0)
            tempMenuType.Type = menuImage;
          else if (strBuff.find("music") == 0)
            tempMenuType.Type = menuMusic;
          else if (strBuff.find("video") == 0)
            tempMenuType.Type = menuVideo;
          else if (strBuff.find("multifeed") == 0)
            tempMenuType.Type = menuMultifeed;
          else
            tempMenuType.Type = menuNormal; // every menu without types are shown as normal

          if(tempMenuType.Title.find("*") == tempMenuType.Title.length() - 1)
          {
            tempMenuType.Title.resize(tempMenuType.Title.length() - 1);
            MenuListWildcard.push_back(tempMenuType);
          }
          else
            MenuList.push_back(tempMenuType);
        }

      getline(file, strBuff);
    }
   } else
      debug("Error: Couldn't load the file %s", FILENAMEMENULIST);

   file.close();
  }
}

int cSkinReelDisplayMenu::GetMenuType(std::string Title)
{
  debug("%s", __PRETTY_FUNCTION__);
  //  printf("\033[0;93m MenuList.size() %i \033[0m\n", MenuList.size());
  unsigned int i=0;

  //First try to detect Menutype by checking if Keyword == Title
  while (i < MenuList.size())
  {
    //printf("\033[0;41m %s == %s ? \033[0m\n", trVDR(MenuList.at(i).Title.c_str()), Title.c_str());
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).Type;
    ++i;
  }

  //Not found? Then try to find Keyword (only the ones with wildcard) in Title
  i=0;
  while (i < MenuListWildcard.size())
  {
    //    printf("\033[0;93m %s* == %s ? \033[0m\n", tr(MenuListWildcard.at(i).Title.c_str()), Title.c_str());
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).Type;
    ++i;
  }
  return menuNormal;
}

bool cSkinReelDisplayMenu::UseImageNumber(std::string Title)
{
  debug("%s", __PRETTY_FUNCTION__);
  unsigned int i=0;
  while (i < MenuList.size())
  {
    //    printf("\033[0;41m %s == %s ? => %s \033[0m\n", tr(MenuList.at(i).Title.c_str()), Title.c_str(), MenuList.at(i).ImgNumber?"TRUE":"FALSE");
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).ImgNumber;
    ++i;
  }

  i=0;
  while (i < MenuListWildcard.size())
  {
    //    printf("\033[0;93m %s* == %s ? => %s \033[0m\n", tr(MenuListWildcard.at(i).Title.c_str()), Title.c_str(), MenuListWildcard.at(i).ImgNumber?"TRUE":"FALSE");
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).ImgNumber;
    ++i;
  }

  return false;
}

bool cSkinReelDisplayMenu::IsUpperCase(std::string Title)
{
  unsigned int i=0;
  while (i < MenuList.size())
  {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).UpperCase;
    ++i;
  }

  i=0;
  while (i < MenuListWildcard.size())
  {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).UpperCase;
    ++i;
  }

  return false;
}

void cSkinReelDisplayMenu::SetScrollbar(void)
{
  debug("%s", __PRETTY_FUNCTION__);
  //Must be TE_LOCKed by caller

  // check if scrollbar is needed
  if (textScroller.CanScroll()) {
    int yt = textScroller.Top();
    int yb = yt + textScroller.Height();
    int st = yt + ScrollbarHeight + Gap;
    int sb = yb - ScrollbarHeight - Gap;
    int tt = st + (sb - st) * textScroller.Offset() / textScroller.Total();
    int tb = tt + (sb - st) * textScroller.Shown() / textScroller.Total();
    int xl = textScroller.Width() + xItemLeft;
    // arrow up
    osd->DrawRectangle(xl, yt, xl + ScrollbarWidth, yt + SmallGap,
                       textScroller.CanScrollUp() ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
    osd->DrawRectangle(xl + ScrollbarWidth - SmallGap, yt + SmallGap, xl + ScrollbarWidth, yt + ScrollbarHeight,
                       textScroller.CanScrollUp() ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
    // draw background of scrollbar
    osd->DrawRectangle(xl + ScrollbarWidth - SmallGap, st, xl + ScrollbarWidth, sb, Theme.Color(clrAltBackground));
    // draw visible area of scrollbar
    osd->DrawRectangle(xl + ScrollbarWidth - SmallGap, tt, xl + ScrollbarWidth, tb, Theme.Color(clrMenuTxtFg));
    // arrow down
    osd->DrawRectangle(xl + ScrollbarWidth - SmallGap, yb - ScrollbarHeight, xl + ScrollbarWidth, yb - SmallGap,
                       textScroller.CanScrollDown() ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
    osd->DrawRectangle(xl, yb - SmallGap, xl + ScrollbarWidth, yb,
                       textScroller.CanScrollDown() ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
  }
}

void cSkinReelDisplayMenu::Scroll(bool Up, bool Page)
{
  debug("%s", __PRETTY_FUNCTION__);
  cSkinDisplayMenu::Scroll(Up, Page);
  SetScrollbar();
}

inline int cSkinReelDisplayMenu::MaxItems(void)
{
  debug("%s", __PRETTY_FUNCTION__);
  // max number of items
  int mx = ( yBodyBottom - yItemTop ) / lineHeight ; // to leave a line gap at the end
  debug("cSkinReelDisplayMenu::MaxItems() = %d, lineHeight = %d", mx, lineHeight);
  return  mx;
}

void cSkinReelDisplayMenu::Clear(void)
{
  debug("cSkinReelDisplayMenu::Clear() %d %d %d", isMainMenu, fShowLogo, fShowInfo);

  textScroller.Reset();

  if (strcmp(strTheme, Theme.Name()) != 0) {
    free(strTheme);
    strTheme = strdup(Theme.Name());
    SetupAreas();
  } else {
    // clear items area
    if (fShowLogo || fShowInfo) {
      osd->DrawRectangle(xBodyLeft, yBodyTop, xBodyRight - 1, yBodyBottom - 1, Theme.Color(clrBackground));
      //TODO? clear logo and/or info area?
    } else {
      osd->DrawRectangle(xBodyLeft, yBodyTop, xInfoRight - 1, yInfoBottom - 1, Theme.Color(clrBackground));
    }
  }
}

void cSkinReelDisplayMenu::DrawTitleImage()
{

    // clear the space for drawing
    switch (MenuType)
    {
        case menuMenuNormal:
        case menuMenuCentered:
        case menuSetupNormal:
        case menuSetupCentered:
        case menuHelp:
        case menuBouquets:
        case menuImage:
        case menuMusic:
        case menuVideo:
        case menuMultifeed:
            // clear previous image, since the TitleImages are blended: they get 'brighter' with every overdraw without a clear
            osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyBottom-3, clrTransparent);
            // draw background
            osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyBottom-3,  Theme.Color(clrBackground) );
            withTitleImage = true;
            break;
        default:
            withTitleImage = false;
            break;
    }

    // draw Title Image
    switch(MenuType)
    {
        case menuMenuNormal:
        case menuMenuCentered:
            osd->DrawImage(imgMenuTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgMenuTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgMenuTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuSetupNormal:
        case menuSetupCentered:
            osd->DrawImage(imgSetupTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgSetupTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgSetupTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;
            break;
        case menuHelp:
            osd->DrawImage(imgHelpTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgHelpTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgHelpTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        case menuBouquets:
            osd->DrawImage(imgBouquetsTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgBouquetsTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgBouquetsTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        case menuImage:
            osd->DrawImage(imgImagesTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgImagesTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgImagesTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        case menuMusic:
            osd->DrawImage(imgMusicTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgMusicTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgMusicTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        case menuVideo:
            osd->DrawImage(imgVideoTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgVideoTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgVideoTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        case menuMultifeed:
            osd->DrawImage(imgMultifeedTitleBottom, 0, yBodyBottom - IMGMENUHEIGHT/2, true);
            osd->DrawImage(imgMultifeedTitleMiddle, 0, yBodyBottom - IMGMENUHEIGHT, true);
            osd->DrawImage(imgMultifeedTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT); // repeat this image vertically
            break;
        default:
            xItemLeft = xBodyLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
    }
}

void cSkinReelDisplayMenu::SetTitle(const char *Title)
{
  //printf("cSkinReelDisplayMenu::SetTitle(%s)\n", Title);

  bool fTitleChanged = false;
  if (Title && strTitle) {
    if (strcmp(Title, strTitle) != 0) {
      fTitleChanged = true;
      free(strTitle);
      strTitle = strdup(Title);
    }
  } else {
    free(strTitle);
    if (Title)
      strTitle = strdup(Title);
    else
      strTitle = NULL;
  }

  static char strTitlePrefix[256];
  snprintf(strTitlePrefix, 256, "%s  -  ", trVDR("VDR"));

  if ((Title == NULL) || (isMainMenu && strncmp(strTitlePrefix, Title, strlen(strTitlePrefix)) == 0)) {
      DrawTitle(Title);
  } else {
    bool old_isMainMenu = isMainMenu;
    bool old_fShowLogo = fShowLogo;
    bool old_fShowInfo = fShowInfo;

    if (strTitle == NULL || strncmp(strTitlePrefix, strTitle, strlen(strTitlePrefix)) == 0) {
      isMainMenu = true;
      fShowLogo = fShowLogoDefault ? ReelConfig.showSymbolsMenu : false;
      fShowInfo = ReelConfig.showInfo;
    } else {
      isMainMenu = false;
      fShowLogo = false;
      fShowInfo = false;
    }

    if (!fSetupAreasDone || old_isMainMenu != isMainMenu || old_fShowLogo != fShowLogo || old_fShowInfo != fShowInfo) {
      SetupAreas();
    } else {
      DrawTitle(Title);
    }
  }

  isNumberImage = UseImageNumber(strTitle);
  MenuType = GetMenuType(strTitle);

#define HIDDENSTRING "menunormalhidden$"
  if (Title) {
     char* pos = (char *) strstr(Title, HIDDENSTRING);
     if (pos>0) {
        MenuType = menuMenuNormal;
        DrawTitle(Title+strlen(HIDDENSTRING));
     }
  }

  isCAPS = IsUpperCase(strTitle);

  pFontList = ReelConfig.GetFont(FONT_LISTITEM, pFontList);

  if (isNumberImage)
      lineHeight = std::max(pFontList->Height() , 35); //change
  else
      lineHeight = pFontList->Height() ; //change

  // Draw menu/setup/help according to menutype
    DrawTitleImage();

  if (MenuType == menuMenuNormal || MenuType == menuSetupNormal ||
      MenuType == menuHelp || MenuType == menuBouquets ||
      MenuType == menuImage || MenuType == menuMusic ||
      MenuType == menuVideo || MenuType == menuMultifeed)
          xItemLeft = xBodyLeft + IMGMENUWIDTH + (ReelConfig.showMarker ? lineHeight : ListHBorder);

  if((MenuType == menuMenuCentered) || (MenuType == menuSetupCentered))
    yItemTop = int(yBodyTop + lineHeight /*TB: what's this?!*/ /*+ 0*(yBodyBottom - yBodyTop) * 1.0/6*/); // The items is centered in the menu
  else
    yItemTop = yBodyTop + 15; // lineHeight ; // leave a line blank

  free(strLastDate);
  strLastDate = NULL;
}

///< absolute path of the thumbnail is expected
///< draws the thumbnail only if the side-Images are present: like menu/setup/Help etc.
bool cSkinReelDisplayMenu::DrawThumbnail(const char* thumbnail_filename)
{
    // if no Title image then do not draw thumbnail
    if ( !withTitleImage )
        return false;

    // if filename == NULL, clear thumbnail by drawning the Title-Image again
    if (thumbnail_filename == NULL) {
        DrawTitleImage();
        return true;
    }

    // if file not readable
    if ( access(thumbnail_filename, R_OK) != 0 ) {
        esyslog("Could not access thumbnail file '%s'", thumbnail_filename);
        std::cout << __PRETTY_FUNCTION__ << " could NOT read " << thumbnail_filename << std::endl;
        return false;
    }

    int leftTop_x           = 43;
    int top_y               = yBodyTop      + 20 ;

    // Draw thumbnail
    std::cout << __PRETTY_FUNCTION__ << " DRAWING " << thumbnail_filename << std::endl;
    osd->SetImagePath(imgUnbufferedEnum, thumbnail_filename);
    osd->DrawImage(imgUnbufferedEnum, leftTop_x, top_y, true, 1, 1);

    return true;
}

void cSkinReelDisplayMenu::DrawTitle(const char *Title)
{
   //printf("%s: %s\n", __PRETTY_FUNCTION__, Title);
  //Must be TE_LOCKed by caller

  // draw titlebar
  osd->DrawImage(imgMenuHeaderLeft, 0, 0, false);
  osd->DrawImage(imgMenuHeaderCenterX, Roundness, 0, false, xDateRight - 2*Roundness, 1);
  osd->DrawImage(imgMenuHeaderRight, xDateRight - Roundness, 0, false);

  if (Title) {
    pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE, pFontOsdTitle); //TODO? get current font which might have been patched meanwhile
    int y = yTitleTop + (yTitleBottom - yTitleTop - /* TB 22*/ pFontOsdTitle->Height()) / 2;
    int pg_start, pg_end, pg_count, pg_total;
    pg_total = pgbar(Title, pg_start, pg_end, pg_count);

    // no progress bar found in Title
    if (pg_total == 0) {
       osd->DrawText(xTitleLeft + Roundness, y, Title,
                  Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                  xTitleRight - xTitleLeft - Roundness - 3, yTitleBottom - y);
       return;
    } else {
       // progress bar found in Title
       static char title_array[512];
       const int pg_length = 50;
       const int pg_thickness = 6;
       strncpy(title_array, Title, pg_start);
       title_array[pg_start] = '\0';

       // first part of Title
       int x_coord = xTitleLeft + Roundness;
       osd->DrawText(x_coord, y, title_array, Theme.Color(clrTitleFg), clrTransparent,
                  pFontOsdTitle, xDateLeft -Roundness - x_coord , yTitleBottom - y);

       x_coord += pFontOsdTitle->Width(title_array);
       x_coord += 2*Gap;

       int py0; /*top y*/
       int py1; /*bottom y*/

       py0 = y + ( yTitleBottom - y ) / 2 - pg_thickness;
       py1 = py0 + pg_thickness;

       // Draw Progress bar
       osd->DrawRectangle(x_coord, py0 , x_coord + pg_length, py1, Theme.Color(clrProgressBarBg));
       osd->DrawRectangle(x_coord, py0 , x_coord + (int)( pg_length* pg_count*1.0/pg_total ), py1, Theme.Color(clrProgressBarFg));

       x_coord += pg_length + 2*Gap;

       strncpy(title_array, Title+pg_end+1, strlen(Title) - pg_end);
       title_array[strlen(Title) - pg_end] = 0;

       // second part of the title
       osd->DrawText(x_coord, y, title_array, Theme.Color(clrTitleFg), clrTransparent,
                  pFontOsdTitle, xDateLeft - Roundness- x_coord , yTitleBottom - y);
    } // end if_else (pg_total==0)
  }// end if (Title)
}

void cSkinReelDisplayMenu::DrawFooter()
{
  debug("%s", __PRETTY_FUNCTION__);
  // draw footer
  osd->DrawImage(imgMenuFooterLeft, 0, yFooterTop, false);
  osd->DrawImage(imgMenuFooterCenterX, Roundness, yFooterTop, false, xFooterRight - 2*Roundness, 1);
  osd->DrawImage(imgMenuFooterRight, xFooterRight - Roundness, yFooterTop, false);
}

void cSkinReelDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  debug("cSkinReelDisplayMenu::SetButtons(%s, %s, %s, %s)", Red, Green, Yellow, Blue);

  const cFont *pFontHelpKeys = ReelConfig.GetFont(FONT_HELPKEYS);
  int w = (xButtonsRight - xButtonsLeft) / 4;
  int t3 = xButtonsLeft + xButtonsRight - xButtonsLeft - w;
  int t2 = t3 - w;
  int t1 = t2 - w;
  int t0 = xButtonsLeft;

  // draw Footer
  DrawFooter();

  // draw color buttons
  if (Red) {
    osd->DrawImage(imgButtonRedX, t0, yButtonsTop, true, w, 1);
    osd->DrawText(t0, yButtonsTop, Red, Theme.Color(clrButtonRedFg),
                  0, pFontHelpKeys, t1 - t0 + 1, yButtonsBottom - yButtonsTop, taCenter);
  }
  if (Green) {
    osd->DrawImage(imgButtonGreenX, t1, yButtonsTop, true, w, 1);
    osd->DrawText(t1, yButtonsTop, Green, Theme.Color(clrButtonGreenFg),
                  0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  }
  if (Yellow) {
    osd->DrawImage(imgButtonYellowX, t2, yButtonsTop, true, w, 1);
    osd->DrawText(t2, yButtonsTop, Yellow, Theme.Color(clrButtonYellowFg),
                  0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  }
  if (Blue) {
    osd->DrawImage(imgButtonBlueX, t3, yButtonsTop, true, w, 1);
    osd->DrawText(t3, yButtonsTop, Blue, Theme.Color(clrButtonBlueFg),
                  0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  }
}

void cSkinReelDisplayMenu::SetMessage(eMessageType Type, const char *Text)
{
  debug("%si %s", __PRETTY_FUNCTION__, Text);

  // check if message
  if (Text) {
    if (strcmp(Text, "(?)") == 0)
    {
      osd->DrawImage(imgIconHelp, xDateRight - 30 - Gap, yBodyBottom - 30 - Gap, true);
      osd->DrawRectangle(xMessageLeft, yMessageTop, xMessageRight-1, yMessageBottom, Theme.Color(clrBackground));
    } else {
      // Clean Messagearea & Remove (?)-Icon
      osd->DrawRectangle(xMessageLeft, yMessageTop, xMessageRight-1, yMessageBottom, Theme.Color(clrBackground));
      osd->DrawRectangle(xDateRight - 30 - Gap, yBodyBottom - 30 - Gap, xDateRight - Gap, yBodyBottom - Gap , Theme.Color(clrBackground));
      // save osd region
      //if (nMessagesShown == 0)
      //  osd->SaveRegion(xMessageLeft, yMessageTop, xMessageRight, yMessageBottom); // cannot save regions

       if (Text)
       {
	        // draw message
	        int imgMessageIcon;
	        tColor  colorfg = 0xFFFFFFFF;
	        switch(Type)
	        {
	          case mtStatus:
	            imgMessageIcon = imgIconMessageInfo;
	            break;
	          case mtWarning:
	            imgMessageIcon = imgIconMessageWarning;
	            break;
	          case mtError:
	            imgMessageIcon = imgIconMessageError;
	            break;
	          default:
	            imgMessageIcon = imgIconMessageNeutral;
	            break;
	        } //end switch

	        const cFont *pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);
	        int MessageWidth = pFontMessage->Width(Text);
	        osd->DrawImage(imgMessageIcon, xMessageLeft + (xMessageRight-xMessageLeft - MessageWidth)/2 - 30 - 2*Gap, yMessageTop - Gap, true, 1, 1);
		int _xMessageStart_ = (xMessageRight-xMessageLeft - MessageWidth)/2;


		int pb_start, pb_end, progressCount; // Progress Bar
		int offset=0;
		int pbtotal = pgbar(Text, pb_start, pb_end, progressCount); // get the length of progress bar if any
	//        printf("Text \"%s\", %d Message width \n", Text,  MessageWidth);
                if (pbtotal < 3) // donot show pbar! [], [ ], [  ] will be displayed as such
	            {
		     osd->DrawText( xMessageLeft , yMessageTop, Text,
	                           colorfg, clrTransparent, pFontMessage,
	                          xMessageRight - xMessageLeft, 0, taCenter );
		    } // end if
		    else
		    {
                      char *text1;
		      if (pb_start>0) // some text before progress bar
		      {
		        text1 = new char[pb_start+1];
		        strncpy(text1,Text, pb_start); // i_start is the index of '['
			text1[pb_start] = '\0';

		        offset = pFontMessage->Width(text1);
			osd->DrawText( _xMessageStart_ , yMessageTop, text1,
	                           colorfg, clrTransparent, pFontMessage,
	                          offset, 0, taCenter );
		        delete text1;
		      }

		      // draw progress bar
		      int pblength = pbtotal * pFontMessage->Width("|");
		      int pbthickness = (int)( 0.5*pFontMessage->Height("|") );

		      int pbTop = (yMessageBottom + yMessageTop - pbthickness)/2;
		      // background
		      osd->DrawRectangle( _xMessageStart_ + offset, pbTop, _xMessageStart_ + offset + pblength , pbTop + pbthickness, Theme.Color(clrProgressBarBg)  );
		      // foreground
		      osd->DrawRectangle( _xMessageStart_ + offset, pbTop, _xMessageStart_ + offset
		                                          + (int) ( (progressCount*1.0/pbtotal) * pblength ),
		                          pbTop + pbthickness, Theme.Color(clrProgressBarFg));

		     offset += pblength +  pFontMessage->Width(' ');

		     if (pb_end + 1 < (int)strlen(Text))// display rest of the text
		     {
		       int len = strlen(Text) - pb_end ; // including '\0'
		       text1 = new char[len];
		       strcpy(text1, Text+pb_end+1);
		       text1[len] = '\0';

		       osd->DrawText( _xMessageStart_ + offset , yMessageTop, text1,
	                              colorfg, clrTransparent, pFontMessage, len , 0, taCenter );

		      delete text1;

		     }

		    } // end else

        } // end if(Text)
      nMessagesShown = 1; //was: nMessagesShown++;
    }
  } else {
    if (nMessagesShown > 0)
      --nMessagesShown;
    // restore saved osd region
    if (nMessagesShown == 0)
    {
     // osd->RestoreRegion();
      osd->DrawRectangle(xMessageLeft, yMessageTop, xMessageRight-1, yMessageBottom, Theme.Color(clrBackground));
      osd->DrawRectangle(xDateRight - 30 - Gap, yBodyBottom - 30 - Gap, xDateRight - Gap, yBodyBottom - Gap , Theme.Color(clrBackground));
    }
  }
}

inline bool cSkinReelDisplayMenu::HasTabbedText(const char *s, int Tab)
{
  debug("%s", __PRETTY_FUNCTION__);
  if (!s)
    return false;

  const char *b = strchrnul(s, '\t');
  while (*b && Tab-- > 0) {
    b = strchrnul(b + 1, '\t');
  }
  if (!*b)
    return (Tab <= 0) ? true : false;
  return true;
}

void cSkinReelDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable)
{
    debug("cSkinReelDisplayMenu::SetItem(%s, %d, %d, %d) MenuType = %d", Text, Index, Current, Selectable, MenuType);
    debug("SetTab widths need to be checked, %d", __LINE__);

    if (Text == NULL)
      return;

    tColor ColorFg, ColorBg;
    // select colors
    ColorBg = Theme.Color(clrBackground);
    if (Current) {
        ColorFg = Theme.Color(clrMenuItemCurrentFg);
        //ColorBg =  Theme.Color(clrMenuItemCurrentBg);
    } else {
        if (Selectable) {
            ColorFg = Theme.Color(clrMenuItemSelectableFg);
            //ColorBg = Theme.Color(clrBackground);
        } else {
            ColorFg = Theme.Color(clrMenuItemNotSelectableFg);
            //ColorBg = Theme.Color(clrBackground);
        }
    }

  char *Text_tmp = strdup (Text);

  int y = yItemTop + Index * lineHeight;

  /** Clear the line **/ // to prevent overwriting
  osd->DrawRectangle(xItemLeft,y, xItemRight,y +lineHeight  , /*Index %2 ? clrRed:clrBlue*/ ColorBg);

  bool isNumImg = false;
  // Display numbers as images
  int tmp;
  if ( isNumberImage && sscanf(Text,"%d%[^\n]", &tmp, Text_tmp)>0 && Text_tmp[0]==' ')
  /* space needed after number
   1 Media
   2 Setup
   1Media : will be displayed as such */
  {
      if (tmp < 10)
        {
            debug("Got number = %d\n",tmp);
            int imgNumber = imgNumber00H + 2*tmp;
            if (!Current) imgNumber ++;

            // Draw number
            /** Draw the number in the middle of the line, (Hack : and then a little lower, +3) */
            //osd->DrawImage (imgNumber, xItemLeft,y + (lineHeight - IMGNUMBERHEIGHT)/2  + "added number here for aligning the text and number images" , true );
            osd->DrawImage (imgNumber, xItemLeft,y + (lineHeight - IMGNUMBERHEIGHT)/2 -2  , true );   // -2 for font "lm sans20" size:=20
            isNumImg = true; // need to adjust the x-coord of the Text
        }
      else  // got number more than 9!
        {
            debug("Got number %d: cannot display it as images are available only till 9", tmp);
            //y = yItemTop + Index * lineHeight; // Image height
            isNumImg = false; // need to adjust the x-coord of the Text
            free(Text_tmp);
            Text_tmp = strdup (Text);
        }
  } else {
      debug("Displaying text numbers only");
      free(Text_tmp);
      Text_tmp = strdup (Text);
  }

  /**
   Set 'y' : such that any text drawn is in the middle of the lineHeightTitle
  */
  y = yItemTop + Index * lineHeight + (lineHeight - /* TB 22 */ pFontList->Height() ) /2; // Draw Texts in the middle of the line

  //change to CAPS if necessary: for MainMenu or Main Setup page
  if(isCAPS)
  {
    // convert Text to all caps
    for (int i = 0; Text_tmp[i]!='\0'; i++)
      Text_tmp[i] = toupper(Text_tmp[i]);
  }

  if (nMessagesShown > 0 && y >= yMessageTop)
    return; //Don't draw above messages

  if (Current)
    pFontList = ReelConfig.GetFont(FONT_LISTITEMCURRENT); //TODO? get current font which might have been patched meanwhile
  else
    pFontList = ReelConfig.GetFont(FONT_LISTITEM); //TODO? get current font which might have been patched meanwhile

  // draw item
  static char s[512];
  //isyslog("Text= \"%s\"  ==> Text_tmp= \"%s\" ", Text, Text_tmp);
  //for (int i = 0; i < MaxTabs; i++) {  isyslog(" Tab(%d)= %d ",i, Tab(i)); }
  for (int i = 0; i < MaxTabs; i++) {
    const char *s1 = GetTabbedText(Text_tmp, i);
    if (s1)
      strncpy(s,s1,512);
    else
      s[0]='\0';

    //printf("\033[0;93m\"%s\"\033[0m\n", s1);

    debug("Tab(%d) = %d", i, Tab(i));

    if (s[0]) {
      static char buffer[10];
      int xt = xItemLeft + Tab(i) + (isNumImg ? 35:0); // add 35px if an image of number was drawn

      debug("xItemLeft = %d, xt = %d", xItemLeft, xt);

      bool iseventinfo = false;
      bool isnewrecording = false;
      bool isarchived = false;
      bool isprogressbar = false;
      bool isHD = false;
      bool isFavouriteFolder = false;
      bool isFolder = false;
      bool isCheckmark = false;
      bool isRunningnow = false;
      bool isSharedDir = false;
      bool isMusic = false;
      bool isFolderUp = false;
      bool isArchive = false;
      bool isVdrRec = false; // char 140
      bool isVideo = false; // char 141
      int singlechar = eNone;
      int now = 0, total = 0;
      int w;

      if (strlen(s) == 1)
        switch ((int) s[0])
        {
          case 80:  singlechar = ePinProtect;        break;
          case 129: singlechar = eArchive;           break;
          case 128: singlechar = eBack;              break;
          case 130: singlechar = eFolder;            break;
          case 132: singlechar = eNewrecording;      break;
          case 133: singlechar = eCut;               break;
          case '#': singlechar = eRecord;            break;
          case '>': singlechar = eTimeractive;       break;
          case '!': singlechar = eSkipnextrecording; break;
          case 136: singlechar = eRunningNow;        break;
          case 137: singlechar = eNetwork;           break;
          case 138: singlechar = eDirShared;         break;
          case 139: singlechar = eMusic;             break;
          default:                                   break;
        }

      if (strlen(s) == 2 && s[1] == (char)133) {
        if (s[0]==(char)132)
          singlechar = eNewCut;
        else if (s[0]==(char)129)
          singlechar = eArchivedCut;
      // check if event info symbol: "StTV*" "R"
      // check if event info characters
      } else if (strlen(s) == 3 && strchr(" StTR", s[0])
          && strchr(" V", s[1]) && strchr(" *", s[2])) {
        // update status
        iseventinfo = true;
      // check if event info symbol: "RTV*" strlen=4
      } else if (strlen(s) == 4 && strchr(" tTR", s[1])
          && strchr(" V", s[2]) && strchr(" *", s[3])) {
        iseventinfo = true;
      // check if new recording: "01.01.06*", "10:10*"
      } else if ((strlen(s) == 6 && s[5] == '*' && s[2] == ':' && isdigit(*s)
           && isdigit(*(s + 1)) && isdigit(*(s + 3)) && isdigit(*(s + 4)))
          || (strlen(s) == 9 && s[8] == '*' && s[5] == '.' && s[2] == '.'
              && isdigit(*s) && isdigit(*(s + 1)) && isdigit(*(s + 3))
              && isdigit(*(s + 4)) && isdigit(*(s + 6))
              && isdigit(*(s + 7)))) {
        // update status
        isnewrecording = true;
        // make a copy
        strncpy(buffer, s, strlen(s));
        // remove the '*' character
        buffer[strlen(s) - 1] = '\0';
      // check if progress bar: "[|||||||   ]"
      } else if (!isarchived && ReelConfig.showProgressbar &&
          (strlen(s) > 5 && s[0] == '[' && s[strlen(s) - 1] == ']')) {
        const char *p = s + 1;
        // update status
        isprogressbar = true;
        for (; *p != ']'; ++p) {
          // check if progressbar characters
          if (*p == ' ' || *p == '|') {
            // update counters
            ++total;
            if (*p == '|')
              ++now;
          } else {
            // wrong character detected; not a progressbar
            isprogressbar = false;
            break;
          }
        }
      }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3 && s[1] == ' ') {
        switch(s[0]) {
          case (char)134: isHD              = true; s[0]=' '; break;
          case (char)130: isFolder          = true; s[0]=' '; break;
          case (char)131: isFavouriteFolder = true; s[0]=' '; break;
          case (char)135: isCheckmark       = true; s[0]=' '; break;
          case (char)136: isRunningnow      = true; s[0]=' '; break;
          case (char)138: isSharedDir       = true; s[0]=' '; break;
          case (char)139: isMusic           = true; s[0]=' '; break;
          case (char)128: isFolderUp        = true; s[0]=' '; break;
          case (char)129: isArchive         = true; s[0]=' '; break;
          case (char)140: isVdrRec          = true; s[0]=' '; break;
          case (char)141: isVideo           = true; s[0]=' '; break;
          default: break;
        }
      }
      if (iseventinfo) {
        int evx = xt + Gap;
        const char *p = s;
        // draw background
        // osd->DrawRectangle(xt, y, xItemRight, y + lineHeight - 1, ColorBg);
        // draw symbols
        for (; *p; ++p) {
          switch (*p) {
            case 't':
              // partial timer event
              osd->DrawImage(imgIconTimerpart, evx, y + 1, true);
              evx += IMGICONWIDTH;
              break;
            case 'T':
              // timer event
              osd->DrawImage(imgIconTimer, evx, y + 1, true);
              evx += IMGICONWIDTH;
              break;
            case 'R':
              // recording event (epgsearch)
              osd->DrawImage(imgIconRecord, evx, y + 1, true);
              evx += IMGICONWIDTH;
              break;
            case 'V':
              // vps event
              osd->DrawImage(imgIconVps, evx, y + 1, true);
              evx += IMGICONWIDTH;
              break;
            case 'S':
              osd->DrawImage(imgIconZappingtimer, evx, y + 1, true);
              evx += IMGICONWIDTH;
              break;
            case '*':
              // running event
              // osd->DrawImage(imgIconRunningnow, evx, y + 1, true);
              // evx += IMGICONWIDTH;
              break;
            case ' ':
            default:
              // let's ignore space character
              break;
          }
        }
      } else if (isnewrecording || isarchived) {
        // draw text
        osd->DrawText(xt, y, buffer, ColorFg, clrTransparent /*ColorBg*/, pFontList, xItemRight - xt);
        // draw symbol
        if (isarchived)
        {
          osd->DrawImage(imgIconArchive, xt + pFontList->Width(buffer), y + 1, true);
          if (isnewrecording)
            osd->DrawImage(imgIconNewrecording, xt + pFontList->Width(buffer) + IMGICONWIDTH, y + 1, true);
        } else
          osd->DrawImage(imgIconNewrecording, xt + pFontList->Width(buffer), y + 1, true);
      } else if (isprogressbar) {
        // define x coordinates of progressbar
        int px0 = xt;
        //int px1 = (Selectable ? (Tab(i + 1) ? Tab(i + 1) : xItemRight - Roundness) : xItemRight-Roundness) ;
        int px1 = px0 ; //= (Tab(i + 1) ? Tab(i + 1) : xItemRight - Roundness)  ;
	// a better calculation of progressbar length
	if( Tab(i+1) > 0 )
            px1 = px0 + Tab(i+1) - Tab(i) - (ReelConfig.showMarker ? lineHeight : ListHBorder) ;
        else
            // no more tabs: progress bar goes till the end
            px1 = xItemRight - Roundness;

        // when the string complete string is a progressbar, ignore tabs
	if (strlen(s) == strlen(Text_tmp)) px1 = xItemRight - Roundness;

#ifdef SLOW_FP_MATH
        int px = px0 + std::max((int)((float) now * (float) (px1 - px0) / (float) total), ListProgressBarBorder);
#else
        int px = px0 + std::max((now * (px1 - px0) / total), ListProgressBarBorder);
#endif
        // define y coordinates of progressbar
        int py0 = y + (lineHeight - PROGRESSBARHEIGHT) / 2;
        int py1 = py0 + PROGRESSBARHEIGHT;
        // draw background
        // osd->DrawRectangle(px0, y, /*TODO? px1 */ xItemRight - 1, y + lineHeight - 1, ColorBg);
        // draw progressbar
        osd->DrawRectangle(px0, py0, px1, py1, Theme.Color(clrProgressBarBg));
        osd->DrawRectangle(px0, py0  , px - 1, py1 , Theme.Color(clrProgressBarFg));
	//debug("ProgressBar: %d-%d/%d (%d/%d): %f ?= %f",px0,px,px1, now,total, (px - (float)px0)/(float)px1,  (float)now/(float)total) ;
      } else if (singlechar != eNone) {
        switch (singlechar)
        {
          case ePinProtect:
            osd->DrawImage(imgIconPinProtect, xt, y + 1, true);
            break;
          case eArchive:
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            break;
          case eBack:
            osd->DrawImage(imgIconFolderUp, xt, y + 1, true);
	    xt += 20;
            w = (Tab(i + 1) && HasTabbedText(Text_tmp, i + 1) ? (xItemLeft + Tab(i + 1)) : xItemRight) - xt;
            osd->DrawText(xt, y, "  ..", ColorFg, /*ColorBg*/ clrTransparent, pFontList, w, nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );
            break;
          case eFolder:
            osd->DrawImage(imgIconFolder, xt, y + 1, true);
            break;
          case eNewrecording:
            osd->DrawImage(imgIconNewrecording, xt, y + 1, true);
            break;
          case eCut:
            osd->DrawImage(imgIconCut, xt, y + 1, true);
            break;
          case eNewCut:
            osd->DrawImage(imgIconNewrecording, xt, y + 1, true);
            osd->DrawImage(imgIconCut, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eArchivedCut:
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            osd->DrawImage(imgIconCut, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eRecord:
            osd->DrawImage(imgIconRecord, xt, y + 1, true);
            break;
          case eTimeractive:
            osd->DrawImage(imgIconTimeractive, xt, y + 1, true);
            break;
          case eSkipnextrecording:
            osd->DrawImage(imgIconSkipnextrecording, xt, y + 1, true);
            break;
          case eNetwork:
            DrawUnbufferedImage(osd, "icon_network_20.png", 0, xt, y+1, true);
            break;
          case eDirShared:
            DrawUnbufferedImage(osd, "icon_foldershare_20.png", 0, xt, y+1, true);
            break;
          case eMusic:
	    DrawUnbufferedImage(osd, "icon_music_20.png", 0, xt, y+1, true);
	    break;
          case eRunningNow:
            osd->DrawImage(imgIconRunningnow, xt, y + 1, true);
            break;
          default:
            break;
        }
      } else {
        if (isHD) {
          osd->DrawImage(imgIconHd, xt, y + 1, true);
          xt += 20;
        } else if (isFavouriteFolder) {
          osd->DrawImage(imgIconFavouriteFolder, xt, y + 1, true);
          xt += 20;
        } else if (isFolder) {
          osd->DrawImage(imgIconFolder, xt, y + 1, true);
          xt += 20;
        } else if (isCheckmark) {
          osd->DrawImage(imgIconTimeractive, xt, y + 1, true);
          xt += 20;
        } else if (isRunningnow) {
          osd->DrawImage(imgIconRunningnow, xt, y + 1, true);
          xt += 20;
        } else if (isSharedDir) {
          DrawUnbufferedImage(osd, "icon_foldershare_20.png", 0, xt, y+1, true);
          xt += 20;
        } else if (isMusic) {
          DrawUnbufferedImage(osd, "icon_music_20.png", 0, xt, y+1, true);
          xt += 20;
        } else if (isFolderUp) {
            osd->DrawImage(imgIconFolderUp, xt, y + 1, true);
            xt += 20;
        } else if (isArchive) {
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            xt += 20;
        } else if (isVdrRec) {
            DrawUnbufferedImage(osd, "icon_vdr_rec_20.png", 0, xt, y+1, true);
            xt += 20;
        } else if (isVideo) {
            DrawUnbufferedImage(osd, "icon_video_20.png", 0, xt, y+1, true);
            xt += 20;
        }

        w = (Tab(i + 1) && HasTabbedText(Text_tmp, i + 1) ? (xItemLeft + Tab(i + 1)) : xItemRight) - xt;
        //TODO? int w = xItemRight - xt;
        // draw text
        if (Current) {
            osd->DrawText(xt, y, s, ColorFg, /*ColorBg*/ clrTransparent, pFontList, w, nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );
            debug("%s Height:= %d width:= %d space:= %d", __PRETTY_FUNCTION__, pFontList->Height(),pFontList->Width(s), w);
        } else
          osd->DrawText(xt, y, s, ColorFg, /*ColorBg*/clrTransparent, pFontList, w, nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );
      }
    }
   // if (!Tab(i + 1)) // for undelete recordings
   //   break;
  }
  //set editable width
  SetEditableWidth(xItemRight - Tab(1) - xItemLeft);

#ifndef SKINREEL_NO_MENULOGO
  if (Current && isMainMenu && fShowLogo && Text_tmp) {
    char *ItemText, *ItemText2;
    int n = strtoul(Text_tmp, &ItemText, 10);
    if (n != 0)
      ItemText2 = ItemText = skipspace(ItemText);
    else
      ItemText2 = skipspace(ItemText);

    bool fFoundLogo = false;
    if (strcmp(ItemText, trVDR("Schedule")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/schedule");
    else if (strcmp(ItemText, trVDR("Channels")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/channels");
    else if (strcmp(ItemText, trVDR("Timers")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("timerinfo")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/timers");
    else if (strcmp(ItemText, trVDR("Recordings")) == 0
             || strcmp(ItemText, trVDR("Recording info")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("extrecmenu")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/recordings");
    else if (strcmp(ItemText, trVDR("Setup")) == 0
             || strcmp(ItemText2, trVDR("Setup")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/setup");
    else if (strcmp(ItemText, trVDR("Commands")) == 0
             || strcmp(ItemText2, trVDR("Commands")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/commands");
    else if (strcmp(ItemText, trVDR(" Stop replaying")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/stop");
    else if (strcmp(ItemText, trVDR(" Cancel editing")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/cancel");
    else if (strcmp(ItemText2, GetPluginMainMenuName("audiorecorder")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/audiorecorder");
    else if (strcmp(ItemText2, GetPluginMainMenuName("burn")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/burn");
    else if (strcmp(ItemText2, GetPluginMainMenuName("cdda")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/cdda");
    else if (strcmp(ItemText2, GetPluginMainMenuName("chanorg")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/chanorg");
    else if (strcmp(ItemText2, GetPluginMainMenuName("channelscan")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/channelscan");
    else if (strcmp(ItemText2, GetPluginMainMenuName("digicam")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/digicam");
    else if (strcmp(ItemText2, GetPluginMainMenuName("director")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/director");
    else if (strcmp(ItemText2, GetPluginMainMenuName("dvd")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/dvd");
    else if (strcmp(ItemText2, GetPluginMainMenuName("dvdselect")) == 0
           || strcmp(ItemText2, GetPluginMainMenuName("dvdswitch")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/dvdselect");
    else if (strcmp(ItemText2, GetPluginMainMenuName("dxr3")) == 0
           || strcmp(ItemText2, GetPluginMainMenuName("softdevice")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/device");
    else if (strcmp(ItemText2, GetPluginMainMenuName("epgsearch")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("epgsearchonly")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("conflictcheckonly")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("nordlichtsepg")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("quickepgsearch")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/epgsearch");
    else if (strcmp(ItemText2, GetPluginMainMenuName("externalplayer")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/externalplayer");
    else if (strcmp(ItemText2, GetPluginMainMenuName("femon")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/femon");
    else if (strcmp(ItemText2, GetPluginMainMenuName("filebrowser")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/filebrowser");
    else if (strcmp(ItemText2, GetPluginMainMenuName("fussball")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("sport")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/fussball");
    else if (strcmp(ItemText2, GetPluginMainMenuName("games")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/games");
    else if (strcmp(ItemText2, GetPluginMainMenuName("image")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("osdimage")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/image");
    else if (strcmp(ItemText2, GetPluginMainMenuName("mp3")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("mp3ng")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("muggle")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("music")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/mp3");
    else if (strcmp(ItemText2, GetPluginMainMenuName("mplayer")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/mplayer");
    else if (strcmp(ItemText2, GetPluginMainMenuName("newsticker")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/newsticker");
    else if (strcmp(ItemText2, GetPluginMainMenuName("osdpip")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/osdpip");
    else if (strcmp(ItemText2, GetPluginMainMenuName("pin")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/pin");
    else if (strcmp(ItemText2, GetPluginMainMenuName("radio")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/radio");
    else if (strcmp(ItemText2, GetPluginMainMenuName("rotor")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/rotor");
    else if (strcmp(ItemText2, GetPluginMainMenuName("solitaire")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/solitaire");
    else if (strcmp(ItemText2, GetPluginMainMenuName("streamdev-client")) == 0
           || strcmp(ItemText2, GetPluginMainMenuName("streamdev-server")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/streamdev");
    else if (strcmp(ItemText2, GetPluginMainMenuName("sudoku")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/sudoku");
    else if (strcmp(ItemText2, GetPluginMainMenuName("teletext")) == 0
           || strcmp(ItemText2, GetPluginMainMenuName("osdteletext")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/teletext");
    else if (strcmp(ItemText2, GetPluginMainMenuName("tvonscreen")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/tvonscreen");
    else if (strcmp(ItemText2, GetPluginMainMenuName("vcd")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/vcd");
    else if (strcmp(ItemText2, GetPluginMainMenuName("vdrc")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/vdrc");
    else if (strcmp(ItemText2, GetPluginMainMenuName("vdrcd")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("mediad")) == 0
             || strcmp(ItemText2, GetPluginMainMenuName("mediamanager")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/vdrcd");
    else if (strcmp(ItemText2, GetPluginMainMenuName("vdrrip")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/vdrrip");
    else if (strcmp(ItemText2, GetPluginMainMenuName("weather")) == 0
           || strcmp(ItemText2, GetPluginMainMenuName("weatherng")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/weather");
    else if (strcmp(ItemText2, GetPluginMainMenuName("webepg")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/webepg");
    else if (strcmp(ItemText2, GetPluginMainMenuName("xineliboutput")) == 0)
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/xineliboutput");
    else
      fFoundLogo = ReelLogoCache.LoadIcon("icons/menu/vdr");

    osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoTop - 1, Theme.Color(clrLogoBg));
  }
#endif

  free(Text_tmp);

  if (Current)
      pFontList = ReelConfig.GetFont(FONT_LISTITEM);  // pFontList is used elsewhere

}

const char *cSkinReelDisplayMenu::GetPluginMainMenuName(const char *plugin)
{
  debug("%s", __PRETTY_FUNCTION__);
  cPlugin *p = cPluginManager::GetPlugin(plugin);
  if (p) {
    const char *entry = p->MainMenuEntry();
    if (entry)
      return entry;
  }
  return plugin;
}

int cSkinReelDisplayMenu::DrawFlag(int x, int y, const tComponent *p) // not used at all
{
  debug("%s", __PRETTY_FUNCTION__);
  //Must be TE_LOCKed by caller

  if (p == NULL)
    return 0;

  if ((p->stream == 2) && p->language) {
    std::string flag("flags/");
    flag += p->language;
    /*TODO
    if (p->description) {

    }
    */
    if (ReelLogoCache.LoadSymbol(flag.c_str())) {
     // osd->DrawBitmap(x, y - ReelLogoCache.Get().Height(), ReelLogoCache.Get());
      return ReelLogoCache.Get().Width();
    }
  }

  return 0;
}

void cSkinReelDisplayMenu::SetEvent(const cEvent *Event)
{
  debug("%s", __PRETTY_FUNCTION__);
  // check if event
  if (!Event)
    return;

  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);
  const cFont *pFontDetailsText = ReelConfig.GetFont(FONT_DETAILSTEXT);

  isMainMenu = false;
  fShowInfo = false;
  fShowLogo = fShowLogoDefault ? ReelConfig.showImages : false;
  SetupAreas();

  time_t now = time(NULL);

  // percent of current program completed
  int percentCompleted = 0;
  // draw recording date string
  std::stringstream sstrDate;
  std::stringstream sstrDuration;
  sstrDate << *Event->GetDateString()
           << "  " << *Event->GetTimeString()
           << " - " << *Event->GetEndTimeString();

  sstrDuration << " (";
  if (now > Event->StartTime()) {
    percentCompleted = ((now - Event->StartTime())*100) /60;
    sstrDuration << (now - Event->StartTime()) / 60 << '/';
  } else
    percentCompleted = 0;

  sstrDuration << Event->Duration() / 60 << tr("min") << ')';

  if ( Event->Duration() > 0 )
      percentCompleted /= ( Event->Duration() / 60 );
  else
      percentCompleted = 0;

  int y = yDateBottom + (pFontDetailsDate->Height() - CHANNELINFOSYMBOLHEIGHT) / 2;
  int xs = xDateRight - Gap;
  // check if event has VPS
  if (ReelConfig.showVps && Event->Vps()) {
    // draw VPS symbol
    if (Event->Vps() != Event->StartTime()) {
      char vps[6];
      struct tm tm_r;
      time_t t_vps = Event->Vps();
      strftime(vps, sizeof(vps), "%H:%M", localtime_r(&t_vps, &tm_r));
      xs -= pFontDetailsDate->Width(vps);
      osd->DrawText(xs, yDateBottom, vps,
                    Theme.Color(clrMenuTxtFg), clrTransparent /*Theme.Color(clrBackground)*/,
                    pFontDetailsDate, pFontDetailsDate->Width(vps), pFontDetailsDate->Height());
      xs -= TinyGap;
    }
  }

  //TODO: shall we implement this? It seems to be a nice feature. (FLOH)
  // check if event is running
  // if (Event->IsRunning()) {     // draw running symbol
    // xs -= (bmRun.Width() + Gap);
    // osd->DrawBitmap(xs, y, bmRun, Theme.Color(clrSymbolRunActive), Theme.Color(clrBackground));
    // xs -= Gap;
  //  }
  // check if event has timer
  /* if (Event->HasTimer()) {
    if (Event->IsRunning()) {
      // draw recording symbol
      xs -= bmRecording.Width();
      osd->DrawBitmap(xs, y, bmRecording, Theme.Color(clrSymbolRecActive), Theme.Color(clrBackground));
      debug("SymbolActive: bmRecording line %d", __LINE__);
    } else {
      // draw timer symbol
      xs -= bmTimer.Width();
      osd->DrawBitmap(xs, y, bmTimer, Theme.Color(clrSymbolTimerActive), Theme.Color(clrBackground));
    }
    xs -= Gap;
  } */

  std::string stringInfo;
  const cComponents *Components = Event->Components();
  if (Components) {
    std::stringstream sstrInfo;
    for (int i = 0; i < Components->NumComponents(); i++) {
      const tComponent *p = Components->Component(i);
      if (p && (p->stream == 2) && p->language) {
        if (p->description) {
          sstrInfo << p->description
                   << " (" << p->language << "), ";
        } else {
          sstrInfo << p->language << ", ";
        }
//        DrawFlag(p); //TODO
      }
    }
    // strip out the last delimiter
    if (!sstrInfo.str().empty())
      stringInfo = tr("Languages");
      stringInfo += ": ";
      stringInfo += sstrInfo.str().substr(0, sstrInfo.str().length() - 2);
  }
  int yHeadlineBottom = yDateBottom + pFontDetailsTitle->Height();
  int xHeadlineRight  = fShowLogo ? xDateLeft : xInfoRight;
  osd->DrawRectangle(xBodyLeft, yBodyTop, xHeadlineRight - 1, yHeadlineBottom - 1, Theme.Color(clrAltBackground));
  int th = pFontDetailsTitle->Height() + (!isempty(Event->Description()) && !isempty(Event->ShortText()) ? Gap + pFontDetailsSubtitle->Height() : 0);
  y = yBodyTop + (yHeadlineBottom - yBodyTop - th) / 2;

  // draw recording title
  osd->DrawText(xBodyLeft + Roundness *2, y, Event->Title(),
                Theme.Color(clrRecTitleFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                pFontDetailsTitle, xHeadlineRight - xBodyLeft - Roundness*2 - 1, pFontDetailsTitle->Height());

  osd->DrawText(xBodyLeft + Roundness*2, yHeadlineBottom, sstrDate.str().c_str(),
                Theme.Color(clrMenuItemNotSelectableFg), clrTransparent /* Theme.Color(clrBackground)*/,
                pFontDetailsDate, xs - xBodyLeft - Roundness*2 - 1, pFontDetailsDate->Height());

  // Draw percent Completed bar
  int pbthickness = (int)( 0.4*pFontDetailsDate->Height("|") );
  int pbLength = pFontDetailsDate->Width("Wed 01.01.2008");
  int xPB = xBodyLeft + Roundness*2;// + 4*Gap + pFontDetailsDate->Width(sstrDate.str().c_str()); //XXX check if it exceeds OSD
  int yPB = yHeadlineBottom + (int)(1.5*lineHeight - 0.5*pbthickness );

  // check if percentage is within [0,100]
  if ( percentCompleted < 0 )
    percentCompleted = 0;
  else if (percentCompleted > 100)
    percentCompleted = 100;

  // Background
  osd->DrawRectangle( xPB , yPB , xPB + pbLength, yPB + pbthickness, Theme.Color(clrProgressBarBg) ) ;
  // Foreground
  osd->DrawRectangle( xPB , yPB , xPB + (int)(pbLength*percentCompleted/100), yPB + pbthickness, Theme.Color(clrProgressBarFg) ) ;

  // Event Duration
  osd->DrawText( xPB + pbLength + 4*Gap, yHeadlineBottom + lineHeight, sstrDuration.str().c_str(),
     Theme.Color(clrMenuItemNotSelectableFg), clrTransparent /*Theme.Color(clrBackground)*/, pFontDetailsDate);

  // draw recording short text and description
  const char *strDescr = NULL;
  if (isempty(Event->Description())) {
    // check if short text
    if (!isempty(Event->ShortText())) {
      // draw short text as description, if no description available
      strDescr = Event->ShortText();
    }
  } else {
    // check if short text
    if (!isempty(Event->ShortText())) {
      // draw below Event->Title
      y += pFontDetailsTitle->Height() + Gap;

      // draw short text
      osd->DrawText(xBodyLeft + Roundness*2, y, Event->ShortText(),
                      Theme.Color(clrMenuItemNotSelectableFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                      pFontDetailsSubtitle, xHeadlineRight - xBodyLeft - Roundness*2 - 1, pFontDetailsSubtitle->Height());
    }
    // draw description
    strDescr = Event->Description();
  }

  std::string stringReruns;

  const char *strFirst = NULL;
  const char *strSecond = NULL;
  const char *strThird = stringReruns.empty() ? NULL : stringReruns.c_str();
  if (ReelConfig.showAuxInfo) {
    strFirst = strDescr;
    strSecond = stringInfo.empty() ? NULL : stringInfo.c_str();
  } else {
    strFirst = stringInfo.empty() ? NULL : stringInfo.c_str();
    strSecond = strDescr;
  }
  if (strFirst || strSecond || strSecond) {
    y = yHeadlineBottom + SmallGap + 3 * pFontDetailsDate->Height(); // 2->3 for the EPG event progressbar
    std::stringstream mytext;
    if(strFirst)
      mytext << strFirst;
    if(strSecond)
      mytext << strSecond;
    if((strFirst || strSecond) && strThird)
      mytext << std::endl << std::endl;
    if(strThird)
      mytext << strThird;
    textScroller.Set(osd, xBodyLeft + Roundness*2, y,
                     xInfoRight - SmallGap - ScrollbarWidth - xBodyLeft - Roundness*3,
                     yBodyBottom - y,
                     mytext.str().c_str(), pFontDetailsText,
                     Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground));
    SetScrollbar();
  }
}

int cSkinReelDisplayMenu::ReadSizeVdr(const char *strPath)
{
  debug("%s", __PRETTY_FUNCTION__);
  int dirSize = -1;
  char buffer[20];
  std::stringstream strFilename;
  strFilename << strPath <<  "/size.vdr";
  struct stat st;
  if (stat(strFilename.str().c_str(), &st) == 0) {
    int fd = open(strFilename.str().c_str(), O_RDONLY);
    if (fd >= 0) {
      if (safe_read(fd, &buffer, sizeof(buffer)) >= 0) {
        dirSize = atoi(buffer);
      }
      close(fd);
    }
  }
  return dirSize;
}

void cSkinReelDisplayMenu::SetRecording(const cRecording *Recording)
{
  debug("%s", __PRETTY_FUNCTION__);
  // check if recording
  if (!Recording)
    return;

  const cRecordingInfo *Info = Recording->Info();
  if (Info == NULL) {
    //TODO: draw error message
    return;
  }

  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);
  const cFont *pFontDetailsText = ReelConfig.GetFont(FONT_DETAILSTEXT);

  isMainMenu = false;
  fShowInfo = false;
  fShowLogo = fShowLogoDefault ? ReelConfig.showImages : false;;
  //  SetupAreas(); Why? It's already called at first time of opening OsdMenu

  // draw additional information
  std::stringstream sstrInfo;
  int dirSize = -1;
  if (ReelConfig.showRecSize > 0) {
    if ((dirSize = ReadSizeVdr(Recording->FileName())) < 0 && ReelConfig.showRecSize == 2) {
      dirSize = DirSizeMB(Recording->FileName());
    }
  }
  cChannel *channel = Channels.GetByChannelID(((cRecordingInfo *)Info)->ChannelID());
  std::stringstream sstrChannelName_Date;
  if (channel) {
      // draw recording date string
#if APIVERSNUM < 10721
      sstrChannelName_Date  << trVDR("Channel") << ": " << channel->Number() << " - " << channel->Name()
          << "      " << *DateString(Recording->start) << "   " << *TimeString(Recording->start) ;
  } else
      sstrChannelName_Date << "      " << *DateString(Recording->start) << "   " << *TimeString(Recording->start) ;
#else
      sstrChannelName_Date  << trVDR("Channel") << ": " << channel->Number() << " - " << channel->Name()
          << "      " << *DateString(Recording->Start()) << "   " << *TimeString(Recording->Start()) ;
  } else
      sstrChannelName_Date << "      " << *DateString(Recording->Start()) << "   " << *TimeString(Recording->Start()) ;
#endif

  const char * priority;

#if APIVERSNUM < 10721
  switch (Recording->priority) {
      case 10: priority = trVDR("low");               break;
      case 50: priority = trVDR("normal");            break;
      case 99: priority = trVDR("high");              break;
      default: priority = *itoa(Recording->priority); break;
  }
#else
  switch (Recording->Priority()) {
      case 10: priority = trVDR("low");               break;
      case 50: priority = trVDR("normal");            break;
      case 99: priority = trVDR("high");              break;
      default: priority = *itoa(Recording->Priority()); break;
  }
#endif

  if (dirSize >= 0)
    sstrInfo << tr("Size") << ": " << std::setprecision(3) << (dirSize > 1023 ? dirSize / 1024.0 : dirSize) << (dirSize > 1023 ? "GB\n" : "MB\n");
  sstrInfo << trVDR("Priority") << ": " << priority << std::endl
           << trVDR("Lifetime") << ": "
#if APIVERSNUM < 10721
           << (Recording->lifetime == 99 ? trVDR("unlimited") : *itoa(Recording->lifetime))
#else
           << (Recording->Lifetime() == 99 ? trVDR("unlimited") : *itoa(Recording->Lifetime()))
#endif
	   << std::endl;
  if (Info->Aux()) {
    sstrInfo << std::endl << tr("Auxiliary information") << ":\n"
             << parseaux(Info->Aux());
  }
  int xs = xDateRight - Gap;
  const cComponents *Components = Info->Components();
  if (Components) {
    //TODO: show flaggs?
    std::stringstream info;
    for (int i = 0; i < Components->NumComponents(); i++) {
      const tComponent *p = Components->Component(i);
      if ((p->stream == 2) && p->language) {
        if (p->description) {
          info << p->description
               << " (" << p->language << "), ";
        } else {
          info << p->language << ", ";
        }
      }
    }
    // strip out the last delimiter
    if (!info.str().empty()) {
      sstrInfo << tr("Languages") << ": " << info.str().substr(0, info.str().length() - 2);
    }
  }

  int yHeadlineBottom = yDateBottom + pFontDetailsTitle->Height();
  int xHeadlineRight  = fShowLogo ? xDateLeft : xInfoRight;
  const char *Title = Info->Title();
  if (isempty(Title))
    Title = Recording->Name();
  osd->DrawRectangle(xItemLeft, yBodyTop, xHeadlineRight - 1, yHeadlineBottom - 1, Theme.Color(clrAltBackground));
  int th = pFontDetailsTitle->Height() + (!isempty(Info->Description()) && !isempty(Info->ShortText()) ? Gap + pFontDetailsSubtitle->Height() : 0);
  int y = yBodyTop + (yHeadlineBottom - yBodyTop - th) / 2;

  // draw recording title
  osd->DrawText(xItemLeft + Gap + Roundness, y, Title, // Roundness added
                Theme.Color(clrRecTitleFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                pFontDetailsTitle, xHeadlineRight - xItemLeft - Gap - 1 - Roundness, pFontDetailsTitle->Height());

  osd->DrawText(xItemLeft + Gap + Roundness, yHeadlineBottom, sstrChannelName_Date.str().c_str(), // Roundness added
                Theme.Color(clrRecDateFg), clrTransparent /*Theme.Color(clrBackground)*/,
                pFontDetailsDate, xs - xItemLeft - Gap - 1 - Roundness, pFontDetailsDate->Height());
  // draw recording short text and description
  const char* strDescr = NULL;
  if (isempty(Info->Description())) {
    // check if short text
    if (!isempty(Info->ShortText())) {
      // draw short text as description, if no description available
      strDescr = Info->ShortText();
    }
  } else {
    // check if short text
    if (!isempty(Info->ShortText())) {
      // draw below Title
      y += pFontDetailsTitle->Height() + Gap;

      // draw short text
      osd->DrawText(xItemLeft + Gap + Roundness, y, Info->ShortText(),
                    Theme.Color(clrMenuItemNotSelectableFg), clrTransparent /*Theme.Color(clrAltBackground)*/,
                    pFontDetailsSubtitle, xHeadlineRight - xItemLeft - Gap - Roundness - 1, pFontDetailsSubtitle->Height());
    }
    // draw description
    strDescr = Info->Description();
  }

  std::string stringInfo = sstrInfo.str();
  const char *strInfo = stringInfo.empty() ? NULL : stringInfo.c_str();
  debug("strInfo: %s", strInfo);
  debug("strDescr: %s", strDescr);
  if (strDescr || strInfo) {
    //y = yHeadlineBottom + pFontDetailsDate->Height() + BigGap;
    y = yHeadlineBottom + SmallGap + 2 * pFontDetailsDate->Height();
    std::stringstream mytext;

    if (ReelConfig.showAuxInfo) {
      if(strDescr)
         mytext << strDescr;
      if(strInfo && strDescr)
        mytext <<  std::endl << std::endl;
      if(strInfo)
        mytext << strInfo;
    } else {
      if(strInfo)
         mytext << strInfo;
      if(strInfo && strDescr)
         mytext << std::endl << std::endl;
      if(strDescr)
         mytext << strDescr;
    }

    textScroller.Set(osd, xItemLeft +2* Roundness, y, // added Roundness
                     xInfoRight - SmallGap- ScrollbarWidth - xItemLeft - Roundness*3,  // added Roundness
                     yBodyBottom - y, mytext.str().c_str(), pFontDetailsText,
                     Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground));
    SetScrollbar();
    debug("\n%s mytext : \n %s \n",__PRETTY_FUNCTION__,mytext.str().c_str());
  }
}

void cSkinReelDisplayMenu::SetText(const char *Text, bool FixedFont)
{
  debug("%s", __PRETTY_FUNCTION__);
  debug("cSkinReelDisplayMenu::SetText(%s,%d)\n",Text,FixedFont);
  // draw text
  textScroller.Set(osd, xItemLeft + Gap, yBodyTop + Gap,
                   GetTextAreaWidth(), yBodyBottom - Gap - yBodyTop - Gap, Text,
                   GetTextAreaFont(FixedFont), Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground));
  SetScrollbar();
}

int cSkinReelDisplayMenu::GetTextAreaWidth(void) const
{
  debug("%s", __PRETTY_FUNCTION__);
  // max text area width
  return (fShowLogo || fShowInfo ? xBodyRight : xInfoRight) - Gap - SmallGap - ScrollbarWidth - SmallGap -xItemLeft;
}

const cFont *cSkinReelDisplayMenu::GetTextAreaFont(bool FixedFont) const
{
  debug("%s", __PRETTY_FUNCTION__);
  // text area font
  return FixedFont ? ReelConfig.GetFont(FONT_FIXED) : ReelConfig.GetFont(FONT_DETAILSTEXT);
}

int cSkinReelDisplayMenu::getDateWidth(const cFont *pFontDate)
{
  // only called from constructor, so pFontDate should be OK
  debug("%s", __PRETTY_FUNCTION__);
  int w = MIN_DATEWIDTH;
  struct tm tm_r;
  time_t t = time(NULL);
  tm *tm = localtime_r(&t, &tm_r);

  int nWeekday = tm->tm_wday;
  if (0 <= nWeekday && nWeekday < 7)
    w = std::max(w, pFontDate->Width(WEEKDAY(nWeekday)));

  char temp[32];
  strftime(temp, sizeof(temp), "%d.%m.%Y", tm);
  w = std::max(w, pFontDate->Width(temp));

  cString time = TimeString(t);
  w = std::max(w, pFontDate->Width(time));

  return w;
}

void cSkinReelDisplayMenu::Flush(void)
{
  debug("%s", __PRETTY_FUNCTION__);
  //printf("cSkinReelDisplayMenu::Flush()\n");

  if (fShowLogo) {
    time_t t = time(NULL);
    cString time = TimeString(t);
    //printf("time: %s strLastDate: %s\n", time, strLastDate);
    if ((strLastDate == NULL) || strcmp(strLastDate, (const char*)time) != 0) {
      free(strLastDate);
      strLastDate = strdup((const char*)time);

      const cFont *pFontDate = ReelConfig.GetFont(FONT_DATE);
      int x = xDateLeft + SmallGap;
      int w = xLogoLeft - x;
      int ys = yDateTop + (yDateBottom - SmallGap - yDateTop - 3 * pFontDate->Height()) / 2;

      char temp[32];
      struct tm tm_r;
      tm *tm = localtime_r(&t, &tm_r);

      int nWeekday = tm->tm_wday;
      if (0 <= nWeekday && nWeekday < 7) {
        osd->DrawText(x, ys,WEEKDAY(nWeekday), Theme.Color(clrMenuTxtFg),
                      Theme.Color(clrLogoBg), pFontDate, w, pFontDate->Height(), taCenter);
      }
      ys += pFontDate->Height();

      strftime(temp, sizeof(temp), "%d.%m.%Y", tm);
      osd->DrawText(x, ys, temp, Theme.Color(clrMenuTxtFg),
                    Theme.Color(clrLogoBg), pFontDate, w, pFontDate->Height(), taCenter);
      ys += pFontDate->Height();

      osd->DrawText(x, ys, time, Theme.Color(clrMenuTxtFg),
                    Theme.Color(clrLogoBg), pFontDate, w, pFontDate->Height(), taCenter);
    }
  } else {
    cString date = GetFullDateTime();
    if ((strLastDate == NULL) || strcmp(strLastDate, (const char*)date) != 0) {

      //printf("strLastDate:%d, date: %s", strLastDate==NULL?0:1, (const char*)date );
      free(strLastDate);
      strLastDate = strdup((const char*)date);
      const cFont *pFontDate = ReelConfig.GetFont(FONT_DATE);
#if 1
      osd->DrawImage(imgMenuHeaderCenterX, xDateLeft, 0, false, xDateRight - xDateLeft -Roundness,1); // to clean before drawing date
      osd->DrawText(xDateLeft + SmallGap, yDateTop, date, Theme.Color(clrMenuTxtFg),
                    clrTransparent, pFontDate, xDateRight - xDateLeft - SmallGap - Roundness,
                    yTitleDecoBottom - yDateTop - 2, taRight);
#endif
    }
  }

#ifdef THUMB_LEFT_SIDE
#define TX 20
#define TY 50
#define TW 100
#define TH 100
#define BCOLOR 0xaaffffff
#else
#define TX 450
#define TY 75
#define TW 150
#define TH 150
#define BCOLOR 0x00000000
#endif
#define BW 2

  //if(MenuType == menuImage){
      if (rbPlugin == NULL) rbPlugin = cPluginManager::GetPlugin("reelbox");
      static struct imgSize picSize[NR_UNBUFFERED_SLOTS];

      for(int i = 0; i<NR_UNBUFFERED_SLOTS; i++) {
      if (strncmp(ReelConfig.curThumb[i].path, oldThumb[i], 512)) {
	 if (strncmp(ReelConfig.curThumb[i].path, "", 512)) {
            std::cout << "showing PIC: " << ReelConfig.curThumb[i].path << std::endl;
	    strncpy((char*)&oldThumb[i], (const char*)&ReelConfig.curThumb[i].path, 512);
            osd->SetImagePath(imgUnbufferedEnum+i, ReelConfig.curThumb[i].path);
            /* only clear & draw border if called by filebrowser - that means with x==0 and y==0 */
#ifdef THUMB_LEFT_SIDE
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
               // clear previous image, since the TitleImages are blended: they get 'brighter' with every overdraw without a clear
               osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW, clrTransparent);
               // draw background
               osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW,  Theme.Color(clrBackground) );
               osd->DrawImage(imgImagesTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT + 16  /* TB: todo fix 16 */); // repeat this image vertically
            }
#endif
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
               osd->DrawImage(imgUnbufferedEnum+i, TX, TY, false);
            else {
               osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);
            }

            /* only draw border if called by filebrowser - that means with x==0 and y==0 */
#ifndef THUMB_LEFT_SIDE
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
	       osd->DrawRectangle(TX - BW, TY - BW, TX + TW + BW, TY + TH + BW, Theme.Color(clrBackground));
#endif
            picSize[i].slot = i;
            if(!ReelConfig.curThumb[i].w && !ReelConfig.curThumb[i].h) {
             if (rbPlugin) rbPlugin->Service("GetUnbufferedPicSize", &picSize[i]);
            } else {
              picSize[i].height = ReelConfig.curThumb[i].h;
              picSize[i].width = ReelConfig.curThumb[i].w;
            }
            std::cout << "SIZE: w: " << picSize[i].width << " h: " << picSize[i].height << std::endl;
#ifndef THUMB_LEFT_SIDE
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
               osd->DrawImage(imgUnbufferedEnum+i, TX + TW - picSize[i].width, TY, false);
            else
               osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
   	       /* draw border */
	       if (picSize[i].width && picSize[i].height) {
	          osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY - BW, TX + TW, TY, BCOLOR); /* top */
	          osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY - BW, TX + TW - picSize[i].width, TY + picSize[i].height, BCOLOR); /* left */
                  osd->DrawRectangle(TX + picSize[i].width + TW - picSize[i].width, TY - BW, TX + BW + TW, TY + picSize[i].height, BCOLOR); /* right */
	          osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY + picSize[i].height, TX + BW + TW, TY + BW + picSize[i].height, BCOLOR); /* bottom */
               }
            }
#else
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
               osd->DrawImage(imgUnbufferedEnum+i, TX, TY, false);
            else
               osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);
            if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
	       /* draw border */
	       if (picSize[i].width && picSize[i].height) {
	          osd->DrawRectangle(TX - BW, TY - BW, TX + picSize.width, TY, BCOLOR); /* top */
	          osd->DrawRectangle(TX - BW, TY - BW, TX, TY + picSize.height, BCOLOR); /* left */
                  osd->DrawRectangle(TX + picSize.width, TY - BW, TX + BW + picSize.width, TY + picSize.height, BCOLOR); /* right */
	          osd->DrawRectangle(TX - BW, TY + picSize.height, TX + BW + picSize.width, TY + BW + picSize.height, BCOLOR); /* bottom */
               }
            }
#endif
	    thumbCleared[i] = 0;
	 } else {
	    strcpy((char*)&oldThumb[i], "");
	    if (!thumbCleared[i]) {
#ifdef THUMB_LEFT_SIDE
               if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                  // clear previous image, since the TitleImages are blended: they get 'brighter' with every overdraw without a clear
                  osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW, clrTransparent);
                  // draw background
                  osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW,  Theme.Color(clrBackground) );
                  osd->DrawImage(imgImagesTitleTopX, 0, yBodyTop, true, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT + 16); // repeat this image vertically
               }
#else
               if (!ReelConfig.curThumb[0].x && !ReelConfig.curThumb[0].y && !ReelConfig.curThumb[1].x && !ReelConfig.curThumb[1].y)
                  osd->DrawRectangle(TX - BW, TY - BW, TX + TW + BW, TY + TH + BW, Theme.Color(clrBackground));
#endif
	       thumbCleared[i] = 1;
	    }
	 }
      }
    }

  osd->Flush();
}

void cSkinReelDisplayMenu::SetScrollbar(int Total, int Offset)
{
//  printf("\033[0;93m --- Total: %i --- Offset: %i --- Max: %i --- \033[0m\n", Total, Offset, MaxItems());
  int yScrollbarTop = yItemTop;
  int yScrollbarBottom = yBodyBottom - SmallGap;
  int xScrollbarRight = xDateRight - Gap;
  int xScrollbarLeft = xScrollbarRight - ScrollbarWidth ;
  int yScrollAreaTop = yScrollbarTop + ScrollbarHeight;
  int yScrollAreaBottom = yScrollbarBottom - ScrollbarHeight;
  int ScrollAreaHeight = yScrollAreaBottom - yScrollAreaTop;
  int xScrollAreaRight = xScrollbarRight;
  int xScrollAreaLeft = xScrollAreaRight - SmallGap;
  int yScrollerTop = yScrollAreaTop + Offset * (ScrollAreaHeight / Total);
  int yScrollerBottom = yScrollerTop + MaxItems() * (ScrollAreaHeight / Total);
//  printf("\033[0;93m --- xItemRight: %i --- xScrollbarRight : %i--- \033[0m\n", xItemRight, xScrollbarRight);

  if(Total > MaxItems())
  {
    // draw background of scrollbar (Clear area)
    osd->DrawRectangle(xScrollbarLeft, yScrollbarTop, xScrollbarRight, yScrollbarBottom, Theme.Color(clrAltBackground));

    // arrow up
    osd->DrawRectangle(xScrollbarLeft, yScrollbarTop, xScrollbarRight, yScrollbarTop + SmallGap,
                       Offset ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
    osd->DrawRectangle(xScrollbarRight - SmallGap, yScrollbarTop, xScrollbarRight, yScrollbarTop + ScrollbarHeight,
                       Offset ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));

    // draw visible area of scrollbar
    osd->DrawRectangle(xScrollAreaLeft, yScrollerTop, xScrollAreaRight, yScrollerBottom, Theme.Color(clrMenuTxtFg));

    // arrow down
    osd->DrawRectangle(xScrollbarLeft, yScrollbarBottom - SmallGap, xScrollbarRight, yScrollbarBottom,
                       MaxItems()+Offset < Total? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
    osd->DrawRectangle(xScrollbarRight - SmallGap, yScrollbarBottom - ScrollbarHeight, xScrollbarRight, yScrollbarBottom,
                       MaxItems()+Offset < Total ? Theme.Color(clrMenuTxtFg) : Theme.Color(clrAltBackground));
  }
}

// --- cSkinReelDisplayReplay ---------------------------------------------

cSkinReelDisplayReplay::cSkinReelDisplayReplay(bool ModeOnly)
{
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontReplayTimes = ReelConfig.GetFont(FONT_REPLAYTIMES);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontLanguage = ReelConfig.GetFont(FONT_CILANGUAGE);
  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  strLastDate = NULL;

  int MessageHeight = std::max( pFontMessage->Height(), MINMESSAGEHEIGHT);

  modeonly = ModeOnly;
  nJumpWidth = 0;
  nCurrentWidth = 0;
  fShowSymbol = ReelConfig.showSymbols && ReelConfig.showSymbolsReplay;

  int LogoSize = Gap + IconHeight + Gap;
  LogoSize += (LogoSize % 2 ? 1 : 0);
  yTitleTop = 0;
  yTitleBottom = yTitleTop + IMGHEADERHEIGHT;
  xLogoLeft = (IMGCHANNELLOGOWIDTH-IMGREPLAYLOGOHEIGHT)/2;
  xLogoRight = xLogoLeft + IMGREPLAYLOGOWIDTH + Gap;
  xTitleLeft = ReelConfig.fullTitleWidth ? 0 : xLogoRight + Gap;
  xTitleRight = OsdSize.w;
  yLogoTop = yTitleBottom + Gap - 2; // -2 is finetuning, so the Icon looks well next to Progressbar
  yLogoBottom = yLogoTop + IMGREPLAYLOGOHEIGHT;
  xProgressLeft = xTitleLeft;
  xProgressRight = xTitleRight;
  yProgressTop = yTitleBottom + Gap;
  yProgressBottom = yLogoBottom - 11; // -11 is finetuning, so the Icon looks well next to Progressbar
  yBottomTop = yProgressBottom + Gap;
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomBottom = yBottomTop + std::max(pFontDate->Height(), pFontLanguage->Height());
  xMessageLeft = xProgressLeft;
  xMessageRight = xProgressRight;
  yMessageTop = yLogoTop + (LogoSize - MessageHeight) / 2;
  yMessageBottom = yMessageTop + MessageHeight;
  xFirstSymbol = xBottomRight - Roundness;

  // create osd
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom,Setup.OSDRandom);
  tArea Areas[] = { {std::min(xTitleLeft, xLogoLeft), yTitleTop, xBottomRight - 1, yBottomBottom - 1, 32} }; // TrueColorOsd complain if <32
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
debug("cSkinReelDisplayReplay: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
  } else {
debug("cSkinReelDisplayReplay: using multiple areas");
    int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
    if (rc == oeOk)
      osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    else {
      error("cSkinReelDisplayReplay: CanHandleAreas() returned %d", rc);
      delete osd;
      osd = NULL;
      throw 1;
      return;
    }
  }
  // clear all
  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  if (!modeonly)
  {
    // draw progress bar area
    osd->DrawRectangle(xProgressLeft, yProgressTop, xProgressRight - 1,
                       yProgressBottom - 1, Theme.Color(clrReplayBarAreaBg));
    // draw bottom area
    osd->DrawImage(imgReplayFooterLeft, xBottomLeft, yBottomTop, false);
    osd->DrawImage(imgReplayFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight - xBottomLeft - Roundness*2);
    osd->DrawImage(imgReplayFooterRight, xBottomRight - Roundness, yBottomTop, false);

    xFirstSymbol = DrawStatusSymbols(0, xFirstSymbol, yBottomTop, yBottomBottom) - Gap;
  }
}

cSkinReelDisplayReplay::~cSkinReelDisplayReplay()
{
  free(strLastDate);
  delete osd;
}

void cSkinReelDisplayReplay::SetTitle(const char *Title)
{
    DrawTitle(Title);
}

void cSkinReelDisplayReplay::DrawTitle(const char *Title)
{

  // draw title area
  osd->DrawImage(imgReplayHeaderLeft, xTitleLeft, yTitleTop, false);
  osd->DrawImage(imgReplayHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight - xTitleLeft - Roundness*2);
  osd->DrawImage(imgReplayHeaderRight, xTitleRight - Roundness, yTitleTop, false);

  if (Title) {
    debug("REPLAY TITLE: %s", Title);
    // draw titlebar
    osd->DrawText(xTitleLeft + Roundness, yTitleTop, Title,
                  Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                  xTitleRight - Roundness - xTitleLeft - Roundness - 3,
                  yTitleBottom - yTitleTop);
  }
}

void cSkinReelDisplayReplay::SetMode(bool Play, bool Forward, int Speed)
{
  if (Speed < -1)
    Speed = -1;

  if (modeonly)
    osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, clrTransparent);

  // clear ControlIcon
  osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight, yBottomBottom, clrTransparent);

  if (Speed == -1)
    osd->DrawImage(Play?imgIconControlPlay:imgIconControlPause, xLogoLeft, yLogoTop, true);
  else if (Play) {
    if (Speed > MAX_SPEED_BITMAPS) {
      error("MAX SPEED %d > 9", Speed);
      Speed = MAX_SPEED_BITMAPS;
    }

    osd->DrawImage(Forward?imgIconControlFastFwd+Speed:imgIconControlFastRew+Speed, xLogoLeft, yLogoTop, true);
  } else {                      // trick speed
    if (Speed > MAX_TRICKSPEED_BITMAPS) {
      error("MAX SPEED %d > 3", Speed);
      Speed = MAX_TRICKSPEED_BITMAPS;
    }
    osd->DrawImage(Forward?imgIconControlSkipFwd+Speed:imgIconControlSkipRew+Speed, xLogoLeft, yLogoTop, true);
  }
}

void cSkinReelDisplayReplay::SetProgress(int Current, int Total)
{
 // create progressbar
  cProgressBar pb(xProgressRight - xProgressLeft - 2 * BigGap,
                  yProgressBottom - yProgressTop - 2 * BigGap, Current, Total,
                  marks, Theme.Color(clrReplayProgressSeen),
                  Theme.Color(clrReplayProgressRest),
                  Theme.Color(clrReplayProgressSelected),
                  Theme.Color(clrReplayProgressMark),
                  Theme.Color(clrReplayProgressCurrent));
  // draw progressbar
  osd->DrawBitmap(xProgressLeft + BigGap, yProgressTop + BigGap, pb);
}

void cSkinReelDisplayReplay::SetCurrent(const char *Current)
{
  if (!Current)
    return;

  // draw current time
  int w = pFontReplayTimes->Width(Current);
  //FLOH
  osd->DrawImage(imgReplayFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, w);
  osd->DrawText(xBottomLeft + Roundness, yBottomTop, Current,
                Theme.Color(clrReplayCurrent), clrTransparent, pFontReplayTimes,
                w, yBottomBottom - yBottomTop, taLeft);
  if (nCurrentWidth > w)
    osd->DrawImage(imgReplayFooterCenterX, xBottomLeft + Roundness + w, yBottomTop, false, xBottomLeft + Roundness + nCurrentWidth);
  nCurrentWidth = w;
}

void cSkinReelDisplayReplay::SetTotal(const char *Total)
{
  if (!Total)
    return;

  // draw total time
  int w = pFontReplayTimes->Width(Total);
  osd->DrawImage(imgReplayFooterCenterX, xBottomRight - Roundness - w, yBottomTop, false, w);
  osd->DrawText(xBottomRight - Roundness - w, yBottomTop, Total,
                Theme.Color(clrReplayTotal), clrTransparent, pFontReplayTimes, w,
                yBottomBottom - yBottomTop, taRight);
}

void cSkinReelDisplayReplay::SetJump(const char *Jump)
{
  if (Jump) {
    // draw jump prompt
    nJumpWidth = pFontReplayTimes->Width(Jump);
    osd->DrawImage( imgReplayFooterCenterX,
        xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2,
        yBottomTop, false, nJumpWidth, 1);

    osd->DrawText(xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2,
                  yBottomTop, Jump, Theme.Color(clrReplayModeJump),
                  clrTransparent, pFontReplayTimes, nJumpWidth,
                  yBottomBottom - yBottomTop, taLeft);
  } else {
    // erase old prompt
    osd->DrawImage( imgReplayFooterCenterX,
        xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2,
        yBottomTop, false, nJumpWidth, 1);
  }
}

void cSkinReelDisplayReplay::SetMessage(eMessageType Type, const char *Text)
{

  int x0 = xMessageLeft, x1 = xMessageLeft + Roundness, x2 = xMessageRight - Roundness;
  int y0 = yMessageTop, y2 = y0 + 30; // BarimageHeight

  int y1 = (y2 + y0 - pFontMessage->Height()) / 2; // yText

  tColor  colorfg = 0xFFFFFFFF;
  int imgMessageIcon;

       switch(Type)
        {
            case mtStatus:
                imgMessageIcon = imgIconMessageInfo;
                break;
            case mtWarning:
                imgMessageIcon = imgIconMessageWarning;
                break;
            case mtError:
                imgMessageIcon = imgIconMessageError;
                break;
            default:
                imgMessageIcon = imgIconMessageNeutral;
                break;
        }
        colorfg = 0xFFFFFFFF;
	if (Text)
	{
		// Draw Message Bar
		osd->DrawImage(imgMessageBarLeft, x0,y0,false);
		osd->DrawImage(imgMessageBarCenterX, x1, y0, false, x2 - x1, 1);
		osd->DrawImage(imgMessageBarRight, x2,y0,false);

		osd->DrawImage(imgMessageIcon, x1,y0, true);
#define messageIconW 30
		//display message
		osd->DrawText(x1 + messageIconW, y1, Text, colorfg, clrTransparent, pFontMessage, x2 - x1 - messageIconW , 0, taCenter);
	} // else must restore
	else
	{
	  osd->DrawRectangle(x0,y0,x2+Roundness,y0+30,0x00000000);
	}
}

void cSkinReelDisplayReplay::Flush(void)
{
  osd->Flush();
}

// --- cSkinReelDisplayVolume ---------------------------------------------

int cSkinReelDisplayVolume::mute = 0;

cSkinReelDisplayVolume::cSkinReelDisplayVolume()
{
    struct ReelOsdSize OsdSize;
    ReelConfig.GetOsdSize(&OsdSize);

    mute = 0;
    int imgWidth = 64, imgHeight = 64; // Height of the icons

    volumeBarLength = int(OsdSize.w * .7);
    // int totalLen = Roundness + .0*Roundness + imgWidth + Roundness + volumeBarLength + Roundness; // to center the volume OSD

    xVolumeBackBarLeft = 0; //( OsdSize.w - totalLen ) / 2; // center the OSD
    xIconLeft = int(xVolumeBackBarLeft + 1.0 * Roundness);
    xVolumeFrontBarLeft = xIconLeft + imgWidth + Roundness;
    xVolumeFrontBarRight = xVolumeFrontBarLeft + volumeBarLength;
    xVolumeBackBarRight = xVolumeFrontBarRight + Roundness;
    int totalLen = xVolumeBackBarRight - xVolumeBackBarLeft; // to center the volume OSD

    int h = 19; // height of the frontBar
    int H = 35 ;  // height of OSD excluding the icon height

    yIconTop = 0; // icon yTop
    yVolumeBackBarTop = yIconTop + imgHeight /2 - H/2;
    yVolumeFrontBarTop = yVolumeBackBarTop + H/2 - h/2; // top volumefront bar
    yVolumeBackBarBottom = yVolumeBackBarTop + H;
    yIconBottom = yIconTop + imgHeight;

    // osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - yVolumeBackBarBottom,Setup.OSDRandom);
    debug("osdsize %d %d", OsdSize.x, OsdSize.y);
    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + ( OsdSize.w - totalLen ) / 2, OsdSize.y + OsdSize.h - yIconBottom,Setup.OSDRandom);
    tArea Areas[] = { { xVolumeBackBarLeft, yIconTop, xVolumeBackBarRight - 1, yIconBottom - 1, 32 } };
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    SetImagePaths(osd);
}

cSkinReelDisplayVolume::~cSkinReelDisplayVolume()
{
    delete osd;
}

void cSkinReelDisplayVolume::SetVolume(int Current, int Total, bool Mute)
{
    osd->DrawRectangle(xVolumeBackBarLeft, yIconTop, xVolumeBackBarRight - 1, yIconBottom - 1, clrTransparent);

    //debug("DisplayVolume::SetVolume ( %d, %d, %d, (mute=%d) )", Current, Total, Mute, mute);

    osd->DrawImage(imgVolumeBackBarLeft, xVolumeBackBarLeft,yVolumeBackBarTop, false);
    osd->DrawImage(imgVolumeBackBarMiddleX, xVolumeBackBarLeft+Roundness,yVolumeBackBarTop,false,xVolumeBackBarRight - xVolumeBackBarLeft - 2*Roundness, 1);
    osd->DrawImage(imgVolumeBackBarRight, xVolumeFrontBarRight,yVolumeBackBarTop,false);

    // Draw the front Bar
    int percent = int((float)(Current) / Total * (xVolumeFrontBarRight - xVolumeFrontBarLeft));
    osd->DrawImage(imgVolumeFrontBarActiveX, xVolumeFrontBarLeft, yVolumeFrontBarTop, false,  percent, 1);
    osd->DrawImage(imgVolumeFrontBarInactiveX, xVolumeFrontBarLeft + percent + 1, yVolumeFrontBarTop, false, xVolumeFrontBarRight - xVolumeFrontBarLeft - percent - 1, 1);

    //Draw Volume icons
    if (Mute) {
        osd->DrawImage(imgMuteOn, xIconLeft, yIconTop, true);
        mute = 1;
    } else {
       // int xVolumeFrontBarLeft_1 = xVolumeFrontBarLeft + (xVolumeFrontBarRight - xVolumeFrontBarLeft) * Current / Total;
        if ( mute==1 ) {
            osd->DrawImage(imgMuteOff, xIconLeft, yIconTop, true);
            mute = 0;
        } else {
            osd->DrawImage(imgVolume, xIconLeft, yIconTop, true);
        }
    }
}

void cSkinReelDisplayVolume::Flush(void)
{
    osd->Flush();
}

// --- cSkinReelDisplayTracks ---------------------------------------------

cSkinReelDisplayTracks::cSkinReelDisplayTracks(const char *Title, int NumTracks, const char *const *Tracks)
{
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontListItem = ReelConfig.GetFont(FONT_LISTITEM);

  lastTime = 0;
  fShowSymbol = ReelConfig.showSymbols && ReelConfig.showSymbolsAudio;

  lineHeight = std::max(pFontListItem->Height() , 35);
  int LogoSize = IconHeight;
  LogoSize += (LogoSize % 2 ? 1 : 0);
  currentIndex = -1;
  int ItemsWidth = 0;
  //Get required space for the list items
  for (int i = 0; i < NumTracks; i++)
    ItemsWidth = std::max(ItemsWidth, pFontListItem->Width(Tracks[i]));
  //Add required space for marker or border
  ItemsWidth += (ReelConfig.showMarker ? lineHeight : ListHBorder) + ListHBorder;
  //If OSD title only covers the list then its content must fit too
  if (!ReelConfig.fullTitleWidth)
      ItemsWidth = std::max(ItemsWidth, pFontOsdTitle->Width(Title) + 2 * Roundness + (fShowSymbol ? 0 : (CHANNELINFOSYMBOLWIDTH + SmallGap)));
  //If the symbol is shown the list's width should be at least as wide as the symbol
  if (fShowSymbol)
    ItemsWidth = std::max(ItemsWidth, LogoSize);
  //Now let's calculate the OSD's full width
  if (ReelConfig.fullTitleWidth) {
    //If the symbol is shown, add its width
    if (fShowSymbol)
      ItemsWidth += LogoSize + LogoDecoGap + LogoDecoWidth + LogoDecoGap2;
    //The width must be wide enough for the OSD title
    ItemsWidth = std::max(ItemsWidth, pFontOsdTitle->Width(Title) + 2 * Roundness + (fShowSymbol ? 0 : (CHANNELINFOSYMBOLWIDTH + SmallGap)));
  }

  yTitleTop = 0;
  yTitleBottom = pFontOsdTitle->Height();
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  xLogoLeft = 0;
  xLogoRight = xLogoLeft + LogoSize;
  xLogoDecoLeft = xLogoRight + LogoDecoGap;
  xLogoDecoRight = xLogoDecoLeft + LogoDecoWidth;
  yLogoTop = yTitleDecoBottom + TitleDecoGap2;
  yLogoBottom = yLogoTop + std::max(LogoSize, NumTracks * lineHeight);
  xTitleLeft = fShowSymbol ? (ReelConfig.fullTitleWidth ? xLogoLeft : xLogoDecoRight + LogoDecoGap2) : 0;
  xTitleRight = xTitleLeft + FixWidth(ItemsWidth, 2) + Roundness;
  xListLeft = fShowSymbol ? (xLogoDecoRight + LogoDecoGap2) : 0;
  xListRight = xTitleRight;
  yListTop = yLogoTop;
  yListBottom = yLogoBottom;
  xItemLeft = xListLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);
  xItemRight = xListRight - ListHBorder;
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomTop = yListBottom + SmallGap;
  yBottomBottom = yBottomTop + pFontDate->Height();

  // create osd
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom, Setup.OSDRandom );
  tArea Areas[] = { {xTitleLeft, yTitleTop, xBottomRight - 1, yBottomBottom - 1, 32} };
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
debug("cSkinReelDisplayTracks: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
  } else {
debug("cSkinReelDisplayTracks: using multiple areas");
    if (fShowSymbol) {
      tArea Areas[] = { {xTitleLeft, yTitleTop, xTitleRight - 1, yTitleDecoBottom- 1, 2},
                         {xLogoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, 4},
                         {xListLeft, yListTop, xListRight - 1, yListBottom - 1, 2},
                         {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 2}
      };
      int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayTracks: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    } else {
      tArea Areas[] = { {xTitleLeft, yTitleTop, xTitleRight - 1, yTitleDecoBottom- 1, 2},
                         {xListLeft, yListTop, xListRight - 1, yListBottom - 1, 2},
                         {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 2}
      };
      int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayTracks: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    }
  }

  SetImagePaths(osd);

  // clear all
  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  // draw titlebar
  // Header //TODO : replace it with AudioHeader
  osd->DrawImage(imgChannelInfoHeaderLeft, xTitleLeft, yTitleTop, false);
  osd->DrawImage(imgChannelInfoHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight-xTitleLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelInfoHeaderRight, xTitleRight - Roundness , yTitleTop, false);

  osd->DrawText(xTitleLeft + Roundness, yTitleTop, Title,
                Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                xTitleRight - Roundness - xTitleLeft - Roundness, yTitleBottom - yTitleTop,
                fShowSymbol ? taCenter : taLeft);
  if (fShowSymbol) {
    // draw logo area
      osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
      osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
  }
  // draw list area
  osd->DrawRectangle(xListLeft, yListTop, xListRight - 1, yListBottom - 1, Theme.Color(clrBackground));

  // Footer //TODO : replace it with AudioFooter
  osd->DrawImage(imgChannelInfoFooterLeft, xBottomLeft, yBottomTop, false);
  osd->DrawImage(imgChannelInfoFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
  osd->DrawImage(imgChannelInfoFooterRight, xBottomRight - Roundness , yBottomTop, false);

  /* only set audio channel if we really are in the audio menu, not when in subtitle-menu */
  if(!strcmp(Title, tr("Subtitles")))
      subTitleMenu = true;
  else
      subTitleMenu = false;

  SetAudioChannel(cDevice::PrimaryDevice()->GetAudioChannel());

  // fill up audio tracks
  for (int i = 0; i < NumTracks; i++)
    SetItem(Tracks[i], i, false);
}

cSkinReelDisplayTracks::~cSkinReelDisplayTracks()
{
  delete osd;
}

void cSkinReelDisplayTracks::SetItem(const char *Text, int Index, bool Current)
{
  int y = yListTop + Index * lineHeight;
  tColor ColorFg, ColorBg;
  if (Current) {
    ColorFg = Theme.Color(clrMenuItemSelectableFg);
    ColorBg = Theme.Color(clrAltBackground);
    currentIndex = Index;
  } else {
    ColorFg = Theme.Color(clrMenuItemSelectableFg);
    ColorBg = Theme.Color(clrBackground);
  }
  // draw track id
  osd->DrawRectangle(xListLeft, y, xListRight-1, y + lineHeight, ColorBg);
  if (ReelConfig.showMarker && Current) {
    osd->DrawImage(imgIconTimeractive, xListLeft + MarkerGap, y + MarkerGap, true);
  }
  osd->DrawText(xItemLeft, y, Text, ColorFg, clrTransparent /*ColorBg*/, pFontListItem, xItemRight - xItemLeft-1, lineHeight);
  //osd->DrawRectangle(xItemRight, y, xListRight - 1, y + lineHeight - 1, ColorBg);
}

void cSkinReelDisplayTracks::SetAudioChannel(int AudioChannel)
{
  if (fShowSymbol) {
    if (!(AudioChannel >= 0 && AudioChannel < MAX_AUDIO_BITMAPS))
      AudioChannel = 0;
    // DRAW AUDIO ICONS
    osd->DrawImage(imgIconAudio, xLogoLeft + (xLogoRight - xLogoLeft - 64) / 2,
                      yLogoTop + (yLogoBottom - yLogoTop - 64) / 2,
                  true);
  } else {
    if (!(AudioChannel >= 0 && AudioChannel < MAX_AUDIO_BITMAPS))
      AudioChannel = 0;
  }
}

void cSkinReelDisplayTracks::SetTrack(int Index, const char *const *Tracks)
{
  //Reel::ReelBoxDevice::Instance()->SetAudioTrack(Index);

  if (currentIndex >= 0)
    SetItem(Tracks[currentIndex], currentIndex, false);
  SetItem(Tracks[Index], Index, true);

  int *index=new int; *index = Index;
  if(!subTitleMenu)
      cPluginManager::CallAllServices("ReelSetAudioTrack", index);
}

void cSkinReelDisplayTracks::Flush(void)
{
  /* TODO? remove date
  time_t now = time(NULL);
  if (now != lastTime) {
    lastTime = now;
    cString date = DayDateTime();
    osd->DrawText(xBottomLeft + Rounsvn dness, yBottomTop, date,
                  Theme.Color(clrTitleFg), Theme.Color(clrBottomBg),
                  pFontDate,
                  xBottomRight - Roundness - xBottomLeft - Roundness - 1,
                  yBottomBottom - yBottomTop - 1, taRight);
  }
  */
  osd->Flush();
}

// --- ReelSkinDisplayMessage ----------------------------------------------

ReelSkinDisplayMessage::ReelSkinDisplayMessage(void)
{
    struct ReelOsdSize OsdSize;
    ReelConfig.GetOsdSize(&OsdSize);

    font = ReelConfig.GetFont(FONT_MESSAGE);

    messageBarLength = int(OsdSize.w * .7);
    int const lineHeight = font->Height();

#define BarImageHeight 30
    x0 = 0;
    x1 = x0 + Roundness;
    x3 = x0 + messageBarLength;
    x2 = x3 - Roundness;

    y0 = 0;
    y2 = y0 + BarImageHeight;

    y1 = (y2 + y0 - lineHeight) / 2; // yText

    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + OsdSize.w/2 - (x3 - x0)/2, OsdSize.y + OsdSize.h - y2, Setup.OSDRandom);
    tArea Areas[] = { { x0, y0, x3 - 1, y2 - 1, 32 } };
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    SetImagePaths(osd);
}

ReelSkinDisplayMessage::~ReelSkinDisplayMessage()
{
    delete osd;
}

void ReelSkinDisplayMessage::SetMessage(eMessageType Type, const char *Text)
{
    tColor  colorfg = 0xFFFFFFFF;
    int imgMessageIcon;

   switch(Type)
    {
        case mtStatus:
            imgMessageIcon = imgIconMessageInfo;
            break;
        case mtWarning:
            imgMessageIcon = imgIconMessageWarning;
            break;
        case mtError:
            imgMessageIcon = imgIconMessageError;
            break;
        default:
            imgMessageIcon = imgIconMessageNeutral;
            break;
    }
    colorfg = 0xFFFFFFFF;
    // Draw Message Bar
    osd->DrawImage(imgMessageBarLeft, x0,y0,false);
    osd->DrawImage(imgMessageBarCenterX, x1, y0, false, x2 - x1, 1);
    osd->DrawImage(imgMessageBarRight, x2,y0,false);

    osd->DrawImage(imgMessageIcon, x1,y0, true);
#define messageIconW 30
    //display message
    osd->DrawText(x1 + messageIconW, y1, Text, colorfg, clrTransparent, font, x2 - x1 - messageIconW , 0, taCenter);
}

void ReelSkinDisplayMessage::Flush()
{
    if(osd)
       osd->Flush();
}

// --- cSkinReelOsd ----------------------------------------------------------

bool cSkinReelBaseOsd::HasChannelTimerRecording(const cChannel *Channel)
{
  // try to find current channel from timers
  for (cTimer * t = Timers.First(); t; t = Timers.Next(t)) {
    if ((t->Channel() == Channel) && t->Recording())
      return true;
  }
  return false;
}

int cSkinReelBaseOsd::DrawStatusSymbols(int x0, int xs, int top, int bottom, const cChannel *Channel /* = NULL */)
// What is x0 for ?
{
  if (!ReelConfig.showStatusSymbols)
    return xs;

  //int ys = top + (bottom - top - SymbolHeight) / 2;
  //const cFont *pFontLanguage = ReelConfig.GetFont(FONT_CILANGUAGE);

  cDevice *Device = cDevice::PrimaryDevice();
  eTrackType TrackType = Device->GetCurrentAudioTrack();
  const tTrackId *Track = Device->GetTrack(TrackType);
  if (Track) {
    int AudioMode = Device->GetAudioChannel();
    if (!(AudioMode >= 0 && AudioMode < MAX_AUDIO_BITMAPS))
      AudioMode = 0;

  }

  // draw recording symbol
 /* if (cRecordControls::Active()) {
    xs -= (bmRecording.Width() + SmallGap);
    //TODO? get current channel on primary device: Channel = Device->CurrentChannel()
    bool fRecording = Channel && HasChannelTimerRecording(Channel);
    osd->DrawBitmap(xs, ys, bmRecording,
                    Theme.Color(fRecording ? clrSymbolRecordBg : clrBottomBg),
                    Theme.Color(fRecording ? clrSymbolRecord : clrSymbolActive));
      debug("SymbolActive: bmRecording line %d", __LINE__);
  } */

  /* TODO? else {
    xs -= (bmRecording.Width() + SmallGap);
    osd->DrawBitmap(xs, ys, bmRecording,
                    Theme.Color(clrBottomBg), Theme.Color(clrSymbolInactive));
  }
  */

  return xs;
}

int cSkinReelBaseOsd::FixWidth(int w, int bpp, bool enlarge /* = true */)
{
  int a = 8 / bpp;
  int r = w & (a - 1);
  if (r == 0)
    return w;

  return enlarge ? (w + a -r) : (w - r);
}

// --- cSkinReel ----------------------------------------------------------

cSkinReel::cSkinReel() : cSkin("ReelNG", &::Theme)
{
  // Get the "classic" skin to be used as fallback skin if any of the OSD
  // menu fails to open.
  skinFallback = Skins.First();
  for (cSkin *Skin = Skins.First(); Skin; Skin = Skins.Next(Skin)) {
    if (strcmp(Skin->Name(), "classic") == 0) {
      skinFallback = Skin;
      break;
    }
  }
}

const char *cSkinReel::Description(void)
{
  return tr("Reel Classic");
}

cSkinDisplayChannel *cSkinReel::DisplayChannel(bool WithInfo)
{
  try {
    return new cSkinReelDisplayChannel(WithInfo);
  } catch(...) {
    return skinFallback->DisplayChannel(WithInfo);
  }
}

cSkinDisplayMenu *cSkinReel::DisplayMenu(void)
{
  try {
    return new cSkinReelDisplayMenu;
  } catch (...) {
    return skinFallback->DisplayMenu();
  }
}

cSkinDisplayReplay *cSkinReel::DisplayReplay(bool ModeOnly)
{
  try {
    return new cSkinReelDisplayReplay(ModeOnly);
  } catch (...) {
    return skinFallback->DisplayReplay(ModeOnly);
  }
}

cSkinDisplayVolume *cSkinReel::DisplayVolume(void)
{
  try {
    return new cSkinReelDisplayVolume;
  } catch (...) {
    return skinFallback->DisplayVolume();
  }
}

cSkinDisplayTracks *cSkinReel::DisplayTracks(const char *Title, int NumTracks, const char *const *Tracks)
{
  try {
    return new cSkinReelDisplayTracks(Title, NumTracks, Tracks);
  } catch (...) {
    return skinFallback->DisplayTracks(Title, NumTracks, Tracks);
  }
}

cSkinDisplayMessage *cSkinReel::DisplayMessage(void)
{
  try {
    return new ReelSkinDisplayMessage;
  } catch (...) {
    return skinFallback->DisplayMessage();
  }
}

// vim:et:sw=2:ts=2:
