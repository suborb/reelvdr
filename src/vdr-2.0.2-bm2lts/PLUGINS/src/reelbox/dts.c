/*
 * dts.c:        Use the S/P-DIF interface for redirecting DTS stream (NO decoding!).
 *
 * Parts based on code by:
 *
 * Copyright (C) 2002 Unknown
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
 * Remark: Note that DTS is NOT an open standard and is the property of
 *         the DIGITAL THEATER SYSTEMS Inc. and therefore (re-)distribution
 *         may violate this owner ship.
 *         ... Nevertheless, the DTS stream is forwarded from a DVB TV channel
 *         which has to pay its license fee and also the hardware decoder
 *         receiving the forwarded DTS stream was paid with this license fee.
 *
 * Copyright (C) 2003,2004 Werner Fink, <werner@suse.de>
 */

#include "dts.h"

//#define DEBUG_DTS
#ifdef  DEBUG_DTS
# define debug_dts(args...) fprintf(stderr, args)
#else
# define debug_dts(args...)
#endif

#define USE_LOOK_UP_TABLE        1        // For getting speed
#define USE_LAST_FRAME                1        // In case of CRC error

// --- cDTS : Scanning DTS stream for counting and warping into PCM frames -------------

cDTS dts(48000);

const uint_32 cDTS::magic = 0x7ffe8001;

const uint_16 cDTS::bitratecod_tbl[32] = {
 32,  56,  64,  96,   112,  128,  192,  224,  256,  320,  384,  448,  512,  576,
 640, 768, 960, 1024, 1152, 1280, 1344, 1408, 1411, 1472, 1536, 1920, 2048, 3072,
 3840, 0, 0, 0 };

const uint_32 cDTS::freqcod_tbl[16] = {
    0, 32000 /* 4x8kHz  */, 32000 /* 2x16kHz */, 32000,
 0, 0, 44100 /* 4*11kHz */, 44100 /* 2x22kHz */, 44100,
 0, 0, 48000 /* 4*12kHz */, 48000 /* 2*24kHz */, 48000, 0, 0 };

const uint_16 cDTS::crc_lut[256] = {
    0x0000,0x8005,0x800f,0x000a,0x801b,0x001e,0x0014,0x8011,
    0x8033,0x0036,0x003c,0x8039,0x0028,0x802d,0x8027,0x0022,
    0x8063,0x0066,0x006c,0x8069,0x0078,0x807d,0x8077,0x0072,
    0x0050,0x8055,0x805f,0x005a,0x804b,0x004e,0x0044,0x8041,
    0x80c3,0x00c6,0x00cc,0x80c9,0x00d8,0x80dd,0x80d7,0x00d2,
    0x00f0,0x80f5,0x80ff,0x00fa,0x80eb,0x00ee,0x00e4,0x80e1,
    0x00a0,0x80a5,0x80af,0x00aa,0x80bb,0x00be,0x00b4,0x80b1,
    0x8093,0x0096,0x009c,0x8099,0x0088,0x808d,0x8087,0x0082,
    0x8183,0x0186,0x018c,0x8189,0x0198,0x819d,0x8197,0x0192,
    0x01b0,0x81b5,0x81bf,0x01ba,0x81ab,0x01ae,0x01a4,0x81a1,
    0x01e0,0x81e5,0x81ef,0x01ea,0x81fb,0x01fe,0x01f4,0x81f1,
    0x81d3,0x01d6,0x01dc,0x81d9,0x01c8,0x81cd,0x81c7,0x01c2,
    0x0140,0x8145,0x814f,0x014a,0x815b,0x015e,0x0154,0x8151,
    0x8173,0x0176,0x017c,0x8179,0x0168,0x816d,0x8167,0x0162,
    0x8123,0x0126,0x012c,0x8129,0x0138,0x813d,0x8137,0x0132,
    0x0110,0x8115,0x811f,0x011a,0x810b,0x010e,0x0104,0x8101,
    0x8303,0x0306,0x030c,0x8309,0x0318,0x831d,0x8317,0x0312,
    0x0330,0x8335,0x833f,0x033a,0x832b,0x032e,0x0324,0x8321,
    0x0360,0x8365,0x836f,0x036a,0x837b,0x037e,0x0374,0x8371,
    0x8353,0x0356,0x035c,0x8359,0x0348,0x834d,0x8347,0x0342,
    0x03c0,0x83c5,0x83cf,0x03ca,0x83db,0x03de,0x03d4,0x83d1,
    0x83f3,0x03f6,0x03fc,0x83f9,0x03e8,0x83ed,0x83e7,0x03e2,
    0x83a3,0x03a6,0x03ac,0x83a9,0x03b8,0x83bd,0x83b7,0x03b2,
    0x0390,0x8395,0x839f,0x039a,0x838b,0x038e,0x0384,0x8381,
    0x0280,0x8285,0x828f,0x028a,0x829b,0x029e,0x0294,0x8291,
    0x82b3,0x02b6,0x02bc,0x82b9,0x02a8,0x82ad,0x82a7,0x02a2,
    0x82e3,0x02e6,0x02ec,0x82e9,0x02f8,0x82fd,0x82f7,0x02f2,
    0x02d0,0x82d5,0x82df,0x02da,0x82cb,0x02ce,0x02c4,0x82c1,
    0x8243,0x0246,0x024c,0x8249,0x0258,0x825d,0x8257,0x0252,
    0x0270,0x8275,0x827f,0x027a,0x826b,0x026e,0x0264,0x8261,
    0x0220,0x8225,0x822f,0x022a,0x823b,0x023e,0x0234,0x8231,
    0x8213,0x0216,0x021c,0x8219,0x0208,0x820d,0x8207,0x0202
};

