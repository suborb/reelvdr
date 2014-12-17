#ifndef ACTIVE_LIST
#define ACTIVE_LIST

#include <vdr/channels.h>

extern cChannels activeChannelList;

bool SaveFilteredChannelList(bool isFav = false);

/* find the next available channel from the active channel list
param currChannel is the channel number (in vdr's channel list) */
int NextAvailableChannel(int currChannel, int direction);
cChannel* NextAvailableChannel(cChannel* channel, int direction);

#endif
