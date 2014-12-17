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

#define LOG_MODULE "hde-ad"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 0
#define LOG_ERR  1
#define LOG_PTS  1
#define LOG_SYNC 0

#include <xine_internal.h>
#include "hde_xine.h"
#include "hde_tool.h"
#include "hde_io.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

typedef struct hde_decoder_class_s {
	audio_decoder_class_t  audio_decoder_class;
	xine_t                *xine;
	int                    instance;
} hde_decoder_class_t;

typedef struct hde_decoder_s {
	audio_decoder_t        audio_decoder;
	hde_decoder_class_t   *class;
	xine_stream_t         *stream;
	hde_scr_t             *scr;
	hde_frame_buffer_t     buffer;
	pthread_mutex_t        lock;
	int64_t                pts;
	hde_frame_parser_t     parser;
	int                    output_open;
	int                    scan_pos;
	int                    bitrate;
	int                    samplerate;
	int                    channels;
unsigned char   *dec_config;
int              dec_config_size;
} hde_decoder_t;

typedef int (*hde_audio_frame)(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra);

int hde_audio_mpa_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	uint8_t *data = &buffer->data[*pos];
	int len = buffer->pos-*pos;
	*size = MPA_HEADER_SIZE;
	while ( len > 0) {
		if(len < *size)
			return 0;
		int id = (data[1] >> 3) & 3;
		int layer = 4 - ((data[1] >> 1) & 3);
		int bri = data[2]>>4;
		int sri = (data[2]>>2)&3;
		int pad = (data[2]>>1)&1;
		int chan = (data[3]>>6)&3;
		if (data[0] != 0xFF || (data[1] & 0xE0) != 0xe0 || layer > 3 || sri > 2 || !bri || bri >= 15 || id==1) {
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
//			printf("mpeg audio out of sync!\n");
			continue;
		} // if
		int lsf  = (id==0) || (id==2);
		int mp25 = (id==0);
		*channels = (chan == 3) ? 1 : 2;
		*bitrate = mpa_bitrate_tab[lsf][layer-1][bri]*1000;
		*samplerate = mpa_samplerate_tab[sri] >> (lsf+mp25);
		switch(layer) {
			case 1 : *num_frames = 384; break;
			case 2 : *num_frames = 1152; break;
			default: *num_frames = lsf ? 576 : 1152;
		} // switch
		*size = ((*num_frames / 8 * *bitrate) / *samplerate) + pad;
		if (len < *size)
			return 0;
		//printf("num_frames %d pad %d sri %d bri %d layer %d samplerate %d lsf %d mp25 %d size %d bitrate %d channels %d\n", *num_frames, pad, sri, bri, layer, *samplerate, lsf, mp25, *size, *bitrate, *channels);
		return 1;
	} // while
	return 0;
} // hde_audio_mpa_frame

