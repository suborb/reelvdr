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

// AudioPlayerHd.c

#include "AudioPlayerHd.h"

#include "HdCommChannel.h"

//#define DEBUG_AUDIO_HD(format, args...) printf (format, ## args)
#define DEBUG_AUDIO_HD(format, args...) 

#include "AudioDecoderPcm.h"
#include "AudioDecoderNull.h"

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Create()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s \033\[0m\n", __PRETTY_FUNCTION__);
        if (!instanceHd_)
        {
            instanceHd_ = new AudioPlayerHd;
        }

    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPlayerHd::AudioPlayerHd()
    :   hdPlayer_(HdCommChannel::hda->player[0]), frameNr_(0),
        freeze_(false), rawaudio_(false)
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
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
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPlayerHd::~AudioPlayerHd() NO_THROW
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        //lastDecoder_->Reset(&emptyQueue_);
        DestroyAudioDecoders();
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  END \033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Clear()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Freeze()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        freeze_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Play()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        freeze_ = false;
        SynchronizeClockInitial();
        currentSampleRate_ = 0;
    }

//--------------------------------------------------------------------------------------------------------------
    void AudioPlayerHd::SetCorrectionBase(uint sampleRate)
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        unsigned long long correction_base = SyncClock::GetSysTime() * (sampleRate / 100);
        hdPlayer_.correction_base_low = LowWord(correction_base);
        hdPlayer_.correction_base_high = HighWord(correction_base);
    }

    void AudioPlayerHd::SynchronizeClock()
    {
        uint time_high = hdPlayer_.clock_high;  //Order dependancy!
        uint time_low = hdPlayer_.clock_low;
        ULLong time = LongVal(time_high, time_low);

        clock_.SetClock(time);
        time = clock_.GetClock();

        hdPlayer_.clock_low = 0;      //Order dependancy!
        hdPlayer_.clock_low =  LowWord(time);
        hdPlayer_.clock_high = HighWord(time);
    }

    #define SYNC_INITIAL_MS 100
    void AudioPlayerHd::SynchronizeClockInitial()
    {
        struct timeval tv1,tv2;
	ULLong t1,t2;
	return; // GA
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
	gettimeofday(&tv1, 0);
	t1=ULLong(tv1.tv_sec) * 1000000 + tv1.tv_usec;
        while(1) {
		SynchronizeClock();
		usleep(1000);	      
		gettimeofday(&tv2, 0); 
		t2=ULLong(tv2.tv_sec) * 1000000 + tv2.tv_usec;
		if ((t2-t1)>SYNC_INITIAL_MS*1000LL)
			break;		
	}
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  END\033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::PlayFrames(AudioFrame const *frames, Int numFrames,
                                   SampleRate sampleRate, UInt pts, bool isAc3)
    {
        if (sampleRate.GetHz() != currentSampleRate_)
        {
            currentSampleRate_ = sampleRate.GetHz();
            SetCorrectionBase(currentSampleRate_);
            frameNr_ = 0;
        }

        static int nr = 0;
        if(++nr%10 == 0)
        {
            SynchronizeClock();
        }

        if (!freeze_)
        {
            Byte const *data = reinterpret_cast<Byte const *>(frames);
            int dataLength = numFrames * sizeof(AudioFrame);

            hd_packet_af_data_t packet;
            packet.sampleRate = sampleRate.GetHz();
            packet.frame_nr = frameNr_;
            frameNr_ += numFrames;
            packet.pts = pts;

            if(!isAc3)
            {
                HdCommChannel::chStream1.SendPacket(HD_PACKET_AF_DATA, packet, data, dataLength);
            }
            else
            {
                HdCommChannel::chStream1.SendPacket(HD_PACKET_AC3_DATA, packet, data, dataLength);
            }

        }
    }
    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::PlayPacket(Mpeg::EsPacket const &esPacket, bool still)
    {
        if (freeze_)
        {
            return;
        }

        rawaudio_ = true;

        AudioDecoder *decoder = esPacket.GetAudioDecoder();

        if (decoder != lastDecoder_)
        {
            if(dynamic_cast<AudioDecoderPcm*>(lastDecoder_) != NULL) //switch from pcm to ?
            // Playback the remaining data in the last decoder.
            {
                AudioPacket *packet = lastDecoder_->Flush();
                if (packet)
                {
                    AudioFrame const *frames = packet->GetFrames();
                    Int numFrames = packet->GetNumFrames();
                    UInt pts = packet->GetPts();
                    SampleRate sampleRate = packet->GetSampleRate();
                    PlayFrames(frames, numFrames, sampleRate, pts, false);
                }
            }

            // Reset last decoder.
            lastDecoder_->Reset(&emptyQueue_);
            lastDecoder_ = decoder;

            // Set the audio channel of the current decoder.
            decoder->SetChannel(audioChannel_);
        }

        if(dynamic_cast<AudioDecoderPcm*>(decoder) == NULL) //no pcm
        {
            //send undecoded data to hdext
            rawaudio_ = false;

            Byte const *data = esPacket.GetData();
            int dataLength = esPacket.GetDataLength();

            hd_packet_es_data_t packet;
	    packet.generation = -1;
            packet.timestamp = esPacket.HasPts() ? esPacket.GetPts() : 0;
            packet.stream_id = esPacket.GetStreamId();
            packet.substream_id = esPacket.GetSubStreamId();

            HdCommChannel::chStream1.SendPacket(HD_PACKET_ES_DATA, packet, data, dataLength);
        }
        else //pcm!
        { 
            decoder->PutEsPacket(esPacket);

            AudioPacket *packet;
            while ((packet = decoder->Decode())) // assignment
            {
                AudioFrame const *frames = packet->GetFrames();
                Int numFrames = packet->GetNumFrames();
                UInt pts = packet->GetPts();
                SampleRate sampleRate = packet->GetSampleRate();
                PlayFrames(frames, numFrames, sampleRate, pts, false);
                emptyQueue_.PutEmptyPacket(packet);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Start()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  \033\[0m\n", __PRETTY_FUNCTION__);
        SynchronizeClockInitial();
        currentSampleRate_ = 0;
        freeze_ = 0;
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  END\033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPlayerHd::Stop()
    {
        DEBUG_AUDIO_HD("\033[0;43m [reelbox] %s  END\033\[0m\n", __PRETTY_FUNCTION__);
    }

    //--------------------------------------------------------------------------------------------------------------
}
