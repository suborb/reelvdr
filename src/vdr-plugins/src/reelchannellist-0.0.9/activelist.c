#include "activelist.h"
#include "menu.h"
#include "favourites.h"
#include <vdr/plugin.h>

/* maintaining a list of active channels and deleting and updating them uses 
   a lot of cpu time */
#define DONOT_SAVE_ACTIVE_CHANNELS 1

cChannels activeChannelList;

/** *********** SaveFilteredChannelList() **************************************
  Copy non-groupsep channels from either vdr's channels list or
  favourites list
  Copy from vdr's channel list is filtered by globalFilters
  Copy from favourites list is NOT filtered by favouritesFilters
*******************************************************************************/
bool SaveFilteredChannelList(bool isFav)
{
    printf("Channel selected from %s\n", isFav?"Favourites":"channellist");
    printf("Global filters count := %d\n", globalFilters.channelFilters.Count());
    printf("Fav filters count := %d\n", favouritesFilters.channelFilters.Count());

    savedGlobalFilters      = globalFilters;
    savedFavouritesFilters  = favouritesFilters;
    savedWasLastMenuFav = isFav;

    Setup.UseZonedChannelList = true;

    savedGlobalFilters.Save();
    savedFavouritesFilters.Save();
            
    cPlugin *plugin = cPluginManager::GetPlugin("reelchannellist");
    if (plugin) 
        plugin->SetupStore("OpenWithFavourites", isFav);
    else 
        esyslog("%s:%d reelchannellist not found", __FILE__, __LINE__);

#if !DONOT_SAVE_ACTIVE_CHANNELS
    cChannels *CopyFrom = &Channels;
    if (isFav)
        CopyFrom = &favourites;

    cChannel *channel ;
    cChannel *newChannel = NULL; // new channel

    // clear active list
    channel = activeChannelList.First();
    for (; channel; channel=newChannel) {
        newChannel = activeChannelList.Next(channel);
        activeChannelList.Del(channel);
    }

    // copy channels into activeChannelsList
    channel = CopyFrom->First();
    newChannel = NULL;
    for ( ; channel; channel = CopyFrom->Next(channel))
        if (!channel->GroupSep() /* if favourites no filters */
                && (isFav || (!isFav && globalFilters.IsAllowedChannel(channel))) ) {
            newChannel = new cChannel;
            *newChannel = *channel;
            newChannel->SetNumber(0);
            activeChannelList.Add(newChannel);
        }

    activeChannelList.ReNumber();
    activeChannelList.Save();
    
#endif
    return true;
}


/** ****************** NextAvailableChannel() **********************************
  * find next available channel in active list given pointer to channel
  * in vdr's list
  *
  * return pointer to channel in vdr's list
  * return NULL if
  *      given channel not in active channel list
  *      reached end of active channel list (and !channelwrap)
*******************************************************************************/
#if DONOT_SAVE_ACTIVE_CHANNELS
cChannel* NextAvailableChannel(cChannel *channel, int direction)
{
    cChannels *activeChannels = NULL;
    GlobalFilters *activeFilters = NULL;
    GlobalFilters emptyFilters("empty filters list"); // use this if user wants to zap through all the channels in Fav.
    cChannel* ch = NULL;
    bool forward = (direction == 1);
    
    if (savedWasLastMenuFav) {
        activeChannels = &favourites;
        activeFilters  = &savedFavouritesFilters;
        //activeFilters  = &emptyFilters;
        printf("\033[7;95m Active list: Favourites (name: '%s:%d', direction %d)\033[0m\n",
               channel->Name(), channel->Number(), forward);
    }
    else {
        activeChannels = &Channels;
        activeFilters  = &savedGlobalFilters;
        printf("\033[7;95m Active list: vdr's list (name: '%s:%d', direction %d)\033[0m\n",
               channel->Name(), channel->Number(), forward);
    }
    
    // find the next channels that is available after filtering
    // assume activeChannels contains no duplicates
    
    bool found = false; // find the given channel in the active channel list
    bool foundNext = false;
    static int lastActiveChannelNumber = -1;
    int activeChNumber = 0;

    // find the active channel Number
    for (ch = activeChannels->First(); ch ; ch = activeChannels->Next(ch))
    {
        if (ch->GroupSep() || !activeFilters->IsAllowedChannel(ch))
            continue;
        
        if (!found)
            activeChNumber++;
        
        if (ch->GetChannelID() == channel->GetChannelID()) 
        {
            found = true;
            printf("\033[0;92mfound given channel in active list %d --> %d (given)\033[0m\n",
                   ch->Number(), channel->Number());
            break;
        }
    } // find 
    
    
    // the given channel was not found in our active list
    // therefore we cannot find the next channel as well
    if (!found)
        return NULL;
    
    
    // iterating through the list till a next available/allowed 
    // channel is found in the requested direction -- starting from the 'found' channel
    
    // next step in the requested direction
    ch = forward?activeChannels->Next(ch):activeChannels->Prev(ch);
    
    for (;
         ch; 
         ch = forward?activeChannels->Next(ch):activeChannels->Prev(ch))
    {
        // skip channels that are filtered out and Group seperators
        if (ch->GroupSep() || !activeFilters->IsAllowedChannel(ch))
            continue;

        foundNext = true;
        break; // this is the channel to return
    } // for active list
    
    
    // found the next channel in our active list
    if (foundNext && ch)
        return Channels.GetByChannelID(ch->GetChannelID());
    
    
    printf("\033[7;92mWrapping around to find a channel \033[0m\n");
    
    // found the given channel in our active list, but the next channel was not found
    // lets start from the beginning of the active list and find a channel
    for (ch=forward?activeChannels->First():activeChannels->Last(); 
         ch; 
         ch = forward?activeChannels->Next(ch):activeChannels->Prev(ch))
    {
        if (activeFilters->IsAllowedChannel(ch) && !ch->GroupSep())
            break;
    }
    
    if (ch)
        return Channels.GetByChannelID(ch->GetChannelID());
    else
        return NULL; // this would not happen!
    
    
#if 0   // another implementation
    
    /// wrap around the active channel list once
    // if the current channel is the last channel of active list
    // then we should wrap around the active list and return the 
    // first channel of the active list to vdr
    int wrap_around = 2; 
    
    for ( ; wrap_around && !foundNext; wrap_around-- ) 
    {
        printf("============= Wrap around := %d\n", wrap_around);
        for (ch=forward?activeChannels->First():activeChannels->Last(); 
             ch; 
             ch = forward?activeChannels->Next(ch):activeChannels->Prev(ch))
        {
            if (activeFilters->IsAllowedChannel(ch) && ch->GetChannelID() == channel->GetChannelID()) 
            {
                found = true;
                printf("\033[0;92mfound given channel in active list %d --> %d (given)\033[0m\n",
                       ch->Number(), channel->Number());
            }
            else if (found && activeFilters->IsAllowedChannel(ch) && !ch->GroupSep()) 
            {
                foundNext = true;
                break; // this is the channel to return
            }
        } // for active list
    } // for wrap_around
    
    if (ch && found) {
        printf("\033[0;92m found next channel '%s:%d'\033[0m\n", ch->Name(), ch->Number());
        return Channels.GetByChannelID(ch->GetChannelID());
    }
    printf("\033[0;91m Did not find next channel, given '%s:%d'\033[0m\n", channel->Name(), channel->Number());
    return NULL;
#endif
}

