/*
 * dpkgopt.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/tools.h>

#include "mainmenu.h"

static const char *DESCRIPTION    = trNOOP("Addon Plugin");
static const char *MAINMENUENTRY  = trNOOP("Optional Software ");
//static const char *MENUSETUPENTRY = trNOOP("Optional Software Setup");

class cPluginDpkgOpt:public cPlugin {
    public:
        cPluginDpkgOpt(void)                                                                  { isyslog("cPluginDpkg::cPluginDpkg\n");}
        virtual ~ cPluginDpkgOpt()                                                            {}
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
            return new cMainMenu(DPKG_MODE_OPT_PKG|DPKG_MODE_PKG_JUMP|DPKG_MODE_DESCR_LINE|DPKG_MODE_SOURCES, tr(MAINMENUENTRY));
        };
        //virtual const char *MenuSetupPluginEntry(void)                                        { return tr(MENUSETUPENTRY);}
        virtual cMenuSetupPage *SetupMenu(void)                                               { return NULL;}
        virtual bool SetupParse(const char *Name, const char *Value)                          { return false;}
        virtual bool HasSetupOptions(void)                                                    { return false; }
        virtual bool Service(const char *Id, void *Data = NULL)                               { return false;};
        virtual const char **SVDRPHelpPages(void)                                             { return NULL;}
        virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) { return NULL;}
}; // cPluginDpkgOpt
VDRPLUGINCREATOR(cPluginDpkgOpt);  // Don't touch this!
