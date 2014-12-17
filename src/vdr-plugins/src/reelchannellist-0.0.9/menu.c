#include "menu.h"
#include "activelist.h"
#include "tools.h"
#include "MenuCISlot.h"

#include "MenuMoveChannels.h"
#include "favourites.h"
#include "MenuEditChannel.h"
#include <vdr/interface.h>
#include <vdr/plugin.h>
#include <algorithm>

eKeys iKeys[] = {kInfo, kNone};

bool memoryOn;
bool  wasLastMenuFav, savedWasLastMenuFav;
bool openWithFavBouquets;

int DeleteChannels(std::vector<int> & chList);
bool SelectedChannelsHaveTimers(const std::vector<int> &chList);

/// GlobalFilters --------------------------------------------------------------
GlobalFilters globalFilters("GlobalFilters"), savedGlobalFilters("SavedGlobalFilters");

GlobalFilters::GlobalFilters(const char *name): expectedFilterCount(-1), channelFilters(name)
{

}

bool GlobalFilters::IsAllowedChannel(cChannel *channel)
{
    return channelFilters.IsAllowed(channel);
}

void GlobalFilters::ClearFilters()
{
    channelFilters.Clear();
}

bool GlobalFilters::HasFilters() const
{
    return channelFilters.Count() > 0;
}

cString GlobalFilters::MakeTitle() const
{
    cString result;
    cChannelFilter *f = channelFilters.First();

    while (f) {
        if (*result)
            result = cString::sprintf("%s/%s", *result, *f->ParamsToString());
        else
            result = f->ParamsToString();

        f = channelFilters.Next(f);
    }

    return result;
}

bool GlobalFilters::AddBouquetFilter(const char * bouquet,
                                     bool searchInFav,
                                     bool subStringMatch)
{
    if (!bouquet) return false;

    cChannelBouquetFilter *filter = new cChannelBouquetFilter;
    if (searchInFav)
        filter->SetFilterParams(bouquet, &favourites, subStringMatch);
    else
        filter->SetFilterParams(bouquet, &Channels, subStringMatch);

    channelFilters.Add(filter);
    return true;
}


bool GlobalFilters::AddTransponderFilter(const cChannel* channel)
{
    if (!channel) return false;

    cChannelTransponderFilter *filter = new cChannelTransponderFilter;
    filter->SetFilterParams(channel->Source(), channel->Frequency());

    channelFilters.Add(filter);
    return true;
}


bool GlobalFilters::AddProviderFilter(const char *provider)
{
    if (!provider) return false;

    cChannelProviderFilter *filter = new cChannelProviderFilter;
    filter->SetFilterParams(cString(provider));

    channelFilters.Add(filter);
    return true;
}


bool GlobalFilters::AddSourceFilter(int code)
{
    cSource* source = Sources.Get(code);
    printf("src:= %d desc:='%s'\n", code, source->Description());

    /* is valid source ? */
    if (source) {

        cChannelSourceFilter *srcFilter = NULL;
        cChannelFilter* f = channelFilters.First();
        // if a source filter exists, just change its parameter instead of adding a new source filter
        // a channel cannot be in both sources, so different source filters are
        // orthogonal and result always in empty list.
        while (f) {
            if ( (srcFilter = dynamic_cast<cChannelSourceFilter*>(f)) )  // found source filter
                break;

            f = channelFilters.Next(f);
        } // while


        if (srcFilter) // found source filter, just change parameters
            srcFilter->SetFilterParams(code);
        else {
            srcFilter = new cChannelSourceFilter;

            srcFilter->SetFilterParams(code);
            channelFilters.Add(srcFilter);
        }

        return true;
    } // if (source)

    return false;
}

bool GlobalFilters::AddTVFilter()
{
    // remove any radio filters!
    // Because, TV and radio filters present result in empty list!
    cChannelFilter* f = channelFilters.First();

    while (f) {
        if (dynamic_cast<cChannelRadioFilter*>(f)) { // found radio filter
            cChannelFilter *t = f;
            f = channelFilters.Next(f);
            channelFilters.Del(t); // remove radio filter
        } else
            f = channelFilters.Next(f);
    } // while

    // add TV filter
    cChannelTVFilter *filter = new cChannelTVFilter;
    channelFilters.Add(filter);
    return true;
}

bool GlobalFilters::AddRadioFilter()
{
    // remove any TV filters!
    // Because, TV and radio filters present result in empty list!
    cChannelFilter* f = channelFilters.First();

    while (f) {
        if (dynamic_cast<cChannelTVFilter*>(f)) { // found TV filter
            cChannelFilter *t = f;
            f = channelFilters.Next(f);
            channelFilters.Del(t); // remove TV filter
        } else
            f = channelFilters.Next(f);
    } // while

    // add radio filter
    cChannelRadioFilter *filter = new cChannelRadioFilter;
    channelFilters.Add(filter);
    return true;
}

bool GlobalFilters::AddTextFilter(const char *text)
{
    if (!text) return false;

    cChannelTextFilter *filter = new cChannelTextFilter;
    filter->SetFilterParams(cString(text));

    channelFilters.Add(filter);
    return true;
}

void GlobalFilters::Save() const
{
    channelFilters.Save();
}

