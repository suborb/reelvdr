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


#include <algorithm>
#include <string>

#include "vdr/channels.h"
#include "vdr/debug.h"
#include "vdr/device.h"
#include "vdr/interface.h"
#include "vdr/remote.h"
#include "vdr/thread.h"

#include "MenuBouquets.h"
#include "MenuBouquetsList.h"
#include "MenuEditChannel.h"
#include "SetChannelOptionsMenu.h"


//---------------------------------------------------------------------
//----------------------------cMenuBouquets-------------------------
//---------------------------------------------------------------------


#ifdef DEBUG_TIMES
extern struct timeval menuTimeVal1;
#endif

cMenuMyBouquets::cMenuMyBouquets(enum eChannellistMode view, enum eEditMode mode):cOsdMenu("", CHNUMWIDTH)

        /// OLD, use the enum types!!!
        ///< view: 0 = current bouquet
        ///<       1 = bouquet list
        ///<       2 = favourites
        ///<       3 = add to favourites
        ///<       4 = classic (full list, w/o bouquets)
        //
        ///< mode: 0 = view
        ///<       1 = edit
{
#ifdef DEBUG_TIMES
    struct timeval now;
    gettimeofday(&now, NULL);
    float secs2 =
        ((float)
         ((1000000 * now.tv_sec + now.tv_usec) -
          (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
    PRINTF("\n======================= time since kOk: secs: %f\n", secs2);
#endif
    editMode = mode;
    view_ = view;
    move = false;
    favourite = false;
    number = 0;
    selectionChanched = false;
    //channelMarked = -1;
    titleBuf[0] = '\0';         // Initialize to empty string
    startChannel = 0;
#if REELVDR && APIVERSNUM >= 10718
    EnableSideNote(true);
#endif

    if (view_ == bouquets)
    {
        calledFromkChan = true;
        view_ = bouquets;
    }  

    if (view_ == osActiveBouquet)
    {
        calledFromkChan = true;
        view_ = osActiveBouquet;
    }

    if (view == Default)   // Use setup setting
    {
        DDD("Setup.UseBouquetList: %d", ::Setup.UseBouquetList);
        switch (::Setup.UseBouquetList)
        {
            case 0:  view_ = classicList; break;
            case 1:  view_ = currentBouquet; break;
            case 2:  view_ = bouquets; break;
        }
    }

    if (view_ == bouquets)
    {
        AddSubMenu(new cMenuBouquetsList());
    }
    else if (view_ == favourites)
    {
        favourite = true;
        Setup();
        SetGroup(0);
        Display();
        Options();
    }
    else if (view_ == addToFavourites)
    {
        favourite = true;
        Setup();
        SetGroup(0);
        Options();
        AddFavourite(true);
    }
    else if (view_ == classicList)
    {
        Setup();
        Display();
        Options();
    }
    else
    {
        Setup();
        Options();
    }
    DDD("view_: %d, editMode: %d, move: %d, favourite: %d", view_, editMode, move, favourite);
    Channels.IncBeingEdited();

    if (editMode == mode_edit)
        SetStatus(tr("Select channels with OK"));

#ifdef DEBUG_TIMES
    struct timeval now2;
    gettimeofday(&now2, NULL);
    secs2 =
        ((float)
         ((1000000 * now2.tv_sec + now2.tv_usec) -
          (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
    PRINTF("======================= cMenuBouquets: time since key: secs: %f\n", secs2);
#endif
}

cMenuMyBouquets::~cMenuMyBouquets()
{
    Channels.DecBeingEdited();
    calledFromkChan = false;
}

void cMenuMyBouquets::ShowEventInfoInSideNote()
{
#if REELVDR && APIVERSNUM >= 10718
    cMenuMyChannelItem *currentItem =
        dynamic_cast < cMenuMyChannelItem * >(Get(Current()));

    if (currentItem) {
        const cEvent *event = currentItem->GetPresentEvent();
        SideNote(event);

        const cChannel *channel = currentItem->Channel();

        /* if event = NULL, then no info is drawn only a banner for now*/
        if (event && channel) {
            /* show HD & encrypted channel icons */
            char icons[3] = {0};
            int i=0;

            if (IS_HD_CHANNEL(channel))
                icons[i++] = char(142);
            if (channel->Ca())
                icons[i++] = 80;

            SideNoteIcons(icons);
        } // if channel

    } // if currentItem
#endif
}

void cMenuMyBouquets::Setup( /*bool selectionChanched */ )
{

    int ch_col = 4;

#if REELVDR
    int ChannelsCount = Channels.Count();
    if (ChannelsCount > 999)
        ch_col = 5;
    if (ChannelsCount > 9999)
        ch_col = 6;
#endif /* REELVDR */

    if (view_ == classicList)
    {                           // "classic" view
        DDD("classic view");
#if APIVERSNUM >= 10716
            SetCols(ch_col, 18, 6);
#else
            SetCols(ch_col, 3, 14, 6);
#endif

    }
    else if (editMode == mode_view)
    {                           // current bouquet or bouquet list
        DDD("edit mode: off");
        if (strcmp(Skins.Current()->Name(), "Reel") == 0)
#if APIVERSNUM >= 10716
            SetCols(ch_col, 18, 6 );
#else
            SetCols(ch_col, 3, 14, 6);
#endif
        else
            SetCols(ch_col, 18, 6);
    }
    else
    {
        DDD("else....");        // edit mode
        SetCols(4, ch_col, 18, 6); // indention in edit mode
    }

    if (view_ == classicList)
    {
        SetChannels();
    }
    else
    {
        int Index = Current();
        if (Index >= 0)
        {
            int index = 0;
            cMenuMyChannelItem *currentItem =
                static_cast < cMenuMyChannelItem * >(Get(Current()));
            for (cChannel * channel = Channels.First(); channel;
                 channel = Channels.Next(channel))
            {
                if (currentItem->Channel() == channel)
                {
                    break;
                }
                ++index;
            }
            Index = index;
        }
        else if (startChannel)
        {
            Index = Channels.GetNextNormal(startChannel);
        }

        if (Index < 0 && Channels.GetByNumber(cDevice::CurrentChannel()))
            Index = Channels.GetByNumber(cDevice::CurrentChannel())->Index();

        if (Index > -1)
            SetGroup(Index);
    }
    Display(); //TB: not needed //RC: IS needed for 1.7
}

void cMenuMyBouquets::CreateTitle(cChannel * channel)
{
    if (channel && channel->GroupSep())
    {
        if (channel->Name() || strlen(channel->Name()) > 0)
        {
            /*if(view_ == 4)
               {
               } */
            if (editMode == mode_edit
                && strcmp(Skins.Current()->Name(), "Reel") == 0
                && view_ != classicList)
            {
                strcpy((char *)&titleBuf, "menunormalhidden$");
                strncat((char *)&titleBuf, tr("Customize channels: "), 100);
                strncat((char *)&titleBuf, channel->Name(), 100);
                SetTitle((const char *)&titleBuf);
            }
            else
            {
                std::string title;
                if (view_ == currentBouquet || view_ == bouquets || view_ == favourites)
                {
                    title = std::string(channel->Name()) + ": ";
                }
                else if (view_ == addToFavourites)
                {
                    title = tr("Favorites: ");
                }
                else
                {
                    title = "..";
                    //strcpy((char*)&titleBuf, ".. ");
                }

                if (::Setup.OnlyRadioChannels == 1 &&::Setup.OnlyEncryptedChannels == 1)
                {
                    title += tr("encrypted radio channels");
                }
                else if (::Setup.OnlyRadioChannels == 1
                         && ::Setup.OnlyEncryptedChannels == 2)
                {
                    title += tr("free radio channels");
                }
                else if (::Setup.OnlyRadioChannels == 1
                         && ::Setup.OnlyEncryptedChannels == 0)
                {
                    title += tr("radio channels");
                }
                if (::Setup.OnlyRadioChannels == 2 &&::Setup.OnlyEncryptedChannels == 1)
                {
                    title += tr("encrypted TV channels");
                }
                else if (::Setup.OnlyRadioChannels == 2
                         && ::Setup.OnlyEncryptedChannels == 2)
                {
                    title += tr("free TV channels");
                }
                else if (::Setup.OnlyRadioChannels == 2
                         && ::Setup.OnlyEncryptedChannels == 0)
                {
                    title += tr("TV channels");
                }
                else if (::Setup.OnlyRadioChannels == 0
                         && ::Setup.OnlyEncryptedChannels == 1)
                {
                    title += tr("encrypted channels");
                }
                else if (::Setup.OnlyRadioChannels == 0
                         && ::Setup.OnlyEncryptedChannels == 2)
                {
                    title += tr("free channels");
                }
                else if (::Setup.OnlyHDChannels == 1)
                {
                    title += tr("HD channels");
                }
                else if (::Setup.OnlyHDChannels == 2)
                {
                    title += tr("non HD channels");
                }
                else if (::Setup.OnlyRadioChannels == 0
                         && ::Setup.OnlyEncryptedChannels == 0
                         && ::Setup.OnlyHDChannels == 0)
                {
                    title += tr("all channels");
                }

                //printf("----------title = %s-----------\n", channel->Name());
                SetTitle(title.c_str());
            }
        }
        else
        {
            SetTitle("");
        }
    }
    else
        SetTitle("");
}

void cMenuMyBouquets::SetChannels(void)
{
    cMenuMyChannelItem *currentItem = 0;
    cChannel *currentChannel = 0;
    if (Current() >= 0)
    {
        currentItem = static_cast < cMenuMyChannelItem * >(Get(Current()));
    }

    if (currentItem)
    {
        currentChannel = currentItem->Channel();
    }

    if (!currentChannel)
    {
        currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
    }

    Clear();

    for (cChannel * channel = Channels.First(); channel; channel = Channels.Next(channel))
    {
        if (!channel->GroupSep()
            || (cMenuMyChannelItem::SortMode() == cMenuMyChannelItem::csmNumber
                && *channel->Name()))
            if ((IS_RADIO_CHANNEL(channel) || (::Setup.OnlyRadioChannels != 1))
                && (!IS_RADIO_CHANNEL(channel) || (::Setup.OnlyRadioChannels != 2))
                && (IS_ENCRYPTED_RADIO_CHANNEL(channel)
                    || IS_ENCRYPTED_VIDEO_CHANNEL(channel)
                    || (::Setup.OnlyEncryptedChannels != 1))
                && (!(IS_ENCRYPTED_RADIO_CHANNEL(channel)
                    || IS_ENCRYPTED_VIDEO_CHANNEL(channel))
                    || (::Setup.OnlyEncryptedChannels != 2))
                && (IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 1))
                && (!IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 2)))
            {
                cMenuMyChannelItem *item;
                if (view_ == classicList)
                    item =
                        new cMenuMyChannelItem(channel, cMenuMyChannelItem::mode_classic);
                else if (editMode == mode_edit)
                    item = new cMenuMyChannelItem(channel, cMenuMyChannelItem::mode_edit);
                else
                    item = new cMenuMyChannelItem(channel);
                Add(item);
                item->Set();
                //printf("-----Add(item) item->Text = %s---\n", item->Text());
                if (channel == currentChannel)
                    currentItem = item;

                unsigned int i;
                for (i = 0; i < channelMarked.size(); i++)
                {
                    if (channelMarked.at(i) == channel->Number())
                    {
                        //printf("MARKING chan pos %i nr: %i name: %s\n", i, channel->Number(), channel->Name());
                        item->SetMarked(true);
                        item->Set();
                    }
                }
            }
    }

    if (cMenuMyChannelItem::SortMode() != cMenuMyChannelItem::csmNumber)
        Sort();

    if (currentItem)
        SetCurrent(currentItem);
}

void cMenuMyBouquets::SetGroup(int Index)
{
    bool back = false;
    if (Index < 0)
        Index = 0;
    cChannel *currentChannel = Channels.Get(Index);
    cChannel *firstChannel = NULL;
    if (Channels.Count() == 0)
        return;
    if (Index == 0)
        currentChannel = Channels.First();
    else if (!currentChannel)
        currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
    else if (Current() > 0)
        back = true;
    cMenuMyChannelItem *currentItem = NULL;
    if (!currentChannel->GroupSep())
    {

        startChannel = Channels.GetPrevGroup(currentChannel->Index());
    }
    else
        startChannel = currentChannel->Index();
    if (startChannel < 0)
    {
        startChannel = 0;
        firstChannel = Channels.First();
    }
    else
        firstChannel = Channels.Get(startChannel + 1);
    if (!firstChannel || firstChannel->GroupSep())
    {
        Clear();
        return;
    }
    if (back == true)
    {
        if (startChannel == 0 && !firstChannel->GroupSep())
            currentChannel = firstChannel;
        else;                   //currentChannel = (cChannel*) (Channels.Get(startChannel)->Next());
    }

    isyslog("start: %d name of first channel: %s", startChannel, firstChannel->Name());
    isyslog("current name: %s", currentChannel->Name());
    //printf("current name: %s", currentChannel->Name());
    Clear();
    int i = 0;
    for (cChannel * channel = firstChannel; channel && !channel->GroupSep();
         channel = Channels.Next(channel))
    {
        if (!channel->GroupSep()
            || (cMenuMyChannelItem::SortMode() == cMenuMyChannelItem::csmNumber
                && *channel->Name()))
        {
            if ((IS_RADIO_CHANNEL(channel) || (::Setup.OnlyRadioChannels != 1))
                && (!IS_RADIO_CHANNEL(channel) || (::Setup.OnlyRadioChannels != 2))
                && (IS_ENCRYPTED_RADIO_CHANNEL(channel)
                    || IS_ENCRYPTED_VIDEO_CHANNEL(channel)
                    || (::Setup.OnlyEncryptedChannels != 1))
                && (!(IS_ENCRYPTED_RADIO_CHANNEL(channel)
                    || IS_ENCRYPTED_VIDEO_CHANNEL(channel))
                    || (::Setup.OnlyEncryptedChannels != 2))
                && (IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 1))
                && (!IS_HD_CHANNEL(channel) || !(::Setup.OnlyHDChannels == 2)))
            {

                cMenuMyChannelItem *item;
                if (editMode == mode_edit)
                    item = new cMenuMyChannelItem(channel, cMenuMyChannelItem::mode_edit);
                else
                    item = new cMenuMyChannelItem(channel);
                Add(item);
                item->Set();
                //printf("-----Add(item) item->Text = %s, i = %d, Ca = %d---\n", item->Text(), i, item->Channel()->Ca());
                ++i;
                if (channel == currentChannel)
                    currentItem = item;
                unsigned int i;
                for (i = 0; i < channelMarked.size(); i++)
                {
                    if (channelMarked.at(i) == channel->Number())
                    {
                        //printf("MARKING chan pos %i nr: %i name: %s\n", i, channel->Number(), channel->Name());
                        item->SetMarked(true);
                        item->Set();
                    }
                }
            }
        }
    }

    if (cMenuMyChannelItem::SortMode() != cMenuMyChannelItem::csmNumber)
        Sort();

    if (currentItem)
        SetCurrent(currentItem);
}

cChannel *cMenuMyBouquets::GetChannel(int Index)
{
    cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(Index);
    return p ? (cChannel *) p->Channel() : NULL;
}

/* just removes the checkmark */
void cMenuMyBouquets::UnMark(cMenuMyChannelItem * p)
{
    if (p)
    {
        p->SetMarked(false);
        p->Set();
    }
}

void cMenuMyBouquets::Mark()
{
    if (Count())
    {
        if (editMode == mode_edit || view_ == classicList)
        {
            cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(Current());
            if (p)
            {
                if (p->IsMarked())
                {
                    //printf("UNMARKED chan nr: %i chnr: %i name: %s\n", Current(), GetChannel(Current())->Number(), GetChannel(Current())->Name());
                    /* remove checkmark */
                    p->SetMarked(false);
                    unsigned int i;
                    /* erase from "marked channels"-list */
                    for (i = 0; i < channelMarked.size(); i++)
                        if (channelMarked.at(i) == GetChannel(Current())->Number())
                            channelMarked.erase(channelMarked.begin() + i);
                    CursorDown();
                }
                else
                {
                    //printf("MARKED chan nr: %i chnr: %i name: %s\n", Current(), GetChannel(Current())->Number(), GetChannel(Current())->Name() );
                    /* set checkmark */
                    p->SetMarked(true);
                    /* put into "marked channels-list */
                    channelMarked.push_back(GetChannel(Current())->Number());
                    CursorDown();
                }
                p->Set();
                Display();
            }
            Options();
        }
        //SetStatus(tr("1-9 for new location - OK to move"));
        if (view_ == classicList && !move)        //???
            SetStatus(tr("Select channels with blue key"));
        else if (editMode == mode_view || (editMode == mode_edit && move)
                 || (view_ == classicList && move))
            SetStatus(tr("Up/Dn for new location - OK to move"));
        else if (editMode == mode_edit)
            SetStatus(tr("Select channels with OK"));
    }
}

void cMenuMyBouquets::Propagate(void)
{
    Channels.ReNumber();
    for (cMenuMyChannelItem * ci = (cMenuMyChannelItem *) First(); ci;
         ci = (cMenuMyChannelItem *) ci->Next())
        ci->Set();
    Display();
    Channels.SetModified(true);
}

/*eOSState cMenuBouquets::Number(eKeys Key)
{

  if (HasSubMenu())
     return osContinue;
  if (numberTimer.TimedOut())
     number = 0;

  number = number * 10 + Key - k0;
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next()) {
       if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number) {
         SetCurrent(ci);
         Display();
         break;
       }
  }
  numberTimer.Set(CHANNELNUMBERTIMEOUT);

  return osContinue;
}*/

eOSState cMenuMyBouquets::Number(eKeys Key)
{
    if (HasSubMenu())
        return osContinue;
    if (numberTimer.TimedOut())
        number = 0;
    if (!number && Key == k0)
    {
        cMenuMyChannelItem::IncSortMode();
        Setup();
        Display();
    }
    else
    {
        number = number * 10 + Key - k0;
        for (cMenuMyChannelItem * ci = (cMenuMyChannelItem *) First(); ci;
             ci = (cMenuMyChannelItem *) ci->Next())
        {
            if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number)
            {
                SetCurrent(ci);
                Display();
                break;
            }
        }
        numberTimer.Set(CHANNELNUMBERTIMEOUT);
    }
    return osContinue;
}

eOSState cMenuMyBouquets::Switch(void)
{
    if (HasSubMenu())
        return osContinue;
    cChannel *ch = GetChannel(Current());
    if (ch)
        return cDevice::PrimaryDevice()->SwitchChannel(ch, true) ? osEnd : osContinue;
    return osEnd;
}

eOSState cMenuMyBouquets::NewChannel(void)
{
    if (HasSubMenu())
        return osContinue;
    cChannel *ch = GetChannel(Current());
    if (!ch || favourite)
    {
        ch = Channels.GetByNumber(cDevice::CurrentChannel());
        favourite = false;
    }
    return AddSubMenu(new cMenuMyEditChannel(ch, true));
}

eOSState cMenuMyBouquets::EditChannel(void)
{
    if (HasSubMenu() || Count() == 0)
        return osContinue;
    cChannel *ch = GetChannel(Current());
    if (channelMarked.size() == 1)      //use marked channel
    {
        ch = Channels.GetByNumber(channelMarked.at(0));
    }
    if (ch)
    {
        return AddSubMenu(new cMenuMyEditChannel(ch, false));
    }
    return osContinue;
}

eOSState cMenuMyBouquets::DeleteChannel(void)
{
    if (!HasSubMenu() && Count() > 0)
    {
        if (channelMarked.empty())
        {
            int Index = Current();
            cChannel *channel = GetChannel(Current());
            int DeletedChannel = channel->Number();
            // Check if there is a timer using this channel:
            for (cTimer * ti = Timers.First(); ti; ti = Timers.Next(ti))
            {
                if (ti->Channel() == channel)
                {
                    Skins.Message(mtError, tr("Channel is being used by a timer!"));
                    return osContinue;
                }
            }
            //do not shwo buttons while deleting
            SetHelp(NULL, NULL, NULL, NULL);
            if (Interface->Confirm(tr("Delete channel?")))
            {  
                cChannel *currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());

                Channels.Del(channel);
                cOsdMenu::Del(Index);
                Propagate();   

                //set new channel number, do not actually swirch
                cDevice::SetCurrentChannel(currentChannel);

                //succesfully deleted
                Skins.Message(mtInfo, tr("Channel deleted"));
                Options();
                isyslog("channel %d deleted", DeletedChannel);
            }
            //redraw buttons
            Options();
        }
        else
        {
            int unsigned i;
            bool confirmed = false;
            /* sort and begin from behind: */
            /* channels must be deleted from the end, otherwise */
            /* the numbers of the channels that still have to be deleted will change */
            if (!channelMarked.empty())
                std::sort(channelMarked.begin(), channelMarked.end());
            //printf("XXX: size: %i\n", channelMarked.size());
            for (i = channelMarked.size(); i > 0; i--)
            {
                //int Index = channelMarked.at(i);
                cChannel *channel = Channels.GetByNumber(channelMarked.at(i - 1));      //GetChannel(channelMarked.at(i));
                //int Index = channel->Index();
                int DeletedChannel = channel->Number();
                //printf("XXX: i: %i at(i): %i Name: %s index: %i\n", i-1, channelMarked.at(i-1), channel->Name(), channel->Index());
                // Check if there is a timer using this channel:
                for (cTimer * ti = Timers.First(); ti; ti = Timers.Next(ti))
                {
                    if (ti->Channel() == channel)
                    {
                        Skins.Message(mtError, tr("Channel is being used by a timer!"));
                        //redraw buttons
                        Options();
                        return osContinue;
                    }
                }
                //do not show buttons while deleting
                SetHelp(NULL, NULL, NULL, NULL);
                if (confirmed || Interface->Confirm(tr("Delete channels?")))
                {
                    confirmed = true;
                    Channels.Del(channel);

                    int start = Current() - LOAD_RANGE;
                    int end = Current() + LOAD_RANGE;
                    //cChannel *channel;
                    if (Count() < end)
                        end = Count();
                    if (start < 0)
                        start = 0;

                    for (int i = start; i < end; i++)
                    {
                        cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(i);
                        if (p)
                        {
                            if (p->IsMarked())
                                cOsdMenu::Del(i);
                        }
                    }
                    //cOsdMenu::Del(Index);
                    Propagate();
                    //succesfully deleted,
                    Skins.Message(mtInfo, tr("Channels deleted"));
                    isyslog("channel %d deleted", DeletedChannel);
                }
                else
                {
                    //not confirmed -> go out
                    break;
                }
            }
            //redraw buttons
            Options();
            if (!channelMarked.empty())
                channelMarked.clear();
        }
    }
    return osContinue;
}

eOSState cMenuMyBouquets::ListBouquets(void)
{
    if (HasSubMenu())
    {
        return osContinue;
    }
    if (editMode == mode_edit || view_ == classicList)
    {
        return AddSubMenu(new cMenuBouquetsList(Channels.Get(startChannel), 0));
    }
    if (editMode == mode_view)
    {
        return AddSubMenu(new cMenuBouquetsList(Channels.Get(startChannel), 1));
    }
    return osContinue;
}

void cMenuMyBouquets::Move(int From, int To)
{
    Move(From, To, true);
}

void cMenuMyBouquets::Move(int From, int To, bool doSwitch)
{
    //printf("#####Move: From = %d To = %d\n", From, To);
    int CurrentChannelNr = cDevice::CurrentChannel();
    cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
    cChannel *FromChannel;
    cChannel *ToChannel;
    if (true)
    {                           //editMode == mode_view || view_ == 4) {
        FromChannel = (cChannel *) Channels.Get(From);
        ToChannel = (cChannel *) Channels.Get(To);
        //printf("#####Move: FromChannel = %d ToChannel = %d\n", FromChannel, ToChannel);
        //if(FromChannel && ToChannel)
        //   printf("#####Move: FromChannel = %s ToChannel = %s\n", FromChannel->Name(), ToChannel->Name());

    }
    else
    {
        FromChannel = (cChannel *) Channels.GetByNumber(From);
        ToChannel = (cChannel *) Channels.GetByNumber(To);
    }
    if (FromChannel && ToChannel /* && (From != To) */ )
    {
        int FromNumber = FromChannel->Number();
        int ToNumber = ToChannel->Number();
        Channels.Move(FromChannel, ToChannel);
        if (doSwitch)
        {
            Propagate();
            if (view_ != classicList)
            {
                SetGroup(startChannel);
            }
            Display();
        }
        if (ToNumber)
            isyslog("channel %d moved to %d", FromNumber, ToNumber);
        else
            isyslog("channel %d moved to %s", FromNumber, ToChannel->Name());
        if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr && doSwitch)
            Channels.SwitchTo(CurrentChannel->Number());
    }
}

