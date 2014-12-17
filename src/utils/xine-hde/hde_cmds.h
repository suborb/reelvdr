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

#ifndef HAVE_HDE_CMDS_H
#define HAVE_HDE_CMDS_H

#define WAIT_TIMEOUT_US 3000000LL
#define WAIT_BUSY_US      10000LL

//#include "../hdshm/src/include/hdshm_user_structs.h"
#include <hdshm_user_structs.h>

int hde_init (void);
int hde_is_init (void);
void hde_done(void);

int hde_osd_xine_clear(void);
int hde_osd_xine_start(void);
void hde_osd_scale(int *x, int *y, int *w, int *h, int sw, int sh, int sax, int say);
int hde_osd_xine_tile(hdcmd_osd_overlay *overlay, int img_width, int img_height, int offset_x, int offset_y, int dst_width, int dst_height);
int hde_osd_xine_end(void);

int hde_get_vol(int *vol);
int hde_set_vol(int vol);

extern hd_vdec_config_t gLastVdecCfg;
int hde_write_video_cfg(hd_vdec_config_t *cfg);
int hde_write_video_data(uint8_t *data, int32_t size, int64_t pts);
int hde_video_reset(void);
int hde_video_resync(void);

extern hd_adec_config_t gLastAdecCfg;
int hde_write_audio_cfg(hd_adec_config_t *cfg);
int hde_write_audio_data(uint8_t *data, int32_t size, int64_t pts);
int hde_audio_reset(void);

int hde_set_speed(int64_t speed);
int hde_get_speed(int64_t *speed);

int hde_get_stc(int64_t *stc);
int hde_adjust_stc(int64_t pts, int64_t *stc);

#endif // HAVE_HDE_CMDS_H

