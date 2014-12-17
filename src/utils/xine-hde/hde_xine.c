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

#define LOG_MODULE "hde-xine"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 1
#define LOG_ERR  1

#include <xine_internal.h>
#include "hde_xine.h"

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


#define OUTPUT_PRIO 1
#define MAX_PRIO    100000
// User has to set custom prio to greater than 100 to use other decoders

static const vo_info_t   hde_video_info_x11 = {
    OUTPUT_PRIO,         /* priority        */
    XINE_VISUAL_TYPE_X11  /* visual type     */
}; // hde_video_info_x11

static const vo_info_t   hde_video_info_aa = {
    OUTPUT_PRIO,         /* priority        */
    XINE_VISUAL_TYPE_AA  /* visual type     */
}; // hde_video_info_aa

static const vo_info_t   hde_video_info_fb = {
    OUTPUT_PRIO,         /* priority        */
    XINE_VISUAL_TYPE_FB  /* visual type     */
}; // hde_video_info_fb

static const ao_info_t hde_audio_info = {
    OUTPUT_PRIO          /* priority        */
}; // hde_audio_info

uint32_t hde_vd_supported_types[] = {	BUF_VIDEO_MPEG,
										BUF_VIDEO_MPEG4, BUF_VIDEO_XVID, BUF_VIDEO_DIVX5, BUF_VIDEO_3IVX,
										BUF_VIDEO_H264, BUF_VIDEO_FLV1,
										0 };
static const decoder_info_t hde_vd_info = {
    hde_vd_supported_types, /* supported types */
    MAX_PRIO                /* priority        */
};

uint32_t hde_ad_supported_types[] = { BUF_AUDIO_MPEG, BUF_AUDIO_A52, BUF_AUDIO_LPCM_BE, BUF_AUDIO_DTS, /*BUF_AUDIO_AAC, */ 0 };
uint32_t hde_ad_all_types[] = { 
	0x03000000, 0x03010000, 0x03020000, 0x03030000, 0x03040000, 0x03050000, 0x03060000, 0x03070000, 0x03080000, 0x03090000, 0x030A0000, 0x030B0000, 0x030C0000, 0x030D0000, 0x030E0000, 0x030F0000,
	0x03100000, 0x03110000, 0x03120000, 0x03130000, 0x03140000, 0x03150000, 0x03160000, 0x03170000, 0x03180000, 0x03190000, 0x031A0000, 0x031B0000, 0x031C0000, 0x031D0000, 0x031E0000, 0x031F0000,
	0x03200000, 0x03210000, 0x03220000, 0x03230000, 0x03240000, 0x03250000, 0x03260000, 0x03270000, 0x03280000, 0x03290000, 0x032A0000, 0x032B0000, 0x032C0000, 0x032D0000, 0x032E0000, 0x032F0000,
	0x03300000, 0x03310000, 0x03320000, 0x03330000, 0x03340000, 0x03350000, 0x03360000, 0x03370000, 0x03380000, 0x03390000, 0x033A0000, 0x033B0000, 0x033C0000, 0x033D0000, 0x033E0000, 0x033F0000,
	0x03400000, 0x03410000, 0x03420000, 0x03430000, 0x03440000, 0x03450000, 0x03460000, 0x03470000, 0x03480000, 0x03490000, 0x034A0000, 0x034B0000, 0x034C0000, 0x034D0000, 0x034E0000, 0x034F0000,
	0x03500000, 0x03510000, 0x03520000, 0x03530000, 0x03540000, 0x03550000, 0x03560000, 0x03570000, 0x03580000, 0x03590000, 0x035A0000, 0x035B0000, 0x035C0000, 0x035D0000, 0x035E0000, 0x035F0000,
	0x03600000, 0x03610000, 0x03620000, 0x03630000, 0x03640000, 0x03650000, 0x03660000, 0x03670000, 0x03680000, 0x03690000, 0x036A0000, 0x036B0000, 0x036C0000, 0x036D0000, 0x036E0000, 0x036F0000,
	0x03700000, 0x03710000, 0x03720000, 0x03730000, 0x03740000, 0x03750000, 0x03760000, 0x03770000, 0x03780000, 0x03790000, 0x037A0000, 0x037B0000, 0x037C0000, 0x037D0000, 0x037E0000, 0x037F0000,
	0x03800000, 0x03810000, 0x03820000, 0x03830000, 0x03840000, 0x03850000, 0x03860000, 0x03870000, 0x03880000, 0x03890000, 0x038A0000, 0x038B0000, 0x038C0000, 0x038D0000, 0x038E0000, 0x038F0000,
	0x03900000, 0x03910000, 0x03920000, 0x03930000, 0x03940000, 0x03950000, 0x03960000, 0x03970000, 0x03980000, 0x03990000, 0x039A0000, 0x039B0000, 0x039C0000, 0x039D0000, 0x039E0000, 0x039F0000,
	0x03A00000, 0x03A10000, 0x03A20000, 0x03A30000, 0x03A40000, 0x03A50000, 0x03A60000, 0x03A70000, 0x03A80000, 0x03A90000, 0x03AA0000, 0x03AB0000, 0x03AC0000, 0x03AD0000, 0x03AE0000, 0x03AF0000,
	0x03B00000, 0x03B10000, 0x03B20000, 0x03B30000, 0x03B40000, 0x03B50000, 0x03B60000, 0x03B70000, 0x03B80000, 0x03B90000, 0x03BA0000, 0x03BB0000, 0x03BC0000, 0x03BD0000, 0x03BE0000, 0x03BF0000,
	0x03C00000, 0x03C10000, 0x03C20000, 0x03C30000, 0x03C40000, 0x03C50000, 0x03C60000, 0x03C70000, 0x03C80000, 0x03C90000, 0x03CA0000, 0x03CB0000, 0x03CC0000, 0x03CD0000, 0x03CE0000, 0x03CF0000,
	0x03D00000, 0x03D10000, 0x03D20000, 0x03D30000, 0x03D40000, 0x03D50000, 0x03D60000, 0x03D70000, 0x03D80000, 0x03D90000, 0x03DA0000, 0x03DB0000, 0x03DC0000, 0x03DD0000, 0x03DE0000, 0x03DF0000,
	0x03E00000, 0x03E10000, 0x03E20000, 0x03E30000, 0x03E40000, 0x03E50000, 0x03E60000, 0x03E70000, 0x03E80000, 0x03E90000, 0x03EA0000, 0x03EB0000, 0x03EC0000, 0x03ED0000, 0x03EE0000, 0x03EF0000,
	0x03F00000, 0x03F10000, 0x03F20000, 0x03F30000, 0x03F40000, 0x03F50000, 0x03F60000, 0x03F70000, 0x03F80000, 0x03F90000, 0x03FA0000, 0x03FB0000, 0x03FC0000, 0x03FD0000, 0x03FE0000, 0 };
