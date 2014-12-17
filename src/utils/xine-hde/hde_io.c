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

#define LOG_MODULE "hde-io"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 1
#define LOG_ERR  1

#include <xine_internal.h>
#include "hde_io.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include "hde_cmds.h"
#include "hde_base.h"

hde_io_t gHdeIO = {0, 0,};
/*****************************************************************************/

int hde_io_init() {
	if(hde_is_init())
		return 1;
	int video = gHdeIO.video;
	int audio = gHdeIO.audio;
	memset (&gHdeIO, 0, sizeof(hde_io_t));
	gHdeIO.video = video;
	gHdeIO.audio = audio;
	if(!hde_init()) {
		llprintf ( LOG_ERR, "hde_open_plugin:failed to initialize hde\n" );
		return 0;
	} // if
	pthread_mutex_init ( &gHdeIO.lock, NULL );
	gHdeIO.v.fd = -1;
	gHdeIO.a.fd = -1;
	gHdeIO.fb.fd = -1;
	return 1;
} // hde_io_init

void hde_io_done() {
	if (!hde_io_has_video() && !hde_io_has_audio() && hde_is_init()) {
		pthread_mutex_destroy ( &gHdeIO.lock );
		hde_done();
	} // if
} // hde_io_done

int hde_io_use_video() {
	return gHdeIO.video;
} // hde_io_use_video

int hde_io_has_video() {
	return gHdeIO.video_used;
} // hde_io_has_video

const char *videoformat2text(int format) {
	switch (format) {
		case 0                 : return "NONE";
		case HDE_PIX_FMT_MPEG1 : return "MP1";
		case HDE_PIX_FMT_MPEG2 : return "MP2";
		case HDE_PIX_FMT_MPEG4 : return "MP4";
		case HDE_PIX_FMT_H264  : return "H264";
		case HDE_PIX_FMT_WMV3  : return "WMV3";
		case HDE_PIX_FMT_WVC1  : return "VC1";
		default                : break;
	} // switch
	return "?";
} // videoformat2text

int hde_io_video_config(hde_io_video_stream_t stream) {
	if (!memcmp(&stream, &gHdeIO.v.stream, sizeof(gHdeIO.v.stream)))
		if(gLastVdecCfg.pixelformat)
			return 0;
	if(!hde_io_init())
		return 0;
	syslog(LOG_INFO, "hde_io_video_config (%x)->%x (%s)->(%s) (%dx%d)->%dx%d f:(%d/%d)->%d/%d a:(%d/%d)->%d/%d o:(%d)->%d\n",
			gHdeIO.v.stream.format, stream.format, 
			videoformat2text(gHdeIO.v.stream.format), videoformat2text(stream.format),
			gHdeIO.v.stream.width, gHdeIO.v.stream.height, 
			stream.width, stream.height,
			gHdeIO.v.stream.framerate.num, gHdeIO.v.stream.framerate.den, stream.framerate.num, stream.framerate.den,
			gHdeIO.v.stream.aspect.num, gHdeIO.v.stream.aspect.den, stream.aspect.num, stream.aspect.den,
			gHdeIO.v.stream.overscan, stream.overscan);
	hd_vdec_config_t cfg = gLastVdecCfg;
	cfg.pixelformat = stream.format;
	cfg.pic_width   = stream.width;
	cfg.pic_height  = stream.height;
	cfg.asp_width   = stream.aspect.num;
	cfg.asp_height  = stream.aspect.den;
	cfg.rate_num    = stream.framerate.num;
	cfg.rate_den    = stream.framerate.den;
	if(hde_write_video_cfg(&cfg))
		return 0;
	gHdeIO.v.stream = stream;
	return 1;
} // hde_io_video_config

int hde_io_video_frame(uint8_t *data, int32_t size, int64_t pts) {
	if(!hde_io_init())
		return 0;
	if(hde_write_video_data(data, size, pts))
		return 0;
	return 1;
} // hde_io_video_frame

int hde_io_video_reset() {
	if(!hde_io_init())
		return 0;
	if(hde_video_reset())
		return 0;
	return 1;
} // hde_io_video_reset

int hde_io_video_resync() {
	if(!hde_io_init())
		return 0;
	if(hde_video_resync())
		return 0;
	return 1;
} // hde_io_video_resync

int hde_io_use_audio() {
	return gHdeIO.audio;
} // hde_io_use_audio

