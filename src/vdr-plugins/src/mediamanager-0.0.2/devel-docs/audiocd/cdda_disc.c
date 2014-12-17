#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdda_disc.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "debug.h"

cMediaCddaTrack::cMediaCddaTrack(int Track, const char *Title, const char *Performer)
{
	char buf[128];
	track = Track;
	if(Title) {
		title = strdup(Title);
	} else {
		snprintf(buf, sizeof(buf), "Track %3d", Track);
		title = strdup(buf);
	}
	performer = Performer ? strdup(Performer) : strdup(tr("Unknown"));
}

cMediaCddaTrack::~cMediaCddaTrack()
{
DBG_D("cMediaCddaTrack::~cMediaCddaTrack()")
	if(title)
		free(title);
	if(performer)
		free(performer);
}

uint32_t cMediaCddaTrack::TrackSize(void)
{
	return TrackSectors() * CDIO_CD_FRAMESIZE_RAW;
}

cMediaCddaDisc::cMediaCddaDisc(const char *DeviceFile)
{
	cdio = NULL;
	device_file = strdup(DeviceFile);
	last_replayed = -1;
}

cMediaCddaDisc::~cMediaCddaDisc()
{
DBG_D("cMediaCddaDisc::~cMediaCddaDisc()")
	if(title)
		free(title);
	if(performer)
		free(performer);
	if(device_file)
		free(device_file);
	Close();
}

void cMediaCddaDisc::Close(void)
{
	cdio_destroy(cdio);
	cdio = NULL;
}

/*
CDTEXT_ARRANGER   =  0, < name(s) of the arranger(s)
CDTEXT_COMPOSER   =  1, < name(s) of the composer(s)
CDTEXT_DISCID     =  2, < disc identification information
CDTEXT_GENRE      =  3, < genre identification and genre information 
CDTEXT_MESSAGE    =  4, < ISRC code of each track 
CDTEXT_ISRC       =  5, < message(s) from the content provider or artist 
CDTEXT_PERFORMER  =  6, < name(s) of the performer(s) 
CDTEXT_SIZE_INFO  =  7, < size information of the block 
CDTEXT_SONGWRITER =  8, < name(s) of the songwriter(s) 
CDTEXT_TITLE      =  9, < title of album name or track titles 
CDTEXT_TOC_INFO   = 10, < table of contents information 
CDTEXT_TOC_INFO2  = 11, < second table of contents information 
CDTEXT_UPC_EAN    = 12,
CDTEXT_INVALID    = MAX_CDTEXT_FIELDS
*/

eCddaStatus cMediaCddaDisc::Open(void)
{
	const cdtext_t *cdtext;
	int i;
	cMediaCddaTrack *t;
	track_t last_track;
	track_format_t format;

	cdio = cdio_open(device_file, DRIVER_DEVICE);
	if(!cdio)
		return scOpenFailed;

	first_track = cdio_get_first_track_num(cdio);
	tracks = cdio_get_num_tracks(cdio);
	last_track = first_track + tracks;
	
	cdtext = cdio_get_cdtext(cdio, 0);
	if(cdtext) {
		title = cdtext->field[CDTEXT_TITLE] ? strdup(cdtext->field[CDTEXT_TITLE]) : strdup(tr("Unknown"));
		performer = cdtext->field[CDTEXT_PERFORMER] ? strdup(cdtext->field[CDTEXT_PERFORMER]) : strdup(tr("Unknown"));
	} else {
		title = strdup(tr("Unknown"));
		performer = strdup(tr("Unknown"));
	}
	
	for(i = first_track; i < last_track; i++) {
		format = cdio_get_track_format(cdio, i);
		cdtext = cdio_get_cdtext(cdio, i);
		if(cdtext && (format == TRACK_FORMAT_AUDIO)) {
			t = new cMediaCddaTrack(i,
				cdtext->field[CDTEXT_TITLE], cdtext->field[CDTEXT_PERFORMER]);
			t->SetStartLsn(cdio_get_track_lsn(cdio, i));
			t->SetEndLsn(cdio_get_track_last_lsn(cdio, i));
			Add(t);
		}
	}
	
	return scSuccess;
}

bool cMediaCddaDisc::ReadAudioSector(uint8_t *Buffer, lsn_t Start, int& Count)
{
	if(cdio_read_audio_sectors(cdio, Buffer, Start, Count) == DRIVER_OP_SUCCESS)
		return true;
	return false;
}

uint32_t cMediaCddaDisc::TrackTime(uint32_t TrackSize)
{
	return TrackSize / 176400;
}

char* cMediaCddaDisc::TrackTimeHMS(uint32_t TrackSize, bool AddSpace)
{
	char *tmp;
	int s = TrackTime(TrackSize);
	int m = s / 60 % 60;
	int h = s / 3600;
	s %= 60;

	if(h)
		asprintf(&tmp, "%02d:%02d:%02d", h, m, s);
	else {
		if(AddSpace)
			asprintf(&tmp, "    %02d:%02d", m, s);
		else
			asprintf(&tmp, "%02d:%02d", m, s);
	}

	if(tmp)
		return tmp;
	
	return strdup("n/a");
}

const char *cMediaCddaDisc::TitleFromTrack(int Track)
{
	for(cMediaCddaTrack *obj = First(); obj; obj = Next(obj)) {
		if(obj->Track() == Track)
			return obj->Title();
	}
	return NULL;
}

const char *cMediaCddaDisc::PerformerFromTrack(int Track)
{
	for(cMediaCddaTrack *obj = First(); obj; obj = Next(obj)) {
		if(obj->Track() == Track)
			return obj->Performer();
	}
	return NULL;
}
