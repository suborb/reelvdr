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

// XineLib.c

#include <map>

#include <xine.h>
#include <xine/audio_out.h>

#include <vdr/plugin.h>
#include <vdr/tools.h>

#include "Utils.h"
#ifndef USE_PLAYPES
#include "XineLib.h"
#endif
#include "VdrXineMpIf.h"

#include "Player.h"
#include "XineEvent.h"
#include "SpuDecode.h"

#include "timecounter.h"
#include "xineOsd.h"

//#define DEBUG_XINE_EVENT(format, args...) printf (format, ## args)
#define DEBUG_XINE_EVENT(format, args...)
//#define DEBUG_XINE(format, args...) printf (format, ## args)
#define DEBUG_XINE(format, args...)

namespace Reel {
  namespace XineMediaplayer {

    #define SEEK_TIMEOUT 500

    int useOld = 1;
    /*static*/ VdrXineMpIf /*const*/ *vxi = nullptr;
    static void (*aoPutBufferOrig_)(xine_audio_port_t *aoPort, audio_buffer_t *buf, xine_stream_t *stream);
    static void AoPutBuffer(xine_audio_port_t *aoPort, audio_buffer_t *buf, xine_stream_t *stream) {
        // We do this wrapping to get to the _original_ presentation timestamps not the
        // virtual ones of xine.
      vxi->AoPutMemPts(buf->mem, buf->vpts); // buf->vpts is at this time the original stream pts!
      aoPutBufferOrig_(aoPort, buf, stream);
    } // AoPutBuffer

    XineLib::XineLib() NO_THROW : xine_(nullptr),
                                  aoPort_(nullptr),
                                  voPort_(nullptr),
                                  stream_(nullptr),
                                  eventQueue_(nullptr),
                                  streamOpen_(false),
                                  streamPlaying_(false),
                                  seekPos(0) {
    } // XineLib::XineLib

    void XineLib::Close() NO_THROW {
      try {
        DEBUG_XINE("DEBUG [xinemedia]: Close() %d %s:%d\n", streamOpen_, __FILE__,__LINE__);
        Stop();
        // Close the stream.
        if (streamOpen_) if(stream_) ::xine_close(stream_);
        DEBUG_XINE("DEBUG [xinemedia]: Close() DONE %d %s:%d\n", streamOpen_, __FILE__,__LINE__);
      } catch(...){}
      streamOpen_ = false;
    } // XineLib::Close

    void XineLib::Exit() NO_THROW {
      try {
        DEBUG_XINE("DEBUG [xinemedia]: Exit() %p %p %p %p %p %s:%d\n", eventQueue_, stream_, voPort_, aoPort_, xine_, __FILE__,__LINE__);
        // Close the stream.
        Close();
      } catch(...){}
      // Dispose the event queue.
      try {
        if (eventQueue_) ::xine_event_dispose_queue(eventQueue_);
      } catch(...){}
      eventQueue_ = nullptr;
      // Dispose the stream.
      try {
        if (stream_) ::xine_dispose(stream_);
      } catch(...){}
      stream_ = nullptr;
      // Close the video driver.
      try {
        if (voPort_) ::xine_close_video_driver(xine_, voPort_);
      } catch(...){}
      voPort_ = nullptr;
      // Close the audio driver.
      try {
        if (aoPort_) ::xine_close_audio_driver(xine_, aoPort_);
      } catch(...){}
      aoPort_ = nullptr;
      // Exit xine
      try {
        if (xine_) {
          ::xine_exit(xine_);
          xine_ = nullptr;
          // FIXME: Helps with bsp craches:
          ::usleep(2 * 1000 * 1000);
          //printf("+++++++++++++----xine_ exit, nach usleep----++++++++++++++++\n");
        } // if
      } catch(...){}
      xine_ = nullptr;
    } // XineLib::Exit

