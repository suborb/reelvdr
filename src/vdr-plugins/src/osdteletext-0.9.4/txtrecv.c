/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <dirent.h>

#include "txtrecv.h"
#include "tables.h"
#include "setup.h"
#include "menu.h"

#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/config.h>

#include <pthread.h> 
#include <signal.h> 
#include <errno.h>
#include <sys/vfs.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

cTelePage::cTelePage(PageID t_page, uchar t_flags, uchar t_lang,int t_mag, Storage *s)
  : mag(t_mag), flags(t_flags), lang(t_lang), page(t_page), storage(s)
{
   memset(pagebuf,' ',26*40);
}

cTelePage::~cTelePage() {
}

void cTelePage::SetLine(int line, uchar *myptr)
{
   memcpy(pagebuf+40*line,myptr,40);
}

void cTelePage::save()
{
   unsigned char buf;
   StorageHandle fd;
   if ( (fd=storage->openForWriting(page)) ) {
      storage->write("VTXV4",5,fd);
      buf=0x01; storage->write(&buf,1,fd);
      buf=mag;  storage->write(&buf,1,fd);
      buf=page.page; storage->write(&buf,1,fd);
      buf=flags; storage->write(&buf,1,fd);
      buf=lang; storage->write(&buf,1,fd);
      buf=0x00; storage->write(&buf,1,fd);
      buf=0x00; storage->write(&buf,1,fd);
      storage->write(pagebuf,24*40,fd);
      storage->close(fd);
   }
}

bool cTelePage::IsTopTextPage()
{
   return (page.page & 0xFF) > 0x99 || (page.page & 0x0F) > 0x9;
}

cTxtStatus::cTxtStatus(bool storeTopText, Storage* storage)
   : receiver(NULL), storeTopText(storeTopText), storage(storage)
{
}

cTxtStatus::~cTxtStatus()
{
    delete receiver;
}

void cTxtStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView)
{
   // Disconnect receiver if channel is 0, will reconnect to new
   // receiver after channel change.
   if (LiveView && ChannelNumber == 0)
      DELETENULL(receiver);

   // ignore if channel is 0
   if (ChannelNumber == 0) return;

   // ignore if channel is invalid (highly unlikely, this will ever
   // be the case, but defensive coding rules!)
   cChannel* newLiveChannel = Channels.GetByNumber(ChannelNumber);
   if (newLiveChannel == NULL) return;

   // ignore non-live-channel-switching
   if (!LiveView || ChannelNumber != cDevice::CurrentChannel()) return;

   // live channel was changed
   // now re-attach the receiver to the new live channel

   DELETENULL(receiver);

   int TPid = newLiveChannel->Tpid();

   if (TPid) {
      receiver = new cTxtReceiver(newLiveChannel, storeTopText, storage);
      cDevice::ActualDevice()->AttachReceiver(receiver);
   }

   TeletextBrowser::ChannelSwitched(ChannelNumber);
}


cTxtReceiver::cTxtReceiver(const cChannel* chan, bool storeTopText, Storage* storage)
 : cReceiver(chan, -1), cThread("osdteletext-receiver"),
   TxtPage(0), storeTopText(storeTopText), buffer((188+60)*75), storage(storage)
{
   SetPids(NULL);
   AddPid(chan->Tpid());
   storage->prepareDirectory(ChannelID());

   // 10 ms timeout on getting TS frames
   buffer.SetTimeouts(0, 10);
}


cTxtReceiver::~cTxtReceiver()
{
   Detach();
   Activate(false);
   buffer.Clear();
   delete TxtPage;
}

void cTxtReceiver::Stop()
{
   Activate(false);
}

void cTxtReceiver::Activate(bool On)
{
  if (On) {
     if (!Running()) {
        Start();
        }
     }
  else if (Running()) {
     buffer.Signal();
     Cancel(2);
     }
}

void cTxtReceiver::Receive(uchar *Data, int Length)
{
   cFrame *frame=new cFrame(Data, Length);
   if (!buffer.Put(frame)) {
      // Buffer overrun
      delete frame;
      buffer.Signal();
   }
}

