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

// Player.h
 
#ifndef XINE_MEDIAPLAYER_PLAYER_H_INCLUDED
#define XINE_MEDIAPLAYER_PLAYER_H_INCLUDED

#include "Reel.h"
#ifndef USE_PLAYPES
#include "XineLib.h"
#endif

#include "SpuDecode.h"

#include <vdr/player.h>
#include <vdr/osdbase.h>
#include <vdr/thread.h>

#include <string>
#include <vector>
#include <queue>

#include "item_tree.h"

#ifdef USE_PLAYPES

#include <vdr/tools.h>
#include <vdr/ringbuffer.h>
#include "InputThread.h"
#include "PesMpaTools.h"

#define BUF_SIZE    KILOBYTE(100)
#define BUF_MARGIN  KILOBYTE(2)

#endif


#define MAX_NW_ERRORS 3

namespace Reel
{
namespace XineMediaplayer
{
#ifdef USE_PLAYPES
extern int useOld ;
enum ePlayModes { pmPlay, pmPause, pmSlow, pmFast, pmStill };

#endif
    enum ePlayerStatus  {eNotPlaying, eOpened, ePlaying, eGettingNext};
    enum eError         {eNone,     eOpenError, eNetworkOpenError };

    class cPlayerStatus
    {
        private:
            eError          PlayerError_;
            int             ErrorCount_;
            time_t          lastError_;
            
            ePlayerStatus   Status_;
            time_t          lastStatusChange_;

        protected:
            void            SetError(eError err);
            void            SetPlayerStatus(ePlayerStatus status);

            friend class    Player;
        public:
            ePlayerStatus   Status();
            time_t          StatusTime(); // returns last status change time

            std::string     ErrorString()   const;
            time_t          ErrorTime()     const;
            int             ErrorCount()    const;

    };
    extern cPlayerStatus PlayerStatus;

    enum {PLAY_NORMAL=1, PLAY_RANDOM, PLAY_REPEAT_SINGLE, PLAY_REPEAT_PLAYLIST};
    // PLAY_REPEAT_SINGLE : plays the same file 1/0 times.

    class Player : public ::cPlayer, ::cThread /* final */
    {
    public:
        bool wasInMenu_;
        static int NetworkErrorCount;
        int player_status;
        Player(const char *mrl, bool playlist, std::vector<std::string> playlistEntries) NO_THROW;
        ~Player() NO_THROW;

        // cPlayer
        /* override */ void Activate(bool on) NO_THROW;
                       void Action(); // thread body

        /* override */ virtual eOSState ProcessKey(eKeys key) NO_THROW;
                               eOSState ProcessKey_r(eKeys key) THROWS;
        /* override */ bool CanHandleMenuCalls() NO_THROW;
        /* override */ bool PlayerIsInMenuDomain() NO_THROW; 

        // start by Balaji
        /* override */ bool CanHandleSubtitles() NO_THROW;
        /* override */ int  NumSubtitleTracks() NO_THROW;
        /* override */ bool SetCurrentSubtitleTrack(int a);
        /* override */ int  GetCurrentSubtitleTrack();
        /* override */ const char* SubtitleTrackName(int a);
        // end by Balaji

        eOSState Number(eKeys Key);

        /* override */ cOsdObject *GetInfo(void);
        /* override */ bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
        /* override */ void SetAudioTrack(eTrackType Type, const tTrackId *TrackId);
        /* override */ bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
        //
        ///< sets given audio tracks  menu:c cDisplayTracks called by kAudio
        ///< register each track with vdr/device.h/Player::DeviceSetAvailableTrack 

        void ShowSplashScreen();

        void HandleEvents() THROWS;   
        void HandleVdrKeys() THROWS;

        void ProcessSpu();
        bool SpuHasOsd();
        void HideSPU();
        void SetMenuDomain(bool isInMenuDomain);
        bool Playing();
        bool IsReplayingMusic();
        bool IsReplayingDvd();
        bool IsReplayingVideo();

    int CurrentPlayNr(); // current position on playlist
    void ReplacePlayList(std::vector<std::string> newPl); // New playlist on the fly
    void StopPlayback() NO_THROW;

        // random access to files in playlist
        bool PlayFileNumber(unsigned int);
        bool PlayFileNumber_r(unsigned int); // called from thread;
        void SetPlayMode(int);
        int GetPlayMode();
        void SetPlayback(int pos);