bool GlobalFilters::SetupParse(const char *name, const char *param)
{
    if (!name) return false;

    if (strstr(name, "count"))
    {
        if (!param) return false;
        expectedFilterCount = atoi(param);

        // remove excess filters
        while (channelFilters.Count() > expectedFilterCount) {
            channelFilters.RemoveLastFilter();
        }

        return true;
    }

    if (expectedFilterCount >= 0
            && channelFilters.Count() >= expectedFilterCount) {
        // got filter 'count' from setup.conf
        // AND
        // there are more filters than there should be

        // so discard the last few filters till ...
        // this happens because a previous 'save' would have had more filters

        // remove those excess filters
        while (channelFilters.Count() > expectedFilterCount) {
            channelFilters.RemoveLastFilter();
        }

        return false;
    }

    // Add filter

    //name : <GlobalFilters' name>_X
    // 'X' is the filter index
    // param: <filtername> <filter params>
    printf("name: '%s' param:'%s'\n", name, param);
    char *str = strdup(param);
    const char *delim = " ";
    char *strtok_saveptr = NULL;

    // filter name
    char *p = strtok_r(str, delim, &strtok_saveptr);
    printf("token 1: '%s'\n", p);
    cString filterName = p;

    // filter parameters -- all the following chars incl. spaces after 'filter name'
    const char* filter_param = strchr(param, ' ');

    cString parameter;
    if (filter_param) parameter = filter_param + 1;

    printf("filter parameter: '%s'\n", *parameter);
    free(str);
    bool ret = channelFilters.AddFilterFromString(filterName, parameter);

    printf("Count: %d\n", channelFilters.Count());
    return ret;
}

/// Next Channel Sort mode
eSortModes NextChannelSortMode(eSortModes curr)
{
    eSortModes nextMode;
    switch (curr) {
    case SortByNumber:      nextMode = SortByName;      break;
    case SortByName:        nextMode = SortByProvider;  break;
    case SortByProvider:    nextMode = SortByNumber;    break;
    default:                nextMode = SortByNumber;    break;
    }

    return nextMode;
}

cString SortModeToString(eSortModes mode)
{
    cString s = "unknown";

    switch (mode) {
    case SortByNumber:      s = tr("Sort by number");           break;
    case SortByName:        s = tr("Sort by channel name");     break;
    case SortByProvider:    s = tr("Sort by provider name");    break;
    default:
        break;
    }

    return s;
}



/// cMenuChannelList -----------------------------------------------------------

cMenuChannelList::cMenuChannelList(): cOsdMenu(tr("Channel List")),
    channelShowMode(eAllChannels),
    red_str(tr(ALL_CHANNELS))
{
    ClearChannelSelectMode();
    Channels.IncBeingEdited();

#if REELVDR && APIVERSNUM >= 10716
    EnableSideNote(true);
#endif /*REELVDR && APIVERSNUM >= 10716*/

    /* remember filters, only if user selects channel with kOk */
    memoryOn = false;

    lastSelectedChannel = cDevice::PrimaryDevice()->CurrentChannel();

    /* Show channels*/
    Set();
}


cMenuChannelList::~cMenuChannelList()
{
    /* forget all filters and last menu */
    if (!memoryOn) {
        wasLastMenuFav = false;
        globalFilters.ClearFilters();
        favouritesFilters.ClearFilters();
    }

    Channels.DecBeingEdited();
    cPluginManager::CallAllServices("stop preview channel");
}


eOSState cMenuChannelList::DeleteSelectedChannels()
{
    if (cMenuChannelList::IsSelectToDeleteMode()) {
        bool result = Interface->Confirm(tr("Delete channels?"));
        if (!result) {
            cMenuChannelList::ClearChannelSelectMode();
            Skins.Message(mtInfo, tr("Deletion cancelled"));
            return osBack;
        }
        // check for timers in selected channels
        result = SelectedChannelsHaveTimers(
                    cMenuChannelList::SelectedChannels);
        if (result) {
            Skins.Message(mtError, tr("A selected channel has timer. Remove timer and try again."));
            return osShowChannelList;
        }
        result = DeleteChannelsFromFavourites(
                    cMenuChannelList::SelectedChannels);
        int sz = DeleteChannels(cMenuChannelList::SelectedChannels);
        
        Skins.Message(mtInfo,
                      cString::sprintf(tr("Deleted %d Channels"), sz));
        
        cMenuChannelList::ClearChannelSelectMode();
    } else {
        cMenuChannelList::SetSelectToDeleteMode();
    }
    
    return osShowChannelList;
}


void cMenuChannelList::SetHelpKeys()
{
    switch (channelShowMode) {
    case eAllChannels:          red_str = tr("Radio Only"); break;
    case eOnlyRadioChannels:    red_str = tr("TV Only");    break;

    case eOnlyTVChannels:
    default:                    red_str = tr(ALL_CHANNELS); break;
    }

    green_str  = tr(SOURCES);
    yellow_str = tr(FAVOURITES);
    blue_str   = tr("Functions");
    
    if (IsSelectToAddMode() || IsSelectToDeleteMode()) {
        green_str = yellow_str = blue_str = NULL;
        if (SelectedChannels.size() > 0 && IsSelectToAddMode())
            blue_str = tr("Move to Fav");
        else if (SelectedChannels.size() > 0 && IsSelectToDeleteMode())
                blue_str = tr("Delete selected");
        else if (SelectedChannels.size() > 0)
            blue_str = tr("Unknown mode");
    } 
    
    SetHelp(red_str, green_str, yellow_str, blue_str);
}

