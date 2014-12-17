/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __TOOLS_DVD_H
#define __TOOLS_DVD_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __QNXNTO__
#include <strings.h>
#endif

#include "mpegtypes.h"

#ifndef AARONS_TYPES
#define AARONS_TYPES
typedef unsigned long long uint_64;
#endif

class cPStream {
 public:
    static bool packetStart(uint8_t * &data, int len) {
        while (len > 6 && !(data[0] == 0x00 &&
                            data[1] == 0x00 && data[2] == 0x01)) {
            data++;
            len--;
        }
        return (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01);
    }

    static int packetType     (const uint8_t *data) { return data[3]; }
    static int stuffingLength (const uint8_t *data) { return data[13] & 0x07; }
    static int packetLength   (const uint8_t *data) { return (data[4] << 8) + data[5] + 6; }
    static int PESHeaderLength(const uint8_t *data) { return (data[8]); }

    static uint64_t fromPTS(const uint8_t *p)
    {
                uint64_t lpts = 0;
                lpts  = (((uint64_t)p[0]) & 0x0e) << 29;
                lpts |= ( (uint64_t)p[1])         << 22;
                lpts |= (((uint64_t)p[2]) & 0xfe) << 14;
                lpts |= ( (uint64_t)p[3])         <<  7;
                lpts |= (((uint64_t)p[4]) & 0xfe) >>  1;
                return lpts;

    }

    static void toPTS(uint8_t *p, uint64_t lpts)
    {
            p[0] =       ((uint8_t)(lpts >> 29) & 0x0e) | 0x21;
            p[1] =        (uint8_t)(lpts >> 22);
            p[2] = 0x01 | (uint8_t)(lpts >> 14);
            p[3] =        (uint8_t)(lpts >> 7);
            p[4] = 0x01 | (uint8_t)(lpts << 1);
    }


/*
 *             28          20          13          5
 *  7654 3210   7654 3210   7654 3210   7654 3210   7654 3210   7654 3210
 *  01xx x1xx   xxxx xxxx   xxxx x1xx   xxxx xxxx   xxxx x1ee   eeee eee1
 */

    static uint32_t fromSCR(const uint8_t *data)
	{
	    uint32_t ret;

	    ret  = (((data[0] & 0x38) >> 1) | (data[0] & 0x03)) << 28;
	    ret |= data[1] << 20;
	    ret |= (((data[2] & 0xF8) >> 1) | (data[2] & 0x03)) << 13;
	    ret |= data[3] << 5;
	    ret |= data[4] >> 3;

	    return ret;
	}


    static uint8_t fromSCRext(const uint8_t *data)
	{
	    uint8_t ret;

	    ret  = (data[0] & 0x03) << 7;
	    ret |=  data[1] >> 1;

	    return ret;
	}

    static uint16_t fromMUXrate(const uint8_t *data)
	{
	    uint16_t ret;

	    ret  = data[0] << 14;
	    ret |= data[1] <<  6;
	    ret |= data[2] >>  2;

	    return ret;
	}
};

#endif // __TOOLS_DVD_H
