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

// AudioDecoderMpeg1.h

#ifndef AUDIO_DECODER_MPEG1_H_INCLUDED
#define AUDIO_DECODER_MPEG1_H_INCLUDED

#include "AudioDecoder.h"

#include <mad.h>

namespace Reel
{
    ///< Audio decoder for MPEG-1 audio (all layers).
    class AudioDecoderMpeg1 : public AudioDecoder /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.
    
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static AudioDecoderMpeg1 *Instance() NO_THROW;
            ///< Return the single instance.

        /* override */ AudioPacket *Decode();

        /* override */ AudioPacket *Flush();

        /* override */ void PutEsPacket(Mpeg::EsPacket const &esPacket);

        /* override */ void Reset(AudioPacketQueue *emptyPacketQueue);

    private:
        static AudioDecoderMpeg1 *instance_;

        AudioDecoderMpeg1(); // Allow construction only through Create().
        ~AudioDecoderMpeg1() NO_THROW; // Allow destruction only through Destroy().

        AudioDecoderMpeg1(AudioDecoderMpeg1 const &); // Forbid copy construction.
        AudioDecoderMpeg1 &operator=(AudioDecoderMpeg1 const &); // Forbid copy assignment.

        void Clean(AudioPacketQueue *emptyPacketQueue);
        bool ConvertDecodedSamples();
        bool FillDecodeBuffer();
        void MadDecode();
        void OutputPacket();

        enum
        {
            DecodeBufferSize = 4096
        };

        AudioPacketQueue   outputPackets_;
        AudioPacket       *decodePacket_;
        AudioPacketQueue  *emptyQueue_;

        mad_fixed_t const *decodedSamples_[2];

        Byte const *inputPtr_;
        UInt        inputSize_;
        UInt        numDecodedSamples_;
        UInt        pts_;

        SampleRate  sampleRate_;
        bool        ptsValid_;

        struct mad_stream  madStream_;
        struct mad_frame   madFrame_;
        struct mad_synth   madSynth_;
        UInt               madErrorCount_;

        Byte   decodeBuffer_[DecodeBufferSize + MAD_BUFFER_GUARD];
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderMpeg1 *AudioDecoderMpeg1::Instance() NO_THROW
    {
        return instance_;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_DECODER_MPEG1_H_INCLUDED