void cMenuChannelList::IncChannelShowMode()
{
    switch (channelShowMode) {
    case eAllChannels:          channelShowMode = eOnlyRadioChannels;   break;
    case eOnlyRadioChannels:    channelShowMode = eOnlyTVChannels;      break;

    case eOnlyTVChannels:
    default:
                                channelShowMode = eAllChannels;         break;
    }
}

void cMenuChannelList::AddChannelShowFilter()
{
    switch (channelShowMode) {
    case eAllChannels: // all channels
        globalFilters.ClearFilters();
        break;

    case eOnlyRadioChannels: // Radio channels
        globalFilters.AddRadioFilter();
        break;

    case eOnlyTVChannels: // TV channels
        globalFilters.AddTVFilter();
        break;

    default:
        break;
    }
}
std::vector<int> cMenuChannelList::SelectedChannels;
eChannelSelectMode cMenuChannelList::channelSelectMode;

bool cMenuChannelList::IsSelectedChannel(int Number)
{
    std::vector<int>::iterator it = std::find(SelectedChannels.begin(), SelectedChannels.end(), Number);
    return it != SelectedChannels.end();

//    if (it != SelectedChannels.end()) {
//        //printf("found number %d\n", Number);
//        return true;
//    } else {
//        //printf("NOT found number %d\n", Number);
//        return false;
//    }
}

void cMenuChannelList::SelectChannel(int Number, bool &added)
{
    if (IsSelectedChannel(Number)) { // channel already present; remove it
        added = false;

        std::vector<int>::iterator it = std::remove(SelectedChannels.begin(),
                                                    SelectedChannels.end(),
                                                    Number);
        SelectedChannels.erase(it, SelectedChannels.end());
    } else { // new channel number; add it to list
        added = true;
        SelectedChannels.push_back(Number);
    }

}

void cMenuChannelList::Set()
{
    Clear();

    /* tabs: Channel Nr / Channel Name / event progress bar  */
    SetCols(5, 18, 6);

    cOsdChannelItem *item, *currentItem = NULL;
    cChannel *channel = Channels.First();
    for ( ; channel; channel = Channels.Next(channel))
    {
        /* do not show, bouquets in normal channel list*/
        if (!channel->GroupSep() && globalFilters.IsAllowedChannel(channel)) {
            Add(item = new cOsdChannelItem(channel));

            if (channel->Number() == lastSelectedChannel)
                currentItem = item;

            if (IsSelectMode() && IsSelectedChannel(channel->Number())) {
                item->SetMarked(true);
                item->Set(); // draw text with tick mark
            }
        }
    } // for

    Sort();
    if (currentItem)
        SetCurrent(currentItem);

    // no channels being shown. Eg. No result from Text search.
    if (Count() == 0) {
        AddFloatingText(tr("No channels found.\nPress red key to list all channels."), 50);
    }

    cString tmpTitle = globalFilters.MakeTitle();

    cString normTitle = tr("Channel List");

    if (IsSelectToDeleteMode()) normTitle = tr("Select channels to delete");
    else if (IsSelectToAddMode())    normTitle = tr("Select channels to add to Fav");

    if (*tmpTitle) {
        tmpTitle = cString::sprintf("%s: %s", *normTitle , *tmpTitle);
    } else {
        if (globalFilters.channelFilters.Count())
            tmpTitle = normTitle;
        else
            tmpTitle = cString::sprintf("%s: %s", *normTitle, tr("All Channels"));
    }

    SetTitle(*tmpTitle);
     
    // Set status message to inform user about possible actions
    if (IsSelectMode()) {
        SetStatus(tr("Select channels with Ok. Then blue key for options."));
    } else // clear status message
        SetStatus(NULL);

    Display();

    SetHelpKeys();

//    if (wasLastMenuFav)
//        AddSubMenu(new cMenuFavourites(!openWithFavBouquets));

}

void cMenuChannelList::Display()
{
    cOsdMenu::Display();
    ShowSideNoteInfo();
}

bool cMenuChannelList::JumpToChannelNumber(int num)
{
    cOsdChannelItem *chItem = NULL;
    cOsdItem *item = NULL;

    for (item=First(); item; item=Next(item)) {
        chItem = dynamic_cast<cOsdChannelItem*>(item);
        if (chItem && chItem->Channel() && chItem->Channel()->Number()==num) {
            SetCurrent(item);
            Display();
            return true;
        }
    }

    return false;
}

void cMenuChannelList::ShowSideNoteInfo()
{
#if REELVDR && APIVERSNUM >= 10716
    // currently selected item
    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*>(Get(Current()));

    if (currItem)
    {
        const cChannel *channel = currItem->Channel();
        SideNote(channel);

        if (channel) {
            /* show HD & encrypted channel icons */
            char icons[3] = {0};
            int i=0;

            if (IS_HD_CHANNEL(channel))
                icons[i++] = char(142);
            if (channel->Ca())
                icons[i++] = 80;

            SideNoteIcons(icons);

            if (Setup.PreviewVideos) {
                int number = channel->Number();
                cPluginManager::CallAllServices("start preview channel", &number);
            } else
                cPluginManager::CallAllServices("stop preview channel");
        } // if channel

    }
#endif /*REELVDR && APIVERSNUM >= 10716*/
}

