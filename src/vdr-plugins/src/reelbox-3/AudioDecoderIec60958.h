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

// AudioDecoderIec60958.h

#ifndef AUDIO_DECODER_IEC60958_H_INCLUDED
#define AUDIO_DECODER_IEC60958_H_INCLUDED

#include "AudioDecoder.h"

#include <vector>

struct iec60958;

namespace Reel
{
    class AudioDecoderIec60958 : public AudioDecoder
    {
    public:
        /* override */ AudioPacket *Decode();

        /* override */ AudioPacket *Flush();

        /* override */ void PutEsPacket(Mpeg::EsPacket const &esPacket);

        /* override */ void Reset(AudioPacketQueue *emptyPacketQueue);

    protected:
        AudioDecoderIec60958();
        ~AudioDecoderIec60958() NO_THROW;

        iec60958 *iec_;
        Byte *buffer_;

    private:
        AudioDecoderIec60958(AudioDecoderIec60958 const &); // Forbid copy construction.
        AudioDecoderIec60958 &operator=(AudioDecoderIec60958 const &); // Forbid copy assignment.

        void Clean(AudioPacketQueue *emptyPacketQueue);
        bool ConvertSamples();
        void OutputPacket();

        AudioPacketQueue   outputPackets_;
        AudioPacket       *decodePacket_;
        AudioPacketQueue  *emptyQueue_;

        Byte const *out_, *end_;

        Short const *inputPtr_;
        UInt         inputSize_;
        UInt         pts_;

        SampleRate  sampleRate_;
        bool        ptsValid_;
        UInt        startFrame_;
    };

    //--------------------------------------------------------------------------------------------------------------

    ///< Audio decoder for AC3.
    class AudioDecoderAc3 : public AudioDecoderIec60958 /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.
    
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static AudioDecoderAc3 *Instance() NO_THROW;
            ///< Return the single instance.

    private:
        static AudioDecoderAc3 *instance_;

        AudioDecoderAc3(); // Allow construction only through Create().

        AudioDecoderAc3(AudioDecoderAc3 const &); // Forbid copy construction.
        AudioDecoderAc3 &operator=(AudioDecoderAc3 const &); // Forbid copy assignment.
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderAc3 *AudioDecoderAc3::Instance() NO_THROW
    {
        return instance_;
    }

    //--------------------------------------------------------------------------------------------------------------

    ///< Audio decoder for dts.
    class AudioDecoderDts : public AudioDecoderIec60958 /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.
    
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static AudioDecoderDts *Instance() NO_THROW;
            ///< Return the single instance.

    private:
        static AudioDecoderDts *instance_;

        AudioDecoderDts(); // Allow construction only through Create().

        AudioDecoderDts(AudioDecoderDts const &); // Forbid copy construction.
        AudioDecoderDts &operator=(AudioDecoderDts const &); // Forbid copy assignment.
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderDts *AudioDecoderDts::Instance() NO_THROW
    {
        return instance_;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_DECODER_IEC60958_H_INCLUDED
