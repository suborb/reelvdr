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

#define LOG_MODULE "hde-vd"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 0
#define LOG_ERR  1
#define LOG_WARN 1

#define XINE_ENGINE_INTERNAL

#include <xine_internal.h>
#include "hde_xine.h"
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
#include <byteswap.h>

#include "parser.h"

uint8_t gSeqEndCode[] = { ( SEQ_END_CODE>>24 ) &0xFF, ( SEQ_END_CODE>>16 ) &0xFF, ( SEQ_END_CODE>>8 ) &0xFF, SEQ_END_CODE&0xFF};

typedef struct hde_decoder_class_s {
	video_decoder_class_t  video_decoder_class;
	xine_t                *xine;
	int                    instance;
} hde_decoder_class_t;

typedef struct hde_decoder_s {
	video_decoder_t        video_decoder;
	hde_decoder_class_t   *class;
	xine_stream_t         *stream;
	hde_scr_t             *scr;
	pthread_mutex_t        lock;
	int64_t                pts;
	hde_frame_buffer_t     buffer;
	int                    last_skip;
	int                    force_repeat;
	int                    scan_pos;
	hde_io_video_stream_t  stream_info;
	hde_io_video_stream_t  stream_hint;
	int                    force_pan_scan;       // to use a certain aspect or to do pan&scan
	int                    force_aspect_idx;
	int                    fields;
	int                    top_first;
	AVCodecParserContext  *parser;
	AVCodecContext         context;
} hde_decoder_t;

#define SYNC_HDE 0x10000000

static void check_context ( AVCodecContext *avctx, const hde_io_video_stream_t *hint, hde_io_video_stream_t *si ) {
	if (hint->width && hint->height) {
		si->width  = hint->width;
		si->height = hint->height;
	} else if (avctx->width && avctx->height) {
		si->width  = avctx->width;
		si->height = avctx->height;
	} // if
	if(si->width < 96 || si->height < 72) {
		si->width  = 720;
		si->height = 576;
	} // if
	if ( avctx->time_base.den > 100 && avctx->time_base.num == 1 ) 
		avctx->time_base.num = 0;
	if(hint->framerate.num && hint->framerate.den)	{
		si->framerate.num = hint->framerate.num;
		si->framerate.den = hint->framerate.den;
	} else if(avctx->time_base.num && avctx->time_base.den) {
		si->framerate.num = avctx->time_base.num;
		si->framerate.den = avctx->time_base.den;
	} // if
	if(!si->framerate.num || !si->framerate.den || !(si->framerate.den/si->framerate.num) || (si->framerate.den/si->framerate.num > 30) || (si->framerate.den/si->framerate.num < 23)) {
		si->framerate.num=1;
		si->framerate.den=25;
	} // if
	if(hint->aspect.num && hint->aspect.den)	{
		si->aspect.num = hint->aspect.num;
		si->aspect.den = hint->aspect.den;
	} else if(avctx->sample_aspect_ratio.num && avctx->sample_aspect_ratio.den) {
		if (si->format == HDE_PIX_FMT_H264) {
			float asp = (float)avctx->sample_aspect_ratio.num / (float)avctx->sample_aspect_ratio.den * (float)avctx->width / (float)avctx->height;
			int frate = (si->framerate.den/si->framerate.num == 25 || si->framerate.den/si->framerate.num == 50) ? 25 : 30;
			if (asp > (frate == 25 ? 1.67 : 2.0)){
				si->aspect.num = 16;
				si->aspect.den = 9;
			} else {
				si->aspect.num = 4;
				si->aspect.den = 3;
			}
		} else {
			si->aspect.num = avctx->sample_aspect_ratio.num;
			si->aspect.den = avctx->sample_aspect_ratio.den;
		}
	} // if
	if(!si->aspect.num || !si->aspect.den) {
		si->aspect.num=4;
		si->aspect.den=3;
	} // if
//printf("%x %x %x \n",si->format,avctx->codec_id,avctx->codec_tag);
/* Makes no difference...
	if (si->format == HDE_PIX_FMT_MPEG1 || si->format == HDE_PIX_FMT_MPEG2) {
		switch(avctx->codec_id) {
			case CODEC_ID_MPEG1VIDEO: si->format = HDE_PIX_FMT_MPEG1; break;
			case CODEC_ID_MPEG2VIDEO: si->format = HDE_PIX_FMT_MPEG2; break;
			default: break;
		} // switch
	} // if
*/
} // check_context

