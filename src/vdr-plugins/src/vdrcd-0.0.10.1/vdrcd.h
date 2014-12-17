#ifndef VDR_VDRCD_H
#define VDR_VDRCD_H

#include <stdio.h>


#if defined DEBUG_VDRCD
#	define DLOG(x...) dsyslog(x)
#else
# define DLOG(x...)
#endif

#endif // VDR_VDRCD_H
