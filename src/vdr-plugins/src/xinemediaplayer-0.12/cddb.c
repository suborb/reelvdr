/*
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#include "cddb.h"
#include <string.h>
#include "./cdtext.h" 

#define FREE_NOT_NULL(p) if (p) { free(p); p = NULL; }
#define STR_OR_NULL(s) ((s) ? s : "")




cTrack::cTrack(const int n, const string t, const string artist, const int len, const string eData): num_(n), title_(t),artist_(artist), length_(len), extData_(eData)
{}

int     cTrack::Number()     const {  return num_;       }
int     cTrack::Length()     const {  return length_;    }
string  cTrack::Title()      const {  return title_;     }
string  cTrack::ExtData()    const {  return extData_;   }
string  cTrack::Artist()    const {  return artist_;   }

void    cTrack::SetNumber(int num)      { num_      = num;      }
void    cTrack::SetLength(int len)      { length_   = len;      }

void    cTrack::SetTitle(string title)      { title_    = title;    }
void    cTrack::SetArtist(string artist)      { artist_ = artist;    }
void    cTrack::SetExtData(string eData)    { extData_  = eData;    }






void    cCdData::Clear()
{
    tracks.clear();
    
    SetTrackCount(0);
    SetYear(0);
    SetLength(0);
    
    SetTitle("");
    SetArtist("");
    SetGenre("");
    SetExtData("");
}

int     cCdData::TrackCount() const  { return trackCount_;   }
int     cCdData::Year() const        { return year_;         }
int     cCdData::Length() const      { return length_;       }
string  cCdData::Genre() const       { return genre_;        }
string  cCdData::Title() const       { return title_;        }
string  cCdData::Artist() const      { return artist_;       }
string  cCdData::ExtData() const     { return extData_;      }

void    cCdData::SetTrackCount(int count)  { trackCount_   = count;    }
void    cCdData::SetYear(int year)         { year_         = year;     }
void    cCdData::SetTitle(string title)    { title_        = title;    }
void    cCdData::SetArtist(string artist)  { artist_       = artist;   }
void    cCdData::SetGenre(string genre)    { genre_        = genre;    }
void    cCdData::SetExtData(string eData)  { extData_      = eData;    }
void    cCdData::SetLength(int len)        { length_       = len;      }

void cCdData::AddTrack(const int trackNum, const string trackTitle,const string trackArtist, const int trackLen, const string trackExtData)
{
    cTrack track( trackNum, trackTitle, trackArtist, trackLen, trackExtData);
    tracks.push_back( track );
}
cCdData::~cCdData()
{
    Clear();
}



/// \param char* device, eg "/dev/CDROM"
/// Reads CDROM device 
/// gets CD length, track count and track offsets from physical CD
/// calls CreateCD() with these parameters
void cCdInfo::ReadAudioCd(const char*dev)
{
  char *device=NULL;
  
  CdIo_t *cdio;               /* libcdio CD access structure */
  track_t count, t;           /* track counters */
  lba_t lba;                  /* Logical Block Address */
  int *foffset = NULL;        /* list of frame offsets */
  
  if (dev)
    device = strdup(dev);
  else
    // get default CDROM device
    device = cdio_get_default_device(NULL);
  
  if ( !device )
  {
    esyslog("Could not get CDROM device");
    return ;
  }
  
  printf("CD-ROM device: %s\n", device);
  
  
  // Load the appropriate driver and open the CD-ROM device for reading.
  cdio = cdio_open(device, DRIVER_UNKNOWN);
  if (!cdio) 
  {
    esyslog("Unable to open CDROM device %s", device);
    // no memory allocated for cdio so no cdio_destroy()

    return ;
  }
  
  // Get the track count for the CD. 
  count = cdio_get_num_tracks(cdio);
  if (count == 0) 
  {
    esyslog("no audio tracks on CD");
    cdio_destroy(cdio);

    return;
  }
  printf("CD contains %d track(s)\n", count);
  
  //frame offsets.
  foffset = new int[count];
  
  // get track data
  for (t = 1; t <= count; t++) 
  {

    //only audio CDs
    if (0 && cdio_get_track_format(cdio, t) != TRACK_FORMAT_AUDIO)  // skipping track format test
    {
      esyslog("track %d is not an audio track", t);
      goto cleanup;
    }

    //Get frame offset of next track
    lba = cdio_get_track_lba(cdio, t);
    if (lba == CDIO_INVALID_LBA) 
    {
      esyslog("track %d has invalid Logical Block Address", t);
      goto cleanup;
    }

    // Add this offset to the list.
    foffset[t - 1] = lba;
  }

  // calculate the length of CD
  lba = cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK);
  if (lba == CDIO_INVALID_LBA) 
  {
      esyslog("LEADOUT_TRACK has invalid Logical Block Address");
      goto cleanup;
  }
  
  // create CD structure
  CreateCd(FRAMES_TO_SECONDS(lba), count, foffset);

