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

// AudioDecoderPcm.c
 
#include "AudioDecoderPcm.h"
#include "MpegPes.h"
#include "ReelBoxMenu.h"

#include <algorithm>

#include <vdr/config.h>

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::Create()
    {
        if (!instance_)
        {
            instance_ = new AudioDecoderPcm;
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::Destroy() NO_THROW
    {
        delete instance_;
        instance_ = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderPcm *AudioDecoderPcm::instance_ = 0;

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderPcm::AudioDecoderPcm()
    :   decodePacket_(0), emptyQueue_(0), swapByteOrder_(false)
    {
        Reset(0);
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioDecoderPcm::~AudioDecoderPcm() NO_THROW
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::AttachPts(UInt pts)
    {
        pts += RBSetup.delay_stereo * 900 - 20 * 900;

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

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::Clean(AudioPacketQueue *emptyPacketQueue)
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
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioDecoderPcm::ConvertSamples()
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

        Byte const  *from        = inputPtr_;
        AudioFrame  *to          = decodePacket_->GetFrames() + decodePacket_->GetNumFrames();
        UInt const   numFrames   = std::min(inputSize_, AudioPacket::MaxFrames - decodePacket_->GetNumFrames());
        AudioFrame  *const toEnd = to + numFrames;

        switch (channel_)
        {
        case AudioChannelStereo:
            if (swapByteOrder_)
            {
                while (to != toEnd)
                {
                    to->Samples.Left = (*from << 8) | (*(from + 1));
                    to->Samples.Right = (*(from + 2) << 8) | (*(from + 3));
                    from += 4;
                    ++ to;
                }
            }
            else
            {
                while (to != toEnd)
                {
                    to->Samples.Left = *reinterpret_cast<short const *>(from);
                    to->Samples.Right = *reinterpret_cast<short const *>(from + 2);
                    from += 4;
                    ++ to;
                }
            }
            break;
        case AudioChannelMonoLeft:
            if (swapByteOrder_)
            {
                while (to != toEnd)
                {
                    Short const sample = (*from << 8) | (*(from + 1));
                    to->Samples.Left = sample;
                    to->Samples.Right = sample;
                    from += 4;
                    ++ to;
                }
            }
            else
            {
                while (to != toEnd)
                {
                    Short const sample = *reinterpret_cast<short const *>(from);
                    to->Samples.Left = sample;
                    to->Samples.Right = sample;
                    from += 4;
                    ++ to;
                }
            }
            break;
        case AudioChannelMonoRight:
            if (swapByteOrder_)
            {
                while (to != toEnd)
                {
                    Short const sample = (*(from + 2) << 8) | (*(from + 3));
                    to->Samples.Left = sample;
                    to->Samples.Right = sample;
                    from += 4;
                    ++ to;
                }
                break;
            }
            else
            {
                while (to != toEnd)
                {
                    Short const sample = *reinterpret_cast<short const *>(from + 2);
                    to->Samples.Left = sample;
                    to->Samples.Right = sample;
                    from += 4;
                    ++ to;
                }
            }
            break;
        case AudioChannelMute:
            std::memset(to, 0, numFrames * sizeof(AudioFrame));
            to = toEnd;
            from += numFrames * sizeof(AudioFrame);
            break;
        }

        inputPtr_   = from;
        inputSize_ -= numFrames;
        decodePacket_->SetNumFrames(decodePacket_->GetNumFrames() + numFrames);

        if (decodePacket_->GetNumFrames() == AudioPacket::MaxFrames)
        {
            // Packet is full.
            OutputPacket();
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderPcm::Decode()
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

            // If there are any samples left, write them to the decode packet.
            if (!ConvertSamples())
            {
                break;
            }
        }

        // Return an output packet if there is one.
        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioDecoderPcm::Flush()
    {
        // FIXME: Fillup packet with zeros.

        OutputPacket();

        return outputPackets_.TryGet();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::OutputPacket()
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

    void AudioDecoderPcm::Reset(AudioPacketQueue *emptyPacketQueue)
    {
        Clean(emptyPacketQueue);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::PutEsPacket(Mpeg::EsPacket const &esPacket)
    {
        Byte const *lpcmHeader = esPacket.GetData();
        UInt const dataSize = esPacket.GetDataLength();

        Byte quantLength = lpcmHeader[4] & 0xC0;
        Byte sampleFreq  = lpcmHeader[4] & 0x30;
        Byte chanCount   = lpcmHeader[4] & 0x07;

        bool supported = !quantLength && chanCount == 1;

        switch (sampleFreq)
        {
        case 0x00:
            sampleRate_ = SampleRate::SampleRate48K();
            break;
        case 0x20:
            sampleRate_ = SampleRate::SampleRate44_1K();
            break;
        default:
            supported = false;
        }

        if (supported)
        {
            inputPtr_ = lpcmHeader + 6;
            inputSize_ = (dataSize - 6) / 4; // Counted in frames.
            swapByteOrder_ = true; // LPCM is high endian.

            if (esPacket.HasPts())
            {
                AttachPts(esPacket.GetPts());
            }
        }
        else
        {
            // ::printf("Unsupported PCM\n");
            inputSize_ = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioDecoderPcm::PutFrames(AudioFrame const *frames, Int numFrames,
                                    SampleRate sampleRate, UInt pts)
    {
        inputPtr_ = static_cast<Byte const *>(implicit_cast<void const *>(frames));
        inputSize_ = numFrames;
        sampleRate_ = sampleRate;
        swapByteOrder_ = false;

        if (pts)
        {
            AttachPts(pts);
        }
    }

    //--------------------------------------------------------------------------------------------------------------
}
