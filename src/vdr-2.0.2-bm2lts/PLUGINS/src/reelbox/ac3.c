/*
 * ac3.c:        Use the S/P-DIF interface for redirecting AC3 stream (NO decoding!).
 *
 * Parts based on code by:
 *
 * Copyright (C) 1999 Aaron Holtzman
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
 * Remark: Note that AC3/A52 standard is (even if publically available)
 *         is the property of the Dolby Labs Inc. and therefore
 *         (re-)distribution in combination with other charged contributions
 *         of what so ever may violate this owner ship.
 *         ... Nevertheless, the AC3 stream is forwarded from a DVB TV channel
 *         which has to pay its license fee and also the hardware decoder
 *         receiving the forwarded AC3 stream was paid with this license fee.
 *
 * Copyright (C) 2002-2004 Werner Fink, <werner@suse.de>
 */

#include "ac3.h"

//#define DEBUG_AC3
#ifdef  DEBUG_AC3
# define debug_ac3(args...) fprintf(stderr, args)
#else
# define debug_ac3(args...)
#endif

#define USE_LOOK_UP_TABLE        1        // For getting speed
#define USE_LAST_FRAME                1        // In case of CRC error

// --- cAC3 : Scanning AC3 stream for counting and warping into PCM frames -------------

cAC3 ac3(48000);

const ac3size_t cAC3::frmsizecod_tbl[64] = {
    { 32  ,{64   ,69   ,96   } },
    { 32  ,{64   ,70   ,96   } },
    { 40  ,{80   ,87   ,120  } },
    { 40  ,{80   ,88   ,120  } },
    { 48  ,{96   ,104  ,144  } },
    { 48  ,{96   ,105  ,144  } },
    { 56  ,{112  ,121  ,168  } },
    { 56  ,{112  ,122  ,168  } },
    { 64  ,{128  ,139  ,192  } },
    { 64  ,{128  ,140  ,192  } },
    { 80  ,{160  ,174  ,240  } },
    { 80  ,{160  ,175  ,240  } },
    { 96  ,{192  ,208  ,288  } },
    { 96  ,{192  ,209  ,288  } },
    { 112 ,{224  ,243  ,336  } },
    { 112 ,{224  ,244  ,336  } },
    { 128 ,{256  ,278  ,384  } },
    { 128 ,{256  ,279  ,384  } },
    { 160 ,{320  ,348  ,480  } },
    { 160 ,{320  ,349  ,480  } },
    { 192 ,{384  ,417  ,576  } },
    { 192 ,{384  ,418  ,576  } },
    { 224 ,{448  ,487  ,672  } },
    { 224 ,{448  ,488  ,672  } },
    { 256 ,{512  ,557  ,768  } },
    { 256 ,{512  ,558  ,768  } },
    { 320 ,{640  ,696  ,960  } },
    { 320 ,{640  ,697  ,960  } },
    { 384 ,{768  ,835  ,1152 } },
    { 384 ,{768  ,836  ,1152 } },
    { 448 ,{896  ,975  ,1344 } },
    { 448 ,{896  ,976  ,1344 } },
    { 512 ,{1024 ,1114 ,1536 } },
    { 512 ,{1024 ,1115 ,1536 } },
    { 576 ,{1152 ,1253 ,1728 } },
    { 576 ,{1152 ,1254 ,1728 } },
    { 640 ,{1280 ,1393 ,1920 } },
    { 640 ,{1280 ,1394 ,1920 } }
};

const uint_32 cAC3::freqcod_tbl[4] = { 48000, 44100, 32000, 0 };