static void hde_decode_data ( video_decoder_t *this_gen, buf_element_t *buf ) {
	llprintf ( LOG_FUNC, "hde_decode_data 0x%08x %lld %lld 0x%08x\n", buf->type, buf->pts, buf->disc_off, buf->decoder_flags );
//	printf ( "hde_decode_data 0x%08x %lld %lld 0x%08x %x %x %x %x %x\n", buf->type, buf->pts, buf->disc_off, buf->decoder_flags,buf->decoder_info[0],buf->decoder_info[1],buf->decoder_info[2],buf->decoder_info[3],buf->decoder_info[4] );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	pthread_mutex_lock ( &this->lock );
		if ( buf->decoder_flags & BUF_FLAG_FRAMERATE ) {
		// Frame rate wont be correct if pts is not set
		//TODO add dynamic frame rate correction if value has big changes?
		if(buf->pts && this->stream_hint.framerate.den != 90000 && buf->decoder_info[0] > 3000 && buf->decoder_info[0] < 4000) {
			this->stream_hint.framerate.num = buf->decoder_info[0];
			this->stream_hint.framerate.den = 90000;
		} // if
	} // if
	switch ( buf->type ) {
		case BUF_VIDEO_MPEG  :
			if(!this->parser || (this->stream_info.format != HDE_PIX_FMT_MPEG1 && this->stream_info.format != HDE_PIX_FMT_MPEG2)) {
				this->stream_info.aspect.num = 4;
				this->stream_info.aspect.den = 3;
				this->stream_info.framerate.num = 1;
				this->stream_info.framerate.den = 25;
				this->stream_info.width  = 720;
				this->stream_info.height = 576;
				if(this->parser)
					hde_av_parser_close(this->parser);
				this->parser = hde_av_parser_init(CODEC_ID_MPEG2VIDEO);
				if(this->parser)
					this->stream_info.format = HDE_PIX_FMT_MPEG2;
				else
					goto error;
				AVCodecContext old = this->context;
				memset(&this->context, 0, sizeof(this->context));
				this->context.codec_tag = old.codec_tag;
				this->context.extradata_size = old.extradata_size;
				this->context.extradata = old.extradata;
			} // if
			break;
		case BUF_VIDEO_MPEG4 :
		case BUF_VIDEO_XVID  :
		case BUF_VIDEO_3IVX  :
		case BUF_VIDEO_DIVX5 :
			if (!this->parser || this->stream_info.format != HDE_PIX_FMT_MPEG4 ) {
				this->stream_info.aspect.num = 4;
				this->stream_info.aspect.den = 3;
				this->stream_info.framerate.num = 1;
				this->stream_info.framerate.den = 25;
				this->stream_info.width  = 720;
				this->stream_info.height = 576;
				if ( this->parser )
					hde_av_parser_close ( this->parser );
				this->parser = hde_av_parser_init ( CODEC_ID_MPEG4 );
				if ( this->parser )
					this->stream_info.format = HDE_PIX_FMT_MPEG4;
				else
					goto error;
				AVCodecContext old = this->context;
				memset ( &this->context, 0, sizeof ( this->context ) );
				this->context.codec_tag = old.codec_tag;
				this->context.extradata_size = old.extradata_size;
				this->context.extradata = old.extradata;
			} // if
			break;
		case BUF_VIDEO_H264  :
			if (!this->parser || this->stream_info.format != HDE_PIX_FMT_H264 ) {
				this->stream_info.aspect.num = 1;
				this->stream_info.aspect.den = 1;
				this->stream_info.framerate.num = 1;
				this->stream_info.framerate.den = 25;
				this->stream_info.width  = 1920;
				this->stream_info.height = 1080;
				if ( this->parser )
					hde_av_parser_close ( this->parser );
				this->parser = hde_av_parser_init ( CODEC_ID_H264 );
				if ( this->parser )
					this->stream_info.format = HDE_PIX_FMT_H264;
				else
					goto error;
				AVCodecContext old = this->context;
				memset ( &this->context, 0, sizeof ( this->context ) );
				this->context.codec_tag = old.codec_tag;
				this->context.extradata_size = old.extradata_size;
				this->context.extradata = old.extradata;
			} // if
			break;
/*		case BUF_VIDEO_FLV1  :
			if (!this->parser || this->stream_info.format != HDE_PIX_FMT_H264 ) {
				this->stream_info.aspect.num = 1;
				this->stream_info.aspect.den = 1;
				this->stream_info.framerate.num = 1;
				this->stream_info.framerate.den = 25;
				this->stream_info.width  = 720;
				this->stream_info.height = 576;
				if ( this->parser )
					hde_av_parser_close ( this->parser );
				this->parser = hde_av_parser_init ( CODEC_ID_H264 );
				if ( this->parser )
					this->stream_info.format = HDE_PIX_FMT_H264;
				else
					goto error;
				AVCodecContext old = this->context;
				memset ( &this->context, 0, sizeof ( this->context ) );
				this->context.codec_tag = old.codec_tag;
				this->context.extradata_size = old.extradata_size;
				this->context.extradata = old.extradata;
			} // if
			break;*/
		default:
			syslog(LOG_INFO, "INVALID video buffer type 0x%x", buf->type);
			if ( this->parser )
				hde_av_parser_close ( this->parser );
			this->parser = NULL;
			goto error;
	} // switch
	if ( buf->decoder_flags & BUF_FLAG_ASPECT ) {
		this->stream_hint.aspect.num = buf->decoder_info[0];
		this->stream_hint.aspect.den = buf->decoder_info[1];
	} // if
	if (( buf->decoder_flags & BUF_FLAG_FRAME_END) && ( buf->decoder_flags & BUF_FLAG_STDHEADER) && ( buf->size >= sizeof ( xine_bmiheader ))) {
		xine_bmiheader hdr;
		memcpy ( &hdr, buf->content, sizeof ( xine_bmiheader ) );
		this->stream_hint.width  = hdr.biWidth;
		this->stream_hint.height = hdr.biHeight;
		this->context.codec_tag  = hdr.biCompression;
//printf("hdr %x\n",this->context.codec_tag);
if(!hdr.biCompression && buf->type == BUF_VIDEO_H264) this->context.codec_tag = ff_get_fourcc("avc1"); //for flv
//if(!hdr.biCompression && buf->type == BUF_VIDEO_FLV1) this->context.codec_tag = ff_get_fourcc("avc1"); //for flv
		this->context.extradata_size = (hdr.biSize-sizeof ( xine_bmiheader ));
		if(this->context.extradata_size)
			this->context.extradata = malloc(this->context.extradata_size);
		if(this->context.extradata)
			memcpy(this->context.extradata, &buf->content[sizeof ( xine_bmiheader )], this->context.extradata_size);
		else
			this->context.extradata_size = 0;
		if ( buf->decoder_flags & BUF_FLAG_FRAMERATE) {
			this->stream_hint.framerate.num = buf->decoder_info[0];
			this->stream_hint.framerate.den = 90000;
		} else {
			printf("Ignore Framerate: %d\n", buf->decoder_info[0]);
		} // if
		pthread_mutex_unlock ( &this->lock );
		return;
	} // if
//	if ( buf->decoder_flags & BUF_FLAG_FRAMERATE ) {
		// Frame rate wont be correct if pts is not set
//		if(buf->pts && this->stream_hint.framerate.den != 90000 && buf->decoder_info[0] > 3000 && buf->decoder_info[0] < 4000) {
//			this->stream_hint.framerate.num = buf->decoder_info[0];
//			this->stream_hint.framerate.den = 90000;
//		} else {
//			printf("Ignore Framerate: %d pts %lld %d\n", buf->decoder_info[0], buf->pts, this->stream_hint.framerate.den);
//		} // if
//	} // if
	if ( buf->decoder_flags & BUF_FLAG_SPECIAL ) {
		if ( buf->decoder_info[1] == BUF_SPECIAL_ASPECT ) {
			switch(buf->decoder_info[2]) {
				case 2: this->stream_hint.aspect.num =   4; this->stream_hint.aspect.den =   3; break;
				case 3: this->stream_hint.aspect.num =  16; this->stream_hint.aspect.den =   9; break;
				case 4: this->stream_hint.aspect.num = 221; this->stream_hint.aspect.den = 100; break;
			} // switch
		} else if (buf->decoder_info[1] == BUF_SPECIAL_DECODER_CONFIG) {
			if(!this->context.extradata_size) {
				this->context.extradata_size = buf->decoder_info[2];
				char *ptr = (char *)buf->decoder_info_ptr[2];
				// esds
				if(this->context.extradata_size > 0)
					this->context.extradata = malloc(this->context.extradata_size);
				if(this->context.extradata)
					memcpy(this->context.extradata, ptr, this->context.extradata_size);
				else
					this->context.extradata_size = 0;
			} // if
		} else if (buf->decoder_info[1] == BUF_SPECIAL_STSD_ATOM) {
			if(!this->context.extradata_size) {
				int size = buf->decoder_info[2];
				char *ptr = (char *)buf->decoder_info_ptr[2];
				if(size > 0) {
					int pos;
					for(pos = 0; pos < size-5; pos++)
						if(ptr[pos]=='a' && ptr[pos+1]=='v' && ptr[pos+2]=='c' && ptr[pos+3]=='C')
							break;
					if(pos < size-5) {
						this->context.extradata_size=size-pos-4;
						this->context.extradata = malloc(this->context.extradata_size);
						memcpy(this->context.extradata, &ptr[pos+4], this->context.extradata_size);
					} // if
				} // if
			} // if
		} // if
	} // if
	if ( buf->decoder_flags & BUF_FLAG_PREVIEW ) {
		pthread_mutex_unlock ( &this->lock );
		return;
	} // if
	int parser_pts= !( buf->decoder_flags & BUF_FLAG_FRAMERATE );
	if((this->stream_info.format != HDE_PIX_FMT_H264) && !( buf->decoder_flags & BUF_FLAG_FRAMERATE )) {
		parser_pts=0;
//		buf->decoder_info[0] = 0;
//		buf->decoder_flags |= BUF_FLAG_FRAMERATE;
	} // if
	if ( buf->pts )
		this->pts = buf->pts;
	int64_t current_pts = this->pts;
	int in_len = buf->size;
	unsigned char *in_data = buf->content;
	while ( in_len ) {
		unsigned char *data = NULL;
		int size = 0;
		int64_t vpts_offset = this->stream->metronom->get_option(this->stream->metronom, METRONOM_VPTS_OFFSET);
		int64_t av_offset   = this->stream->metronom->get_option(this->stream->metronom, METRONOM_AV_OFFSET);
		int len = hde_av_parser_parse ( this->parser, &this->context, &data, &size, in_data, in_len, 
										this->pts+vpts_offset+av_offset,
										this->pts-buf->decoder_info[0]+vpts_offset+av_offset);
		check_context ( &this->context, &this->stream_hint, &this->stream_info );
		if(hde_io_video_config ( this->stream_info ))
			syslog(LOG_INFO, "->hde_decode_video_data %x %s %d", buf->type, _x_buf_video_name(buf->type), buf->decoder_flags & BUF_FLAG_FRAMERATE);
		in_data += len;
		in_len  -= len;
		if ( size ) {
			double aspect = 1;
			if(gHdeIO.v.stream.aspect.den) 
				aspect = ( ( double ) gHdeIO.v.stream.aspect.num ) / gHdeIO.v.stream.aspect.den;
			vo_frame_t *img = this->stream->video_out->get_frame ( this->stream->video_out, gHdeIO.v.stream.width, gHdeIO.v.stream.height, aspect, XINE_IMGFMT_HDE, VO_BOTH_FIELDS | ( this->force_pan_scan ? VO_PAN_SCAN_FLAG : 0 ) );
			img->pts        = this->pts;
			int skip_hde = 0;
//this->last_skip=0;
			if ( this->last_skip & ~SYNC_HDE )
				img->bad_frame = 1;
			if ( this->last_skip && (this->parser->pict_type == FF_B_TYPE) && (this->stream_info.format == HDE_PIX_FMT_MPEG2)) {
				skip_hde = 1;
			} else if ( ( this->last_skip & SYNC_HDE ) && ( this->parser->pict_type != FF_I_TYPE ) ) {
				skip_hde = 1;
			} else if ( gHdeIO.v.stream.framerate.num && this->last_skip > ( gHdeIO.v.stream.framerate.den / gHdeIO.v.stream.framerate.num ) && this->parser->pict_type != FF_I_TYPE ) {
				skip_hde = 1;
			} // if
			int64_t speed = 0;
			if ( hde_io_get_speed ( &speed ) ) {
				if (( speed >= 360000) && (this->parser->pict_type != FF_I_TYPE)) {
					skip_hde = 1;
				} else if (( speed >= 180000) && (this->parser->pict_type == FF_B_TYPE)) {
					skip_hde = 1;
				} else if (( speed >= 180000)  && (this->parser->pict_type != FF_I_TYPE) && (this->stream_info.format != HDE_PIX_FMT_MPEG2)) {
					skip_hde = 1;
				} // if
			} // if
			uint64_t pts = HDE_UNSPEC_PTS;
			if(this->last_skip & SYNC_HDE || speed >= 180000)
				pts = 0;
			if ( gHdeIO.v.stream.framerate.den )
				img->duration  = ( 90000ll*gHdeIO.v.stream.framerate.num ) /gHdeIO.v.stream.framerate.den;
			if(this->parser->repeat_pict < 0)
				img->duration /= 2;
			else if (this->parser->repeat_pict)
				img->duration *= 2;
			if((buf->decoder_flags & BUF_FLAG_FRAMERATE)/* && buf->decoder_info[0]*/)
				img->duration = buf->decoder_info[0];
			if((this->parser->repeat_pict==-2) && this->force_repeat)
				skip_hde=0; // Don skip on interlaced frames if other half was sent
			if(!(buf->decoder_flags & BUF_FLAG_FRAMERATE) || current_pts)
				this->last_skip = img->draw ( img, this->stream );
			else if(this->last_skip > 0)
				this->last_skip = 0;
			if(parser_pts ||(this->parser->pict_type == FF_I_TYPE)) //&& this->stream_info.format == HDE_PIX_FMT_H264)
				if(pts && this->parser->pts && this->parser->pts != AV_NOPTS_VALUE)
					pts = this->parser->pts & HDE_VALID_PTS;
			if(!parser_pts && (pts==HDE_UNSPEC_PTS) && (this->parser->pict_type == FF_I_TYPE))
				pts = img->vpts; // Needed to set pts or hde will use now and thats too late (stillframe till next i-frame)
			if((this->last_skip > (img->duration ? 90000/img->duration : 25)) && img->vpts) {
				printf("seeking to 0x%llx due to last_skip = %d (%lld %lld)\n", img->vpts, this->last_skip, vpts_offset, av_offset);
				this->class->xine->clock->adjust_clock(this->class->xine->clock, img->vpts);
//				this->scr->scr_plugin.adjust(&this->scr->scr_plugin, (img->vpts-this->last_skip*90000)/(img->duration ? 90000/img->duration : 25));
//				pts = 0;
			} // if
			if ((pts & HDE_VALID_PTS) && ((pts & HDE_VALID_PTS) < 4000))
				printf("low pts %lld %lld %lld\n", (pts & HDE_VALID_PTS), img->vpts, this->parser->pts);
			int is_still = 0;
			if(_x_stream_info_get ( this->stream, XINE_STREAM_INFO_VIDEO_HAS_STILL ))
				if(in_len == sizeof(gSeqEndCode) && !memcmp(&data[size], gSeqEndCode, sizeof(gSeqEndCode)))
					is_still = 1;
			this->force_repeat = 0;
			if ( is_still) {
				hde_io_video_frame ( data, size, ( pts & HDE_VALID_PTS ) | HDE_FLAG_STILLFRAME_PTS );
			} else if ( skip_hde && this->parser->pict_type != FF_B_TYPE ) {
				this->last_skip |= SYNC_HDE;
			} else if ( !skip_hde && !hde_io_video_frame ( data, size, pts)) {
				this->last_skip |= SYNC_HDE;
			} else if(!skip_hde && (this->parser->repeat_pict==-1)) {
				this->force_repeat = 1;
			} // if
			if ( ( this->last_skip || skip_hde ) && ( speed <= 90000 ) )
				llprintf (LOG_WARN, "skip=%d (%d/%d)\n", this->last_skip & ~SYNC_HDE, skip_hde, ( this->last_skip & SYNC_HDE ) ? 1 : 0 );
			img->free ( img );
		} // if
		this->pts = 0;
	} // while
error:
	pthread_mutex_unlock ( &this->lock );
} // hde_decode_data

