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

#include "MenuEditBouquet.h"
#include "MenuEditChannel.h"

#include "vdr/menu.h"

//---------------------------------------------------------------------
//----------------------------cMenuEditBouquet-------------------------
//---------------------------------------------------------------------

cMenuEditBouquet::cMenuEditBouquet(cChannel * Channel, bool New, cChannel * &newChannel)
: cOsdMenu(tr("Edit Bouquet"), 24), newchannel (newChannel), number(0)
{
    printf("----------cMenuEditBouquet::cMenuEditBouquet------------\n");
    channel = Channel;
    prevchannel = Channel;
    if (channel)
    {
        printf("channel->Number = %d\n", channel->Number());
        data = *channel;
        number = channel->Number();
    }

    if (New)
    {
        SetTitle(tr("Add Bouquet"));
        channel = NULL;
        data.groupSep = true;
        data.nid = 0;
        data.tid = 0;
        data.rid = 0;
    }
    bouquetCaId = 0;
    min_chan=1;

    /* TB: if all channels of the bouqet have the same CAID, show this in the menu */
    cChannel *channelE;
    bool first = 1;
    /* loop through all channels of the bouquet */
    if (!New)
        for (channelE = (cChannel *) channel->Next(); channelE && !channelE->GroupSep();
             channelE = (cChannel *) channelE->Next())
        {
            /* remember the first CAID */
            if (first)
            {
                first = false;
                bouquetCaId = channelE->Ca();
            }
            else if (channelE->Ca() != bouquetCaId)
            {
                /* remeber if there is one differing CAID */
                bouquetCaId = 0;
            }
        }

    Setup();
}

void cMenuEditBouquet::Setup(void)
{
    int current = Current();

    Clear();
    min_chan=1;
    SetCols(29);
    cChannel *ch = prevchannel ? (cChannel *)prevchannel->Prev() : NULL;
    while(ch && !ch->Number())
        ch = (cChannel *)ch->Prev();
    if(ch) min_chan = ch->Number() + (ch->GroupSep() ? 0 : 1);

    strn0cpy(name, data.name, sizeof(name));
    Add(new cMenuEditStrItem(trVDR("Name"), name, sizeof(name), trVDR(FileNameChars)));
    Add(new cMenuEditMyCaItem(tr("CI-Slot for this Bouquet"), &bouquetCaId, true));       //XXX
    Add(new cMenuEditIntItem(tr("Bouquet starts with number"), &number, min_chan-1, INT_MAX, trVDR("auto")));
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);

    SetCurrent(Get(current));

    if (Get(Current()))
    {
	if (strstr(Get(Current())->Text(), tr("CI-Slot for this Bouquet")))
	{
	    Add(new cOsdItem(tr("Note:"), osUnknown, false), false, NULL);
	    AddFloatingText(tr("Select CI-Slot for this Bouquet."), 50);
	    Add(new cOsdItem("", osUnknown, false), false, NULL);
	} else
	if (strstr(Get(Current())->Text(), tr("Bouquet starts with number")))
	{
	    Add(new cOsdItem(tr("Note:"), osUnknown, false), false, NULL);
	    AddFloatingText(tr("Select channelnumber for first channel inside this bouquet"), 50);
	}
    }
    Display();
}

eOSState cMenuEditBouquet::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown)
    {
        switch (Key)
        {
        case kOk:
            {
                if(number < min_chan) number=0;
                if (Channels.HasUniqueChannelID(&data, channel))
                {
                    data.name = strcpyrealloc(data.name, name);
                    if (channel)
                    {
                        //printf("######### channel copy########\n");
                        *channel = data;
                        newchannel = channel;
                        isyslog("edited bouquet %s", *data.ToText());
                        state = osBack;
                        if(channel->Number() != number)
                        {
                            cChannel *currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
                            channel->SetNumber(number);
                            Channels.ReNumber();
                            cDevice::SetCurrentChannel(currentChannel);
                        }
                    }
                    else
                    {
                        //printf("######### channel new########\n");
                        channel = new cChannel;
                        *channel = data;
                        newchannel = channel;
                        Channels.Ins(channel, prevchannel);     //add before current bouquet
                        isyslog("added bouquet %s", *data.ToText());
                        state = osUser1;
                    }
                    if (bouquetCaId > 0)
                    {
                        cChannel *channelE;
                        for (channelE = (cChannel *) channel->Next();
                             channelE && !channelE->GroupSep();
                             channelE = (cChannel *) channelE->Next())
                        {
                            int caids[2] = { bouquetCaId, 0 };
                            if (channelE)
                            {
#ifdef REELVDR
                                channelE->ForceCaIds((const int *)&caids);
#else
                                channelE->SetCaIds((const int *)&caids);
#endif
                                isyslog
                                    ("editing complete bouquet: setting caid of channel %s to %i",
                                     channelE->Name(), bouquetCaId);
                            }
                        }
                    }
                    Channels.SetModified(true);
                }
                else
                {
                    Skins.Message(mtError, trVDR("Channel settings are not unique!"));
                    state = osContinue;
                }
            }
            case kBack:
            {
                return osBack;
            }
            break;
        default:
            state = osUnknown;
        }
	return state;
    } else if ((Key == kUp) || (Key == kDown) || (Key == (kUp|k_Repeat)) || (Key == (kDown|k_Repeat)))
    {
	Setup();
    }
    if (state == osContinue)
	state = osUnknown;
    return state;
}
