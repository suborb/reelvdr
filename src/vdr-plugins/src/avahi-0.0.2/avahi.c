/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************
 *
 * avahi.c
 *
 ***************************************************************************/

#include <vdr/plugin.h>

#if VDRVERSNUM < 10727
#include "i18n.h"
#endif
#include "avahiservice.h"
#include "avahithread.h"
#include "avahilog.h"

#define AVAHI_TIMEOUT 5

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Avahi plugin";
static const char *MAINMENUENTRY  = trNOOP("Avahi");

class cPluginAvahi : public cPlugin {
    private:
        // Add any member variables or functions you may need here.
    public:
        cPluginAvahi(void);
        virtual ~cPluginAvahi();
        virtual const char *Version(void) { return VERSION; }
        virtual const char *Description(void) { return tr(DESCRIPTION); }
        virtual const char *CommandLineHelp(void);
        virtual bool ProcessArgs(int argc, char *argv[]);
        virtual bool Initialize(void);
        virtual bool Start(void);
        virtual void Stop(void);
        virtual void Housekeeping(void);
        virtual void MainThreadHook(void);
        virtual cString Active(void);
        virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
        virtual cOsdObject *MainMenuAction(void);
        virtual cMenuSetupPage *SetupMenu(void);
        virtual bool SetupParse(const char *Name, const char *Value);
        virtual bool HasSetupOptions(void) { return false; };
        virtual bool Service(const char *Id, void *Data = NULL);
        virtual const char **SVDRPHelpPages(void);
        virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

        cAvahiThread *AvahiThread_;
        int AvahiTimeout_;
};

cPluginAvahi::cPluginAvahi(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    AvahiThread_ = new cAvahiThread();
    AvahiTimeout_ = AVAHI_TIMEOUT;
}

cPluginAvahi::~cPluginAvahi()
{
    // Clean up after yourself!
    if(AvahiThread_)
        delete AvahiThread_;
}

const char *cPluginAvahi::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginAvahi::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginAvahi::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
#if VDRVERSNUM < 10727
    RegisterI18n(Phrases);
#endif
    return true;
}

bool cPluginAvahi::Start(void)
{
    // Start any background activities the plugin shall perform.
    cAvahiLog::Log("Plugin started");
    AvahiThread_->Start();
    return true;
}

void cPluginAvahi::Stop(void)
{
    // Stop any background activities the plugin shall perform.
}

void cPluginAvahi::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginAvahi::MainThreadHook(void)
{
    if(AvahiThread_ && AvahiThread_->Active() && (Setup.ReelboxMode==eModeClient))
    {
        if(Setup.ReelboxModeTemp != AvahiThread_->ReelboxMode_)
        {
            if(AvahiThread_->ReelboxMode_ == eModeClient)
            {
#ifdef AVAHI_DEBUG
                printf("\033[0;92m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "Enabling MultiRoom");
#endif
                cAvahiLog::Log("Enabling MultiRoom", __FILE__, __LINE__);
                Setup.ReelboxModeTemp=AvahiThread_->ReelboxMode_;
                AvahiTimeout_ = AVAHI_TIMEOUT; // Reset Timeout
            }
            else if(AvahiThread_->ReelboxMode_ == eModeStandalone)
            {
                {
                    if(!--AvahiTimeout_) // If timeout happen, disable MultiRoom temporarily
                    {
#ifdef AVAHI_DEBUG
                        printf("\033[0;92m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "Disabling MultiRoom");
#endif
                        cAvahiLog::Log("Disabling MultiRoom", __FILE__, __LINE__);
                        Setup.ReelboxModeTemp = AvahiThread_->ReelboxMode_;
                    }
                    else
                    {
#ifdef AVAHI_DEBUG
                        printf("\033[0;92m %s(%i): %s %i \033[0m\n", __FILE__, __LINE__, "MultiRoom gone...", AvahiTimeout_);
#endif
                        char buffer[32];
                        snprintf(buffer, 32, "MultiRoom gone... %i", AvahiTimeout_);
                        cAvahiLog::Log(buffer, __FILE__, __LINE__);
                    }
                }
            }
        }
        else
            AvahiTimeout_ = AVAHI_TIMEOUT; // Reset Timeout
    }
}

cString cPluginAvahi::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

cOsdObject *cPluginAvahi::MainMenuAction(void)
{
    //  // Perform the action when selected from the main VDR menu.
    return NULL;
}

cMenuSetupPage *cPluginAvahi::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return NULL;
}

bool cPluginAvahi::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return false;
}

bool cPluginAvahi::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    if(strcmp(Id, "Avahi ReelBox-List") == 0)
    {
        if(Data != NULL)
        {
            std::vector<ReelBox_t> *reelboxes = (std::vector<ReelBox_t>*) Data;
            cAvahiThread::GetReelBoxList(reelboxes);
        }
        return true;
    }
    else if(strcmp(Id, "Avahi MultiRoom-List") == 0)
    {
        if(Data != NULL)
        {
            std::vector<ReelBox_t> *databases = (std::vector<ReelBox_t>*) Data;
            cAvahiThread::GetDatabaseList(databases);
        }
        return true;
    }
    else if(strcmp(Id, "Avahi check NetServer") == 0)
    {
        // This services checks if NetServer is available and its MultiRoom is enabled
        if(Data != NULL)
        {
            bool *NetServerAvailable = (bool*) Data;
            *NetServerAvailable = cAvahiThread::CheckNetServer();
        }
        return true;
    }
    else if(strcmp(Id, "Avahi Restart") == 0)
    {
        AvahiThread_->Restart();
        return true;
    }
    else if(strcmp(Id, "Avahi MySQL enable") == 0)
    {
        AvahiThread_->ReelboxMode_ = eModeClient;
        return true;
    }
    return false;
}

const char **cPluginAvahi::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    return NULL;
}

cString cPluginAvahi::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    return NULL;
}

VDRPLUGINCREATOR(cPluginAvahi); // Don't touch this!
