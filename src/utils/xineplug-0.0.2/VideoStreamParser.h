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

/* VideoStreamParser.h */

#ifndef XINEPLUG_REELBOX_VIDEO_STREAM_PARSER_H_INCLUDED
#define XINEPLUG_REELBOX_VIDEO_STREAM_PARSER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void VideoStreamParserGetLastIFrame(void const **data, int *length);
    /**< Get last i-frame. */

int VideoStreamParserFeed(void const *streamData, int length);
    /**< Feed some length bytes of stream data into the parser. */

void VideoStreamParserReset();
    /**< Reset parser state. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XINEPLUG_REELBOX_VIDEO_STREAM_PARSER_H_INCLUDED */
