/*
  netcvrotor - Rotor plugin control for NetCeiver
  (c) 2008 BayCom GmbH

  #include <gpl_v2.h>
*/

#ifndef NETCV_ROTOR_H
#define NETCV_ROTOR_H

#include <vdr/diseqc.h>
#include <vdr/plugin.h>
#include <vdr/dvbdevice.h>
#include <vdr/i18n.h>

static const char *VERSION        = "0.1.0";
static const char *DESCRIPTION    = trNOOP("Rotor interface for NetCeiver");
static const char *MAINMENUENTRY  = trNOOP("Rotor Setup");
static const char *SETUPMENUENTRY = trNOOP("Rotor Setup");

#define PATH_ROTOR_CONF "/etc/vdr/rotor.conf"
#define MAX_ROTORS 8

struct RotorData {
	int available;
	int minpos;
	int maxpos;
};

class cPluginNCRotor : public cPlugin {
private:

	struct RotorData rotor_data[MAX_ROTORS];

public:

	cPluginNCRotor(void);
	virtual ~cPluginNCRotor();
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
	virtual bool HasSetupOptions(void) { return false; }
	virtual int LoadConfig(void);
};

#endif
