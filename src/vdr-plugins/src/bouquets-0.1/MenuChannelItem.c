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

#include "MenuChannelItem.h"

//-----------------------------------------------------------
//---------------cMenuMyChannelItem----------------------
//-----------------------------------------------------------


cMenuMyChannelItem::eChannelSortMode cMenuMyChannelItem::sortMode = csmNumber;

cMenuMyChannelItem::cMenuMyChannelItem(cChannel * Channel, eViewMode2 vMode)
{
    channel = Channel;
    event = NULL;
    isSet = false;
    isMarked = false;
    viewMode = vMode;
    szProgressPart[0] = '\0';   /* initialize to valid string */
    if (channel->GroupSep())
        SetSelectable(false);
    //Set();
}

int cMenuMyChannelItem::Compare(const cListObject &ListObject) const
{
    cMenuMyChannelItem *p = (cMenuMyChannelItem *) &ListObject;
    int r = -1;
    if (sortMode == csmProvider)
        //r = strcoll(channel->Provider(), p->channel->Provider());
        r = strcasecmp(channel->Provider(), p->channel->Provider());
    if (sortMode == csmName || r == 0)
        //r = strcoll(channel->Name(), p->channel->Name());
        r = strcasecmp(channel->Name(), p->channel->Name());
    if (sortMode == csmNumber || r == 0)
        r = channel->Number() - p->channel->Number();
    return r;
}

bool cMenuMyChannelItem::IsSet(void)
{
    return isSet;
}

void cMenuMyChannelItem::Set(void)
{
    char buffer[256];
/*#if APIVERSNUM >= 10716
    cDvbTransponderParameters dtp(channel->Parameters());
    printf("#'%d' bw=%d Mod=%d Source=%d isHD=%d\n", channel->Number(),
           dtp.Bandwidth(), dtp.Modulation(), channel->Source(),IS_HD_CHANNEL(channel));

    bool isHD = (dtp.Modulation() == 9);
#endif */
    bool isHD = IS_HD_CHANNEL(channel);

    if (!channel->GroupSep())
    {
        schedules = cSchedules::Schedules(schedulesLock);
        if (schedules && (viewMode == mode_view || mode_classic))
        {
            const cSchedule *Schedule = schedules->GetSchedule(channel->GetChannelID());
            if (Schedule)
            {
                event = Schedule->GetPresentEvent();
                if (event)
                {
                    char szProgress[9];
                    int frac = 0;
#ifdef SLOW_FPMATH
                    frac =
                        (int)roundf((float)(time(NULL) - event->StartTime()) /
                                    (float)(event->Duration()) * 8.0);
                    frac = min(8, max(0, frac));
#else
                    frac =
                        min(8,
                            max(0,
                                (int)((time(NULL) -
                                       event->StartTime()) * 8) / event->Duration()));
#endif
                    for (int i = 0; i < frac; i++)
                        szProgress[i] = '|';
                    szProgress[frac] = 0;
                    sprintf(szProgressPart, "[%-8s]\t", szProgress);
                }
            }
        }
        if (sortMode == csmProvider)
        {
            snprintf(buffer, 255, "%d\t%s - %s", channel->Number(), channel->Provider(),
                     channel->Name());
        }
        else if (viewMode == mode_classic)      // full list "classic"
        {
            /*char tag_icon = 0;
            if (channel->Ca())  // ChannelItem() (CA)
                if (isMarked)
                {
                    tag_icon = '>';     // check mark
                }
                else
                {
                    tag_icon = 80;      // encryption (key) symbol
                }
            else if (isMarked)
            {
                tag_icon = '>'; // check mark
            }
            else
            {
                tag_icon = ' ';
            }
            */
            char tag_icon = ' ';

// 142 ... 145 HD, HDenc, newHD, newEnc
            if(channel->Ca()) 
	    {
		tag_icon = isHD? char(143) :80;    // encryption (key) symbol
	    }
	    else 
	    {
		tag_icon = isHD? char(142) :' ';   //HD icons or nothing
	    }

            if(isMarked) tag_icon = '>';        //check mark

#if REELVDR && APIVERSNUM >= 10716
            if (isMarked) /* show tick mark instead of channel number */
                snprintf(buffer, 255, ">\t%-.18s\t%s%-.40s",
                         channel->Name(), szProgressPart, event ? event->Title() : " ");
            else
            snprintf(buffer, 255, "%d\t%-.18s\t%s%-.40s", channel->Number(),
                     channel->Name(), szProgressPart, event ? event->Title() : " ");
#else
            snprintf(buffer, 255, "%d\t%c\t%-.18s\t%s%-.40s", channel->Number(), tag_icon,
                     channel->Name(), szProgressPart, event ? event->Title() : " "); // channel name width set according to cMenuMyBouquets::Setup
#endif /* REELVDR */
        }
        else if (viewMode == mode_edit)
        {    
            char tag_icon = 0;
            if (isMarked)
            {
                tag_icon = '>';     // check mark
            }
            else
            {
                tag_icon = ' ';      // encryption (key) symbol

            }

            snprintf(buffer, 255,"%c\t%d\t%-.40s", tag_icon, channel->Number(), channel->Name());
        }
        else if (viewMode == mode_view)
        {
            if (strcmp(Skins.Current()->Name(), "Reel") == 0)
            {                       // current bouquet
                char tag_icon = 0;
                if (channel->Ca())
		{
		    tag_icon = isHD? char(143) : 80;
		}
                else
                    tag_icon = isHD? char(142) : ' ';


#if REELVDR && APIVERSNUM >= 10716
            snprintf(buffer, 255, "%d\t%-.18s\t%s%-.40s", channel->Number(),
                     channel->Name(), szProgressPart, event ? event->Title() : " ");
#else
                snprintf(buffer, 255, "%02d\t%c\t%-.18s\t%s%-.40s", channel->Number(), tag_icon, 
                        channel->Name(), szProgressPart, event ? event->Title() : " "); // channel name width set according to cMenuMyBouquets::Setup
#endif
            }
            else
            {
                snprintf(buffer, 255, "%d\t%-.14s\t%s    %-.40s", channel->Number(),
                        channel->Name(), szProgressPart, event ? event->Title() : " ");
            }
        }
    }
    else
    {
        if (viewMode == mode_classic)
        {                       // Bouquet seperator
            //asprintf(&buffer, "     %-.25s", channel->Name());
            snprintf(buffer, 255, "\t%-.25s", channel->Name());
        }
        else
        {
            snprintf(buffer, 255,"-----\t\t%-.25s\t------------------", channel->Name());
        }
    }
    event = NULL;
    isSet = true;
    SetText(buffer, true);
}
