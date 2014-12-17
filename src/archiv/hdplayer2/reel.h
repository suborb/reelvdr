// reel.h

#ifndef REEL_H_INCLUDED
#define REEL_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <sys/time.h>

//Start by Klaus
#include <pthread.h>
//end by Klaus

#define REEL_ENABLE_ASSERTIONS

#ifdef REEL_ENABLE_ASSERTIONS

#define REEL_ASSERT(exp) (exp) ? ((void)0) : ReelDebugAssertFailed(__FILE__, __LINE__)
#define REEL_VERIFY(exp) REEL_ASSERT(exp)

void ReelDebugAssertFailed(char const *file, int line);

#else // ifndef REEL_ENABLE_ASSERTIONS

#define REEL_ASSERT(exp) ((void)0)
#define REEL_VERIFY(exp) (exp)

#endif // def REEL_ENABLE_ASSERTIONS

static inline int64_t GetCurrentTime()
{
    struct timeval now;

    gettimeofday(&now, NULL);

    return now.tv_sec * 1000000LL + now.tv_usec;
}

#define ALIGN_DOWN(v, a) ((v) & ~((a) - 1))
#define ALIGN_UP(v, a) ALIGN_DOWN((v) + (a) - 1, a)

//Start by Klaus
static inline unsigned int HighWord(unsigned long long val)
{
    return val >> 32;
}

static inline unsigned int LowWord(unsigned long long val)
{
    return val & 0x00000000FFFFFFFF;
}

static inline unsigned long long LongVal(unsigned int high, unsigned int low)
{
    unsigned long long val = high;
    val <<= 32;
    val |= low;
    return val;
}

static inline void MemB()
{
    //FIXME
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr = {PTHREAD_MUTEX_RECURSIVE_NP};
    pthread_mutex_init(&mutex, &attr);
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);   
}
//End by Klaus

#endif // ndef REEL_H_INCLUDED