const uint_16 cDTS::gpolynomial = 0x8005;        // x^16 + x^15 + x^2 + 1 generator polynomial

cDTS::cDTS(unsigned int rate)
: iec60958(rate, DTS_BURST_SIZE, 8)                // Burst size may vary
{
    reset_scan();
    reset_count();
}

// This internal function determines from the first eight bytes after
// the four sync words the sampling rate(?), frame size, bit stream
// identification(?), and the bit stream mode(?).
inline void cDTS::parse_syncinfo(dtsinfo_t &syncinfo, const uint_8 *data)
{
    uint_8 nbs = (uint_8)((data[4]&0x01)<<6)|((data[5]>>2)&0x3f);

    // Normal 16bit Stereo PCM is used to embed DTS data therefore
    // we have two bytes per 16bit sample for both channels, whereas
    // DTS use 32 samples for each decoded block of each decoded channel

    // Calculate the sampling rate and expanded size identifier
    switch(nbs) {
    case 0x07:                                // Frame expands to  8 PCM sample blocks??
        syncinfo.bsid = 0x0a;
        syncinfo.burst_size = 1024;        // (7+1)*32*2*2
        break;
    case 0x0f:                                // Frame expands to 16 PCM sample blocks??
        syncinfo.bsid = 0x0b;
        syncinfo.burst_size = 2048;        // (15+1)*32*2*2
        break;
    case 0x1f:                                // Frame expands to 32 PCM sample blocks??
        syncinfo.bsid = 0x0c;
        syncinfo.burst_size = 4096;        // (31+1)*32*2*2
        break;
    case 0x3f:                                // Frame expands to 64 PCM sample blocks??
        syncinfo.bsid = 0x0d;
        syncinfo.burst_size = 8192;        // (63+1)*32*2*2  _not_ supported
        break;
    default:                                // Termination frame????
        syncinfo.bsid = 0x00;
        if (nbs < 5)
            nbs = 127;                        // Values large than 47 are _not_ supported
        syncinfo.burst_size = (nbs+1)*32*2+2;
        break;
    }

    // We do not support more than 6144 bytes, do we?
    if (syncinfo.burst_size > SPDIF_BURST_SIZE)
        syncinfo.burst_size = SPDIF_BURST_SIZE;

    // Calculate the frame size
    syncinfo.frame_size =
        (((uint_16)(data[5] & 0x03) << 12) |
         ((uint_16)(data[6] & 0xff) <<  4) |
         ((uint_16)(data[7] & 0xf0) >>  4)) + 1;

    // Calculate the sampling rate and bitrate
    syncinfo.sampling_rate = freqcod_tbl[(data[8] & 0x3c) >> 2];
    syncinfo.bit_rate = bitratecod_tbl[((data[8]&0x03)|((data[9]>>5)&0x07))];
}

