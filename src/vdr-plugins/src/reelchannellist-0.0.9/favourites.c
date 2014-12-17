#include "favourites.h"
#include "menuitem.h"
#include "menu.h"
#include "activelist.h"
#include "MenuCISlot.h"
#include <vdr/interface.h>
#include <vdr/plugin.h>


cChannels favourites;
GlobalFilters favouritesFilters("FavouriteFilters"), savedFavouritesFilters("SavedFavouriteFilters");
#define osRefreshPage osUser2

bool UpdateFavChannelNames()
{
    bool success = false;
    cChannel *ch = favourites.First();
    cChannel *vdrCh = NULL;

    while (ch) {
        if (!ch->GroupSep()) {
            vdrCh = Channels.GetByChannelID(ch->GetChannelID());
            if (vdrCh) {
                if (strcmp(vdrCh->Name(), ch->Name()) != 0) {
                    isyslog("changing name of '%s' fav. channel", ch->Name());
                    ch->SetName(vdrCh->Name(),
                                vdrCh->ShortName()?vdrCh->ShortName():"",
                                vdrCh->Provider()?vdrCh->Provider():"");
                }

                // update Caids if they differ
                ch->ForceCaIds(vdrCh->Caids());
            } else
                esyslog("fav. channel '%s' not found in vdr's channel list!", ch->Name());
        }
        ch = favourites.Next(ch);
    } //while

    return success;
}

/** ********************** AddNewFavFolder *************************************
Add new folder (group sep) to favourites list with

name := folderName
after channel := channel
before channel if 'before' param is true
return true if successful in adding a new folder
*******************************************************************************/
bool AddNewFavFolder(const char* folderName,
                     cChannel* ch,
                     bool before)
{

    if (!folderName) return false;

    /* avoid duplicate bouquets */
    if (IsBouquetInFavourites(folderName)) return false;

    cChannel *channel = new cChannel;
    channel->SetGroupSep(true); /* make it a bouquet */
    channel->SetName(folderName, "", "");
    favourites.Add(channel); /* add bouquet to the end of the list */

    printf("after addition, Fav count: %d\n", favourites.Count());
    return true;
}


/** ************** AddChannelToFavourites **************************************
*** add given channel to a bouquet in favourites
*** makes a copy of the channel
*******************************************************************************/
bool AddChannelToFavourites(const cChannel* ch, const cChannel* favBouquet)
{
    if (!ch || !favBouquet) return false;

    // allow multiple channels in fav. list, but channels inside each bouquets are unique
#if 0
    /* channel already in favourites? */
    if (IsChannelInFavourites(ch))
        return false;
#endif

    printf("Adding channel '%s' to bouquet '%s'\n", ch->Name(),
           favBouquet->Name());

    cChannel* channel = new cChannel;
    // copy channel data
    *channel = *ch;

    bool wasChannelAdded = false;
    cChannel *it = favourites.First();
    for(; it; it = favourites.Next(it)) {
        if (it->GroupSep() && strcmp(favBouquet->Name(), it->Name()) == 0) {

#if 0
            /* add channel at the end of bouquet,
               find next bouquet, subtract 1 from its index
                => last channel of current bouquet */
            int nextBouquetIdx = favourites.GetNextGroup(it->Index());
            cChannel *lastChannelInBouquet = favourites.Get(nextBouquetIdx-1);
#else
            cChannel *lastChannelInBouquet = NULL;

            /* does this bouquet alreday contain the channel ?,
               traverse the bouquet to find out */
            it = favourites.Next(it);
            while (it && !it->GroupSep()) {

                // already contains channel
                if (it->GetChannelID() == ch->GetChannelID()) return false;

                lastChannelInBouquet = it;
                it = favourites.Next(it);
            }
#endif
            /*lastChannelInBouquet can be null, no checks required*/
            favourites.Add(channel, lastChannelInBouquet);
            wasChannelAdded = true;
            break;
        }
    } //

    if (wasChannelAdded) {
        favourites.ReNumber();
        favourites.Save();
    } else
        delete channel;

    return wasChannelAdded;
}

/** ****************************************************************************
*** Add multiple channels to favourites
*** returns false if no channel is added to favourites else returns true
*** Do not add channels that are already present in the bouquet, ignore them
*******************************************************************************/
bool AddChannelsToFavourites(const std::vector<int> &channelNumbers, const cChannel
*favBouquet)
{
    if (!favBouquet) {
        esyslog("%s: no favourites bouquet provided", __PRETTY_FUNCTION__);
        return false;
    }

    int count = 0;
    cChannel *channel = NULL;

    for (std::vector<int>::size_type i=0; i < channelNumbers.size(); i++) {
        channel = Channels.GetByNumber(channelNumbers.at(i));

        if (channel && AddChannelToFavourites(channel, favBouquet))
            count++;
    } // for

    isyslog("%d channels added to favourite bouquet '%s'",
            count, favBouquet->Name());

    return count > 0;
}


/**  ********** AddProviderToFavourites *****************************************
*** Add channels that are provided by a single provider 'providerName'
*** to favourites list as a new folder.
********************************************************************************/
bool AddProviderToFavourites(const char* providerName)
{
    if (IsBouquetInFavourites(providerName))
        return false; // bouquet name already present

    bool flag = AddNewFavFolder(providerName);
    if (!flag) // error, not added
        return false;

    cChannel *favBouquet = IsBouquetInFavourites(providerName);

    cChannelProviderFilter *filter = new cChannelProviderFilter;
    if (!filter) return false;

    filter->SetFilterParams(providerName);

    cChannel *channel = Channels.First();
    // add all channels
    while(channel) {
        if (globalFilters.IsAllowedChannel(channel) && filter->IsAllowed(channel))
            flag = AddChannelToFavourites(channel, favBouquet);

        channel = Channels.Next(channel);
    }

    return flag;
}

