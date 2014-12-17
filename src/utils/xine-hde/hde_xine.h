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
//TODO Handle hdplayer failures
//TODO Cooperative xine/vdr osd
//TODO Use hdplayer(3) player (special for video/audio queue)

//TODO Handle aspect (video/fb) - Check 4:3 mode & 16:9 menu
//TODO Check for FB DBD mode and change it to FIT (save old video mode and restore it at end)

//TODO Implement dts
//TODO Implement rawpcm?
//TODO Implement direct audio (need xine patch?)
//TODO Implement h.264
//TODO Implement mpeg4
//TODO Implement wma/wmv
//TODO Implement reencode video

#ifndef HAVE_HDE_XINE_H
#define HAVE_HDE_XINE_H

#include <xine_internal.h>

/* data for the device name config entry */
#define CONF_KEY  "hde.device_number"
#define CONF_NAME _("HDE device number")
#define CONF_HELP _("If you have more than one HDE in your computer, you can specify which one to use here.")

/* image format used by hde_decoder to tag undecoded video data */
#define XINE_IMGFMT_HDE (('V'<<24)|('E'<<16)|('D'<<8)|'H')

#ifndef HDE_PLAYER
#error "HDE_PLAYER not defined"
#endif

#if HDE_PLAYER!=0
#define HDE_PREFIX "hde1-"
#else
#define HDE_PREFIX "hde-"
#endif

#define HDE_VIDEO_ID     HDE_PREFIX"video"
#define HDE_VIDEO_ID_X11 HDE_VIDEO_ID"-x11"
#define HDE_VIDEO_ID_AA  HDE_VIDEO_ID"-aa"
#define HDE_VIDEO_ID_FB  HDE_VIDEO_ID"-fb"
#define HDE_VIDEO_DESCR  "video output plugin for the hde."

#define HDE_AUDIO_ID      HDE_PREFIX"audio"
#define HDE_AUDIO_DESCR   "audio output plugin for the hde."

#define HDE_VD_ID         HDE_PREFIX"vd"
#define HDE_VD_DESCR      "video decoder plugin using the hardware decoding capabilities of the hde."

#define HDE_AD_ID         HDE_PREFIX"ad"
#define HDE_AD_DESCR      "audio decoder plugin using the hardware decoding capabilities of the hde."

#define HDE_AD_HOOK_ID    HDE_PREFIX"ad-hook"
#define HDE_AD_HOOK_DESCR "hook to capture pcm-data for the hde."

#define HDE_SD_ID         HDE_PREFIX"sd"
#define HDE_SD_DESCR      "subtitle decoder plugin using the hardware decoding capabilities of the hde."

extern uint32_t hde_vd_supported_types[];
extern uint32_t hde_ad_supported_types[];
extern uint32_t hde_ad_all_types[];

/* plugin class initialization functions */
void *hde_video_init_x11(xine_t *xine, void *visual_gen);
void *hde_video_init_aa(xine_t *xine, void *visual_gen);
void *hde_video_init_fb(xine_t *xine, void *visual_gen);
void *hde_audio_init(xine_t *xine, void *visual_gen);
void *hde_vd_init(xine_t *xine, void *visual_gen);
void *hde_ad_init(xine_t *xine, void *visual_gen);
void *hde_ad_hook_init(xine_t *xine, void *visual_gen);
void *hde_sd_init(xine_t *xine, void *visual_gen);
void *hde_post_init(xine_t *xine, void *visual_gen);

#define HDE_SCR_PRIORITY 10
typedef struct hde_scr_s hde_scr_t;
struct hde_scr_s {
    scr_plugin_t scr_plugin;
    pthread_mutex_t  mutex;
    xine_t *         xine;
    int64_t          speed;
    int64_t          pts;
    int64_t          ct;
	int64_t          stc;
};
hde_scr_t *hde_scr_init(xine_t *xine);

#define AO_CTRL_NATIVE 0xFF

#include <syslog.h>
#endif // HAVE_HDE_XINE_H

