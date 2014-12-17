#ifndef _REEL_H
#define _REEL_H

#include "../../../kernel/reeldvb/reeldvb.h"
#include <sys/ioctl.h>


// Destinations

#define REEL_DST_DMA0     0
#define REEL_DST_PIDF0    1
#define REEL_DST_DMA1     2
#define REEL_DST_PIDF1    3
#define REEL_DST_DMA2     4
#define REEL_DST_PIDF2    5
#define REEL_DST_DMA3     6
#define REEL_DST_PIDF3    7
#define REEL_DST_CI0      8
#define REEL_DST_X0       9
#define REEL_DST_CI1      10
#define REEL_DST_X1       11
#define REEL_DST_CI2      12
#define REEL_DST_X2       13

#define REEL_DST_DMA(n)   ((n)*2)
#define REEL_DST_PIDF(n)  (1+(n)*2)
#define REEL_DST_CI(n)    (8+(n)*2)
#define REEL_DST_X(n)     (9+(n)*2)

// Source definitions for Matrix

#define REEL_SRC_TS0   0
#define REEL_SRC_PIDF0 1
#define REEL_SRC_TS1   2
#define REEL_SRC_PIDF1 3
#define REEL_SRC_TS2   4
#define REEL_SRC_PIDF2 5
#define REEL_SRC_TS3   6
#define REEL_SRC_PIDF3 7
#define REEL_SRC_CI0   8
#define REEL_SRC_X0    9
#define REEL_SRC_CI1   10
#define REEL_SRC_X1    11
#define REEL_SRC_CI2   12
#define REEL_SRC_X2    13

#define REEL_SRC_TS(n)   ((n)*2)
#define REEL_SRC_PIDF(n) (1+(n)*2)
#define REEL_SRC_CI(n)   (8+(n)*2)
#define REEL_SRC_X(n)    (9+(n)*2)

#endif
