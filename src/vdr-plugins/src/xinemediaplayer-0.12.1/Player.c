/***************************************************************************
 *   Copyright (C) 2005-2011 by Reel Multimedia                            *
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

// Player.c
#include <memory>
#include <linux/types.h>
#include <linux/dvb/video.h>  // video_still_picture

#include <vdr/status.h>
#include <vdr/remote.h>
#include <vdr/plugin.h>
#include <vdr/debugmacros.h>
#include <vdr/resumeDvd.h>

#include <iostream>

#include "Player.h"
#include "Control.h"
#ifdef USE_PLAYPES
#include "timecounter.h"
#else
#include "XineEvent.h"
#endif
#include "xineOsd.h"
#include "Utils.h"
#include "curl_download.h"

#include "tree.hh"

//#define DEBUG_PLAYER(format, args...) printf (format, ## args)
#define DEBUG_PLAYER(format, args...)

//#define DEBUG_XINE_EVENT(format, args...) printf (format, ## args)
#define DEBUG_XINE_EVENT(format, args...)

//#define DEBUG_AUDIO_ID(format, args...) printf (format, ## args)
#define DEBUG_AUDIO_ID(format, args...)
#define DEBUG_RESTARTPLAYBACK printf("calling RestartPlayback() at %s:%d\n", __FILE__, __LINE__)
using std::string;

std::string make_gst_uri(const char * mrl);

// TODO  Xine Setup
//static int XineSetup_Display = 0;
                        //0;  short
                      //1; // medium
                      //2; // long

namespace Reel
{
namespace XineMediaplayer
{

#ifdef USE_PLAYPES
int useOld = 1;
#endif
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //Tools
   inline std::string GetFileName(const std::string &path)
   {
       return path.substr(path.find_last_of("/") + 1, path.size());
   }


   //////////////////////////////class cPlayerStatus/////////////////////////////////////////////////////////////////////
   cPlayerStatus PlayerStatus;
   ePlayerStatus cPlayerStatus::Status()
   {
       return Status_;
   }


   void cPlayerStatus::SetPlayerStatus(ePlayerStatus status)
   {
       lastStatusChange_ = time(NULL);
       Status_ = status;
   }

   //remember last error and time
   void cPlayerStatus::SetError(eError err)
   {
       if(PlayerError_ == err)
       {
           ErrorCount_++;
       }
       else
       {
           lastError_ = time(NULL);
           PlayerError_ = err;
           ErrorCount_ = 0;
       }
   }

   std::string cPlayerStatus::ErrorString() const
   {
       // return string according to PlayerError

       switch(PlayerError_)
       {
           case eOpenError:
               //if (Player::OpenError_) return Player::OpenError_;
               return std::string(tr("Open Error"));

           case eNetworkOpenError:
               return std::string(tr("Network read error"));

           default: //anything else
           case eNone:
               return std::string(); //empty string

       } // switch
   }

   time_t cPlayerStatus::ErrorTime()const
   {
       return lastError_;
   }

   int cPlayerStatus::ErrorCount()const
   {
       return ErrorCount_;
   }


   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Player

   const int Player::MaxAudioTracks    = 0x20;
   const int Player::AudioTrackMask    = 0x1F;
   const int Player::AC3AudioTrackMask = 0x07;
   const int Player::MaxSubpStreams    = 0x20;
   const int Player::SubpStreamMask    = 0x1F;
   int Player::NetworkErrorCount = 0;


    Player::Player(const char *mrl, bool playlist, std::vector<std::string> playlistEntries) NO_THROW :
#ifdef USE_PLAYPES
        cPlayer(pmAudioVideo),
        cThread("xinemediaplayer Player Thread"),
        duration(0),
        duration_pad(0),
        ringbuffer(0),
#else
        cPlayer(useOld?pmAudioVideo:pmExtern_THIS_SHOULD_BE_AVOIDED),
        cThread("xinemediaplayer Player Thread"),
        xineLib_(XineLib::Instance()),
#endif
        playing_(false),
        mrlIsInPlaylist_(playlist),
        playlist_(playlist && !playlistEntries.empty()),
        playlistEntries_(playlistEntries),
        loopmode_(false),
        isInMenuDomain_(useOld?true:false),
        mrl_(mrl),
        titleInfo_(""),
        writeWait(60000),
        number(0)
        {
            wasInMenu_ = false;
            srand((unsigned)time(0));

            //HERE;
            DEBUG_PLAYER ("DEBUG [xinemedia]:  Player::Player  --- mrl  %s \n", mrl);

#if 0
            CreateItemTree();
            InitPlayListPos();
#endif
            play_mode = PLAY_NORMAL;
            player_status = 0;
            NetworkErrorCount = 0;
            pl_pos = 0;
            // depends of mrl
            SetInitLangCode();
#ifdef USE_PLAYPES
            playMode = pmPlay;
#else
            switch (BackEnd()) {

                case eXineBackEnd:
                    xineLib_.SetParamAudioReportLevel(5);
                    break;

                case eGstreamerBackend:
             //  gstLib_.Init();
                    break;
                case eExternalBackend:
                    extLib_.Init();
                    break;
            } // switch
#endif

    }

    Player::~Player() NO_THROW
    {
        DEBUG_PLAYER ("DEBUG [xinemedia]:  Player::~Player  \n");

        cXineOsd::isPlaying = false;
        esyslog("[xinemediaplayer] Player::~Player(), Running()=%i", Running());

        //printf("Cancelling Player-thread. Running?%i\n", Running());
        Cancel(3);
        //printf("Cancelling Done. Running?%i\n", Running());
#ifdef USE_PLAYPES
        DELETENULL(ringbuffer);
#else
try {
            switch (BackEnd()) {
            case eXineBackEnd:
                xineLib_.Exit();
                break;

            case eGstreamerBackend:
                gstLib_.Stop();
                break;
            case eExternalBackend:
                extLib_.Exit();
                break;

            } // switch
} catch(...) {}
#endif
        //Detach();

#if 1
        // CALLBACK
        const char* already_called_plugin = cRemote::GetPlugin();
        if (already_called_plugin)
        {
            //printf("\033[1;91mThis plugin was already called : %s\033[0m\n", already_called_plugin);
            // if another plugin was called in the mean time. Let that call through.
            // if xinemediaplayer was called, then block it, since we are exiting xinemediaplayer.

            // cRemote::GetPlugin() removes the "call plugin cache".
            // So, put it back using cRemote::CallPlugin()
            if (strcmp(already_called_plugin, "xinemediaplayer") != 0)
                cRemote::CallPlugin(already_called_plugin);
            else
                printf("Exiting xinemediaplayer, so donot call xinemediaplayer\n\n");
        }
        if(playlist_ && cXineOsd::k_blue == FILEBROWSER_PLUGIN)
        {
            CallFilebrowser(xine_exit);
        }
        else if ( cXineOsd::k_blue == SHOUTCAST_PLUGIN)
            cRemote::CallPlugin("shoutcast");
        else if ( cXineOsd::k_blue == MMFILES_PLUGIN)
            cRemote::CallPlugin("mmfiles");
        else if ( cXineOsd::k_blue == YOUTUBE_PLUGIN)
            cRemote::CallPlugin("youtube");
#endif

    }

#ifdef USE_PLAYPES
/**
 * returns number of bytes sent as pes packet from ringbuffer
 * interesting return values are 0, -1
 * -1 indicates no data was got from buffer
 *  0 indicates Device was not ready
 */
    int Player::PlayOnePesPacket()
    {
   //     printf("PlayOnePesPacket()\n");

        cMutexLock mutexLock(&Mutex);
        //check if device is ready
        if(DevicePoll(Poll,100))
        {
            int len = 0;
            uchar *Data = ringbuffer->Get(len);

            if(new_mrl_opened && Data)
            {
                duration_pad = duration = 0;
                new_mrl_opened = false;

                // discard all data that have not been sent by vdr's cDevice class
                //DeviceClear();
                //PlayPes(NULL, 0, false);
            }

            if((!Data || len <= 0) && (ReadThread.Status() == eReachedEOF) && ringbuffer->Available()) {
                // Fill buffer so we get the remaining bytes
                int fill = BUF_MARGIN-ringbuffer->Available();
                uchar dummy[BUF_MARGIN];
                memset(&dummy, 0xFF, sizeof(dummy));
                ringbuffer->Put(dummy, fill);
                Data = ringbuffer->Get(len);
                printf("Player::PlayOnePesPacket after filled %d to get remaining %p %d %d\n", fill, Data, len, ringbuffer->Available());
            }

            if(!Data || len <= 0)
            {
                return -1;
#if 0
                /* No data in buffer and Read till EOF, then play next
                 * TODO: wait till the device plays back all the
                 * pes packets that were sent
                 **/
                if(ReadThread.Status() == eReachedEOF)
                {
                    printf("Reached EOF, waiting for DeviceFlush()\n");

                    while(Running() && !DeviceFlush(10))
                        ;

                    if(Running())
                    {
                        printf("---------- DeviceFlush() done ---------\n");
                        Player::PlayNextPlayListEntry(true);
                    }
                    else
                        return;
                }
                else
                {
                    /* no data got from buffer
                       Nothing to do in the rest of the loop
                       take a quick nap and try again
                       */
                    cCondWait::SleepMs(3);
                    //               printf("No data in buffer yet\n");
                    return;
                }
#endif
            }
#define PTS 1
            uint64_t pts;
#if PTS
//DL            pts = (uint64_t) (duration*90000UL) ; // 90KHz clock
            pts=duration;
#else
            pts  = 0;
#endif

            //Play as pes packets
            int last_valid_frame_ending = PlayAsPesPackets(Data, len, pts, duration);

            if(last_valid_frame_ending)
            {
                // delete the used bytes from the buffer
                ringbuffer->Del(last_valid_frame_ending);
            }
            else
            {
                //printf("No frames found in %i bytes : %i available\n",len, ringbuffer->Available());
                esyslog("No frames found in %i bytes : %i available",len, ringbuffer->Available());
                if(len>2000)
                {
                    ringbuffer->Del(len);
                }
                //cCondWait::SleepMs(10);
            }
            return last_valid_frame_ending;
        }//DevicePoll()
        else
        {
//            printf("Device Not ready\n");
            return 0;
        }

    }//PlayOnePesPacket()

