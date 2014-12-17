//***************************************************************************
/*
 * itunes.hpp - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

#ifndef __ITUNES_H__
#define __ITUNES_H__

//***************************************************************************
// Includes
//***************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>

#include "common.hpp"
#include "list.hpp"

//***************************************************************************
// iTunes Service
//***************************************************************************

class ITunesService
{

   public:

      // definitions
   
      enum Size
      {
         sizeName         = 100,
         sizeTitleName    = sizeName,
         sizeGenreName    = sizeName,
         sizeAlbumName    = sizeName,
         sizeArtistName   = sizeName,
         sizeComposerName = sizeName,
         sizeComment      = 1000
      };
      
      enum Group
      {
         grpUnknown = na,

         grpComposer,
         grpGenre,
         grpArtist,
         grpAlbum
      };
      
      struct Song
      {
         long id;
         char title[sizeTitleName+TB];
         char album[sizeAlbumName+TB];
         char artist[sizeAlbumName+TB];
         char genre[sizeGenreName+TB];
         char composer[sizeComposerName+TB];
         char path[sizePath+TB];
         char comment[sizeComment+TB];
         int type;
         long trackNo;
      };

};

//***************************************************************************
// iTunes Parser
//***************************************************************************

class ITunesParser : public ITunesService
{
   public:

      // definitions

      struct MHBD
      {
            char desc[4];
            unsigned int hlen;
            unsigned int rlen;
      };

      struct MHSD
      {
            char desc[4];
            unsigned int hlen;
            unsigned int rlen;
            unsigned int idx;
      };

      struct MHLP
      {
            char desc[4];
            unsigned int hlen;
            unsigned int rlen;
      };

      struct MHYP
      {
            char desc[4];
            unsigned int hlen;
            unsigned int rlen;
            unsigned int idx;
            unsigned int songcount;
            unsigned int type;
            unsigned int timestamp;
            unsigned int unknown[2];
      };

      struct MHIP
      {
            char desc[4];
            unsigned int hlen;
            unsigned int unknown;
            unsigned int hiphod[2];
            unsigned int trackno;
            unsigned int tunesID;
      };

      struct MHLT
      {
            char desc[4];
            unsigned int hlen;
            unsigned int rlen;
      };

      struct MHIT
      {
            char desc[4];
            unsigned int hlen;
            unsigned int tlen;
            unsigned int mhod_cnt;
            unsigned int id;
            unsigned int dummy[3];
            unsigned int modtime;
            unsigned int size;
            unsigned int duration;
            unsigned int trackno;
            unsigned int dummy2;
            unsigned int year;
            unsigned int bitrate;
            unsigned int dummy3[5];
            unsigned int playcount;
            unsigned int playcount2;	
            unsigned dummy4[8];
            unsigned char info0;
            unsigned char rating;
            unsigned char info1[2];
            unsigned int dummy5;
      };

      struct MHOD
      {
            char desc[4];
            unsigned int dummy3;
            unsigned int strsize;
            unsigned int type;
            unsigned int dummy[2];
            unsigned int id;
            unsigned int dummy2[3]; 
      };

      enum 
      { 
         TITLE    = 0x01, 
         PATH     = 0x02, 
         ALBUM    = 0x03, 
         ARTIST   = 0x04, 
         GENRE    = 0x05,
         TYPE     = 0x06,
         COMMENT  = 0x08, 
         COMPOSER = 0x0C, 
         WHATEVER = 0x64
      };

      // Object

      ITunesParser();
      ~ITunesParser();

      // interface

      int init(char* path, List* songs);
      int exit();
      int parse();

   protected:

      // functions

      int starCode2Int(char b);

      void parseTracks();
      void parseTrack();
      void parseTrackItem(unsigned int id, Song* s);

      void parsePlaylists();
      void parsePlaylist();
      void parsePlaylistEntry(unsigned int list);

      // data

      List* songs;
      char buffer1[100000];
      FILE* fd;
      char buffer2[100000];
};

//***************************************************************************
// Container Service
//***************************************************************************

class ContainerService
{
   public:

      // definitions

      enum 
      { 
         sizeName = 100 
      };

      struct Item
      {
         char name[sizeName+TB];
         List* list;
      };
};

//***************************************************************************
// Container List
//***************************************************************************

class ContainerList : public List, public ContainerService
{
   public:

      // object

      ContainerList(Mode aMode, const char* aName, int aGroup = na);

      // interface
      
      static int cmpFct(void* arg1, void* arg2);

      Item* getFirst() { return (Item*)List::getFirst(); }
      Item* getNext()  { return (Item*)List::getNext(); }

      Item* findByName(const char* name);
      Item* append(const char* name);
      int freeAll();
      void setName(const char* aName) { strcpy(name, aName); }

      const char* getName()           { return name; }
      int getGroup()                  { return group; }

      int show();

   protected:

      // data

      char name[sizeName+TB];                 // my name
      int group;
};

//***************************************************************************
// ITunes
//***************************************************************************

class ITunes : public ITunesService, public ContainerService
{
   public:

      // object
      
      ITunes();
      ~ITunes();

      // interface

      static int cmpFct(void* arg1, void* arg2);

      int parse();
      int showTracks();

      ContainerList* getGenres()       { return genres; }
      ContainerList* getAlbums()       { return albums; }
      ContainerList* getArtists()      { return artists; }
      ContainerList* getComposers()    { return composers; }

      List* getSongs()                 { return songs; }

      void setPath(const char* p)      { strcpy(path, p); }
      int clear();
      int clearGroupLists();
      
      ContainerList* createGroupList(List* aList, const char* aName, Group aGroup);
      Song* findSongByPath(const char* aPath);

   protected:

      // Data
      
      ITunesParser* parser;
      List* songs;

      ContainerList* genres;
      ContainerList* albums;
      ContainerList* artists;
      ContainerList* composers;

      List* groupLists;
      char path[sizePath+TB];

};

//***************************************************************************
#endif // __ITUNES_H__
