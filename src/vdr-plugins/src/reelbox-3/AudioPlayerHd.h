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

// AudioPlayerHd.h

#ifndef AUDIO_PLAYER_HD_H_INCLUDED
#define AUDIO_PLAYER_HD_H_INCLUDED

#include "AudioPlayer.h"

#include "HdCommChannel.h"

namespace Reel
{
    const int64_t CLOCK_RESOLUTION = 1000;
    class SyncClock  //synchronized clock 
    {
        public:
            SyncClock():
            timediff_(0LL){}
            void SetClock(ULLong time)
            {
                //printf("---SyncClock:SetClock, time = %0llx, systime = %0llx, clock->timediff_ = %0llx-----\n", time, GetSysTime(), timediff_);
                int64_t correction = time - (GetSysTime() + timediff_);
                if (correction > CLOCK_RESOLUTION)
                {
                    //printf("-----Reel:SyncClock:SetClock, timediff = %0llx------\n",  timediff_);
                    timediff_ += correction - CLOCK_RESOLUTION; //TODO: Kleine Schritte
                }
            }
            ULLong GetClock() const
            {
                ULLong time = GetSysTime() + timediff_;
                //printf("-----Reel:SyncClock:GetClock, time = %0llx------\n",  time);
                return time;
            }
            static ULLong GetSysTime() 
            {
                struct timeval tv;
                gettimeofday(&tv, 0);
                return ULLong(tv.tv_sec) * 1000000 + tv.tv_usec;
            }
        private:
            ULLong timediff_;
    };

    class AudioPlayerHd : public AudioPlayer /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.

        /* override */ void Clear();

        /* override */ Int Delay() const;

        /* override */ bool Flush();

        /* override */ void Freeze();

        /* override */ void Play();

        /* override */ void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false);

        /* override */ void PlayFrames(AudioFrame const *frames, Int numFrames,
                                       SampleRate sampleRate, UInt pts, bool isAc3);

        /* override */ bool Poll();

        /* override */ bool PollAudio();

        /* override */ void SetAVSyncListener(AVSyncListener *listener);

        /* override */ void SetChannel(AudioChannel audioChannel);

        /* override */ void Start();

        /* override */ void Stop();

    protected:

        AudioPlayerHd(); // Allow construction only through Create().
        ~AudioPlayerHd() NO_THROW; // Allow destruction only through Destroy().

        AudioPlayerHd(AudioPlayerHd const &); // Forbid copy construction.
        AudioPlayerHd &operator=(AudioPlayerHd const &); // Forbid copy assignment.

        void SetCorrectionBase(uint sampleRate);
        void SynchronizeClock();
        void SynchronizeClockInitial();
        SyncClock clock_;
        ::hd_player_t volatile &hdPlayer_; // Hd shared mem struct.
        uint currentSampleRate_;
        ULLong frameNr_;

    private:

        AudioPacketQueue    emptyQueue_;
        AudioPacketQueue    fullQueue_;

        AudioDecoder       *lastDecoder_;

        AudioChannel        audioChannel_;

        bool freeze_;

        bool rawaudio_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline Int AudioPlayerHd::Delay() const
    {
        //::printf("AudioPlayerHd::Delay()\n");
        //int64_t correction = hdPlayer_.correction_diff - LongVal(hdPlayer_.correction_base_high, hdPlayer_.correction_base_low);
        //int64_t currentFrame = (clock_.GetClock() * (currentSampleRate_ / 100) + correction) / 10000;
        //int delay = frameNr_ - currentFrame; //linearly interpolated delay
        //printf("-----AudioPlayerHd::currentFrame = %lld-------frameNr = %lld--------\n", currentFrame, frameNr_);
        //printf("-----------------AudioPlayerHd::delay  = %d----------------------\n", delay);
        int alternative_delay = frameNr_ - hdPlayer_.current_frame; //simple delay  
        //printf("-----------------AudioPlayerHd:: alternative_delay = %d----------------------\n", alternative_delay);

        /*static int old_delay = 0;
        static int old_alternative_delay = 0;
        static int64_t old_frameNr_ = 0ll;
        static int64_t old_currentFrame = 0ll;
        static int64_t old_hdPlayer_current_frame = 0ll;
        static int64_t old_systime = 0ll;
        int64_t systime = clock_.GetSysTime();
        int diff = 10000;
        if(old_delay - delay > diff || delay - old_delay > diff)
        {
            printf("-----------------AudioPlayerHd::delay  = %d----------------------\n", delay);
            printf("-----------------AudioPlayerHd::old_delay  = %d----------------------\n", old_delay);
            printf("-----------------AudioPlayerHd::alternative_delay  = %d----------------------\n", alternative_delay);
            printf("-----------------AudioPlayerHd::old_alternative_delay  = %d----------------------\n", old_alternative_delay);
            printf("-----------------AudioPlayerHd::frameNr_  = %lld----------------------\n", frameNr_);
            printf("-----------------AudioPlayerHd::old_frameNr_  = %lld----------------------\n", old_frameNr_);
            printf("-----------------AudioPlayerHd::currentFrame  = %lld----------------------\n", currentFrame);
            printf("-----------------AudioPlayerHd::old_currentFrame  = %lld----------------------\n", old_currentFrame);
            printf("-----------------AudioPlayerHd::hdPlayer_.current_frame  = %lld----------------------\n", hdPlayer_.current_frame);
            printf("-----------------AudioPlayerHd::old_hdPlayer_current_frame = %lld----------------------\n", old_hdPlayer_current_frame);
            printf("-----------------AudioPlayerHd::systime = %lld----------------------\n", systime);
            printf("-----------------AudioPlayerHd::old_systime  = %lld----------------------\n", old_systime);
        }
        old_delay = delay;
        old_alternative_delay = alternative_delay;
        old_frameNr_ = frameNr_;
        old_currentFrame = currentFrame;
        old_hdPlayer_current_frame = hdPlayer_.current_frame;
        old_systime = systime;*/

	if(alternative_delay > 0 && alternative_delay < 1000000)
        {
            return alternative_delay;
        }
        else
        {
            return 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPlayerHd::Flush()
    {
        ::printf("AudioPlayerHd::Flush()\n");
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPlayerHd::Poll()
    {
        if(rawaudio_)
        {
#ifdef RBLITE
           return Delay() < 50000 && !freeze_;  //TODO: test with RBlite
#else	
           return Delay() < 90000 && !freeze_;  //needed for mp3 plugin
#endif	
        }
        else
        {
            return !freeze_;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    //TODO: Unify polling (no more extra audio/video polling for xine)!
    inline bool AudioPlayerHd::PollAudio()  //needed fo xine
    {
	    return !freeze_;
#ifdef RBLITE
       return Delay() < 50000 && !freeze_;
#else	
       return Delay() < 75000 && !freeze_;
#endif		
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPlayerHd::SetAVSyncListener(AVSyncListener *listener)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPlayerHd::SetChannel(AudioChannel audioChannel)
    {
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def AUDIO_PLAYER_HD_H_INCLUDED
