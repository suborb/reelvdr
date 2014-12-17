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

#define LOG_MODULE "hde-audio"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 0
#define LOG_ERR  1

#include <xine_internal.h>
#include "hde_xine.h"
#include "hde_io.h"

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

//#define GAP_TOLERANCE        AO_MAX_GAP
#define GAP_TOLERANCE        150000

typedef struct hde_driver_class_s {
	audio_driver_class_t  audio_driver_class;
	xine_t               *xine;
	int                   instance;
	config_values_t      *config;
} hde_driver_class_t;

typedef struct hde_driver_s {
	ao_driver_t          ao_driver;
	hde_driver_class_t  *class;
    //hde_scr_t           *scr;
	int                  capabilities;
	uint32_t             num_channels;
	uint32_t             bytes_per_frame;
	uint32_t             samplerate;
	uint32_t             bits_per_sample;
	hde_frame_buffer_t   buffer;
	int64_t              time;
	int64_t              t2;
	int                  is_native;
	int                  is_init;
} hde_driver_t;

static int hde_open(ao_driver_t *this_gen, uint32_t bits, uint32_t rate, int mode) {
	llprintf ( LOG_FUNC, "hde_open\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	switch(mode) {
		case AO_CAP_MODE_MONO:
			this->num_channels = 1;
			break;
		case AO_CAP_MODE_STEREO:
			this->num_channels = 2;
			break;
	} // switch
	this->samplerate      = rate;
	this->bits_per_sample = bits;
	this->bytes_per_frame = this->num_channels * (this->bits_per_sample/8);
	this->time            = GetCurrentTime();
	this->is_native       = 0;
	this->is_init         = 0;
	return rate;
} // hde_open

static int hde_num_channels(ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_num_channels\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	return this->num_channels;
} // hde_num_channels

static int hde_bytes_per_frame(ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_bytes_per_frame\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	return this->bytes_per_frame;
} // hde_bytes_per_frame

static int hde_get_gap_tolerance (ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_get_gap_tolerance\n" );
	return GAP_TOLERANCE;
} // hde_get_gap_tolerance

