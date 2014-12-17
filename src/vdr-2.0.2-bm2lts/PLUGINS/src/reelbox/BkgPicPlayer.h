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

// BkgPicPlayer.h
 
#ifndef BKG_PIC_PLAYER_H_INCLUDED
#define BKG_PIC_PLAYER_H_INCLUDED

#include "Reel.h"

#include "CondVar.h"
#include "Mutex.h"
#include "Thread.h"
#include "vdr/config.h"

namespace Reel
{
    class ReelBoxDevice;

    class BkgPicPlayer
    {
    public:
        BkgPicPlayer(ReelBoxDevice &device);
        ~BkgPicPlayer();

        void Start();
        void Stop();

#if VDRVERSNUM >= 10716
        void ResetTimer() { lastPicTime_ = lastAudioTime_ = lastVideoTime_ = ::time(0);};
        void PlayedVideo() { lastVideoTime_ = ::time(0);};
        void PlayedAudio() { lastAudioTime_ = ::time(0);};
#endif
    private:
        BkgPicPlayer(BkgPicPlayer const &);
        BkgPicPlayer &operator=(BkgPicPlayer const &);

        void Action();
        void Sleep();
        void ShowPic();
        void ShowPic2();

        bool ShowPicFile(const char *file);

        ReelBoxDevice &device_;

        Thread<BkgPicPlayer> thread_;
        Mutex mutex_;
        CondVar condWake_;

        bool stop_; // shared

#if VDRVERSNUM < 10716
        int  lastPicTime_, nextPic_, picCount_, picInterval_;
#else
        int  nextPic_, picInterval_;
        time_t lastPicTime_, lastVideoTime_, lastAudioTime_, lastReplayTime_;
        const char *picBase;
        const char *CheckSignal();
#endif
        bool running_;
    };
}

#endif // BKG_PIC_PLAYER_H_INCLUDED
