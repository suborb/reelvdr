/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#include <assert.h>
#include <vdr/i18n.h>
#include <vdr/thread.h>
#include <vdr/skins.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tools-dvd.h"
#include "player-dvd.h"
#include "control-dvd.h"

#define MENUTIMEOUT     120 // seconds
#define MAXWAIT4EPGINFO  10 // seconds
#define MODETIMEOUT       3 // seconds

bool cDvdPlayerControl::dvd_active = false;

#define DVDDEBUG

#define DEBUG_KEY(format, args...)
#define DEBUG_SHOW(format, args...)
#define DEBUG_OSDCTRL(format, args...)
//#define DEBUG_KEY(format, args...) printf (format, ## args); fflush(NULL)
//#define DEBUG_SHOW(format, args...) printf (format, ## args); fflush(NULL)
//#define DEBUG_OSDCTRL(format, args...) printf (format, ## args); fflush(NULL)

// --- cDvdPlayerControl -----------------------------------------------------

cDvdPlayerControl::cDvdPlayerControl(void):cControl(player = new cDvdPlayer()) {
    assert(dvd_active == false);
    dvd_active = true;
    visible = modeOnly = shown = displayFrames = false;
    osdTaker=NULL;
    lastCurrent = lastTotal = -1;
    lastPlay = lastForward = false;
    lastSpeed = -1;
    displayReplay=NULL;
    timeoutShow = 0;
    inputActive = NoneInput;
    inputHide=true;
    forceDvdNavigation=false;

    player->setController(this);

    cStatus::MsgReplaying(this, "DVD", NULL, true);
}

cDvdPlayerControl::~cDvdPlayerControl()
{
    Hide();
    cStatus::MsgReplaying(this, NULL, NULL, false);
    Stop();
    assert(dvd_active == true);
    dvd_active = false;
    delete player;
    player = NULL;
}

bool cDvdPlayerControl::Active(void)
{
  return player && player->Active();
}

void cDvdPlayerControl::Stop(void)
{
    if(player) {
        player->Stop();
    }
}

void cDvdPlayerControl::Pause(void)
{
    if (player)
        player->Pause();
}

void cDvdPlayerControl::Play(void)
{
    if (player)
        player->Play();
}

void cDvdPlayerControl::Forward(void)
{
    if (player)
        player->Forward();
}

void cDvdPlayerControl::Backward(void)
{
    if (player)
        player->Backward();
}

void cDvdPlayerControl::SkipSeconds(int Seconds)
{
    if (player)
        player->SkipSeconds(Seconds);
}

int cDvdPlayerControl::SkipFrames(int Frames)
{
    if (player)
        return player->SkipFrames(Frames);
  return -1;
}

bool cDvdPlayerControl::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
    if (player) {
        player->GetIndex(Current, Total, SnapToIFrame);
        return true;
    }
    Current=-1; Total=-1;
    return false;
}

bool cDvdPlayerControl::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
    return player && player->GetReplayMode(Play, Forward, Speed);
}

void cDvdPlayerControl::Goto(int Seconds, bool Still)
{
    if (player)
    {
        player->Goto(Seconds, Still);
    }
}

void cDvdPlayerControl::OsdClose()
{
    DELETENULL(displayReplay);
}

void cDvdPlayerControl::OsdOpen(void)
{
    if (visible) {
        DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: allready visible\n");
	    return;
    }

    if (OsdTaken(this)) {
        if(player) {
            DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: AllreadyOpen -> HideSPU ... \n");
		    player->HideSPU();
	    }
    }
    if (!TakeOsd((void *)this)) {
        DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: AllreadyOpen -> !visible, osdTaken=%d\n", OsdTaken(this));
    	visible=false;
    } else {
        DEBUG_OSDCTRL("DVD-Ctrl: OsdOpen: visible\n");
    	visible=true;
        displayReplay=Skins.Current()->DisplayReplay(modeOnly);
    }
}

void cDvdPlayerControl::ShowTimed(int Seconds) {
    if (modeOnly)
        Hide();
    if (!visible) {
        shown = ShowProgress(true);
        timeoutShow = (shown && Seconds > 0) ? time(NULL) + Seconds : 0;
    }
}