#ifdef USE_LOOK_UP_TABLE
//
// This internal function uses the look up table crc_lut to check
// the consistency of the DTS frame
//
inline bool cDTS::crc_check(const uint_8 *const data, const size_t num_bytes)
{
    const uint_8 *      out  = data;
    const uint_8 *const tail = data + num_bytes;
    volatile uint_16 state = 0;

    while (out < tail)
        state = crc_lut[*out++ ^ (state>>8)] ^ (state<<8);
    return (state == 0);
}
#else
//
// This internal function uses the generator polynomial x^16 + x^15 + x^2 +1
// to check the consistency of the DTS frame (which in fact is also used
// for getting the look up table crc_lut.
//
inline bool cDTS::crc_check(const uint_8 *const data, const size_t num_bytes)
{
    const uint_8 *      out  = data;
    const uint_8 *const tail = data + num_bytes;
    volatile uint_16 state = 0;
    volatile uint_16 lfsr;

    unsigned short k;

    while (out < tail) {
        lfsr = ((uint_16)(*out++) << 8);
        for (k = 0; k < 8; k++) {
            if ((state ^ lfsr) & 0x8000)
                state = (state << 1) ^ gpolynomial;
            else
                state = (state << 1);
            lfsr <<= 1;
        }
    }
    return (state == 0);
}
#endif

