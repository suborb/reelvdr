/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
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

/* plugin.c */

#include "audio_out.h"
#include "mpeg_videodec.h"
#include "xine_decoder.h"

#include <xine.h>
#ifndef RBLITE
    #include <config.h>
#endif
#include <xine_internal.h>
#include <xine_plugin.h>
#include <video_decoder.h>
#include "spu_decoder.h"

/*
 * exported plugin catalog entry
 */

static const ao_info_t ao_info_reel =
{
    //20  /* priority */
    0  /* priority */
     
};

static uint32_t supported_types[] = {BUF_VIDEO_MPEG, 0};
static decoder_info_t const dec_info_mpeg_videodec_reel =
{
    supported_types,     /* supported types */
    // 20                   /* priority        */
    0                   /* priority        */
};

/* plugin catalog information */
static uint32_t supported_spu_types[] = { BUF_SPU_DVD, 0 };

static const decoder_info_t spu_info_data = 
{
    supported_spu_types,     /* supported types */
    // 20                    /* priority        */
    0                        /* priority        */
};

plugin_info_t const xine_plugin_info[] EXPORTED =
{
    /* type, API, "name", version, special_info, init_function */  
    { PLUGIN_VIDEO_DECODER, 18, "mpeg_videodec_reel", XINE_VERSION_CODE,
                &dec_info_mpeg_videodec_reel, init_plugin_mpeg_videodec_reel },
    { PLUGIN_AUDIO_OUT, 8, "audio_out_reel", XINE_VERSION_CODE, 
                &ao_info_reel, init_plugin_audio_out_reel },
    { PLUGIN_SPU_DECODER, 16, "spudec_reel", XINE_VERSION_CODE, &spu_info_data, init_reel_spu_decode_plugin },
    { PLUGIN_NONE, 0, "", 0, NULL, NULL }
};
