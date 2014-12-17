/*
 * iec60958.h:        Define the S/P-DIF interface for redirecting
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
 */
#ifndef __IEC60958_H
#define __IEC60958_H

#include <netinet/in.h>
#include <string.h>
// #include "types.h"
// #include "pts.h"
// #include "bitstreamout.h"

typedef unsigned int        uint_32;
typedef unsigned short      uint_16;
typedef unsigned char       uint_8;
typedef unsigned long       flags_t;

// Used for little endian data stream (most sound cards)
#ifndef WORDS_BIGENDIAN
# define char2short(a,b)    ((((uint_16)(a) << 8) & 0xff00) ^ ((uint_16)(b) & 0x00ff))
# define shorts(a)      (a)
# define char2int(a,b,c,d)  ((((uint_32)(a)<<24)&0xff000000)^(((uint_32)(b)<<16)&0x00ff0000)^\
                 (((uint_32)(c)<< 8)&0x0000ff00)^( (uint_32)(d)     &0x000000ff))
# define mshort(a)      (uint_16)(((a)&0xffff0000)>>16)
#else
# define char2short(a,b)    ((((uint_16)(b) << 8) & 0xff00) ^ ((uint_16)(a) & 0x00ff))
# define shorts(a)      char2short(((uint_8)(a) & 0xff),((uint_8)((a) >> 8) & 0xff));
# define char2int(a,b,c,d)  ((((uint_32)(d)<<24)&0xff000000)^(((uint_32)(c)<<16)&0x00ff0000)^\
                 (((uint_32)(b)<< 8)&0x0000ff00)^( (uint_32)(a)     &0x000000ff))
# define mshort(a)      (uint_16)((a)&0x0000ffff)
#endif

#define SPDIF_BURST_SIZE    6144        // 32ms at 48kHz
#define B2F(a)          ((a)>>2)    // Bytes to PCM 16bit stereo samples
#define F2B(a)          ((a)<<2)    // PCM 16bit stereo samples to Bytes
#define SPDIF_SAMPLE_FRAMES B2F(SPDIF_BURST_SIZE)
#define SPDIF_SAMPLE_MAGIC  B2F(8)      // 64 bits are required for IEC60958 magic
#define SPDIF_SAMPLE_ALIGN(x)   (((x)+(SPDIF_SAMPLE_MAGIC-1))&~(SPDIF_SAMPLE_MAGIC-1))

#ifdef SPDIF_TEST
# include <stdio.h>
# undef  esyslog
# undef  dsyslog
# define esyslog(format, args...)  fprintf(stderr, format "\n", ## args)
# define dsyslog(format, args...)  fprintf(stderr, format "\n", ## args)
#else
# include <vdr/tools.h>
#endif

#ifndef WORDS_BIGENDIAN
# ifndef _GNU_SOURCE
extern inline void swab (const void *bfrom, void *bto, ssize_t n)
{
    const char *from = (const char *) bfrom;
    char *to = (char *) bto;

    n &= ~((ssize_t) 1);
    while (n > 1) { // Compiled with -funroll-loops it should be optimized
        const char b0 = from[--n], b1 = from[--n];
        to[n] = b0;
        to[n + 1] = b1;
    }
}
# else // if _GNU_SOURCE
extern inline void swab (const void *bfrom, void *bto, ssize_t n)
{
    const uint_16 *from = (const uint_16 *)bfrom;
    const uint_16 *end  = from + (n >> 1);
    uint_16 *to = (uint_16 *)bto;

    while (from < end)
        *to++ = ntohs(*from++);
}
# endif
#else  // if WORDS_BIGENDIAN
# define swab(bfrom, bto, n)
#endif // if WORDS_BIGENDIAN

#define test_and_set_flags(flag)        test_and_set_bit(SETUP_ ## flag, &(flags))
#define test_and_clear_flags(flag)      test_and_clear_bit(SETUP_ ## flag, &(flags))
#define test_flags(flag)                test_bit(SETUP_ ## flag, &(flags))
#define set_flags(flag)                 set_bit(SETUP_ ## flag, &(flags))
#define clear_flags(flag)               clear_bit(SETUP_ ## flag, &(flags))

typedef enum {
    PCM_STOP   = -1,
    PCM_DATA   =  0,
    PCM_WAIT   =  1,
    PCM_START  =  2,
    PCM_SILENT =  3,
    PCM_WAIT2  =  4
} enum_frame_t;

typedef enum {
    IEC_NONE   = 0,
    IEC_PCM    = 1,
    IEC_AC3    = 2,
    IEC_DTS    = 3,
    IEC_MP2    = 4
} enum_stream_t;
extern const char * const audioTypes[5];

typedef struct _frame {
    uint_32 *burst;
    unsigned int size;
    unsigned int pay;
} frame_t;

class iec60958 {
    friend class cAC3;                                        // Access private section
    friend class cDTS;
    friend class cPCM;
    friend class cMP2;
protected:
    const uint_32 burst_size;
private:
    //
    // The scanner function for a data section, it sets
    // the current frame hold in `play'
    //
    uint_8 offset;                                        // Playload offset from IEC60958 head
    uint_8 *start;
    uint_8 *current;
    uint_8 *remember;
    uint_8 *payload;
    inline void buffer_reset(void)
    {
        current  = start;                                // At first adress we start
        remember = start + SPDIF_BURST_SIZE;                // Remember the last PCM frame
        payload  = start + offset;                        // Pointer to payload of the PCM frame,
                                                        // reserve 4 shorts for lPCM starting head
        pcm      = (uint_16 *)&start[0];                // Provide access to 16bit PCM samples

        memset(start, 0x00, 2*SPDIF_BURST_SIZE);
        play.burst = (uint_32 *)0;
        play.size  = play.pay =  0;
        last.burst = (uint_32 *)0;
        last.size  = last.pay =  0;
    };
    //
    // Frame and PCM interface
    //
    frame_t play;
    frame_t last;
    uint_16 *pcm;
    flags_t flags;                                       // Private to the specific class
public:
    iec60958(unsigned int rate,                          // Sample rate
             const uint_32 bsize,                        // Burst size
             const uint_8  poff);                        // Offset to payload (AC3/DTS)
    virtual ~iec60958() {};
    virtual void Offset(const uint_8 poff) { offset = poff; };
    //
    // Public variables
    //
    bool isDVD;                                          // PS1 Mpeg2 of DVD differs to DVB
    unsigned int sample_rate;                            // To overwrite the sample rate
    uint_8 track;                                        // For knowing the audio track number
    //
    // Public functions
    // 
    
    // Reset all versus reset partly
    //
    void Reset(flags_t arg = 0);
    void Clear(void);
    //
    // This we need to get our external buffer
    //
    inline void SetBuffer(uint_8 *buf)
    {
        start = buf;                                     // Buffer with 2*SPDIF_BURST_SIZE bytes
        buffer_reset();
    };
    //
    // Switch to previous buffer section
    // if e.g. the current frame is broken
    //
    inline void Switch(void)
    {
        uint_8 *tmp = current;
        current  = remember;
        remember = tmp;
        payload  = current + offset;
        pcm      = (uint_16 *)&current[0];
    };
    //
    // Retrurn true is we've crossed one or more data frames
    //
    virtual const bool Count(const uint_8 *buf, const uint_8 *const tail) = 0;
    //
    // Call this in a loop to be sure to catch _all_ encoded frames
    // within out ... end,  returns a pcm frame
    //
    virtual const frame_t & Frame(const uint_8 *&out, const uint_8 *const end) = 0;
private:
    //
    // Reset Frame and Count internals
    //
    virtual const void ClassReset(void) {};
public:
    //
    // Return current pcm frame if scanner had success
    // else the last frame is given back
    //
    inline const frame_t & Frame(void) const { return last; };
    //
    // Returns various pcm frames (Pause,Start,Stop,Wait) (buffer is remember[])
    //
    const frame_t & Frame(enum_frame_t type);
    inline unsigned int BurstSize (void) const { return B2F(burst_size);  };
    inline unsigned int SampleRate(void) const { return sample_rate; };
    inline void SetErr   (void) const { if (pcm) pcm[2] |=  char2short(0x00, 0x01<<7); };
    inline void ClearErr (void) const { if (pcm) pcm[2] &= ~char2short(0x00, 0x01<<7); };
};

#endif // __IEC60958_H
