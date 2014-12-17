#ifndef ___COMMON_H
#define ___COMMON_H

#define SLEEPMS(x) cCondWait::SleepMs(x)

#include <vdr/tools.h>
static cTimeMs __time;
#define time_ms() ((int)__time.Elapsed())

#define DEV_DVB_DEMUX "demux"
#define DEV_DVB_CA    "ca"

#define MAX_CI_SLOTS 3

int DvbOpen(const char *Name, int n, int Mode);

// Debugging
#define DEBUG

#ifdef DEBUG
#define d(x) { (x); }
#else
#define d(x) ; 
#endif


#ifdef DEBUG_LOG
#define dl(x) { (x); }
#else
#define dl(x) ; 
#endif

#endif //___COMMON_H
