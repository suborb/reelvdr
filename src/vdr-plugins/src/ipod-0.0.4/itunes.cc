//***************************************************************************
/*
 * itunes.cc - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include "itunes.hpp"

#define NONAME "- no name -"

//***************************************************************************
// Class Container List
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

ContainerList::ContainerList(Mode aMode, const char* aName, int aGroup)
   : List(aMode)
{
   group = aGroup;
   setCompareFunction(cmpFct);
   strcpy(name, aName);
}

//***************************************************************************
// Compare Function
//***************************************************************************

int ContainerList::cmpFct(void* arg1, void* arg2)
{
   return strcasecmp(((Item*)arg1)->name, ((Item*)arg2)->name);
}

//***************************************************************************
// Find By Name
//***************************************************************************

ContainerService::Item* ContainerList::findByName(const char* name)
{
   Item* i;
   const char* s;

   s = (!Cs::isEmpty(name)) ? name : NONAME;

   for (i = getFirst(); i; i = getNext())
   {
      if (strcmp(i->name, s) == 0)
         return i;
   }
   
   return 0;
}

//***************************************************************************
// Append
//***************************************************************************

ContainerService::Item* ContainerList::append(const char* name)
{
   Item* i;

   i = new Item;
   List::append(i);

   strcpy(i->name, !Cs::isEmpty(name) ? name : NONAME);
   i->list = new List;

   return i;
}


//***************************************************************************
// Free All
//***************************************************************************

int ContainerList::freeAll()
{
   Item* i;

   while ((i = getFirst()))
   {
      remove(i);
      delete i->list;
      delete i;
   }

   return success;
}

int ContainerList::show()
{
   Item* i;

   printf("----- %d %s -----\n", getCount(), getName());

   for (i = getFirst(); i; i = getNext())
      printf("'%-30.30s' with %d titles\n", i->name, i->list->getCount());

   return success;
}

//***************************************************************************
// Class ITunes
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

ITunes::ITunes()
{
   parser = new ITunesParser;
   songs = new List;
   songs->setCompareFunction(cmpFct);

   artists = new ContainerList(List::mdNone, "Artists", grpArtist);
   albums = new ContainerList(List::mdNone, "Albums", grpAlbum);
   genres = new ContainerList(List::mdNone, "Genres", grpGenre);
   composers = new ContainerList(List::mdNone, "Composers", grpComposer);
   // groupList = new ContainerList(List::mdNone, "groupList");
   
   groupLists = new List;
}

ITunes::~ITunes()
{
   clear();
   
   if (parser)     delete parser;
   if (genres)     delete genres;
   if (albums)     delete albums;
   if (artists)    delete artists;
   if (composers)  delete composers;
   //if (groupList) delete groupList;
   
   if (groupLists) delete groupLists;
   if (songs)      delete songs;
}

//***************************************************************************
// Clear
//***************************************************************************

int ITunes::clear()
{
   Song* s;

   if (genres)    genres->freeAll();
   if (albums)    albums->freeAll();
   if (artists)   artists->freeAll();
   if (composers) composers->freeAll();
   //if (groupList) groupList->freeAll();

   if (groupLists) clearGroupLists();

   if (songs)
   {
      while ((s = (Song*)songs->getFirst()))
      {
         songs->remove(s);
         delete s;
      }
   }

   return done;
}

//***************************************************************************
// Clear Group Lists
//***************************************************************************

int ITunes::clearGroupLists()
{
   ContainerList* c;

   while ((c = (ContainerList*)groupLists->getFirst()))
   {
      groupLists->remove(c);
      c->freeAll();
      delete c;
   }

   return done;
}

//***************************************************************************
// Compare Function
//***************************************************************************

int ITunes::cmpFct(void* arg1, void* arg2)
{
   return strcasecmp(((Song*)arg1)->title, ((Song*)arg2)->title);
}

//***************************************************************************
// Parse
//***************************************************************************

int ITunes::parse()
{
   Song* s;
   Item* item;
   
   if (!path || !*path)
      return fail;

   if (parser->init(path, songs) != success)
      return fail;

   if (parser->parse() != success)
      return fail;

   // important !
   // exit is closing iTunesDB !!

   parser->exit();

   // sort song list

   songs->sort();

   // ...

   for (s = (Song*)songs->getFirst(); s; s = (Song*)songs->getNext())
   {
      // append to genre list

      if (!(item = genres->findByName(s->genre)))
         item = genres->append(s->genre);

      item->list->append(s);

      // append to artist list

      if (!(item = artists->findByName(s->artist)))
         item = artists->append(s->artist);

      item->list->append(s);

      // append to album list

      if (!(item = albums->findByName(s->album)))
         item = albums->append(s->album);

      item->list->append(s);

      // append to composer list

      if (!(item = composers->findByName(s->composer)))
         item = composers->append(s->composer);

      item->list->append(s);
   }

   genres->sort();
   artists->sort();
   albums->sort();
   composers->sort();
   
   return success; 
}

//***************************************************************************
// Find Song By Path
//***************************************************************************

ITunesService::Song* ITunes::findSongByPath(const char* aPath)
{
   ITunesService::Song* s;

   for (s = (ITunesService::Song*)songs->getFirst(); s; s = (ITunesService::Song*)songs->getNext())
   {
      if (strcmp(s->path, aPath) == 0)
         return s;
   }
   
   return 0;
}

//***************************************************************************
// Parse
//***************************************************************************

int ITunes::showTracks()
{
   Song* s;

   for (s = (Song*)songs->getFirst(); s; s = (Song*)songs->getNext())
   {
      printf("[%4.4ld] %3ld.) %-40.40s - %-40.40s %-15.15s\n",
             s->id, s->trackNo, s->title, s->artist, s->genre);
   }

   return success; 
}

//***************************************************************************
// Create List By Artist
//***************************************************************************

ContainerList* ITunes::createGroupList(List* aList, const char* aName, Group aGroup)
{
   ContainerList* containerList;
   Item* item;
   Item* itemAll;
   Song* s;
   const char* groupAttibute;

   containerList = new ContainerList(List::mdNone, aName);
   groupLists->append(containerList);

   // mit dem am Anfabg Blank steht's oben ;-) meistens ...

   itemAll = containerList->append(" All");

   for (s = (Song*)aList->getFirst(); s; s = (Song*)aList->getNext())
   {
      switch (aGroup)
      {
         case grpArtist:   groupAttibute = s->artist;   break;
         case grpGenre:    groupAttibute = s->genre;    break;
         case grpAlbum:    groupAttibute = s->album;    break;
         case grpComposer: groupAttibute = s->composer; break;

         case grpUnknown:  continue;
         default:          continue;
      }

      // search, append group attribute

      if (!(item = containerList->findByName(groupAttibute)))
         item = containerList->append(groupAttibute);

      // append to this artist item

      item->list->append(s);

      // and each track to the "All" Item

      itemAll->list->append(s);
   }

   containerList->sort();

   return containerList;
}
