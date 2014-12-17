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

#include <vdr/debug.h>
#include <vdr/plugin.h>
#include "setup.h"
#include "setupmenu.h"
#include "setupsetup.h"
#include "setupwlanmenu.h"
#if VDRVERSNUM >= 10716
#include "mainmenu.h"
#endif

#ifdef USEMYSQL
cInstallMysqlThread* cPluginSetup::MysqlInstallThread_ = NULL;
#endif

bool
    cPluginSetup::MenuLink = false;

cSetupSetup
    SetupSetup;

cPluginSetup::cPluginSetup(void)
{
    linkLabel_ = NULL;
#ifdef USEMYSQL
    if (!MysqlInstallThread_)
        MysqlInstallThread_ = new cInstallMysqlThread();
#endif
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginSetup::~cPluginSetup()
{
    // Clean up after yourself!
#ifdef USEMYSQL
    if(MysqlInstallThread_)
        delete MysqlInstallThread_;
    MysqlInstallThread_ = NULL;
#endif
}

const char *
cPluginSetup::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool
cPluginSetup::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool
cPluginSetup::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
#if VDRVERSNUM < 10727
    LoadXmlPhrases();
#endif
    return true;
}

bool
cPluginSetup::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}

void
cPluginSetup::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

cOsdObject *
cPluginSetup::MainMenuAction(void)
{
    //if (cPluginSetup::MenuLink)
    //printf (" cSetupMenu  ::MenuLink :  %s \n", cPluginSetup::MenuLink?"is Set":"is Not Set");
    return new cSetupMenu(linkLabel_);
}

cMenuSetupPage *
cPluginSetup::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return new cSetupSetupPage;

}

bool
cPluginSetup::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return SetupSetup.SetupParse(Name, Value);
}

