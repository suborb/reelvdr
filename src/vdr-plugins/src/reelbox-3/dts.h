/*
 * dts.h:        Use the S/P-DIF interface for redirecting DTS stream (NO decoding!).
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

#ifndef __DTS_H
#define __DTS_H

// #include "types.h"
#include "iec60958.h"

#define DTS_BURST_SIZE        2048        // Not SPDIF_BURST_SIZE !

typedef struct _dts_syncinfo {
    uint_32 sampling_rate;        // Sampling rate in hertz (?)
    uint_16 bit_rate;                // Bit rate in kilobits (?)
    uint_16 frame_size;                // Frame size in 8 bit words
    uint_16 burst_size;                // burst size (?)
    uint_8  bsid;                // Bit stream identification
} dtsinfo_t;

class cDTS : public iec60958 {
private:
    static const uint_32 magic;
    struct {
        size_t    pos;
        size_t    payload_size;
        uint_32   syncword;
        dtsinfo_t syncinfo;
    } s;
    struct {
        uint_32   syncword;
        uint_16   bfound;
        uint_16   fsize;
    } c;
    static const uint_16 bitratecod_tbl[32];
    static const uint_32 freqcod_tbl   [16];
    static const uint_16 crc_lut      [256];
    static const uint_16 gpolynomial;
    inline void parse_syncinfo(dtsinfo_t &syncinfo, const uint_8 *data);
    inline bool crc_check(const uint_8 *const data, const size_t num_bytes);
    inline void reset_scan (void)
    {
        s.pos = 4;
        s.syncword = 0xffffffff;
        s.payload_size = 0;
        memset(&s.syncinfo, 0, sizeof(dtsinfo_t));
    };
    inline void reset_count(void)
    {
        c.syncword = 0xffffffff;
        c.bfound = 0;
        c.fsize = 0;
    }
public: 
    cDTS(unsigned int rate);        // Sample rate
    virtual ~cDTS() {};
    virtual const bool Count(const uint_8 *buf, const uint_8 *const tail);
    virtual const frame_t & Frame(const uint_8 *&out, const uint_8 *const end);
    virtual const void ClassReset(void)
    {
        reset_scan();
        reset_count();
    }
};

extern cDTS dts;

#endif // __DTS_H