static int hde_write(ao_driver_t *this_gen, int16_t *data, uint32_t num_frames) {
	llprintf ( LOG_FUNC, "hde_write\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	int64_t duration = (10000*num_frames) / (this->samplerate/100);
	this->time += duration;
	if(this->is_native)
		return num_frames;
	return num_frames;
} // hde_write

static int hde_delay (ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_delay\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
/*    if(!this->is_native)
	return 0;*/
	int64_t now = GetCurrentTime();
	if (now > this->time) {
		this->time = now;
		return 0;
	} // if
	xine_usec_sleep(this->time - now);
	return 0;
} // hde_delay

static void hde_close(ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_close\n" );
} // hde_close

static uint32_t hde_get_capabilities (ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_get_capabilities\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	return this->capabilities;
} // hde_get_capabilities

static void hde_exit(ao_driver_t *this_gen) {
	llprintf ( LOG_FUNC, "hde_exit\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	hde_close(this_gen);
    //if(this->scr) {
    //    this->class->xine->clock->unregister_scr(this->class->xine->clock, &this->scr->scr_plugin);
    //    this->scr->scr_plugin.exit(&this->scr->scr_plugin);
    //} // if
	if (this->buffer.data)
		free(this->buffer.data);
	this->class->instance = 0;
	gHdeIO.audio_used = 0;
	hde_io_done();
	free (this);
} // hde_exit

static int hde_get_property (ao_driver_t *this_gen, int property) {
	llprintf ( LOG_FUNC, "hde_get_property\n" );
	int ret = 0;
	switch(property) {
		case AO_PROP_MIXER_VOL:
		case AO_PROP_PCM_VOL:
			if(hde_io_get_vol(&ret))
				ret = (ret*100)/255;
			break;
	} // switch
	return ret;
} // hde_get_property

static int hde_set_property (ao_driver_t *this_gen, int property, int value) {
	llprintf ( LOG_FUNC, "hde_set_property\n" );
	switch(property) {
		case AO_PROP_MIXER_VOL:
		case AO_PROP_PCM_VOL:
			if(hde_io_set_vol((value*255)/100))
				return value;
			break;
	} // switch
	return ~value;
} // hde_set_property

static int hde_control(ao_driver_t *this_gen, int cmd, ...) {
	llprintf ( LOG_FUNC, "hde_control\n" );
	hde_driver_t *this = (hde_driver_t *) this_gen;
	switch(cmd) {
		case AO_CTRL_NATIVE:
			this->is_native = 1;
			break;
	} // switch
	return 0;
} // hde_control

static ao_driver_t *hde_ao_open_plugin (audio_driver_class_t *class_gen, const void *data) {
	llprintf ( LOG_FUNC, "hde_ao_open_plugin\n" );
	hde_driver_class_t *class = ( hde_driver_class_t * ) class_gen;
	if(class->instance) return NULL;
	hde_driver_t *this = ( hde_driver_t * ) xine_xmalloc ( sizeof ( hde_driver_t ) );
	if ( !this ) return NULL;
	this->ao_driver.get_capabilities     = hde_get_capabilities;
	this->ao_driver.get_property         = hde_get_property;
	this->ao_driver.set_property         = hde_set_property;
	this->ao_driver.open                 = hde_open;
	this->ao_driver.num_channels         = hde_num_channels;
	this->ao_driver.bytes_per_frame      = hde_bytes_per_frame;
	this->ao_driver.delay                = hde_delay;
	this->ao_driver.write                = hde_write;
	this->ao_driver.close                = hde_close;
	this->ao_driver.exit                 = hde_exit;
	this->ao_driver.get_gap_tolerance    = hde_get_gap_tolerance;
	this->ao_driver.control              = hde_control;
	this->class                          = class;
	this->capabilities = AO_CAP_16BITS /*| AO_CAP_MODE_MONO*/ | AO_CAP_MODE_STEREO;
	class->instance = 1;
	gHdeIO.audio_used = 1;
	hde_io_init();
    //this->scr = hde_scr_init(this->class->xine);
    //this->scr->scr_plugin.start(&this->scr->scr_plugin, this->class->xine->clock->get_current_time(this->class->xine->clock));
    //this->class->xine->clock->register_scr(this->class->xine->clock, &this->scr->scr_plugin);
	return &this->ao_driver;
} // hde_ao_open_plugin

static char *hde_ao_get_identifier ( audio_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_ao_get_identifier\n" );
	return HDE_AUDIO_ID;
} // hde_vo_get_identifier

static char *hde_ao_get_description ( audio_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_ao_get_description\n" );
	return HDE_AUDIO_DESCR;
} // hde_vo_get_description

static void hde_ao_class_dispose ( audio_driver_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_ao_class_dispose\n" );
	hde_driver_class_t *class = ( hde_driver_class_t * ) class_gen;
	gHdeIO.audio = 0;
	free ( class );
} // hde_vo_class_dispose

void *hde_audio_init(xine_t *xine, void *visual_gen) {
	llprintf ( LOG_FUNC, "hde_audio_init\n" );
	hde_driver_class_t *this;
	this = ( hde_driver_class_t * ) xine_xmalloc ( sizeof ( hde_driver_class_t ) );
	if ( !this ) return NULL;
//    this->devnum = xine->config->register_num ( xine->config, CONF_KEY, 0, CONF_NAME, CONF_HELP, 10, NULL, NULL );
	this->audio_driver_class.open_plugin     = hde_ao_open_plugin;
#if (XINE_SUB_VERSION < 90 && XINE_MINOR_VERSION < 2)
	this->audio_driver_class.get_identifier  = hde_ao_get_identifier;
	this->audio_driver_class.get_description = hde_ao_get_description;
#else
	this->audio_driver_class.identifier  = hde_ao_get_identifier;
	this->audio_driver_class.description = hde_ao_get_description;
#endif
	this->audio_driver_class.dispose         = hde_ao_class_dispose;
	this->xine                               = xine;
	this->instance                           = 0;
	gHdeIO.audio = 1;
	return &this->audio_driver_class;
} // hde_audio_init
