#ifndef MENU_H
#define MENU_H

#include <vdr/osdbase.h>
#include <vdr/channels.h>
#include <vdr/keys.h>

#include <vector>

#include "menuitem.h"
#include "channelListFilter.h"

/* taken from bouquet plugin */
#define IS_RADIO_CHANNEL(c)           (c->Vpid()==0||c->Vpid()==1)
#define IS_ENCRYPTED_RADIO_CHANNEL(c) (IS_RADIO_CHANNEL(c) && (c->Ca() > 0))
#define IS_ENCRYPTED_VIDEO_CHANNEL(c) (!IS_RADIO_CHANNEL(c) && (c->Ca() > 0))

#if APIVERSNUM < 10716
#define IS_HD_CHANNEL(c)              (c->IsSat() && (c->Modulation() == 10))
#else
#define IS_HD_CHANNEL(c)              ( (c->Vpid() != 0) /*radio channel are not HD*/ && c->IsSat() && (cDvbTransponderParameters(c->Parameters()).System() == SYS_DVBS2 || strstr(c->Name(), " HD")/* assume HD channel if name contains " HD" substring*/))
#endif /*APIVERSNUM < 10716*/


#define ISCOLOUR_KEY(k) (k==kRed||k==kGreen||k==kYellow||k==kBlue)

/* memoryOn == true means remember all the channel and fav. filters and
   open this plugin with the same filters (and menu/view), the next time */
extern bool memoryOn;

/* open this plugin with favourites list? */
extern bool wasLastMenuFav, savedWasLastMenuFav;
extern bool openWithFavBouquets;


/** ****************************************************************************
*** if menu receives osShowChannelList, it means show the channel list!
*** Therefore, a menu should close it's submenu and return osShowChannelList
*** if it is not cMenuChannelList where it is handled
*******************************************************************************/
#define osShowChannelList osUser1



#define ALL_CHANNELS    trNOOP("All Channels")
#define SOURCES         trNOOP("Sources")
#define PROVIDERS       trNOOP("Providers")
#define FAVOURITES      trNOOP("Favourites")

/** ****************************************************************************
***  GlobalFilters struct is a wapper around cChannelFilters
***
***     functions as a global variable, so that different menus may set filters
***     on channel list and the cumulative filter is shown as the result
*******************************************************************************/
struct GlobalFilters {
private:
    int expectedFilterCount;
public:
    GlobalFilters(const char* name_);
    cChannelFilters channelFilters;

    void ClearFilters();

    /*test if a current channel is to be shown*/
    bool IsAllowedChannel(cChannel*);

    /* returing true indicates addition of filter, menu will be redrawn*/
    bool AddTextFilter(const char*);
    bool AddProviderFilter(const char*);
    bool AddBouquetFilter(const char*, bool searchInFav = false, bool subStringMatch=false);

    /* creates a filter with transponder from cChannel param*/
    bool AddTransponderFilter(const cChannel*);

    bool AddSourceFilter(int code);
    bool AddRadioFilter();
    bool AddTVFilter();

    template <class T> T* GetLastFilter();

    /* true if channelFilters.Count > 0 */
    bool HasFilters() const;

    cString MakeTitle() const;

    bool SetupParse(const char*name, const char* param);
    void Save() const;
};

// must be here in the header file, else linker error
// for details http://www.parashift.com/c++-faq-lite/templates.html#faq-35.12
template <class T> T* GlobalFilters::GetLastFilter()
{
    cChannelFilter* filter = channelFilters.Last();
    T* tFilter = NULL;

    while (filter) {
        tFilter = dynamic_cast<T*> (filter);
        if (tFilter) return tFilter;

        filter = channelFilters.Prev(filter);
    }

    return NULL;
}


extern GlobalFilters globalFilters, savedGlobalFilters;
extern eKeys iKeys[];

enum eChannelSelectMode {eNoSelect = 0, eSelectToDelete, eSelectToAddToFav};
/** ******************* cMenuChannelList ***************************************
** lists all channels from 'Channels' that are permitted by global filters
*******************************************************************************/
class cMenuChannelList : public cOsdMenu
{
private:
    /* channel number of last selected channel*/
    int lastSelectedChannel;
    enum eChannelShowMode { eAllChannels, eOnlyRadioChannels, eOnlyTVChannels,
                          eUnknownMode};
    eChannelShowMode channelShowMode;
    cString red_str, green_str, yellow_str, blue_str;
    void IncChannelShowMode();
    void AddChannelShowFilter();
public:
    cMenuChannelList();
    ~cMenuChannelList();

    eOSState ProcessKey(eKeys Key);
    void SetHelpKeys();
    void Set(); /* show channel list*/
    void Display();
    void ShowSideNoteInfo();
    bool JumpToChannelNumber(int num);

    bool IsSelectedChannel(int Number);
    void SelectChannel(int Number, bool &added);

    static std::vector<int> SelectedChannels;
    static eChannelSelectMode channelSelectMode; /*select multiple channels and add to favourites*/

    // Mode : Select channels to delete them
    static void SetSelectToDeleteMode() {
        channelSelectMode = eSelectToDelete;
    }
    //Mode : Select channels to add them favourites list
    static void SetSelectToAddMode() {
        channelSelectMode = eSelectToAddToFav;
    }

    static bool IsSelectToDeleteMode() {
        return channelSelectMode == eSelectToDelete;
    }
    static bool IsSelectToAddMode() {
        return channelSelectMode == eSelectToAddToFav;
    }
    // In select mode ? ie. Not in No-select-mode
    static bool IsSelectMode() {
        return channelSelectMode != eNoSelect;
    }

    // No longer in channel select mode, forget selected channels too
    static void ClearChannelSelectMode() {
        channelSelectMode = eNoSelect;
        SelectedChannels.clear();
    }
    
    // Delete the selected channels from all channels list
    // confirms deletion from user
    static eOSState DeleteSelectedChannels();
};


/** ************************** cMenuProviderList *******************************
** list all providers in channels list filtered through global filters
**    with kOk user can 'filter' the channel list to be from this provider
*******************************************************************************/

class cMenuProviderList : public cOsdMenu
{
public:
    cMenuProviderList();
    ~cMenuProviderList();

    eOSState ProcessKey(eKeys Key);
    void Set(); /* show a list of providers permitted by global filters*/

    void JumpToProviderStartingWith(char ch);

};


/** **************** cMenuProvidersFunction *************************************
*** Menu to offer functions on provider list menu
*******************************************************************************/
class cMenuProvidersFunction : public cOsdMenu {
private:
    cString providerName;
public:
    cMenuProvidersFunction(const char* providerName_);
    void Set();
    eOSState ProcessKey(eKeys Key);
};


/** ************************ cMenuSourceList ***********************************
** list all unique sources in  channels list
**    with kOk user can 'filter' the channel list to be from this Source
*******************************************************************************/
class cMenuSourceList : public cOsdMenu
{
public:
    cMenuSourceList();
    ~cMenuSourceList();

    eOSState ProcessKey(eKeys Key);
    void Set(); /* show a list of providers*/

};
#endif /*MENU_H*/




/** **************** cMenuChannelsFunction *************************************
*** Menu to offer functions on channel-list menu
*******************************************************************************/
class cMenuChannelsFunction : public cOsdMenu {
private:
    cChannel* channel;
public:
    cMenuChannelsFunction(cChannel* chan);
    void Set();
    eOSState ProcessKey(eKeys Key);
};




class cMenuChannelSearch : public cOsdMenu
{
private:
    // search text
    char searchText[64];
public:
    cMenuChannelSearch();
    void Set();
    eOSState ProcessKey(eKeys Key);
};
