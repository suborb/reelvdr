#ifndef _GRAPHLCD_COMPAT_H_
#define _GRAPHLCD_COMPAT_H_


#if VDRVERSNUM < 10318
#define TimeMs time_ms
#else
#define TimeMs cTimeMs::Now
#endif

#if VDRVERSNUM >= 10318
#define Apid1() Apid(0)
#define Apid2() Apid(1)
#define Dpid1() Dpid(0)
#define Dpid2() Dpid(1)
#endif

#endif
