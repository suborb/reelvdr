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
 * networkfilesystem.cpp
 *
 ***************************************************************************/


#include "networkfilesystem.h"

#include "vdr/tools.h"
#include "vdr/interface.h"
#include <vdr/sysconfig_vdr.h>

#include<algorithm>

#define CHANGE_RECORDING_MEDIA trNOOP("Do you want store recordings here?")

//###########################################################################
//  cNetworkFilesystem
//###########################################################################
cNetworkFilesystem::cNetworkFilesystem(const char *Host, const char *Sharename)
{
    char *strBuff;
    Host_ = strdup(Host);
    Sharename_ = strdup(Sharename);
    Number_ = -1;

    if(strchr(Sharename, '/') == Sharename)
        strBuff = strdup(Sharename+1); // skip the first char if it begins with '/'
    else
        strBuff = strdup(Sharename);

    // Replace every '/' and ' ' by '_'
    while(strchr(strBuff, '/'))
        *(strchr(strBuff, '/')) = '_';
    while(strchr(strBuff, ' '))
        *(strchr(strBuff, ' ')) = '_';

    Displayname_ = (char*)malloc(DISPLAYNAME_LENGTH*sizeof(char));
    strncpy(Displayname_ , strBuff, DISPLAYNAME_LENGTH);
    free(strBuff);

    Pictures_ = 1; // 0=no; 1=yes
    Music_ = 1; // 0=no; 1=yes
    Recordings_ = 1; // 0=no; 1=yes
    Videos_ = 1; // 0=no; 1=yes
    StartClient_ = 1; // 0=no; 1=yes
}

cNetworkFilesystem::~cNetworkFilesystem()
{
    if(Host_)
        free(Host_);
    if(Sharename_)
        free(Sharename_);
    if(Displayname_)
        free(Displayname_);
}

void cNetworkFilesystem::GetFreeNumber()
{
    std::vector<int> usedNumbers;
    int freeNumber = 0;
#if RBMINI
    freeNumber = 1; // #0 is reserved for NetClient-AVG-Mount
#endif
    char *buffer;
    FILE *process;

    process = popen("grep HOST /etc/default/sysconfig |grep ^MOUNT| sed -e 's/MOUNT//' | awk -F'_' '{print $1 }'", "r");

    if(process)
    {
        cReadLine readline;
        buffer = readline.Read(process);
        while(buffer)
        {
            usedNumbers.push_back(atoi(buffer));
            buffer = readline.Read(process);
        }
        pclose(process);
    }

    sort( usedNumbers.begin(), usedNumbers.end());

    // check for gaps
    for(unsigned int i = 0; i < usedNumbers.size(); ++i)
        if (freeNumber == usedNumbers.at(i) )
            ++freeNumber;
        else
            Number_ = freeNumber;


    Number_ = freeNumber;
}

bool cNetworkFilesystem::MountActive()
{
    bool retvalue = false;
    char *strBuff;
    FILE *process;
    cReadLine readline;

    asprintf(&strBuff, "cat /proc/mounts | grep \"/media/.mount%i \"", Number_);
    process = popen(strBuff, "r");
    free(strBuff);

    if(process)
    {
        strBuff = readline.Read(process);
        if(strBuff)
            retvalue = true;
        pclose(process);
    }

    return retvalue;
}

