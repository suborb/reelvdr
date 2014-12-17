/*-------------------------------------------------------------------------*/
/*
  Low Level Player functions for Micronas Decypher

  (c) 2007 Georg Acher (acher at baycom dot de)

  #include <GPL_v2.h>
*/
/*-------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <linux/videodev.h>
#include <linux/soundcard.h>

#include <linux/go8xxx.h>
#include <linux/go8xxx-adec.h>

#include "player_ll.h"

#define MAX_VBUFFERS 128   // max.
#define VIDEO_DEVICE "/dev/video0"
#define AUDIO_DEVICE "/dev/adec0"

void hexdump(uint8_t const *x, int len);
/*-------------------------------------------------------------------------*/
void start_video_player(video_player_t  *vp,
                        media_format_t   media_format,
                        int              media_w,
                        int              media_h,
                        int              crop_x_off,
                        int              crop_y_off,
                        int              crop_w,
                        int              crop_h,
                        int              aspect_overscan,
                        int              aspect_w,
                        int              aspect_h)
{
    stop_video_player(vp);

//    set_crop_area(vp, crop_x_off, crop_y_off, crop_w, crop_h);
    set_vformat(vp, media_format, 0*media_w, 0*media_h);
    set_aspect_ratio(vp, aspect_overscan, media_w, media_h, aspect_w, aspect_h);

    set_frameperiod(vp,400,10000); // Default 25Hz
    stream(vp, 1);


    vp->media_format = media_format;
    vp->media_w = media_w;
    vp->media_h = media_h;

    vp->crop_x_off = crop_x_off;
    vp->crop_y_off = crop_y_off;
    vp->crop_w = crop_w;
    vp->crop_h = crop_h;

    vp->aspect_overscan = aspect_overscan;
    vp->aspect_w = aspect_w;
    vp->aspect_h = aspect_h;

    vp->playing = 1;

    init_vcxo();
    set_vcxo(0x000);
}
/*-------------------------------------------------------------------------*/
void stop_video_player(video_player_t *vp)
{
    if (vp->playing)
    {
        stream(vp, 0);
	ioctl(vp->fd, GO8XXXIOC_FLUSH);
        vp->playing = 0;
    }
}
/*-------------------------------------------------------------------------*/
void close_video_player(video_player_t *vp)
{
    stop_video_player(vp);

    if (vp->data)
	    munmap(vp->data, vp->bufsize);
    if (vp->fd != -1)
    {
        close(vp->fd);
	vp->data = NULL;
        vp->fd = -1;
    }
}
/*-------------------------------------------------------------------------*/
#define GO_RINGSIZE 4194304
void mmap_video_buffer(video_player_t *vp)
{
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = GO8XXX_MEMORY_MMAP_RING;

	if (ioctl(vp->fd, VIDIOC_REQBUFS, &req) != 0) {
		perror("ioctl VIDIOC_REQBUFS");
		exit(-1);
	}

	vp->data = (u08_t*)mmap(NULL, GO_RINGSIZE,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				vp->fd, 0);
	if (vp->data==(void*)-1) {
		perror("mmap_video_buffer");
		exit(-1); 
	}
	vp->bufsize = GO_RINGSIZE;
	vp->rdptr= 0;
	vp->wrptr= 0;
	vp->owrptr= 0;
	vp->len_queued=0;
	vp->first=1;
	printf("mmap_video_buffer: %i bytes @ %p\n",vp->bufsize,vp->data);
}
/*-------------------------------------------------------------------------*/
void open_video_player(video_player_t *vp)
{
    vp->fd = open(VIDEO_DEVICE, O_RDWR | O_NONBLOCK);
    vp->data = NULL;
    mmap_video_buffer(vp);
    ioctl(vp->fd, GO8XXXIOC_FLUSH);
}

