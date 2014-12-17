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

// AudioOut.c
 
#include "AudioOut.h"
#include "ReelBoxDevice.h"

#include <vdr/tools.h>

#include "HdCommChannel.h"


namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------
    
    namespace
    {
        // bool             lastWasSilent = true;

        UInt const       AlsaBufferNumFrames   = 0x2000; // FIXME
        UInt const       SilentPacketNumFrames = 0x800;
        AudioFrame const silentPacketData_[SilentPacketNumFrames] = {};
    };

    //--------------------------------------------------------------------------------------------------------------

    AudioOut::AudioOut(AudioPacketQueue &fullQueue,
                       AudioPacketQueue &emptyQueue)
    :   fullQueue_(fullQueue), emptyQueue_(emptyQueue),
        aVSyncListener_(0),
        stcInterpolationTime_(0),
        suspendCount_(0),
        aVSyncMode_(0),
        quit_(false),
        stcValid_(false),
        sampleRate_(SampleRate::SampleRate48K())
    {
        thread_.Start(*this, &AudioOut::Action);
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioOut::~AudioOut() NO_THROW
    {
        quit_ = true;
        thread_.Join();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::Action()
    {
        // This is the realtime playback thread. Be careful not to call any potentially blocking funcions other than
        // ALSA pcm operations.

        // ALSA mixer: Enable digitial output.
        SystemExec("amixer cset numid=38 1 > /dev/null\n");
        SystemExec("amixer cset numid=38 0 > /dev/null\n");

        Init();

        while (!StartPlayback() && !quit_)
        { 
            usleep (50*1000); // 500
        }

        if (!quit_)
        {
           MainLoop();
           StopPlayback();
        }
        Cleanup();
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::Cleanup()
    {
        // Free hardware params.
        ::snd_pcm_hw_params_free(hwParams_);
    }

    //--------------------------------------------------------------------------------------------------------------

    Int AudioOut::Delay() const
    {
        return fullQueue_.GetPlayTime() + latency_;
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioOut::HandleSampleRateChange(AudioPacket *packet)
    {
        SampleRate const sampleRate = packet->GetSampleRate();
        if (!sampleRate.IsSupported())
        {
            return false;
        }

        if (sampleRate != sampleRate_)
        {
            StopPlayback();
            sampleRate_ = sampleRate;
            StartPlayback();
        }
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::Init()
    {
        // Realtime prio = 5.
        thread_.SetSchedParam(SCHED_RR, 5);

        // Alloc hardware params.
        REEL_VERIFY(::snd_pcm_hw_params_malloc(&hwParams_) == 0);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::MainLoop()
    {
        while (!quit_)
        {
            if (!PlaybackFilledPacket())
            {
                // We have not played a filled packet for whatever reason.
                PlaybackSilentPacket();
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::PcmPlay(AudioFrame const *frames, UInt numFrames)
    {
        //::printf("PcmPlay\n");
        snd_pcm_sframes_t r = snd_pcm_writei(pcmHandle_, frames, numFrames);
        if (r == -EPIPE)
        {
            ::printf("PcmPlay, ALSA pcm underrun\n");
            // An underflow occured. This should normally never happen (we are realtime).
            REEL_VERIFY(snd_pcm_prepare(pcmHandle_) == 0);
            syslog(LOG_ERR, "ALSA pcm underrun");
            // Drop the data and then sleep to let other system threads run.
            StopPlayback();
            ::usleep(200000); // 200 ms
            StartPlayback();
        }
    } 

    bool AudioOut::Sync(bool &discard, AudioPacket *packet)
    {
        static int num_updates = 0; 
        static int num_runaways = 0;
        const int updates = 5;
        const int max_runaways = 10; 
        LLong diff = 40; //max diff in ms 
        LLong shift = 100; //shift in ms, > 0 ; play audio later

        diff *= (SampleRate::TicksPerSecond/1000);
        shift *= (SampleRate::TicksPerSecond/1000);

        if(++num_updates%updates != 0)
        {
            return false;
        }
        else
        {
            UpdateExtStc();
        }

        if((stc_ + shift) - extStc_ > diff || extStc_ - (stc_ + shift) > diff)
        {
            num_runaways++;
        }
        else
        {
            num_runaways = 0;
        }

        //printf("--extStc_ - stc_ = %lld, num_runaways = %d, num = %d--\n", extStc_ - stc_, num_runaways, fullQueue_.GetNumElements());

        if (num_runaways > max_runaways && !emptyQueue_.Empty() && (stc_ + shift) - extStc_ >  diff) //video to late
        { 
            printf("-------Sync: play zero packet----------\n"); 
            discard = true; 
            num_runaways = 0;
            return true;
        }

        if (num_runaways > max_runaways && fullQueue_.GetNumElements() > 3 && extStc_ - (stc_ + shift) > diff) //audio to late
        { 
            printf("-------Sync: discard packet----------\n");
            fullQueue_.Pop();
            emptyQueue_.PutEmptyPacket(packet);
            UpdateStc(packet); 
            discard = false;  
            num_runaways = 0;
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioOut::PlaybackFilledPacket()
    {
        if (suspendCount_)
        {
            // Suspended.;
            return false;
        }

        AudioPacket *packet = fullQueue_.Peek();
        if (!packet)
        {
            // No packet in queue.
            // ::printf("No packet in queue!\n");
            return false;
        }

        if (packet->GetGeneration() != fullQueue_.GetGeneration())
        {
            ::printf("Stale packet!\n");

            // Packet stale, discard.
            fullQueue_.Pop();
            emptyQueue_.PutEmptyPacket(packet);
            return true;
        }
 
        if(RBSetup.usehdext) //extension provides the stc
        {
            bool discard = false;
            if(Sync(discard, packet))
            {
                return !discard;
            }
        }

        /*if (!aVSyncMode_ && fullQueue_.GetNumElements() > 16) // Testcode
        {
            static int count = 0;

            if (++count > 1000)
            {
                ::printf("Drop Audio Packet\n");
                fullQueue_.Pop();
                emptyQueue_.PutEmptyPacket(packet);
                return true;
            }
        }*/

        // All packets now in the queue are not stale.
        // Delay playback if AV sync.
        // Note: To avoid deadlocks, we must ensure that the empty queue still contains packets.
        if (aVSyncMode_ && !emptyQueue_.Empty() && fullQueue_.GetPlayTime() < AVSyncPlayTimeThreshold)
        { 
            //return false; may interfere with sync to external stc from hdext
        }

        aVSyncMode_ = false;

        // Play the first packet in the queue.
        fullQueue_.Pop();

        if (HandleSampleRateChange(packet))
        {
            // Playback if we can handle the samplerate.
            //printf("------------------PcmPlay-------------------------\n");
            PcmPlay(packet->GetFrames(), packet->GetNumFrames());
            /*if (lastWasSilent) // FIXME
            {
                //::printf("Filled packet!\n");
                lastWasSilent = false;
            }*/
        }

        // Update the system time counter.
        UpdateStc(packet);

        // Inform the AV sync listener.
        if (aVSyncListener_)
        {
            UInt stc = implicit_cast<UInt>(stc_ / SampleRate::TicksPerMpegTick);
            aVSyncListener_->SetStc(stcValid_, stc);
            if(ReelBoxDevice::Instance())
               ReelBoxDevice::Instance()->SetStc(stcValid_, stc);
        }

        // Return packet to empty queue.
        emptyQueue_.PutEmptyPacket(packet);

        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::PlaybackSilentPacket()
    {
        stcValid_ = false;
        if (!aVSyncMode_ && aVSyncListener_)
        {
            // Inform the AV sync listener about the invalid STC.
            aVSyncListener_->SetStc(false, 0);
        }
        PcmPlay(silentPacketData_, SilentPacketNumFrames);
    }

    //--------------------------------------------------------------------------------------------------------------

    bool AudioOut::StartPlayback()
    {
        UInt rate = sampleRate_.GetHz();

        // Set the iec sample rate first.
        char iecCmd[64];
        ::sprintf(iecCmd, "iecset rate %d", rate);
        SystemExec(iecCmd);

        #ifdef RBLITE
        char hwdevice[64] = "hw:0,0";
        #else
        char hwdevice[64] = "plug:dmix";
        #endif

        // Open pcm playback.
        if (::snd_pcm_open(&pcmHandle_, hwdevice, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        {
           esyslog ("cannot open audio device  %s ", hwdevice);
           return false; 
        }

        // Get full hardware config space.
        REEL_VERIFY(::snd_pcm_hw_params_any(pcmHandle_, hwParams_) >= 0);

        // Access only with snd_pcm_writei().
        REEL_VERIFY(::snd_pcm_hw_params_set_access(pcmHandle_, hwParams_,
                                                   SND_PCM_ACCESS_RW_INTERLEAVED) == 0);

        // Use only signed 16-bit samples, low endian.
        REEL_VERIFY(::snd_pcm_hw_params_set_format(pcmHandle_, hwParams_,
                                                   SND_PCM_FORMAT_S16_LE) == 0);

        // Disable resampling. FIXME
        /* REEL_VERIFY(::snd_pcm_hw_params_set_rate_resample(pcmHandle_, hwParams_,
                                                            0) == 0); */

        // Set samplerate.
        REEL_VERIFY(::snd_pcm_hw_params_set_rate_near(pcmHandle_, hwParams_,
                                                      &rate,
                                                      0) == 0);
        REEL_ASSERT(rate == sampleRate_.GetHz());

        // Set 2 channels only.
        REEL_VERIFY(::snd_pcm_hw_params_set_channels(pcmHandle_, hwParams_,
                                                     2) == 0);

        // Using 8 periods.
        REEL_VERIFY(::snd_pcm_hw_params_set_periods(pcmHandle_, hwParams_,
                                                    8,
                                                    0) == 0);

        // Buffer size in frames.
        REEL_VERIFY(::snd_pcm_hw_params_set_buffer_size(pcmHandle_, hwParams_,
                                                        AlsaBufferNumFrames) == 0);

        // Apply configuration.
        REEL_VERIFY(::snd_pcm_hw_params(pcmHandle_, hwParams_) == 0);

        // Fill playback buffer by writing slient data.
        for (UInt n = 0; n < AlsaBufferNumFrames / SilentPacketNumFrames; ++n)
        {
            PlaybackSilentPacket();
        }

        latency_ = sampleRate_.GetTicksPerFrame() * AlsaBufferNumFrames;
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::StopPlayback()
    {
        // Stop playback immediately.
        REEL_VERIFY(::snd_pcm_drop(pcmHandle_) == 0);

        // Close pcm playback handle.
        REEL_VERIFY(::snd_pcm_close(pcmHandle_) == 0);
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioOut::UpdateStc(AudioPacket *filledPacket)
    {
        if (filledPacket->GetGeneration() != fullQueue_.GetGeneration())
        {
            stcValid_ = false;
        }

        // The end of the played packet is now at the end of the pcm buffer.
        if (filledPacket->HasPts())
        {
            // We set the STC after the packets PTS.
            stcValid_ = true;
            stcInterpolationTime_ = 0;
            stc_ = implicit_cast<LLong>(filledPacket->GetPts()) * SampleRate::TicksPerMpegTick
                       + AudioPacketPlayTime(*filledPacket) - latency_;

        }
        else if (stcValid_)
        {
            UInt packetPlayTime = AudioPacketPlayTime(*filledPacket);

            // Interpolate STC.
            stc_ += packetPlayTime;

            // We do not want to interpolate the STC for extended periods if no PTS is encountered.
            stcInterpolationTime_ += packetPlayTime;
            if (stcInterpolationTime_ >= StcMaxInterpolation)
            {
                stcValid_ = false;
            }
        }
    }

    /*ULLong AudioOut::GetSysTime() 
    {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return ULLong(tv.tv_sec) * 1000000 + tv.tv_usec;
    }*/

    void AudioOut::UpdateExtStc()
    {
        int stc = 0;
        if(HdCommChannel::hda)
	   stc = HdCommChannel::hda->player[0].hde_stc - HdCommChannel::hda->player[0].pts_offset;
        //convert to unsigned presentation and standard ticks 
        extStc_ = implicit_cast<LLong>(static_cast<uint>(stc)) * SampleRate::TicksPerMpegTick;
        /*static int num = 0;
        static LLong lastSysTime = 0;
        static LLong lasttestpts = 0;
        LLong testpts = 0;
        if(initial || ++num%2000 == 0)
        { 
             printf("----------GetPTS-----------------\n");
             extStc_ = implicit_cast<LLong>((uint)(HdCommChannel::hda->player[0].hde_stc)) * SampleRate::TicksPerMpegTick;
             lastSysTime = GetSysTime();
        }
        else
        {
             extStc_ += ((GetSysTime() - lastSysTime) * SampleRate::TicksPerSecond) / 1000000;
             lastSysTime = GetSysTime();
             //printf("------------interploated pts = %lld------------\n", pts_);
        }*/
    }

    //--------------------------------------------------------------------------------------------------------------
}
