// player.c

#include "player.h"

#include "channel.h"
#include "hdplayer.h"

//Start by dl
#include "erfplayer.h"
//End by dl

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev.h>
#include <linux/soundcard.h>
#include <syslog.h>
#include <stdarg.h>


#include <linux/go8xxx.h>
#include <linux/go8xxx-adec.h>

#include "ac3_parser.h"
#include "mpa_parser.h"

#include <hdchannel.h>

//#define DEBUG

static void change_aformat(player_t *pl, audio_info_t const *ai);
static void vflush(void *player, video_frame_info_t const *frame);
static void aflush(void *player, audio_info_t const *ai);
static uint8_t *get_video_out_buffer(void *d);
//static int play_buffered_vframes(player_t *pl);
static void aparser_feed_ac3(player_t *pl, uint8_t const *data, int len);
static abuf_t *aparser_get_frame_ac3(player_t *pl);
static void aparser_feed_mpa(player_t *pl, uint8_t const *data, int len);
static abuf_t *aparser_get_frame_mpa(player_t *pl);
static void aparser_feed_none(player_t *pl, uint8_t const *data, int len);
static abuf_t *aparser_get_frame_none(player_t *pl);

static void put_packet_es_data_mpa(player_t *pl, hd_packet_es_data_t const *packet);
static void put_packet_es_data_mpv(player_t *pl, hd_packet_es_data_t const *packet);
//Start by Klaus
static void put_packet_af_data(player_t *pl, hd_packet_af_data_t const *packet);
//End by Klaus

/*-------------------------------------------------------------------------*/
#define USE_FAST_TRICKMODE

#ifdef USE_FAST_TRICKMODE
#define MAX_BUFF_HOLD_US 500000ll
#define MAX_BUFF_BLOC_US 1000000ll
#define MAX_BUFF_DIFF_US 100000ll
#define MAX_AUDIO_ITEMS 40      //Astra-HD fix (more than 12s auf audio in abuf will discard video frames)
#define DELAY_ON_ERROR 10000
struct vbuf_queue_item {
    struct vbuf_queue_item *next;
    video_frame_info_t vfi;
    struct timeval stamp;
};
#define ABUF_RAW 0
#define ABUF_AC3 1
#define ABUF_DEF 2
struct abuf_queue_item {
    struct abuf_queue_item *next;
    audio_info_t ai;
    int type;
    int64_t tm;
};

struct vbuf_queue_item *vbuf_queue = 0;
struct abuf_queue_item *abuf_queue = 0;
int64_t buf_queue_freeze = 0;

static void rawaflush(void *player, audio_info_t const *ai);
static void ac3flush(void *player, audio_info_t const *ai);
static void aflush(void *player, audio_info_t const *ai);

int vpackets = 0;
int apackets = 0;

int current_vformat=FORMAT_NONE;

int vbuf_queue_play(player_t *pl, int flush) {
    struct vbuf_queue_item *item = vbuf_queue;
    int64_t now = GetCurrentTime();
    int64_t tm = 0;
    if(!item)
        return 0;
    tm = ((int64_t)1000000ll)*item->stamp.tv_sec+item->stamp.tv_usec;
    if (!buf_queue_freeze && (((tm - now) < MAX_BUFF_HOLD_US) || flush)) { 
        queue_vdata(pl->vplay, &item->vfi, &item->stamp);
        vbuf_queue = item->next;
        if(item->vfi.data)
            free(item->vfi.data);
        free(item);
        return 1;
    } // while
    return 0;
} // vbuf_queue_play
void vbuf_queue_add(player_t *pl, video_frame_info_t *vfi, struct timeval *stamp) {
    vpackets++;
    while(vfi && stamp) {
        struct vbuf_queue_item *item = vbuf_queue;
        int64_t now = GetCurrentTime();
        int64_t tm = ((int64_t)1000000ll)*stamp->tv_sec+stamp->tv_usec;
        if(item && tm < ((int64_t)1000000ll)*item->stamp.tv_sec+item->stamp.tv_usec-MAX_BUFF_DIFF_US) {
	    if(vfi->pts) {
		if (vbuf_queue->vfi.pts && (vbuf_queue->vfi.pts < vfi->pts)) {
//        	    syslog(LOG_INFO,"vbuf_queue_add: clear stamp (stamp out of order %lld)(but pts continues %d -> %d)", ((int64_t)1000000ll)*vbuf_queue->stamp.tv_sec+vbuf_queue->stamp.tv_usec - now, vbuf_queue->vfi.pts, vfi->pts);
		    tm = stamp->tv_sec = stamp->tv_usec = 0;
		} else {
        	    struct vbuf_queue_item *vitem = vbuf_queue;
        	    syslog(LOG_INFO,"vbuf_queue_add: flushed queue (stamp out of order %lld)(%d -> %d)", ((int64_t)1000000ll)*vbuf_queue->stamp.tv_sec+vbuf_queue->stamp.tv_usec - now, vbuf_queue->vfi.pts, vfi->pts);
        	    while (vitem) {
            		struct vbuf_queue_item *next = vitem->next;
            		if(vitem->vfi.data)
                	    free(vitem->vfi.data);
            		free(vitem);
            		vitem = next;
        	    } // while
        	    item = vbuf_queue = 0;
		} // if
	    } else {
//        	syslog(LOG_INFO,"vbuf_queue_add: clear stamp (stamp out of order %lld)(but pts not set %d -> %d)", ((int64_t)1000000ll)*vbuf_queue->stamp.tv_sec+vbuf_queue->stamp.tv_usec - now, vbuf_queue->vfi.pts, vfi->pts);
		tm = stamp->tv_sec = stamp->tv_usec = 0;
	    } // if
        } // if
        if(item || (tm - now) > MAX_BUFF_HOLD_US) { 
            while (item && item->next)
                item = item->next;
            uint8_t *data = malloc(vfi->data_len);
            if(!data) { // out of mem! stop shm queue processing and try again...
                syslog(LOG_INFO,"vbuf_queue_add: unable to create data %u",vfi->data_len);
                usleep(DELAY_ON_ERROR);
                continue;
            } // if
            if(item)
                item = item->next = malloc(sizeof(struct vbuf_queue_item));
            else 
                vbuf_queue = item = malloc(sizeof(struct vbuf_queue_item));
            if(item) { 
                item->next  = 0;
                item->vfi   = *vfi;
                memcpy (data, item->vfi.data, vfi->data_len);
                item->vfi.data = data;
                item->stamp = *stamp;
            } else { // out of mem! stop shm queue processing and try again...
                syslog(LOG_INFO,"vbuf_queue_add: unable to create item");
                free(data);
                usleep(DELAY_ON_ERROR);
                continue;
            } // if
        } else // we are late -> direct queue data 
            queue_vdata(pl->vplay, vfi, stamp);
        break;
    } // while
} // vbuf_queue_add