/** **************** IsChannelInFavourites *************************************
  * returns channel if it is in favourites or NULL
  * to be used when adding channel to favourites (avoid duplicates)
*******************************************************************************/
cChannel* IsChannelInFavourites(const cChannel* ch)
{
    if (!ch) return NULL;

    return favourites.GetByChannelID(ch->GetChannelID());
}


/** ******************** DeleteChannelsFromFavourites **************************
  * Delete all copies of one given channel from ALL favourites folders
*******************************************************************************/
bool DeleteChannelFromFavourites(cChannel *ch) {

    if (!ch) return false;

    bool result = false;
    // delete all channels with this channel id
    tChannelID Id = ch->GetChannelID();

    cChannel *channel = favourites.First();
    cChannel *nextChannel = NULL;

    while (channel) {
        nextChannel = favourites.Next(channel);

        // found same channel id
        if (channel->GetChannelID() == Id) {
            favourites.Del(channel);
            result = true;
        } // if

        channel = nextChannel;
    } // while

    return result;
}

/** *************** DeleteChannelsFromFavourites *******************************
  * deletes channels from *ALL* favourite folders (as duplication is possible)
*******************************************************************************/
bool DeleteChannelsFromFavourites(const std::vector<int> &chList)
{
    bool result = false;
    cChannel *ch = NULL;
    for (std::vector<int>::size_type i=0; i < chList.size(); ++i) {
        ch = Channels.GetByNumber(chList.at(i));
        result |= DeleteChannelFromFavourites(ch);
    } // for

    return result;
}

/** **************** BouquetInFavourites ***************************************
  * returns bouquet/GroupSep of given name or NULL
  * to be used when creating a new bouquet (avoid duplicate bouquet names)
*******************************************************************************/
cChannel* IsBouquetInFavourites(const char* bouquetName)
{
    if (!bouquetName) return NULL;

    cChannel *channel = favourites.First();
    for ( ; channel; channel = favourites.Next(channel)) {
        if (channel->Name() && strcmp(channel->Name(), bouquetName) == 0)
            return channel;
    } // for

    return NULL; /* not found */
}

//
// returns the fav. folder of current channel
// with @param filtered=false returns the first occurance of the channel
// if filtered=true, the last saved bouquet is found
// can be NULL
cChannel* FavCurrentBouquet(bool filtered) {
    // get the current channel
    cChannel *currChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
    if (!currChannel) return NULL;
    tChannelID id = currChannel->GetChannelID();
    
    bool found = false; // true if current channel found the favourites list
    
    // hold the bouquet name while running through the favourites list
    // it is to be returned if current channel is found
    cChannel *bouquet = NULL; 
    
    cChannel* ch=favourites.First();
    for(; ch ; ch = favourites.Next(ch)) {
        if (ch->GroupSep()) bouquet = ch;
        
        if (filtered && !savedFavouritesFilters.IsAllowedChannel(ch))
            continue;
        
        if (id == ch->GetChannelID()) {
            found = true;
            break;
        }
    }
    
    return found ? bouquet:NULL;
    
    //TBD:: If !found, search through the favourite list unfiltered ?
}

/// cMenuFavourites ------------------------------------------------------------
cString cMenuFavourites::lastSelectedFolder;
cMenuFavourites::cMenuFavourites(bool startWithCurrChannelBouquet):
    cOsdMenu(tr("Favourites")),
    mode(eFavFolders), lastPosition(-1)
{
    //bool flag = favourites.Load("/tmp/favs.conf");
    printf("(%s:%d) Fav count: %d\n", __FILE__,
           __LINE__, favourites.Count());

    if (startWithCurrChannelBouquet) {
        // find bouquet of current channel, if current channel exists in fav. list
        cChannel *bouquet = FavCurrentBouquet();

        if(bouquet) {
            printf("clearing %d filters\n",favouritesFilters.channelFilters.Count());
            //bouquet of current channel found
            favouritesFilters.ClearFilters();
            favouritesFilters.AddBouquetFilter(bouquet->Name(), true);
            printf("after adding '%s' bouquet filter : %d filters\n",
                   bouquet->Name(),
                   favouritesFilters.channelFilters.Count());
        }

    }

    // Update favourite channel names and channel-CaIDs (CI slot allocation)
    UpdateFavChannelNames();

    // if filters present assume view=channel's view
    if (favouritesFilters.channelFilters.Count())
        mode = eFavChannels;
    else
        mode = eFavFolders;

    wasLastMenuFav = true;


#if REELVDR && APIVERSNUM >= 10716
    EnableSideNote(true);
#endif
    // show channels/favourites
    Set();
}


cMenuFavourites::~cMenuFavourites()
{
    if (!memoryOn) {
        favouritesFilters.ClearFilters();
        wasLastMenuFav = false;

    }
    printf("save favourites\n");
    favourites.Save();
}

