/*
 * VCD plugin for VDR setup
 * setup.h: Setup the plugin
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __VCD_SETUP_H
#define __VCD_SETUP_H

#include <vdr/menuitems.h>
#include "functions.h"

class cVcdSetupData {
public:
  int DriveSpeed;
  int BrokenMode;
  int HideMainMenuEntry;
  int PlayTracksContinuously;
  int AutostartReplay;
  int PlaySequenceReplay;
public:
  cVcdSetupData(void);
  };

class cVcdSetupMenu : public cMenuSetupPage {
private:
  cVcd *vcd;
  cVcdSetupData setupData;
protected:
  virtual void Store(void);
public:
  cVcdSetupMenu(cVcd *Vcd);
  };

extern cVcdSetupData VcdSetupData;

#endif //__VCD_SETUP_H