int abuf_queue_play(player_t *pl, int flush) {
    struct abuf_queue_item *item = abuf_queue;
    int64_t now = GetCurrentTime();
    int64_t tm = 0;
    if(!item)
        return 0;
    if(pl && pl->trickmode) {
        struct abuf_queue_item *aitem = abuf_queue;
        while (aitem) {
            struct abuf_queue_item *next = aitem->next;
            if(aitem->ai.data)
                free((uint8_t *)aitem->ai.data);
            free(aitem);
            aitem = next;
        } // while
        abuf_queue = 0;
        return 0;
    } // if
    if(pl->base_time && item->ai.pts && pl->base_pts)
        tm = pl->base_time + 1000000LL * (item->ai.pts - pl->base_pts) / 90000;
    if (!buf_queue_freeze && (((tm - now) < MAX_BUFF_HOLD_US) || ((tm - now) > MAX_BUFF_BLOC_US) || flush)) {
        if (item->type == ABUF_RAW)
            rawaflush(pl, &item->ai);
        else if (item->type == ABUF_AC3)
            ac3flush(pl, &item->ai);
        else
            aflush(pl, &item->ai);
        abuf_queue = item->next;
        if(item->ai.data)
            free((uint8_t *)item->ai.data);
        free(item);
        return 1;
    } // while
    return 0;
} // abuf_queue_play
void abuf_queue_add(player_t *pl, uint8_t const *adata, int len, int pts, int samplerate, uint64_t frame_nr, media_format_t format, int type) {
    apackets++;
    if(pl && pl->trickmode)
        return;
    while(adata && len) {
        struct abuf_queue_item *item = abuf_queue;
        uint8_t *data = malloc(len);
        if(!data) { // out of mem! stop shm queue processing and try again...
            syslog(LOG_INFO,"abuf_queue_add: unable to create data %u",len);
            usleep(DELAY_ON_ERROR);
            continue;
        } // if
        while (item && item->next) 
            item = item->next;
        if(item)
            item = item->next = malloc(sizeof(struct abuf_queue_item));
        else
            abuf_queue = item = malloc(sizeof(struct abuf_queue_item));
        if(item) { 
            item->next = 0;
            item->type = type;
            memcpy (data, adata, len);
            item->ai.data   = data;
            item->ai.len    = len;
            item->ai.pts    = pts;
            item->ai.format = format;
            item->ai.samplerate = samplerate;
            item->ai.frame_nr = frame_nr;

        } else { // out of mem! stop shm queue processing and try again...
            syslog(LOG_INFO,"abuf_queue_add: unable to create item");
            free(data);
            usleep(DELAY_ON_ERROR);
            continue;
        } // if
        break;
    } // while
} // abuf_queue_add
static void aqueue(void *player, audio_info_t const *ai) {
    if(ai)
        abuf_queue_add((player_t *)player, ai->data, ai->len, ai->pts, ai->samplerate, ai->frame_nr, ai->format, ABUF_DEF);
} // aqueue

void buf_queue_play(player_t *pl, int flush) {
    while (1) {
        while (abuf_queue_play(pl, flush) || vbuf_queue_play(pl, flush))
            ;
        int aitems = 0;
        struct abuf_queue_item *aitem = abuf_queue;
        while (aitem) {
            aitems++;
            aitem = aitem->next;
        }
        if (aitems>MAX_AUDIO_ITEMS) {
            usleep(DELAY_ON_ERROR);
            continue;
        } // while
        break;
    } // while
#if 0
    {
        int lv = 0, la = 0;
        int64_t now = GetCurrentTime();
        static int64_t last = 0;
        if (now-last < 1000000)
            return;
        last = now;
        struct vbuf_queue_item *vitem = vbuf_queue;
        while (vitem) {
            lv++;
            vitem = vitem->next;
        }
        struct abuf_queue_item *aitem = abuf_queue;
        while (aitem) {
            la++;
            aitem = aitem->next;
        }
        syslog(LOG_INFO, "processed (%d-%d) in queue (%d-%d) info (%lld - %d) (%lld - %d)",
	    vpackets, apackets, lv, la,
            vbuf_queue?((int64_t)1000000ll)*vbuf_queue->stamp.tv_sec+vbuf_queue->stamp.tv_usec - now:0,vbuf_queue?vbuf_queue->vfi.pts:0,
            abuf_queue?(pl->base_time + 1000000LL * (abuf_queue->ai.pts - pl->base_pts) / 90000)-now:0, abuf_queue?abuf_queue->ai.pts:0);
	vpackets = apackets =0;
    }
#endif
} // buf_queue_play
void buf_queue_clear(player_t *pl, bool flushaudio) {
    struct vbuf_queue_item *vitem = vbuf_queue;
    while (vitem) {
        struct vbuf_queue_item *next = vitem->next;
        if(vitem->vfi.data)
            free(vitem->vfi.data);
        free(vitem);
        vitem = next;
    } // while
    vbuf_queue = 0;
    struct abuf_queue_item *aitem = abuf_queue;
    while (aitem) {
        struct abuf_queue_item *next = aitem->next;
        if(aitem->ai.data)
            free((uint8_t *)aitem->ai.data);
        free(aitem);
        aitem = next;
    } // while
    abuf_queue = 0;
    buf_queue_freeze = 0;
    if(flushaudio) {
        audio_decoder_flush(pl->aplay);
    }
    pl->base_time = 0;
    pl->base_pts = 0;
} // buf_queue_clear
void buf_queue_restamp() {
    int64_t diff = GetCurrentTime()-buf_queue_freeze-MAX_BUFF_HOLD_US;
    struct vbuf_queue_item *item = vbuf_queue;
    if(!buf_queue_freeze) 
        return;
    while (item) {
        if (item->stamp.tv_sec+item->stamp.tv_usec) {
            int64_t t = ((int64_t)1000000ll)*item->stamp.tv_sec+item->stamp.tv_usec+diff;
            item->stamp.tv_sec  = t/1000000ll;
            item->stamp.tv_usec = t%1000000ll;
        } // if
        item = item->next; 
    } // while
    buf_queue_freeze = 0;
} // buf_queue_restamp

