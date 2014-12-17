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


#ifndef RAWPLAYER_H_INCLUDED
#define RAWPLAYER_H_INCLUDED

#include "../reelbox/HdCommChannel.h"
#include "Thread.h"
#include <string>

namespace ERFPlayer {
	class RawPlayer {
		public:
			RawPlayer();
			virtual ~RawPlayer();
			bool Play(const char *fFilePath, const char *fDefLang);
			bool Play();
			bool Stop();
			bool Pause();
			bool FF();
			bool FR();
			bool Skip(long lOffset);
			bool IsPlaying();
			bool IsPaused();
			int GetSpeed();
			unsigned long long GetPos();
			unsigned long long GetSize();
			void SetCurrentAudio(int fId);
			int GetCurrentAudio();
			int GetAudioCount();
			char const *const *GetAudioTracks();
		protected:
			void ThreadProc();
			bool Cmd(int fCmd, long long fOffset, int fSpeed, unsigned long fId);
			const char *GetStateText(int fState);
			const char *GetFormatText(unsigned long  fFormat);
			Reel::HdCommChannel::Channel mCmdChan;
			hd_channel_t *mReqChan;
			hd_channel_t *mInfoChan;
			std::string mFilePath;
			int mFile;
			Reel::Thread < RawPlayer > mThread;
			bool mInitialized;
			bool mTerminate;
			bool mPlaying;
			bool mPaused;
			int mSpeed;
			unsigned long long mPos;
			unsigned long long mSize;
			int mLastState;
			int mAudioCount;
			int mCurrentAudio;
			unsigned long mAudioIDs[ERF_MAX_AUDIO];
			char *mAudioTracks[ERF_MAX_AUDIO];
	}; // RawPlayer
} // ERFPlayer

#endif // RAWPLAYER_H_INCLUDED