bool
cPluginSetup::Service(const char *Id, void *Data)
{

    if (Id && strcmp(Id, "link") == 0)
    {
        linkLabel_ = static_cast < const char *>(Data);
        // printf ( " ++++++++++++++++++++ Get Service Link  Call Menu %s  \n", linkLabel_);
        cPluginSetup::MenuLink = true;

    }
    else if (std::strcmp(Id, "Setup Timezone") == 0)
    {
        if (Data != NULL)
        {
            cOsdMenu **menu = (cOsdMenu**) Data;
            if (menu)
                *menu = new cSetupMenu(NULL, "Timezone");
        }
    }
    else if (std::strcmp(Id, "Energy Options") == 0)
    {
        if (Data != NULL)
        {
            cOsdMenu **menu = (cOsdMenu**) Data;
            if (menu)
                *menu = new cSetupMenu(NULL, "Energy Options");
        }
    }
    else if (std::strcmp(Id, "Setup MultiRoom AVG not found") == 0)
    {
        if(Data != NULL)
        {
            cOsdMenu **menu = (cOsdMenu**) Data;
            if(menu)
                *menu = new cSetupNetClientMissingServerMenu(tr("MultiRoom - Error!"));
        }
    } else if (std::strcmp(Id, "Install Wizard WLAN setup menu") == 0) {
        struct InstallWizMenu { cOsdMenu*menu; const char*title;};
        if (Data) {
            InstallWizMenu* wiz = (InstallWizMenu*)Data;
            if (wiz) 
                wiz->menu = new cSetupWLANMenu(wiz->title, true);
        } else 
            isyslog("setup::service('%d') got NULL Data", Id);
    } else if (std::strcmp(Id, "Setup set mediadevice") == 0)
    {
        struct setupMediaDevice
        {
            std::string id;
        };
        
        std::string uuid = static_cast<setupMediaDevice *>(Data)->id;
        int ReplyCode;
        SVDRPCommand("CHMD", uuid.c_str(), ReplyCode);
    } else if (std::strcmp(Id, "MultiRoom enable") == 0) {
#ifdef USEMYSQL
    if (Setup.ReelboxMode == eModeServer) {
        if (!MysqlInstallThread_)
            MysqlInstallThread_ = new cInstallMysqlThread();

        if (!MysqlInstallThread_->Active()) {
            printf("\033[0;91mMysql install thread started\033[0m\n");
            MysqlInstallThread_->Start(true);
        } else
            printf("\033[0;91mMysql install thread already active\033[0m\n");
    }
#endif
    }
#if VDRVERSNUM >= 10716
    else if (std::strcmp(Id, "MenuMainHook-V1.0") == 0)
    {
        MenuMainHook_Data_V1_0 *mmh = (MenuMainHook_Data_V1_0 *)Data;
        switch(mmh->Function) {
            case osUnknown: mmh->pResultMenu = new cMainMenu(mmh->Function); break;
            case osSetup: mmh->pResultMenu = new cSetupMenu(linkLabel_); break;
            default     : DDD("MenuMainHook-V1.0 service %d: Not for me", mmh->Function); return false;
        }
    }
#endif
    else if (std::strcmp(Id, "parse external sysconfig") == 0) {
        bool hasNasAsMediaDir = false;
        cSetupNetClientMenu::parseAVGsSysconfig(&hasNasAsMediaDir);
        
        /* if the server does not use NAS for its recordings etc... mount
           the server's /media/hd */
        cSetupNetClientMenu::MountDefaultDir(Setup.NetServerIP, 0, hasNasAsMediaDir);
        printf("\033[0;91mNAS as media dir? %d\033[0m\n", hasNasAsMediaDir);
    }
    else if (std::strcmp(Id, "Set MaxMultiRoomClients") == 0) {
        if (Setup.ReelboxMode == eModeServer) {
            int *max  = (int*) Data;
            if (max) {
                isyslog("Changing MaxMultiRoomClients from %d to %d", 
                            Setup.MaxMultiRoomClients,
                            *max);

                Setup.MaxMultiRoomClients = *max;

                // announce the change to all plugins
                cPluginManager::CallAllServices("MaxMultiRoomClients changed");
            } // if max
            else 
                esyslog("MaxMultiRoomClients unchanged %d -- no 'Data' provided", 
                        Setup.MaxMultiRoomClients);
        } // max eModeServer
        else 
            esyslog("MaxMultiRoomClients unchanged %d -- not in server mode",
                    Setup.MaxMultiRoomClients);
    }
    else
        return false;

    return true;
}
const char **cPluginSetup::SVDRPHelpPages(void)
{
    static const char *HelpPages[] =
    {
        "CHMD\n"
            "    Change Media Drive: CHMD <UUID=uuid>",
        NULL
    };
    return HelpPages;
}

cString cPluginSetup::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (strcasecmp(Command, "CHMD") == 0)
    {
        if(!Option || strstr(Option, "UUID=")!=Option)
            return cString("Needs a UUID of partition/drive (where media files are stored)");

/*
        // check if *Option is a valid directory
        if (opendir(Option) == NULL && errno == ENOENT)
            return cString("Not a valid directory");

        // Option != /media/reel/ or its sub directories
        if (strcmp(Option, "/media/reel")==0 || strstr(Option, "/media/reel/") == Option)
            return cString("reelbox already uses /media/reel/, use another directory");
*/

        // taken from setuprecordingdevicemenu.cpp
        // remove old links before changing MEDIADEVICE in sysconfig
        SystemExec("(link-shares stop& ); sleep 3;");

        // Wait for it to finish reading the sysconfig file before changing it here
        // find a better way of handling it. link-shares takes too much time on NetClient.

        //cSysConfig_vdr::GetInstance().Save(); // save before destroy?
        cSysConfig_vdr::Destroy();
        cSysConfig_vdr::Create();
        cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");
        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", Option);
        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
        cSysConfig_vdr::GetInstance().Save();

        //create link (to network drives/shares) in the new mediadevice
        SystemExec("setup-mediadir");

        return cString("Changing media drive in the background, please wait.") ;
    }

    return NULL;
}


VDRPLUGINCREATOR(cPluginSetup); // Don't touch this!