int hde_audio_ac3_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	uint8_t *data = &buffer->data[*pos];
	int len = buffer->pos-*pos;
	*size = AC3_HEADER_SIZE;
	while ( len > 0) {
		if(len < *size)
			return 0;
		int frmsizecod = data[4] & 0x3f;
		int bsid       = data[5];
		if (data[0] != 0x0b || data[1] != 0x77 || frmsizecod >=38 || bsid >= 0x60) {
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
			continue;
		} // if
		int half = ac3_halfrate[data[5]>>3];
		int acmod = data[6] >> 5;
		int flags = (((data[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) | ((data[6] & ac3_lfeon[acmod]) ? A52_LFE : 0);
		*channels = ac3_channels[flags & 7];
		if(flags & A52_LFE)
			(*channels)++;
		*bitrate = ac3_bitrate_tab[frmsizecod>>1];
		switch(data[4] & 0xc0) {
			case 0:
				*samplerate = 48000 >> half;
				*size = 4 * *bitrate;
				break;
			case 0x40:
				*samplerate = 44100 >> half;
				*size = 2 * (320 * *bitrate / 147 + (frmsizecod & 1));
				break;
			case 0x80:
				*samplerate = 32000 >> half;
				*size = 6 * *bitrate;
				break;
			default :
				*size = 0;
		} // switch
		if (*samplerate <= 0 || *bitrate <= 0 || *size <= 0) {
			llprintf(LOG_SYNC, "Audio sync lost %d %d %d\n", *samplerate, *bitrate, *size);
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
			continue;
		} // if
		if (len < *size)
			return 0;
		*num_frames = 1536;
		return 1;
	} // while
	return 0;
} // hde_audio_ac3_frame

int hde_audio_dts_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	uint8_t *data = &buffer->data[*pos];
	int len = buffer->pos-*pos;
	*size = DTS_HEADER_SIZE;
	while ( len > 0) {
		if(len < *size)
			return 0;
		int word_mode = 0;
		int be_mode = 0;
		int i, j;
		if (data[0] == 0xff && data[1] == 0x1f && data[2] == 0x00 && data[3] == 0xe8 && (data[4] & 0xf0) == 0xf0 && data[5] == 0x07) {
            // 14 le
		} else if (data[0] == 0x1f && data[1] == 0xff && data[2] == 0xe8 && data[3] == 000 && data[4] == 0x07 && (data[5] % 0xf0) == 0xf0) {
            // 14 be
			be_mode = 1;
		} else if (data[0] == 0xfe && data[1] == 0x7f && data[2] == 0x01 && data[3] == 0x80) {
            // 16 le
			word_mode = 1;
		} else if (data[0] == 0x7f && data[1] == 0xfe && data[2] == 0x80 && data[3] == 0x01) {
            // 16 be
			word_mode = 1;
			be_mode = 1;
		} else {
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
			continue;
		} // if
//printf("be %d word %d\r", be_mode, word_mode);
		uint32_t wdata[DTS_HEADER_SIZE/4];
		for(i = 0, j = 0; i < sizeof(wdata)/sizeof(uint32_t); i++, j+=4) {
			if(be_mode)
				wdata[i] = data[j]<<24|data[j+1]<<16|data[j+2]<<8|data[j+3];
			else
				wdata[i] = data[j+2]<<24|data[j+3]<<16|data[j]<<8|data[j+1];
			if (!word_mode)
				wdata[i] = (wdata[i] & 0x00003fff) | ((wdata[i] & 0x3fff0000) >> 2);
		} // for
		int nblks   = (wdata[1]>>18)&0x7f;
		int frm_len =(nblks+1)*32; // samples per frame
		int frm_size = (wdata[1]>>4)&0x3fff;
		if (frm_size == 2011) frm_size++; //will be 1005 or 2012
		if (!word_mode) 
			frm_size = frm_size * 8 / 14 * 2;
		int chan     = (((wdata[1])&0xf)<<2)|((wdata[2]>>30)&0x3);
		int sample   = (wdata[2]>>26)&0xf;
		int bit      = (wdata[2]>>21)&0x1f;
		int lfe      = (wdata[2]>>9)&0x3;
		*samplerate = dts_samplerate_tab[sample];
		*bitrate    = dts_bitrate_tab[bit];
		*channels   = dts_channel_tab[chan & DTS_CHANNEL_MASK];
		if(lfe)
			(*channels)++;
		if(frm_size < 80 || frm_size > MAX_DTS_FRAME) {
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
			continue;
		} // if
		*size = frm_size;
		if (len < *size)
			return 0;
		*num_frames = frm_len;
		*extra = nblks+1;
		return 1;
	} // while
	return 0;
} // hde_audio_dts_frame

int hde_audio_lpcm_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	if(xine_buf->decoder_flags & BUF_FLAG_SPECIAL && xine_buf->decoder_info[1] == BUF_SPECIAL_LPCM_CONFIG) {
		*channels = (xine_buf->decoder_info[2] & 0x7)+1;
		switch((xine_buf->decoder_info[2]>>4) & 3) {
			case 0: *samplerate = 48000; break;
			case 1: *samplerate = 96000; break;
			case 2: *samplerate = 44100; break;
			case 3: *samplerate = 32000; break;
			default: *samplerate = 0;
		} // switch
		switch((xine_buf->decoder_info[2]>>6) & 3) {
			case 0: *bitrate = 16; break;
			case 1: *bitrate = 20; break;
			case 2: *bitrate = 24; break;
			default: *bitrate = 16;
		} // switch
	} // if
	if(xine_buf->decoder_flags & BUF_FLAG_STDHEADER) {
		*samplerate = xine_buf->decoder_info[1];
		*bitrate    = xine_buf->decoder_info[2];
		*channels   = xine_buf->decoder_info[3];
	} // if
	if(! *samplerate || ! *bitrate || ! *channels) {
		*pos = buffer->pos;
		return 0;
	} // if
	*num_frames = *samplerate/25;
	if(gHdeIO.v.stream.framerate.den)
		*num_frames = (*samplerate * gHdeIO.v.stream.framerate.num) / gHdeIO.v.stream.framerate.den;
	*size = (*num_frames * *bitrate * *channels) / 8;
	if (*size > buffer->pos-*pos)
		return 0;
	return 1;
} // hde_audio_lpcm_frame

