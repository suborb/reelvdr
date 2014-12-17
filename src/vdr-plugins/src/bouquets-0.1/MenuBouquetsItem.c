/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "MenuBouquetItem.h"


#include "vdr/remote.h"
#include "vdr/thread.h"



//---------------------------------------------------------------------
//----------------------------cMenuBouquetItem-------------------------
//---------------------------------------------------------------------


cMenuBouquetItem::cMenuBouquetItem(cChannel * channel)
{
    bouquet = channel;
    event = NULL;
    szProgressPart[0] = '\0';   /* initialize to valid string */
}

void cMenuBouquetItem::Set()
{
    char *buffer = NULL;
/*  cChannel *channel = (cChannel*) bouquet->Next();

  if (channel && !channel->GroupSep()) {
     schedules = cSchedules::Schedules(schedulesLock);
     if (schedules) {
    const cSchedule *Schedule = schedules->GetSchedule(channel->GetChannelID());
    if (Schedule) {
       event = Schedule->GetPresentEvent();
       if (event) {
          char szProgress[9];
          int frac = 0;
          frac = (int)roundf( (float)(time(NULL) - event->StartTime()) / (float)(event->Duration()) * 8.0 );
          frac = min(8,max(0, frac));
          for(int i = 0; i < frac; i++)
         szProgress[i] = '|';
          szProgress[frac]=0;
          sprintf(szProgressPart, "%c%-8s%c", '[', szProgress, ']');
       }
    }
        asprintf(&buffer, "%s    -    %s   %s  %-.20s", bouquet->Name(), channel->Name(), szProgressPart, event?event->Title():" ");
     }
     else
        asprintf(&buffer, "%s    -    %s", bouquet->Name(), channel->Name());
  }
  else */
    asprintf(&buffer, "%s", bouquet->Name());
//  event = NULL;
    SetText(buffer, false);
}
