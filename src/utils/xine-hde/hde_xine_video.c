/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia Vertriebs GmbH                  *
 *                         E-Mail  : info @ reel-multimedia.com            *
 *                         Internet: www.reel-multimedia.com               *
 *                                                                         *
 *   This code is free software; you can redistribute it and/or modify     *
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define LOG_MODULE "hde-video"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 0
#define LOG_ERR  1

#include <xine_internal.h>
#include "hde_xine.h"
#include "hde_io.h"
#include "vo_scale.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

typedef struct hde_frame_s {
	vo_frame_t       vo_frame;
} hde_frame_t;

typedef struct hde_driver_class_s {
	video_driver_class_t  video_driver_class;
	xine_t               *xine;
	int                   instance;
	int                   type;
	config_values_t      *config;
} hde_driver_class_t;

typedef struct hde_driver_s {
	vo_driver_t          vo_driver;
	hde_driver_class_t  *class;
	alphablend_t         alphablend;
	int                  ov_changed;
} hde_driver_t;

static void hde_frame_proc_frame ( vo_frame_t *frame_gen ) {
	llprintf ( LOG_FUNC, "hde_frame_proc_frame\n" );
	/* we reduce the vpts to give the card some extra decoding time */
    /*
	if (frame_gen->format != XINE_IMGFMT_DXR3 && !frame_gen->proc_called)
	frame_gen->vpts -= DECODE_PIPE_PREBUFFER;

	frame_gen->proc_called = 1;
	*/
	frame_gen->proc_called = 1;
} // hde_frame_proc_frame

static void hde_frame_proc_slice ( vo_frame_t *frame_gen, uint8_t **src ) {
	llprintf ( LOG_FUNC, "hde_frame_proc_slice\n" );

// hde_frame_t *frame = (hde_frame_t *)frame_gen;
// hde_driver_t *this = (hde_driver_t *)frame_gen->driver;
	/* we reduce the vpts to give the card some extra decoding time */
    /*
	if (frame_gen->format != XINE_IMGFMT_DXR3 && !frame_gen->proc_called)
	frame_gen->vpts -= DECODE_PIPE_PREBUFFER;

	frame_gen->proc_called = 1;

	if (frame_gen->format != XINE_IMGFMT_DXR3 && this->enc && this->enc->on_frame_copy)
	this->enc->on_frame_copy(this, frame, src);
	*/
	frame_gen->proc_called = 1;
} // hde_frame_proc_slice

static void hde_frame_field ( vo_frame_t *vo_img, int which_field ) {
	llprintf ( LOG_FUNC, "hde_frame_field\n" );
	/* dummy function */
} // hde_frame_field

static void hde_frame_dispose ( vo_frame_t *frame_gen ) {
	llprintf ( LOG_FUNC, "hde_frame_dispose\n" );
	hde_frame_t *frame = ( hde_frame_t * ) frame_gen;
	if (frame->vo_frame.base[0]) free(frame->vo_frame.base[0]);
	if (frame->vo_frame.base[1]) free(frame->vo_frame.base[1]);
	if (frame->vo_frame.base[2]) free(frame->vo_frame.base[2]);
	pthread_mutex_destroy ( &frame->vo_frame.mutex );
	free ( frame );
} // hde_frame_dispose

static uint32_t hde_get_capabilities ( vo_driver_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_get_capabilities\n" );
	return VO_CAP_YV12 | VO_CAP_YUY2;
} // hde_get_capabilities

static vo_frame_t *hde_alloc_frame ( vo_driver_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_alloc_frame\n" );
	hde_frame_t *frame;
	//hde_driver_t *this = (hde_driver_t *)this_gen;
	frame = ( hde_frame_t * ) xine_xmalloc ( sizeof ( hde_frame_t ) );
	pthread_mutex_init ( &frame->vo_frame.mutex, NULL );
	frame->vo_frame.proc_frame = hde_frame_proc_frame;
	frame->vo_frame.proc_slice = hde_frame_proc_slice;
	frame->vo_frame.field      = hde_frame_field;
	frame->vo_frame.dispose    = hde_frame_dispose;
	frame->vo_frame.driver     = this_gen;
	return &frame->vo_frame;
} // hde_alloc_frame