enum directions
{ dir_none, dir_forward, dir_backward };

eOSState cMenuMyBouquets::MoveMultipleOrCurrent(void)
{
    if (channelMarked.size() == 0)
    {
        Mark();
        return MoveMultiple();
    }
    return MoveMultiple();
}

eOSState cMenuMyBouquets::MoveMultiple(void)
{
    int current, currentIndex;
    unsigned int i;
    enum directions direction = dir_none;

    cChannel *playingchannel =
        cDevice::CurrentChannel() !=
        0 ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
    int playingchannelnum = cDevice::CurrentChannel();

    Current() > -1 ? current = GetChannel(Current())->Number() : current = startChannel;
    Current() > -1 ? currentIndex = GetChannel(Current())->Index() : currentIndex =
        startChannel;

    /* we are finishing the "move-mode" */
    move = false;
    /* sort the list of channels to change - makes it easier */
    std::sort(channelMarked.begin(), channelMarked.end());
    //bool inced = false;
    Setup();
    /* for all marked channels */
    for (i = 0; i < channelMarked.size(); i++)
    {
        Channels.ReNumber();
        if (!startChannel || current <= channelMarked.at(i) + 1)
            direction = dir_backward;
        else
            direction = dir_forward;

        /* the channel shall be moved before the channel highlighted by the cursor */
        /* so decrease BUT NOT if "current-1" is a separator */
        /* and NOT if two channels with a distance of 1 should be swapped */
        if (direction == dir_backward
            /*&& current != startChannel && current > 1  && !((cChannel*)Channels.GetByNumber(current)->Prev())->GroupSep() */
            &&
            !(current - channelMarked.at(i) ==
              1 /*&& !GetChannel(currentIndex-1)->GroupSep() */ ) && i != 0)
        {                       //do not increment for first channel
            current++;
            currentIndex++;
        }

        cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(currentIndex);
        UnMark(p);
        if (channelMarked.at(i) != current || editMode == mode_edit)
        {
            if (!Channels.Get(currentIndex)->GroupSep())
            {
                if (channelMarked.at(i) != current)
                {
                    Channels.Move(Channels.GetByNumber(channelMarked.at(i)),
                                  Channels.GetByNumber(current));
                    unsigned int j;
                    /* if we are moving forward, the target number has to be decreased */
                    if (direction == dir_forward)
                        for (j = i; j < channelMarked.size(); j++)
                            if (channelMarked.at(j) > 1
                                && !Channels.GetByNumber(channelMarked.at(j) -
                                                         1)->GroupSep())
                            {
                                channelMarked[j]--;
                            }
                }
            }
            else
            {
                cChannel *chan = Channels.GetByNumber(channelMarked.at(i));
                Channels.Del(Channels.GetByNumber(channelMarked.at(i)), false);
                if (direction == dir_forward)
                {
                    Channels.Add(chan, Channels.Get(currentIndex - 1));

                    int j;

                    /* if we are moving forward, the target number has to be decreased */
                    if (direction == dir_forward)
                        for (j = i; j < (int)channelMarked.size(); j++)
                            if (channelMarked.at(j) > 1
                                && !Channels.GetByNumber(channelMarked.at(j) -
                                                         1)->GroupSep())
                            {
                                channelMarked[j]--;
                            }
                    currentIndex--;
                }
                else
                {
                    Channels.Add(chan, Channels.Get(currentIndex));
                }
            }
        }
        if (editMode == mode_edit || view_ == classicList)
        {
            p = (cMenuMyChannelItem *) Get(current);
            UnMark(p);
            p = (cMenuMyChannelItem *) Get(channelMarked.at(i));
            UnMark(p);
            p = (cMenuMyChannelItem *) Get(current);
            UnMark(p);
        }
    }
    for (i = 0; i < channelMarked.size(); i++)
    {
        cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(channelMarked.at(i));
        UnMark(p);
    }
    channelMarked.clear();
    if (view_ != classicList)
    {
        SetGroup(currentIndex);
    }
    Propagate();

    //SetStatus(tr("Select channels with OK"));
    //if(GetChannel(current))
    //    Channels.SwitchTo( GetChannel(Current())->Number() );

    if (playingchannel && playingchannelnum != playingchannel->Number())        //if position of channel beeing replayed has chanched
    {
        Channels.SwitchTo(playingchannel->Number());
    }

    if (view_ == classicList)
    {
        int curPos = Current();
        Clear();
        Setup();
        SetCurrent(Get(curPos));
        SetStatus(NULL);
    }
    Display();
    Skins.Message(mtInfo, tr("Channels have been moved"));
    return osContinue;
}

