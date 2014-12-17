/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// ReelSkin.c

#include "ReelSkin.h"
#include "ReelBoxMenu.h"

#include <vdr/tools.h>
#include <vdr/dvbdevice.h>
#include <vdr/font.h>
#include <vdr/osd.h>
#include <vdr/menu.h>
#include <vdr/themes.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <string>

#include "config.h"

#include "BspCommChannel.h"
#include "HdCommChannel.h"
#include "ReelBoxDevice.h"

#define Roundness   10
#define Gap          5
#define ScrollWidth  5
#define IMGICONWIDTH 20

enum { eNone, ePinProtect, eArchive, eBack, eFolder, eNewrecording, eCut, eNewCut, eArchivedCut, eRecord, eTimeractive, eSkipnextrecording, eNetwork, eDirShared, eMusic, eRunningNow };

#define IMG_ARROW_UP "arrow_up.png"
#define IMG_ARROW_DOWN "arrow_down.png"

#define IMG_A_TOP_LEFT "a_top_left.png"
#define IMG_A_TOP_BORDER "a_top_border.png"
#define IMG_A_TOP_RIGHT "a_top_right.png"
#define IMG_A_BOTTOM_LEFT "a_bottom_left.png"
#define IMG_A_BOTTOM_BORDER "a_bottom_border.png"
#define IMG_A_BOTTOM_RIGHT "a_bottom_right.png"

#define IMG_B_TOP_LEFT "b_top_left.png"
#define IMG_B_TOP_BORDER "b_top_border.png"
#define IMG_B_TOP_RIGHT "b_top_right.png"
#define IMG_B_BOTTOM_LEFT "b_bottom_left.png"
#define IMG_B_BOTTOM_BORDER "b_bottom_border.png"
#define IMG_B_BOTTOM_RIGHT "b_bottom_right.png"
#define IMG_B_BORDER_LEFT "b_border_left.png"
#define IMG_B_BORDER_RIGHT "b_border_right.png"

#define IMG_C_TOP_LEFT "c_top_left.png"
#define IMG_C_TOP_BORDER "c_top_border.png"
#define IMG_C_TOP_RIGHT "c_top_right.png"
#define IMG_C_BOTTOM_LEFT "c_bottom_left.png"
#define IMG_C_BOTTOM_BORDER "c_bottom_border.png"
#define IMG_C_BOTTOM_RIGHT "c_bottom_right.png"
#define IMG_C_BORDER_LEFT "c_border_left.png"
#define IMG_C_BORDER_RIGHT "c_border_right.png"

#define IMG_PRE_TOP_LEFT "pre_top_left.png"
#define IMG_PRE_TOP_BORDER "pre_top_border.png"
#define IMG_PRE_TOP_RIGHT "pre_top_right.png"
#define IMG_PRE_BOTTOM_LEFT "pre_bottom_left.png"
#define IMG_PRE_BOTTOM_BORDER "pre_bottom_border.png"
#define IMG_PRE_BOTTOM_RIGHT "pre_bottom_right.png"
#define IMG_PRE_BORDER_LEFT "pre_border_left.png"
#define IMG_PRE_BORDER_RIGHT "pre_border_right.png"

#define IMG_FFWD "ffwd.png"
#define IMG_FFWD1 "ffwd1.png"
#define IMG_FFWD2 "ffwd2.png"
#define IMG_FFWD3 "ffwd3.png"

#define IMG_FREW  "frew.png"
#define IMG_FREW1 "frew1.png"
#define IMG_FREW2 "frew2.png"
#define IMG_FREW3 "frew3.png"

#define IMG_PAUSE "pause.png"
#define IMG_PLAY "play.png"

#define IMG_SFWD  "sfwd.png"
#define IMG_SFWD1 "sfwd1.png"
#define IMG_SFWD2 "sfwd2.png"
#define IMG_SFWD3 "sfwd3.png"

#define IMG_SREW  "srew.png"
#define IMG_SREW1 "srew1.png"
#define IMG_SREW2 "srew2.png"
#define IMG_SREW3 "srew3.png"

#define IMG_TRICK "trick.png"

#define IMG_43 "43.png"
#define IMG_169 "169.png"

#define IMG_INFO_BORDER_LEFT "info_border_left.png"
#define IMG_INFO_BORDER_RIGHT "info_border_right.png"
#define IMG_STATUS_BORDER_LEFT "status_border_left.png"
#define IMG_STATUS_BORDER_RIGHT "status_border_right.png"
#define IMG_WARNING_BORDER_LEFT "warning_border_left.png"
#define IMG_WARNING_BORDER_RIGHT "warning_border_right.png"
#define IMG_ERROR_BORDER_LEFT "error_border_left.png"
#define IMG_ERROR_BORDER_RIGHT "error_border_right.png"

#define IMG_BUTTON_RED_1 "button_red_01.png"
#define IMG_BUTTON_RED_2 "button_red_02.png"
#define IMG_BUTTON_RED_3 "button_red_03.png"

#define IMG_BUTTON_GREEN_1 "button_green_01.png"
#define IMG_BUTTON_GREEN_2 "button_green_02.png"
#define IMG_BUTTON_GREEN_3 "button_green_03.png"

#define IMG_BUTTON_YELLOW_1 "button_yellow_01.png"
#define IMG_BUTTON_YELLOW_2 "button_yellow_02.png"
#define IMG_BUTTON_YELLOW_3 "button_yellow_03.png"

#define IMG_BUTTON_BLUE_1 "button_blue_01.png"
#define IMG_BUTTON_BLUE_2 "button_blue_02.png"
#define IMG_BUTTON_BLUE_3 "button_blue_03.png"

#define IMG_SYMBOL "symbol.png"
#define IMG_VOLUME "volume.png"

#define IMG_RECORDING_ON "recording.png"
#define IMG_RECORDING_OFF "recording_off.png"

#define IMG_ENCRYPTED_ON "encrypted.png"
#define IMG_ENCRYPTED_OFF "encrypted_off.png"

#define IMG_DOLBY_DIGITAL_ON "dolbydigital.png"
#define IMG_DOLBY_DIGITAL_OFF "dolbydigital_off.png"

#define IMG_AUDIO_ON "audio.png"
#define IMG_AUDIO_OFF "audio_off.png"

#define IMG_TELETEXT_ON "teletext.png"
#define IMG_TELETEXT_OFF "teletext_off.png"

#define IMG_RADIO "radio.png"

#define IMG_MARKER "marker.png"

#define IMG_MUTE "mute.png"

#define IMG_AUDIO_STEREO "audiostereo.png"
#define IMG_AUDIO_LEFT "audioleft.png"
#define IMG_AUDIO_RIGHT "audioright.png"

#define IMG_SIGNAL "signal.png"
#define IMG_VIACCESS "viaccess.png"
#define IMG_SKYCRYPT "skycrypt.png"
#define IMG_SECA "seca.png"
#define IMG_POWERVU "powervu.png"
#define IMG_NDS "nds.png"
#define IMG_NAGRAVISION "nagravision.png"
#define IMG_IRDETO "irdeto.png"
#define IMG_FREETOAIR "freetoair.png"
#define IMG_CRYPTOWORKS "cryptoworks.png"
#define IMG_CONAX "conax.png"
#define IMG_BETACRYPT "betacrypt.png"
#define IMG_TUNER1_ACTIVE "tuner1_active.png"
#define IMG_TUNER1_INACTIVE "tuner1_inactive.png"
#define IMG_TUNER2_ACTIVE "tuner2_active.png"
#define IMG_TUNER2_INACTIVE "tuner2_inactive.png"
#define IMG_QUESTIONMARK "questionmark.png"

int pgbar(const char *buff, int&i_start, int&i_end, int&progressCount)
{
 /*
  * "...text...[|||||   ]...text...
  */

 progressCount=0;
 int spaceCount = 0;

 int i=0;
 while (buff[i] && buff[i]!='[') i++; //find pbstart

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
          case ' ': spaceCount++; break;
          case '|': if(spaceCount>0) return 0; // not a progress bar
                    progressCount++; break;
          default: return 0 ; // not a progress bar
         } // end switch

 } // end for

 if(!buff[i]) return 0; // could not find ']'

 return spaceCount+progressCount; // return total

 }// end  pgbar()

static cTheme Theme;


namespace Reel
{

   enum
   {
     // highlineColor = 0xFF3E5572,
      clrBlue5     = 0xD9353E70,
      clrPreBkgnd  = 0xD9355E70,
      clrGreen5    = 0xD94B7035,
      clrRed5      = 0xD9704735,
      clrYellow5   = 0xD9706E35,
      clrWhiteTxt  = 0xFFF0F0F0,
      clrMainTxt   = 0xFFF0F0F0
   };

    THEME_CLR(Theme, clrBackground,             clrBlue5);
    THEME_CLR(Theme, clrButtonRedFg,            clrMainTxt);
    THEME_CLR(Theme, clrButtonRedBg,            clrRed5);
    THEME_CLR(Theme, clrButtonGreenFg,          clrMainTxt);
    THEME_CLR(Theme, clrButtonGreenBg,          clrGreen5);
    THEME_CLR(Theme, clrButtonYellowFg,         clrMainTxt);
    THEME_CLR(Theme, clrButtonYellowBg,         clrYellow5);
    THEME_CLR(Theme, clrButtonBlueFg,           clrMainTxt);
    THEME_CLR(Theme, clrButtonBlueBg,           clrBlue);
    THEME_CLR(Theme, clrMessageFrame,           clrYellow);
    THEME_CLR(Theme, clrMessageStatusFg,        clrMainTxt);
    THEME_CLR(Theme, clrMessageStatusBg,        clrGreen5);
    THEME_CLR(Theme, clrMessageInfoFg,          clrMainTxt);
    THEME_CLR(Theme, clrMessageInfoBg,          clrBlue5);
    THEME_CLR(Theme, clrMessageWarningFg,       clrMainTxt);
    THEME_CLR(Theme, clrMessageWarningBg,       clrYellow5);
    THEME_CLR(Theme, clrMessageErrorFg,         clrMainTxt);
    THEME_CLR(Theme, clrMessageErrorBg,         clrRed5);
    THEME_CLR(Theme, clrVolumeFrame,            clrYellow);
    THEME_CLR(Theme, clrVolumeSymbol,           clrBlack);
    THEME_CLR(Theme, clrVolumeBarUpper,         clrVolumeBarUpper);
    THEME_CLR(Theme, clrVolumeBarLower,         clrVolumeBarLower);
    THEME_CLR(Theme, clrVolumeText,             clrMainTxt);
    THEME_CLR(Theme, clrChannelFrame,           clrYellow);
    THEME_CLR(Theme, clrChannelName,            clrMainTxt);
    THEME_CLR(Theme, clrChannelDate,            clrMainTxt);
    THEME_CLR(Theme, clrChannelSymbolOn,        clrBlack);
    THEME_CLR(Theme, clrChannelSymbolOff,       clrChannelSymbolOff);
    THEME_CLR(Theme, clrChannelSymbolRecFg,     clrMainTxt);
    THEME_CLR(Theme, clrChannelSymbolRecBg,     clrRed);
    THEME_CLR(Theme, clrChannelEpgTime,         clrBlack);
    THEME_CLR(Theme, clrChannelEpgTitle,        clrCyan);
    THEME_CLR(Theme, clrChannelEpgShortText,    clrYellow);
    THEME_CLR(Theme, clrChannelEpgProgress,     clrMainTxt);         // hw epg progress in channelinfo
    THEME_CLR(Theme, clrChannelTimebarSeen,     clrYellow);
    THEME_CLR(Theme, clrChannelTimebarRest,     clrGray50);
    THEME_CLR(Theme, clrMenuFrame,              clrYellow);
    THEME_CLR(Theme, clrMenuTitle,              clrMenuTitle);
    THEME_CLR(Theme, clrMenuDate,               clrMenuDate);
    THEME_CLR(Theme, clrMenuItemCurrentFg,      clrBlack);
    THEME_CLR(Theme, clrMenuItemCurrentBg,      clrYellow);
    THEME_CLR(Theme, clrMenuItemSelectable,     clrYellow);
    THEME_CLR(Theme, clrMenuItemNonSelectable,  clrCyan);
    THEME_CLR(Theme, clrMenuEventTime,          clrYellow);
    THEME_CLR(Theme, clrMenuEventVps,           clrBlack);
    THEME_CLR(Theme, clrMenuEventTitle,         clrCyan);
    THEME_CLR(Theme, clrMenuEventShortText,     clrYellow);
    THEME_CLR(Theme, clrMenuEventDescription,   clrCyan);
    THEME_CLR(Theme, clrMenuScrollbarTotal,     clrYellow);
    THEME_CLR(Theme, clrMenuScrollbarShown,     clrCyan);
    THEME_CLR(Theme, clrMenuScrollbarArrow,     clrBlack);
    THEME_CLR(Theme, clrMenuText,               clrCyan);
    THEME_CLR(Theme, clrReplayFrame,            clrYellow);
    THEME_CLR(Theme, clrReplayTitle,            clrBlack);
    THEME_CLR(Theme, clrReplayMode,             clrBlack);
    THEME_CLR(Theme, clrReplayCurrent,          clrBlack);
    THEME_CLR(Theme, clrReplayTotal,            clrBlack);
    THEME_CLR(Theme, clrReplayJump,             clrBlack);
    THEME_CLR(Theme, clrReplayProgressSeen,     clrGreen);
    THEME_CLR(Theme, clrReplayProgressRest,     clrMainTxt);
    THEME_CLR(Theme, clrReplayProgressSelected, clrRed);
    THEME_CLR(Theme, clrReplayProgressMark,     clrBlack);
    THEME_CLR(Theme, clrReplayProgressCurrent,  clrRed);
    THEME_CLR(Theme, clrPreBackground,          clrPreBkgnd);
    THEME_CLR(Theme, clrMainText,               clrMainTxt);
    THEME_CLR(Theme, clrWhiteText,              clrWhiteTxt);
    THEME_CLR(Theme, clrSignalFg,               clrWhiteTxt);
    THEME_CLR(Theme, clrSignalBg,               clrBlack);