int hde_audio_aac_es_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	uint8_t sr=0;
	uint8_t rdb=0;
	uint8_t chan=0;
	uint8_t oti=0;
	*size = 9;
	uint8_t *data = &buffer->data[*pos];
	int len = buffer->pos-*pos;
	if(! *samplerate || !*channels) {
		*pos = buffer->pos;
	//printf("no sr ch \n");
		return 0;
	}
	for (sr = 0;sr<17;sr++){
		if (*samplerate == aac_samplerate_tab[sr])
		break;
	}
	for (chan = 0;chan<8;chan++){
		if (*channels == aac_channels[chan])
		break;
	}
	oti=2;
	unsigned char c0=(oti<<3)|(sr>>1);
	unsigned char c1=(sr<<7)|(chan<<3);
	*extra = (c0)<<8|(c1);
	*num_frames = 1024 * (rdb+1);
	if (*size > buffer->pos-*pos){
//	printf("errr %d \n",xine_buf->decoder_flags);
		*pos = buffer->pos;
		return 0;                              }
	*size = len;
//printf("%x %x %x %x %x %x %x %x %d %d \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],len,*size);
	return 1;
} // hde_audio_aac_es_frame

int hde_audio_aac_frame(buf_element_t *xine_buf, hde_frame_buffer_t *buffer, int *pos, int *size, int *bitrate, int *samplerate, int *channels, int *num_frames, int *extra) {
	uint8_t sr=0;
	uint8_t rdb=0;
	uint8_t chan=0;
	uint8_t oti=0;
	*size = 9;
	uint8_t *data = &buffer->data[*pos];
	int len = buffer->pos-*pos;
	while ( len > 0) {
		if(len < *size)
			return 0;
//printf("%x %x %x %x %x %x %x %x %d\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],len);
//printf("%x %x\n",data[0]!=0xff,(data[1]&0xf0)>>4);
//		if ((data[0]!=0xff) || (((data[1]&0xf8)>>3)!=0x1e)){
		if ((data[0]!=0xff) || (data[1]!=0xf1) || (data[3]!=0x80)){
			(*pos)++;
			len--;
			data = &buffer->data[*pos];
//			printf("aac audio out of sync!\n");
			continue;
		}
//		int pad = (data[2]>>1)&1;
//		int layer = 5; /* using 5 for AAC */
		oti = ((data[2]&0xc0) >> 6)+1;
		sr = (data[2] & 0x3c) >> 2;
		*samplerate = aac_samplerate_tab[sr];
		chan = ((data[2] & 0x01) << 2) | ((data[3] & 0xc0) >> 6);
		*channels = aac_channels[chan];
		*size = ((data[3] & 0x03) << 11) | (data[4] << 3) | (data[5] >> 5);
		if(len < *size)
			return 0;
		rdb = (data[6] & 0x03);
//		int mpg25 = (rdb+1);
		*bitrate = *size * 8 * *samplerate/(1024 *(1+rdb));
		*num_frames = 1024 * (rdb+1);
		unsigned char c0=(oti<<3)|(sr>>1);
		unsigned char c1=(sr<<7)|(chan<<3);
		*extra = (c0)<<8|(c1);
		int hlen = data[1]&0x01 ? 7:9;
		*pos+=hlen;
		*size-=hlen;
//*data = &buffer->data[*pos];
//printf("%x %x %x %x %x %x %x %x %d %d %d %d %d %x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],len,*size,oti,*samplerate,*channels,*extra);
		return 1;
	}
	return 0;

return 1;
} // hde_audio_aac_frame