#endif /* USE_FAST_TRICKMODE */
/*-------------------------------------------------------------------------*/
player_t *new_player(ts_demux_t *td, hd_channel_t *channel, int n_player)
{
    printf ("\033[0;41m new_player \033[0m\n");
    player_t *pl;

    pl = (player_t*)malloc(sizeof(player_t));
    pl->vdmxbuf = malloc(VDMXBUFLEN);

    pl->vplay = new_video_player();
    pl->aplay = new_audio_player();

    open_video_player(pl->vplay);

    pl->base_time = 0;
    pl->base_pts = 0;

    pl->last_vformat = FORMAT_NONE;
    pl->aparser_feed = aparser_feed_none;
    pl->aparser_get_frame = aparser_get_frame_none;

    pl->current_aformat = FORMAT_NONE;

    pl->channel = channel;

    pl->td = new_ts_demux(pl->vdmxbuf);

    if (pl->td)
    {
        ts_demux_set_player(pl->td,
                            pl,
                            vflush,
#ifdef USE_FAST_TRICKMODE
                            aqueue,
#else
                            aflush,
#endif /* USE_FAST_TRICKMODE */
                            get_video_out_buffer);
    }

    pl->vfb = vfb_new();

    memset(&pl->vfi, 0, sizeof(video_frame_info_t));
    pl->video_generation = 0;
    pl->shm_player = &hda->player[n_player];
    pl->last_temp_ref = 0;
    pl->gop_temp_ref = 0;

    pl->last_frame_trickspeed = -1;
    pl->last_frame_i_only = 0;

    pl->seq_width = 0;
    pl->seq_height = 0;
    pl->seq_aspect_w = 0;
    pl->seq_aspect_h = 0;

    //Start by Klaus
    InitClock(&pl->clock);
    //End by Klaus

#ifdef USE_FAST_TRICKMODE    
    buf_queue_clear(pl, true);
#endif /* USE_FAST_TRICKMODE */

    return pl;
}
/*-------------------------------------------------------------------------*/
void delete_player(player_t *pl)
{
#ifdef USE_FAST_TRICKMODE    
    buf_queue_clear(pl, true);
#endif /* USE_FAST_TRICKMODE */
    delete_audio_player(pl->aplay);
    delete_video_player(pl->vplay);
    free(pl->vdmxbuf);
    free(pl);
}
/*-------------------------------------------------------------------------*/
// Play ES stored in vresult
#if 0
static void vflush(void *player, video_frame_info_t const *frame)
{
    player_t *pl = (player_t*)player;

//    vfb_put_frame(pl->vfb, frame);

    
}
#endif
/*-------------------------------------------------------------------------*/
// TODO: move to some proper struct
static uint64_t lastpts = 0;
static uint8_t  nrPacketsAfterPts = 0;
static const int ptsOffsets[3] = { -80000, 40000, 160000};

#if 0
static int play_buffered_vframes(player_t *pl)
{
    return 0;
}
#endif 

#ifdef DUMP_ES
FILE *xf=NULL;
#endif

