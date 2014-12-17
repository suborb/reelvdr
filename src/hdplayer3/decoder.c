/*****************************************************************************
*
* decoder.c - Handles lowlevel audio/video for DeCypher and trickmodes
*
* Copyright (C) 2008 Georg Acher (acher (at) baycom dot de)
*
* #include <gpl_v2.h>
*
******************************************************************************/

#include "osd.h"
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <syslog.h>
#include <linux/fb.h>
#include <linux/go8xxx-video.h>
#include "demux.h"
#include <hdchannel.h>
#include "hw_specials.h"

char *dec_device = "/dev/video0";
char *disp_device = "/dev/video10";
char *audio_device0 = "/dev/adec0";
char *audio_device1 = "/dev/adec1";

extern hd_data_t volatile *hda;

// Needed for A1-Workaround
extern int chiprev;
extern osd_t *osd;
extern int osd_enabled;
int decoder_first_queue=1;

#define GPIO_SCART_RGB  12      // 0:RGB

// Set to 1 to enable all fields for letterboxed 16:9 SDTV
#define SDTV_169_MATSCH 0
#define EMPTY_QUEUE_DELAY 5
#define EMPTY_QUEUE_STEP 250

#define FIELD_STR(x) \
	(x==V4L2_FIELD_ANY          )? "Any"         : \
	(x==V4L2_FIELD_NONE         )? "None"        : \
	(x==V4L2_FIELD_TOP          )? "Top"         : \
	(x==V4L2_FIELD_BOTTOM       )? "Bottom"      : \
	(x==V4L2_FIELD_INTERLACED   )? "Interlaced"  : \
	(x==V4L2_FIELD_SEQ_TB       )? "SeqTB"       : \
	(x==V4L2_FIELD_SEQ_BT       )? "SeqBT"       : \
	(x==V4L2_FIELD_ALTERNATE    )? "Alternate"   : \
	(x==V4L2_FIELD_INTERLACED_TB)? "InterlacedTB": \
	(x==V4L2_FIELD_INTERLACED_BT)? "InterlacedBT": \
	"Unknown"

#define vdec_printf(format, args...) printf("%i: " format, vdec->id, ##args)

float avg_fill=0;
/*----------------------------------------------------------------------------*/
u64 vdec_get_stc ( vdec_t *vdec ) {
	if (vdec)
		return hw_get_stc(0);
	return 0;
}
void vdec_set_stc( vdec_t *vdec, u64 pts ) {
	u64 stc = 0;
	if (!vdec->xine_mode || vdec->id) {// Don´t allow adjusting with xine or it will get confused
//		wis_dec_reset_stc(vdec->disp_fd, pts);
//		vdec->stc_diff=0;
		stc=hw_get_stc(0);
//		if(!wis_dec_get_stc ( vdec->disp_fd, &stc )) // Don't directly adjust pts or disp_fd will get confused
			vdec->stc_diff = stc-pts;
		vdec_printf("set_stc pts %llx stc %llx->%llx %lld\n", pts, stc, vdec_get_stc(vdec), vdec->stc_diff);
	} // if
//	wis_dec_reset_stc(vdec->disp_fd, pts);
//	vdec->stc_diff=0;
}