#define BUFFER_EXTRA 8
static void hde_decode_data ( audio_decoder_t *this_gen, buf_element_t *buf ) {
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	hde_audio_frame frame = 0;
	int format = 0;
	pthread_mutex_lock ( &this->lock );
	switch (buf->type & (BUF_MAJOR_MASK | BUF_DECODER_MASK)) {
		case BUF_AUDIO_MPEG: 
			format = HDE_ADEC_FMT_MPEG;
			frame  = hde_audio_mpa_frame; 
			break;
		case BUF_AUDIO_A52 : 
			format = HDE_ADEC_FMT_AC3;
			frame  = hde_audio_ac3_frame; 
			break;
		case BUF_AUDIO_DTS :
			format = HDE_ADEC_FMT_DTS;
			frame  = hde_audio_dts_frame; 
			break;
		case BUF_AUDIO_LPCM_BE:
			format = HDE_ADEC_FMT_PCM_S16BE;
			frame  = hde_audio_lpcm_frame;
			break;
		case BUF_AUDIO_LPCM_LE:
			format = HDE_ADEC_FMT_PCM_S16LE;
			frame  = hde_audio_lpcm_frame;
			break;
/*		case BUF_AUDIO_AAC:
			format = HDE_ADEC_FMT_AAC;
			if ((buf->decoder_flags & BUF_FLAG_SPECIAL) && buf->decoder_info[1] == BUF_SPECIAL_DECODER_CONFIG ){
			//printf("BUF_SPECIAL_DECODER_CONFIG\n");
				this->dec_config = malloc(buf->decoder_info[2]);
				memcpy(this->dec_config, buf->decoder_info_ptr[2], buf->decoder_info[2]);
			}
			if (buf->decoder_flags & BUF_FLAG_STDHEADER){
			//printf("BUF_FLAG_STDHEADER\n");
				this->samplerate=buf->decoder_info[1];
				this->bitrate=buf->decoder_info[2];
				this->channels=buf->decoder_info[3];
			}
			if (this->dec_config)
				frame  = hde_audio_aac_es_frame;
			else
				frame  = hde_audio_aac_frame;
			break; */
		default:
			syslog(LOG_INFO, "INVALID audio buffer type 0x%x", buf->type);
			printf("INVALID audio buffer type 0x%x", buf->type);
			goto error;
	} // switch
	if (buf->pts)
		this->pts = buf->pts;
	this->buffer.data = av_fast_realloc(this->buffer.data, &this->buffer.size, this->buffer.pos+buf->size);
	memcpy(&this->buffer.data[this->buffer.pos], buf->content, buf->size);
	this->buffer.pos += buf->size;
	int lSize, num_frames, extra=0;
	while (frame(buf, &this->buffer, &this->scan_pos, &lSize, &this->bitrate, &this->samplerate, &this->channels, &num_frames, &extra)) {
//printf("size %d br %d sr %d ch %d nf %d\n",lSize,this->bitrate,this->samplerate,this->channels,num_frames);
//printf("%x %x %x %x %x \n",this->buffer.data[0],this->buffer.data[1],this->buffer.data[2],this->buffer.data[3],this->buffer.data[4]);
		if(hde_io_audio_config(format, this->samplerate, this->channels, this->bitrate, lSize, extra) || !this->output_open) {
			syslog(LOG_INFO, "->hde_decode_audio_data %x %s", buf->type, _x_buf_audio_name(buf->type));
			if(this->output_open)
				this->stream->audio_out->close(this->stream->audio_out, this->stream);
			this->output_open = this->stream->audio_out->open(this->stream->audio_out, this->stream, this->bitrate, this->samplerate, _x_ao_channels2mode(this->channels));
			if(this->output_open)
				this->stream->audio_out->control(this->stream->audio_out, AO_CTRL_NATIVE);
			hde_io_audio_reset();
		} // if
		if ( buf->decoder_flags & BUF_FLAG_PREVIEW )
			goto error;
		if(!this->output_open)
			goto error;
		audio_buffer_t *ab = this->stream->audio_out->get_buffer(this->stream->audio_out);
		ab->num_frames = num_frames;
		ab->vpts = this->pts;
		ab->stream = this->stream; // Just to be sure the hook won't send data
		this->stream->audio_out->put_buffer(this->stream->audio_out, ab, this->stream);
		hde_io_audio_frame(this->buffer.data+this->scan_pos, lSize, ab->vpts & HDE_VALID_PTS);
//printf("put %d %d \n",this->scan_pos,lSize);
		this->pts = 0;
		this->scan_pos += lSize;
	} // while
	if(this->scan_pos) {
		this->buffer.pos -= this->scan_pos;
		if(this->buffer.pos > 0)
			memcpy(this->buffer.data, &this->buffer.data[this->scan_pos], this->buffer.pos);
		else
		    this->buffer.pos = 0;
		this->scan_pos = 0;
	} // if
error:
	pthread_mutex_unlock ( &this->lock );
} // hde_decode_data