void cTxtReceiver::Action() {

   while (Running()) {
      cFrame *frame=buffer.Get();
      if (frame) {
         uchar *Datai=frame->Data();

         for (int i=0; i < 4; i++) {
            if (Datai[4+i*46]==2 || Datai[4+i*46]==3) {
               for (int j=(8+i*46);j<(50+i*46);j++)
                  Datai[j]=invtab[Datai[j]];
               DecodeTXT(&Datai[i*46]);
            }
         }

         buffer.Drop(frame);
      } else
         buffer.Wait();
   }

   buffer.Clear();
}

uchar cTxtReceiver::unham16 (uchar *p)
{
  unsigned short c1,c2;
  c1=unhamtab[p[0]];
  c2=unhamtab[p[1]];
  return (c1 & 0x0F) | (c2 & 0x0F) *16;
}

void cTxtReceiver::SaveAndDeleteTxtPage()
{
  if (TxtPage) {
     if (storeTopText || !TxtPage->IsTopTextPage()) {
        TxtPage->save();
        DELETENULL(TxtPage);
     }
  }
}

void cTxtReceiver::DecodeTXT(uchar* TXT_buf)
{
   // Format of buffer:
   //   0x00-0x04  ?
   //   0x05-0x06  Clock Run-In?
   //   0x07       Framing Code?
   //   0x08       Magazine number (100-digit of page number)
   //   0x09       Line number
   //   0x0A..0x31 Line data
   // Line 0 only:
   //   0x0A       10-digit of page number
   //   0x0B       1-digit of page number
   //   0x0C       Sub-Code bits 0..3
   //   0x0D       Sub-Code bits 4..6 + C4 flag
   //   0x0E       Sub-Code bits 8..11
   //   0x0F       Sub-Code bits 12..13 + C5,C6 flag
   //   0x10       C7-C10 flags
   //   0x11       C11-C14 flags
   //
   // Flags:
   //   C4 - Erase last page, new page transmitted
   //   C5 - News flash, boxed display
   //   C6 - Subtitle, boxed display
   //   C7 - Suppress Header, dont show line 0
   //   C8 - Update, page has changed
   //   C9 - Interrupt Sequence, page number is out of order
   //   C10 - Inhibit Display
   //   C11 - Magazine Serial mode
   //   C12-C14 - Language selection, lower 3 bits


   int hdr,mag,mag8,line;
   uchar *ptr;
   uchar flags,lang;

   hdr = unham16 (&TXT_buf[0x8]);
   mag = hdr & 0x07;
   mag8 = mag ?: 8;
   line = (hdr>>3) & 0x1f;
   ptr = &TXT_buf[10];

   switch (line) {
   case 0: 
      {
      unsigned char b1, b2, b3, b4;
      int pgno, subno;
      b1 = unham16 (ptr);    
      // Page no, 10- and 1-digit

      if (b1 == 0xff) break;
      SaveAndDeleteTxtPage();

      b2 = unham16 (ptr+2); // Sub-code 0..6 + C4
      b3 = unham16 (ptr+4); // Sub-code 8..13 + C5,C6
      b4 = unham16 (ptr+6); // C7..C14

      // flags:
      //   0x80  C4 - Erase page
      //   0x40  C5 - News flash
      //   0x20  C6 - Subtitle
      //   0x10  C7 - Suppress Header
      //   0x08  C8 - Update
      //   0x04  C9 - Interrupt Sequence
      //   0x02  C9 (Bug?)
      //   0x01  C11 - Magazine Serial mode
      flags=b2 & 0x80;
      flags|=(b3&0x40)|((b3>>2)&0x20); //??????
      flags|=((b4<<4)&0x10)|((b4<<2)&0x08)|(b4&0x04)|((b4>>1)&0x02)|((b4>>4)&0x01);
      lang=((b4>>5) & 0x07);

      pgno = mag8 * 256 + b1;
      subno = (b2 + b3 * 256) & 0x3f7f;         // Sub Page Number

      TxtPage = new cTelePage(PageID(ChannelID(), pgno, subno), flags, lang, mag, storage);
      TxtPage->SetLine((int)line,(uchar *)ptr);
      break;
      }
   case 1 ... 25: 
      {
      if (TxtPage) TxtPage->SetLine((int)line,(uchar *)ptr); 
      break;
      }
   /*case 23: 
      {
      SaveAndDeleteTxtPage();
      break;
      }*/
   default:
      break;
   }
}