void cMenuFavourites::Set()
{
    Clear();

    SetCols(18, 5); // same tab size as channellist menu
    cChannel *currChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
    tChannelID channelId;
    if (lastPosition < 0 && currChannel) channelId = currChannel->GetChannelID();
    cChannel *channel = favourites.First();
    cOsdItem *item = NULL;
    bool currentSet = false;

    for ( ; channel; channel = favourites.Next(channel)) {
        if (mode == eFavFolders) {
            // show only fav folder names
            if (channel->GroupSep()) {
                Add(item = new cOsdChannelItem(channel, true));

                if (*lastSelectedFolder && strcmp(channel->Name(), *lastSelectedFolder)==0) {
                    SetCurrent(item);
                    currentSet = true; // don't override this current-item below
                }
            }
        } else if (favouritesFilters.IsAllowedChannel(channel)) {
            Add(item = new cOsdChannelItem(channel, true));

            if (item && channel->GetChannelID() == channelId) {
                SetCurrent(item);
                currentSet = true;
            }
        }
    }// for

    // lets not have empty menus
    if (Count()==0) { // no osditems
        if (mode == eFavFolders) {
            AddFloatingText(tr("No favourite folders available"), 50);
            Add(new cOsdItem("",osUnknown, false)); // blank line
            Add(new cOsdItem("",osUnknown, false)); // blank line
            Add(new cOsdItem("",osUnknown, false)); // blank line
            Add(new cOsdItem("",osUnknown, false)); // blank line
            Add(new cOsdItem(tr("Info:"), osUnknown, false));
            AddFloatingText(tr("Please select 'all Channels' to add channels to your favorites."), 50);
       } else {
            AddFloatingText(tr("No channels in this bouquet"), 50);
    }
 } else {
        if (!currentSet) {
            if (lastPosition >= Count())
                lastPosition = Count() - 1;
            SetCurrent(Get(lastPosition));
        }
    }


    cString tmpTitle = favouritesFilters.MakeTitle();
    if (*tmpTitle)
        tmpTitle = cString::sprintf("%s: %s/%s", tr("Channel List"),
                                    tr(FAVOURITES), *tmpTitle);
    else
        tmpTitle = cString::sprintf("%s: %s", tr("Channel List"), tr(FAVOURITES));

    SetTitle(*tmpTitle);
    Display();
    SetHelp(tr(ALL_CHANNELS), tr(SOURCES), tr(FAVOURITES), tr("Functions"));

}


bool SwitchToFavChannel(cChannel *favCh)
{
    if (!favCh) return false;

#if 0 // does not work as expected
    return favourites.SwitchTo(favCh->Number());
#endif

    /**
      switch to channel
        find selected channel (in fav) in global vdr-channellist
     **/

    bool success = false; /* switched to channel? */

    /* find the channel in vdr's channel list */
    cChannel *ch = Channels.GetByChannelID(favCh->GetChannelID());

    // try to switch to channel
    if (ch)
        success = Channels.SwitchTo(ch->Number());
    else
        esyslog("fav channel '%d %s' not found in vdr's list", favCh->Number(),
                favCh->Name());

    return success;
}

void cMenuFavourites::ShowPrevFolder()
{
    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
    cChannel *currBouquet = NULL;
    if (currItem) {
        currBouquet = currItem->Channel();
        // find current bouquet
        while(currBouquet && !currBouquet->GroupSep())
            currBouquet = favourites.Prev(currBouquet);
    } else
    {
        // try to get bouquet/folder name from filter list
        cChannelBouquetFilter *filter =
                favouritesFilters.GetLastFilter<cChannelBouquetFilter>();

        if (filter) {
            // printf("bouquet filter: '%s'\n", *filter->ParamsToString());
            currBouquet = IsBouquetInFavourites(*filter->ParamsToString());

            if (currBouquet) {
                // printf("Got bouquet! %s\n", currBouquet->Name());
                // code below expects the current bouquet
            } else {
                // no such bouquet found, give up and return. Do nothing.
                printf("no such bouquet %s\n", *filter->ParamsToString());
                return;
            }

            // code below expects to encounter 2 bouquets
            // current and previous.
            // So, let 'ch' be current bouquet

        } else {
            // no bouquet filter found, do nothing.
            printf("bouquet filter not found\n");
            return;
        }
    } // !currItem


    // skip current bouquet and go to the previous bouquet : in all 2 bouquets
    while(currBouquet) {
        currBouquet = favourites.Prev(currBouquet);
        if (currBouquet && currBouquet->GroupSep()) break;
    }

    if (currBouquet && currBouquet->GroupSep()) {
        favouritesFilters.ClearFilters();
        favouritesFilters.AddBouquetFilter(currBouquet->Name(), true);
        lastPosition = -1; // reset position of last selected item
        Set();
    }

}

void cMenuFavourites::ShowNextFolder()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
    cChannel *ch = NULL; // to point to a channel in current bouquet
    if (currItem) {
        ch = currItem->Channel();
    } else
    {
        // try to get bouquet/folder name from filter list
        cChannelBouquetFilter *filter =
                favouritesFilters.GetLastFilter<cChannelBouquetFilter>();

        if (filter) {
            printf("bouquet filter: '%s'\n", *filter->ParamsToString());
            ch = IsBouquetInFavourites(*filter->ParamsToString());

            if (ch) {
                printf("Got bouquet! %s\n", ch->Name());
                // code below expects a channel inside current bouquet
                // (note the next channel maybe the next bouquet)
                ch = favourites.Next(ch);
            } else {
                // no such bouquet found, give up and return. Do nothing.
                printf("no such bouquet %s\n", *filter->ParamsToString());
                return;
            }

        } else {
            // no bouquet filter found, do nothing.
            printf("bouquet filter not found\n");
            return;
        }
    }


    while(ch && !ch->GroupSep()) ch = favourites.Next(ch);

    if (ch && ch->GroupSep()) {
        favouritesFilters.ClearFilters();
        favouritesFilters.AddBouquetFilter(ch->Name(), true);
        lastPosition = -1; // reset position of last selected item
        Set();
    }
}

