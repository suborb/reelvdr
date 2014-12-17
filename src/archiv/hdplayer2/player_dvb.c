/*
  Player over DVB-API
  (c) 2008 Georg Acher (acher at baycom dot de)

  #include <GPL_v2.h>

  FIXME: Trick modes
         External PTS Sync
*/

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>

#include <linux/types.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <linux/go8xxx-video.h>

#include "player_dvb.h"

uint8_t const *findMarker(uint8_t const *Data, int Count, int Offset, unsigned char *PictureType);

/* --------------------------------------------------------------------- */
void dvb_int_shutdown(dvb_player_t *dvbp, int av)
{
	if (av==2) {
		close(dvbp->fd_dvr);
		dvbp->fd_dvr=-1;
	}
	else if (av==1) {
		if (dvbp->fd_video>=0) {
			ioctl(dvbp->fd_video, VIDEO_STOP);
			usleep(200*1000);
			close(dvbp->fd_video);
			usleep(200*1000);
			dvbp->fd_video=-1;
		}	
	}
	else {
		if (dvbp->fd_audio>=0) {
			ioctl(dvbp->fd_audio, AUDIO_STOP);
			close(dvbp->fd_audio);	
			dvbp->fd_audio=-1;
		}
	}
}
/* --------------------------------------------------------------------- */
void dvb_int_open(dvb_player_t *dvbp, int av, int force)
{
	if (av==2) {
		if (dvbp->fd_dvr<0) 
			dvbp->fd_dvr=open("/dev/dvb0.dvr0", O_WRONLY);
		printf("DVR FD %i\n",dvbp->fd_dvr);
	}
	else if (av==1) {
		if (dvbp->fd_video<0 || force) {
			if (force)
				dvb_int_shutdown(dvbp,1);			
			dvbp->fd_video=open(dvbp->output ? "/dev/dvb0.video1" : "/dev/dvb0.video0",
					    O_RDWR|O_NONBLOCK);
		}
	}
	else {
		if (dvbp->fd_audio<0 || force) {
			if (force)
				dvb_int_shutdown(dvbp,0);
			dvbp->fd_audio=open(dvbp->output ? "/dev/dvb0.audio1" : "/dev/dvb0.audio0", 
					    O_RDWR|O_NONBLOCK);
		}
	}      
}
/* --------------------------------------------------------------------- */
int set_dvb_vpid(dvb_player_t *dvbp, int vpid, int vcomp, int fr)
{
	int fd;
	struct dmx_pes_filter_params pesFilterParams;

	if (vpid) {
		if (dvbp->fd_dv>=0) {
			close(dvbp->fd_dv);
			dvbp->fd_dv=-1;
			dvbp->vcomp=-1;
			dvbp->tmp_vcomp=-1;
		}
		if (vcomp>=0)
			ioctl(dvbp->fd_video, VIDEO_SET_STREAMTYPE, vcomp);
	
		ioctl(dvbp->fd_video, VIDEO_PLAY);
		fd=open("/dev/dvb0.demux0", O_RDWR|O_NONBLOCK);
		if (fd<0)
			return -1;
		dvbp->fd_dv=fd;
                pesFilterParams.pid = vpid;
                pesFilterParams.input = DMX_IN_DVR; 
                pesFilterParams.output = DMX_OUT_DECODER; 
                pesFilterParams.pes_type = DMX_PES_VIDEO+5*dvbp->output; 
                pesFilterParams.flags = DMX_IMMEDIATE_START;
                if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0) {
			perror("VPES");
                        return(1); 
		}
                printf("Video filter OK\n");

		dvbp->active=1;
		dvbp->vcomp=vcomp;
        }
	else {
		if (dvbp->fd_dv>=0) {
			close(dvbp->fd_dv);
			dvbp->fd_dv=-1;	
			dvbp->vcomp=-1;
			dvbp->tmp_vcomp=-1;
		}
	}
	return 0;
}
/* --------------------------------------------------------------------- */
int set_dvb_apid(dvb_player_t *dvbp, int apid, int acomp)
{
	int fd;
	struct dmx_pes_filter_params pesFilterParams;

	if (apid) {
		if (dvbp->fd_da>=0) {
			close(dvbp->fd_da);
			dvbp->fd_da=-1;
			dvbp->acomp=-1;
			dvbp->tmp_acomp=-1;
			dvbp->tmp_acomp1=-1;
		}
		if (acomp>=0)
			ioctl(dvbp->fd_audio, AUDIO_SET_STREAMTYPE, acomp);		
		ioctl(dvbp->fd_audio, AUDIO_PLAY);
		fd=open("/dev/dvb0.demux0", O_RDWR|O_NONBLOCK);
		if (fd<0)
			return -1;
		dvbp->fd_da=fd;
		ioctl(dvbp->fd_audio, AUDIO_SET_DELAY, dvbp->audio_delay);
                pesFilterParams.pid = apid;
                pesFilterParams.input = DMX_IN_DVR; 
                pesFilterParams.output = DMX_OUT_DECODER; 
                pesFilterParams.pes_type = DMX_PES_AUDIO+5*dvbp->output; 
                pesFilterParams.flags = DMX_IMMEDIATE_START;
                if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0) {
			perror("audio pes");
                        return(1); 
		}
                printf("Audio filter OK\n");

		dvbp->active=1;
		dvbp->acomp=acomp;
        }
	else {
		if (dvbp->fd_da>=0) {
			close(dvbp->fd_da);
			dvbp->fd_da=-1;
			dvbp->acomp=-1;
			dvbp->tmp_acomp=-1;
			dvbp->tmp_acomp1=-1;
		}
	}
	return 0;
}
/* --------------------------------------------------------------------- */
dvb_player_t* new_dvb_player(int output)
{
	dvb_player_t *dvbp;
	dvbp=(dvb_player_t*)malloc(sizeof(dvb_player_t));

	memset(dvbp,0,sizeof(dvb_player_t));

	dvbp->fd_video=-1;
	dvbp->fd_audio=-1;
	dvbp->fd_dvr=-1;
	dvbp->fd_dv=-1;
	dvbp->fd_da=-1;
	dvbp->vcomp=-1;
	dvbp->acomp=-1;
	dvbp->tmp_vcomp=-1;
	dvbp->tmp_acomp=-1;
	dvbp->tmp_acomp1=-1;
	dvbp->active=0;
	dvbp->output=0;
	dvb_int_open(dvbp, 2, 1);

	printf("new dvb player\n");
	return dvbp;       
}
/* --------------------------------------------------------------------- */
void stop_dvb_player(dvb_player_t *dvbp)
{
	dvb_int_shutdown(dvbp,1);
	dvb_int_shutdown(dvbp,0);
	dvb_int_shutdown(dvbp,2);
	set_dvb_vpid(dvbp, 0, 0, 0);
	set_dvb_apid(dvbp, 0, 0);
	dvbp->active=0;
}
/* --------------------------------------------------------------------- */
void del_dvb_player(dvb_player_t *dvbp)
{
	stop_dvb_player(dvbp);
	free(dvbp);
}
/* --------------------------------------------------------------------- */
void dvb_set_ts_data(dvb_player_t *dvbp, int vpid, int apid, int fr)
{
	printf("dvb_set_data %p %x %x\n",dvbp,vpid,apid);

	dvb_int_shutdown(dvbp,1);
	dvb_int_shutdown(dvbp,0);
	dvb_int_shutdown(dvbp,2);

	dvb_int_open(dvbp, 2, 1);
	dvb_int_open(dvbp, 0, 1);
	dvb_int_open(dvbp, 1, 1);

	if (fr>=0)
		ioctl(dvbp->fd_video, VIDEO_SET_FRAME_RATE, &fr);
		
	set_dvb_vpid(dvbp, 0, -1, 0);
	set_dvb_apid(dvbp, 0, -1);

	dvbp->vpid=vpid;
	dvbp->apid=apid;
	dvbp->framerate=fr;
	dvbp->freeze=0;
	dvbp->trick=0;	
}
/* --------------------------------------------------------------------- */
void dvb_analyze_video(dvb_player_t *dvbp, const uint8_t *data, int len, int flag)
{
	unsigned char const *x;
	unsigned char z,pt;
	int l,pos=0;

	if (flag) {
		while(1) {
			x=findMarker(data,len,pos, &pt);
			if (!x) 
				return;
			l=data+len-x;
			z=x[3];

			if (z==0x00) {
				if (dvbp->tmp_vcomp<0)
					printf("############################## VIDEO SYNC MPEG2\n");
				dvbp->tmp_vcomp=VIDEO_CAP_MPEG2;
				return;
			}                       
		
			if ((z==0x67 || z==0x09)&&dvbp->vcomp!=VIDEO_CAP_MPEG2) {
				if (dvbp->tmp_vcomp<0)
					printf("############################## SYNC H.264\n");
				dvbp->tmp_vcomp=VIDEO_CAP_H264;
				return;
			}               
			pos=x-data+4;
		}
	}
}
/* --------------------------------------------------------------------- */
void dvb_analyze_audio(dvb_player_t *dvbp, const uint8_t *data, int len, int flag)
{
	uint8_t const *x=data,*lx;
        uint8_t pt,z;
        int pos=0,l;
        int fr=0;
        int start;

        lx=data;
	if (flag) {
		while(1) {
			x=findMarker(data,len,pos, &pt);
			if (!x)				
				return;

			l=data+len-x;
			z=x[3];
			start=0;
			if (z == 0xbd && (x[7]&0xc0)==0x80) {
				fr = AUDIO_CAP_AC3;
				start=1;
				if (dvbp->tmp_acomp==-1)
					printf("############################## Detected AC3 Audio\n");
			}
			if ((z&0xf0)==0xc0 && (x[7]&0xc0)==0x80) {
				fr = AUDIO_CAP_MP2;
				start=1;
				if (dvbp->tmp_acomp==-1)
					printf("############################## Detected MPEG2 Audio\n");
			}
			if (start) {
				if (fr==dvbp->tmp_acomp1)
					dvbp->tmp_acomp = fr;

				dvbp->tmp_acomp1 = fr;
			}
			pos=x-data+4;       
		}
        }
}
/* --------------------------------------------------------------------- */
void dvb_detect_format(dvb_player_t *dvbp, const uint8_t *data, int len)
{
	int adapfield, i, offset;
	int pes_start, pid;

	for ( i = 0; i < len; i += 188, data+=188) {
		if (data[3]&0xc0) // Scrambled 
                        continue;
		adapfield=data[3]&0x30;
                
                if (adapfield==0 || adapfield==0x20)
                        continue;
		
		if (adapfield==0x30) {
                        offset=5+data[4];
                        if (offset>188)
                                continue;
                }
                else
                        offset=4;
                        
                pes_start=data[1]&0x40;
		pid=((data[1]&0x1f)<<8)|data[2];
		if (pid==dvbp->vpid && (data[3]&0x10))
			dvb_analyze_video(dvbp, data, 188-offset, pes_start);
		if (pid==dvbp->apid && (data[3]&0x10))
			dvb_analyze_audio(dvbp, data, 188-offset, pes_start);
	}
}
/* --------------------------------------------------------------------- */
void dvb_freeze(dvb_player_t *dvbp)
{
	if (dvbp && dvbp->fd_audio>=0 && dvbp->fd_video>=0) {
		printf("Freeze\n");
		ioctl(dvbp->fd_audio,AUDIO_PAUSE);	
		ioctl(dvbp->fd_video,VIDEO_FREEZE);
		dvbp->freeze=1;
	}
}
/* --------------------------------------------------------------------- */
void dvb_stats(dvb_player_t *dvbp, dvb_player_stats_t *stats)
{
	video_dec_stats_t vstats={0};
	audio_dec_stat_t astats={0};
	
	if (dvbp->vcomp>=0)
		ioctl(dvbp->fd_video, VIDEO_DEC_STATUS, &vstats);

	stats->v_pes_frames=vstats.sent_pes_count;
	stats->v_displayed_frames=vstats.dsp_frame_count;
	stats->v_decoded_frames=vstats.dcd_frame_count;
	stats->v_dropped_frames=vstats.drop_frame_count;
	stats->v_invalid_pts=vstats.inv_pts_frame_count;

	if (dvbp->acomp>=0) 
		ioctl(dvbp->fd_audio, AUDIO_DEC_STATUS, &astats );

	stats->a_pes_frames=astats.sent_pes_count;
	stats->a_recv_frames=astats.recv_frame_count;
	stats->a_late_frames=astats.late_frame_count;
	stats->a_dropped_frames=astats.drop_frame_count;
	stats->a_sent_frames=astats.sent_frame_count;
	stats->a_bytes=astats.play_byte_count;
}
/* --------------------------------------------------------------------- */
void dvb_trick(dvb_player_t *dvbp, int mode)
{
	if (dvbp && mode!=dvbp->trick) {
		
		if (mode==1) {
			dvbp->apid_bak=dvbp->apid;
			dvbp->apid=0;
			set_dvb_apid(dvbp, dvbp->apid, dvbp->tmp_acomp);
			printf("FREEZE X\n");
		}
		if (mode==0) {
			dvbp->apid=dvbp->apid_bak;
			set_dvb_apid(dvbp, dvbp->apid, dvbp->tmp_acomp);
		}
		dvbp->trick=mode;
	}
}
/* --------------------------------------------------------------------- */
void dvb_audio_delay(dvb_player_t *dvbp, int delay)
{
	if (dvbp->fd_audio>=0)
		ioctl(dvbp->fd_audio, AUDIO_SET_DELAY, delay);
	dvbp->audio_delay=delay;
}
/* --------------------------------------------------------------------- */
void dvb_write_common(dvb_player_t *dvbp, const uint8_t *data, int size)
{
	if ((dvbp->vpid && dvbp->tmp_vcomp<0) ||
	    (dvbp->apid && dvbp->tmp_acomp<0))
		dvb_detect_format(dvbp, data, size);

	if (dvbp->vpid && dvbp->vcomp<0 && dvbp->tmp_vcomp>=0)
		set_dvb_vpid(dvbp, dvbp->vpid, dvbp->tmp_vcomp, dvbp->framerate);

	if (dvbp->apid && dvbp->acomp<0 && dvbp->tmp_acomp>=0)
		set_dvb_apid(dvbp, dvbp->apid, dvbp->tmp_acomp);

	if (dvbp->freeze) {
		ioctl(dvbp->fd_video,VIDEO_PLAY);
		ioctl(dvbp->fd_audio,AUDIO_PLAY);
		dvbp->freeze=0;
	}	
}
/* --------------------------------------------------------------------- */
typedef struct  { int kmem; int size} wkbuf_t;
#define VIDEO_WRITE_KBUF       _IOW('o', 99, wkbuf_t)

