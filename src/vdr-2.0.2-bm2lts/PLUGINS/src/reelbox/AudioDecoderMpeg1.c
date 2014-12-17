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

// AudioDecoderMpeg1.c
 
#include "AudioDecoderMpeg1.h"
#include "MpegPes.h"
#include "ReelBoxMenu.h"

#include <algorithm>

#include <vdr/config.h>

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    namespace
    {
        inline Short MadFixedToShort(mad_fixed_t fixed)
        {
            if (fixed >= MAD_F_ONE)
            {
                return SHRT_MAX;
            }
            if (fixed <= -MAD_F_ONE)
                return -SHRT_MAX;
        
            return implicit_cast<Short>(fixed >> (MAD_F_FRACBITS - 15));
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::Create()
    {
        if (!instance_)
        {
            instance_ = new AudioDecoderMpeg1;
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::Destroy() NO_THROW
    {
        delete instance_;
        instance_ = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderMpeg1 *AudioDecoderMpeg1::instance_ = 0;

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderMpeg1::AudioDecoderMpeg1()
    :   decodePacket_(0), emptyQueue_(0),
        madErrorCount_(0)
    {
        Reset(0);
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderMpeg1::~AudioDecoderMpeg1() NO_THROW
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::Clean(AudioPacketQueue *emptyPacketQueue)
    {
        AudioPacket *outPacket;
        while ((outPacket = outputPackets_.TryGet()))
        {
            REEL_ASSERT(emptyQueue_);
            emptyQueue_->PutEmptyPacket(outPacket);
        }
        if (decodePacket_)
        {
            REEL_ASSERT(emptyQueue_);
            emptyQueue_->PutEmptyPacket(decodePacket_);
            decodePacket_ = 0;
        }
        emptyQueue_ = emptyPacketQueue;

        inputSize_         = 0;
        numDecodedSamples_ = 0;
        ptsValid_          = false;
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioDecoderMpeg1::ConvertDecodedSamples()
    {
        if (!numDecodedSamples_)
        {
            // No decoded samples. Nothing to do.
            return false;
        }

        if (!decodePacket_)
        {
            // Get a new packet do decode into.
            REEL_ASSERT(emptyQueue_);
            decodePacket_ = GetPacketFromQueue(*emptyQueue_);
            decodePacket_->SetSampleRate(sampleRate_);
            decodePacket_->SetPtsValid(ptsValid_);
            decodePacket_->SetPts(pts_);
            decodePacket_->SetNumFrames(0);
            ptsValid_ = false;
        }

        // Now convert the pcm frames from mad to user format.

        mad_fixed_t const *from[2] = {decodedSamples_[0], decodedSamples_[1]};

        AudioFrame  *to              = decodePacket_->GetFrames() + decodePacket_->GetNumFrames();
        UInt const numSamples        = std::min(numDecodedSamples_, AudioPacket::MaxFrames - decodePacket_->GetNumFrames());
        AudioFrame  *toEnd           = to + numSamples;
        decodedSamples_[0]          += numSamples;
        decodedSamples_[1]          += numSamples;
        numDecodedSamples_          -= numSamples;
        decodePacket_->SetNumFrames(decodePacket_->GetNumFrames() + numSamples);

        AudioChannel channel = channel_;
        if (madErrorCount_)
        {
            channel = AudioChannelMute;
            -- madErrorCount_;
        }

        switch (channel)
        {
        case AudioChannelStereo:
            while (to != toEnd)
            {
                to->Samples.Left = MadFixedToShort(*from[0]);
                to->Samples.Right = MadFixedToShort(*from[1]);
                ++ from[0];
                ++ from[1];
                ++ to;
            }
            break;
        case AudioChannelMonoLeft:
            while (to != toEnd)
            {
                Short const sample = MadFixedToShort(*from[0]);
                to->Samples.Left = sample;
                to->Samples.Right = sample;
                ++ from[0];
                ++ to;
            }
            break;
        case AudioChannelMonoRight:
            while (to != toEnd)
            {
                Short const sample = MadFixedToShort(*from[1]);
                to->Samples.Left = sample;
                to->Samples.Right = sample;
                ++ from[1];
                ++ to;
            }
            break;
        case AudioChannelMute:
            memset(to, 0, numSamples * sizeof(AudioFrame));
            break;
        }

        if (decodePacket_->GetNumFrames() == AudioPacket::MaxFrames)
        {
            // Packet is full.
            OutputPacket();
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderMpeg1::Decode()
    {
        AudioPacket *packet = outputPackets_.TryGet();
        if (packet)
        {
            return packet;
        }

        for (;;)
        {
            // Exit if we have enough filled output packets.
            if (outputPackets_.GetNumElements() >= NumberOfAudioPackets / 2)
            {
                break;
            }

            // If there are any decoded samples left, write them to the decode packet.
            if (ConvertDecodedSamples())
            {
                continue;
            }

            // Fill the decode buffer, if necessary.
            if (!FillDecodeBuffer())
            {
                // Exit if we can't put any data into the decode buffer.
                break;
            }

            // Now all decoded samples have been consumed and the decode buffer is filled.
            MadDecode();
        }

        // Return an output packet if there is one.
        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioDecoderMpeg1::FillDecodeBuffer()
    {
        if (!madStream_.buffer || madStream_.error == MAD_ERROR_BUFLEN)
        {
            if (!inputSize_)
            {
                // Need to fill decode buffer but out of input data. Exit.
                return false;
            }

            UInt remaining;
            if (madStream_.next_frame)
            {
                // The decode buffer has been incompletely consumed by mad. Move the remaining data to the start of the
                // buffer.
                remaining = madStream_.bufend - madStream_.next_frame;
                if (madStream_.next_frame != decodeBuffer_)
                {
                    std::memmove(decodeBuffer_, madStream_.next_frame, remaining);
                }
            }
            else
            {
                remaining = 0;
            }
            Byte *decodePtr = decodeBuffer_ + remaining;

            // Copy input data to decode buffer.
            UInt copySize = std::min(inputSize_, DecodeBufferSize - remaining);

            std::memcpy(decodePtr, inputPtr_, copySize);
            inputPtr_ += copySize;
            inputSize_ -= copySize;

            mad_stream_buffer(&madStream_, decodeBuffer_, copySize + remaining);
            madStream_.error = MAD_ERROR_NONE;
        }
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderMpeg1::Flush()
    {
        // FIXME: Fillup packet with zeros.

        OutputPacket();

        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::MadDecode()
    {
        if (mad_frame_decode(&madFrame_, &madStream_))
        {
            if (!MAD_RECOVERABLE(madStream_.error) && madStream_.error != MAD_ERROR_BUFLEN)
            {
                // Fatal nonrecoverable error. Restart everything.
                //::printf("mad: nonrecoverable error!\n");
                Reset(emptyQueue_);
                madErrorCount_ = 3;
            }
            else if (madStream_.error != MAD_ERROR_BUFLEN)
            {
                // Recoverable error. Discard decode / output packet.

                //::printf("mad: recoverable error!\n");
                Clean(emptyQueue_);
                madErrorCount_ = 3;
            }
            return;
        }

        mad_synth_frame(&madSynth_, &madFrame_);

        sampleRate_ = SampleRate::HzToSampleRate(madSynth_.pcm.samplerate);
        if (sampleRate_.IsSupported())
        {
            REEL_ASSERT(madSynth_.pcm.channels == 1 || madSynth_.pcm.channels == 2);
            numDecodedSamples_ = madSynth_.pcm.length;
            decodedSamples_[0] = madSynth_.pcm.samples[0];
            if (madSynth_.pcm.channels == 2)
            {
                decodedSamples_[1] = madSynth_.pcm.samples[1];
            }
            else // madSynth_.pcm.channels == 1
            {
                decodedSamples_[1] = decodedSamples_[0];
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::OutputPacket()
    {
        if (decodePacket_ && decodePacket_->GetNumFrames())
        {
            // We have a decoded packet.
            outputPackets_.PutFilledPacket(decodePacket_);
            decodePacket_ = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::Reset(AudioPacketQueue *emptyPacketQueue)
    {
        Clean(emptyPacketQueue);
        mad_stream_init(&madStream_);
        mad_frame_init(&madFrame_);
        mad_synth_init(&madSynth_);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderMpeg1::PutEsPacket(Mpeg::EsPacket const &esPacket)
    {
        REEL_ASSERT(inputSize_ == 0); // Previous packet must have been completely consumed.

        inputPtr_  = esPacket.GetData();
        inputSize_ = esPacket.GetDataLength();

        if (esPacket.HasPts())
        {
            UInt const pts = esPacket.GetPts() + RBSetup.delay_stereo * 900;

            if (decodePacket_)
            {
                // We have a partially filled decode packet. Attach the timestamp to it.
                UInt const decodePacketPlayTime = AudioPacketPlayTime(*decodePacket_) / SampleRate::TicksPerMpegTick;
                
                decodePacket_->SetPtsValid(true);
                decodePacket_->SetPts(pts - decodePacketPlayTime);
            }
            else
            {
                // Currently no decode packet active. Attach the PTS to the following packet.
                ptsValid_ = true;
                pts_ = pts;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------
}