// This function requires two arguments:
//   first is the start of the data segment
//   second is the tail of the data segment
//   Note: the start will be shift by the amount
//   for a full DTS frame found in the segment.
const frame_t & cDTS::Frame(const uint_8 *&out, const uint_8 *const tail)
{
    play.burst = (uint_32 *)0;
    play.size  = 0;

resync:
    //
    // Find the DTS sync double word.
    //
    while(s.syncword != magic) {
        if (out >= tail)
            goto done;
        s.syncword = (s.syncword << 8) | *out++;
    }

    //
    // Need the next 5 bytes to decide how big
    // and which mode the frame is
    //
    while(s.pos < 10) {
        if (out >= tail)
            goto done;
        payload[s.pos++] = *out++;
    }

    //
    // Parse and check syncinfo if not known for this frame
    //
    if (!s.payload_size) {
        parse_syncinfo(s.syncinfo, payload);
        if (!s.syncinfo.frame_size) {
            s.syncword = 0xffffffff;
            s.pos = 4;
            esyslog("DTSPCM: ** Invalid frame found - try to syncing **");
            if (out >= tail)
                goto done;
            else
                goto resync;
        }
        s.payload_size = (uint_32)((s.syncinfo.frame_size));
    }

    //
    // Sanity check
    //
    if (s.syncinfo.burst_size < s.payload_size) {
        s.syncword = 0xffffffff;
        s.pos = 4;
        esyslog("DTSPCM: ** Invalid burst size found - try to syncing **");
        if (out >= tail)
            goto done;
        else
            goto resync;
    }

    //
    // Maybe: Reinit spdif interface if sample rate is changed
    //
    if (s.syncinfo.sampling_rate != sample_rate) {
        s.syncword = 0xffffffff;
        s.pos = 4;
        esyslog("DTSPCM: ** Invalid sample rate found - try to syncing **");
        if (out >= tail)
            goto done;
        else
            goto resync;
    }
    
    while(s.pos < s.payload_size) {
        int rest = tail - out;
        if (rest <= 0)
            goto done;
        if (rest + s.pos > s.payload_size)
            rest = s.payload_size - s.pos;
        memcpy(&payload[s.pos], out, rest);
        s.pos += rest;
        out   += rest;
    }
#if 0
    //
    // Check the crc over the entire frame (which does not
    // work, because it must be done step by step, from
    // crc to crc).
    //
    if(!crc_check(payload + 4, s.payload_size - 4)) {
        s.syncword = 0xffffffff;
        s.pos = 4;
        s.payload_size = 0;
#ifdef USE_LAST_FRAME
        printf("DTSPCM: ** CRC failed - repeat last frame **");
        play.burst = last.burst;
        play.size  = last.size;
        play.pay   = last.pay;
        goto done;
#else
        printf("DTSPCM: ** CRC failed - try to syncing **");
        if (out >= tail)
            goto done;
        else
            goto resync;
#endif
    }
#endif
    //
    // If we reach this point, we found a valid DTS frame to warp into PCM (IEC60958)
    // accordingly to IEC61937
    //
    // What | PCM head              |    DTS      payload
    // pos  | 0,1 | 2,3 | 4,5 | 6,7 | 8,9,10,11 |
    // size |  16 |  16 |  16 |  16 |  16 |  16 |
    // pos  |                       | 0,1 | 2,3 |
    // mean |                       |   sync    |
    //
    pcm[0] = char2short(0xf8, 0x72);                                // Pa
    pcm[1] = char2short(0x4e, 0x1f);                                // Pb
    pcm[2] = char2short(0x00, s.syncinfo.bsid);                // DTS data
    pcm[3] = shorts(s.syncinfo.frame_size * 8);                // Trailing DTS frame size
    pcm[4] = char2short(0x7f, 0xfe);                                // DTS first syncwork
    pcm[5] = char2short(0x80, 0x01);                                // DTS second syncwork
    swab(payload + 4, payload + 4, s.payload_size - 4);
    // With padding zeros and do not overwrite IEC958 head
    memset(payload + s.payload_size, 0,  s.syncinfo.burst_size - 8 - s.payload_size);

    play.burst = (uint_32 *)(&current[0]);
    play.size  = s.syncinfo.burst_size;
    play.pay   = s.payload_size;

    Switch();

    last.burst = (uint_32 *)(&remember[0]);
    last.size  = play.size;
    last.pay   = play.pay;

    //
    // Reset the syncword for next time
    //
    s.syncword = 0xffffffff;
    s.pos = 4;
    s.payload_size = 0;
done:
    return play;
}

const bool cDTS::Count(const uint_8 *buf, const uint_8 *const tail)
{
    bool ret = false;

resync:
    if (c.bfound < 8) {
        switch (c.bfound) {
        case 0 ... 3:
            while(c.syncword != magic) {
                if (buf >= tail)
                    goto out;
                c.syncword = (c.syncword << 8) | *buf++;
            }
            c.bfound = 4;
        case 4:
            if (buf >= tail)
                goto out;
            buf++;
            c.bfound++;
        case 5:
            if (buf >= tail)
                goto out;
            c.fsize  = ((uint_16)((*buf) & 0x03) << 12);
            buf++;
            c.bfound++;
        case 6:
            if (buf >= tail)
                goto out;
            c.fsize |= ((uint_16)((*buf) & 0xff) << 4);
            buf++;
            c.bfound++;
        case 7:
            if (buf >= tail)
                goto out;
            c.fsize |= ((uint_16)((*buf) & 0xf0) >> 4);
            c.fsize++;
            debug_dts("count_dts_frame: %d\n", c.fsize);
            buf++;
            c.bfound++;
        default:
            break;
        }
    }

    while (c.bfound < c.fsize) {
        register int skip, pay;
        if ((skip = tail - buf) <= 0)
            goto out;
        if (skip > (pay = c.fsize - c.bfound))
            skip = pay;
        c.bfound += skip;
        buf         += skip;
    }

    if (c.fsize && (c.bfound >= c.fsize)) {
        c.bfound = 0;
        c.syncword = 0xffffffff;
        ret = true;
        debug_dts("count_dts_frame: frame crossed\n");
        if (buf >= tail)
            goto out;
        goto resync;
    }
out:
    return ret;
}
