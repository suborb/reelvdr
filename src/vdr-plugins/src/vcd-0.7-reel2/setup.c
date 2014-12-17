/*
 * VCD plugin for VDR setup
 * setup.c: Setup the plugin
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#include <vdr/menuitems.h>
#include "setup.h"
#include "i18n.h"

cVcdSetupData VcdSetupData;

// --- cVcdSetupData ---------------------------------------------------------

cVcdSetupData::cVcdSetupData(void)
{
  DriveSpeed = 2;
  BrokenMode = 1;
  HideMainMenuEntry = 0;
  PlayTracksContinuously = 1;
  AutostartReplay = 0;
  PlaySequenceReplay = 0;
}


// --- cVcdSetupMenu ---------------------------------------------------------

cVcdSetupMenu::cVcdSetupMenu(cVcd *Vcd)
{
  vcd = Vcd;
  setupData = VcdSetupData;
  SetSection(tr("VCD"));
  Add(new cMenuEditIntItem(tr("Setup.VCD$Drive speed"), &setupData.DriveSpeed, 1, 50));
  //Add(new cMenuEditBoolItem(tr("Setup.VCD$Broken mode"), &setupData.BrokenMode));
//  Add(new cMenuEditBoolItem(tr("Setup.VCD$Hide main menu entry"), &setupData.HideMainMenuEntry));
  Add(new cMenuEditBoolItem(tr("Setup.VCD$Play tracks continuously"), &setupData.PlayTracksContinuously));
  Add(new cMenuEditBoolItem(tr("Setup.VCD$Autostart replay"), &setupData.AutostartReplay));
  Add(new cMenuEditBoolItem(tr("Setup.VCD$Play sequence replay"), &setupData.PlaySequenceReplay));
}

void cVcdSetupMenu::Store(void)
{
  VcdSetupData = setupData;
  SetupStore("DriveSpeed", VcdSetupData.DriveSpeed);
  vcd->SetDriveSpeed(VcdSetupData.DriveSpeed);
  SetupStore("BrokenMode", VcdSetupData.BrokenMode);
  SetupStore("HideMainMenuEntry", VcdSetupData.HideMainMenuEntry);
  SetupStore("PlayTracksContinuously", VcdSetupData.PlayTracksContinuously);
  SetupStore("AutostartReplay", VcdSetupData.AutostartReplay);
  SetupStore("PlaySequenceReplay", VcdSetupData.PlaySequenceReplay);
}
