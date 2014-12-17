/*
 * recorder.h: The actual DVB recorder
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recorder.h 2.3 2010/12/27 11:17:04 kls Exp $
 */

#ifndef __RECORDER_H
#define __RECORDER_H

#include "receiver.h"
#include "recording.h"
#include "remux.h"
#include "ringbuffer.h"
#include "thread.h"

#ifdef USE_LIVEBUFFER
class cRecorder : public cReceiver, public cThread {
protected:
#else
class cRecorder : public cReceiver, cThread {
private:
#endif /*USE_LIVEBUFFER*/
  cRingBufferLinear *ringBuffer;
  cFrameDetector *frameDetector;
  cPatPmtGenerator patPmtGenerator;
  cFileName *fileName;
#ifdef USE_LIVEBUFFER
  cIndex *index;
  bool handleError;
#else
  cIndexFile *index;
#endif /*USE_LIVEBUFFER*/
  cUnbufferedFile *recordFile;
  char *recordingName;
  off_t fileSize;
  time_t lastDiskSpaceCheck;
#ifdef USE_LIVEBUFFER
  virtual bool RunningLowOnDiskSpace(void);
  virtual bool NextFile(void);
  virtual void FillInitialData(uchar *Data, int Size) {};
#else
  bool RunningLowOnDiskSpace(void);
  bool NextFile(void);
#endif /*USE_LIVEBUFFER*/
protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);
  virtual void Action(void);
public:
  cRecorder(const char *FileName, const cChannel *Channel, int Priority);
               // Creates a new recorder for the given Channel and
               // the given Priority that will record into the file FileName.
  virtual ~cRecorder();
#ifdef USE_LIVEBUFFER
  virtual int GetFramesPerSecond() { return frameDetector? frameDetector->Synced() ? frameDetector->FramesPerSecond() : DEFAULTFRAMESPERSECOND : DEFAULTFRAMESPERSECOND;};
#endif /*USE_LIVEBUFFER*/
  };

#endif //__RECORDER_H