int hde_io_has_audio() {
	return gHdeIO.audio_used;
} // hde_io_has_audio

const char *audioformat2text(int format) {
	switch (format) {
		case 0                      : return "NONE";
		case HDE_ADEC_FMT_MPEG      : return "MPA";
		case HDE_ADEC_FMT_AC3       : return "AC3";
		case HDE_ADEC_FMT_WMA       : return "WMA";
		case HDE_ADEC_FMT_AAC       : return "AAC";
		case HDE_ADEC_FMT_DTS       : return "DTS";
		case HDE_ADEC_FMT_PCM_S16LE : return "S16LE";
		case HDE_ADEC_FMT_PCM_S16BE : return "S16BE";
		case HDE_ADEC_FMT_PCM_U16LE : return "U16LE";
		case HDE_ADEC_FMT_PCM_U16BE : return "U16BE";
		case HDE_ADEC_FMT_PCM_S8    : return "S8";
		case HDE_ADEC_FMT_PCM_U8    : return "U8";
		case HDE_ADEC_FMT_MULAW     : return "MULAW";
		case HDE_ADEC_FMT_ALAW      : return "ALAW";
		default                     : break;
	} // switch
	return "?";
} // audioformat2text

int hde_io_audio_config(int format, int samplerate, int channels, int bitrate, int size, int extra) {
	if (format == gHdeIO.a.format && samplerate == gHdeIO.a.samplerate && channels == gHdeIO.a.channels)
		if (gLastAdecCfg.format)
			return 0;
	if(!hde_io_init())
		return 0;
	syslog(LOG_INFO, "hde_io_audio_config (%x)->%x (%s)->%s (%d)->%d ch:(%d)->%d %d-%d|%d\n",
			gHdeIO.a.format, format,
			audioformat2text(gHdeIO.a.format), audioformat2text(format),
			gHdeIO.a.samplerate, samplerate,
			gHdeIO.a.channels, channels,
			bitrate, size, extra);
	hd_adec_config_t cfg = gLastAdecCfg;
	cfg.format      = format;
	cfg.channels    = channels;
	cfg.samplerate  = samplerate;
	cfg.bitrate     = bitrate;
	cfg.bpf         = size;
	cfg.extra       = extra;

	if(hde_write_audio_cfg(&cfg))
		return 0;
	gHdeIO.a.running    = 1;
	gHdeIO.a.format     = format;
	gHdeIO.a.samplerate = samplerate;
	gHdeIO.a.channels   = channels;
	return 1;
} // hde_io_audio_config

int hde_io_audio_frame(uint8_t *data, int32_t size, int64_t pts) {
	if(!hde_io_init())
		return 0;
	int64_t speed;
	if(hde_io_get_speed(&speed) && speed > 90000)
		return 0;
	if(hde_write_audio_data(data, size, pts))
		return 0;
	return 1;
} // hde_io_audio_frame

int hde_io_audio_reset() {
	if(!hde_io_init())
		return 0;
	if(hde_audio_reset())
		return 0;
	return 1;
} // hde_io_audio_reset

int hde_io_set_speed(int64_t speed) {
	if(!hde_io_init())
		return 0;
	if(gHdeIO.speed == speed)
		return 1;
	int ret = 1;
	pthread_mutex_lock ( &gHdeIO.lock );
	if(hde_set_speed(speed))
		ret = 0;
	pthread_mutex_unlock ( &gHdeIO.lock );
	return ret;
} // hde_io_set_speed

int hde_io_get_speed(int64_t *speed) {
	if(!hde_io_init())
		return 0;
	int ret = 1;
	pthread_mutex_lock ( &gHdeIO.lock );
	if(hde_get_speed(speed))
		ret = 0;
	else
		gHdeIO.speed = *speed;
	pthread_mutex_unlock ( &gHdeIO.lock );
	return ret;
} // hde_io_get_speed

void hde_io_fb_begin() {
	if(!hde_io_init())
		return;
	if(sizeof(hdcmd_osd_overlay) != sizeof(vo_overlay_t) || sizeof(hdcmd_osd_rle_elem) != sizeof(rle_elem_t)) {
		printf ( "hde_io_fb_begin: vo_overlay_t or rle_elem_t does not match!\n");
		return;
	} // if
	//pthread_mutex_lock ( &gHdeIO.lock );
	hde_osd_xine_start();
	//pthread_mutex_unlock ( &gHdeIO.lock );
} // hde_io_fb_begin

