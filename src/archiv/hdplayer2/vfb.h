// vfb.h

#ifndef VFB_H_INCLUDED
#define VFB_H_INCLUDED

#include "player_ll.h"

#define VFB_MAX_FRAMES 512
#define VFB_MEM_SIZE   (4 * 1024 * 1024)

#define MAX_VBUF_LEN (32 * 1024)

#define VFB_MEM_MARGIN MAX_VBUF_LEN

struct video_frame_buffer_s
{
    int used_frames;
    int frame_used_begin, frame_used_end;

    int used_mem, mem_pos;

    video_frame_info_t frames[VFB_MAX_FRAMES];
    uint8_t mem[VFB_MEM_SIZE + 2 * VFB_MEM_MARGIN];
};

typedef struct video_frame_buffer_s video_frame_buffer_t;

void vfb_delete(video_frame_buffer_t *vfb);

video_frame_buffer_t *vfb_new(void);

void vfb_put_frame(video_frame_buffer_t *self, video_frame_info_t const *vfi);

int vfb_get_frame(video_frame_buffer_t  *self,
                  video_player_t        *vplay,
                  vbuf_t               **vbuf,
                  video_frame_info_t    *vfi);

uint8_t *vfb_get_write_ptr(video_frame_buffer_t *self);

#endif // VFB_H_INCLUDED
