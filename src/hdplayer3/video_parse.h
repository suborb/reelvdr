#ifndef _VIDEO_PARSE_H
#define _VIDEO_PARSE_H

#include "types.h"

typedef struct {
        u8 pic_type;
        int width, height;
        int aspect_w, aspect_h;
	int framerate_nom, framerate_div;
} video_attr_t;


int video_parse(video_attr_t *vat, u8 *data, int length);
int video_h264_parse(video_attr_t *vat, u8 *data, int length);

#endif
