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

// AudioDecoderIec60958.c
 
#include "AudioDecoderIec60958.h"
#include "MpegPes.h"
#include "ReelBoxMenu.h"

#include <algorithm>

#include <vdr/config.h>

#include "ac3.h"
#include "dts.h"

namespace Reel
{
    UInt const bufferSize = 12 * 1024; 

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderAc3::Create()
    {
        if (!instance_)
        {
            instance_ = new AudioDecoderAc3;
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderAc3::Destroy() NO_THROW
    {
        delete instance_;
        instance_ = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderAc3 *AudioDecoderAc3::instance_ = 0;

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderDts::Create()
    {
        if (!instance_)
        {
            instance_ = new AudioDecoderDts;
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderDts::Destroy() NO_THROW
    {
        delete instance_;
        instance_ = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderDts *AudioDecoderDts::instance_ = 0;

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderIec60958::AudioDecoderIec60958()
    :   decodePacket_(0), emptyQueue_(0)
    {
        buffer_ = new Byte[bufferSize];
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderIec60958::~AudioDecoderIec60958() NO_THROW
    {
        delete[] buffer_;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderAc3::AudioDecoderAc3()
    {
        iec_ = &ac3;
        iec_->SetBuffer(buffer_);
        Reset(0);
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderDts::AudioDecoderDts()
    {
        iec_ = &dts;
        iec_->SetBuffer(buffer_);
        Reset(0);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderIec60958::Clean(AudioPacketQueue *emptyPacketQueue)
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
        ptsValid_          = false;
        out_               = 0;
        end_               = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioDecoderIec60958::ConvertSamples()
    {
        if (!inputSize_)
        {
            // No input pcm samples. Nothing to do.
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

        // Now convert the pcm frames to user format.

        Short const *from        = inputPtr_;
        AudioFrame  *to          = decodePacket_->GetFrames() + decodePacket_->GetNumFrames();
        UInt const   numFrames   = std::min(inputSize_, AudioPacket::MaxFrames - decodePacket_->GetNumFrames());
        AudioFrame  *const toEnd = to + numFrames;

        if (channel_ == AudioChannelMute)
        {
            std::memset(to, 0, numFrames * sizeof(AudioFrame));
            from += numFrames * 2;
            to = toEnd;
        }
        else
        {
            while (to != toEnd)
            {
                to->Samples.Left  = *from++;
                to->Samples.Right = *from++;
                ++ to;
            }
        }

        inputPtr_   = from;
        inputSize_ -= numFrames;
        decodePacket_->SetNumFrames(decodePacket_->GetNumFrames() + numFrames);

        if (decodePacket_->GetNumFrames() == AudioPacket::MaxFrames)
        {
            // ::printf("full\n");
            // Packet is full.
            OutputPacket();
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderIec60958::Decode()
    {
        AudioPacket *packet = outputPackets_.TryGet();
        if (packet)
        {
            return packet;
        }

        for (;;)
        {
        loop_start:
            // Exit if we have enough filled output packets.
            if (outputPackets_.GetNumElements() >= NumberOfAudioPackets / 2)
            {
                break;
            }

            // If there are any samples left, write them to the decode packet.
            if (ConvertSamples())
            {
                continue;
            }

            if (startFrame_)
            {
                if ((startFrame_ & 1) == 0)
                {
                    UInt size = bufferSize / 2;
                    std::memset(buffer_, 0, size);
                    inputPtr_ = reinterpret_cast<Short const *>(buffer_);
                    inputSize_ = size / 4;
                }
                else
                {
                    frame_t const &frame = iec_->Frame(PCM_WAIT2);
                    if (frame.burst)
                    {
                        inputPtr_ = reinterpret_cast<Short const *>(frame.burst);
                        inputSize_ = frame.size / 4;
                    }
                }
                -- startFrame_;
                continue;
            }

            // Parse the stream.
            while (out_ < end_)
            {
                frame_t const &frame = iec_->Frame(out_, end_);
                if (frame.burst)
                {
                    inputPtr_ = reinterpret_cast<Short const *>(frame.burst);
                    inputSize_ = frame.size / 4;
                    // ::printf("Frame\n");
                    goto loop_start;
                }
            }
            break;
        }

        // Return an output packet if there is one.
        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderIec60958::Flush()
    {
        // FIXME: Fillup packet with zeros.

        OutputPacket();

        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderIec60958::OutputPacket()
    {
        if (decodePacket_ && decodePacket_->GetNumFrames())
        {
            // REEL_ASSERT(decodePacket_->GetNumFrames() == AudioPacket::MaxFrames); // Must be full.

            // We have a decoded packet.
            outputPackets_.PutFilledPacket(decodePacket_);
            decodePacket_ = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderIec60958::Reset(AudioPacketQueue *emptyPacketQueue)
    {
        startFrame_ = 20;
        iec_->Reset(0);
        Clean(emptyPacketQueue);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderIec60958::PutEsPacket(Mpeg::EsPacket const &esPacket)
    {
        out_ = esPacket.GetData() + 3;
        end_ = out_ + (esPacket.GetDataLength() - 3);

        // ::printf("Put\n");

        sampleRate_ = SampleRate::SampleRate48K();

        if (esPacket.HasPts())
        {
            UInt const pts = esPacket.GetPts() + RBSetup.delay_ac3 * 900 - 20 * 900;

            // ::printf("Setup.ReplayDelay = %d\n", Setup.ReplayDelay);

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
