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

/* VdrXineMpIf.h */

#ifndef VDR_XINE_MP_IF_H_INCLUDED
#define VDR_XINE_MP_IF_H_INCLUDED

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

//Start by Klaus

#ifndef XINE_MP_IF_H
class reel_video_overlay_event_t;
#endif 

//End by Klaus

typedef enum
{
    VdrXineMpAoCtrlNone,
    VdrXineMpAoCtrlPlayPause,
    VdrXineMpAoCtrlPlayResume,
    VdrXineMpAoCtrlFlushBuffers
} VdrXineMpAoCtrl;

typedef enum
{
    VdrXineMpAoCapModeUnsupported,
    VdrXineMpAoCapModeStereo,
    VdrXineMpAoCapModeA52
} VdrXineMpAoCapMode;

typedef struct
{
    void (*AoClose)();

    int (*AoControl)(VdrXineMpAoCtrl ctrlCmd);

    int (*AoDelay)();

    int (*AoOpen)(unsigned int       bits,
                  unsigned int       rate,
                  VdrXineMpAoCapMode mode);

    int (*AoUseDigitalOut)();

    int (*AoWrite)(unsigned short *data,
                   unsigned int    numFrames);

    void (*AoPutMemPts)(void const   *mem,
                        unsigned int  pts);

    void (*VdDecodeData)(unsigned char const *data,
                         unsigned int         length,
                         unsigned int         pts);

    void (*VdDiscontinuity)();

    void (*VdFlush)();

    void (*VdReset)();

    //Start by Klaus 
    void (*SetOsdScaleMode) (int on);
    void (*SetPlayMode) (int on);
    void (*SpuPutEvents)(reel_video_overlay_event_t *event);

    reel_video_overlay_event_t* (*SpuGetEventStruct)
                        (size_t paletteSize, size_t rleSize);

    long long (*GetMetronomTime) (struct spudec_decoder_s *instance);
    //End by Klaus
} VdrXineMpIf;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VDR_XINE_MP_IF_H_INCLUDED */