#define PES_LEN 4000
    /* returns the number of bytes from ringbuffer to be deleted
     * possibly these bytes were played or these number of bytes
     * did not have any valid mpa frame
     */
//DL    int Player::PlayAsPesPackets(uchar* Data, int len, uint64_t pts, float& duration)
    int Player::PlayAsPesPackets(uchar* Data, int len, uint64_t pts, uint64_t& duration)
    {

        mpa_frame_t frame;
        uchar pesPacket[PES_LEN];

        int payload_i = AddPesAudioHeader(pesPacket, PES_LEN, pts);

        int i = 0,j = 0;
        int last_valid_frame_ending = 0;
        int payload_len = 0;

        // find all valid mpa frames and copy into the pes packet
        while(i<len && Data && Running())
        {
            j = find_valid_mpa_frame(Data+i, len-i, &frame);

            if(j>=0) //check for min. packet length?
            {
                //copy to pes packet
                if(frame.len>0)
                {
                    if(payload_i + frame.len > PES_LEN) // more than what pesPacket can hold
                        break;

                    i += j;

                    if(i+(int)frame.len>len) // not enough data in buffer to complete this frame
                        break;

                    //copy one mpa frame
                    memcpy(pesPacket+payload_i, Data+i, frame.len);

                    payload_i += frame.len;

                    i += frame.len;
                    last_valid_frame_ending = i;
                    payload_len += frame.len;

                    duration_pad += (uint64_t)(frame.samples_per_frame)*90000UL;
                    duration += duration_pad/frame.samplerate;
                    duration_pad %= frame.samplerate;
//DL                     duration += frame.duration;
                }
                else
                    i++;
            }
            else
                // no more valid frames
                break;

        } // while

        if(last_valid_frame_ending && Running())
        {
            int pes_len = payload_i - 6;
            pesPacket[4] = (pes_len >> 8) & 0xff;
            pesPacket[5] = pes_len & 0xff;

            if (!firstPacketSent)
            {
                dsyslog(" ---- before sending first packet ------ ");
                DeviceClear();
                PlayPes(NULL, 0, false);
                firstPacketSent = true;
                timeCounter.CleanStart();
                dsyslog("----- first PES packet sent via PlayPes ------ timeCounter.CleanStart-ed");

                // NetClient still cannot play mpeg-2 audio. Warn users.
                // Some shoutcast streams are mpeg2 Layer-III :(
                if (frame.id != 0x03)
                    Skins.QueueMessage(mtError, tr("Only MPEG-1 format supported"), 2, -1);
            }

            /*int ret =*/ PlayPes(pesPacket, payload_i, false);
            /*
              dsyslog("[xinemediaplayer] PlayPes(, %i, false) = %i    afq=%i   buffer.avail=%i",
                     payload_i, ret, DeviceAproxFramesInQueue(), ringbuffer->Available());
             */
            cCondWait::SleepMs(3);
        }

        return last_valid_frame_ending;
    }

