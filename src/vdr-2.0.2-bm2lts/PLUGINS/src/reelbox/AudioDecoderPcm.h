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

// AudioDecoderPcm.h

#ifndef AUDIO_DECODER_PCM_H_INCLUDED
#define AUDIO_DECODER_PCM_H_INCLUDED

#include "AudioDecoder.h"

namespace Reel
{
    ///< Audio decoder for mpeg PCM audio.
    class AudioDecoderPcm : public AudioDecoder /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.
    
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static AudioDecoderPcm *Instance() NO_THROW;
            ///< Return the single instance.

        /* override */ AudioPacket *Decode();

        /* override */ AudioPacket *Flush();

        /* override */ void PutEsPacket(Mpeg::EsPacket const &esPacket);

        /* override */ void Reset(AudioPacketQueue *emptyPacketQueue);

        void PutFrames(AudioFrame const *frames, Int numFrames,
                       SampleRate sampleRate, UInt pts);

    private:
        static AudioDecoderPcm *instance_;

        AudioDecoderPcm(); // Allow construction only through Create().
        ~AudioDecoderPcm() NO_THROW; // Allow destruction only through Destroy().

        AudioDecoderPcm(AudioDecoderPcm const &); // Forbid copy construction.
        AudioDecoderPcm &operator=(AudioDecoderPcm const &); // Forbid copy assignment.

        void AttachPts(UInt pts);
        void Clean(AudioPacketQueue *emptyPacketQueue);
        bool ConvertSamples();
        void OutputPacket();

        AudioPacketQueue   outputPackets_;
        AudioPacket       *decodePacket_;
        AudioPacketQueue  *emptyQueue_;

        Byte const *inputPtr_;
        UInt        inputSize_;
        UInt        pts_;

        SampleRate  sampleRate_;
        bool        ptsValid_, swapByteOrder_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline AudioDecoderPcm *AudioDecoderPcm::Instance() NO_THROW
    {
        return instance_;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_DECODER_PCM_H_INCLUDED