eOSState cMenuChannelList::ProcessKey(eKeys Key)
{
    bool hadSubMenu = HasSubMenu();
    eOSState state;
#define NUMKEYTIMEOUT 2000
    static cTimeMs timeOut(NUMKEYTIMEOUT);
    static int number = 0;

    // not a number key, reset timeout
    if (Key < k0 || Key > k9) {
        number = 0;
        timeOut.Set(NUMKEYTIMEOUT);
    }

    // get out of channel select mode with exit key
    if (!HasSubMenu() && IsSelectMode() && NORMALKEY(Key) == kBack) {
        ClearChannelSelectMode();
        Set();
        Key = kNone;
    }
#if 0
    // donot close menu if some filters are active;
    // use kBack to revert to earlier filter?
    if (!hadSubMenu && Key == kBack && globalFilters.HasFilters())
        state = osUnknown; // handle this key
    else
#endif
        state = cOsdMenu::ProcessKey(Key);


    // Submenu requested list of channels to be shown
    // close submenu and redraw channel list
    if (state == osShowChannelList) {
        channelShowMode = eUnknownMode;
        /* redraw menu first as CloseSubMenu() refreshes the current item
         and current item may have been deleted!*/
        Set(); 
        CloseSubMenu();
        return osContinue;
    }

    // show side note info if no submenu and key has been handled
    if (!HasSubMenu() && state != osUnknown)
        ShowSideNoteInfo();

    // currently selected item
    cOsdChannelItem *currItem = dynamic_cast<cOsdChannelItem*>(Get(Current()));

    /* state != osUnknown ==> key handled and the current-selected item
       may have changed, update lastSelectedChannel */
    if(state != osUnknown && currItem && currItem->Channel())
        lastSelectedChannel = currItem->Channel()->Number();

    // Submenu closed; redraw
    if (hadSubMenu && !HasSubMenu()) {
        /// TODO redraw only if necessary
        printf("Submenu closed. Redrawing. TODO. redraw only if warranted\n");
        Set();
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

#if 0 // should not be necessary anymore

    // if the submenu does not handle colorkeys
    // color keys should bring up the "main four menus"
    // "All Channels", "Sources/Satellites", "Fav", "Providers"
    // and close all the submenus
    if(ISCOLOUR_KEY(Key) && state == osUnknown) {
        // channelShowMode is incremented in switch below
        // but when red-key is pressed from another menu
        // channelShowMode should be eAllChannels, so set it to be unknown here
        if (HasSubMenu())
            channelShowMode = eUnknownMode;

        CloseSubMenu();
    }
#endif

    // Handle color keys
    if (state == osUnknown)
    switch(Key) {

    case kRed: // no filter, all channels
    {
        // DO NOT clear filters, allow filtering on search/filtered
        // result for TV/radio channels
        //globalFilters.ClearFilters();

        IncChannelShowMode();
        AddChannelShowFilter();

        // display list again
        Set();
        state = osContinue;
    }
        break;

    case kGreen: // Sources
        // ignore in select mode
        if (IsSelectMode()) return osContinue;
        
        globalFilters.ClearFilters();
        return AddSubMenu(new cMenuSourceList);

        break;

    case kBlue:
        break; // blue is now function menu, ignore this key here
//       globalFilters.ClearFilters();
//        return AddSubMenu(new cMenuProviderList);
//
//        break;

    case kYellow: // Fav
        // ignore in select mode
        if (IsSelectMode()) return osContinue;
        
        favouritesFilters.ClearFilters();
        return AddSubMenu(new cMenuFavourites);
        globalFilters.ClearFilters();

        // display list again
        Set();
        break;
    default:
        break;
    } // switch for colorkeys


    if (!HasSubMenu() && state == osUnknown) {
        switch(Key) {
        case kBack:
            /* if channel list is filtered, clear filtering */
            if (globalFilters.HasFilters()) {
                globalFilters.ClearFilters();

                // draw menu again
                Set();
            } // if
            break;

        case kOk:
            if (IsSelectMode()) {
                printf("Channel Select Mode: current item marked? %d\n",
                       currItem->IsMarked());
                bool added;
                
                // restrict the number of channels selected at one time
                if (!IsSelectedChannel(currItem->Channel()->Number())) {
                    if (SelectedChannels.size() > 50) // Maximum 50 channels are marked at a time
                        return osContinue;
                }
                
                SelectChannel(currItem->Channel()->Number(), added);
                currItem->SetMarked(added);
                RefreshCurrent();
                
                // find the next selectable item and jump to it
                cOsdItem *nextItem = Next(currItem);
                while (nextItem && !nextItem->Selectable()) nextItem = Next(nextItem);
                if (nextItem) {
                    DisplayCurrent(false);
                    SetCurrent(nextItem);
                    Display();
                    SetHelpKeys();
                } else
                    DisplayCurrent(true);
                
                for (int i = 0; i < SelectedChannels.size(); i++)
                    printf("%d ", SelectedChannels.at(i));
                printf("\n\n");
                return osContinue;
            }
            if (currItem && currItem->Channel()) {
                bool flag = Channels.SwitchTo(currItem->Channel()->Number());
                if (flag) {
                    memoryOn = true;
                    SaveFilteredChannelList(false);
                    return osEnd;
                }
                state = osContinue;
            } // currItem
            break;

        case kBlue:
            if (IsSelectToAddMode()) {
                if (SelectedChannels.size() <= 0) 
                    return osContinue;
                
                AddSubMenu(new cMenuAddChannelToFavourites(SelectedChannels));
                return osContinue;
            }
            else if (IsSelectToDeleteMode()) {
                if (SelectedChannels.size() <= 0) 
                    return osContinue;
                
                eOSState State = cMenuChannelList::DeleteSelectedChannels();
                
                /* redraw menu - even if channels were not deleted, mode has changed.*/
                Set();
                
                return osContinue;
            }
            
            if (currItem && currItem->Channel())
                return AddSubMenu(new cMenuChannelsFunction(currItem->Channel()));

            // if menu was empty, then do not allow macrokey to be activated
            state = osContinue;
            break;

        case k0...k9:
        {
            int n = Key - k0;
            if (timeOut.TimedOut())
                number = 0;

            if (!number && !n) {
                //handle '0' key
                cOsdItem *curr = Get(Current());
                ChannelSortMode = NextChannelSortMode(ChannelSortMode);
                Sort();

                // make sure current item stays the same
                if (curr) SetCurrent(curr);
                Display(); // not Set()

                Skins.Message(mtInfo, *SortModeToString(ChannelSortMode), 1);

                /* store sort mode in setup.conf */
#define PL_NAME "reelchannellist"
                cPlugin *p = cPluginManager::GetPlugin(PL_NAME);
                if (p)
                    p->SetupStore("ChannelSortMode", PL_NAME, (int)ChannelSortMode);
                else
                    esyslog("getting plugin %s failed! %s:%d"
                                             , PL_NAME, __FILE__, __LINE__);


            } else {
                number = number*10 + n;
                JumpToChannelNumber(number);
                timeOut.Set(NUMKEYTIMEOUT);
            }
            state = osContinue;
        }
        break;

        default:
            break;
        } // switch

    } // if

    return state;
}



/// ****************************************************************************
/// *             cMenuProviderList                                            *
/// ****************************************************************************
/// Show a sorted list of providers present in channels list (no duplicate providers)
/// set provider-filter according to selected provider

cMenuProviderList::cMenuProviderList() : cOsdMenu(tr("Providers"))
{
#if REELVDR && APIVERSNUM>= 10716
    EnableSideNote(true);
#endif /* REELVDR && APIVERSNUM>= 10716 */
    Channels.IncBeingEdited();
    Set();
}


cMenuProviderList::~cMenuProviderList()
{
    Channels.DecBeingEdited();
}

void cMenuProviderList::Set()
{
    Clear();

    SetCols(4);
    cChannel *channel = Channels.First();
    const char* provider;
    cStringList providerList;

    // get a list of all providers; no duplicates
    for ( ;channel; channel = Channels.Next(channel)) {
        if (!channel->GroupSep() && globalFilters.IsAllowedChannel(channel)) {
            provider = channel->Provider();

            if (provider && providerList.Find(provider) == -1) // not found
                providerList.Append(strdup(provider));
        } // if allowed channel
    } // for

    // sort
#if APIVERSNUM >= 10720
    providerList.Sort(true);
#elif REELVDR && APIVERSNUM >= 10718 /*APIVERSION >= "1.7.18.5"*/
    providerList.SortIgnoreCase();
#else
    providerList.Sort();
#endif

    // and display
    for (int i=0; i < providerList.Size(); ++i) {
        Add(new cOsdProviderItem(providerList.At(i)));
    }

    cString tmpTitle = globalFilters.MakeTitle();
    if (*tmpTitle) {
        tmpTitle = cString::sprintf("%s: %s/%s", tr("Channel List"), *tmpTitle,
                                    tr(PROVIDERS));
    } else
        tmpTitle = cString::sprintf("%s: %s", tr("Channel List"), tr(PROVIDERS));

    SetTitle(*tmpTitle);
    Display();
    SetHelp(tr(ALL_CHANNELS), tr(SOURCES), tr(FAVOURITES), tr("Functions"));
}


/* find the first provider that starts with 'c'
   assume providers are sorted */
void cMenuProviderList::JumpToProviderStartingWith(char c)
{
    char ch[2] = {0};
    ch[0] = c;

    cOsdItem *item = First();
    cOsdProviderItem *providerItem = NULL;

    // go through all items
    for (; item; item = Next(item)) {
        providerItem = dynamic_cast<cOsdProviderItem*> (item);
        if (providerItem &&
                strncasecmp(providerItem->ProviderName(), ch, strlen(ch)) == 0) {
                SetCurrent(providerItem);
                Display();
                break;
            } // if
    } // for

}

eOSState cMenuProviderList::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    // Submenu requested list of channels to be shown
    // close submenu and return state
    if (state == osShowChannelList)
    {
        CloseSubMenu();
        return osShowChannelList;
    }


    if (state == osUnknown)
        switch (Key)
        {
        case kOk:
        {
            cOsdProviderItem *currItem = dynamic_cast<cOsdProviderItem*>(Get(Current()));
            printf("'%s'\n", Get(Current())->Text());
            if (currItem)
            {
                globalFilters.AddProviderFilter(currItem->ProviderName());
            } else
                printf("ERROR(%s:%d) this is no provider item\n", __FILE__, __LINE__);

            return AddSubMenu(new cMenuChannelList);

        }
            break;

        case kBlue:
        {
            cOsdProviderItem *currItem = dynamic_cast<cOsdProviderItem*>(Get(Current()));
            if (currItem)
                return AddSubMenu(new cMenuProvidersFunction(currItem->ProviderName()));
        }
        break;

        case k0...k9:
        {
            char ch;

            if ( (ch = NumpadChar(Key)) )
                JumpToProviderStartingWith(ch);

           // printf("pressed %d, %d times\t", pressedNum, multiples);
            if (ch != 0) {
                printf("got '%c'\n", ch);
            } else
                printf("Got error\n");
        }
        break;

        default:
            break;
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
    if (state == osUnknown && !HasSubMenu())
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
        favouritesFilters.ClearFilters();
        AddSubMenu(new cMenuFavourites);
        state = osContinue;
        break;

    default:
        break;
    } // switch for colorkeys


    return state;
}


