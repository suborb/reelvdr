/*
 * VCD Player plugin for VDR
 * menucontrol.h: Menu control of VCD replay
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#include "viewer.h"
#include "player.h"


// --- cMenuSpiControl -------------------------------------------------------

class cMenuSpiControl : public cVcdViewerControl {
private:
  cSkinDisplayReplay *displayReplay;
  bool visible, modeOnly;
  static int spi;
  static cVcd *vcd;
public:
  cMenuSpiControl(void);
  virtual ~cMenuSpiControl();
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Hide(void);
  static void SetItem(int Spi, cVcd *Vcd);
  };


// --- cMenuVcdControl -------------------------------------------------------

class cMenuVcdControl : public cVcdPlayerControl {
private:
  cSkinDisplayReplay *displayReplay;
  bool visible, modeOnly, shown, displayFrames;
  time_t timeoutShow;
  bool timeSearchActive, timeSearchHide;
  int timeSearchTime, timeSearchPos;
  int lastCurrent, lastTotal;
  bool lastPlay, lastForward;
  int lastSpeed;
  static int track;
  static cVcd *vcd;
  static char *title;
  void ShowMode(void);
  bool ShowProgress(bool Initial);
  void TimeSearchDisplay(void);
  void TimeSearchProcess(eKeys Key);
  void TimeSearch(void);
  void ShowTimed(int Seconds = 0);
public:
  cMenuVcdControl(void);
  virtual ~cMenuVcdControl();
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);
  virtual void Hide(void);
  bool Visible(void) { return visible; }
  static void SetTrack(int Track, cVcd *Vcd);
  static const int LastReplayed(void);
  };