    static void Draw3TilesHor(cOsd *osd, UInt img1, UInt img2, UInt img3,
                              int l, int t, int r,
                              int img1Width, int img3Width)
    {
        int count = r - l - img1Width - img3Width;
        osd->DrawImage(img1, l, t, false);
        osd->DrawImage(img2, l + img1Width, t, false, count);
        osd->DrawImage(img3, r - img3Width, t, false);
    }

    enum
    {
        img_arrow_up,
        img_arrow_down,
        
        img_a_top_left,
        img_a_top_border,
        img_a_top_right,
        img_a_bottom_left,
        img_a_bottom_border,
        img_a_bottom_right,
        img_b_top_left,
        img_b_top_border,
        img_b_top_right,
        img_b_bottom_left,
        img_b_bottom_border,
        img_b_bottom_right,
        img_b_border_left,
        img_b_border_right,
        img_c_top_left,
        img_c_top_border,
        img_c_top_right,
        img_c_bottom_left,
        img_c_bottom_border,
        img_c_bottom_right,
        img_c_border_left,
        img_c_border_right,
        img_pre_top_left,
        img_pre_top_border,
        img_pre_top_right,
        img_pre_bottom_left,
        img_pre_bottom_border,
        img_pre_bottom_right,
        img_pre_border_left,
        img_pre_border_right,

        img_info_border_left,
        img_info_border_right,
        img_status_border_left,
        img_status_border_right,
        img_warning_border_left,
        img_warning_border_right,
        img_error_border_left,
        img_error_border_right,
        
        imgButtonRed1,
        imgButtonRed2,
        imgButtonRed3,
        imgButtonGreen1,
        imgButtonGreen2,
        imgButtonGreen3,
        imgButtonYellow1,
        imgButtonYellow2,
        imgButtonYellow3,
        imgButtonBlue1,
        imgButtonBlue2,
        imgButtonBlue3,
        imgSymbol,
        imgVolume,
        imgRecordingOn,
        imgRecordingOff,
        imgEncryptedOn,
        imgEncryptedOff,
        imgDolbyDigitalOn,
        imgDolbyDigitalOff,
        imgTeletextOn,
        imgTeletextOff,
        imgAudioOn,
        imgAudioOff,
        imgRadio,
        imgMarker,
        imgMute,
        imgAudioStereo,
        imgAudioLeft,
        imgAudioRight,
        imgFFwd,
        imgFFwd1,
        imgFFwd2,
        imgFFwd3,
        imgFRew,
        imgFRew1,
        imgFRew2,
        imgFRew3,
        imgPause,
        imgPlay,
        imgSFwd,
        imgSFwd1,
        imgSFwd2,
        imgSFwd3,
        imgSRew,
        imgSRew1,
        imgSRew2,
        imgSRew3,
        imgTrick,
        img43,
        img169,
        imgSignal,
        
        imgViaccess,
        imgSkycrypt,
        imgSeca,
        imgPowervu,
        imgNds,
        imgNagravision,
        imgIrdeto,
        imgFreetoair,
        imgCryptoworks,
        imgConax,
        imgBetacrypt,

        imgTuner1Active,
        imgTuner1Inactive,
        imgTuner2Active,
        imgTuner2Inactive,
        imgQuestionmark, 

    imgUnbufferedEnum,

    // image for Icons
    imgIconArchive,             // char 129
    imgIconCut,                 // char 133
    imgIconFolder,              // char 130
    imgIconFolderUp,            // char 128
    imgIconFavouriteFolder,     // char 131
    imgIconHd,                  // char 134
    imgIconPinProtect,          // char 80
//    imgIconMessage,
//    imgIconMuteOff,
//    imgIconMuteOn,
    imgIconNewrecording,        // char 132
    imgIconRecord,
    imgIconRunningnow,          // char 136
    imgIconSkipnextrecording,
    imgIconTimer,
    imgIconTimeractive,         // char 135
    imgIconTimerpart,
//    imgIconVolume,
    imgIconVps,
    imgIconZappingtimer,
    };

    enum
    {
        imgLeftWidth = 21,
        imgRightWidth = 23,
        imgTopHeight = 20,
        imgBottomHeight = 24,

        imgCaWidth        = 96,
        imgHighline1Width = 10,
        imgHighline3Width = 10,
        imgButton1Width   = 21,
        imgButton3Width   = 23,
        imgButtonHeight   = 47,
        imgSymbolWidth    = 60,
        imgSymbolHeight   = 60,
        imgVolumeWidth    = 15,
        imgChannelInfoIconWidth = 27,
        imgChannelInfoIconHeight = 18,
        imgMuteHeight       = 68,
        imgAudioChannelIconWidth = 27,
        imgAudioChannelIconHeight = 18,
        img43IconWidth = 27,
        img43IconHeight = 18,
        imgVolumeHeight     = 24,
        imgMarkerWidth      = 12,
        imgTrickSymbolWidth = 28,
        imgTrickSymbolHeight = 28,
        imgTrickWidth       = 56,
        imgTrickHeight      = 56,
        imgTunerWidth       = 19,
        imgQuestionmarkWidth = 22,
        imgSignalWidth       = 72
    };
// one should define imgPath string and m_path char array.
// imgPath is the directory underwhich all images are available
#define IMGPATH2(imgEnumNum, imgName) sprintf(m_path, "%s/%s", imgPath, imgName); \
        osd->SetImagePath(imgEnumNum, m_path)

#define IMGPATH(imgName, img, themePath, fd) sprintf(m_path, "%s/%s", themePath, img); \
    fd = open(m_path, O_RDONLY); \
    if(fd == -1) \
      sprintf(m_path, "%s/%s", REEL_SKIN_IMAGES_FOLDER, img); \
    else \
      close(fd); \
    osd->SetImagePath(imgName, m_path);

    static std::string lastPath;

    void ResetImagePaths() {
      lastPath = "";
    }

    static void SetImagePaths(cOsd *osd)
    {
            char path[128];
            char *m_path = (char*)malloc(256);
            //if(!strcmp(Theme.Name(), "default"))
            if(!strcmp(Theme.Name(), "Blue"))
                snprintf(path, sizeof(path), "%s", REEL_SKIN_IMAGES_FOLDER);
            else
            {
                DIR *dir = NULL;
                char pathTry[128];
                snprintf(pathTry, sizeof(pathTry), "%s/%s", DEFAULT_BASE, Theme.Name());
                if( (dir = opendir(pathTry))!= NULL)
                {
                    //path = pathTry;
                    snprintf(path, sizeof(path), "%s", pathTry);
                    closedir(dir);
                }
                else
                    snprintf(path, sizeof(path), "%s/%s", cThemes::GetThemesDirectory(), Theme.Name());
            }
        if (lastPath != path)
        {
            int fd;

            IMGPATH(img_arrow_up, IMG_ARROW_UP, path, fd);
            IMGPATH(img_arrow_down, IMG_ARROW_DOWN, path, fd);

            IMGPATH(img_a_top_left, IMG_A_TOP_LEFT, path, fd);
            IMGPATH(img_a_top_border, IMG_A_TOP_BORDER, path, fd);
            IMGPATH(img_a_top_right, IMG_A_TOP_RIGHT, path, fd);
            IMGPATH(img_a_bottom_left, IMG_A_BOTTOM_LEFT, path, fd);
            IMGPATH(img_a_bottom_border, IMG_A_BOTTOM_BORDER, path, fd);
            IMGPATH(img_a_bottom_right, IMG_A_BOTTOM_RIGHT, path, fd);
            IMGPATH(img_b_top_left, IMG_B_TOP_LEFT, path, fd);
            IMGPATH(img_b_top_border, IMG_B_TOP_BORDER, path, fd);
            IMGPATH(img_b_top_right, IMG_B_TOP_RIGHT, path, fd);
            IMGPATH(img_b_bottom_left, IMG_B_BOTTOM_LEFT, path, fd);
            IMGPATH(img_b_bottom_border, IMG_B_BOTTOM_BORDER, path, fd);
            IMGPATH(img_b_bottom_right, IMG_B_BOTTOM_RIGHT, path, fd);
            IMGPATH(img_b_border_left, IMG_B_BORDER_LEFT, path, fd);
            IMGPATH(img_b_border_right, IMG_B_BORDER_RIGHT, path, fd);
            IMGPATH(img_c_top_left, IMG_C_TOP_LEFT, path, fd);
            IMGPATH(img_c_top_border, IMG_C_TOP_BORDER, path, fd);
            IMGPATH(img_c_top_right, IMG_C_TOP_RIGHT, path, fd);
            IMGPATH(img_c_bottom_left, IMG_C_BOTTOM_LEFT, path, fd);
            IMGPATH(img_c_bottom_border, IMG_C_BOTTOM_BORDER, path, fd);
            IMGPATH(img_c_bottom_right, IMG_C_BOTTOM_RIGHT, path, fd);
            IMGPATH(img_c_border_left, IMG_B_BORDER_LEFT, path, fd);
            IMGPATH(img_c_border_right, IMG_B_BORDER_RIGHT, path, fd);
            IMGPATH(img_pre_top_left, IMG_PRE_TOP_LEFT, path, fd);
            IMGPATH(img_pre_top_border, IMG_PRE_TOP_BORDER, path, fd);
            IMGPATH(img_pre_top_right, IMG_PRE_TOP_RIGHT, path, fd);
            IMGPATH(img_pre_bottom_left, IMG_PRE_BOTTOM_LEFT, path, fd);
            IMGPATH(img_pre_bottom_border, IMG_PRE_BOTTOM_BORDER, path, fd);
            IMGPATH(img_pre_bottom_right, IMG_PRE_BOTTOM_RIGHT, path, fd);
            IMGPATH(img_pre_border_left, IMG_PRE_BORDER_LEFT, path, fd);
            IMGPATH(img_pre_border_right, IMG_PRE_BORDER_RIGHT, path, fd);

            IMGPATH(img_info_border_left, IMG_INFO_BORDER_LEFT, path, fd);
            IMGPATH(img_info_border_right, IMG_INFO_BORDER_RIGHT, path, fd);
            IMGPATH(img_status_border_left, IMG_STATUS_BORDER_LEFT, path, fd);
            IMGPATH(img_status_border_right, IMG_STATUS_BORDER_RIGHT, path, fd);
            IMGPATH(img_warning_border_left, IMG_WARNING_BORDER_LEFT, path, fd);
            IMGPATH(img_warning_border_right, IMG_WARNING_BORDER_RIGHT, path, fd);
            IMGPATH(img_error_border_left, IMG_ERROR_BORDER_LEFT, path, fd);
            IMGPATH(img_error_border_right, IMG_ERROR_BORDER_RIGHT, path, fd);

            IMGPATH(imgButtonRed1, IMG_BUTTON_RED_1, path, fd);
            IMGPATH(imgButtonRed2, IMG_BUTTON_RED_2, path, fd);
            IMGPATH(imgButtonRed3, IMG_BUTTON_RED_3, path, fd);
            IMGPATH(imgButtonGreen1, IMG_BUTTON_GREEN_1, path, fd);
            IMGPATH(imgButtonGreen2, IMG_BUTTON_GREEN_2, path, fd);
            IMGPATH(imgButtonGreen3, IMG_BUTTON_GREEN_3, path, fd);
            IMGPATH(imgButtonYellow1, IMG_BUTTON_YELLOW_1, path, fd);
            IMGPATH(imgButtonYellow2, IMG_BUTTON_YELLOW_2, path, fd);
            IMGPATH(imgButtonYellow3, IMG_BUTTON_YELLOW_3, path, fd);
            IMGPATH(imgButtonBlue1, IMG_BUTTON_BLUE_1, path, fd);
            IMGPATH(imgButtonBlue2, IMG_BUTTON_BLUE_2, path, fd);
            IMGPATH(imgButtonBlue3, IMG_BUTTON_BLUE_3, path, fd);
            IMGPATH(imgSymbol, IMG_SYMBOL, path, fd);
            IMGPATH(imgVolume, IMG_VOLUME, path, fd);
            IMGPATH(imgRecordingOn, IMG_RECORDING_ON, path, fd);
            IMGPATH(imgRecordingOff, IMG_RECORDING_OFF, path, fd);
            IMGPATH(imgEncryptedOn, IMG_ENCRYPTED_ON, path, fd);
            IMGPATH(imgEncryptedOff, IMG_ENCRYPTED_OFF, path, fd);
            IMGPATH(imgDolbyDigitalOn, IMG_DOLBY_DIGITAL_ON, path, fd);
            IMGPATH(imgDolbyDigitalOff, IMG_DOLBY_DIGITAL_OFF, path, fd);
            IMGPATH(imgAudioOn, IMG_AUDIO_ON, path, fd);
            IMGPATH(imgAudioOff, IMG_AUDIO_OFF, path, fd);
            IMGPATH(imgTeletextOn, IMG_TELETEXT_ON, path, fd);
            IMGPATH(imgTeletextOff, IMG_TELETEXT_OFF, path, fd);
            IMGPATH(imgRadio, IMG_RADIO, path, fd);
            IMGPATH(imgMarker, IMG_MARKER, path, fd);
            IMGPATH(imgMute, IMG_MUTE, path, fd);
            IMGPATH(imgAudioStereo, IMG_AUDIO_STEREO, path, fd);
            IMGPATH(imgAudioLeft, IMG_AUDIO_LEFT, path, fd);
            IMGPATH(imgAudioRight, IMG_AUDIO_RIGHT, path, fd);

            IMGPATH(imgFFwd,  IMG_FFWD, path, fd);
            IMGPATH(imgFFwd1, IMG_FFWD1, path, fd);
            IMGPATH(imgFFwd2, IMG_FFWD2, path, fd);
            IMGPATH(imgFFwd3, IMG_FFWD3, path, fd);
            IMGPATH(imgFRew,  IMG_FREW, path, fd);
            IMGPATH(imgFRew1, IMG_FREW1, path, fd);
            IMGPATH(imgFRew2, IMG_FREW2, path, fd);
            IMGPATH(imgFRew3, IMG_FREW3, path, fd);

            IMGPATH(imgPlay, IMG_PLAY, path, fd);
            IMGPATH(imgPause, IMG_PAUSE, path, fd);

            IMGPATH(imgSFwd,  IMG_SFWD, path, fd);
            IMGPATH(imgSFwd1, IMG_SFWD1, path, fd);
            IMGPATH(imgSFwd2, IMG_SFWD2, path, fd);
            IMGPATH(imgSFwd3, IMG_SFWD3, path, fd);
            IMGPATH(imgSRew,  IMG_SREW, path, fd);
            IMGPATH(imgSRew1, IMG_SREW1, path, fd);
            IMGPATH(imgSRew2, IMG_SREW2, path, fd);
            IMGPATH(imgSRew3, IMG_SREW3, path, fd);

            IMGPATH(imgTrick, IMG_TRICK, path, fd);
            IMGPATH(img43, IMG_43, path, fd);
            IMGPATH(img169, IMG_169, path, fd);

            IMGPATH(imgViaccess, IMG_VIACCESS, path, fd);
            IMGPATH(imgSkycrypt, IMG_SKYCRYPT, path, fd);
            IMGPATH(imgSeca, IMG_SECA, path, fd);
            IMGPATH(imgPowervu, IMG_POWERVU, path, fd);
            IMGPATH(imgNds, IMG_NDS, path, fd);
            IMGPATH(imgNagravision, IMG_NAGRAVISION, path, fd);
            IMGPATH(imgIrdeto, IMG_IRDETO, path, fd);
            IMGPATH(imgFreetoair, IMG_FREETOAIR, path, fd);
            IMGPATH(imgCryptoworks, IMG_CRYPTOWORKS, path, fd);
            IMGPATH(imgConax, IMG_CONAX, path, fd);
            IMGPATH(imgBetacrypt, IMG_BETACRYPT, path, fd);
            IMGPATH(imgTuner1Active, IMG_TUNER1_ACTIVE, path, fd);
            IMGPATH(imgTuner1Inactive, IMG_TUNER1_INACTIVE, path, fd);
            IMGPATH(imgTuner2Active, IMG_TUNER2_ACTIVE, path, fd);
            IMGPATH(imgTuner2Inactive, IMG_TUNER2_INACTIVE, path, fd);
            IMGPATH(imgQuestionmark, IMG_QUESTIONMARK, path, fd);
            IMGPATH(imgSignal, IMG_SIGNAL, path, fd);

    const char *imgPath = "/usr/share/reel/skinreel";

    // ICONS
    IMGPATH2(imgIconPinProtect,"icon_pin_protect_20.png");
    IMGPATH2(imgIconArchive,"icon_archive_20.png");
    //IMGPATH2(imgIconMusic,"icon_music_20.png");
    IMGPATH2(imgIconCut,"icon_cut_20.png");
    IMGPATH2(imgIconFolder,"icon_folder_20.png");
    IMGPATH2(imgIconFolderUp,"icon_folderup_20.png");
    IMGPATH2(imgIconFavouriteFolder,"icon_favouritefolder_20.png");
    IMGPATH2(imgIconHd,"icon_hd_20.png");
    //IMGPATH2(imgIconMessage,"icon_message_30.png");
    //IMGPATH2(imgIconMuteOff,"icon_muteoff_64.png");
    //IMGPATH2(imgIconMuteOn,"icon_muteon_64.png");
    IMGPATH2(imgIconNewrecording,"icon_newrecording_20.png");
    IMGPATH2(imgIconRecord,"icon_record_20.png");
    IMGPATH2(imgIconRunningnow,"icon_runningnow_20.png");
    IMGPATH2(imgIconSkipnextrecording,"icon_skipnextrecording_20.png");
    IMGPATH2(imgIconTimer,"icon_timer_20.png");
    IMGPATH2(imgIconTimeractive,"icon_timeractive_20.png");
    IMGPATH2(imgIconTimerpart,"icon_timerpart_20.png");
    //IMGPATH2(imgIconVolume,"icon_volume_64.png");
    IMGPATH2(imgIconVps,"icon_vps_20.png");
    IMGPATH2(imgIconZappingtimer,"icon_zappingtimer_20.png");

            lastPath = path;
        }
        //free(path);
        free(m_path);
    }

void DrawUnbufferedImage(cOsd* osd, std::string imgName, int xLeft, int yTop, int blend, int horRep=1, int vertRep=1)
{
  //TODO checks if the last image is the same => No SetImagePath
  std::string path;
  path = REEL_SKIN_IMAGES_FOLDER; //ReelConfig.GetImagesDir();
  path = path + "/" + imgName;
  osd->SetImagePath(imgUnbufferedEnum, path.c_str());
  osd->DrawImage(imgUnbufferedEnum, xLeft, yTop, blend, horRep, vertRep);
}