void cDvdPlayerControl::Show(void) {
    ShowTimed();
}

void cDvdPlayerControl::Hide(void)
{
    DEBUG_SHOW("DVD-Ctrl: Hide\n");

    TakeOsd((void *)(-1)); // somebody else (external) might take the osd ..
    if(player) player->HideSPU();

    HideOwnOsd();
}

void cDvdPlayerControl::HideOwnOsd(void)
{
    DEBUG_SHOW("DVD-Ctrl: HideOwnOsd: visible=%d\n", visible);

    if (visible) {
        OsdClose();
        needsFastResponse = visible = false;
        modeOnly = false;
        lastPlay = lastForward = false;
        lastSpeed = -1;
    }
}

bool cDvdPlayerControl::TakeOsd(void * owner)
{
  if(owner==(void *)-1 || !OsdTaken(owner))
  {
      DEBUG_OSDCTRL("DVD-Ctrl: TakeOsd(%p) new owner!\n", owner);
	  osdTaker=owner; // not taken by other: new owner
	  return true;
  } else {
          DEBUG_OSDCTRL("DVD-Ctrl: TakeOsd(%p) not taken!\n", owner);
  }
  return false;
}

bool cDvdPlayerControl::OsdTaken(void *me)
{
  if(osdTaker && !cOsd::IsOpen()) {
        DEBUG_OSDCTRL("DVD-Ctrl: OsdTaken(%p) !IsOpen->taker:=NULL\n", me);
  	osdTaker=NULL; // update info ..
  }
  DEBUG_OSDCTRL("DVD-Ctrl: OsdTaken(%p) != %p := %d \n", me, osdTaker, osdTaker!=me);
  return osdTaker!=NULL && osdTaker!=me;
}

bool cDvdPlayerControl::OsdVisible(void *me)
{
  return visible || OsdTaken(me) ;
}

void cDvdPlayerControl::ShowMode(void)
{
    if (Setup.ShowReplayMode && inputActive==NoneInput) {
        bool Play, Forward;
        int Speed;
        if (GetReplayMode(Play, Forward, Speed) && (!visible || Play != lastPlay || Forward != lastForward || Speed != lastSpeed)) {
            bool NormalPlay = (Play && Speed == -1);

            if (!visible) {
                if (NormalPlay)
                    return; // no need to do indicate ">" unless there was a different mode displayed before
                modeOnly = true;
                // open small display
                OsdOpen();
            }
            if (!visible)
	            return;

            if (modeOnly && !timeoutShow && NormalPlay)
                timeoutShow = time(NULL) + MODETIMEOUT;
            displayReplay->SetMode(Play, Forward, Speed);
            lastPlay = Play;
            lastForward = Forward;
            lastSpeed = Speed;
        }
    }
}

const char * cDvdPlayerControl::GetDisplayHeaderLine()
{
    char * titleinfo_str=NULL;
    char * title_str=NULL;
    char * aspect_str=NULL;

    const char * audiolang_str=NULL;
    const char * spulang_str=NULL;

    static char title_buffer[256];

    title_buffer[0]=0;
    if(!player) return title_buffer;

    titleinfo_str = player->GetTitleInfoString();
    title_str  = player->GetTitleString( );
    aspect_str = player->GetAspectString( );

    player->GetAudioLangCode( &audiolang_str );
    player->GetSubpLangCode( &spulang_str );

    snprintf(title_buffer, 255, "%s, %s, %s, %s, %s    ",
    titleinfo_str, audiolang_str, spulang_str, aspect_str, title_str);

    free(titleinfo_str);
    free(title_str);
    free(aspect_str);

    return title_buffer;
}

