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

// AudioDecoderNull.h

#ifndef AUDIO_DECODER_NULL_H_INCLUDED
#define AUDIO_DECODER_NULL_H_INCLUDED

#include "AudioDecoder.h"

#include "MpegPes.h" // FIXME

namespace Reel
{
    ///< Dummy audio decoder. Generates nothing.
    class AudioDecoderNull : public AudioDecoder /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.
    
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static AudioDecoderNull *Instance() NO_THROW;
            ///< Return the single instance.

        /* override */ AudioPacket *Decode();

        /* override */ AudioPacket *Flush();

        /* override */ void PutEsPacket(Mpeg::EsPacket const &esPacket);

        /* override */ void Reset(AudioPacketQueue *emptyPacketQueue);

    private:
        static AudioDecoderNull *instance_;

        AudioDecoderNull(); // Allow construction only through Create().
        ~AudioDecoderNull() NO_THROW; // Allow destruction only through Destroy().

        AudioDecoderNull(AudioDecoderNull const &); // Forbid copy construction.
        AudioDecoderNull &operator=(AudioDecoderNull const &); // Forbid copy assignment.
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderNull *AudioDecoderNull::Instance() NO_THROW
    {
        return instance_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderNull::AudioDecoderNull()
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderNull::~AudioDecoderNull() NO_THROW
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket *AudioDecoderNull::Decode()
    {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket *AudioDecoderNull::Flush()
    {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioDecoderNull::PutEsPacket(Mpeg::EsPacket const &esPacket)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioDecoderNull::Reset(AudioPacketQueue *emptyPacketQueue)
    {
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_DECODER_H_INCLUDED
