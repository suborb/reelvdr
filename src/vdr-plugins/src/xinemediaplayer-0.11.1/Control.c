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

// Control.c
#include <memory>

#include <vdr/status.h>
#include <vdr/remote.h>
#ifdef REELVDR
#include <vdr/reelboxbase.h>
#endif

#include "Player.h"
#include "Control.h"
#ifndef USE_PLAYPES
#include "XineEvent.h"
#include <xine.h> // ??
#endif
#include "xineOsd.h"

#include "timecounter.h"

#define MODETIMEOUT   4 // seconds

//#define DEBUG_OSDCTRL(format, args...) printf (format, ## args)
#define DEBUG_OSDCTRL(format, args...) 

//#define DEBUG_XINE_EVENT(format, args...) printf (format, ## args)
#define DEBUG_XINE_EVENT(format, args...)

//#define DEBUG_KEYS(format, args...) printf (format, ## args)
#define DEBUG_KEYS(format, args...)

//#define DEBUG_AUDIO_ID(format, args...) printf (format, ## args)
#define DEBUG_AUDIO_ID(format, args...)

using std::string; 

// TODO  Xine Setup
//DL Not used!
//static  int XineSetup_Display = 0;  // TODO see also Player.c
                       //0;  // short
                      // 1; // medium
                      // 2; // long

namespace Reel
{
namespace XineMediaplayer
{ 
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Control
    bool global_sent_k3 = false;
    Control::Control(char const *mrl, bool playlist, std::vector<std::string> playlistEntries) NO_THROW
    :    ::cControl(new Player(mrl, playlist, playlistEntries))
    {
        DEBUG_OSDCTRL (" --- %s -- new Player- \n", __PRETTY_FUNCTION__);
        player_ = static_cast<Player *>(player); 
        //::cStatus::MsgReplaying(this, mrl, NULL, true); has no effect!
        printf("Control::Control, vor cStatus::MsgOsdClear()\n");
        cStatus::MsgOsdClear();

        displayReplay_ = NULL;
#if APIVERSNUM >= 10408
        SetNeedsFastResponse(true);
#else
    needsFastResponse = true;
#endif
        visible_ = modeOnly_ = shown_ = displayFrames_ = false;
        lastPlay_ = 1;
        lastForward_ = 1;
        lastSpeed_ = -1;
        osdTaker_ = NULL; // test
        title_ = NULL;
        timeoutShow_ = 0;
        lastTitleBuffer_ = ""; 
        timeSearchActive = false;

    cXineOsd::isPlaying = true ;
    global_sent_k3 = false;
    }

    Control::~Control() NO_THROW
    {
        DEBUG_OSDCTRL (" --- %s --- \n", __PRETTY_FUNCTION__);
        Hide(); 
        Stop();

    cXineOsd::ClearStaticVariables();
    }

    void Control::PlayerPositionInfo(int &plNr, int &curr, int &total) // show the position on the playlist AND position of the file being played
    {
        // avoid getting stuck when xinelib is not "playing"
        if ( !player_->player_status || !cXineOsd::isPlaying ) return;
        int Current, Total;
        //printf("Control.c:%i Calling Player::GetIndex()\n", __LINE__);
        if (player_ && player_->GetIndex(Current, Total) && Total>0)
        {
            lastCurrent_ = Current;
            lastTotal_   = Total;
            curr = Current;
            total = Total;
        }
        else
        { // error from XineLib; stream not seekable
#if VDRVERSNUM < 10704
         curr = FRAMESPERSEC * timeCounter.Seconds();
#else
         curr = DEFAULTFRAMESPERSECOND * timeCounter.Seconds();
#endif
             if (!player_) 
                 curr =0;
             total = 0;
             //printf("%lu \n", timeCounter.Seconds() );
        }
       
        plNr = player_->CurrentPlayNr();
    }

    bool Control::IsPlayingVideo()
    {
      return player_ && player_->IsReplayingVideo();
    }
    bool Control::HasActiveVideo() {
      if(!player_) return false;
      return player_->HasVideo() && player_->VideoHandled() && !player_->IgnoreVideo();
    }

    bool Control::IsPaused()
    {
      return player_ && player_->IsPaused();
    }

    void Control::SetPlayList( std::vector<std::string> pl)
    {
        player_->ReplacePlayList(pl);
    }
    
    void Control::StopPlayback()
    {
        if (player_) player_->StopPlayback();
    }

