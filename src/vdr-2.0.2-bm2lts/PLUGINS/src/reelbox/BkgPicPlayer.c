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

// BkgPicPlayer.c

#include "BkgPicPlayer.h"

#include "config.h"
#include "ReelBoxDevice.h"

#include <vector>

#if VDRVERSNUM >= 10716
#include "../mcli/mcli_service.h"

#define AUDIO_ONLY_TIMEOUT  3
#define NO_REPLAY_TIMEOUT  10
#define NO_DATA_TIMEOUT    10
#endif

namespace Reel
{
    int const BKGND_IMG_INTERVAL = 15;

    BkgPicPlayer::BkgPicPlayer(ReelBoxDevice &device)
    :   device_(device), stop_(false),
        lastPicTime_(0), nextPic_(0), picInterval_(15), running_(false)
    {
#if VDRVERSNUM >= 10716
        picBase=0;
        lastReplayTime_=0;
#endif
    }

    BkgPicPlayer::~BkgPicPlayer()
    {
        Stop();
    }

    void BkgPicPlayer::Action()
    {
        MutexLocker lock(mutex_);
#if VDRVERSNUM < 10716
        picCount_ = 0;
#endif
        while (!stop_)
        {
            ShowPic();
            Sleep();
        }
    }

#if VDRVERSNUM >= 10716
    const char *BkgPicPlayer::CheckSignal() {
        cPlugin *plugin = cPluginManager::GetPlugin("mcli");
        mclituner_info_t info;
        memset(&info, 0, sizeof(info));
        if(plugin && plugin->Service("GetTunerInfo", (void *)&info))
            if(!info.name[0][0])
                return NONETCEIVER_IMAGE;
        return NOSIGNAL_IMAGE;
    } // BkgPicPlayer::CheckSignal
#endif

    void BkgPicPlayer::ShowPic()
    {
        time_t now = ::time(0);
#if VDRVERSNUM < 10716
        if (now >= lastPicTime_ + picInterval_)
        {
            lastPicTime_ = now;
            ++ nextPic_;
        }
        ShowPic2();
        ++ picCount_;
#else
        if(!cDevice::ActualDevice() || (&device_ != cDevice::PrimaryDevice())) return;
        if(!lastReplayTime_) lastReplayTime_ = now;

        const char *newBase=0;

#if 0
isyslog("Receiving %d (%d Lock %d) Replaying %d (%d) Transferring %d Actual %p Primary %p ShowAudio %d", device_.Receiving(true), cDevice::ActualDevice()->Receiving(true), cDevice::ActualDevice()->HasLock(), device_.Replaying(), cDevice::ActualDevice()->Replaying(), device_.Transferring(), cDevice::ActualDevice(), cDevice::PrimaryDevice(), device_.ShowAudioBackgroundPics());
#endif

        if(device_.Replaying() || !device_.Transferring()) lastReplayTime_ = now;

        if(device_.ShowAudioBackgroundPics() && ((lastAudioTime_ + AUDIO_ONLY_TIMEOUT) > now) && ((lastVideoTime_ + AUDIO_ONLY_TIMEOUT) < now)) {
#if defined RBLITE || ! defined(REELVDR)
            newBase = BACKGROUND_IMAGE;
#else
            cPlugin *plugin=cPluginManager::GetPlugin("music");
            if (plugin && plugin->Service("running playback")) {
                newBase = BACKGROUND_IMAGE2;
            } else {
                newBase = BACKGROUND_IMAGE;
            }
#endif
        } // if
#if 0 // Not used/makes trouble?
        if((lastReplayTime_ + NO_REPLAY_TIMEOUT) < now) {
            newBase = CheckSignal();
        } else if(device_.Transferring()) {
            if(((lastAudioTime_ + NO_DATA_TIMEOUT) < now) && (lastVideoTime_ + NO_DATA_TIMEOUT < now)) {
                newBase = CheckSignal();
                if(newBase != picBase) device_.Restart(); // Need to restart if we have played something before
            } // if
        } // if
#endif
        if (newBase && ((newBase != picBase) || (now >= lastPicTime_ + picInterval_))) {
            char *bkgndImgFileName=NULL;
            if(asprintf(&bkgndImgFileName, newBase, ++nextPic_) == -1) bkgndImgFileName = NULL;
#if 0
isyslog("ShowPic %s", bkgndImgFileName);
#endif
            if (bkgndImgFileName && !ShowPicFile(bkgndImgFileName) && (nextPic_ != 1)) {
                nextPic_ = 0; // Retry with first file
            } else {
                lastPicTime_ = now;
                picBase = newBase;
            } // if
            if(bkgndImgFileName) free(bkgndImgFileName);
        } else {
            picBase = newBase;
        } // if
#endif
    }

    void BkgPicPlayer::ShowPic2()
    {
	char bkgndImgFileName[512];
#if defined RBLITE || ! defined(REELVDR)
	::sprintf(bkgndImgFileName, BACKGROUND_IMAGE, nextPic_);
#else
	cPlugin *plugin=cPluginManager::GetPlugin("music");
	if (plugin && plugin->Service("running playback")) {
	 ::sprintf(bkgndImgFileName, BACKGROUND_IMAGE2, nextPic_);
	 } else {
	 ::sprintf(bkgndImgFileName, BACKGROUND_IMAGE, nextPic_);
	}
#endif	
        if (!ShowPicFile(bkgndImgFileName) && (nextPic_ != 1)) {
            nextPic_ = 1; // Retry with first file
            ShowPic2();
        }
    }

    bool BkgPicPlayer::ShowPicFile(const char *file) {
        FILE *imgFile = ::fopen(file, "rb");
        if (!imgFile) return false;

        struct stat fileStat;
        ::stat(file, &fileStat);
        int fileSize = fileStat.st_size;

        if (fileSize > 0) {
            std::vector<Byte> buffer(fileSize);
            Int r = ::fread(&buffer[0], 1, fileSize, imgFile);
            if (r == fileSize)
                    device_.StillPicture(&buffer[0], fileSize);
        }
        ::fclose(imgFile);
        return true;
    } // BkgPicPlayer::ShowPicFile

    void BkgPicPlayer::Sleep()
    {
#if VDRVERSNUM < 10716
        int const minT = 2;
        int t = picCount_ < 15 ? minT : lastPicTime_ + picInterval_ - ::time(0);
        if (t < minT)
        {
            t = minT;
        }
        condWake_.Wait(mutex_, t * 1000000000LL);
#else
        cCondWait::SleepMs(3); // Keeps cpu load low, even if condWake_.Wait returns immediately?
        condWake_.Wait(mutex_, 1000000000LL);
#endif
    }

    void BkgPicPlayer::Start()
    {
        if (!running_)
        {
            running_ = true;
#if VDRVERSNUM >= 10716
            ResetTimer();
#else
            MutexLocker lock(mutex_);
#endif
            stop_ = false;
            thread_.Start(*this, &BkgPicPlayer::Action);
        }
    }

    void BkgPicPlayer::Stop()
    {
        if (running_)
        {
            {
                MutexLocker lock(mutex_);
                stop_ = true;
                condWake_.Broadcast(mutex_);
            }
            thread_.Join();
            running_ = false;
        }
    }
}
