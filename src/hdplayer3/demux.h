#ifndef _DEMUX_H
#define _DEMUX_H
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <errno.h>
#include <linux/types.h>

#include <linux/go8xxx.h>
#include <linux/go8xxx-adec.h>
#include <hdshm_user_structs.h>

#include "audio_parse.h"
#include "video_parse.h"
#include "wis-decoder-api.h"

#define SMOOTH_AUDIO_CHG

#define NEED_RESET_TRESHOLD 30
#define MAX_YUV_BUFFERS 32
// Changed from 128 due to h.264 playback of mov files. Having more than 32? won't work
// 32 is the default from libdecoder and with a max-resolution of 1920x1080 only 17 are returned.

#define INIT_PTS_DELAY   (0.20)
#define VIDEO_AHEAD      (0.20)
#define AUDIO_AHEAD      (VIDEO_AHEAD+0.10)
#define AUDIO_AC3_AHEAD  (VIDEO_AHEAD+0.25)
#define AUDIO_AAC_AHEAD  (VIDEO_AHEAD+0.20)

#define DEFAULT_FRAMERATE_DIV AUDIO_ONLY_DEFAULT_FRAMERATE_DIV
#define DEFAULT_FRAMERATE_NOM 1

#define NOPTS_VALUE 0xffffffffffffffffULL
#define PTS_RANGE (45000ULL)
#define NO_PICTURE 0
#define I_FRAME    1
#define P_FRAME    2
#define B_FRAME    3


#define TS_PES_VIDEO 0
#define TS_PES_AUDIO 1

enum Mpeg_TSState {
        MPEGTS_HEADER = 0,
        MPEGTS_PESHEADER_FILL,
        MPEGTS_FIRST_PAYLOAD,
        MPEGTS_PAYLOAD,
        MPEGTS_FIRST_PAYLOAD_VIDEO,
        MPEGTS_PAYLOAD_VIDEO,
        MPEGTS_SKIP,
};

#define MAX_PES_HEADER_SIZE         (9 + 255)
#define PES_START_SIZE              9

typedef struct pes_context {
        int stream_type;
        enum Mpeg_TSState state;
        int data_index;
        int total_size;
        int pes_header_size;
        u64 pts, dts;
        u32 flags;
        int error;      
        u8 header[MAX_PES_HEADER_SIZE];
} pes_context_t;

/*----------------------------------------------------------------------------*/

#define VES_BUF_START(v)   (v->ves_buf)
#define VES_BUF_SIZE(v)    (v->ves_size)

//#define AUDIO_QUEUE_BUFS 1024
#define AUDIO_QUEUE_BUFS 256      // about 3s for 24ms frames

typedef struct {
	u64 pts;
	int len;
        int size;
	u8  *data;
} audio_queue_t;

