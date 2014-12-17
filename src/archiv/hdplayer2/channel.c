// channel.c

#include "channel.h"

#include <stdio.h>

hd_channel_t *ch_stream1_data;
hd_channel_t *ch_stream1_info;
hd_channel_t *ch_stream1;
hd_channel_t *ch_osd;

int create_channels(void)
{
    ch_stream1 = (hd_channel_t*)hd_get_area(HDCH_STREAM1);
    
    if(!ch_stream1)	
        ch_stream1 = hd_channel_create(HDCH_STREAM1, 0x40000, HD_CHANNEL_READ);
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

    ch_stream1_data = (hd_channel_t*)hd_get_area(HDCH_STREAM1_DATA);
    
    if(!ch_stream1_data)	
        ch_stream1_data = hd_channel_create(HDCH_STREAM1_DATA, HDCH_STREAM1_DATA_SIZE, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_stream1_data, 1);
    
    if (!ch_stream1_data)
    {
        fprintf(stderr, "Error opening HDCH_STREAM1_DATA\n");
        return 0;
    }

    ch_osd = (hd_channel_t*)hd_get_area(HDCH_OSD);

    if(!ch_osd)
        ch_osd = hd_channel_create(HDCH_OSD, 0x20000, HD_CHANNEL_READ);
    else
        hd_channel_invalidate(ch_osd, 1);

    if (!ch_osd)
    {
        fprintf(stderr, "Error opening HDCH_OSD\n");
        return 0;
    }
    return 1;
}