#endif

    void Player::Action()
    {
        //printf("++++++++ \033[7;94mPlayer::Action()\033[0m ++++++++\n");

        // prepare playlist and playlist-tree ie. item tree
        CreateItemTree();
        InitPlayListPos();

        //nobody has called PlayFileNumber() yet
        playfile_pl_pos = -1;

        if (cXineOsd::k_blue != MUSIC_LIBRARY)
        {
            printf("\033[0;92m(%s:%d)Not Music Library\033[0m\n", __FILE__, __LINE__);
        //TODO remove this
        {
            cMutexLock l(&mutexVItemTree);
            vItemTree[pl_pos].Rewind();
        } // mutex lock vItemTree

        mrl_ = GetNextFromTree( pl_pos );

        // nothing in the current tree; go to next tree
        if ( mrl_.empty() )
            mrl_ = GetNextPlayListEntryCyclic(true);

        // initialize xine stream and start playing mrl
        try {
            StartPlayback();
        } catch(...){
            StopPlayback();
            return;
        }
        }
        else {
            // mediaplayer  in music library mode, do not start playing immediately
#ifdef USE_PLAYPES
            // TODO: xineLib_.Init() equivalent for PES player here
#else
            switch(BackEnd()) {

            case eXineBackEnd:
                xineLib_.Init();
                break;

            case eGstreamerBackend:
                printf("\033[0;91m(%s:%d)Music Library GST INIT\033[0m\n", __FILE__, __LINE__);
                gstLib_.Init(false);
                break;
            case eExternalBackend:
                extLib_.Init();
                break;
            } // switch
#endif
        }

#ifdef USE_PLAYPES
        if(!DeviceSetAvailableTrack(ttAudio, 0 ,0xC0))
        {
            printf("(%s:%i) Setting Audio tracks failed.\n", __FILE__, __LINE__);
        }
        DeviceSetCurrentAudioTrack(ttAudio);
        int bytesPlayed = 0;
#endif

        while(Running() /*&& playing_*/)
        {
            //printf("\033[7;91mThread iteration begins\033[0m\n");

            if (!IsAttached())
                printf("\033[7;91m Player Detached!\033[0m\n");

#ifdef USE_PLAYPES
            if (ReadThread.HasData())
                bytesPlayed = PlayOnePesPacket();
            else
            {
if(ringbuffer->Available()) printf("ReadThread has NO data %d\n",ringbuffer->Available());
                firstPacketSent = false;
                timeCounter.CleanStop();
                bytesPlayed = -1;
            }

            if( bytesPlayed < 0 && ReadThread.Status() == eReachedEOF)
            {
                //Wait for device buffer to become empty || user input
                if(Running() && !DeviceFlush(100) && playfile_pl_pos < 0) {
                    ; // Don't block in while - keep processing HandleVdrKeys
                } else if(Running())
                {
printf("Reached EOF, play next file in playlist %d\n", ringbuffer->Available());
                    //play next file in playlist only if user has not choosen a file
                    if (playfile_pl_pos < 0)
                    {
                        //printf("---------- DeviceFlush() done ---------\n");
                        Player::PlayNextPlayListEntry(true);
                    }
                    //else play user choosen file below
                }
                else
                    break; // out of while
            }

            //Sleep needed to give up lock
            cCondWait::SleepMs(5);
#else
            //handle xine events
            try {
                HandleEvents();
            } catch(...) {printf("----------------------Handle Events catched\n");}
#endif

            //handle vdr keys
            try {
                HandleVdrKeys();
            } catch(...) {printf("----------------------HandleVdrKeys catched\n");}

            // check if PlayFileNumber was called
            if(playfile_pl_pos >= 0)
            {
                bool flag = PlayFileNumber_r(playfile_pl_pos);
                if(!flag)
                    printf("could not play #%i\n", playfile_pl_pos);
#if 0
                else
                    printf("playing #%i \n", playfile_pl_pos);
#endif

                // reset
                playfile_pl_pos = -1;
            }

#ifdef USE_PLAYPES
        // donot not wait
#else
            //wait 0.5 sec
            cCondWait::SleepMs(500);
#endif

        }// while (Running)

#ifdef USE_PLAYPES
        printf("--------- Waiting for DeviceFlush(100) -------------\n");
        // if player is still running, Wait for the device to
        // finish playing back all the packets sent to it
        while(Running() && !DeviceFlush(100));

        printf("--------- DeviceFlush(100) Done -------------\n");
#endif
        //printf("\033[7;92m Thread Ended\033[0m\n");
    }

    bool Player::InitPlayListPos()
    {
        unsigned int i = 0;
        std::string myMrl = mrl_;
        size_t found;

        // remove 'file://'
        if ((found=myMrl.find("file://")) != std::string::npos)
            myMrl.erase(found,found+7);

        for (; i < playlistEntries_.size() ; ++i)
        {
            //printf("(%s:%d) \t%d...(%s\t%s)\n", __FILE__, __LINE__, i, myMrl.c_str(), playlistEntries_[i].c_str() );
            if( playlistEntries_[i] == myMrl)
            {
                //LOC;printf("mrl(%s)==(%s)playlistentry(%i)\n",myMrl.c_str(), playlistEntries_.at(i).c_str(), i);
                pl_pos = i;

                // can continue playing, since the current item is in the new playlist
                return false;
            }
        }

        // mrl_ not in playlistEntries
        // start from beginning
        pl_pos = 0;
        return true; // stop current playback and play afresh
#if 0

        //LOC; printf("\n\nmrl not found in playlist.\n\n");
        /***
         * Play the new playlist if possible
         ***/
        if ( !playlistEntries_.empty() )
        {
            pl_pos = 0;
            // get mrl from the tree

            {
                cMutexLock l(&mutexVItemTree);
                vItemTree[pl_pos].Rewind();
            } // mutex lock vItemTree

            mrl_ = GetNextFromTree( pl_pos );

            // nothing in the current tree; go to next tree
            if ( mrl_.empty() )
                mrl_ = GetNextPlayListEntryCyclic(true);

            //mrl_ = playlistEntries_[pl_pos];
            ReStartPlayback();
        }
        else
        {
            //LOC; printf("playlist is empty. Stopping xinemediaplayer.\n");
            StopPlayback();
        }
#endif
    }//end InitPlayListPos()

    // to be called from within thread
    bool Player::PlayFileNumber_r( unsigned int num )
    {
        // outside bounds of playlist
        if ( num < 0 || num >= playlistEntries_.size() )  return false;

        pl_pos = num ;
        //printf("Playing #%i -----------------\n", num);
        esyslog("[xinemediaplayer] PlayFileNumber_r(%i)", num);

        //  get mrl from the tree
        {
            cMutexLock l(&mutexVItemTree);
            vItemTree[num].Rewind();
        } // mutex lock vItemTree

        mrl_ = GetNextFromTree( num );

        // nothing in the current tree; go to next tree
        if ( mrl_.empty() ) {
            printf("======= Got nothing from tree #%d. Trying next Tree ===========\n", pl_pos);
            mrl_ = GetNextPlayListEntryCyclic(true);
        }


        //mrl_ = playlistEntries_[ num ];
        DEBUG_RESTARTPLAYBACK;
        ReStartPlayback();

        return true;
    }

    bool Player::PlayFileNumber( unsigned int num )
    {
        // outside bounds of playlist
        if ( num < 0 || num >= playlistEntries_.size() )  return false;

        playfile_pl_pos = num ;
        //printf("Got #%i -----------------\n", num);
        return true;
#if 0
        //  get mrl from the tree
        vItemTree[num].Rewind();
        mrl_ = GetNextPlayListEntryCyclic(true);
        //mrl_ = playlistEntries_[ num ];
        ReStartPlayback();

        return true;
#endif
    }

    void Player::SetPlayMode(int mode)
    {
        play_mode = mode;

        if ( play_mode == PLAY_REPEAT_PLAYLIST )
            loopmode_ = true;
        else
            loopmode_ = false;
    }


    int Player::GetPlayMode()
    {
        return play_mode;
    }


    void Player::CreateItemTree()
    {
        //HERE;
        cItemTree iTree; // tmp tree
        unsigned int i;
        cMutexLock l(&mutexVItemTree);
        vItemTree.clear();

        // iterate thro' playlist
        for (i=0; i<playlistEntries_.size();++i)
        {

            vItemTree.push_back(iTree);
            vItemTree[i].set_head( playlistEntries_[i] );

        }
    }


    void Player::ReplacePlayList(std::vector<std::string> pl)
    {
        //HERE;
        playlistEntries_ = pl;

        //XXX set item_trees
        CreateItemTree();

        //update parameters with new playlist's
        playlist_ = mrlIsInPlaylist_ && !playlistEntries_.empty();

        if (playlistEntries_.empty()) {
            StopPlayback(); // stop the playback
        }
        else {
            bool restart = InitPlayListPos(); // compute the new mrl position in the playlist

            // if current mrl not in playlist, restart playback
            // only if previously playing
            if (restart && playing_)
            {
                // get mrl from the tree
                {
                    cMutexLock l(&mutexVItemTree);
                    vItemTree[pl_pos].Rewind();
                } // mutex lock vItemTree

                mrl_ = GetNextFromTree( pl_pos );

                // nothing in the current tree; go to next tree
                if ( mrl_.empty() )
                    mrl_ = GetNextPlayListEntryCyclic(true);

                //mrl_ = playlistEntries_[pl_pos];
                DEBUG_RESTARTPLAYBACK;
                ReStartPlayback();
            } // if restart
            else {
                // if no restart required, that is:
                // the currently playing mrl is also in the new playlist
                // then do not play this item twice

                cMutexLock l(&mutexVItemTree);
                int size = vItemTree[pl_pos].Size();

                // if there is only one leaf (mrl) in this tree
                // and since this leaf/mrl is being played, lets not play it again
                // go to end of this tree, "undoing" CreateItemTree()
                if (size == 1)
                    vItemTree[pl_pos].GoToLastLeaf();

            } // else restart
        } // else

    }


    void Player::Activate(bool on) NO_THROW
    {
        //HERE;
        try
        {
            if (on && !playing_)
            {
#if 0
                //TODO remove this
                vItemTree[pl_pos].Rewind();
                mrl_ = GetNextFromTree( pl_pos );

                // nothing in the current tree; go to next tree
                if ( mrl_.empty() )
                    mrl_ = GetNextPlayListEntryCyclic(true);

                StartPlayback();
#endif
                // start thread
                Start();
            }
            else if (!on && playing_)
            {
                StopPlayback(); // This cannot fail.
            }
        }
#ifdef USE_PLAYPES
        catch (...)
        {
            Detach();
        }
#else
        catch (XineError const &ex)
        {
            LogXineError(ex);
            Detach();
        }
#endif
    }

    void Player::ShowSplashScreen()
    {
        DEBUG_PLAYER ("DEBUG [xinemedia]:  Player::ShowSplashScreen() \n");
        string stillPicture = cPlugin::ConfigDirectory();
        // depends of mrl
        stillPicture += "/xinemediaplayer/splash.mpg";

        uchar *buffer;
        int fd;
        struct stat st;
        struct video_still_picture sp;

        if ((fd = open (stillPicture.c_str(), O_RDONLY)) >= 0)
        {
            fstat (fd, &st);
            sp.iFrame = new char[st.st_size];
            if (sp.iFrame)
            {
                sp.size = st.st_size;
                if (read (fd, sp.iFrame, sp.size) > 0)
                {
                    buffer = (uchar *) sp.iFrame;
                    cDevice::PrimaryDevice()->StillPicture(buffer, sp.size);
                }
                delete [] sp.iFrame;
            }
            close (fd);
        } // if (open())
    }

#define NUMBERTIMEOUT 1500

    eOSState Player::Number(eKeys Key)
    {
        //printf("number: %i\n", number);

        if (numberTimer.TimedOut())
            number = 0;

        number = number * 10 + Key - k0;

        if(number)
        {
            PlayFileNumber(number-1);
            numberTimer.Set(NUMBERTIMEOUT);
        }

        return osContinue;
    }

    void Player::SetInitLangCode()
    {
#ifndef USE_PLAYPES
        switch (BackEnd()) {

           case eXineBackEnd:
        // maybe better to set this with
        try {
            strncpy(selectedLangCode_,xineLib_.GetSetupLang(),sizeof(selectedLangCode_));
        } catch(...) {}
        break;

        case eGstreamerBackend:
            printf("%s not implemented for gst backend\n", __PRETTY_FUNCTION__);
            strncpy(selectedLangCode_,xineLib_.GetSetupLang(),sizeof(selectedLangCode_));
            break;
        case eExternalBackend:
            strncpy(selectedLangCode_,extLib_.GetSetupLang(),sizeof(selectedLangCode_));
            break;
        } // switch

        DEBUG_AUDIO_ID  ("DEBUG [xinemedia]: SetInitLangCode -----------------\
            SetInitLangCode set selectedLangCode_ to %s \n", selectedLangCode_);
#endif
    }

    eOSState Player::ProcessKey(eKeys key) THROWS
    {
        //printf("++++++++ \033[7;94mPlayer::ProcessKey(%i)\033[0m ++++++++\n", key);
        if(kNone!=key) {
printf("++++++++ \033[7;94mPlayer::ProcessKey(%i)\033[0m ++++++++\n", key);
            cMutexLock lock(&mutexKeyQ);
            // put key into queue for the thread to process
            KeyQueue.push(key);
        }
        cMutexLock mRQ(&mutexReturnQ);
        //check return queue
        if(returnQueue.empty())
        {
          //  printf("return queue empty\n");
            return osContinue;
        }

       // printf("return NOT queue empty\n");

        // return value was put in by thread
        eOSState state = returnQueue.front();
        //printf("Got state %i ----------\n", state);
printf("Got state from returnQueue %i ----------\n", state);

        returnQueue.pop(); // remove the oldest element
        return state;
    }


    eOSState Player::ProcessKey_r(eKeys key) THROWS
    {
        //printf("++++++++ \033[7;94mPlayer::ProcessKey_r()\033[0m ++++++++\n");

        if(!IsReplayingDvd())
            switch(key)
            {
                case k0...k9: // selecting an entry by numbers
                    return Number(key);
#ifdef USE_PLAYPES
                case kPause:
                    if(playMode == pmPlay)
                    {
                        playMode = pmPause;
                        DeviceFreeze();
                        timeCounter.Pause();
                    }
                    else
                    {
                        playMode = pmPlay;
                        DevicePlay();
                        timeCounter.Start();
                    }
                    break;

                case kPlay:
                    if(playMode == pmPause)
                    {
                        playMode = pmPlay;
                        DevicePlay();
                        timeCounter.Start();
                    }
                    break;
#endif

                case kDown:  // Prev Entry
                    if(playlist_)
                    {
                        PlayNextPlayListEntry(false);
                    }
                    else if(mrlIsInPlaylist_)
                    {
                        //Detach();
                    }
                    break;

                    //case kDown:  // Next Entry
                case kUp:  // Prev Entry
                    if (playlist_)
                    {
                        PlayNextPlayListEntry(true);
                    }
                    break;

                default:
                    break;
            } // switch

        // if playing a DVD and not in DVD menu, write the title and chapter info into "resume DB"
        //  So, that next time the dvd is inserted mediad gives xinemediaplayer a url
        //  with title.chapter in it to resume playing
        if (IsReplayingDvd() && !PlayerIsInMenuDomain() && writeWait.TimedOut())
        {
            // write resume info for current DVD
            //printf("Title.Chapter := %i.%i\n", DvdTitleNumber(), DvdChapterNumber());
            writeWait.Set(60000);

            ResumeDvd.SetTitleChapter(*cString::sprintf("%i.%i",
                            DvdTitleNumber(), DvdChapterNumber()));
        }

        if(PlayerIsInMenuDomain() && IsReplayingDvd())
        {
#ifndef USE_PLAYPES

            switch (BackEnd()) {

            case eXineBackEnd:
                xineLib_.ProcessKeyMenuMode(key);
                break;

            case eGstreamerBackend:
                GstProcessKeyMenuMode(key);
                break;

            case eExternalBackend:
                extLib_.ProcessKeyMenuMode(key);
                break;
            } // switch

#endif
        }
        else
        {
#ifndef USE_PLAYPES
            switch (BackEnd()) {
                case eXineBackEnd:
                    xineLib_.ProcessKey(key,IsReplayingMusic());
                    break;

            case eGstreamerBackend:
                // catch key to open DVD Menu
                GstProcessKeyMenuMode(key);
                GstProcessKey(key);
                break;

                case eExternalBackend:
                    extLib_.ProcessKey(key,IsReplayingMusic());
                    break;
            } // switch
#endif // USE_PLAYPES

        } // else

        return osContinue;
    }

