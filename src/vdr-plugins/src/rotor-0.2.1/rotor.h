#ifndef ROTOR_H
#define ROTOR_H

#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <vdr/diseqc.h>
#include <vdr/plugin.h>
#include <vdr/dvbdevice.h>
#include "i18n.h"
#include "status.h"

static const char *VERSION        = "0.2.1";
static const char *DESCRIPTION    = "Steuerung eines Rotors";
static const char *MAINMENUENTRY  = "Rotor";
static const char *SETUPMENUENTRY = "Rotor";

// --- cPluginRotor ---------------------------------------------------------

class cPluginRotor : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cStatusMonitor *statusMonitor;
public:
  cPluginRotor(void);
  virtual ~cPluginRotor();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
#if VDRVERSNUM >= 10330
  virtual bool Service(const char *Id, void *Data);
#endif
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual bool ProvidesSource(int Source, int Tuner);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual const char *MenuSetupPluginEntry(void) { return tr(SETUPMENUENTRY); }
  virtual bool SetupParse(const char *Name, const char *Value);
#if 0 /* VDRVERSNUM >= 10331*/
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
#endif
  virtual bool HasSetupOptions(void) { return false; }
};

#endif