    XineEventBase *XineLib::GetEvent() const THROWS {
      std::auto_ptr<XineEventBase> cpEvent;
      if (!eventQueue_) {
        DEBUG_XINE_EVENT(" no event queue \n");
        return nullptr;
      } // if
      xine_event_t *event = ::xine_event_get(eventQueue_);
      if (event) {
        DEBUG_XINE_EVENT("Xine event %d\n", event->type);
        switch(event->type) {
          case XINE_EVENT_UI_PLAYBACK_FINISHED:
isyslog("[xinemedia] Playback finished\n");
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_UI_PLAYBACK_FINISHED---++++++++++++++++\n");
            cpEvent.reset(new EventUIPlaybackFinished);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_UI_CHANNELS_CHANGED:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_UI_CHANNELS_CHANGED---++++++++++++++++\n");
            cpEvent.reset(new EventUIChannelsChanged);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_UI_SET_TITLE:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_UI_SET_TITLE---++++++++++++++++\n");
            cpEvent.reset(new EventUISetTitle);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_UI_MESSAGE:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_UI_MESSAGE ---++++++++++++++++\n");
            cpEvent.reset(new EventUIMessage);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_FRAME_FORMAT_CHANGE:
            DEBUG_XINE_EVENT("!!!!!!!!!!!!!!!!!!!!! XINE_EVENT_FRAME_FORMAT_CHANGE---!!!!!!!!!!!!!!!!!!!!!n");
            cpEvent.reset(new EventFormatChange);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_AUDIO_LEVEL:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_AUDIO_LEVEL---++++++++++++++++\n");
            cpEvent.reset(new EventAudioLevel);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_PROGRESS: // network conection
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_PROGRESS---++++++++++++++++\n");
            cpEvent.reset(new EventProgress);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_UI_NUM_BUTTONS:
            //printf("++++++++++++---XINE_EVENT_UI_NUM_BUTTONS---++++++++++++++++\n");
            cpEvent.reset(new EventUINumButtons);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_SPU_BUTTON:
            //printf("--*************----XINE_EVENT_SPU_BUTTON----******************--\n");
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_SPU_BUTTON---++++++++++++++++\n");
            cpEvent.reset(new EventSpuButton);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_DROPPED_FRAMES:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_DROPPED_FRAMES---++++++++++++++++\n");
            cpEvent.reset(new EventDroppendFrames);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_MRL_REFERENCE_EXT:
            DEBUG_XINE_EVENT("++++++++++++---XINE_EVENT_MRL_REFERENCE_EXT---++++++++++++++++\n");
            cpEvent.reset(new EventMrlReferenceExt);
            cpEvent->CopyEvent(event);
            break;
          case XINE_EVENT_QUIT:
            DEBUG_XINE_EVENT("+++++++++++++++---XINE_EVENT_UI_QUIT---+++++++++++++++++++++\n");
            cpEvent.reset(new EventQuit);
            cpEvent->CopyEvent(event);
            break;
          default:
            break;
        } // switch
        DEBUG_XINE_EVENT("+++  xine_event_free +++\n");
        ::xine_event_free(event);
        return cpEvent.release();
      } // if
      return nullptr;
    } // XineLib::GetEvent