    // --- ReelSkinDisplayChannel ----------------------------------------------

    class ReelSkinDisplayChannel : public cSkinDisplayChannel
    {
    public:
        ReelSkinDisplayChannel(bool WithInfo);
        virtual ~ReelSkinDisplayChannel();
        virtual void SetChannel(const cChannel *Channel, int Number);
        virtual void SetEvents(const cEvent *Present, const cEvent *Following);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();
    private:
        cOsd *osd;
        cFont const *font;
        int x0, x0_1, x1, x2, x3, x4, x5, x6, x7;
        int ym, y0, y1, y2, y2_1, y3, y4, y5, y5_1, y6, y7;
        int x43;

        int xsDateTime, xeDateTime;
        int xsSignalBar, xeSignalBar;
        int xsSignalImg, xeSignalImg;
        int xsTrackDesc, xeTrackDesc;

        struct Tuner
        {
            cDvbDevice *dvbDevice;
            int         xs, xe;
            int         imgId;
        };

        Tuner tuners[2];
        int numTuners;

        int lineHeight;
        const cEvent *present;

        // lower bar display state
        std::string dateTime, trackDesc;
        int         strBarWidth, snrBarWidth;

        bool message;

        void DrawUpperBorder();
        void DrawLowerBarBkgnd();
        void DrawLowerBorder();
        void ResetLowerBarState();
        void Update43();
        void UpdateDateTime();
        void UpdateSeen();
        void UpdateSignalStrength();
        void UpdateTrackDescription();
        void UpdateTuners();
    };

    ReelSkinDisplayChannel::ReelSkinDisplayChannel(bool WithInfo)
    :x43(0), message(false)
    {
        present = NULL;
        font = cFont::GetFont(fontSml);
        lineHeight = font->Height();
        x0 = 0;
        x0_1 = x0 + imgLeftWidth;
        x1 = x0_1 + font->Width("00:00") + 10;
        x2 = x1 + ScrollWidth + 4; // + Roundness;
        x3 = x2; // + Gap;
        x7 = Setup.OSDWidth;
        x6 = x7 - imgRightWidth;// - lineHeight / 2;
        x5 = x6; // - lineHeight / 2;
        x4 = x5; // - Gap;
        y0 = 0;
        y2 = y0 + imgTopHeight;
        y2_1 = y2 + imgBottomHeight;
        y3 = y2_1 + imgTopHeight;
        y1 = y2 - lineHeight / 2;
        y4 = y3 + 4 * lineHeight;
        y5 = y4 + imgBottomHeight;
        y6 = y5 + imgTopHeight;
        y5_1 = y6 - lineHeight / 2;
        y7 = y6 + imgBottomHeight;

        xeDateTime = x6;
        cString date = DayDateTime();
        xsDateTime = xeDateTime - font->Width(*date);
        xeSignalBar = xsDateTime - 8;
        xsSignalBar = xeSignalBar - 40;
        xeSignalImg = xsSignalBar - 2;
        xsSignalImg = xeSignalImg - imgSignalWidth;

        numTuners = 0;
        int x = xsSignalImg - 3;
        for (int n = 0; n < cDevice::NumDevices() && numTuners < 2; ++n)
        {
            cDevice *device = cDevice::GetDevice(n);
            if (device)
            {
                cDvbDevice *dvbDevice = dynamic_cast<cDvbDevice *>(device);
                if (dvbDevice)
                {
                    tuners[numTuners].dvbDevice = dvbDevice;
                    x -= 2;
                    tuners[numTuners].xe = x;
                    x -= imgTunerWidth;
                    tuners[numTuners].xs = x;
                    ++numTuners;
                }
            }
        }
        if (numTuners == 2)
        {
            std::swap(tuners[0].xs, tuners[1].xs);
            std::swap(tuners[0].xe, tuners[1].xe);
        }
        xeTrackDesc = x - 2;
        xsTrackDesc = x0_1;

        ResetLowerBarState();

        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + (Setup.ChannelInfoPos ? 0 : Setup.OSDHeight - y7),Setup.OSDRandom);
        tArea Areas[] = { { 0, 0, x7 - 1, y7 - 1, 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        SetImagePaths(osd);

 
        DrawUpperBorder();
        DrawLowerBorder();
        DrawLowerBarBkgnd();
    }

    ReelSkinDisplayChannel::~ReelSkinDisplayChannel()
    {
        delete osd;
    }

    char const *const FRONTEND_DEVICE = "/dev/dvb/adapter%d/frontend%d";

    void ReelSkinDisplayChannel::DrawUpperBorder()
    {
        osd->DrawImage(img_a_top_left, x0, y0, false);
        osd->DrawImage(img_a_top_border, x0_1, y0, false, x6 - x0_1);
        osd->DrawImage(img_a_top_right, x6, y0, false);

        osd->DrawImage(img_a_bottom_left, x0, y2, false);
        osd->DrawImage(img_a_bottom_border, x0_1, y2, false, x6 - x0_1);
        osd->DrawImage(img_a_bottom_right, x6, y2, false);

        osd->DrawImage(img_pre_top_left, x0, y2_1, false);
        osd->DrawImage(img_pre_top_border, x0_1, y2_1, false, x1 - x0_1);
        osd->DrawImage(img_b_top_border, x1, y2_1, false, x6 - x1);
        osd->DrawImage(img_b_top_right, x6, y2_1, false);

        osd->DrawImage(img_pre_border_left, x0, y3, false, 1, y4 - y3);
        osd->DrawRectangle(x0_1, y3, x1 - 1, y4 - 1, Theme.Color(clrPreBackground));
        osd->DrawRectangle(x1, y3, x6 - 1,  y4 - 1, Theme.Color(clrBackground));
        osd->DrawImage(img_b_border_right, x6, y3, false, 1, y4 - y3);
    }

    void ReelSkinDisplayChannel::DrawLowerBarBkgnd()
    {
        osd->DrawImage(img_c_top_left, x0, y5, false);
        osd->DrawImage(img_c_top_border, x0_1, y5, false, x6 - x0_1);
        osd->DrawImage(img_c_top_right, x6, y5, false);

        osd->DrawImage(img_c_bottom_left, x0, y6, false);
        osd->DrawImage(img_c_bottom_border, x0_1, y6, false, x6 - x0_1);
        osd->DrawImage(img_c_bottom_right, x6, y6, false);

        osd->DrawImage(imgSignal, xsSignalImg, y5 + 13, true);
    }

    void ReelSkinDisplayChannel::DrawLowerBorder()
    {
        osd->DrawImage(img_pre_bottom_left, x0, y4, false);
        osd->DrawImage(img_pre_bottom_border, x0_1, y4, false, x1 - x0_1);
        osd->DrawImage(img_b_bottom_border, x1, y4, false, x6 - x1);
        osd->DrawImage(img_b_bottom_right, x6, y4, false);
    }

    void ReelSkinDisplayChannel::ResetLowerBarState()
    {
        dateTime = "";
        strBarWidth = 10000;
        snrBarWidth = 10000;
        tuners[0].imgId = -1;
        tuners[1].imgId = -1;
        trackDesc = "";
    }

    int GetSnr()
    {
        int const cardIndex = cDevice::ActualDevice()->CardIndex();
        uint16_t value = 0;
        char dev[256];

        sprintf(dev, FRONTEND_DEVICE, cardIndex, 0);
        int fe = open(dev, O_RDONLY | O_NONBLOCK);
        if (fe < 0)
            return value;
        ::ioctl(fe, FE_READ_SNR, &value);
        close(fe);

        return value;
    }

    void GetSignal(int &str, int &snr)
    {
        str = 0;
        snr = 0;
        int const cardIndex = cDevice::ActualDevice()->CardIndex();
        char dev[256];

        sprintf(dev, FRONTEND_DEVICE, cardIndex, 0);
        int fe = open(dev, O_RDONLY | O_NONBLOCK);
        if (fe < 0)
            return;
        ::ioctl(fe, FE_READ_SIGNAL_STRENGTH, &str);
        ::ioctl(fe, FE_READ_SNR, &snr);
        close(fe);
    }

    void ReelSkinDisplayChannel::SetChannel(const cChannel *Channel, int Number)
    {
        osd->DrawImage(img_a_top_border, x0_1, y0, false, x6 - x0_1);
        osd->DrawImage(img_a_bottom_border, x0_1, y2, false, x6 - x0_1);

        int x = x6 - 5;
        x43 = 0;
        if (Channel && !Channel->GroupSep())
        {
            int d = 3;
            int y = y0 + imgTopHeight - imgChannelInfoIconHeight / 2;
            int ca = Channel->Ca();
            bool rec = cRecordControls::Active();

            x -= imgChannelInfoIconWidth + d;
            osd->DrawImage(rec ? imgRecordingOn : imgRecordingOff, x, y, true);

            x -= imgChannelInfoIconWidth + d;
            osd->DrawImage(ca ? imgEncryptedOn : imgEncryptedOff, x, y, true);

            x -= imgChannelInfoIconWidth + d;
            osd->DrawImage(Channel->Dpid(0) ? imgDolbyDigitalOn : imgDolbyDigitalOff, x, y, true);

            x -= imgChannelInfoIconWidth + d;
            osd->DrawImage(Channel->Apid(1) ? imgAudioOn : imgAudioOff, x, y, true);

            if (Channel->Vpid())
            {
                x -= imgChannelInfoIconWidth + d;
                osd->DrawImage(Channel->Tpid() ? imgTeletextOn : imgTeletextOff, x, y, true);
            }
            else if (Channel->Apid(0))
            {
                x -= imgChannelInfoIconWidth + d;
                osd->DrawImage(Channel->Tpid() ? imgRadio : imgTeletextOff, x, y, true);
            }

            x -= img43IconWidth + d;
            x43 = x;
            x -= imgCaWidth + d;

            int imgCa;
            switch (ca)
            {
            case 0x0000:
                /* Reserved */
                imgCa = imgFreetoair;
                break;
            case 0x0100 ... 0x01FF:
                /* Canal Plus */
                imgCa = imgSeca;
                break;
            case 0x0500 ... 0x05FF:
                /* France Telecom */
                imgCa = imgViaccess;
                break;
            case 0x0600 ... 0x06FF:
                /* Irdeto */
                imgCa = imgIrdeto;
                break;
            case 0x0900 ... 0x09FF:
                /* News Datacom */
                imgCa = imgNds;
                break;
            case 0x0B00 ... 0x0BFF:
                /* Norwegian Telekom */
                imgCa = imgConax;
                break;
            case 0x0D00 ... 0x0DFF:
                /* Philips */
                imgCa = imgCryptoworks;
                break;
            case 0x0E00 ... 0x0EFF:
                /* Scientific Atlanta */
                imgCa = imgPowervu;
                break;
            case 0x1200 ... 0x12FF:
                /* BellVu Express */
                imgCa = imgNagravision;
                break;
            case 0x1700 ... 0x17FF:
                /* BetaTechnik */
                imgCa = imgBetacrypt;
                break;
            case 0x1800 ... 0x18FF:
                /* Kudelski SA */
                imgCa = imgNagravision;
                break;
            case 0x4A60 ... 0x4A6F:
                /* @Sky */
                imgCa = imgSkycrypt;
                break;
            default:
                imgCa = -1;
                break;
            }

            osd->DrawImage(imgCa, x, y, true);
        }
        osd->DrawText(x2, y1, ChannelString(Channel, Number), Theme.Color(clrChannelName), clrTransparent, font, x - x3 - 2);
    }

    void ReelSkinDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following)
    {
        present = Present;
        osd->DrawRectangle(x0_1, y3, x1 - 1, y4 - 1, Theme.Color(clrPreBackground));
        osd->DrawRectangle(x1, y3, x6 - 1,  y4 - 1, Theme.Color(clrBackground));

        for (int i = 0; i < 2; i++)
        {
            const cEvent *e = !i ? Present : Following;

            if (e) {
                char duration[32];
                int showTime = 0;
                time_t t = time(NULL);
                if (i == 0 && Setup.OSDRemainTime)
                {
                    showTime = e->StartTime() + e->Duration() - t;
                    showTime = showTime < 0 ? 0 : showTime; // fix negativ value
                    ::sprintf(duration, "- %d min", (showTime + 59)/60);
                }
                else
                {
                    showTime = e->Duration();
                    ::sprintf(duration, "%d min", (showTime + 59)/60);
                }

                int durationWidth = font->Width(duration);
                osd->DrawText(x0_1, y3 + 2 * i * lineHeight, e->GetTimeString(), Theme.Color(clrMainText), clrTransparent, font);
                osd->DrawText(x2, y3 + 2 * i * lineHeight, e->Title(), Theme.Color(clrChannelEpgTitle), clrTransparent, font, x6 - x2 - durationWidth - 4);
                osd->DrawText(x2, y3 + (2 * i + 1) * lineHeight, e->ShortText(), Theme.Color(clrChannelEpgShortText), clrTransparent, font, x6 - x2);
                osd->DrawText(x6 - durationWidth, y3 + 2 * i * lineHeight, duration, Theme.Color(clrMainText), clrTransparent, font);
            }
        }
    }

