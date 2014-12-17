/*
 * thread.c: A simple thread base class
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: thread.c 1.25 2003/05/18 12:45:13 kls Exp $
 *
 * This was taken from the VDR package, which was released under the GPL.
 * Stripped down to make the PES parser happy
 */

#include "thread.h"
#include <unistd.h>

// --- cMutex ----------------------------------------------------------------

cMutex::cMutex(void)
{
  lockingPid = 0;
  locked = 0;
  pthread_mutex_init(&mutex, NULL);
}

cMutex::~cMutex()
{
  pthread_mutex_destroy(&mutex);
}

void cMutex::Lock(void)
{
  if (getpid() != lockingPid || !locked) {
     pthread_mutex_lock(&mutex);
     lockingPid = getpid();
     }
  locked++;
}

void cMutex::Unlock(void)
{
 if (!--locked) {
    lockingPid = 0;
    pthread_mutex_unlock(&mutex);
    }
}
