#include "MenuMoveChannels.h"
#include "menu.h"
#include <vdr/channels.h>

cMenuMoveChannels::cMenuMoveChannels(cChannel* initChannel) :
    cOsdMenu(tr("Channel List: Move Channel")), initialChannel(initChannel)
{
    selectedChannels.clear();

    if (initialChannel) {
        selectedChannels.push_back(initialChannel->Number());
        SetMode(eMoveMode);
    } else
        SetMode(eSelectMode);

    Set(true); // start with given initial channel

}


void cMenuMoveChannels::Set(bool startWithGivenChannel)
{
    int curr = Current();

    Clear();
    /* tabs: Channel Nr / Channel Name / event progress bar  */
    SetCols(5, 18, 6);

    cOsdItem *item=NULL;
    bool isSelected = false;
    cChannel *ch = Channels.First();

    for ( ; ch; ch = Channels.Next(ch)) {
        if (!ch->GroupSep()) {
            isSelected = IsSelectedChannel(ch->Number());
            Add(item=new cOsdChannelItem(ch, false, isSelected));

            if (startWithGivenChannel &&
                    initialChannel &&
                    initialChannel->GetChannelID()==ch->GetChannelID())
                curr = item->Index();

        } // if !group sep
    } // for

    SetCurrent(Get(curr));
    Display();
} // Set()


cChannel* cMenuMoveChannels::CurrentChannel()
{
    cOsdChannelItem *chItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
    if (chItem)
        return chItem->Channel();

    return NULL;
}


cOsdChannelItem* cMenuMoveChannels::CurrentChannelItem()
{
    return dynamic_cast<cOsdChannelItem*> (Get(Current()));
}

void cMenuMoveChannels::MoveChannelItem(eKeys Key)
{
    cOsdItem *cItem = Get(Current());
    // move only when current item is a channel item
    if (!dynamic_cast<cOsdChannelItem*>(cItem)) return;

#if 0
    cOsdItem *pItem = Get(Current()-1); // prev item
    cOsdItem *nItem = Get(Current()+1); // next item
#endif

    int jumpBy = 0;

    switch(NORMALKEY(Key)) {
    case kUp:
        jumpBy = -1; break;
#if 0
        if (!pItem) return; // current item is first item

        if (Current()>0) {
            Move(Current(), Current()-1);
            SetCurrent(cItem);
        }

        break;
#endif

    case kDown:
        jumpBy = 1; break;
#if 0
        if (!nItem) return; // current item is last item

        if (Current() < Count()) {
            Move(Current(), Current()+1);
            SetCurrent(cItem);
        }
        break;
#endif

    case kLeft:
        jumpBy = -15; break;

    case kRight:
        jumpBy = 15; break;

    default:
        break;
    } // switch

    if (jumpBy && Current()+jumpBy >= 0 && Current()+jumpBy < Count()) {
        Move(Current(), Current()+jumpBy);
        SetCurrent(cItem);
        Display();
    } // if

}


cChannel * Position2Channel(int pos) {
    cChannel *toPosition = Channels.First();

    int i = -1;
    while (toPosition) {
        if (!toPosition->GroupSep()) i++; // another channel found
        //printf("%d   %d:%s\n", i, toPosition->Number(), toPosition->Name());
        if (i >= pos) break;
        toPosition = Channels.Next(toPosition);
    } // while

    return toPosition;
} // Position2Channel()


int Position2ChannelNumber(int pos) {
    cChannel *channel = Position2Channel(pos);
    return channel?channel->Number():-1;
} // Position2ChannelNumber()