int NextAvailableChannel(int currChannel, int direction)
{
    cChannel* channel = Channels.GetByNumber(currChannel);
    if (!channel) return currChannel;
    
    channel = NextAvailableChannel(channel, direction);
    if (!channel) return currChannel;
    
    return channel->Number();
}

#else
cChannel * NextAvailableChannel(cChannel* currVdrChannel, int direction)
{
    if(!currVdrChannel) {
        printf("param NULL\n");
        return NULL;
    }

    if (!direction) {
        printf("param direction %d\n", direction);
        return currVdrChannel;
    }

    static int lastActiveChannelNumber = -1;

    cChannel* channel = activeChannelList.GetByNumber(lastActiveChannelNumber);

    /* could not find current channel in active list by NUMBER, try by ChannelID*/
    if (!channel || !(channel->GetChannelID() == currVdrChannel->GetChannelID()))
        channel = activeChannelList.GetByChannelID(currVdrChannel->GetChannelID());

    if (!channel) { /* channel not found in active list */
        printf("Channel '%s' not found in active list\n", currVdrChannel->Name());
        return NULL;
    }

    /* find next channel in active list */
    cChannel *nextChannel = direction>0? activeChannelList.Next(channel)
                                           :activeChannelList.Prev(channel);

    if (!nextChannel /*&& Setup.ChannelsWrap*/) {
        printf("====== end of active channels list. Wrapping around ======\n");
        nextChannel = direction>0? activeChannelList.First()
                                 :activeChannelList.Last();
    }

    if (!nextChannel) {
        lastActiveChannelNumber = channel->Number();
        printf("Next channel not found: set lastActiveChannelNumer %d\n", lastActiveChannelNumber);
        return NULL;
    }

    lastActiveChannelNumber = nextChannel->Number();
    printf("Next channel FOUND: set lastActiveChannelNumer %d\n", lastActiveChannelNumber);

    /* return channel with same id in vdr's channel list */
    return Channels.GetByChannelID(nextChannel->GetChannelID());
}

int NextAvailableChannel(int currChannel, int direction)
{
    int channelNumber = -1; // invalid channel number

    cChannel *vdrChannel = Channels.GetByNumber(currChannel);
    cChannel *channel = NULL;

    if (vdrChannel) {
        channel = activeChannelList.GetByChannelID(vdrChannel->GetChannelID());

        if (channel) {/* vdr's channel found in active channel list */
            cChannel *nextChannel = direction>0? activeChannelList.Next(channel)
                                               :activeChannelList.Prev(channel);

            if (!nextChannel /*&& Setup.ChannelsWrap*/) nextChannel = direction>0? activeChannelList.First():activeChannelList.Last();

            if (nextChannel) { /* next channel in active list */
                cChannel *vdrNextChannel = Channels.GetByChannelID(nextChannel->GetChannelID());

                if (vdrNextChannel)
                    channelNumber = vdrNextChannel->Number();
                 else
                    esyslog("next channel in active list not found in vdr's list!");

            } else
                esyslog("active list has no '%s' channel", direction>0?"next":"prev");

        } else
            esyslog("vdr's channel #%d not found in active list", currChannel);

    } else
        esyslog("vdr's channel #%d invalid", currChannel);

    return channelNumber;
}
#endif
