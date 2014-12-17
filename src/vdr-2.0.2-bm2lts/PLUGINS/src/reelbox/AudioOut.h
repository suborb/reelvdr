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

// AudioOut.h

#ifndef AUDIO_OUT_H_INCLUDED
#define AUDIO_OUT_H_INCLUDED

#include "AudioPacketQueue.h"
#include "AVSyncListener.h"
#include "Thread.h"

#include <alsa/asoundlib.h>

namespace Reel
{
    ///< Audio output "device".
    class AudioOut /* final */
    {
    public:
        AudioOut(AudioPacketQueue &fullQueue,
                 AudioPacketQueue &emptyQueue);
            ///< Construct the output device with the given audio packet queues.

        ~AudioOut() NO_THROW;
            ///< Stop playback as soon as possible. There may be packets left in the full queue.

        Int Delay() const;
            ///< Return audio delay in ticks.

        bool Flush();
            ///< Return true if all data has been played.

        void InvalidateStc();
            ///< Invalidates the STC.

        void Resume();
            ///< Resumes playback after previous Suspend().

        void StartAVSync();
            ///< Start AV sync mode.

        void SetAVSyncListener(AVSyncListener *aVSyncListener);
            ///< Set/Clear the AV sync listener.

        void Suspend();
            ///< Suspends playback.
            ///< Note: Beware of deadlocks, no audio packets will be freed while the device is suspended. 

        //void SetStc(int stc);

    private:
        AudioOut(AudioOut const &); // Forbid copy construction.
        AudioOut &operator=(AudioOut const &); // Forbid copy assignment.

        void Action();
        void Cleanup();
        bool HandleSampleRateChange(AudioPacket *packet);
        void Init();
        void MainLoop();
        void PcmPlay(AudioFrame const *frames, UInt numFrames);
        bool PlaybackFilledPacket();
        void PlaybackSilentPacket();
        bool StartPlayback();
        void StopPlayback();
        void UpdateStc(AudioPacket *filledPacket);
        bool Sync(bool &discard, AudioPacket *packet);
        //ULLong GetSysTime();
        void UpdateExtStc();

        AudioPacketQueue          &fullQueue_, &emptyQueue_;
        AVSyncListener   *volatile aVSyncListener_;

        Thread<AudioOut> thread_;

        LLong         stc_; 
        LLong         extStc_;
        UInt          stcInterpolationTime_;
        UInt volatile suspendCount_;
        UInt          latency_;

        bool volatile aVSyncMode_;
        bool volatile quit_;
        bool volatile stcValid_;

        SampleRate           sampleRate_;
        snd_pcm_t           *pcmHandle_;
        snd_pcm_hw_params_t *hwParams_;

        enum
        {
            AVSyncPlayTimeThreshold = SampleRate::TicksPerSecond / 4, // 1/4 second FIXME
            StcMaxInterpolation     = SampleRate::TicksPerSecond * 3    // 3 seconds
        };
    };

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioOut::Flush()
    {
        aVSyncMode_ = false;
        return fullQueue_.Empty();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioOut::InvalidateStc()
    {
        stcValid_ = false;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioOut::Resume()
    {
        REEL_ASSERT(suspendCount_);
        -- suspendCount_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioOut::SetAVSyncListener(AVSyncListener *aVSyncListener)
    {
        aVSyncListener_ = aVSyncListener;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioOut::StartAVSync()
    {
        aVSyncMode_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioOut::Suspend()
    {
        ++ suspendCount_;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_OUT_H_INCLUDED
