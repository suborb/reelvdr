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

// AudioPlayerBsp.c

#include "AudioPlayerBsp.h"

#include "AudioDecoderNull.h"
#include "AudioDecoderPcm.h"

#define DEBUG_AUDIO_BSP(format, args...) printf (format, ## args)
//#define DEBUG_AUDIO_BSP(format, args...) 

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Create()
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        if (!instanceBsp_)
        {
            instanceBsp_ = new AudioPlayerBsp;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPlayerBsp::AudioPlayerBsp()
    :   audioChannel_(AudioChannelStereo)
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        CreateAudioDecoders();

        for (AudioDecoders::Iterator audioDecoder = AudioDecoders::Begin();
                                     audioDecoder != AudioDecoders::End();
                                   ++audioDecoder)
        {
            audioDecoder->Reset(&emptyQueue_);
        }

        lastDecoder_ = AudioDecoderNull::Instance();

        // Create empty audio packets.
        for (UInt n = 0; n < NumberOfAudioPackets; ++n)
        {
            emptyQueue_.PutEmptyPacket(new AudioPacket);
        }

        audioOut_ = new AudioOut(fullQueue_, emptyQueue_);
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPlayerBsp::~AudioPlayerBsp() NO_THROW
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        delete audioOut_;

        //lastDecoder_->Reset(&emptyQueue_);
        DestroyAudioDecoders();
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  END\033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Clear()
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        fullQueue_.IncrementGeneration();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Freeze()
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        freeze_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::PlayFrames(AudioFrame const *frames, Int numFrames,
                                 SampleRate sampleRate, UInt pts, bool isAc3)
    {
        if (freeze_)
        {
            return;
        }

        DEBUG_AUDIO_BSP("[reelbox] %s \n",__PRETTY_FUNCTION__);
        MutexLocker lock(mutex_);

        // Use the PCM-Decoder.
        AudioDecoderPcm *decoder = AudioDecoderPcm::Instance();

        if (decoder != lastDecoder_)
        {
            // Playback the remaining data in the last decoder.
            AudioPacket *packet = lastDecoder_->Flush();
            if (packet)
            {
                fullQueue_.PutFilledPacket(packet);
            }

            // Reset last decoder.
            lastDecoder_->Reset(&emptyQueue_);
            lastDecoder_ = decoder;

            // Set the audio channel of the current decoder.
            decoder->SetChannel(audioChannel_);

            // Invalidate the stc interpolation until next timestamp.
            audioOut_->InvalidateStc();
        }

        decoder->PutFrames(frames, numFrames, sampleRate, pts);

        AudioPacket *packet;
        while ((packet = decoder->Decode())) // assignment
        {
            fullQueue_.PutFilledPacket(packet);
        }
    }

    void AudioPlayerBsp::PlayPacket(Mpeg::EsPacket const &esPacket, bool still)
    {
        if (freeze_)
        {
            return;
        }

        MutexLocker lock(mutex_);

        AudioDecoder *decoder = esPacket.GetAudioDecoder();

        if (decoder != lastDecoder_)
        {
            // Playback the remaining data in the last decoder.
            AudioPacket *packet = lastDecoder_->Flush();
            if (packet)
            {
                fullQueue_.PutFilledPacket(packet);
            }

            // Reset last decoder.
            lastDecoder_->Reset(&emptyQueue_);
            lastDecoder_ = decoder;

            // Set the audio channel of the current decoder.
            decoder->SetChannel(audioChannel_);

            // Invalidate the stc interpolation until next timestamp.
            audioOut_->InvalidateStc();
        }

        decoder->PutEsPacket(esPacket);
        AudioPacket *packet;
        while ((packet = decoder->Decode())) // assignment
        {
            fullQueue_.PutFilledPacket(packet);
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Play()
    {
    //    DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        MutexLocker lock(mutex_);

        Clear();
        freeze_  = false;

        // Enter AV-Sync mode.
        audioOut_->StartAVSync();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Start()
    {
        DEBUG_AUDIO_BSP("\033[0;45m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        Play();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerBsp::Stop()
    {
        DEBUG_AUDIO_BSP("[reelbox] %s \n",__PRETTY_FUNCTION__);
        Clear();
    }

    //--------------------------------------------------------------------------------------------------------------
}
