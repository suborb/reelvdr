// channel.h

#ifndef CHANNEL_H_INCLUDED
#define CHANNEL_H_INCLUDED

#include "reel.h"

#include "hdchannel.h"

extern hd_channel_t *ch_stream1_data;
extern hd_channel_t *ch_stream1_info;
extern hd_channel_t *ch_stream1;
extern hd_channel_t *ch_osd;

int create_channels(void);

#endif // def CHANNEL_H_INCLUDED