    void XineLib::Init() THROWS {
      DEBUG_XINE("+++++++++++++++---XINE_LIB_INIT---+++++++++++++++++++++\n");
      Exit();
      // Create and init xine.
      xine_ = ::xine_new();
      xine_init(xine_);
      int major, minor, sub;
      major = minor = sub = -1;
      xine_get_version(&major, &minor,&sub);
      isyslog("[xinemedia] libxine version %d.%d.%d\n",  major, minor, sub);
      isyslog("[xinemedia] vdr plugin built on " XINE_VERSION "\n");
      isyslog("[xinemedia] versions %s compatible   \n",   xine_check_version(major, minor, sub)==1 ? "are" : "are NOT!");
      xine_engine_set_param(xine_, XINE_ENGINE_PARAM_VERBOSITY,  XINE_VERBOSITY_NONE/*XINE_VERBOSITY_LOG*/);
      xine_engine_set_param(xine_, XINE_PARAM_VERBOSITY,  XINE_VERBOSITY_LOG);
      DEBUG_XINE("DEBUG [xinemedia]  set xine verbose \n");
      if (!xine_check_version(major, minor, sub)) {
        throw XineError("XineLib is not compatible");
        DEBUG_XINE("DEBUG [xinemedia] XineLib is not compatible");
        Exit();
      } // if
      if(useOld) {
        //  switch reelbox plugin
        cPluginManager::CallAllServices("GetVdrXineMpIf", &vxi);
        if (!vxi) {
          DEBUG_XINE("DEBUG [xinemedia] VdrXineMpIf not found");
          throw XineError("VdrXineMpIf not found");
          Exit();
        } // if
      } // if
      SetXineConfig();
      //PrintXineConfig();
      char const *audioDriverId;
      if(useOld) {
        vxi->SpuPutEvents = SpuDecoder::SpuPutEvents;
        vxi->SpuGetEventStruct = SpuDecoder::SpuGetEventStruct;
        // Open the audio driver.
        //printf("[xinemedia] Open audio driver\n");
        audioDriverId = "audio_out_reel";
        aoPort_ = ::xine_open_audio_driver(xine_, audioDriverId, const_cast<VdrXineMpIf *>(vxi));
      } else {
        audioDriverId = "hde-audio";
        aoPort_ = ::xine_open_audio_driver(xine_, audioDriverId, 0);
      } // if
      if (!aoPort_) {
        DEBUG_XINE("DEBUG [xinemedia] unable to open audio driver \"%s\"", audioDriverId);
        PrintXinePlugins();
        std::string errorMsg = FormatString("unable to open audio driver \"%s\"", audioDriverId);
        throw XineError(errorMsg.c_str());
        Exit();
      } // if
      // HACK: Decorate put_buffer().
      if (useOld && aoPort_->put_buffer != AoPutBuffer) {
        aoPutBufferOrig_ = aoPort_->put_buffer;
        aoPort_->put_buffer = AoPutBuffer;
      } // if
      // Open the video driver.
      DEBUG_XINE("DEBUG [xinemedia] Open video driver\n");
      char const *videoDriverId;
      if(useOld) {
        videoDriverId = "none";
        voPort_ = ::xine_open_video_driver(xine_, videoDriverId, XINE_VISUAL_TYPE_NONE, nullptr);
      } else {
        videoDriverId = "hde-video-aa";
        voPort_ = ::xine_open_video_driver(xine_, videoDriverId, XINE_VISUAL_TYPE_AA, nullptr);
      } // if
      if (!voPort_) {
        std::string errorMsg = FormatString("unable to open video driver \"%s\"", videoDriverId);
        throw XineError(errorMsg.c_str());
        Exit();
      } // if
      if(!useOld) { // Disable unused video decoders (needs patched xine-lib)
        const char *const *driver_ids;
        const char *driver_id;
        if ((driver_ids = xine_list_video_decoder_plugins (xine_))) {
          driver_id  = *driver_ids++;
          while (driver_id) {
            char key[1024] = "engine.decoder_priorities.";
            strcat(key, driver_id);
            xine_cfg_entry_t entry;
            if(xine_config_lookup_entry(xine_, key, &entry)) {
              entry.num_value = strcmp(driver_id, "hde-vd") ? -1 : 0;
              xine_config_update_entry(xine_, &entry);
            } // if
            driver_id  = *driver_ids++;
          } // while
        } // if
      } // if
      // Create the stream.
      stream_ = xine_stream_new(xine_, aoPort_, voPort_);
      if (!stream_) {
        esyslog ("ERROR [xinemediaplayer]: unable to create stream: %s; exit %s:%d", XineGetErrorStr().c_str(),__FILE__,__LINE__);
        std::string errorMsg = FormatString("unable to create stream: %s", XineGetErrorStr().c_str());
        throw XineError(errorMsg.c_str());
        Exit();
      } // if
      if(!useOld) {
        cSetupLine *lLine = Setup.Get("DelayAudio");
        if(lLine && lLine->Value())
          SetParamAVOffset(-1*atoi(lLine->Value()));
        lLine = Setup.Get("DelaySPU");
        if(lLine && lLine->Value())
          SetParamSpuOffset(atoi(lLine->Value()));
        else
          SetParamSpuOffset(90000);
        isyslog("[xinemedia] av offset %d\n", -1*GetParamAVOffset());
        isyslog("[xinemedia] spu offset %d\n", GetParamSpuOffset());
      } // if
      seekPos = 0;
      // Register the event queue.
      eventQueue_ = ::xine_event_new_queue(stream_);
      DEBUG_XINE("+++++++++++++++---XINE_LIB_INIT END---+++++++++++++++++++++\n");
    } // XineLib::Init

