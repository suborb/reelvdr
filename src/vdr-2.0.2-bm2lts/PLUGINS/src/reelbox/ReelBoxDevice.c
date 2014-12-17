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

// ReelboxDevice.c

#include "ReelBoxDevice.h"
#include "ReelBoxMenu.h"

#include "AudioPlayerBsp.h"
#include "VideoPlayerBsp.h"
#include "VideoPlayerPipBsp.h"
#include "AudioPlayerHd.h"
#include "VideoPlayerHd.h"
#include "VideoPlayerPipHd.h"

#include "config.h"

#include <vdr/dvbspu.h>
#include <vdr/channels.h>

#include <alsa/asoundlib.h>
#include <iostream>
#include <vector>

#include "BspOsdProvider.h"
#include "HdOsdProvider.h"
#include "fs453settings.h"

//#define DEBUG_DEVICE(format, args...) printf (format, ## args)
#define DEBUG_DEVICE(format, args...)

#define BP  //
//#define BP

// setting the volume on Lite + AVG
#ifdef RBLITE
       #define SET_VOLUME(volume) SetVolume(volume, "Master")
       #define MUTE(switch)       SetVolume(0, switch)
       #define UNMUTE(switch)     SetVolume(23, switch)
#else
       #define SET_VOLUME(volume) SetVolume(volume, "Front")
       #define MUTE(switch)
       #define UNMUTE(switch)
#endif


namespace Reel
{
#define CHECK_CONCURRENCY ((void)0)

    ReelBoxDevice *ReelBoxDevice::instance_;

    ReelBoxDevice::ReelBoxDevice()
    :   digitalAudio_(false), pipActive_(false), audioChannel_(0),
#if VDRVERSNUM < 10716
        audioPlayback_(0), videoPlayback_(0),
#endif
        playMode_(pmNone),
        bkgPicPlayer_(*this),
        useHDExtension_(RBSetup.usehdext),
        audioOverHDMI_(0),//RBSetup.audio_over_hdmi)!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        audioOverHd_(RBSetup.audio_over_hd),
        needRestart(false),
        normalPlay(false)
    {
        DEBUG_DEVICE("\033[0;44m [reelbox] %s  \033[0m \n", __PRETTY_FUNCTION__);
        instance_ = this;

        SystemExec("iecset audio on");

        if (useHDExtension_)
        {
            // HD
            DEBUG_DEVICE("\033[0;44m [reelbox] use hdextension AoHDMO ? %s \033[0m \n", audioOverHDMI_?"YES":"NO" );
            VideoPlayerHd::Create();
            VideoPlayerPipHd::Create();
            tmpHDaspect = -1;
#ifdef RBLITE
            if (!audioOverHDMI_ && digitalAudio_ )
#else
            if (!audioOverHd_ && !digitalAudio_)
#endif
            {
                audioPlayerHd_ = 0;
                AudioPlayerBsp::Create(); // audio over bsp
                audioPlayerBsp_ = &AudioPlayer::InstanceBsp();
            }
            else
            {
                audioPlayerBsp_ = 0;
                AudioPlayerHd::Create(); // audio over hd
                audioPlayerHd_ = &AudioPlayer::InstanceHd();
            }
        }
        else
        {
            // BSP
            DEBUG_DEVICE("\033[0;44m [reelbox] BSP only \033[0m \n");

            audioOverHDMI_ = false; // for save
            VideoPlayerBsp::Create();
            AudioPlayerBsp::Create();
            VideoPlayerPipBsp::Create();
            audioPlayerBsp_ = &AudioPlayer::InstanceBsp();
            audioPlayerHd_ = 0;
        }
        videoPlayer_ = &VideoPlayer::Instance();

        if(audioPlayerBsp_)
        {
            audioPlayerBsp_->SetAVSyncListener(videoPlayer_);
        }
        else
        {
            videoPlayer_->SetStc(false, 0);
        }
        videoPlayerPip_ = &VideoPlayerPip::Instance();
	    audioBackgroundPics_ = true;

#if VDRVERSNUM >= 10716
        bkgPicPlayer_.Start();
#endif
    }


    ReelBoxDevice::~ReelBoxDevice()
    {
        DEBUG_DEVICE("\033[0;44m [reelbox] %s  \033[0m \n", __PRETTY_FUNCTION__);
        instance_ = 0;

#if VDRVERSNUM >= 10716
        bkgPicPlayer_.Stop();
#endif

        if (audioPlayerBsp_)
        {
           audioPlayerBsp_->SetAVSyncListener(0);
        }

        VideoPlayerPip::Destroy();
        AudioPlayerBsp::Destroy();
        AudioPlayerHd::Destroy();
        VideoPlayer::Destroy();
    }

    Int ReelBoxDevice::AudioDelay() const
    {
        DEBUG_DEVICE("[reelbox]  %s  \n", __PRETTY_FUNCTION__);
        if (audioPlayerHd_)
        {
            return audioPlayerHd_->Delay();
        }
        else if(audioPlayerBsp_)
        {
            return audioPlayerBsp_->Delay();
        }
        return 0;
    }

