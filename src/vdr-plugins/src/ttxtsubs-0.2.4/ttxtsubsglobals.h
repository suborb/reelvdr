/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

class cPluginTtxtsubs;
class cMenuSetupTtxtsubs;
class cTtxtsubsPageMenu;
class cTtxtSubsChannelSettings;
class cTtxtSubsDisplay;

#define MAXLANGUAGES 5

class cTtxtsubsConf {
  friend class cPluginTtxtsubs;
  friend class cMenuSetupTtxtsubs;
  friend class cTtxtsubsPageMenu;
  friend class cTtxtSubsDisplay;

 public:
  cTtxtsubsConf(void)
    {
      mDoDisplay = 1;
      mRealDoDisplay =1;
      mDoRecord = 1;
      mMainMenuEntry = 0;
      mFrenchSpecial = 0;
      mDvbSources = 0;
      mFontSize = 20;
      mOutlineWidth = 2;
      memset(mLanguages, 0, sizeof(mLanguages));
      memset(mHearingImpaireds, 0, sizeof(mHearingImpaireds));
      mI18nLanguage = 0;
      mLiveDelay = 0;
      mReplayDelay = 0;
      mReplayTsDelay = 0;
    }

 public:
  int doDisplay(void) {return mRealDoDisplay;}
  int doRecord(void) {return mDoRecord;}
  int mainMenuEntry(void) {return mMainMenuEntry;}
  int frenchSpecial(void) {return mFrenchSpecial;}
  int dvbSources(void) {return mDvbSources;}
  int fontSize(void) {return mFontSize;}
  int outlineWidth(void) {return mOutlineWidth;}
  char (*languages(void))[MAXLANGUAGES][2][4] {return &mLanguages;}
  int (*hearingImpaireds(void))[MAXLANGUAGES][2] {return &mHearingImpaireds;}

  int langChoise(const char *lang, const int HI);
  int i18nLanguage(void) {return mI18nLanguage;}
  int liveDelay(void) {return mLiveDelay;}
  int replayDelay(void) {return mReplayDelay;}
  int replayTsDelay(void) {return mReplayTsDelay;}

 protected:
  int mDoDisplay;
  int mRealDoDisplay;
  int mDoRecord;
  int mMainMenuEntry;
  int mFrenchSpecial;
  int mDvbSources;
  int mFontSize;
  int mOutlineWidth;
  char mLanguages[MAXLANGUAGES][2][4];
  int mHearingImpaireds[MAXLANGUAGES][2];
  int mI18nLanguage;
  int mLiveDelay;
  int mReplayDelay;
  int mReplayTsDelay;
};

extern cTtxtsubsConf globals;
    
