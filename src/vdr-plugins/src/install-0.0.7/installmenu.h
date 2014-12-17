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
 * installmenu.h
 *
 ***************************************************************************/

#ifndef INSTALLMENU_H
#define INSTALLMENU_H

#include <vector>
#include <string>
#include <vdr/plugin.h>
#include "install.h"
#include "common.h"
#include "system_devices.h"
#include "tools.h"

std::string AvailableHardDisk();

#define HDDEVICE AvailableHardDisk()

enum eStep
{ eStart, eLanguage, eVideo, eConnections, eDiseqc, eChannellist, eChannelscan, eNetwork, eClient, eFormatHD, eConfigMultiroom, eHarddisk, eUpdate, eFinished, eInactive = 255 };

int GetFreeMountNumber();
std::string GetUUID(std::string harddiskDevice);

struct StructImage
{
    int x;
    int y;
    char path[255];
    bool blend;
    int slot;
    int w;
    int h;
};

class cInstallMenu:public cOsdMenu
{
  private:
    static unsigned int setupState; // how far we are in the wizard
      std::vector < int >InstallMenus;  // a list of single menus
    cSetup data;
    bool HarddiskAvailable_;
      std::string harddiskDevice;
    bool InternalHD_;
    bool skipChannelscan_;
    static int *ErrorState_;
    bool chListShowMsg_;
  public:
      cInstallMenu(int startWithStep, bool SkipChannelScan);
     ~cInstallMenu();
    void Display(void);
    void NextMenu(bool GoForward);
    static void SetErrorState(int number)
    {
        ErrorState_[setupState] = number;
    };
    eOSState GetMenu();
    eOSState ProcessKey(eKeys Key);
    void Set();
    void CheckForHD();
};

class cInstallSubMenu:public cOsdMenu
{
  protected:
    virtual bool Save() = 0;
    virtual void Set() = 0;
  public:
      cInstallSubMenu(const char*);
    void Display(void)
    {
        cOsdMenu::Display();
    }
    
    void SetTitle(const char* title) 
    {
        cString Title;
        if (title)
            Title = cString::sprintf("%s - %s", tr(MAINMENUENTRY), title);
        else
            Title = tr(MAINMENUENTRY);
        
        cOsdMenu::SetTitle(Title);
    }

    virtual eOSState ProcessKey(eKeys Key) = 0;
};

class cInstallFinished:public cOsdMenu
{
  private:
    void Set();
    bool Save();
  public:
      cInstallFinished(int messagenumber = 0);
    void Display(void);
    eOSState ProcessKey(eKeys Key);
};

bool CheckNetceiver();
bool CheckNetworkCarrier();

const char* DefaultNetworkDevice();
#define NETWORK_DEVICE DefaultNetworkDevice()

bool HDDHasMediaDevice(std::string device);

#endif
