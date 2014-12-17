/*
 * filetransfer.h: The video file transfer facilities
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: $
 */

#ifndef __FILETRANSFER_H
#define __FILETRANSFER_H

#include "recording.h"
#include "thread.h"

class cCopyingThread;

class cFileTransfer {
private:
  static cMutex mutex;
  static char *copiedVersionName;
  static cCopyingThread *copyingThread;
  static bool error;
  static bool ended;
public:
  static bool Start(cRecording *Recording, const char *NewName, bool CopyOnly = false);
  static void Stop(void);
  static bool Active(void);
  static bool Error(void);
  static bool Ended(void);
  };

#endif //__FILETRANSFER_H
