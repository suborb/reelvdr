/****************************************************************************
 * DESCRIPTION:
 *             Setup a Plugin vor VDR
 *
 * $Id: setup.cpp,v 1.13 2005/10/03 16:18:37 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#ifndef SETUP_H
#define SETUP_H

#include <vdr/plugin.h>
#include <vdr/config.h>
#include "setupmenu.h"
#include "setupsetup.h"
#include "setupmultiroom.h"
#if VDRVERSNUM < 10727
#include "i18n.h"
#endif

static const char *VERSION = "0.9.0";
static const char *DESCRIPTION = "ReelBox Setup";
static const char *MAINMENUENTRY = "ReelBox Setup";

class cPluginSetup:public cPlugin
{
  public:
    cPluginSetup(void);
      virtual ~ cPluginSetup();
    virtual const char *Version(void)
    {
        return VERSION;
    }
    virtual const char *Description(void)
    {
        return DESCRIPTION;
    }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Housekeeping(void);
    virtual const char *MainMenuEntry(void)
    {
        return MAINMENUENTRY;
    }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data);
    const char **SVDRPHelpPages(void);
    cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
    virtual bool HasSetupOptions(void)
    {
        return false;
    }
    static bool MenuLink;
#ifdef USEMYSQL
    static cInstallMysqlThread *MysqlInstallThread_;
#endif
  private:
    const char *linkLabel_;
};

#endif // SETUP_H
