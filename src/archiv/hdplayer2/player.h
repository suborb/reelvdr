#ifndef _INCLUDE_PLAYER_H
#define _INCLUDE_PLAYER_H

#include "vfb.h"
#include "ts_demux.h"
#include "player_dvb.h"

#include "ac3_parser_types.h"
#include "mpa_parser_types.h"

#include "channel.h"
#include <hdshm_user_structs.h>

extern hd_data_t volatile *hda;

//Start by Klaus
#define CLOCK_RESOLUTION 1000LL

struct SyncClock_s
{
    unsigned long long timediff_;
};

typedef struct SyncClock_s SyncClock_t;

static inline void InitClock(SyncClock_t *clock)
{
    clock->timediff_ = 0LL;
}

static inline unsigned long long GetSysTime()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (unsigned long long)(tv.tv_sec) * 1000000 + tv.tv_usec;
}

static inline void SetClock(unsigned long long time, SyncClock_t *clock)
{
    static long long counter = 0;
    if (++counter%1000 == 0)
    {
        //printf("---SyncClock:SetClock, time = %0llx, systime = %0llx, clock->timediff_ = %0llx-----\n", time, GetSysTime(),  clock->timediff_);
    }
    long long correction = time - (GetSysTime() + clock->timediff_);
    //printf("---------SyncClock:SetClock, correction = %0llx-----------\n",  correction);
    if (correction > CLOCK_RESOLUTION)
    {
        clock->timediff_ += correction - CLOCK_RESOLUTION; //TODO: Kleine Schritte
        //printf("---------SyncClock:SetClock, timediff = %0llx-----------\n",  clock->timediff_);
    }
}

static inline unsigned long long GetClock(SyncClock_t *clock)
{
    unsigned long long time = GetSysTime() + clock->timediff_;

    static long long counter = 0;
    if (++counter%1000 == 0)
    {
        //printf("---------SyncClock:GetClock, time = %0llx-----------\n",  time);
    }
    return time;
    return clock->timediff_;
}

static inline unsigned long long GetClockTimeDiff(SyncClock_t *clock)
{
    //printf("---------SyncClock:GetClockTimeDiff = %0llx-----------\n",  clock->timediff_);
    return clock->timediff_;
}
//End by Klaus

struct player_s
{
    video_player_t *vplay;
    audio_player_t *aplay;
    dvb_player_t *dvbp;

    ts_demux_t *td;

    video_frame_buffer_t *vfb;

    media_format_t last_vformat;
    int last_width;
    int last_height;
    int last_aspect_w;
    int last_aspect_h;

    media_format_t current_aformat;

    mpa_parser_t mpa_parser;
    ac3_parser_t ac3_parser;

    void (*aparser_feed)(struct player_s *pl, uint8_t const *data, int len);
    abuf_t *(*aparser_get_frame)(struct player_s *pl);

    int64_t base_time; // AV-Sync base values.
    int     base_pts;  //

    //start by Klaus
    int latest_pts;
    //End by Klaus

    hd_channel_t *channel;

    uint8_t *vdmxbuf;

    video_frame_info_t vfi;
    uint32_t video_generation;

    hd_player_t volatile *shm_player;

    uint64_t last_temp_ref, gop_temp_ref;

    int seq_width, seq_height;
    int seq_aspect_w, seq_aspect_h;

    int trickmode, i_frames_only;

    int last_frame_trickspeed, last_frame_i_only, trick_frame_ref;
    int64_t trick_base_time, trick_frame_count;

    //Start by Klaus
    SyncClock_t clock;
    //End by Klaus
};

typedef struct player_s player_t;

player_t *new_player(ts_demux_t *td, hd_channel_t *channel, int n_player);
void delete_player(player_t *pl);
int run_player(player_t *pl);

#endif