static void hde_reset ( video_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_reset\n" );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	pthread_mutex_lock ( &this->lock );
	hde_io_video_resync();
	hde_io_video_reset();
	this->buffer.pos = 0;
	this->scan_pos   = 0;
	this->pts        = 0;
	if(this->stream_info.format != HDE_PIX_FMT_MPEG2) 
		this->last_skip |= SYNC_HDE;
	if(this->parser)
		hde_av_parser_close(this->parser);
	this->parser=NULL;
	pthread_mutex_unlock ( &this->lock );
} // hde_reset

static void hde_discontinuity ( video_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_discontinuity\n" );
//	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
//	this->pts      = 0;
} // hde_discontinuity

static void hde_flush ( video_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_flush\n" );
} // hde_flush

static void hde_dispose ( video_decoder_t *this_gen ) {
	llprintf ( LOG_FUNC, "hde_dispose\n" );
	hde_decoder_t *this = ( hde_decoder_t * ) this_gen;
	pthread_mutex_destroy ( &this->lock );
	if ( this->scr ) {
		this->stream->xine->clock->unregister_scr ( this->stream->xine->clock, &this->scr->scr_plugin );
		this->scr->scr_plugin.exit ( &this->scr->scr_plugin );
	} // if
	if ( this->buffer.data )
		free ( this->buffer.data );
	if(this->parser)
		hde_av_parser_close(this->parser);
	if(this->context.extradata)
		free(this->context.extradata);
	this->class->instance = 0;
	free ( this );
} // hde_dispose