static void convert_palette(vo_overlay_t * overlay) {
	int i, y, cb, cr, r, g, b;
	if (!overlay->rgb_clut) {
		for (i = 0; i < OVL_PALETTE_SIZE; i++) {
			y  = (overlay->color[i] >> 16) & 0xff;
			cr = (overlay->color[i] >>  8) & 0xff;
			cb = (overlay->color[i]      ) & 0xff;
			r  = 1.164 * y + 1.596 * (cr - 128);
			g  = 1.164 * y - 0.813 * (cr - 128) - 0.392 * (cb - 128);
			b  = 1.164 * y + 2.017 * (cb - 128);
			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			if (r > 0xff) r = 0xff;
			if (g > 0xff) g = 0xff;
			if (b > 0xff) b = 0xff;
			overlay->color[i] = (r << 16) | (g << 8) | b;
		} // for
		overlay->rgb_clut = 1;
	} // if
	if (!overlay->hili_rgb_clut) {
		for (i = 0; i < OVL_PALETTE_SIZE; i++) {
			y  = (overlay->hili_color[i] >> 16) & 0xff;
			cr = (overlay->hili_color[i] >>  8) & 0xff;
			cb = (overlay->hili_color[i]      ) & 0xff;
			r  = 1.164 * y + 1.596 * (cr - 128);
			g  = 1.164 * y - 0.813 * (cr - 128) - 0.392 * (cb - 128);
			b  = 1.164 * y + 2.017 * (cb - 128);
			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			if (r > 0xff) r = 0xff;
			if (g > 0xff) g = 0xff;
			if (b > 0xff) b = 0xff;
			overlay->hili_color[i] = (r << 16) | (g << 8) | b;
		} // for
		overlay->hili_rgb_clut = 1;
	} // if
} // convert_palette

void hde_io_fb_blend(alphablend_t *alphablend, vo_overlay_t *overlay) {
	if(!hde_io_init())
		return;
	if(sizeof(hdcmd_osd_overlay) != sizeof(vo_overlay_t) || sizeof(hdcmd_osd_rle_elem) != sizeof(rle_elem_t))
		return;
	//pthread_mutex_lock ( &gHdeIO.lock );
	gHdeIO.fb.count++;
	int width  = gHdeIO.v.stream.width;
	int height = gHdeIO.v.stream.height;
	int x = 0;//-overlay->x;
	int y = 0;//-overlay->y;
	hde_osd_scale(&x, &y, &width, &height,
				  gHdeIO.v.stream.width, gHdeIO.v.stream.height,
				  gHdeIO.v.stream.aspect.num, gHdeIO.v.stream.aspect.den);
	convert_palette(overlay);
	hde_osd_xine_tile((hdcmd_osd_overlay *)overlay, gHdeIO.v.stream.width, gHdeIO.v.stream.height, x, y, width, height);
	//pthread_mutex_unlock ( &gHdeIO.lock );
} // hde_io_fb_blend

void hde_io_fb_end() {
	if(!hde_io_init())
		return;
	if(sizeof(hdcmd_osd_overlay) != sizeof(vo_overlay_t) || sizeof(hdcmd_osd_rle_elem) != sizeof(rle_elem_t))
		return;
	//pthread_mutex_lock ( &gHdeIO.lock );
	hde_osd_xine_end();
	//pthread_mutex_unlock ( &gHdeIO.lock );
} // hde_io_fb_end

int hde_io_get_vol(int *vol) {
	if(hde_get_vol(vol))
		return 0;
	return 1;
} // hde_io_get_vol

int hde_io_set_vol(int vol) {
	if(!hde_io_init())
		return 0;
	if(hde_set_vol(vol))
		return 0;
	return 1;
} // hde_io_set_vol

int hde_io_get_stc(int64_t *stc) {
	if(!hde_io_init())
		return 0;
	if(hde_get_stc(stc))
		return 0;
	return 1;
} // hde_io_get_stc

int hde_io_adjust_stc(int64_t pts, int64_t *stc) {
	if(!hde_io_init())
		return 0;
	if(hde_adjust_stc(pts, stc))
		return 0;
	return 1;
} // hde_io_adjust_stc