void cNetworkFilesystem::SetRecordingMedia()
{
    const int size = 64;
    char strBuff[size];
    const char *mountpoints = "/media/.mount";
    char *CurrentRecDevice=NULL;
    int CurrentRecDeviceNumber = -1;

    if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
    {
        CurrentRecDevice = strdup(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"));
        if(strstr(CurrentRecDevice, mountpoints))
            CurrentRecDeviceNumber = atoi(CurrentRecDevice+strlen(mountpoints));
    }
    if(CurrentRecDevice)
        free(CurrentRecDevice);

    if((Number_ != CurrentRecDeviceNumber) && Interface->Confirm(tr(CHANGE_RECORDING_MEDIA), 20))
    {
        snprintf(strBuff, size, "%s%i", mountpoints, Number_);
        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", strBuff );
        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE_ONLY_RECORDING", "yes");
        cSysConfig_vdr::GetInstance().Save();
        SystemExec("setup-mediadir");

#if USE_LIVEBUFFER
        if(Setup.LiveBuffer)
        {
            Skins.Message(mtInfo, tr("Permament timeshift disabled!"), 2);
            Setup.LiveBuffer = 0;
            Setup.Save();
        }
#endif
        Skins.Message(mtInfo, tr("Settings stored"), 2);
    }
}

//###########################################################################
//  cNetworkFSSMB
//###########################################################################
cNetworkFSSMB::cNetworkFSSMB(const char *Host, const char *Sharename) : cNetworkFilesystem(Host, Sharename)
{
    Username_ = (char*)malloc(USERNAME_LENGTH*sizeof(char));
    Username_[0] = '\0';
    Password_ = (char*)malloc(PASSWORD_LENGTH*sizeof(char));
    Password_[0] = '\0';
}

cNetworkFSSMB::cNetworkFSSMB(const cNetworkFSSMB &NetworkFSSMB) : cNetworkFilesystem(NetworkFSSMB.GetHost(), NetworkFSSMB.GetSharename())
{
    Number_ = NetworkFSSMB.Number_;
    Displayname_ = strdup(NetworkFSSMB.Displayname_);
    Username_ = strdup(NetworkFSSMB.Username_);
    Password_ = strdup(NetworkFSSMB.Password_);
    Pictures_ = NetworkFSSMB.Pictures_;
    Music_ = NetworkFSSMB.Music_;
    Recordings_ = NetworkFSSMB.Recordings_;
    Videos_ = NetworkFSSMB.Videos_;
    StartClient_ = NetworkFSSMB.StartClient_;
}

cNetworkFSSMB::~cNetworkFSSMB()
{
    if(Username_)
        free(Username_);
    if(Password_)
        free(Password_);
}

//###########################################################################
//  cNetworkFSNFS
//###########################################################################

cNetworkFSNFS::cNetworkFSNFS(const char *Host, const char *Sharename) : cNetworkFilesystem(Host, Sharename)
{
    NetworkProtocol_ = 1; // 0 = "tcp"
    BlockSize_ = 2; // 2 = "8192"
    OptionHS_ = 0; // 0=soft; 1=hard
    OptionLock_ = 1; // 0=no; 1=yes
    OptionRW_ = 1; // 0=no; 1=yes
    NFSVersion_ = 3;
}

cNetworkFSNFS::cNetworkFSNFS(const cNetworkFSNFS &NetworkFSNFS) : cNetworkFilesystem(NetworkFSNFS.GetHost(), NetworkFSNFS.GetSharename())
{
    Number_ = NetworkFSNFS.Number_;
    Displayname_ = strdup(NetworkFSNFS.Displayname_);
    Pictures_ = NetworkFSNFS.Pictures_;
    Music_ = NetworkFSNFS.Music_;
    Recordings_ = NetworkFSNFS.Recordings_;
    Videos_ = NetworkFSNFS.Videos_;
    StartClient_ = NetworkFSNFS.StartClient_;
    NetworkProtocol_ = NetworkFSNFS.NetworkProtocol_;
    BlockSize_ = NetworkFSNFS.BlockSize_;
    OptionHS_ = NetworkFSNFS.OptionHS_;
    OptionLock_ = NetworkFSNFS.OptionLock_;
    OptionRW_ = NetworkFSNFS.OptionRW_;
    NFSVersion_ = NetworkFSNFS.NFSVersion_;
}

cNetworkFSNFS::~cNetworkFSNFS()
{
}

//###########################################################################
//  cNetworkFSAFP
//###########################################################################
cNetworkFSAFP::cNetworkFSAFP(const char *Host, const char *Sharename) : cNetworkFilesystem(Host, Sharename)
{
    Username_ = (char*)malloc(USERNAME_LENGTH*sizeof(char));
    Username_[0] = '\0';
    Password_ = (char*)malloc(PASSWORD_LENGTH*sizeof(char));
    Password_[0] = '\0';
}

cNetworkFSAFP::cNetworkFSAFP(const cNetworkFSAFP &NetworkFSAFP) : cNetworkFilesystem(NetworkFSAFP.GetHost(), NetworkFSAFP.GetSharename())
{
    Number_ = NetworkFSAFP.Number_;
    Displayname_ = strdup(NetworkFSAFP.Displayname_);
    Pictures_ = NetworkFSAFP.Pictures_;
    Music_ = NetworkFSAFP.Music_;
    Recordings_ = NetworkFSAFP.Recordings_;
    Videos_ = NetworkFSAFP.Videos_;
    StartClient_ = NetworkFSAFP.StartClient_;
}

cNetworkFSAFP::~cNetworkFSAFP()
{
    if(Username_)
        free(Username_);
    if(Password_)
        free(Password_);
}

//###########################################################################
//  cNetworkFSHost
//###########################################################################
cNetworkFSHost::cNetworkFSHost(const char *Host)
{
    Host_ = strdup(Host);
    ShowShares_ = false;
    printf("\033[0;93m %s(%i): %s\033[0m\n", __FILE__, __LINE__,  Host);
}

cNetworkFSHost::~cNetworkFSHost()
{
    if(Host_)
        free(Host_);
    for(unsigned int i = 0; i < vNetworkFSSMB_.size(); ++i)
        delete vNetworkFSSMB_.at(i);
    vNetworkFSSMB_.clear();
    for(unsigned int i = 0; i < vNetworkFSNFS_.size(); ++i)
        delete vNetworkFSNFS_.at(i);
    vNetworkFSNFS_.clear();
}

void cNetworkFSHost::AddShareSMB(const char *Sharename)
{
    vNetworkFSSMB_.push_back(new cNetworkFSSMB(Host_, Sharename));
}

void cNetworkFSHost::AddShareNFS(const char *Sharename)
{
    vNetworkFSNFS_.push_back(new cNetworkFSNFS(Host_, Sharename));
}

//###########################################################################
//  cNetworkFSMacHost
//###########################################################################
cNetworkFSMacHost::cNetworkFSMacHost(const char *Host) : cNetworkFSHost(Host)
{
    Username_ = (char*)malloc(USERNAME_LENGTH*sizeof(char));
    Username_[0] = '\0';
    Password_ = (char*)malloc(PASSWORD_LENGTH*sizeof(char));
    Password_[0] = '\0';
}

cNetworkFSMacHost::~cNetworkFSMacHost()
{
    if(Username_)
        free(Username_);
    if(Password_)
        free(Password_);
}

void cNetworkFSMacHost::AddShareAFP(const char *Sharename)
{
    vNetworkFSAFP_.push_back(new cNetworkFSAFP(Host_, Sharename));
}

//###########################################################################
//  cNetworkFSVistaHost
//###########################################################################
cNetworkFSVistaHost::cNetworkFSVistaHost(const char *Host) : cNetworkFSHost(Host)
{
    Username_ = (char*)malloc(USERNAME_LENGTH*sizeof(char));
    Username_[0] = '\0';
    Password_ = (char*)malloc(PASSWORD_LENGTH*sizeof(char));
    Password_[0] = '\0';
}

cNetworkFSVistaHost::~cNetworkFSVistaHost()
{
    if(Username_)
        free(Username_);
    if(Password_)
        free(Password_);
}