void dvb_write_ts(dvb_player_t *dvbp, const uint8_t *data, void *kmem, int size)
{
	if (! dvbp->apid && !dvbp->vpid)
		return;

	dvb_write_common(dvbp,data,size);

//	printf("%i %i %x %x %x\n",dvbp->vpid, dvbp->vcomp, dvbp->apid, dvbp->acomp, dvbp->tmp_acomp);
	if (!(dvbp->vpid && dvbp->vcomp<0) &&
	    !(dvbp->apid && dvbp->acomp<0)) {

		if (1) // !kmem)
			write (dvbp->fd_dvr, data, size);
		else {
			wkbuf_t val;
			val.kmem=kmem;
			val.size=size;
			ioctl(dvbp->fd_video, VIDEO_WRITE_KBUF, &val);
		}
	
		// Buffer management
		if (0){
			dvb_player_stats_t stats;
			dvb_stats(dvbp,&stats);

			int diff=stats.a_sent_frames-stats.a_recv_frames;
//			printf("%i %i %i %i %i # %i\n",stats.a_pes_frames,stats.a_recv_frames,
//			       stats.a_sent_frames,stats.a_late_frames,stats.a_dropped_frames,diff);		

			if (diff>30 && stats.a_dropped_frames==dvbp->audio_dropped) {
				printf("sleep\n");
				if (diff>50)
					diff=50;
				usleep((diff-10)*20*1000); // 24ms per frame

			dvbp->audio_dropped=stats.a_dropped_frames;
			}
		}
	}
}
/* --------------------------------------------------------------------- */
void dvb_set_es_data(dvb_player_t *dvbp, int vcomp, int acomp)
{
	int vcomp_int=VIDEO_CAP_MPEG2, acomp_int=AUDIO_CAP_MP2;
	printf("dvb_set_es_data %p %x %x\n",dvbp,vcomp,acomp);

	dvb_int_shutdown(dvbp, 1);
	dvb_int_shutdown(dvbp, 0);
	dvb_int_open(dvbp, 1, 1);
	dvb_int_open(dvbp, 0, 1);
	if (vcomp)
		vcomp_int=VIDEO_CAP_H264;
	if (acomp)
		acomp_int=AUDIO_CAP_AC3;

	printf("VFD%i\n",dvbp->fd_video);
	ioctl(dvbp->fd_video, VIDEO_SET_STREAMTYPE, vcomp_int);
	ioctl(dvbp->fd_video, VIDEO_PLAY);
	ioctl(dvbp->fd_audio, AUDIO_SET_STREAMTYPE, acomp_int);
	ioctl(dvbp->fd_audio, AUDIO_PLAY);		
}
/* --------------------------------------------------------------------- */
void dvb_write_es(dvb_player_t *dvbp, uint64_t pts, const uint8_t *data, int size)
{
	if (dvbp->fd_video<0)
		return;
//	printf("write es %i\n",size);
//	write(dvbp->fd_video,  &pts, 8);
	write(dvbp->fd_video,  data, size);
}
/* --------------------------------------------------------------------- */