       std::string GetTitleInfoString(); 
       ///< returns current Title Chapter and Angle if aviable 
       ///< 
       void SetTitleInfoString();
       ///< writes current Title Chapter and Angle if aviable  
       ///< updates are triggerd by xine events
       
        
       const char *GetTitleString() const;




       const char *GetAspectString() const;
       ///< can`t get valid Aspect infos ???

       void SetAllAudioTracks();
       void SetAllSpuTracks();
       ///< missing implementation of spu tracks inside vdr 

       void SetInitLangCode();
       std::string GetAudioLangCode() const;
       std::string GetSpuLangCode() const;

    private:
        Player(Player const &); // No copying.
        Player &operator=(Player const &); // No assigning.

        std::vector < cItemTree > vItemTree;
        cMutex mutexVItemTree;

        // holds all unprocessed keys got from vdr
        std::queue  <eKeys>       KeyQueue;
        //holds return values that were got from processing KeyQueue elements
        std::queue  <eOSState>    returnQueue;
        // mutexes for the above queues
        cMutex mutexKeyQ;
        cMutex mutexReturnQ;

        // sets elements of vItemTree with playlistEntries items as "head"
        void CreateItemTree(); 

        // Get next mrl from tree nr. tree_num: "" is returned if tree has no more playable entries, go to next tree
        std::string GetNextFromTree(unsigned int tree_num)
        { 
            cMutexLock l(&mutexVItemTree);
            return tree_num < vItemTree.size() ? vItemTree[tree_num].GetNext(true) : "";
        }  
        std::string GetPrevFromTree(unsigned int tree_num)
        { 
            cMutexLock l(&mutexVItemTree);
            return tree_num < vItemTree.size() ? vItemTree[tree_num].GetNext(false): "";
        }    

        std::string GetNextPlayListEntryCyclic(bool up) NO_THROW; 
        void PlayNextPlayListEntry(bool up) THROWS;
        void StartPlayback() THROWS;
        void ReStartPlayback() THROWS;   

        // sets  pl_pos according to position of  mrl_ in playlistEntries
        // returns false if the current mrl is in the playlist
        // (the player does not need to be restarted)
        bool InitPlayListPos();
    int pl_pos;         // position of currently playing file in playlistEntries
#ifdef USE_PLAYPES
//DL    float duration;
    uint64_t duration;
    uint64_t duration_pad;
    bool firstPacketSent;
    cPoller Poll;
    ePlayModes playMode;
#else
        XineLib &xineLib_;
#endif

        // Indicates how Player selectes the next file to be played
        int play_mode;
        int playfile_pl_pos;

        bool playing_;
        bool  mrlOpenError_;

        bool mrlIsInPlaylist_; //mrl is part of a playlist handled externally
        bool playlist_;  //playlist is handled by this plugin
        std::vector<std::string> playlistEntries_; //the playlist
        bool loopmode_;

        bool isInMenuDomain_;

        SpuDecoder spu_;

        std::string mrl_;

        std::string titleInfo_;

        int SearchAudioStream(int AudioStreamId);

        struct tTrackId_type {
            tTrackId tTID  ;
            eTrackType eTType; 
        };
        //std::vector<tTrackId_type > aviableAudioTracks_;
        std::vector<tTrackId> aviableAudioTracks_;
        std::vector<tTrackId> aviableSpuTracks_;
        void SetAvailableTracks();
        uint32_t GetAuidoPids(std::string& langCode, eTrackType TrackType);


        int currentAudioTrack;
        // XXX verify  currentLangCode 
        //char currentLangCode_[32];
        char selectedLangCode_[32];

        int currentSpuTrack;

        cTimeMs numberTimer;
        cTimeMs writeWait;
        int number;

#ifdef USE_PLAYPES
        cRingBufferLinear *ringbuffer;
        cMutex            Mutex;
        cInputThread ReadThread;

        /**
         * returns number of bytes sent as pes packet from ringbuffer
         * interesting return values are 0, -1
         * -1 indicates no data was got from buffer
         *  0 indicates Device was not ready
         */
        int  PlayOnePesPacket();

        /* returns the number of bytes from ringbuffer to be deleted
         * possibly these bytes were played or these number of bytes 
         * did not have any valid mpa frame
         */
//DL        int PlayAsPesPackets(uchar* Data, int len, uint64_t pts, float& duration);
        int PlayAsPesPackets(uchar* Data, int len, uint64_t pts, uint64_t& duration);
#endif

        
    public:
        static const int MaxAudioTracks;
        static const int AudioTrackMask;
        static const int AC3AudioTrackMask;
        static const int MaxSubpStreams;
        static const int SubpStreamMask;
        /// xinelib Infos 
        bool GetInfos();
        std::string GetMrl() { return mrl_; }