static const decoder_info_t hde_ad_info = {
	hde_ad_supported_types, /* supported types */
	MAX_PRIO                /* priority        */
};
static const decoder_info_t hde_ad_hook_info = {
	hde_ad_all_types, /* supported types */
	MAX_PRIO          /* priority        */
};

const plugin_info_t xine_plugin_info[] = {
    /* type, API, "name", version, special_info, init_function */
	{ PLUGIN_VIDEO_OUT,     VIDEO_OUT_DRIVER_IFACE_VERSION, HDE_VIDEO_ID_X11, XINE_VERSION_CODE, &hde_video_info_x11, &hde_video_init_x11 },
	{ PLUGIN_VIDEO_OUT,     VIDEO_OUT_DRIVER_IFACE_VERSION, HDE_VIDEO_ID_AA,  XINE_VERSION_CODE, &hde_video_info_aa,  &hde_video_init_aa  },
	{ PLUGIN_VIDEO_OUT,     VIDEO_OUT_DRIVER_IFACE_VERSION, HDE_VIDEO_ID_FB,  XINE_VERSION_CODE, &hde_video_info_fb,  &hde_video_init_fb  },
	{ PLUGIN_AUDIO_OUT,     AUDIO_OUT_IFACE_VERSION,        HDE_AUDIO_ID,     XINE_VERSION_CODE, &hde_audio_info,     &hde_audio_init },
	{ PLUGIN_VIDEO_DECODER, VIDEO_DECODER_IFACE_VERSION,    HDE_VD_ID,        XINE_VERSION_CODE, &hde_vd_info,        &hde_vd_init },
	{ PLUGIN_AUDIO_DECODER, AUDIO_DECODER_IFACE_VERSION,    HDE_AD_ID,        XINE_VERSION_CODE, &hde_ad_info,        &hde_ad_init },
	{ PLUGIN_AUDIO_DECODER, AUDIO_DECODER_IFACE_VERSION,    HDE_AD_HOOK_ID,   XINE_VERSION_CODE, &hde_ad_hook_info,   &hde_ad_hook_init },
	{ PLUGIN_NONE,           0,                             "",               0,                 NULL,                NULL }
};
