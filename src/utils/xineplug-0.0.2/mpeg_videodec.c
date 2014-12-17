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

/* mpeg_videodec.c */

#include "mpeg_videodec.h"

#include "utils.h"

#include "VdrXineMpIf.h"
#include "VideoStreamParser.h"

#include <xine.h>
#include <xineutils.h>
#include <video_decoder.h>

/*******************************************************************************
 * Video decoder instance.
 */

#define STILL_FRAME_REPEATS 20

static video_decoder_t video_decoder_;    /* Single decoder instance. */
static int             decoder_open_ = 0;

extern VdrXineMpIf const *VXI;

static void vd_decode_data(video_decoder_t *self, buf_element_t *buf)
{
    typedef unsigned char Byte;
    int bufSize = buf->size;
    Byte const *bufContent = buf->content;
    int stillPicture = VideoStreamParserFeed(buf->content, buf->size);

    if (stillPicture >= 0)
    {
        int size1 = bufSize - stillPicture;
        void const *iFrameData;
        int iFrameLength;
        int n;

        VXI->VdDecodeData(bufContent, size1, 0);
        VXI->VdReset();
        VideoStreamParserGetLastIFrame(&iFrameData, &iFrameLength);
        for (n = 0; n < STILL_FRAME_REPEATS ; ++n)
        {
            VXI->VdDecodeData(iFrameData, iFrameLength, 0);
        }
        VXI->VdDecodeData(bufContent + size1, bufSize - size1, 0);
    }
    else
    {
        VXI->VdDecodeData(buf->content, buf->size, buf->pts);
    }
}

static void vd_discontinuity(video_decoder_t *self)
{
    VXI->VdDiscontinuity();
}

static void vd_dispose(video_decoder_t *self)
{
    decoder_open_ = 0;
}

static void vd_flush(video_decoder_t *self)
{
    VXI->VdFlush();
}

static void vd_reset(video_decoder_t *self)
{
    VideoStreamParserReset();
    VXI->VdReset();
}

/*******************************************************************************
 * Video decoder class.
 */

static void vdcls_dispose(video_decoder_class_t *cls)
{
    REEL_LOG("mpeg_videodec_reel: vdcls_dispose()\n");
    free(cls);
}

static char *vdcls_get_description(video_decoder_class_t *cls)
{
    return "MPEG-2 video decoder plugin for reelbox";
}

static char *vdcls_get_identifier(video_decoder_class_t *cls)
{
    return "mpeg_videodec_reel";
}

static video_decoder_t *vdcls_open_plugin(video_decoder_class_t *cls, xine_stream_t *stream)
{
    if (decoder_open_)
    {
        /* We are unable to support multiple simultaneous decoders. */
        REEL_LOG("mpeg_videodec_reel: open_plugin() : "
                 "Video decoder instance is already active.\n");
        return NULL;
    }

    video_decoder_.decode_data   = vd_decode_data;
    video_decoder_.discontinuity = vd_discontinuity;
    video_decoder_.dispose       = vd_dispose;
    video_decoder_.flush         = vd_flush;
    video_decoder_.reset         = vd_reset;

    decoder_open_ = 1;

    VideoStreamParserReset();

    return &video_decoder_;
}

/*******************************************************************************
 * Plugin initialization.
 */

void *init_plugin_mpeg_videodec_reel(xine_t *xine, void *data)
{
    video_decoder_class_t *cls;

    REEL_LOG("init_plugin_mpeg_videodec_reel()\n");

    cls = (video_decoder_class_t *)xine_xmalloc(sizeof(video_decoder_class_t));

    cls->dispose         = vdcls_dispose;
    cls->get_description = vdcls_get_description;
    cls->get_identifier  = vdcls_get_identifier;
    cls->open_plugin     = vdcls_open_plugin;

    return cls;
}