eOSState cMenuFavourites::ProcessKey(eKeys Key)
{
#if 1
    /* exit key & yellow key goes to "previous" menu, ie. list of all fav. folders */
    if (!HasSubMenu() && mode == eFavChannels && (NORMALKEY(Key) == kBack || NORMALKEY(Key) == kYellow)) {
        mode = eFavFolders;
        favouritesFilters.ClearFilters();
        lastPosition = -1; // reset position of last selected item
        Set();
        return osContinue;
    }
#endif

    /* navigate folders with :
          Left and Right keys : if 'cursor' is on first/last item on osd
          OR
          with Channel up/down (unconditional jump to prev/next fav. folder)
     */
    if (!HasSubMenu() && mode == eFavChannels) {
        bool isFirstItem = (Current() == 0);
        bool isLastItem  =  (Current() == Count()-1);

        if ( (kLeft == NORMALKEY(Key) && isFirstItem) || kChanDn == NORMALKEY(Key)) {
            ShowPrevFolder();
            return osContinue;
        }
        else if ( (kRight == NORMALKEY(Key) && isLastItem) || kChanUp == NORMALKEY(Key)) {
            ShowNextFolder();
            return osContinue;
        }
    }

    eOSState state = cOsdMenu::ProcessKey(Key);

    // refresh page
    if (state == osRefreshPage) {
        /* NOTE: call Set() before CloseSubMenu()
            CloseSubMenu() calls RefreshCurrent() to redraw the current item
            which might have just been deleted!
          Therefore, redraw updated list and then call CloseSubMenu() */
        Set();
        CloseSubMenu();

        state = osContinue;
    } // refresh page

    // show side note info if no submenu and key has been handled
    if (!HasSubMenu() && state != osUnknown)
        ShowSideNoteInfo();

    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
    if (state == osUnknown) {
        switch (Key) {
        case kOk:
            /* switching to channel successful? change state to osEnd */
            state = osContinue;

            if (currItem && currItem->Channel()) {
                if (mode == eFavFolders) {
                    // open fav folder
                    favouritesFilters.AddBouquetFilter(currItem->Channel()->Name(), true);
                    lastSelectedFolder = currItem->Channel()->Name();
                    //return AddSubMenu(new cMenuFavourites);
                    mode = eFavChannels;
                    lastPosition = -1; // reset position of last selected item
                    Set();
                } else {
                    bool success = SwitchToFavChannel(currItem->Channel());

                    if (success) {
                        state = osEnd; // switched to channel so close menu
                        memoryOn = true;
                        SaveFilteredChannelList(true);
                    }
                    else
                        Skins.Message(mtError, tr("Switching to channel failed"));
                } // else
            } // if (currItem && currItem->Channel())
            break;

        case kBlue:
            if (currItem && currItem->Channel())
                return AddSubMenu(new cMenuFavouritesFunction(currItem->Channel()));
            break;
        default:
            break;
        } // switch
    } // if state == osUnknown

    if (state != osUnknown && !HasSubMenu() && currItem && mode == eFavChannels)
    {
        lastPosition = Current(); // update position of last selected fav. channel
    }

    // ignore color keys when this menu has submenus,
    // else vdr opens Macrobinding for these keys
    if (HasSubMenu() && state == osUnknown)
        switch (NORMALKEY(Key))
        {
        case kRed:
        case kGreen:
        case kYellow:
        case kBlue:
            return osContinue;
        default:
            break;
        }


    // Handle color keys
    if (state == osUnknown)
    switch(Key) {

    case kRed: // no filter, all channels
        globalFilters.ClearFilters();
        return AddSubMenu(new cMenuChannelList);
        break;

    case kGreen: // Sources
        globalFilters.ClearFilters();
        return AddSubMenu(new cMenuSourceList);

        break;

    case kBlue:
        break; // blue is now function menu, ignore this key here

    case kYellow: // reset Fav listing, show fav. folders list
        if (mode != eFavFolders) {
            favouritesFilters.ClearFilters();
            mode = eFavFolders;
            lastPosition = -1; // reset position of last selected item
            Set();
        }
        state = osContinue;
        break;

    default:
        break;
    } // switch for colorkeys

    return state;
}

void cMenuFavourites::Display()
{
    cOsdMenu::Display();
    ShowSideNoteInfo();
}

void cMenuFavourites::ShowSideNoteInfo()
{
#if REELVDR && APIVERSNUM >= 10716
    // currently selected item
    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*>(Get(Current()));

    if (currItem)
    {
        const cEvent *event = currItem->PresentEvent();
        SideNote(event);
        const cChannel *channel = currItem->Channel();

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

        if (Setup.PreviewVideos && channel)  {
            /* get channel number in vdr's list*/
            cChannel* vdrChannel = Channels.GetByChannelID(channel->GetChannelID());
            if (vdrChannel) {
                int number = vdrChannel->Number();
                cPluginManager::CallAllServices("start preview channel", &number);
            } // if vdrChannel
            else cPluginManager::CallAllServices("stop preview channel");

        } else
            cPluginManager::CallAllServices("stop preview channel");

    }
#endif /*REELVDR && APIVERSNUM >= 10716*/
}




/** ************** cMenuCreateFavouritesFolder *********************************
*******************************************************************************/
cMenuCreateFavouritesFolder::cMenuCreateFavouritesFolder()
    : cOsdMenu(trVDR("Functions: Create new Favourite folder"))
{
    Set();

    // start in editmode
    cRemote::Put(kRight, true);
}