bool cDvdPlayerControl::ShowProgress(bool Initial)
{
    int Current, Total;
    const char * title_buffer=NULL;
    static char last_title_buffer[256];

    if (GetIndex(Current, Total) && Total > 0) {
	    DEBUG_SHOW("DVD-Ctrl: ShowProgress: ... \n");
        if (!visible) {
            needsFastResponse = true;
            OsdOpen();
        }
        if (!visible)
	        return false;

        if (Initial) {
            lastCurrent = lastTotal = -1;
            last_title_buffer[0]=0;
            displayReplay->SetTitle("unknown title");
            cStatus::MsgReplaying(this, "unknown title", NULL, true);
        }

        if (player) {
            title_buffer = GetDisplayHeaderLine();
            if ( strcmp(title_buffer,last_title_buffer) != 0 ) {
                displayReplay->SetTitle(title_buffer);
                if (!Initial) displayReplay->Flush();
                cStatus::MsgReplaying(this, title_buffer, NULL, true);
                strcpy(last_title_buffer, title_buffer);
            }
        }

        if (Current != lastCurrent || Total != lastTotal) {
            displayReplay->SetProgress(Current, Total);
            displayReplay->SetTotal(IndexToHMSF(Total));
            displayReplay->SetCurrent(IndexToHMSF(Current, displayFrames));
            lastCurrent = Current;
            if (!Initial) displayReplay->Flush();
        }
        lastTotal = Total;
        ShowMode();
        return true;
    } else {
        DEBUG_SHOW("DVD-Ctrl: ShowProgress: nope \n");
    }
    return false;
}

void cDvdPlayerControl::InputIntDisplay(const char * msg, int val)
{
    char buf[120];
    snprintf(buf,sizeof(buf),"%s %d", msg, val);
    displayReplay->SetJump(buf);
}

void cDvdPlayerControl::InputIntProcess(eKeys Key, const char * msg, int & val)
{
    switch (Key) {
        case k0 ... k9:
            val = val * 10 + Key - k0;
            InputIntDisplay(msg, val);
            break;
        case kOk:
        case kLeft:
        case kRight:
        case kUp:
        case kDown:
            switch ( inputActive ) {
                case TimeSearchInput:
                    break;
                case TrackSearchInput:
                    if(player) {
	                    player->GotoTitle(val);
		            }
                    break;
                default:
                    break;
            }
            inputActive = NoneInput;
            break;
        default:
            inputActive = NoneInput;
            break;
    }

    if (inputActive==NoneInput) {
        if (inputHide)
            Hide();
        else
            displayReplay->SetJump(NULL);
        ShowMode();
    }
}

void cDvdPlayerControl::TrackSearch(void)
{
    inputIntVal=0;
    inputIntMsg="Track: ";

    inputHide = false;

    Show();
    if (visible)
	    inputHide = true;
    else
        return;
    timeoutShow = 0;
    InputIntDisplay(inputIntMsg, inputIntVal);
    inputActive = TrackSearchInput;
}

void cDvdPlayerControl::TimeSearchDisplay(void)
{
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
    displayReplay->SetJump(buf);
}

void cDvdPlayerControl::TimeSearchProcess(eKeys Key)
{
#define STAY_SECONDS_OFF_END 10
    int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
    int Current = (lastCurrent / FRAMESPERSEC);
    int Total = (lastTotal / FRAMESPERSEC);
    switch (Key) {
        case k0 ... k9:
            if (timeSearchPos < 4) {
                timeSearchTime <<= 8;
                timeSearchTime |= Key - k0;
                timeSearchPos++;
                TimeSearchDisplay();
            }
            break;
        case kLeft:
        case kRight: {
            int dir = (Key == kRight ? 1 : -1);
            if (dir > 0)
                Seconds = min(Total - Current - STAY_SECONDS_OFF_END, Seconds);
            switch ( inputActive ) {
                case TimeSearchInput:
                    SkipSeconds(Seconds * dir);
                    break;
                case TrackSearchInput:
                    break;
                default:
                    break;
            }
            inputActive = NoneInput;
            break;
        }
        case kUp:
        case kDown:
            Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
            switch ( inputActive ) {
                case TimeSearchInput:
                    Goto(Seconds, Key == kDown);
                    break;
                case TrackSearchInput:
                    break;
                default:
                    break;
            }
            inputActive = NoneInput;
            break;
        default:
            inputActive = NoneInput;
            break;
    }

    if (inputActive==NoneInput) {
        if (inputHide)
            Hide();
        else
            displayReplay->SetJump(NULL);
        ShowMode();
    }
}