/*----------------------------------------------------------------------------*/
typedef struct {
        int                   id;  
	int                   audio_fd;
        struct audio_frame_s  af;
        u32                   audio_id; // pid
	int                   cur_audio_type;

        int                   audio_on;
        u8                    abuf[32*1024];

        u32                   frame_count;
        u32                   pes_count;
        s32                   delay;

        u32                   sample_rate;
        u8                    channels;
        int                   type;
        
        audio_queue_t         aqueue[AUDIO_QUEUE_BUFS];
        int                   wp;
        int                   rp;
	u64                   last_played_pts;
	int                   nb_channels;
	u32                   frequency;
} adec_t;
/*----------------------------------------------------------------------------*/
typedef struct {
	int                    decoder_fd;
	int                    disp_fd;
	int                    yuv_fd;
	int                    id; // plane
	
	adec_t                 *adec;
        u32                    video_id;  // pid
	int                    video_mode;
	int                    tmp_video_mode;

#define AVOUTLEN (32*1024)
        u8                     vbuf[AVOUTLEN];
        u32                    pts_inc;
        int                    aspect;
        u32                    pes_count;
	int                    cont;

        u64                    next_pts;
        u64                    last_pts;
        u32                    last_start;
        u32                    last_len;
        u32                    last_flags;
        unsigned long          last_jiffies;

	u32                    ves_free;
        u32                    ves_wp;
        u32                    ves_rp;
        u32                    ves_rb;
        u32                    ves_wb;
        u32                    ves_state;

        int                    ves_nalu;
        int                    ves_nalu_pos;
        u32                    ves_nalu_start;
        u32                    ves_nalu_start_len;
        u32                    ves_size;
        u8                    *ves_buf;
        u8                     ves_nalu_buf[64];

        u64                    total_len;

        u64                    first_pts;
        u64                    prev_pts;
        int64_t                pts_off;
        int64_t                old_pts_off;
        int                    old_pts_count;
        u64                    aspect_chg_pts;
        u64                    audio_chg_pts;

        video_attr_t           vattr;
        video_attr_t           vattr_old;

        int                    stc_mode;
        int                    live_mode;

        pes_context_t          pes[16]; // FIXME

        int                    need_reset;
	int                    force_field;
	wis_field_t            field;
	int                    run_video_out;
	int                    display_configured;
	int                    found_valid_pts;
	int                    pause;
	int                    old_pause;

	int                    yuv_buffer_count;
	void                   *yuv_buffers[MAX_YUV_BUFFERS];
	int                    yuv_buffer_size;
	pthread_t              video_out_thread;
	int                    stc_init;

	int                    trickspeed;
	int                    old_trickspeed;
	int                    trickmode;
	int                    old_trickmode;
	u64                    last_stc;
        int                    xine_mode;
	int                    wakeup_throttle;
	int                    frameno;
	int                    is_sdtv_out;
	int                    needs_sdtv_hack;
	u64                    last_disp_pts;
	int                    pts_shift;
	int                    ac3_pts_shift;
	u64                    pts_delay;
	int                    pts_delay_flag;
	int                    decoder_frames;
	u64                    last_decoder_pts;
	u64                    last_orig_pts;
	int64_t                stc_diff;
	int                    got_video_data;
	int                    frame_rate;
} vdec_t;

#define IS_SLOMO(v) ((!v->trickmode && v->trickspeed>0) || (v->trickmode && v->trickspeed>=24))
#define IS_FAST(v) (v->trickmode && (v->trickspeed && v->trickspeed<24))
#define IS_ANY_TRICK(v) (IS_SLOMO(v) || IS_FAST(v))
#define IS_PLAY(v) (v->trickmode && !v->trickspeed)

vdec_t *new_decoder(int num);
void delete_decoder(vdec_t *vdec);
void decoder_setup(vdec_t *vdec, int vpid, int apid);
void decoder_trick(vdec_t *vdec, int a, int b);
void decoder_pause(vdec_t *vdec, int pause);
void decoder_stop(vdec_t *vdec);
void decoder_force_av(vdec_t *vdec);

void audio_configure(vdec_t *vdec);
void video_configure(vdec_t *vdec);
void video_decoder_configure(vdec_t *vdec);
void set_default_framerate(vdec_t *vdec);

int audio_wait_queue(vdec_t *vdec, int timeout);

int decoder_video_qbuf(vdec_t *vdec, u32 start, u32 len, u64 pts, u32 flags);
u32 decoder_vbuf_free(vdec_t *vdec);
void decoder_stc_init(vdec_t *vdec, u64 pts);

void vdec_reset(vdec_t *vdec);
int vdec_write_ts_block(vdec_t * vdec, u8 *data, int len);

int vdec_write_es(vdec_t * vdec, const u8 *es, int len, int pusi, u64 pts, u32 flags, u32 error);
int vdec_write_pes(vdec_t *vdec, u8 * buf, int buf_size, int is_start, int pes_type);

void vdec_adjust_pts(vdec_t *vdec, u64 *pts, u64 off, int doad);
void vdec_set_stc(vdec_t *vdec, u64 pts);
u64 vdec_get_stc(vdec_t *vdec);

int adec_push_es(vdec_t * vdec, const u8 * buf, unsigned int buf_size, int pes_start, u64 pts, u32 error);
int adec_callback(struct audio_frame_s *af, int start, int len, uint64_t pts);
int adec_config(struct audio_frame_s *af, int force);
int adec_write_buff(vdec_t *vdec, uint64_t pts, u8 *data, int len);

int vdec_write_cfg (vdec_t *vdec, hd_vdec_config_t *cfg);
int vdec_write_data(vdec_t *vdec, uint64_t pts, u8 *data, int len);
int adec_write_cfg (vdec_t *vdec, hd_adec_config_t *cfg);
int adec_write_data(vdec_t *vdec, uint64_t pts, u8 *data, int len);

int vdec_check_stillframe(vdec_t *vdec);

#endif
