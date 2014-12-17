#ifndef _INCLUDE_PLAYER_LL_H
#define _INCLUDE_PLAYER_LL_H

#include <sys/time.h>
#include <stdint.h>

#include "reel.h"

typedef unsigned char u08_t;
typedef unsigned long long u64_t;


// from nmtk.h

typedef enum {
        FORMAT_NONE,
        FORMAT_UNSUPPORTED,
        FORMAT_TYPE_MUX                 = 0x01000000,
        FORMAT_DEMUX_MGR,
        FORMAT_AVI,
        FORMAT_WAV,
        FORMAT_MP3_FILE,
        FORMAT_MOV,
        FORMAT_M2TS,
        FORMAT_MPG,
        FORMAT_RTP,
        FORMAT_TYPE_VIDEO               = 0x02000000,
        FORMAT_RAWVIDEO,
        FORMAT_MPV,
        FORMAT_M4V,
        FORMAT_H264,
        FORMAT_H263,
        FORMAT_JPEG,
        FORMAT_TYPE_AUDIO               = 0x04000000,
        FORMAT_RAWAUDIO,
        FORMAT_MPA,
        FORMAT_M4A,
        FORMAT_AC3,
        FORMAT_AMR,
        FORMAT_TYPE_MASK                = 0xff000000
} media_format_t;

enum {
        FORMAT_ASF = FORMAT_TYPE_MUX + 0x8080,
        FORMAT_WMV = FORMAT_TYPE_VIDEO + 0x8080,
        FORMAT_WMA = FORMAT_TYPE_AUDIO + 0x8080,
};

typedef enum {
        RAWAUDIO_S16LE,
        RAWAUDIO_S16BE,
        RAWAUDIO_ALAW,
        RAWAUDIO_ULAW,
} rawaudio_subformat_t;

typedef struct {
	u08_t *data;
	int length;
	int max_length;
	int used;
	int queued;
	int index;
	int frame_start;
} vbuf_t;

#define PUT_16(p,v) (((unsigned char *)(p))[0]=((v)>>8)&0xff,((unsigned char *)(p))[1]=(v)&0xff)
#define PUT_32(p,v) (((unsigned char *)(p))[0]=((v)>>24)&0xff,((unsigned char *)(p))[1]=((v)>>16)&0xff, \
		     ((unsigned char *)(p))[2]=((v)>>8)&0xff,((unsigned char *)(p))[3]=(v)&0xff)
#define GET_16(p) (unsigned short)((((unsigned char *)(p))[0]<<8)|((unsigned char *)(p))[1])
#define GET_32(p) (unsigned int)((((unsigned char *)(p))[0]<<24)|(((unsigned char *)(p))[1]<<16)| \
				 (((unsigned char *)(p))[2]<<8)|((unsigned char *)(p))[3])
#define GET_16_LE(p) (unsigned short)((((unsigned char *)(p))[1]<<8)|((unsigned char *)(p))[0])
#define GET_32_LE(p) (unsigned int)((((unsigned char *)(p))[3]<<24)|(((unsigned char *)(p))[2]<<16)| \
				    (((unsigned char *)(p))[1]<<8)|((unsigned char *)(p))[0])
#define FOURCC(c) (unsigned int)(((c)[0]<<24)|((c)[1]<<16)|((c)[2]<<8)|(c)[3])
#define CONST_FOURCC(a,b,c,d) (unsigned int)(((a)<<24)|((b)<<16)|((c)<<8)|(d))


/*-------------------------------------------------------------------------*/

typedef struct {
    int fd;
    int playing;

    // media format
    media_format_t   media_format;
    int              media_w;
    int              media_h;

    // crop area
    int  crop_x_off;
    int  crop_y_off;
    int  crop_w;
    int  crop_h;

    // aspect ratio
    int  aspect_overscan;
    int  aspect_w;
    int  aspect_h;

	u08_t *data;
	int bufsize;
	int rdptr;
	int wrptr;
	int owrptr;
	int len_queued;
	int first;
	
    vbuf_t *buffers;
    int buf_count;
    int used_buffers;
} video_player_t;

