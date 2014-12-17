/*
 * cdda_disc.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_CDDA_DISC_H
#define _MEDIA_CDDA_DISC_H

#include <vdr/i18n.h>

#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <vdr/tools.h>

typedef enum {
  scLocked = -6,
  scCannotEjectMedia = -5,
  scOutOfMemory = -4,
  scOpenFailed = -3,
  scFail = -2,
  scEOF = -1,
  scSuccess = 0,
} eCddaStatus;

class cMediaCddaTrack : public cListObject {
  private:
	track_t track;
	lsn_t track_start, track_end;
	char *title;
	char *performer;
  public:
	cMediaCddaTrack(int Track, const char *Title, const char *Performer);
	~cMediaCddaTrack();

	uint32_t TrackSectors(void) { return track_end - track_start; }
	uint32_t TrackSize(void);
	
	void SetStartLsn(lsn_t Start) { track_start = Start; }
	void SetEndLsn(lsn_t End) { track_end = End; }
	
	track_t Track(void) { return track; }
	const char *Title(void) { return title; }
	const char *Performer(void) { return performer; }
	lsn_t StartLsn(void) { return track_start; }
	lsn_t EndLsn(void) { return track_end; }
};

class cMediaCddaDisc : public cList<cMediaCddaTrack> {
  private:
	CdIo_t *cdio;
	track_t first_track;
	track_t tracks;
	
	const char *device_file;
	char *title;
	char *performer;
	int last_replayed;

	uint32_t TrackTime(uint32_t TrackSize);
	void Close(void);
  public:
	cMediaCddaDisc();
	~cMediaCddaDisc();

	const char *Title(void) { return title; }
	const char *Performer(void) { return performer; }
	const char *TitleFromTrack(int Track);
	const char *PerformerFromTrack(int Track);
	void SetLastReplayed(int Last) { last_replayed = Last; }
	int LastReplayed(void) { return last_replayed; }

	eCddaStatus Open(void);
	bool ReadAudioSector(uint8_t *Buffer, lsn_t Start, int& Count);
	char *TrackTimeHMS(uint32_t TrackSize, bool AddSpace = false);

	//bool isOpen(void) { return cdio != NULL; }
};

#endif
