/*
 * iec60958.c:        Define the S/P-DIF interface for redirecting
 *                16bit none audio stream (NO decoding!).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 * Copyright (C) 2002,2003 Werner Fink, <werner@suse.de>
 *
 */ 

#include "iec60958.h"

// see iec60958.h enum_stream_t
const char * const audioTypes[5] = {"", "PCM",  "AC3", "DTS", "MP2"};

iec60958::iec60958(unsigned int rate,
                   const unsigned int bsize,
                   const uint_8 poff)
: burst_size(bsize), offset(poff), start(0), current(0), remember(0), payload(0), flags(0)
{
    sample_rate = rate;
    play.burst = (uint_32 *)0;        // Provide pointer to 16bit PCM stereo samples
    play.size  = play.pay = 0;
    last.burst = (uint_32 *)0;        // Provide pointer to 16bit PCM stereo samples
    last.size  = last.pay = 0;
    isDVD = false;
    track = 0;
}

void iec60958::Reset(flags_t arg)
{
    flags = arg;
    Clear();
    isDVD = false;                // Is this a DVD stream?
}

void iec60958::Clear(void)
{ 
    buffer_reset();                // Clear buffer
    ClassReset();                // E.g. counters, pointers
}

const frame_t & iec60958::Frame(enum_frame_t type)
{
    uint_16 *sh = (uint_16 *)&remember[0];
    memset(&remember[0], 0, burst_size);

    switch (type) {
    default:
    case PCM_DATA:
        sh[0] = char2short(0xf8, 0x72);                // Pa
        sh[1] = char2short(0x4e, 0x1f);                // Pb
        sh[2] = char2short(7<<5, 0x00);                // null frame requires stream = 7
        sh[3] = shorts(0x0000);                        // Null data_type
        last.size  = burst_size;
        break;
    case PCM_WAIT:
        sh[0] = char2short(0xf8, 0x72);                // Pa
        sh[1] = char2short(0x4e, 0x1f);                // Pb
        sh[2] = char2short(0x00, 0x03);         // Audio ES Channel empty, wait for DD Decoder or pause
        sh[3] = char2short(0x00, 0x20);         // Trailing frame size is 0x0020 aka 32 bits payload
        last.size  = SPDIF_BURST_SIZE;                // _OR_ simply  32 aka 0x20 ms fill frame???
        break;
    case PCM_WAIT2:
        sh[0] = char2short(0xf8, 0x72);                // Pa
        sh[1] = char2short(0x4e, 0x1f);                // Pb
        sh[2] = char2short(0x00, 0x03);         // Audio ES Channel empty, wait for DD Decoder or pause
        sh[3] = shorts(8*(burst_size-8));        // Trailing frame size is burst_size - header_size
        last.size  = burst_size;
        break;
    case PCM_STOP:
        sh[0] = char2short(0xf8, 0x72);                // Pa
        sh[1] = char2short(0x4e, 0x1f);                // Pb
        sh[2] = char2short(0x01, 0x03);         // User stop, skip or error
        sh[3] = char2short(0x08, 0x00);         // Trailing frame size is 2048 bits (silent)
        last.size  = burst_size;
        break;
    case PCM_START:                                // Combination of PCM_WAIT and PCM_DATA
        // char  | 0,1 | 2,3 | 4,5 | 6,7 | 8,9 |10,11|12,13|14,15
        // short |   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7
        //           0     0     0     0  ,   magic  ,stream, len
        sh[4] = char2short(0xf8, 0x72);                // Pa
        sh[5] = char2short(0x4e, 0x1f);                // Pb
        sh[6] = char2short(7<<5, 0x00);         // null frame requires stream = 7
        sh[7] = shorts(0x0000);                 // Null data_type
        // char  |16,17|18,19|20,21|22,23|24,25|26,27|28,29|30,31
        // short |   8 |   9 |   10|   11|   12|   13|   14|   15
        //           0     0     0     0  ,   magic  ,stream, len
        sh[12] = char2short(0xf8, 0x72);        // Pa
        sh[13] = char2short(0x4e, 0x1f);        // Pb
        sh[14] = char2short(0x00, 0x03);         // Audio ES Channel empty, wait for DD Decoder or pause
        sh[15] = char2short(0x00, 0x40);         // Trailing frame size is 0x0040 aka 64 bits payload
        // char  |32,33|34,35|36,37|38,39
        // short |   16|   17|   18|   19
        //           0     0     0     0
        last.size  = burst_size;
        break;
    case PCM_SILENT:                                // Just silent
        last.size  = burst_size;
        break;
    }

    last.burst = (uint_32 *)&remember[0];
    return last;
}
