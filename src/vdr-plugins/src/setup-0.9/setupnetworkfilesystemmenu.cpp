/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 fixes by Tobias Bratfisch                          *
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
 * setupnetworkfilesystemmenu.cpp
 *
 ***************************************************************************/

#include "setupnetworkfilesystemmenu.h"
#include "menueditrefreshitems.h"
#include <vdr/sysconfig_vdr.h>
#include "setupmenu.h"

#include <vdr/debug.h>
#include <vdr/tools.h>

#define SHARE_MOUNTPOINTS "/media/.mount"
#define COMMAND_SHOWSHARES "showshares"
#define SETCOLS_SHARESETTINGS 18
#define MAX_LETTER_PER_LINE 48

#define MESSAGE_PLEASE_WAIT trNOOP("Please wait...")
#define MESSAGE_PLEASE_TEST trNOOP("Please test:")
#define MESSAGE_SETTING_STORED trNOOP("Settings stored")
#define SELECT_SHARE_TEXT trNOOP("Please select a share:")
#define PRESS_OK_TO_STORE trNOOP("Please press OK to store settings.")

const char *allowed_chars = trNOOP("AllowedCharsNetShares$ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!_-.,#~^$+@{}");

static const char* NetworkProtocol_string[] = { "udp", "tcp" };
static const char* BlockSize_string[] = { "2048", "4096", "8192" };

//###########################################################################
//  cScanNetworkFSThread
//###########################################################################
cScanNetworkFSThread::cScanNetworkFSThread(cOsdMenu *menu, std::vector<cNetworkFSHost*> *vHosts)
{
    successful_ = false;
    Progress_ = -1; // inactive
    vHosts_ = vHosts;
}

cScanNetworkFSThread::~cScanNetworkFSThread()
{
}

void cScanNetworkFSThread::Action()
{
    successful_ = false;
    Progress_ = 0;
    FILE *process;
    char *buffer;
    std::string strBuff;
    char *trash = (char*) malloc(256 * sizeof(char));
    char *host = (char*) malloc(256 * sizeof(char));
    char *port = (char*) malloc(16 * sizeof(char));
    char *sharename = (char*) malloc(256 * sizeof(char));

    strBuff = std::string(COMMAND_SHOWSHARES) + " --status";
    process = popen(strBuff.c_str(), "r");
    if(process)
    {
        cReadLine readline;

        buffer = readline.Read(process);
        while(Running() && buffer)
        {
            if(strstr(buffer, "[PROGRESS]\t") == buffer)
                Progress_ = atoi(buffer+11);
            else if(strstr(buffer, "[FOUND]\t") == buffer)
            {
                int sscanfSuccess = 0;
                if(sscanf(buffer, "%s\t%s\t%s\t%s", trash, host, port, sharename) == 4 && atoi(port))
                    sscanfSuccess = 1;
                // Fix for SMB-shares with spaces
                if(!strcmp(port, "139"))
                    if(sscanf(buffer, "%s\t%s\t%s\t%[^\n]", trash, host, port, sharename) == 4 && atoi(port))
                        sscanfSuccess = 1;

                if(sscanfSuccess && !CheckLocalIP(host))
                {
                    unsigned int i = 0;
                    bool found = false;
                    while(!found && i < vHosts_->size())
                    {
                        if(strcmp(host, vHosts_->at(i)->GetHost()))
                            ++i;
                        else
                            found = true;
                    }
                    if(!found)
                    {
                        //printf("\033[0;91m %s(%i): Add new Host %s \033[0m\n", __FILE__, __LINE__, host);
                        vHosts_->push_back(new cNetworkFSHost(host));
                    }
                    if(!strcmp(port, "139"))
                        vHosts_->at(i)->AddShareSMB(sharename);
                    else if(!strcmp(port, "2049"))
                        vHosts_->at(i)->AddShareNFS(sharename);
                }
            }
            else if(strstr(buffer, "[FOUND-AUTH]\t") == buffer)
            {
                int sscanfSuccess = 0;
                if(sscanf(buffer, "%s\t%s\t%s", trash, host, port) == 3 && atoi(port))
                    sscanfSuccess = 1;

                if(sscanfSuccess)
                {
                    unsigned int i = 0;
                    bool found = false;
                    while(!found && i < vHosts_->size())
                    {
                        if(strcmp(host, vHosts_->at(i)->GetHost()))
                            ++i;
                        else
                            found = true;
                    }
                    if(!found)
                    {
                        if(!strcmp(port, "548"))
                        {
                            //printf("\033[0;91m %s(%i): Add new Host (Mac) %s \033[0m\n", __FILE__, __LINE__, host);
                            vHosts_->push_back(new cNetworkFSMacHost(host));
                        }
                        else if(!strcmp(port, "139"))
                        {
                            //printf("\033[0;91m %s(%i): Add new Host (Vista) %s \033[0m\n", __FILE__, __LINE__, host);
                            vHosts_->push_back(new cNetworkFSVistaHost(host));
                        }
                    }
                }

            }
            else if(!strcmp(buffer, "completed"))
                successful_ = true;

            buffer = readline.Read(process);
        }
        pclose(process);
    }
    if(Running())
    {
        if(successful_)
            Progress_ = 100; // successful
        else
            Progress_ = -2; // failed
    }
    free(trash);
    free(host);
    free(port);
    free(sharename);
}

bool cScanNetworkFSThread::CheckLocalIP(const char *IP)
{
    cNetInfo netinfo;
    for(int i =0; i < netinfo.IfNum(); ++i)
    {
        if(!strcmp(IP, netinfo[i]->IpAddress()))
            return true;
    }
    return false;
}

//###########################################################################
//  cSetupNetworkFilesystemMenu
//###########################################################################

cSetupNetworkFilesystemMenu::cSetupNetworkFilesystemMenu(const char *title) : cOsdMenu(title)
{
    hadSubmenu = false;
    Load();
    Set();
}

cSetupNetworkFilesystemMenu::~cSetupNetworkFilesystemMenu()
{
    for(unsigned int i=0; i < vHosts_.size(); ++i)
        delete vHosts_.at(i);
    vHosts_.clear();
    for(unsigned int i=0; i < vShares_.size(); ++i)
        delete vShares_.at(i);
    vShares_.clear();
}

void cSetupNetworkFilesystemMenu::Set()
{
    Clear();
    SetCols(2, 2);

//    printf("\033[0;92m %s(%i) Numbers of Shares: %i \033[0m\n", __FILE__, __LINE__, vShares_.size());
    for(unsigned int i = 0; i < vShares_.size(); ++i)
    {
        cNetworkFSSMB *SmbShare = dynamic_cast<cNetworkFSSMB*>(vShares_.at(i));
        cNetworkFSNFS *NfsShare = dynamic_cast<cNetworkFSNFS*>(vShares_.at(i));
        cNetworkFSAFP *ApfShare = dynamic_cast<cNetworkFSAFP*>(vShares_.at(i));
        if(SmbShare)
            Add(new cShareSmbItem(SmbShare, true));
        if(NfsShare)
            Add(new cShareNfsItem(NfsShare, true));
        if(ApfShare)
            Add(new cShareAfpItem(ApfShare, true));
    }

    if(!vShares_.size())
    {
        Add(new cOsdItem(tr("Currently no active connections")));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Info:")));
        AddFloatingText(tr("Do you want to use Network Shares? Press OK."), 45 );
    }
    Display();
    SetHelpKeys();
}