    void ReelSkinDisplayChannel::SetMessage(eMessageType type, const char *text)
    {
        if (text)
        {
            DrawLowerBorder();
            osd->DrawRectangle(x0, y5, x7 - 1, y7 - 1, clrTransparent);
            message = true;

            int imgId;
            UInt colorfg;
            switch(type)
            {
            case mtStatus:
                imgId = imgButtonGreen1;
                colorfg = Theme.Color(clrMessageStatusFg);
                break;
            case mtWarning:
                imgId = imgButtonYellow1;
                colorfg = Theme.Color(clrMessageWarningFg);
                break;
            case mtError:
                imgId = imgButtonRed1;
                colorfg = Theme.Color(clrMessageErrorFg);
                break;
            default:
                imgId = imgButtonBlue1;
                colorfg = Theme.Color(clrMessageInfoFg);
                break;
            }
            int y = y7 - imgButtonHeight;
            int xs = x0 + imgButton1Width;
            int xe = x7 - imgButton3Width;
            osd->DrawImage(imgId, x0, y, true);
            osd->DrawImage(imgId + 1, xs, y, true, xe - xs);
            osd->DrawImage(imgId + 2, xe, y, true);
            osd->DrawText(xs, y + 10, text, colorfg, clrTransparent, font, xe - xs, 0, taCenter);
        }
        else
        {
            ResetLowerBarState();
            DrawLowerBorder();
            DrawLowerBarBkgnd();
            message = false;
        }
    }

    void ReelSkinDisplayChannel::Update43()
    {
        if (x43)
        {
            osd->DrawImage(img_a_top_border, x43, y0, false, img43IconWidth);
            osd->DrawImage(img_a_bottom_border, x43, y2, false, img43IconWidth);

            int width = 0;
            int height = 0;
            if (!RBSetup.usehdext) 
            {
                bspd_data_t volatile *bspd = Bsp::BspCommChannel::Instance().bspd;

                width = bspd->video_attributes[0].imgAspectRatioX;
                height = bspd->video_attributes[0].imgAspectRatioY;
            } else {
//         printf ("\033[0;41m aspect.w = %i \033[0m\n", HdCommChannel::hda->video_mode.aspect.w );
//         printf ("\033[0;41m aspect.h = %i \033[0m\n", HdCommChannel::hda->video_mode.aspect.h );
                width = HdCommChannel::hda->source_aspect.w;
                height = HdCommChannel::hda->source_aspect.h;
            }
            
            if (width > 0 || height > 0)
                osd->DrawImage((width == 16 && height == 9) ? img169 : img43, x43, y0 + imgTopHeight - imgChannelInfoIconHeight / 2, true);
        }
    }

    void ReelSkinDisplayChannel::UpdateDateTime()
    {
        cString dt = DayDateTime();

        if (dateTime != implicit_cast<char const *>(dt))
        {
            dateTime = implicit_cast<char const *>(dt);
            osd->DrawImage(img_c_top_border, xsDateTime, y5, false, xeDateTime - xsDateTime);
            osd->DrawImage(img_c_bottom_border, xsDateTime, y6, false, xeDateTime - xsDateTime);
            osd->DrawText(xsDateTime, y5_1, dateTime.c_str(), Theme.Color(clrChannelDate), clrTransparent, font, xeDateTime - xsDateTime);
        }
    }

    void ReelSkinDisplayChannel::UpdateTrackDescription()
    {
        cDevice *device = cDevice::PrimaryDevice();
        tTrackId const *track = device->GetTrack(device->GetCurrentAudioTrack());

        if (track && trackDesc != track->description)
        {
            trackDesc = track->description;

            osd->DrawImage(img_c_top_border, xsTrackDesc, y5, false, xeTrackDesc - xsTrackDesc);
            osd->DrawImage(img_c_bottom_border, xsTrackDesc, y6, false, xeTrackDesc - xsTrackDesc);
            osd->DrawText(xsTrackDesc, y5_1, trackDesc.c_str(), Theme.Color(clrMainText), clrTransparent, font, xeTrackDesc);
        }
    }

    void ReelSkinDisplayChannel::UpdateSeen()
    {
        int seen = 0;
        int bw = 40;
        int x = x0_1 + 4;
        int y = y3 + lineHeight;
        int h = 6;
        if (present)
        {
            osd->DrawRectangle(x, y, x + bw - 1, y + h + 1, Theme.Color(clrChannelTimebarRest));
            time_t t = time(NULL);
            if (t > present->StartTime())
            seen = min(bw, int(bw * double(t - present->StartTime()) / present->Duration()));
            if (seen)
            {
                osd->DrawRectangle(x, y + 1, x + seen - 1, y + h - 1, Theme.Color(clrChannelTimebarSeen));
            }
        }
        else
        {
            osd->DrawRectangle(x, y, x + bw - 1, y + h + 1, Theme.Color(clrPreBackground));
        }
    }

    void ReelSkinDisplayChannel::UpdateSignalStrength()
    {
        int str, snr;
        GetSignal(str, snr);
        int bw = xeSignalBar - xsSignalBar;
        str = str * bw / 0xFFFF;
        snr = snr * bw / 0xFFFF;
        if (str != strBarWidth || snr != snrBarWidth)
        {
            strBarWidth = str;
            snrBarWidth = snr;

            int y = y5_1 + 8;
            int yStr = y - 6;
            int ySnr = y + 6;
            int h = 6;
            osd->DrawImage(img_c_top_border, xsSignalBar, y5, false, xeSignalBar - xsSignalBar);
            osd->DrawImage(img_c_bottom_border, xsSignalBar, y6, false, xeSignalBar - xsSignalBar);
            osd->DrawRectangle(xsSignalBar, yStr, xeSignalBar - 1, yStr + h + 1, Theme.Color(clrSignalBg));
            osd->DrawRectangle(xsSignalBar, ySnr, xeSignalBar - 1, ySnr + h + 1, Theme.Color(clrSignalBg));
            if (str)
            {
                osd->DrawRectangle(xsSignalBar, yStr + 1, xsSignalBar + str - 1, yStr + h - 1, Theme.Color(clrSignalFg));
            }
            if (snr)
            {
                osd->DrawRectangle(xsSignalBar, ySnr + 1, xsSignalBar + snr - 1, ySnr + h - 1, Theme.Color(clrSignalFg));
            }
        }
    }

    void ReelSkinDisplayChannel::UpdateTuners()
    {
        for (int n = 0; n < numTuners; ++n)
        {
            cDvbDevice *dvbDevice = tuners[n].dvbDevice;
            int const imgId = imgTuner1Active + 2 * n + (dvbDevice->HasLock() ? 0 : 1);
            if (tuners[n].imgId != imgId)
            {
                tuners[n].imgId = imgId;

                osd->DrawImage(img_c_top_border, tuners[n].xs, y5, false, tuners[n].xe - tuners[n].xs);
                osd->DrawImage(img_c_bottom_border, tuners[n].xs, y6, false, tuners[n].xe - tuners[n].xs);
                osd->DrawImage(imgId, tuners[n].xs, y5_1 + 3, true);
            }
        }
    }

    void ReelSkinDisplayChannel::Flush()
    {
        Update43();
        UpdateSeen();

        if (!message)
        {
            // ---------
            UpdateDateTime();
            UpdateTrackDescription();
            UpdateSignalStrength();
            UpdateTuners();
        }

        osd->Flush();
    }

    // --- ReelSkinMenu -------------------------------------------------

    int const symbolOffset = 3;

    class ReelSkinMenu : public cSkinDisplayMenu
    {
    private:
        UInt themeClrBackground, themeClrPreBackground, themeClrMainText; 
        UInt themeClrButtonRedFg, themeClrButtonGreenFg, themeClrButtonYellowFg, themeClrButtonBlueFg;
        UInt themeClrMessageStatusBg, themeClrMessageWarningBg, themeClrMessageErrorBg, themeClrMessageInfoBg;
        UInt themeClrMessageStatusFg, themeClrMessageWarningFg, themeClrMessageErrorFg, themeClrMessageInfoFg;
        UInt themeClrMenuEventTime, themeClrMenuEventTitle, themeClrMenuEventShortText;
        UInt themeClrMenuEventDescription, themeClrVolumeBarUpper, themeClrVolumeBarLower;
        UInt themeClrMenuDate, themeClrMenuTitle;

        cOsd *osd;
        int x0, x1, x2, x3, x4, x5, x6, x7;
        int y0, y1, y2, y3, y4, y5, y6, y7;
        int titleBarHeight;
        int lineHeight;
        tColor frameColor;
        bool message;
        void DrawButton(UInt imgButton1, int x, int width);
        void DrawTitleBar();
        void SetScrollBar();  // hw 
     public:

        ReelSkinMenu();
        virtual ~ReelSkinMenu();
        virtual void Scroll (bool Up, bool Page); // hw
        virtual int MaxItems();
        virtual void Clear();
        virtual void SetTitle(const char *Title);
        virtual void SetButtons(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void SetItem(const char *Text, int Index, bool Current, bool Selectable);
        virtual void SetEvent(const cEvent *Event);
        virtual void SetRecording(const cRecording *Recording);
        virtual void SetText(const char *Text, bool FixedFont);
        virtual int GetTextAreaWidth(void) const;
        virtual const cFont *GetTextAreaFont(bool FixedFont) const;
        virtual void Flush(void);
    };

    void ReelSkinMenu::DrawButton(UInt imgButton1, int x, int width)
    {
        int const y = y6 - imgButtonHeight / 2;
        osd->DrawImage(imgButton1, x, y, true);
        osd->DrawImage(imgButton1 + 1, x + imgButton1Width, y, true, width - imgButton1Width - imgButton3Width);
        osd->DrawImage(imgButton1 + 2, x + width - imgButton3Width, y, true);
    }

    void ReelSkinMenu::DrawTitleBar()
    {
        osd->DrawRectangle(x0, y0, x7 - 1, y2 + symbolOffset - 1, clrTransparent);
        Draw3TilesHor(osd, img_a_top_left, img_a_top_border, img_a_top_right, x0, y0, x7,
                      imgLeftWidth, imgRightWidth);
        Draw3TilesHor(osd, img_a_bottom_left, img_a_bottom_border, img_a_bottom_right, x0, y0 + imgTopHeight, x7,
                      imgLeftWidth, imgRightWidth);
        Draw3TilesHor(osd, img_b_top_left, img_b_top_border, img_b_top_right, x0, y1, x7,
                      imgLeftWidth, imgRightWidth);
        if ( Setup.OSDUseSymbol == true )
        {
          osd->DrawImage(imgSymbol, x0 + imgLeftWidth, y0 + 5, true);
        }
        osd->DrawImage(img_b_border_left, x0, y2, false, 1, symbolOffset);
        osd->DrawRectangle(x1, y2, x6 - 1, y2 + symbolOffset - 1, themeClrBackground);
        osd->DrawImage(img_b_border_right, x6, y2, false, 1, symbolOffset);
    }