void cMenuMyBouquets::GetFavourite(void)
{
    cChannel *channel = Channels.First();
    if (!channel->GroupSep() || 0 != strcmp(channel->Name(), tr("Favorites")))
    {
        cChannel *newChannel = new cChannel();
        newChannel->groupSep = true;
        newChannel->nid = 0;
        newChannel->tid = 0;
        newChannel->rid = 0;
        strcpyrealloc(newChannel->name, tr("Favorites"));
        Channels.Add(newChannel);
        Channels.Move(newChannel, Channels.First());
        Channels.ReNumber();
        Channels.SetModified(true);
    }
}

void cMenuMyBouquets::AddFavourite(bool active)
{
    cChannel *channel =
        active ? Channels.GetByNumber(cDevice::CurrentChannel()) : GetChannel(Current());
    Display();
    if (channel && Interface->Confirm(tr("Add to Favorites?")))
    {
        cChannel *newChannel = new cChannel();
        *newChannel = *channel;
        newChannel->rid = 100;
        Channels.Add(newChannel);
        GetFavourite();
        Channels.Move(newChannel, Channels.First()->Next());
        Channels.ReNumber();
        Channels.SetModified(true);
        SetGroup(0);
        Display();
    }
}

eOSState cMenuMyBouquets::PrevBouquet()
{
    if (HasSubMenu())
        return osContinue;
    if (startChannel > 0)
    {
        SetGroup(startChannel - 1);
        Display();
    }
    else
    {
        SetGroup(Channels.Count() - 1);
        Display();
    }
    return osContinue;
}

