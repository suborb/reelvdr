/***************************************************************************
 *   Copyright (C) 2007 by Dirk Leber                                      *
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

#include "Player.h"
#include <stdarg.h>
#include <vdr/status.h>
#include <hdshm_user_structs.h>

namespace ERFPlayer {
	std::string FormatStringV(char const *fmt, va_list ap) {
		std::string ret;
		char *cstr;
		if (::vasprintf(&cstr, fmt, ap) != -1) {
			ret = cstr;
			::free(cstr);
		} // if
		return ret;
	} // FormatStringV
	std::string FormatString(char const *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		std::string ret = FormatStringV(fmt, ap);
		va_end(ap);
		return ret;
	} // FormatString
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Control
	Control::Control(char const *filePath, const char *defLang): 
			::cControl(new Player(filePath, defLang)), mSkinDisplayReplay(0), mSkinDisplayTracks(0), mShowProgress(false), mTitle(filePath), mDispReplSpeed(0), mDispReplPlay(false), mDispReplDir(false)  {
		mPlayer = static_cast < Player * >(player);
		//::cStatus::MsgReplaying(this, mTitle.c_str());
		::cStatus::MsgReplaying(this, mTitle.c_str(), NULL, true);  //patched by AR
	} // Control::Control
	Control::~Control() {
		Stop();
	} // Control::~Control
	::cOsdObject * Control::GetInfo() {
		return::cControl::GetInfo();
	} // Control::GetInfo
	void Control::Hide() {
		if (mSkinDisplayTracks)
			delete mSkinDisplayTracks;
		mSkinDisplayTracks = 0;
		if (mSkinDisplayReplay)
			delete mSkinDisplayReplay;
		mSkinDisplayReplay = 0;
	} // Control::Hide
	eOSState Control::ProcessKey(eKeys key) {
		if (mShowProgress && !mSkinDisplayReplay && !::cOsd::IsOpen()) {
			ShowProgress();
		} // if
		if (!mPlayer || !mPlayer->mPlayer || !mPlayer->mPlayer->IsPlaying()) {
			Hide();
			return osEnd;
		} // if
		UpdateProgress();
		if (mSkinDisplayTracks) {
			if(!mPlayer || !mPlayer->mPlayer)
				return osContinue;	
			switch (key) {
				case kUp|k_Repeat:
				case kUp:
					if (mDisplTrack > 0)
						mDisplTrack--; 
					break;
				case kDown|k_Repeat:
				case kDown: 			    
					if (mDisplTrack < mPlayer->mPlayer->GetAudioCount() -1)
						mDisplTrack++; 
					break;
				case kOk:
					mPlayer->mPlayer->SetCurrentAudio(mDisplTrack);
					// nobreak
				case kBack:
					Hide();
					// nobreak
				case kNone:  
					return osContinue;
				default: break; 
			} // switch
			mSkinDisplayTracks->SetTrack(mDisplTrack, mPlayer->mPlayer->GetAudioTracks());
			mSkinDisplayTracks->Flush();
			return osContinue;
		} // if
		switch (key) {
			case kLeft:
			case kLeft | k_Repeat:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Skip(-5);
				break;
			case kRight:
			case kRight | k_Repeat:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Skip(5);
				break;
			case kFastRew:
			case kFastRew | k_Repeat:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->FR();
				break;
			case k1:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Skip(-20);
				break;
			case k3 | k_Repeat:
			case k3:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Skip(20);
				break;
/*			case kRed | k_Repeat:
			case kRed: // default: record
				if (mPlayer) mPlayer->Skip(-60);
				break;
*/			case kGreen | k_Repeat:
			case kGreen: // default: audio selection but mpegplayer says skip back 60s
				break;
			case kYellow | k_Repeat:
			case kYellow: // default: toggle pause but mpegplayer says skip forward 60s
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Skip(60);
				break;
			case kBlue | k_Repeat:
			case kBlue: // default: toggle replay
				if (mPlayer && mPlayer->mPlayer) {
					mSkinDisplayTracks = Skins.Current()->DisplayTracks(tr("Button$Audio"), mPlayer->mPlayer->GetAudioCount(), mPlayer->mPlayer->GetAudioTracks());
					mDisplTrack = mPlayer->mPlayer->GetCurrentAudio();
					mSkinDisplayTracks->SetTrack(mDisplTrack, mPlayer->mPlayer->GetAudioTracks());
					mSkinDisplayTracks->Flush();
				} // if
				break;
			case kFastFwd:
			case kFastFwd | k_Repeat:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->FF();
				break;
			case kOk:
				if (mShowProgress)
					ProgressOff();
				else
					ProgressOn();
				break;
			case kBack:
				if (mShowProgress) {
					ProgressOff();
					break;
				} else {
					Hide();
					return osEnd;
				} // if
			case kPause:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Pause();
				break;
			case kPlay:
				if (mPlayer && mPlayer->mPlayer) mPlayer->mPlayer->Play();
				break;
			case kStop:
				Hide();
				return osEnd;
			default:
				return cControl::ProcessKey(key);
		} // switch
		return osContinue;
	} // Control::ProcessKey
	void Control::ProgressOff() {
		Hide();
		mShowProgress = false;
		needsFastResponse = false;
	} // Control::ProgressOff
	void Control::ProgressOn() {
		mShowProgress = true;
		needsFastResponse = true;
		ShowProgress();
	} // Control::ProgressOn
	void Control::ShowProgress() {
		mSkinDisplayReplay = Skins.Current()->DisplayReplay(false);
		mSkinDisplayReplay->SetTitle(mTitle.c_str());
		mDispReplPlay    = true;
		mDispReplCurrent = -1;
		mDispReplTotal   = -1;
		mDispReplSpeed   = -999;
		mDispReplDir     = true;
		mSkinDisplayReplay->Flush();
	} // Control::ShowProgress
	void Control::Stop() {
		//::cStatus::MsgReplaying(this, NULL);
		::cStatus::MsgReplaying(this, NULL, NULL, false);   //patched by AR
		delete mPlayer;
		mPlayer = 0;
		player = 0;
	} // Control::Stop
	void Control::UpdateProgress() {
		if (mSkinDisplayReplay) {
			int c, t;
			bool lFlush = false;
			if (!mPlayer || !mPlayer->GetIndex(c, t, false)) {
				c = 0;
				t = 3600 * 25;
			} // if
			if (c != mDispReplCurrent || t != mDispReplTotal) {
				mDispReplCurrent = c;
				mDispReplTotal = t;
				mSkinDisplayReplay->SetProgress(mDispReplCurrent, mDispReplTotal);
				std::string str;
				int hour = c / (60 * 60 * 25);
				if (hour <= 9) {
					int r = c % (60 * 60 * 25);
					int min = r / (60 * 25);
					int sec = r % (60 * 25) / 25;
					str = FormatString("%d:%02d:%02d", hour, min, sec);
				} else {
					str = "9:59:59";
				} // if
				mSkinDisplayReplay->SetCurrent(str.c_str());
				hour = t / (60 * 60 * 25);
				if (hour <= 9) {
					int r = t % (60 * 60 * 25);
					int min = r / (60 * 25);
					int sec = r % (60 * 25) / 25;
					str = FormatString("%d:%02d:%02d", hour, min, sec);
				} else {
					str = "9:59:59";
				} // if
				mSkinDisplayReplay->SetTotal(str.c_str());
				lFlush = true;
			} // if
			bool lPlay, lDir;
			int lSpeed;
			if (mPlayer && mPlayer->GetReplayMode(lPlay, lDir, lSpeed)) { 
				if (mDispReplPlay != lPlay || mDispReplSpeed != lSpeed || mDispReplDir != lDir) {
					mSkinDisplayReplay->SetMode(lPlay, lDir, lSpeed);
					mDispReplPlay  = lPlay;
					mDispReplDir   = lDir;
					mDispReplSpeed = lSpeed;
					lFlush = true;
				} // if
			} // if
			if (lFlush)
				mSkinDisplayReplay->Flush();
		}
	} // Control::UpdateProgress
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Player
	Player::Player(const char *filePath, const char *defLang):
			mPlayer(0),
			filePath_(std::string("shm:")+filePath),
			defLang_(defLang) {
	} // Player::Player
	Player::~Player() {
		if(mPlayer)
			delete mPlayer;
		mPlayer = 0;
		Detach();
	} // Player::~Player
	void Player::Activate(bool on) {
		if(on) {
			if (!mPlayer)
				mPlayer = new RawPlayer();
			if (!mPlayer)
				return;
			mPlayer->Play(filePath_.c_str(), defLang_.c_str());
		} else {
			if (mPlayer)
				delete mPlayer;
			mPlayer = 0;
		} // if
	} // Player::Activate
	bool Player::GetIndex(int &current, int &total, bool snapToIFrame) {
		if(!mPlayer)
			return false; 
		if(!mPlayer->IsPlaying()) 
			return false;
		if (mPlayer->GetPos() < 0 || mPlayer->GetSize() < 0)
			return false;
		current = (mPlayer->GetPos()  / 10000)/4;
		total   = (mPlayer->GetSize() / 10000)/4;
		return true;
	} // Player::GetIndex
	bool Player::GetReplayMode(bool &Play, bool &Forward, int &Speed) { 
		if(!mPlayer)
			return false;
		if(!mPlayer->IsPlaying())
			return false;
		Play    =  !mPlayer->IsPaused();
		Forward = mPlayer->GetSpeed() >= 0;
		int lSpeed = mPlayer->GetSpeed();
		if(lSpeed < 0)
			lSpeed = -lSpeed;
		if (lSpeed < 100)
			Speed = (lSpeed/10)-1;
		else
			Speed = (lSpeed/100)-2;
		return true;
	} // Player::GetReplayMode 
} // namespace ERFPlayer

