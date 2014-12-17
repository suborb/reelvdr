/*
 * ac3.h:        Use the S/P-DIF interface for redirecting AC3 stream (NO decoding!).
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

#ifndef AC3_H
#define AC3_H

// #include "types.h"
#include "iec60958.h"

#define AC3_BURST_SIZE                SPDIF_BURST_SIZE

typedef struct _ac3_frmsize
{
    uint_16 bit_rate;
    uint_16 frame_size[3];
} ac3size_t;

typedef struct _ac3_syncinfo {
    uint_32 sampling_rate;        // Sampling rate in hertz
    uint_16 bit_rate;                // Bit rate in kilobits
    uint_16 frame_size;                // Frame size in 16 bit words
    uint_8  bsid;                // Bit stream identification
    uint_8  bsmod;                // Bit stream mode
} ac3info_t;

class cAC3 : public iec60958 {
private:
    static const uint_16   magic;
    struct {
        size_t    pos;
        size_t    payload_size;
        ac3info_t syncinfo;
        uint_16   syncword;
    } s;
    struct {
        uint_16   syncword;
        uint_16   bfound;
        uint_16   fsize;
    } c;
    
    static const ac3size_t frmsizecod_tbl[64];
    static const uint_32   freqcod_tbl    [4];
    static const uint_16   crc_lut      [256];
    static const uint_16   gpolynomial;
    inline void parse_syncinfo(ac3info_t &syncinfo, const uint_8 *data);
    inline bool crc_check     (const uint_8 *const data, const size_t num_bytes);
    inline void reset_scan (void)
    {
        s.pos = 2;
        s.syncword = 0xffff;
        s.payload_size = 0;
        memset(&s.syncinfo, 0, sizeof(ac3info_t));
    };
    inline void reset_count(void)
    {
        c.syncword = 0xffff;
        c.bfound = 0;
        c.fsize = 0;
    }
public: 
    cAC3(unsigned int rate);        // Default Sample rate
    virtual ~cAC3() {};
    virtual const bool      Count(const uint_8 *buf, const uint_8 *const tail);
    virtual const frame_t & Frame(const uint_8 *&out, const uint_8 *const end);
    virtual const void      ClassReset(void)
    {
        reset_scan();
        reset_count();
    }
};

extern cAC3 ac3;

#endif // AC3_H