/*-------------------------------------------------------------------------*/
video_player_t *new_video_player(void)
{
    video_player_t *vp;

    vp = malloc(sizeof(video_player_t));
    vp->buffers = (vbuf_t*)malloc(MAX_VBUFFERS * sizeof(vbuf_t));

    vp->fd = -1;
    vp->playing = 0;

    return vp;
}
/*-------------------------------------------------------------------------*/
void delete_video_player(video_player_t *vp)
{
    close_video_player(vp);

    free(vp->buffers);
    free(vp);
}
/*-------------------------------------------------------------------------*/
int set_vformat(video_player_t *vp, media_format_t format, int w, int h)
{
    struct v4l2_format pixfmt;

    memset(&pixfmt, 0, sizeof(pixfmt));
        pixfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        switch (format) {
        case FORMAT_H264:
                pixfmt.fmt.pix.pixelformat = GO8XXX_PIX_FMT_H264;
                break;
        case FORMAT_M4V:
                pixfmt.fmt.pix.pixelformat = GO8XXX_PIX_FMT_MPEG4;
                break;
        case FORMAT_MPV:
                pixfmt.fmt.pix.pixelformat = GO8XXX_PIX_FMT_MPEG2;
                break;
        case FORMAT_WMV:
                pixfmt.fmt.pix.pixelformat = GO8XXX_PIX_FMT_WMV3;
                break;
        default:
        printf("Unsupported video format %i\n",format);
        return -1;
    }
//    pixfmt.fmt.pix.width = w;
//    pixfmt.fmt.pix.height = h;
	printf("Set Format %x (%c%c%c%c)\n",pixfmt.fmt.pix.pixelformat,
	       pixfmt.fmt.pix.pixelformat,pixfmt.fmt.pix.pixelformat>>8,	 
	       pixfmt.fmt.pix.pixelformat>>16,pixfmt.fmt.pix.pixelformat>>24);
    if (ioctl(vp->fd, VIDIOC_S_FMT, &pixfmt) < 0) {
        perror("VIDIOC_S_FMT failed");
        return -1;
    }
    return 0;
}
/*-------------------------------------------------------------------------*/
int set_frameperiod(video_player_t *vp, int numerator, int denominator)
{
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    parm.parm.output.timeperframe.numerator = numerator;
    parm.parm.output.timeperframe.denominator = denominator;
    if (ioctl(vp->fd, VIDIOC_S_PARM, &parm)<0) {
        perror("VIDIOC_S_PARAM failed");
        return -1;
    }
    return 0;
}
/*-------------------------------------------------------------------------*/
void set_crop_area(video_player_t *vp, int x_off, int y_off, int w, int h)
{
    struct v4l2_crop crop;

    memset(&crop, 0, sizeof(crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    crop.c.left = x_off;
    crop.c.top = y_off;
    crop.c.width = w;
    crop.c.height = h;
    REEL_VERIFY(ioctl(vp->fd, VIDIOC_S_CROP, &crop) != -1);
}
/*-------------------------------------------------------------------------*/
void set_aspect_ratio(video_player_t *vp, int overscan, int px, int py, int pw, int ph)
{
    struct go8xxx_pic_params pic;
    int playing=vp->playing;

    memset(&pic, 0, sizeof(pic));
    printf("Set aspect ratio video %ix%i %i:%i\n",px,py,pw,ph);
    if (!pw ||  !ph)
	    return;

    stream(vp,0);
    pic.par.width = py*pw;
    pic.par.height =px*ph;
    printf("ASP %i %i %f\n",pic.par.width,pic.par.height,pic.par.height?(float)pic.par.width/(float)pic.par.height:0);
    if (1||overscan)
    {
        pic.flags |= GO8XXX_PIC_FLAG_OVERSCAN;
    }

    ioctl(vp->fd, GO8XXXIOC_S_PIC_PARAMS, &pic); //ignore error
    if (playing)
	    stream(vp,1);
}
/*-------------------------------------------------------------------------*/
int stream(video_player_t *vp, int enable)
{
    int arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    printf("Stream %s\n",enable?"ON":"OFF");
    if (enable) {
        ioctl(vp->fd, VIDIOC_STREAMON, &arg);
    }
    else {
        ioctl(vp->fd, VIDIOC_STREAMOFF, &arg);
	vp->rdptr= 0;
	vp->wrptr= 0;
	vp->owrptr= 0;
	vp->len_queued=0;
	vp->first=1;
	ioctl(vp->fd, GO8XXXIOC_FLUSH);
    }

    return 0;
}
/*-------------------------------------------------------------------------*/
// And now a creative hack...
void reset_video_player(video_player_t *vp, void *xfi, int restart)
{
	struct v4l2_requestbuffers req;
	video_frame_info_t *vfi=(video_frame_info_t *)xfi;

	printf("################### reset_video_player %i #################\n", restart);
	stream(vp,0);
	if (restart==2) {
		close_video_player(vp);
		usleep(20*10000);
		open_video_player(vp);
	}

	if (restart && vp->media_format) {
		set_vformat(vp, vp->media_format, 0*vp->media_w, 0*vp->media_h);
		set_frameperiod(vp,417,10000); // Default 25Hz
		set_aspect_ratio(vp, 0, vfi->width, vfi->height,vfi->aspect_w, vfi->aspect_h);
		stream(vp, 1);
	}
	printf("Reset done\n");
}
/*-------------------------------------------------------------------------*/
void skip_video(video_player_t *vp, struct timeval *tv)
{
	printf("SKIP\n");
	ioctl(vp->fd, GO8XXXIOC_SKIP_TO, tv);
}
/*-------------------------------------------------------------------------*/
FILE *XF=NULL;
int queue_vdata(video_player_t *vp, video_frame_info_t *vfi, struct timeval *tv)
{
	struct v4l2_buffer buf;
	int bfree;
	int retries=0;
#if 0
	if (!XF)
		XF=fopen("log.txt","w");
	fprintf(XF,"Queue %i.%06i %i\n",tv->tv_sec,tv->tv_usec,vfi->data_len);
#endif
	if (vfi->data_len<=0)
		return 0;

	while(1) {
		retries++;
		if (retries>300 || (retries>100 && (time(0)-tv->tv_sec)>=1)) {
			printf("RETRY#1, now %i, pts %i\n",time(0), tv->tv_sec);
			goto do_reset;
		}
		
		if (!vp->first) {       
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = GO8XXX_MEMORY_MMAP_RING;
			if (ioctl(vp->fd, VIDIOC_DQBUF, &buf)==0)
				vp->rdptr=buf.m.offset;
//			printf("rdptr %i\n",vp->rdptr);

		}
		if (vp->wrptr >= vp->rdptr) {
			bfree = vp->bufsize - (vp->wrptr-vp->rdptr);
			if (bfree < vfi->data_len+1*1024*1024) {
				usleep(10000);
				continue;
			}
			if (vp->wrptr+vfi->data_len >= vp->bufsize) {
				int part1=vp->bufsize - vp->wrptr;
				memcpy(vp->data+vp->wrptr, vfi->data, part1);
				memcpy(vp->data, vfi->data+part1, vfi->data_len - part1);
				vp->wrptr=vfi->data_len - part1;
			}
			else {
				memcpy(vp->data+vp->wrptr, vfi->data, vfi->data_len);
				vp->wrptr+=vfi->data_len;
			}
			break;
		}
		else {
			bfree = vp->rdptr - vp->wrptr;
			if (bfree < vfi->data_len+1*1024*1024) {
				usleep(10000);
				continue;
			}
			memcpy(vp->data+vp->wrptr, vfi->data,  vfi->data_len);
			vp->wrptr+=vfi->data_len;
			break;
		}
	}

	vp->len_queued+=vfi->data_len;

	if (vfi->data_len>=256) { // HACK
		retries=0;
		while(1) {
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = GO8XXX_MEMORY_MMAP_RING;
			buf.m.offset = vp->owrptr;
			buf.bytesused = vp->len_queued;

//			printf("o %i wrptr %i l %i, rdptr %i, %i %i\n",vp->owrptr,vp->wrptr,vp->len_queued,vp->rdptr,tv->tv_sec,time(NULL));
			if (tv)
				buf.timestamp = *tv;
			if (ioctl(vp->fd, VIDIOC_QBUF, &buf)<0) {
				if (errno != EAGAIN && errno!=EBUSY) {
					perror("VIDIOC_QBUF");
					printf("old %i, end %i, len %i, rdptr %i\n",vp->owrptr,vp->wrptr,vp->len_queued,vp->rdptr);
					goto do_reset;
				}
			}
			else {
//				printf("QUEUED old %i, end %i, len %i, rdptr %i, pts %08i.%08i\n",
//				       vp->owrptr,vp->wrptr,vp->len_queued,vp->rdptr,(int)tv->tv_sec,(int)tv->tv_usec);
				vp->first=0;
				vp->owrptr=vp->wrptr;
				vp->len_queued=0;
				break;
			}
			retries++;
			if (retries>20)
				goto do_reset;

			printf("RETRY %i, now %i, pts %i\n",retries, time(0), tv->tv_sec);
			if (tv->tv_sec && time(0)>tv->tv_sec)
				goto do_reset;
			usleep(10*10000);

		}
	}		
	return 0;

do_reset:
	printf("o %i wrptr %i l %i, rdptr %i, %i %i\n",vp->owrptr,vp->wrptr,vp->len_queued,vp->rdptr,tv->tv_sec,time(NULL));
	reset_video_player(vp, vfi, 2);
	return 0;
}
/*-------------------------------------------------------------------------*/
FILE *FX=NULL;
int queue_vbuffer(video_player_t *vp, vbuf_t *vbuf, struct timeval *tv)
{
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.index=vbuf->index;
    buf.type=V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.bytesused = vbuf->length;

    if (tv && vbuf->frame_start)
    {
        buf.timestamp = *tv;
    }
#if 0
    if (!FX)
                FX=fopen("hdump.es","w");
        fwrite("01234567",1,8,FX);
        fwrite(vbuf->data,1,buf.bytesused,FX);
#endif

//    printf("queued %i %06i\n",vbuf->length,buf.timestamp.tv_usec);
    if (ioctl(vp->fd, VIDIOC_QBUF, &buf) < 0) {
        perror("VIDIOC_QBUF failed");
        return -1;
    }
    vbuf->queued = 1;
    return 0;
}
/*-------------------------------------------------------------------------*/
u64_t get_stc(video_player_t *vp)
{
    return 0;
}
/*-------------------------------------------------------------------------*/
void release_vbuffer(video_player_t *vp, vbuf_t *vbuf)
{
    vbuf->queued = 0;
    vbuf->used = 0;
    -- vp->used_buffers;
}
/*-------------------------------------------------------------------------*/
vbuf_t *get_vbuffer(video_player_t *vp)
{
    vbuf_t *ret = NULL;
    struct v4l2_buffer buf;

    if (vp->used_buffers < vp->buf_count)
    {
        int n;

        for (n = 0; n < vp->buf_count; ++n)
        {
            ret = &vp->buffers[n];
            if (!ret->used)
            {
                break;
            }
        }
        ++ vp->used_buffers;
    }
    else
    {
        // Dequeue a buffer.
        int r, n;

        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;

        for (n = 0; n < 100; ++n)  // Timeout 3 secs
        {
            r = ioctl(vp->fd, VIDIOC_DQBUF, &buf);
            if (r != -1 || errno != EAGAIN)
            {
                goto got_one_or_error; // Got a buffer back, or error.
            }
            usleep(10000); // 1/100 sec
        }
        // Timed out.

        printf("get_vbuffer() timeout!!!\n");

        // Restart everything.
        close_video_player(vp);
        open_video_player(vp);
        start_video_player(vp,
                           vp->media_format, vp->media_w, vp->media_h,
                           vp->crop_x_off, vp->crop_y_off, vp->crop_w, vp->crop_h,
                           vp->aspect_overscan, vp->aspect_w, vp->aspect_h);

        return get_vbuffer(vp); // This will return an unqueued buffer and end the recursion.

    got_one_or_error:

        if (r == -1)
        {
            perror("VIDIOC_DQBUF failed");
            printf("vp->fd = %d\n", vp->fd);
        }

        REEL_ASSERT(r >= 0);

        ret = &vp->buffers[buf.index];
    }
    ret->queued = 0;
    ret->used = 1;

    // Dequeue as many buffers as possible without blocking.
    /*for (;;)
    {
        int r = ioctl(vp->fd, VIDIOC_DQBUF, &buf);
        if (r == -1)
        {
            break;
        }
        release_vbuffer(vp, &vp->buffers[buf.index]);
    }*/

    return ret;
}
/*-------------------------------------------------------------------------*/
vbuf_t *get_vbuffer_nb(video_player_t *vp)
{
    vbuf_t *ret = NULL;
    struct v4l2_buffer buf;

    if (vp->used_buffers < vp->buf_count)
    {
        int n;

        for (n = 0; n < vp->buf_count; ++n)
        {
            ret = &vp->buffers[n];
            if (!ret->used)
            {
                break;
            }
        }
        ++ vp->used_buffers;
    }
    else
    {
        // Dequeue a buffer.
        int r;

        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;

        r = ioctl(vp->fd, VIDIOC_DQBUF, &buf);
        if (r == -1)
        {
            if (errno != EAGAIN)
            {
                perror("VIDIOC_DQBUF failed");
            }
            return NULL;
        }

        ret = &vp->buffers[buf.index];
    }
    ret->queued = 0;
    ret->used = 1;

    return ret;
}
/*-------------------------------------------------------------------------*/
// Release unqueued buffer
/*void put_vbuffer(video_player_t *vp, vbuf_t *buf)
{
    buf->used=0;
    if (vp->used_buffers!=0)
        vp->used_buffers--;
}*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
//                               AUDIO
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

audio_player_t *new_audio_player(void)
{
    audio_player_t *ap;
    ap = (audio_player_t *)malloc(sizeof(audio_player_t));

    ap->fd = -1;

    ap->last_media_format = FORMAT_NONE;
    ap->last_samplerate   = 0;
    ap->last_channels = 0;
    ap->avg_delay = 50000;
    ap->last_sample_size=32768;
    return ap;
}
/*-------------------------------------------------------------------------*/
void close_audio_player(audio_player_t *ap)
{
    if (ap->fd != -1)
    {
        close(ap->fd);
        ap->fd=-1;
    }      
}
/*-------------------------------------------------------------------------*/
void delete_audio_player(audio_player_t *ap)
{
    if (ap->fd != -1)
    {
        close(ap->fd);
    }
    free(ap);
}
/*-------------------------------------------------------------------------*/
int set_aformat(audio_player_t *ap, media_format_t format, int subformat,
                int samplerate_in, int samplerate_out,
                int channels, int downmix,
                u08_t *add_data, int add_len)
{
    struct adec_config adec_config;

    uint8_t *config_set = NULL;
    int      int_config[15];
    int      config_set_len = 0;
    int      ac3_codec_config_tmpl[15] = { 2, 0, 0, 3, 1, 7,
                                           0x7fffffff, 0x7fffffff, 6, 0, 1, 2,
                                           3, 4, 5 };

    int use_stc = 0;

    printf("set_aformat %x, sf %i, si %i, so %i, chan %i, dm %i\n",
           format,subformat, samplerate_in, samplerate_out,
           channels, downmix);

    if (ap->fd != -1)
    {
        close(ap->fd);
    }

    ap->fd = open(AUDIO_DEVICE, O_WRONLY | 0*O_NONBLOCK);

    adec_config.codec_config = NULL;
    adec_config.codec_config_size = 0;
    adec_config.samplerate = samplerate_in;
    adec_config.out_samplerate = samplerate_out;

    adec_config.channels = channels;
    if (channels > 2 && !downmix)
        adec_config.out_channels = 6;
    else
        adec_config.out_channels = 2;

    if (add_data)
    {
        config_set = add_data;
        config_set_len = add_len;
    }

    switch (format) {
        case FORMAT_RAWAUDIO:
        switch (subformat) {
                case RAWAUDIO_S16LE:
                        adec_config.format = ADEC_FMT_PCM_S16LE;
                        break;
                case RAWAUDIO_S16BE:
                        adec_config.format = ADEC_FMT_PCM_S16BE;
                        break;
                case RAWAUDIO_ALAW:
                        adec_config.format = ADEC_FMT_ALAW;
                        break;
                case RAWAUDIO_ULAW:
                        adec_config.format = ADEC_FMT_MULAW;
                        break;
                default:
            return -1;
                }
                break;
        case FORMAT_MPA:
                adec_config.format = ADEC_FMT_MPEG;
                break;
        case FORMAT_M4A:
                adec_config.format = ADEC_FMT_AAC;
                adec_config.codec_config = config_set;
                adec_config.codec_config_size = config_set_len;
                break;
    case FORMAT_AC3:
                adec_config.format = ADEC_FMT_AC3;
                memcpy(int_config, ac3_codec_config_tmpl, 15 * sizeof(int));
                int_config[8] = adec_config.out_channels;
                if (adec_config.out_channels == 2) {
                        int_config[1] = 2;
                        int_config[5] = 2;
                        int_config[9] = 0;
                        int_config[10] = 2;
                }
                adec_config.codec_config = int_config;
                adec_config.codec_config_size = 15 * sizeof(int);
                break;
        case FORMAT_WMA:
                use_stc = 1;
                adec_config.format = ADEC_FMT_WMA;
                int_config[1] = GET_16_LE(config_set);
                int_config[3] = GET_32_LE(config_set + 8) << 3;
                int_config[4] = GET_16_LE(config_set + 12);
                int_config[5] = GET_16_LE(config_set + 14);
                if (int_config[1] == 0x160 || int_config[1] == 0x161)
                        switch (adec_config.channels) {
                        case 1:
                                int_config[0] = 0x4;
                                break;
                        case 2:
                                int_config[0] = 0x3;
                                break;
                        case 6:
                                int_config[0] = 0x3f;
                                break;
                        default:
                                break;
                        }
                else
                        int_config[0] = GET_32_LE(config_set + 20);
                if (int_config[1] == 0x160)
                        int_config[2] = GET_16_LE(config_set + 20);
                else if (int_config[1] == 0x161)
                        int_config[2] = GET_16_LE(config_set + 22);
                else
                        int_config[2] = GET_16_LE(config_set + 32);
                adec_config.codec_config = int_config;
                adec_config.codec_config_size = 6 * sizeof(int);
                break;
    default:
        return -1;
    }

    set_clock(ap, use_stc);

    if (ioctl(ap->fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
        perror("ADEC_STREAM_CONFIG failed");
        return -1;
    }

    return 0;
}
/*-------------------------------------------------------------------------*/
int set_clock(audio_player_t *ap, int clock)
{
    struct adec_output_config output;

    memset(&output, 0, sizeof(output));
    if (clock)
        output.pts_clock = ADEC_PTS_STC_TSI0;
        else
                output.pts_clock = ADEC_PTS_AUDIO;
//    output.audio_port=ADEC_AUDIO_PORT_SPDIF;

    if (ioctl(ap->fd, ADEC_OUTPUT_CONFIG, &output) < 0) {
        perror("ADEC_OUTPUT_CONFIG failed");
        return -1;
    }
    return 0;
}
/*-------------------------------------------------------------------------*/
int set_skew(audio_player_t *ap, int skew)
{
    ioctl(ap->fd, ADEC_SET_CLOCK_SKEW, &skew);
    return 0;
}
/*-------------------------------------------------------------------------*/
u64_t get_position(audio_player_t *ap)
{
    struct adec_position pos;
    ioctl(ap->fd, ADEC_GET_POSITION, &pos);
    return pos.position;
}
/*-------------------------------------------------------------------------*/
abuf_t *get_abuffer(audio_player_t *ap)
{
    int i;

    for (i = 0; i < NOF_ABUFFERS; ++i)
    {
        if (!ap->buffers[i].used)
        {
            ap->buffers[i].used = true;
            return &ap->buffers[i];
        }
    }

    // Should never get here.
    REEL_ASSERT(false);

    return NULL;
}
/*-------------------------------------------------------------------------*/
int play_frame_raw(audio_player_t *ap, u08_t *data, int len, u64_t pts)
{
    return 0;
}
/*-------------------------------------------------------------------------*/
void get_position_and_time(audio_player_t *ap, int64_t *position, int64_t *tm)
{
    struct adec_position pos;
    ioctl(ap->fd, ADEC_GET_POSITION, &pos);
    *position = pos.position;
    *tm = pos.timestamp.tv_sec * 1000000LL + pos.timestamp.tv_usec;
}
/*-------------------------------------------------------------------------*/
void audio_decoder_flush(audio_player_t *ap)
{
	//printf("audio_decoder_flush\n");
	close(ap->fd);
	ap->fd=-1;
	ap->last_samplerate=0;
	ap->avg_delay=50000;
	ap->last_sample_size=32768;
	ap->play_pos=0;
}
/*-------------------------------------------------------------------------*/
int xskew=0;
int play_audio_frame(audio_player_t *ap, abuf_t *buf)
{
    ssize_t ret;
    int64_t position, tm;
    int64_t delay;

    if (ap->last_samplerate != buf->samplerate ||
        ap->last_media_format != buf->media_format ||
	(buf->media_format==FORMAT_MPA && ap->last_channels != buf->channels   ))
    {
        ap->last_samplerate = buf->samplerate;
        ap->last_media_format = buf->media_format;
	ap->last_channels = buf->channels;
        ap->play_pos = 0;

        set_aformat(ap, buf->media_format, 0,
                    buf->samplerate, buf->samplerate,
                    buf->channels, 0,
                    NULL, 0);
    }

    get_position_and_time(ap, &position, &tm);
    delay =  ap->play_pos - position;

    if (delay < 10000)
    {
//        printf("delay = %lld\n", delay);
    }

    if (ap->avg_delay<8192 && ap->play_pos >= 16384 && position)
    {
        printf("correcting audio play position\n");
        printf("delay = %lld\n", ap->avg_delay);
        printf("ap->play_pos = %lld\n", ap->play_pos);
        printf("position = %lld\n", position);
        ap->play_pos = position + ap->last_sample_size*16; //32768;
	// FIXME: Use STC
    }

    ap->avg_delay=(ap->avg_delay*15+delay)/16;


    buf->data.pos = ap->play_pos;

    /*{
        static FILE *f = NULL;

        if (!f)
        {
            f = fopen("02.mp2", "wb");
        }
        if (f)
        {
            fwrite(buf->data.buf, 1, buf->length, f);
            fflush(f);
        }
        else
        {
            printf("error opening file\n");
        }
    }*/

    ret = write(ap->fd, &buf->data, sizeof(buf->data.pos) +  buf->length);

    ap->play_pos += buf->sample_count;
    ap->last_sample_size=buf->sample_count;

    if ((ap->avg_delay>10*1000*1000)) {
	    audio_decoder_flush(ap);
    } 

    return ret;
}
/*-------------------------------------------------------------------------*/