static void hde_update_frame_format ( vo_driver_t *this_gen, vo_frame_t *frame_gen, uint32_t width, uint32_t height, double ratio, int format, int flags ) {
	llprintf ( LOG_FUNC, "hde_update_frame_format w:%d h:%d r:%f f:%c%c%c%c flags:%d\n", width, height, ratio, ( format&0xff ), ( format&0xff00 ) >>8, ( format&0xff0000 ) >>16, ( format&0xff000000 ) >>24, flags );
	hde_frame_t *frame = (hde_frame_t *)frame_gen;
//    hde_driver_t *this = (hde_driver_t *)frame_gen->driver;
	if ( format == XINE_IMGFMT_HDE ) {
		frame->vo_frame.width  = width;
		frame->vo_frame.height = height;
		frame->vo_frame.ratio  = ratio;
	} else {
		if (frame->vo_frame.base[0]) free(frame->vo_frame.base[0]);
		if (frame->vo_frame.base[1]) free(frame->vo_frame.base[1]);
		if (frame->vo_frame.base[2]) free(frame->vo_frame.base[2]);
		frame->vo_frame.width  = width;
		frame->vo_frame.height = height;
		frame->vo_frame.ratio  = ratio;
		if(format == XINE_IMGFMT_YUY2) {
			frame->vo_frame.pitches[0] = 32 * ((width+15)/16);
			frame->vo_frame.base[0] = xine_xmalloc(frame->vo_frame.pitches[0]*height);
			frame->vo_frame.base[1] = frame->vo_frame.base[2] = 0;
		} else if (format == XINE_IMGFMT_YV12) {
			frame->vo_frame.pitches[0] = 16 * ((width+15)/16);
			frame->vo_frame.pitches[1] = 8 * ((width+15)/16);
			frame->vo_frame.pitches[2] = 8 * ((width+15)/16);
			frame->vo_frame.base[0] = xine_xmalloc(frame->vo_frame.pitches[0]*height);
			frame->vo_frame.base[1] = xine_xmalloc(frame->vo_frame.pitches[1]*height);
			frame->vo_frame.base[2] = xine_xmalloc(frame->vo_frame.pitches[2]*height);
		} // if
	} // if
} // hde_update_frame_format

static void hde_overlay_begin ( vo_driver_t *this_gen, vo_frame_t *frame_gen, int changed ) {
	llprintf ( LOG_FUNC, "hde_overlay_begin\n" );
	hde_driver_t *this = (hde_driver_t *)frame_gen->driver;
	this->ov_changed = changed;
	if(this->ov_changed)
		hde_io_fb_begin();
} // hde_overlay_begin

static void hde_overlay_blend ( vo_driver_t *this_gen, vo_frame_t *frame_gen, vo_overlay_t *overlay ) {
	llprintf ( LOG_FUNC, "hde_overlay_blend\n" );
	hde_frame_t *frame = (hde_frame_t *)frame_gen;
	hde_driver_t *this = (hde_driver_t *)frame_gen->driver;
	if(!this->ov_changed)
		return;
	this->alphablend.offset_x = frame->vo_frame.overlay_offset_x;
	this->alphablend.offset_y = frame->vo_frame.overlay_offset_y;
	hde_io_fb_blend(&this->alphablend, overlay);
} // hde_overlay_blend

static void hde_overlay_end ( vo_driver_t *this_gen, vo_frame_t *frame_gen ) {
	llprintf ( LOG_FUNC, "hde_overlay_end\n" );
	hde_driver_t *this = (hde_driver_t *)frame_gen->driver;
	if(!this->ov_changed)
		return;
	hde_io_fb_end();
} // hde_overlay_end

static void hde_display_frame ( vo_driver_t *this_gen, vo_frame_t *frame_gen ) {
	llprintf ( LOG_FUNC, "hde_display_frame\n" );
	hde_frame_t *frame = (hde_frame_t *)frame_gen;
	if ( frame->vo_frame.format == XINE_IMGFMT_HDE ) {
		frame->vo_frame.format = XINE_IMGFMT_HDE;
		frame_gen->free(frame_gen);
	} else {
		frame_gen->free(frame_gen);
	} // if
} // hde_display_frame

static int hde_redraw_needed ( vo_driver_t *this_gen ) {
// llprintf(LOG_FUNC, "hde_redraw_needed\n");
	xine_usec_sleep(10);
	return 0;
} // hde_redraw_needed

static int hde_get_property ( vo_driver_t *this_gen, int property ) {
	llprintf ( LOG_FUNC, "hde_get_property %d\n", property );
	hde_driver_t *this = ( hde_driver_t * ) this_gen;
	switch ( property ) {
		case VO_PROP_SATURATION:
		case VO_PROP_CONTRAST:
		case VO_PROP_BRIGHTNESS:
			return 0;
		case VO_PROP_ASPECT_RATIO:
			return XINE_VO_ASPECT_AUTO;
		case VO_PROP_COLORKEY:
			return 0;
		case VO_PROP_ZOOM_X:
		case VO_PROP_ZOOM_Y:
		case VO_PROP_TVMODE:
			return 0;
		case VO_PROP_WINDOW_WIDTH:
			return 720;
		case VO_PROP_WINDOW_HEIGHT:
			return 576;
		case VO_PROP_MAX_NUM_FRAMES:
			return 0; // default
	} // switch
	xprintf ( this->class->xine, XINE_VERBOSITY_DEBUG, "hde_get_property: property %d not implemented.\n", property );
	return 0;
} // hde_get_property

static int hde_set_property ( vo_driver_t *this_gen, int property, int value ) {
	llprintf ( LOG_FUNC, "hde_set_property %d=%d\n", property, value );
	return 0;
} // hde_set_property

static void hde_get_property_min_max ( vo_driver_t *this_gen, int property, int *min, int *max ) {
	llprintf ( LOG_FUNC, "hde_get_property_min_max %d\n", property );
	switch ( property ) {
		case VO_PROP_SATURATION:
		case VO_PROP_CONTRAST:
		case VO_PROP_BRIGHTNESS:
			*min = 0;
			*max = 1000;
			break;
		default:
			*min = 0;
			*max = 0;
	} // switch
} // hde_get_property_min_max