void cDvdPlayerControl::TimeSearch(void)
{
    timeSearchTime = timeSearchPos = 0;
    inputHide = false;
    Show();
    if (visible)
        inputHide = true;
    else
        return;
    timeoutShow = 0;
    TimeSearchDisplay();
    inputActive = TimeSearchInput;
}

bool cDvdPlayerControl::DvdNavigation(eKeys Key)
{
    if (!player)
        return false;

    HideOwnOsd();
    if(player) player->DrawSPU();

    switch (Key) {
    case kUp:
    case k2:
        player->selectUpButton();
        break;
    case kDown:
    case k8:
        player->selectDownButton();
        break;
    case kLeft:
    case k4:
        player->selectLeftButton();
        break;
    case kRight:
    case k6:
        player->selectRightButton();
        break;

    case kOk:
    case k5:
        player->activateButton();
        break;

    case k1:
        player->callRootMenu();
        break;
    case k3:
        player->callTitleMenu();
        break;
    case k7:
        player->callSubpMenu();
        break;
    case k9:
        player->callAudioMenu();
        break;

    default:
        return false;
    }
    return true;
}

void cDvdPlayerControl::updateShow(bool force)
{
    DEBUG_SHOW("DVD-Ctrl: updateShow: force=%d, visible=%d, modeOnly=%d\n",
        force, visible, modeOnly);
    if (visible || force)
    {
        if (timeoutShow && time(NULL) > timeoutShow)
        {
            Hide();
            ShowMode();
            timeoutShow = 0;
            return;
        }
        if (modeOnly)
            ShowMode();
        else
            shown = ShowProgress(!shown) || shown;
    } else {
        const char * title_buffer=NULL;
        static char last_title_buffer[256];

        if (player) {
	        title_buffer = GetDisplayHeaderLine();
            if ( strcmp(title_buffer,last_title_buffer) != 0 ) {
 	            strcpy(last_title_buffer, title_buffer);
 	            cStatus::MsgReplaying(this, title_buffer, NULL, true);
            }
        }
    }
}

