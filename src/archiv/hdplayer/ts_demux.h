#ifndef _INCLUDE_TS_DEMUX_H
#define _INCLUDE_TS_DEMUX_H

#include <stdlib.h>
#include <stdio.h>

#include "vfb.h"

//typedef unsigned char u08_t;

typedef struct
{
    int apid, vpid;

    int aspect;

    int resolution;
    // int vbuf_index;
    int synced;
    int video_synced;
    int audio_synced;
    int pic_type;
    media_format_t  tmp_aformat;
    audio_info_t ai;
    video_frame_info_t vfi;
    // int pts_synched;
    int frame_start;

    struct timeval tb; // timebase ref for video
    // int pts_offset; // a-v
    // int av_sync_state;

    u08_t *vdmxbuf;
    int vdmxbuflen;
    int vdmxbuflen_max;
    u08_t *admxbuf;
    int admxbuflen;
    int admxbuflen_max;

    video_flush_func video_flush;
    audio_flush_func audio_flush;
    get_video_out_buffer_func get_video_out_buffer;
    void *player;
} ts_demux_t;


ts_demux_t *new_ts_demux(uint8_t *vdmxbuf);
void delete_ts_demux(ts_demux_t *td);
void ts_demux_set_pids(ts_demux_t *td, int vpid, int apid);
void ts_demux_set_player(ts_demux_t *self,
                         void *player,
                         video_flush_func video_flush,
                         audio_flush_func audio_flush,
                         get_video_out_buffer_func get_video_out_buffer);
void ts_reset(ts_demux_t *td);
int ts_parse(ts_demux_t *td, u08_t const *data, int len);
int parsevpes(ts_demux_t *td, u08_t const *data, int len);
int parseapes(ts_demux_t *td, uint8_t const *data, size_t len);
void store_pts(ts_demux_t *td, u08_t const *x, int type);
void store_adata(ts_demux_t *td, uint8_t const *x, size_t len);
int FindPacketHeader(const u08_t *Data, int s, int l);
uint8_t const *findMarker(uint8_t const *Data, int Count, int Offset, unsigned char *PictureType);
int get_e0_length(uint8_t const  *p);
void hexdump(uint8_t const *x, int len);

int parse_pes(uint8_t const **pes_packet,        // in/out (will be advanced)
              size_t         *pes_packet_length, // in/out (will be advanced)
              int            *pts,               // out pts (0 = no pts)
              int            *stream_id,         // out
              int            *substream_id,      // out (valid if stream_id == 0xBD)
              uint8_t const **es_ptr,            // out
              size_t         *es_length          // out
             );
#endif