static void hde_reset ( audio_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_reset\n" );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	hde_io_audio_reset();
	if(this->output_open)
		this->stream->audio_out->close(this->stream->audio_out, this->stream);
} // hde_reset

static void hde_discontinuity ( audio_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_discontinuity\n" );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	this->pts = 0;
} // hde_discontinuity

static void hde_dispose ( audio_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_dispose\n" );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	pthread_mutex_destroy ( &this->lock );
	if(this->output_open)
		this->stream->audio_out->close(this->stream->audio_out, this->stream);
	if(this->scr) {
		this->stream->xine->clock->unregister_scr(this->stream->xine->clock, &this->scr->scr_plugin);
		this->scr->scr_plugin.exit(&this->scr->scr_plugin);
	} // if
if( this->dec_config )
  free(this->dec_config);
this->dec_config = NULL;
  this->dec_config_size = 0;

	if (this->buffer.data)
		free(this->buffer.data);
	this->class->instance = 0;
	free ( this );
} // hde_dispose

static audio_decoder_t *hde_open_plugin ( audio_decoder_class_t *class_gen, xine_stream_t *stream ) {
	llprintf ( LOG_FUNC, "hde_open_plugin\n" );
	if (!hde_io_use_audio())
		return NULL;
	hde_decoder_class_t *class = ( hde_decoder_class_t * ) class_gen;
	if(class->instance) return NULL;
	hde_decoder_t *this = ( hde_decoder_t * ) xine_xmalloc ( sizeof ( hde_decoder_t ) );
	if ( !this ) return NULL;
	this->audio_decoder.decode_data   = hde_decode_data;
	this->audio_decoder.reset         = hde_reset;
	this->audio_decoder.discontinuity = hde_discontinuity;
	this->audio_decoder.dispose       = hde_dispose;
	this->class                       = class;
	this->stream                      = stream;
	pthread_mutex_init ( &this->lock, NULL );
	this->scr = hde_scr_init(this->stream->xine);
	this->scr->scr_plugin.start(&this->scr->scr_plugin, this->stream->xine->clock->get_current_time(this->stream->xine->clock));
	this->stream->xine->clock->register_scr(this->stream->xine->clock, &this->scr->scr_plugin);
	class->instance = 1;
this->dec_config=NULL;
this->dec_config_size=0;
	return &this->audio_decoder;
} // hde_open_plugin

static char *hde_get_identifier ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_get_identifier\n" );
	return HDE_AD_ID;
} // hde_get_identifier

