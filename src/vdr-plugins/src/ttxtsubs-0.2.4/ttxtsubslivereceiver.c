/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <vdr/device.h>
#include <vdr/remux.h>
#include "ttxtsubslivereceiver.h"

cTtxtSubsLiveReceiver::cTtxtSubsLiveReceiver(cChannel* channel, cVDRTtxtsubsHookListener* hook)
{
  _channel = channel;
  _hook = hook;
  AddPid(channel->Tpid());
}

cTtxtSubsLiveReceiver::~cTtxtSubsLiveReceiver()
{
  cReceiver::Detach();
}

void cTtxtSubsLiveReceiver::Receive(uchar *Data, int Length)
{
  if (Data == NULL)
  {
     tsToPesTeletext.Reset();
     return;
  }

  if (Length < TS_SIZE)
  {
     esyslog("ERROR: skipped %d bytes of TS fragment", Length);
     return;
  }

  while (Length >= TS_SIZE)
  {
    if (Data[0] != TS_SYNC_BYTE)
    {
      return;
    }
    if (TsHasPayload(Data)) // silently ignore TS packets w/o payload
    {
      int PayloadOffset = TsPayloadOffset(Data);
      if (PayloadOffset < TS_SIZE)
      {
        int l;
        tsToPesTeletext.PutTs(Data, Length);
        if (const uchar *p = tsToPesTeletext.GetPes(l))
        {
          if ((l > 45) && (p[0] == 0x00) && (p[1] == 0x00) && (p[2] == 0x01) && (p[3] == 0xbd) && (p[8] == 0x24) && (p[45] >= 0x10) && (p[45] < 0x20))
          {
            _hook->PlayerTeletextData((uchar *)p, l, false, _channel->TeletextSubtitlePages(), _channel->TotalTeletextSubtitlePages());
          }
          tsToPesTeletext.Reset();
        }
      }
    }
    Length -= TS_SIZE;
    Data += TS_SIZE;
  }
}
