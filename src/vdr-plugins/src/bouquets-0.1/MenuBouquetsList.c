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

#include "MenuBouquetsList.h"

#include "vdr/remote.h"
#include "vdr/thread.h"
#include "vdr/device.h"
#include "vdr/interface.h"
#include "vdr/menu.h"

#include "def.h"
#include "MenuBouquetItem.h"
#include "MenuEditBouquet.h"
#include "MenuChannelItem.h"

bool calledFromkChan = false;

//---------------------------------------------------------------------
//----------------------------cMenuBouquetsList-------------------------
//---------------------------------------------------------------------

cMenuBouquetsList::cMenuBouquetsList(cChannel * channel, int mode):cOsdMenu("Bouquets",
                                                                            CHNUMWIDTH)
{
    mode_ = mode;
    HadSubMenu_ = false;
    newChannel_ = NULL;
    bouquetMarked = -1;
    Setup(channel);
}

void cMenuBouquetsList::Setup(cChannel * channel)
{
    Clear();
    cChannel *currentChannel =
        channel ? channel : Channels.GetByNumber(cDevice::CurrentChannel());
    cMenuBouquetItem *currentItem = NULL;
    while (currentChannel && !currentChannel->GroupSep())
    {
        if (currentChannel->Prev())
            currentChannel = (cChannel *) currentChannel->Prev();
        else
            break;
    }
    for (cChannel * channel = (cChannel *) Channels.First(); channel;
         channel = (cChannel *) channel->Next())
    {
        //printf("##cMenuBouquetsList::Setup, channel->Name() = %s\n", channel->Name());
        if (channel->GroupSep() && !(FilteredBouquetIsEmpty(channel) && mode_ == 1))
        {
            cMenuBouquetItem *item = new cMenuBouquetItem(channel);
            Add(item);
            if (!currentItem && item->Bouquet() == currentChannel)
                currentItem = item;
        }
    }

    if (!currentItem)
        currentItem = (cMenuBouquetItem *) First();
    SetCurrent(currentItem);
    Options();
    Display();
}

bool cMenuBouquetsList::FilteredBouquetIsEmpty(cChannel * startChannel)
{
    //printf("-----cMenuBouquetsList::FilteredBouquetIsEmpty, startChannel->Name() = %s--------\n", startChannel->Name());
    for (cChannel * channel = startChannel; channel;
         channel = (cChannel *) channel->Next())
    {

        if (channel != startChannel && channel->GroupSep())
        {
            return true;
        }
        if ((!channel->GroupSep())
            && (IS_RADIO_CHANNEL(channel) || !(::Setup.OnlyRadioChannels == 1))
            && (!IS_RADIO_CHANNEL(channel) || !(::Setup.OnlyRadioChannels == 2))
            && (IS_ENCRYPTED_RADIO_CHANNEL(channel)
                || IS_ENCRYPTED_VIDEO_CHANNEL(channel)
                || !(::Setup.OnlyEncryptedChannels == 1))
            && (!(IS_ENCRYPTED_RADIO_CHANNEL(channel)
                || IS_ENCRYPTED_VIDEO_CHANNEL(channel))
                || (::Setup.OnlyEncryptedChannels != 2))
            && (IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 1))
            && (!IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 2)))
        {
            return false;
        }
    }
    return true;
}

void cMenuBouquetsList::Options()
{
    if (mode_ == 0)
    {
        SetHelp(NULL, NULL, NULL, NULL);
    }
    else if (mode_ == 1)
    {
        SetTitle(tr("Bouquets"));
        SetHelp(tr("Channels"), NULL, NULL, tr("Button$Customize"));
    }
    else if (mode_ == 2)
    {
        //SetStatus(tr("Select bouquet with OK"));
        SetTitle(tr("Customize Bouquets"));
        SetHelp(tr("New"), tr("Move"), trVDR("Button$Delete"), trVDR("Button$Edit"));
    }
    Display();
}