void cMenuCreateFavouritesFolder::Set()
{
    Clear();
    SetCols(15);

    // clear name
    folderName[0] = 0;

    AddFloatingText(tr("Enter new favourites folder name"), 50);
    Add(new cOsdItem("",osUnknown, false));
    Add(new cMenuEditStrItem(tr("Folder name"), folderName, sizeof(folderName)-1));

    Display();
}


eOSState cMenuCreateFavouritesFolder::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    printf("state = %d key = %d\n", state, Key);
    if (state == osUnknown) {
        switch (Key) {
        case kOk:
            state = osContinue;

            if(strlen(folderName) > 0) {
                if (IsBouquetInFavourites(folderName)) {
                    Skins.Message(mtError, tr("Duplicate folder name. Please try another name."), 2);
                } else if (AddNewFavFolder(folderName))
                    state = osBack;
                else
                    esyslog("%s:%d '%s' not added to fav.", __FILE__, __LINE__, folderName);
            }
            else // name not set, empty string
                Skins.Message(mtError, tr("Invalid folder name"));

            break;

        default:
            break;
        } // switch
    } //if

    return state;
}




/** ****************** cMenuAddChannelToFavourites *****************************
*** Select a bouquet and add give channel to favourites list
*** if no bouquet is available create a new bouquet
*******************************************************************************/
cMenuAddChannelToFavourites::cMenuAddChannelToFavourites(std::vector<int> channels, bool standAlone_)
    : cOsdMenu(trVDR("Functions: Select favourite folder")), channelsToAdd(channels), standAlone(standAlone_)
{
    Set();
}


int cMenuAddChannelToFavourites::CountFavFolders()
{
    int count = 0;

    cChannel *ch = favourites.First();
    for ( ; ch; ch = favourites.Next(ch)) {
        if (ch->GroupSep())
            ++count;
    } // for

    return count;
}


void cMenuAddChannelToFavourites::Set(bool selectLast)
{
    Clear();
    SetCols(6);

    if (CountFavFolders() == 0)
        AddFloatingText(tr("You have no favourite folders. Please create one before you can add the channel"), 50);
     else
        AddFloatingText (cString::sprintf(tr("Select folder for %d channels"),
                                          channelsToAdd.size())
                         , 50);

    Add(new cOsdItem("", osUnknown, false));


    // Give user chance to add new folder
    Add(new cOsdItem(*cString::sprintf("%c\t%s", char(132), tr("Create new folder"))));

    /* if folder available, differentiate them from the above option */
    if (CountFavFolders() != 0) {
        Add(new cOsdItem("", osUnknown, false));
    }

    cOsdChannelItem *firstFolderItem = NULL, *bouquetItem = NULL;

    // count the number of to-be-added channels that are in a bouquet
    // if it is equal to the channelsToAdd.size() then the bouquet contains
    // all the to-be-added channels, so the bouquet should be marked as unavailable
    unsigned int count_repeats = 0;

    // show all 'bouquets' from favourite list
    cChannel *ch = favourites.First();
    for ( ; ch; ch = favourites.Next(ch)) {
        if (ch->GroupSep()) {

            // all channels to be added were in the previous bouquet, so
            // the bouquet should be unselectable
            if (bouquetItem && count_repeats == channelsToAdd.size()) {
                // if this bouquet was the current item
                // now it no longer is.
                if (firstFolderItem == bouquetItem)
                    firstFolderItem = NULL;

                // bouquet not selectable
                bouquetItem->SetForbidden(true);
                bouquetItem->Set();
            } // if (all channels in prev bouquet)

            bouquetItem = new cOsdChannelItem(ch, true);
            Add(bouquetItem);
            count_repeats = 0;

            if (!firstFolderItem)
                firstFolderItem = bouquetItem;
        } else {
            if (bouquetItem) {
                tChannelID channelID = ch->GetChannelID();
                // check if any to-be-added channel is present in the current bouquet
                // if so, increment count_repeats
                for(unsigned int i=0; i< channelsToAdd.size(); ++i) {
                    cChannel *channel = Channels.GetByNumber(channelsToAdd.at(i));
                    if (channel && channel->GetChannelID() == channelID) {
                        count_repeats ++;
                        break;
                    } // if
                } // for
            } // if bouquetItem
        } // else
    } // for

    /* last entry is the newly created folder */
    if (selectLast) {
        SetCurrent(Get(Count()-1));
    }
    else
        SetCurrent(firstFolderItem);

    Display();

    SetStatus(tr("Select folder with 'Ok'"));
}


/**
 Move favourite bouquet 'From' to position before bouquet 'To'
 if 'To' is NULL, move 'From' bouquet to the end of list

 MoveFavBouquet's 'To' parameter indicates the place *BEFORE* which
 'From' bouquet has to be moved. EXCEPT when To == NULL then move the 'From'
 bouquet to the end of the favourites list.

 cListBase::Move() does not always move before 'To'!
 When 'From' is before 'To' in the list, cListBase::Move() moves 'From'
 to _after_ 'To'. We do not want that.

 So, check the positions of 'From' and 'To' and if 'From' is before 'To' in the
 list then accordingly change 'To' (move one step up the list) so that move
 _after_ the changed 'To' results in the expected behaviour of moving after
 given parameter (ie. original) 'To'.
*/
bool MoveFavBouquet(cChannel *From, cChannel *To, bool before=true)
{
    if (!before || !From) return false;

    printf("From Idx: %d,  Name: %s\n", From->Index(), From->Name());
    if(To)
        printf("To Idx: %d, Name: %s\n", To->Index(), To->Name());
    else
        printf("To NULL\n");

    cChannel *ch = favourites.Next(From);
    cChannel *n = NULL;

    // does Move() move 'From' after 'To' ?
    bool movesAfter = (To && From->Index() < To->Index());
    // but we want before always!, so lets go one up to move before 'To'

    // move 'From'
    favourites.Move(From, movesAfter?favourites.Prev(To):(To?To:favourites.Last()));

    // move all channels till next bouquet
    for ( ; ch && !ch->GroupSep(); ch = n) {
        n = favourites.Next(ch);
        favourites.Move(ch, movesAfter?favourites.Prev(To):(To?To:favourites.Last()));
    }

    return true;
}