    void Control::SetPlayMode(int mode)
    {
      player_->SetPlayMode(mode);
    }
    
    int Control::GetPlayMode()
    {
      return player_->GetPlayMode();
    }
    ///////////////////////////////////////////////////////////////////////////
    // Inline Control 
    inline void Control::Hide() NO_THROW
    {
        TakeOsd((void *) -1);
        
        if (player_)
          player_->HideSPU();
        HideOsd();
    }
    
    cOsdObject* Control::GetInfo()
        /*
         * this function is called for getting the information of the currently playing file.
         */
    {
        if (player_)
            return new cFileInfoMenu( player_->GetMrl().c_str() );
        else 
            return NULL;
    }

    bool Control::PlayFileNumber(int n)
    {
        return  player_->PlayFileNumber(n);
    }

    void Control::TimeSearchDisplay(void) {
      char buf[64];
      strcpy(buf, tr("Jump: "));
      int len = strlen(buf);
      char h10 = '0' + (timeSearchTime >> 24);
      char h1  = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
      char m10 = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
      char m1  = '0' + (timeSearchTime & 0x000000FF);
      char ch10 = timeSearchPos > 3 ? h10 : '-';
      char ch1  = timeSearchPos > 2 ? h1  : '-';
      char cm10 = timeSearchPos > 1 ? m10 : '-';
      char cm1  = timeSearchPos > 0 ? m1  : '-';
      sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
      if(displayReplay_) displayReplay_->SetJump(buf);
    } // Control::TimeSearchDisplay

    void Control::TimeSearchProcess(eKeys Key) {
#define STAY_SECONDS_OFF_END 10
      int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;


#if VDRVERSNUM < 10704
      int Current = (lastCurrent_ / FRAMESPERSEC);
      int Total = (lastTotal_ / FRAMESPERSEC);
#else
      int Current = (lastCurrent_ / DEFAULTFRAMESPERSECOND);
      int Total = (lastTotal_ / DEFAULTFRAMESPERSECOND);
#endif
      switch (Key) {
        case k0 ... k9:
          if (timeSearchPos < 4) {
            timeSearchTime <<= 8;
            timeSearchTime |= Key - k0;
            timeSearchPos++;
            TimeSearchDisplay();
          } // if
          break;
        case kFastRew:
        case kLeft:
        case kFastFwd:
        case kRight:
          Seconds = (Key == kRight || Key == kFastFwd) ? Current+Seconds : Current-Seconds;
          // no break
        case kPlay:
        case kUp:
        case kPause:
        case kDown:
        case kOk:
          Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
          if(Seconds < 0) Seconds = 0;
          if(player_) player_->SetPlayback(Seconds*1000);
          timeSearchActive = false;
          break;
        default:
          timeSearchActive = false;
          break;
      } // switch
      if (!timeSearchActive) {
        if (timeSearchHide)
          Hide();
        else if (displayReplay_)
          displayReplay_->SetJump(NULL);
        ShowMode();
      } // if
    } // Control::TimeSearchProcess

    void Control::TimeSearch(void) {
      timeSearchTime = timeSearchPos = 0;
      timeSearchHide = false;
      if (modeOnly_)
         Hide();
      if (!visible_) {
         Show();
         if (visible_)
            timeSearchHide = true;
         else
            return;
      } // if
      timeoutShow_ = 0;
      TimeSearchDisplay();
      timeSearchActive = true;
    } // Control::TimeSearch

    eOSState Control::ProcessKey(eKeys key) NO_THROW
    {
      if(key != kNone) lastTitleBuffer_=""; // Graphlcd sometimes loose the replay status
      if (timeSearchActive && key != kNone) {
        TimeSearchProcess(key);
        return osContinue;
      }
      // get OSD again
      if (!player_->IsReplayingDvd() && key == kOk)
      {
         cXineOsd::firstOpen=false;
         cRemote::CallPlugin("xinemediaplayer");
         return osContinue;
      }
      try
      {
        return ProcessKey2(key);
      }
      catch (...)
      {
        Hide();
        return osEnd;
      }
      return osContinue;
    }