void vflush(void *player, video_frame_info_t const *vfix)
{
    player_t *pl = (player_t*)player;
    // vbuf_t *vbuf; not used
    video_frame_info_t vfi;
    struct timeval timestamp;
    int res=0;

    vfi=*vfix;
#ifdef DUMP_ES
    if (!xf)
	    xf=fopen("dump.es","w");

//    printf("v: %c %lld\n", vfi.pic_type, vfi.temp_ref);
//    printf("vflush %i, %p\n",vfi.data_len,vfi.data);
//    hexdump(vfi.data,32);
    fwrite(vfi.data, vfi.data_len, 1, xf);
#endif
    res=1;

    if (vfi.vformat!=0 && (vfi.generation != pl->video_generation || vfi.vformat != pl->last_vformat))
    {
#ifdef USE_FAST_TRICKMODE
//        buf_queue_clear(pl, false);   //do not flush audio here
	syslog(LOG_INFO,"buf_queue_clear: %d<<%d %d<<%d", vfi.generation, pl->video_generation, vfi.vformat, pl->last_vformat);
        buf_queue_clear(pl, true);      //^^why?? Not flushing will result in pausing playback after seeking
#else
        //audio_decoder_flush(pl->aplay);   why neccessary when video stream changes?
#endif /* USE_FAST_TRICKMODE */
        reset_video_player(pl->vplay, &vfi, 0);

        if (vfi.vformat==FORMAT_H264) {
            // HACK: Need real format from NALs
            vfi.width=1920;
            vfi.height=1080;
            vfi.aspect_w=16;
            vfi.aspect_h=9;
        }   

        start_video_player(pl->vplay,
                   vfi.vformat, vfi.width, vfi.height,
                   0, 0, 0, 0,  // crop
                   0, vfi.aspect_w, vfi.aspect_h     // aspect
            );

        pl->video_generation = vfi.generation;
        pl->last_vformat = vfi.vformat;

        pl->last_width = vfi.width;
        pl->last_height = vfi.height;
        pl->last_aspect_w = vfi.aspect_w;
        pl->last_aspect_h = vfi.aspect_h;
    }

    //set aspect ratios in hdshm
    hda->source_aspect.w = vfi.aspect_w;
    hda->source_aspect.h = vfi.aspect_h;

    if (pl->vplay->playing && (vfi.width != pl->last_width ||
                   vfi.height != pl->last_height ||
                   vfi.aspect_w != pl->last_aspect_w ||
                   vfi.aspect_h != pl->last_aspect_h)) {

        set_aspect_ratio(pl->vplay, 0, vfi.width, vfi.height, vfi.aspect_w, vfi.aspect_h);

        pl->last_width = vfi.width;
        pl->last_height = vfi.height;
        pl->last_aspect_w = vfi.aspect_w;
        pl->last_aspect_h = vfi.aspect_h;
    }

    if (vfi.generation != pl->shm_player->data_generation || vfi.vformat == FORMAT_NONE)
    {
        printf (" TODO release buffer \n"); 
        //printf (" vfi.gen %u  data_gen %u  vfi.format == NONE %s\n", vfi.generation,  pl->shm_player->data_generation, vfi.vformat == FORMAT_NONE?"YES":"NONE");
#ifdef USE_FAST_TRICKMODE
	syslog(LOG_INFO,"buf_queue_clear: %d<<%d %d", vfi.generation, pl->shm_player->data_generation, vfi.vformat);
        buf_queue_clear(pl, true);
#else
        audio_decoder_flush(pl->aplay);
#endif /* USE_FAST_TRICKMODE */
//      reset_video_player(pl->vplay, &vfi, 0);
        return;

//      release_vbuffer(pl->vplay, vbuf);
    }
    else
    {
        int64_t now = GetCurrentTime();
        int64_t present_time = 0;
        //int64_t zero=0; not used

        // printf("base_time = %lld\n", pl->base_time / 100000);

        if (pl->last_frame_trickspeed != vfi.trickmode ||
            pl->last_frame_i_only != vfi.i_frames_only)
        {
            pl->last_frame_trickspeed = vfi.trickmode;
            pl->last_frame_i_only = vfi.i_frames_only;

            pl->trick_base_time = now;
            pl->trick_frame_ref = vfi.temp_ref - 1;
            pl->trick_frame_count = 1;

            printf("trickmode change\n");
        }

        if (vfi.trickmode)
        {
            printf (" trickmode \n");
            if (vfi.i_frames_only)
            {
                present_time = pl->trick_base_time
                    + (1000000LL / 25) * pl->trick_frame_count * vfi.trickmode; // TODO: other framerates
            }
            else
            {
                present_time = pl->trick_base_time
                    + (1000000LL / 25)
                    * (vfi.temp_ref - pl->trick_frame_ref) * vfi.trickmode; // TODO: other framerates
            }

            if (present_time < now)
            {
                present_time = now + 4 * (1000000LL / 25); // TODO: other framerates
                pl->trick_base_time = present_time;
                pl->trick_frame_ref = vfi.temp_ref;
                pl->trick_frame_count = 0;

                // printf("Was paused?\n");
            }
            else if ((pl->i_frames_only || vfi.pic_type == 'B') && present_time < now + 6000000LL) // Never sleep too long!
            {
                // printf("sleep\n");
                while (present_time >= now + 4 * (1000000LL / 25))
                {
                    usleep(20000);
                    now = GetCurrentTime();
                }
            }

            ++ pl->trick_frame_count;
        }
        else
        {
            // AV-Sync            

            //shift vpts (sync tuning) 
            static int num = 0;
            static int pts_shift = 0;
            if(++num%10 == 0) //do not access shm to frequently
            {
                //printf("-------pts_shift = %d-------\n", pl->shm_player->pts_shift);
                pts_shift = pl->shm_player->pts_shift;
            } 
            if(vfi.pts)
            { 
                vfi.pts -= (pl->shm_player->pts_shift) * (90000 / 1000); // subtract delay: late audio corresponds to early video
            }

            if (pl->shm_player->stc_valid && vfi.pts) //stc provided over shm
            {
                static int stc = 0; //local stc, must be smooth
                static bool out = false;
                static int num = 0;
                static int64_t lastTime = 0;
                const int maxdiff = 3600*5; //5 frames, 200ms
                const int num_frames = 10; 

                if (stc - pl->shm_player->stc > maxdiff || pl->shm_player->stc - stc > maxdiff)
                {
                    out = true;
                    num++;
                }
                else
                {
                    out = false;
                    num = 0;
                }

                //only correct when at least num_frames are out of sync
                if(out && num >= num_frames)
                {
                    //printf("-------stc = pl->shm_player->stc-------\n");
                    out = false;
                    num = 0;
                    stc = pl->shm_player->stc; //reset local stc
                }
                else
                {
                    //add system time diff to local stc
                    stc += ((now - lastTime) * 90000LL) / 1000000; 
                }
                lastTime = now;

                present_time = now + ((int)(vfi.pts - stc)) * 1000000LL / 90000;
                //printf("----------stc_valid = %d, stc = %d, pl->shm_player->stc = %d-----\n", (int)pl->shm_player->stc_valid, stc, pl->shm_player->stc);
            } 
            else if (pl->base_time && vfi.pts && now - pl->base_time < 2000000LL)
            {
                present_time = pl->base_time + (vfi.pts - pl->base_pts) * 1000000LL / 90000;
                //printf("---------present_time = %lld, vfi.pts = %d-----------\n", present_time, vfi.pts);
            }
            pl->latest_pts = vfi.pts;
        }

        if(present_time)
            nrPacketsAfterPts=0;
        else
            nrPacketsAfterPts++;

        /* interpolate pts if not present in packet*/ 
        static int64_t last_temp_ref = 0; 
        static int64_t last_present_time = 0;

        if(present_time && vfi.temp_ref && !vfi.still)
        {
            last_temp_ref = vfi.temp_ref;
            lastpts = present_time;
        }
        if( present_time==0 && lastpts && last_temp_ref && !vfi.still)
        {
            present_time = lastpts + (vfi.temp_ref - last_temp_ref) * 40000;
            //printf("interpolate pts, %lld %lld \n", vfi.temp_ref - last_temp_ref, present_time - last_present_time);
        }
        last_present_time = present_time;

        //printf("DIFF %lld %i.%06i \n", (present_time-now), (int)(present_time/ 1000000),(int)(present_time % 1000000));

        if (present_time > now + 12000000 && !vfi.still)
        {
            if (present_time > now + 14*1000*1000 ) { 
                if (present_time < now + 30*1000*1000) { // Better forward skip
                    printf("throw away pts diff %i (%u, %u)\n",(int)present_time-(int)now,vfi.pts,pl->base_pts);
                    return;
                }
                else
                    present_time=0;
            }
            else {
                printf("bound pts diff %i\n",(int)present_time-(int)now);
                present_time = now + 8000000;
            }
        }
        else if (present_time && present_time < now && !vfi.still)
        {
            printf("-------------video underflow-----------------\n");
            if (present_time<now-6*1000*1000 && present_time>now-30*1000*1000) { // Better reverse skip
                printf("throw away #2 pts diff %i\n",(int)(present_time-now));
//          present_time=0;
                return;
            }
            else
                present_time = now;
        }

//      printf("timestamp = %lld , PTS %f\n", present_time, (float)((unsigned int)vfi.pts)/90000);
        //lastpts = present_time;

        timestamp.tv_sec  = present_time / 1000000;
        timestamp.tv_usec = present_time % 1000000;

#if 0 // enable to display fps
        {
            static int64_t last_time = 0;
            static double av_frame_dur = 1.0 / 25.0;

            if (last_time == 0)
            {
                last_time = now;
            }
            else
            {
                double frame_dur = (now - last_time) / 1000000.0;

                av_frame_dur = (99.0 * av_frame_dur + frame_dur) / 100.0;

                if (av_frame_dur)
                {
                    printf("fps = %f\n", 1.0 / av_frame_dur);
                }
                last_time = now;
            }
        }
#endif
        /*if(vfi.still)
        {
            printf("------*****-------skip-------*****---------\n");
            //skip_video(pl->vplay, &timestamp);
        }*/
#ifdef USE_FAST_TRICKMODE        
	vbuf_queue_add(pl, &vfi, &timestamp);
#else
        queue_vdata(pl->vplay, &vfi, &timestamp);
#endif /* USE_FAST_TRICKMODE */
    }

    // return res; return void
}
/*-------------------------------------------------------------------------*/
static void adjust_base_pts(player_t *pl, int audio_pts)
{
    audio_player_t *ap        = pl->aplay;

    if (audio_pts && ap->last_samplerate)
    {
        int64_t         position;
        int64_t         tm;

        get_position_and_time(ap, &position, &tm);
        if (position)
        {
            pl->base_pts = audio_pts;
            pl->base_time = tm + ((int64_t)(ap->play_pos - position)) * 1000000 / ap->last_samplerate;
        }
    }
}
/*-------------------------------------------------------------------------*/
static void change_aformat(player_t *pl, audio_info_t const *ai)
{
    media_format_t format = ai->format;

    printf("change_aformat %#0x\n", format);

    // Close current parser.
    if (pl->current_aformat != FORMAT_NONE)
    {
        switch (pl->current_aformat)
        {
        case FORMAT_MPA:
            mpa_parser_close(&pl->mpa_parser);
            break;
        case FORMAT_AC3:
            ac3_parser_close(&pl->ac3_parser);
            break;
        default:
            // Unknown format.
            break;
        }
        pl->current_aformat = FORMAT_NONE;
    }

    // Open parser for format.
    pl->current_aformat = format;
    switch (format)
    {
    case FORMAT_MPA:
        mpa_parser_open(&pl->mpa_parser, pl->aplay);
        pl->aparser_feed = aparser_feed_mpa;
        pl->aparser_get_frame = aparser_get_frame_mpa;
        break;
    case FORMAT_AC3:
        ac3_parser_open(&pl->ac3_parser, pl->aplay);
        pl->aparser_feed = aparser_feed_ac3;
        pl->aparser_get_frame = aparser_get_frame_ac3;
        break;
    default:
        // Unknown format.
        pl->aparser_feed = aparser_feed_none;
        pl->aparser_get_frame = aparser_get_frame_none;
        pl->current_aformat = FORMAT_NONE;
    }
}
/*-------------------------------------------------------------------------*/
static void rawaflush(void *player, audio_info_t const *ai)
{
    printf("+++++++++++++++++rawaflush()+++++++++++++++++++\n");
    player_t *pl = (player_t*)player;
    audio_player_t *ap = pl->aplay;
    abuf_t *abuf;
    media_format_t format = ai->format;

    if (format != pl->current_aformat)
    {
        change_aformat(pl, ai);
    }

    adjust_base_pts(pl, ai->pts);

    abuf = get_abuffer(ap); //should not fail

    //hd_packet_af_data_t *packet = (hd_packet_af_data_t *)(ai->data - sizeof(hd_packet_af_data_t));
    abuf->length = ai->len;
    abuf->samplerate = ai->samplerate;
    abuf->sample_count = ai->len/4;
    abuf->media_format = FORMAT_RAWAUDIO;

    memcpy(abuf->data.buf, ai->data, ai->len);

    //----sync stuff------------
    int64_t position = 0ll;
    int64_t tm = 0ll;
    volatile hd_player_t *hd_pl = pl->shm_player; 
    get_position_and_time(ap, &position, &tm);
    int64_t delay =  ap->play_pos - position;
    int64_t current_frame = ai->frame_nr - delay;
    hd_pl->current_frame = current_frame;
    //--------------------------

    //TODO: implement a proper mechanism to report delay over hdshm (see below)
    /*int delay_diff = 5000;
    int frame_diff = 5000;
    static int64_t last_delay = 0ll;
    static int64_t last_current_frame = 0ll;
    if( delay - last_delay > delay_diff || last_delay - delay > delay_diff ||
        last_current_frame - current_frame > frame_diff || current_frame - last_current_frame > frame_diff)
    {
        printf("+++++++++++++++++delay = %lld+++++++++++++++\n", delay);
        printf("+++++++++++++++++last_delay = %lld+++++++++++++++\n", last_delay);
        printf("+++++++++++++++++ current_frame = %lld+++++++++++++++\n", current_frame);
        printf("+++++++++++++++++last_current_frame = %lld+++++++++++++++\n\n\n", last_current_frame);
    }
    last_delay = delay;
    last_current_frame = current_frame;

    int64_t sync_time = tm + GetClockTimeDiff(&pl->clock);
    int64_t correction = 10000 * current_frame - (abuf->samplerate / 100) * sync_time ;
    //printf("+++++correction = %lld, samplerate = %d, tm = %lld++++++\n", correction, abuf->samplerate, tm);
    unsigned long long correction_base = LongVal(hd_pl->correction_base_high, hd_pl->correction_base_low);
    //printf("+++++++++correction_base_high = %u+++&high = %0x+++++\n", hd_pl->correction_base_high, (uint) &hd_pl->correction_base_high);
    //printf("+++++++++correction_base_low = %u+++&high = %0x+++++\n", hd_pl->correction_base_low, (uint) &hd_pl->correction_base_low);
    //printf("++++++++++++++++++correction = %lld++++++++++++++++\n", correction);
    //printf("++++++++++++++++++correction_base = %lld++++++++++++++++\n", correction_base);
    //static int64_t last_correction_diff = 0;
    int64_t correction_diff  = correction + correction_base;
    int diff = 10000;
    if(correction_diff - last_correction_diff > diff || last_correction_diff - correction_diff > diff)
    {
        printf("+++delay = %lld++++correction_diff = %lld++++++++++++++++\n", delay, correction_diff);
        printf("++++++++++++++last_correction_diff = %lld++++++++++++++++\n", last_correction_diff);
    }    
    last_correction_diff = correction_diff;

    //printf("+++delay = %lld++++correction_diff = %lld++++++++++++++++\n", delay, correction_diff);
    hd_pl->correction_diff = correction_diff;
    hd_pl->pts = pl->base_pts; */ 
    //---------------------------------------------------------

    play_audio_frame(ap, abuf); // TODO: life-mode small delay

    put_abuffer(ap, abuf);
}
/*-------------------------------------------------------------------------*/
static void ac3flush(void *player, audio_info_t const *ai)
{
    //discard first 8 bytes
    int len = ai->len - 8;

    player_t *pl = (player_t*)player;
    audio_player_t *ap = pl->aplay;
    abuf_t *abuf;
    media_format_t format = ai->format;

    if (format != pl->current_aformat)
    {
        change_aformat(pl, ai);
    }

    adjust_base_pts(pl, ai->pts);

    abuf = get_abuffer(ap); //should not fail

    //hd_packet_af_data_t *packet = (hd_packet_af_data_t *)(ai->data - sizeof(hd_packet_af_data_t));
    abuf->length = len;
    abuf->samplerate = ai->samplerate;
    abuf->sample_count = len/4;
    abuf->media_format = FORMAT_AC3;

    memcpy(abuf->data.buf, ai->data + 8, len);

    //reorder bytes
    int i = 0;
    uint8_t t = 0;
    for (i = 0; i < len; i+=2)
    {
        t = (abuf->data.buf)[i];
        (abuf->data.buf)[i] = (abuf->data.buf)[i+1];
        (abuf->data.buf)[i+1] = t;
    }
    //printf("++reordered: [0] = %0x [1] = %0x [2] = %0x [3] = %0x++\n", ((int*) abuf->data.buf)[0], ((int*) abuf->data.buf)[1], ((int*) abuf->data.buf)[2], ((int*) abuf->data.buf)[3]);

    //----sync stuff------------
    int64_t position = 0ll;
    int64_t tm = 0ll;
    volatile hd_player_t *hd_pl = pl->shm_player; // warning
    get_position_and_time(ap, &position, &tm);
    int64_t delay =  ap->play_pos - position;
    int64_t current_frame = ai->frame_nr - delay;
    hd_pl->current_frame = current_frame;
    //--------------------------

    play_audio_frame(ap, abuf); 

    put_abuffer(ap, abuf);
}
/*-------------------------------------------------------------------------*/
static void aparser_feed_ac3(player_t *pl, uint8_t const *data, int len)
{
    ac3_parser_feed(&pl->ac3_parser, data, len);
}
/*-------------------------------------------------------------------------*/
static abuf_t *aparser_get_frame_ac3(player_t *pl)
{
    return ac3_parser_get_frame(&pl->ac3_parser);
}
/*-------------------------------------------------------------------------*/
static void aparser_feed_mpa(player_t *pl, uint8_t const *data, int len)
{
    mpa_parser_feed(&pl->mpa_parser, data, len);
}
/*-------------------------------------------------------------------------*/
static abuf_t *aparser_get_frame_mpa(player_t *pl)
{
    return mpa_parser_get_frame(&pl->mpa_parser);
}
/*-------------------------------------------------------------------------*/
static void aparser_feed_none(player_t *pl, uint8_t const *data, int len)
{
}
/*-------------------------------------------------------------------------*/
static abuf_t *aparser_get_frame_none(player_t *pl)
{
    return NULL;
}
/*-------------------------------------------------------------------------*/
static void aflush(void *player, audio_info_t const *ai)
{
    player_t *pl = (player_t*)player;
    audio_player_t *ap = pl->aplay;
    abuf_t *abuf;
    media_format_t format = ai->format;

    if (format != pl->current_aformat)
    {
        change_aformat(pl, ai);
    }

    pl->aparser_feed(pl, ai->data, ai->len);

    adjust_base_pts(pl, ai->pts);

    while ((abuf = pl->aparser_get_frame(pl)))
    {
        play_audio_frame(ap, abuf); // TODO: life-mode small delay

        put_abuffer(ap, abuf);
    }
}
/*-------------------------------------------------------------------------*/
static uint8_t *get_video_out_buffer(void *player)
{
    player_t *pl = (player_t*)player;

    return vfb_get_write_ptr(pl->vfb);
}
/*-------------------------------------------------------------------------*/
static unsigned int old_apid = 0; // FIXME
static unsigned int old_vpid = 0;

