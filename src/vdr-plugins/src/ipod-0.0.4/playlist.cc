//***************************************************************************
/*
 * playlist.cc: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include "ipod.hpp"

#if HAVE_XINEMEDIAPLAYER_PLUGIN
const char* trackPathPrefix = "/media/hd/music/ipod";
#else
const char* trackPathPrefix = "ipod";
#endif

//***************************************************************************
// Compare Function
//***************************************************************************

int PlayList::cmpFct(void* arg1, void* arg2)
{
   return strcasecmp(((Song*)arg1)->title, ((Song*)arg2)->title);
}

//***************************************************************************
// Object
//***************************************************************************

PlayList::PlayList()
   : List(mdSorted) 
{ 
   iTunes = 0;
   *path = 0;
   setCompareFunction(cmpFct);
}

PlayList::~PlayList()
{
   if (getCount())
   {
      write();
      removeAll();
   }
}

//***************************************************************************
// Set Path
//***************************************************************************

void PlayList::setPath(const char* aPath, const char* aFile)
{
   sprintf(path, "%s/%s", aPath, aFile); 
}

//***************************************************************************
// Read
//***************************************************************************

int PlayList::read()
{
   FILE* fd;
   char line[1000+TB];
   Song* s;

   if ((fd = fopen(path, "r")) <= 0)
   {
      Cs::tell(Cs::eloError, "Error: Opening of file '%s' failed", getPath());
      return fail;
   }

   removeAll();

   while (fgets(line, 1000, fd) > 0)
   {
      // convert filename in buffer to track-name via iTunesDB

      if (!(s = iTunes->findSongByPath(fromTrackPath(line))))
         continue;

      insert(s);
   }

   fclose(fd);

   return success;
}

//***************************************************************************
// Write
//***************************************************************************

int PlayList::write()
{
   FILE* fd;
   Song* s;

   if ((fd = fopen(path, "w")) <= 0)
   {
      Cs::tell(Cs::eloError, "Error: Opening of file '%s' failed", getPath());
      return fail;
   }
   Cs::tell(Cs::eloDetail, "writing (%d) ertrys to playlist '%s'", getCount(), getPath());

#if HAVE_XINEMEDIAPLAYER_PLUGIN
    /* added m3u header for xine plugin  */
    fputs("#EXTM3U\n", fd);
#endif

   for (s = getFirst(); s; s = getNext())
   {
#if HAVE_XINEMEDIAPLAYER_PLUGIN
     /* added title for xine plugin  */
     fprintf(fd,"#EXTINF:-1,%s\n", s->title);
#endif
   fputs(toTrackPath(s->path), fd);
      fputs("\n", fd);
   }

   fclose(fd);

   return success;
}

//***************************************************************************
// To Track Path
//***************************************************************************

const char* PlayList::toTrackPath(const char* aPath)
{
   static char buf[sizePath+TB];
   char* p = buf;
   
   // in path steht sowas
   // :iPod_Control:Music:F10:gtkpod00785.mp3

   sprintf(buf, "%s%s", trackPathPrefix, aPath);   
  
   while ((p = strchr(buf, ':')))
      *p = '/';
   
   return buf;
}

//***************************************************************************
// From Track Path
//***************************************************************************

const char* PlayList::fromTrackPath(const char* aPath)
{
   static char buf[sizePath+TB];
   char* p = buf;
   
   // von
   // ipod/iPod_Control/Music/F10/gtkpod00785.mp3
   // nach
   // :iPod_Control:Music:F10:gtkpod00785.mp3
   
   sprintf(buf, "%s", aPath+strlen(trackPathPrefix));

   while ((p = strchr(buf, '/')))
      *p = ':';

   // ggf. Linefeed entfernen 

   if ((p = strchr(buf, '\n')))
      *p = 0;

   return buf;
}
