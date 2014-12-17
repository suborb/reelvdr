#ifndef _INCLUDE_OSD_H
#define _INCLUDE_OSD_H

#include "channel.h"

struct osd_s
{
    int fd; // fd for framebuffer-device
    int bpp;
    int width;
    int height;
    unsigned char * data; // fb mmaped
    unsigned char * buffer;
    unsigned char * savedRegion;
    int xine_osd;
    int vdr_osd;
};

typedef struct osd_s osd_t;

int start_thread();
osd_t *new_osd(hd_channel_t *channel);
void delete_osd();
int run_osd();
int run_xine_osd();

#endif