eOSState cDvdPlayerControl::ProcessKey(eKeys Key)
{
  DEBUG_KEY("DVD-Ctrl: ProcessKey BEGIN\n");
  eOSState state = cOsdObject::ProcessKey(Key);

  if (state != osUnknown) {
     Hide();
     DEBUG_KEY("cDvdPlayerControl::ProcessKey key: %d 0x%X, state=%d 0x%X FIN!\n",
	 Key, Key, state, state);
     DEBUG_KEY("DVD-Ctrl: ProcessKey END\n");
     return state;
  }

  DEBUG_KEY("cDvdPlayerControl::ProcessKey key: %d 0x%X, state=unknown\n", Key, Key);

  if (player && player->DVDRemoveable()) {
     DEBUG_KEY("cDvdPlayerControl::ProcessKey DVDRemoveable -> Stop\n");
     Hide();
     DEBUG_KEY("DVD-Ctrl: ProcessKey END\n");
     return osEnd;
  }
  if (!Active()) {
     DEBUG_KEY("cDvdPlayerControl::ProcessKey !Active -> Stop\n");
     Hide();
     Stop();
     DEBUG_KEY("DVD-Ctrl: ProcessKey END\n");
     return osEnd;
  }
  updateShow();

  bool DisplayedFrames = displayFrames;
  displayFrames = false;
  if (inputActive!=NoneInput && Key != kNone) {
	 switch ( inputActive ) {
		case TimeSearchInput:
	     	    TimeSearchProcess(Key);
			break;
		case TrackSearchInput:
		        InputIntProcess(Key, inputIntMsg, inputIntVal);
			break;
		default:
			break;
	 }
     DEBUG_KEY("DVD-Ctrl: ProcessKey END\n");
     return osContinue;
  }
  bool DoShowMode = true;

  state = osContinue;

  if ( ( player && player->IsInMenuDomain() ) || forceDvdNavigation ) {

      switch (Key) {
          case kRed: forceDvdNavigation=false; break;
	  case kGreen|k_Repeat:
	  case kGreen:   player->PreviousTitle(); break;
	  case kYellow|k_Repeat:
	  case kYellow:  player->NextTitle(); break;

	  case kUp:
	  case kDown:
	  case kLeft:
	  case kRight:
	  case kOk:
	  case k0 ... k9:
	      DoShowMode = false;
	      displayFrames = DisplayedFrames;
	      if (DvdNavigation(Key)) {
		  break;
	      }
	  default:
              state = osUnknown;
	      break;
      }
  } else {
      switch (Key) {
	  // Positioning:
	  case kPlay:
	  //case kUp:      
      Play(); break;

	  case kPause:
	  //case kDown:    
      Pause(); break;

	  case kFastRew|k_Release:
	  //case kLeft|k_Release:
	      if (Setup.MultiSpeedMode) break;
	  case kFastRew:
	  //case kLeft:    
          Backward(); break;

	  case kFastFwd|k_Release:
	  //case kRight|k_Release:
	      if (Setup.MultiSpeedMode) break;
	  case kFastFwd:
	  //case  kRight:   
           Forward(); break;

	  default:
              state = osUnknown;
	      break;
      }
  }


  if(state==osUnknown) {
      state = osContinue;

      switch (Key) {
	  // Positioning:
	  case kRed:     TimeSearch(); break;
	  case kGreen|k_Repeat:
	  case kGreen:   SkipSeconds(-60); break;
	  case kYellow|k_Repeat:
	  case kYellow:  SkipSeconds( 60); break;
	  case kBlue:    TrackSearch(); break;
      
      case kLeft: 
      case kLeft|k_Repeat:
           SkipSeconds(-3); break;
      case kRight:
      case kRight|k_Repeat:
           SkipSeconds(3); break;

	  default: {
	      DoShowMode = false;
	      displayFrames = DisplayedFrames;
	      switch (Key) {
		  // Menu control:
                    case kOk:
                        if (visible && !modeOnly ) {
                            Hide();
                            DoShowMode = true;
                        } else
                            Show();
                        break;
                    case kStop:
                    case kBack: 
                        Hide();
                        Stop();
	                    state=osEnd;
                        break;
                    //Start by Klaus
                    /*case kBack:
                        player->GoUp();
                    break;*/
                    case kPlay:
                        player->GotoFilm();
                    break;    
                    //End by Klaus
                    case k2: if(player) {
                                if (player->NextSubpStream() == -1)
                                    Skins.Message(mtError, tr("Error.DVD$Current subp stream not seen!"));

                                if (player->GetCurrentNavSubpStream() == -1) {
                                    Hide();
                                    Show();
                                }
                             }
                    break;
                    case k3: if(player) player->NextAngle(); break;

                    case k4: 
                    case kDown:
                        if(player) player->PreviousPart(); break;
                    case k6: 
                    case kUp:
                        if(player) player->NextPart(); break;

                    case kChanDn:
                    case k7: if(player) player->PreviousTitle(); break;

                    case kChanUp:
                    case k9: if(player) player->NextTitle(); break;
                    case k5:
                        if(visible && !modeOnly ) {
                            Hide();
                            player->callRootMenu();
                        } else {
                            Hide();
                            forceDvdNavigation=true;
                        }
                        break;

                    case k8:
                        if (player) {
                            Hide();
                            player->callRootMenu();
                        }
                        break;
                    case k0:
                        if (player) {
                            Hide();
                            player->callAudioMenu();
                        }
                        break;

                    default:
	  	                state=osUnknown;
                }
            }
        }
    }

    if (DoShowMode && state!=osEnd)
    {
        DEBUG_SHOW("cDvdPlayerControl::ProcessKey ShowMode\n");
        ShowMode();
    }

    DEBUG_KEY("DVD-Ctrl: ProcessKey END\n");
    return state;
}