eOSState cMenuMyBouquets::NextBouquet()
{
    int next;
    if (HasSubMenu())
        return osContinue;
    next = Channels.GetNextGroup(startChannel);
    if (-1 < next && next < Channels.Count())
    {
        SetGroup(next);
        Display();
    }
    else
    {
        SetGroup(0);
        Display();
    }
    return osContinue;
}

void cMenuMyBouquets::Options()
{
    if (view_ == classicList)
    {
        SetHelp(trVDR("Button$Edit"), tr("Button$New"), trVDR("Button$Delete"),
                trVDR("Button$Mark"));
    }
    else if (editMode == mode_view)
    {
        if (::Setup.ExpertNavi)
        {
            SetHelp(tr("Bouquets"), tr("Key$Previous"), tr("Key$Next"),
                    Count()? tr("Button$Customize") : NULL);
        }
        else
        {
            SetHelp(tr("Bouquets"), NULL, NULL, Count()? tr("Button$Customize") : NULL);
        }
    }
    else
    {
        if (move)
        {
            SetHelp(tr("Bouquets"), trVDR("Button$Insert"), NULL, NULL);
        }
        else
        {
            SetStatus(tr("Select channels with OK"));
            if (channelMarked.size() > 1)
            {
                SetHelp(tr("Bouquets"), tr("Move"), trVDR("Button$Delete"), NULL);
            }
            else if (channelMarked.size() == 1)
            {
                SetHelp(tr("Bouquets"), tr("Move"), trVDR("Button$Delete"), trVDR("Button$Edit"));
            }
            else
            {
                SetHelp(NULL, tr("Move"), trVDR("Button$Delete"), trVDR("Button$Edit"));
            }
        }
    }
}