    eOSState Control::ProcessKey2(eKeys key) THROWS
    {
      eOSState state = cOsdObject::ProcessKey(key);
      if (player_) {
        // player_->HandleEvents(); // called by player thread
        if(useOld)
          player_->ProcessSpu();
        player_->ProcessKey(key);  
      } // if
      UpdateShow();
      state = osContinue; 
      //is inside a dvd menu or not replaying a dvd
      if (player_  && player_->PlayerIsInMenuDomain()) {
        //DEBUG_OSDCTRL("DVD-Ctrl:   PlayerIsInMenuDomain \n");
        //HideOsd();  // disable by ar (mp3 close osd)
        //printf("ProcessKey: InMenuDomain()\n");
        switch (key) {
          case kBack: 
          case kStop: 
            Hide();
            player_->Activate(false);
            printf("\033[7;91m Got kBack||kStop. Returning osEnd\033[0m\n");
            return osEnd;
          default: 
            break;
        } // switch
      } // if
      //is not inside a dvd menu 
      if (player_  && !(player_->IsReplayingDvd() && player_->PlayerIsInMenuDomain())) { 
        //printf("ProcessKey: NOT InMenuDomain()\n");
        switch (key) {
          case kOk:
            if (visible_ && !modeOnly_) {
              DEBUG_OSDCTRL("DVD-Ctrl:  kOk ---    (visible_  && !modeOnly_) call Hide \n");
              Hide();
            } else {// !vissible  || (vissible && modeOnly== 
              DEBUG_OSDCTRL("DVD-Ctrl:  kOk ---    !visible_ || (visible && modeOnly_) call Show() v:%d mo:%d  \n",visible_,modeOnly_);
              Show();
            } // if
            break;
          case kBack:  
            printf("\033[7;91m Got kBack\033[0m\n");
            if (visible_ ) {
              DEBUG_OSDCTRL("DVD-Ctrl:  kBack ---    visible ==  true  && !modeOnly_  call Hide \n"); 
              Hide();
            } else {
              DEBUG_OSDCTRL("DVD-Ctrl:  kBack ---  NOT visible Call Put(k3) (dvd main menu)  \n"); 
              /*not playing Dvd, kBack ends player*/
              if (!player_->IsReplayingDvd()) {
                printf("Sending osEnd\n");
                player_->Activate(false);
                return osEnd;
              }
              /* playing Dvd, kBack should call DVD-Menu (if it is present)
               * if no DVD-Menu xine starts playing from the beginning!
               *
               * So, for want of an elegant solution: here is a hack:
               *
               * first kBack we ask xine to show DVD-Menu(global_sent_k3), 
               * and see if DVD-menu was shown (player_->wasInMenu_)
               *
               * the second kBack checks if the DVD-Menu was shown, if not conclude 
               * this DVD has no menus and therefore end replay by osEnd
               */
              if ( player_->wasInMenu_ || (!global_sent_k3 && !player_->wasInMenu_)) {
                DEBUG_OSDCTRL("DVD-Ctrl:  Call Put(k3) (dvd main menu)  \n"); 
                cRemote::Put(k3);  // fixme: k3 is mapped to DVD main menu 
                global_sent_k3 = true;
              } else {
                /*playing Dvd and no menu was found, exit*/ 
                printf("***** Playing DVD: tried to call DVD-Menu using kBack once. ******\
                      \n****** No DVD-Menu was opened. So now, second kBack exits. ******\n");
                player_->Activate(false);
                return osEnd;
              } // if
            } // if
            break;
          case kStop:
            DEBUG_OSDCTRL("DVD-Ctrl:  kStop ---    visible ==  true  && !modeOnly_  call Hide \n");
            Hide();
            printf("\033[7;91m Got kStop: sending osEnd\033[0m\n");
            player_->Activate(false);
            return osEnd;    
#ifndef USE_PLAYPES
          case kRed:
            if(player_ && player_->Playing()) TimeSearch(); 
            return osContinue;
#endif
          case k3:
            DEBUG_OSDCTRL("DVD-Ctrl:  k3 ---  (dvd main menu)\n");
            /* Hide replayOSD when menu is called
             * replay Osd breaks when menu is called, since the titlebar is 
             * not update it is missing after menu
             */
            if(visible_) {
              Hide(); 
              printf("Called menu => replayOSD now hidden \n");
            }
          default: {
            eOSState state1 = cControl::ProcessKey(key);
            //printf("cControl::ProcessKey() returned %i ---------------\n", state);

            // do not return osUnknown
            if (state1 != osUnknown)
                state = state1;

            return state;
          } // default
        } // switch
      } // if
      //printf("ProcessKey: returning %i\n", state);
      return state;
    } // Control::ProcessKey2

