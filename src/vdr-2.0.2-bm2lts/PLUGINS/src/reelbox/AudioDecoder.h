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

// AudioDecoder.h

#ifndef AUDIO_DECODER_H_INCLUDED
#define AUDIO_DECODER_H_INCLUDED

#include <cstring>

#include "AudioPacketQueue.h"
#include "Utils.h"

namespace Reel
{
    namespace Mpeg
    {
        class EsPacket;
    }

    enum AudioChannel
    {
        AudioChannelStereo    = 0,
        AudioChannelMonoLeft  = 1,
        AudioChannelMonoRight = 2,
        AudioChannelMute      = 3
    };

    //--------------------------------------------------------------------------------------------------------------

    class AudioDecoder /* abstract */
    {
    public:
        virtual AudioPacket *Decode() = 0;
            ///< Must be called repeatedly after PutEsPacket() until it returns 0. A pts will only be attached to some
            ///< (not all) packets, pts-interpolation is not done.
            ///< Note: A partially decoded packet may be left in the decoder after 0 is returned.

        virtual AudioPacket *Flush() = 0;
            ///< Return the partially decoded packet if present, otherwise 0.

        virtual void PutEsPacket(Mpeg::EsPacket const &esPacket) = 0;
            ///< Puts an es packet into the decoder. A reference to esPacket is kept by the decoder until a subsequent
            ///< Decode() returns 0.

        virtual void Reset(AudioPacketQueue *emptyPacketQueue) = 0;
            ///< Reset decoder state and tell decoder the queue from which to get empty audio packets to decode into.

        void SetChannel(AudioChannel audioChannel);
            ///< Set the audio channel.

    protected:
        AudioDecoder(); // Allow construction only by derived classes.
        virtual ~AudioDecoder() NO_THROW; // Allow destruction only by derived classes.

        AudioChannel volatile channel_;
    };

    //--------------------------------------------------------------------------------------------------------------

    class AudioDecoders
    {
    public:
        friend class AudioDecoder;

        typedef ObjectSequence<AudioDecoder>::Iterator Iterator;

        static Iterator Begin();
        static Iterator End();
            ///< Return sequence of all audio decoders.

    private:
        static ObjectSequence<AudioDecoder> audioDecoders_;

        static void Erase(Iterator it);
    };

    void CreateAudioDecoders();
        ///< Creates all audio decoders.

    void DestroyAudioDecoders();
        ///< Destroys all audio decoders.

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoders::Iterator AudioDecoders::Begin()
    {
        return audioDecoders_.Begin();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoders::Iterator AudioDecoders::End()
    {
        return audioDecoders_.End();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioDecoders::Erase(AudioDecoders::Iterator it)
    {
        audioDecoders_.Erase(it);
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioDecoder::SetChannel(AudioChannel audioChannel)
    {
        channel_ = audioChannel;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_DECODER_H_INCLUDED