    ReelSkinMenu::ReelSkinMenu()
    {
        themeClrBackground = Theme.Color(clrBackground);
        themeClrMainText = Theme.Color(clrMainText);
        themeClrButtonRedFg = Theme.Color(clrButtonRedFg);
        themeClrButtonGreenFg = Theme.Color(clrButtonGreenFg);
        themeClrButtonYellowFg = Theme.Color(clrButtonYellowFg);
        themeClrButtonBlueFg = Theme.Color(clrButtonBlueFg);
        themeClrMessageStatusBg = Theme.Color(clrMessageStatusBg);
        themeClrMessageWarningBg = Theme.Color(clrMessageWarningBg);
        themeClrMessageErrorBg = Theme.Color(clrMessageErrorBg);
        themeClrMessageInfoBg = Theme.Color(clrMessageInfoBg);
        themeClrMessageStatusFg = Theme.Color(clrMessageStatusFg);
        themeClrMessageWarningFg = Theme.Color(clrMessageWarningFg);
        themeClrMessageErrorFg = Theme.Color(clrMessageErrorFg);
        themeClrMessageInfoFg = Theme.Color(clrMessageInfoFg);
        themeClrPreBackground = Theme.Color(clrPreBackground);
        themeClrMenuEventTime = Theme.Color(clrMenuEventTime);
        themeClrMenuEventTitle = Theme.Color(clrMenuEventTitle);
        themeClrMenuEventShortText = Theme.Color(clrMenuEventShortText);
        themeClrMenuEventDescription = Theme.Color(clrMenuEventDescription);
        themeClrMenuTitle = Theme.Color(clrMenuTitle);
        themeClrMenuDate = Theme.Color(clrMenuDate);


        const cFont *font = cFont::GetFont(fontSml);
        lineHeight = font->Height();
        frameColor = Theme.Color(clrMenuFrame);
        message = false;
        x0 = 0;
        x1 = imgLeftWidth;
        x2 = x1 + imgHighline1Width;
        x3 = x2;
        x7 = Setup.OSDWidth;
        x6 = x7 - imgRightWidth;
        x5 = x6 - imgHighline3Width;
        x4 = x5;
        y0 = 0;
        titleBarHeight = imgTopHeight + imgBottomHeight;
        y1 = y0 + titleBarHeight;
        y2 = y1 + imgTopHeight;
        y7 = Setup.OSDHeight;
        y6 = y7 - imgButtonHeight / 2 - 1;
        y5 = y6 - imgBottomHeight;
        y4 = y5 - lineHeight;
        y3 = y4; // - Possible gap to msg line
        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop, Setup.OSDRandom);
        tArea Areas[] = { { x0, 0, x7 - 1, y7 - 1, 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        SetImagePaths(osd);

        osd->DrawRectangle(x0, 0, x7 - 1, y7 - 1, clrTransparent);

        DrawTitleBar();

        Draw3TilesHor(osd, img_b_bottom_left, img_b_bottom_border, img_b_bottom_right, x0, y1, x7,
                      imgLeftWidth, imgRightWidth);
        osd->DrawImage(img_b_border_left, x0, y2 + symbolOffset, false, 1, y6 - y2);
        osd->DrawRectangle(x1, y2 + symbolOffset, x6 - 1, y6 - 1, themeClrBackground);
        osd->DrawImage(img_b_border_right, x6, y2 + symbolOffset, false, 1, y6 - y2);
        Draw3TilesHor(osd, img_b_bottom_left, img_b_bottom_border, img_b_bottom_right, x0, y5, x7,
                      imgLeftWidth, imgRightWidth);
    }

    ReelSkinMenu::~ReelSkinMenu()
    {
        delete osd;
    }
    // --- ReelSkinMenuScrollBar ----------------------------------------------- // hw 

    void ReelSkinMenu::SetScrollBar(void)
    {
    // If ScroolBar needed?
        if (textScroller.CanScroll()) {
            int ScrollTop = textScroller.Top();
            int ScrollBottom = ScrollTop + textScroller.Height();
            int ShownTop = ScrollTop + (ScrollBottom - ScrollTop) * textScroller.Offset() / textScroller.Total();
            int ShownBottom = ShownTop + (ScrollBottom - ScrollTop) * textScroller.Shown() / textScroller.Total();
            
            int xright = x6 - 5;
            int ArrowWidth = 15;
            int ArrowHeight = 15;
            int ScrollBarWidth = Setup.OSDScrollBarWidth;
            int ArrowDist = 2;                                          // vertical Distance Arrow to ScrollBar
            int ScrollBarDist = 0;
            
            if ( ScrollBarWidth == 15)  // Div.->Null
                ScrollBarDist = 0;
            else
                ScrollBarDist = ((ArrowWidth - ScrollBarWidth ) / 2); // centers ScrollBar to Arrow
            
            if (textScroller.CanScrollUp())  // draw arrow_up
            {
                osd->DrawImage(img_arrow_up, xright, ScrollTop, true);
            }
            else  // clear arrow_up
            {
                osd->DrawRectangle(xright, ScrollTop , xright + ArrowWidth, ScrollTop + ArrowHeight, Theme.Color(clrBackground));
            }

            if (textScroller.CanScrollDown())  // draw arrow_down
            {
                osd->DrawImage(img_arrow_down, xright, ScrollBottom, true);
            }
            else  // clear arrow_down
            {
                osd->DrawRectangle(xright, ScrollBottom, xright + ArrowWidth, ScrollBottom + ArrowHeight, Theme.Color(clrBackground));
            }
            // draw ScrollBar
            osd->DrawRectangle(xright + ScrollBarDist, ScrollTop + ArrowHeight + ArrowDist, xright + ScrollBarDist + ScrollBarWidth - 1, ScrollBottom - ArrowDist - 1, Theme.Color(clrMenuScrollbarTotal));
            osd->DrawRectangle(xright + ScrollBarDist, ShownTop + ArrowHeight + ArrowDist, xright + ScrollBarDist + ScrollBarWidth - 1, ShownBottom - ArrowDist - 1, Theme.Color(clrMenuScrollbarShown));
        }
    }

    void ReelSkinMenu::Scroll(bool Up, bool Page)  // hw
    {
        cSkinDisplayMenu::Scroll(Up, Page);
        SetScrollBar();
    }

    int ReelSkinMenu::MaxItems(void)
    {
        return (y4 - y2 - 2) / lineHeight;
    }

    void ReelSkinMenu::Clear(void)
    {
        textScroller.Reset();
        osd->DrawImage(img_b_border_left, x0, y2 + symbolOffset, false, 1, y6 - y2 - symbolOffset);
        osd->DrawRectangle(x1, y2 + symbolOffset, x6 - 1, y5 - 1, themeClrBackground);
        osd->DrawImage(img_b_border_right, x6, y2 + symbolOffset, false, 1, y6 - y2 - symbolOffset);
    }

    void ReelSkinMenu::SetTitle(const char *Title)
    {
        DrawTitleBar();
        const cFont *font = cFont::GetFont(fontSml);
        int y = y0 + (titleBarHeight - font->Height()) / 2;
        cString date = DayDateTime();
        int w = font->Width(*date);

#define HIDDENSTRING "menunormalhidden$"
        if (Title) {
           char* pos = (char *) strstr(Title, HIDDENSTRING);
           if (pos>0) {
              osd->DrawText(x3 + 80, y, Title+strlen(HIDDENSTRING), themeClrMenuTitle, clrTransparent, font, x4 - w - x3 - 80);
           } else 
              osd->DrawText(x3 + 80, y, Title, themeClrMenuTitle, clrTransparent, font, x4 - w - x3 - 80);
        }

        osd->DrawText(x4 - w, y, *date, themeClrMenuDate, clrTransparent, font);
    }

    void ReelSkinMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue)
    {
        osd->DrawRectangle(x0, y5, x7 - 1, y7 - 1, clrTransparent);

        Draw3TilesHor(osd, img_b_bottom_left, img_b_bottom_border, img_b_bottom_right, x0, y5, x7,
                      imgLeftWidth, imgRightWidth);

        int xs = x1;
        int xe = x6;

        int buttonCount = 0;

        if (Red)
        {
            ++ buttonCount;
        }
        if (Green)
        {
            ++ buttonCount;
        }
        if (Yellow)
        {
            ++ buttonCount;
        }
        if (Blue)
        {
            ++ buttonCount;
        }


        if (buttonCount)
        {
            int width = (xe - xs) / buttonCount;

            int x = xs;
            int yt = y6 - lineHeight / 2 - 1;
            int tw = width - imgButton1Width - imgButton3Width;

            const cFont *font = cFont::GetFont(fontSml);


            if (Red)
            {
                DrawButton(imgButtonRed1, x, width);
                osd->DrawText(x + imgButton1Width, yt, Red, themeClrButtonRedFg, clrTransparent, font, tw, 0, taCenter);
                x += width;
            }
            if (Green)
            {
                DrawButton(imgButtonGreen1, x, width);
                osd->DrawText(x + imgButton1Width, yt, Green, themeClrButtonGreenFg, clrTransparent, font, tw, 0, taCenter);
                x += width;
            }
            if (Yellow)
            {
                DrawButton(imgButtonYellow1, x, width);
                osd->DrawText(x + imgButton1Width, yt, Yellow, themeClrButtonYellowFg, clrTransparent, font, tw, 0, taCenter);
                x += width;
            }
            if (Blue)
            {
                DrawButton(imgButtonBlue1, x, width);
                osd->DrawText(x + imgButton1Width, yt, Blue, themeClrButtonBlueFg, clrTransparent, font, tw, 0, taCenter);
            }
        }
    }

