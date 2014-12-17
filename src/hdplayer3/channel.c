// channel.c

#include "channel.h"

#include <stdio.h>

hd_channel_t *ch_stream1;
hd_channel_t *ch_stream1_info;

hd_channel_t *ch_stream2;
hd_channel_t *ch_stream2_info;

hd_channel_t *ch_osd;
hd_channel_t *ch_xine_osd;

int create_channels(void)
{
    ch_stream1 = (hd_channel_t*)hd_get_area(HDCH_STREAM1);
    
    if(!ch_stream1)	
	    ch_stream1 = hd_channel_create(HDCH_STREAM1, HDCH_STREAM1_SIZE, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_stream1, 1);
    
    if (!ch_stream1)
    {
        fprintf(stderr, "Error opening HDCH_STREAM1\n");
        return 0;
    }

    ch_stream1_info = (hd_channel_t*)hd_get_area(HDCH_STREAM1_INFO);
    
    if(!ch_stream1_info)	
        ch_stream1_info = hd_channel_create(HDCH_STREAM1_INFO, HDCH_STREAM1_INFO_SIZE, HD_CHANNEL_WRITE);
    else
        hd_channel_invalidate(ch_stream1_info, 1);
    
    if (!ch_stream1_info)
    {
        fprintf(stderr, "Error opening HDCH_STREAM1_INFO\n");
        return 0;
    }

    ch_stream2 = (hd_channel_t*)hd_get_area(HDCH_STREAM2);
    
    if(!ch_stream2)	
        ch_stream2 = hd_channel_create(HDCH_STREAM2, HDCH_STREAM2_SIZE, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_stream2, 1);
    
    if (!ch_stream2)
    {
        fprintf(stderr, "Error opening HDCH_STREAM2\n");
        return 0;
    }

    ch_stream2_info = (hd_channel_t*)hd_get_area(HDCH_STREAM2_INFO);
    
    if(!ch_stream2_info)	
        ch_stream2_info = hd_channel_create(HDCH_STREAM2_INFO, HDCH_STREAM2_INFO_SIZE, HD_CHANNEL_WRITE);
    else
        hd_channel_invalidate(ch_stream2_info, 1);
    
    if (!ch_stream2_info)
    {
        fprintf(stderr, "Error opening HDCH_STREAM2_INFO\n");
        return 0;
    }

    ch_osd = (hd_channel_t*)hd_get_area(HDCH_OSD);

    if(!ch_osd)
	    ch_osd = hd_channel_create(HDCH_OSD, HDCH_OSD_SIZE, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_osd, 1);

    if (!ch_osd)
    {
        fprintf(stderr, "Error opening HDCH_OSD\n");
        return 0;
    }

    ch_xine_osd = (hd_channel_t*)hd_get_area(HDCH_XINE_OSD);

    if(!ch_xine_osd)
	    ch_xine_osd = hd_channel_create(HDCH_XINE_OSD, HDCH_XINE_OSD_SIZE, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_xine_osd, 1);

    if (!ch_xine_osd)
    {
        fprintf(stderr, "Error opening HDCH_XINE_OSD\n");
        return 0;
    }
    return 1;
}
