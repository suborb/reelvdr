

#include "itunes.hpp"

ITunes itunes;

//***************************************************************************
// Genres
//***************************************************************************

int genres()
{
   itunes.getGenres()->show();
   
   return success;
}

int help()
{
   printf("-----------------\n");
   printf("1 - Genres\n");
   printf("2 - Artists\n");
   printf("3 - Albums\n");
   printf("4 - Composers\n");
   printf("5 - Tracks\n");
   printf("h - Help\n");
   printf("q - Quit\n");
   printf("-----------------\n");
   
   return success;
}

//***************************************************************************
// To/From Track Path
//***************************************************************************

const char* trackPathPrefix = "ipod";

const char* toTrackPath(const char* aPath)
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

const char* fromTrackPath(const char* aPath)
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

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** args)
{
   char value[1024+TB];
   
   itunes.setPath("./iTunesDB");
 	itunes.parse();
   
   {
      ITunesService::Song* song;

      for (int i = 0; i < 10; i++)
         itunes.getGenres()->getFirst()->list->getNext();

      song = (ITunesService::Song*)itunes.getGenres()->getFirst()->list->getNext();
      
      const char* path = toTrackPath(song->path);
      printf("Track: '%s'\n", song->title);
      printf("Path: '%s'\n", song->path);
      printf("Path: '%s'\n", path);
      printf("Path: '%s'\n", fromTrackPath(path));
      
      song = itunes.findSongByPath(fromTrackPath(path));
      
      if (!song)
         printf("Song to path '%s' not found!\n", fromTrackPath(path));
      else
         printf("found '%s'\n", song->title);
   }
   
   help();
   printf("Select:");
   
   while (fgets(value, 1024, stdin))
   {
      switch (value[0])
      {
         case '1': itunes.getGenres()->show();    break;
         case '2': itunes.getArtists()->show();   break;
         case '3': itunes.getAlbums()->show();    break;
         case '4': itunes.getComposers()->show(); break;
         case '5': itunes.showTracks();           break;
         case 'h': help();                        break;
         case 'q': return success;
      }
      
      printf("Select: ");
   }
   
	return 0;
}

