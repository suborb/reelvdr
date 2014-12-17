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

// AudioPlayerBsp.h

#ifndef AUDIO_PLAYER_BSP_H_INCLUDED
#define AUDIO_PLAYER_BSP_H_INCLUDED

#include "AudioOut.h"
#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "Thread.h"

namespace Reel
{
    class AudioPlayerBsp : public AudioPlayer /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.

        /* override */ void Clear();

        /* override */ Int Delay() const;

        /* override */ bool Flush();

        /* override */ void Freeze();

        /* override */ void Play();

        /* override */ void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false);

        /* override */ void PlayFrames(AudioFrame const *frames, Int numFrames,
                                       SampleRate sampleRate, UInt pts, bool isAc3);

        /* override */ bool Poll();

        /* override */ bool PollAudio();

        /* override */ void SetAVSyncListener(AVSyncListener *listener);

        /* override */ void SetChannel(AudioChannel audioChannel);

        /* override */ void Start();

        /* override */ void Stop();

    protected:

        AudioPlayerBsp(); // Allow construction only through Create().
        ~AudioPlayerBsp() NO_THROW; // Allow destruction only through Destroy().

        AudioPlayerBsp(AudioPlayerBsp const &); // Forbid copy construction.
        AudioPlayerBsp &operator=(AudioPlayerBsp const &); // Forbid copy assignment.


    private:

        enum
        {
            PollBufferThreshold = SampleRate::TicksPerSecond / 2 // FIXME!
        };

        AudioPacketQueue    emptyQueue_;
        AudioPacketQueue    fullQueue_;

        Mutex               mutex_;
        AudioDecoder       *lastDecoder_;
        AudioOut           *audioOut_;

        bool volatile       freeze_;
        AudioChannel        audioChannel_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline Int AudioPlayerBsp::Delay() const
    {
        return audioOut_->Delay();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPlayerBsp::Flush()
    {
        return audioOut_->Flush();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPlayerBsp::Poll()
    {
        bool ret = !freeze_ && fullQueue_.GetPlayTime() < PollBufferThreshold;
        // ::printf("AudioPlayerBsp::Poll() = %d\n", ret);
        return ret;
    }

     //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPlayerBsp::PollAudio()
    {
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPlayerBsp::SetAVSyncListener(AVSyncListener *listener)
    {
        audioOut_->SetAVSyncListener(listener);
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPlayerBsp::SetChannel(AudioChannel audioChannel)
    {
        audioChannel_ = audioChannel;
        lastDecoder_->SetChannel(audioChannel);
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_PLAYER_BSP_H_INCLUDED
