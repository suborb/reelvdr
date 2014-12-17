/*****************************************************************************
*
* demux.c - TS/PES demux for DeCypher
*
* Copyright (C) 2006-2007 Micronas
* Copyright (C) 2008 Georg Acher (acher (at) baycom dot de)
* based on code
* Copyright (C) 1999-2002 Ralph  Metzler
*                       & Marcus Metzler for convergence integrated media GmbH
*
* #include <gpl_v2.h>
*
******************************************************************************/

#include "audio_parse.h"
#include "video_parse.h"
#include "h264_helper.h"
#include "demux.h"
#include <linux/go8xxx-video.h>
#include <hdchannel.h>
#include <sys/time.h>
#include <syslog.h>
extern int chiprev;
extern hd_data_t volatile *hda;

#define vdec_printf(format, args...) printf("%i: " format, vdec->id, ##args)

/*-------------------------------------------------------------------------*/
inline u32 get_time ( void ) {
	struct timeval tv;
	gettimeofday ( &tv, 0 );
	return ( tv.tv_sec ) * 1000 + tv.tv_usec/1000;
}
/*-------------------------------------------------------------------------*/
#define CMP_CORE(offset) \
        x=Data[i+1+offset]; \
        if (x<2) { \
                if (x==0) { \
                        if ( Data[i+offset]==0 && Data[i+2+offset]==1) \
                                return i+offset; \
                        else if (Data[i+2+offset]==0 && Data[i+3+offset]==1) \
                                return i+1+offset; \
                } \
                else if (x==1 && Data[i-1+offset]==0 && Data[i+offset]==0 && (i+offset)>0) \
                         return i-1+offset; \
         }
