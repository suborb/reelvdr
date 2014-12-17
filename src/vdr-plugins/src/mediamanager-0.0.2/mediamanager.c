/*
 * Mediamanager.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <getopt.h>
#include <vdr/plugin.h>

#include "i18n.h"
#include "mediaconfig.h"
#include "mediahandler.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "mediadebug.h"

static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = "Media Manager";
static const char *MAINMENUENTRY  = "Media Manager";


class cMediaSetup : public cMenuSetupPage {
  private:
	int autoplay;
  protected:
	void Store(void);
  public:
	cMediaSetup(void);
};

cMediaSetup::cMediaSetup()
{
	autoplay = cMediaConfig::GetConfig().AutoPlay();
	Add(new cMenuEditBoolItem(tr("Autoplay media"), &autoplay, tr("no"), tr("yes")));
}

void cMediaSetup::Store(void)
{

	SetupStore("autoplay", autoplay);
	cMediaConfig::GetConfig().SetAutoPlay(autoplay);
}

class cPluginMediamanager : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cMediaHandler *mHandler;
  bool have_device;
public:
  cPluginMediamanager(void);
  virtual ~cPluginMediamanager();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void);
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginMediamanager::cPluginMediamanager(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  mHandler = NULL;
  have_device = false;
}

cPluginMediamanager::~cPluginMediamanager()
{
  // Clean up after yourself!
  delete(mHandler);
  delete(&cMediaConfig::GetConfig());
}

const char *cPluginMediamanager::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  static char *help;

  if(help == NULL)
	asprintf(&help, "  -d DEVICE (as listed in fstab default: auto detect)\n");
	
  return help;
}

bool cPluginMediamanager::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  int c;
  
  while ((c = getopt (argc,argv,"d:")) != -1) {
  	switch(c) {
		case 'd':
			cMediaConfig::GetConfig().SetDeviceFile(optarg);
			break;
		default:
			return false;
	}
  }
  return true;
}

bool cPluginMediamanager::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
	RegisterI18n(Phrases);
	cMediaConfig& conf = cMediaConfig::GetConfig();
	if(conf.GetDeviceFile() == NULL)
		conf.SetDeviceFile("/dev/cdrom");
	if(conf.GetDeviceFile() == NULL)
		conf.SetDeviceFile("/dev/cdrom0");
	if(conf.GetDeviceFile() == NULL)
		conf.SetDeviceFile("/dev/dvd");

	conf.SetArchivDir("/tmp/vdrarchive");
	if(conf.GetDeviceFile() == NULL)
		have_device = false;
	else
		have_device = true;

	mHandler = new cMediaHandler();

	return true;
}

bool cPluginMediamanager::Start(void)
{
	// Start any background activities the plugin shall perform.
	if(!have_device) {
		Skins.QueueMessage(mtError, tr("Media Manager: Cannot detect DEVICE. Please use commandline"), 10);
		mHandler->SetSuspendPolling(true);
	}
	mHandler->Start();

	return true;
}

void cPluginMediamanager::Stop(void)
{
  // Stop any background activities the plugin shall perform.
	mHandler->Stop();
}

void cPluginMediamanager::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

const char *cPluginMediamanager::MainMenuEntry(void)
{
  const char *m = mHandler->MainMenuEntry();
  if(m)
  	return m;
  return tr(MAINMENUENTRY);
}

cOsdObject *cPluginMediamanager::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return mHandler->MainMenuAction();
}

cMenuSetupPage *cPluginMediamanager::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL; //new cMediaSetup();
}

bool cPluginMediamanager::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
/*
  if(!strcasecmp(Name, "autoplay"))
  	cMediaConfig::GetConfig().SetAutoPlay(atoi(Value));
  else
  	return false;

  return true;
*/
	return false;
}

bool cPluginMediamanager::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if(!have_device)
  	return false;
  return mHandler->Service(Id, Data);
}

const char **cPluginMediamanager::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginMediamanager::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginMediamanager); // Don't touch this!
