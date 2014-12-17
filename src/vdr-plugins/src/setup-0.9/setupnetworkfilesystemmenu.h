/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * setupnetworkfilesystemmenu.h
 *
 ***************************************************************************/

#ifndef SETUPNETWORKFILESYSTEMMENU_H
#define SETUPNETWORKFILESYSTEMMENU_H

#include <vector>

#include <vdr/osdbase.h>
#include <vdr/thread.h>

#include "networkfilesystem.h"

class cScanNetworkFSThread : public cThread
{
    private:
        int Progress_;
        bool successful_;
        std::vector<cNetworkFSHost*> *vHosts_;

        bool CheckLocalIP(const char *IP);

    public:
        cScanNetworkFSThread(cOsdMenu *menu, std::vector<cNetworkFSHost*> *vHosts);
        ~cScanNetworkFSThread();

        void Action();
        void Interrupt() { Cancel(); };
        void Scan();
        int Progress() { return Progress_; };
};

//#### cSetupNetworkFilesystemMenu ##################################
class cSetupNetworkFilesystemMenu : public cOsdMenu
{
    private:
        std::vector<cNetworkFSHost*> vHosts_; // Scanned Hosts
        std::vector<cNetworkFilesystem*> vShares_; // Shares, which is in list

        bool hadSubmenu;
        void Set();
        void SetHelpKeys();
        eOSState ProcessKey(eKeys Key);
        void Load();
#if VDRVERSNUM >= 10716
        // Easier than patching all GetTitle...
        const char *GetTitle( void) {return Title();}
#endif
    public:
        cSetupNetworkFilesystemMenu(const char *title);
        ~cSetupNetworkFilesystemMenu();
};

//#### cSetupNetworkFSScanMenu ######################################
class cSetupNetworkFSScanMenu : public cOsdMenu
{
    private:
        bool Scanning_;
        int OldSize_;
        int OldProgress_;
        cScanNetworkFSThread *ScanNetworkFSThread_;
        std::vector<cNetworkFSHost*> *vHosts_;
        std::vector<cNetworkFilesystem*> *vShares_;

        void Set();
        eOSState ProcessKey(eKeys Key);
#if VDRVERSNUM >= 10716
        // Easier than patching all GetTitle...
        const char *GetTitle( void) {return Title();}
#endif

    public:
        cSetupNetworkFSScanMenu(const char *title, std::vector<cNetworkFSHost*> *vHosts, std::vector<cNetworkFilesystem*> *vShares);
        ~cSetupNetworkFSScanMenu();

        void Scan();
        void Update();
};

class cHostItem : public cOsdItem
{
    private:
        cNetworkFSHost *NetworkFSHost_;

        eOSState ProcessKey(eKeys Key);

    public:
        cHostItem(cNetworkFSHost *NetworkFSHost);
        ~cHostItem();
};

class cMacHostItem : public cHostItem
{
    private:
        cOsdMenu *Menu_;
        cNetworkFSMacHost *MacHost_;

        eOSState ProcessKey(eKeys Key);

    public:
        cMacHostItem(cOsdMenu *Menu, cNetworkFSMacHost *NetworkFSMacHost);
        ~cMacHostItem();

        cNetworkFSMacHost* GetObject() { return MacHost_; };
};

class cVistaHostItem : public cHostItem
{
    private:
        cOsdMenu *Menu_;
        cNetworkFSVistaHost *VistaHost_;

        eOSState ProcessKey(eKeys Key);

    public:
        cVistaHostItem(cOsdMenu *Menu, cNetworkFSVistaHost *VistaHost);
        ~cVistaHostItem();

        cNetworkFSVistaHost* GetObject() { return VistaHost_; };
};

class cShareSmbItem : public cOsdItem
{
    private:
        cNetworkFSSMB *NetworkFSSMB_;

    public:
        cShareSmbItem(cNetworkFSSMB *NetworkFSSMB, bool UseDisplayname = false);
        ~cShareSmbItem();

        cNetworkFSSMB* GetObject() { return NetworkFSSMB_; };
};

class cShareNfsItem : public cOsdItem
{
    private:
        cNetworkFSNFS *NetworkFSNFS_;

    public:
        cShareNfsItem(cNetworkFSNFS *NetworkFSNFS, bool UseDisplayname = false);
        ~cShareNfsItem();

        cNetworkFSNFS* GetObject() { return NetworkFSNFS_; };
};

class cShareAfpItem : public cOsdItem
{
    private:
        cNetworkFSAFP *NetworkFSAFP_;

    public:
        cShareAfpItem(cNetworkFSAFP *NetworkFSAFP, bool UseDisplayname = false);
        ~cShareAfpItem();

        cNetworkFSAFP* GetObject() { return NetworkFSAFP_; };
};

//#### cShareSmbSettingsMenu ###############################
class cShareSmbSettingsMenu : public cOsdMenu
{
    private:
        cNetworkFSSMB *NetworkFSSMB_;
        bool ExpertSettings_;
        bool edit_;

        void Set();
        void Save();
        eOSState ProcessKey(eKeys Key);

    public:
        cShareSmbSettingsMenu(const char *title, cNetworkFSSMB *NetworkFSSMB, bool edit=false);
        ~cShareSmbSettingsMenu();
};

//#### cShareNfsSettingsMenu ###############################
class cShareNfsSettingsMenu : public cOsdMenu
{
    private:
        cNetworkFSNFS *NetworkFSNFS_;
        bool ExpertSettings_;
        bool edit_;

        void Set();
        void Save();
        eOSState ProcessKey(eKeys Key);

    public:
        cShareNfsSettingsMenu(const char *title, cNetworkFSNFS *NetworkFSNFS, bool edit=false);
        ~cShareNfsSettingsMenu();
};

//#### cShareAfpSettingsMenu ###############################
class cShareAfpSettingsMenu : public cOsdMenu
{
    private:
        cNetworkFSAFP *NetworkFSAFP_;
        bool ExpertSettings_;
        bool edit_;

        void Set();
        void Save();
        eOSState ProcessKey(eKeys Key);

    public:
        cShareAfpSettingsMenu(const char *title, cNetworkFSAFP *NetworkFSAFP, bool edit=false);
        ~cShareAfpSettingsMenu();
};

//#### cHostAuthenticationMenu ###############################
class cHostAuthenticationMenu : public cOsdMenu
{
    private:
        const char *Address_;
        char *Username_;
        char *Password_;
        bool *ShowShares_;
        cNetworkFSMacHost *MacHost_;
        cNetworkFSVistaHost *VistaHost_;

        void Set();
        eOSState ProcessKey(eKeys Key);

    public:
        cHostAuthenticationMenu(const char *title, cNetworkFSHost *NetworkFSHost);
        ~cHostAuthenticationMenu();

        void ScanShares();
};

#endif