void cSetupNetworkFilesystemMenu::SetHelpKeys()
{
    cShareSmbItem *smbshareitem = dynamic_cast<cShareSmbItem*>(Get(Current()));
    cShareNfsItem *nfsshareitem = dynamic_cast<cShareNfsItem*>(Get(Current()));
    cShareAfpItem *afpshareitem = dynamic_cast<cShareAfpItem*>(Get(Current()));

    if(smbshareitem)
        SetHelp(tr("Search"), NULL, tr("Delete"), smbshareitem->GetObject()->StartClient_?tr("Button$Disable"):tr("Button$Enable"));
    else if(nfsshareitem)
        SetHelp(tr("Search"), NULL, tr("Delete"), nfsshareitem->GetObject()->StartClient_?tr("Button$Disable"):tr("Button$Enable"));
    else if(afpshareitem)
        SetHelp(tr("Search"), NULL, tr("Delete"), afpshareitem->GetObject()->StartClient_?tr("Button$Disable"):tr("Button$Enable"));
    else
        SetHelp(tr("Search"));
}

eOSState cSetupNetworkFilesystemMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    cShareSmbItem *smbshareitem = dynamic_cast<cShareSmbItem*>(Get(Current()));
    cShareNfsItem *nfsshareitem = dynamic_cast<cShareNfsItem*>(Get(Current()));
    cShareAfpItem *afpshareitem = dynamic_cast<cShareAfpItem*>(Get(Current()));

    char strBuff[512];
    if(HasSubMenu())
    {
        if(state == osBack)
        {
            CloseSubMenu();
            return osContinue;
        }
        else if(state == osUser1) // Close Submenu & Refresh Menu
        {
            CloseSubMenu();
            int current = Current();
            Set();
            SetCurrent(Get(current));
            Display();
            SetHelpKeys();
            return osContinue;
        }
    }
    else
        SetHelpKeys();

    if (!HasSubMenu() && hadSubmenu) /** redraw if a submenu was closed */
        Set();

    if (HasSubMenu())
        hadSubmenu = true;
    else
        hadSubmenu = false;

    if(state == osContinue)
        return osContinue;

    if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                if(smbshareitem)
                    return AddSubMenu(new cShareSmbSettingsMenu(GetTitle(), smbshareitem->GetObject(), true));
                if(nfsshareitem)
                    return AddSubMenu(new cShareNfsSettingsMenu(GetTitle(), nfsshareitem->GetObject(), true));
                if(afpshareitem)
                    return AddSubMenu(new cShareAfpSettingsMenu(GetTitle(), afpshareitem->GetObject(), true));

                if(!vShares_.size())
                    return AddSubMenu(new cSetupNetworkFSScanMenu(GetTitle(), &vHosts_, &vShares_));
                break;
            case kRed:
                return AddSubMenu(new cSetupNetworkFSScanMenu(GetTitle(), &vHosts_, &vShares_));
            case kYellow:
                {
                    //Get Object and delete this + set to NULL then remove NULL-Element from vector
                    bool found = false;
                    int current = Current();
                    if(smbshareitem || nfsshareitem || afpshareitem)
                    {
                        if(Interface->Confirm(tr("Do you want to remove the connection?")))
                        {
                            Skins.Message(mtStatus, tr("Please wait..."));
                            cNetworkFilesystem *share;
                            if(smbshareitem)
                                share = smbshareitem->GetObject();
                            else if(nfsshareitem)
                                share = nfsshareitem->GetObject();
                            else
                                share = afpshareitem->GetObject();

                            if(share)
                            {
                                // Remove RecordingMedia settings if this element is the used one
                                if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
                                {
                                    int CurrentRecDeviceNumber = -1;
                                    const char *CurrentRecDevice = cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE");
                                    if(strstr(CurrentRecDevice, SHARE_MOUNTPOINTS))
                                        CurrentRecDeviceNumber = atoi(CurrentRecDevice+strlen(SHARE_MOUNTPOINTS));
                                    if(CurrentRecDeviceNumber == share->GetNumber())
                                    {
                                        if(Interface->Confirm(tr("Recording won't be possible on this share. Continue?")))
                                        {
                                            Skins.Message(mtStatus, tr("Please wait..."));
                                            // Delete settings
                                            cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", "");
                                            cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE_ONLY_RECORDING", "");
                                            cSysConfig_vdr::GetInstance().Save();
                                            SystemExec("setup-mediadir");
                                            Recordings.Update(false);
                                            sprintf(strBuff, "umount -f /media/.mount%i", CurrentRecDeviceNumber);
                                            SystemExec(strBuff);
                                        }
                                        else
                                            return osContinue;
                                    }
                                }
                            }

                            std::vector<cNetworkFilesystem*>::iterator shareIter = vShares_.begin();
                            while(!found && (shareIter != vShares_.end()))
                            {
                                if(*(shareIter) == share)
                                {
                                    vShares_.erase(shareIter);
                                    found = true;
                                    sprintf(strBuff, "setup-shares stop %i", share->GetNumber());
                                    SystemExec(strBuff);
                                    if(smbshareitem)
                                        sprintf(strBuff, "sed -i \"/^MOUNT%i_SMB/d\" /etc/default/sysconfig", share->GetNumber());
                                    else if(nfsshareitem)
                                        sprintf(strBuff, "sed -i \"/^MOUNT%i_NFS/d\" /etc/default/sysconfig", share->GetNumber());
                                    else
                                        sprintf(strBuff, "sed -i \"/^MOUNT%i_AFP/d\" /etc/default/sysconfig", share->GetNumber());
                                    SystemExec(strBuff);
                                    // TODO: If we use vdr-sysconfig, then do cSysConfig_vdr::Load() here!

                                    delete share;
                                }
                                ++shareIter;
                                Skins.Message(mtStatus, tr("Please wait..."));
                            }
                        }
                        else
                            return osContinue;
                    }
                    else
                        return osUnknown;

                    Set();
                    SetCurrent(Get(current));
                    Display();
                    SetHelpKeys();
                }
                break;
            case kBlue:
                if(smbshareitem || nfsshareitem || afpshareitem)
                {
                    int current = Current();
                    int Number;
                    bool *StartClient;
                    char *strBuff;

                    if(smbshareitem)
                    {
                        StartClient = &smbshareitem->GetObject()->StartClient_;
                        Number = smbshareitem->GetObject()->Number_;
                    }
                    else if(nfsshareitem)
                    {
                        StartClient = &nfsshareitem->GetObject()->StartClient_;
                        Number = nfsshareitem->GetObject()->Number_;
                    }
                    else
                    {
                        StartClient = &afpshareitem->GetObject()->StartClient_;
                        Number = afpshareitem->GetObject()->Number_;
                    }

                    *StartClient = *StartClient ? false : true;
                    // write to sysconfig
                    asprintf(&strBuff, "MOUNT%i_%s_STARTCLIENT", Number, smbshareitem ? "SMB" : "NFS");
                    cSysConfig_vdr::GetInstance().SetVariable(strBuff, *StartClient ? "yes" : "no");
                    cSysConfig_vdr::GetInstance().Save();
                    free(strBuff);

                    // Umount share
                    asprintf(&strBuff, "setup-shares %s %i", *StartClient ? "start" : "stop", Number);
                    SystemExec(strBuff);
                    free(strBuff);

                    Set();
                    SetCurrent(Get(current));
                    Display();
                    SetHelpKeys();
                    return osContinue;
                }
                return osUnknown;
            default:
                return osUnknown;
        }
    }
    return state;
}

