/*
 * vdrarchiv_player.h:
 *
 * This is a copy of vdr's cReplayControl removing cRecording stuff which is
 * not compatible with vdrarchiv filehandling.
 * Cutting is disabled.
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_VDRARCHIV_PLAYER_H
#define _MEDIA_VDRARCHIV_PLAYER_H

#include <vdr/menu.h>
#include <vdr/dvbplayer.h>

#include "media_vdrarchiv/vdrarchiv_disc.h"

class cMediaVDRArchivControl : public cDvbPlayerControl {
  private:
  	cMediaVDRArchivDisc *disc;
	bool kback_key;
	cSkinDisplayReplay *displayReplay;
	static char *fileName;
	static char *title;
	cMarks marks;
	bool visible, modeOnly, shown, displayFrames;
	int lastCurrent, lastTotal;
	bool lastPlay, lastForward;
	int lastSpeed;
	time_t timeoutShow;
	bool timeSearchActive, timeSearchHide;
	int timeSearchTime, timeSearchPos;
	void TimeSearchDisplay(void);
	void TimeSearchProcess(eKeys Key);
	void TimeSearch(void);
	void ShowTimed(int Seconds = 0);
	void ShowMode(void);
	bool ShowProgress(bool Initial);
	void MarkToggle(void);
	void MarkJump(bool Forward);
	void MarkMove(bool Forward);
	/*
	void EditCut(void);
	void EditTest(void);
	*/
  public:
	cMediaVDRArchivControl(cMediaVDRArchivDisc *Disc);
	~cMediaVDRArchivControl();
  
	virtual cOsdObject *GetInfo(void);
	virtual eOSState ProcessKey(eKeys Key);
	virtual void Show(void);
	virtual void Hide(void);
	bool Visible(void) { return visible; }
	static void SetRecording(const char *FileName, const char *Title);
};

#endif