cleanup:  
  cdio_destroy(cdio);
  delete [] foffset;
}

/// \param CD length
/// \param track count
/// \param track frame offsets
/// creates a cddb_disc_t structure out of disc length, track count 
/// and track offsets
void cCdInfo::CreateCd(int dlength, int tcount, int *foffset)
{
    //destroy earlier disc structure, //TODO //XXX mem leak
    //Create a new disc structure
//TODO: why not simply destroy old disc?    if(disc) cddb_disc_destroy(disc);
    disc = cddb_disc_new();

    if (disc) 
    { 
        // Initialize disc length
        cddb_disc_set_length(disc, dlength);

        cddb_track_t *track = NULL; // libcddb track structure
        //add basic track data
        for (int i = 0; i < tcount; i++) 
        {
            // Create a new libcddb track structure
            track = cddb_track_new();

            if (!track) 
            { 
                esyslog("Memory allocation error at %i",i);
                cddb_disc_destroy(disc);
                disc=0;
                return ;
            } // end if

            // First add the track to the disc
            cddb_disc_add_track(disc, track);
            
            // Set frame offset in track
            cddb_track_set_frame_offset(track, foffset[i]);
            
        } // for
        
    }// if
    
} // CreateCd()

void cCdInfo::QueryCddbServer()
{
    // new connection
    cddb_conn_t* conn = cddb_new();
    
    // set connection parameter, else use default
        
        if ( !disc ) return;
    // calculate disc id
    cddb_disc_calc_discid(disc);
    
    // ask server for matches, gets also the categories
    int matches = cddb_query(conn, disc);
    int discid  = cddb_disc_get_discid(disc);
    
    if (matches == -1) 
    {
        esyslog("could not query cddb server");
        cddb_destroy(conn);

        return;
    } else if (matches == 0) 
    {
        esyslog("no matching discs found for discid %#x", discid);
        cddb_destroy(conn);

        return;
    }
    
    char *category = strdup(cddb_disc_get_category_str(disc));
    
    // create new disc with category and discid
    // free current disc
    cddb_disc_destroy(disc);
    disc = NULL;
    
    disc = cddb_disc_new();
    
    if (disc)
    {
        /*
         * This function converts a string into a category ID 
         * as used by libcddb.If the specified string does not 
         * match any of the known categories, 
         * then the category is set to 'misc'. 
         */
        cddb_disc_set_category_str(disc, category);

        // set disc id
        cddb_disc_set_discid(disc, discid);
    }
    else
    {
        esyslog("Memory allocation error");
        return;
    }
    
    // Get rest of the disc data from the server
    if ( !cddb_read(conn, disc) )
    {
        // error messages
        esyslog("Failed to get disc data from server");
        // more detailed error messages //XXX //TODO
        
        cddb_disc_destroy(disc);
        disc = NULL;
        
        return;
    } // if
    
    // here have disc structure read to be read from
}