void cMenuMyBouquets::Display(void)
{
#ifdef DEBUG_TIMES
    struct timeval now2;
    gettimeofday(&now2, NULL);
#endif

    int start = Current() - LOAD_RANGE;
    int end = Current() + LOAD_RANGE;
    cChannel *channel;
    if (Count() < end)
        end = Count();
    if (start < 0)
        start = 0;
    for (int i = start; i < end; i++)
    {
        cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(i);
        if (p)
        {
            if (!p->IsSet())
                p->Set();
        }
    }

    if (editMode == mode_view && !view_ == classicList)
    {
        cChannel *chan = NULL;
        cMenuMyChannelItem *p = (cMenuMyChannelItem *) Get(end - 1);

        if (p)
            chan = p->Channel();
        int endNr;
        int nrCharsForChanNr = 0;

        if (chan)
        {
            endNr = chan->Number();
            while (endNr > 0)
            {
                endNr /= 10;
                nrCharsForChanNr++;
            }
        }
        if (nrCharsForChanNr < 2)
            nrCharsForChanNr = 2;
        //DDD("editMode == mode_view && !view_ == 4");
        SetCols(nrCharsForChanNr, 1, 14, 6);
    }

    channel = Channels.Get(startChannel);
    CreateTitle(channel);
    Options();
    cOsdMenu::Display();
    ShowEventInfoInSideNote();
#ifdef DEBUG_TIMES
    struct timeval now3;
    gettimeofday(&now3, NULL);

    float secs =
        ((float)
         ((1000000 * now3.tv_sec + now3.tv_usec) -
          (now2.tv_sec * 1000000 + now2.tv_usec))) / 1000000;
#endif
}

