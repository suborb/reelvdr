// vfb.c

#include "vfb.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#define VFB_ASSERT REEL_ASSERT

#define MIN_FREE_SPACE (256 * 1024)

void vfb_delete(video_frame_buffer_t *vfb)
{
    if (vfb)
        free(vfb);
}

video_frame_buffer_t *vfb_new(void)
{
    video_frame_buffer_t *vfb = malloc(sizeof(video_frame_buffer_t));

    vfb->used_frames = 0;
    vfb->frame_used_begin = 0;
    vfb->frame_used_end = 0;
    vfb->used_mem = 0;
    vfb->mem_pos = 0;

    return vfb;
}

void vfb_put_frame(video_frame_buffer_t *self, video_frame_info_t const *vfi)
{
    VFB_ASSERT(self->used_frames < VFB_MAX_FRAMES);

    self->frames[self->frame_used_end] = *vfi;

    ++ self->used_frames;
    ++ self->frame_used_end;
    if (self->frame_used_end == VFB_MAX_FRAMES)
    {
        self->frame_used_end = 0;
    }
    self->used_mem += vfi->data_len;
    self->mem_pos += vfi->data_len;
    if (self->mem_pos > VFB_MEM_SIZE + VFB_MEM_MARGIN)
    {
        self->mem_pos = 0;
    }
}

int vfb_get_frame(video_frame_buffer_t  *self,
                  video_player_t        *vplay,
                  vbuf_t               **vbuf,
                  video_frame_info_t    *vfi)
{
    // int n;
    video_frame_info_t    vfi1;
    /*if (self->used_frames >= VFB_MAX_FRAMES)
    {
        printf("vfb to many frames\n");
    }

    if (self->used_mem > VFB_MEM_SIZE)
    {
        printf("vfb out of mem\n");
    }*/

    if (self->used_frames < 2)
    {
        return 0;
    }

    if (self->used_frames >= VFB_MAX_FRAMES ||
        self->used_mem > VFB_MEM_SIZE)
    {
        // block until a vbuffer is available.
        *vbuf = get_vbuffer(vplay);
    }
    else
    {
        *vbuf = get_vbuffer_nb(vplay);
    }

    if (!*vbuf)
    {
        // printf("no vbuf!!\n");
        return 0;
    }

    VFB_ASSERT((*vbuf)->max_length == MAX_VBUF_LEN);


    *vfi = self->frames[self->frame_used_begin++];

    // GA: look ahead to get newest PTS
    vfi1=self->frames[(self->frame_used_begin)%VFB_MAX_FRAMES];
    if (vfi->frame_start && !vfi1.frame_start) {
	    printf("forward PTS %f instead of %f\n",(float)((unsigned int)vfi1.pts)/90000, (float)((unsigned int)vfi->pts)/90000);
	    vfi->pts=vfi1.pts;
    }

    if (self->frame_used_begin == VFB_MAX_FRAMES)
    {
        self->frame_used_begin = 0;
    }
    -- self->used_frames;
    self->used_mem -= vfi->data_len;
    printf("vfi->data_len = %d, start %i\n", vfi->data_len, vfi->frame_start);
    memcpy((*vbuf)->data, vfi->data, vfi->data_len);
    (*vbuf)->length = vfi->data_len;
    (*vbuf)->frame_start = vfi->frame_start;
    return 1;
}

uint8_t *vfb_get_write_ptr(video_frame_buffer_t *self)
{
    return self->mem + self->mem_pos;
}
