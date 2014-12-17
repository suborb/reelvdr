/************
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

#ifndef HAVE_HDE_IO_H
#define HAVE_HDE_IO_H

#include <xine_internal.h>
#include <hdchannel.h>
#include "hde_tool.h"
#include "hde_base.h"

typedef struct hde_io_video_stream_s {
    uint32_t format;
    uint32_t width;
    uint32_t height;
    AVRational framerate;
//    AVRational timebase;
    AVRational aspect;
//    uint32_t aspect_idx;
    uint32_t overscan;
} hde_io_video_stream_t;

typedef struct hde_io_video_s {
    int fd;
    int mem;
    int mem_size;
    int running;
    int stc_init;
    int64_t last_read;
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t block_pos;
    uint32_t block_size;
    int64_t block_pts;
    int64_t last_block_pts;
    hde_io_video_stream_t stream;
} hde_io_video_t;

typedef struct hde_io_audio_s {
    int fd;
    int running;
    hde_audio_frame_t frame;
    uint64_t read_pos;
    int format;
    int samplerate; 
    int channels;
} hde_io_audio_t;

typedef struct hde_io_fb_s {
    int fd;
    uint8_t *mem;
    uint8_t *buf;
    int size;
    int bbp;
    int width;
    int height;
    int count;
} hde_io_fb_t;

typedef struct hde_io_s {
    int video;
    int video_used;
    int audio;
    int audio_used;
    int64_t speed;
    pthread_mutex_t lock;
    hde_io_video_t v;
    hde_io_audio_t a;
    hde_io_fb_t fb;
} hde_io_t;

extern hde_io_t gHdeIO;

int hde_io_init(void);
void hde_io_done(void);

int hde_io_video_config(hde_io_video_stream_t stream);
int hde_io_video_frame(uint8_t *data, int32_t size, int64_t pts);
int hde_io_video_reset(void);
int hde_io_use_video(void);
int hde_io_has_video(void);

int hde_io_audio_config(int format, int samplerate, int channels, int bitrate, int size, int extra);
int hde_io_audio_frame(uint8_t *data, int32_t size, int64_t pts);
int hde_io_audio_reset(void);
int hde_io_use_audio(void);
int hde_io_has_audio(void);

int hde_io_video_resync(void);

void hde_io_fb_begin(void);
void hde_io_fb_blend(alphablend_t *alphablend, vo_overlay_t *overlay);
void hde_io_fb_end(void);

int hde_io_set_speed(int64_t speed);
int hde_io_get_speed(int64_t *speed);

int hde_io_get_vol(int *vol);
int hde_io_set_vol(int vol);

int hde_io_get_stc(int64_t *stc);
int hde_io_adjust_stc(int64_t pts, int64_t *stc);
#endif // HAVE_HDE_IO_H