/*----------------------------------------------------------------------------*/
void decoder_stc_init ( vdec_t *vdec, u64 pts ) {
	vdec_printf("decoder_stc_init %lld\n", pts);
	vdec_set_stc( vdec, pts );	
	vdec->pts_delay=INIT_PTS_DELAY*90000;
	vdec->pts_delay_flag=0;
	vdec->stc_init=1;
}
/*----------------------------------------------------------------------------*/
static void *av_realloc ( void *ptr, unsigned int size ) {
	if ( size > ( INT_MAX-16 ) )
		return 0;
	return realloc ( ptr, size );
} // av_realloc
/*----------------------------------------------------------------------------*/
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
static void *av_fast_realloc ( void *ptr, unsigned int *size, unsigned int min_size ) {
	if ( min_size < *size )
		return ptr;
	*size = FFMAX ( 17*min_size/16+32, min_size );
	return av_realloc ( ptr, *size );
} // av_fast_realloc
/*----------------------------------------------------------------------------*/
int adec_write_buff ( vdec_t *vdec, uint64_t pts, u8 *data, int len ) {
//if(len>10)
//syslog(LOG_WARNING,"rpc %d %X %X %X %X %X %X %X %X %X %X \n",len,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
	adec_t *adec = vdec->adec;
	if ( !adec || !len || ( vdec->trickmode && vdec->trickspeed ) )
		return 0;
	int wp = adec->wp;
	adec->aqueue[wp].data = av_fast_realloc ( adec->aqueue[wp].data, &adec->aqueue[wp].size, len );
	if (adec->af.type==DTS) {
//syslog(LOG_WARNING,"buff  %d %d \n",pts&0xFFFFFFFF,(pts>>32)&0xFFFFFFFF);
//syslog(LOG_WARNING,"buff len %d fr %d \n",len,adec->af.framesize);
		if (1) {
			int n;
			// Someone at Micronas got this really, really wrong...
			// Convert BE->LE
			for(n=0;n<len;n+=4){
				adec->aqueue[wp].data[n]=data[n+3];
				adec->aqueue[wp].data[n+1]=data[n+2];
				adec->aqueue[wp].data[n+2]=data[n+1];
				adec->aqueue[wp].data[n+3]=data[n];
			}
		}
	}
	else
		memcpy ( adec->aqueue[wp].data, data, len );
	adec->aqueue[wp].len  = len;
	adec->aqueue[wp].pts  = pts;
	adec->wp = ( wp+1 ) %AUDIO_QUEUE_BUFS;
	return 0;
} // adec_write_buff
/*----------------------------------------------------------------------------*/
int adec_config ( struct audio_frame_s *af, int force ) {
	vdec_t *vdec= ( vdec_t* ) af->priv;
	adec_t *adec=vdec->adec;
	int samplerate=48000;
	int out_channels=1;
	int i;
	if (vdec->run_video_out==0 || !adec || !adec->af.frequency ||  !adec->af.nb_channels ||
			( adec->af.set && ( adec->af.type==adec->cur_audio_type ) &&
			( adec->af.sample_rate==adec->sample_rate ) &&
			( adec->af.frequency==adec->frequency ) /*&&
			( adec->af.nb_channels==adec->nb_channels )*/ ) )
		return 0;
	if(!adec->id && adec->af.set && !vdec->audio_chg_pts) {
		if(!adec->af.curpts || (adec->af.curpts&NOPTS))
			return 0; // Wait for next valid pts
		vdec_printf("audio change delayed to %lld type %d->%d freq %d->%d rate %d->%d chan %d->%d set %d next %lld diff last/next %lld\n",
		       adec->af.lastpts, adec->cur_audio_type, adec->af.type, adec->frequency, adec->af.frequency, adec->sample_rate,
		       adec->af.sample_rate, adec->nb_channels, adec->af.nb_channels, adec->af.set, adec->af.nextpts, adec->af.curpts);
		vdec->audio_chg_pts = adec->af.curpts;
	} // if
	if(adec->af.set && vdec->audio_chg_pts && !force)
		return 0;
	
	if ( adec->audio_fd>=0 )
		close ( adec->audio_fd );
	if(!adec->id && !vdec->audio_chg_pts) {
		vdec_printf("adec_config reset buffers\n");
		adec->wp = adec->rp; // Stop audio processing
			for ( i=0;i<AUDIO_QUEUE_BUFS;i++ ) { // Reset buffers
			if ( adec->aqueue[i].data )
				free ( adec->aqueue[i].data );
			memset ( &adec->aqueue[i], 0, sizeof ( adec->aqueue[i] ) );
		} // for
	} // if
	if (adec->id==1)
		adec->audio_fd=open ( audio_device1, O_RDWR );
	else
		adec->audio_fd=open ( audio_device0, O_RDWR );

	if ( adec->audio_fd<0 ) {
		fprintf ( stderr, "Unable to open %s: %s\n", (adec->id?audio_device1:audio_device0), strerror ( errno ) );
		return -1;
	} // if

	vdec_printf ( "####################### AUDIO FORMAT %i(%x), FR %i, SR %i, CH %i, BR %d, FS %d (%d %d %d) [%d]\n",
//	syslog(LOG_WARNING, "####################### AUDIO FORMAT %i(%x), FR %i, SR %i, CH %i, BR %d, FS %d (%d %d %d) [%d]\n",
		 adec->af.type, adec->af.sub_type, adec->af.frequency, adec->af.sample_rate, adec->af.nb_channels, af->bit_rate,
		 af->framesize, af->lsf, af->mpg25, af->padding, adec->af.extra);

	if ( wis_adec_output_config ( adec->audio_fd, 1, 0 ) < 0 )
			return -1;

	if ( adec->af.nb_channels >= 2 )
		out_channels = 2; /* Dirk: immer max 2 Channels für analog-out (oder hab ich das falsch verstanden?) */
	if ( adec->af.type==MPEG_AUDIO ) {
		if ( wis_adec_config_mpeg ( adec->audio_fd,
		                            adec->af.frequency, samplerate,
		                            adec->af.nb_channels, out_channels ) < 0 )
			return -1;
	} else if ( adec->af.type==AAC ) {
		unsigned char int_config[2]={0,0};
		int_config[0] = (adec->af.extra)>>8;
		int_config[1] = (adec->af.extra)&0xff;
//syslog(LOG_WARNING, "####################### AAC FREQ %d (%d) CH %d (%d) extra %d %d\n",
//adec->af.frequency, samplerate, adec->af.nb_channels, out_channels, int_config[0],int_config[1]);
		if ( adec->af.frequency<=24000) adec->af.frequency *=2;
		if ( wis_adec_config_aac ( adec->audio_fd,
		                            adec->af.frequency, samplerate,
		                            adec->af.nb_channels, out_channels,
		                            int_config, sizeof(int_config)) < 0 )
			return -1;
	} else if ( adec->af.type==AC3 ) {
		int int_config[15] = { 3, 0, 0, 3, 1, 7,
		                       0x7fffffff, 0x7fffffff, 6,
		                       0, 1, 2, 3, 4, 5
		                     };
		int_config[8] = out_channels;
		if ( out_channels == 2 ) {
			int_config[1] = 2;
			int_config[5] = 2;
			int_config[9] = 0;
			int_config[10] = 2;
		} else if ( out_channels == 1 ) {
			int_config[1] = 0;
			int_config[5] = 1;
			int_config[9] = 1;
		} // if
		if ( wis_adec_config_ac3 ( adec->audio_fd,
		                           adec->af.frequency, samplerate,
		                           adec->af.nb_channels, out_channels,
		                           int_config, sizeof(int_config)) < 0 )
			return -1;
	} else if ( adec->af.type==DTS ) {
		int int_config[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
		int_config[0] = adec->af.extra; // NBLKS
		vdec_printf("wis_adec_config_dts %d %d %d %d [0]%d\n", 
		       adec->af.frequency, samplerate, adec->af.nb_channels, out_channels, int_config[0]);

		if(wis_adec_config_dts(adec->audio_fd, 
		                       adec->af.frequency, samplerate, 
		                       adec->af.nb_channels, out_channels,
		                       int_config, sizeof(int_config)) < 0)
			return -1;
	} else if ( adec->af.type==LPCM ) {
		if ( !adec->af.sub_type )
			adec->af.sub_type = ADEC_FMT_PCM_S16BE;
		if ( wis_adec_config_pcm ( adec->audio_fd,
		                           adec->af.frequency, samplerate,
		                           adec->af.nb_channels, out_channels,
		                           adec->af.sub_type ) < 0 )
			return -1;
	} // if
	adec->cur_audio_type=adec->af.type;
	adec->sample_rate=adec->af.sample_rate;
	adec->nb_channels=adec->af.nb_channels;
	adec->frequency=adec->af.frequency;
	return 0;
}
/*----------------------------------------------------------------------------*/
int audio_fillstate ( vdec_t *vdec ) {
	adec_t *adec=vdec->adec;
	int wp, rp;
	int fill;
	wp=adec->wp;
	rp=adec->rp;
	if ( wp>=rp )
		fill=wp-rp;
	else
		fill=wp+AUDIO_QUEUE_BUFS-rp;
	return fill;
}
/*----------------------------------------------------------------------------*/
int audio_wait_queue ( vdec_t *vdec, int timeout ) {
//	adec_t *adec=vdec->adec;
	int fill;
	int retry=0;

	while ( 1 ) {
		fill=audio_fillstate ( vdec );
		if ( fill<AUDIO_QUEUE_BUFS-50 )
			break;
		retry++;
		usleep ( 10*1000 );
		if ( retry>timeout || !vdec->run_video_out ) {
			vdec_printf ( "FORGET AUDIO ES %d/%d\n", vdec->need_reset, NEED_RESET_TRESHOLD );
//syslog(LOG_WARNING,"audio wait queue reset\n");
			vdec->need_reset++;
			return 1;
		}
	}
	avg_fill= ( avg_fill*999.0+fill ) /1000.0;
//	printf("AFILL %.02f\n",avg_fill);
	return 0;
}
/*----------------------------------------------------------------------------*/
// Store audio frames in history buffer
int adec_callback ( struct audio_frame_s *af, int start, int len, uint64_t pts ) {
	vdec_t *vdec= ( vdec_t* ) af->priv;
	adec_t *adec=vdec->adec;
	//int wp;
	if (vdec->id)
		printf("ADCB\n");

	if ( audio_wait_queue ( vdec, 50 ) )
		return 0;
	if ( vdec->trickmode && vdec->trickspeed )
		return 0;
	adec_write_buff ( vdec, pts, adec->abuf+start, len );
	/*
	        wp=adec->wp;
		memcpy(adec->aqueue[wp].data, adec->abuf+start, len);
		adec->aqueue[wp].len=len;
		adec->aqueue[wp].pts=pts;
		adec->wp=(wp+1)%AUDIO_QUEUE_BUFS;
	*/
	return 0;
}
/*----------------------------------------------------------------------------*/
// Get data from audio buffer
int audio_dequeue ( vdec_t *vdec, u8 **data, u64 *pts, int peek ) {
	adec_t *adec=vdec->adec;
	int wp,rp;
	u64 ptsi;

	wp=adec->wp;
	rp=adec->rp;

	if ( wp!=rp ) {
		int len = adec->aqueue[rp].len;
		if ( len ) {
			if ( data )
				*data=adec->aqueue[rp].data;
			ptsi=adec->aqueue[rp].pts;
			if ( pts )
				*pts=ptsi;
		} else {
			vdec_printf("Audio data without len!\n");
		} // if
		if ( !peek )
			adec->rp= ( rp+1 ) %AUDIO_QUEUE_BUFS;
		return len;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
// Force AV-Sync
void decoder_force_av ( vdec_t *vdec ) {
	vdec->last_pts=NOPTS_VALUE;
	vdec->first_pts=NOPTS_VALUE;
	vdec->prev_pts=NOPTS_VALUE;
	vdec->pts_off=0;
	vdec->old_pts_off=0;
	vdec->old_pts_count=0;
	vdec->stc_init=0;
	vdec->frameno=0;
	vdec->pts_delay=INIT_PTS_DELAY*90000;
}
/*----------------------------------------------------------------------------*/
// rewind audio audio history
void audio_windback ( vdec_t *vdec, u64 skip ) {
	adec_t *adec=vdec->adec;
	int fill;

	fill=audio_fillstate ( vdec );
	if ( fill<skip )
		skip=fill;

	adec->rp-=skip;
	if ( adec->rp<0 )
		adec->rp+=AUDIO_QUEUE_BUFS;

	decoder_force_av ( vdec );
	vdec_printf ( "WINDBACK Audio rp %i wp %i\n", adec->rp, adec->wp );
	if ( vdec->trickspeed )
		vdec_adjust_pts ( vdec, &adec->aqueue[adec->rp].pts, 20000ULL, 1 );
}

/*----------------------------------------------------------------------------*/
// configure video decoder
void video_decoder_configure ( vdec_t *vdec ) {
	if ( !vdec->id && vdec->video_mode!=-1 ) {
		int delay = (vdec->trickmode && vdec->trickspeed); // All but play & slow forward
		vdec_printf ( "SET VFORMAT %x (%c%c%c%c) delay %d mode %d speed %d\n", vdec->video_mode,
		         vdec->video_mode,vdec->video_mode>>8,
		         vdec->video_mode>>16,vdec->video_mode>>24,
		         delay, vdec->trickmode, vdec->trickspeed);

		wis_dec_set_format ( vdec->decoder_fd, vdec->video_mode );
		set_default_framerate(vdec);
		wis_dec_set_nodelay (vdec->decoder_fd, delay);

		wis_dec_start ( vdec->decoder_fd );
	}
}
/*----------------------------------------------------------------------------*/
// configure display output
// aspect ratio/scaling, deinterlacer, default framerate
void video_display_configure ( vdec_t *vdec ) {
	vdec_printf ( "video_display_configure\n");
	struct v4l2_format fmt;
	struct v4l2_crop crop;
	struct go8xxx_pic_params pic;

	memset ( &fmt, 0, sizeof ( fmt ) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if ( ioctl ( vdec->yuv_fd, VIDIOC_G_FMT, &fmt ) < 0 ) {
		perror ( "VIDIOC_G_FMT" );
		return;
	}
	memset ( &crop, 0, sizeof ( crop ) );
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if ( ioctl ( vdec->yuv_fd, VIDIOC_G_CROP, &crop ) < 0 ) {
		perror ( "VIDIOC_G_CROP" );
		return;
	}
	memset ( &pic, 0, sizeof ( pic ) );

vdec_printf("vdec aspect %d:%d fmt %dx%d crop %d %d %d %d", vdec->vattr.aspect_w, vdec->vattr.aspect_h, fmt.fmt.pix.width, fmt.fmt.pix.height, crop.c.left, crop.c.top, crop.c.width, crop.c.height);

	pic.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/*
		if (ioctl(vdec->yuv_fd, GO8XXXIOC_G_PIC_PARAMS, &pic) < 0) {
			perror("GO8XXXIOC_G_PIC_PARAMS");
			return;
		}
	*/
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	vdec->is_sdtv_out=0;
	vdec->needs_sdtv_hack=0;

	if ( ( hda->video_mode.height<=576 && hda->video_mode.interlace && ( hda->video_mode.outputa&255 ) !=HD_VM_OUTA_OFF ) )
		vdec->is_sdtv_out=1;

	// WSS and scart voltage
	if (vdec->is_sdtv_out) {
		int asw=vdec->vattr.aspect_w==16?1:0;

		if (vdec->video_mode!=WIS_FMT_MPEG2 && !vdec->vattr.aspect_w)
			asw=1;

		set_sdtv_aspect(hda->video_mode.height==576?0:1, asw);
	}
	else 
		set_gpio(GPIO_SCART_RGB,0);  // Signal "Video on" 
	
	// scaled image needd special handling
	if (/*fmt.fmt.pix.height<=576 &&*/ vdec->is_sdtv_out && 
	    ((hda->aspect.scale&HD_VM_SCALE_VID)!=HD_VM_SCALE_F2S) && 
		hda->aspect.w==4 && (vdec->vattr.aspect_w==16 ) ) {
#if SDTV_169_MATSCH==1
		vdec_printf ( "ENABLE STDV 16:9 MATSCH HACK\n" );
		vdec->needs_sdtv_hack=2;
#else
		vdec_printf ( "ENABLE STDV 16:9 HACK\n" );
		vdec->needs_sdtv_hack=1;
#endif
	}

	// Disable deinterlacer
	// Fixme: Detect progressive MPEG2
	int field_1 = fmt.fmt.pix.field;
	fmt.fmt.pix.field = vdec->field;
	int field_2 = fmt.fmt.pix.field;
	if ( ! ( vdec->trickmode && vdec->trickspeed ) && ( fmt.fmt.pix.width<=720 ) &&
	     ( ( (hda->video_mode.deinterlacer&1)==HD_VM_DEINT_OFF && !vdec->needs_sdtv_hack ) ||
	                  ( vdec->is_sdtv_out && !vdec->needs_sdtv_hack ) ||
	                  ( fmt.fmt.pix.height<480 ) ) ) {

		// Don't use for 480i/576i when "HDMI only"
		if (fmt.fmt.pix.height<=576 &&
		    (hda->video_mode.auto_format || hda->video_mode.width<=576) &&
		    !vdec->is_sdtv_out &&
		    hda->video_mode.interlace) {
			vdec_printf ( "Ignored  deinterlacer setting\n");
		}
		else {
			fmt.fmt.pix.field = 1;
			vdec_printf ( "Disabled deinterlacer %x\n",hda->video_mode.deinterlacer );
		}
	}
	vdec_printf("Field %s -> %s -> %s\n", FIELD_STR(field_1), FIELD_STR(field_2), FIELD_STR(fmt.fmt.pix.field));

	if ( ioctl ( vdec->disp_fd, VIDIOC_S_FMT, &fmt ) < 0 ) {
		perror ( "VIDIOC_S_FMT" );
		return;
	}
	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	// Crop top, left and right border for SDTV
	if (fmt.fmt.pix.width<=720) {
//              Removed: vertical cropping is a bad idea for 576i material - do we need it for 720?
//		if (!vdec->is_sdtv_out || vdec->needs_sdtv_hack ) 
//		{ // Sorry, can't do this with 4:3-SCART due to weird scaling issues
//			crop.c.top=1*8;
//			crop.c.height=fmt.fmt.pix.height-2*crop.c.top;			
//		}

		if (fmt.fmt.pix.width>480) {
			crop.c.left=16;
			crop.c.width=fmt.fmt.pix.width-2*crop.c.left;
		} else {
			crop.c.left=8;
			crop.c.width=fmt.fmt.pix.width-2*crop.c.left;
		}
	}
	vdec_printf ( "####################### CROP L %i T %i W %i H %i\n",crop.c.left,crop.c.top,crop.c.width,crop.c.height );
	if ( ioctl ( vdec->disp_fd, VIDIOC_S_CROP, &crop ) < 0 ) {
		perror ( "VIDIOC_S_CROP" );
		return;
	}
	pic.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
//if(vdec->xine_mode) syslog(LOG_WARNING,"xine w %d h %d aw %d ah %d",fmt.fmt.pix.width,fmt.fmt.pix.height,vdec->vattr.aspect_w,vdec->vattr.aspect_h);
//	if (!vdec->xine_mode) {
		if ( vdec->vattr.aspect_w==0 ) {  // Default ratio
			if(vdec->video_mode==WIS_FMT_H264) {
				// Fix for Dr.Dish TV h.264 SD format
				vdec->vattr.aspect_w=16;
				vdec->vattr.aspect_h=9;
			} else {
				vdec->vattr.aspect_w=4;
				vdec->vattr.aspect_h=3;
			}
		}
		    int frate = 25;//default framerate
		    if (vdec->vattr.framerate_div && vdec->vattr.framerate_nom)
		    frate = (vdec->vattr.framerate_div / vdec->vattr.framerate_nom == 25 || vdec->vattr.framerate_div / vdec->vattr.framerate_nom == 50) ? 50 : 60;
		    vdec->frame_rate = frate;
//syslog(LOG_WARNING,"frate %d hdafr %d \n",frate,hda->video_mode.framerate);
		if(vdec->video_mode==WIS_FMT_MPEG2){//aspect ratio from mpeg2
		    if ( vdec->vattr.aspect_w==16 )
			pic.par.width=(fmt.fmt.pix.height)*16.00;
		    else
			pic.par.width=(fmt.fmt.pix.height)*vdec->vattr.aspect_w;
		    pic.par.height=(fmt.fmt.pix.width)*vdec->vattr.aspect_h;
		} else if (vdec->video_mode==WIS_FMT_H264){//aspect ratio from h264
		    float asp = (float)vdec->vattr.aspect_w / (float)vdec->vattr.aspect_h * (float)fmt.fmt.pix.width / (float)fmt.fmt.pix.height;
		    if ( (frate == 50 ? 1.67 : 2.0) < asp ) {
			pic.par.width=(fmt.fmt.pix.height)*16.00;
                    pic.par.height=(fmt.fmt.pix.width)*9.00;
		    } else {                                //aspect ratio from mp4/divx
			pic.par.width=(fmt.fmt.pix.height)*4;
                        pic.par.height=(fmt.fmt.pix.width)*3;
            	    }
		}
//	} else {
//		pic.par.width=1;
//		pic.par.height=1;
//	}
// Misnomer: FLAG_OVERSCAN is essentially the opposite, it actually decreases the width
/*	if (hda->aspect.overscan==1)
		pic.flags = GO8XXX_PIC_FLAG_OVERSCAN;
        else */
                pic.flags = 0;
                
		vdec_printf("####################### overscan=%s\n",(hda->aspect.overscan==1)?"yes":"no");
        /* TB: workaround for BBC HD */
        if (fmt.fmt.pix.height == 1080 && fmt.fmt.pix.width == 1440 && vdec->vattr.aspect_w == 0 && vdec->vattr.aspect_h == 0) {
		vdec_printf("Switching to BBC HD workaround\n");
                vdec->vattr.aspect_w=16;
                vdec->vattr.aspect_h=9;
                pic.par.width=(fmt.fmt.pix.height)*16.00;
                pic.par.height=(fmt.fmt.pix.width)*vdec->vattr.aspect_h;
	/* TB: workaround for Luxe HD */
        } else if (fmt.fmt.pix.height == 1080 && fmt.fmt.pix.width == 1280 && vdec->vattr.aspect_w == 0 && vdec->vattr.aspect_h == 0) {
                vdec_printf("Switching to Luxe HD workaround\n");
                vdec->vattr.aspect_w=16;
                vdec->vattr.aspect_h=9;
                pic.par.width=(fmt.fmt.pix.height)*16.00;
                pic.par.height=(fmt.fmt.pix.width)*vdec->vattr.aspect_h;
        }

	vdec_printf("####################### SIZE %ix%i, %i:%i (%d) -> (%ix%i)\n",
		fmt.fmt.pix.width,fmt.fmt.pix.height,vdec->vattr.aspect_w,vdec->vattr.aspect_h, vdec->video_mode==WIS_FMT_MPEG2,
		pic.par.width,pic.par.height);
	vdec->vattr.width=fmt.fmt.pix.width;
	vdec->vattr.height=fmt.fmt.pix.height;
	hd_player_status_t lStatus = {fmt.fmt.pix.width,fmt.fmt.pix.height,vdec->vattr.aspect_w,vdec->vattr.aspect_h};
	hda->player_status[0] = lStatus; // Dirty hack - we don't know that we are player[0] ...

	if ( ioctl ( vdec->disp_fd, GO8XXXIOC_S_PIC_PARAMS, &pic ) < 0 ) {
		perror ( "GO8XXXIOC_S_PIC_PARAMS" );
		return ;
	}
	set_default_framerate ( vdec );

	if(frate != hda->video_mode.framerate && hda->video_mode.norm){
//syslog(LOG_WARNING,"==================vdec %d fmt %d pic %d hda %d \n",vdec->vattr.height,fmt.fmt.pix.height,pic.par.height,hda->video_mode.height);
		if(hda->video_mode.auto_format){
			hda->video_mode.framerate=frate;

			hda->video_mode.changed++;
		}else{
//syslog(LOG_WARNING,"==================vopmode %d hdarf %d frate %d \n",vmode.vop_mode,hda->video_mode.framerate,frate);
//syslog(LOG_WARNING,"================== %d \n",vmode.vop_mode);
			int fd=open("/dev/fb0",O_RDONLY);
			if(fd<0)return;

			dspc_vop_t vmode;
			ioctl ( fd,DSPC_GET_VOP, &vmode );

			if(vmode.vop_mode == VOP_MODE_1080i50 || vmode.vop_mode == VOP_MODE_1080i)
				vmode.vop_mode = frate == 50 ? VOP_MODE_1080i50 : VOP_MODE_1080i;

			if(vmode.vop_mode == VOP_MODE_720p50 || vmode.vop_mode == VOP_MODE_720p)
				vmode.vop_mode = frate == 50 ? VOP_MODE_720p50 : VOP_MODE_720p;
/*
if(vmode.vop_mode == VOP_MODE_576p || vmode.vop_mode == VOP_MODE_480p)
vmode.vop_mode = frate == 50 ? VOP_MODE_576p : VOP_MODE_480p;
if(vmode.vop_mode == VOP_MODE_576i || vmode.vop_mode == VOP_MODE_480i)
vmode.vop_mode = frate == 50 ? VOP_MODE_576i : VOP_MODE_480i;
*/
			hda->video_mode.framerate=frate;

			if(vmode.vop_mode == VOP_MODE_576i || vmode.vop_mode == VOP_MODE_576p || vmode.vop_mode == VOP_MODE_480i || vmode.vop_mode == VOP_MODE_480p){
				hda->video_mode.height=frate==50 ? 576:480;
				hda->video_mode.changed++;
			}else
				ioctl ( fd,DSPC_SET_VOP, &vmode );
			close(fd);
		}

	}
	return;
}
void set_default_framerate ( vdec_t *vdec ) {
	if (vdec->id)
		return;
	if(!vdec->vattr.framerate_div || !vdec->vattr.framerate_nom) {
		vdec->vattr.framerate_div = DEFAULT_FRAMERATE_DIV;
		vdec->vattr.framerate_nom = DEFAULT_FRAMERATE_NOM;
		vdec_printf ( "SET Default Framerate %.02f\n", ( float ) ( vdec->vattr.framerate_div ) /vdec->vattr.framerate_nom );
		wis_dec_set_default_fps ( vdec->decoder_fd, vdec->vattr.framerate_nom, vdec->vattr.framerate_div );
	} else {
		vdec_printf ( "SET Framerate %.02f\n", ( float ) ( vdec->vattr.framerate_div ) /vdec->vattr.framerate_nom );
		wis_dec_set_default_fps ( vdec->decoder_fd, vdec->vattr.framerate_nom, vdec->vattr.framerate_div );
//syslog(LOG_WARNING,"set frate %.02f \n",( float ) ( vdec->vattr.framerate_div ) /vdec->vattr.framerate_nom );
	} // if
} //
/*----------------------------------------------------------------------------*/
// Get decoder fillstate
u32 decoder_vbuf_free ( vdec_t *vdec ) {
	struct v4l2_buffer buf;
	int read_off=-1, i,ret=0;

	// Avoid calling ioctl until a certain fillstate is exceeded
	if ( vdec->ves_free>1*1024*1024 || vdec->decoder_fd<0 || vdec->video_mode==-1 ) {
		return vdec->ves_free;
	}

	memset ( &buf, 0, sizeof ( buf ) );
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_MMAP_RING;
	for ( i=0;i< (IS_PLAY(vdec)?350:10);i++ ) {
		ret=ioctl ( vdec->decoder_fd, VIDIOC_DQBUF, &buf );
		if ( !ret )
			break;
		usleep ( 10*1000 );
	}
	if ( ret ) {
		vdec->need_reset=NEED_RESET_TRESHOLD;
		vdec_printf ( "Hang, need decoder reset\n" );
	}

	read_off= buf.m.offset;

	if ( read_off!=1 )
		vdec->ves_rp=read_off;

	if ( vdec->ves_wp < read_off && read_off!=-1 )
		i = read_off - vdec->ves_wp - 1;
	else
		i = vdec->ves_size - vdec->ves_wp + read_off - 1;
	vdec->ves_free=i;
	return i;
}
/*----------------------------------------------------------------------------*/
// Send ES video packet to decoder

int decoder_video_qbuf ( vdec_t *vdec, u32 start, u32 len, u64 pts, u32 flags ) {
	int ret;
	int n;
#if 0
	u64 diff;
	printf ( "QUEUE %i, %09llx %d\n",len,pts,vdec->video_mode );
	diff=pts-vdec->last_disp_pts;
	if ( diff>3*90000 && vdec->last_disp_pts ) {
		printf ( "SLEEP " );
		usleep ( 40*1000 );
	}

	hexdump ( vdec->ves_buf+start,16 );
#endif

#if 1
	//------------------------------------
	// A1 crash Workaround, hide FB before displaying first frame
	if (!chiprev && decoder_first_queue) { 
		if (osd_enabled)
		{
			int pid=2;
			ioctl(osd->fd, DSPC_HIDE, &pid);
		}			
		decoder_first_queue=0;
		usleep(100*1000);
	}
	//------------------------------------
#endif
	if(pts && (pts != NOPTS_VALUE)) {
		vdec->last_decoder_pts = pts;
//		syslog(LOG_INFO, "%d >>>> %lld = %lld - %lld (%d %d)", vdec->decoder_frames, pts - vdec->last_disp_pts, pts, vdec->last_disp_pts, vdec->trickmode, vdec->trickspeed);
	} // if
	if(vdec->trickspeed)
		vdec->decoder_frames = 0; // We are playing frames as fast as we can so assume there are no frames in queue

	if ( vdec->video_mode==-1 ) {
		vdec_printf ( "ERROR: video not initialized!!!!\n" );
		return len;
	}

	if ( vdec->trickmode && vdec->trickspeed )
		pts=NOPTS_VALUE;

//	printf("QBUF: %llx mode %d speed %d\n", pts, vdec->trickmode, vdec->trickspeed);
	for ( n=0;n<10;n++ ) {
		ret= wis_dec_ring_play_pts ( vdec->decoder_fd, start, len, pts );
		if ( !ret )
			break;
		usleep ( 100*1000 );
		if ( ret<0 )
			vdec->need_reset++;
	}
	if ( !ret )
		vdec->ves_free-=len;
	return ret;
}
/*----------------------------------------------------------------------------*/
// Send decoded frame to display
void show_frame(vdec_t *vdec, unsigned int index, wis_field_t field,
                unsigned int display_handle, u64 *pts)
{
	if (wis_dec_yuv_play_pts(vdec->disp_fd, display_handle, *pts, field, index))
		vdec->need_reset+=NEED_RESET_TRESHOLD/2;
}
/*----------------------------------------------------------------------------*/
// send audio frames to decoder, play at specified PTS
void queue_audio ( vdec_t *vdec, u64 vpts ) {
	adec_t *adec=vdec->adec;
	u8 *data;
	int len;
	int64_t pts,opts;
	u64 current_pts;
	int slomo=0;

	if ( vdec->pause )
		return;

	if ( IS_SLOMO ( vdec ) )
		slomo=1;

	while ( vdec->run_video_out ) {
		len=audio_dequeue ( vdec, &data, &pts, 1 );  // only peek
		if ( len ) {
			if(vdec->adec->af.set && vdec->audio_chg_pts && vdec->adec->af.config && (vdec->audio_chg_pts<= pts)) {
				vdec_printf("late audio chg: set %d at %lld (%lld)\n", vdec->adec->af.set, vdec->audio_chg_pts, pts);
				ioctl ( vdec->adec->audio_fd, ADEC_FLUSH );
				vdec->adec->af.config(&vdec->adec->af, 1);
				vdec->audio_chg_pts=0;
				return;
			} // if

			if((pts == NOPTS) || (pts==NOPTS_VALUE))
				pts = 0;

			if(pts)
				pts += vdec->stc_diff;

//			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AUDIO: %llx %llx %llx\n", pts, vpts, vdec_get_stc( vdec));
			if ( vdec->id || IS_PLAY ( vdec ) || slomo ) {
				s64 diff;
				if ( slomo )
					current_pts=vpts+0.2*90000; // sync (inaudible) audio to video
				else {
					current_pts = vdec_get_stc ( vdec);
//					vdec->last_stc=current_pts;
					vdec->last_stc=current_pts-vdec->stc_diff-vdec->pts_delay-VIDEO_AHEAD*90000;
				}
				opts=pts;

				if ( !vdec->xine_mode) {// adjusting while using xine results in stocking video
					if(pts) vdec_adjust_pts ( vdec, &pts, 0, 1 );
				} else if ( !vdec->stc_init ) {
					if ( pts ) {
						vdec_printf("queue_audio : ");
 						decoder_stc_init ( vdec, pts -45000 );
					} else {
						audio_dequeue ( vdec, &data, &pts, 0 );
						continue;
					} // if
				} // if

				if(pts) {
					pts += vdec->pts_delay;
					diff=pts-current_pts;
				} else
					diff=0;
				// Do not queue in the past

				if ( vdec->xine_mode && !slomo && diff<-0.6*90000 )
					syslog ( LOG_INFO,"AUDIO: Do not queue in the past IGNORED!\n" );
				if ( !vdec->xine_mode && !slomo && diff<-0.6*90000 ) {
					vdec_printf("AUDIO: Do not queue in the past %lld\n", diff);
					len=audio_dequeue ( vdec, &data, &pts, 0 );
					if ( diff<-1*90000 )
						vdec->need_reset++;
				}
				// Do not queue audio more than ... in the future
				else if ( slomo || diff<0.5*90000) {
					u64 dummy;
					if(/*vdec->xine_mode &&*/ pts && (pts < current_pts)) {
						if (vdec->id) { // Fast balance for audio-only
							if (pts<current_pts-200000)
								vdec->pts_delay+=7LL*(current_pts-pts)/8LL;
							else
								vdec->pts_delay += 18000;
						}
						else
							vdec->pts_delay += 18000;// 0.2*90000
						vdec_printf("Got late audio: 0x%llx 0x%llx %lld %lld\n", current_pts, pts, pts-current_pts, vdec->pts_delay);
						pts = current_pts;
//					} else {
//						printf("Got audio %llx %llx %lld %d\n", current_pts, pts, pts-current_pts, vdec->pts_delay);
					} // if
					if(pts) {
						if ( vdec->adec->af.type==AC3 )
							pts+=90000ULL*AUDIO_AC3_AHEAD+vdec->ac3_pts_shift;
						else
							pts+=90000ULL*AUDIO_AHEAD+vdec->pts_shift;
					} // if
					pts&=0x1ffffffffULL;

					if ( !slomo ) {
						printf("%i: STC %09llx APTS %09llx, OPTS %09llx, DIFF %09llx\n",vdec->id,current_pts,pts,opts,pts-current_pts);
/*if(vdec->adec->af.type==AAC)len=audio_dequeue ( vdec, &data, &dummy, 0 );
else*/						if(!wis_adec_play_frame ( adec->audio_fd, data, len, pts )) {
							len=audio_dequeue ( vdec, &data, &dummy, 0 );
							if(vdec->need_reset) vdec->need_reset--;
						} else {
							vdec->need_reset++;
						} // if
					} else {
						len=audio_dequeue ( vdec, &data, &dummy, 0 );
					}
					adec->last_played_pts=opts;
				} else {
					//printf("Do not queue audio more than ... in the future %lld\n", diff);
					break;
				}
			} else {
				//printf("Audio drop due to trickmode\n");
				len=audio_dequeue ( vdec, &data, &pts, 0 );
			}
		} else {
//			printf("Audio Underrun\n");
			break;
		}
	}
}
/*----------------------------------------------------------------------------*/
#define AUDIO_REV_SKIP_PAUSE 15
#define AUDIO_REV_SKIP_TRICK 15

void handle_tricks ( vdec_t *vdec, int pause ) {
	if ( vdec->pause!=vdec->old_pause ) {
		vdec_printf ( "PAUSE CHANGE\n" );
		if ( vdec->pause ) {
			ioctl ( vdec->adec->audio_fd, ADEC_FLUSH );
		} else {
			//audio_windback(vdec,AUDIO_REV_SKIP_PAUSE);
			decoder_force_av ( vdec );
		}
		vdec->old_pause=vdec->pause;
	}
	
	if(vdec->xine_mode) {
		// Delay of frames moved to vdr...
		if ( IS_SLOMO ( vdec ) ) {
			if ( vdec->trickspeed<24 )
				usleep ( 40*1000*vdec->trickspeed );
			else
				usleep ( 10*1000*vdec->trickspeed );
		}
	
		if ( IS_FAST ( vdec ) )
			usleep ( 20*1000*vdec->trickspeed );
	}
	
	if(vdec->trickspeed)
		vdec->decoder_frames = 0; // We are playing frames as fast as we can so assume there are no frames in queue
	
	if ( vdec->trickspeed!=vdec->old_trickspeed ||
	                vdec->trickmode!=vdec->old_trickmode ) {
		vdec_printf ( "TRICK CHANGE mode %d->%d speed %d->%d\n", vdec->old_trickmode, vdec->trickmode, vdec->old_trickspeed, vdec->trickspeed);
		vdec->wakeup_throttle=1;
		if ( !vdec->trickspeed || vdec->trickmode ) {
			vdec->force_field=WIS_FIELD_TOP_ONLY;
		} 
		// Recover from slow motion
		if ( vdec->trickmode==1 && !vdec->trickspeed && !vdec->old_trickmode ) {
			decoder_force_av ( vdec );
			if ( !vdec->xine_mode ) {
				vdec_printf("audio_windback 2 %d %d\n", vdec->trickmode, vdec->old_trickspeed);
				audio_windback ( vdec, AUDIO_REV_SKIP_TRICK );
			} // if
			vdec->force_field=0;
//			vdec->need_reset=-200; // avoid decoder reset due to skipped images
			printf ( "RECOVER\n" );

		}

		// Recover from fast fwd, already done in decoder_trick
		if ( vdec->trickmode==1 && !vdec->trickspeed && vdec->old_trickmode ) {
			decoder_force_av ( vdec );
			vdec->force_field=0;
		}
		vdec->old_trickspeed=vdec->trickspeed;
		vdec->old_trickmode=vdec->trickmode;
	}

	if ( pause ) {

		while ( vdec->pause &&vdec->run_video_out )
			usleep ( 10*1000 );
		// Reover from pause
		decoder_force_av ( vdec );
//		if ( vdec->trickmode && !vdec->old_trickspeed ) {
//			printf("audio_windback 1 %d %d\n", vdec->trickmode && !vdec->old_trickspeed);
//			if ( !vdec->xine_mode ) audio_windback ( vdec,AUDIO_REV_SKIP_PAUSE );
//		}
	}

	return;
}
/*----------------------------------------------------------------------------*/
#define SDTV_SYNC_MAX_RETRIES 10
#define IGNORE_PTS_COUNT      3
void *video_out_thread(void *p)
{
	vdec_t *vdec=(vdec_t*)p;
        fd_set rfds;
        unsigned int index, display_handle;
	int64_t pts=0,last_pts=0;
	struct timeval timeout;
	int queued=0;
	int ticks=0;
	int pause=0;
	avg_fill=0;
	int first=IGNORE_PTS_COUNT;  // ignore PTS for the first n frames (fast sync)
	int skip_next=first-1; // Skip n-1 frames at the beginning (avoid scrambled images)
	int sdtv_sync_retries=0;
	int pts_mismatch=0;
	double fade_val=256;//0x1;

	if(IS_ANY_TRICK(vdec) || vdec->xine_mode)
		fade_val=256;
	fade(fade_val);

	usleep ( 100*1000 ); // Fillup audio
	vdec_printf ( "video_out_thread started\n" );
	if(vdec->xine_mode) {
		first=0; skip_next=0; // Just to be sure pts is correct and stillframes are shown
	}

	// Audio-only
	if (vdec->id) {
		while ( vdec->run_video_out ) {
			queue_audio ( vdec,pts );
			usleep(10*1000);
		}
		return NULL;
	}

	while ( vdec->run_video_out ) {
		u64 now;
		queue_audio ( vdec,pts );

		FD_ZERO ( &rfds );
		FD_SET ( vdec->yuv_fd, &rfds );
		FD_SET ( vdec->disp_fd, &rfds );
		timeout.tv_sec=0;
		timeout.tv_usec=10*1000;
		if ( select ( ( vdec->yuv_fd > vdec->disp_fd ? vdec->yuv_fd : vdec->disp_fd ) + 1,
		                &rfds, NULL, NULL, &timeout ) < 0 ) {
			perror ( "video_out_thread select" );
			vdec->need_reset=NEED_RESET_TRESHOLD;
			break;
		}
		bool processed=false;
		if ( queued<6 && FD_ISSET ( vdec->yuv_fd, &rfds ) ) {
			s64 diff,diff1;
			processed=true;
			pause=vdec->pause;
			if(pause) first=IGNORE_PTS_COUNT;
			if ( wis_dec_dequeue_yuv_buffer_pts ( vdec->yuv_fd, &index,
			                                      &pts, &vdec->field, &display_handle ) ) {
				perror ( "video_out_thread wis_dec_dequeue_yuv_buffer_pts" );
				vdec->need_reset=NEED_RESET_TRESHOLD;
				break;
			} // if
			if(!pts ) first=IGNORE_PTS_COUNT;
			if (pts && (pts < 0x1ffffffffULL)) {
				if(pts < vdec->last_orig_pts) {
					pts_mismatch=1;
					vdec_printf("pts mismatch %llx -> %llx\n", vdec->last_orig_pts, pts);
				} // if
				if (vdec->last_orig_pts && (vdec->video_mode==WIS_FMT_H264)) {
					int64_t diff = (vdec->last_orig_pts > pts) ? (vdec->last_orig_pts - pts) : (pts - vdec->last_orig_pts);
					if(diff > 90000) {
						vdec_printf("h.264 pts mismatch %llx -> %llx (%lld)\n", vdec->last_orig_pts, pts, diff);
						vdec->need_reset=NEED_RESET_TRESHOLD;
						break;
					} // if
				} // if
				vdec->last_orig_pts = pts;
			} // if
			if(vdec->stc_init && vdec->last_decoder_pts && 
					pts && (pts < 0x1ffffffffULL) && 
					(vdec->last_decoder_pts > pts) && !vdec->trickspeed)
				vdec->decoder_frames = (vdec->last_decoder_pts - pts)* (vdec->frame_rate ? vdec->frame_rate/2 : 25 )/90000;
			else
				vdec->decoder_frames = 0;
//printf("decoder_frames %d        \r", vdec->decoder_frames);
//			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>VIDEO: %llx %llx %llx %d %d [%d]\n", pts, vdec_get_stc( vdec), vdec->last_disp_pts, vdec->pts_delay, first, vdec->decoder_frames);
			if ( !vdec->display_configured ||
					( vdec->video_mode==WIS_FMT_MPEG2 &&
					( vdec->vattr.aspect_w!=vdec->vattr_old.aspect_w ||
					vdec->vattr.aspect_h!=vdec->vattr_old.aspect_h ) &&
					vdec->aspect_chg_pts <= pts ) ) {
				video_display_configure ( vdec );
				vdec->display_configured = 1;
				vdec->vattr_old=vdec->vattr;
				vdec->aspect_chg_pts=0;
				vdec->old_pts_off=0;
				vdec->old_pts_count=0;
				vdec->frameno=0; // Force Focus sync
			}
			if(pts && (pts < 0x1ffffffffULL))
				pts += vdec->stc_diff;

			now = vdec_get_stc( vdec);
//			vdec->last_stc=now;
			// immediate queuing for first pictures (faster zapping)
			if ( !pts ||  !IS_PLAY ( vdec ) || first ) {
				pts=0;

				if (first==IGNORE_PTS_COUNT && vdec->video_mode==WIS_FMT_H264)
					skip_next=IGNORE_PTS_COUNT-1;
				if (!first || vdec->xine_mode || vdec->video_mode==WIS_FMT_MPEG2) 
					skip_next = 0; // Don't skip frames since we now get delayed frames
			} else {
				//u64 xpts=pts;
				if ( !vdec->stc_init ) {
					vdec_printf("vqueue : ");
					decoder_stc_init ( vdec, pts-vdec->stc_diff);
					pts += vdec->stc_diff;
					now = vdec_get_stc( vdec);
					vdec->frameno=0; // Force Focus sync
//					vdec->last_stc=now;
					first=IGNORE_PTS_COUNT;
				} // if
//				vdec_adjust_pts(vdec, &pts, VIDEO_AHEAD*90000, (vdec->adec->audio_id?0:1));
#ifdef EMPTY_QUEUE_DELAY
				if(!vdec->xine_mode && queued<3 && queued >= 0 && vdec->stc_init && !IS_ANY_TRICK(vdec) && !first) {
					if(vdec->pts_delay_flag < EMPTY_QUEUE_DELAY*EMPTY_QUEUE_STEP) {
						vdec->pts_delay_flag+=EMPTY_QUEUE_STEP;
					} else {
						vdec->pts_delay += 18000;// 0.2*90000
						vdec->pts_delay_flag=0;
						vdec_printf("pts delayed due to empty queue (%d) to %lld at %d\n", queued, vdec->pts_delay, vdec->frameno);
						vdec->frameno=0; // Force Focus sync
						if (vdec->video_mode==WIS_FMT_H264) {
							// a reset prevents distorted screen
							vdec->need_reset=NEED_RESET_TRESHOLD;
							break;
					        }
					} // if
				} else if(vdec->pts_delay_flag)
					vdec->pts_delay_flag--;
#endif
					pts+=VIDEO_AHEAD*90000+vdec->pts_off+vdec->pts_delay;
			}
			vdec->last_stc=now-vdec->stc_diff-vdec->pts_delay-AUDIO_AHEAD*90000;
			pts&=0x1ffffffffULL;
			diff=pts-now;
#ifdef SMOOTH_AUDIO_CHG
			if(pts && diff<=-0.2*90000LL && !IS_SLOMO(vdec) && vdec->old_pts_off && vdec->old_pts_count) { // We are still showing old pts_off frames?
				int64_t diff_old = vdec->pts_off-vdec->old_pts_off;
				pts -= diff_old;
				diff=pts-now;
				vdec->old_pts_count--;
			} else if (vdec->old_pts_off) {
				vdec->frameno=0; // Force Focus sync
				vdec->old_pts_off=0;
				vdec->old_pts_count=0;
			}
#endif
			diff1=pts-last_pts;
//			if (diff1>3600 || diff1<0)
//				printf("VQUEUE %i.%i.%i.%i %09llx, diff %09llx, diff1 %i, delay %d(%d), frame %d offset_change %lld\n", queued, field, vdec->force_field, pause, pts,diff,(int)diff1, vdec->pts_delay, vdec->pts_delay_flag, vdec->frameno);
			last_pts=pts;
			if(vdec->xine_mode && pts && (diff<=-0.2*90000LL)) {
				pts -= vdec->pts_delay;
				if(!pts_mismatch) {
					vdec->pts_delay += 18000;// 0.2*90000
					pts += vdec->pts_delay;
				} else
					pts = 0;
				vdec_printf("Got late video 0x%llx 0x%llx->0x%llx %lld %lld\n", now, last_pts, pts, diff, vdec->pts_delay);
				diff = 0;
			} // if
			if (skip_next==0 && (!pts || (diff>-0.2*90000LL) || IS_SLOMO(vdec))) { // Skipping with xine may result in black screen
				static u64 lpts = 0;
				int frame_duration = vdec->frame_rate ? 180000/vdec->frame_rate : 3600;
//				if ( vdec->vattr.framerate_div && vdec->vattr.framerate_nom)
//					frame_duration = ( 90000LL*vdec->vattr.framerate_nom ) /vdec->vattr.framerate_div;
				if(!pts) {
					if ( vdec->vattr.framerate_div && vdec->vattr.framerate_nom)
						frame_duration = ( 90000LL*vdec->vattr.framerate_nom ) /vdec->vattr.framerate_div;
					if(!vdec->trickmode) // Smow forward
						frame_duration *= vdec->trickspeed;
					if(now-vdec->last_disp_pts < frame_duration)
						pts = vdec->last_disp_pts + frame_duration;
					else
						pts = now;
				} // if
				lpts = pts;
				if (vdec->needs_sdtv_hack==1) {
//					u64 x=pts+1800; //FIXME
					show_frame ( vdec, index, WIS_FIELD_TOP_ONLY, display_handle, &pts );
//					show_frame(vdec, -1, WIS_FIELD_TOP_ONLY, display_handle-1*(720+32)*2, &x);
				}
				else if ((vdec->field == WIS_FIELD_NONE) || pause ) {
					show_frame(vdec, index, WIS_FIELD_NONE, display_handle, &pts);
				} else if (vdec->force_field!=-1 ) {
					show_frame(vdec, index, vdec->force_field, display_handle, &pts);
//Not working with scart!					show_frame(vdec, index, vdec->force_field?vdec->force_field:field, display_handle, &pts);
				} else {
					vdec_printf ( "SKIP IMAGE due to field %x %d\n",vdec->field, vdec->force_field );
				}
				index=-1;
				queued++;
				ticks++;
				if ( vdec->need_reset )
					vdec->need_reset--;
			} else {
				vdec_printf ( "SKIP IMAGE diff %09llx (=%lli) %llx\n",diff, diff, vdec->stc_diff );
#ifdef EMPTY_QUEUE_DELAY_PREVENTION
				if(vdec->pts_delay_flag)
					vdec->pts_delay_flag--;
#endif
				//In Xine-Mode FF->Play causes skips and resetting at this point completely crashes the decoder.
				//TODO: Test if reset needs some adjustment
				if(!vdec->xine_mode)
					vdec->need_reset++;
				wis_dec_queue_yuv_buffer ( vdec->yuv_fd, index ); // Forget without displaying
			}

			vdec->last_disp_pts=pts;

			if ( first )
				first--;
			vdec->frameno++;

			//------------------------------------
			// A1 Workaround, show FB after a few frames startup
			if (!chiprev && (vdec->frameno==10) && osd_enabled) {
				int pid=2;
				ioctl(osd->fd, DSPC_SHOW, &pid);
			}
			//------------------------------------

			if (skip_next>0)
				skip_next--;

#if 1
			if (sdtv_sync_retries<SDTV_SYNC_MAX_RETRIES && 
			    ((vdec->frameno+8)&15)==0 && hda->video_mode.height<=576 && hda->video_mode.interlace && 
			    (hda->video_mode.outputa&255)!=HD_VM_OUTA_OFF && vdec->field!=WIS_FIELD_NONE) { 
				do_focus_sync(vdec, pts, &sdtv_sync_retries, &skip_next);
}
#endif			
		}

		handle_tricks ( vdec, pause );
		pause=0;
		if ( FD_ISSET ( vdec->disp_fd, &rfds ) ) {
			processed=true;
			if(wis_dec_get_displayed_yuv ( vdec->disp_fd, &index )) {
				perror ( "video_out_thread wis_dec_get_displayed_yuv" );
				vdec->need_reset=NEED_RESET_TRESHOLD;
				break;
			} // if
//			printf("VQUEUE %i.%i.%i.%i %d\n", queued, field, vdec->force_field, pause, index);
			if ( index != -1 ) {
				if(wis_dec_queue_yuv_buffer ( vdec->yuv_fd, index )) {
					perror ( "video_out_thread wis_dec_queue_yuv_buffer" );
					vdec->need_reset=NEED_RESET_TRESHOLD;
					break;
				} // if
				//printf("VQUEUE %i.%i.%i.%i %d\n", queued, field, vdec->force_field, pause, index);
				queued--;
				if (fade_val<256 ) {
					if (IS_ANY_TRICK(vdec)) 
						fade_val=256;
					fade(fade_val);
//					printf("FADE  %f %i\n",fade_val,vdec->vattr.width);
					if (vdec->video_mode==WIS_FMT_H264) {
						if (vdec->vattr.width>1280)  // usually less startup distortions than 1280
							fade_val=fade_val*2.55;
						else
							fade_val=0.5+fade_val*1.8;
					}
					else
						fade_val=256;//Don´t fade mpeg2 - imho not needed and conflicts with stillframes 
					if (fade_val>=256) {
						fade(256);
					}
				}
			}
		}
		if(!processed) usleep(100);
	}
	vdec_printf ( "YUV thread DONE\n" );
	return NULL;
}
/*----------------------------------------------------------------------------*/

void decoder_pause ( vdec_t *vdec, int pause ) {
	vdec_printf ( "####################### PAUSE %i\n",pause );
	vdec->pause=pause;
}
/*----------------------------------------------------------------------------*/
void decoder_trick ( vdec_t *vdec, int a, int b ) {
	vdec_printf ( "####################### TRICK %i %i\n",a,b );

	// reset decoder when switching to or back from fast fwd/rev
	// otherwise the image may be distorted for a longer time
	if ( !vdec->xine_mode && b && vdec->trickmode && ( !vdec->pause && vdec->trickspeed==0 ))
		decoder_stop ( vdec );
		decoder_setup(vdec, vdec->video_id, vdec->adec->audio_id);
	// Calling decoder_stop results in killing hdplayer!
	//decoder_stop(vdec/*, vdec->video_id, vdec->adec->audio_id*/);

	vdec->trickspeed=a;
	vdec->trickmode=b;
	if ( vdec->xine_mode ) {
		if(vdec->display_configured) video_display_configure(vdec);
		if(vdec->adec && vdec->adec->af.config) {
			vdec->adec->af.set=0;
			if ( !vdec->adec->af.config ( &vdec->adec->af, 1) ) {
				vdec->adec->af.set=1;
				decoder_force_av ( vdec );
			} // if
		} // if
	} // if
}
/*----------------------------------------------------------------------------*/
void decoder_flush ( vdec_t *vdec ) {
	ioctl ( vdec->decoder_fd, GO8XXXIOC_FLUSH );
	ioctl ( vdec->adec->audio_fd, ADEC_FLUSH );
	ioctl ( vdec->disp_fd, GO8XXXIOC_FLUSH );
	ioctl ( vdec->yuv_fd, GO8XXXIOC_FLUSH );
}
/*----------------------------------------------------------------------------*/
void decoder_stop ( vdec_t *vdec ) {
	fade(0xc0);
	vdec_printf ( "Decoder Stop\n" );
	if (vdec->id) {
		vdec->run_video_out=0;
		fade(0x80);
		pthread_join ( vdec->video_out_thread, NULL );
		fade(0x40);
		ioctl ( vdec->adec->audio_fd, ADEC_FLUSH );
		fade(0x20);
		decoder_flush(vdec);
		vdec_printf("Decoder stop done 1\n");
		fade(0x00);
		usleep(40*1000);
	}	
	else if ( vdec->decoder_fd>=0 ) {
		vdec->run_video_out=0;
		fade(0x80);
		pthread_join ( vdec->video_out_thread, NULL );
		fade(0x40);
		decoder_flush ( vdec );
		fade(0x20);
		wis_dec_stop ( vdec->decoder_fd );
		wis_dec_yuv_display_stop ( vdec->disp_fd );
		wis_dec_yuv_capture_stop ( vdec->yuv_fd );
		fade(0x00);
		wis_dec_unmap_yuv_buffers ( vdec->yuv_fd, ( void ** ) &vdec->yuv_buffers, vdec->yuv_buffer_count, vdec->yuv_buffer_size );
		wis_dec_unmap_ring_buffer ( vdec->decoder_fd, vdec->ves_buf, vdec->ves_size );

		close ( vdec->decoder_fd );
		close ( vdec->yuv_fd );
		close ( vdec->disp_fd );
		vdec->decoder_fd=-1;

		if (!chiprev)
			usleep(250*1000);  // set 100ms for rblite !!!
		else
			usleep(40*1000);

		vdec_printf("Decoder stop done 2\n");
	}
	fade(0x100);
}
/*----------------------------------------------------------------------------*/

void decoder_setup ( vdec_t *vdec, int vpid, int apid ) {
	int i,w,h;
	vdec_printf ( "Decoder setup %x %x\n",vpid,apid );
	decoder_stop ( vdec );
	if (vdec->id==0) {
		vdec->decoder_fd = open ( dec_device, O_RDWR|O_NONBLOCK );
		if ( vdec->decoder_fd < 0 ) {
			fprintf ( stderr, "Unable to open %s: %s\n", dec_device,
				  strerror ( errno ) );
			return;
		}
		vdec->ves_size = wis_dec_map_ring_buffer ( vdec->decoder_fd,
							   ( void** ) &vdec->ves_buf );
		printf ( "ves buffer size %i\n",vdec->ves_size );
		if ( vdec->ves_size < 0 )
			return;
		
		vdec->yuv_fd = open ( dec_device, O_RDWR );
		if ( vdec->yuv_fd < 0 ) {
			fprintf ( stderr, "Unable to open %s: %s\n", dec_device,
				  strerror ( errno ) );
			return;
		}
		vdec->yuv_buffer_count=MAX_YUV_BUFFERS;
		w = (vdec->xine_mode&&vdec->vattr.width) ?vdec->vattr.width :1920;
		h = (vdec->xine_mode&&vdec->vattr.height)?vdec->vattr.height:1080;
		vdec->yuv_buffer_size=wis_dec_map_yuv_buffers ( vdec->yuv_fd, &vdec->yuv_buffer_count, ( void ** ) &vdec->yuv_buffers, w, h);

		vdec_printf ( "YUV BUFFERS %i, size %i (%dx%d) %d %d %d\n",vdec->yuv_buffer_count,vdec->yuv_buffer_size,w,h,vdec->xine_mode,vdec->vattr.width,vdec->vattr.height );

		for ( i = 0; i < vdec->yuv_buffer_count; ++i )
			wis_dec_queue_yuv_buffer ( vdec->yuv_fd, i );


		vdec->disp_fd = open ( disp_device, O_RDWR );
		if ( vdec->disp_fd < 0 ) {
			fprintf ( stderr, "Unable to open %s: %s\n", disp_device,
				  strerror ( errno ) );
			return;
		}

		wis_dec_yuv_display_start ( vdec->disp_fd );
		wis_dec_yuv_capture_start ( vdec->yuv_fd );
	}

	vdec->display_configured=0;
	vdec->found_valid_pts=0;
	vdec->run_video_out=1;
	decoder_flush(vdec);
	vdec_reset ( vdec );
	decoder_first_queue=1;
	pthread_create ( &vdec->video_out_thread, NULL, video_out_thread, vdec );

	vdec->video_id=vpid;
	vdec->adec->audio_id=apid;
}
/*----------------------------------------------------------------------------*/
vdec_t *new_decoder ( int num) {
	vdec_t *vdec;
	int i;

	vdec= ( vdec_t* ) malloc ( sizeof ( vdec_t ) );
	if ( !vdec )
		return 0;
	memset ( vdec, 0, sizeof ( vdec_t ) );
	vdec->adec= ( adec_t* ) malloc ( sizeof ( adec_t ) );
	if ( !vdec->adec ) {
		free ( vdec );
		return 0;
	} // if
	memset ( vdec->adec, 0, sizeof ( adec_t ) );
	vdec->adec->audio_fd=-1;
	vdec_reset ( vdec );
	vdec->disp_fd=-1;
	vdec->decoder_fd=-1;
	vdec->yuv_fd=-1;
	vdec->ves_size=0;
	vdec->id=num;
	vdec->adec->id=num;

	for ( i=0;i<AUDIO_QUEUE_BUFS;i++ ) {
		memset ( &vdec->adec->aqueue[i], 0, sizeof ( vdec->adec->aqueue[i] ) );
//		vdec->adec->aqueue[i].data=malloc(4096);
//		vdec->adec->aqueue[i].len=0;
	}
	init_vcxo();
	fade(0x0); // avoid green rubbish
	set_vcxo ( 0x000 );
	if (num)
		decoder_stc_init(vdec, 0);
	return vdec;
}
/*----------------------------------------------------------------------------*/
void delete_decoder ( vdec_t *vdec ) {
	int i;
	vdec->run_video_out=0;
	pthread_join ( vdec->video_out_thread, NULL );
	close ( vdec->adec->audio_fd );

	close ( vdec->disp_fd );
	close ( vdec->yuv_fd );
	close ( vdec->decoder_fd );

	for ( i=0;i<AUDIO_QUEUE_BUFS;i++ )
		if ( vdec->adec->aqueue[i].data )
			free ( vdec->adec->aqueue[i].data );

	free ( vdec->adec );
	free ( vdec );
}
/*----------------------------------------------------------------------------*/
