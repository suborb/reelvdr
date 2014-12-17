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
 * install_harddisk.c
 *
 ***************************************************************************/

#include "install_harddisk.h"
#include <vdr/sysconfig_vdr.h>

cInstallHarddisk::cInstallHarddisk(bool HarddiskAvailable)
{
    HarddiskAvailable_ = HarddiskAvailable;
    UUID_ = "";
    Set();
}

cInstallHarddisk::~cInstallHarddisk()
{
}

void cInstallHarddisk::Set()
{
    char title[64];
    if (HarddiskAvailable_)
    {
        snprintf(title, 64, "%s", tr("Harddisk"));
        Add(new cOsdItem("Harddisk found", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Please select the target for your recordings:"), osUnknown, false));
        Add(new cOsdItem(tr(" I want to store my recordings ..."), osUnknown, false));
        Add(new cOsdItem(tr(" - on the local harddisk of this NetClient."), osUser1, true));    // osUser1 = use local Harddisk for recording
        Add(new cOsdItem(tr(" - on the harddisk of the ReelBox Avantgarde."), osUser2, true));  // osUser2 = use Avantgarde for recording
    }
    else
    {
        snprintf(title, 64, "%s", tr("Error"));
        Add(new cOsdItem(tr("No Harddisk found."), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Recording is not possible."), osUnknown, false));
        cInstallMenu::SetErrorState(1); // No Harddisk
    }
    SetTitle(title);
    Display();
}

eOSState cInstallHarddisk::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (state)
    {
    case osUnknown:
        if (Key == kOk)
        {
            StorageAVG_ = false;
            Save();
            return osUser1;     // Next Menu
        }
        break;
    case osUser1:
        StorageAVG_ = false;
        Save();
        return osUser1;         // Next Menu
        break;
    case osUser2:
        StorageAVG_ = true;
        Save();
        return osUser1;         // Next Menu
        break;
    default:
        break;
    }

    return state;
}

bool cInstallHarddisk::Save()
{
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
    bool save = true;
    // Setup Recording Media
    if (StorageAVG_)
    {
        char strBuff[512];
        int Number = GetFreeMountNumber();

        if (Number != -1)
        {
            // Store NFS-Shares
            sprintf(strBuff, "MOUNT%i_NFS_STARTCLIENT", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_HOST", Number);
            sysconfig.SetVariable(strBuff, Setup.NetServerIP);
            sprintf(strBuff, "MOUNT%i_NFS_SHARENAME", Number);
            sysconfig.SetVariable(strBuff, "/media/hd");
            sprintf(strBuff, "MOUNT%i_NFS_DISPLAYNAME", Number);
            sysconfig.SetVariable(strBuff, "Avantgarde");
            sprintf(strBuff, "MOUNT%i_NFS_PICTURES", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_MUSIC", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_RECORDINGS", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_VIDEOS", Number);
            sysconfig.SetVariable(strBuff, "yes");

            // NFS-specified options
            sprintf(strBuff, "MOUNT%i_NFS_NETWORKPROTOCOL", Number);
            sysconfig.SetVariable(strBuff, "tcp");
            sprintf(strBuff, "MOUNT%i_NFS_BLOCKSIZE", Number);
            sysconfig.SetVariable(strBuff, "8192");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONHS", Number);
            sysconfig.SetVariable(strBuff, "soft");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONLOCK", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONRW", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_NFSVERSION", Number);
            sysconfig.SetVariable(strBuff, "3");

            // Use NFS-Mount as MediaDevice
            sprintf(strBuff, "/media/.mount%i", Number);
            sysconfig.SetVariable("MEDIADEVICE", strBuff);
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

            sysconfig.Save();

            sprintf(strBuff, "/usr/sbin/setup-shares start %i", Number);
            SystemExec(strBuff);
        }

        // Use NFS-Mount as MediaDevice
        sprintf(strBuff, "/media/.mount%i", Number);
        sysconfig.SetVariable("MEDIADEVICE", strBuff);
        sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

        sysconfig.Save();
    }
    else
    {
        if (!HarddiskAvailable_)
        {
            // No harddisk
            sysconfig.SetVariable("MEDIADEVICE", "");
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "yes");
        }
        else
        {
            UUID_ = GetUUID();
            if (UUID_.size())
            {
                // Use local harddisk
                sysconfig.SetVariable("MEDIADEVICE", UUID_.c_str());
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            }
            else
            {
                Skins.Message(mtError, "Store failed! (missing UUID)");
                save = false;
            }
        }
    }

    if (save)
        sysconfig.Save();

    //TODO: return save;
    return true;
}
