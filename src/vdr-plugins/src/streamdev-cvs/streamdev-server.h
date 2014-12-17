/*
 *  $Id: streamdev-server.h,v 1.4 2007/02/19 12:08:16 schmirl Exp $
 */
 
#ifndef VDR_STREAMDEVSERVER_H
#define VDR_STREAMDEVSERVER_H

#include "common.h"

#include <vdr/plugin.h>

#undef tr
#define tr(s) I18nTranslate(s, "vdr-streamdev-server")

class cPluginStreamdevServer : public cPlugin {
private:
	static const char *DESCRIPTION;
	static const char *MAINMENUENTRY;
	static const char *SETUPMENUENTRY;
public:
	cPluginStreamdevServer(void);
	virtual ~cPluginStreamdevServer();

	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void);
	virtual const char *CommandLineHelp(void);
	virtual bool ProcessArgs(int argc, char *argv[]);
	virtual bool Start(void);
	virtual void Stop(void);
	virtual cString Active(void);
        virtual const char *MenuSetupPluginEntry(void);
	virtual const char *MainMenuEntry(void);
	virtual cOsdObject *MainMenuAction(void);
	virtual cMenuSetupPage *SetupMenu(void);
	virtual bool SetupParse(const char *Name, const char *Value);
	virtual bool Service(const char *Id, void *Data);
#if REELVDR
        const char** SVDRPHelpPages();
        cString SVDRPCommand(const char*, const char*, int&);
#endif /* REELVDR */
};

#endif // VDR_STREAMDEVSERVER_H