void cMenuBouquetsList::Propagate()
{
    Channels.ReNumber();
    for (cMenuBouquetItem * ci = (cMenuBouquetItem *) First(); ci;
         ci = (cMenuBouquetItem *) ci->Next())
        ci->Set();
    Display();
    Channels.SetModified(true);
}

cChannel *cMenuBouquetsList::GetBouquet(int Index)
{
    if (Count() <= Index)
        return NULL;
    cMenuBouquetItem *p = (cMenuBouquetItem *) Get(Index);
    return p ? (cChannel *) p->Bouquet() : NULL;
}

eOSState cMenuBouquetsList::Switch()
{
    if (HasSubMenu())
        return osContinue;
    cChannel *channel = GetBouquet(Current());
    while (channel && channel->GroupSep())
        channel = (cChannel *) channel->Next();
    if (channel)
        return cDevice::PrimaryDevice()->SwitchChannel(channel,
                                                       true) ? osEnd : osContinue;
    return osEnd;
}

eOSState cMenuBouquetsList::ViewChannels()
{
    if (HasSubMenu())
        return osContinue;
    cChannel *channel = GetBouquet(Current());
    if (!channel)
        return osEnd;
    ::Setup.CurrentChannel = channel->Index();
    return osUser5;
}

eOSState cMenuBouquetsList::EditBouquet()
{
    cChannel *channel;
    if (HasSubMenu() || Count() == 0)
        return osContinue;
    channel = GetBouquet(Current());
    if (channel)
    {
        newChannel_ = channel;
        return AddSubMenu(new cMenuEditBouquet(channel, false, newChannel_));
    }
    return osContinue;
}

eOSState cMenuBouquetsList::NewBouquet()
{
    cChannel *channel;
    if (HasSubMenu())
        return osContinue;
    channel = GetBouquet(Current());
    newChannel_ = channel;
    return AddSubMenu(new cMenuEditBouquet(channel, true, newChannel_));
}

eOSState cMenuBouquetsList::DeleteBouquet()
{
    if (Interface->Confirm(tr("Delete Bouquet?")))
    {
        cChannel *bouquet = GetBouquet(Current());
        cChannel *next = (cChannel *) bouquet->Next();
        /* Delete all channels up to the beginning */
        /* of the next bouquet */
        while (next && !next->GroupSep())
        {
            cChannel *p = next;
            next = (cChannel *) next->Next();
            Channels.Del(p);
        }
        /* Delete the bouquet itself */
        Channels.Del(bouquet);
        /* Remove the OSD-item */
        cOsdMenu::Del(Current());
        Propagate();
        if (Current() < -1)
            SetCurrent(0);
        return osContinue;
    }
    return osContinue;
}

void cMenuBouquetsList::Mark(void)
{

    if (Count() && bouquetMarked < 0)
    {
        bouquetMarked = Current();
        SetStatus(tr("Up/Dn for new location - OK to move"));
    }
}

void cMenuBouquetsList::Move(int From, int To)
{
    int CurrentChannelNr = cDevice::CurrentChannel();
    cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
    cChannel *FromChannel = GetBouquet(From);
    cChannel *ToChannel = GetBouquet(To);
    if (To > From)
        for (cChannel * channel = (cChannel *) ToChannel->Next();
             channel && !channel->GroupSep(); channel = (cChannel *) channel->Next())
            ToChannel = channel;

    if (FromChannel && ToChannel && FromChannel != ToChannel)
    {
        int FromIndex = FromChannel->Index();
        int ToIndex = ToChannel->Index();
        cChannel *NextFromChannel = NULL;
        while (FromChannel && (!FromChannel->GroupSep() || !NextFromChannel))
        {
            NextFromChannel = (cChannel *) FromChannel->Next();
            Channels.Move(FromChannel, ToChannel);
            if (To > From)
                ToChannel = FromChannel;
            FromChannel = NextFromChannel;
        }

        if (From != To)
        {
            cOsdMenu::Move(From, To);
            Propagate();
            isyslog("bouquet from %d moved to %d", FromIndex, ToIndex);
        }
        if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr)
        {
            isyslog("CurrentChannelNr = %d; CurrentChannel::number = %d",
                    CurrentChannelNr, CurrentChannel->Number());
            Channels.SwitchTo(CurrentChannel->Number());
        }
    }
}

