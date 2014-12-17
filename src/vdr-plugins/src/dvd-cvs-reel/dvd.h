/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#include <vdr/plugin.h>
#include <vdr/i18n.h>
#ifndef __QNXNTO__
#include <getopt.h>
#else
#include <unistd.h>
#endif
#include "dvddev.h"
#include "tools-dvd.h"
#include "player-dvd.h"
#include "control-dvd.h"
#include "setup-dvd.h"
#include "i18n.h"

static const char *VERSION        = "0.3.6-b03";
static const char *DESCRIPTION    = "Plugin.DVD$turn VDR into an (almost) full featured DVD player";
static const char *MAINMENUENTRY  = "Plugin.DVD$DVD";

// --- cPluginDvd ------------------------------------------------------------

class cPluginDvd : public cPlugin {
  friend class cDvdPlayer;
  friend class cDvdPlayerControl;
  static cSetupLine *GetSetupLine(const char *Name, const char *Plugin);
private:
  // Add any member variables or functions you may need here.
public:
  cPluginDvd(void);
  virtual ~cPluginDvd();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return DVDSetup.HideMainMenu ? NULL : tr(MAINMENUENTRY); }
  virtual cOsdMenu *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
};
