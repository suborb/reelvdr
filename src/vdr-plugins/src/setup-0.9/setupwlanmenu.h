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
 * setupwlanmenu.h
 *
 ***************************************************************************/


#ifndef SETUPWLANMENU_H
#define SETUPWLANMENU_H

#include <vdr/menu.h>

#include "wlansetting.h"

enum eWLANSetupState { eDevice, eSSID, eEncryption, eExpert, eFinish };

class cWlanDeviceItem : public cOsdItem
{
    private:
        bool Marked_;
        char* Interface_;
        char* HardwareName_;

    public:
        cWlanDeviceItem(char *Interface, bool Marked=false, char* HardwareName=NULL);
        ~cWlanDeviceItem();

        const char* GetInterface() { return Interface_; };
        const char* GetHardwareName() { return HardwareName_; };
};

class cSetupWLANMenu : public cOsdMenu
{
    private:
        eWLANSetupState State_;
        std::vector<cWlanDevice *> Devices_;
        cWLANSetting *WlanSetting;
        int wlanEnable; // 0 : disabled, 1: enabled
        bool changed; // if some setting is changed, then it's true (useful for osBack)
//        bool finished_text;
        int counter; // UGLY Hack against UGLY twice-kOk-pressed-bug

        void runDHCP();
        bool InstallWiz;
    public:
        cSetupWLANMenu(const char *title, bool installWizard=false);
        ~cSetupWLANMenu();

        void Set();
        eOSState ProcessKey(eKeys Key);
        void ResetSettings();
        void LoadSettings();
        void SaveSettings();
};

class cSetupWLANMenuFunctions : public cOsdMenu
{
    public:
        cSetupWLANMenuFunctions();
        ~cSetupWLANMenuFunctions();

        void Set();
};

#endif