void cSetupNetworkFilesystemMenu::Load()
{
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");
    char *buffer;
    char strBuff[512];
    char *Host;
    char *Sharename;
    FILE *process;

    // SMB
    process = popen("grep SMB_HOST /etc/default/sysconfig | grep ^MOUNT | sed -e 's/^MOUNT//' | awk -F'_' '{print $1}'", "r");

    cReadLine readline;
    buffer = readline.Read(process);
    while(buffer)
    {
        sprintf(strBuff, "MOUNT%s_SMB_HOST", buffer);
        Host = strdup(sysconfig.GetVariable(strBuff));
        sprintf(strBuff, "MOUNT%s_SMB_SHARENAME", buffer);
        Sharename = strdup(sysconfig.GetVariable(strBuff));

        cNetworkFSSMB *SmbShare = new cNetworkFSSMB(Host, Sharename);
        free(Host);
        free(Sharename);

        SmbShare->Number_ = atoi(buffer);

        sprintf(strBuff, "MOUNT%s_SMB_STARTCLIENT", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->StartClient_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_SMB_DISPLAYNAME", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Displayname_ = strdup(sysconfig.GetVariable(strBuff));

        sprintf(strBuff, "MOUNT%s_SMB_PICTURES", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Pictures_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_SMB_MUSIC", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Music_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_SMB_RECORDINGS", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Recordings_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_SMB_VIDEOS", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Videos_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        //SMB-specified settings
        sprintf(strBuff, "MOUNT%s_SMB_USERNAME", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Username_ = strdup(sysconfig.GetVariable(strBuff));

        sprintf(strBuff, "MOUNT%s_SMB_PASSWORD", buffer);
        if(sysconfig.GetVariable(strBuff))
            SmbShare->Password_ = strdup(sysconfig.GetVariable(strBuff));

        vShares_.push_back(SmbShare);
        buffer = readline.Read(process);
    }
    pclose(process);

    // NFS
    process = popen("grep NFS_HOST /etc/default/sysconfig | grep ^MOUNT | sed -e 's/^MOUNT//' | awk -F'_' '{print $1}'", "r");

    buffer = readline.Read(process);
    while(buffer)
    {
        sprintf(strBuff, "MOUNT%s_NFS_HOST", buffer);
        Host = strdup(sysconfig.GetVariable(strBuff));
        sprintf(strBuff, "MOUNT%s_NFS_SHARENAME", buffer);
        Sharename = strdup(sysconfig.GetVariable(strBuff));

        cNetworkFSNFS *NfsShare = new cNetworkFSNFS(Host, Sharename);
        free(Host);
        free(Sharename);

        NfsShare->Number_ = atoi(buffer);

        sprintf(strBuff, "MOUNT%s_NFS_STARTCLIENT", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->StartClient_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_DISPLAYNAME", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->Displayname_ = strdup(sysconfig.GetVariable(strBuff));

        sprintf(strBuff, "MOUNT%s_NFS_PICTURES", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->Pictures_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_MUSIC", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->Music_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_RECORDINGS", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->Recordings_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_VIDEOS", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->Videos_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        //NFS-specified settings
        sprintf(strBuff, "MOUNT%s_NFS_NETWORKPROTOCOL", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->NetworkProtocol_ = !strcmp(sysconfig.GetVariable(strBuff), "tcp");

        sprintf(strBuff, "MOUNT%s_NFS_BLOCKSIZE", buffer);
        if(sysconfig.GetVariable(strBuff))
        {
            if(!strcmp(sysconfig.GetVariable(strBuff), "2048"))
                NfsShare->BlockSize_ = 0;
            else if(!strcmp(sysconfig.GetVariable(strBuff), "4096"))
                NfsShare->BlockSize_ = 1;
            else if(!strcmp(sysconfig.GetVariable(strBuff), "8192"))
                NfsShare->BlockSize_ = 2;
        }
        sprintf(strBuff, "MOUNT%s_NFS_OPTIONSHS", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->OptionHS_ = !strcmp(sysconfig.GetVariable(strBuff), "hard");

        sprintf(strBuff, "MOUNT%s_NFS_OPTIONLOCK", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->OptionLock_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_OPTIONRW", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->OptionRW_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_NFS_NFSVERSION", buffer);
        if(sysconfig.GetVariable(strBuff))
            NfsShare->NFSVersion_ = atoi(sysconfig.GetVariable(strBuff));

        vShares_.push_back(NfsShare);
        buffer = readline.Read(process);
    }
    pclose(process);

    // AFP
    process = popen("grep AFP_HOST /etc/default/sysconfig | grep ^MOUNT | sed -e 's/^MOUNT//' | awk -F'_' '{print $1}'", "r");

    buffer = readline.Read(process);
    while(buffer)
    {
        sprintf(strBuff, "MOUNT%s_AFP_HOST", buffer);
        Host = strdup(sysconfig.GetVariable(strBuff));
        sprintf(strBuff, "MOUNT%s_AFP_SHARENAME", buffer);
        Sharename = strdup(sysconfig.GetVariable(strBuff));

        cNetworkFSAFP *AfpShare = new cNetworkFSAFP(Host, Sharename);
        free(Host);
        free(Sharename);

        AfpShare->Number_ = atoi(buffer);

        sprintf(strBuff, "MOUNT%s_AFP_STARTCLIENT", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->StartClient_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_AFP_DISPLAYNAME", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Displayname_ = strdup(sysconfig.GetVariable(strBuff));

        sprintf(strBuff, "MOUNT%s_AFP_PICTURES", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Pictures_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_AFP_MUSIC", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Music_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_AFP_RECORDINGS", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Recordings_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        sprintf(strBuff, "MOUNT%s_AFP_VIDEOS", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Videos_ = !strcmp(sysconfig.GetVariable(strBuff), "yes");

        //AFP-specified settings
        sprintf(strBuff, "MOUNT%s_AFP_USERNAME", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Username_ = strdup(sysconfig.GetVariable(strBuff));

        sprintf(strBuff, "MOUNT%s_AFP_PASSWORD", buffer);
        if(sysconfig.GetVariable(strBuff))
            AfpShare->Password_ = strdup(sysconfig.GetVariable(strBuff));

        vShares_.push_back(AfpShare);
        buffer = readline.Read(process);
    }
    pclose(process);

}

//###########################################################################
//  cSetupNetworkFSScanMenu
//###########################################################################
cSetupNetworkFSScanMenu::cSetupNetworkFSScanMenu(const char *title, std::vector<cNetworkFSHost*> *vHosts, std::vector<cNetworkFilesystem*> *vShares) : cOsdMenu(title)
{
    Scanning_ = false;
    vHosts_ = vHosts;
    vShares_ = vShares;
    ScanNetworkFSThread_ = new cScanNetworkFSThread(this, vHosts_);

    SetCols(2);
    if(vHosts_->size() == 0)
        Scan();
    else
        Set();
    Display();
}

cSetupNetworkFSScanMenu::~cSetupNetworkFSScanMenu()
{
    delete ScanNetworkFSThread_;
}

void cSetupNetworkFSScanMenu::Set()
{
    bool used_share;
    char strBuff[MOUNTPOINT_LENGTH];

    Clear();
    SetCols(2, 2);

    if(vHosts_->size())
    {
        Add(new cOsdItem(tr(SELECT_SHARE_TEXT), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
    }

    for(unsigned int i = 0; i < vHosts_->size(); ++i)
    {
        // Add Host
        cNetworkFSMacHost *MacHost = dynamic_cast<cNetworkFSMacHost*>(vHosts_->at(i));
        cNetworkFSVistaHost *VistaHost = dynamic_cast<cNetworkFSVistaHost*>(vHosts_->at(i));
        if(MacHost)
            Add(new cMacHostItem(this, MacHost));
        else if(VistaHost)
            Add(new cVistaHostItem(this, VistaHost));
        else
            Add(new cHostItem(vHosts_->at(i)));

        // Add Shares of the Host
        if(vHosts_->at(i)->ShowShares_)
        {
            for(unsigned int j = 0; j < vHosts_->at(i)->CountShareSMB(); ++j)
            {
                cNetworkFSSMB *ShareSMB = vHosts_->at(i)->GetShareSMB(j);
                used_share = false;

                unsigned int x = 0;
                while(!used_share && x < vShares_->size())
                {
                    cNetworkFSSMB *smbshare = dynamic_cast<cNetworkFSSMB*>(vShares_->at(x));
                    if(smbshare)
                    {
                        if(!strcmp(vHosts_->at(i)->GetHost(), smbshare->GetHost()))
                            if(!strcmp(ShareSMB->GetSharename(), smbshare->GetSharename()))
                                used_share = true;
                    }
                    if(!used_share)
                        ++x;
                }

                if(used_share)
                {
                    snprintf(strBuff, MOUNTPOINT_LENGTH-1, "\t>\t%c %s", (char) 138, ShareSMB->Displayname_);
                    Add(new cOsdItem(strBuff, osUnknown, false));
                }
                else
                    Add(new cShareSmbItem(ShareSMB));
            }

            for(unsigned int j = 0; j < vHosts_->at(i)->CountShareNFS(); ++j)
            {
                cNetworkFSNFS *ShareNFS = vHosts_->at(i)->GetShareNFS(j);
                used_share = false;

                unsigned int x = 0;
                while(!used_share && x < vShares_->size())
                {
                    cNetworkFSNFS *nfsshare = dynamic_cast<cNetworkFSNFS*>(vShares_->at(x));
                    if(nfsshare)
                    {
                        if(!strcmp(vHosts_->at(i)->GetHost(), nfsshare->GetHost()))
                            if(!strcmp(ShareNFS->GetSharename(), nfsshare->GetSharename()))
                                used_share = true;
                    }
                    if(!used_share)
                        ++x;
                }

                if(used_share)
                {
                    snprintf(strBuff, MOUNTPOINT_LENGTH-1, "\t>\t%c %s", (char) 138, ShareNFS->Displayname_);
                    Add(new cOsdItem(strBuff, osUnknown, false));
                }
                else
                    Add(new cShareNfsItem(ShareNFS));
            }
        }
        if(MacHost && MacHost->ShowShares_)
        {
            for(unsigned int j =0 ; j < MacHost->CountShareAFP(); ++j)
            {
                cNetworkFSAFP *ShareAFP = MacHost->GetShareAFP(j);
                used_share = false;

                unsigned int x = 0;
                do
                {
                    cNetworkFSAFP *afpshare = dynamic_cast<cNetworkFSAFP*>(vShares_->at(x));
                    if(afpshare)
                    {
                        if(!strcmp(MacHost->GetHost(), afpshare->GetHost()))
                            if(!strcmp(MacHost->GetShareAFP(j)->GetSharename(), afpshare->GetSharename()))
                                used_share = true;
                    }
                    if(!used_share)
                        ++x;
                }
                while(!used_share && x < vShares_->size());

                if(used_share)
                {
                    snprintf(strBuff, MOUNTPOINT_LENGTH-1, "\t>\t%c %s", (char) 138, ShareAFP->Displayname_);
                    Add(new cOsdItem(strBuff, osUnknown, false));
                }
                else
                    Add(new cShareAfpItem(ShareAFP));
            }
        }
    }
}

eOSState cSetupNetworkFSScanMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(HasSubMenu())
    {
        switch (state)
        {
          case osBack:
          {
            CloseSubMenu();
            return osContinue;
          }
          case osUser1: // Close Submenu & Refresh Menu
          {
            CloseSubMenu();
            int current = Current();
            Set();
            SetCurrent(Get(current));
            Display();
            return osContinue;
          }
          case osUser2: // Save settings
          {
            CloseSubMenu();
            cShareSmbItem *sharesmbitem = dynamic_cast<cShareSmbItem*>(Get(Current()));
            cShareNfsItem *sharenfsitem = dynamic_cast<cShareNfsItem*>(Get(Current()));
            cShareAfpItem *shareafpitem = dynamic_cast<cShareAfpItem*>(Get(Current()));
            if(sharesmbitem)
                vShares_->push_back(new cNetworkFSSMB(*(sharesmbitem->GetObject())));
            else if(sharenfsitem)
                vShares_->push_back(new cNetworkFSNFS(*(sharenfsitem->GetObject())));
            else if(shareafpitem)
                vShares_->push_back(new cNetworkFSAFP(*(shareafpitem->GetObject())));
            return osUser1; // Close this & Refresh parent Menu
          }
          default:
              return state;
        }
    }

    if(ScanNetworkFSThread_->Active())
    {
        Scanning_ = true;
        if((int)vHosts_->size() != OldSize_)
            Update();
        if(OldProgress_ != ScanNetworkFSThread_->Progress())
        {
            OldProgress_ = ScanNetworkFSThread_->Progress();
            if(OldProgress_>=0)
            {
                char strBuff[64];
                snprintf(strBuff, 64, "%s %i%%", tr("Scanning..."), OldProgress_);
                Skins.Message(mtStatus, strBuff);
            }
        }
        switch(Key)
        {
            /* stop nfs scan */
            case kOk: // fall through
            case kBlue:
                isyslog("kblue: stop scanning for nfs shares");
                ScanNetworkFSThread_->Interrupt();
                Scanning_ = false;
                Skins.Message(mtStatus, NULL);          // clear status message
                return osContinue;                      // don't handle key in the parent menu
                break;
            case kBack:
                ScanNetworkFSThread_->Interrupt();
                return osBack;
            default:
                return osContinue;
        }
    }
    else
    {
        if(Scanning_)
        {
            // After scan actions
            if(ScanNetworkFSThread_->Progress() == 100)
                Skins.Message(mtInfo, tr("Scanning complete."));
            else if(ScanNetworkFSThread_->Progress() == -2)
                Skins.Message(mtError, tr("Scanning failed!"));

            Get(0)->SetText(tr(SELECT_SHARE_TEXT));
            Scanning_ = false;
            if(vHosts_->size())
                SetCurrent(Get(2)); // Select first Host if available
            Display();
        }
        SetHelp(tr("Rescan"),NULL ,NULL, NULL);
    }

    if(state == os_User) // Refresh Menu
    {
        int current = Current();
        Set();
        SetCurrent(Get(current));
        Display();
    }

    if(state == osContinue)
        return osContinue;

    if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                {
                    // Shares
                    cShareSmbItem *sharesmbitem = dynamic_cast<cShareSmbItem*>(Get(Current()));
                    if(sharesmbitem)
                        return AddSubMenu(new cShareSmbSettingsMenu(GetTitle(), sharesmbitem->GetObject()));
                    cShareNfsItem *sharenfsitem = dynamic_cast<cShareNfsItem*>(Get(Current()));
                    if(sharenfsitem)
                        return AddSubMenu(new cShareNfsSettingsMenu(GetTitle(), sharenfsitem->GetObject()));
                    cShareAfpItem *shareafpitem = dynamic_cast<cShareAfpItem*>(Get(Current()));
                    if(shareafpitem)
                        return AddSubMenu(new cShareAfpSettingsMenu(GetTitle(), shareafpitem->GetObject()));

                    // Host
                    cMacHostItem *machostitem = dynamic_cast<cMacHostItem*>(Get(Current()));
                    if(machostitem)
                    {
                        char title[128];
                        snprintf(title, 128, "%s: %s", GetTitle(), machostitem->GetObject()->GetHost());
                        return AddSubMenu(new cHostAuthenticationMenu(title, machostitem->GetObject()));
                    }
                    cVistaHostItem *vistahostitem = dynamic_cast<cVistaHostItem*>(Get(Current()));
                    if(vistahostitem)
                    {
                        char title[128];
                        snprintf(title, 128, "%s: %s", GetTitle(), vistahostitem->GetObject()->GetHost());
                        return AddSubMenu(new cHostAuthenticationMenu(title, vistahostitem->GetObject()));
                    }

                    return osContinue;
                }
            case kRed:
                Scan();
                return osContinue;
            case kBack:
                return osBack;
            default:
                return osContinue;  //RC: do not close the menu here cause scanning can take quite long
                                    //and we don't want to purge the results
                                    //TODO: close the menu after 30 or so minutes
        }
    }
    return state;
}

void cSetupNetworkFSScanMenu::Scan()
{
    Skins.Message(mtStatus, tr("Please wait..."));

    // Clear list
    if(vHosts_->size())
    {
        for(unsigned int i=0; i < vHosts_->size(); ++i)
            delete vHosts_->at(i);
        vHosts_->clear();
    }
    OldSize_ = 0;
    OldProgress_ = -1;

    // Prepare menu
    Clear();
    SetHelp(NULL);
    Add(new cOsdItem(tr("Found shares:"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Display();
    SetHelp(NULL,NULL,NULL,tr("Stop"));

    ScanNetworkFSThread_->Start();
}

void cSetupNetworkFSScanMenu::Update()
{
    int Size = vHosts_->size();

    for(int i=OldSize_; i<Size; ++i)
    {
        // Add Host
        cNetworkFSMacHost *MacHost = dynamic_cast<cNetworkFSMacHost*>(vHosts_->at(i));
        cNetworkFSVistaHost *VistaHost = dynamic_cast<cNetworkFSVistaHost*>(vHosts_->at(i));
        if(MacHost)
            Add(new cMacHostItem(this, MacHost));
        else if(VistaHost)
            Add(new cVistaHostItem(this, VistaHost));
        else
            Add(new cHostItem(vHosts_->at(i)));
    }

    OldSize_ = Size;
    Display();

    // Redisplay Status
    if(OldProgress_ >=0)
    {
        char strBuff[64];
        snprintf(strBuff, 64, "%s %i%%", tr("Scanning..."), OldProgress_);
        Skins.Message(mtStatus, strBuff);
    }
}

//###########################################################################
//  cHostItem
//###########################################################################
cHostItem::cHostItem(cNetworkFSHost *NetworkFSHost)
{
    char *strBuff;
    NetworkFSHost_ = NetworkFSHost;

    asprintf(&strBuff, "%c\t%s %s",(char)137, NetworkFSHost_->ShowShares_ ? "- " : "+", NetworkFSHost_->GetHost());
    SetText(strBuff);
    free(strBuff);
}

cHostItem::~cHostItem()
{
}

eOSState cHostItem::ProcessKey(eKeys Key)
{
    if(Key == kOk)
    {
        NetworkFSHost_->ShowShares_ = !NetworkFSHost_->ShowShares_;
        return os_User; //Refresh menu
    }
    else
        return osUnknown;
}

//###########################################################################
//  cMacHostItem
//###########################################################################
cMacHostItem::cMacHostItem(cOsdMenu *Menu, cNetworkFSMacHost *NetworkFSMacHost) : cHostItem(NetworkFSMacHost)
{
    Menu_ = Menu;
    MacHost_ = NetworkFSMacHost;
}

cMacHostItem::~cMacHostItem()
{
}

eOSState cMacHostItem::ProcessKey(eKeys Key)
{
    if(Key == kOk)
    {
        if(strlen(MacHost_->Username_))
        {
            MacHost_->ShowShares_ = !MacHost_->ShowShares_;
            return os_User; //Refresh menu
        }
    }

    return osUnknown;
}

//###########################################################################
//  cVistaHostItem
//###########################################################################
cVistaHostItem::cVistaHostItem(cOsdMenu *Menu, cNetworkFSVistaHost *NetworkFSVistaHost) : cHostItem(NetworkFSVistaHost)
{
    Menu_ = Menu;
    VistaHost_ = NetworkFSVistaHost;
}

cVistaHostItem::~cVistaHostItem()
{
}

eOSState cVistaHostItem::ProcessKey(eKeys Key)
{
    if(Key == kOk)
    {
        if(strlen(VistaHost_->Username_))
        {
            VistaHost_->ShowShares_ = !VistaHost_->ShowShares_;
            return os_User; //Refresh menu
        }
    }

    return osUnknown;
}

//###########################################################################
//  cShareSmbItem
//###########################################################################
cShareSmbItem::cShareSmbItem(cNetworkFSSMB *NetworkFSSMB, bool UseDisplayname)
{
    NetworkFSSMB_ = NetworkFSSMB;
    char buffer[MOUNTPOINT_LENGTH];
    char *CurrentRecDevice=NULL;
    int CurrentRecDeviceNumber = -1;

    if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
    {
        CurrentRecDevice = strdup(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"));
        if(strstr(CurrentRecDevice, SHARE_MOUNTPOINTS))
            CurrentRecDeviceNumber = atoi(CurrentRecDevice+strlen(SHARE_MOUNTPOINTS));
    }
    if(CurrentRecDevice)
        free(CurrentRecDevice);

    if(UseDisplayname)
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "%c\t%c\t%c %s (%s)", CurrentRecDeviceNumber==NetworkFSSMB_->Number_?'#':' ', NetworkFSSMB_->MountActive()?'>':' ', (char) 138, NetworkFSSMB_->Displayname_, NetworkFSSMB_->GetHost());
    else
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "\t%c %s   ", (char) 138, NetworkFSSMB_->GetSharename());

    SetText( buffer );
}

cShareSmbItem::~cShareSmbItem()
{
}

//###########################################################################
//  cShareNfsItem
//###########################################################################
cShareNfsItem::cShareNfsItem(cNetworkFSNFS *NetworkFSNFS, bool UseDisplayname)
{
    NetworkFSNFS_ = NetworkFSNFS;
    char buffer[MOUNTPOINT_LENGTH];
    char *CurrentRecDevice=NULL;
    int CurrentRecDeviceNumber = -1;

    if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
    {
        CurrentRecDevice = strdup(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"));
        if(strstr(CurrentRecDevice, SHARE_MOUNTPOINTS))
            CurrentRecDeviceNumber = atoi(CurrentRecDevice+strlen(SHARE_MOUNTPOINTS));
    }
    if(CurrentRecDevice)
        free(CurrentRecDevice);

    if(UseDisplayname)
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "%c\t%c\t%c %s (%s)", CurrentRecDeviceNumber==NetworkFSNFS_->Number_?'#':' ', NetworkFSNFS_->MountActive()?'>':' ', (char) 138, NetworkFSNFS_->Displayname_,NetworkFSNFS_->GetHost() );
    else
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "\t%c %s   ", (char) 138, NetworkFSNFS_->GetSharename());

    SetText( buffer );
}

cShareNfsItem::~cShareNfsItem()
{
}

//###########################################################################
//  cShareAfpItem
//###########################################################################
cShareAfpItem::cShareAfpItem(cNetworkFSAFP *NetworkFSAFP, bool UseDisplayname)
{
    NetworkFSAFP_ = NetworkFSAFP;
    char buffer[MOUNTPOINT_LENGTH];
    char *CurrentRecDevice=NULL;
    int CurrentRecDeviceNumber = -1;

    if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
    {
        CurrentRecDevice = strdup(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"));
        if(strstr(CurrentRecDevice, SHARE_MOUNTPOINTS))
            CurrentRecDeviceNumber = atoi(CurrentRecDevice+strlen(SHARE_MOUNTPOINTS));
    }
    if(CurrentRecDevice)
        free(CurrentRecDevice);

    if(UseDisplayname)
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "%c\t%c\t%c %s (%s)", CurrentRecDeviceNumber==NetworkFSAFP_->Number_?'#':' ', NetworkFSAFP_->MountActive()?'>':' ', (char) 138, NetworkFSAFP_->Displayname_,NetworkFSAFP_->GetHost() );
    else
        snprintf(buffer, MOUNTPOINT_LENGTH-1, "\t%c %s   ", (char) 138, NetworkFSAFP_->GetSharename());

    SetText( buffer );
}

cShareAfpItem::~cShareAfpItem()
{
}

//###########################################################################
//  cShareSmbSettingsMenu
//###########################################################################
cShareSmbSettingsMenu::cShareSmbSettingsMenu(const char *title, cNetworkFSSMB *NetworkFSSMB, bool edit) : cOsdMenu(title)
{
    char *strBuff;
    ExpertSettings_ = false;
    NetworkFSSMB_ = NetworkFSSMB;
    edit_ = edit;

    asprintf(&strBuff, "%s: %s:%s", title, NetworkFSSMB_->GetHost(), NetworkFSSMB_->GetSharename());
    SetTitle(strBuff);
    free(strBuff);

    Set();
    Display();
}

cShareSmbSettingsMenu::~cShareSmbSettingsMenu()
{
}

void cShareSmbSettingsMenu::Set()
{
    char *strBuff;

    Clear();
    Display();
#if VDRVERSNUM < 10716
    int MaxItems = displayMenu->MaxItems();
#else
    int MaxItems = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->MaxItems();
#endif
    SetCols(SETCOLS_SHARESETTINGS);

    asprintf(&strBuff, "%s:\t%s", tr("Host"), NetworkFSSMB_->GetHost());
    Add(new cOsdItem(strBuff, osUnknown, false));
    free(strBuff);
    Add(new cMenuEditStrItem(tr("Share name"), NetworkFSSMB_->Sharename_, DISPLAYNAME_LENGTH, tr(allowed_chars)));
    if(ExpertSettings_)
    {
        asprintf(&strBuff, "%s:\t%i", tr("Share Number"), NetworkFSSMB_->GetNumber());
        Add(new cOsdItem(strBuff, osUnknown, false));
        free(strBuff);
    }
    Add(new cMenuEditStrItem(tr("Display name"), NetworkFSSMB_->Displayname_, DISPLAYNAME_LENGTH, tr(allowed_chars)));
    Add(new cMenuEditStrItem(tr("Username"), NetworkFSSMB_->Username_, USERNAME_LENGTH, tr(allowed_chars)));
    Add(new cMenuEditStrItem(tr("Password"), NetworkFSSMB_->Password_, PASSWORD_LENGTH, tr(allowed_chars)));

    if(ExpertSettings_)
    {
        SetHelp(NULL, NULL, NULL, trVDR("Standard"));

        Add(new cMenuEditBoolItem(tr("Pictures"), &(NetworkFSSMB_->Pictures_)));
        Add(new cMenuEditBoolItem(tr("Music"), &(NetworkFSSMB_->Music_)));
        Add(new cMenuEditBoolItem(tr("Videos"), &(NetworkFSSMB_->Videos_)));
        Add(new cMenuEditBoolItem(tr("Recordings"), &(NetworkFSSMB_->Recordings_)));
    }
    else
        SetHelp(NULL, NULL, NULL, tr("Expert"));

    // Add info for users, to tell them to press "OK" for store settings (why? :/ )
    int addSpaces = std::max(MaxItems-Count()-2, 1);
    for(int i=0; i<addSpaces; ++i)
        Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr(PRESS_OK_TO_STORE), MAX_LETTER_PER_LINE);
}

eOSState cShareSmbSettingsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    /** redraw the help keys when canceling editing the display name with kBack */
    if (state == osContinue && Key == kBack) {
        if(ExpertSettings_)
            SetHelp(NULL, NULL, NULL, trVDR("Standard"));
        else
            SetHelp(NULL, NULL, NULL, tr("Expert"));
    }

    if(state == os_User) // Refresh Menu
    {
        int current = Current();
        Set();
        SetCurrent(Get(current));
        Display();
        return osContinue;
    }
    else if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                {
                    bool save = false;
                    if(strlen(NetworkFSSMB_->Username_) == 0)
                    {
                        if(Interface->Confirm(tr("Do you want to use the guest account?")))
                        {
                            strcpy(NetworkFSSMB_->Username_, "guest");
                            save = true;
                        }
                    }
                    else
                        save = true;
                    if(save)
                    {
                        Skins.Message(mtInfo, tr(MESSAGE_PLEASE_WAIT));
                        Save();
                        Skins.Message(mtInfo, tr(MESSAGE_SETTING_STORED));
                        if(!edit_ && Setup.ReelboxMode != eModeClient)
                            NetworkFSSMB_->SetRecordingMedia();
                        return edit_ ? osUser1 : osUser2;
                    }
                    else
                        return osContinue;
                }
            case kBack:
                return osBack;
            case kBlue:
                ExpertSettings_ = !ExpertSettings_;
                Set();
                Display();
                return osContinue;
            default:
                return osUnknown;
        }
    }
    return state;
}

void cShareSmbSettingsMenu::Save()
{
    char strBuff[512];
    if(NetworkFSSMB_->Number_ == -1)
        NetworkFSSMB_->GetFreeNumber();
    int Number = NetworkFSSMB_->Number_;

    cSysConfig_vdr::Destroy();
    cSysConfig_vdr::Create();
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    // Store SMB-Shares
    sprintf(strBuff, "MOUNT%i_SMB_STARTCLIENT", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->StartClient_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_SMB_HOST", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->GetHost());
    sprintf(strBuff, "MOUNT%i_SMB_SHARENAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->GetSharename());
    sprintf(strBuff, "MOUNT%i_SMB_DISPLAYNAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Displayname_);
    sprintf(strBuff, "MOUNT%i_SMB_PICTURES", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Pictures_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_SMB_MUSIC", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Music_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_SMB_RECORDINGS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Recordings_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_SMB_VIDEOS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Videos_ ? "yes" : "no");

    //SMB-specified options
    sprintf(strBuff, "MOUNT%i_SMB_USERNAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Username_);
    sprintf(strBuff, "MOUNT%i_SMB_PASSWORD", Number);
    sysconfig.SetVariable(strBuff, NetworkFSSMB_->Password_);

    sysconfig.Save();

    sprintf(strBuff, "setup-shares %s %i", edit_ ? "restart" : "start", Number);
    SystemExec(strBuff);
}

//###########################################################################
//  cShareNfsSettingsMenu
//###########################################################################
cShareNfsSettingsMenu::cShareNfsSettingsMenu(const char *title, cNetworkFSNFS *NetworkFSNFS, bool edit) : cOsdMenu(title)
{
    char *strBuff;
    ExpertSettings_ = false;
    NetworkFSNFS_ = NetworkFSNFS;
    edit_ = edit;

    asprintf(&strBuff, "%s: %s:%s", title, NetworkFSNFS_->GetHost(), NetworkFSNFS_->GetSharename());
    SetTitle(strBuff);
    free(strBuff);

    Set();
    Display();
}

cShareNfsSettingsMenu::~cShareNfsSettingsMenu()
{
}

void cShareNfsSettingsMenu::Set()
{
    char *strBuff;

    Clear();
    Display();
#if VDRVERSNUM < 10716
    int MaxItems = displayMenu->MaxItems();
#else
    int MaxItems = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->MaxItems();
#endif
    SetCols(SETCOLS_SHARESETTINGS);

    asprintf(&strBuff, "%s:\t%s", tr("Host"), NetworkFSNFS_->GetHost());
    Add(new cOsdItem(strBuff, osUnknown, false));
    free(strBuff);

    Add(new cMenuEditStrItem(tr("Share name"), NetworkFSNFS_->Sharename_, DISPLAYNAME_LENGTH, tr(FileNameChars)));

    if(ExpertSettings_)
    {
        asprintf(&strBuff, "%s:\t%i", tr("Share Number"), NetworkFSNFS_->GetNumber());
        Add(new cOsdItem(strBuff, osUnknown, false));
        free(strBuff);
    }
    Add(new cMenuEditStrItem(tr("Display name"), NetworkFSNFS_->Displayname_, DISPLAYNAME_LENGTH, tr(allowed_chars)));

    if(ExpertSettings_)
    {
        SetHelp(NULL, NULL, NULL, trVDR("Standard"));

        Add(new cMenuEditBoolItem(tr("Pictures"), &(NetworkFSNFS_->Pictures_)));
        Add(new cMenuEditBoolItem(tr("Music"), &(NetworkFSNFS_->Music_)));
        Add(new cMenuEditBoolItem(tr("Videos"), &(NetworkFSNFS_->Videos_)));
        Add(new cMenuEditBoolItem(tr("Recordings"), &(NetworkFSNFS_->Recordings_)));

        // NFS-specified options
        Add(new cMenuEditIntRefreshItem(tr("NFS Version"), &NetworkFSNFS_->NFSVersion_, 3, 4));
        Add(new cMenuEditBoolItem(tr("Protocol"), &NetworkFSNFS_->NetworkProtocol_, "UDP", "TCP"));
        Add(new cMenuEditStraItem(tr("Block size"), &NetworkFSNFS_->BlockSize_, 3, BlockSize_string));
        Add(new cMenuEditBoolItem(tr("hard/soft"), &NetworkFSNFS_->OptionHS_, tr("soft"), tr("hard")));
        if(NetworkFSNFS_->NFSVersion_ == 3)
            Add(new cMenuEditBoolItem(tr("Lock"), &NetworkFSNFS_->OptionLock_));
        Add(new cMenuEditBoolItem(tr("Read/Write"), &NetworkFSNFS_->OptionRW_, tr("Readonly"), tr("Read&Write")));
    }
    else
        SetHelp(NULL, NULL, NULL, tr("Expert"));

    // Add info for users, to tell them to press "OK" for store settings (why? :/ )
    int addSpaces = std::max(MaxItems-Count()-2, 1);
    for(int i = 0; i < addSpaces; ++i)
        Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr(PRESS_OK_TO_STORE), MAX_LETTER_PER_LINE);
}

eOSState cShareNfsSettingsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    /** redraw the help keys when canceling editing the display name with kBack */
    if (state == osContinue && Key == kBack) {
        if(ExpertSettings_)
            SetHelp(NULL, NULL, NULL, trVDR("Standard"));
        else
            SetHelp(NULL, NULL, NULL, tr("Expert"));
    }

    if(state == os_User) // Refresh Menu
    {
        int current = Current();
        Set();
        SetCurrent(Get(current));
        Display();
        return osContinue;
    }
    else if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                Skins.Message(mtInfo, tr(MESSAGE_PLEASE_WAIT));
                Save();
                Skins.Message(mtInfo, tr(MESSAGE_SETTING_STORED));
                if(!edit_ && Setup.ReelboxMode != eModeClient)
                    NetworkFSNFS_->SetRecordingMedia();
                return edit_ ? osUser1 : osUser2;
            case kBack:
                return osBack;
            case kBlue:
                ExpertSettings_ = !ExpertSettings_;
                Set();
                Display();
                return osContinue;
            default:
                return osUnknown;
        }
    }
    return state;
}

void cShareNfsSettingsMenu::Save()
{
    char strBuff[512];
    if(NetworkFSNFS_->Number_ == -1)
        NetworkFSNFS_->GetFreeNumber();
    int Number = NetworkFSNFS_->Number_;

    cSysConfig_vdr::Destroy();
    cSysConfig_vdr::Create();
    cSysConfig_vdr &sysconfig=cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    // Store NFS-Shares
    sprintf(strBuff, "MOUNT%i_NFS_STARTCLIENT", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->StartClient_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_HOST", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->GetHost());
    sprintf(strBuff, "MOUNT%i_NFS_SHARENAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->GetSharename());
    sprintf(strBuff, "MOUNT%i_NFS_DISPLAYNAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->Displayname_);
    sprintf(strBuff, "MOUNT%i_NFS_PICTURES", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->Pictures_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_MUSIC", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->Music_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_RECORDINGS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->Recordings_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_VIDEOS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->Videos_ ? "yes" : "no");

    // NFS-specified options
    sprintf(strBuff, "MOUNT%i_NFS_NETWORKPROTOCOL", Number);
    sysconfig.SetVariable(strBuff, NetworkProtocol_string[NetworkFSNFS_->NetworkProtocol_]);
    sprintf(strBuff, "MOUNT%i_NFS_BLOCKSIZE", Number);
    sysconfig.SetVariable(strBuff, BlockSize_string[NetworkFSNFS_->BlockSize_]);
    sprintf(strBuff, "MOUNT%i_NFS_OPTIONHS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->OptionHS_ ? "hard" : "soft");
    sprintf(strBuff, "MOUNT%i_NFS_OPTIONLOCK", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->OptionLock_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_OPTIONRW", Number);
    sysconfig.SetVariable(strBuff, NetworkFSNFS_->OptionRW_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_NFS_NFSVERSION", Number);
    sysconfig.SetVariable(strBuff, itoa(NetworkFSNFS_->NFSVersion_));

    sysconfig.Save();

    sprintf(strBuff, "setup-shares %s %i", edit_?"restart":"start", Number);
    SystemExec(strBuff);
}

//###########################################################################
//  cShareAfpSettingsMenu
//###########################################################################
cShareAfpSettingsMenu::cShareAfpSettingsMenu(const char *title, cNetworkFSAFP *NetworkFSAFP, bool edit) : cOsdMenu(title)
{
    char *strBuff;
    ExpertSettings_ = false;
    NetworkFSAFP_ = NetworkFSAFP;
    edit_ = edit;

    asprintf(&strBuff, "%s: %s:%s", title, NetworkFSAFP_->GetHost(), NetworkFSAFP_->GetSharename());
    SetTitle(strBuff);
    free(strBuff);

    Set();
    Display();
}

cShareAfpSettingsMenu::~cShareAfpSettingsMenu()
{
}

void cShareAfpSettingsMenu::Set()
{
    char *strBuff;

    Clear();
    Display();
#if VDRVERSNUM < 10716
    int MaxItems = displayMenu->MaxItems();
#else
    int MaxItems = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->MaxItems();
#endif
    SetCols(SETCOLS_SHARESETTINGS);

    asprintf(&strBuff, "%s:\t%s", tr("Host"), NetworkFSAFP_->GetHost());
    Add(new cOsdItem(strBuff, osUnknown, false));
    free(strBuff);

    asprintf(&strBuff, "%s:\t%s", tr("Share name"), NetworkFSAFP_->GetSharename());
    Add(new cOsdItem(strBuff, osUnknown, false));
    free(strBuff);

    Add(new cMenuEditStrItem(tr("Share name"), NetworkFSAFP_->Sharename_, DISPLAYNAME_LENGTH, tr(allowed_chars)));

    if(ExpertSettings_)
    {
        asprintf(&strBuff, "%s:\t%i", tr("Share Number"), NetworkFSAFP_->GetNumber());
        Add(new cOsdItem(strBuff, osUnknown, false));
        free(strBuff);
    }
    Add(new cMenuEditStrItem(tr("Display name"), NetworkFSAFP_->Displayname_, DISPLAYNAME_LENGTH, tr(allowed_chars)));
    Add(new cMenuEditStrItem(tr("Username"), NetworkFSAFP_->Username_, USERNAME_LENGTH, tr(allowed_chars)));
    Add(new cMenuEditStrItem(tr("Password"), NetworkFSAFP_->Password_, PASSWORD_LENGTH, tr(allowed_chars)));

    if(ExpertSettings_)
    {
        SetHelp(NULL, NULL, NULL, trVDR("Standard"));

        Add(new cMenuEditBoolItem(tr("Pictures"), &(NetworkFSAFP_->Pictures_)));
        Add(new cMenuEditBoolItem(tr("Music"), &(NetworkFSAFP_->Music_)));
        Add(new cMenuEditBoolItem(tr("Videos"), &(NetworkFSAFP_->Videos_)));
        Add(new cMenuEditBoolItem(tr("Recordings"), &(NetworkFSAFP_->Recordings_)));
    }
    else
        SetHelp(NULL, NULL, NULL, tr("Expert"));

    // Add info for users, to tell them to press "OK" for store settings (why? :/ )
    int addSpaces = std::max(MaxItems-Count()-2, 1);
    for(int i = 0; i < addSpaces; ++i)
        Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr(PRESS_OK_TO_STORE), MAX_LETTER_PER_LINE);
}

eOSState cShareAfpSettingsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    /** redraw the help keys when canceling editing the display name with kBack */
    if (state == osContinue && Key == kBack) {
        if(ExpertSettings_)
            SetHelp(NULL, NULL, NULL, trVDR("Standard"));
        else
            SetHelp(NULL, NULL, NULL, tr("Expert"));
    }

    if(state == os_User) // Refresh Menu
    {
        int current = Current();
        Set();
        SetCurrent(Get(current));
        Display();
        return osContinue;
    }
    else if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                {
                    bool save = false;
                    if(strlen(NetworkFSAFP_->Username_) == 0)
                    {
                        if(Interface->Confirm(tr("Do you want to use the guest account?")))
                        {
                            strcpy(NetworkFSAFP_->Username_, "guest");
                            save = true;
                        }
                    }
                    else
                        save = true;
                    if(save)
                    {
                        Skins.Message(mtInfo, tr(MESSAGE_PLEASE_WAIT));
                        Save();
                        Skins.Message(mtInfo, tr(MESSAGE_SETTING_STORED));
                        if(!edit_ && Setup.ReelboxMode!=eModeClient)
                            NetworkFSAFP_->SetRecordingMedia();
                        return edit_ ? osUser1 : osUser2;
                    }
                    else
                        return osContinue;
                }
            case kBack:
                return osBack;
            case kBlue:
                ExpertSettings_ = !ExpertSettings_;
                Set();
                Display();
                return osContinue;
            default:
                return osUnknown;
        }
    }
    return state;
}

void cShareAfpSettingsMenu::Save()
{
    char strBuff[512];
    if(NetworkFSAFP_->Number_ == -1)
        NetworkFSAFP_->GetFreeNumber();
    int Number = NetworkFSAFP_->Number_;

    cSysConfig_vdr::Destroy();
    cSysConfig_vdr::Create();
    cSysConfig_vdr &sysconfig=cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    // Store AFP-Shares
    sprintf(strBuff, "MOUNT%i_AFP_STARTCLIENT", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->StartClient_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_AFP_HOST", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->GetHost());
    sprintf(strBuff, "MOUNT%i_AFP_SHARENAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->GetSharename());
    sprintf(strBuff, "MOUNT%i_AFP_DISPLAYNAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Displayname_);
    sprintf(strBuff, "MOUNT%i_AFP_PICTURES", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Pictures_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_AFP_MUSIC", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Music_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_AFP_RECORDINGS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Recordings_ ? "yes" : "no");
    sprintf(strBuff, "MOUNT%i_AFP_VIDEOS", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Videos_ ? "yes" : "no");

    //AFP-specified options
    sprintf(strBuff, "MOUNT%i_AFP_USERNAME", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Username_);
    sprintf(strBuff, "MOUNT%i_AFP_PASSWORD", Number);
    sysconfig.SetVariable(strBuff, NetworkFSAFP_->Password_);

    sysconfig.Save();

    sprintf(strBuff, "setup-shares %s %i", edit_ ? "restart" : "start", Number);
    SystemExec(strBuff);
}

//###########################################################################
//  cHostAuthenticationMenu
//###########################################################################
cHostAuthenticationMenu::cHostAuthenticationMenu(const char* title, cNetworkFSHost *Host) : cOsdMenu(title), Address_(Host->GetHost())
{
    MacHost_ = dynamic_cast<cNetworkFSMacHost*>(Host);
    VistaHost_ = dynamic_cast<cNetworkFSVistaHost*>(Host);
    if(MacHost_)
    {
        Username_ = MacHost_->Username_;
        Password_ = MacHost_->Password_;
    }
    else if(VistaHost_)
    {
        Username_ = VistaHost_->Username_;
        Password_ = VistaHost_->Password_;
    }
    ShowShares_ = &Host->ShowShares_;

    Set();
}

cHostAuthenticationMenu::~cHostAuthenticationMenu()
{
}

void cHostAuthenticationMenu::Set()
{
    SetCols(18);
    Add(new cOsdItem(tr("Please enter Username & Password,"), osUnknown, false));
    Add(new cOsdItem(tr("which is needed for browsing the shares."), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cMenuEditStrItem(tr("Username"), Username_, USERNAME_LENGTH, tr(allowed_chars)));
    Add(new cMenuEditStrItem(tr("Password"), Password_, PASSWORD_LENGTH, tr(allowed_chars)));
    Display();
}

eOSState cHostAuthenticationMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(state == osUnknown)
    {
        if(Key == kOk)
        {
            if(strlen(Username_) && strlen(Password_))
            {
                *ShowShares_ = true;
                ScanShares();
                return osUser1; // Close this & Refresh parent Menu
            }
            else
            {
                Skins.Message(mtInfo, tr("Please enter username & password!"));
                return osContinue;
            }
        }
    }
    return state;
}

void cHostAuthenticationMenu::ScanShares()
{
    Skins.Message(mtStatus, tr("Please wait..."));
    bool successful_ = false;
    FILE *process;
    char *buffer;
    char strBuff[256];
    char *trash = (char*) malloc(256 * sizeof(char));
    char *sharename = (char*) malloc(256 * sizeof(char));

    sprintf(strBuff, "%s --status -u %s -p %s -h %s", COMMAND_SHOWSHARES, Username_, Password_, Address_);
    process = popen(strBuff, "r");
    if(process)
    {
        cReadLine readline;

        buffer = readline.Read(process);
        while(buffer)
        {
            //printf("\033[0;93m %s(%i) %s \033[0m\n", __FILE__, __LINE__, buffer);
            if(strstr(buffer, "[PROGRESS]\t") == buffer)
            {
                sprintf(strBuff, "%s %s", tr("Scanning..."), buffer+11);
                Skins.Message(mtStatus, strBuff);
            }
            else if(strstr(buffer, "[FOUND]\t") == buffer)
            {
                int sscanfSuccess = 0;
                if(sscanf(buffer, "%s\t%s\t%s\t%s", trash, trash, trash, sharename) == 4)
                    sscanfSuccess = 1;
                //printf("\033[0;93m %s(%i) %s \033[0m\n", __FILE__, __LINE__, sharename);

                if(sscanfSuccess && sharename)
                {
                    //if(MacHost_)
                        //printf("\033[0;94m %s(%i) %s \033[0m\n", __FILE__, __LINE__, sharename);
                    if(MacHost_)
                        MacHost_->AddShareAFP(sharename);
                    if(VistaHost_)
                        VistaHost_->AddShareSMB(sharename);
                }
            }
            else if(!strcmp(buffer, "completed"))
                successful_ = true;

            buffer = readline.Read(process);
        }
        pclose(process);
    }
    if(successful_)
        Skins.Message(mtInfo, tr("Scanning complete."));
    else
        Skins.Message(mtError, tr("Scanning failed!"));
    free(trash);
    free(sharename);
}

