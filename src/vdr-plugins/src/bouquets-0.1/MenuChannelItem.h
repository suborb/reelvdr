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

// MenuChannelItem.h

#ifndef P__MENU_CHANNELITEM_H
#define P__MENU_CHANNELITEM_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

#define IS_RADIO_CHANNEL(c)           (c->Vpid()==0||c->Vpid()==1)
#define IS_ENCRYPTED_RADIO_CHANNEL(c) (IS_RADIO_CHANNEL(c) && (c->Ca() > 0))
#define IS_ENCRYPTED_VIDEO_CHANNEL(c) (!IS_RADIO_CHANNEL(c) && (c->Ca() > 0))
#if APIVERSNUM < 10716
#define IS_HD_CHANNEL(c)              (c->IsSat() && (c->Modulation() == 10))
#else
#define IS_HD_CHANNEL(c)              ( (c->Vpid() != 0) /*radio channel are not HD*/ && c->IsSat() && (cDvbTransponderParameters(c->Parameters()).System() == SYS_DVBS2 || strstr(c->Name(), " HD")/* assume HD channel if name contains " HD" substring*/))
#endif


class cMenuMyChannelItem:public cOsdItem
{
  public:
    enum eViewMode2
    { mode_edit, mode_view, mode_classic };
    enum eChannelSortMode
    { csmNumber, csmName, csmProvider };
  private:
    static eChannelSortMode sortMode;
    enum eViewMode2 viewMode;
    cChannel *channel;
    cSchedulesLock schedulesLock;
    const cSchedules *schedules;
    const cEvent *event;
    char szProgressPart[12];
    bool isSet;
    bool isMarked;
  public:
      cMenuMyChannelItem(cChannel * Channel, eViewMode2 viewMode = mode_view);
    static void SetSortMode(eChannelSortMode SortMode)
    {
        sortMode = SortMode;
    }
    static void IncSortMode(void)
    {
        sortMode = eChannelSortMode((sortMode == csmProvider) ? csmNumber : sortMode + 1);
    }
    static eChannelSortMode SortMode(void)
    {
        return sortMode;
    }
    virtual int Compare(const cListObject & ListObject) const;
    virtual void Set(void);
    bool IsSet(void);
    bool IsMarked(void)
    {
        return isMarked;
    }
    void SetMarked(bool marked)
    {
        isMarked = marked;
    }
    cChannel *Channel(void)
    {
        return channel;
    }

    const cEvent* GetPresentEvent()
    {

        if (channel)
        {
            cSchedulesLock schedulesLock;
            schedules = cSchedules::Schedules(schedulesLock);

            if (schedules)
            {
                const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());

                if (schedule)
                    event = schedule->GetPresentEvent();

            } // if

        } // if (bouquet)

        return event;
    } // GetPresentEvent()
};

#endif // P__MENU_CHANNELITEM