    bool IsPaused();

        uint32_t GetBitrate();
        
        uint32_t Bitrate();

        uint32_t Seekable();

        uint32_t VideoWidth();

        uint32_t VideoHeight();

        uint32_t VideoRatio();

        uint32_t VideoChannels();

        uint32_t VideoStreams();

        uint32_t VideoBitrate();

        uint32_t VideoFourcc();

        uint32_t VideoHandled();

        uint32_t FrameDuration();

        uint32_t AudioChannels();

        uint32_t AudioBits();

        uint32_t AudioSamplerate();

        uint32_t AudioBitrate();

        uint32_t AudioFourcc();

        uint32_t AudioHandled();

        uint32_t HasChapters();

        uint32_t HasVideo();

        uint32_t HasAudio();

        uint32_t IgnoreVideo();

        uint32_t IgnoreAudio();

        uint32_t IgnoreSpu();

        uint32_t VideoHasStill();

        uint32_t MaxAudioChannel();

        uint32_t MaxSpuChannel();

        uint32_t AudioMode();

        uint32_t SkippedFrames();

        uint32_t DiscardedFrames();

        uint32_t VideoAfd();

        uint32_t DvdTitleNumber();

        uint32_t DvdTitleCount();

        uint32_t DvdChapterNumber();

        uint32_t DvdChapterCount();

        uint32_t DvdAngleNumber();

        uint32_t DvdAngleCount();


        // xine meta infos 
       
        const char *GetTitle() const;

        const char *Comment() const;

        const char *Artist() const;

        const char *Genre() const;

        const char *Album() const;

        const char *Year() const;

        const char *VideoCodec() const;

        std::string AudioCodec();

        const char *SystemLayer() const;

        const char *InputPlugin() const;

        const char *CDIndexDiscId() const;

        const char *TrackNumber() const;

    };

    // Inline Player 
    ///////////////////////////////////////////////////////////////////////////

    inline bool Player::IsPaused()
    {
#ifdef USE_PLAYPES
        return playMode == pmPause;
#else   
        return xineLib_.GetParamSpeed() == XINE_SPEED_PAUSE;
#endif
    }
    inline bool Player::CanHandleMenuCalls() 
    {
        //printf("-------Player::CanHandleMenuCalls------ return true ----\n");
        return true;
    }  

    inline bool Player::Playing() // PlayStream
    {
        return playing_;
    }

    //is inside a DVD Menu or not replaying a dvd
    inline bool Player::PlayerIsInMenuDomain() 
    { 
        return isInMenuDomain_ || !IsReplayingDvd();
    } 
    // Start by Balaji
    inline bool Player::CanHandleSubtitles()
    {
        return playing_ ;  // if playing, it can handle subtitles
    }
    inline int Player::NumSubtitleTracks()
    {
        if ( GetSpuLangCode().find("menu")!=std::string::npos) return 0;
        else
        return MaxSpuChannel() ;
    }
    inline bool Player::SetCurrentSubtitleTrack(int idx)
    {
#ifdef USE_PLAYPES
        return true;
#else   
        xineLib_.SetParamSPUChannel(idx-1);
        return true;
#endif
    }
    inline int Player::GetCurrentSubtitleTrack()
    {
#ifdef USE_PLAYPES
        return 1;
#else   
        return xineLib_.GetParamSPUChannel();
#endif
    }
    inline const char* Player::SubtitleTrackName(int idx)
    {
#ifdef USE_PLAYPES
        return "";
#else   
        char *langStr = (char*) malloc(sizeof(char)*80);
        if ( xineLib_.GetSpuLangCode(langStr, idx) )
        {
            return langStr;
        }
        else 
        {
            free(langStr);
            return "";
        }
#endif
    }
    // end by Balaji
    inline void Player::ProcessSpu()
    {
        if(useOld)
          spu_.ProcessSpuEvents();
    } 
    inline bool Player::SpuHasOsd() 
    {
        //printf("------- %s ----------\n", __PRETTY_FUNCTION__);
        return useOld?spu_.HasOsd():false;
    }
    inline void Player::HideSPU() 
    {
        //printf("------- %s ----------\n", __PRETTY_FUNCTION__);
        //spu_.Hide(true);
        if(useOld)
      spu_.TakeOsd();
        // TODO rename  spu_.OsdTaken();
    }
    inline void Player::SetMenuDomain(bool isInMenuDomain)
    {
        //printf("------- %s ----------\n", __PRETTY_FUNCTION__);
        isInMenuDomain_ = isInMenuDomain;
        if(useOld)
          spu_.SetMenuDomain(isInMenuDomain);
    }

