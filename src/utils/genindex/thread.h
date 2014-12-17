/*
 * thread.h: A simple thread base class
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: thread.h 1.15 2003/05/03 14:03:36 kls Exp $
 *
 * This was taken from the VDR package, which was released under the GPL.
 * Stripped down to make the PES parser happy
 */

#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

class cMutex {
private:
  pthread_mutex_t mutex;
  pid_t lockingPid;
  int locked;
public:
  cMutex(void);
  ~cMutex();
  void Lock(void);
  void Unlock(void);
  };

#endif //__THREAD_H
