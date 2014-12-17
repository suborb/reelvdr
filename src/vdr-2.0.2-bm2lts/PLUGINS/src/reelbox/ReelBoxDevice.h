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

// ReelboxDevice.h

#ifndef REEL_BOX_DEVICE_H_INCLUDED
#define REEL_BOX_DEVICE_H_INCLUDED

#include "AudioPacket.h"
#include "BkgPicPlayer.h"
#include "Reel.h"
#include "ReelBoxMenu.h"

//Start by Klaus
#include "VideoPlayer.h"
//End by Klaus

#include <vdr/device.h>
#include <vdr/plugin.h>

#if 1 //def REELVDR
#include <vdr/reelboxbase.h>
#endif

#include <memory>

namespace Reel
{
#if 1 //def REELVDR
    class ReelBoxDevice : public cDevice, public cReelBoxBase /* final */
#else
    class ReelBoxDevice : public cDevice /* final */
#endif
    {
    public:
        static ReelBoxDevice *Instance();
        static void Create();

        virtual void MakePrimaryDevice(bool On);

        ReelBoxDevice();
        virtual ~ReelBoxDevice();

        Int AudioDelay() const; // Return audio delay in ticks.

        /* override */ void AudioUnderflow();

        /* override */ bool CanReplay() const;

        /* override */ void Clear();

        /* override */ bool Flush(int timeoutMs);

        void Restart();

        void FlushAudio();

        /* override */ void Freeze();

        /* override */ int GetAudioChannelDevice();

        /* override */ cSpuDecoder *GetSpuDecoder();

        /* override */ bool HasDecoder() const;

        /* override */ void Mute();

        /* override */ void Play();

#if VDRVERSNUM < 10400
        /* override */ Int PlayAudio(Byte const *data, Int length);
#else
        /* override */ Int PlayAudio(Byte const *data, Int length, uchar Id);
#endif

        void PlayAudioRaw(AudioFrame const *frames, Int numFrames,
                                         SampleRate sampleRate, UInt pts);

        /* override */ Int PlayVideo(Byte const *data, Int length);

        void PlayVideoEs(Byte const *data, Int length, UInt pts);

        Int PlayVideoTs(Byte const *data, Int length, bool VideoOnly, uchar* PATPMT);

        /* override */ void PlayPipVideo(Byte const *data, Int length);

        /* override */ bool Poll(cPoller &Poller, Int timeoutMs);

        /* override */ bool PollVideo(int timeoutMs);

        /* override */ void SetAudioChannelDevice(int audioChannel);

        /* override */ void SetDigitalAudioDevice(bool on);

        /* override */ void SetPipDimensions(uint x, uint y, uint width, uint height);

        /* override */ bool SetPlayMode(ePlayMode playMode);

        /* override */ void SetVideoFormat(bool videoFormat16_9);

        /* override */ void SetVolumeDevice(int volume);

        /* override */ void StartPip(bool on);

        /* override */ void StillPicture(Byte const *data, Int length);

        /* override */ void TrickSpeed(Int speed);

        /*override*/ int64_t GetSTC();

        //Start by Klaus
        bool PollAudio(int timeoutMs);

        void SetStc(bool stcValid, UInt stc);

        void SetAudioBackgroundPics(bool on);

        void RestartAudio();

        void SetAudioTrack(int index);

        int GetAudioTrack(void) { return audioIndex; }

        int GetAspectRatio();

#if VDRVERSNUM >= 10716
        virtual void GetVideoSize (int &Width, int &Height, double &Aspect);
        virtual void GetOsdSize (int &Width, int &Height, double &Aspect);
#endif
        virtual bool NeedDelayedTrickmode() { return true; }
        virtual bool NeedTSCheck() { return true; }
        virtual bool NeedTSCheckEOS() { return true; }

        virtual int AproxFramesInQueue(void);
        virtual bool ShowAudioBackgroundPics();
    private:
        static ReelBoxDevice *instance_;

        ReelBoxDevice(ReelBoxDevice const &); // Forbid copy construction.
        ReelBoxDevice &operator=(ReelBoxDevice const &); // Forbid copy assignment.

        void PlayAudioVideoPesPacket(Byte const *data, Int length);
        void SetPlayModeOff();
        void SetPlayModeOn();
        void SetVolume(int volume, const char *device);

        std::auto_ptr<cSpuDecoder> spuDecoder_;

        class VideoPlayerPip *videoPlayerPip_;
        class AudioPlayer *audioPlayerBsp_;
        class AudioPlayer *audioPlayerHd_;
        class VideoPlayer *videoPlayer_;
        AudioPlayer *audioPlayerProvidingStc_;
        bool digitalAudio_;
        bool volatile pipActive_;
        int audioIndex;
        int audioChannel_;

#if VDRVERSNUM < 10716
        int volatile audioPlayback_, videoPlayback_;
#endif
        ePlayMode playMode_;
        ReelBoxSetup newRbsetup_;
        BkgPicPlayer bkgPicPlayer_;
        bool audioBackgroundPics_;
        volatile UInt stc_;
        bool useHDExtension_;
        bool audioOverHDMI_;
        bool audioOverHd_;
        int tmpHDaspect;
        int oldAudioChannel;
        int oldAudioIndex;
        int needRestart;
        bool normalPlay;
    };

    inline bool ReelBoxDevice::CanReplay() const
    {
        return true;
    }

    inline bool ReelBoxDevice::HasDecoder() const
    {
        return true;
    }

    inline ReelBoxDevice *ReelBoxDevice::Instance()
    {
#ifdef REEL_DEBUG
        REEL_VERIFY(instance_);
#endif
        return instance_;
    }
}

#endif // REEL_BOX_DEVICE_H_INCLUDED