    inline std::string Player::GetTitleInfoString()
    {
        //printf("------- %s ----------\n", __PRETTY_FUNCTION__);
        return titleInfo_;
    }

    inline const char *Player::GetAspectString() const
    {
#ifdef USE_PLAYPES
        return NULL;
#else   
        //printf("------- %s ----------\n", __PRETTY_FUNCTION__);
        // gets no usfull information
        return xineLib_.GetAspectString();
#endif
    }
 
    inline std::string Player::GetAudioLangCode() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        char lang[XINE_LANG_MAX];
        if (!xineLib_.GetAudioLangCode(lang))
          return "";
        else 
          return lang;
#endif
    }

    //inline const char *Player::GetSpuLangCode() const
    inline std::string Player::GetSpuLangCode() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        char lang[XINE_LANG_MAX];
        if (!xineLib_.GetSpuLangCode(lang))
          return "";
        else 
          return lang;  
#endif
    }

    inline bool Player::GetReplayMode(bool &Play, bool &Forward, int &Speed)
    {
#ifdef USE_PLAYPES
        if(playMode == pmPause)
            {Play = false; Forward = true;Speed = -1;}
        else if (playMode == pmPlay)
            {Play=true; Forward=true;Speed= -1;}
        else // lets say we are playing, by default
            {Play=true; Forward=true;Speed= -1;}

        return true;
#else
        int speed_param = xineLib_.GetParamSpeed();
        //printf (" --- %s --- speed %d \n", __PRETTY_FUNCTION__, speed_param);

        // TODO play direction

        switch (speed_param)
        {
            case XINE_SPEED_PAUSE:
                Play=false; Forward=true;Speed= -1;
                return true;
            case XINE_SPEED_SLOW_4:
                Play=false; Forward=true;Speed= 3;
                return true;
            case XINE_SPEED_SLOW_2:
                Play=false; Forward=true;Speed= 1;
                return true;
            case XINE_SPEED_NORMAL:
                Play=true; Forward=true;Speed= -1;
                return true;
            case XINE_SPEED_FAST_2:
                Play=true; Forward=true;Speed= 1;
                return true;
            case XINE_SPEED_FAST_4:
                Play=true; Forward=true;Speed= 3;
                return true;
            default:
              return false;
       }
#endif
    }

    inline bool Player::GetIndex(int &Current, int &Total, bool SnapToFrame)
    {
#ifdef USE_PLAYPES

#if APIVERSNUM < 10704
         const int FPS = FRAMESPERSEC;
#else
         const int FPS = DEFAULTFRAMESPERSECOND;
#endif /* APIVERSNUM < 10704 */

                       
        if (ReadThread.HasData())
        {
            Total = (int)ReadThread.MrlDuration() * FPS;

#if APIVERSNUM >= 10716 
            uint64_t stc = DeviceGetSTC();
            if ( uint64_t(-1) == stc ) // no device or no playback or no STC
            { 
                Current = 0;
            } 
            else
            {
                Current = (stc * FPS) /90000L;
            }
#else /* APIVERSNUM < 10716 */
            int framesInQueue = DeviceAproxFramesInQueue();
            // printf("Aprox. Frames in Queue: %i\n", framesInQueue);
            Current = (( duration* FPS )/90000UL) - framesInQueue;
#endif /* APIVERSNUM < 10716 */
        } // ReadThread.HasData()
        else 
        {
            Current = 0 ; 
            Total   = 0; 
        }

        return true;
#else /* USE_PLAYPES */
        //printf("Player.h:%i calling xineLib_.GetIndex()\n",__LINE__);
        bool x = xineLib_.GetIndex(Current, Total);
        //printf("Player.h:%i calling xineLib_.GetIndex() done\n",__LINE__);
    return x;
#endif /* USE_PLAYPES */
    }

    ///////////////////////////////////////////////////////////////////////////
    // Xine lib stream info and meta info

    ///////////////////////////////////////////////////////////////////////////
    // xine stream info
    inline uint32_t Player::GetBitrate()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.GetBitrate();
#endif
    }

    inline uint32_t Player::Seekable()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.Seekable();
#endif
    }

    inline uint32_t Player::VideoWidth()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoWidth();
#endif
    }

    inline uint32_t Player::VideoHeight()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoHeight();