static char *hde_get_description ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_get_description\n" );
	return HDE_AD_DESCR;
} // hde_get_description

static void hde_class_dispose ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_class_dispose\n" );
	hde_decoder_class_t *this = ( hde_decoder_class_t * ) class_gen;
	free ( this );
} // hde_class_dispose

void *hde_ad_init ( xine_t *xine, void *visual_gen ) {
	llprintf ( LOG_FUNC, "hde_ad_init\n" );
	if (!hde_io_use_audio())
		return NULL;
	hde_decoder_class_t *this;
	this = ( hde_decoder_class_t * ) xine_xmalloc ( sizeof ( hde_decoder_class_t ) );
	if ( !this ) return NULL;
	this->audio_decoder_class.open_plugin     = hde_open_plugin;
#if (XINE_SUB_VERSION < 90 && XINE_MINOR_VERSION < 2)
	this->audio_decoder_class.get_identifier  = hde_get_identifier;
	this->audio_decoder_class.get_description = hde_get_description;
#else
	this->audio_decoder_class.identifier  = hde_get_identifier;
	this->audio_decoder_class.description = hde_get_description;
#endif
	this->audio_decoder_class.dispose         = hde_class_dispose;
	this->xine                                = xine;
	this->instance                            = 0;
	return &this->audio_decoder_class;
} // hde_ad_init

/*****************************************************************************/

typedef void (*audio_port_put_buffer)(xine_audio_port_t *port, audio_buffer_t *buf, xine_stream_t *stream);
typedef int  (*audio_port_open)(xine_audio_port_t *port, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode);
typedef void (*audio_port_flush)(xine_audio_port_t *port);
typedef void (*audio_port_close)(xine_audio_port_t *port, xine_stream_t *stream);
typedef void (*audio_port_exit)(xine_audio_port_t *port);
audio_port_put_buffer gHookAP_PutBuffer = 0;
audio_port_open       gHookAP_Open      = 0;
audio_port_flush      gHookAP_Flush     = 0;
audio_port_close      gHookAP_Close     = 0;
audio_port_exit       gHookAP_Exit      = 0;
hde_scr_t             *gHookScr         = 0;

void hde_ap_put_buffer (xine_audio_port_t *port, audio_buffer_t *ab, xine_stream_t *stream) {
//	llprintf ( LOG_FUNC, "hde_ap_put_buffer\n" );
	if(!gHookAP_PutBuffer)
		return;
	int lIsHde = (ab->stream == stream);
	gHookAP_PutBuffer(port, ab, stream);
	if(lIsHde || !ab->mem || !ab->mem_size || ab->format.bits != 16 || !ab->format.rate || !ab->format.mode)
		return;
	int lFrames   = ab->num_frames;
	int lChannels = _x_ao_mode2channels(ab->format.mode);
	int lSize = (lFrames * ab->format.bits * lChannels) / 8;
	if(hde_io_audio_config(HDE_ADEC_FMT_PCM_S16LE, ab->format.rate, lChannels, ab->format.bits, lSize, 0))
		syslog(LOG_INFO, "->hde_decode_audio_data via hook");
	hde_io_audio_frame((uint8_t *)ab->mem, lSize, ab->vpts & HDE_VALID_PTS);
} // hde_ap_put_buffer


int hde_ap_open(xine_audio_port_t *port, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode) {
	llprintf ( LOG_FUNC, "hde_ap_open %p, bits %d rate %d mode %d\n", gHookAP_Open, bits, rate, mode);
	if(gHookScr && stream) 
		stream->xine->clock->register_scr(stream->xine->clock, &gHookScr->scr_plugin);
	else
		syslog(LOG_INFO, "hde_ap_open: HookScr or stream not set! (%p/%p)", gHookScr, stream);
	if(gHookAP_Open)
		return gHookAP_Open(port, stream, bits, rate, mode);
	return 0;
} // hde_ap_open

void hde_ap_flush(xine_audio_port_t *port) {
	llprintf ( LOG_FUNC, "hde_ap_flush\n");
	if(gHookAP_Flush)
		gHookAP_Flush(port);
} // hde_ap_flush