    void ReelBoxDevice::RestartAudio()
    {
        audioOverHDMI_ = RBSetup.audio_over_hdmi;
        audioOverHd_ = RBSetup.audio_over_hd;
        printf("RestartAudio, audioOverHDMI_ = %d, digitalAudio_ = %d, audioOverHd = %d\n", (int) audioOverHDMI_, (int)digitalAudio_, (int)audioOverHd_ );

        bool switchToBspAudio = false;
        bool switchToHdAudio = false;

#ifdef RBLITE
        if(!audioOverHDMI_ && digitalAudio_ )
#else
        if(!audioOverHd_ && !digitalAudio_)
#endif
        {
            if(audioPlayerHd_)
            {
                switchToBspAudio = true;
            }
        }
#ifdef RBLITE
        else if(useHDExtension_ && audioPlayerBsp_)
#else
        else //if(audioOverHd_ || digitalAudio_)
#endif
        {
            if(audioPlayerBsp_)
            {
                switchToHdAudio = true;
            }
        }

        if(switchToHdAudio)
        {
            audioPlayerBsp_->Stop();
            audioPlayerBsp_->SetAVSyncListener(0);
            AudioPlayer::Destroy();
            audioPlayerBsp_ = 0;

            AudioPlayerHd::Create();
            audioPlayerHd_ = &AudioPlayer::InstanceHd();
            audioPlayerHd_->Start();

            videoPlayer_->SetStc(false, 0);  //clean?
        }

        if(switchToBspAudio)
        {
            audioPlayerHd_->Stop();
            AudioPlayer::Destroy();
            audioPlayerHd_ = 0;

            AudioPlayerBsp::Create();
            audioPlayerBsp_ = &AudioPlayer::InstanceBsp();
            audioPlayerBsp_->SetAVSyncListener(videoPlayer_);
            audioPlayerBsp_->Start();
        }
    }

    enum { aspectnone, aspect43, aspect169 };
    int ReelBoxDevice::GetAspectRatio()
    {
        int width=0, height=0;

        if (useHDExtension_)
        {
            width = HdCommChannel::hda->player_status[0].asp_x;
            height = HdCommChannel::hda->player_status[0].asp_y;
        }
        else
        {
            bspd_data_t volatile *bspd = Bsp::BspCommChannel::Instance().bspd;

            width = bspd->video_attributes[0].imgAspectRatioX;
            height = bspd->video_attributes[0].imgAspectRatioY;
        }

        if (width > 0 || height > 0)
            return (width == 16 && height == 9)?aspect169:aspect43;
        else
            return aspectnone;
    }

#if VDRVERSNUM >= 10716
    void ReelBoxDevice::GetVideoSize (int &Width, int &Height, double &Aspect) {
      Width=0;
      Height=0;
      if (useHDExtension_) {
        Width = HdCommChannel::hda->player_status[0].w;
        Height = HdCommChannel::hda->player_status[0].h;
      } else {
        printf("ERROR: Not implement yet: ReelBoxDevice::GetVideoSize for bspd");
      }
      Aspect = 1.0;
      switch(GetAspectRatio()) {
        case aspect43 : Aspect =  4.0/3.0; break;
        case aspect169: Aspect = 16.0/9.0; break;
        default       : break;
      } 
    }

    void ReelBoxDevice::GetOsdSize (int &Width, int &Height, double &Aspect) {
      Width=720;
      Height=576;
      Aspect=1.0; // DL TODO: Check aspect for 16:9/4:3
    }
#endif

