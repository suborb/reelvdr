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

#ifndef __TTXTSUBSCHANNELSETTINGS_H
#define __TTXTSUBSCHANNELSETTINGS_H

#include <vdr/tools.h>
#include <vdr/channels.h>

#define PAGE_MODE_AUTO 0
#define PAGE_MODE_MANUAL 1
#define PAGE_MODE_DISABLED 2

class cTtxtSubsChannelSetting:public cListObject {
private:
  tChannelID channelid;
  int pagemode;
  int pagenumber;
public:
  cTtxtSubsChannelSetting(void) {}
  cTtxtSubsChannelSetting(const cChannel* Channel, int PageMode, int PageNumber);
  bool Parse(const char *s);
  bool Save(FILE *f);
  tChannelID ChannelID(void) const { return channelid; }
  int PageMode(void) const { return pagemode; }
  int PageNumber(void) const { return pagenumber; }
  void Set(int PageMode, int PageNumber) { pagemode=PageMode; pagenumber=PageNumber; }
  };
  
class cTtxtSubsChannelSettings:public cConfig<cTtxtSubsChannelSetting> {
public:
  // TODO: This methode can be dropped when using only the ChannelID
  cTtxtSubsChannelSetting *Get(const cChannel *Channel) {
    tChannelID ChannelID=Channel->GetChannelID();
    return Get(ChannelID);
  }
  cTtxtSubsChannelSetting *Get(tChannelID ChannelID) {
    for (cTtxtSubsChannelSetting *p=First(); p; p=Next(p)) {
      if (p->ChannelID()==ChannelID) return p;
    }
    return NULL;
  }
  int Page(const cChannel *Channel) {
    cTtxtSubsChannelSetting *cs=Get(Channel);
    if (!cs) return(0);
    if (cs->PageMode()==PAGE_MODE_DISABLED) return(-1);
    int temp=cs->PageNumber();
    int mag=temp/100;
    temp=temp%100;
    int page_no=(mag<<8)+((temp/10)<<4)+(temp%10);
    if (page_no>=0x800) page_no-=0x800;
    return(page_no);
  }
};

extern cTtxtSubsChannelSettings TtxtSubsChannelSettings;

#endif //__TTXTSUBSCHANNELSETTINGS_H
