/*
 * ringbuffer.h: A ring buffer
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: ringbuffer.h 1.12 2003/05/12 17:35:10 kls Exp $
 *
 * This was taken from the VDR package, which was released under the GPL.
 * Stripped down to make the PES parser happy
 */

#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include "thread.h"
#include "tools.h"

class cRingBuffer {
private:
protected:
  void WaitForPut(void);
  void WaitForGet(void);
  void EnablePut(void);
  void EnableGet(void);
  virtual void Clear(void) = 0;
  virtual int Available(void) = 0;
  int Free(void) { return 0; }
  void Lock(void) { }
  void Unlock(void) { }
  int Size(void) { return 0; }
public:
  cRingBuffer(int Size, bool Statistics = false);
  virtual ~cRingBuffer();
  void SetTimeouts(int PutTimeout, int GetTimeout);
  };

enum eFrameType { ftUnknown, ftVideo, ftAudio, ftDolby };

class cFrame {
  friend class cRingBufferFrame;
private:
public:
  cFrame(const uchar *Data, int Count, eFrameType = ftUnknown, int Index = -1);
  ~cFrame();
  uchar *Data(void) const { return 0; }
  int Count(void) const { return 0; }
  eFrameType Type(void) const { return ftUnknown; }
  int Index(void) const { return 0; }
  };

class cRingBufferFrame : public cRingBuffer {
private:
public:
  cRingBufferFrame(int Size, bool Statistics = false);
  virtual ~cRingBufferFrame();
  virtual int Available(void);
  virtual void Clear(void);
    // Immediately clears the ring buffer.
  bool Put(cFrame *Frame);
    // Puts the Frame into the ring buffer.
    // Returns true if this was possible.
  cFrame *Get(void);
    // Gets the next frame from the ring buffer.
    // The actual data still remains in the buffer until Drop() is called.
  void Drop(cFrame *Frame);
    // Drops the Frame that has just been fetched with Get().
  };

#endif // __RINGBUFFER_H