void hde_ap_close(xine_audio_port_t *port, xine_stream_t *stream) {
	llprintf ( LOG_FUNC, "hde_ap_close\n" );
	if(gHookScr && stream)
		stream->xine->clock->unregister_scr(stream->xine->clock, &gHookScr->scr_plugin);
	else
		syslog(LOG_INFO, "hde_ap_close: HookScr or stream not set! (%p/%p)", gHookScr, stream);
	if(gHookAP_Close)
		gHookAP_Close(port, stream);
} // hde_ap_close

void hde_ap_exit(xine_audio_port_t *port) {
	llprintf ( LOG_FUNC, "hde_ap_exit\n" );
	if(gHookAP_Exit)
		gHookAP_Exit(port);
	if(gHookScr)
		gHookScr->scr_plugin.exit(&gHookScr->scr_plugin);
	else
		syslog(LOG_INFO, "hde_ap_close: HookScr not set!");
	gHookScr    = 0; 
} // hde_ap_exit
		
static audio_decoder_t *hde_hook_open_plugin ( audio_decoder_class_t *class_gen, xine_stream_t *stream ) {
	llprintf ( LOG_FUNC, "hde_hook_open_plugin\n" );
	if (!hde_io_use_audio())
		return NULL;
	if(hde_ap_put_buffer != stream->audio_out->put_buffer) {
		gHookAP_PutBuffer             = stream->audio_out->put_buffer;
		stream->audio_out->put_buffer = hde_ap_put_buffer;
	} // if
	if(hde_ap_open != stream->audio_out->open) {
		gHookAP_Open            = stream->audio_out->open;
		stream->audio_out->open = hde_ap_open;
	} // if
	if(hde_ap_flush != stream->audio_out->flush) {
		gHookAP_Flush            = stream->audio_out->flush;
		stream->audio_out->flush = hde_ap_flush;
	} // if
	if(hde_ap_close != stream->audio_out->close) {
		gHookAP_Close            = stream->audio_out->close;
		stream->audio_out->close = hde_ap_close;
	} // if
	if(hde_ap_exit != stream->audio_out->exit) {
		gHookAP_Exit            = stream->audio_out->exit;
		stream->audio_out->exit = hde_ap_exit;
	} // if
	gHookScr = hde_scr_init(stream->xine);
	gHookScr->scr_plugin.start(&gHookScr->scr_plugin, stream->xine->clock->get_current_time(stream->xine->clock));
	return NULL;
} // hde_hook_open_plugin

static char *hde_hook_get_identifier ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_hook_get_identifier\n" );
	return HDE_AD_HOOK_ID;
} // hde_hook_get_identifier

static char *hde_hook_get_description ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_hook_get_description\n" );
	return HDE_AD_HOOK_DESCR;
} // hde_hook_get_description

static void hde_hook_class_dispose ( audio_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_hook_class_dispose\n" );
	gHookAP_PutBuffer = 0;
	gHookAP_Open      = 0;
	gHookAP_Exit      = 0;
	hde_decoder_class_t *this = ( hde_decoder_class_t * ) class_gen;
	free ( this );
} // hde_hook_class_dispose

void *hde_ad_hook_init(xine_t *xine, void *visual_gen) {
	llprintf ( LOG_FUNC, "hde_ad_hook_init\n" );
	if (!hde_io_use_audio())
		return NULL;
	audio_decoder_class_t *this;
	this = ( audio_decoder_class_t * ) xine_xmalloc ( sizeof ( audio_decoder_class_t ) );
	if ( !this ) return NULL;
	this->open_plugin     = hde_hook_open_plugin;
#if (XINE_SUB_VERSION < 90 && XINE_MINOR_VERSION < 2)
	this->get_identifier  = hde_hook_get_identifier;
	this->get_description = hde_hook_get_description;
#else
	this->identifier  = hde_hook_get_identifier;
	this->description = hde_hook_get_description;
#endif
	this->dispose         = hde_hook_class_dispose;
	return this;
} // hde_ad_hook_init