    void Control::Stop() NO_THROW
    {
        printf("Control::Stop(), vor cStatus::MsgReplaying\n");
        ::cStatus::MsgReplaying(this, NULL, NULL, false);
    cXineOsd::isPlaying = false;
        delete player_;
        player_ = 0;
        player = 0; //?
    }

   void Control::OsdClose()
   {
       DEBUG_OSDCTRL (" ------ %s ---- \n", __PRETTY_FUNCTION__);
       DELETENULL(displayReplay_);
       visible_=false;
   }
  
   void Control::OsdOpen(void)
   {
       DEBUG_OSDCTRL("\033[0;48m DVD-Ctrl: ++++++++++++++++++++ [ \033[31;0m  %s \033[0;48m ] +++++++++++++++++++++++++++++  \033[0m\n", __PRETTY_FUNCTION__);
       if (visible_) {
           DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: allready visible\n");
           return;
       }
   
       if (OsdTaken(this))  // control has OSD 
       {
           if (player_) 
           {
               DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: has player -> TakeSPU ... \n");
               player_->HideSPU();
           }
       }

       if (TakeOsd((void *)this)) 
       {
           DEBUG_OSDCTRL("\033[0;48m DVD-Ctrl: ++++++++++++++++++++ [OsdOpen: SET  visible_ \"true\"] +++++++++++++++++++++++++++++  \033[0m\n");
           visible_= true; ///< XXX true  1.)
           displayReplay_ = Skins.Current()->DisplayReplay(modeOnly_);
       } 
       else 
       {
           DEBUG_OSDCTRL("\033[0;48m DVD-Ctrl: ------------ OsdOpen: set visible \"false\"  reason: TakeOsd() returns false  \033[0m\n");
           visible_ = false;
       }
   }

   void Control::HideOsd(void)
   {

      if (visible_) 
      {
          DEBUG_OSDCTRL (" --- %s --- \n", __PRETTY_FUNCTION__);
          OsdClose();
#if APIVERSNUM >= 10408
          visible_ = false;
          SetNeedsFastResponse(visible_);
#else
          needsFastResponse = visible_ = false;
#endif
          modeOnly_ = false;
          timeoutShow_ = 0;
      }
   }

   bool Control::TakeOsd(void *owner)
   {
       DEBUG_OSDCTRL (" --- %s --- \n", __PRETTY_FUNCTION__);
       if (owner== (void *) -1 || !OsdTaken(owner))
       {
           DEBUG_OSDCTRL("DVD-Ctrl: TakeOsd(%p) new owner!\n", owner);
           osdTaker_=owner;
           return true;
         } 
         else 
         {
             DEBUG_OSDCTRL("DVD-Ctrl: TakeOsd(%p) not taken!\n", owner);
         }
         return false;
   }

   bool Control::OsdTaken(void *me)
   {
       if (osdTaker_ && !cOsd::IsOpen())
       {
          osdTaker_=NULL; // update info ..
       }
       DEBUG_OSDCTRL("DVD-Ctrl: OsdTaken(%p) != %p := %d \n", me, osdTaker_, osdTaker_!=me);
       return osdTaker_ != NULL && osdTaker_ != me;
   }

   bool Control::OsdVisible(void *me)
   {
       DEBUG_OSDCTRL (" --- %s --- \n", __PRETTY_FUNCTION__);
       return visible_ || OsdTaken(me);
   }

   void Control::Show() 
   {
       //int Seconds = 0;
       DEBUG_OSDCTRL("DVD-Ctrl: l.268 Show()  modeOnly_? %s   visible_ %s \n", modeOnly_?"true":"false", visible_?"true":"false");
       if (modeOnly_)
       {
           Hide();
       }

       if (!visible_) 
       {
           DEBUG_OSDCTRL("DVD-Ctrl: l.276 Show()  modeOnly_? %s   visible_ %s \n", modeOnly_?"true":"false", visible_?"true":"false");
           shown_ = ShowProgress(true);
       }
   }