bool MoveFavChannel(cChannel *From, cChannel *To, bool before=true)
{
    if (!before || !From) return false;

    int toIdx = favourites.Last()->Index();

    // find the end of the current bouquet (containing 'From' channel)
    if (!To) {
        toIdx = favourites.GetNextGroup(From->Index())-1;

        if (toIdx < 0)
            toIdx = favourites.Last()->Index();
    } else {
        toIdx = To->Index();

        if (From->Index() < To->Index())
            toIdx --; // go back one step
    }

    favourites.Move(From->Index(), toIdx);

    return true;
}

eOSState cMenuAddChannelToFavourites::ProcessKey(eKeys Key)
{
    bool hadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    /* close sub menus if channel list is requested */
    if (state == osShowChannelList) {
        CloseSubMenu();
        return state;
    }

    /* redrawn menu, if submenu was closed (create new folder menu adds new folder entries)*/
    if (hadSubMenu && !HasSubMenu()) {
        Set(true); // param true selects the last entry ie. newly created folder
    }

    if (state == osUnknown) {
        switch(Key) {
        case kOk:
        {
            const char* text = Get(Current())->Text();
            if (text && strstr(text, tr("Create new folder")))
                return AddSubMenu(new cMenuCreateFavouritesFolder);

            cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
            if (currItem && currItem->Channel()) {
                bool success = AddChannelsToFavourites(channelsToAdd, currItem->Channel());

                if (success)  // added
                    Skins.Message(mtInfo, tr("Channel added to favourites") , 1);
                else
                    Skins.Message(mtError, tr("Channel not added to favourites") , 1);

                cMenuChannelList::ClearChannelSelectMode();
                return standAlone?osBack:osShowChannelList;
            } // if fav bouquet available
        }
        break;

        case kRed:
        case kBlue:
        case kGreen:
        case kYellow:
            if (standAlone) // ignore colour keys when in standalone mode
                state = osContinue;
            break;

        default:
            break;
        } // switch
    } // if state == osUnknown

    return state;
}






/** **************** cMenuFavouritesFunction ***********************************
*** Menu to offer functions on favourites menu
*******************************************************************************/
cMenuFavouritesFunction::cMenuFavouritesFunction(cChannel* chan) :
    cOsdMenu(trVDR("Functions: Favourites")),
    channel(chan)
{
    Set();
}

void cMenuFavouritesFunction::Set()
{
    Clear();
    SetHasHotkeys();
/*
    cString note_text = cString::sprintf("%s '%s', %s",
                                         channel && channel->GroupSep() ?  tr("For folder") : tr("For channel"),
                                         channel?channel->Name():"undefined",
                                         tr("please choose one of the following options")
                                         );
*/
    cString note_text;
    if (channel && channel->GroupSep())
        note_text = cString::sprintf(tr("For folder '%s' please choose one of the following options:"),
                                         channel ? channel->Name() : "undefined"
                                         );
    else
        note_text = cString::sprintf(tr("For channel '%s' please choose one of the following options:"),
                                         channel ? channel->Name() : "undefined"
                                         );

    AddFloatingText(*note_text, 45);
    Add(new cOsdItem("",osUnknown, false)); // blank line
    // Add(new cOsdItem("",osUnknown, false)); // blank line

    //Add(new cOsdItem(hk(tr("Create new favourites folder"))));
    //Add(new cOsdItem(hk(tr("Move selected favourites"))));
    if (channel) {
        cString  tmp;
        if (channel->GroupSep()) {
            // selected is a fav. folder/bouquet
            //tmp = cString::sprintf("%s: '%s'", tr("Rename folder"), channel->Name());
            tmp = tr("Rename folder");
            Add(new cOsdItem(hk(*tmp)));

            //tmp = cString::sprintf("%s: '%s'", tr("Delete folder"), channel->Name());
            tmp = tr("Delete folder");
            Add(new cOsdItem(hk(*tmp)));

            //tmp = cString::sprintf("%s: '%s'", tr("Move folder"), channel->Name());
            tmp = tr("Move folder");
            Add(new cOsdItem(hk(*tmp)));

            //tmp = cString::sprintf("%s: '%s'", tr("CI-slot assignment for folder"),
            //                        channel->Name());
            tmp = tr("CI-slot assignment for folder");
            Add(new cOsdItem(hk(*tmp)));
        }
        else {
            // selected is a channel
            //tmp = cString::sprintf("%s: '%s'", tr("Move channel"), channel->Name());
            tmp = tr("Move channel");
            Add(new cOsdItem(hk(*tmp)));

            //tmp = cString::sprintf("%s: '%s'", tr("Delete channel"), channel->Name());
            tmp = tr("Delete channel");
            Add(new cOsdItem(hk(*tmp)));

            //tmp = cString::sprintf("%s: '%s'", tr("CI-slot assignment for channel"),
            //                       channel->Name());
            tmp = tr("CI-slot assignment for channel");
            Add(new cOsdItem(hk(*tmp)));
        }
    } // if

    Display();
}

