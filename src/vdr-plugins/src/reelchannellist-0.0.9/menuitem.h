#ifndef MENUITEM_H
#define MENUITEM_H

#include <vdr/menuitems.h>
#include <vdr/channels.h>

// shown in source list menu
#define CURRENT_TRANSPONDER trNOOP("Current Transponder")

/** sort modes for channel items */
enum eSortModes { SortByNumber, SortByName, SortByProvider};

extern eSortModes ChannelSortMode;

/** **************** cOsdChannelItem *******************************************
*** Osd Item that shows channel and bouquet names
*******************************************************************************/
class cOsdChannelItem : public cOsdItem
{
private:
    cChannel *channel;
    const cEvent   *event; /* holds current event */

    /* show 'bouquet' names with folder icon and make them selectable */
    bool showAsFav;
    bool isMarked;
    bool isForbidden; // use forbidden folder icon and set unselectable?
public:
    cOsdChannelItem(cChannel*, bool fav=false, bool mark=false, bool forbidden=false);
    ~cOsdChannelItem();

    cChannel* Channel();
    const cChannel* Channel() const;

    const cEvent* PresentEvent();
    cString       ProgressbarString();
    void SetMarked(bool marked) { isMarked = marked; }
    void SetForbidden(bool forbid) { isForbidden = forbid;}
    bool IsMarked() const { return isMarked;}

    void Set();
    int Compare(const cListObject &ListObject) const;
};


/** ***************** eItemActionType ******************************************
*** eItemActionType indicates type of a action to be carried out when items like
*** cOsdSourceItem are selected (with kOk)
*** it can be of types:
***         Current Tranponder : filter channels from current transponder
***         New Channels :       filter new channels from current source
***         Channels :           filter all channels from current source
***         Providers :          filter all providers from current source
*******************************************************************************/
enum eItemActionType
{
    eActionCurrentTransponder,
    eActionChannels,
    eActionProviders,
    eActionNewChannels
};


/** ****************** cOsdSourceItem  *****************************************
***    Show source name and hold source's number
*******************************************************************************/
class cOsdSourceItem : public cOsdItem
{
private:
    int source;
    eItemActionType actionType;

public:
    cOsdSourceItem(int src, eItemActionType action=eActionChannels);
    ~cOsdSourceItem();

    eItemActionType ActionType() const { return actionType; }
    int Source() const { return source;}
    void Set();
};


/** ****************** cOsdProviderItem  ***************************************
***    Show Provider name and an icon before it (folder icon)
*******************************************************************************/
class cOsdProviderItem : public cOsdItem
{
private:
    cString providerName;
    ///TODO cChannel* channel;
    /* to show additional info in sidenote
    such as number of channels with this provider*/
public:
    cOsdProviderItem(const char*name);

    cString ProviderName() const {return providerName;}

    // show provider name with icon preceeding it
    void Set();
};

#endif /*MENUITEM_H*/