    void XineLib::Open(char const *mrl) THROWS {
      DEBUG_XINE("DEBUG [xinemedia]: Open(%s)  %s:%d\n", mrl, __FILE__,__LINE__);
      seekPos = 0;
      if (stream_) {
        Close();
        //printf("%s: Opening %s (%s:%d)\n", __PRETTY_FUNCTION__, mrl, __FILE__, __LINE__);
        // Open the stream.
        if (!::xine_open(stream_, mrl)) {
          std::string errorMsg = FormatString("unable to open mrl \"%s\": %s", mrl, XineGetErrorStr().c_str());
          Close();
          throw XineError(errorMsg.c_str());
        } // if
        streamOpen_ = true;
      } // if
    } // XineLib::Open

    void XineLib::Play(int startPos, int startTime) THROWS {
      DEBUG_XINE("DEBUG [xinemedia]: Play(%d,%d)  %s:%d\n", startPos, startTime, __FILE__,__LINE__);
      if (stream_ && streamOpen_) {
        // Stream may already be playing.
        if (!::xine_play(stream_, startPos, startTime)) {
          esyslog ("ERROR [xinemediaplayer]: unable to create stream: %s; exit %s:%d", XineGetErrorStr().c_str(),__FILE__,__LINE__);
          std::string errorMsg = FormatString("unable to play stream from pos %d, %d: %s", startPos, startTime, XineGetErrorStr().c_str());
          Stop();
          throw XineError(errorMsg.c_str());
        } // if
        streamPlaying_ = true;
        // start time counter
        timeCounter.CleanStart();
      } // if
    } // XineLib::Play

