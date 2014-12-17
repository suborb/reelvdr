/*
 * Copyright (C) 2000-2003 the xine project
 *
 * This file is part of xine, a free video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: video_overlay.h,v 1.21 2006/09/26 05:19:49 dgp85 Exp $
 *
 */

#ifndef REEL_XINE_EVENT_H
#define REEL_XINE_EVENT_H

#include <sys/time.h>
#include <xine.h>

#define REEL_XINE_EVENT_UI_NUM_BUTTONS XINE_EVENT_UI_NUM_BUTTONS
#define REEL_XINE_EVENT_UI_PLAYBACK_FINISHED XINE_EVENT_UI_PLAYBACK_FINISHED

namespace Reel
{
namespace XineMediaplayer
{ 
    class Player;
    
    class XineEventBase 
    {
      public:
        int                              type_;   /* event type (constants see above) */
        void /*xine_stream_t*/           *stream_; /* stream this event belongs to     */
    
        int                              data_length_;
    
        /* you do not have to provide this, it will be filled in by xine_event_send() */
        struct timeval                   tv_;     /* timestamp of event creation */   

        //xine_ui_data_t                   data_;  don`t ask
        void CopyEventBase(const xine_event_t *event);
        virtual void CopyEvent(const  xine_event_t *event) = 0;
        virtual bool CompareStream(void /*xine_stream_t*/ *stream_);

        virtual ~XineEventBase() {} ; 

        //virtual void Handle(Player &player) = 0;
      
    };
    
    class EventUIPlaybackFinished : public XineEventBase
    {
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventUIChannelsChanged : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventUISetTitle : public XineEventBase
    {
      public:
        xine_ui_data_t                   data_; 
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventUIMessage : public XineEventBase
    {
      public:
        xine_ui_data_t                   data_; 
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventUINumButtons : public XineEventBase
    {
      public:
        xine_ui_data_t                   data_; 
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

     /// 
    class EventFormatChange : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventAudioLevel : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventProgress : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventSpuButton : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventDroppendFrames : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventMrlReferenceExt : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };

    class EventQuit : public XineEventBase
    {
      public:
        /*override*/ void CopyEvent(const  xine_event_t *event);
        // /*override*/ void Handle(Player &player);
    };
 
    // INLINE  definitions 
    
    inline void XineEventBase::CopyEventBase(const xine_event_t *event)
    {
        type_ = event->type;
        stream_ = event->stream; //no deep copy! (unused)
        data_length_ = event->data_length;
        tv_ = event->tv;
    }
    inline bool XineEventBase::CompareStream(void /*xine_stream_t*/ *stream)
    {
           return stream_ == stream;
    }
    
    //-------------------------------------------------------------------- 
    
    inline void EventUIPlaybackFinished::CopyEvent(const xine_event_t *event)
    {
        printf (" EventUIPlaybackFinished CopyEvent\n");
        if (event->data) 
          printf (" EventUIPlaybackFinished has data \n");
        else 
          printf (" EventUIPlaybackFinished has NO data \n");

        CopyEventBase(event);
    } 

    inline void EventUIChannelsChanged::CopyEvent(const xine_event_t *event)
    {
        if (event->data) 
          printf (" EventUIChannelsChanged  has data \n");
        CopyEventBase(event);
    } 

    inline void EventUISetTitle::CopyEvent(const xine_event_t *event)
    {
        if (event->data)  {
          data_ = *(static_cast<xine_ui_data_t *>(event->data));
          //printf (" EventUISetTitle   has data \n");
          //printf (" ----------------- --------------  TITLE STR:???  \"%s\" \n", data_ptr->str);
        }
        CopyEventBase(event);
    } 

    inline void EventUIMessage::CopyEvent(const xine_event_t *event)
    {
        if (event->data) 
          printf (" EventUIMessage    has data \n");
        data_ = *(static_cast<xine_ui_data_t *>(event->data));
        CopyEventBase(event);
    } 

    inline void EventUINumButtons::CopyEvent(const xine_event_t *event)
    {
        if (event->data) 
          printf (" EventUINumButtons    has data \n");
        //data_ = *(static_cast<xine_ui_data_t *>(event->data));
        CopyEventBase(event);
    } 

    inline void EventFormatChange::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventAudioLevel::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventProgress::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventSpuButton::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventDroppendFrames::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventQuit::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 

    inline void EventMrlReferenceExt::CopyEvent(const xine_event_t *event)
    {
        CopyEventBase(event);
    } 
}
}

#endif //REEL_XINE_EVENT_H
