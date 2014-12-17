//***************************************************************************
/*
 * menu.cc - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/interface.h>
#include <vdr/menuitems.h>
#include <vdr/osdbase.h>

#include "ipod.hpp"
#include "menu.hpp"

//***************************************************************************
// Statics
//***************************************************************************

cPluginIpod* cPodMenuBase::iPod = 0;

//***************************************************************************
// iPod Menu Base Class
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPodMenuBase::cPodMenuBase(const char* aTitle, int c0, int c1)
         : cOsdMenu(aTitle, c0, c1)
{ 
   strcpy(myTitle, aTitle);

   // get some pointer for easier access ;-)

   iTunes = iPod->getITunes();
   playlist = iPod->getPlaylist();
   autolist = iPod->getAutolist();
}

//***************************************************************************
// Compare Function
//***************************************************************************

void cPodMenuBase::init(cPluginIpod* parent)
{ 
   iPod = parent;
}

//***************************************************************************
// Add To Playlist
//***************************************************************************

int cPodMenuBase::addToPlaylist(PlayList* aPlaylist, int silent)
{
   ITunesService::Song* song;

   silent |= getCurrent()->isType(mtTrack);

   if (!silent)
      if (!(Interface->Confirm(tr("Add recursivly?"))))
         return done;

   if (getCurrent()->isType(mtContainer))
   {
      List* list;

      if (!(list = ((cContainerItem*)getCurrent())->getTrackList()))
         return fail;

      // Add all track to playlist

      for (song = (ITunesService::Song*)list->getFirst(); song; 
           song = (ITunesService::Song*)list->getNext())
      {
         if (!(aPlaylist->search(song)))
            aPlaylist->insert(song);
      }
   }

   else if (getCurrent()->isType(mtContainerList))
   {
      ContainerList* containerList;
      ContainerService::Item* item;

      if (!(containerList = ((cContainerListItem*)getCurrent())->getContainerList()))
         return fail;

      // Add all track to playlist

      for (item = containerList->getFirst(); item; item = containerList->getNext())
      {
         for (song = (ITunesService::Song*)item->list->getFirst(); song; 
              song = (ITunesService::Song*)item->list->getNext())
         {
            if (!(aPlaylist->search(song)))
               aPlaylist->insert(song);
         }
      }
   }

   else if (getCurrent()->isType(mtTrack))
   {
      if (!(song = ((cTrackItem*)getCurrent())->getTrack()))
         return fail;

      if (aPlaylist->search(song))
         return done;

      aPlaylist->insert(song);
   }

   else
   {
      tell(eloInfo, "Sorry not supported yet :-(");
      return done;
   }

   if (!silent)
      tell(eloInfo, "done");
   return success;
   
}

//***************************************************************************
// Play
//***************************************************************************

eOSState cPodMenuBase::play()
{
   autolist->removeAll();

   if (addToPlaylist(autolist, yes) == success)
      iPod->play(autolist, this);

    return osEnd;
}

//***************************************************************************
// iPod Main Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPodMenu::cPodMenu()
   : cPodMenuBase()
{
   playlist->read();
   SetHelp(tr("Play"), tr("Add"), NULL, tr("Playlist"));
}

cPodMenu::~cPodMenu()
{
   playlist->write();
}

//***************************************************************************
// Copy
//***************************************************************************

int cPodMenu::fill()
{
   ContainerService::Item aItem;
   strcpy(aItem.name, "Songs");
   aItem.list = iTunes->getSongs();

   cOsdMenu::Add(new cContainerListItem(iTunes->getArtists()));
   cOsdMenu::Add(new cContainerListItem(iTunes->getAlbums()));
   cOsdMenu::Add(new cContainerItem(&aItem, yes));
   cOsdMenu::Add(new cContainerListItem(iTunes->getGenres()));
   cOsdMenu::Add(new cContainerListItem(iTunes->getComposers()));

   return success;
}

//***************************************************************************
// Sub Menu
//***************************************************************************

eOSState cPodMenu::subMenu()
{
   eOSState state = osContinue;
   char* buf;
   cPodMenuItemBase* item = getCurrent();

   if (HasSubMenu() || !item)
      return osContinue;

   if (item->isType(mtContainerList))
   {
      asprintf(&buf, "%s -> %s", getTitle(), 
               ((cContainerListItem*)item)->getContainerList()->getName());

      state =  AddSubMenu(new cContainerMenu(((cContainerListItem*)item)->getContainerList(), buf));
   }

   else if (item->isType(mtContainer))
   {
      asprintf(&buf, "%s -> %s", getTitle(),
               ((cContainerItem*)item)->getContainerItem()->name);

      state = AddSubMenu(new cTrackMenu(((cContainerItem*)item)->getTrackList(), buf));
   }

   free(buf);

   return state;
}

//***************************************************************************
// Playlist
//***************************************************************************

eOSState cPodMenu::playlistMenu()
{
   return AddSubMenu(new cPlaylistMenu(playlist, tr("Playlist")));
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cPodMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

   if (HasSubMenu()) return state;

   if (state == osUnknown)
   {
      switch (key)
      {
         case kOk:     state = subMenu();       break;
         case k1:
         case kRed:    state = play();          break;
         case k2:
         case kGreen:  addToPlaylist(playlist); Skins.Message(mtStatus, tr("Add to playlist")); break;
         case k4:   
         case kBlue:   state = playlistMenu();  break;
         default:                               break;
      }
   }

   return state;
}

//***************************************************************************
// Playlist Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPlaylistMenu::cPlaylistMenu(List* aList, const char* aTitle)
   : cPodMenuBase(aTitle)
{
   list = aList;

   fill();

   SetHelp(NULL, NULL, tr("Delete"), tr("Clear"));
}

//***************************************************************************
// Fill
//***************************************************************************

int cPlaylistMenu::fill()
{
   ITunesService::Song* s;

   for (s = (ITunesService::Song*)list->getFirst(); s; s = (ITunesService::Song*)list->getNext())
      cOsdMenu::Add(new cTrackItem(s));

   return success;
}

//***************************************************************************
// Delete Selected Entry
//***************************************************************************

int cPlaylistMenu::deleteItem()
{
   cTrackItem* item;

   if (!(Interface->Confirm(tr("Delete entry?"))))
      return done;

   if (!(item = (cTrackItem*)getCurrent()))
      return fail;
   
   // remove from list

   list->remove(item->getTrack());
   cOsdMenu::Del(Current());
   Display();

   return success;
}

//***************************************************************************
// Clear List
//***************************************************************************

int cPlaylistMenu::clearList()
{
   if (!(Interface->Confirm(tr("Clear playlist?"))))
      return done;

   list->removeAll();

   cOsdMenu::Clear();
   Display();

   return done;
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cPlaylistMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

   if (state == osUnknown)
   {
      switch (key)
      {
         case kOk: state = play(); break;
         case k3:
         case kYellow: deleteItem(); break;
         case k4:   
         case kBlue:   clearList();  break;
         default:                    break;
      }
   }

   return state;
}

//***************************************************************************
// Container Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cContainerMenu::cContainerMenu(ContainerList* aList, const char* aTitle, int aLevel)
   : cPodMenuBase(aTitle)
{
   level = aLevel;
   list = aList;
   fill();

   SetHelp(tr("Play"), tr("Add"), NULL, NULL);
}

//***************************************************************************
// Fill
//***************************************************************************

int cContainerMenu::fill()
{
   ContainerService::Item* item;
   ContainerList* containerList;
   ITunesService::Group group;

   switch (list->getGroup())
   {
      case ITunesService::grpComposer: group = ITunesService::grpAlbum;   break;
      case ITunesService::grpGenre:    group = ITunesService::grpArtist;  break;
      case ITunesService::grpArtist:   group = ITunesService::grpAlbum;   break;
      case ITunesService::grpAlbum:    group = ITunesService::grpUnknown; break;

      default: group = ITunesService::grpUnknown;
   }

   if (level == 1 && group != ITunesService::grpUnknown)
   {
      iTunes->clearGroupLists();

      for (item = list->getFirst(); item; item = list->getNext())
      {
         containerList = iTunes->createGroupList(item->list, item->name, group);
         cOsdMenu::Add(new cContainerListItem(containerList));
      }
   }
   else
   {
      for (item = list->getFirst(); item; item = list->getNext())
         cOsdMenu::Add(new cContainerItem(item));
   }

   return success;
}

//***************************************************************************
// Copy
//***************************************************************************

eOSState cContainerMenu::subMenu()
{
   eOSState state = osContinue;
   char* buf = 0;
   cPodMenuItemBase* item = getCurrent();

   if (HasSubMenu() || !item)
      return osContinue;

   asprintf(&buf, "%s -> %s", getTitle(), item->isType(mtContainerList) ? 
            ((cContainerListItem*)item)->getContainerList()->getName() : 
            ((cContainerItem*)item)->getContainerItem()->name);

   if (item->isType(mtContainerList))
      state =  AddSubMenu(new cContainerMenu(((cContainerListItem*)item)->getContainerList(), buf, level+1));
   else if (item->isType(mtContainer))
      state = AddSubMenu(new cTrackMenu(((cContainerItem*)item)->getTrackList(), buf));

   free(buf);

   return state;
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cContainerMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

   if (HasSubMenu()) return state;

   if (state == osUnknown)
   {
      switch (key)
      {
         case kOk:     state = subMenu();
	 break;
         case k1:
         case kRed:    state = play();          break;
         case k2:
         case kGreen:  addToPlaylist(playlist); Skins.Message(mtStatus, tr("Add to playlist")); break;

         default:      break;
      }
   }

   return state;
}

//***************************************************************************
// Track Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cTrackMenu::cTrackMenu(List* aList, const char* aTitle)
   : cPodMenuBase(aTitle)
{
   list = aList;
   fill();
   //SetHelp(NULL, tr("Add"), NULL, NULL);
   SetHelp(NULL, tr("Add"), NULL, NULL);
}

//***************************************************************************
// Fill
//***************************************************************************

int cTrackMenu::fill()
{
   ITunesService::Song* song;

   for (song = (ITunesService::Song*)list->getFirst(); song; song = (ITunesService::Song*)list->getNext())
      cOsdMenu::Add(new cTrackItem(song));
   
   return success;
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cTrackMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

   // if (HasSubMenu()) return state;

   if (state == osUnknown)
   {
      switch (key)
      {
         case k1:
         case kOk:   state = play();          break;
         case k2:
         case kGreen: addToPlaylist(playlist); Skins.Message(mtStatus, tr("Add to playlist")); break;

         default:                              break;
      }
   }

   return state;
}

//***************************************************************************
// Menu Items
//***************************************************************************
//***************************************************************************
// Container List Item
//***************************************************************************

cContainerListItem::cContainerListItem(ContainerList* p)
{
   char* buf = 0;

   containerList = p;
   type = mtContainerList;
   
   asprintf(&buf, "%s   ->", p->getName());
   SetText(buf, /*copy*/ false);
}

//***************************************************************************
// Container Item
//***************************************************************************

cContainerItem::cContainerItem(ContainerService::Item* p, int copy)
{
   char* buf = 0;

   copied = copy;
   type = mtContainer;

   if (copied)
   {
      containerItem = new ContainerService::Item;
      *containerItem = *p;
   }
   else
      containerItem = p;

   asprintf(&buf, "%s   ->", p->name);
   SetText(buf, /*copy*/ false);
}

cContainerItem::~cContainerItem()
{
   if (copied && containerItem) delete containerItem;
}

//***************************************************************************
// Track Item
//***************************************************************************

cTrackItem::cTrackItem(ITunesService::Song* s)
{
   char* buf = 0;
   track = s;
   type = mtTrack;

   asprintf(&buf, "%s - %s", s->title, s->artist);
   SetText(buf, /*copy*/ false);
}
