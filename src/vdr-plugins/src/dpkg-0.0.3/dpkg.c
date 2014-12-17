/*
 * dpkg.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/tools.h>
#include <vdr/interface.h>

#include "mainmenu.h"

static const char *DESCRIPTION     = trNOOP("Dpkg Plugin");
static const char *MAINMENUENTRY   = trNOOP("Start Online Update");
//static const char *MENUSETUPENTRY  = trNOOP("Dpkg Setup");

static int gCallFromSvdrp = 0;

class cPluginDpkg:public cPlugin {
public:
	cPluginDpkg(void)                                                                     { isyslog("cPluginDpkg::cPluginDpkg\n");}
	virtual ~ cPluginDpkg()                                                               {}
	virtual const char *Version(void)                                                     { return VERSION;}
	virtual const char *Description(void)                                                 { return DESCRIPTION;}
	virtual const char *CommandLineHelp(void)                                             { return NULL;}
	virtual bool ProcessArgs(int argc, char *argv[])                                      { return true;}
	virtual bool Initialize(void)                                                         { /*RegisterI18n(tlPhrases);*/ return true;}
	virtual bool Start(void)                                                              { return true;}
	virtual void Stop(void)                                                               {}
	virtual void Housekeeping(void)                                                       {}
	virtual void MainThreadHook(void)                                                     {}
	virtual cString Active(void)                                                          { return NULL;}
	virtual const char *MainMenuEntry(void)                                               { return tr(MAINMENUENTRY);}
	virtual cOsdObject *MainMenuAction(void)                                              {
		cOsdObject *ret = gCallFromSvdrp ? new cMainMenu(DPKG_MODE_UPGRADE|DPKG_MODE_AUTO_INSTALL) :
                                                   new cMainMenu(DPKG_MODE_UPGRADE|DPKG_MODE_REEL_PKG|DPKG_MODE_PKG_JUMP|DPKG_MODE_DESCR_LINE|DPKG_MODE_SOURCES);
		gCallFromSvdrp = 0;
		return ret;
	}; // MainMenuAction
	//virtual const char *MenuSetupPluginEntry(void)                                        { return tr(MENUSETUPENTRY);}
	virtual cMenuSetupPage *SetupMenu(void)                                               { return NULL;}
	virtual bool SetupParse(const char *Name, const char *Value)                          { return false;}
	virtual bool HasSetupOptions(void)                                                    { return false; }
	virtual bool Service(const char *Id, void *Data = NULL)                               { 
		if(Data && !strcmp(Id, DPKG_DLG_V1_0)) {
			dpkg_dlg_v1_0 *dlgData = (dpkg_dlg_v1_0 *)Data;
			dlgData->pMenu = new cMainMenu(dlgData->iMode, dlgData->title);
			return true;
		} // if
		return false; 
	} // Service
	virtual const char **SVDRPHelpPages(void)                                             {
		static const char *HelpPages[] =
		{
			"ASKUP\n"
			"          Asks to install updates.",
			NULL
		};
		return HelpPages;
	}; // SVDRPHelpPages
	virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
		if(!strcmp(Command, "ASKUP")) {
		if(Interface->Confirm(tr("Updates available! Press OK to update now."))) {
			gCallFromSvdrp = 1;
			cRemote::CallPlugin("dpkg");
			return cString("Update processed.");
		} // if
		return cString("Update canceled.");
		} // if
		return NULL;
	}; // SVDRPCommand
}; // cPluginDpkg
VDRPLUGINCREATOR(cPluginDpkg);  // Don't touch this!