    void ReelSkinMenu::SetMessage(eMessageType type, const char *text)
    {
        cFont const *font = cFont::GetFont(fontSml);
        tColor color = 0;
        if (text)
        {
            switch(type)
            {
            case mtStatus:
                if (strcmp(text, "(?)") == 0)
                {
                    osd->DrawImage(img_b_border_left, x0, y4, false, 1, y5 - y4);
                    osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrBackground);
                    osd->DrawImage(img_b_border_right, x6, y4, false, 1, y5 - y4);
                    osd->DrawImage(imgQuestionmark, x6 - imgQuestionmarkWidth, y4, true);
                }
                else
                {
                    osd->DrawImage(img_status_border_left, x0, y4, false, 1, y5 - y4);
                    osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrMessageStatusBg);
                    osd->DrawImage(img_status_border_right, x6, y4, false, 1, y5 - y4);
                    //osd->DrawText(x1, y4, text, themeClrMessageStatusFg, clrTransparent, font, x6 - x1, 0, taCenter);
                    color = themeClrMessageStatusFg;
                }
                break;
            case mtWarning:
                osd->DrawImage(img_warning_border_left, x0, y4, false, 1, y5 - y4);
                osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrMessageWarningBg);
                osd->DrawImage(img_warning_border_right, x6, y4, false, 1, y5 - y4);
                //osd->DrawText(x1, y4, text, themeClrMessageWarningFg, clrTransparent, font, x6 - x1, 0, taCenter);
                color = themeClrMessageWarningFg;
                break;
            case mtError:
                osd->DrawImage(img_error_border_left, x0, y4, false, 1, y5 - y4);
                osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrMessageErrorBg);
                osd->DrawImage(img_error_border_right, x6, y4, false, 1, y5 - y4);
                //osd->DrawText(x1, y4, text, themeClrMessageErrorFg, clrTransparent, font, x6 - x1, 0, taCenter);
                color = themeClrMessageErrorFg;
                break;
            default:
                osd->DrawImage(img_info_border_left, x0, y4, false, 1, y5 - y4);
                osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrMessageInfoBg);
                osd->DrawImage(img_info_border_right, x6, y4, false, 1, y5 - y4);
                //osd->DrawText(x1, y4, text, themeClrMessageInfoFg, clrTransparent, font, x6 - x1, 0, taCenter);
                color = themeClrMessageInfoFg;
                break;
            }

                int pb_start, pb_end, progressCount; // Progress Bar
                int offset=0;
                int pbtotal = pgbar(text, pb_start, pb_end, progressCount); // get the length of progress bar if any
                int _x_ = (x6-x1 - font->Width(text))/2;

        //        printf("Text \"%s\", %d Message width \n", Text,  MessageWidth);
                if (pbtotal < 3) // donot show pbar! [], [ ], [  ] will be displayed as such
                    {
		     if(strcmp(text, "(?)"))
                        osd->DrawText( x1 , y4, text, color, clrTransparent, font, x6 - x1, 0, taCenter );
                    } // end if
                    else
                    {
                      char *text1;
                      if (pb_start>0) // some text before progress bar
                      {
                        text1 = new char[pb_start+1];
                        strncpy(text1,text, pb_start); // i_start is the index of '['
                        text1[pb_start] = '\0';

                        offset = font->Width(text1);
                        osd->DrawText( _x_ , y4, text1, color, clrTransparent, font, offset, 0, taCenter );
                        delete text1;
                      }

                      // draw progress bar
                      int pblength = pbtotal * font->Width('|');
                      int pbthickness = (int)( 0.5*font->Height("|") );

                      int pbTop = (y4 + y5 - pbthickness)/2;
                      // background
                      osd->DrawRectangle( _x_ + offset, pbTop, _x_ + offset + pblength , pbTop + pbthickness, themeClrBackground/*Theme.Color(clrProgressBarBg)*/  );
                      // foreground
                      osd->DrawRectangle( _x_ + offset, pbTop, _x_ + offset + (int) ( (progressCount*1.0/pbtotal) * pblength ),
                                          pbTop + pbthickness, color/*Theme.Color(clrProgressBarFg)*/);

                     offset += pblength +  font->Width(' ');
                     if (pb_end + 1 < (int)strlen(text))// display rest of the text
                     {
                       int len = strlen(text) - pb_end ; // including '\0'
                       text1 = new char[len];
                       strcpy(text1, text+pb_end+1);
                       text1[len] = '\0';
                       osd->DrawText( _x_ + offset , y5-1, text1, color, clrTransparent, font, len , 0, taCenter );
                       delete text1;
                     }
                    } // end else
        }
        else
        {
            osd->DrawImage(img_b_border_left, x0, y4, false, 1, y5 - y4);
            osd->DrawRectangle(x1, y4, x6 - 1, y5  - 1, themeClrBackground);
            osd->DrawImage(img_b_border_right, x6, y4, false, 1, y5 - y4);
        }
    }

    void ReelSkinMenu::SetItem(char const *text, int Index, bool Current, bool Selectable)
    {
        int y = y2 + Index * lineHeight + 2;
        const cFont *font = cFont::GetFont(fontSml);
        int xs = x1;
        int xe = x6;
        char s[512];

        if (Current)
        {
            osd->DrawImage(img_pre_border_left, x0, y, false, 1, lineHeight);
            osd->DrawImage(img_pre_border_right, x6, y, false, 1, lineHeight);
            // osd->DrawImage(imgHighline1, x0, y, true, 1, lineHeight);
            // osd->DrawImage(imgHighline3, x7 - imgHighline3Width, y, true, 1, lineHeight);
            osd->DrawRectangle(x1, y, x6 - 1, y + lineHeight - 1, themeClrPreBackground);
            osd->DrawImage(imgMarker, x0 + 8, y, true);
        }
        else
        {
            osd->DrawImage(img_b_border_left, x0, y, false, 1, lineHeight);
            osd->DrawRectangle(xs, y, xe - 1, y + lineHeight - 1, themeClrBackground);
            osd->DrawImage(img_b_border_right, x6, y, false, 1, lineHeight);
        }
        for (int n = 0; n < MaxTabs; ++n)
        {
        int singlechar = eNone;
        bool iseventinfo = false;
        bool isnewrecording = false;
        bool isarchived = false;
        bool isHD = false;
        bool isFavouriteFolder = false;
        bool isFolder = false;
        bool isprogressbar = false;
        bool isMusic = false;
        bool isCheckmark = false;
        bool isRunningnow = false;
        bool isSharedDir = false;
        bool isFolderUp = false;
        bool isArchive = false;
        int now = 0, total = 0;

        int xt = x2 + Tab(n);

        char const *s1 = GetTabbedText(text, n);
    if (s1)
        strncpy(s,s1,512);
    else
      s[0]='\0';

            if (s1)
            {
//#define PROGBARS 1
#ifdef PROGBARS
              bool isprogress = false;
              if (strlen(s) > 5 && s[0] == '[' && s[strlen(s) - 1] == ']') {
                const char *p = s + 1;
                isprogress = true;
                for (; *p != ']'; ++p) {
                  if (*p != ' ' && *p != '|') {
                    isprogress = false;
                    break;
                  }
                }
              }
#define SEPBARS 1
#ifdef SEPBARS
              bool issepbar = false;
              int len = strlen(s);
              if(len > 4){
                issepbar = true;
                for (int i = 0; i < len; i++) {
                  if(s[i] != '-'){
                    issepbar = false;
                    break;
                  }
                }
              }
              if (isprogress || issepbar) {
                int total;
#else
              if (isprogress) {
                int total;
#endif
                int current = 0;
                if(isprogress){
                total = strlen(s) - 2;
                const char *p = s + 1;
                while (*p == '|')
                  (++current, ++p);
              } else {
                total = len;
                const char *p = s;
                while (*p == '-')
                  (++current, ++p);
              }

              xt = x2 + Tab(n)-10;
              int w = cFont::GetFont(fontOsd)->Width(s)-8;
              int fh = cFont::GetFont(fontOsd)->Height()-4;

              //osd->DrawRectangle(xt, y, xt + w, y + fh, clrTransparent);                        /* background   */

              /* border width */
              #define bw 2

              int border = current*(w-2*bw)/total;

//#define NEW_PROGBARS 1
//#define OLD_PROGBARS 1
#ifdef NEW_PROGBARS
#define offset 4
              if(isprogress){
                osd->DrawRectangle(xt, y + bw + offset, xt + bw, y + fh - bw - offset, themeClrMainText);             /* left border  */
                osd->DrawRectangle(xt, y + offset, xt + w, y + bw + offset, themeClrMainText);                        /* lower border */
                osd->DrawRectangle(xt, y + fh - 2*bw - offset, xt + w, y + fh - bw - offset, themeClrMainText);       /* upper border */
                osd->DrawRectangle(xt + w - bw, y + bw + offset, xt + w, y + fh - 2*bw - offset, themeClrMainText);   /* right border */
                osd->DrawRectangle(xt+bw, y + 8 - 4, xt + border, y + fh - 10 + 4, themeClrMainText);
#endif
#ifdef OLD_PROGBARS
              if(isprogress){
                osd->DrawRectangle(xt + w - bw, y + bw, xt + w, y + fh - 2*bw, themeClrMainText);                     /* right border */
                osd->DrawRectangle(xt+bw, y + 8, xt + border, y + fh - 10, themeClrMainText);
#endif
#ifdef SEPBARS
              } else if(issepbar){
                // hw osd->DrawRectangle(xt, y + fh/2 - 2, xt + w, y + fh/2 + 2, themeClrMainText);
                osd->DrawRectangle(xt, y + fh/2 - 2, xt + w, y + fh/2 + 2, Theme.Color(clrChannelEpgProgress));       // hw EpgProgrees
              }
#endif
            } else {
#endif


#if 1
		if(strlen(s) == 1) 
        switch (s[0])
        {
          case 80:
            singlechar = ePinProtect;
	    s[0] = ' ';
            break;
          case 129:
            singlechar = eArchive;
	    s[0] = ' ';
            break;
          case 128:
            singlechar = eBack;
            s[0] = ' ';
            break;
          case 130:
            singlechar = eFolder;
            s[0] = ' ';
            break;
          case 132:
            singlechar = eNewrecording;
            s[0] = ' ';
            break;
          case 133:
            singlechar = eCut;
            s[0] = ' ';
            break;
          case '#':
            singlechar = eRecord;
	    s[0] = ' ';
            break;
          case '>':
            singlechar = eTimeractive;
	    s[0] = ' ';
            break;
          case '!':
            singlechar = eSkipnextrecording;
	    s[0] = ' ';
          case 136:
            singlechar = eRunningNow;
	    s[0] = ' ';
            break;
          case 137:
            singlechar = eNetwork;
	    s[0] = ' ';
            break;
          case 138:
            singlechar = eDirShared;
	    s[0] = ' ';
            break;
          case 139:
            singlechar = eMusic;
	    s[0] = ' ';
            break;
          default:
            break;
        }

      // check if event info symbol: "StTV*" "R"
      // check if event info characters
      if (strlen(s) == 3 && strchr(" StTR", s[0])
          && strchr(" V", s[1]) && strchr(" *", s[2])) {
        // update status
        iseventinfo = true;
      }

      // check if event info symbol: "RTV*" strlen=4
      if (strlen(s) == 4 && strchr(" tTR", s[1])
          && strchr(" V", s[2]) && strchr(" *", s[3])) {
        iseventinfo = true;
      }

      // check if new recording: "01.01.06*", "10:10*"
      if (!iseventinfo &&
          (strlen(s) == 6 && s[5] == '*' && s[2] == ':' && isdigit(*s)
           && isdigit(*(s + 1)) && isdigit(*(s + 3)) && isdigit(*(s + 4)))
          || (strlen(s) == 9 && s[8] == '*' && s[5] == '.' && s[2] == '.'
              && isdigit(*s) && isdigit(*(s + 1)) && isdigit(*(s + 3))
              && isdigit(*(s + 4)) && isdigit(*(s + 6))
              && isdigit(*(s + 7)))) {
        // update status
        isnewrecording = true;
        // make a copy
       // strncpy(buffer, s, strlen(s));
        // remove the '*' character
        s[strlen(s) - 1] = '\0';
      }

      // check if progress bar: "[|||||||   ]"
      if (!iseventinfo && !isnewrecording && !isarchived && 
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

      if (strlen(s) == 2)
        if ((s[0]==(char)132) && (s[1]==(char)133)) {
          singlechar = eNewCut;
          s[0] = s[1] = ' ';
        }

      if (strlen(s) == 2)
        if ((s[0]==(char)129) && (s[1]==(char)133)) {
          singlechar = eArchivedCut;
          s[0] = s[1] = ' ';
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
        if ((s[0] == (char)134) && (s[1] == ' ')) {
          isHD = true;
          s[0]=' '; // remove the character for HD
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
        if ((s[0] == (char)130) && (s[1] == ' ')) {
          isFolder = true;
          s[0]=' ';// remove the character for Folder
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
        if ((s[0] == (char)131) && (s[1] == ' ')) {
          isFavouriteFolder = true;
          s[0]=' ';// remove the character for Favouritefolder
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
        if ((s[0] == (char)135) && (s[1] == ' ')) {
          isCheckmark = true;
          s[0]=' ';// remove the character for Checkmark
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
        if ((s[0] == (char)136) && (s[1] == ' ')) {
          isRunningnow = true;
          s[0]=' ';// remove the character
        }
      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
          if ((s[0] == (char)138) && (s[1] == ' ')) {
          isSharedDir = true;
          s[0]=' ';// remove the character
        }
      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
          if ((s[0] == (char)139) && (s[1] == ' ')) {
          isMusic = true;
          s[0]=' ';// remove the character
        }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
          if ((s[0] == (char)128) && (s[1] == ' ')) {
          isFolderUp = true;
          s[0]=' ';// remove the character
        }
      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && strlen(s) > 3)
          if ((s[0] == (char)129) && (s[1] == ' ')) {
          isArchive = true;
          s[0]=' ';// remove the character
        }
      if (iseventinfo) {
        int evx = xt + Gap;
        char *p = s;
        // draw background
        //osd->DrawRectangle(xt, y, xItemRight, y + lineHeight - 1, ColorBg);
        // draw symbols
        for (; *p; ++p) {
          switch (*p) {
            case 't':
              // partial timer event
              osd->DrawImage(imgIconTimerpart, evx, y + 1, true);
              evx += IMGICONWIDTH;
              *p = ' ';
              break;
            case 'T':
              // timer event
              osd->DrawImage(imgIconTimer, evx, y + 1, true);
              evx += IMGICONWIDTH;
              *p = ' ';
              break;
            case 'R':
              // recording event (epgsearch)
              osd->DrawImage(imgIconRecord, evx, y + 1, true);
              evx += IMGICONWIDTH;
	      *p = ' ';
              break;
            case 'V':
              // vps event
              osd->DrawImage(imgIconVps, evx, y + 1, true);
              evx += IMGICONWIDTH;
	      *p = ' ';
              break;
            case 'S':
              osd->DrawImage(imgIconZappingtimer, evx, y + 1, true);
              evx += IMGICONWIDTH;
              *p = ' ';
              break;
            case '*':
              // running event
//              osd->DrawImage(imgIconRunningnow, evx, y + 1, true);
//              evx += IMGICONWIDTH;
              *p = ' ';
              break;
            case ' ':
            default:
              // let's ignore space character
              break;
          }
        }
      } else if (isnewrecording || isarchived) {
        // draw text
        //printf("s: %s\n", s);
        osd->DrawText(x2 + Tab(n)+20, y, s, themeClrMainText, Current ? themeClrPreBackground : themeClrBackground, font, x5 - xt);

        // draw symbol
        if (isarchived)
        {
          osd->DrawImage(imgIconArchive, xt + cFont::GetFont(fontOsd)->Width(s), y + 1, true);
          if (isnewrecording)
            osd->DrawImage(imgIconNewrecording, xt + cFont::GetFont(fontOsd)->Width(s) + IMGICONWIDTH, y + 1, true);
        } else
          osd->DrawImage(imgIconNewrecording, xt + cFont::GetFont(fontOsd)->Width(s), y + 1, true);
      } else if (isprogressbar) {
        // define x coordinates of progressbar
        int px0 = xt;
        int px1 = px0 ; //= (Tab(i + 1) ? Tab(i + 1) : xItemRight - Roundness)  ;
        // a better calculation of progressbar length
        if( Tab(n+1) > 0 )
            px1 = px0 + Tab(n+1) - Tab(n) - (/*ReelConfig.showMarker ? */lineHeight/* : ListHBorder*/) ;
        else
            // no more tabs: progress bar goes till the end
            px1 = x2 + Tab(n) /*xItemRight*/ - Roundness;

        // when the string complete string is a progressbar, ignore tabs
        //if (strlen(s) == strlen(s)) px1 = xItemRight - Roundness;

        int px = px0 + std::max((int)((float) now * (float) (px1 - px0) / (float) total), 1 /*ListProgressBarBorder*/);
        // define y coordinates of progressbar
#define PROGRESSBARHEIGHT  7
        int py0 = y + (lineHeight - PROGRESSBARHEIGHT) / 2;
        int py1 = py0 + PROGRESSBARHEIGHT;
        // clear a possibly too long channel name
        osd->DrawRectangle(px0, y, px1, y+lineHeight-1, Current ? themeClrPreBackground : themeClrBackground);
        // draw progressbar
        osd->DrawRectangle(px0, py0, px1, py1, Current ? themeClrBackground : themeClrPreBackground/*Theme.Color(clrProgressBarBg)*/);
        osd->DrawRectangle(px0, py0  , px - 1, py1 , themeClrMainText /*Theme.Color(clrProgressBarFg)*/);
        //printf("ProgressBar: %d-%d/%d (%d/%d): %f ?= %f\n",px0,px,px1, now,total, (px - (float)px0)/(float)px1,  (float)now/(float)total) ;
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
            //w = (Tab(n + 1) && HasTabbedText(Text_tmp, n + 1) ? (xItemLeft + Tab(n + 1)) : xItemRight) - xt;
            //osd->DrawText(xt, y, "  ..", ColorFg, ColorBg/*clrTransparent*/, pFontList, w, nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );
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
            DrawUnbufferedImage(osd, "icon_network_20.png", xt, y+1, true);
            break;
          case eDirShared:
            DrawUnbufferedImage(osd, "icon_foldershare_20.png", xt, y+1, true);
            break;
          case eMusic:
            DrawUnbufferedImage(osd, "icon_music_20.png", xt, y+1, true);
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
          //DrawUnbufferedImage(osd, "icon_foldershare_20.png", xt, y+1, true);
          xt += 20;
        } else if (isMusic) {
          //DrawUnbufferedImage(osd, "icon_music_20.png", xt, y+1, true);
          xt += 20;
        } else if (isFolderUp) {
            osd->DrawImage(imgIconFolderUp, xt, y + 1, true);
            xt += 20;
        } else if (isArchive) {
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            xt += 20;
        }
       // osd->DrawText(xt /*+ Tab(n)*/, y, s, themeClrMainText, Current ? themeClrPreBackground : themeClrBackground, font, x5 - (x2 + Tab(n)));
}
#endif
       if(singlechar != eNone && !isHD)
		xt+=20;
       if(!iseventinfo && !isprogressbar)
                osd->DrawText(xt, y, s, themeClrMainText, Current ? themeClrPreBackground : themeClrBackground, font, x5 - xt);
}
#ifdef PROGBARS
          }
#endif
        }
        SetEditableWidth(x6 - x1 - Tab(1));
    }

    void ReelSkinMenu::SetEvent(const cEvent *Event)
    {
        if (!Event)
            return;
        osd->DrawRectangle(x1, y2, x6 - 1, y4 - 1, themeClrBackground);
        const cFont *font = cFont::GetFont(fontSml);
        int xl = x1 + 10;
        int y = y2;
        cTextScroller ts;
        char t[32];
        snprintf(t, sizeof(t), "%s  %s - %s", *Event->GetDateString(), *Event->GetTimeString(), *Event->GetEndTimeString());
        ts.Set(osd, xl, y, x6 - xl, y4 - y, t, font, themeClrMenuEventTime, themeClrBackground);
        if (Event->Vps() && Event->Vps() != Event->StartTime()) {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), " VPS: %s", *Event->GetVpsString());
            const cFont *font = cFont::GetFont(fontSml);
            osd->DrawText(x6 - font->Width(buffer), y, buffer, themeClrMainText, themeClrBackground, font);
            //free(buffer);
            }
        y += ts.Height();
        y += font->Height();
        ts.Set(osd, xl, y, x6 - xl, y4 - y, Event->Title(), font, themeClrMenuEventTitle, themeClrBackground);
        y += ts.Height();
        if (!isempty(Event->ShortText())) {
            const cFont *font = cFont::GetFont(fontSml);
            ts.Set(osd, xl, y, x6 - xl, y4 - y, Event->ShortText(), font, themeClrMenuEventShortText, themeClrBackground);
            y += ts.Height();
            }
        if (!isempty(Event->Description())) {
            const cFont *font = cFont::GetFont(fontOsd);
            // hw const cFont *font = cFont::GetFont(fontNew24);
            y += font->Height();
            textScroller.Set(osd, xl, y, x6 - xl - 2 * ScrollWidth, y4 - y, Event->Description(), font, themeClrMenuEventDescription, themeClrBackground);
            SetScrollBar();  // hw
        }
    }

    void ReelSkinMenu::SetRecording(const cRecording *Recording)
    {
        if (!Recording)
        {
            return;
        }
        const cRecordingInfo *Info = Recording->Info();
        const cFont *font = cFont::GetFont(fontSml);
        int xl = x1 + 10;
        int y = y2;
        cTextScroller ts;
        char t[32];
#if APIVERSNUM < 10721
        snprintf(t, sizeof(t), "%s  %s", *DateString(Recording->start), *TimeString(Recording->start));
#else
        snprintf(t, sizeof(t), "%s  %s", *DateString(Recording->Start()), *TimeString(Recording->Start()));
#endif
        ts.Set(osd, xl, y, x6 - xl, y4 - y, t, font, themeClrMenuEventTime, themeClrBackground);

        y += font->Height();
        cChannel *channel = Channels.GetByChannelID(Info->ChannelID());
        const char *channelName;
        if(channel == NULL) {
          channelName="";
        } else {
           channelName=channel->Name();
        }
        ts.Set(osd, xl, y, x6 - xl, y4 - y, channelName, font, themeClrMenuEventTime, themeClrBackground);

        y += ts.Height();
        y += font->Height();
        const char *Title = Info->Title();
        if (isempty(Title))
            Title = Recording->Name();
        ts.Set(osd, xl, y, x6 - xl, y4 - y, Title, font, themeClrMenuEventTitle, themeClrBackground);
        y += ts.Height();
        if (!isempty(Info->ShortText())) {
            const cFont *font = cFont::GetFont(fontSml);
            ts.Set(osd, xl, y, x6 - xl, y4 - y, Info->ShortText(), font, themeClrMenuEventShortText, themeClrBackground);
            y += ts.Height();
            }
        if (!isempty(Info->Description())) {
            const cFont *font = cFont::GetFont(fontOsd);
            // hw const cFont *font = cFont::GetFont(fontNew24);
            y += font->Height();
            textScroller.Set(osd, xl, y, x6 - xl - 2 * ScrollWidth, y4 - y, Info->Description(), font, themeClrMenuEventDescription, themeClrBackground);
            SetScrollBar();  // hw
        }
    }

    void ReelSkinMenu::SetText(char const *Text, bool FixedFont)
    {
      textScroller.Set(osd, x2, y2, GetTextAreaWidth(), y4 - y2, Text, GetTextAreaFont(FixedFont), Theme.Color(clrMenuText), Theme.Color(clrBackground));
      SetScrollBar();  // hw
    }

    int ReelSkinMenu::GetTextAreaWidth(void) const
    {
        return x5 - x2;
    }

    const cFont *ReelSkinMenu::GetTextAreaFont(bool FixedFont) const
    {
        const cFont *font = cFont::GetFont(FixedFont ? fontFix : fontSml);
        font = cFont::GetFont(fontSml);//XXX -> make a way to let the text define which font to use
        return font;
    }

    void ReelSkinMenu::Flush()
    {
        osd->Flush();
    }

    // --- ReelSkinDisplayReplay -----------------------------------------------

    class ReelSkinDisplayReplay : public cSkinDisplayReplay
    {
    private:
        cOsd *osd;
        int x0, x1, x2, x3, x4;
        int y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10;
        int lastCurrentWidth, lastTotalWidth;
        std::string lastCurrent, lastTotal, messageText, title;
        int message;
        bool modeOnly;
        int modeImgId;

        void DrawBottomBar();
        void DrawCurrent();
        void DrawFrame();
        void DrawMessage();
        void DrawMode();
        void DrawTopBar();
        void DrawTotal();

        void EraseTrick();

    public:
        ReelSkinDisplayReplay(bool ModeOnly);
        virtual ~ReelSkinDisplayReplay();
        virtual void SetTitle(const char *Title);
        virtual void SetMode(bool Play, bool Forward, int Speed);
        virtual void SetProgress(int Current, int Total);
        virtual void SetCurrent(const char *Current);
        virtual void SetTotal(const char *Total);
        virtual void SetJump(const char *Jump);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush(void);
    };

    #define SymbolWidth 30
    #define SymbolHeight 30

    ReelSkinDisplayReplay::ReelSkinDisplayReplay(bool ModeOnly)
    :   lastCurrentWidth(0), lastTotalWidth(0),
        message(-1),
        modeOnly(ModeOnly),
        modeImgId(-1)
    {
        cFont const *font = cFont::GetFont(fontSml);
        int lineHeight = font->Height();

        x0 = 0;
        x2 = x0 + imgTrickWidth;
        x1 = x2 - imgLeftWidth;
        x4 = Setup.OSDWidth;
        x3 = x4 - imgRightWidth;

        y0 = 0;
        y2 = y0 + imgTopHeight;
        y1 = y2 - lineHeight / 2;
        y3 = y2;
        y4 = y3 + imgBottomHeight;
        y5 = y4 + 24;
        y8 = y5 + imgTopHeight;
        y7 = y8 - lineHeight / 2;
        y9 = y8;
        y10 = y9 + imgBottomHeight;
        y6 = y5;

        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - y10,Setup.OSDRandom);
        tArea Areas[] = { { 0, 0, x4 - 1, y10 - 1, 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        SetImagePaths(osd);

        DrawFrame();
    }

    ReelSkinDisplayReplay::~ReelSkinDisplayReplay()
    {
        delete osd;
    }

    void ReelSkinDisplayReplay::SetTitle(const char *title)
    {
        this->title = title;
        DrawFrame();
    }

    void ReelSkinDisplayReplay::DrawFrame()
    {
        EraseTrick();
        if (!modeOnly)
        {
            DrawTopBar();
        }
        DrawBottomBar();
        DrawMode();
    }

    static int replaySymbols[2][2][5] =
    {
        {
            { imgPause, imgSRew, imgSRew1, imgSRew2, imgSRew3 },
            { imgPause, imgSFwd, imgSFwd1, imgSFwd2, imgSFwd3 }
        },
        {
            { imgPlay,  imgFRew, imgFRew1, imgFRew2, imgFRew3 },
            { imgPlay, imgFFwd, imgFFwd1, imgFFwd2, imgFFwd3 }
        }
    };

    void ReelSkinDisplayReplay::SetMode(bool play, bool forward, int speed)
    {
        if (speed < -1)
            speed = -1;
        if (speed > 3)
            speed = 3;
        modeImgId = replaySymbols[play][forward][speed + 1];
        DrawFrame();
    }

    void ReelSkinDisplayReplay::SetProgress(int current, int total)
    {
        cProgressBar pb(x3 - x2, y5 - y4 - 4, current, total, marks,
                        Theme.Color(clrReplayProgressSeen), Theme.Color(clrReplayProgressRest),
                        Theme.Color(clrReplayProgressSelected), Theme.Color(clrReplayProgressMark),
                        Theme.Color(clrReplayProgressCurrent));
        osd->DrawBitmap(x2, y4, pb);
    }

    void ReelSkinDisplayReplay::SetCurrent(char const *current)
    {
        REEL_ASSERT(current);
        lastCurrent = current;

        if (message < 0)
        {
            DrawCurrent();
        }
    }

    void ReelSkinDisplayReplay::SetTotal(char const *total)
    {
        REEL_ASSERT(total);
        lastTotal = total;

        if (message < 0)
        {
            DrawTotal();
        }
    }

    void ReelSkinDisplayReplay::SetJump(char const *jump)
    {
        if (message < 0)
        {
            DrawFrame();

            if (jump)
            {
                osd->DrawText(x2, y7, jump, Theme.Color(clrMainText), clrTransparent, cFont::GetFont(fontSml), x3 - x2, 0, taCenter);
            }
        }
    }

    void ReelSkinDisplayReplay::SetMessage(eMessageType type, char const *text)
    {
        if (text)
        {
            message = type;
            messageText = text;
        }
        else
        {
            message = -1;
        }
        DrawFrame();
    }

    void ReelSkinDisplayReplay::DrawMessage()
    {
        osd->DrawRectangle(x1, y5, x4 - 1, y10 - 1, clrTransparent);
        cFont const *font = cFont::GetFont(fontSml);

        UInt imgId;

        switch(message)
        {
        case mtStatus:
            imgId = imgButtonGreen1;
            osd->DrawImage(imgId, x1, y6, false);
            osd->DrawImage(imgId + 1, x2, y6, false, x3 - x2);
            osd->DrawImage(imgId + 2, x3, y6, false);
            osd->DrawText(x2, y7, messageText.c_str(), Theme.Color(clrMessageStatusFg), clrTransparent, font, x3 - x2, 0, taCenter);
            break;
        case mtWarning:
            imgId = imgButtonYellow1;
            osd->DrawImage(imgId, x1, y6, false);
            osd->DrawImage(imgId + 1, x2, y6, false, x3 - x2);
            osd->DrawImage(imgId + 2, x3, y6, false);
            osd->DrawText(x2, y7, messageText.c_str(), Theme.Color(clrMessageWarningFg), clrTransparent, font, x3 - x2, 0, taCenter);
            break;
        case mtError:
            imgId = imgButtonRed1;
            osd->DrawImage(imgId, x1, y6, false);
            osd->DrawImage(imgId + 1, x2, y6, false, x3 - x2);
            osd->DrawImage(imgId + 2, x3, y6, false);
            osd->DrawText(x2, y7, messageText.c_str(), Theme.Color(clrMessageErrorFg), clrTransparent, font, x3 - x2, 0, taCenter);
            break;
        default:
            imgId = imgButtonBlue1;
            osd->DrawImage(imgId, x1, y6, false);
            osd->DrawImage(imgId + 1, x2, y6, false, x3 - x2);
            osd->DrawImage(imgId + 2, x3, y6, false);
            osd->DrawText(x2, y7, messageText.c_str(), Theme.Color(clrMessageInfoFg), clrTransparent, font, x3 - x2, 0, taCenter);
            break;
        }
    }

    void ReelSkinDisplayReplay::EraseTrick()
    {
        osd->DrawRectangle(x0, y0, x2 - 1, y10 - 1, clrTransparent);
    }

    void ReelSkinDisplayReplay::Flush()
    {
        osd->Flush();
    }

    void ReelSkinDisplayReplay::DrawBottomBar()
    {
        if (message >= 0)
        {
            DrawMessage();
        }
        else
        {
            osd->DrawRectangle(x1, y5, x4 - 1, y10 - 1, clrTransparent);

            if (!modeOnly)
            {

                osd->DrawImage(img_c_top_left, x1, y5, false);
                osd->DrawImage(img_c_top_border, x2, y5, false, x3 - x2);
                osd->DrawImage(img_c_top_right, x3, y5, false);

                osd->DrawImage(img_c_bottom_left, x1, y9, false);
                osd->DrawImage(img_c_bottom_border, x2, y9, false, x3 - x2);
                osd->DrawImage(img_c_bottom_right, x3, y9, false);

                DrawCurrent();

                DrawTotal();
            }
        }
    }

    void ReelSkinDisplayReplay::DrawCurrent()
    {
        if (lastCurrentWidth)
        {
            osd->DrawImage(img_c_top_border, x2, y5, false, lastCurrentWidth);
            osd->DrawImage(img_c_bottom_border, x2, y9, false, lastCurrentWidth);
        }

        if (!lastCurrent.empty())
        {
            cFont const *font = cFont::GetFont(fontSml);
            osd->DrawText(x2, y7, lastCurrent.c_str(), Theme.Color(clrMainText), clrTransparent, font);
            lastCurrentWidth = font->Width(lastCurrent.c_str());
        }
    }

    void ReelSkinDisplayReplay::DrawMode()
    {
        int x = x0;
        int y = y4 - 16;

        int xs = x + (imgTrickWidth - imgTrickSymbolWidth) / 2 - 2;
        int ys = y + (imgTrickHeight - imgTrickSymbolHeight) / 2;

        osd->DrawImage(imgTrick, x, y, true);
        if (modeImgId >= 0)
        {
            osd->DrawImage(modeImgId, xs, ys, true);
        }
    }

    void ReelSkinDisplayReplay::DrawTotal()
    {
        if (lastTotalWidth)
        {
            int x = x3 - lastTotalWidth;
            osd->DrawImage(img_c_top_border, x, y5, false, lastTotalWidth);
            osd->DrawImage(img_c_bottom_border, x, y9, false, lastTotalWidth);
        }

        if (!lastTotal.empty())
        {
            cFont const *font = cFont::GetFont(fontSml);
            lastTotalWidth = font->Width(lastTotal.c_str());
            int x = x3 - lastTotalWidth;
            osd->DrawText(x, y7, lastTotal.c_str(), Theme.Color(clrMainText), clrTransparent, font);
        }
    }

    void ReelSkinDisplayReplay::DrawTopBar()
    {
        osd->DrawImage(img_a_top_left, x1, y0, false);
        osd->DrawImage(img_a_top_border, x2, y0, false, x3 - x2);
        osd->DrawImage(img_a_top_right, x3, y0, false);

        osd->DrawImage(img_a_bottom_left, x1, y3, false);
        osd->DrawImage(img_a_bottom_border, x2, y3, false, x3 - x2);
        osd->DrawImage(img_a_bottom_right, x3, y3, false);

        osd->DrawText(x2, y1, title.c_str(), Theme.Color(clrMainText), clrTransparent, cFont::GetFont(fontSml), x3 - x2);
    }

    // --- ReelSkinDisplayVolume -----------------------------------------------

    class ReelSkinDisplayVolume : public cSkinDisplayVolume
    { // TODO
    private:
        cOsd *osd;
        int x0, x1, x2, x3, x4;
        int y0, y1, y2, y3, y4;
        int mute;
    public:
        ReelSkinDisplayVolume();
        virtual ~ReelSkinDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        virtual void Flush();
    };

    ReelSkinDisplayVolume::ReelSkinDisplayVolume()
    {
        mute = -1;
        x0 = 0;
        x1 = x0 + imgLeftWidth;
        x2 = x1 + imgVolumeWidth;
        x4 = Setup.OSDWidth;
        x3 = x4 - imgRightWidth;
        y0 = 0;
        y1 = y0 + imgTopHeight;
        y2 = y1;
        y3 = y2 + imgBottomHeight;
        y4 = imgMuteHeight;
        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - y3,Setup.OSDRandom);
        tArea Areas[] = { { x0, y0, x4 - 1, y4 - 1, 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        SetImagePaths(osd);
    }

    ReelSkinDisplayVolume::~ReelSkinDisplayVolume()
    {
        delete osd;
    }

    void ReelSkinDisplayVolume::SetVolume(int Current, int Total, bool Mute)
    {
        osd->DrawRectangle(x0, y0, x4 - 1, y4 - 1, clrTransparent);
        if (Mute)
        {
            osd->DrawImage(imgMute, x0, y0, false);
        }
        else
        {
            int x2_1 = x2 + (x3 - x2) * Current / Total;
            osd->DrawImage(img_pre_top_left, x0, y0, false);
            osd->DrawImage(img_pre_top_border, x1, y0, false, x2_1 - x1);
            osd->DrawImage(img_a_top_border, x2_1, y0, false, x3 - x2_1);
            osd->DrawImage(img_a_top_right, x3, y0, false);

            osd->DrawImage(img_pre_border_left, x0, y1, false);
            osd->DrawImage(img_pre_border_right, x3, y1, false);

            osd->DrawImage(img_pre_bottom_left, x0, y2, false);
            osd->DrawImage(img_pre_bottom_border, x1, y2, false, x2_1 - x1);
            osd->DrawImage(img_a_bottom_border, x2_1, y2, false, x3 - x2_1);
            osd->DrawImage(img_a_bottom_right, x3, y2, false);

            osd->DrawImage(imgVolume, x1 - 4, y1 - imgVolumeHeight / 2, true);

            int percent = Current * 100 / Total ;

            char strPercent[16];
            ::sprintf(strPercent, "%d%%", percent);
            cFont const *font = cFont::GetFont(fontSml);
            osd->DrawText(x0, y0 + 10, strPercent, Theme.Color(clrVolumeText), clrTransparent, font, x4 - x0,
                          font->Height(), taCenter);

        }
    }

    void ReelSkinDisplayVolume::Flush(void)
    {
        osd->Flush();
    }

    // --- ReelSkinDisplayTracks -----------------------------------------------

    class ReelSkinDisplayTracks : public cSkinDisplayTracks
    {
    private:
        cOsd *osd;
        int x0, x1, x2, x3, x4, x5, x6;
        int y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10, y11;
        int lineHeight;
        int numTracks_;
    public:
        ReelSkinDisplayTracks(char const *title, int numTracks, char const *const *tracks);
        virtual ~ReelSkinDisplayTracks();
        virtual void SetTrack(int index, char const *const *tracks);
        virtual void SetAudioChannel(int audioChannel);
        virtual void Flush();
    };

    ReelSkinDisplayTracks::ReelSkinDisplayTracks(const char *title, int numTracks, char const *const *tracks)
    {
        numTracks_ = numTracks;
        cFont const *font = cFont::GetFont(fontSml);
        lineHeight = font->Height();
        int itemsWidth = font->Width(title);
        for (int i = 0; i < numTracks; ++i)
        {
            itemsWidth = max(itemsWidth, font->Width(tracks[i]));
        }
        x0 = 0;
        x1 = x0 + imgLeftWidth;
        x2 = x1; // + 2;
        x3 = x2; // + imgMarkerWidth;
        x4 = x3; // + 2;
        x5 = x4 + itemsWidth;
        x6 = x5 + imgRightWidth;
        y0 = 0;
        y2 = y0 + imgTopHeight;
        y3 = y2;
        y4 = y2 + imgBottomHeight;
        y1 = (y0 + y4 - lineHeight) / 2;
        y5 = y4 + imgTopHeight;
        y6 = y5 + numTracks * lineHeight;
        y7 = y6 + imgBottomHeight;
        y9 = y7 + imgTopHeight;
        y10 = y9;
        y11 = y10 + imgBottomHeight;
        y8 = (y7 + y11 - imgAudioChannelIconHeight) / 2;
        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - y11,Setup.OSDRandom);
        tArea Areas[] = { { x0, y0, x6 - 1, y11 - 1, 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        SetImagePaths(osd);

        osd->DrawImage(img_a_top_left, x0, y0, false);
        osd->DrawImage(img_a_top_border, x1, y0, false, x5 - x1);
        osd->DrawImage(img_a_top_right, x5, y0, false);
        osd->DrawImage(img_a_bottom_left, x0, y3, false);
        osd->DrawImage(img_a_bottom_border, x1, y3, false, x5 - x1);
        osd->DrawImage(img_a_bottom_right, x5, y3, false);

        osd->DrawText(x4, y1, title, Theme.Color(clrMainText), clrTransparent, font);

        osd->DrawImage(img_b_top_left, x0, y4, false);
        osd->DrawImage(img_b_top_border, x1, y4, false, x5 - x1);
        osd->DrawImage(img_b_top_right, x5, y4, false);
        osd->DrawImage(img_b_border_left, x0, y5, false, 1, y6 - y5);

        osd->DrawImage(img_b_border_right, x5, y5, false, 1, y6 - y5);
        osd->DrawImage(img_b_bottom_left, x0, y6, false);
        osd->DrawImage(img_b_bottom_border, x1, y6, false, x5 - x1);
        osd->DrawImage(img_b_bottom_right, x5, y6, false);

        SetTrack(-1, tracks);
        SetAudioChannel(-1);
    }

    ReelSkinDisplayTracks::~ReelSkinDisplayTracks()
    {
        delete osd;
    }

    void ReelSkinDisplayTracks::SetTrack(int index, char const *const *tracks)
    {
        cFont const *font = cFont::GetFont(fontSml);

        ReelBoxDevice::Instance()->SetAudioTrack(index);

        osd->DrawRectangle(x1, y5, x5 - 1, y6 - 1, Theme.Color(clrBackground));

        int y = y5;
        for (int n = 0; n < numTracks_; ++n)
        {
            if (index == n)
            {
                osd->DrawImage(img_pre_border_left, x0, y, false, 1, lineHeight);
                osd->DrawImage(img_pre_border_right, x5, y, false, 1, lineHeight);
                // osd->DrawImage(imgHighline1, x0, y, true, 1, lineHeight);
                // osd->DrawImage(imgHighline3, x6 - imgHighline3Width, y, true, 1, lineHeight);
                osd->DrawRectangle(x1, y, x5, y + lineHeight - 1, Theme.Color(clrPreBackground));
                osd->DrawImage(imgMarker, x0 + 8, y, true);
            }
            else
            {
                osd->DrawImage(img_b_border_left, x0, y, false, 1, lineHeight);
                osd->DrawImage(img_b_border_right, x5, y, false, 1, lineHeight);
            }
            osd->DrawText(x4, y, tracks[n], Theme.Color(clrMainText), clrTransparent, font);
            y += lineHeight;
        }
    }

    void ReelSkinDisplayTracks::SetAudioChannel(int audioChannel)
    {
        osd->DrawImage(img_c_top_left, x0, y7, false);
        osd->DrawImage(img_c_top_border, x1, y7, false, x5 - x1);
        osd->DrawImage(img_c_top_right, x5, y7, false);
        osd->DrawImage(img_c_bottom_left, x0, y10, false);
        osd->DrawImage(img_c_bottom_border, x1, y10, false, x5 - x1);
        osd->DrawImage(img_c_bottom_right, x5, y10, false);

        int imgId;

        switch(audioChannel)
        {
        case 0:
            imgId = imgAudioStereo;
            break;
        case 1:
            imgId = imgAudioLeft;
            break;
        case 2:
            imgId = imgAudioRight;
            break;
        default:
            imgId = -1;
            break;
        }
        if (imgId >= 0)
        {
            osd->DrawImage(imgId, x4, y8, true);
        }
    }

    void ReelSkinDisplayTracks::Flush(void)
    {
        osd->Flush();
    }

    // --- ReelSkinDisplayMessage ----------------------------------------------

    class ReelSkinDisplayMessage : public cSkinDisplayMessage { // TODO
    private:
        cOsd *osd;
        int x0, x1, x2, x3;
        int y0, y1, y2;
    public:
        ReelSkinDisplayMessage();
        virtual ~ReelSkinDisplayMessage();
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();
    };

    ReelSkinDisplayMessage::ReelSkinDisplayMessage(void)
    {
        cFont const *font = cFont::GetFont(fontSml);
        int const lineHeight = font->Height();
        x0 = 0;
        x1 = x0 + imgButton1Width;
        x3 = Setup.OSDWidth;
        x2 = x3 - imgButton3Width;
        y0 = 0;
        y2 = imgButtonHeight;
        y1 = (y2 + y0 - lineHeight) / 2;

        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - y2,Setup.OSDRandom);
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
        cFont const *font = cFont::GetFont(fontSml);

        UInt imgId, colorfg;

        switch(Type)
        {
        case mtStatus:
            imgId = imgButtonGreen1;
            colorfg = Theme.Color(clrMessageStatusFg);
            break;
        case mtWarning:
            imgId = imgButtonYellow1;
            colorfg = Theme.Color(clrMessageWarningFg);
            break;
        case mtError:
            imgId = imgButtonRed1;
            colorfg = Theme.Color(clrMessageErrorFg);
            break;
        default:
            imgId = imgButtonBlue1;
            colorfg = Theme.Color(clrMessageInfoFg);
            break;
        }

        osd->DrawImage(imgId, x0, y0, false);
        osd->DrawImage(imgId + 1, x1, y0, false, x2 - x1);
        osd->DrawImage(imgId + 2, x2, y0, false);
        osd->DrawText(x1, y1, Text, colorfg, clrTransparent, font, x2 - x1, 0, taCenter);
    }

    void ReelSkinDisplayMessage::Flush()
    {
        osd->Flush();
    }

    // --- ReelSkin ------------------------------------------------------------

#ifdef RBLITE

    ReelSkin::ReelSkin(void)
    :cSkin("Reel", &::Theme)//XXX naming problem???
    {
    }

    char const *ReelSkin::Description()
    {
        return "Reel";
    }

#else

    ReelSkin::ReelSkin(void)
    :cSkin("ReelBox_Lite", &::Theme)//XXX naming problem???
    {
    }

    char const *ReelSkin::Description()
    {
        return "Reel Lite";
    }
#endif

    cSkinDisplayChannel *ReelSkin::DisplayChannel(bool withInfo)
    {
        return new ReelSkinDisplayChannel(withInfo);
    }

    cSkinDisplayMenu *ReelSkin::DisplayMenu(void)
    {
        return new ReelSkinMenu;
    }

    cSkinDisplayReplay *ReelSkin::DisplayReplay(bool modeOnly)
    {
        return new ReelSkinDisplayReplay(modeOnly);
    }

    cSkinDisplayVolume *ReelSkin::DisplayVolume()
    {
        return new ReelSkinDisplayVolume;
    }

    cSkinDisplayTracks *ReelSkin::DisplayTracks(char const *title, int numTracks, char const *const *tracks)
    {
        return new ReelSkinDisplayTracks(title, numTracks, tracks);
    }

    cSkinDisplayMessage *ReelSkin::DisplayMessage()
    {
        return new ReelSkinDisplayMessage;
    }
}