#ifndef USE_PLAYPES

    void Player::GstProcessKey(eKeys key) {

        printf("============ gst IsPaused? %d IsPlaying %d\n", gstLib_.IsPaused(),
               gstLib_.IsPlaying());
        double playback_rate = gstLib_.PlaybackRate();
            switch(key)
            {
            case kPlay:
                printf("============ gst kPlay\n");
                gstLib_.SetPlaybackRate(playback_rate=1.0);
                gstLib_.Play();
                break;

            case kBack:
                // play DVD and not in DVD Menu == > allow showing of DVD Menu,
                // so donot stop playback
                if (IsReplayingDvd() && !PlayerIsInMenuDomain())
                    break;

                // else stop playback
            case kStop:
                printf("============ gst Stop\n");
                gstLib_.Stop();
                break;

            case kPause:
                printf("============ gst kPause\n");
                gstLib_.Pause();
                break;

            case kGreen:
            case kGreen|k_Repeat:
                gstLib_.SeekRelative(-60); //back 1 minute
                break;

            case kLeft|k_Repeat:
            case kLeft:
                printf("============ gst kLeft\n");
                gstLib_.SeekRelative(-5); //-5 secs
                break;

            case kYellow|k_Repeat:
            case kYellow:
                gstLib_.SeekRelative(60); // 1 minute
                break;

            case kRight|k_Repeat:
            case kRight:
                printf("============ gst kRight|kYellow\n");
                gstLib_.SeekRelative(5); //5secs
                break;

            case kFastRew:
                printf("============ gst kFastRwd\n");
                if (!HasVideo())
                {
                    printf("Has no video. No FF or Frew. Playback rate %f\n", playback_rate);
                    break;
                }

                if (playback_rate == 1) // forward to rewind
                    gstLib_.SetPlaybackRate(-1);
                else
                    gstLib_.SetPlaybackRate(--playback_rate);
                break;

            case kFastFwd:
                printf("============ gst kFastFwd\n");
                if (!HasVideo())
                {
                    printf("Has no video. No FF or Frew. Playback rate %f\n", playback_rate);
                    break;
                }

                if (playback_rate == -1) // rewind to forward
                    gstLib_.SetPlaybackRate(1);
                else
                    gstLib_.SetPlaybackRate(++playback_rate);
                break;

            case kMute:
                printf("============ gst kMute\n");
                gstLib_.ToggleMute();
                break;


            default:
                break;

            } // switch

    } // GstProcessKey()

    void Player::GstProcessKeyMenuMode(eKeys key) {

        std::cout << "Playing DVD in Menu mode" << std::endl;
        GstNavigationCommand command;

        switch (key) {
          case kLeft : command =  GST_NAVIGATION_COMMAND_LEFT;    break;
          case kUp   : command =  GST_NAVIGATION_COMMAND_UP;       break;
          case kRight: command =  GST_NAVIGATION_COMMAND_RIGHT;   break;
          case kDown : command =  GST_NAVIGATION_COMMAND_DOWN;  break;
          case kOk   : command =  GST_NAVIGATION_COMMAND_ACTIVATE; break;
          case k1    : command =  GST_NAVIGATION_COMMAND_MENU1;  break;
          case k2    : command =  GST_NAVIGATION_COMMAND_MENU2;   break;
          case k3    : command =  GST_NAVIGATION_COMMAND_MENU3;  break;
          case k4    : command =  GST_NAVIGATION_COMMAND_MENU4;  break;
          case k5    : command =  GST_NAVIGATION_COMMAND_MENU5;  break;
          case k6    : command =  GST_NAVIGATION_COMMAND_MENU6;  break;
          case k7    : command =  GST_NAVIGATION_COMMAND_MENU7;   break;
          case kNext : command =  GST_NAVIGATION_COMMAND_LEFT;  break;
          case kPrev : command =  GST_NAVIGATION_COMMAND_LEFT; break;
          default    : command =  GST_NAVIGATION_COMMAND_INVALID; break;
        } // switch

        bool ret = gstLib_.SendGstCommand(command);
        isyslog("SendGstCommand returns %d for %d key", ret, key);

    }  //GstProcessKeyMenuMode()
