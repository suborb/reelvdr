#ifndef FAVOURITES_H
#define FAVOURITES_H

#include <vdr/channels.h>
#include <vdr/osdbase.h>
#include <vdr/menu.h>
#include <vector>

struct GlobalFilters;

extern cChannels favourites;
extern GlobalFilters favouritesFilters, savedFavouritesFilters;


/** /// Wrapper functions for adding channel and folder into favourites **/

/** ********************** AddNewFavFolder *************************************
Add new folder (group sep) to favourites list with

name := folderName
after channel := channel
before channel if 'before' param is true
return true if successful in adding a new folder
*******************************************************************************/
bool AddNewFavFolder(const char* folderName,
                     cChannel* channel=NULL,
                     bool before=false);


/** **************** AddChannelToFavourites ************************************
** add given channel to a bouquet in favourites
*** makes a copy of the channel
*******************************************************************************/
bool AddChannelToFavourites(const cChannel* ch, const cChannel* favBouquet);
bool AddChannelsToFavourites(const std::vector<int>& channelNumbers, const cChannel *favBouquet);

/**  ********** AddProviderToFavourites *****************************************
*** Add channels that are provided by a single provider 'providerName'
*** to favourites list as a new folder.
********************************************************************************/
bool AddProviderToFavourites(const char* providerName);


/** **************** BouquetInFavourites ***************************************
  * returns bouquet/GroupSep of given name or NULL
  * to be used when creating a new bouquet (avoid duplicate bouquet names)
*******************************************************************************/
cChannel* IsBouquetInFavourites(const char* bouquetName);

/** **************** IsChannelInFavourites *************************************
  * returns channel if it is in favourites or NULL
  * to be used when adding channel to favourites (avoid duplicates)
*******************************************************************************/
cChannel* IsChannelInFavourites(const cChannel* ch);

/** ******************** DeleteChannelsFromFavourites **************************
  * Delete all copies of one given channel from ALL favourites folders
*******************************************************************************/
bool DeleteChannelFromFavourites(cChannel *ch);

/** *************** DeleteChannelsFromFavourites *******************************
  * deletes channels from *ALL* favourite folders (as duplication is possible)
*******************************************************************************/
bool DeleteChannelsFromFavourites(const std::vector<int> &chList);

/** ********* cMenuFavourites **************************************************
*** first shows favourite folders and when selected all channels in
*** the favourites folder
***
*** Selecting a channel should change the live-tv channel
*******************************************************************************/
class cMenuFavourites : public cOsdMenu
{
private:
    enum eMode {eFavFolders, eFavChannels};
    eMode mode;

public:
    cMenuFavourites(bool startWithCurrentChannelBouquet=false);
    ~cMenuFavourites();

    void Set();
    eOSState ProcessKey(eKeys Key);

    void ShowSideNoteInfo();
    void Display();

    void ShowNextFolder();
    void ShowPrevFolder();

    static cString lastSelectedFolder;
    int lastPosition; // last selected item
};


/** ******************* cMenuCreateFavouritesFolder ****************************
*** creates a channel with GroupSep 'on' and adds to end of favourites list
*******************************************************************************/
class cMenuCreateFavouritesFolder : public cOsdMenu
{
private:
    char folderName[128];

public:
    cMenuCreateFavouritesFolder();

    void Set();
    eOSState ProcessKey(eKeys Key);
};

/** ****************** cMenuAddChannelToFavourites *****************************
*** Select a bouquet and add give channel to favourites list
*** if no bouquet is available create a new bouquet
*******************************************************************************/
class cMenuAddChannelToFavourites:public cOsdMenu
{
private:
    std::vector<int> channelsToAdd;
    int CountFavFolders();
    bool standAlone; // return osBack after choosing a folder
    // used when '>' is pressed to add current channel to favourites

public:
    cMenuAddChannelToFavourites(std::vector<int> channels, bool standAlone_=false);
    void Set(bool selectLast = false);
    eOSState ProcessKey(eKeys Key);
};

/** ****************** cMenuMoveChannelInFavBouquet ****************************
*** Select a channel in bouquet and change its position within the bouquet
*******************************************************************************/
class cMenuMoveChannelInFavBouquet:public cOsdMenu
{
private:
    cChannel *channel;

public:
    cMenuMoveChannelInFavBouquet(cChannel *ch);
    void Set();
    eOSState ProcessKey(eKeys Key);
};


/** **************** cMenuFavouritesFunction ***********************************
*** Menu to offer functions on favourites menu
*******************************************************************************/
class cMenuFavouritesFunction : public cOsdMenu {
private:
    cChannel* channel;
public:
    cMenuFavouritesFunction(cChannel* chan);
    void Set();
    eOSState ProcessKey(eKeys Key);
};

/** ******************** cMenuRenameFavFolder ****************************
*** Menu to rename favourites bouquet
********************************************************************************/
class cMenuRenameFavFolder : public cOsdMenu {
private:
    cChannel* channel;
    char folderName[64];

    /* renames bouquet (channel name) with folderName after few checks
       (unique, channel given is a bouquet, folderName not empty)*/
    bool RenameChannel();

public:
    cMenuRenameFavFolder(cChannel* chan);
    void Set();
    eOSState ProcessKey(eKeys Key);
};



/** ******************** cMenuMoveFavFolder ****************************
*** Menu to move favourites bouquet
********************************************************************************/
class cMenuMoveFavFolder : public cOsdMenu {
private:
    cChannel* channel;

public:
    cMenuMoveFavFolder(cChannel* chan=NULL);
    void Set();
    eOSState ProcessKey(eKeys Key);
    bool IsInMoveMode() const { return channel != NULL; }
};


//
// Get the favorite folder that contains the current channel
//
cChannel* FavCurrentBouquet(bool filtered=true);
#endif /*FAVOURITES_H*/