static int hde_gui_data_exchange ( vo_driver_t *this_gen, int data_type, void *data ) {
	llprintf ( LOG_FUNC, "hde_gui_data_exchange\n" );
	return 0;
} // hde_gui_data_exchange

static void hde_dispose ( vo_driver_t *this_gen ) {
	hde_driver_t *this = ( hde_driver_t * ) this_gen;
	_x_alphablend_free(&this->alphablend);
	this->class->instance = 0;
	gHdeIO.video_used = 0;
	hde_io_done();
	free ( this );
} // hde_dispose

static vo_driver_t *hde_vo_open_plugin ( video_driver_class_t *class_gen, const void *visual_gen ) {
	llprintf ( LOG_FUNC, "hde_vo_open_plugin\n" );
	hde_driver_t *this;
	hde_driver_class_t *class = ( hde_driver_class_t * ) class_gen;
	if (class->instance) return NULL;
	this = ( hde_driver_t * ) xine_xmalloc ( sizeof ( hde_driver_t ) );
	if ( !this ) return NULL;
	this->vo_driver.get_capabilities     = hde_get_capabilities;
	this->vo_driver.alloc_frame          = hde_alloc_frame;
	this->vo_driver.update_frame_format  = hde_update_frame_format;
	this->vo_driver.overlay_begin        = hde_overlay_begin;
	this->vo_driver.overlay_blend        = hde_overlay_blend;
	this->vo_driver.overlay_end          = hde_overlay_end;
	this->vo_driver.display_frame        = hde_display_frame;
	this->vo_driver.redraw_needed        = hde_redraw_needed;
	this->vo_driver.get_property         = hde_get_property;
	this->vo_driver.set_property         = hde_set_property;
	this->vo_driver.get_property_min_max = hde_get_property_min_max;
	this->vo_driver.gui_data_exchange    = hde_gui_data_exchange;
	this->vo_driver.dispose              = hde_dispose;
	this->class                          = class;
	_x_alphablend_init(&this->alphablend, class->xine);
	class->instance = 1;
	gHdeIO.video_used = 1;
	hde_io_init();
	return &this->vo_driver;
} // hde_vo_open_plugin

static char *hde_vo_get_identifier ( video_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_vo_get_identifier\n" );
	hde_driver_class_t *class = ( hde_driver_class_t * ) class_gen;
	switch(class->type) {
		case XINE_VISUAL_TYPE_X11 : return HDE_VIDEO_ID_X11;
		case XINE_VISUAL_TYPE_AA  : return HDE_VIDEO_ID_AA;
		case XINE_VISUAL_TYPE_FB  : return HDE_VIDEO_ID_FB;
	} // switch
	return HDE_VIDEO_ID;
} // hde_vo_get_identifier

static char *hde_vo_get_description ( video_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_vo_get_description\n" );
	return HDE_VIDEO_DESCR;
} // hde_vo_get_description

static void hde_vo_class_dispose ( video_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_vo_class_dispose\n" );
	hde_driver_class_t *class = ( hde_driver_class_t * ) class_gen;
	gHdeIO.video = 0;
	hde_io_done();
	free ( class );
} // hde_vo_class_dispose

void *hde_video_init ( xine_t *xine, void *visual_gen, int type) {
	llprintf ( LOG_FUNC, "hde_video_init\n" );
	hde_driver_class_t *this;
	this = ( hde_driver_class_t * ) xine_xmalloc ( sizeof ( hde_driver_class_t ) );
	if ( !this ) return NULL;
	//this->devnum = xine->config->register_num ( xine->config, CONF_KEY, 0, CONF_NAME, CONF_HELP, 10, NULL, NULL );
	this->video_driver_class.open_plugin     = hde_vo_open_plugin;
#if (XINE_SUB_VERSION < 90 && XINE_MINOR_VERSION < 2)
	this->video_driver_class.get_identifier  = hde_vo_get_identifier;
	this->video_driver_class.get_description = hde_vo_get_description;
#else
	this->video_driver_class.identifier  = hde_vo_get_identifier;
	this->video_driver_class.description = hde_vo_get_description;
#endif
	this->video_driver_class.dispose         = hde_vo_class_dispose;
	this->xine                               = xine;
	this->instance                           = 0;
	this->type                               = type;
	gHdeIO.video = 1;
	hde_io_init();
	return &this->video_driver_class;
} // hde_video_init

void *hde_video_init_x11 ( xine_t *xine, void *visual_gen ) {
	return hde_video_init(xine, visual_gen, XINE_VISUAL_TYPE_X11);
} // hde_video_init_x11

void *hde_video_init_aa ( xine_t *xine, void *visual_gen ) {
	return hde_video_init(xine, visual_gen, XINE_VISUAL_TYPE_AA);
} // hde_video_init_aa

void *hde_video_init_fb ( xine_t *xine, void *visual_gen ) {
	return hde_video_init(xine, visual_gen, XINE_VISUAL_TYPE_FB);
} // hde_video_init_fb