/** **************** cMenuProvidersFunction *************************************
*** Menu to offer functions on provider list menu
*******************************************************************************/
cMenuProvidersFunction::cMenuProvidersFunction(const char *providerName_) : cOsdMenu(*cString::sprintf("%s: %s", trVDR("Functions"), trVDR(PROVIDERS))), providerName(providerName_)
{
    Set();
}

void cMenuProvidersFunction::Set()
{
    Clear();
    SetHasHotkeys();

    cString tmp = cString::sprintf("%s '%s' %s", tr("Add provider"), *providerName,
                                   tr("to favourites"));
    Add(new cOsdItem(hk(*tmp)));

    tmp = cString::sprintf("%s: '%s'", tr("CI-slot assignment for provider"),
                           *providerName);
    Add(new cOsdItem(hk(*tmp)));

    Display();
}

eOSState cMenuProvidersFunction::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown)
    switch (Key) {
    case kOk:
        if (Get(Current())) {
            const char* text = Get(Current())->Text();

            if (text && strcasestr(text, tr("CI-slot assignment for provider")))
                            return AddSubMenu(new cOsdMenuCISlot(providerName));
            else if (text && strcasestr(text, tr("Add provider"))) {
                if (AddProviderToFavourites(*providerName))
                    Skins.Message(mtInfo, tr("Provider added to favourites"));
                else
                    Skins.Message(mtError, tr("Error: Provider not added to favourites"));

                state = osBack;
            }
        } // Get(Current())

        break;
    default:
        break;
    } // switch

    return state;
}