/// gets CD information from disc structure and
/// loads it into accessible data structures
/// assumes disc is valid
void cCdInfo::ExtractCdInfo()
{
    cdData.Clear();

        if ( !disc ) return;    
    //CD Title
    cdData.SetTitle     ( STR_OR_NULL(cddb_disc_get_title(disc)) );
    
    //CD Artist
    cdData.SetArtist    ( STR_OR_NULL(cddb_disc_get_artist(disc)) );
    
    //Genre
    cdData.SetGenre     ( STR_OR_NULL(cddb_disc_get_genre(disc)) );
    //Ext. data
    cdData.SetExtData   ( STR_OR_NULL(cddb_disc_get_ext_data(disc)) );
    //Year
    cdData.SetYear      ( cddb_disc_get_year(disc) );
    
    //CD Length
    cdData.SetLength    (cddb_disc_get_length(disc));
    
    //Track Count
    cdData.SetTrackCount( cddb_disc_get_track_count(disc));
    
    
    //Set Track Data
    cddb_track_t *track = cddb_disc_get_track_first(disc);  // libcddb track structure
    for (;track != NULL; track = cddb_disc_get_track_next(disc)) 
    {

        // add track
        cdData.AddTrack(
            cddb_track_get_number(track), // track number
            STR_OR_NULL(cddb_track_get_title(track)), // track title
            STR_OR_NULL(cddb_track_get_artist(track)), // track artist
            cddb_track_get_length(track), // track length
            STR_OR_NULL(cddb_track_get_ext_data(track)) // track ext. data
        ); // AddTrack()
        
    } // for
}
int cCdInfo::TrackNumberByIndex(int index)
{
    if ( index < cdData.TrackCount() )
        return cdData.tracks.at(index).Number();
    else
        return -1;
}
string cCdInfo::TrackTitleByIndex(int index)
{
    if ( index < cdData.TrackCount() )
        return cdData.tracks.at(index).Title();
    else
        return "";
}

string cCdInfo::TrackArtistByIndex(int index)
{
    if ( index < cdData.TrackCount() )
        return cdData.tracks.at(index).Artist();
    else
        return "";
}

int cCdInfo::TrackCount()
{
    return cdData.TrackCount();
}

cCdInfo::cCdInfo()
{
    cdData.Clear();
    disc = NULL;
}
cCdInfo* cCdInfo::instance = NULL;
cCdInfo& cCdInfo::GetInstance()
{
    if (!instance) instance = new cCdInfo;
    
    return *instance;
}

int cCdInfo::ExtractCdText()
{
    cCdText *cd_text = new cCdText((char*)"/dev/cdrom");
    int totalTracks = -2;

    if ( cd_text &&  cd_text->read_cdtext() > 0 && (totalTracks = cd_text->get_disc_track_count()) >0 )
    {
        int i = 1;
    cdData.Clear();
        // assuming track number starts from 1 to totalTracks
        for (; i<= totalTracks; ++i)
        {
            cdData.AddTrack(i, cd_text->get_name(i), cd_text->get_performer(i), 0, ""); 
            printf(" \033[0;100m adding --> %i %s \033[0;m // %s \n", i, cd_text->get_name(i).c_str(), cd_text->get_performer(i).c_str() );
        }
    cdData.SetTrackCount( totalTracks );

    }

    printf("got track count %i\n", totalTracks);

    delete cd_text;
    return totalTracks;
}


int cCdInfo::AudioCdInit()
{
    if(disc) cddb_disc_destroy(disc);
        disc = 0;
/*
ioctl won´t return with DVD-Drive SN-T083 and CD Asylum (Disturbed)       
        if ( ExtractCdText()>0 )
        {
            printf("\033[0;92m CD Text Found\033[0m\n");
            return 1; // got CD text, donot try cddb
        }
        else
            printf("\033[0;91m CD Texti not Found \033[0m\n");
*/
    ReadAudioCd();
    QueryCddbServer();
    ExtractCdInfo();
        return 0;
}
cCdInfo::~cCdInfo()
{
    if(disc) cddb_disc_destroy(disc);
    disc = 0;
    
    cdData.Clear();
}
