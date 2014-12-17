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
 * setuprecordingdevicemenu.h
 *
 ***************************************************************************/

#ifndef SETUPRECORDINGDEVICEMENU_H
#define SETUPRECORDINGDEVICEMENU_H

#include <vdr/menu.h>

#include "blockdevice.h"

// ---- Class cSetupRecordingDeviceMenu ---------------------------------------------
class cSetupRecordingDeviceMenu : public cOsdMenu
{
    private:
        std::vector<cBlockDevice*> blockdevice_;
        std::vector<cRemoteBlockDevice*> remoteblockdevice_;
        std::string CurrentDevice_;
        bool LiveBuffer_;
        bool InitMenu_;
        bool DisableMultiRoom_;
        void DisableMultiRoom();

    public:
        cSetupRecordingDeviceMenu(const char *title);
        ~cSetupRecordingDeviceMenu();

        void GetDiskInformations();
        void ClearDiskInformations();
        void Set();
        void SetHelpKeys();
        eOSState ProcessKey(eKeys Key);
};

class cBlockDeviceItem : public cOsdItem
{
    private:
        char Letter_;
        cPartition *Partition_;
        std::string *CurrentDevice_;
        bool *OnlyRecording_;
        unsigned long long size_;
        unsigned long long free_;
        bool Removable_;
        bool Resize_;
        bool LiveBuffer_;
        bool onInternalUsbStick_;

    public:
        cBlockDeviceItem(const char *text, cPartition *Partition, char Letter, std::string *CurrentDevice, bool *OnlyRecording, bool Removable, bool Resize, bool LiveBuffer, bool onInternalUsbStick = false);
        ~cBlockDeviceItem();
        std::string GetDevicename();

        bool Formatable() const;
        bool MountActive(const char* device);
        eOSState ProcessKey(eKeys Key);
        bool Resizeable() const { return Resize_; };
        int Size() const { return size_; }
        int Free() const { return free_; }
};

class cRemoteBlockDeviceItem : public cOsdItem
{
    private:
        const char *mountpoint_;
        std::string *CurrentDevice_;
        bool *OnlyRecording_;
        unsigned long long size_;
        unsigned long long free_;

    public:
        cRemoteBlockDeviceItem(const char *text, const char *mountpoint, std::string *CurrentDevice, bool *OnlyRecording, int size, int free);
        ~cRemoteBlockDeviceItem();

        void SaveSettings();
        eOSState ProcessKey(eKeys Key);
};

#endif

