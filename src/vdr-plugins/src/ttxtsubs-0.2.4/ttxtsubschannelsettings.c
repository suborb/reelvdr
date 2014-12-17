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

#include "ttxtsubschannelsettings.h"

#include <vdr/channels.h>
#include <vdr/plugin.h>
#include <vdr/tools.h>

cTtxtSubsChannelSetting::cTtxtSubsChannelSetting(const cChannel *Channel, int PageMode, int PageNumber)
{
  channelid=Channel->GetChannelID();
  pagemode=PageMode;
  pagenumber=PageNumber;
}

bool cTtxtSubsChannelSetting::Parse(const char *s)
{
   bool result=false;
   char *buffer=NULL;
   int PageMode;
   int PageNumber;
   if (3 == sscanf(s, "%a[^,],%d,%d\n", &buffer, &PageMode, &PageNumber)) {
      tChannelID channelID=tChannelID::FromString(buffer);
      if (channelID.Valid()) {
        channelid=channelID;
        pagemode=PageMode;
        pagenumber=PageNumber;
        result=true;
      }
      else { // parse old format
        int sid;
        if (1 == sscanf(buffer, "%d", &sid)) {
          for (cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch)) {
            if (!ch->GroupSep() && ch->Sid() == sid) {
              channelid=ch->GetChannelID();
              pagemode=PageMode;
              pagenumber=PageNumber;
              result=true;
              break;
            }
          }
        }
      }
   }
   free(buffer);
   return result;
}        

bool cTtxtSubsChannelSetting::Save(FILE *f)
{
  return fprintf(f, "%s,%d,%d\n", *channelid.ToString(), pagemode, pagenumber) > 0; 
}

