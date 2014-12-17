#ifndef __ARGHDIRECTOR_H
#define __ARGHDIRECTOR_H

#include <vdr/menuitems.h>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/remote.h>

static const char *VERSION        = "0.2.8";
static const char *DESCRIPTION    = trNOOP("plugin to use the multifeed option");
static const char *MAINMENUENTRY  = trNOOP("Multifeed Options");
static const char *MENUSETUPENTRY = trNOOP("Multifeed Options ");

const char* allowedButtonNames[] = {
 "Ok",
 "Red",
 "Green",
 "Yellow",
 "Blue",
 "User1",
 "User2",
 "User3",
 "User4",
 "User5",
 "User6",
 "User7",
 "User8",
 "User9",
};

eKeys allowedButtons[] =
{
 kOk,
 kRed,
 kGreen,
 kYellow,
 kBlue,
 kUser1,
 kUser2,
 kUser3,
 kUser4,
 kUser5,
 kUser6,
 kUser7,
 kUser8,
 kUser9
};

#define NOOFALLOWEDBUTTONS 14

int hide = 0;
int portalmode = 1;
int swapKeys = 1;
int swapKeysInMenu = 0;
int displayChannelInfo = 0;
int switchChannelOnItemSelect = 0;
int hidePluginAfterChannelSwitch = 1;
int displayShortcutNumbers = 1;
int displayEpgInfo = 1;
int showOldDirectorOsd = 0;
int showPluginButton = 0;
int autostart = 0;
int themelike = 1;

class cDirectorStatus : public cStatus
{
private:
  cPlugin* parent;
protected:
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
public:
  cDirectorStatus(cPlugin* plugin);
};

class cMenuEditStraItemTr : public cMenuEditIntItem {
private:
  const char * const *strings;
protected:
  virtual void Set(void);
public:
  cMenuEditStraItemTr(const char *Name, int *Value, int NumStrings, const char * const *Strings);
};

class cPluginDirector : public cPlugin {
private:	
  // Add any member variables or functions you may need here.
  cDirectorStatus *directorStatus;  
public:
  cPluginDirector(void);
  virtual ~cPluginDirector();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return (hide ? NULL : tr(MAINMENUENTRY)); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual const char *MenuSetupPluginEntry(void) { return tr(MENUSETUPENTRY);}
  virtual bool SetupParse(const char *Name, const char *Value);
  };


#endif //__ARGHDIRECTOR_H
