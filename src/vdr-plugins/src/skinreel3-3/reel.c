/*
 * reel.c: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "reel.h"
#include "logo.h"
#include "status.h"
#include "texteffects.h"
#include "tools.h"

#include "displaymenu.h"
#include "displaychannel.h"
#include "displayvolume.h"
#include "displayreplay.h"
#include "displaytracks.h"
#include "displaymessage.h"

#include <math.h>
#include <ctype.h>
#include <locale.h>
#include <stdio.h>

#include <fstream>
//#include <iostream>
#include <sstream>              // zeicheketten streams
#include <string>

#include <vdr/device.h>
#include <vdr/timers.h>
#include <vdr/menu.h>
#include <vdr/font.h>
#include <vdr/osd.h>
#include <vdr/themes.h>
#include <vdr/plugin.h>

#include <unistd.h>



int progressbarheight = 9;
static char oldTheme[64] = {'\0'}; /** remember the theme name that's set when the skin gets called */
static bool imagePathsSet = false; /** true if the imagePaths have already been set */

int pgbar(const char *buff, int&i_start, int&i_end, int&progressCount) {
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
  for (; buff[i]; i++) {
    if (buff[i]==']') {
      i_end = i;
      break;
    }
    switch (buff[i]) {
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

void DrawProgressBar(cOsd *osd, int x, int y, int x_filled, int x_right) {
    switch(ReelConfig.pgBarType) {
        default:
        case 0:
        {
            osd->DrawRectangle(x, y, x_filled, y+progressbarheight, Theme.Color(clrProgressBarFg));
            osd->DrawRectangle(x_filled, y, x_right - 1, y+progressbarheight, Theme.Color(clrProgressBarBg));
            break;
        }
        case 1:
        case 2:
        {
            osd->DrawImage(imgPgBarLeft, x, y, true);
            osd->DrawImage(imgPgBarFilled, x+1, y, true, x_filled-x+1, 1);
            osd->DrawImage(imgPgBarEmpty, x_filled+1, y, true, x_right-x_filled, 1);
            osd->DrawImage(imgPgBarRight, x_right, y, true);
            break;
        }
    }
}

bool HasDevice(const char *deviceID)
{
    std::ifstream inFile("/proc/bus/pci/devices");

    if (!inFile.is_open() || !deviceID)
        return  false;

    string line, word;

    bool found = false;
    while (getline(inFile, line)) {
       std::istringstream splitStr(line);
       string::size_type pos = line.find(deviceID);
       if (pos != string::npos) {
          found = true;
          break;
       }
    }
    return found;
}

bool HasFrontpanel ()
{
    std::string test_file = "/dev/.have_frontpanel";
    return (access(test_file.c_str(), R_OK) == 0);
}

cTheme Theme;

// one should define m_path string and imgPath char array.
// imgPath is the directory underwhich the image is available
// default_path is a string is the fall back directory under which image _must_ be available
#define IMGPATH(imgEnumNum, imgName) m_path = std::string(imgPath) + "/" + imgName; \
        { struct stat imgfileStat; \
    if(stat(m_path.c_str(), &imgfileStat) == -1) /*unavailable image*/\
      m_path = std::string(default_imgPath) + "/" + imgName;}\
    osd->SetImagePath(imgEnumNum, m_path.c_str())

void SetImagePaths(cOsd *osd) {

    /* TB: reelepg calls SetImagePaths(), so it has to be redone
     * TODO: find some reliable caching mechanism */
    if(0 && imagePathsSet == true && (strcmp(Theme.Name(), oldTheme) == 0)) /* paths have already been set - nothing to do */
       return;
    else
        strncpy(oldTheme, Theme.Name(), 64);

    std::string path;
#ifdef REELVDR
    path = std::string("/usr/share/reel/skinreel3/") + Theme.Name();
#else
    path = std::string("/usr/share/vdr/skinreel3/") + Theme.Name();
#endif

    struct stat myStat;
    /** check if path exists */
    if(stat(path.c_str(), &myStat) == -1){
        esyslog("ERROR: theme \"%s\" does not exist, falling back to theme \"Blue\"", Theme.Name());
#ifdef REELVDR
        path = std::string("/usr/share/reel/skinreel3/Blue");
#else
        path = std::string("/usr/share/vdr/skinreel3/Blue");
#endif
    }

    ReelConfig.SetImagesDir(path.c_str());
    char *imgPath = strdup(ReelConfig.GetImagesDir());
    const char *default_imgPath = "/usr/share/vdr/skinreel3/Blue";

#ifdef REELVDR
    default_imgPath = "/usr/share/reel/skinreel3/Blue";
#endif

    std::string m_path;

DDD("before first IMGPATH()");
    /* MENU */
    if (HasDevice("808627a0") /* AVG (Intel) */
        || HasDevice("10027910") /* AVG II (AMD) */
        || (HasDevice("80860709") && HasFrontpanel()) /* ICEBox */ )
    {
        IMGPATH(imgMenuTitleBottom,"menu_title_bottom.png");
        IMGPATH(imgMenuTitleMiddle,"menu_title_middle.png");
        IMGPATH(imgMenuTitleTop  ,"menu_title_top.png");
    }
    else if (HasDevice("14f12450")  /* NetClient I */
             || HasDevice("80860709") && !HasFrontpanel() /* ICEBox */ )
    {
        IMGPATH(imgMenuTitleBottom,"menu_client_bottom.png");
        IMGPATH(imgMenuTitleMiddle,"menu_client_middle.png");
        IMGPATH(imgMenuTitleTop  ,"menu_client_top.png");
    }
    else /* Unknown hardware */
    {
        IMGPATH(imgMenuTitleBottom,"banner_halfempty_bottom.png");
        IMGPATH(imgMenuTitleMiddle,"banner_halfempty_middle.png");
        IMGPATH(imgMenuTitleTop  ,"banner_halfempty_top.png");
    }
DDD("after first IMGPATH()");
//DDD("m_path %s\n\t imgPath %s\n\t default_imgPath %s", m_path, imgPath, default_imgPath);

    /* empty banner */
    IMGPATH(imgEmptyBannerBottom, "banner_empty_bottom.png");
    IMGPATH(imgEmptyBannerMiddle, "banner_empty_middle.png");
    IMGPATH(imgEmptyBannerTop, "banner_empty_top.png");

    IMGPATH(imgMenuTitleTopX  ,"menu_title_top_x.png");

    IMGPATH(imgMenuHeaderRight,"menu_header_right.png");
    IMGPATH(imgMenuHeaderCenterX,"menu_header_center_x.png");
    IMGPATH(imgMenuHeaderLeft,"menu_header_left.png");

    IMGPATH(imgMenuFooterRight,"menu_footer_right.png");
    IMGPATH(imgMenuFooterCenterX,"menu_footer_center_x.png");
    IMGPATH(imgMenuFooterLeft,"menu_footer_left.png");

    IMGPATH(imgMenuMessageCenterX,"menu_message_center_x.png");
    IMGPATH(imgMenuMessageLeft,"menu_message_left.png");
    IMGPATH(imgMenuMessageRight,"menu_message_right.png");

    /* menu body */
    IMGPATH(imgMenuBodyUpperPart,"menu_center_x_upper_part.png");
    IMGPATH(imgMenuBodyLowerPart,"menu_center_x_lower_part.png");
    IMGPATH(imgMenuBodyLeftUpperPart,"menu_body_left_upper_part.png");
    IMGPATH(imgMenuBodyLeftLowerPart,"menu_body_left_lower_part.png");
    IMGPATH(imgMenuBodyRightUpperPart,"menu_body_right_upper_part.png");
    IMGPATH(imgMenuBodyRightLowerPart,"menu_body_right_lower_part.png");

    /* left part of menu body */
    IMGPATH(imgMenuHighlightUpperPart,"menu_highlight_center_x_upper_part.png");
    IMGPATH(imgMenuHighlightLowerPart,"menu_highlight_center_x_lower_part.png");
    IMGPATH(imgMenuHighlightLeftUpperPart,"menu_highlight_left_upper_part.png");
    IMGPATH(imgMenuHighlightLeftLowerPart,"menu_highlight_left_lower_part.png");

    /* ICONS */
    IMGPATH(imgIconPinProtect,"icon_pin_protect_20.png");
    IMGPATH(imgIconArchive,"icon_archive_20.png");
    IMGPATH(imgIconMusic,"icon_music_20.png");
    IMGPATH(imgIconCut,"icon_cut_20.png");
    IMGPATH(imgIconFolder,"icon_folder_20.png");
    IMGPATH(imgIconFolderUp,"icon_folderup_20.png");
    IMGPATH(imgIconFavouriteFolder,"icon_favouritefolder_20.png");
    IMGPATH(imgIconHd,"icon_hd_20.png");
    IMGPATH(imgIconHdKey,"icon_hd_key_20.png");
    IMGPATH(imgIconMessage,"icon_message_25.png");
    IMGPATH(imgIconNewrecording,"icon_newrecording_20.png");
    IMGPATH(imgIconRecord,"icon_record_20.png");
    IMGPATH(imgIconRunningnow,"icon_runningnow_20.png");
    IMGPATH(imgIconSkipnextrecording,"icon_skipnextrecording_20.png");
    IMGPATH(imgIconTimer,"icon_timer_20.png");
    IMGPATH(imgIconTimeractive,"icon_timeractive_20.png");
    IMGPATH(imgIconTimerpart,"icon_timerpart_20.png");
    IMGPATH(imgIconVps,"icon_vps_20.png");
    IMGPATH(imgIconZappingtimer,"icon_zappingtimer_20.png");

    /* ChannelInfo */
    IMGPATH( imgChannelInfoFooterCenterX , "channelinfo_footer_center_x.png" );
    IMGPATH( imgChannelInfoFooterLeft , "channelinfo_footer_left.png" );
    IMGPATH( imgChannelInfoFooterRight , "channelinfo_footer_right.png" );

    /* Message Bar */
    IMGPATH( imgMessageBarCenterX , "message_bar_center_x.png" );
    IMGPATH( imgMessageBarLeft , "message_bar_left.png" );
    IMGPATH( imgMessageBarRight , "message_bar_right.png" );

    /* HelpButtons */
    IMGPATH(imgButtonRedX, "button_red_x.png");
    IMGPATH(imgButtonGreenX, "button_green_x.png");
    IMGPATH(imgButtonYellowX, "button_yellow_x.png");
    IMGPATH(imgButtonBlueX, "button_blue_x.png");

    /* Replay */
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

    /* Message */
    IMGPATH( imgIconHelpActive ,"icon_help_active.png");
    IMGPATH( imgIconHelpInactive, "icon_help_inactive.png");
    IMGPATH( imgIconMessageError ,"icon_message_error_25.png");
    IMGPATH( imgIconMessageInfo ,"icon_message_info_25.png");
    IMGPATH( imgIconMessageNeutral ,"icon_message_neutral_25.png");
    IMGPATH( imgIconMessageWarning ,"icon_message_warning_25.png");

    IMGPATH(imgSignal, "signal.png");

    /* Channel Icons */
    IMGPATH(imgNoChannelIcon,"channel.png");

    /* Music Banner */
    IMGPATH(imgMusicBodyTop,"banner_music_top.png");
    IMGPATH(imgMusicBodyMiddle,"banner_music_middle.png");
    IMGPATH(imgMusicBodyBottom,"banner_music_bottom.png");

    /* The "small button"-backgrounds */
    IMGPATH(imgButtonSmallActive, "button_small_active.png");
    IMGPATH(imgButtonSmallInactive, "button_small_inactive.png");

    /* The "big button"-backgrounds */
    IMGPATH(imgButtonActive, "button-active.png");
    IMGPATH(imgButtonInactive, "button-inactive.png");
    IMGPATH(imgButtonBigActive, "button-active-big.png");

    /* the progressbar */
    switch(ReelConfig.pgBarType) {
        default:
        case 1:
        {
            IMGPATH(imgPgBarLeft, "pgbar_left.png");
            IMGPATH(imgPgBarRight, "pgbar_right.png");
            IMGPATH(imgPgBarEmpty, "pgbar_empty.png");
            IMGPATH(imgPgBarFilled, "pgbar_filled.png");
            break;
        }
        case 2:
        {
            IMGPATH(imgPgBarLeft, "pgbar_small_left.png");
            IMGPATH(imgPgBarRight, "pgbar_small_right.png");
            IMGPATH(imgPgBarEmpty, "pgbar_small_empty.png");
            IMGPATH(imgPgBarFilled, "pgbar_small_filled.png");
            break;
        }
    }

    free (imgPath);
#ifdef REELVDR
    imgPath = strdup("/usr/share/reel/skinreel3");
#else
    imgPath = strdup("/usr/share/vdr/skinreel3");
#endif

    /* Channel info symbols */
    IMGPATH(imgRecordingOn,"recording.png");
    IMGPATH(imgRecordingOff,"recording_off.png");
    IMGPATH(imgDolbyDigitalOn,"dolbydigital.png");
    IMGPATH(imgDolbyDigitalOff,"dolbydigital_off.png");
    IMGPATH(imgEncryptedOn,"encrypted.png");
    IMGPATH(imgEncryptedOff,"encrypted_off.png");
    IMGPATH(imgTeletextOn,"teletext.png");
    IMGPATH(imgTeletextOff,"teletext_off.png");
    IMGPATH(imgRadio,"radio.png");
    IMGPATH(imgAudio, "audio.png");
    IMGPATH(img43, "43.png");
    IMGPATH(img169, "169.png");

    IMGPATH( imgFreetoair,"freetoair.png");

    free(imgPath);

    imagePathsSet = true;
}

void DrawUnbufferedImage(cOsd* osd, std::string imgName, int xLeft, int yTop, int blend, int horRep, int vertRep) {
  std::string m_path, path = ReelConfig.GetImagesDir();
  path = path + "/" + imgName;

  char *imgPath = ReelConfig.GetImagesDir();

  // fall back image directory
  const char *default_imgPath = "/usr/share/vdr/skinreel3/Blue";
#ifdef REELVDR
   default_imgPath = "/usr/share/reel/skinreel3/Blue";
#endif

   /* IMG_PATH uses imgPath, m_path, default_imgPath */
  IMGPATH(imgUnbufferedEnum, imgName);
  osd->DrawImage(imgUnbufferedEnum, xLeft, yTop, blend, horRep, vertRep);
}

cString GetFullDateTime(time_t t) {
	char buffer[32];
	if (t == 0)
		time(&t);
	struct tm tm_r;
	tm *tm = localtime_r(&t, &tm_r);
    if(tm->tm_year == 70) /** time is still set to unix NULL-time, so return an empty string */
        buffer[0] = '\0';
    else
	    snprintf(buffer, sizeof(buffer), "%s %02d.%02d.%04d %02d:%02d", *WeekDayName(tm->tm_wday), tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
	return buffer;
}


/** cSkinReelOsd */
inline bool cSkinReelBaseOsd::HasChannelTimerRecording(const cChannel *Channel) {
  /* try to find current channel from timers */
  for (cTimer * t = Timers.First(); t; t = Timers.Next(t)) {
    if ((t->Channel() == Channel) && t->Recording())
      return true;
  }
  return false;
}

int cSkinReelBaseOsd::DrawStatusSymbols(int x0, int xs, int top, int bottom, const cChannel *Channel /* = NULL */) { // What is x0 for ?
  if (!ReelConfig.showStatusSymbols)
    return xs;

  cDevice *Device = cDevice::PrimaryDevice();
  eTrackType TrackType = Device->GetCurrentAudioTrack();
  const tTrackId *Track = Device->GetTrack(TrackType);
  if (Track) {
    int AudioMode = Device->GetAudioChannel();
    if (!(AudioMode >= 0 && AudioMode < MAX_AUDIO_BITMAPS))
      AudioMode = 0;
  }
  return xs;
}

int cSkinReelBaseOsd::FixWidth(int w, int bpp, bool enlarge /* = true */) {
  int a = 8 / bpp;
  int r = w & (a - 1);
  if (r == 0)
    return w;
  return enlarge ? (w + a -r) : (w - r);
}

/** cSkinReel */
cSkinReel::cSkinReel() : cSkin("Reel", &::Theme) {
  // Get the "classic" skin to be used as fallback skin if any of the OSD
  // menu fails to open.
  skinFallback = Skins.First();
  for (cSkin *Skin = Skins.First(); Skin; Skin = Skins.Next(Skin)) {
    if (strcmp(Skin->Name(), "sttng") == 0) {
      skinFallback = Skin;
      break;
    }
  }
}

const char *cSkinReel::Description(void) {
  return tr("Reel Avantgarde");
}

cSkinDisplayChannel *cSkinReel::DisplayChannel(bool WithInfo) {
  try {
    return new cSkinReelDisplayChannel(WithInfo);
  } catch(...) {
    return skinFallback->DisplayChannel(WithInfo);
  }
}

cSkinDisplayMenu *cSkinReel::DisplayMenu(void) {
  try {
    return new cSkinReelDisplayMenu;
  } catch (...) {
    return skinFallback->DisplayMenu();
  }
}

cSkinDisplayReplay *cSkinReel::DisplayReplay(bool ModeOnly) {
  try {
    return new cSkinReelDisplayReplay(ModeOnly);
  } catch (...) {
    return skinFallback->DisplayReplay(ModeOnly);
  }
}

cSkinDisplayVolume *cSkinReel::DisplayVolume(void) {
  try {
    return new cSkinReelDisplayVolume;
  } catch (...) {
    return skinFallback->DisplayVolume();
  }
}

cSkinDisplayTracks *cSkinReel::DisplayTracks(const char *Title, int NumTracks, const char *const *Tracks) {
  try {
    return new cSkinReelDisplayTracks(Title, NumTracks, Tracks);
  } catch (...) {
    return skinFallback->DisplayTracks(Title, NumTracks, Tracks);
  }
}

cSkinDisplayMessage *cSkinReel::DisplayMessage(void) {
  try {
    return new ReelSkinDisplayMessage;
  } catch (...) {
    return skinFallback->DisplayMessage();
  }
}

// vim:et:sw=2:ts=2:
