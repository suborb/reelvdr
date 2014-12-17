/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia Vertriebs GmbH                  *
 *                         E-Mail  : info @ reel-multimedia.com            *
 *                         Internet: www.reel-multimedia.com               *
 *                                                                         *
 *   This code is free software; you can redistribute it and/or modify     *
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define LOG_MODULE "hde-tool"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 1
#define LOG_ERR  1

#include "hde_tool.h"
#include <ctype.h>

void *av_realloc(void *ptr, unsigned int size) {
    if (size > (INT_MAX-16))
        return 0;
    return realloc(ptr, size);
} // av_realloc

void *av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size) {
    if(min_size < *size)
        return ptr;
    *size = FFMAX(17*min_size/16+32, min_size);
    return av_realloc(ptr, *size);
} // av_fast_realloc

const uint8_t *hde_ff_find_start_code ( const uint8_t * p, const uint8_t *end, uint32_t * state ) {
    int i;
//    assert(p<=end);
    if ( p>=end )
        return end;
    for ( i=0; i<3; i++ ) {
        uint32_t tmp= *state << 8;
        *state= tmp + * ( p++ );
        if ( tmp == 0x100 || p==end )
            return p;
    } // for
    while ( p<end ) {
        if ( p[-1] > 1 ) p+= 3;
        else if ( p[-2] ) p+= 2;
        else if ( p[-3]| ( p[-1]-1 ) ) p++;
        else {
            p++;
            break;
        } // if
    } // while
    p= FFMIN ( p, end )-4;
    *state=  bswap_32 ( unaligned32 ( p ) );
    return p+4;
} // ff_find_start_code

int mpeg_find_frame_end ( hde_frame_parser_t *this, const uint8_t *buf, int buf_size ) {
    int i;
    uint32_t state= this->state;
    i=0;
    if ( !this->frame_start_found ) {
        for ( i=0; i<buf_size; i++ ) {
            i= hde_ff_find_start_code ( buf+i, buf+buf_size, &state ) - buf - 1;
            if ( state >= SLICE_MIN_START_CODE && state <= SLICE_MAX_START_CODE ) {
                i++;
                this->frame_start_found=1;
                break;
            } // if
        } // for
    } // if
    if ( this->frame_start_found ) {
        /* EOF considered as end of frame */
        if ( buf_size == 0 )
            return 0;
        for ( ; i<buf_size; i++ ) {
            i= hde_ff_find_start_code ( buf+i, buf+buf_size, &state ) - buf - 1;
            if ( ( state&0xFFFFFF00 ) == 0x100 ) {
                if ( state < SLICE_MIN_START_CODE || state > SLICE_MAX_START_CODE ) {
                    this->frame_start_found=0;
                    this->state=-1;
                    return i-3;
                } // if
            } // if
        } // for
    } // if
    this->state= state;
    return -1;
} // mpeg_find_frame_end

void reduce_factors(uint32_t *a, uint32_t *b) {
    int m, n, r;
    if (*a > *b) {
        n = *a;
        m = *b;
    } else {
        n = *b;
        m = *a;
    } // if
    while (m) {
        r = n % m;
        n = m;
        m = r;
    } // while
    if (n > 1) {
        *a /= n;
        *b /= n;
    } // if
} // reduce_factors

void dump_memory ( unsigned char *d, int len ) {
    int i, x = 10, a = 61;
    char line[80];
    for ( i = 0; i < len; ++i ) {
        if ( ( i & 0x0f ) == 0 ) {
            sprintf ( line, "%08x", i );
            memset ( line + 8, ' ', 70 );
            line[60] = line[77] = '|';
            line[78] = 0;
            x = 10;
            a = 61;
        } // if
        sprintf ( line + x, "%02x", d[i] );
        line[x + 2] = ' ';
        x += 3;
        if ( ( i & 0x0f ) == 7 )
            ++x;
        if ( d[i] >= 32 && d[i] <= 126 )
            line[a++] = d[i];
        else
            line[a++] = '.';
        if ( ( i & 0x0f ) == 15 )
            puts ( line );
    } // for
    if ( ( len & 0xf ) != 0 ) {
        line[a++] = '|';
        line[a++] = 0;
        puts ( line );
    } // if
} // dump_memory

void *av_malloc(unsigned int size) {
	if(size > (INT_MAX-16))
		return NULL;
#ifdef HAVE_MEMALIGN
	return memalign(16,size);
#else
	return malloc(size);
#endif
} // av_malloc

void *av_mallocz(unsigned int size) {
	void *ptr = av_malloc(size);
	if(ptr)
		memset(ptr, 0, size);
	return ptr;
} // av_mallocz

void av_free(void *ptr) {
	if(ptr)
		free(ptr);
} // av_free

void av_log(void *avcl, int level, const char *fmt, ...) {
} // av_log

const uint8_t ff_log2_tab[256]={
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

int MPV_common_init (MpegEncContext *s) {
	s->mb_height = (s->height +15) /16;
	if ((s->width || s->height) && myavcodec_check_dimensions(s->avctx, s->width, s->height))
		return -1;
	s->flags = s->avctx->flags;
	s->flags2 = s->avctx->flags2;
	s->mb_width = (s->mb_width +15) /16;
	s->h_edge_pos = s->mb_width * 16;
	s->v_edge_pos = s->mb_height * 16;
	s->mb_num = s->mb_width * s->mb_height;
	s->codec_tag = toupper( s->avctx->codec_tag     & 0xFF)
				+ (toupper( s->avctx->codec_tag>>8  & 0xFF)<<8)
				+ (toupper( s->avctx->codec_tag>>16 & 0xFF)<<16)
				+ (toupper( s->avctx->codec_tag>>24 & 0xFF)<<24);
	s->current_picture_ptr = &s->current_picture;
	s->context_initialized = 1;
	return 0;
} // MPV_common_init

void MPV_common_end (MpegEncContext *s) {
	av_free(&s->parse_context.buffer);
	s->parse_context.buffer_size = 0;
	s->context_initialized = 0;
	s->current_picture_ptr = NULL;
} // MPV_common_end

void myavcodec_set_dimensions(AVCodecContext *s, int width, int height){
	s->coded_width = width;
	s->coded_height= height;
	s->width = -((-width )>>s->lowres);
	s->height= -((-height)>>s->lowres);
}

int myavcodec_check_dimensions(AVCodecContext *s, unsigned int width, unsigned int height) {
	if((int)width>0 && (int)height>0 && width+128*(uint64_t)(height+128)< INT_MAX/4)
		return 0;
	return -1;
} // myavcodec_check_dimensions

static int64_t ff_gcd(int64_t a, int64_t b) {
	if(b) return ff_gcd(b, a%b);
	else return a;
} // ff_gcd

int av_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max){
	AVRational a0={0,1}, a1={1,0};
	int sign= (nom<0) ^ (den<0);
	int64_t gcd= ff_gcd(FFABS(nom), FFABS(den));

	if(gcd){
		nom = FFABS(nom)/gcd;
		den = FFABS(den)/gcd;
	}
	if(nom<=max && den<=max){
		a1= (AVRational){nom, den};
		den=0;
	}

	while(den){
		uint64_t x      = nom / den;
		int64_t next_den= nom - den*x;
		int64_t a2n= x*a1.num + a0.num;
		int64_t a2d= x*a1.den + a0.den;

		if(a2n > max || a2d > max){
			if(a1.num) x= (max - a0.num) / a1.num;
			if(a1.den) x= FFMIN(x, (max - a0.den) / a1.den);

			if (den*(2*x*a1.den + a0.den) > nom*a1.den)
				a1 = (AVRational){x*a1.num + a0.num, x*a1.den + a0.den};
			break;
		}

		a0= a1;
		a1= (AVRational){a2n, a2d};
		nom= den;
		den= next_den;
	}
	assert(ff_gcd(a1.num, a1.den) <= 1U);

	*dst_nom = sign ? -a1.num : a1.num;
	*dst_den = a1.den;

	return den==0;
}// av_reduce