    //void ReelBoxDevice::Stop()
    void ReelBoxDevice::Restart()
    {
        needRestart = false;
        DEBUG_DEVICE("\033[0;41m [reelbox]  !!! %s  !!!  \033[0m \n", __PRETTY_FUNCTION__);
        digitalAudio_ = false; pipActive_ = false; audioChannel_ = 0;
#if VDRVERSNUM < 10716
        audioPlayback_ = 0; videoPlayback_ = 0;
#endif

        int restartVideo = useHDExtension_ != RBSetup.usehdext;
        useHDExtension_ = RBSetup.usehdext;
        audioOverHDMI_ = RBSetup.audio_over_hdmi;

        //bkgPicPlayer_ = *this;
        //Clear();
        SetPlayModeOff();

        if (audioPlayerBsp_)
        {
           audioPlayerBsp_->SetAVSyncListener(0);
        }

        if (restartVideo)
        {
            VideoPlayerPip::Destroy();
            VideoPlayerBsp::Destroy();
            VideoPlayerHd::Destroy();
            videoPlayer_ = 0;
#if VDRVERSNUM < 10716
            videoPlayback_ = 0;
#endif
        }

        AudioPlayer::Destroy();

        audioPlayerHd_ = 0;
        audioPlayerBsp_ = 0;
#if VDRVERSNUM < 10716
        audioPlayback_ = 0;
#endif
        instance_ = 0; //? dangerous ?

        SystemExec("iecset audio on");
        DEBUG_DEVICE("[reelbox] \033[0;41m !! %s  %d !! \033[0m \n", __PRETTY_FUNCTION__,RBSetup.usehdext);
        // }
        //
        //void ReelBoxDevice::Start() {

        if (useHDExtension_)
        {
            DEBUG_DEVICE("[reelbox] \033[0;41m !!  USE HD-Extension  !! \033[0m \n");
            // HD
            //usleep(1000*1000);
            if (restartVideo)
            {
                Bsp::BspCommChannel::Destroy();
                DEBUG_DEVICE("[reelbox] \033[0;41m !!  :BspCommChannel::Destroy() ed  !! \033[0m \n");
                usleep(1000*1000);
                Reel::HdCommChannel::Init();
                DEBUG_DEVICE("[reelbox] \033[0;41m !!  :HdCommChannel::Init() ed  !! \033[0m \n");
                //Reel::HdCommChannel::InitHda();
	            usleep(1000*1000);
            }

            //audioPlayerBsp_ = &AudioPlayer::InstanceBsp();
            if (!audioOverHDMI_ && digitalAudio_ )
            {
                DEBUG_DEVICE("[reelbox] \033[0;48m !! NO  Audio Over HDMI_  !! \033[0m \n");
                AudioPlayerBsp::Create();
                audioPlayerBsp_ = &AudioPlayer::InstanceBsp();
            }
            else
            {
                DEBUG_DEVICE("[reelbox] \033[0;43m !!  AudioOverHDMI_  !! \033[0m \n");
                AudioPlayerHd::Create();
                audioPlayerHd_ = &AudioPlayer::InstanceHd();
            }

            if (restartVideo)
            {
                VideoPlayerHd::Create();
                VideoPlayerPipHd::Create();
            }
        }
        else
        {
            // BSP
            if (restartVideo)
            {
                DEBUG_DEVICE("[reelbox] \033[0;41m !!  USE BSP!! \033[0m \n");
                HdCommChannel::Exit();
                usleep(1000*1000);
                Bsp::BspCommChannel::Create();
                usleep(1000*1000);

                VideoPlayerBsp::Create();
                VideoPlayerPipBsp::Create();
            }

            AudioPlayerBsp::Create();
        }

        instance_ = this;

        if (restartVideo)
        {
            videoPlayer_ = &VideoPlayer::Instance();
        }

        if (audioPlayerBsp_)
        {
            DEBUG_DEVICE("[reelbox] \033[0;44m !!  audioPlayerBsp_->SetAVSyncListener(videoPlayer_)   !! \033[0m \n");
            audioPlayerBsp_->SetAVSyncListener(videoPlayer_);
        }

        if (restartVideo)
        {
            videoPlayerPip_ = &VideoPlayerPip::Instance();
            audioBackgroundPics_ = true;
        }
        SetPlayModeOn();

	/* restore default values*/
	if(restartVideo){
	  if(!useHDExtension_){
            RBSetup.brightness = fs453_defaultval_tab0_BSP;
            RBSetup.contrast   = fs453_defaultval_tab1_BSP;
            RBSetup.colour     = fs453_defaultval_tab2_BSP;
            RBSetup.sharpness  = fs453_defaultval_tab3_BSP;
            RBSetup.gamma      = fs453_defaultval_tab4_BSP;
            RBSetup.flicker    = fs453_defaultval_tab5_BSP;
            Reel::Bsp::BspCommChannel::SetPicture();
	  } else {
            RBSetup.brightness = fs453_defaultval_tab0_HD;
            RBSetup.contrast   = fs453_defaultval_tab1_HD;
            RBSetup.colour     = fs453_defaultval_tab2_HD;
            RBSetup.sharpness  = fs453_defaultval_tab3_HD;
            RBSetup.gamma      = fs453_defaultval_tab4_HD;
            RBSetup.flicker    = fs453_defaultval_tab5_HD;
            Reel::HdCommChannel::SetPicture(&RBSetup);
	  }
	}

        DEBUG_DEVICE("[reelbox] \033[0;41m !!  %s  SUCCESS   !! \033[0m \n",__PRETTY_FUNCTION__);
    }

    void ReelBoxDevice::AudioUnderflow()
    {
        DEBUG_DEVICE("[reelbox]  %s  \n", __PRETTY_FUNCTION__);
    }

    void ReelBoxDevice::Clear()
    {
        CHECK_CONCURRENCY;
        DEBUG_DEVICE("[reelbox]:  %s  \n", __PRETTY_FUNCTION__);
        try
        {
            cDevice::Clear();
            if (audioPlayerHd_) audioPlayerHd_->Clear();
            if (audioPlayerBsp_) audioPlayerBsp_->Clear();
            if (videoPlayer_) videoPlayer_->Clear();
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            // Restart();
        }
    }

    void ReelBoxDevice::Freeze()
    {
        CHECK_CONCURRENCY;
        DEBUG_DEVICE("[reelbox]:  %s  \n", __PRETTY_FUNCTION__);
        try
        {
            cDevice::Freeze();
            videoPlayer_->Freeze();
            if (audioPlayerHd_)  audioPlayerHd_->Freeze();
            if (audioPlayerBsp_) audioPlayerBsp_->Freeze();
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            // Restart();
        }
    }

    cSpuDecoder *ReelBoxDevice::GetSpuDecoder()
    {
        static bool useHDExtension =  useHDExtension_;
        useHDExtension = useHDExtension_; //if value of useHDExtension_ chanches

        if (!spuDecoder_.get())
        {
            try
            {
                class ReelSpuDecoder : public cDvbSpuDecoder
                {
                    /* override */ cSpuDecoder::eScaleMode getScaleMode()
                    {
                        return eSpuNormal;
                    }

                    /* override */ void Draw(void)
                    {
                        if (useHDExtension)
                        {
                            cDvbSpuDecoder::Draw();
                        }
                        else
                        {
                           BspOsdProvider::SetOsdScaleMode(true);
                           cDvbSpuDecoder::Draw();
                           BspOsdProvider::SetOsdScaleMode(false);
                        }
                    }
                };

                spuDecoder_.reset(new ReelSpuDecoder);
            }
            catch (std::exception const &e)
            {
                REEL_LOG_EXCEPTION(e);
                Restart();
            }
        }
        return spuDecoder_.get();
    }

