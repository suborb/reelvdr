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

#ifndef HAVE_HDE_BASE_H
#define HAVE_HDE_BASE_H

#ifndef __user
#define __user
#endif

#include <xine_internal.h>

#define HDE_RPC_PLAYER 2

#define HDE_VIDEO_DEVICE "/dev/video0"
#define HDE_AUDIO_DEVICE "/dev/adec0"
#define HDE_FB_DEVICE    "/dev/fb0"

#define HDE_O_WRONLY  0x0001
#define HDE_O_RW      0x0002
#define HDE_O_NOBLOCK 0x0080
#define HDE_RING_SIZE 0x400000

#define HDE_PIX_FMT_MPEG1 0x3147504d
#define HDE_PIX_FMT_MPEG2 0x3247504d
#define HDE_PIX_FMT_MPEG4 0x3447504d
#define HDE_PIX_FMT_H264  0x34363248
#define HDE_PIX_FMT_WMV3  0x33564d57
#define HDE_PIX_FMT_WVC1  0x31435657

#define HDE_MEMORY_MMAP_RING      0x80
#define HDE_PIC_FLAG_OVERSCAN     0x1
#define HDE_BUF_TYPE_VIDEO_OUTPUT 0x2

#define HDE_QUERYCAP     0x40685600
#define HDE_S_FMT        0xc0cc5605
#define HDE_G_FMT        0xc0cc5604
#define HDE_REQBUFS      0xc0145608
#define HDE_QUERYBUF     0xc0445609
#define HDE_QBUF_PTS     0xc04856c6
#define HDE_QBUF         0xc044560f
#define HDE_DQBUF_PTS    0xc04856c7
#define HDE_DQBUF        0xc0445611
#define HDE_STREAMON     0x80045612
#define HDE_STREAMOFF    0x80045613
#define HDE_FLUSH        0x200056c8
#define HDE_SKIP_TO      0x800856c9
#define HDE_SKIP_TO_PTS  0x800856ca
#define HDE_G_PARM       0xc0cc5615
#define HDE_S_PARM       0xc0cc5616
#define HDE_G_CROP       0xc014563b
#define HDE_S_CROP       0x8014563c
#define HDE_G_PIC_PARAMS 0x404856c3
#define HDE_S_PIC_PARAMS 0x804856c2
#define HDE_S_STC        0x800856c0
#define HDE_G_STC        0x400856c1

#define HDE_ADEC_S_CONFIG  0x80402501
//#define HDE_ADEC_S_CONFIG  0x803c2501
#define HDE_ADEC_G_POS     0x40102502
#define HDE_ADEC_S_SKEW    0x80042503
#define HDE_ADEC_O_CONFIG  0x80442504
#define HDE_ADEC_S_VOLUME  0x80042505
#define HDE_ADEC_FLUSH     0x20002506
#define HDE_ADEC_S_SPDIF   0x80042507

#define HDE_FB_HIDE     0x8100
#define HDE_FB_SHOW     0x8101
#define HDE_FB_RESIZE   0x8102
#define HDE_FB_MOVE     0x8103
#define HDE_FB_INIT     0x8104
#define HDE_FB_ALPHA    0x8105
#define HDE_FB_P_CFG    0x8106
#define HDE_FB_SET_VOP  0x8107
#define HDE_FB_STICKY   0x8108
#define HDE_FB_GET_VOP  0x810a
#define HDE_FB_GET_INFO 0x810b

#define HDE_PCI      0xe0000000
#define HDE_BASE     0x2000000
#define HDE_FB_BASE  0x1600000
#define HDE_FB_SIZE  0x300000

#include <linux/videodev2.h>

// __attribute__((__aligned__(8))) 
struct hde_buffer_pts
{
    __u32			index;
    enum v4l2_buf_type      type;
    __u32			bytesused;
    __u32			flags;
    enum v4l2_field		field;
    __u32			align1;
    __u64			pts;
    struct v4l2_timecode	timecode;
    __u32			sequence;

    /* memory location */
    enum v4l2_memory        memory;
    union {
        __u32           offset;
        unsigned long   userptr;
    } m;
    __u32			length;
    __u32			input;
    __u32			reserved;
};

struct hde_aspect_ratio
{
    __u16 width;
    __u16 height;
};

struct hde_pic_params
{
    struct hde_aspect_ratio par;
    __u32 flags;
    __u32 reserved[16];
};

enum hde_adec_format
{
    HDE_ADEC_FMT_MPEG = 9,
    HDE_ADEC_FMT_AC3 = 11,
    HDE_ADEC_FMT_WMA = 30,
    HDE_ADEC_FMT_AAC = 41,
    HDE_ADEC_FMT_DTS = 48,
    HDE_ADEC_FMT_PCM_S16LE = 0x10000,
    HDE_ADEC_FMT_PCM_S16BE = 0x10001,
    HDE_ADEC_FMT_PCM_U16LE = 0x10002,
    HDE_ADEC_FMT_PCM_U16BE = 0x10003,
    HDE_ADEC_FMT_PCM_S8 = 0x10004,
    HDE_ADEC_FMT_PCM_U8 = 0x10005,
    HDE_ADEC_FMT_MULAW = 0x10006,
    HDE_ADEC_FMT_ALAW = 0x10007,
};

struct hde_adec_stream_config 
{
    enum hde_adec_format format;
    __u32 channels;
    __u32 samplerate;
    __u32 out_channels;
    __u32 out_samplerate;
    __u32 downmix_mode;
    void __user *codec_config;
    __u32 codec_config_size;
    __u32 reserved[8];
};

enum hde_adec_pts_clock
{
    HDE_ADEC_PTS_AUDIO = 0,
    HDE_ADEC_PTS_STC_TSI0 = 1,
};

enum hde_adec_audio_port
{
    HDE_ADEC_AUDIO_PORT_PRMI2S = 0,
    HDE_ADEC_AUDIO_PORT_SECI2S = 1,
    HDE_ADEC_AUDIO_PORT_SPDIF = 2,
    HDE_ADEC_AUDIO_PORT_INVALID = 3,
};

struct hde_adec_output_config
{
    enum hde_adec_pts_clock pts_clock;
    enum hde_adec_audio_port audio_port;
    __u32 reserved[15];
};

struct hde_adec_position
{
    __u64 position;
    struct timeval timestamp;
};

typedef struct hde_audio_frame_s 
{
    uint64_t pts;
    uint8_t  data[32*1024];
    uint32_t len;
} hde_audio_frame_t;

#endif // HAVE_HDE_BASE_H