static video_decoder_t *hde_open_plugin ( video_decoder_class_t *class_gen, xine_stream_t *stream ) {
	llprintf ( LOG_FUNC, "hde_open_plugin\n" );
	if ( !gHdeIO.video )
		return NULL;
	hde_decoder_t *this;
	hde_decoder_class_t *class = ( hde_decoder_class_t * ) class_gen;
	if ( class->instance ) return NULL;
	this = ( hde_decoder_t * ) xine_xmalloc ( sizeof ( hde_decoder_t ) );
	if ( !this ) return NULL;
	this->video_decoder.decode_data   = hde_decode_data;
	this->video_decoder.reset         = hde_reset;
	this->video_decoder.discontinuity = hde_discontinuity;
	this->video_decoder.flush         = hde_flush;
	this->video_decoder.dispose       = hde_dispose;
	this->class                       = class;
	this->stream                      = stream;
	pthread_mutex_init ( &this->lock, NULL );
	this->stream->metronom->set_option(this->stream->metronom, METRONOM_PREBUFFER, 90000);
	this->stream->video_out->open ( this->stream->video_out, this->stream );
	this->scr = hde_scr_init ( this->stream->xine );
	this->scr->scr_plugin.start ( &this->scr->scr_plugin, this->stream->xine->clock->get_current_time ( this->stream->xine->clock ) );
	this->stream->xine->clock->register_scr ( this->stream->xine->clock, &this->scr->scr_plugin );
	class->instance = 1;
	return &this->video_decoder;
} // hde_open_plugin