#endif // USE_PLAYPES

    // fetch audio track infos from xine_lib
    void Player::SetAllAudioTracks()
    {

#ifdef USE_PLAYPES
#else
        unsigned int old_count = aviableAudioTracks_.size();
        aviableAudioTracks_.clear();
        SetMenuDomain(false);
        std::cout << "Audio lang code " << GetAudioLangCode() << std::endl;
        bool isMenuDomain  = false;

        switch (BackEnd()) {

        case eXineBackEnd:
            isMenuDomain = (GetAudioLangCode().find("menu") == 0);
            break;

        case eGstreamerBackend:
            isMenuDomain = gstLib_.IsInDVDMenu();
            break;

        case eExternalBackend:
            isMenuDomain = extLib_.IsInDVDMenu();
            break;
        } // switch

        if (isMenuDomain)
        {
            SetMenuDomain(true); // looks strang but works

            /* set playback rate to NORMAL */
            switch (BackEnd()) {

            case eXineBackEnd: {
            int speed = xineLib_.GetParamSpeed();
            if(XINE_SPEED_NORMAL != speed) {
              DEBUG_PLAYER ("DEBUG [xinemedia]: Reset to normal play due to menu domain (was %d)\n", speed);
              xineLib_.SetParamSpeed(XINE_SPEED_NORMAL);
            } // if
            }// case
            break;

            case eGstreamerBackend:
                if (gstLib_.PlaybackRate() != 1.0) // set to normal playback
                    gstLib_.SetPlaybackRate(1.0);
                break;

            case eExternalBackend:
                if (extLib_.PlaybackRate() != 1.0) // set to normal playback
                    extLib_.SetPlaybackRate(1.0);
                break;
            } // switch

            wasInMenu_ = true;
            SetAllSpuTracks();
            DeviceClrAvailableTracks();
            SetAvailableTracks();
            DEBUG_AUDIO_ID( "\t \033[0;44m  GetAudioLangCode \"menu\"  return \033[0m \n");
            return;
        }

        // TODO  workaround for autodetect
        int currentAudioTrack = -2;
        tTrackId aTrack;
        try {

            switch (BackEnd()) {

            case eXineBackEnd:
            currentAudioTrack = xineLib_.GetParamAudioChannelLogical();
            break;

            case eGstreamerBackend:
                currentAudioTrack = gstLib_.AudioStream();
                gstLib_.SetAudioTagsChanged(false);
                break;

            case eExternalBackend:
                currentAudioTrack = extLib_.AudioStream();
                break;
            } // switch

        } catch(...) {}

        DEBUG_AUDIO_ID("\t \033[0;44m  current Audio track Nr %d (%d) \033[0m \n",
                       currentAudioTrack, MaxAudioChannel());
        // -1:  auto detect
        snprintf(aTrack.language, sizeof(aTrack.language), "auto");
        memset (aTrack.description,0,sizeof(aTrack.description));
        aviableAudioTracks_.push_back(aTrack);


        //int dolbyTracks = 0;
        std::string lang;

        for (int i= 0;(i< MaxAudioTracks) && (i < MaxAudioChannel());i++)
        {
            // tTrackId = {
            //  uint_16 id; // The PES packet id or the PID id;
            //  char language[8];
            //  char description[32];
            // };

            lang.clear();

            switch (BackEnd()) {

            case eXineBackEnd: {
            char langCode[XINE_LANG_MAX];
            if (!xineLib_.GetAudioLangCode(langCode,i))
                ;
               // break;
            lang = langCode;
            } break;

            case eGstreamerBackend:
                lang = gstLib_.AudioLang(i);
                break;
            case eExternalBackend:
                lang = extLib_.AudioLang(i);
                break;
            } // switch

            strn0cpy(aTrack.language,lang.c_str(), sizeof(aTrack.language));
            memset (aTrack.description,0,sizeof(aTrack.description));

            DEBUG_AUDIO_ID( "\033[0;44m  audio trackNr.  %2d.) lang: %s   \033[0m \n", i, aTrack.language);

            aviableAudioTracks_.push_back(aTrack);
        }// for

        if(old_count != aviableAudioTracks_.size())
            DeviceClrAvailableTracks();
        SetAvailableTracks();
#endif
    }

    //  track infos  to vdr
    void Player::SetAvailableTracks()
    {
        DEBUG_AUDIO_ID(" \033[0;45m  %s  found %d  audio tracks \033[0m\n",__PRETTY_FUNCTION__, aviableAudioTracks_.size());

        for (unsigned int i = 0;i< aviableAudioTracks_.size(); i++)
        {
            eTrackType ttype = ttDolby; // + i;
            aviableAudioTracks_[i].id = 0x80 + i;

            DEBUG_AUDIO_ID(" \033[0;45m register track  %d  lang: \"%s\" to\
                        vdr  \033[0m\n", i,  aviableAudioTracks_[i].language);

            DeviceSetAvailableTrack(ttype, i, aviableAudioTracks_[i].id,
                                    aviableAudioTracks_[i].language,
                                    aviableAudioTracks_[i].description);
        } // for
    }


     // called by vdr/menu.c   set chosen  audio track to xine Lib
     void Player::SetAudioTrack(eTrackType Type, const tTrackId *TrackId)
     {

#ifdef USE_PLAYPES
#else
         int currentAudioTrack = -1;

         switch (BackEnd()) {
            case eXineBackEnd:
                 currentAudioTrack = xineLib_.GetParamAudioChannelLogical();
                 break;

            case eGstreamerBackend:
                 currentAudioTrack = gstLib_.AudioStream();
                 break;
            case eExternalBackend:
                 currentAudioTrack = extLib_.AudioStream();
                 break;
         } // switch

         DEBUG_AUDIO_ID(" \033[0;48m %s  -- ttype %d TrackID %2X  current\
                 trackNum %d  !\033[0m\n", __PRETTY_FUNCTION__, Type,
                 TrackId->id, currentAudioTrack);

         //if (!TrackId) return;

         unsigned int idx = TrackId->id - 0x80;
         if (idx < 0 || idx >= aviableAudioTracks_.size())
         {
             DEBUG_AUDIO_ID(" \033[0;48m %s  ERRRROOR Have no Track Nr %d\
                              \033[0m\n", __PRETTY_FUNCTION__, idx );
             return;
         }
         idx--; // -1 = auto
         if (currentAudioTrack   != (int) idx)
         {
             if (currentAudioTrack >= 0)
                 DEBUG_AUDIO_ID(" \033[0;48 cXineMP::SetAudioTrack: change\
                         Track- id %d - l: %s  to track id %d lang  %s ! \033[0m \n",
                         currentAudioTrack , aviableAudioTracks_[currentAudioTrack].language,
                         idx,  aviableAudioTracks_[idx].language);
             else
                 DEBUG_AUDIO_ID("\033[0;48m cXineMp::SetAudioTrack: change\
                         Track- id AUTO - l: ?? to track id %d lang  %s !\033[0m \n",
                         idx,  aviableAudioTracks_[idx].language);
             switch (BackEnd()) {

                case eXineBackEnd:
                    xineLib_.SetAudioTrack(idx);
                    break;

                case eGstreamerBackend:
                    gstLib_.SetAudioStream((int)idx);
                    break;
                case eExternalBackend:
                    extLib_.SetAudioStream((int)idx);
                    break;
             } // switch
         }
         else
             DEBUG_AUDIO_ID(" Same TrackId  -- no need to change \n");
#endif
     }


    void Player::SetAllSpuTracks()
    {
        // TODO XXX GST
#ifdef USE_PLAYPES
#else

        switch (BackEnd()) {

        case eXineBackEnd:
        for (int i = 0;i< 32;i++)
        {
            char lang[XINE_LANG_MAX];
            if (xineLib_.GetSpuLangCode(lang,i) == 0)
                break;

            DEBUG_AUDIO_ID ( "\t \033[0;41m spu track %2d.)  lang \"%s\" --- \033[0m \n",i,  lang);
        }
        break;

        case eGstreamerBackend:
            //printf("%s not implmented for gst\n", __PRETTY_FUNCTION__);
            break;
        } // switch
#endif
    }


    void Player::HandleVdrKeys() THROWS
    {
        //    printf("++++++++ \033[7;94mPlayer::HandleVdrKeys()\033[0m %i ++++++++\n", KeyQueue.size());

        eKeys key = kNone;
        eOSState state = osUnknown;

        while(Running())
        {
            {
                cMutexLock lock(&mutexKeyQ);
                if (KeyQueue.empty())
                {
#ifndef USE_PLAYPES
                    switch(BackEnd()) {
                    case eXineBackEnd:
                        xineLib_.ProcessSeek();
                        break;
                    case eGstreamerBackend:
                        break;
                    }
#endif
                    return;
                }
                /* get key from key-queue*/
                key = KeyQueue.front();
                // remove the front element
                KeyQueue.pop();
            }
            //get return state
            state = ProcessKey_r(key);
            cMutexLock lock(&mutexReturnQ);
            //put it in queue
            returnQueue.push(state);
        } // while
    }


    void Player::HandleEvents() THROWS
    {
        //printf("++++++++ \033[7;94mPlayer::HandleEvents()\033[0m ++++++++\n");
#ifdef USE_PLAYPES
#else

        switch (BackEnd()) {

        case eXineBackEnd:
        for (;;)
        {
            //SetAvailableTracks(); No need to allways set tracks

            std::auto_ptr<XineEventBase> event(xineLib_.GetEvent()) ;
            if (event.get() == nullptr) break;

            if(event.get())
            {
                switch(event->type_)
                {
                    case XINE_EVENT_UI_NUM_BUTTONS:
                        {
                            DEBUG_XINE_EVENT ("\033[0;36m  --- XINE_EVENT_UI_NUM_BUTTONS  ---- menu buttons: %d  ---- \033[0m \n", static_cast<EventUINumButtons *>(event.get())->data_.num_buttons);
                            break;

                        }

                    case XINE_EVENT_UI_PLAYBACK_FINISHED:
                        {
                            // int current, total; unused vars
                            DEBUG_XINE_EVENT ( "\033[0;36m  --- %s : REEL_XINE_EVENT_UI_PLAYBACK_FINISHED \033[0m \n",__PRETTY_FUNCTION__ );
                            SetTitleInfoString();
                            SetAllAudioTracks();

                            DEBUG_XINE_EVENT("--------------xineLib_.GetStream() = %u, event->stream_ = %u------\n",
                                    (uint)(xineLib_.GetStream()), (uint)(event->stream_));

                            // printf("****** playback over *******\n");
                            player_status = 0;

                            if (xineLib_.GetStream() == event->stream_)
                            {
                                if (playlist_)
                                {
                                    PlayNextPlayListEntry(true);
                                }
                            }
                            break;
                        }

                    case XINE_EVENT_UI_SET_TITLE:
                        {
                            DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_UI_SET_TITLE  \033[0m \n",__PRETTY_FUNCTION__ );
                            // DVD Titles spoils Title
                            SetTitleInfoString();
                            DEBUG_XINE_EVENT ( "\033[0;36m    TITLE %s \033[0m\n", static_cast<EventUISetTitle *>(event.get())->data_.str );
                        }
                        break;

                    case XINE_EVENT_UI_CHANNELS_CHANGED:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_UI_CHANNELS_CHANGED  \033[0m \n",__PRETTY_FUNCTION__ );
                        SetAllAudioTracks();
                        SetTitleInfoString();
                        DEBUG_XINE_EVENT ( "\033[0;36m   break \033[0m\n");
                        break;

                    case XINE_EVENT_UI_MESSAGE:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_UI_MESSAGE  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_FRAME_FORMAT_CHANGE :
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_FRAME_FORMAT_CHANGE  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_AUDIO_LEVEL:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_AUDIO_LEVEL  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_QUIT:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_QUIT  \033[0m \n",__PRETTY_FUNCTION__ );
                        // ProcessKey(kMenu); clean up start FileManager
                        break;

                    case XINE_EVENT_PROGRESS:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_PROGRESS  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_MRL_REFERENCE:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_MRL_REFERENCE  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_SPU_BUTTON:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_SPU_BUTTON  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_DROPPED_FRAMES:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_DROPPED_FRAMES   \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    case XINE_EVENT_MRL_REFERENCE_EXT:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT_MRL_REFERENCE_EXT  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;

                    default:
                        DEBUG_XINE_EVENT ( "\033[0;36m --- %s : XINE_EVENT not handled  \033[0m \n",__PRETTY_FUNCTION__ );
                        break;
                } // switch
            } // if (event.get())
        }// for(;;)

        break;

        case eGstreamerBackend:
            // Handle all messages on GstBus
            gstLib_.HandleBusMessages();

            // play next if eos reached
            if (gstLib_.EoS() && playlist_)
            {
                PlayNextPlayListEntry(true);
            }

            // update possible changes in tags?
            if (gstLib_.AudioTagsChanged())
            {
                //std::cout << "got new audio tags" << std::endl;
                SetTitleInfoString();
                SetAllAudioTracks();
            }

            //std::cout<< "playback rate: " << gstLib_.PlaybackRate() << std::endl;
            break;
        case eExternalBackend:
            if(extLib_.HandleEvents()) {
                if(extLib_.HasError() && !mrlOpenError_) {
                    mrlOpenError_ = true;
                    cXineOsd::openError = tr("Could not play file");
                } // if
                SetAllAudioTracks();
                SetTitleInfoString();
            } // if
            if (extLib_.EoS() && playlist_ && !cXineOsd::openError.length())
                PlayNextPlayListEntry(true);
            break;

    } // switch

#endif
    }

    int Player::CurrentPlayNr() // Position in the playlist counted from 1...playlistEntries_.size()
        // 0 for error

    {
        return pl_pos;
    }
 /*  {
        uint pos=-1;
        for (pos = 0; pos < playlistEntries_.size(); pos++)
        {
            if(mrl_.find(playlistEntries_[pos]) != std::string::npos)
                break;
        }
    return pos;
   }
   */

  /*  std::string Player::GetNextPlayListEntryCyclic(bool up)
    {
        std::string currentEntry = mrl_;
        bool found = false;
        uint pos;
        for (pos = 0; pos < playlistEntries_.size(); pos++)
        {
            if(mrl_.find(playlistEntries_[pos]) != std::string::npos)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            return "";
        }
        if(!up)
        {
            if(pos == 0)
            {
                return playlistEntries_[playlistEntries_.size()-1];
            }
            else
            {
                return playlistEntries_[--pos];
            }
        }
        else
        {
            if(pos == playlistEntries_.size() - 1)
            {
                return playlistEntries_[0];
            }
            else
            {
                return playlistEntries_[++pos];
            }
        }
    }
    */

   std::string Player::GetNextPlayListEntryCyclic(bool up)
   {
       //        HERE;
       unsigned int next = pl_pos;
       //DEBUG_VAR(pl_pos);

       PlayerStatus.SetPlayerStatus(eGettingNext);

       // OSD Closed
       // jump in the OSD playlist : the original playlist
       if (!mrlOpenError_ && !cXineOsd::isOpen)
       {
           next = up? next + 1 : next - 1;

           if ( next < 0 ) // play from last
               next = playlistEntries_.size() -1 ; // got to last file
           else if ( next > playlistEntries_.size() -1 )
               next = 0 ; // got to first file

           pl_pos = next;
       }

       std::string next_mrl = up? GetNextFromTree( next ): GetPrevFromTree( next );

       if (next_mrl.size() > 0)
           return next_mrl;
#if 0
       else
           LOC; printf("nothing found in tree#%i, trying the next tree\n\n", next);
#endif

       do
       {

           next = up? next + 1 : next - 1;

           if ( next < 0 ) // play from last
               next = playlistEntries_.size() -1 ; // got to last file
           else if ( next > playlistEntries_.size() -1 )
               next = 0 ; // got to first file

           if ((int)next == pl_pos) // back again after looping through the playlist
           {
               esyslog("xinemediaplayer: (%s:%d) looped through the playlist,\
                 couldnot find a suitable file/url to play", __FILE__,__LINE__);

               return "";
           }

           next_mrl = up? GetNextFromTree( next ): GetPrevFromTree( next );

       } while ( next_mrl.size()<= 0  && NetworkErrorCount <= MAX_NW_ERRORS && Running());

       pl_pos = next;
       return next_mrl;
   }


    void Player::PlayNextPlayListEntry(bool up)
    {
        //printf("-------PlayNextPlayListEntry, %s ----\n", mrl_.c_str());
        bool detach = false;
        std::string nextMrl = mrl_;

        //Set Player Status : GETTING_NEXT_MRL
        PlayerStatus.SetPlayerStatus(eGettingNext);

        switch( play_mode )
        {
            case PLAY_RANDOM:
                {
                    unsigned int next_num = RandomNumberLessThan( playlistEntries_.size(), pl_pos);
                    //printf("RANDOM: playing %u/%u\n\n", next_num, playlistEntries_.size() );
                    PlayFileNumber(next_num );
                    return;
                }

            case PLAY_REPEAT_SINGLE:

                // TODO check if mrl was playable
                // play same mrl
                if (mrlOpenError_)
                {
                    // get next mrl
                }DEBUG_RESTARTPLAYBACK;
                ReStartPlayback();
                break;

            default:
                esyslog("(%s:%d) Invalid playlist. Playing in normal mode\n", __FILE__, __LINE__);
            case PLAY_REPEAT_PLAYLIST:
                // fall through play normal
            case PLAY_NORMAL:

                if(up)
                {
                    //if(!loopmode_ && mrl_ == playlistEntries_[playlistEntries_.size() - 1])
                    if(!loopmode_ && pl_pos == (int)playlistEntries_.size()-1 )
                    {
                        detach = true;
                    }
                    nextMrl = GetNextPlayListEntryCyclic(true);
                }
                else
                {
                    //if(!loopmode_ && mrl_ == playlistEntries_[0])
                    if(!loopmode_ && pl_pos ==  0 )
                    {
                        detach = true;
                    }
                    nextMrl = GetNextPlayListEntryCyclic(false);
                }
        } // switch ( play_mode )

        if(detach)
        {
            esyslog("[xinemediaplayer] (%s:%d) Detaching player", __FILE__, __LINE__);

            //printf("\033[1;92m Calling Detach() %s \033[0m\n",__PRETTY_FUNCTION__);
            //Detach();
            //printf("\033[1;91m After calling Detach() %s \033[0m\n",__PRETTY_FUNCTION__);
        }
        else
        {
            mrl_ =  nextMrl;
DEBUG_RESTARTPLAYBACK;
            if(Running())
                ReStartPlayback();
        }

        //printf("\033[1;92m Exiting %s \033[0m\n",__PRETTY_FUNCTION__);
    } // end PlayNextPlayListEntry()


   void Player::StartPlayback() THROWS
   {
       //HERE;
       //printf("++++++++ \033[7;94mPlayer::StartPlayback()\033[0m ++++++++\n");
       DEBUG_PLAYER(" Player::StartPlayback()\n");

#ifdef USE_PLAYPES
       esyslog("[xinemediaplayer] StartPlayback() : USE_PLAYPES\n");
       DELETENULL(ringbuffer);
       ringbuffer = new cRingBufferLinear(BUF_SIZE, BUF_MARGIN);
       ReadThread.Init("", ringbuffer, &Mutex);
       ReadThread.NewMrl(mrl_);
       player_status = 1;
       playing_ = true;
       return;
#else
       //ShowSplashScreen();  //now done by mediad

       // TODO XXX GST

       PlayerStatus.SetPlayerStatus(eNotPlaying);
       PlayerStatus.SetError(eNone);
       mrlOpenError_ = false;

           switch (BackEnd()) {

           case eXineBackEnd:

       xineLib_.Init();

       //xineLib_.PrintXineConfig();
       //xineLib_.PrintXinePlugins();
       //DEBUG_VAR(mrl_) ;
       mrlOpenError_ = false;
       try
       {
           if (mrl_.empty())
           {
               //printf("\033[0;91mempty mrl\033[0m\n");
               throw XineError("unable to open empty mrl");
           }
           // Set Player Status: OPENNING_MRL
           xineLib_.Open(mrl_.c_str()); //THROWS



           PlayerStatus.SetPlayerStatus(eOpened);
           if (std::string(InputPlugin()) == "http")
           {
               NetworkErrorCount=0;
           }
           if(xineLib_.HasVideo() && !xineLib_.VideoHandled()) {
               PlayerStatus.SetError(eOpenError);

               cXineOsd::openError = tr("VideoCodec not supported!");
               if(xineLib_.VideoCodec()) {
                   cXineOsd::openError += " ( ";
                   cXineOsd::openError += xineLib_.VideoCodec();
                   cXineOsd::openError += " )";
               } // if
               isyslog("[xinemedia] %s", cXineOsd::openError.c_str());

               if(!xineLib_.HasAudio() || !xineLib_.AudioHandled()) {
                   mrlOpenError_ = true;
                   //Set Error : OPEN ERROR
                   //Player Status NOT_PLAYING
                   PlayNextPlayListEntry(true);
                   return;
               } // if
           } else if(xineLib_.HasAudio() && !xineLib_.AudioHandled()) {
               PlayerStatus.SetError(eOpenError);

               cXineOsd::openError = tr("AudioCodec not supported!");
               if(xineLib_.AudioCodec()) {
                   cXineOsd::openError += " ( ";
                   cXineOsd::openError += xineLib_.AudioCodec();
                   cXineOsd::openError += " )";
               } // if
               isyslog("[xinemedia] %s", cXineOsd::openError.c_str());
               if(!xineLib_.HasVideo() || !xineLib_.VideoHandled()) {
                   mrlOpenError_ = true;
                   //Set Error : OPEN ERROR|CODEC_NOT_SUPPORTED
                   //Player Status NOT_PLAYING
                   PlayNextPlayListEntry(true);
                   return;
               } // if
           } else {
               cXineOsd::openError = "";
           }

           // debug
           if(useOld)
               DeviceFlush();
           xineLib_.Play(0, 0); //THROWS
           PlayerStatus.SetError(eNone);

           playing_ = true;
           player_status = 1;

           PlayerStatus.SetPlayerStatus(ePlaying);
       }

       catch (XineError const &ex)
       {
           LogXineError(ex);
           mrlOpenError_ = true;

           PlayerStatus.SetPlayerStatus(eNotPlaying);
           cXineOsd::openError = tr("Could not play file");

           //Set Error : OPEN ERROR
           //Player Status NOT_PLAYING


           //Skins.Message(mtError, tr("Could not play file"));
           /// log and silently play the next ;)
           player_status = 0;
           PlayerStatus.SetError(eOpenError);

           //if ((!mrl_.empty() && std::string(mrl_).find("http")!=std::string::npos )|| std::string(InputPlugin()) == "http")
           /* identify network error as, xine having http input plugin,
            * xine engine reporting 'unknown error' and a non-empty mrl
            *
            * this avoids "no demux plugin" and "no input plugin" errors
            * to be counted towards network unavailability.
            */
           if (xineLib_.XineGetErrorStr() == "unknown error" && !mrl_.empty() && std::string(InputPlugin()) == "http")
           {
               NetworkErrorCount++ ;

               //Set Error : NETWORK_OPEN_ERROR
               PlayerStatus.SetError(eNetworkOpenError);

               Skins.QueueMessage(mtError, tr("Network read error"), 2, -1);
               //printf("Player.c:%i Network Error\n", __LINE__);
           }


           if(NetworkErrorCount > MAX_NW_ERRORS)
           {
               if (curl_download("http://www.google.com", "/tmp/check_internet_connection.tmp"))
               {
                   // empty message to remove all previous messages from queue
                   Skins.QueueMessage(mtError, "");

                   // network okay, maybe one particular server was down. Try the rest.
                   Skins.QueueMessage(mtError, tr("Please wait, Loading..."),2);
                   PlayNextPlayListEntry(true);
               }
               else
               {
               Skins.QueueMessage(mtError, tr("Please check your network connection"),3);
               cCondWait::SleepMs(3000);
               //Detach();
               }
           }
           else
               PlayNextPlayListEntry(true);
       } //catch

           break;

           case eGstreamerBackend: {

               gstLib_.Init(true);
               std::string uri = make_gst_uri(mrl_.c_str());
               std::cout << "============= PlayMrl() >>>  " << uri << std::endl;
               gstLib_.PlayMrl(uri.c_str());
               std::cout << "after PlayMrl()" << uri << std::endl;

               playing_ = true;
               player_status = 1;
               PlayerStatus.SetPlayerStatus(ePlaying);


                break;
                }
           case eExternalBackend:
               if(extLib_.Play(mrl_.c_str())) {
                   playing_ = true;
                   player_status = 1;
                   PlayerStatus.SetPlayerStatus(ePlaying);
               } else {
                   playing_ = false;
                   player_status = 0;
                   PlayerStatus.SetError(eOpenError);
                   PlayerStatus.SetPlayerStatus(eNotPlaying);
               } // if
               break;
 } // switch
#endif
   } //end StartPlayback()

    void Player::ReStartPlayback() THROWS
    {
        if (!Running())
        {
            esyslog("[xinemediaplayer] (%s:%i) Player::ReStartPlayback() exiting since thread not running",
                    __FILE__, __LINE__);

            return;
        }

        DEBUG_PLAYER("DEBUG [xinemedia]:  Player::ReStartPlayback(): %s\n", mrl_.c_str());

#if 0
        printf("\033[0;91m DEBUG [xinemedia]: (threadid %i, ismainthread? %i)\
            Player::ReStartPlayback() pl_pos:%i %s\033[0m\n",
            ThreadId(), IsMainThread(), pl_pos, mrl_.c_str());
#endif

#ifdef USE_PLAYPES
        printf("Got new mrl. Calling DeviceClear()\n");
        dsyslog("--------- Got new mrl. DeviceClear() -----------");
        // discard all data related to the current mrl
        DeviceClear();
        PlayPes(NULL, 0, false);
        //the pass new mrl to cInputThread
        ReadThread.NewMrl(mrl_);
        dsyslog("[xinemediaplayer] ********** DeviceClear-ed *******, new mrl");
        return ;
#else
        mrlOpenError_ = false;
        // TODO XXX GST

        // Set Player Status: OPENNING_MRL

        switch (BackEnd()) {

        case eXineBackEnd:
        try
        {
            if (mrl_.empty())
            {
                printf("\033[0;91mempty mrl\033[0m\n");
                throw XineError("unable to open empty mrl");
            }

            //DEBUG_VAR(mrl_) ;

            xineLib_.Open(mrl_.c_str()); //THROWS

            PlayerStatus.SetPlayerStatus(eOpened);

            if (std::string(InputPlugin()) == "http")
            {
                NetworkErrorCount=0;
            }

            if(xineLib_.HasVideo() && !xineLib_.VideoHandled()) {
                PlayerStatus.SetError(eOpenError);
                cXineOsd::openError = tr("VideoCodec not supported!");
                if(xineLib_.VideoCodec()) {
                    cXineOsd::openError += " ( ";
                    cXineOsd::openError += xineLib_.VideoCodec();
                    cXineOsd::openError += " )";
                } // if
                isyslog("[xinemedia] %s", cXineOsd::openError.c_str());

                //Set Error : OPEN ERROR|CODEC_NOT_SUPPORTED
                //Player Status NOT_PLAYING

                if(!xineLib_.HasAudio() || !xineLib_.AudioHandled()) {
                    mrlOpenError_ = true;
                    PlayNextPlayListEntry(true);
                    return;
                } // if
            } else if(xineLib_.HasAudio() && !xineLib_.AudioHandled()) {
                PlayerStatus.SetError(eOpenError);
                cXineOsd::openError = tr("AudioCodec not supported!");
                if(xineLib_.AudioCodec()) {
                    cXineOsd::openError += " ( ";
                    cXineOsd::openError += xineLib_.AudioCodec();
                    cXineOsd::openError += " )";
                } // if
                isyslog("[xinemedia] %s", cXineOsd::openError.c_str());
                if(!xineLib_.HasVideo() || !xineLib_.VideoHandled()) {
                    mrlOpenError_ = true;
                    PlayNextPlayListEntry(true);
                    return;
                } // if
            } else {
                cXineOsd::openError = "";
            }

            if(useOld) {
                sleep(1); //dirty! may skip short videos without sleep...
                vxi->SetPlayMode(0);
            } // if


            xineLib_.Play(0, 0); //THROWS
            PlayerStatus.SetError(eNone);

            PlayerStatus.SetPlayerStatus(ePlaying);

            if(useOld)
                vxi->SetPlayMode(1);
            playing_ = true;
            player_status = 1;

            // display new file name/title
            SetTitleInfoString();
            std::string titleStr = GetTitleInfoString();

            if ( !cXineOsd::isOpen )
            {
                if (titleStr.size()>40) titleStr = titleStr.substr(40);
                Skins.QueueMessage(mtInfo, NULL); // remove all other messages (error messasges)
                Skins.QueueMessage(mtInfo, titleStr.c_str());
                //printf("(%s:%d) displayed %s\n", __FILE__, __LINE__, titleStr.c_str());
            }
            else
            {
                // reusing titleStr
                if (cXineOsd::k_blue == SHOUTCAST_PLUGIN )
                {
                    titleStr = tr("Opening ") + mrl_;
                    Skins.QueueMessage(mtInfo, NULL); // remove all other messages (error messasges)
                    Skins.QueueMessage(mtInfo, titleStr.c_str() );
                }
            }

        } // try

        catch (XineError const &ex)
        {
            LogXineError(ex);
            mrlOpenError_ = true;
            PlayerStatus.SetPlayerStatus(eNotPlaying);
            cXineOsd::openError = tr("Could not play file");
            //Skins.Message(mtError, tr("Could not play file"));
            player_status = 0;

          //if ((!mrl_.empty() && std::string(mrl_).find("http")!=std::string::npos )|| std::string(InputPlugin()) == "http")


           /* identify network error as, xine having http input plugin,
            * xine engine reporting 'unknown error' and a non-empty mrl
            *
            * this avoids "no demux plugin" and "no input plugin" errors
            * to be counted towards network unavailability.
            */
           if (xineLib_.XineGetErrorStr() == "unknown error" && !mrl_.empty() && std::string(InputPlugin()) == "http")
            {
                NetworkErrorCount++;

                //Set Error : NETWORK_OPEN_ERROR
                PlayerStatus.SetError(eNetworkOpenError);

                Skins.QueueMessage(mtError, tr("Network read error"), 2,-1);
                //printf("------------- Player.c:%i Network Error ------------\n\n", __LINE__);
            }

            if(NetworkErrorCount > MAX_NW_ERRORS)
            {
               if (curl_download("http://www.google.com", "/tmp/check_internet_connection.tmp"))
               {
                   // empty message to remove all previous messages from queue
                   Skins.QueueMessage(mtError, NULL);

                   // network okay, maybe one particular server was down. Try the rest.
                   Skins.QueueMessage(mtError, tr("Please wait, Loading..."), 2);
                   PlayNextPlayListEntry(true);
               }
               else
               {
                //printf("message shown\n\n");
                // empty message to remove all previous messages from queue
                Skins.QueueMessage(mtError, NULL);
                Skins.QueueMessage(mtError, tr("Please check your network connection"),3);
                cCondWait::SleepMs(3000);
                //Detach();
               }
            }
            else
                PlayNextPlayListEntry(true);
        } //catch

        break;

        case eGstreamerBackend: {

            std::string uri = make_gst_uri(mrl_.c_str());
            gstLib_.PlayMrl(uri.c_str());
            std::cout << "after PlayMrl()" << uri << std::endl;
            player_status = 1;
            PlayerStatus.SetPlayerStatus(ePlaying);
        {
            /// TODO remove ugly duplication of code !

            // display new file name/title
            SetTitleInfoString();
            std::string titleStr = GetTitleInfoString();

            if ( !cXineOsd::isOpen )
            {
                if (titleStr.size()>40) titleStr = titleStr.substr(40);
                Skins.QueueMessage(mtInfo, NULL); // remove all other messages (error messasges)
                Skins.QueueMessage(mtInfo, titleStr.c_str());
                //printf("(%s:%d) displayed %s\n", __FILE__, __LINE__, titleStr.c_str());
            }
            else
            {
                // reusing titleStr
                if (cXineOsd::k_blue == SHOUTCAST_PLUGIN )
                {
                    titleStr = tr("Opening ") + mrl_;
                    Skins.QueueMessage(mtInfo, NULL); // remove all other messages (error messasges)
                    Skins.QueueMessage(mtInfo, titleStr.c_str() );
                }
            }
        }

            break;
            }
        case eExternalBackend:
            if(extLib_.Play(mrl_.c_str())) {
                playing_ = true;
                player_status = 1;
                PlayerStatus.SetPlayerStatus(ePlaying);
            } else {
                playing_ = false;
                player_status = 0;
                PlayerStatus.SetError(eOpenError);
                PlayerStatus.SetPlayerStatus(eNotPlaying);
            } // if
            break;

        }// switch

#endif
    } //end ReStartPlayback()

   void Player::StopPlayback() NO_THROW
   {
       //HERE;
       DEBUG_PLAYER("DEBUG [xinemedia]: Player::StopPlayback()\n");
       player_status = 0; // not playing anymore
       mrl_.clear();
#ifdef USE_PLAYPES
       printf("Stopping ReadThread\n");
       ReadThread.StopThread();

       playing_ = false;

       DeviceClear();
       PlayPes(NULL, 0, false);

       return ;
#else


       /*sometimes this function is not called when exiting,
        * for example when xine is not playing and Player is deleted
        *
        * So, do the call back in Player::~Player()
        */

#if 0
       if(playlist_ && cXineOsd::k_blue == FILEBROWSER_PLUGIN)
       {
           CallFilebrowser(xine_exit);
           /*
              struct
              {
              char const *entry;
              }fileBrowserSetPlayListEntry  =
              {
              mrl_.c_str()
              };
              printf ("----restart filebrowser %s---\n", mrl_.c_str());
              cPluginManager::CallAllServices("Filebrowser set playlist entry", &fileBrowserSetPlayListEntry);
              */
       }
       else if ( cXineOsd::k_blue == SHOUTCAST_PLUGIN)
           cRemote::CallPlugin("shoutcast");
       else if ( cXineOsd::k_blue == MMFILES_PLUGIN)
           cRemote::CallPlugin("mmfiles");
       else if ( cXineOsd::k_blue == YOUTUBE_PLUGIN)
           cRemote::CallPlugin("youtube");
#endif

       //Only call Exit with destructor        xineLib_.Exit();
       // Cancel(3);

       switch (BackEnd()) {
            case eXineBackEnd:
                xineLib_.Close();
                break;

            case eGstreamerBackend:
                gstLib_.Stop();
                break;
            case eExternalBackend:
                extLib_.Stop();
                break;
       } // switch

       playing_ = false;
#endif
   }

#ifdef USE_PLAYPES
     void Player::SetPlayback(int pos) {};
#else
    void Player::SetPlayback(int pos) {
       try {
           if(!Playing()) return;

           switch (BackEnd()) {
                case eXineBackEnd:
                    xineLib_.Play(0, pos); //THROWS
                    break;

                case eGstreamerBackend:
                    // pos is in milliseconds. SeekAbsolute() requires seconds
                    gstLib_.SeekAbsolute(pos/1000);
                    break;
                case eExternalBackend:
                    extLib_.Seek(pos);
                    break;
           }

       } catch(...) {}
    } // Player::SetPlayback
#endif

   bool Player::IsReplayingMusic()
   {
#ifdef USE_PLAYPES
       return true;
#else
       switch(BackEnd())
       {
       case eXineBackEnd: {
       if (!Playing())
       {
           return false;
       }

       bool isMusic = false;

       if ( ( std::string(InputPlugin()) == "cdda" ||
              std::string(InputPlugin()) == "file" ||
              std::string(InputPlugin())== "http") && HasAudio())
       {
           isMusic = true;
       }

       if (HasVideo())
       {
           isMusic = false;
       }

       return isMusic;
       }

       case eGstreamerBackend:

            return Playing()
                    && !IsReplayingVideo()
                    && gstLib_.AudioStream() >= 0 ;
       case eExternalBackend:
            return Playing()
                    && !IsReplayingVideo()
                    && extLib_.AudioStream() >= 0 ;
       } // switch
#endif
       return true;
   }

   bool Player::IsReplayingDvd()
   {
#ifdef USE_PLAYPES
       return false;
#else
       switch(BackEnd())
       {
       case eXineBackEnd: {
       if (!Playing())
       {
           return false;
       }

       bool isDvd = false;
       if (std::string(InputPlugin()) == "DVD")
       {
           isDvd = true;
       }
       return isDvd;
       }

       case eGstreamerBackend:
          // std::cout << "Source Factor name: " << gstLib_.SourceFactoryName()
          //           << " Isreplaying DVD?: "<< (gstLib_.SourceFactoryName() == "rsndvdbin") << std::endl;
           return gstLib_.SourceFactoryName() == "rsndvdbin"; // or look at uri?
       case eExternalBackend:
           return extLib_.IsDVD();

    } // switch

#endif
       return false;
   }

   bool Player::IsReplayingVideo()
   {
#ifdef USE_PLAYPES
       return false;
#else
       switch(BackEnd()) {

       case eXineBackEnd: {
       if (!Playing())
       {
           return false;
       }

       bool isVideo = false;
       if (HasVideo() && !IsReplayingDvd())
       {
           isVideo = true;
       }

       return isVideo;
        }

       case eGstreamerBackend:

            return Playing()
                    && !IsReplayingDvd()
                    && gstLib_.VideoStream() >= 0;
       case eExternalBackend:
            return Playing()
                    && !IsReplayingDvd()
                    && extLib_.VideoStream() >= 0;
   }// switch
#endif
       return false;
   }

   void Player::SetTitleInfoString()
   {
       DEBUG_PLAYER("DEBUG [xinemedia]: Player::SetTitleInfoString()\n");

       titleInfo_.clear();
#ifdef USE_PLAYPES
       titleInfo_ = "PlayPES";
#else
       char titleinfo[256];
       titleinfo[0] = 0;

       if(IsReplayingDvd())
       {
           if (PlayerIsInMenuDomain())
           {
               sprintf(titleinfo, "DVD Menu");
           }
           else
           {
               int titleNumber=-1, titleCount=-1, chapterNumber=-1, chapterCount=-1;
               //int num_angle = -1, cur_angle = -1;

               titleNumber = DvdTitleNumber();
               titleCount = DvdTitleCount();
               chapterNumber = DvdChapterNumber();
               chapterCount = DvdChapterCount();

               //more detailed information
               /*if (titleCount >= 1)
                 {
               //no menu here
               //Reflect angle info if appropriate
               num_angle = DvdAngleNumber();
               cur_angle = DvdAngleCount();
               }

               if (XineSetup_Display >=1)
               {
               if(num_angle>1)
               {
               sprintf(titleinfo, "dvd: %d/%d %d/%d %d/%d",
               titleNumber, titleCount,  chapterNumber, chapterCount,
               cur_angle, num_angle);
               }
               else
               {
               sprintf(titleinfo, "dvd: %d/%d %d/%d",
               titleNumber, titleCount,  chapterNumber, chapterCount);
               }
               }*/
               if(GetTitle())
               {
                   snprintf(titleinfo, 255, "%s %s %d" ,GetTitle(), tr("Chapter"), chapterNumber);
               }
           }
       }// if (IsReplayingDvd())
       else if(IsReplayingMusic())
       {
           if(GetTitle())
           {
               //snprintf(titleinfo, 255, "%s %s %s Track %s" ,GetTitle(), Artist(), Album(), TrackNumber());
               snprintf(titleinfo, 255, "%s" ,GetTitle());
           }
           //else //use mrl
           //no else, since xinelib may return empty strings instead of NULL!
           if (string(titleinfo).empty())
           {
               snprintf(titleinfo, 255, "%s" , GetFileName(GetMrl()).c_str());
           }
       }
       else
       {
           if(GetTitle())
           {
               snprintf(titleinfo, 255, "%s" ,GetTitle());
           }
           //else //use mrl
           //no else, since xinelib may return empty strings instead of NULL!
           if (string(titleinfo).empty())
           {
               snprintf(titleinfo, 255, "%s" , GetFileName(GetMrl()).c_str());
           }
       }

       if (string(titleinfo) != "")  // XXX !!
       {
           titleInfo_ = titleinfo;
       }
       else
       {
           titleInfo_ = "";// leave titleInfo_ empty // XXX !!
       }
#endif
   } // end SetTitleInfoString()

}//namespace
}//namespace
