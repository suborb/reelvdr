#ifndef reelcamlink_h
#define reelcamlink_h

#include "plugin.h"
#include "device.h"

#define RE_NAME "reelcam"
#define RE_MAGIC { 0,'R','E',0xc4,0xa3,0xf2 }

#define OP_PROVIDES 0
#define OP_GETSLOT  1
#define OP_RESET    2

struct ReLink {
  char magic[6];
  short op;
  const cDevice *dev;
  unsigned slotOnDev;
  unsigned short *caids;
  int channelNumber;
  int num;
  int slot;
  };

static const char magic[] = RE_MAGIC;

#ifndef PLUGIN_COMPILE

void PrepareReLink(struct ReLink *link, const cDevice *dev, short op);
int DoReLinkOp(struct ReLink *link);
int GetSlotOnDev(const cDevice *dev);
int ResetSlot(int slot);

#endif

#endif