    void XineLib::ProcessKeyMenuMode(eKeys key) THROWS {
      if (stream_) {
        int speed = ::xine_get_param(stream_, XINE_PARAM_SPEED);
        if(XINE_SPEED_NORMAL != speed) {
          DEBUG_XINE("DEBUG [xinemedia]: Reset to normal play due to menu mode (was %d)\n", speed);
          ::xine_set_param(stream_, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
        } // if
        ProcessInputKeyMenuMode(key);
        //ProcessPlaymodeKey(key,false); // No trickmode during menu mode
      } // if
    } // XineLib::ProcessKeyMenuMode

    void XineLib::ProcessKey(eKeys key,bool isMusic) THROWS {
      if (stream_) {
        ProcessInputKey(key);
        ProcessPlaymodeKey(key,isMusic);
        ProcessSeekKey(key, isMusic);
      } // if
    } // XineLib::ProcessKey

    void XineLib::ProcessInputKeyMenuMode(eKeys key) THROWS {
      // PRE: Stream exists.
      int eventType;
      switch (key) {
        case kLeft : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_LEFT\n"    ); eventType = XINE_EVENT_INPUT_LEFT;     break;
        case kUp   : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_UP\n"      ); eventType = XINE_EVENT_INPUT_UP;       break;
        case kRight: DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_RIGHT\n"   ); eventType = XINE_EVENT_INPUT_RIGHT;    break;
        case kDown : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_DOWN\n"    ); eventType = XINE_EVENT_INPUT_DOWN;     break;
        case kOk   : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_SELECT\n"  ); eventType = XINE_EVENT_INPUT_SELECT;   break;
        case k1    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU1\n"   ); eventType = XINE_EVENT_INPUT_MENU1;    break;
        case k2    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU2\n"   ); eventType = XINE_EVENT_INPUT_MENU2;    break;
        case k3    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU3\n"   ); eventType = XINE_EVENT_INPUT_MENU3;    break;
        case k4    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU4\n"   ); eventType = XINE_EVENT_INPUT_MENU4;    break;
        case k5    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU5\n"   ); eventType = XINE_EVENT_INPUT_MENU5;    break;
        case k6    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU6\n"   ); eventType = XINE_EVENT_INPUT_MENU6;    break;
        case k7    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_MENU7\n"   ); eventType = XINE_EVENT_INPUT_MENU7;    break;
        case kNext : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_NEXT\n"    ); eventType = XINE_EVENT_INPUT_NEXT;     break;
        case kPrev : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode XINE_EVENT_INPUT_PREVIOUS\n"); eventType = XINE_EVENT_INPUT_PREVIOUS; break;
        default    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKeyMenuMode UNKNOWN %d\n", key); eventType = 0;
      } // switch
      if (eventType) {
        seekPos = 0;
        // Send the xine input event.
        xine_event_t inputEvent;
        inputEvent.type        = eventType;
        inputEvent.stream      = stream_;
        inputEvent.data        = nullptr;
        inputEvent.data_length = 0;
        //printf("--*************----vor ::xine_event_send--******************--\n");
        if(stream_) ::xine_event_send(stream_, &inputEvent);
      } // if
    } // XineLib::ProcessInputKeyMenuMode

    void XineLib::ProcessInputKey(eKeys key) THROWS {
      // PRE: Stream exists.
      int eventType;
      switch (key) {
        case kLeft : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_LEFT\n"    ); eventType = XINE_EVENT_INPUT_LEFT;     break;
        case kUp   : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_NEXT\n"    ); eventType = XINE_EVENT_INPUT_NEXT;     break;
        case kRight: DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_RIGHT\n"   ); eventType = XINE_EVENT_INPUT_RIGHT;    break;
        case kDown : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_PREVIOUS\n"); eventType = XINE_EVENT_INPUT_PREVIOUS; break;
        case kOk   : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_SELECT\n"  ); eventType = XINE_EVENT_INPUT_SELECT;   break;
        case k1    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU1\n"   ); eventType = XINE_EVENT_INPUT_MENU1;    break;
        case k2    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU2\n"   ); eventType = XINE_EVENT_INPUT_MENU2;    break;
        case k3    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU3\n"   ); eventType = XINE_EVENT_INPUT_MENU3;    break;
        case k4    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU4\n"   ); eventType = XINE_EVENT_INPUT_MENU4;    break;
        case k5    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU5\n"   ); eventType = XINE_EVENT_INPUT_MENU5;    break;
        case k6    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU6\n"   ); eventType = XINE_EVENT_INPUT_MENU6;    break;
        case k7    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_MENU7\n"   ); eventType = XINE_EVENT_INPUT_MENU7;    break;
        case kNext : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_NEXT\n"    ); eventType = XINE_EVENT_INPUT_NEXT;     break;
        case kPrev : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey XINE_EVENT_INPUT_PREVIOUS\n"); eventType = XINE_EVENT_INPUT_PREVIOUS; break;
        default    : DEBUG_XINE("DEBUG [xinemedia]: ProcessInputKey UNKNOWN %d\n", key); eventType = 0;
      } // switch
      if (eventType && stream_) {
        seekPos = 0;
        // Send the xine input event.
        xine_event_t inputEvent;
        inputEvent.type        = eventType;
        inputEvent.stream      = stream_;
        inputEvent.data        = nullptr;
        inputEvent.data_length = 0;
        //printf("--*************----vor ::xine_event_send--******************--\n");
        if(stream_) ::xine_event_send(stream_, &inputEvent);
      } // if
    } // XineLib::ProcessInputKey

    void XineLib::ProcessPlaymodeKey(eKeys key,bool isMusic) THROWS {
      // PRE: Stream exists.
      // TODO: Make ffwd work.
      // TODO: Make prev/next chapter work
      if(!stream_) return;
      int speed = XINE_SPEED_NORMAL;
      //static bool paused = false;
      switch (key) {
        case kPause:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessPlaymodeKey kPause\n");
          if(::xine_get_param(stream_, XINE_PARAM_SPEED) == XINE_SPEED_PAUSE) {
            speed = XINE_SPEED_NORMAL;
            timeCounter.Start();
          } else {
            speed = XINE_SPEED_PAUSE;
            timeCounter.Pause();
            cXineOsd::kPausePressed = 1;
          }
          break;
        case kPlay:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessPlaymodeKey kPlay\n");
          timeCounter.Start();
          speed = XINE_SPEED_NORMAL;
          break;
        case kFastFwd:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessPlaymodeKey kFastFwd\n");
          if(!useOld && !isMusic)
            speed = ::xine_get_param(stream_, XINE_PARAM_SPEED)*2;
          break;
        case kFastRew:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessPlaymodeKey kFastRew\n");
          if(!useOld && !isMusic)
            speed = ::xine_get_param(stream_, XINE_PARAM_SPEED)/2;
          break;
        default:
          return;
      } // switch
      seekPos = 0;
      ::xine_set_param(stream_, XINE_PARAM_SPEED, speed);
    } // XineLib::ProcessPlaymodeKey

    void XineLib::ProcessSeekKey(eKeys key, bool isMusic) THROWS {
#if 0 // Old handling: skipping more than one time results in full queue and processing continues far after key is released
        // PRE: Stream exists.
        int posStream;  // not used
        int posTime;    // ms
        int posLength;  // ms

        if(!stream_) return;

        // try to get current position 5 times. Increases wait time by a maximum of 500ms.
        int ret , t = 0;
        while ((ret = ::xine_get_pos_length(stream_, &posStream, &posTime, &posLength))==0 && (++t < 5))
            cCondWait::SleepMs(100);
        if (key != kNone)
            printf("(%s:%d) xine_get_pos after tries: \t\t %i\n",__FILE__, __LINE__, t);

        if (ret &&
            posTime >= 0 &&
            posTime < posLength)
        {
            switch (key)
            {
        case kFastFwd:
        case kFastFwd | k_Repeat:
            if(!useOld && !isMusic) return;
            case kYellow | k_Repeat:
            case kYellow:
                // One minute forward.
                posTime += 60000; // 60s
                if (posTime >= posLength)
                {
                    return;
                }
                break;
        case kFastRew:
        case kFastRew | k_Repeat:
                if (!useOld && !isMusic) return;
            case kGreen | k_Repeat:
            case kGreen:
                // One minute backward.
                posTime -= 60000; // 60s
                if (posTime < 0)
                {
                    posTime = 0;
                }
                break;
            case kRight | k_Repeat:
            case kRight:
                // Five second forward.
                posTime += 5000; // 5s
                if (posTime >= posLength)
                {
                    return;
                }
                break;
            case kLeft | k_Repeat:
            case kLeft:
                // Five second backward.
                posTime -= 5000; // 5s
                if (posTime < 0)
                {
                    return;
                }
                break;
            default:
                return;
            }
            //::printf("Play(%d)\n", posTime);
printf("Play(%d)\n", posTime);
            Play(0, posTime);
printf("Play(%d) DONE\n", posTime);
        }
        else
        {
            if(key != kNone)
                printf("skipping key: %d posTime %i, posLength %i\n", key,posTime, posLength);
        }
#else
      if(!stream_) return;
      switch (key) {
        case kFastFwd | k_Repeat:
        case kFastFwd:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kFastFwd\n");
          if(!useOld && !isMusic) return;
        case kYellow | k_Repeat:
        case kYellow:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kYellow\n");
          // One minute forward.
          seekPos += 60000; // 60s
          seekTime.Set(SEEK_TIMEOUT);
          break;
        case kFastRew | k_Repeat:
        case kFastRew:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kFastRew\n");
          if (!useOld && !isMusic) return;
        case kGreen | k_Repeat:
        case kGreen:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kGreen\n");
          // One minute backward.
          seekPos -= 60000; // 60s
          seekTime.Set(SEEK_TIMEOUT);
          break;
        case kRight | k_Repeat:
        case kRight:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kRight\n");
          // Five second forward.
          seekPos += 5000; // 5s
          seekTime.Set(SEEK_TIMEOUT);
          break;
        case kLeft | k_Repeat:
        case kLeft:
          DEBUG_XINE("DEBUG [xinemedia]: ProcessSeekKey kLeft\n");
          // Five second backward.
          seekPos -= 5000; // 5s
          seekTime.Set(SEEK_TIMEOUT);
          break;
        default:
          return;
      } // switch
#endif
    } // XineLib::ProcessSeekKey

    void XineLib::ProcessSeek() NO_THROW {
      try {
        int posStream;  // not used
        int posTime;    // ms
        int posLength;  // ms
        if(!stream_ || !seekPos) return;
        if(!seekTime.TimedOut()) return;
        int ret , t = 0;
        while ((ret = ::xine_get_pos_length(stream_, &posStream, &posTime, &posLength))==0 && (++t < 5))
          cCondWait::SleepMs(100);
        if (ret && (posTime >= 0) && (posTime < posLength)) {
          posTime += seekPos;
          seekPos=0;
          if(posTime < 0) posTime = 0;
          if(posTime > posLength) posTime = posLength;
          Play(0, posTime);
        } // if
      } catch(...){}
    } // XineLib::ProcessSeek

    void XineLib::Stop() NO_THROW {
      try {
        DEBUG_XINE("DEBUG [xinemedia]: Stop() %d  %s:%d\n", streamPlaying_, __FILE__,__LINE__);
        if (streamPlaying_ && stream_) ::xine_stop(stream_);
        DEBUG_XINE("DEBUG [xinemedia]: Stop() DONE %d  %s:%d\n", streamPlaying_, __FILE__,__LINE__);
      } catch(...) {}
      streamPlaying_ = false;
    } // XineLib::Stop

    std::string XineLib::XineGetErrorStr() const NO_THROW {
      char const *ret = "unknown error";
      try {
        if (stream_) {
          switch (::xine_get_error(stream_)) {
            case XINE_ERROR_NO_INPUT_PLUGIN: ret = "no input plugin"; break;
            case XINE_ERROR_NO_DEMUX_PLUGIN: ret = "no demux plugin"; break;
            case XINE_ERROR_DEMUX_FAILED   : ret = "no demux failed"; break;
            case XINE_ERROR_MALFORMED_MRL  : ret = "malformed mrl";   break;
            case XINE_ERROR_INPUT_FAILED   : ret = "input failed";    break;
            default: // unknown error.
              break;
          } // switch
        } // if
      } catch(...){}
      return ret;
    } // XineLib::XineGetErrorStr

    const char *XineLib::GetSetupLang() const NO_THROW {
#ifdef REELVDR
      try {
        int  *langIdx =  ::Setup.AudioLanguages;
        char  lang[4] = { 0 };
        const char *langCode = I18nLanguageCode(*langIdx);
        if(langCode) strncpy(lang, langCode,3);
        lang [3] = '\0';
        //printf (" ------------ lang %s ---------- \n", lang);
        if (strcmp(lang,"eng") == 0 || strcmp(lang ,"dos") == 0)
          return "en";
        else if (strcmp(lang,"deu") == 0 || strcmp(lang ,"ger") == 0)
          return "de";
        else if (strcmp(lang,"slv") == 0 || strcmp(lang ,"slo") == 0)
          return "si";
        else if (strcmp(lang,"ita") == 0)
          return "it";
        else if (strcmp(lang,"dut") == 0 || strcmp(lang ,"nla") == 0 || strcmp(lang,"nld") == 0)
          return "nl";
        else if (strcmp(lang,"por") == 0)
          return "pt";
        else if (strcmp(lang,"fra") == 0 || strcmp(lang ,"fre") == 0)
          return "fr";
        else if (strcmp(lang,"fin") == 0 || strcmp(lang ,"smi") == 0)
          return "fi";
        else if (strcmp(lang,"pol") == 0)
          return "pl";
        else if (strcmp(lang,"esl") == 0 || strcmp(lang ,"spa") == 0)
          return "es";
        else if (strcmp(lang,"ell") == 0 || strcmp(lang ,"gre") == 0)
          return "gr";
        else if (strcmp(lang,"sve") == 0 || strcmp(lang ,"swe") == 0)
          return "se";
        else if (strcmp(lang,"rom") == 0 || strcmp(lang ,"rum") == 0)
          return "ru";
        else if (strcmp(lang,"hun") == 0)
          return "hu";
        else if (strcmp(lang,"cat") == 0 || strcmp(lang ,"cln") == 0)
          return "es";
        else if (strcmp(lang,"rus") == 0)
          return "hu";
        else if (strcmp(lang,"est") == 0)
          return "hu";
        else if (strcmp(lang,"dan") == 0)
          return "dk";
        else if (strcmp(lang,"hrv") == 0)
          return "hr";
        else if (strcmp(lang,"cze") == 0 || strcmp(lang ,"ces") == 0)
          return "cz";
      } catch(...){}
#endif
      return "en";
    } // XineLib::GetSetupLang

    void XineLib::SetXineConfig() {
      REEL_ASSERT(xine_);
      xine_cfg_entry_t entry;
      if (xine_config_lookup_entry(xine_, "media.dvd.language", &entry) == 1) {
        entry.str_value = (char*)GetSetupLang();
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key -- media.dvd.language \n");
      if (xine_config_lookup_entry(xine_, "media.dvd.region" ,&entry) == 1) {
        entry.num_value = 2; // todo select via  setup timezone
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key -- media.dvd.region \n");
      if (xine_config_lookup_entry(xine_, "media.dvd.device",&entry) == 1) {
        entry.str_value = (char*)"/dev/dvd";
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key -- media.dvd.device \n");
      if (xine_config_lookup_entry(xine_, "engine.decoder_priorities.a/52",&entry) == 1) {
        entry.num_value = !::Setup.UseDolbyDigital;
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key: engine.decoder_priorities.a/52 \n");
      if (xine_config_lookup_entry(xine_, "engine.decoder_priorities.mpeg_videodec_reel",&entry) == 1) {
        entry.num_value = useOld ? 20 : 0; // Only use this plugin in bsp mode
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key: engine.decoder_priorities.mpeg_videodec_reel \n");
      if (xine_config_lookup_entry(xine_, "engine.decoder_priorities.spudec_reel",&entry) == 1) {
        entry.num_value = useOld ? 20 : 0; // Only use this plugin in bsp mode
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key: engine.decoder_priorities.spudec_reel \n");
       /* TB: xine should not do cddb-lookups - hangs if there's no internet-connection */
      if (xine_config_lookup_entry(xine_, "media.audio_cd.use_cddb",&entry) == 1) {
        entry.num_value = 0;
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key: media.audio_cd.use_cddb \n");
      if (xine_config_lookup_entry(xine_, "media.network.timeout",&entry) == 1) {
        entry.num_value = 4; // 4 secs
        xine_config_update_entry(xine_, &entry);
      } else
        printf (" ---  could not find entry-key: media.network.timeout \n");
    } // XineLib::SetXineConfig

    void XineLib::PrintXineConfig() NO_THROW {
      try {
        REEL_ASSERT(xine_);
        xine_cfg_entry_t entry;
        if (!xine_config_get_first_entry(xine_, &entry))
          printf(" error \n");
        else
          printf ("---  cfg - key \"%s\" type %d \n", entry.key, entry.type);
        while (xine_config_get_next_entry(xine_, &entry)) {
          printf ("---  cfg - key \"%s\"  ", entry.key);
          switch (entry.type) {
            case XINE_CONFIG_TYPE_UNKNOWN:
              printf ("unknown %s  \n", entry.unknown_value);
              break;
            case XINE_CONFIG_TYPE_RANGE:
              printf ("range: (2%d - 2%d) val %3d  def %d  \n", entry.range_min, entry.range_max, entry.num_value, entry.num_default);
              break;
            case XINE_CONFIG_TYPE_STRING:
              printf ("string: %s  (default %s )\n", entry.str_value, entry.str_default);
              break;
            case XINE_CONFIG_TYPE_ENUM:
              printf ("enum : %d  (default %d )\n", entry.num_value, entry.num_default);
              break;
            case XINE_CONFIG_TYPE_NUM:
              printf ("num : %d  (default %d )\n", entry.num_value, entry.num_default);
              break;
            case XINE_CONFIG_TYPE_BOOL:
              printf ("num : %d  (default %d )\n", entry.num_value, entry.num_default);
              break;
            default:
              printf ("unknown %s  \n", entry.unknown_value);
              break;
          } // switch
        } // while
      } catch(...){}
    } // XineLib::PrintXineConfig

    void XineLib::PrintXinePlugins() NO_THROW {
      try {
        const char *const *driver_ids;
        const char *driver_id;
        printf (" Audio output plugins \n");
        if ((driver_ids = xine_list_audio_output_plugins (xine_))) {
          driver_id  = *driver_ids++;
          while (driver_id) {
            printf ("%s ", driver_id);
            driver_id  = *driver_ids++;
          } // while
        } // if
        printf (" Video  output plugins \n");
        if ((driver_ids = xine_list_video_output_plugins (xine_))) {
          driver_id  = *driver_ids++;
          while (driver_id) {
            printf ("%s ", driver_id);
            driver_id  = *driver_ids++;
          } // while
        } // if
      } catch(...){}
    } // XineLib::PrintXinePlugins

    XineError::XineError(char const *msg) NO_THROW : msg_(msg) {
      printf ("  XineError %s  \n", msg_.c_str());
    } // XineError::XineError

    char const *XineError::Msg() const NO_THROW {
      printf ("  ret XineError::Msg  %s  \n", msg_.c_str());
      return msg_.c_str();
    } // XineError::Msg

    XineLib XineLib::instance_;

    void LogXineError(XineError const &ex) NO_THROW {
      esyslog("ERROR: Xine: %s", ex.Msg());
    } // LogXineError

  } // namespace XineMediaplayer
} // namespace Reel
