//***************************************************************************
/*
 * menu.hpp - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

#ifndef __MENU_H__
#define __MENU_H__

#include <vdr/osdbase.h>
#include "common.hpp"
#include "itunes.hpp"
#include "ipod.hpp"

//***************************************************************************
// Class Menu Service
//***************************************************************************

class cPodMenuService : public Cs
{
   public:

      enum Size
      {
         sizeTitle = 100
      };

      enum MenuItemType
      {
         mtUnknown,
         mtContainerList,
         mtContainer,
         mtTrack,
      };
};

//***************************************************************************
// The Menu Item Classes
//***************************************************************************
//***************************************************************************
// Class Pod Menu Item - Menu Item Base Class
//***************************************************************************

class cPodMenuItemBase : public cOsdItem, public cPodMenuService
{
      
   public:

      cPodMenuItemBase()                { type = mtUnknown; }
      MenuItemType getType()            { return type; }
      int isType(MenuItemType t)        { return type == t; }

   protected:

      MenuItemType type;
};

//***************************************************************************
// Class Container List Item - represents a Container List
//***************************************************************************

class cContainerListItem : public cPodMenuItemBase
{
      
   public:

      cContainerListItem(ContainerList* p);
      ContainerList* getContainerList()          { return containerList; }

   protected:

      ContainerList* containerList;
};

//***************************************************************************
// Class Container Item - represents a Container Item
//***************************************************************************

class cContainerItem : public cPodMenuItemBase
{
      
   public:

      cContainerItem(ContainerService::Item* p, int copy = no);
      ~cContainerItem();

      ContainerService::Item* getContainerItem() { return containerItem; }
      List* getTrackList()                       { return containerItem->list; }

   protected:

      ContainerService::Item* containerItem;
      int copied;
};

//***************************************************************************
// Class Track Item - represents a single Track
//***************************************************************************

class cTrackItem : public cPodMenuItemBase
{
      
   public:
      
      cTrackItem(ITunesService::Song* s);
      ITunesService::Song* getTrack()    { return track; }

   protected:

      ITunesService::Song* track;
};


//***************************************************************************
// The Menu Classes
//***************************************************************************
//***************************************************************************
// Class Pod Menu Base
//***************************************************************************

class cPodMenuBase : public cOsdMenu, public cPodMenuService
{
   public:

      cPodMenuBase(const char* aTitle = "iPod", int c0 = 10, int c1 = 10);

      const char* getTitle()         { return myTitle; }
      cPodMenuItemBase* getCurrent() { return (cPodMenuItemBase*)Get(Current()); }

   protected:

      int addToPlaylist(PlayList* aPlaylist, int silent = no);

      // data

      char myTitle[sizeTitle+TB];
      ITunes* iTunes;
      PlayList* playlist;
      PlayList* autolist;
      eOSState play();

      // statics

      static cPluginIpod* iPod;

   public:

      static void init(cPluginIpod* parent);
};

//***************************************************************************
// Class Pod Menu
//***************************************************************************

class cPodMenu : public cPodMenuBase
{
      
   public:
      
      cPodMenu();
      ~cPodMenu();
      
      virtual eOSState ProcessKey(eKeys key);
      int fill();
      
   protected:
      
      eOSState subMenu();
      eOSState playlistMenu();
};

//***************************************************************************
// Class Container Menu
//***************************************************************************

class cContainerMenu : public cPodMenuBase
{
      
   public:
      
      cContainerMenu(ContainerList* aList, const char* aTitle, int aLevel = 1);
      
      virtual eOSState ProcessKey(eKeys key);
      
   protected:

      int fill();      
      eOSState subMenu();

      // data

      ContainerList* list;
      int level;
};

//***************************************************************************
// Class Track Menu
//***************************************************************************

class cTrackMenu : public cPodMenuBase
{
      
   public:
      
      cTrackMenu(List* aList, const char* aTitle);
      
      virtual eOSState ProcessKey(eKeys key);
      int fill();
      
   protected:
      // data

      List* list;
};

//***************************************************************************
// Class Playlist Menu
//***************************************************************************

class cPlaylistMenu : public cPodMenuBase
{
      
   public:
      
      cPlaylistMenu(List* aList, const char* aTitle);
      
      virtual eOSState ProcessKey(eKeys key);
      
   protected:
      
      int fill();
      int deleteItem();
      int clearList();

      // data

      List* list;
};

//***************************************************************************
#endif // __MENU_H__
