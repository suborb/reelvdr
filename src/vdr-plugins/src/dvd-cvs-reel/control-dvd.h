/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __CONTROL_DVD_H
#define __CONTROL_DVD_H

#include <vdr/status.h>
#include <vdr/player.h>
#include <vdr/thread.h>

class cDvdPlayerControl : public cControl {
  friend class cDvdPlayer;
private:
  static bool dvd_active;
  bool visible, modeOnly, shown, displayFrames, forceDvdNavigation;
  void *osdTaker;
  int lastCurrent, lastTotal;
  bool lastPlay, lastForward;
  int lastSpeed;
  cSkinDisplayReplay* displayReplay;

  enum InputProcessType { NoneInput, TimeSearchInput, TrackSearchInput };

  InputProcessType inputActive;
  bool inputHide;
  time_t timeoutShow;

  int timeSearchTime, timeSearchPos;

  int inputIntVal;
  const char * inputIntMsg;

  void TimeSearchDisplay(void);
  void TimeSearchProcess(eKeys Key);
  void TimeSearch(void);

  void InputIntDisplay(const char * msg, int val);
  void InputIntProcess(eKeys Key, const char * msg, int & val);

  void TrackSearch(void);
  void OsdOpen(void);
  void OsdClose();
  void ShowTimed(int Seconds = 0);
  void updateShow(bool force=false);

  void ShowMode(void);
  bool ShowProgress(bool Initial);

  bool IsDvdNavigationForced() { return forceDvdNavigation; }

public:
  cDvdPlayerControl(void);
  virtual ~cDvdPlayerControl();
  bool DvdNavigation(eKeys Key);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);
  virtual void Hide(void);
  bool OsdVisible(void *me);
  bool OsdTaken(void *me);
  bool TakeOsd(void *obj);
  static bool DVDActive() { return dvd_active; };
  const char * GetDisplayHeaderLine();
  virtual bool NoAutoClose(void) { return true; }

protected:
  void HideOwnOsd(void);

private:
  cDvdPlayer *player;
public:
  bool Active(void);
  bool Start(const char *FileName);
       // Starts replaying the given file.
  void Stop(void);
       // Stops the current replay session (if any).
  void Pause(void);
       // Pauses the current replay session, or resumes a paused session.
  void Play(void);
       // Resumes normal replay mode.
  void Forward(void);
       // Runs the current replay session forward at a higher speed.
  void Backward(void);
       // Runs the current replay session backwards at a higher speed.
  int  SkipFrames(int Frames);
       // Returns the new index into the current replay session after skipping
       // the given number of frames (no actual repositioning is done!).
       // The sign of 'Frames' determines the direction in which to skip.
  void SkipSeconds(int Seconds);
       // Skips the given number of seconds in the current replay session.
       // The sign of 'Seconds' determines the direction in which to skip.
       // Use a very large negative value to go all the way back to the
       // beginning of the recording.
  bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
       // Returns the current and total frame index, optionally snapped to the
       // nearest I-frame.
  bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
       // Returns the current replay mode (if applicable).
       // 'Play' tells whether we are playing or pausing, 'Forward' tells whether
       // we are going forward or backward and 'Speed' is -1 if this is normal
       // play/pause mode, 0 if it is single speed fast/slow forward/back mode
       // and >0 if this is multi speed mode.
  void Goto(int Index, bool Still = false);
       // Positions to the given index and displays that frame as a still picture
       // if Still is true.
  };

#endif // __CONTROL_DVD_H