    bool ReelBoxDevice::Flush(Int timeoutMs)
    {
        // This function will be called by the vdr concurrently to the other functions.
        DEBUG_DEVICE("[reelbox] \033[0;41n %s \033[0m \n", __PRETTY_FUNCTION__);
        bool ret = false;
        for (;;)
        {
            if (audioPlayerHd_)
            {
               ret = audioPlayerHd_ && audioPlayerHd_->Flush() && videoPlayer_->Flush();
            }
            if (audioPlayerBsp_)
            {
               ret = audioPlayerBsp_ && audioPlayerBsp_->Flush() && videoPlayer_->Flush();
            }

            if ((ret) || timeoutMs <= 0)
            {
                break;
            }
            ::usleep(5000); // sleep 5 ms
            timeoutMs -= 5;
        }
        return ret;
    }

    void ReelBoxDevice::FlushAudio()
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
        if (audioPlayerHd_)
        {
          while (audioPlayerHd_ && !audioPlayerHd_->Flush())
              ; // noop
        }
        if (audioPlayerBsp_)
        {
          while (audioPlayerBsp_ && !audioPlayerBsp_->Flush())
              ; // noop
        }
    }

    int ReelBoxDevice::GetAudioChannelDevice()
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
        return audioChannel_;
    }

    void ReelBoxDevice::Mute()
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
        cDevice::Mute();
    }

    void ReelBoxDevice::Play()
    {
        CHECK_CONCURRENCY;
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
        try
        {
            cDevice::Play();
            normalPlay = true;

            if (audioPlayerHd_)
            {
                audioPlayerHd_->Play();
            }
            if (audioPlayerBsp_)
            {
                audioPlayerBsp_->Play();
            }

            DEBUG_DEVICE("[reelbox] \033[0;44n %s --- videoPlayer_->Play(() \033[0m \n", __PRETTY_FUNCTION__);
            videoPlayer_->Play();
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            Restart();
        }
    }

#if 0
#include <sys/time.h>
#include <time.h>

        ULLong GetTimeUsec()
        {
            struct timeval tv;
            gettimeofday(&tv, 0);
            return ULLong(tv.tv_sec) * 1000000 + tv.tv_usec;
        }
#endif

#if VDRVERSNUM < 10400
    Int ReelBoxDevice::PlayAudio(Byte const *data, Int length)
#else
    Int ReelBoxDevice::PlayAudio(Byte const *data, Int length, uchar id)
#endif
    {;
        CHECK_CONCURRENCY;

        const tTrackId *trackId = GetTrack(GetCurrentAudioTrack());
        
#if VDRVERSNUM < 10704
        if (!trackId || trackId->id != id)
        {
            return length;
        }
#endif

#if VDRVERSNUM < 10716
        audioPlayback_ = 200;

        if (-- videoPlayback_ < 0)
        {
            videoPlayback_ = 0;
#if 1
            bkgPicPlayer_.Start();
#else
            if (audioBackgroundPics_)
                bkgPicPlayer_.Start();
            else
                bkgPicPlayer_.Stop();
#endif
        }
#else
        bkgPicPlayer_.PlayedAudio();
#endif

	if (useHDExtension_) {
		videoPlayer_->PlayPesPacket((void*)data, length, 0);
                if(!audioPlayerBsp_) //send packets to alsa too
                {
		    return length;
                }
	}


        try
        {
            // LogData(data, length, 0);

            UInt pesPacketLength = length;

            while (pesPacketLength > 0)
            {
                Mpeg::EsPacket esPacket(data, pesPacketLength);
                if (audioPlayerHd_)
                {
                   // audioPlayerHd_->PlayPacket(esPacket);
                }
                if (audioPlayerBsp_)
                {
                    audioPlayerBsp_->PlayPacket(esPacket);
                }
            }
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            // Restart();
        }
        return length;
    }

    void ReelBoxDevice::PlayAudioRaw(AudioFrame const *frames, Int numFrames,
                                    SampleRate sampleRate, UInt pts)
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
#if VDRVERSNUM < 10716
        audioPlayback_ = 200;

        if (-- videoPlayback_ < 0)
        {
            videoPlayback_ = 0;
#if 1
            bkgPicPlayer_.Start();
#else
            if (audioBackgroundPics_)
                bkgPicPlayer_.Start();
            else
                bkgPicPlayer_.Stop();
#endif
        }
#else
        bkgPicPlayer_.PlayedAudio();
