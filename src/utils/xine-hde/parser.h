/*
 * AVCodecParser prototypes and definitions
 * Copyright (c) 2003 Fabrice Bellard.
 * Copyright (c) 2003 Michael Niedermayer.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FFMPEG_PARSER_H
#define FFMPEG_PARSER_H

//*****************************************************************************
// Added only really nessesary data for player.c
#include <xine_internal.h>
#include "hde_tool.h"
//*****************************************************************************
/*DL moved to hde_tool.h
typedef struct ParseContext{
    uint8_t *buffer;
    int index;
    int last_index;
    unsigned int buffer_size;
    uint32_t state;             ///< contains the last few bytes in MSB order
    int frame_start_found;
    int overread;               ///< the number of bytes which where irreversibly read from the next frame
    int overread_index;         ///< the index into ParseContext.buffer of the overreaded bytes
} ParseContext;
DL*/

struct MpegEncContext;

typedef struct ParseContext1{
    ParseContext pc;
/* XXX/FIXME PC1 vs. PC */
    /* MPEG2 specific */
    AVRational frame_rate;
    int progressive_sequence;
    int width, height;

    /* XXX: suppress that, needed by MPEG4 */
    struct MpegEncContext *enc;
    int first_picture;
} ParseContext1;

#define END_NOT_FOUND (-100)

int hde_ff_combine_frame(ParseContext *pc, int next, uint8_t **buf, int *buf_size);
void hde_ff_parse_close(AVCodecParserContext *s);
void hde_ff_parse1_close(AVCodecParserContext *s);

#endif /* !FFMPEG_PARSER_H */
