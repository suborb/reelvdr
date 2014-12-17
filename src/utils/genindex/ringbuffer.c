/*
 * ringbuffer.c: A ring buffer
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Parts of this file were inspired by the 'ringbuffy.c' from the
 * LinuxDVB driver (see linuxtv.org).
 *
 * $Id: ringbuffer.c 1.17 2003/05/12 17:38:11 kls Exp $
 *
 * This was taken from the VDR package, which was released under the GPL.
 * Stripped down to make the PES parser happy
 */

#include "ringbuffer.h"
#include <stdlib.h>
#include <unistd.h>
#include "tools.h"

// --- cRingBuffer -----------------------------------------------------------

cRingBuffer::cRingBuffer(int Size, bool Statistics)
{
}

cRingBuffer::~cRingBuffer()
{
}

void cRingBuffer::WaitForPut(void)
{
}

void cRingBuffer::WaitForGet(void)
{
}

void cRingBuffer::EnablePut(void)
{
}

void cRingBuffer::EnableGet(void)
{
}

void cRingBuffer::SetTimeouts(int PutTimeout, int GetTimeout)
{
}

// --- cFrame ----------------------------------------------------------------

cFrame::cFrame(const uchar *Data, int Count, eFrameType Type, int Index)
{
}

cFrame::~cFrame()
{
}

// --- cRingBufferFrame ------------------------------------------------------

cRingBufferFrame::cRingBufferFrame(int Size, bool Statistics)
:cRingBuffer(Size, Statistics)
{
}

cRingBufferFrame::~cRingBufferFrame()
{
}

void cRingBufferFrame::Clear(void)
{
}

bool cRingBufferFrame::Put(cFrame *Frame)
{
  return false;
}

cFrame *cRingBufferFrame::Get(void)
{
  return 0;
}

void cRingBufferFrame::Drop(cFrame *Frame)
{
}

int cRingBufferFrame::Available(void)
{
  return 0;
}