/// ****************************************************************************
/// *             cMenuSourceList                                              *
/// ****************************************************************************
/// Show a sorted list of providers present in channels list (no duplicate providers)
/// set provider-filter according to selected provider

cMenuSourceList::cMenuSourceList() : cOsdMenu(tr("Sources"))
{
#if REELVDR && APIVERSNUM>= 10716
    EnableSideNote(true);
#endif /* REELVDR && APIVERSNUM>= 10716 */

    Channels.IncBeingEdited();
    Set();
}


cMenuSourceList::~cMenuSourceList()
{
    Channels.DecBeingEdited();
}


/* return true, if one of the devices says it can provice this source */
bool CanTuneToSource(int source) {

    cDevice *device = NULL;

    for (int i=0; i<MAXDEVICES; ++i) {

        device = cDevice::GetDevice(i);
        if (device && device->ProvidesSource(source))
            return true;
    } // for

    return false;
}


void cMenuSourceList::Set()
{
    Clear();
    cMySourceList sourceList;

    printf("Total sources: %d\n", Sources.Count());
    // add folder icon to current transponder item
    Add(new cOsdSourceItem(0, eActionCurrentTransponder));

    //get sources from channels list
    cChannel *channel = Channels.First();
    for ( ;channel; channel = Channels.Next(channel)) {

        /* channel not bouquet name and not filtered,
           add source from the channel */
        if (!channel->GroupSep() && globalFilters.IsAllowedChannel(channel))
        {
            // ignore invalid sources
            if (!Sources.Get(channel->Source())) {
                isyslog("channel has invalid source %d:%s source code:%d (ptr:%p)",
                        channel->Number(), channel->Name(),
                        channel->Source(), Sources.Get(channel->Source()));
                        continue;
            }

            if (CanTuneToSource(channel->Source()) &&
                    sourceList.Find(channel->Source()) == -1)
                sourceList.Append(channel->Source());
        } // if
    } // for

    //sort sources according to name (not number)
    sourceList.Sort();

    // empty line after "Current Transponder" only is there are some sources
    if (sourceList.Size() > 0)
        Add(new cOsdItem("", osUnknown, false));

    // display the name of sources
    for (int i=0; i<sourceList.Size(); ++i) {
        Add(new cOsdSourceItem(sourceList.At(i)));
        Add(new cOsdSourceItem(sourceList.At(i), eActionProviders));
        Add(new cOsdSourceItem(sourceList.At(i), eActionNewChannels));

        // empty line for all but last source
        if (i < sourceList.Size()-1)
            Add(new cOsdItem("", osUnknown, false));
    }
    printf("Total sources: %d, total uniq sources displayed := %d\n",
           Sources.Count(), sourceList.Size());

    cString tmpTitle = globalFilters.MakeTitle();
    if (*tmpTitle) {
        tmpTitle = cString::sprintf("%s: %s/%s", tr("Channel List"), *tmpTitle,
                                    tr(SOURCES));
    } else
        tmpTitle = cString::sprintf("%s: %s", tr("Channel List"), tr(SOURCES));

    SetTitle(*tmpTitle);
    Display();
    SetHelp(tr(ALL_CHANNELS), tr(SOURCES), tr(FAVOURITES), NULL);
}

