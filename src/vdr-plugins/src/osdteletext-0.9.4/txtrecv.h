/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __TXTRECV_H
#define __TXTRECV_H

#include <vdr/status.h>
#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include <stdio.h>
#include <unistd.h>

#include "storage.h"

class cTelePage {
 private:
  int mag;
  unsigned char flags;
  unsigned char lang;
  PageID page;
  unsigned char pagebuf[27*40];
  Storage* storage;
 public:
  cTelePage(PageID page, uchar flags, uchar lang, int mag, Storage *s);
  ~cTelePage();
  void SetLine(int, uchar*);
  void save();
  bool IsTopTextPage();
 };

class cRingTxtFrames : public cRingBufferFrame {
 public:
  cRingTxtFrames(int Size) : cRingBufferFrame(Size, true) {};
  ~cRingTxtFrames() { Clear(); };
  void Wait(void) { WaitForGet(); };
  void Signal(void) { EnableGet(); };
};

class cTxtReceiver : public cReceiver, public cThread {
private:
   void DecodeTXT(uchar*);
   uchar unham16 (uchar*);
   cTelePage *TxtPage;
   void SaveAndDeleteTxtPage();
   bool storeTopText;
   cRingTxtFrames buffer;
   Storage *storage;
protected:
   virtual void Activate(bool On);
   virtual void Receive(uchar *Data, int Length);
   virtual void Action();
public:
   cTxtReceiver(const cChannel* chan, bool storeTopText, Storage* storage);
   virtual ~cTxtReceiver();
   virtual void Stop();
};

class cTxtStatus : public cStatus {
private:
   cTxtReceiver *receiver;
   bool storeTopText;
   Storage* storage;
protected:
   virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView);
public:
   cTxtStatus(bool storeTopText, Storage* storage);
   ~cTxtStatus();
};


#endif