void cMenuBouquetsList::Display(void)
{
    int start = Current() - LOAD_RANGE;
    int end = Current() + LOAD_RANGE;
    for (int i = start; i < end; i++)
    {
        cMenuBouquetItem *p = (cMenuBouquetItem *) Get(i);
        if (p && p->Bouquet())
        {
            p->Set();
        }
    }
    cOsdMenu::Display();
}

eOSState cMenuBouquetsList::ProcessKey(eKeys Key)
{
    if (!HasSubMenu() && Key == kBack && (mode_ == 2))   //go back to standard mode
    {
        cChannel *currentbouquet = GetBouquet(Current());
        if (currentbouquet && FilteredBouquetIsEmpty(currentbouquet))
        {
            for (cChannel * channel = currentbouquet; channel;
                channel = (cChannel *) channel->Next())
            {
                if (channel->GroupSep() && !(FilteredBouquetIsEmpty(channel)))
                {
                    currentbouquet = channel;
                    break;
                }
            }
        }

        mode_ = 1;
        Setup(currentbouquet);
        SetStatus(NULL);
        return osContinue;
    }

    eOSState state = cOsdMenu::ProcessKey(NORMALKEY(Key));

    //redraw
    if (HadSubMenu_ && !HasSubMenu())
    {
        mode_ = 2;
        Setup(newChannel_);
        Options();
    }
    HadSubMenu_ = HasSubMenu();

    switch (state)
    {
    case osUser1:
        {                       // add bouquet
            cChannel *channel = Channels.Last();
            if (channel)
            {
                cMenuBouquetItem *item = new cMenuBouquetItem(channel);
                item->Set();
                Add(item, true);
                if (HasSubMenu())
                    return CloseSubMenu();
            }
            break;
        }
    case osBack:
        if (mode_ == 2)
        {
            ;
        }
        else
        {
            if (::Setup.UseBouquetList == 1 && !calledFromkChan)
                return ViewChannels();
            else
                return osEnd;
        }
    default:
        if (state == osUnknown)
        {
            switch (Key)
            {
            case kOk:
                if (bouquetMarked >= 0)
                {
                    SetStatus(NULL);
                    if (bouquetMarked != Current())
                        Move(bouquetMarked, Current());
                    bouquetMarked = -1;
                }
                else
                    return ViewChannels();
                break;
            default:
                //if (showHelp) //if helpkeys were not shown,donot act on them
                switch (Key)
                {
                case kRed:
                    if (mode_ == 2)
                    {
                        return NewBouquet();
                    }
                    else if (mode_ == 1)
                    {
                        return ViewChannels();  //back to Channels
                    }
                    break;;
                case kGreen:
                    if (mode_ == 2)
                    {           // Move Bouquet
                        Mark();
                        break;
                    }
                    break;
                case kYellow:
                    if (mode_ == 2)
                    {           // Delete Bouquet
                        return DeleteBouquet();
                    }
                    break;
                case kBlue:
                    if (mode_ == 2)
                    {
                        return EditBouquet();
                    }
                    else if (mode_ == 1)
                    {
                        mode_ = 2;
                        Setup(GetBouquet(Current()));
                        return osContinue;
                    }
                    break;

                /*case kGreater:
                    if (mode_ == 2)
                    {
                        newChannel_ = GetBouquet(Current());
                        return AddSubMenu(new cMenuChannels());
                    }
                    break;*/
                default:
                    break;
                }               // switch
                //else PRINTF("showHelp = %i\n", showHelp);
                break;
            }
        }
    }
    return state;
}
