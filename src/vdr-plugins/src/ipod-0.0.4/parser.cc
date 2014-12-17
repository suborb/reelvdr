
//***************************************************************************
// Includes
//***************************************************************************

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <iconv.h>

#include "itunes.hpp"

//***************************************************************************
// Tell
//***************************************************************************

int tell(const char* format, ...)
{
   va_list more;
   char buf[1000+TB];

   if (format)
   {
      va_start(more, format);

      sprintf(buf, "[ipod]");
      vsprintf(buf+strlen(buf), format, more);

      printf("%s\n", buf);
   }

   return fail;
}

//***************************************************************************
// Tools
//***************************************************************************

void l2b(unsigned int* value)
{
#ifdef BIGENDIAN

	unsigned int t;

	t = (    (*value & 0xFF000000) >> 24) | ((*value & 0x00FF0000) >>  8)
         | ((*value & 0x0000FF00) <<  8) | ((*value & 0x000000FF) << 24);	

	*value = t;

#endif	
}

unsigned int utf16to8(char* src, unsigned int len, char* dst)
{
   static int isInitialized = 0;
   static iconv_t uni;

	char *s, *d;
	size_t slen = len;
	size_t dlen = 2*slen;
   unsigned int result = 0;
	s = src;
	d = dst;

   if (!isInitialized)
   {
      isInitialized = 1;

      // if ((uni = iconv_open("UTF-8", "UTF-16LE")) == (iconv_t)-1)
      if ((uni = iconv_open("UTF8", "UTF16LE")) == (iconv_t)-1)
         tell("Fatal: Could not init unicode translator UTF-8 to UTF-16LE, errno was '%s'", 
              strerror(errno));

      // if (iconv_open("ISO-8859-1", "UTF-8") == (iconv_t)-1)
      //   tell("Fatal: Could not init unicode translator: '%s'", strerror(errno));

      // if (iconv_open("ISO-8859-1", "UTF-16LE") == (iconv_t)-1)
      //   tell("could not init unicode translator: '%s'", strerror(errno));


   }

   if (uni != (iconv_t)-1)
   {
      if (iconv(uni, &s, &slen, &d, &dlen) == (size_t) -1)
         tell("could not translate to UTF-8: '%s'", strerror(errno));

      result = 2*len - dlen;
   }
   else
   {
      strncpy(dst, src, len);
      dst[len] = 0;

      result = len;
   }

   return result;
}

//***************************************************************************
// Object
//***************************************************************************

ITunesParser::ITunesParser()
{
   songs = 0;
}

ITunesParser::~ITunesParser()
{
}

//***************************************************************************
// Star Code to Integer
//***************************************************************************

int ITunesParser::starCode2Int(char b)
{
	switch (b) 
   {
		case 0x64: return 5;
		case 0x50: return 4;
		case 0x3c: return 3;
		case 0x28: return 2;
		case 0x14: return 1;
		case 0x00: return 0;
		default:   return na;
	}
}

//***************************************************************************
// Parse Track Item
//***************************************************************************

void ITunesParser::parseTrackItem(unsigned int id, Song* s)
{
	MHOD mhod;
	char *utf8, *utf16;
	unsigned int utf8len, utf16len;
   int res;

	if ((res = ::fread(&mhod, sizeof(MHOD), 1, fd)) != 1)
		tell("could not fread: '%s'", strerror(errno));
	
	if (strncmp("mhod", mhod.desc, 4) != 0)
		tell("could not find MHOD header");
		
	l2b(&mhod.id);
	l2b(&mhod.type);
	l2b(&mhod.strsize);

	utf16len = (mhod.strsize - sizeof(MHOD));
	utf16 = (char*)malloc(utf16len);
	fread(utf16, 1, utf16len, fd);

	utf8 = (char*) malloc(2*utf16len);
	utf8len = utf16to8(utf16, utf16len, utf8);

	switch (mhod.type)
   {
		case TITLE:    strncpy(s->title, utf8, utf8len);    s->title[utf8len] = 0;    break;
		case ALBUM:    strncpy(s->album, utf8, utf8len);    s->album[utf8len] = 0;    break;
		case ARTIST:   strncpy(s->artist, utf8, utf8len);   s->artist[utf8len] = 0;   break;
		case GENRE:    strncpy(s->genre, utf8, utf8len);    s->genre[utf8len] = 0;    break;
		case COMPOSER: strncpy(s->composer, utf8, utf8len); s->composer[utf8len] = 0; break;
		case PATH:     strncpy(s->path, utf8, utf8len);     s->path[utf8len] = 0;     break;
		case COMMENT:  strncpy(s->comment, utf8, utf8len);  s->comment[utf8len] = 0;  break;
		case TYPE:     break;

		default: tell("read unknown mhod type (%ld) '%s'", mhod.type, utf8);          break;
	}

	free(utf8);
	free(utf16);
}