static char *hde_get_identifier ( video_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_get_identifier\n" );
	return HDE_VD_ID;
} // hde_get_identifier

static char *hde_get_description ( video_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_get_description\n" );
	return HDE_VD_DESCR;
} // hde_get_description

static void hde_class_dispose ( video_decoder_class_t *class_gen ) {
	llprintf ( LOG_FUNC, "hde_class_dispose\n" );
	hde_decoder_class_t *this = ( hde_decoder_class_t * ) class_gen;
	free ( this );
} // hde_class_dispose

void *hde_vd_init ( xine_t *xine, void *visual_gen ) {
	llprintf ( LOG_FUNC, "hde_vd_init\n" );
	if ( !hde_io_use_video())
		return NULL;
	hde_decoder_class_t *this;
	this = ( hde_decoder_class_t * ) xine_xmalloc ( sizeof ( hde_decoder_class_t ) );
	if ( !this ) return NULL;
	this->video_decoder_class.open_plugin     = hde_open_plugin;
#if (XINE_SUB_VERSION < 90 && XINE_MINOR_VERSION < 2)
	this->video_decoder_class.get_identifier  = hde_get_identifier;
	this->video_decoder_class.get_description = hde_get_description;
#else
	this->video_decoder_class.identifier  = hde_get_identifier;
	this->video_decoder_class.description = hde_get_description;
#endif
	this->video_decoder_class.dispose         = hde_class_dispose;
	this->xine                                = xine;
	this->instance                            = 0;
	hde_av_register_codec_parser(&hde_mpegvideo_parser);
	hde_av_register_codec_parser(&hde_mpeg4video_parser);
	hde_av_register_codec_parser(&hde_h264_parser);
	return &this->video_decoder_class;
} // hde_vd_init