#if APIVERSNUM < 10700
extern eOSState active_function;
#endif

eOSState cMenuMyBouquets::ProcessKey(eKeys Key)
{
    if (Key == kInfo)
    {
        return AddSubMenu(new cSetChannelOptionsMenu(selectionChanched));
    }
    //if(Key==kUp ||Key==kDown)

    //printf("current name: %s, current = %d\n", Channels.Get(Current())->Name(), Current());
    eOSState state = cOsdMenu::ProcessKey(NORMALKEY(Key));

    if (state != osUnknown && Key != kNone && !HasSubMenu())
        ShowEventInfoInSideNote();

    static bool hadSubMenu = false;
    if (hadSubMenu && !HasSubMenu() && selectionChanched)
    {
        selectionChanched = false;
        if (::Setup.UseBouquetList == 2)
        {
            view_ = currentBouquet;
        }
        else if (::Setup.UseBouquetList == 0)
        {
            view_ = classicList;
        }
        else if (::Setup.UseBouquetList == 1)
        {
            view_ = currentBouquet;
        }
        Setup();
        Display();
    }
    hadSubMenu = HasSubMenu();

    //  isyslog("ProcessKey ChannelKey: %d", Key);

    switch (state)
    {
    case osUser1:
        {                       // new channel
            cChannel *channel = Channels.Last();
            if (channel)
            {
                int current;
                cChannel *currentChannel = GetChannel(Current());
                //if(currentChannel)
                //    printf("currentChannel->Name = %s\n", currentChannel->Name());
                if (currentChannel)
                    current = currentChannel->Index() + 1;
                else
                    current = Channels.GetNextGroup(startChannel);
                if (-1 < current && current < Channels.Count())
                {
                    Move(channel->Index(), current, true);
                    Setup();
                    Display();
                }
                else
                {
                    Add(new cMenuMyChannelItem(channel), cMenuMyChannelItem::mode_view);
                }
                return CloseSubMenu();
            }
            break;
        }
    case osUser5:              // view bouquet channels
        if (HasSubMenu())
            CloseSubMenu();
        view_ = currentBouquet;
        SetGroup(::Setup.CurrentChannel);
        Setup();
        Display();
        break;
    case osBack:
        DDD("osBack");
        if (editMode == mode_edit)
        {
            editMode = mode_view;
            SetStatus(NULL);
            Setup();
            Display();
            return osContinue;
        }
        if (calledFromkChan)
        {
            return osEnd;
        }
        if (::Setup.UseBouquetList == 2)
        { 
            return ListBouquets();
        }
        else
        {
            /* TB: test if it was called via menu or via kOk */
#if APIVERSNUM < 10700
            if (active_function == 0)
#else
            //TODO: fixme - do this correct without active_function
            if (true)
#endif
            {
                move = false;
                editMode = mode_view;
                channelMarked.clear();
                printf("########return osBack;###########\n");
                return osBack;
            }
            else if (editMode == mode_edit)
            {
                if (move)       //???
                {
                    move = false;
                    channelMarked.clear();
                    int curPos = Current();
                    if (Current() && GetChannel(Current()))
                        SetGroup(GetChannel(Current())->Index());
                    SetCurrent(Get(curPos));
                    SetStatus(NULL);
                    Display();
                    return osContinue;
                }
                editMode = mode_view;
                channelMarked.clear();
                int curPos = Current();
                /*if(Current() && GetChannel(Current()))     ----------needed?
                   SetGroup(GetChannel(Current())->Index()); */
                /*if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                   SetCols(4, 14, 6);
                   else
                   SetCols(5, 18, 6); */
                Clear();
                Setup();
                SetCurrent(Get(curPos));
                SetStatus(NULL);
                Display();
                return osContinue;
            }
            else
                return osEnd;
        }
        break;
    default:
        if (state == osUnknown)
        {
            switch (Key)
            {
            case k0...k9:
                {
                    dsyslog("ProcessKey GetKey: %d extractVal %d", Key, Key - k0);
                    Number(Key);
                }
                Setup();
                break;
            case kOk:
                if (channelMarked.size() > 0)
                {
                    if (view_ == classicList)
                    {
                        if (move)
                        {
                            return MoveMultipleOrCurrent();
                            /*int From = channelMarked[0];
                               int To = Current();
                               Move(From, To); */
                        }
                    }
                    else if (editMode == mode_view)
                    {
                        SetStatus(NULL);
                    }
                    else if ((editMode == mode_edit && !move) /*||(channelMarked.empty() && editMode == mode_view) */ ) //??
                    {
                        Mark();
                        return osContinue;
                    }
                }
                else
                {
                    if (editMode == mode_view || view_ == classicList)
                    {
                        return Switch();
                    }
                    else
                    {
                        Mark();
                    }
                }
                return osContinue;
            case kRed:
                //printf("-------return ListBouquets()-----------\n");
                if (view_ == classicList)
                {
                    return EditChannel();       //Edit();//TODO
                }
                else if (editMode == mode_view || channelMarked.size() > 0)
                {
                    calledFromkChan = false;
                    return ListBouquets();
                }
                return osContinue;

            case kGreen:
                if (view_ == classicList)
                {
                    return NewChannel();
                }
                else if (editMode == mode_view)
                {
                    if (::Setup.ExpertNavi)
                    {
                        return PrevBouquet();
                    }
                }
                else
                {
                    if (!move)
                    {
                        if (channelMarked.empty())      //mark current item for move, if no item selected
                        {
                            Mark();
                        }
                        SetStatus(tr
                                  ("Move up/down for new position - then press 'Insert'"));
                        move = true;
                        Options();
                    }
                    else if (move)
                    {
                        return MoveMultipleOrCurrent();
                    }
                }
                state = osContinue;
                break;
            case kYellow:
                if (view_ == classicList || editMode == mode_edit)
                {
                    if (!move)
                    {           // Delete
                        DeleteChannel();
                    }
                }
                else if (editMode == mode_view)
                {
                    if (::Setup.ExpertNavi)
                    {
                        return NextBouquet();
                    }
                }
                state = osContinue;
                break;
            case kBlue:
                if (view_ == classicList)
                {
                    if (channelMarked.empty())  //mark current item for move, if no item selected
                    {
                        Mark();
                        SetStatus(tr("Move up/down for new position - then press 'OK'"));
                        move = true;
                        Options();
                    }
                    else
                    {
                        Mark();
                    }
                }
                else if (editMode == mode_view)
                {
#if 1
                    if (!GetChannel(Current()))
                    {
                        return osContinue;
                    }
                    editMode = mode_edit;
                    int curPos = Current();
                    SetGroup(GetChannel(Current())->Index());
                    if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                        SetCols(4, 5, 14, 6);
                    else
                        SetCols(4, 5, 18, 6);
                    SetCurrent(Get(curPos));
                    SetStatus(tr("Select channels with OK"));
                    Display();
#else
                    return EditChannel();
#endif
                }
                else
                {
                    return EditChannel();
                }
                state = osContinue;
                break;
            case k2digit:
                AddFavourite(false);
                break;
            case kGreater:
                if (view_ == currentBouquet)
                {
                    //printf("-------editMode2 = 1------\n");
                    //editMode2 = 1;
                    view_ = classicList;
                    ::Setup.UseBouquetList = 0;
                }
                else if (view_ == classicList)
                {
                    //printf("-------editMode2 = 0------\n");
                    //editMode2 = 0;
                    view_ = currentBouquet;
                    ::Setup.UseBouquetList = 1;
                    //printf("editMode2 = 0: current name: %s\n", Channels.Get(Current())->Name());
                }
                Setup();
                Display();
                state = osContinue;
                break;
            default:
                break;
            }
        }
    }
    return state;
}
