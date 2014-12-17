/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                                                    Tobias Bratfisch     *
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
 * setuprecordingdevicemenu.cpp
 *
 ***************************************************************************/

#include <string>
#include <algorithm>

#include <vdr/debug.h>
#include <vdr/videodir.h>
#include <vdr/interface.h>
#include <vdr/recording.h>
#if VDRVERSNUM >= 10716
#include <sstream>
#endif

#include <vdr/sysconfig_vdr.h>
#include "setuprecordingdevicemenu.h"
#include "formatmenu.h"

using std::string;

#define FORMAT_CMD "preparehd.sh"

//#################################################################################################
//  cSetupRecordingDeviceMenu
//################################################################################################

// Blacklist partitions with following mountpoints, which shouldn't used for recording device
const char *BlacklistPartition[] = { "/", "/var", "/usr" };

cSetupRecordingDeviceMenu::cSetupRecordingDeviceMenu(const char *title) : cOsdMenu(title)
{
    LiveBuffer_ = false;
    InitMenu_ = true;
    DisableMultiRoom_ = false;

    GetDiskInformations();

    //load settings from sysconfig
    if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE"))
    {
        CurrentDevice_ = cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE");
        if(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE_ONLY_RECORDING") && strcmp(cSysConfig_vdr::GetInstance().GetVariable("MEDIADEVICE_ONLY_RECORDING"), "yes") == 0)
        {
            bool found = false;
            for(std::vector<cBlockDevice*>::iterator devIter = blockdevice_.begin(); devIter != blockdevice_.end(); ++devIter)
                for(std::vector<cPartition*>::iterator partIter = (*devIter)->GetPartitions()->begin(); partIter != (*devIter)->GetPartitions()->end(); ++partIter)
                    if(((*partIter)->GetUUID()) && (CurrentDevice_.find((*partIter)->GetUUID()) != string::npos))
                    {
                        found = true;
                        (*partIter)->SetOnlyRecording(true);
                    }
            if(!found)
                for(std::vector<cRemoteBlockDevice*>::iterator devIter = remoteblockdevice_.begin(); devIter != remoteblockdevice_.end(); ++devIter)
                    if(CurrentDevice_ == (*devIter)->GetMountpoint())
                        (*devIter)->SetOnlyRecording(true);
        }
    }

    Display(); // Display here since we need to update MaxItems
    Set();
    InitMenu_ = false;
}

cSetupRecordingDeviceMenu::~cSetupRecordingDeviceMenu()
{
    ClearDiskInformations();
}

void cSetupRecordingDeviceMenu::GetDiskInformations()
{
    FILE *process = NULL;
    DIR *dir = NULL;
    struct dirent *dp = NULL;
    std::vector<char> letters;
    char *strBuff = NULL;
    char service[256];
    char mountpoint[256];
    char protocol[265];

    /** collect local devices */
    dir = opendir("/dev/");
    if(dir) {
        do {
            errno = 0;
            if ((dp = readdir(dir)) != NULL) {
                if ((strncmp(dp->d_name, "sd", 2) != 0) || (strlen(dp->d_name) != 3))
                    continue;
                letters.push_back(*(dp->d_name + 2));
            }
        } while (dp != NULL);
    }
    closedir(dir);
    sort(letters.begin(), letters.end());

    // Get informations of found local devices
    for(unsigned int i = 0; i < letters.size(); ++i)
    {
        cBlockDevice *device = new cBlockDevice(letters.at(i));
        if(device->GetSize() > 0)
            blockdevice_.push_back(device);
    }

    //getting infos about remote devices
    process = popen("grep \"nfs\\|smb\\|cifs\" /proc/mounts", "r");
    if (process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        while(strBuff)
        {
            if (sscanf(strBuff, "%s %s %s", service, mountpoint, protocol) == 3)
                if((strcmp(protocol, "nfs") == 0) || (strcmp(protocol, "smb") == 0) || (strcmp(protocol, "cifs") == 0))
                    remoteblockdevice_.push_back(new cRemoteBlockDevice(service, mountpoint, protocol));

            strBuff = readline.Read(process);
        }
    }
    pclose(process);
}

void cSetupRecordingDeviceMenu::ClearDiskInformations()
{
    for(std::vector<cBlockDevice*>::iterator devIter = blockdevice_.begin(); devIter != blockdevice_.end(); ++devIter)
        delete (*devIter);
    blockdevice_.clear();
    for(std::vector<cRemoteBlockDevice*>::iterator devIter = remoteblockdevice_.begin(); devIter != remoteblockdevice_.end(); ++devIter)
        delete (*devIter);
    remoteblockdevice_.clear();
}

#define TINY_PARTITION (100*1024)
#define SMALLEST_MEDIA (4*1024*1024)

void cSetupRecordingDeviceMenu::Set()
{
    char buffer[1024];
    char *freespace = NULL;
    bool isCurrentDevice = false;
    bool modified = false;
#ifdef RBMINI
    bool firstDevice = true;
#endif
    bool hide = false;

    int current = Current();
    Clear();

    SetCols(18, 6, -40, 9, 9);

    Add(new cOsdItem(tr("Please select the device for storing your media files:"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Drive\tDevice\t\tSize\tFree\tType"), osUnknown, false));

    // local devices
    for(std::vector<cBlockDevice*>::iterator devIter = blockdevice_.begin(); devIter != blockdevice_.end(); ++devIter)
    {
        hide = false;
        bool firstpartition = true;

        if((*devIter)->GetSize() > 0 ) //SMALLEST_MEDIA)
        {
            if ((*devIter)->GetSize() > (1024*1024*1024) )  //>1TB
                snprintf(buffer, 1024, "%s\t\t\t%.2fTiB",
                         (*devIter)->GetModel(),
                         (float)((*devIter)->GetSize()) / (1024*1024*1024)
                        );
            else
                snprintf(buffer, 1024, "%s\t\t\t%.2fGiB",
                         (*devIter)->GetModel(),
                         (float)((*devIter)->GetSize()) / (1024*1024)
                        );
#if 0//def RBMINI
            if(!firstDevice)
#endif
            /// add base device
            if (!(*devIter)->IsRootDevice() || ( (*devIter)->GetSize() > SMALLEST_MEDIA ) )
            {
                //DDD("Adding device sd%c, IsInternal: %d", (*devIter)->GetLetter(),
                //					  (*devIter)->IsRootDevice());
                if ((*devIter)->GetPartitions()->size())
                    Add(new cOsdItem(buffer, osUnknown, false));
                else {
                    bool Resize = false;
                    Add(new cBlockDeviceItem(buffer, new cPartition((*devIter)->GetLetter(), 999), (*devIter)->GetLetter(), &CurrentDevice_, NULL, (*devIter)->Removable(), Resize, LiveBuffer_, (*devIter)->IsRootDevice()));
                }

            }
            else
            {
                hide = true;
                //DDD("Hiding device sd%c, size: %lld", (*devIter)->GetLetter(), (*devIter)->GetSize());
            }
#ifdef RBMINI
            firstDevice = false;
#endif
            /// add partitions
            for(std::vector<cPartition*>::iterator partIter = (*devIter)->GetPartitions()->begin(); partIter != (*devIter)->GetPartitions()->end(); ++partIter)
            {
                // printf("GetMountPoint: '%s'\n", (*partIter)->GetMountpoint());
                if(!(((*partIter)->GetMountpoint()) && (strcmp((*partIter)->GetMountpoint(), "SWAP") == 0)))
                {
                    bool blacklist = false;
                    //bool firstpartition = true;
//#ifndef RBMINI
                    for(unsigned int i = 0; i < sizeof(BlacklistPartition)/sizeof(char*); ++i)
                        if(((*partIter)->GetMountpoint()) && (strcmp((*partIter)->GetMountpoint(), BlacklistPartition[i]) == 0))
                            blacklist = true;
//#endif
                    if (!blacklist && ((*partIter)->GetSize()>TINY_PARTITION) /* && ! ((*devIter)->IsRootDevice()) */ )
                        // No blacklisted or small partitions! Hide internal (root) USB stick
                    {
                        /*DDD("Adding partition sd%c%c, IsInternal: %d", (*devIter)->GetLetter(),
                                                                    (*partIter)->GetNumber(),
                                                                    (*devIter)->IsRootDevice());
                        */
                        if(((*partIter)->GetUUID()) && (CurrentDevice_.find((*partIter)->GetUUID()) != string::npos))
                        {
                            isCurrentDevice = true;
                            if(Count() != current)
                                modified = true;
                            if((*devIter)->Removable())
                                LiveBuffer_ = false;
                            else
                                LiveBuffer_ = true;
                        }
                        else
                            isCurrentDevice = false;

                        if((*partIter)->GetFreeSpace() == -1 || (*partIter)->GetUsedSpace() == -1)
                            freespace = strdup("");
                        else
                        {
                            if ((*partIter)->GetFreeSpace() > (1024*1024*1024) )  //TB
                                asprintf(&freespace, "%.2fTiB", (float)((*partIter)->GetFreeSpace())/(1024*1024*1024));
                            else
                                asprintf(&freespace, "%.2fGiB", (float)((*partIter)->GetFreeSpace())/(1024*1024));
                        }

                        bool DisplayMediaType = false;
                        if(InitMenu_)
                        {
                            if(isCurrentDevice)
                            {
                                current = Count();
                                DisplayMediaType = true;
                            }
                        }
                        else
                        {
                            if(Count() == current)
                                DisplayMediaType = true;
                        }

                        std::string displayMediaType;
                        if (DisplayMediaType) {
                            if ((*partIter)->GetOnlyRecording())
                                displayMediaType = tr("Recording");
                            else
                                displayMediaType = tr("All media");
                        }

                        // if this device is removable, on the beginning of the first partition,
                        // the word "(removeable)" is shown
                        std::string attribute;
                        if ( firstpartition && (*devIter)->Removable())
                        {
                            attribute = std::string(tr("(removable)"));
                        }
                        else
                            attribute = "";

                        if ((*partIter)->GetSize() > (1024*1024*1024) )  //>1TB
                            snprintf(buffer, 1024, "%s\tsd%c%i\t%c%c\t%.2fTiB\t%s\t%s",
                                     attribute.c_str(),
                                     (*devIter)->GetLetter(),
                                     (*partIter)->GetNumber(),
                                     isCurrentDevice ? '#' : ' ',
                                     (*partIter)->GetMountpoint()?'>':' ',
                                     (float)((*partIter)->GetSize())/(1024*1024*1024),
                                     freespace,
                                     displayMediaType.c_str()
                                    );
                        else
                            snprintf(buffer, 1024, "%s\tsd%c%i\t%c%c\t%.2fGiB\t%s\t%s",
                                     attribute.c_str(),
                                     (*devIter)->GetLetter(),
                                     (*partIter)->GetNumber(),
                                     isCurrentDevice ? '#' : ' ',
                                     (*partIter)->GetMountpoint()?'>':' ',
                                     (float)((*partIter)->GetSize())/(1024*1024),
                                     freespace,
                                     displayMediaType.c_str()
                                    );
                        /*} else
                            snprintf(buffer, 1024, "\tsd%c%i\t%c%c\t%.2fGiB\t%s\t%s",
                                     (*devIter)->GetLetter(),
                                     (*partIter)->GetNumber(),
                                     isCurrentDevice ? '#' : ' ', (*partIter)->GetMountpoint()?'>':' ',

                                    (float)((*partIter)->GetSize())/(1024*1024),
                                    freespace,
                                    displayMediaType.c_str()
                                    );
                        */
                        free(freespace);
#ifdef RBMINI
                        if((*partIter)->GetUUID() && !(*devIter)->IsRootDevice())
#else
                        if((*partIter)->GetUUID())
#endif
                        {
                            bool Resize = false;
                            if((*partIter)->Resizable() && (*partIter)->GetMountpoint())
#ifdef RBMINI
                                if ((*devIter)->IsRootDevice() == false)
#endif
                                    Resize = true;
#ifdef RBMINI
                            Add(new cBlockDeviceItem(buffer, *partIter, (*devIter)->GetLetter(), &CurrentDevice_, &((*partIter)->OnlyRecording_), (*devIter)->Removable(), Resize, LiveBuffer_, (*devIter)->IsRootDevice()));
#else
                            Add(new cBlockDeviceItem(buffer, *partIter, (*devIter)->GetLetter(), &CurrentDevice_, &((*partIter)->OnlyRecording_), (*devIter)->Removable(), Resize, LiveBuffer_));
#endif
                        }
                        else {
                            Add(new cOsdItem(buffer, osUnknown, false));
                        }
                        firstpartition = false;
                    }
                }
            }
        }

        if (!hide)  // avoid double blank-lines if the device was hidden
            Add(new cOsdItem("", osUnknown, false));
    }

    // remote devices
    for(std::vector<cRemoteBlockDevice*>::iterator devIter=remoteblockdevice_.begin(); devIter!=remoteblockdevice_.end(); ++devIter)
    {
        if(CurrentDevice_ == (*devIter)->GetMountpoint())
        {
            isCurrentDevice = true;
            if(Count() != current)
                modified = true;
            LiveBuffer_ = false;
        }
        else
            isCurrentDevice = false;

        const char *service = (*devIter)->GetService();
        bool isMultiRoom = false;

        /* Replace IP of remote Avantgarde with "MultiRoom"
         * only for NetClient and if NetClient is in "Multiroom mode" */

        if (::Setup.ReelboxMode == eModeClient && ::Setup.NetServerIP)
            isMultiRoom = (strstr(service, ::Setup.NetServerIP) == service);

        bool DisplayMediaType = false;
        if(InitMenu_)
        {
            if(isCurrentDevice)
            {
                current = Count();
                DisplayMediaType = true;
            }
        }
        else
        {
            if(Count() == current)
                DisplayMediaType = true;
        }

        std::string displayMediaType;
        if (DisplayMediaType) {
            if ((*devIter)->GetOnlyRecording())
                displayMediaType = tr("Recording");
            else
                displayMediaType = tr("All media");
        }

        std::string displayName;
        if (isMultiRoom)
            displayName = tr("MultiRoom Server");
        else
            displayName = std::string((*devIter)->GetProtocol()) + ":" + std::string(service);

        if ((*devIter)->GetSize() > (1024*1024*1024) )  //>1TB
        {
            snprintf(buffer, 1024, "%s\t\t%c%c\t%.2fTiB\t%.2fTiB\t%s",
                     displayName.c_str(),
                     isCurrentDevice ? '#' : ' ',
                     (*devIter)->GetMountpoint() ? '>' : ' ',
                     (float)((*devIter)->GetSize()) / (1024*1024*1024),
                     (float)((*devIter)->GetFreeSpace()) / (1024*1024*1024),
                     displayMediaType.c_str()
                    );
        }
        else
            snprintf(buffer, 1024, "%s\t\t%c%c\t%.2fGiB\t%.2fGiB\t%s",
                     displayName.c_str(),
                     isCurrentDevice ? '#' : ' ',
                     (*devIter)->GetMountpoint() ? '>' : ' ',
                     (float)((*devIter)->GetSize()) / (1024*1024),
                     (float)((*devIter)->GetFreeSpace()) / (1024*1024),
                     displayMediaType.c_str()
                    );
        /*
        {
            snprintf(buffer, 1024, "%s:%s\t\t%c%c\t%.2fGiB\t%.2fGiB\t%s",
                    (*devIter)->GetProtocol(),
                    service,
                    isCurrentDevice ? '#' : ' ', (*devIter)->GetMountpoint()?'>':' ',
                    (float)((*devIter)->GetSize())/(1024*1024),
                    (float)((*devIter)->GetFreeSpace())/(1024*1024),
                    displayMediaType.c_str()
                    );
        }
        */
        Add(new cRemoteBlockDeviceItem(buffer, (*devIter)->GetMountpoint(), &CurrentDevice_, &((*devIter)->OnlyRecording_), (*devIter)->GetSize(), (*devIter)->GetFreeSpace()));
        if(isMultiRoom)
        {
            snprintf(buffer, 1024, "(%s)", ::Setup.NetServerIP);
            Add(new cOsdItem(buffer, osUnknown, false));
        }
        Add(new cOsdItem("", osUnknown, false));
    }

    while(DisplayMenu()->MaxItems() - Count() > 3)
        Add(new cOsdItem("", osUnknown, false));


    if(::Setup.ReelboxMode == eModeClient)
    {
        if(modified && !InitMenu_)
        {
            Add(new cOsdItem(tr("Note: MultiRoom will be disabled."), osUnknown, false));
            DisableMultiRoom_ = true;
        }
        else
        {
            Add(new cOsdItem("", osUnknown, false));
            DisableMultiRoom_ = false;
        }
    }

    Add(new cOsdItem("", osUnknown, false));
#if USE_LIVEBUFFER
    snprintf(buffer, 1024, "%s: %s", tr("Permanent Timeshift"), ::Setup.LiveBuffer ? tr("enabled") : tr("disabled"));
    Add(new cOsdItem(buffer, osUnknown, false));
#endif

    SetCurrent(Get(current));
    Display();
    SetHelpKeys();
}

void cSetupRecordingDeviceMenu::SetHelpKeys()
{
    cBlockDeviceItem *blockdeviceitem = dynamic_cast<cBlockDeviceItem*>(Get(Current()));
    bool Resizeable = false;
    const char *Red = NULL;
    const char *Green = NULL;
    const char *Blue = NULL;
    if(blockdeviceitem)
    {
        Resizeable = blockdeviceitem->Resizeable();

        if (blockdeviceitem->Formatable())
            Red = tr("Format");
#if 1  // WIP
        //TODO: test if device is mounted or not
        if (blockdeviceitem->MountActive(blockdeviceitem->GetDevicename().c_str()))
            Green = tr("Unmount");
        else
            Green = tr("Mount");
#endif
#if USE_LIVEBUFFER
        if (::Setup.LiveBuffer)
            Blue = tr("timeshift off");
        else
            Blue = tr("timeshift on");
#endif
    }

    if(LiveBuffer_)
        SetHelp(Red, Green, Resizeable ? tr("Resize partition") : NULL, Blue );
    else
        SetHelp(Red, Green, Resizeable ? tr("Resize partition") : NULL, NULL);
}

eOSState cSetupRecordingDeviceMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (!HasSubMenu())
    {
	    SetHelpKeys();
        switch (state)
        {
            case os_User:  // Redraw menu
                if(Key == kOk)
                {
//                    if(!Recordings.Load())
//                        Skins.Message(mtError, tr("Loading recordings failed!"), 2);

                    if(DisableMultiRoom_)
                        DisableMultiRoom();
                    Recordings.Update(false);

                    Skins.Message(mtInfo, tr("Settings stored"), 2);
//obsolete                    Skins.Message(mtInfo, tr("Please reboot ReelBox"), 2);
                }
                Set();
                return osContinue;
                break;
            case osUser1:  // Reboot
                SystemExec("reboot");
                Skins.Message(mtInfo, tr("Rebooting!"), 2);
                return osContinue;
                break;
            case osUnknown:
                if (Key == kRed)
                {
                    cBlockDeviceItem *blockdeviceitem = dynamic_cast<cBlockDeviceItem*>(Get(Current()));
                    if (blockdeviceitem && blockdeviceitem->Formatable())
                        return AddSubMenu(new cFormatMenu(blockdeviceitem->GetDevicename().c_str(), blockdeviceitem->Size(), blockdeviceitem->Free()));
                    state = osContinue;
                }
                else if (Key == kGreen)
                {
                    DDD("key: kGreen pressed");

                    std::string path = std::string("/usr/sbin/") + FORMAT_CMD;
                    if (access(path.c_str(), X_OK) != 0)
                    {
                        Skins.Message(mtError, tr("Unable to mount. Missing command"));
                        printf("Missing file '%s' or file not executeable\n", FORMAT_CMD);
                        esyslog("Missing file '%s' or file not executeable", FORMAT_CMD);

                        return osContinue;
                    }

                    cBlockDeviceItem *blockdeviceitem = dynamic_cast<cBlockDeviceItem*>(Get(Current()));
                    // format_device.sh -d /dev/sdxN ext3 /tmp/format_dev_sdxN.log
                    if (blockdeviceitem)
                    {
                        std::string format_cmd;
                        if (blockdeviceitem->MountActive(blockdeviceitem->GetDevicename().c_str()))
                        {
                            format_cmd = std::string(FORMAT_CMD)
                                + " --unmount -d " + blockdeviceitem->GetDevicename().c_str();
                            Skins.QueueMessage(mtInfo, tr("unmounting")); //TODO: show blockdeviceitem->GetModel()
                        }
                        else
                        {
                            format_cmd = std::string(FORMAT_CMD)
                                + " --mount -d " + blockdeviceitem->GetDevicename().c_str();
                            Skins.QueueMessage(mtInfo, tr("mounting"));
                        }
                        printf("\033[7;91m'%s'\033[0m\n", format_cmd.c_str());
                        SystemExec(format_cmd.c_str());

                        /* update OSD */
                        ClearDiskInformations();
                        GetDiskInformations();
                        Set();
                    }
                    state = osContinue;
                }
                else if (Key == kYellow)
                     state = osContinue;
                else if (Key == kBlue)
                {
#if USE_LIVEBUFFER
                    if(LiveBuffer_)
                    {
                        if(::Setup.LiveBuffer)
                            ::Setup.LiveBuffer = 0;
                        else
                            ::Setup.LiveBuffer = 1;
                        ::Setup.Save();
                    }
#endif
                    Set();
                    state = osContinue;
                }
                break;
            case osContinue:
                switch(Key)
                {
                    case kUp:
                    case kUp|k_Repeat:
                    case kDown:
                    case kDown|k_Repeat:
                        Set();
                        return osContinue;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    return state;
}

void cSetupRecordingDeviceMenu::DisableMultiRoom()
{
    bool found = false;

//    cSysConfig_vdr_vdr &sysconfig = cSysConfig_vdr_vdr::GetInstance();
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone; // 0=Standalone => use local Harddisk

    // Load Timers
    Timers.Load("/etc/vdr/timers.conf");

    sysconfig.Save();
    // Hint: do this *after* sysconfig.Save()!
    // Stop & Remove NetClient-AVG-Mount
    SystemExec("setup-shares stop 0");
    SystemExec("sed -i \"/^MOUNT0_NFS/d\" /etc/default/sysconfig");
    // TODO: If we use vdr-sysconfig, then do cSysConfig_vdr::Load() here!
    //VideoDirectory = strdup("/media/reel/recordings");

    // Remove remote device from list
    std::vector<cRemoteBlockDevice*>::iterator devIter = remoteblockdevice_.begin();
    while(!found && (devIter != remoteblockdevice_.end()))
    {
        const char *service = (*devIter)->GetService();
        if(strstr(service, ::Setup.NetServerIP) == service)
        {
            remoteblockdevice_.erase(devIter);
            found = true;
        }
        ++devIter;
    }

    Setup.Save();
}

// --- Class cBlockDeviceItem --------------------------------------------------------
cBlockDeviceItem::cBlockDeviceItem(const char *text, cPartition *Partition, char Letter,
                                   std::string *CurrentDevice, bool *OnlyRecording,
                                   bool Removable, bool Resize, bool LiveBuffer, bool onInternalUsbStick)
{
    SetText(text);
    Partition_ = Partition;
    Letter_ = Letter;
    onInternalUsbStick_ = onInternalUsbStick;
    CurrentDevice_ = CurrentDevice;
    OnlyRecording_ = OnlyRecording;
    size_ = Partition ? Partition->GetSize() / (1024*1024) : -1;
    free_ = Partition ? Partition->GetFreeSpace() / (1024*1024) : -1;
    Removable_ = Removable;
    Resize_ = Resize;
    LiveBuffer_ = LiveBuffer;
    //printf("text:'%s' letter:'%c' CurrDevice:'%s' onlyrec:%d \n", text, Letter, (CurrentDevice)->c_str(), *OnlyRecording);
}

cBlockDeviceItem::~cBlockDeviceItem()
{
}

bool cBlockDeviceItem::Formatable() const
{
    bool ret = false;
#ifndef RBMINI
    ret = Letter_ != 'a';
#else
    /** a HD without partitions should be "formatable" too (that means it will be partitioned and formatted) */
    ret = !onInternalUsbStick_ /*&& (Partition_ == NULL || (Partition_->GetSize() == 0 && Partition_->GetFreeSpace() == -1))*/; //TB: not only empty harddisks...
#endif
    return ret;
}

bool cBlockDeviceItem::MountActive(const char* device)
{
    bool retvalue = false;
    char *strBuff;
    FILE *process;
    cReadLine readline;

    asprintf(&strBuff, "cat /proc/mounts | grep \"%s \"", device);
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

eOSState cBlockDeviceItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kOk:
            if(cRecordControls::Active())
            {
                Skins.Message(mtWarning, tr("Warning, recording is running!"), 2);
                return osContinue;
            }
            else
            {
                Skins.Message(mtStatus, tr("Please wait..."));
                #define MAXBUF 100
                char buffer[MAXBUF];

                //neu:
                //mount the device, create fstab entry etc.
                DDD("preparehd.sh -d /dev/sd%c%d -m", Partition_->GetLetter(), Partition_->GetNumber());
                snprintf(buffer, MAXBUF, "preparehd.sh -d /dev/sd%c%d -m", Partition_->GetLetter(), Partition_->GetNumber());

                if (SystemExec(buffer) != 0)
                {
                    Skins.Message(mtError, tr("Mounting device failed"), 2);
                    return osContinue;
                }

                //create basic filesystem structure
                if (!*OnlyRecording_)
                {
                    DDD("preparehd.sh -d /dev/sd%c%d -b", Partition_->GetLetter(), Partition_->GetNumber());
                    snprintf(buffer, MAXBUF, "preparehd.sh -d /dev/sd%c%d -b", Partition_->GetLetter(), Partition_->GetNumber());
                    SystemExec(buffer);
                }

                // remove old links before changing MEDIADEVICE in sysconfig
                SystemExec("(link-shares stop& ); sleep 2");
                // Wait for it to finish reading the sysconfig file before changing it here
                // find a better way of handling it. link-shares takes too much time on NetClient.

                snprintf(buffer, MAXBUF, "UUID=%s", Partition_ ? Partition_->GetUUID() : "");
                cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", buffer);
                cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE_ONLY_RECORDING", *OnlyRecording_ ? "yes" : "no");
                cSysConfig_vdr::GetInstance().Save();

                Recordings.Clear();

                //create link (to network drives/shares) in the new mediadevice
                SystemExec("setup-mediadir");

                *CurrentDevice_ = Partition_ ? Partition_->GetUUID() : NULL;
                free_ = Partition_ ? Partition_->GetFreeSpace() / (1024*1024) : -1;
                DDD("size_: %llu free_: %llu Free: %d", size_, free_, Free());

                if(size_ < 5)
                    Skins.Message(mtWarning, tr("Warning, device may be to small for recordings!"), 2);
                else if(free_ < 5)
                    Skins.Message(mtWarning, tr("Warning, few space is left on this device!"), 2);

#if 0 //USE_LIVEBUFFER
                if((::Setup.LiveBuffer == 1) && Removable_)
                {
                    Skins.Message(mtInfo, tr("Permament timeshift disabled!"), 2);
                    ::Setup.LiveBuffer = 0;
                    ::Setup.Save();
                }
#endif

                return os_User; // Redraw Menu
            }
            break;
        case kLeft:
        case kRight:
            *OnlyRecording_ = !(*OnlyRecording_);
            return os_User;
            break;
        case kYellow:
            if(cRecordControls::Active())
            {
                Skins.Message(mtWarning, tr("Warning, recording is running!"), 2);
                return osContinue;
            }
            else
            {
                if(Resize_ && Partition_)
                {
                    if (Interface->Confirm(tr("Resize partition now?"), 30))
                    {
                        Skins.Message(mtInfo, tr("Resizing - please wait..."), 2);
                        bool updateuuid = false;
                        if((Partition_->GetUUID()) && (CurrentDevice_->find(Partition_->GetUUID()) != string::npos))
                            updateuuid = true;
                        switch(Partition_->GetFileSystem()) {
                            case eJFS:
                                if(Partition_->ResizeJFS() == 0) {
                                    if(updateuuid) {
                                        char buffer[256];
                                        snprintf(buffer, 256, "UUID=%s", Partition_->GetUUID());
                                        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", buffer);
                                        cSysConfig_vdr::GetInstance().Save();
                                    }
                                    if(Interface->Confirm(tr("Do you want to reboot the ReelBox now?"), 30))
                                        return osUser1; // Reboot
                                    else
                                        return osBack;
                                } else {
                                    Skins.Message(mtError, tr("Resizing failed!"), 2);
                                    return osContinue;
                                }
                                break;
                            case eEXT3:
                                if(Partition_->ResizeEXT3() == 0) {
                                    if(updateuuid) {
                                        char buffer[256];
                                        snprintf(buffer, 256, "UUID=%s", Partition_->GetUUID());
                                        cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", buffer);
                                        cSysConfig_vdr::GetInstance().Save();
                                    }
                                } else {
                                    Skins.Message(mtError, tr("Resizing failed!"), 2);
                                    return osContinue;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    else
                        return osContinue;
                }
                else
                    return osUnknown;
            }
            return osContinue; //kYellow
            break;
        default:
            return osUnknown;
    }
    return osUnknown;
}

std::string cBlockDeviceItem::GetDevicename()
{
    std::stringstream buffer;
    if (Partition_)
        buffer << "/dev/sd" << Letter_ << Partition_->GetNumber();
    else
        buffer << "/dev/sd" << Letter_;
    return buffer.str();
}
// --- Class cRemoteBlockDeviceItem --------------------------------------------------
cRemoteBlockDeviceItem::cRemoteBlockDeviceItem(const char *text, const char *mountpoint, std::string *CurrentDevice, bool *OnlyRecording, int size, int free)
{
    SetText(text);
    mountpoint_ = mountpoint;
    CurrentDevice_ = CurrentDevice;
    OnlyRecording_ = OnlyRecording;
    size_ = size / (1024*1024);
    free_ = free / (1024*1024);
//    printf("text:'%s' mp:'%s' CurrDevice:'%s' onlyrec:%d \n", text, mountpoint, (CurrentDevice)->c_str(), *OnlyRecording);

}

cRemoteBlockDeviceItem::~cRemoteBlockDeviceItem()
{
}

void cRemoteBlockDeviceItem::SaveSettings()
{
    Skins.Message(mtStatus, tr("Please wait..."));

    cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE", mountpoint_);
    cSysConfig_vdr::GetInstance().SetVariable("MEDIADEVICE_ONLY_RECORDING", *OnlyRecording_?"yes":"no");
    cSysConfig_vdr::GetInstance().Save();

    *CurrentDevice_ = mountpoint_;

    if(size_ < 5)
        Skins.Message(mtWarning, tr("Warning, device may be too small for recording!"), 2);
    else if(free_ < 5)
        Skins.Message(mtWarning, tr("Warning, few space is left on this device!"), 2);
/*
    if(*OnlyRecording_)
        VideoDirectory = strdup(mountpoint_);
    else
    {
        // If all Media is selected, then check if there is recordings-dir
        char path[512];
        snprintf(path, 512, "%s/%s", mountpoint_, "recordings");
        if(!access(path, F_OK))
            VideoDirectory = strdup(path);
    }
*/
    Recordings.Clear();

    SystemExec("setup-mediadir");

/*
    if(::Setup.LiveBuffer)
    {
        Skins.Message(mtInfo, tr("Permament timeshift disabled!"), 2);
        ::Setup.LiveBuffer = 0;
        ::Setup.Save();
    }
*/
}

eOSState cRemoteBlockDeviceItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kOk:
            if(cRecordControls::Active())
            {
                Skins.Message(mtWarning, tr("Warning, recording is running!"), 2);
                return osContinue;
            }
            else
            {
                SaveSettings();
                return os_User; // Redraw Menu
            }
            break;
        case kLeft:
        case kRight:
            *OnlyRecording_ = !(*OnlyRecording_);
            return os_User;
            break;
        default:
            return osUnknown;
    }
}