   void Control::ShowMode()
   {
       if (Setup.ShowReplayMode)
       {
           //DEBUG_OSDCTRL("\033[0;46 DVD-Ctrl: ShowMode: visible=%d, modeOnly=%d \033[0m\n", visible_, modeOnly_);
           bool Play, Forward;
           int Speed = 0;


           if (GetReplayMode(Play, Forward, Speed))
           {
               //DEBUG_OSDCTRL("\033[0;46 DVD-Ctrl: !!!!!!!! Play %d - lastPlay: %d  Speed %d visible  %s  !!!!!!!!!!!!!!!!!!!!!!!!! \033[0m\n", Play, lastPlay_, Speed, visible_?"true":"false");

               if (/*rule out other osds*/ !cOsd::IsOpen() && !visible_ &&  ( Play != lastPlay_ || lastForward_ != Forward || lastSpeed_ != Speed))
               {
                   DEBUG_OSDCTRL("\033[0;46 DVD-Ctrl: !!!!!!!! Playmode changed   !!!!!!!!!!!!!!!!!!!!!!!!! \033[0m\n");
                   DEBUG_OSDCTRL("\033[0;46 DVD-Ctrl: !!!!!!!! set modOnly \"true\"  - call  OsdOpen() Play: %d, FWD: %d Speed %d !!\033[0m\n", Play, Forward, Speed);
                   modeOnly_ = true; // open small playmode display
                   OsdOpen(); 
               }
               if (!visible_)
                  return; // could not openOsd 

               if (modeOnly_ && !timeoutShow_ && !(Play == 0 && Speed == -1))
                  timeoutShow_ = time(NULL) + MODETIMEOUT; /// set timeout; keeps  pause icon open 

               displayReplay_->SetMode(Play, Forward, Speed);
               lastPlay_ = Play;
               lastForward_ = Forward;
               lastSpeed_ = Speed;
               DEBUG_OSDCTRL("DVD-Ctrl: DisplayReplay SetMode .. time_out %ld  return setMode > >>  \n", timeoutShow_);
           }
           else 
               DEBUG_OSDCTRL("DVD-Ctrl:  -------------------- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Fehler !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  \n");

       }
   }

   void Control::UpdateShow() // Controls statusMsg & ReplayMode & 
   {
       if (visible_)
       {
           if (timeoutShow_ && time(NULL) > timeoutShow_)
           {
               DEBUG_OSDCTRL ("\033[0;45m DVD-Ctrl UpdateShow()---- timeoutShow_ = 1 timeout (%ld)abgelaufen   --- Call Hide() ... ShowMode \033[0m\n", timeoutShow_);
               HideOsd();
               ShowMode();
               return;
           }
           if (modeOnly_) 
           {
               DEBUG_OSDCTRL ("\033[0;45m DVD-Ctrl UpdateShow()---- modeOnly_ = true  ShowMode \033[0m\n");
               ShowMode();
           }
           else 
           {
               shown_ = ShowProgress(!shown_) || shown_;
               if (shown_) 
               {
                   DEBUG_OSDCTRL ("\033[0;45m DVD-Ctrl UpdateShow()  ---- !modeOnly_  toggle shown_  \033[0m\n");
               }
           }
       }
       else if (player_ && player_->Playing()) //FIXME: hotfix to avoid crashes during shutdown, when xine has already been closed 
       { 
           string titleBuffer = player_->GetTitleInfoString();
       static int dummy;
       /* Graphlcd plugin : ignores MsgReplaying() calls if 
        * control->GetIndex() is false, so wait till it is true to call it*/
           if (lastTitleBuffer_ != titleBuffer && GetIndex(dummy, dummy, false))
           {
                lastTitleBuffer_ = titleBuffer;
                printf("Control::UpdateShow(), vor cStatus::MsgReplaying\n");
                cStatus::MsgReplaying(this, GetDisplayHeaderLine().c_str(), NULL, true);
           }
       else 
       {
        int dummy; if (0)
        printf("%i (%i == %i?): lastTitleBuffer = '%s', GetIndex() %i \t '%s'\n", 
            lastTitleBuffer_ != titleBuffer, 
            lastTitleBuffer_.size(), titleBuffer.size(),
            lastTitleBuffer_.c_str(),  
            GetIndex(dummy, dummy, false),
            GetDisplayHeaderLine().c_str()
            );
       }
           
           if (!cOsd::IsOpen()) // if no other osd is open then show modes
               ShowMode();
       }
   }