bool DelFavFolder(cChannel* bouquet)
{
    if (!bouquet) {
        printf("Param NULL\n");
        return false;
    }

    //
    //cChannel *tobeDel = favourites.GetByChannelID(bouquet->GetChannelID());
    cChannel *tobeDel = bouquet;

    /* no such bouqet found OR is not a bouquet*/
    if (!tobeDel) {
        printf("no such bouquet found\n");
        return false;
    }
    if (!tobeDel->GroupSep()) {
        printf("param not a bouquet. Not deleting\n");
        return false;
    }

    /* delete till next bouquet */
    int delTill = favourites.GetNextGroup(tobeDel->Index());
    cChannel *end = favourites.Get(delTill);
    printf("delete till: GroupSel? %d\n", end?end->GroupSep():-1);

    cChannel *next = favourites.Next(tobeDel);

    while (tobeDel) {
        if (tobeDel == end) break;
        next = favourites.Next(tobeDel);
        favourites.Del(tobeDel);
        tobeDel = next;
    }

    favourites.ReNumber();
    return true;
}

bool DelFavChannel(cChannel* channel)
{
    if (!channel) return false;

    cChannel *tobeDel = favourites.GetByChannelID(channel->GetChannelID());
    /* channel not found or channel is bouquet*/
    if (!tobeDel ||tobeDel->GroupSep()) return false;

    printf("before Del channel favourites.Count() := %d\n", favourites.Count());
#if 0
    favourites.Del(tobeDel);
#else
    favourites.Del(channel);
#endif
    printf("After Del channel favourites.Count() := %d\n", favourites.Count());
    favourites.ReNumber();
    return true;
}


eOSState cMenuFavouritesFunction::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    const char* text = Get(Current())?Get(Current())->Text():NULL;
    if (state == osUnknown) {
        switch(Key) {
        case kOk:
            state = osContinue;
            if (text) {
                printf("Got text : '%s'\n", text);

                if (strstr(text, tr("Delete channel"))) {
                    printf("Deleting channel\n");
                    if (Interface->Confirm(tr("Delete channel from favourites?"), 3)) {
                        if (DelFavChannel(channel)) {
                            Skins.Message(mtInfo, tr("Channel deleted"));
                            state = osRefreshPage;
                        } else
                            Skins.Message(mtError, tr("Deletion of channel failed"));
                    } else
                        Skins.Message(mtInfo, tr("Delete cancelled"));

                } else if (strstr(text, tr("Delete folder"))) {
                    if (Interface->Confirm(tr("Delete folder and all its channels?"), 3)) {
                        // delete all channels in folder including folder
                        if (DelFavFolder(channel)) {
                            Skins.Message(mtInfo, tr("Folder and its contents deleted"));
                            state = osRefreshPage;
                        } else
                            Skins.Message(mtError, tr("Deletion of folder failed"));
                    } else
                        Skins.Message(mtInfo, tr("Delete cancelled"));

                } else if (strstr(text, tr("Rename folder")))
                    return AddSubMenu(new cMenuRenameFavFolder(channel));
                else if (strstr(text, tr("Move folder")))
                    return AddSubMenu(new cMenuMoveFavFolder(channel));
                else if (strstr(text, tr("Move channel")))
                    return AddSubMenu(new cMenuMoveChannelInFavBouquet(channel));
                else if (strstr(text, tr("CI-slot assignment for folder")))
                    return AddSubMenu(new cOsdMenuCISlot(channel->Name(), true));
                else if (strstr(text, tr("CI-slot assignment for channel")))
                    return AddSubMenu(new cOsdMenuCISlot(channel->Name(), true, channel));
            }

            break;

        default:
            break;
        } // switch
    } //if

    return state;
}




cMenuRenameFavFolder::cMenuRenameFavFolder(cChannel *chan)
    : cOsdMenu(trVDR("Functions: Rename favourite folder")), channel(chan)
{
    strncpy(folderName, channel->Name(), sizeof(folderName));
    Set();
}

void cMenuRenameFavFolder::Set()
{
    Clear();
    SetCols(20);

    AddFloatingText(tr("Rename favourites folder"), 50);
   // Add(new cOsdItem("", osUnknown, false)); //empty line

    Add(new cMenuEditStrItem(tr("New folder name"), folderName, sizeof(folderName)));


    for (int i=0; i<3; ++i)
        Add(new cOsdItem("",osUnknown, false)); //blank lines

    Add(new cOsdItem(tr("Info:"),osUnknown, false));
    AddFloatingText(tr("Press right key to enter folder name and Ok to rename."), 50);

    Display();
}

eOSState cMenuRenameFavFolder::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if(state == osUnknown)
        switch (Key) {
        case kOk:
            if (RenameChannel())
                Skins.Message(mtInfo, tr("Renamed folder"));
            else
                Skins.Message(mtError, tr("Rename failed"));

            state = osRefreshPage;

            break;

        default:
            break;
        } // switch

    return state;
}

bool cMenuRenameFavFolder::RenameChannel()
{
    if (strlen(folderName)<=0) return false;

    // check for folders with same name;
    // also checks for no change in folder name, incidentally
    if (IsBouquetInFavourites(folderName)) return false;

    // channel is not bouquet
    if (!channel || !channel->GroupSep()) return false;

    channel->SetName(folderName, "", "");
    return true;
}


cMenuMoveFavFolder::cMenuMoveFavFolder(cChannel *chan)
    : cOsdMenu(trVDR("Functions: Move favourites bouquet")), channel(chan)
{
    Set();
}