#endif
    }

    inline uint32_t Player::VideoRatio()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoRatio();
#endif
    }

    inline uint32_t Player::VideoChannels()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoChannels();
#endif
    }

    inline uint32_t Player::VideoStreams()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoStreams();
#endif
    }

    inline uint32_t Player::VideoBitrate()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoBitrate();
#endif
    }

    inline uint32_t Player::VideoFourcc()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoFourcc();
#endif
    }

    inline uint32_t Player::VideoHandled()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoHandled();
#endif
    }

    inline uint32_t Player::FrameDuration()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.FrameDuration();
#endif
    }

    inline uint32_t Player::AudioChannels()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioChannels();
#endif
    }

    inline uint32_t Player::AudioBits()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioBits();
#endif
    }

    inline uint32_t Player::AudioSamplerate()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioSamplerate();
#endif
    }

    inline uint32_t Player::AudioBitrate()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioBitrate();
#endif
    }

    inline uint32_t Player::AudioFourcc()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioFourcc();
#endif
    }

    inline uint32_t Player::AudioHandled()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioHandled();
#endif
    }

    inline uint32_t Player::HasChapters()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.HasChapters();
#endif
    }

    inline uint32_t Player::HasVideo()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return playing_ && xineLib_.HasVideo();
#endif
    }

    inline uint32_t Player::HasAudio()
    {
#ifdef USE_PLAYPES
        return 1;
#else
        return xineLib_.HasAudio();
#endif
    }

    inline uint32_t Player::IgnoreVideo()
    {
#ifdef USE_PLAYPES
        return 1;
#else
        return xineLib_.IgnoreVideo();
#endif
    }

    inline uint32_t Player::IgnoreAudio()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.IgnoreAudio();
#endif
    }
    inline uint32_t Player::IgnoreSpu()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.IgnoreSpu();
#endif
    }

    inline uint32_t Player::VideoHasStill()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoHasStill();
#endif
    }

    inline uint32_t Player::MaxAudioChannel()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.MaxAudioChannel();
#endif
    }

    inline uint32_t Player::MaxSpuChannel()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.MaxSpuChannel();
#endif
    }

    inline uint32_t Player::AudioMode()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.AudioMode();
#endif
    }

    inline uint32_t Player::SkippedFrames()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.SkippedFrames();
#endif
    }

    inline uint32_t Player::DiscardedFrames()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DiscardedFrames();
#endif
    }

    inline uint32_t Player::VideoAfd()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.VideoAfd();
#endif
    }

    inline uint32_t Player::DvdTitleNumber()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdTitleNumber();
#endif
    }

    inline uint32_t Player::DvdTitleCount()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdTitleCount();
#endif
    }

    inline uint32_t Player::DvdChapterNumber()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdChapterNumber();
#endif
    }

    inline uint32_t Player::DvdChapterCount()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdChapterCount();
#endif
    }

    inline uint32_t Player::DvdAngleNumber()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdAngleNumber();
#endif
    }

    inline uint32_t Player::DvdAngleCount()
    {
#ifdef USE_PLAYPES
        return 0;
#else
        return xineLib_.DvdAngleCount();
#endif
    }


    ///////////////////////////////////////////////////////////////////////////
    // xine meta infos 

    inline const char *Player::GetTitle() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.GetTitle()?xineLib_.GetTitle():"";
#endif
    }

    inline const char *Player::Comment() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.Comment();
#endif
    }

    inline const char *Player::Artist() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.Artist();
#endif
    }

    inline const char *Player::Genre() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.Genre();
#endif
    }

    inline const char *Player::Album() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.Album();
#endif
    }

    inline const char *Player::Year() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.Year();
#endif
    }

    inline const char *Player::VideoCodec() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.VideoCodec();
#endif
    }

    inline std::string Player::AudioCodec()
    {
#ifdef USE_PLAYPES
        return "";
#else
        const char *buf  = xineLib_.AudioCodec();
        if (!buf)
          return "";
        else 
          return buf;
#endif
    }

    inline const char *Player::SystemLayer() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.SystemLayer();
#endif
    }

    inline const char *Player::InputPlugin() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.InputPlugin();
#endif
    }

    inline const char *Player::CDIndexDiscId() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.CDIndexDiscId();
#endif
    }

    inline const char *Player::TrackNumber() const
    {
#ifdef USE_PLAYPES
        return "";
#else
        return xineLib_.TrackNumber();
#endif
    }
}
}

#endif // XINE_MEDIAPLAYER_PLAYER_H_INCLUDED