/*-------------------------------------------------------------------------*/
int FindPacketHeader ( const u8 *Data, int s, int l ) {
	int i;
	u8 x;

	if ( l>12 ) {
		for ( i=s;i<l-12;i+=12 ) {
			CMP_CORE ( 0 );
			CMP_CORE ( 3 );
			CMP_CORE ( 6 );
			CMP_CORE ( 9 );
		}
		for ( ;i<l-3;i+=3 ) {
			CMP_CORE ( 0 );
		}
	} else {
		for ( i=s; i<l-3; i+=3 ) {
			CMP_CORE ( 0 );
		}
	}
	return -1;
}
/*----------------------------------------------------------------------------*/
u8 const *findMarker ( u8 const *Data, int Count, int Offset, u8 *PictureType ) {
	int Length = Count;

	*PictureType = NO_PICTURE;
	if ( Length > 0 ) {
		uint8_t const *p = Data + Offset;
		uint8_t const *pLimit = Data + Offset + Length - 3;
		while ( p < pLimit ) {
			int x;
			x=FindPacketHeader ( p, 0, pLimit - p );
			if ( x!=-1 ) {         // found 0x000001
				p+=x+2;
				switch ( p[1] ) {
					case 0:
						*PictureType = ( p[3] >> 3 ) & 0x07;
						return p-2;

					case 9: // Access Unit Delimiter AUD h.264
						if ( p[2]==0x10 )
							*PictureType=I_FRAME;
						else
							*PictureType=B_FRAME;
						return p-2; //
					case 0xe0 ... 0xef:
					case 0x67:
					case 0xb3:
					case 0xb5:
					case 0xbd:
					case 0xc0:
					case 0xc8: //needed for BBC HD
						return p-2;
				}
			} else
				break;
		}
	}
	return NULL;
}
/*----------------------------------------------------------------------------*/
int video_analyze_es ( vdec_t *vdec, const u8 *data, int len, int flag ) {
	unsigned char const *x;
	unsigned char z,pt;
	int l,pos=0;

	if ( flag ) {
		while ( 1 ) {
			x=findMarker ( data,len,pos, &pt );
			if ( !x )
				return 0;
			l=data+len-x;
			z=x[3];
			if ( z==0x00 ) {
				if ( vdec->tmp_video_mode<0 )
					printf ( "####################### VIDEO SYNC MPEG2\n" );
				vdec->tmp_video_mode=GO8XXX_PIX_FMT_MPEG2;
				return 1;
			}

			if ( ( z==0x67 || z==0x09 ) &&vdec->tmp_video_mode!=GO8XXX_PIX_FMT_MPEG2 ) {
				if ( vdec->tmp_video_mode<0 )
					printf ( "####################### SYNC H.264\n" );
				vdec->tmp_video_mode=GO8XXX_PIX_FMT_H264;
				return 1;
			}
			pos=x-data+4;
		}
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static inline int
pts_in_range ( u64 start, u64 val, u64 end ) {
	start &= 0x1ffffffffULL;
	end &= 0x1ffffffffULL;
	if ( start <= end )
		return ( ( start <= val ) && ( val < end ) );
	else
		return ( ( start <= val ) || ( val < end ) );
}
/*----------------------------------------------------------------------------*/
// doad=1 -> no audio
void vdec_adjust_pts ( vdec_t *vdec, u64 *pts, u64 off, int doad ) {
	u64 npts;
//        struct go_demux *dmx;
	
	if ( !vdec )
		return;

	if ( *pts==NOPTS_VALUE )
		return;

	if ( !vdec->stc_init ) {
		decoder_stc_init ( vdec, *pts );
		printf ( "STC INIT %09llx\n",*pts );
	}

	npts= ( *pts+vdec->pts_off ) &0x1ffffffffULL;

	if ( !doad ||
	                pts_in_range ( vdec->prev_pts-PTS_RANGE, npts, vdec->prev_pts+PTS_RANGE ) ) {
		*pts=npts;
		if ( doad )
			vdec->prev_pts=*pts;
		return;
	}
	printf ( "PPTS=%09llx NPTS=%09llx off=%d\n", vdec->prev_pts, npts, ( int ) off);
	if ( vdec->first_pts==NOPTS_VALUE ) {
		u64 stc=vdec_get_stc ( vdec );
		vdec->first_pts=*pts;
		printf ( "STC%d=%09llx PTS=%09llx  old offset=%09llx\n", vdec->id, stc, *pts, vdec->pts_off );
		vdec->pts_off= ( ((int64_t)stc)-*pts+off );
		*pts= ( *pts+vdec->pts_off ) &0x1ffffffffULL;
		vdec->frameno=0; // Force Focus sync
	} else {
		vdec->old_pts_count = 50; // max 50 frames (2 sec)
		vdec->old_pts_off = vdec->pts_off;
		vdec->pts_off= ( ((int64_t)vdec->prev_pts)-*pts+off );
		*pts= ( *pts+vdec->pts_off ) &0x1ffffffffULL;
#ifndef SMOOTH_AUDIO_CHG
		vdec->frameno=0; // Force Focus sync
#endif
	}
	printf ( "New PTS offset %09llx, PTS=%09llx\n", vdec->pts_off, *pts );
	vdec->frameno=0; // Force Focus sync
	vdec->prev_pts=*pts;
}
/*----------------------------------------------------------------------------*/
int adec_push_es ( vdec_t *vdec, const u8 *buf,
                   unsigned int buf_size, int pes_start, u64 pts,
                   u32 error ) {
	adec_t *adec=vdec->adec;
	if (vdec->id)
		printf("PUSH");
	if ( !adec )
		return 0;

	if ( vdec && pes_start && pts!=NOPTS_VALUE ) {
	} else
		pts=NOPTS;

	if ( pes_start )
		adec->pes_count++;

	if ( adec->audio_on ) {
		if( !audio_parse_ac3 ( ( u8 * ) buf, buf_size, &adec->af, pes_start, pts, error ))
			vdec->need_reset++;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
int vdec_qbuf ( vdec_t *vdec, u32 start, u32 len, u64 pts, u32 flags ) {
	int ret;

	ret=decoder_video_qbuf ( vdec, start, len, pts, flags );

	return ret;
}
/*----------------------------------------------------------------------------*/
static u32 ves_buf_free ( vdec_t *vdec ) {
	return decoder_vbuf_free ( vdec );
}
/*----------------------------------------------------------------------------*/
#if 0
static void push_h264 ( vdec_t *vdec ) {
	int len=vdec->last_len-3;
	u32 end=vdec->last_start+len;

	if ( end>=vdec->ves_size )
		end-=vdec->ves_size;

	if ( len>0 ) {
		/* Align to M1 padding length of 256.
		   This way the frame will be flushed right away instead
		   of when the next frame is queued.
		*/
		if ( end&0xff ) {
			u32 n=0x100- ( end&0xff );
			memset ( vdec->ves_buf+end, 0x55, n );
			len+=n;
			end+=n;
			if ( end>=vdec->ves_size )
				end-=vdec->ves_size;
		}
		vdec_qbuf ( vdec, vdec->last_start,
		            len, vdec->last_pts,
		            vdec->last_flags );
	}
	vdec->last_len=3;
	vdec->last_pts=vdec->next_pts;
	vdec->next_pts=NOPTS_VALUE;
	vdec->last_start=end;
	vdec->last_flags=0;
}
#endif
/*----------------------------------------------------------------------------*/
#define PUSH_H264(B) {\
                wb=(wb>>8)|(B<<24);             \
                if ((wp&3)==3)                  \
                        wbuf[wp>>2]=wb;         \
                if (++wp==vdec->ves_size)       \
                        wp=0;                   \
                vdec->last_len++;               \
        }
#if 0
static int write_es_h264 ( vdec_t *vdec,
                           const u8 *es,
                           unsigned int len,
                           int pusi, u64 pts, u32 error ) {
	u32 *wbuf;
	u32 v=0, *wes;
	u32 off;
	u32 wb=vdec->ves_wb;
	u32 wp=vdec->ves_wp;
	u32 rb=vdec->ves_rb;
	u32 w;

	if ( pusi ) {
		vdec->pes_count++;
		vdec->next_pts=pts;
	}
	if ( !len )
		return 0;

	while ( 1 ) {
		if ( ( ves_buf_free ( vdec ) >len+8192 ) )
			break;
		usleep ( 10*1000 );
	}

	wes= ( u32 * ) ( ( ~3UL ) & ( u32 ) es );
	vdec->ves_size=VES_BUF_SIZE ( vdec );
	vdec->ves_buf= ( u8 * ) ( VES_BUF_START ( vdec ) );
	wbuf= ( u32 * ) ( u32 ) ( VES_BUF_START ( vdec ) );
	off=3UL& ( u32 ) es;

	if ( error ) {
		vdec->ves_state=2;
		vdec->ves_nalu_pos=-1;
		vdec->ves_wp=vdec->last_start;
		vdec->ves_wb=wes[vdec->ves_wp>>2];
		if ( vdec->ves_wp&3 ) {
			vdec->ves_wb=wbuf[vdec->ves_wp>>2];
			vdec->ves_wb<<= ( 4- ( vdec->ves_wp&3 ) );
		} else
			vdec->ves_wb=0;
		vdec->last_len=0;
		vdec->ves_rb=0xffffffff;
		vdec->next_pts=NOPTS_VALUE;
		vdec->last_pts=NOPTS_VALUE;
		return 0;
	}
	if ( off )
		v= ( * ( wes++ ) ) >> ( off<<3 );
	while ( len ) {
		if ( !off )
			v=* ( wes++ );
		w=rb>>8;
		rb=w| ( v<<24 );

		if ( vdec->ves_state==0 ) {
			if ( w!=0x010000 )
				goto skip;
			if ( ( v&0x1f ) !=0x07 )
				goto skip;
			vdec->last_len+=3;
			wb=w<<8;
			wp+=3;
		}
		if ( vdec->ves_state==1 ) {
			if ( w!=0x010000 )
				goto skip;
			if ( ( v&0x1f ) !=0x09 )
				goto skip;
			vdec->last_len+=3;
			wb=w<<8;
			wp+=3;
		}
		if ( w==0x010000 ) {
			u8 t=vdec->ves_nalu&0x1f;

			//if (vdec->ves_nalu_pos>0)
			//parse_nalu(vdec);
			//vdec->ves_nalu_pos=0;
			vdec->ves_nalu=rb>>24;

			vdec->ves_nalu_start=vdec->ves_wp;
			vdec->ves_nalu_start_len=vdec->last_len;

			if ( ( vdec->ves_state==2 && t==9 ) ||
			                ( vdec->ves_state<2 && t==7 ) ) {
				pusi=0;
				vdec->ves_state=2;
				push_h264 ( vdec );
				wp=vdec->last_start+3;
				wb=0x01000000;
			}
		}
		//if (vdec->ves_nalu_pos<64)
		//vdec->ves_nalu_buf[vdec->ves_nalu_pos]=v&0xff;
		//vdec->ves_nalu_pos++;

		wb= ( wb>>8 ) | ( v<<24 );
		if ( ( wp&3 ) ==3 )
			wbuf[wp>>2]=wb;
		if ( ++wp==vdec->ves_size )
			wp=0;
		vdec->last_len++;
	skip:
		v>>=8;
		off= ( off+1 ) &3;
		len--;
	}
	vdec->ves_rb=rb;
	vdec->ves_wb=wb;
	vdec->ves_wp=wp;
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static int write_es ( vdec_t *vdec,
                      const u8 *es,
                      int len ) {
	u8 *buf;
	u32 wp;
	int vfree;

	if ( len<=0 )
		return 0;

	while ( 1 ) {
		vfree=ves_buf_free ( vdec );
		if ( vfree>len ) //  && (vfree>3*1024*1024 || vdec->video_mode==GO8XXX_PIX_FMT_H264 || vdec->wakeup_throttle))
			break;
		usleep ( 10*1000 );
	}

	vdec->wakeup_throttle=0;
	vdec->last_len+=len;
	buf=VES_BUF_START ( vdec );
	wp=vdec->ves_wp;
	if ( wp+len>VES_BUF_SIZE ( vdec ) ) {
		u32 len1=VES_BUF_SIZE ( vdec )-wp;

		memcpy ( buf+wp, es, len1 );
		len-=len1;
		es+=len1;
		wp=0;
	}

	memcpy ( buf+wp, es, len );
	wp+=len;
	vdec->ves_wp=wp;
	return vdec->last_len;
}
/*----------------------------------------------------------------------------*/
#define time_after(a,b)         \
         ((long)(b) - (long)(a) < 0)

//#define WRITE_VIDEO_ES
#ifdef WRITE_VIDEO_ES
static int fr_c=0;
static int fd_out=-1;
#endif
int vdec_write_es ( vdec_t *vdec,
                    const u8 *es,
                    int len,
                    int pusi, u64 pts, u32 flags,
                    u32 error ) {
	unsigned long now;
	int ret;
	if ( !vdec || len<0 )
		return 0;

	if((vdec->video_mode==GO8XXX_PIX_FMT_H264) && (len >=4) && (es[0]==0) && (es[1]==0) && (es[2]==1) && (es[3]==11)) {
		printf("GOT EOS NAL %d\n", len);
		vdec->need_reset=NEED_RESET_TRESHOLD;
		return len;
	}

	if ( vdec->video_mode==-1 /*&& vdec->last_len*/ ) {
		if ( video_analyze_es ( vdec, es, len, 1 ) ) {
			video_parse ( &vdec->vattr, ( u8* ) es, len );
			vdec->video_mode=vdec->tmp_video_mode;
			video_decoder_configure ( vdec );
#ifdef WRITE_VIDEO_ES
			if(fd_out!=-1) close(fd_out);
			char file[256];
			sprintf(file, "/mnt/out%d.es", ++fr_c);
			fd_out=open(file, O_CREAT|O_WRONLY);
			if(fd_out==-1) printf("failed to open file %s\n", file);
#endif
		}
	}
	if ( vdec->video_mode==-1 )
		return 0;
	video_attr_t vattr = vdec->vattr;
	if(!video_parse ( &vattr, ( u8* ) es, len )){
//ioctl ( vdec->decoder_fd, GO8XXXIOC_FLUSH );
//ioctl ( vdec->yuv_fd, GO8XXXIOC_FLUSH );
//wis_dec_set_nodelay(vdec->decoder_fd,1);
//return len;
}
//else wis_dec_set_nodelay(vdec->decoder_fd,0);
	if ( vdec->vattr.aspect_w!=vattr.aspect_w || vdec->vattr.aspect_h!=vattr.aspect_h)
		vdec->aspect_chg_pts = pts;
int a=0;
	if ( vdec->vattr.framerate_nom!=vattr.framerate_nom || vdec->vattr.framerate_div!=vattr.framerate_div ||
	(vdec->vattr.height == 480 && hda->video_mode.height == 576) || (vdec->vattr.height == 576 && hda->video_mode.height == 480)){
//syslog(LOG_WARNING,"==== old fn %d old fd %d new fn %d new fd %d frame rate\n",vdec->vattr.framerate_nom,vdec->vattr.framerate_div,vattr.framerate_nom,vattr.framerate_div,vdec->frame_rate);
//syslog(LOG_WARNING,"==== frame rate %d \n",vdec->frame_rate);
a=1;
}
	vdec->vattr = vattr;
if (a){
//video_display_configure ( vdec );
video_decoder_configure ( vdec );

}
//hd_plane_config_t conf = hda->plane[2];
//syslog(LOG_WARNING,"########## x %d y %d w %d h %d mode %d ws %d hs %d scale %d v %d %d %d %d \n",conf.x,conf.y,conf.w,conf.h,conf.mode,conf.ws,conf.hs,conf.osd_scale,conf.vx,conf.vy,conf.vw,conf.vh);
//vdec->pes_count = 0;
//vdec->cont=-1;
//vdec->last_start=0;
//vdec->last_len=0;
//vdec->frameno=0;


#if 0
	if ( vdec->video_mode==GO8XXX_PIX_FMT_H264 ) {
		int ret=write_es_h264 ( vdec, es, len, pusi, pts, error );
#if 0
		if ( ret<0 )
			printf ( "R %d\n", ret );
#endif
		return ret;
	}
#endif
	if ( pusi )
		vdec->pes_count++;
	now=get_time();
	if ( pusi||time_after ( now, vdec->last_jiffies ) ) {
		if ( !pusi )
			pts=NOPTS_VALUE;

		if ( vdec->last_len ) {
			while(ves_buf_free(vdec)<vdec->last_len) // security check, helps for vdr 1.7.9 and Pro7...
				usleep(20*1000);				
			if (vdec->need_reset==NEED_RESET_TRESHOLD)
				return len;
			vdec_qbuf ( vdec, vdec->last_start,
			            vdec->last_len, vdec->last_pts,
			            vdec->last_flags );
		}
		vdec->last_len=0;
		vdec->last_pts=pts;
		vdec->last_jiffies=now+300; //ms
		vdec->last_start=vdec->ves_wp;
		vdec->last_flags=flags;
	}
	ret=write_es ( vdec, es, len );
#ifdef WRITE_VIDEO_ES
	if(fd_out!=-1) write(fd_out, es, len);
#endif
	return ret;
}
/*----------------------------------------------------------------------------*/
static inline u64 parse_pts ( const u8 * p ) {
	u64 pts;
	int val;

	pts = ( u64 ) ( ( p[0] >> 1 ) & 0x07 ) << 30;
	val = ( p[1] << 8 ) | p[2];
	pts |= ( u64 ) ( val >> 1 ) << 15;
	val = ( p[3] << 8 ) | p[4];
	pts |= ( u64 ) ( val >> 1 );

	return pts;
}
/*----------------------------------------------------------------------------*/
static void pes_reset ( pes_context_t *pes ) {
	pes->state = MPEGTS_SKIP;
	pes->data_index = 0;
	pes->pts = NOPTS_VALUE;
}
/*----------------------------------------------------------------------------*/
static int video_pes_header_error = 0;
static int audio_pes_header_error = 0;
static int unknown_pes_header_error = 0;

void decoder_reset(vdec_t *vdec) {
	printf ( "Perform decoder reset\n" );
//syslog(LOG_WARNING,"Perform decoder reset\n" );
	int tm = vdec->trickmode;
	int ts = vdec->trickspeed;
	int dl = vdec->pts_delay;
	if (chiprev==0) {
		decoder_stop(vdec);
		hda->video_mode.changed++; // restart whole player
		printf("Wait for restart by hdctrld\n");
		sleep(10000);
	}
	decoder_setup ( vdec,vdec->video_id, vdec->adec->audio_id);
	vdec->pts_delay  = dl;
	vdec->trickmode  = tm;
	vdec->trickspeed = ts;
}

int vdec_write_pes ( vdec_t *vdec, u8 * buf, int buf_size,
                     int is_start, int pes_type ) {
	adec_t *adec=vdec->adec;
	pes_context_t *pes;
	int len, ret=0, first=0;
	u8 *p, id;
	pes = &vdec->pes[pes_type];
	if ( is_start ) {
		pes->state = MPEGTS_HEADER;
		pes->data_index = 0;
	}
	p = buf;

	while ( buf_size ) {
		switch ( pes->state ) {
			case MPEGTS_HEADER:
				len = PES_START_SIZE - pes->data_index;
				if ( len > buf_size )
					len = buf_size;

				memcpy ( pes->header + pes->data_index,
				         p, len );
				pes->data_index += len;
				p += len;
				buf_size -= len;
				if ( pes->data_index < PES_START_SIZE )
					break;
				pes->state = MPEGTS_SKIP;

				if ( pes->header[0] || pes->header[1] || pes->header[2] != 0x01 ) {
					// if is_start is set (->state == MPEGTS_HEADER) it has to start with pes start code!
					// If not, drop all data till next pes start. Maybe this packet is bad crypted!
					pes_reset(pes);
					switch(pes_type) {
						case TS_PES_VIDEO: 
							if(!video_pes_header_error)
								printf(">>>>>> Dropped due to bad video pes header!\n");
							if(vdec->display_configured) {
								printf("Wrong MPEGTS_HEADER %i %i %i\n",pes->header[0], pes->header[1], pes->header[2]);
								decoder_reset(vdec); // Force reset
							}
							video_pes_header_error++; 
							break;
						case TS_PES_AUDIO: 
							if(!audio_pes_header_error)
								printf(">>>>>> Dropped due to bad audio pes header!\n");
							audio_pes_header_error++; 
							break;
						default          : 
							if(!unknown_pes_header_error)
								printf(">>>>>> Dropped due to bad unknown pes header! (%d)\n", pes_type);
							unknown_pes_header_error++;
					} // switch
					return 0;
				} else {
				    switch(pes_type) {
					case TS_PES_VIDEO: video_pes_header_error = 0; break;
					case TS_PES_AUDIO: audio_pes_header_error = 0; break;
					default          : unknown_pes_header_error = 0; break;
				    } // switch
				}

				id=pes->header[3];
				if ( pes_type==TS_PES_VIDEO ) {
					if ( ! ( ( id >= 0xe0 && id <= 0xef ) || ( id == 0xfd ) ) ) {
						printf(">>>>>> Dropped due to invalid video pid 0x%04x!\n", id);
						pes_reset(pes);
						return 0;
					}
				}
				if ( pes_type==TS_PES_AUDIO ) {
					if ( ! ( ( id >= 0xc0 && id <= 0xdf ) || ( id == 0xbd ) ) ) {
						printf(">>>>>> Dropped due to invalid audio pid 0x%04x!\n", id);
						pes_reset(pes);
						return 0;
					}

					if ( adec ) {
						if ( id==0xbd )
							adec->af.type
							= AC3;
						else
							adec->af.type
							= MPEG_AUDIO;
					}
				}
				pes->state = MPEGTS_PESHEADER_FILL;
				pes->total_size =
				        ( pes->header[4] << 8 ) |
				        pes->header[5];
				if ( pes->total_size )
					pes->total_size += 6;
				pes->pes_header_size = pes->header[8] + 9;
				break;

			case MPEGTS_PESHEADER_FILL:
				len = pes->pes_header_size - pes->data_index;
				if ( len > buf_size )
					len = buf_size;
				memcpy ( pes->header + pes->data_index, p, len );
				pes->data_index += len;
				p += len;
				buf_size -= len;
				if ( pes->data_index == pes->pes_header_size ) {
					const u8 *r;
					u8 flags;

					flags = pes->header[7];
					r = pes->header + 9;
					pes->pts = NOPTS_VALUE;
					pes->dts = NOPTS_VALUE;
					if ( flags & 0x80 )
						pes->pts = parse_pts ( r );
					if ( pes_type==TS_PES_VIDEO )
						pes->state = MPEGTS_FIRST_PAYLOAD_VIDEO;
					else
						pes->state = MPEGTS_FIRST_PAYLOAD;
				}
				break;
			case MPEGTS_FIRST_PAYLOAD:
				first=1;
				pes->state = MPEGTS_PAYLOAD;
			case MPEGTS_PAYLOAD:
				if ( pes->total_size ) {
					len = pes->total_size - pes->data_index;
					if ( len > buf_size )
						len = buf_size;
					pes->data_index += len;
				} else {
					len = buf_size;
				}
				if ( adec && len) {
					ret |= adec_push_es ( vdec, p, len,
					                      first, pes->pts, 0 );
					pes->pts = NOPTS_VALUE;
				}
				if ( len<=0 ) {
					printf ( "MPEGTS_PAYLOAD AUDIO: Len<=0\n" );
					buf_size=0;
				}
				buf_size-=len;
				if(buf_size)printf("buf_size %d\n", buf_size);
				break;

			case MPEGTS_FIRST_PAYLOAD_VIDEO:
				first=1;
				pes->state = MPEGTS_PAYLOAD_VIDEO;
			case MPEGTS_PAYLOAD_VIDEO:
				if ( pes->total_size ) {
					len = pes->total_size - pes->data_index;
					if ( len > buf_size )
						len = buf_size;
					pes->data_index += len;
printf("%d %x %02x %02x %d\n", pes->total_size, pes->total_size, pes->header[4], pes->header[5], len);
				} else {
					len = buf_size;
				}
buf_size-=				vdec_write_es ( vdec, p, len,
				                first, pes->pts,
				                0, 0 );
//				buf_size-=len;
//                     printf("BUFSIZE %i %i %i %i\n",buf_size, len, pes->total_size, pes->data_index);
				if ( len<=0 ) {
					printf ( "MPEGTS_PAYLOAD VIDEO: Len<=0\n" );
					buf_size=0;
				}
				break;
			case MPEGTS_SKIP:
				buf_size = 0;
				break;
			default:
				printf ( "PES parsing: Invalid state\n" );
		}
	}

	// Emergency
	if ( vdec->need_reset>=NEED_RESET_TRESHOLD ) {
		printf("vdec_write_pes: Reached reset threshold\n");
		decoder_reset(vdec);
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static inline void automagic_video ( vdec_t *vdec, const u8 *buf, int len, u16 pid ) {
	if ( !vdec || vdec->video_id )
		return;
	if ( len>=4 && !buf[0] && !buf[1] && buf[2]==1 &&
	                ( ( buf[3]>=0xe0 && buf[3]<=0xef ) || buf[3]==0xfd ) )
		vdec->video_id=pid;
}
/*----------------------------------------------------------------------------*/
static inline void automagic_audio ( adec_t *adec, const u8 *buf, int len, u16 pid ) {
	if ( !adec || adec->audio_id )
		return;
	if ( len>=4 && !buf[0] && !buf[1] && buf[2]==1 &&
	                ( ( buf[3]>=0xc0 && buf[3]<=0xdf ) || buf[3]==0xbd || buf[3]==0xfd ) )
		adec->audio_id=pid;
}
/*----------------------------------------------------------------------------*/
u8 xxbuf[7];
int vdec_write_tsp ( vdec_t *vdec, u8 *buf ) {
	adec_t *adec=vdec->adec;
	int pusi=0, len=188;
	u16 pid= ( ( buf[1]&0x1f ) <<8 ) |buf[2];
	int cont = buf[3]&0x0f;
	u8 *obuf=buf;

	if ( buf[0]!=0x47 )
		return -1;

	if( buf[3]&0xC0) // Scrambled
		return 0;

	if ( ! ( buf[3] & 0x10 ) ) /* no payload? */
		return 0;

	if ( buf[1]&0x40 )
		pusi=1;

	if ( buf[3] & 0x20 ) {  /* adaptation field? */
		len -= buf[4] + 1;
		buf += buf[4] + 1;
		if ( len<=4 )
			return 0;
	}
	len-=4;
	buf+=4;
	/*
	        if (pusi) {
	                automagic_video(vdec, buf, len, pid);
	                automagic_audio(adec, buf, len, pid);
	        }
	*/
	if ( pid==vdec->video_id ) {
		if((vdec->cont != -1) && ((vdec->cont+1)%0x10 != cont) && IS_PLAY(vdec) ) {
//		    printf(">>>>>> Video counter mismatch: %d (%d)\n", cont, (vdec->cont+1)%0x10);
//syslog(LOG_WARNING,">>>>>> Video counter mismatch: %d (%d)\n", cont, (vdec->cont+1)%0x10);
		    if(vdec->display_configured) { // Already sent data
//			    printf("#### Cont error old %i now %i, hdr %02x %02x %02x %02x %02x %02x %02x\n",vdec->cont, cont,obuf[0],obuf[1],obuf[2],obuf[3],obuf[4],obuf[5],obuf[6]);
//			    printf("####                      old  hdr %02x %02x %02x %02x %02x %02x %02x\n",xxbuf[0],xxbuf[1],xxbuf[2],xxbuf[3],xxbuf[4],xxbuf[5],xxbuf[6]);
//syslog(LOG_WARNING,"-----------Already sent data \n");
//		    	decoder_reset(vdec);
//decoder_setup ( vdec,vdec->video_id, vdec->adec->audio_id);
vdec->cont = -1;
vdec->pes_count=0;
		    } else {
//syslog(LOG_WARNING,"-----------pes_reset \n");
		    	vdec->cont = -1;
		    	pes_reset(&vdec->pes[TS_PES_VIDEO]);
		    }
		} else {
			int nn;
			for(nn=0;nn<7;nn++) xxbuf[nn]=obuf[nn];
		    vdec_write_pes ( vdec, ( u8 * ) buf, len, pusi, TS_PES_VIDEO );
		    vdec->cont = cont;
		} // if
	} else if ( adec && pid==adec->audio_id )
		vdec_write_pes ( vdec, ( u8 * ) buf, len, pusi, TS_PES_AUDIO );
	return 0;

}
/*----------------------------------------------------------------------------*/
int vdec_write_ts_block ( vdec_t * vdec, u8 *data, int len ) {
	int i;
	if ( !vdec->run_video_out || len<=0 )
		return 0;
	for ( i=0; i<len; i+=188 )
		vdec_write_tsp ( vdec, data+i );
	return 0;
}
/*----------------------------------------------------------------------------*/
void vdec_reset ( vdec_t *vdec ) {
	int n;

	pes_reset ( &vdec->pes[0] );
	pes_reset ( &vdec->pes[1] );

	vdec->ves_free=vdec->ves_size;
	vdec->ves_rb=0xffffffff;
	vdec->ves_wb=0;
	vdec->ves_wp=0;
	vdec->ves_rp=0;
	vdec->ves_nalu=-1;
	vdec->ves_nalu_pos=-1;
	vdec->ves_nalu_start=0;
	vdec->ves_nalu_start_len=0;

	vdec->ves_state=0;
	vdec->last_len=0;
	vdec->last_pts=NOPTS_VALUE;
	vdec->last_start=0;
	vdec->first_pts=NOPTS_VALUE;
	vdec->last_flags=0;
	vdec->pes_count = 0;
	vdec->cont=-1;
	
	vdec->prev_pts=NOPTS_VALUE;

	memset ( &vdec->vattr, 0, sizeof ( video_attr_t ) );
	memset ( &vdec->vattr_old, 0, sizeof ( video_attr_t ) );

	vdec->stc_init=0;
	vdec->pause=0;
	vdec->old_pause=0;
	vdec->display_configured=0;
	vdec->found_valid_pts=0;
	vdec->run_video_out=1;
	vdec->force_field=0;
	vdec->video_mode=-1;
	vdec->tmp_video_mode=-1;
	vdec->trickspeed=0;
	vdec->old_trickspeed=0;
	vdec->trickmode=1;
	vdec->old_trickmode=1;
	vdec->wakeup_throttle=0;
	vdec->frameno=0;

	vdec->video_id=0;

	vdec->pts_delay=0;
	vdec->pts_delay_flag=0;
	vdec->stc_diff=0;
	vdec->decoder_frames=0;
	vdec->last_decoder_pts=0;
	vdec->got_video_data=0;
	vdec->last_orig_pts=0;
	
	vdec->need_reset=0;
	vdec->adec->audio_id=0;
	vdec->adec->af.type=NONE;
	vdec->adec->af.set=0;
	vdec->adec->delay=0;
	vdec->adec->audio_on=1;
	vdec->adec->cur_audio_type=NONE;

	vdec->adec->af.callback=adec_callback;
	vdec->adec->af.config=adec_config;
	vdec->adec->af.mainbuf=vdec->adec->abuf;
	vdec->adec->af.mainsize=sizeof ( vdec->adec->abuf );
	vdec->adec->af.priv=vdec;
	vdec->adec->af.lastpts=NOPTS;
	vdec->adec->af.nextpts=NOPTS;
	vdec->adec->af.bpf=0;
	vdec->adec->af.sub_type=0;
	vdec->adec->af.bit_rate=0;

	for ( n=0;n<AUDIO_QUEUE_BUFS;n++ )
		vdec->adec->aqueue[n].len=0;
	vdec->adec->wp=0;
	vdec->adec->rp=0;
}
/*----------------------------------------------------------------------------*/
extern hd_data_t volatile *hda;

static hd_vdec_config_t last_video_cfg = {0,};
int vdec_write_cfg ( vdec_t *vdec, hd_vdec_config_t *cfg ) {
	int tm,ts;
	printf("vdec_write_cfg\n");
	if ( last_video_cfg.av_sync != cfg->av_sync ) {
		last_video_cfg.av_sync = cfg->av_sync;
		decoder_force_av ( vdec );
	} // if
	if ( !memcmp ( &last_video_cfg, cfg, sizeof ( last_video_cfg ) ) ) {
		if ( ( vdec->video_mode          == cfg->pixelformat ) &&
		                ( vdec->vattr.aspect_w      == cfg->asp_width ) &&
		                ( vdec->vattr.aspect_h      == cfg->asp_height ) &&
		                ( vdec->vattr.framerate_nom == cfg->rate_num ) &&
		                ( vdec->vattr.width         == cfg->pic_width ) &&
		                ( vdec->vattr.height        == cfg->pic_height ) &&
		                ( vdec->vattr.framerate_div == cfg->rate_den ) )
			return 0;
	} // if
	printf ( "videocfg %d %d\n", last_video_cfg.generation, cfg->generation );
	last_video_cfg = *cfg;
	tm=vdec->trickmode;
	ts=vdec->trickspeed;
	vdec->vattr.width         = cfg->pic_width;
	vdec->vattr.height        = cfg->pic_height;
	decoder_setup ( vdec, 0, 0 );
	vdec->trickmode=tm;
	vdec->trickspeed=ts;
	vdec->video_mode          = cfg->pixelformat;
	vdec->vattr.aspect_w      = cfg->asp_width;
	vdec->vattr.aspect_h      = cfg->asp_height;
	vdec->vattr.framerate_nom = cfg->rate_num;
	vdec->vattr.framerate_div = cfg->rate_den;
	vdec->aspect_chg_pts      = 0;
	video_decoder_configure ( vdec );
	decoder_force_av ( vdec );
	return 0;
} // vdec_write_cfg


#define STILL_FRAME_REPEAT (1000/25)
static u32 still_frame_show = 0;
static int still_frame_len  = 0;
static u8 *still_frame_data = NULL;
int vdec_write_data ( vdec_t *vdec, uint64_t pts, u8 *data, int len ) {
//	printf("vdec_write_data: %lld %d\n", pts, len);
	if ( vdec->video_mode == -1 ) {
		if ( !last_video_cfg.pixelformat || vdec_write_cfg ( vdec, &last_video_cfg ) )
			return -1;
	} // if
	if(!vdec->display_configured && ( pts & HDE_UNSPEC_PTS ) && !vdec->found_valid_pts) {
		printf("Dropped video data %llx\n", pts);
		return 0; // don't let first frames be without pts
	} else if(!( pts & HDE_UNSPEC_PTS )) {
		vdec->found_valid_pts = 1;
	} // if
	if ( pts & HDE_FLAG_FRAME_START_PTS ) {
		vdec->ves_wp     = vdec->last_start;
		vdec->last_flags = 0;
		vdec->last_len   = 0;
		vdec->last_pts   = ( pts & HDE_UNSPEC_PTS ) ? NOPTS_VALUE : ( pts & HDE_VALID_PTS ); 
		if(!vdec->got_video_data) {
			if(vdec->last_pts == NOPTS_VALUE)
				vdec->last_pts = 0;
		} else if(vdec->last_pts == 0)
			vdec->last_pts = NOPTS_VALUE;
		if (!vdec->got_video_data && vdec->last_pts) printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>set pts to init %llx\n", vdec->last_pts);
#if 0
		static u64 my_last_vpts = 0;
		if (vdec->last_pts != NOPTS_VALUE) {
			if(vdec->last_pts < my_last_vpts)
				printf("v-pts mismatch %llx -> %llx\n", my_last_vpts, vdec->last_pts);
			my_last_vpts = vdec->last_pts;
		} // if
#endif
//		if(vdec->last_pts!=NOPTS_VALUE) printf("video pts %llx %llx %llx\n", vdec->last_pts, vdec->stc_diff, vdec_get_stc(vdec));
	} // if
	still_frame_show = 0;
	if ( len)
		write_es ( vdec, data, len );
	if ( pts & HDE_FLAG_STILLFRAME_PTS ) { // prepare data for repeat
		if ( pts & HDE_FLAG_FRAME_START_PTS )
			still_frame_len = 0;
		still_frame_data = ( u8 * ) realloc ( still_frame_data, still_frame_len+len );
		memcpy ( &still_frame_data[still_frame_len], data, len );
		still_frame_len += len;
	} // if
#ifdef CONT_STILL_FRAME_REPEAT
	// Continuous still frame repeation
	if ( pts & HDE_FLAG_FRAME_END_PTS && vdec->last_len ) {
		if ( pts & HDE_FLAG_STILLFRAME_PTS ) {
			vdec->last_pts = NOPTS_VALUE;
			still_frame_show = get_time() +STILL_FRAME_REPEAT;
		} else if ( still_frame_data ) {
			free ( still_frame_data );
			still_frame_data = NULL;
			still_frame_len  = 0;
		} // if
		vdec_qbuf ( vdec, vdec->last_start, vdec->last_len, vdec->last_pts, vdec->last_flags );
		vdec->last_flags = 0;
		vdec->last_len   = 0;
		vdec->last_start = vdec->ves_wp;
	} // if
#else
	// Normal still frame n-time repeat
	if ( pts & HDE_FLAG_FRAME_END_PTS && vdec->last_len ) {
		if(vdec->last_pts && (vdec->last_pts != NOPTS_VALUE))
			vdec->got_video_data = 1;
		if ( pts & HDE_FLAG_STILLFRAME_PTS ) {
			int repeat = 8;
			while ( --repeat ) {
//				if(vdec->last_pts != NOPTS_VALUE) printf("put pts: %llx\n", vdec->last_pts);
				vdec_qbuf ( vdec, vdec->last_start, vdec->last_len, vdec->last_pts, vdec->last_flags );
				vdec->last_pts = NOPTS_VALUE;
				vdec->last_flags = 0;
				vdec->last_len   = 0;
				vdec->last_start = vdec->ves_wp;
				write_es ( vdec, still_frame_data, still_frame_len );
			} // while
			free ( still_frame_data );
			still_frame_data = NULL;
			still_frame_len  = 0;
		} // if
//		if(vdec->last_pts != NOPTS_VALUE) printf("put pts: %llx\n", vdec->last_pts);
		vdec_qbuf ( vdec, vdec->last_start, vdec->last_len, vdec->last_pts, vdec->last_flags );
		vdec->last_flags = 0;
		vdec->last_len   = 0;
		vdec->last_start = vdec->ves_wp;
	} // if
#endif
	if ( vdec->need_reset>=NEED_RESET_TRESHOLD ) {
		printf ( "Issue decoder reset (vdec_write_data)\n" );
		hd_vdec_config_t lCfg = last_video_cfg;
		last_video_cfg.pixelformat=-1;
		vdec_write_cfg ( vdec,&lCfg );
	}
	return 0;
} // vdec_write_data

int vdec_check_stillframe ( vdec_t *vdec ) {
	if ( still_frame_show && ( still_frame_show < get_time() ) && still_frame_data && still_frame_len ) {
		syslog ( LOG_INFO, "vdec_check_stillframe: %d %d %d", still_frame_len, vdec->video_mode, last_video_cfg.pixelformat );
		write_es ( vdec, still_frame_data, still_frame_len );
		vdec_qbuf ( vdec, vdec->last_start, vdec->last_len, vdec->last_pts, vdec->last_flags );
		vdec->last_flags = 0;
		vdec->last_len   = 0;
		vdec->last_start = vdec->ves_wp;
		still_frame_show = get_time() +STILL_FRAME_REPEAT;
	} // if
	return still_frame_show;
} // vdec_check_stillframe

/*----------------------------------------------------------------------------*/
static hd_adec_config_t last_audio_cfg = { 0, };
int adec_write_cfg ( vdec_t *vdec, hd_adec_config_t *cfg ) {
	vdec_printf("adec_write_cfg %p %p\n", vdec->adec, vdec->adec?vdec->adec->af.config:0);
	if ( !vdec->adec || !vdec->adec->af.config )
		return -1;
	int format = NONE;
	switch ( cfg->format ) {
		case ADEC_FMT_MPEG      : format = MPEG_AUDIO; break;
		case ADEC_FMT_AC3       : format = AC3;  break;
		case ADEC_FMT_WMA       : format = NONE; break;
		case ADEC_FMT_AAC       : format = AAC;  break;
		case ADEC_FMT_DTS       : format = DTS;  break;
		case ADEC_FMT_PCM_S16LE : format = LPCM; break;
		case ADEC_FMT_PCM_S16BE : format = LPCM; break;
		case ADEC_FMT_PCM_U16LE : format = LPCM; break;
		case ADEC_FMT_PCM_U16BE : format = LPCM; break;
		case ADEC_FMT_PCM_S8    : format = LPCM; break;
		case ADEC_FMT_PCM_U8    : format = LPCM; break;
		case ADEC_FMT_MULAW     : format = NONE; break;
		case ADEC_FMT_ALAW      : format = NONE; break;
		default                 : format = NONE; break;
	} // switch
	printf ( "audiocfg %d %d %d\n", last_audio_cfg.generation, cfg->generation, cfg->format);
	if ( !memcmp ( &last_audio_cfg, cfg, sizeof ( last_audio_cfg ) ) ) {
		if ( ( vdec->adec->af.type        == format ) &&
		                ( vdec->adec->af.sub_type    == cfg->format ) &&
		                ( vdec->adec->af.bit_rate    == cfg->bitrate ) &&
		                ( vdec->adec->af.frequency   == cfg->samplerate ) &&
		                ( vdec->adec->af.nb_channels == cfg->channels ) &&
		                ( vdec->adec->af.bpf         == cfg->bpf ) &&
		                vdec->adec->af.set ) {
			printf("ignored\n");
			return 0;
		} // if
	} // if
	last_audio_cfg = *cfg;
	vdec->adec->af.type        = format;
	vdec->adec->af.sub_type    = cfg->format;
	vdec->adec->af.bit_rate    = cfg->bitrate;
	vdec->adec->af.frequency   = cfg->samplerate;
	vdec->adec->af.nb_channels = cfg->channels;
	vdec->adec->af.bpf         = cfg->bpf;
	vdec->adec->af.extra       = cfg->extra;
	vdec->adec->af.set         = 0;
	int ret = vdec->adec->af.config ( &vdec->adec->af, 1);
	if ( !ret ) {
		vdec->adec->af.set = 1;
		decoder_force_av ( vdec );
	} // if
	set_default_framerate(vdec);
	return ret;
} // adec_write_cfg
/*----------------------------------------------------------------------------*/
int adec_write_data ( vdec_t *vdec, uint64_t pts, u8 *data, int len ) {
//if(len>10)
//syslog(LOG_WARNING,"rpc %d %X %X %X %X %X %X %X %X %X %X \n",len,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
	if ( /*IS_ANY_TRICK(vdec) || */!vdec->adec )
		return 0;
	if ( vdec->adec->af.type == NONE || !vdec->adec->af.set ) { // Recover from trickmode change
		if ( !last_audio_cfg.format || adec_write_cfg ( vdec, &last_audio_cfg ) ) {
			vdec_printf("failed adec_write_cfg %d \n", last_audio_cfg.format);
			return -1;
		} // if
	} // if
//	printf("adec_write_data %lld %d(%d)\n", pts, len, vdec->adec->af.bpf);
	if ( audio_wait_queue ( vdec, 50 ) ) {
		vdec_printf ( "adec_write_data FAILED %lld %d(%d)\n", pts, len, vdec->adec->af.bpf );
		hd_vdec_config_t lCfg = last_video_cfg;
		last_video_cfg.pixelformat=-1;
		vdec_write_cfg ( vdec,&lCfg );
		return 0;
	} // if
	if ( pts == HDE_UNSPEC_PTS )
		pts = 0;
	static u64 my_last_apts = 0;
	if (pts) {
		if(pts < my_last_apts)
			printf("a-pts mismatch %llx -> %llx\n", my_last_apts, pts);
		my_last_apts = pts;
	} // if
	if ( adec_write_buff ( vdec, pts & HDE_VALID_PTS, data, len ) )
		vdec_printf ( "adec_write_data FAILED %lld %d\n", pts, len );
	if ( vdec->need_reset>=NEED_RESET_TRESHOLD ) {
		vdec_printf ( "Issue decoder reset (adec_write_data)\n" );
		hd_vdec_config_t lCfg = last_video_cfg;
		last_video_cfg.pixelformat=-1;
		vdec_write_cfg ( vdec,&lCfg );
	}
	return 0;
} // adec_write_data