   bool Control::ShowProgress(bool Initial)
   {
      //return true;
      DEBUG_OSDCTRL("\033[0;44m DVD-Ctrl: ------------ %s  ; Intit:%d ------------- \033[0m\n", __PRETTY_FUNCTION__,Initial);

      int Current, Total;
      string titleBuffer;

      if (player_->GetIndex(Current, Total) && Total > 0) 
      {
         //A 
         if (!visible_)
         {
            DEBUG_OSDCTRL ("\033[30;44m  --- %s -- Not visible --  title %s -- \033[0m\n", __PRETTY_FUNCTION__, title_);
#if APIVERSNUM >= 10408
            SetNeedsFastResponse(true);
#else
            needsFastResponse = true;
#endif
            modeOnly_ = false; //PP
            OsdOpen(); // mach auf
         }

         //B
         if (!visible_)
         {
            return false;
         }
         
         //C
         if (Initial) 
         {
            DEBUG_OSDCTRL ("\033[30;48m  --- %s --!!!!!!!!!!!!!!!!!!  INITIAL !!!!!!!!!!!!!!!!!!!! %s -- \033[0m\n", __PRETTY_FUNCTION__, title_);
            lastCurrent_ = lastTotal_ = -1;
            displayReplay_->SetTitle("unknown title"); 
             printf("Control::ShowProgress, vor cStatus::MsgReplaying\n");
            cStatus::MsgReplaying(this, "Title", "", true);
            lastTitleBuffer_= "";   
         }

         //D
         if (player_ && player_->Playing()) 
         {
             DEBUG_OSDCTRL ("\033[30;48m  --- %s --!!  Player DisplayHead    %s -- \033[0m\n", __PRETTY_FUNCTION__, titleBuffer.c_str());
             string titleBuffer = player_->GetTitleInfoString();
        
             if (lastTitleBuffer_ != titleBuffer)
             {
                displayReplay_->SetTitle(titleBuffer.c_str());
                lastTitleBuffer_ = titleBuffer;

                if (!Initial) 
                {
                    displayReplay_->Flush(); // XXX check this
                }   
                 printf("Control::ShowProgress, vor cStatus::MsgReplaying 2\n");
                cStatus::MsgReplaying(this, GetDisplayHeaderLine().c_str(), NULL, true);   
             }
         }
         else 
         { 
            printf("Control::ShowProgress, vor cStatus::MsgReplaying 3\n");
            cStatus::MsgReplaying(this, "Xine Media", NULL, true);
         }
         
         //E
         if (Current != lastCurrent_ || Total != lastTotal_) 
         {
             if (!modeOnly_) 
             {
               DEBUG_OSDCTRL ("\033[30;48m  --- %s  SetProgress--!!   -- \033[0m\n", __PRETTY_FUNCTION__);
               displayReplay_->SetProgress(Current, Total);
               displayReplay_->SetTotal(IndexToHMSF(Total));
               displayReplay_->SetCurrent(IndexToHMSF(Current));
               lastCurrent_ = Current;
               if (!Initial) 
                   displayReplay_->Flush();
            }
         }
         //F
         lastTotal_ = Total;
         ShowMode();
         return true;
      } 
      return false;
   }

   std::string Control::GetDisplayHeaderLine() const
   {
       if (!player || !player_->Playing())  
       {
           return "    ";
       }
       else
       {
           string titleinfo;
           if(player_->IsReplayingDvd())
           {
               titleinfo = "[dvd] " + player_->GetTitleInfoString();
           }
           else if(player_->IsReplayingMusic())
           {
               titleinfo = "[music] " + player_->GetTitleInfoString();
           }
           else if(player_->IsReplayingVideo())
           {
               titleinfo = "[video] " + player_->GetTitleInfoString();
           }
           else
           {
               titleinfo = "[unknown] " + player_->GetTitleInfoString();
           }
           return titleinfo;
       }

       //more detailed dvd info
       /*
       switch (XineSetup_Display)
       {
           case  0: // short
             titleinfo += player_->GetTitleInfoString();
             // only Chapter 
             break;
           case  1: // short
             titleinfo += player_->GetTitleInfoString();
             // only Chapter 
             titleinfo += " - ";  
             titleinfo += player_->GetAudioLangCode();         
             titleinfo += " ";   
             titleinfo += player_->AudioCodec();
             break;
           case  2: // short
             titleinfo += player_->GetTitleInfoString();
             // title and chapter 
             titleinfo += " lang ";  
             titleinfo += player_->GetAudioLangCode();         
             titleinfo += " ";   
             titleinfo += player_->AudioCodec();
             //if (useSpu) 
             if (false) 
             {
                titleinfo += " sub: ";  
                titleinfo += player_->GetSpuLangCode();
             }
             break;
           default:
             titleinfo = "  ";
       }*/
   }
}
}