//***************************************************************************
// Parse Track
//***************************************************************************

void ITunesParser::parseTrack()
{
   Song* s;
	MHIT mhit;
	static unsigned int id = 0;
	int res;

	if ((res = fread(&mhit, sizeof(MHIT), 1, fd)) != 1)
		tell("could not fread: '%s'",strerror(errno));

	if (strncmp("mhit", mhit.desc, 4) != 0)
		tell("could not find MHIT header");

	if (fseek(fd, mhit.hlen-sizeof(MHIT), SEEK_CUR) != 0)
		tell("could not fseek: '%s'", strerror(errno));

   id++;

   s = new Song;
   songs->append(s);

   memset(s, 0, sizeof(Song));

   s->id = id;
   s->trackNo = mhit.trackno;
   
	for (unsigned int i = 0; i < mhit.mhod_cnt; i++)
		parseTrackItem(id, s);

   // tell("Debug: [%ld] (%3ld) %-40.40s - %-40.40s %-15.15s",
   //        s->id, s->trackNo, s->artist, s->title, s->genre);
}

//***************************************************************************
// Parse Tracks
//***************************************************************************

void ITunesParser::parseTracks()
{
	MHSD mhsd;
	MHLT mhlt;
   int res;

	if ((res = ::fread(&mhsd, sizeof(MHSD), 1, fd)) != 1)
		tell("could not fread: '%s' (%d), res was (%d)",strerror(errno), errno, res);
		
	if (strncmp("mhsd", mhsd.desc, 4) != 0)
		tell("could not find MHSD header");
			
	l2b(&mhsd.hlen);
	l2b(&mhsd.rlen);		

	if (fseek(fd, mhsd.hlen-sizeof(MHSD), SEEK_CUR) != 0)
		tell("could not fseek: '%s'", strerror(errno));
	
	if ((res = fread(&mhlt, sizeof(MHLT), 1, fd)) != 1)
		tell("could not fread: '%s'", strerror(errno));
		
	if (strncmp("mhlt", mhlt.desc, 4) != 0)
		tell("could not find MHLT header");
		
	l2b(&mhlt.hlen);	
	l2b(&mhlt.rlen);	

	if (fseek(fd, mhlt.hlen-sizeof(MHLT), SEEK_CUR) != 0)
		tell("could not fseek: '%s'",strerror(errno));
	
	while ((long)ftell(fd) < (long)mhsd.rlen)
		parseTrack();
}

//***************************************************************************
// Parse Playlist Entry
//***************************************************************************

void ITunesParser::parsePlaylistEntry(unsigned int list)
{
	MHIP mhip;
	MHOD mhod;
	char *utf16;
	unsigned int utf16len;
   unsigned int res;

	if ((res = fread(&mhip, sizeof(MHIP), 1, fd)) != 1)
		tell("could not fread: '%s'", strerror(errno));

	if (strncmp("mhip", mhip.desc, 4) != 0)
		tell("could not find MHIP header");

	l2b(&mhip.hlen);	
	l2b(&mhip.unknown);
	l2b(&mhip.hiphod[0]);
	l2b(&mhip.hiphod[1]);
	l2b(&mhip.trackno);
	l2b(&mhip.tunesID);
	
	if (list > 1) // first playlist is all songs on pod
      tell("addSong trano (%ld)", mhip.trackno);
		
	if (fseek(fd, mhip.hlen-sizeof(MHIP), SEEK_CUR) != 0)
		tell("could not fseek: '%s'", strerror(errno));			

	if ((res = fread(&mhod, sizeof(MHOD), 1, fd)) != 1)
		tell("could not fread: '%s'",strerror(errno));

	if (strncmp("mhod", mhod.desc, 4) != 0)
		tell("could not find MHOD");

	l2b(&mhod.type);
	l2b(&mhod.strsize);

	utf16len = mhod.strsize-sizeof(MHOD);
	utf16 = (char*)malloc(utf16len);

	if ((res = fread(utf16, 1, utf16len, fd)) != utf16len)
		tell("could not fread: '%s'",strerror(errno));

	free(utf16);
}

//***************************************************************************
// Parse Playlist
//***************************************************************************