#endif

        if (audioPlayerHd_)
        {
            audioPlayerHd_->PlayFrames(frames, numFrames, sampleRate, pts, digitalAudio_);
        }
        if (audioPlayerBsp_)
        {
            audioPlayerBsp_->PlayFrames(frames, numFrames, sampleRate, pts, digitalAudio_);
        }
    }

   void ReelBoxDevice::SetAudioTrack(int index){
       if (index >=0)
          audioIndex = index;
       VideoPlayerHd *player = dynamic_cast<VideoPlayerHd*>(&VideoPlayer::Instance());
       if(player && oldAudioIndex != audioIndex)
		player->IncGen();
       oldAudioIndex = audioIndex;
       printf("SetAudioTrack: %i\n", index);
  }

    Int ReelBoxDevice::PlayVideoTs(Byte const *data, Int length, bool VideoOnly, uchar* PATPMT)
    {
       CHECK_CONCURRENCY;
        if(needRestart) Restart();
        if(audioChannel_ != oldAudioChannel){
	   oldAudioChannel = audioChannel_;
        }
        if(PATPMT){
          int vpidpatpmt = PATPMT[TS_SIZE + 19]&0xff | ((PATPMT[TS_SIZE+18]&0x1f)  << 8);
          int apidpatpmt[MAXAPIDS] = { 0 };
          int dpidpatpmt[MAXDPIDS] = { 0 };
          int dpidsfound = 0, apidsfound = 0;
	  int offset = 17+5; /* after header and vpid */
	  /* as long as there are audio tracks */
	  while(PATPMT[TS_SIZE + offset] == 0x06 || PATPMT[TS_SIZE + offset] == 0x04){
             /* test for dolby */
             if (PATPMT[TS_SIZE + offset]==0x06 && dpidsfound < MAXDPIDS){
		/* set first mpa also, will be overriden if a real mpa-track is found */
                dpidpatpmt[dpidsfound] = apidpatpmt[apidsfound] = PATPMT[TS_SIZE + offset + 2]&0xff | ((PATPMT[TS_SIZE + offset + 1]&0x1f ) << 8);
                SetAvailableTrack(ttDolby, dpidsfound, dpidpatpmt[dpidsfound], 0);
		dpidsfound++;
	        offset += 8;
             }
             /* test for mpa */
	     if (PATPMT[TS_SIZE + offset]==0x04 && apidsfound < MAXAPIDS){
		apidpatpmt[apidsfound] = PATPMT[TS_SIZE + offset + 2]&0xff | ((PATPMT[TS_SIZE + offset + 1]&0x1f ) << 8);
		SetAvailableTrack(ttAudio, apidsfound, apidpatpmt[apidsfound], 0);
		offset += 5;
		apidsfound++;
             }
          }

          printf("PATPMT: vpid: %x apid: %x dpid : %x dpid2: %x\n",vpidpatpmt, apidpatpmt[0], dpidpatpmt[0], dpidpatpmt[1]);

          if(vpidpatpmt != 0 && (apidpatpmt[0] !=0 || dpidpatpmt[0] != 0) ){
             printf("VALID PATPMT: vpid: %x apid: %x dpid : %x dpid2: %x\n",vpidpatpmt, apidpatpmt[0], dpidpatpmt[0], dpidpatpmt[1]);
	     if(!dpidpatpmt[0] && audioIndex > 0) //TB: if there's only one track select it regardless what VDR wants
		audioIndex = 0;
          }
        }

           try
           {
#if VDRVERSNUM < 10716
              bkgPicPlayer_.Stop();
              videoPlayback_ = 3500;
              if (-- audioPlayback_ < 0)
              {
                 audioPlayback_ = 0;
              }
#else
              bkgPicPlayer_.PlayedVideo();
#endif

              videoPlayer_->PlayTsPacket((void*)data, length, PATPMT);

	       } catch (std::exception const &e)
		   {
	            REEL_LOG_EXCEPTION(e);
		   }
	       return length;
    }


    Int ReelBoxDevice::PlayVideo(Byte const *data, Int length)
    {
        CHECK_CONCURRENCY;
        if(needRestart) Restart();

#if VDRVERSNUM >= 10716
        bkgPicPlayer_.PlayedVideo();
#endif

	if (useHDExtension_) {
#if VDRVERSNUM < 10716
		bkgPicPlayer_.Stop();
		videoPlayback_ = 3500;
#else
		if((length > 7) && !data[0] && !data[1] && (data[2]==1) && (data[7]&0x20)) {
			VideoPlayerHd *player = dynamic_cast<VideoPlayerHd*>(&VideoPlayer::Instance());
			isyslog("ReelBoxDevice::PlayVideo: Broken Link %p", player);
			if(player) player->IncGen();
		} // if
#endif
		videoPlayer_->PlayPesPacket((void*)data, length, 1);
		return length;
	}

        try
        {
            // LogData(data, length, 0);

#if VDRVERSNUM < 10716
            bkgPicPlayer_.Stop();
            videoPlayback_ = 3500;

            if (-- audioPlayback_ < 0)
            {
                audioPlayback_ = 0;
            }
#endif
            UInt pesPacketLength = length;

            // ::printf("PV\n");
            while (pesPacketLength > 0)
            {
                // ::printf(" *\n");
                Mpeg::EsPacket esPacket(data, pesPacketLength);
                if (videoPlayer_ && esPacket.GetMediaType() == Mpeg::MediaTypeVideo)
                {
                    videoPlayer_->PlayPacket(esPacket);
                }
            }
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);

            // PrintPes(data, length);
            // length = -1;
        }
        return length;
    }

    void ReelBoxDevice::PlayVideoEs(Byte const *data, Int length, UInt pts)
    {
        if(needRestart) Restart();
#if VDRVERSNUM < 10716
        bkgPicPlayer_.Stop();
        videoPlayback_ = 3500;

        if (-- audioPlayback_ < 0)
        {
            audioPlayback_ = 0;
        }
#else
        bkgPicPlayer_.PlayedVideo();
#endif

        Mpeg::EsPacket esPacket(data, length,
                                Mpeg::StreamIdVideoStream0,
                                Mpeg::SubStreamIdNone,
                                Mpeg::MediaTypeVideo,
                                pts);

        videoPlayer_->PlayPacket(esPacket);
    }

    void ReelBoxDevice::PlayPipVideo(Byte const *data, Int length)
    {
        CHECK_CONCURRENCY;

        try
        {
            UInt pesPacketLength = length;

            while (pesPacketLength > 0)
            {
                Mpeg::EsPacket esPacket(data, pesPacketLength);
                if (esPacket.GetMediaType() == Mpeg::MediaTypeVideo)
                {
                    videoPlayerPip_->PlayPacket(esPacket);
                }
            }
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
        }
    }

	int ReelBoxDevice::AproxFramesInQueue(void) {
		if(!videoPlayer_) return 0;
		return videoPlayer_->AproxFramesInQueue();
	} // ReelBoxDevice::AproxFramesInQueue

        bool ReelBoxDevice::ShowAudioBackgroundPics() { 
            if(!audioBackgroundPics_) return false;
            if((playMode_ == pmNone          ) ||
               (playMode_ == pmAudioOnlyBlack) ||
               (playMode_ == pmExtern_THIS_SHOULD_BE_AVOIDED)) return false;
            return true;
        };

	bool ReelBoxDevice::Poll(cPoller &Poller, Int timeoutMs)
    {
        // This function will be called by the vdr concurrently to the other functions.
        if (pipActive_)
        {
            return true;
        }

#if VDRVERSNUM < 10716
        if (!audioPlayback_ && !videoPlayback_)
        {
            return true;
        }
#else
        bool audioPlayback_ = true;
        bool videoPlayback_ = IsPlayingVideo();
#endif

        // We ingore the Poller, because we're not sure about threading issues.
        //poll only the player which sets STC ???
        bool ret = false;
        for (;;)
        {
	    if (audioPlayerHd_)
            {
                ret = audioPlayback_ &&  audioPlayerHd_ &&  audioPlayerHd_->Poll() || videoPlayback_ && videoPlayer_ && videoPlayer_->Poll();
            }
            if (audioPlayerBsp_)
            {
                ret = audioPlayback_ &&  audioPlayerBsp_ &&  audioPlayerBsp_->Poll() || videoPlayback_ && videoPlayer_ && videoPlayer_->Poll();
            }

            if (ret || timeoutMs <= 0)
            {
                break;
            }
            ::usleep(5000); // sleep 5 ms
            timeoutMs -= 5;
        }
        return ret;
    }

    bool ReelBoxDevice::PollAudio(int timeoutMs)
	{
	   bool ret = false;
#if VDRVERSNUM >= 10716
           bool audioPlayback_ = true;
#endif

	   for (;;)
	   {
            if (audioPlayerHd_)
            {
                ret = audioPlayback_ && audioPlayerHd_->PollAudio();
            }
            if (audioPlayerBsp_)
            {
                ret = audioPlayback_ && audioPlayerBsp_->PollAudio();
            }

	        if (ret || timeoutMs <= 0)
	        {
	            break;
	        }
	        ::usleep(5000); // sleep 5 ms
	        timeoutMs -= 5;
	    }
	    return ret;
	}

    bool ReelBoxDevice::PollVideo(int timeoutMs)
    {
#if VDRVERSNUM >= 10716
           bool videoPlayback_ = IsPlayingVideo();
#endif
        bool ret = false;
        for (;;)
        {
            ret = videoPlayback_ && videoPlayer_ && videoPlayer_->PollVideo();
            if (ret || timeoutMs <= 0)
            {
                break;
            }
            ::usleep(5000); // sleep 5 ms
            timeoutMs -= 5;
        }
        return ret;
    }
    //End by Klaus

    void ReelBoxDevice::SetAudioChannelDevice(int audioChannel)
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
        audioChannel_ = audioChannel;
        AudioChannel channel = IsMute() ? AudioChannelMute : AudioChannel(audioChannel_);
        if (audioPlayerHd_)
        {
           audioPlayerHd_->SetChannel(channel);
        }
        if (audioPlayerBsp_)
        {
           audioPlayerBsp_->SetChannel(channel);
        }
        DEBUG_DEVICE("[reelbox] \033[0;44n %s \033[0m \n", __PRETTY_FUNCTION__);
    }

    void ReelBoxDevice::SetDigitalAudioDevice(bool on)
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s  ON? %s \033[0m \n", __PRETTY_FUNCTION__, on?"YES":"NO");
        if (digitalAudio_ != on)
        {
            if (digitalAudio_)
            {
                ::usleep(1000000); // Wait until any leftover digital data has been flushed
            }
            digitalAudio_ = on;
            SystemExec(on ? "iecset audio off" : "iecset audio on");
            SetVolumeDevice(IsMute() ? 0 : CurrentVolume());
        }
        RestartAudio();
    }

    int64_t ReelBoxDevice::GetSTC()
    {
        int64_t stc;
	stc=HdCommChannel::hda->player[0].hde_last_pts; // needs hdplayer >SVN r12241
//        int64_t stc_base = ((((int64_t)HdCommChannel::hda->player[0].hde_stc_base_high)&0xFFFFFFFF)<<32)|(HdCommChannel::hda->player[0].hde_stc_base_low&0xFFFFFFFF);
//	printf("GET STC %llx\n",stc);
/*	if (audioPlayerHd_) {
		printf("GET STC %x\n",HdCommChannel::hda->player[0].stc+stc_);
		return HdCommChannel::hda->player[0].stc+stc_;
	}
*/
//        return (stc == 0) ? -1LL : stc;
        if(!stc) return -1LL; // just check if we have a valid stream
        if(!normalPlay) return stc; // Use pts if not playing normal speed
        // needs hdplayer >= SVN r17472
        int64_t stc_base   = ((((int64_t)HdCommChannel::hda->player[0].hde_stc_base_high)&0xFFFFFFFF)<<32)|(HdCommChannel::hda->player[0].hde_stc_base_low&0xFFFFFFFF);
        return stc_base;
    }

    void ReelBoxDevice::SetStc(bool stcValid, UInt stc)
    {
        stc  = (stc == 0) ? 1 : stc;
        stc_ = stcValid ? stc : 0;
    }

    void ReelBoxDevice::SetAudioBackgroundPics(bool active)
    {
	    audioBackgroundPics_ = active;
    }

    bool ReelBoxDevice::SetPlayMode(ePlayMode playMode)
    {
        CHECK_CONCURRENCY;
        DEBUG_DEVICE("[reelbox] \033[0;44n %s  Playmode? %d \033[0m \n", __PRETTY_FUNCTION__, playMode);
        bool ret = true;
        try
        {
            playMode_ = playMode;
            if (playMode == pmAudioVideo || playMode == pmAudioOnly || playMode == pmAudioOnlyBlack || playMode == pmVideoOnly)
            {
                SetPlayModeOn();
            }
            else
            {
                SetPlayModeOff();
            }
#if VDRVERSNUM >= 10716
            bkgPicPlayer_.ResetTimer();
#endif
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            ret = false;
        }
        return ret;
    }

    void ReelBoxDevice::SetVideoFormat(bool videoFormat16_9)
    {
        if (useHDExtension_)
        {
            // HD
            if (tmpHDaspect == -1)
                tmpHDaspect = RBSetup.HDaspect;
            else
            {
                tmpHDaspect = (tmpHDaspect + 1) % 3;
                Reel::HdCommChannel::SetAspect(tmpHDaspect);
                switch(tmpHDaspect) {
                    case 0:
                        Skins.Message(mtInfo, tr("Fill to Screen"));
                        break;
                    case 1:
                        Skins.Message(mtInfo, tr("Fill to Aspect"));
                        break;
                    case 2:
                        Skins.Message(mtInfo, tr("Crop to Fill"));
                        break;
                }
            }
        }
        else
        {
            // BSP

            try
            {
                int pip=0;
                Bsp::BspCommChannel &bspCommChannel = Bsp::BspCommChannel::Instance();

                if (videoFormat16_9)
                {
                    bspCommChannel.bspd->video_player[pip].aspect[0]= 16;
                    bspCommChannel.bspd->video_player[pip].aspect[1]= 9;
                }
                else
                {
                    bspCommChannel.bspd->video_player[pip].aspect[0]= 4;
                    bspCommChannel.bspd->video_player[pip].aspect[1]= 3;
                }
                bspCommChannel.bspd->video_player[pip].changed++;
            }
            catch (std::exception const &e)
            {
                REEL_LOG_EXCEPTION(e);
                Restart();
            }
        }
    }

    void ReelBoxDevice::SetVolumeDevice(int volume)
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s  \033[0m \n", __PRETTY_FUNCTION__);
        AudioChannel channel = volume ? AudioChannel(audioChannel_) : AudioChannelMute;
        if(audioPlayerHd_)
        {
            audioPlayerHd_->SetChannel(channel);
            HdCommChannel::SetVolume(volume);
        }
        if(audioPlayerBsp_)
        {
            audioPlayerBsp_->SetChannel(channel);
        }

#ifndef RBLITE
        if (audioOverHd_)
            return;
#endif
        // ALSA

        volume = volume / 8;  // range is from 0-31 on RB_Lite, 0-255 on RB_II;
                              // FIXME: SetVolume() should check volume_range before setting
                              //        and take volume as % as input

        if (digitalAudio_ )  // we have ac3
        {
            if (RBSetup.ac3 || !useHDExtension_)
            {
                // ac3 over spdif
                // mute all analog  outputs !
                MUTE("PCM");
                MUTE("CD");
                SET_VOLUME(0);
            }
            else
            {
                // HD-Ext decode AC3 -> analog -> CD -> Realtek
                MUTE("PCM");
                UNMUTE("CD");
                SET_VOLUME(volume);
            }
        }
        else   // mp2
        {

            if (useHDExtension_) // bypass
            {
                MUTE("PCM");
                UNMUTE("CD");
                SET_VOLUME(volume);
            }
            else
            {
                MUTE("CD");
                UNMUTE("PCM");
                SET_VOLUME(volume);
            }

            if (volume == 0)
            {
                // be silent
                SET_VOLUME(0);
            }
        }

    }

    void ReelBoxDevice::SetVolume(int Volume, const char *device)
    {
        DEBUG_DEVICE("[reelbox] \033[0;44n %s  \033[0m \n", __PRETTY_FUNCTION__);
        int err;
        snd_mixer_t *handle;
        const char *card= "default";
        snd_mixer_elem_t *elem;
        snd_mixer_selem_id_t *sid;
        snd_mixer_selem_id_alloca(&sid);
        unsigned int channels = ~0UL;
        snd_mixer_selem_channel_id_t chn;

        snd_mixer_selem_id_set_name(sid, device);
        if ((err = snd_mixer_open(&handle, 0)) < 0) {
            printf("Mixer %s open error: %s\n", card, snd_strerror(err));
            return;
        }
        if ((err = snd_mixer_attach(handle, card)) < 0) {
            printf("Mixer attach %s error: %s", card, snd_strerror(err));
            snd_mixer_close(handle);
            return;
        }
        if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
            printf("Mixer register error: %s", snd_strerror(err));
            snd_mixer_close(handle);
            return;
        }
        err = snd_mixer_load(handle);
        if (err < 0) {
            printf("Mixer %s load error: %s", card, snd_strerror(err));
            snd_mixer_close(handle);
            return;
        }
        elem = snd_mixer_find_selem(handle, sid);
        if (!elem) {
            printf("Unable to find simple control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
            snd_mixer_close(handle);
            return;
        }

        for (chn = static_cast<snd_mixer_selem_channel_id_t> (0); chn <= SND_MIXER_SCHN_LAST; chn = static_cast<snd_mixer_selem_channel_id_t>(chn + 1))
        {
            if (!(channels & (1 << chn)))
                continue;
            if (snd_mixer_selem_has_playback_channel(elem, chn)) {
                    if (snd_mixer_selem_has_playback_volume(elem)) {
                        snd_mixer_selem_set_playback_volume(elem, chn, Volume);
                    }
            }
        }
        snd_mixer_close(handle);
    }

    void ReelBoxDevice::StartPip(bool on)
    {
        pipActive_ = on;
        if (on)
        {
            videoPlayerPip_->Start();
        }
        else
        {
            videoPlayerPip_->Stop();
        }
    }

    void ReelBoxDevice::StillPicture(Byte const *data, Int length)
    {
        CHECK_CONCURRENCY;
        needRestart=true;
        try
        {
            std::vector<Mpeg::EsPacket> videoPackets;

#if VDRVERSNUM >= 10716
            if (length && data[0] == 0x47) {
                return cDevice::StillPicture(data, length);
            } else 
#endif
            if (length >= 4 &&
                data[0] == 0x00 &&
                data[1] == 0x00 &&
                data[2] == 0x01 &&
                data[3] <= 0xB8)
            {
                // ES

                int const maxLen = 2000;
                while (length > 0)
                {
                    int const l = length > maxLen ? maxLen : length;
                    Mpeg::EsPacket const esPacket(data, l,
                                                  Mpeg::StreamIdVideoStream0,
                                                  Mpeg::SubStreamIdNone,
                                                  Mpeg::MediaTypeVideo);
                    videoPackets.push_back(esPacket);
                    data += l;
                    length -= l;
                }
            }
            else
            {
                // PES

                UInt pesLength = length;

                // Parse packets.
                while (pesLength)
                {
                    Mpeg::EsPacket const esPacket(data, pesLength);
                    if (esPacket.GetMediaType() == Mpeg::MediaTypeVideo)
                    {
                        videoPackets.push_back(esPacket);
                    }
                }
            }

            // Display them.
            if (videoPackets.size())
            {
                videoPlayer_->StillPicture(&videoPackets[0], videoPackets.size());
            }
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            // Restart();
        }
    }

    void ReelBoxDevice::TrickSpeed(Int speed)
    {
        try
        {
            // printf("Trick speed %i\n",speed);
            normalPlay = false;
/*            if (audioPlayerHd_)
            {
                audioPlayerHd_->Freeze();
		}*/
            if (audioPlayerBsp_)
            {
                audioPlayerBsp_->Freeze();
            }

            videoPlayer_->Trickmode(speed);
        }
        catch (std::exception const &e)
        {
            REEL_LOG_EXCEPTION(e);
            Restart();
        }
    }

    void ReelBoxDevice::SetPipDimensions(uint x, uint y, uint width, uint height)
    {
        videoPlayerPip_->SetDimensions(x, y, width, height);
    }

    void ReelBoxDevice::SetPlayModeOff()
    {
        printf ("[reelbox] \033[0;44m  SetPlayModeOff() \033[0m\n");

#if VDRVERSNUM < 10716
        audioPlayback_ = 0;
        videoPlayback_ = 0;

        bkgPicPlayer_.Stop();
#endif
        normalPlay = false;

        if (audioPlayerHd_)
        {
            audioPlayerHd_->Stop();
        }
        if (audioPlayerBsp_)
        {
            audioPlayerBsp_->Stop();
        }

        videoPlayer_->Stop();
    }

    void ReelBoxDevice::SetPlayModeOn()
    {
        printf ("[reelbox] \033[0;44m  SetPlayModeOn() \033[0m\n");

#if VDRVERSNUM < 10716
        audioPlayback_ = 100;
        videoPlayback_ = 10;
#endif
        normalPlay = true;

        if (audioPlayerHd_)
        {
            //printf ("[reelbox] \033[0;44m  audioPlayerHd_->Start() \033[0m\n");
            audioPlayerHd_->Start();
        }
        if (audioPlayerBsp_)
        {
            //printf ("[reelbox] \033[0;41m  !!! NO audioPlayerHd_ !! \033[0m\n");
            audioPlayerBsp_->Start();
        }

        if (tmpHDaspect != RBSetup.HDaspect && useHDExtension_)
        {
            tmpHDaspect = RBSetup.HDaspect;
            Reel::HdCommChannel::SetVideomode();
        }
        videoPlayer_->Start();
    }

    void ReelBoxDevice::Create()
    {
        if (!instance_)
        {
            instance_ = new ReelBoxDevice;
        }
    }

    void ReelBoxDevice::MakePrimaryDevice(bool On)
    {
       if (On)
       {
          HdOsdProvider::Create();
       }
    }
}