const uint_16 cAC3::crc_lut[256] = {
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

const uint_16 cAC3::gpolynomial = 0x8005;        // x^16 + x^15 + x^2 + 1 generator polynomial

const uint_16 cAC3::magic = 0x0b77;                // The magic word of AC3 frames

cAC3::cAC3(unsigned int rate)
: iec60958(rate, AC3_BURST_SIZE, 8)
{
    reset_scan();
    reset_count();
}

// This internal function determines from the first four bytes after
// the two sync words the sampling rate, frame size, bit stream
// identification, and the bit stream mode.
inline void cAC3::parse_syncinfo(ac3info_t &syncinfo, const uint_8 *data)
{
    uint_16        fscod;                // Stream Sampling Rate (kHz)
                                // 0 = 48, 1 = 44.1, 2 = 32, 3 = reserved
    uint_16        frmsizecod;        // Frame size code

    //
    // We need to read in the entire syncinfo struct (0x0b77 + 24 bits)
    // in order to determine how big the frame is
    //

    // Get the frequency and sampling code
    fscod      = (data[4] >> 6) & 0x3;
    // Get the frame size code 
    frmsizecod = (data[4] & 0x3f);

    // Get mode and identifier
    syncinfo.bsid  = (data[5] >> 3) & 0x1f;
    syncinfo.bsmod = (data[5] & 0x07);

    // Calculate the sampling rate
    syncinfo.sampling_rate = freqcod_tbl[fscod];
    if (fscod > 2) {
        syncinfo.bit_rate   = 0;
        syncinfo.frame_size = 0; 
        goto out;
    }
    // Calculate the frame size and bitrate
    syncinfo.bit_rate   = frmsizecod_tbl[frmsizecod].bit_rate;
    syncinfo.frame_size = frmsizecod_tbl[frmsizecod].frame_size[fscod];
out:
    return;
}

#ifdef USE_LOOK_UP_TABLE
//
// This internal function uses the look up table crc_lut to check
// the consistency of the AC3 frame
//
inline bool cAC3::crc_check(const uint_8 *const data, const size_t num_bytes)
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
// to check the consistency of the AC3 frame (which in fact is also used
// for getting the look up table crc_lut.
//
inline bool cAC3::crc_check(const uint_8 *const data, const size_t num_bytes)
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
                state = (state << 1) ^ polynomial;
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
//   for a full AC3 frame found in the segment.
const frame_t & cAC3::Frame(const uint_8 *&out, const uint_8 *const tail)
{
    play.burst = (uint_32 *)0;
    play.size  = 0;

resync:
    //
    // Find the ac3 sync word.
    //
    while(s.syncword != magic) {
        if (out >= tail)
            goto done;
        s.syncword = (s.syncword << 8) | *out++;
    }

    //
    // Need the next 4 bytes to decide how big
    // and which mode the frame is
    //
    while(s.pos < 6) {
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
            s.syncword = 0xffff;
            s.pos = 2;
            esyslog("AC3PCM: ** Invalid frame found - try to syncing **");
            if (out >= tail)
                goto done;
            else
                goto resync;
        }
        s.payload_size = (uint_32)((s.syncinfo.frame_size * 2));
    }

    //
    // Maybe: Reinit spdif interface if sample rate is changed
    //
    if (s.syncinfo.sampling_rate != sample_rate) {
        s.syncword = 0xffff;
        s.pos = 2;
        s.payload_size = 0;
        esyslog("AC3PCM: ** Invalid sample rate found - try to syncing **");
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

    //
    // Check the crc over the entire frame
    //
    if(!crc_check(payload + 2, s.payload_size - 2)) {
        s.syncword = 0xffff;
        s.pos = 2;
        s.payload_size = 0;
#ifdef USE_LAST_FRAME
        ::printf("AC3PCM: ** CRC failed - repeat last frame **\n");
        play.burst = last.burst;
        play.size  = last.size;
        play.pay   = last.pay;
        goto done;
#else
        ::printf("AC3PCM: ** CRC failed - try to syncing **\n");
        if (out >= tail)
            goto done;
        else
            goto resync;
#endif
    }

    //
    // If we reach this point, we found a valid AC3 frame to warp into PCM (IEC60958)
    // accordingly to IEC61937
    //
    // What | PCM head              | AC3   payload
    // pos  | 0,1 | 2,3 | 4,5 | 6,7 | 8,9 |10,11|12,13
    // size |  16 |  16 |  16 |  16 |  16 |  16 |  16
    // pos  |                       | 0,1 | 2,3 | 4,5                                     |
    // mean |                       |sync | crc | fscod(2)+frmsizecod(6)+bsid(5)+bsmod(3) |
    //
    pcm[0] = char2short(0xf8, 0x72);                                // Pa
    pcm[1] = char2short(0x4e, 0x1f);                                // Pb
    pcm[2] = char2short(s.syncinfo.bsmod, 0x01);                // AC3 data, bsmod, stream = 0
    pcm[3] = shorts(s.syncinfo.frame_size * 16);                // Trailing AC3 frame size
    pcm[4] = char2short(0x0b, 0x77);                                // AC3 syncwork
    swab(payload + 2, payload + 2, s.payload_size - 2);
    // With padding zeros and do not overwrite IEC958 head
    memset(payload + s.payload_size, 0,  AC3_BURST_SIZE - 8 - s.payload_size);

    play.burst = (uint_32 *)(&(current[0]));
    play.size  = AC3_BURST_SIZE;
    play.pay   = s.payload_size;

    Switch();

    last.burst = (uint_32 *)(&(remember[0]));
    last.size  = play.size;
    last.pay   = play.pay;

    //
    // Reset the syncword for next time
    //
    s.syncword = 0xffff;
    s.pos = 2;
    s.payload_size = 0;
done:
    return play;
}

const bool cAC3::Count(const uint_8 *buf, const uint_8 *const tail)
{
    bool ret = false;

resync:
    if (c.bfound < 5) {
        switch (c.bfound) {
        case 0 ... 1:
            while(c.syncword != magic) {
                if (buf >= tail)
                    goto out;
                c.syncword = (c.syncword << 8) | *buf++;
            }
            c.bfound = 2;
        case 2:
            if (buf >= tail)
                goto out;
            buf++;
            c.bfound++;
        case 3:
            if (buf >= tail)
                goto out;
            buf++;
            c.bfound++;
        case 4:
            if (buf >= tail)
                goto out;
            c.fsize = 2 * frmsizecod_tbl[(*buf)&0x3f].frame_size[((*buf)>>6)&0x3];
            debug_ac3("count_ac3_frame: %d\n", c.fsize);
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
        c.syncword = 0xffff;
        ret = true;
        debug_ac3("count_ac3_frame: frame crossed\n");
        if (buf >= tail)
            goto out;
        goto resync;
    }
out:
    return ret;
}