void ITunesParser::parsePlaylist()
{
	MHYP mhyp;
	unsigned int idx;
	unsigned int songcount= 0;
	unsigned int playlist= 0;
	unsigned int i;
	unsigned int res; 

	if ((res = fread(&mhyp, sizeof(MHYP), 1, fd)) != 1)
		tell("could not fread: '%s'",strerror(errno));

	if (strncmp("mhyp", mhyp.desc, 4) != 0)
		tell("could not find MHYP header");

	l2b(&mhyp.hlen);	
	l2b(&mhyp.songcount);	
	l2b(&mhyp.type);	
	l2b(&mhyp.idx);
	
	idx= mhyp.idx;
	songcount= mhyp.songcount;

	if (fseek(fd, mhyp.hlen-sizeof(MHYP), SEEK_CUR) != 0)
		tell("could not fseek: '%s'", strerror(errno));

	// REM $idx mhods?

	for (i = 0; i < idx; i++) 
   { 
		MHOD mhod;
		char *utf16,*utf8;
		unsigned int utf8len, utf16len;

		if ((res = fread(&mhod, sizeof(MHOD), 1, fd)) != 1)
			tell("could not fread: '%s'",strerror(errno));

		if (strncmp("mhod", mhod.desc, 4) != 0)
			tell("could not find MHOD");

		l2b(&mhod.type);
		l2b(&mhod.strsize);

		utf16len= mhod.strsize-sizeof(MHOD);
		utf16= (char*) malloc(utf16len);
		utf8= (char*) malloc(2*utf16len);

		if ((res = fread(utf16,1, utf16len, fd)) != utf16len)
			tell("could not fread: '%s'",strerror(errno));

		utf8len= utf16to8(utf16,utf16len,utf8);

		switch (mhod.type) 
      {
			case TITLE: break;
			default:    break;	
		}
				
		free(utf8);
		free(utf16);
	}

	for (i= 0; i<songcount; i++)
		parsePlaylistEntry(playlist);
}	

//***************************************************************************
// Parse Playlists
//***************************************************************************

void ITunesParser::parsePlaylists()
{
	MHLP mhlp;
	unsigned int j;
	int res;

	if ((res = fread(&mhlp, sizeof(MHLP), 1, fd)) != 1)
		tell("could not fread: '%s'",strerror(errno));

	if (strncmp("mhlp", mhlp.desc, 4) != 0)
		tell("could not find MHLP header");

	l2b(&mhlp.hlen);	
	l2b(&mhlp.rlen);

	if (fseek(fd, mhlp.hlen-sizeof(MHLP), SEEK_CUR) != 0)
		tell("could not fseek: '%s'",strerror(errno));		

	for (j = 0; j < mhlp.rlen; j++)
		parsePlaylist();
}

//***************************************************************************
// Parse
//***************************************************************************

int ITunesParser::parse()
{
	MHBD mhbd;
//	MHSD mhsd;
   int res;

   if (!songs)
      return fail;

	if ((res = fread(&mhbd, sizeof(MHBD), 1, fd)) != 1)
		tell("Error Read failed due to '%s'", strerror(errno));

	if (strncmp("mhbd", mhbd.desc, 4) != 0)
		tell("Error: 'MHBD' record not found");

	l2b(&mhbd.hlen);	
	l2b(&mhbd.rlen);	

	if (fseek(fd, mhbd.hlen, SEEK_SET) != 0)
		tell("Error: Seek failed due to '%s'",strerror(errno));

	parseTracks();

// 	if (fread(&mhsd, sizeof(MHSD), 1, fd) != 1)
// 		tell("could not fread: '%s'",strerror(errno));		

// 	if (strncmp("mhsd", mhsd.desc, 4) != 0)
// 		tell("could not find MHSD header");

// 	l2b(&mhsd.hlen);

// 	if (fseek(fd, mhsd.hlen-sizeof(MHSD), SEEK_CUR) != 0)
// 		tell("could not fseek: '%s'",strerror(errno));
		
// 	tell("reading playlists...");
// 	parsePlaylists();

   return success;
}

//***************************************************************************
// Init
//***************************************************************************

int ITunesParser::init(char* path, List* list)
{
   tell("Open file '%s'", path);

	fd = fopen(path, "r");

	if (!fd)
   {
		tell("Error: Could not open '%s' due to '%s'", path, strerror(errno));	
      return fail;
   }

   songs = list;
	
	return success;
}

//***************************************************************************
// Exit
//***************************************************************************

int ITunesParser::exit()
{
	fclose(fd);

   return done;
}
