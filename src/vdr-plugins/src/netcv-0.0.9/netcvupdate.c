/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * netcvupdate.c
 *
 ***************************************************************************/

#include <vdr/plugin.h>
#include <vdr/interface.h>

#include "netcvupdatemenu.h"
#include "netcvthread.h"

static const char *VERSION        = "0.0.5";
static const char *DESCRIPTION    = "NetCeiver update plugin";
static const char *MAINMENUENTRY  = "NetCeiver firmware update";

class cPluginNetcvUpdate : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  bool CompareVersion(const char *version1, const char *version2);
      
  char *NetcvUUID_;
  char *NetcvVersion_;
  char *UpdateVersion_;
public:
  cPluginNetcvUpdate(void);
  virtual ~cPluginNetcvUpdate();
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

  cNetCVThread *NetcvThread_;
  bool bRunningUpdate_;

  class cNetcvVersionThread : public cThread {
  public:
    cNetcvVersionThread(cPluginNetcvUpdate *parent):cThread("NetcvVersionThread"),p(parent) {}
    virtual void Action(void) {
      cCondWait::SleepMs(10000); // let the mcli-plugin and the system start

      isyslog("Checking for netcv updates");
      // Start any background activities the plugin shall perform.
      FILE *file;
      char *command;
      char *buffer;
      char *tmpversion;

      asprintf(&command, "ls -l %s | grep %s | grep .tgz", UPDATESDIRECTORY, UPDATEFILENAME_PREFIX);
      file = popen(command, "r");
      free(command);

      if(file)
      {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer)
        {
            tmpversion = strstr(buffer, UPDATEFILENAME_PREFIX) + strlen(UPDATEFILENAME_PREFIX);
            *(strstr(tmpversion, UPDATEFILENAME_EXTENSION)) = '\0';
            if(!p->UpdateVersion_)
                p->UpdateVersion_ = strdup(tmpversion);
            else if(p->CompareVersion(p->UpdateVersion_, tmpversion))
            {
                free(p->UpdateVersion_);
                p->UpdateVersion_ = strdup(tmpversion);
            }
            buffer = readline.Read(file);
        }
      }
      pclose(file);

      if(p->UpdateVersion_)
      {
        file = popen("netcvdiag -v | grep \"\\(FW\\|UUID\\)\"", "r");
        if(file)
        {
            cReadLine readline;
            buffer = readline.Read(file);
            while(buffer)
            {
                if(strstr(buffer, "FW"))
                {
                    tmpversion = strstr(buffer, "FW");
                    *(strchr(tmpversion, '>')) = '\0';
                    p->NetcvVersion_ = strdup(strchr(tmpversion, '\0') - 3);
                }
                else if(strstr(buffer, "UUID <"))
                {
                    *(strchr(buffer, '>')) = '\0';
                    p->NetcvUUID_ = strdup(strstr(buffer, "UUID <") + 6);
                }
                buffer = readline.Read(file);
            }
        }
        pclose(file);
      }
    } // Action
  private:
    cPluginNetcvUpdate *p;
  } NetcvVersionThread;
  };


cPluginNetcvUpdate::cPluginNetcvUpdate(void):NetcvVersionThread(this)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  NetcvUUID_     = NULL;
  NetcvVersion_  = NULL;
  UpdateVersion_ = NULL;
}

cPluginNetcvUpdate::~cPluginNetcvUpdate()
{
    // Clean up after yourself!
    if(NetcvUUID_)
        free(NetcvUUID_);
    if(NetcvVersion_)
        free(NetcvVersion_);
    if(UpdateVersion_)
        free(UpdateVersion_);
}

const char *cPluginNetcvUpdate::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginNetcvUpdate::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginNetcvUpdate::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginNetcvUpdate::Start(void)
{
#ifndef DONT_USE_SEPERATE_THREAD
    NetcvVersionThread.Start();
#else
    // Start any background activities the plugin shall perform.
    FILE *file;
    char *command;
    char *buffer;
    char *tmpversion;

    cCondWait::SleepMs(2000); // let the mcli-plugin start

    asprintf(&command, "ls -l %s | grep %s | grep .tgz", UPDATESDIRECTORY, UPDATEFILENAME_PREFIX);
    file = popen(command, "r");
    free(command);

    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer)
        {
            tmpversion = strstr(buffer, UPDATEFILENAME_PREFIX) + strlen(UPDATEFILENAME_PREFIX);
            *(strstr(tmpversion, UPDATEFILENAME_EXTENSION)) = '\0';
            if(!UpdateVersion_)
                UpdateVersion_ = strdup(tmpversion);
            else if(CompareVersion(UpdateVersion_, tmpversion))
            {
                free(UpdateVersion_);
                UpdateVersion_ = strdup(tmpversion);
            }
            buffer = readline.Read(file);
        }
    }
    pclose(file);

    if(UpdateVersion_)
    {
        file = popen("netcvdiag -v | grep \"\\(FW\\|UUID\\)\"", "r");
        if(file)
        {
            cReadLine readline;
            buffer = readline.Read(file);
            while(buffer)
            {
                if(strstr(buffer, "FW"))
                {
                    tmpversion = strstr(buffer, "FW");
                    *(strchr(tmpversion, '>')) = '\0';
                    NetcvVersion_ = strdup(strchr(tmpversion, '\0') - 3);
                }
                else if(strstr(buffer, "UUID <"))
                {
                    *(strchr(buffer, '>')) = '\0';
                    NetcvUUID_ = strdup(strstr(buffer, "UUID <") + 6);
                }
                buffer = readline.Read(file);
            }
        }
        pclose(file);
    }
#endif
    return true;
}

void cPluginNetcvUpdate::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginNetcvUpdate::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginNetcvUpdate::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginNetcvUpdate::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginNetcvUpdate::MainMenuAction(void)
{
//  // Perform the action when selected from the main VDR menu.
    if(NetcvVersion_ && UpdateVersion_)
    {
        if ((strcasecmp(UpdateVersion_, "9A5") >= 0) && (strcasecmp(NetcvVersion_, "9A5") < 0)) {
            NetcvThread_ = new cNetCVThread();
            return new cNetCVUpdateMenu(NetcvThread_, NetcvUUID_, NetcvVersion_, UpdateVersion_, true);
        } else if(CompareVersion(NetcvVersion_, UpdateVersion_) && Interface->Confirm(tr("NetCeiver update available. Do you want to update?")))
        {
            NetcvThread_ = new cNetCVThread();
            return new cNetCVUpdateMenu(NetcvThread_, NetcvUUID_, NetcvVersion_, UpdateVersion_);
        }
    }
    return NULL;
}

cMenuSetupPage *cPluginNetcvUpdate::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginNetcvUpdate::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginNetcvUpdate::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if(strcasecmp(Id, "ready") == 0) {
      if(Data != NULL) {
          *((bool*)Data) = (NetcvVersion_ && UpdateVersion_) ? strcmp(NetcvVersion_, UpdateVersion_) : 0;
      }
      return true;
  }
  return false;
}

const char **cPluginNetcvUpdate::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginNetcvUpdate::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginNetcvUpdate); // Don't touch this!

bool cPluginNetcvUpdate::CompareVersion(const char *version1, const char *version2)
{
    if(version1[0] < version2[0])
        return true;
    else if(version1[0] == version2[0])
    {
        if(version1[1] < version2[1])
            return true;
        else if((version1[1] == version2[1]) && (version1[2] < version2[2]))
            return true;
    }
    return false;
}