/* Display all folders in favourites list,
   if 'channel' is !NULL then in move mode else in select mode
*/
void cMenuMoveFavFolder::Set()
{
    Clear();

    cChannel *ch = favourites.First();

    for (; ch; ch = favourites.Next(ch))
        if (ch->GroupSep()) {
            cOsdChannelItem *item = new cOsdChannelItem(ch, true, channel == ch);
            Add(item);
            if (channel == ch)
                SetCurrent(item);
        }

    Display();
}

eOSState cMenuMoveFavFolder::ProcessKey(eKeys Key)
{

    eOSState state = osUnknown;

    if (channel)
        switch (Key) {
        case kBack:
            if (channel) {
                channel = NULL;
                Set();
                state = osContinue;
            }
            break;

        case kLeft:
        case kLeft|k_Repeat:
        case kRight:
        case kRight|k_Repeat:
            // ignore PageUp and PageDown  keys for now
            state = osContinue;
            break;

        case kUp|k_Repeat:
        case kUp: {
            cOsdItem *item = Get(Current());
            if (!item) break;

            int currIdx = item->Index();
            Move(currIdx, currIdx-1);
            printf("Move %d -> %d\n", currIdx, currIdx -1);
            SetCurrent(item);

            Display(); // not set
            state = osContinue;
        }
        break;

        case kDown|k_Repeat:
        case kDown: {
            cOsdItem *item = Get(Current());
            if (!item) break;

            int currIdx = item->Index();
            Move(currIdx, currIdx+1);
            printf("Move %d -> %d\n", currIdx, currIdx +1);

            SetCurrent(item);

            Display(); // not set
            state = osContinue;
        }
        break;

        case kOk: {
            cOsdItem *item = Get(Current());
            cOsdItem *nextItem = Get(Current()+1);
            cOsdChannelItem *From = dynamic_cast<cOsdChannelItem*> (item);
            cOsdChannelItem *To = dynamic_cast<cOsdChannelItem*> (nextItem);

            MoveFavBouquet(From->Channel(),To?To->Channel():NULL);
            state = osRefreshPage;
        }
            break;

        default:
            break;
        }

    if (state == osUnknown)
        state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
        switch (Key)
        {
        case kOk:
        {
            cOsdChannelItem *chItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
            if (chItem && chItem->Channel() && !channel) {
                channel = chItem->Channel();
                chItem->SetMarked(true);
                RefreshCurrent();
                Display();
            }
        }
        state = osContinue;
        break;

        default:
            break;
        } // switch

    } // if

    return state;
}

/** ****************** cMenuMoveChannelInFavBouquet ****************************
*** Select a channel in bouquet and change its position within the bouquet
*******************************************************************************/
cMenuMoveChannelInFavBouquet::cMenuMoveChannelInFavBouquet(cChannel *ch)
    :cOsdMenu(trVDR("Functions: Move Channel")), channel(ch)
{
    Set();
}


void cMenuMoveChannelInFavBouquet::Set()
{
    Clear();
    SetCols(18, 5); // same tab size as channellist menu

    cChannel *ch = favourites.First();

    for (; ch; ch = favourites.Next(ch))
        if (!ch->GroupSep() && favouritesFilters.IsAllowedChannel(ch)) {
            cOsdChannelItem *item = new cOsdChannelItem(ch, true, channel == ch);
            Add(item);
            if (channel == ch)
                SetCurrent(item);
        }

    Display();
}


eOSState cMenuMoveChannelInFavBouquet::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;

    if (channel)
        switch (Key) {
        case kBack:
            if (channel) {
                channel = NULL;
                Set();
                state = osContinue;
            }
            break;

        case kLeft:
        case kLeft|k_Repeat:
        case kRight:
        case kRight|k_Repeat:
            // ignore PageUp and PageDown  keys for now
            state = osContinue;
            break;

        case kUp|k_Repeat:
        case kUp: {
            cOsdItem *item = Get(Current());
            if (!item) break;

            int currIdx = item->Index();
            Move(currIdx, currIdx-1);
            printf("Move %d -> %d\n", currIdx, currIdx -1);
            SetCurrent(item);

            Display(); // not set
            state = osContinue;
        }
        break;

        case kDown|k_Repeat:
        case kDown: {
            cOsdItem *item = Get(Current());
            if (!item) break;

            int currIdx = item->Index();
            Move(currIdx, currIdx+1);
            printf("Move %d -> %d\n", currIdx, currIdx +1);

            SetCurrent(item);

            Display(); // not set
            state = osContinue;
        }
        break;

        case kOk: {
            cOsdItem *item = Get(Current());
            cOsdItem *nextItem = Get(Current()+1);
            cOsdChannelItem *From = dynamic_cast<cOsdChannelItem*> (item);
            cOsdChannelItem *To = dynamic_cast<cOsdChannelItem*> (nextItem);

            MoveFavChannel(From->Channel(),To?To->Channel():NULL);
            state = osRefreshPage;
        }
            break;

        default:
            break;
        }

    if (state == osUnknown)
        state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
        switch (Key)
        {
        case kOk:
        {
            cOsdChannelItem *chItem = dynamic_cast<cOsdChannelItem*> (Get(Current()));
            if (chItem && chItem->Channel() && !channel) {
                channel = chItem->Channel();
                chItem->SetMarked(true);
                RefreshCurrent();
                Display();
            }
        }
        state = osContinue;
        break;

        default:
            break;
        } // switch

    } // if

    return state;
}
