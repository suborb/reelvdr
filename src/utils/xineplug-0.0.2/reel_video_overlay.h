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
#include "sys/time.h"

#ifdef XINE_COMPILE
#  include "xine_internal.h"
#else
#  include <xine_internal.h>
#endif

#ifdef	__GNUC__
#define CLUT_Y_CR_CB_INIT(_y,_cr,_cb)	{y: (_y), cr: (_cr), cb: (_cb)}
#else
#define CLUT_Y_CR_CB_INIT(_y,_cr,_cb)	{ (_cb), (_cr), (_y) }
#endif

#define MAX_OBJECTS   50
#define MAX_EVENTS    50
#define MAX_SHOWING   16

#define OVERLAY_EVENT_NULL             0
#define OVERLAY_EVENT_SHOW             1
#define OVERLAY_EVENT_HIDE             2
#define OVERLAY_EVENT_MENU_BUTTON      3
#define OVERLAY_EVENT_FREE_HANDLE      8 /* Frees a handle, previous allocated via get_handle */

/* number of colors in the overlay palette. Currently limited to 256
   at most, because some alphablend functions use an 8-bit index into
   the palette. This should probably be classified as a bug. */
/* FIXME: Also defines in video_out.h */
#define OVL_PALETTE_SIZE 256

struct spudec_decoder_t;

typedef struct reel_rle_elem_s
{
  uint16_t len;
  uint16_t color;
} reel_rle_elem_t ;

typedef struct reel_vo_overlay_s 
{
  reel_rle_elem_t       *rle;           /* rle code buffer                  */  
  int               data_valid;    /*additional element*/
  int               data_size;     /* useful for deciding realloc      */
  int               num_rle;       /* number of active rle codes       */
  int               x;             /* x start of subpicture area       */
  int               y;             /* y start of subpicture area       */
  int               width;         /* width of subpicture area         */
  int               height;        /* height of subpicture area        */
  
  uint32_t          color[OVL_PALETTE_SIZE];  /* color lookup table     */
  uint8_t           trans[OVL_PALETTE_SIZE];  /* mixer key table        */
  int               rgb_clut;      /* true if clut was converted to rgb */

  /* define a highlight area with different colors */
  int               hili_top;
  int               hili_bottom;
  int               hili_left;
  int               hili_right;
  uint32_t          hili_color[OVL_PALETTE_SIZE];
  uint8_t           hili_trans[OVL_PALETTE_SIZE];
  int               hili_rgb_clut; /* true if clut was converted to rgb */
  
  int               unscaled;      /* true if it should be blended unscaled */
}reel_vo_overlay_t;

/*
 * xine event struct
 */
typedef struct reel_xine_event_s 
{
  int                              type;   /* event type (constants see above) */
  void/*xine_stream_t*/           *stream; /* stream this event belongs to     */

  void                            *data;   /* contents depending on type */
  int                              data_length;

  /* you do not have to provide this, it will be filled in by xine_event_send() */
  struct timeval                   tv;     /* timestamp of event creation */
}reel_xine_event_t ;

typedef struct  reel_vo_buttons_s 
{
  int32_t           type;        /*  0:Button not valid, 
                                     1:Button Valid, no auto_action,
                                     2:Button Valid, auto_action.
                                  */

  /* The following clipping coordinates are relative to the left upper corner
   * of the OVERLAY, not of the target FRAME. Please do not mix them up! */
  int32_t           hili_top;
  int32_t           hili_bottom;
  int32_t           hili_left;
  int32_t           hili_right;
  int32_t           up; 
  int32_t           down; 
  int32_t           left; 
  int32_t           right; 
  uint32_t          select_color[OVL_PALETTE_SIZE];
  uint8_t           select_trans[OVL_PALETTE_SIZE];
  reel_xine_event_t      select_event;
  uint32_t          active_color[OVL_PALETTE_SIZE];
  uint8_t           active_trans[OVL_PALETTE_SIZE];
  reel_xine_event_t      active_event;
  int32_t           hili_rgb_clut;      /* true if clut was converted to rgb*/
                                        /* FIXME: Probably not needed ^^^ */
}reel_vo_buttons_t; 
  
typedef struct reel_video_overlay_object_s 
{
  int32_t	 handle;       /* Used to match Show and Hide events. */
  uint32_t	 object_type;  /* 0=Subtitle, 1=Menu */
  int64_t        pts;          /* Needed for Menu button compares */
  reel_vo_overlay_t  *overlay;      /* The image data. */
  uint32_t       palette_type; /* 1 Y'CrCB, 2 R'G'B' */ 
  uint32_t       palette_exists;/*additional!!!*/ 
  uint32_t	*palette;      /* If NULL, no palette contained in this event. */
  int32_t        buttonN;      /* Current highlighed button.  0 means no info on which button to higlight */
                               /*                            -1 means don't use this button info. */
  reel_vo_buttons_t   button[32];   /* Info regarding each button on the overlay */

} reel_video_overlay_object_t; 

/* This will hold all details of an event item, needed for event queue to function */ 
typedef struct reel_video_overlay_event_s 
{
  uint32_t	 event_type;  /* Show SPU, Show OSD, Hide etc. */
  int64_t	 vpts;        /* Time when event will action. 0 means action now */
/* Once video_out blend_yuv etc. can take rle_elem_t with Colour, blend and length information.
 * we can remove clut and blend from this structure.
 * This will allow for many more colours for OSD.
 */
  reel_video_overlay_object_t   object; /* The image data. */ 
  struct spudec_decoder_s      *decoder_instance; /*decoder instance for metronom call, additional*/
} reel_video_overlay_event_t ;