static void put_packet_ts_data(player_t *pl, hd_packet_ts_data_t const *packet)
{
    unsigned int apid = packet->apid;
    unsigned int vpid = packet->vpid;
    uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_ts_data_t);
    uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_ts_data_t);

    pl->td->vfi.generation = packet->generation;

    if(apid<=0x1FFF && vpid<=0x1FFF && apid >=0 && vpid >=1 && (apid != old_apid ||vpid != old_vpid)){
        printf("SET DEMUX: vpid: %x apid: %x\n", vpid, apid);
    if (vpid != old_vpid)
        pl->video_generation=-1; // Force video decoder flush

        old_apid = apid; old_vpid = vpid;
    ts_demux_set_pids(pl->td, vpid, apid);
    pl->td->synced = 0;
    if (vpid != old_vpid)
        pl->video_generation=-1; // Force video decoder flush
    }
    if(data_size>0){
             //printf("DEBUG: put_packet_ts_data: len: %i\n", data_size);
             ts_parse(pl->td,data,data_size);
    }
}
/*-------------------------------------------------------------------------*/
static void put_packet_af_data(player_t *pl, hd_packet_af_data_t const *packet)
{
    printf("--put_packet_af_data_mpa, sampleRate = %d, frame_nr = %lld--\n", packet->sampleRate, packet->frame_nr);
#ifdef USE_FAST_TRICKMODE
	abuf_queue_add(pl, 
			(uint8_t const *)packet + sizeof(hd_packet_af_data_t), 
			packet->header.packet_size - sizeof(hd_packet_af_data_t), 
			packet->pts, packet->sampleRate, packet->frame_nr, FORMAT_NONE, ABUF_RAW);
#else
    audio_info_t ai;
 
    ai.data = (uint8_t const *)packet + sizeof(hd_packet_af_data_t);
    ai.len = packet->header.packet_size - sizeof(hd_packet_af_data_t);
    ai.pts = packet->pts;
    ai.format = FORMAT_NONE;
    ai.samplerate = packet->sampleRate; 
    ai.frame_nr = packet->frame_nr;

    rawaflush(pl, &ai);
#endif /* USE_FAST_TRICKMODE */    
}
/*-------------------------------------------------------------------------*/
static void put_packet_es_data(player_t *pl, hd_packet_es_data_t const *packet)
{
    if ((packet->stream_id & 0xF0) == 0xE0)
    {
        put_packet_es_data_mpv(pl, packet);
    }
    else
    {
        put_packet_es_data_mpa(pl, packet);
    }
}
/*-------------------------------------------------------------------------*/
static void put_packet_es_data_mpa(player_t *pl, hd_packet_es_data_t const *packet)
{
#ifdef USE_FAST_TRICKMODE
	abuf_queue_add(pl, 
			(uint8_t const *)packet + sizeof(hd_packet_es_data_t), 
			packet->header.packet_size - sizeof(hd_packet_es_data_t), 
			packet->timestamp, 0, 0, packet->stream_id == 0xBD ? FORMAT_AC3 : FORMAT_MPA, ABUF_DEF);
#else
    audio_info_t ai;

    ai.data = (uint8_t const *)packet + sizeof(hd_packet_es_data_t);
    ai.len = packet->header.packet_size - sizeof(hd_packet_es_data_t);
    ai.pts = packet->timestamp;
    ai.format = packet->stream_id == 0xBD ? FORMAT_AC3 : FORMAT_MPA; 
    ai.samplerate = 0; //not used
    ai.frame_nr = 0; //not used

    aflush(pl, &ai);
#endif /* USE_FAST_TRICKMODE */
}
/*-------------------------------------------------------------------------*/
static void put_packet_ac3_data_mpa(player_t *pl, hd_packet_af_data_t const *packet)
{ 
    //printf("--put_packet_ac3_data_mpa, >sampleRate = %d, frame_nr = %lld--\n", packet->sampleRate, packet->frame_nr);
#ifdef USE_FAST_TRICKMODE
	abuf_queue_add(pl, 
			(uint8_t const *)packet + sizeof(hd_packet_af_data_t), 
			packet->header.packet_size - sizeof(hd_packet_af_data_t), 
			packet->pts, packet->sampleRate, packet->frame_nr, FORMAT_AC3, ABUF_AC3);
#else
    audio_info_t ai;

    ai.data = (uint8_t const *)packet + sizeof(hd_packet_af_data_t);
    ai.len = packet->header.packet_size - sizeof(hd_packet_af_data_t);
    ai.pts = packet->pts; 
    ai.format = FORMAT_AC3;
    ai.samplerate = packet->sampleRate;
    ai.frame_nr = packet->frame_nr;

    ac3flush(pl, &ai);
#endif /* USE_FAST_TRICKMODE */
}
/*-------------------------------------------------------------------------*/
static inline uint8_t const *find_next_start_code(uint8_t const *d,
                                                  uint8_t const *e)
{
    for ( ; d != e; ++d)
    {
        if (d[0] == 0 && d[1] == 0 && d[2] == 1)
        {
            return d;
        }
    }
    return NULL;
}
/*-------------------------------------------------------------------------*/
static uint8_t const *parse_group_start(player_t *pl, uint8_t const *d, uint8_t const *e)
{
    // printf("pv: group_start\n");

    pl->gop_temp_ref = pl->last_temp_ref + 1;
    return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
static uint8_t const *parse_seq_ext(uint8_t const *d, uint8_t const *e)
{
    // printf("pv: seq_ext\n");

    return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
static uint8_t const *parse_user_data(uint8_t const *d, uint8_t const *e)
{
    //printf("pv: user_data\n");

    return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
static uint8_t const *parse_seq_header(player_t *pl, uint8_t const *d, uint8_t const *e)
{
    int aspect_info;

    pl->seq_width = (d[4] << 4) | (d[5] >> 4);
    pl->seq_height = ((d[5] << 8) & 0xF00) + d[6];
    aspect_info = d[7] >> 4;

    switch(aspect_info)
    {
    case 3:
        pl->seq_aspect_w = 16;
        pl->seq_aspect_h = 9;
        break;
    case 4:
        pl->seq_aspect_w = 221;  // 2.21 : 1
        pl->seq_aspect_h = 100;
        break;
    default:
        pl->seq_aspect_w = 4;
        pl->seq_aspect_h = 3;
    }

    // find here also: frame_rate_code
    return find_next_start_code(d + 12, e);
}
/*-------------------------------------------------------------------------*/
static int parse_video(player_t *pl, uint8_t const *data, size_t length, video_frame_info_t *vfi)
{
    uint8_t const *end;

    if (length < 4 || data[0] != 0 || data[1] != 0 || data[2] != 1
        || (data[3] != 0 && data[3] != 9 && data[3] < 0xB0))
    {
        return 0;
    }

    end =  data + (length - 4);

    while (data + 64 <= end)
    {
        switch (data[3])
        {
        case 0:
            {
                char type;
                switch ((data[5] >> 3) & 7)
                {
                case 1:
                    type = 'I';
                    break;
                case 2:
                    type = 'P';
                    break;
                case 3:
                    type = 'B';
                    break;
                default:
                    type = 'U';
                }
                vfi->pic_type = type;
                vfi->temp_ref = pl->gop_temp_ref + (data[4] << 2) + (data[5] >> 6);
                if (vfi->temp_ref > pl->last_temp_ref)
                {
                    pl->last_temp_ref = vfi->temp_ref;
                }
            }

	    if (current_vformat != FORMAT_MPV ) {
	        current_vformat=FORMAT_MPV;
		printf("change to MPV (00 00 01 00 %02X %02X)\n",data[4],data[5]);
	    }
            return 1;
        case 9:
            {
                int prim_pic_type = data[4] >> 5;
                char type = 'U';
                switch (prim_pic_type) {
                case 0:
                case 3:
                case 5:
                    type = 'I';
                    break;
                case 1:
                case 4:
                case 6:
                    type = 'P';
                    break;
                case 2:
                case 7:
                    type = 'B';
                    break;
                }
                vfi->pic_type = type;
	    if (current_vformat != FORMAT_H264 && data[5]==0 && type=='B' ) {
	        current_vformat=FORMAT_H264;
		printf("change to H264 (00 00 01 09 %02X %02X)\n",data[4],data[5]);
	    }
            }
            return 1;

        case 0xB3:
            data = parse_seq_header(pl, data, end);
            break;
        case 0xB5:
            data = parse_seq_ext(data, end);
            break;
        case 0xB8:
            data = parse_group_start(pl, data, end);
            break;
        case 0xB2:
            data = parse_user_data(data, end);
            break;
        default:
            printf("pv: %02X\n", data[3]);
            //sleep(3); // FIXME
            return 0;
        }

        if (!data)
        {
            return 0;
        }
    }

    // parse error.uint32_t
    return 1;
}
/*-------------------------------------------------------------------------*/
static void put_packet_es_data_mpv(player_t *pl, hd_packet_es_data_t const *packet)
{
    uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_es_data_t);
    uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_es_data_t);

    pl->vfi.generation = packet->generation;
    pl->vfi.vformat = current_vformat;  //send no FORMAT_NONE packet to avoid second flush of audio decoder

    video_frame_info_t new_vfi; //get pic_type and temp_ref of next frame

    if (pl->vfi.data_len && parse_video(pl, data, data_size, &new_vfi))
    { 
        vflush(pl, &pl->vfi); // 2
        pl->vfi.data = NULL;
        pl->vfi.pts = packet->timestamp;
    }

    if (!pl->vfi.data)
    { 
        pl->vfi.pic_type =  new_vfi.pic_type;
        pl->vfi.temp_ref = new_vfi.temp_ref;
        pl->vfi.vformat = current_vformat;
        pl->vfi.data_len = 0;
        pl->vfi.data = vfb_get_write_ptr(pl->vfb);
        pl->vfi.width = pl->seq_width;
        pl->vfi.height = pl->seq_height;
        pl->vfi.aspect_w = pl->seq_aspect_w;
        pl->vfi.aspect_h = pl->seq_aspect_h;
    }
    if (pl->vfi.data_len+data_size>(VFB_MEM_SIZE + 2 * VFB_MEM_MARGIN)) // avoid overflow
	    pl->vfi.data_len=0;
    memcpy(pl->vfi.data + pl->vfi.data_len, data, data_size); 
    pl->vfi.data_len += data_size;
    pl->vfi.trickmode = pl->trickmode;
    pl->vfi.i_frames_only = pl->i_frames_only;
    pl->vfi.still = packet->still_frame;
}
/*-------------------------------------------------------------------------*/
static void packet_trickmode(player_t *pl, hd_packet_trickmode_t const *packet)
{
    pl->trickmode = packet->trickspeed;
    pl->i_frames_only = packet->i_frames_only;
    printf("trickmode (%d, %d)\n", packet->trickspeed, packet->i_frames_only);
}
/*-------------------------------------------------------------------------*/
static void packet_off(player_t *pl, hd_packet_off_t const *packet)
{
    printf("Stop!\n");

    if (!pl->vfi.data)
    {
        pl->vfi.data = vfb_get_write_ptr(pl->vfb);
        pl->vfi.data_len = 4;
    }

    // Send a dummy frame.
    pl->vfi.trickmode = 1; // This will flush the buffer.
    pl->vfi.vformat = FORMAT_NONE;
    pl->vfi.width = 0;
    pl->vfi.height = 0;
    pl->vfi.aspect_w = 0;
    pl->vfi.aspect_h = 0;
    pl->vfi.still = false;
    vflush(pl, &pl->vfi); // 1
}
//Start by Klaus
/*-------------------------------------------------------------------------*/
void UpdateClock(player_t *pl, SyncClock_t *clock)
{
    hd_player_t hd_pl = hda->player[0];

    unsigned int time_high = hd_pl.clock_high;  //Order dependancy!
    MemB(); //TODO : only use when changed!
    unsigned int time_low = hd_pl.clock_low;
    MemB();
    unsigned long long time = LongVal(time_high, time_low);

    SetClock(time, clock);


    hd_pl.clock_low = 0;      //Order dependancy!
    MemB();
    hd_pl.clock_low =  LowWord(time);
    MemB();
    hd_pl.clock_high = HighWord(time);
}
//End by Klaus
/*-------------------------------------------------------------------------*/
static enum {
    MODE_RMM = 0,
    MODE_ERF,
} gState = MODE_RMM;
int last_seq;

int run_player(player_t *pl)
{
    //Start by Klaus
    UpdateClock(pl, &pl->clock);
    //End by Klaus
    hd_packet_header_t const *packet_header;
    void *rcv_buffer;
    int packet_size;
    int result = 1;

#ifdef USE_FAST_TRICKMODE
    buf_queue_play(pl,0);
#endif /* USE_FAST_TRICKMODE */
    if (hd_channel_read_start(pl->channel, &rcv_buffer, &packet_size, 0))
    {
        packet_header = (hd_packet_header_t *)rcv_buffer;
        if(MODE_RMM == gState) { // Use orig player
            switch (packet_header->type)
            {
            case HD_PACKET_TS_DATA:
#ifdef DEBUG
                if (packet_header->seq_nr!=last_seq+1) {
                    printf("SEQ ERR %x %x\n",packet_header->seq_nr,last_seq+1);

                }
                last_seq=packet_header->seq_nr;
#endif
                put_packet_ts_data(pl, (hd_packet_ts_data_t const *)packet_header);
                break;
            case HD_PACKET_ES_DATA: 
                //printf("-----run_player:HD_PACKET_ES_DATA------------\n");
                put_packet_es_data(pl, (hd_packet_es_data_t const *)packet_header);
                break;
            case HD_PACKET_TRICKMODE:
                //printf("-----run_player:HD_PACKET_TRICK_MODE------------\n");
#ifdef USE_FAST_TRICKMODE
                buf_queue_restamp();
#endif /* USE_FAST_TRICKMODE */
                packet_trickmode(pl, (hd_packet_trickmode_t const *)packet_header);
                break;
            case HD_PACKET_AF_DATA:
                //printf("-----run_player:HD_PACKET_AF_DATA------------\n");
                put_packet_af_data(pl, (hd_packet_af_data_t const *)packet_header);
                break;
            case HD_PACKET_AC3_DATA:
                //printf("-----run_player:HD_PACKET_AC3_DATA------------\n");
                put_packet_ac3_data_mpa(pl, (hd_packet_af_data_t const *)packet_header);
                break; 
#ifdef USE_FAST_TRICKMODE
            case HD_PACKET_CLEAR:
                syslog(LOG_INFO,"HD_PACKET_CLEAR");
                buf_queue_clear(pl, true);
                break;
            case HD_PACKET_FREEZE:
                syslog(LOG_INFO,"HD_PACKET_FREEZE");
                buf_queue_freeze = GetCurrentTime();
                break;
#endif /* USE_FAST_TRICKMODE */
#ifndef DISABLE_ERF
            case HD_PACKET_ERF_INIT:
                syslog(LOG_INFO, "HD_PACKET_ERF_INIT");
                if(sizeof(hd_packet_erf_init_t) != packet_size) {
                    syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_ERF_INIT packet size: %d (%d)", packet_size, sizeof(hd_packet_erf_init_t));
                    break;
                } // if
                gState = MODE_ERF;
                if(pl->vplay) 
                    delete_video_player(pl->vplay);
                pl->vplay = 0;
                if(pl->aplay)
                    delete_audio_player(pl->aplay);
                pl->aplay = 0;
                erf_init((hd_packet_erf_init_t const *)packet_header);   
                syslog(LOG_INFO, "HD_PACKET_ERF_INIT DONE");
                break;
#endif                
            case HD_PACKET_OFF:
                packet_off(pl, (hd_packet_off_t const *)packet_header);
                break;
            case HD_PACKET_ERF_PLAY:
            case HD_PACKET_ERF_CMD:
            case HD_PACKET_ERF_DONE:
                syslog(LOG_WARNING, "WARN: Packet not supported in normal mode: %d", packet_header->type);
                break;
            default:
                syslog(LOG_WARNING, "WARN: Unknown packet type: %d", packet_header->type);
            }
        } else if (MODE_ERF == gState) { // Use file player
#ifndef DISABLE_ERF
            switch (packet_header->type)
            {
            case HD_PACKET_ERF_PLAY:
                syslog(LOG_INFO, "HD_PACKET_ERF_PLAY");
                if(sizeof(hd_packet_erf_play_t) != packet_size) {
                    syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_ERF_PLAY packet size: %d (%d)", packet_size, sizeof(hd_packet_erf_play_t));
                    break;
                } // if
                erf_play((hd_packet_erf_play_t const *)packet_header);
                syslog(LOG_INFO, "HD_PACKET_ERF_PLAY DONE");
                break;
            case HD_PACKET_ERF_CMD:
                syslog(LOG_INFO, "HD_PACKET_ERF_CMD");
                if(sizeof(hd_packet_erf_cmd_t) != packet_size) {
                    syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_ERF_CMD packet size: %d (%d)", packet_size, sizeof(hd_packet_erf_cmd_t));
                    break;
                } // if
                erf_cmd((hd_packet_erf_cmd_t const *)packet_header);
                syslog(LOG_INFO, "HD_PACKET_ERF_CMD DONE");
                break;
            case HD_PACKET_ERF_DONE:
                syslog(LOG_INFO, "HD_PACKET_ERF_DONE");
                if(sizeof(hd_packet_erf_done_t) != packet_size) {
                    syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_ERF_DONE packet size: %d (%d)", packet_size, sizeof(hd_packet_erf_done_t));
                    break;
                } // if
                gState = MODE_RMM;
                erf_done((hd_packet_erf_done_t const *)packet_header);
                if(!pl->vplay)
                    pl->vplay = new_video_player();
                if(!pl->aplay)
                    pl->aplay = new_audio_player();
                syslog(LOG_INFO, "HD_PACKET_ERF_DONE DONE");
                break;
            case HD_PACKET_TS_DATA:
            case HD_PACKET_ES_DATA:
            case HD_PACKET_TRICKMODE:
            case HD_PACKET_AF_DATA:
            case HD_PACKET_OFF:
            case HD_PACKET_ERF_INIT:
                syslog(LOG_WARNING, "WARN: Packet not supported in erf mode: %d", packet_header->type);
                break;
            default:
                syslog(LOG_WARNING, "WARN: Unknown packet type: %d", packet_header->type);
            }
#endif                
        } // if
        hd_channel_read_finish(pl->channel);
        result = 2;
    }

    const int max_delay = 10000000;
    //printf("------------pl->latest_pts = %d, pl->base_pts = %d--------\n", pl->latest_pts, pl->base_pts);
    int video_delay = (pl->vfi.pts - pl->base_pts) * 1000000LL / 90000;
    if (video_delay < 0 || video_delay > max_delay)
    {
        video_delay = 0;
    }
    pl->shm_player->video_delay = video_delay; //set videodelay for polling

    return result;
}
