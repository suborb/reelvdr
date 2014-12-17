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

// AudioPlayer.h

#ifndef AUDIO_PLAYER_H_INCLUDED
#define AUDIO_PLAYER_H_INCLUDED

#include "AVSyncListener.h"
#include "MpegPes.h"
#include "Reel.h"

namespace Reel
{
    class AudioPlayer
    {
    public:
        static AudioPlayer &InstanceBsp() NO_THROW;
            ///< Return the single instance.

        static AudioPlayer &InstanceHd() NO_THROW;
            ///< Return the single instance.

        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        // All public nonstatic member functions are threadsafe.

        virtual void Clear() = 0;
            ///< Discard all buffered data.

        virtual Int Delay() const = 0;
            ///< Return audio delay in ticks.

        virtual bool Flush() = 0;
            ///< Return true if all data has been processed and played. Return immediately without blocking, even if
            ///< other member functions are currently executing.

        virtual void Freeze() = 0;
            ///< Freeze playback.

        virtual void Play() = 0;
            ///< Unfreeze and prepare for standard AV-synchronized playback.
            ///< Inform the AV-Sync listener to start sync.

        virtual void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false) = 0;
            ///< Play the given audio es packet. Return as soon as possible if data can be buffered, otherwise block.

        virtual void PlayFrames(AudioFrame const *frames, Int numFrames,
                        SampleRate sampleRate, UInt pts, bool isAc3) = 0;
            ///< Play the given raw audio frames. Return as soon as possible if data can be buffered, otherwise block.

        virtual bool Poll() = 0;
            ///< Return true if further data is needed. Return immediately without blocking, even if other member
            ///< functions are currently executing.

        virtual bool PollAudio() = 0;
           ///< FIXME hotfix for Xine raw audio playback by Klaus 

        virtual void SetAVSyncListener(AVSyncListener *listener) = 0;
            ///< Set(remove) the AVSyncListener, wich will be informed when AV sync events occur. Call with listener
            ///< = 0 to remove the listener.

        virtual void SetChannel(AudioChannel audioChannel) = 0;
            ///< Set the audio channel.

        virtual void Start() = 0;
            ///< Start playback.

        virtual void Stop() = 0;
            ///< Stop playback.

    protected:
        static AudioPlayer *instanceHd_;
        static AudioPlayer *instanceBsp_;

        AudioPlayer();
        virtual ~AudioPlayer() NO_THROW;

        AudioPlayer(AudioPlayer const &); // Forbid copy construction.
        AudioPlayer &operator=(AudioPlayer const &); // Forbid copy assignment.
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPlayer &AudioPlayer::InstanceBsp() NO_THROW
    {
        REEL_ASSERT(instanceBsp_);
        return *instanceBsp_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPlayer &AudioPlayer::InstanceHd() NO_THROW
    {
        REEL_ASSERT(instanceHd_);
        return *instanceHd_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPlayer::AudioPlayer()
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPlayer::~AudioPlayer() NO_THROW
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_PLAYER_H_INCLUDED