video_player_t *new_video_player(void);
void delete_video_player(video_player_t* vp);
void open_video_player(video_player_t *vp);
void close_video_player(video_player_t *vp);
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
                        int              aspect_h);

int set_vformat(video_player_t* vp, media_format_t format, int w, int h);
int set_frameperiod(video_player_t *vp, int numerator, int denominator);
void stop_video_player(video_player_t *vp);
int set_frameperiod(video_player_t* vp, int numerator, int denominator);
void set_crop_area(video_player_t* vp, int x_off, int y_off, int w, int h);
void set_aspect_ratio(video_player_t *vp, int overscan, int px, int py, int pw, int ph);
int stream(video_player_t* vp, int enable);
int queue_vbuffer(video_player_t* vp, vbuf_t *buf, struct timeval *tv);
u64_t get_stc(video_player_t* vp);
vbuf_t *get_vbuffer(video_player_t* vp);
vbuf_t *get_vbuffer_nb(video_player_t* vp);
void release_vbuffer(video_player_t *vp, vbuf_t *vbuf);
void reset_video_player(video_player_t *vp, void *vfi, int restart);

/*-------------------------------------------------------------------------*/

#define ABUFFER_CAPACITY (16 * 1024)
struct abuf_s
{
    size_t         length; // Length of used data in buf.
    int            samplerate;
    int            sample_count;
    int            channels;
    media_format_t media_format;
    bool           used;    

    struct
    {
        uint64_t  pos; // Sample-position in audio stream.
        uint8_t   buf[ABUFFER_CAPACITY];
    } data;
};

typedef struct abuf_s abuf_t;

#define NOF_ABUFFERS 2
typedef struct audio_player_s
{
    int            fd;
    media_format_t last_media_format;
    int            last_samplerate;
    int            last_channels;
    uint64_t       play_pos; // Sample-position in audio stream.
    uint64_t       avg_delay;	
    int            last_sample_size;
    abuf_t buffers[NOF_ABUFFERS];
} audio_player_t;

audio_player_t *new_audio_player(void);
void close_audio_player(audio_player_t *ap);
void delete_audio_player(audio_player_t *ap);
int set_aformat(audio_player_t *ap, media_format_t format, int subformat,
               int samplerate_in, int sample_rate_out,
               int channels, int downmix,
               u08_t *add_data, int add_len);
int set_clock(audio_player_t *ap, int clock);
int set_skew(audio_player_t *ap, int skew);
u64_t get_position(audio_player_t *ap);
abuf_t *get_abuffer(audio_player_t *ap);
int play_frame(audio_player_t *ap, abuf_t *buf);
int play_audio_frame(audio_player_t *ap, abuf_t *buf);
int play_frame_raw(audio_player_t *ap, u08_t *data, int len, u64_t pts);
void get_position_and_time(audio_player_t *ap, int64_t *position, int64_t *tm);

/*-------------------------------------------------------------------------*/
static inline void put_abuffer(audio_player_t *ap, abuf_t *abuf)
{
    abuf->used = false;
}
/*-------------------------------------------------------------------------*/

struct video_frame_info_s
{
    media_format_t vformat;
    uint32_t generation;
    int width, height;
    int aspect_w, aspect_h;
    char pic_type;
    int64_t temp_ref;
    int pts;
    int trickmode, i_frames_only;
    uint8_t *data;
    int data_len;
    int frame_start;
    bool still;
};

typedef struct video_frame_info_s video_frame_info_t;

struct audio_info_s
{
    uint8_t const  *data;
    int             len;
    int             pts;
    int             samplerate;
    uint64_t        frame_nr;
    media_format_t  format;
};

typedef struct audio_info_s audio_info_t;

typedef void (*audio_flush_func)(void *player, audio_info_t const *ai);
typedef void (*video_flush_func)(void *player, video_frame_info_t const *frame);
typedef uint8_t *(*get_video_out_buffer_func)(void *player);

int queue_vdata(video_player_t *vp, video_frame_info_t *vfi, struct timeval *tv);
void skip_video(video_player_t *vp, struct timeval *tv);
void audio_decoder_flush(audio_player_t *ap);

#define VDMXBUFLEN (1024*1024)

#endif