eOSState cMenuSourceList::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    // Submenu requested list of channels to be shown
    // close submenu and return state
    if (state == osShowChannelList)
    {
        CloseSubMenu();
        return osShowChannelList;
    }

    cOsdSourceItem *srcItem = dynamic_cast<cOsdSourceItem*> (Get(Current()));

    if (state == osUnknown)
        switch (Key)
        {
        case kBlue: // no function menu for cMenuSourceList menu, ignore this key
            return osContinue;

        case kOk:
            printf("=============================\nGot text '%s'\n", Get(Current())->Text());
            if (srcItem) {
                printf("src id: %d\n", srcItem->Source());


                switch (srcItem->ActionType())
                {

                // filter providers from this source
                case eActionProviders:
                    globalFilters.AddSourceFilter(srcItem->Source());
                    return AddSubMenu(new cMenuProviderList);

                // look for bouquet "auto added"
                case eActionNewChannels:
                    globalFilters.AddSourceFilter(srcItem->Source());
                    globalFilters.AddBouquetFilter("auto added",
                                                   false /*not in fav*/,
                                                   true /*match sub string*/);
                    break;

                case eActionCurrentTransponder:
                    if (cDevice::PrimaryDevice())
                    {
                        int currChannel = cDevice::PrimaryDevice()->CurrentChannel();
                        const cChannel *channel = Channels.GetByNumber(currChannel);

                        if (channel)
                        {
                            globalFilters.ClearFilters();
                            globalFilters.AddTransponderFilter(channel);
                        } // current channel
                    } // primary device
                    break;

                case eActionChannels:
                    globalFilters.AddSourceFilter(srcItem->Source());
                    break;

                default:
                    break;
                }// switch


            } else if (strcmp(tr(CURRENT_TRANSPONDER), Get(Current())->Text()) == 0)
            {
                if (cDevice::PrimaryDevice())
                {
                    int currChannel = cDevice::PrimaryDevice()->CurrentChannel();
                    const cChannel *channel = Channels.GetByNumber(currChannel);

                    if (channel)
                    {
                        globalFilters.ClearFilters();
                        globalFilters.AddTransponderFilter(channel);
                    } // current channel
                } // primary device
            } // Current Transponder

            return AddSubMenu(new cMenuChannelList);
            break;

        default:
            break;
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
        // do nothing!
        state = osContinue;
        break;

    case kBlue:
        break; // blue is now function menu, ignore this key here

    case kYellow: // reset Fav listing, show fav. folders list
        favouritesFilters.ClearFilters();
        AddSubMenu(new cMenuFavourites);
        state = osContinue;
        break;

    default:
        break;
    } // switch for colorkeys


    return state;
}



/** **************** cMenuChannelsFunction *************************************
*** Menu to offer functions on channel-list menu
*******************************************************************************/
cMenuChannelsFunction::cMenuChannelsFunction(cChannel* chan) :
    cOsdMenu(trVDR("Functions: Channel list")),
    channel(chan)
{
    Set();
}

void cMenuChannelsFunction::Set()
{
    Clear();
    SetHasHotkeys();

    // If in select mode, show options according to select mode
    if (cMenuChannelList::IsSelectToAddMode()) {
        cString note = cString::sprintf(tr("Add the %d selected channels to favourites with the option below:"),
            cMenuChannelList::SelectedChannels.size());
        AddFloatingText(note, 45);

        Add(new cOsdItem("",osUnknown, false)); // blank line
        Add(new cOsdItem(hk(tr("Add multiple channels to Favourites"))));
        Display();
        return;
    } else if (cMenuChannelList::IsSelectToDeleteMode()) {
        cString note = cString::sprintf(tr("Delete the %d selected channels:"),
            cMenuChannelList::SelectedChannels.size());
        AddFloatingText(note, 45);

        Add(new cOsdItem("",osUnknown, false)); // blank line
        Add(new cOsdItem(hk(tr("Delete channels"))));
        Display();
        return;
    }

    //cString note_text = cString::sprintf("%s '%s' %s:", tr("For channel"),
    //                                     channel->Name(),
    //                                     tr("please choose one of the following options"));

    cString note_text = cString::sprintf(tr("For channel '%s' please choose one of the following options:"),
                                         channel->Name());
    AddFloatingText(*note_text, 45);
    Add(new cOsdItem("",osUnknown, false)); // blank line
    //Add(new cOsdItem("",osUnknown, false)); // blank line

    Add(new cOsdItem(hk(tr("Move channel"))));
    Add(new cOsdItem(hk(tr("Edit selected channel"))));

    //if (!IsChannelInFavourites(channel))
    Add(new cOsdItem(hk(tr("Add selected channel to Favourites"))));
    Add(new cOsdItem(hk(tr("Add multiple channels to Favourites"))));
    Add(new cOsdItem(hk(tr("Delete channels"))));

    SetHelp(NULL, NULL, NULL, tr("Search"));
    Display();
}