//
// Move selected channels to given 'position' in channellist.
// 'position' corresponds to position of channels not counting bouquets or
// jumps in channel number
// 'position' counts from 0
void cMenuMoveChannels::MoveChannelsToPosition(int position)
{
    // TODO; XXX
    // channel number are not continuous. There maybe breaks in channel number
    // 102...110 then 200!
    // following code fails in this case. eg. Radiosender @500
    cChannel *toPosition = Channels.GetByNumber(position);
    int i = 0;
#if 1 // USE this later
    toPosition = Position2Channel(position);

    // no channel at 'position' found
    if (!toPosition) {
        esyslog("%s:%s no channel at position %d found, hence move failed",
                __FILE__,
                __PRETTY_FUNCTION__,
                position);
        return;
    }
    printf("at position %d found channel %d:%s\n",
           position,
           toPosition->Number(),
           toPosition->Name());
#endif

    printf("\033[0;92m vvvvvvvv     Move selected channels to position :%d '%s'\033[0m\n",
           position, toPosition?toPosition->Name():"<nil>");

    cList<cChannel> tmpList; // list of selected channels

    Channels.IncBeingEdited();

    // delete selected channels from "Channels" list and
    // add to tmpList
    for (i=0; i<selectedChannels.size(); ++i) {
        cChannel *channel = Channels.GetByNumber(selectedChannels.at(i));
        if (channel) {
            Channels.Del(channel, false /*do not delete obj*/);
            tmpList.Add(channel);
        } else {
            printf("\033[0;91m #%d channel not found\033[0m\n",
                   selectedChannels.at(i));
        }
    } // for

    printf("tmpList count=%d, '%s'\n",
           tmpList.Count(),
           toPosition?toPosition->Name():"");

    cChannel *channel = tmpList.First();
    i = 0;

    while(channel) {
        cChannel *nextChannel = tmpList.Next(channel);
        // remove channel from tmpList (unlink)
        tmpList.Del(channel, false /*do not delete obj*/);
        printf("ADD channel %d '%s'\n",  ++i, channel->Name());

        if (channel->Number() < /*position*/ toPosition->Number())
            Channels.Add(channel, toPosition); // add after position
        else
            Channels.Ins(channel, toPosition); // before position

        channel = nextChannel;
    } // while

    Channels.ReNumber();
    Channels.SetModified(true);
    Channels.DecBeingEdited();
    printf("\033[0;91m ======== Moving channels DONE ==========\033[0m\n");
}

void cMenuMoveChannels::SetModeMessage()
{
    cString msg;
    switch (mode) {
    case eSelectMode:
        msg = tr("Select a channel with OK key");
        break;

    case eMoveMode:
        msg = tr("Up/Down to move channel. Exit key removes selection.");
        break;

    default:
        msg = tr("Unknown mode");
        break;
    } // switch

    SetStatus(msg);
} // SetModeMessage()


eOSState cMenuMoveChannels::ProcessKey(eKeys Key)
{
    if (IsInMoveMode()) {
            if (NORMALKEY(Key) == kUp ||
                    NORMALKEY(Key) == kDown   ||
                    NORMALKEY(Key) == kLeft ||
                    NORMALKEY(Key) == kRight
                    ) {
                MoveChannelItem(Key);
                return osContinue;
            }
            else if (NORMALKEY(Key) == kBack) { // exit move mode
                SetMode(eSelectMode);
                selectedChannels.clear();
                Set();
                return osContinue;
            } // if kBack

    } // if in Move mode

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HasSubMenu()) return state;

    if (state == osUnknown) {
        switch(NORMALKEY(Key)) {
            case kOk:
        {
            printf("kOk at %d item\n", Current());
            if (IsInSelectMode()) {
                cChannel *currCh = CurrentChannel();
                if(currCh) {
                    if (!IsSelectedChannel(currCh->Number()))
                    {
                        SelectChannel(currCh->Number());
                        Set();
                        SetMode(eMoveMode);
                    } else {
                     // unselect channel
                    }
                } // if
            } // if select mode
            else if (IsInMoveMode()) {
                int channelNumberAtCurrentPosition = Position2ChannelNumber(Current());

                // "moving" selected channel to the same position?
                if (!IsSelectedChannel(channelNumberAtCurrentPosition)) {
                    MoveChannelsToPosition(Current());
                    selectedChannels.clear();
                    Set();
                    SetMode(eSelectMode);
                } else {
                    printf(" >>>>>>  channel #%d is already selected\n",
                           channelNumberAtCurrentPosition);
                    UnSelectChannel(channelNumberAtCurrentPosition);
                    Set();
                    // no channels selected, go into select mode
                    if (!selectedChannels.size())
                        SetMode(eSelectMode);
                }
            } // if move mode
        }
            state = osContinue;
            break;

        case kYellow: // undo, move
            Set(true);
            state = osContinue;
            break;

        default:
            break;
        } // switch
    } // if

    return state;
}
