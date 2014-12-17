/*
 * cdda_player.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_CDDA_PLAYER_H
#define _MEDIA_CDDA_PLAYER_H

#include "vdr/player.h"
#include "vdr/thread.h"

#include "media_cdda/cdda_disc.h"

class cMediaCddaPlayer : public cPlayer, cThread {
  private:
	cMediaCddaDisc *disc;
	cRingBufferFrame *ringBuffer;
	cFrame *rFrame;
	cFrame *pFrame;
	int first_track;
	int current_track;
	int last_track;
	int current_index;
	int total_index;
	int skip_track;
	enum ePlayMode { pmPlay=0, pmPause, pmStop, pmNone };
	ePlayMode currentMode;
	void GenerateLPCMHeader(uint8_t *Buffer, int Size, int channels, int freq);
  protected:
	virtual void Activate(bool On);
	virtual void Action(void);
  public:
	cMediaCddaPlayer(cMediaCddaDisc *Disc, int Track);
	virtual ~cMediaCddaPlayer();
	
	int CurrentTrack(void) { return current_track; }
	bool Active(void);
	void Stop(void);
	void Pause(void);
	void Play(void);
	void PreviousTrack(void);
	void NextTrack(void);
	virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
	virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
};

class cMediaCddaControl : public cControl {
  private:
	cMediaCddaPlayer *player;
	cMediaCddaDisc *disc;
	bool kback_key;
	cSkinDisplayReplay *replayMenu;
	bool visible;
	char *replayTitle;
	int first_track;
	int current_track;
	int last_track;
  public:
	cMediaCddaControl(cMediaCddaDisc *Disc, int Track);
	virtual ~cMediaCddaControl();

	void ShowReplayMenu(bool NewTrack);
	void UpdateReplayMenu(void);
	

	bool Active(void);
	void Stop(void);
		// Stops the current replay session (if any).
	void Pause(void);
		// Pauses the current replay session, or resumes a paused session.
	void Play(void);
		// Resumes normal replay mode.
	void PreviousTrack(void);
	void NextTrack(void);
	bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
		// Returns the current and total frame index, optionally snapped to the
		// nearest I-frame.
	bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
		// Returns the current replay mode (if applicable).
		// 'Play' tells whether we are playing or pausing, 'Forward' tells whether
		// we are going forward or backward and 'Speed' is -1 if this is normal
		// play/pause mode, 0 if it is single speed fast/slow forward/back mode
		// and >0 if this is multi speed mode.

	virtual void Hide(void);
	virtual eOSState ProcessKey(eKeys Key);
};

#endif