bool SelectedChannelsHaveTimers(const std::vector<int> &chList)
{
    cTimer *timer = NULL;
    for (std::vector<int>::size_type i=0 ; i <  chList.size(); ++i) {
        // loop through the timers list and see if chList.at(i) has a timer
        for (timer = Timers.First(); timer ; timer = Timers.Next(timer)) {
            if (timer->Channel()->Number() == chList.at(i)) {
                isyslog("timer found for selected channel #%d", chList.at(i));
                return true;
            }
        } // for timer

    }// for chList

    return false; // no timer found in list of selected channels
} //


int DeleteChannels(std::vector<int> & chList/*selectedChannelsList*/)
{

    Channels.IncBeingEdited();

    // count number of channels deleted
    int count = 0;

    for (std::vector<int>::size_type i=0 ; i <  chList.size(); ++i) {
        cChannel *ch = Channels.GetByNumber(chList.at(i));

        if (!ch) continue;

        // selected channel has Timer ?
        // then delete timer as well

        // selected channel has running recording?
        // stop recording (?)

        // remove channels in favourites bouquet

        // delete them from vdr list
        Channels.Del(ch);
        count++;

    } // for

    // fix channel number as they are 'broken' by channel deletion
    Channels.ReNumber();

    Channels.DecBeingEdited();
    Channels.SetModified(true);

    return count;
} //

eOSState cMenuChannelsFunction::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    /* close sub menus if channel list is requested */
    if (state == osShowChannelList) {
        CloseSubMenu();
        return state;
    }

    const char* text = Get(Current())->Text();
    if (state == osUnknown) {
        switch(Key) {
        case kOk:
            if(text) {
                if (strstr(text, tr("Move channel")))
                    return AddSubMenu(new cMenuMoveChannels(channel));
                else if(strstr(text, tr("Edit selected channel")))
                    return AddSubMenu(new cMenuMyEditChannel(channel));
                else if(strstr(text, tr("Search for channel")))
                    return AddSubMenu(new cMenuChannelSearch);
                else if (strstr(text, tr("Add selected channel to Favourites"))) {
                    std::vector<int> channels;
                    channels.push_back(channel->Number());
                    return AddSubMenu(new cMenuAddChannelToFavourites(channels));
                }
                else if (strstr(text, tr("Add multiple channels to Favourites")))
                {
                    if (cMenuChannelList::IsSelectToAddMode()){
                        AddSubMenu(new cMenuAddChannelToFavourites(cMenuChannelList::SelectedChannels));
                        cMenuChannelList::ClearChannelSelectMode();
                        return osContinue;
                    } else {
                        cMenuChannelList::SetSelectToAddMode();
                        return osBack;
                    }
                } else if (strstr(text, tr("Delete channels"))) {
                     // if in Delete channel mode
                            // delete the selected channels
                    // else Go into Delete channel mode
                            // display channellist page again for channelselection

#if 0
                    if (cMenuChannelList::IsSelectToDeleteMode()) {
                        bool result = Interface->Confirm(tr("Delete channels?"));
                        if (!result) {
                            cMenuChannelList::ClearChannelSelectMode();
                            Skins.Message(mtInfo, tr("Deletion cancelled"));
                            return osBack;
                        }
                        // check for timers in selected channels
                        result = SelectedChannelsHaveTimers(
                                    cMenuChannelList::SelectedChannels);
                        if (result) {
                            Skins.Message(mtError, tr("A selected channel has timer. Remove timer and try again."));
                            return osShowChannelList;
                        }
                        result = DeleteChannelsFromFavourites(
                                    cMenuChannelList::SelectedChannels);
                        int sz = DeleteChannels(cMenuChannelList::SelectedChannels);

                        Skins.Message(mtInfo,
                                      cString::sprintf(tr("Deleted %d Channels"), sz));

                        cMenuChannelList::ClearChannelSelectMode();
                    } else {
                        cMenuChannelList::SetSelectToDeleteMode();
                    }
                    state = osShowChannelList; // redraw channellist menu
#endif
                    state = cMenuChannelList::DeleteSelectedChannels();
                    
                }
            } //if

            break;

        case kBlue:
            return AddSubMenu(new cMenuChannelSearch);

        default:
            break;
        } // switch
    } //if

    return state;
}




cMenuChannelSearch::cMenuChannelSearch() : cOsdMenu(trVDR("Functions: search for channel"))
{
    // clear string
    searchText[0] = 0;

    Set();

    // start in editmode
    cRemote::Put(kRight, true);
}

void cMenuChannelSearch::Set()
{
    Clear();
    SetCols(15);

    AddFloatingText(tr("Enter string to search"), 50);
    Add(new cOsdItem("",osUnknown, false));
    Add(new cMenuEditStrItem(tr("Search for"), searchText, sizeof(searchText)/*, tr("abcdefghijklmnopqrstuvwxyz "*/));
}

eOSState cMenuChannelSearch::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
        /// TODO if color key
        /// return osContinue so that "main" menu does not handle it

        if(kOk == Key) {
            globalFilters.AddTextFilter(searchText);
            return osShowChannelList;
        } // if kOK

    } // if

    return state;
}
