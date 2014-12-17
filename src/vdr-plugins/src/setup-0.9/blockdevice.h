/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 additions by                  Tobias Bratfisch     *
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
 * blockdevice.h
 *
 ***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include <vector>

enum eFileSystem { eUnknownFS, eJFS, eEXT2, eEXT3, eSWAP };

struct geometry
{
    int start;
    int end;
    char *device;
};

class cPartition
{
    private:
        const char letter_;
        int number_;
        char *uuid_;
        long long int size_;
        long long int used_;
        char *mountpoint_;
        eFileSystem filesystem_;
        bool resizable_;

        void CheckResizable();
        bool DetectFreePartitionSpace(std::vector<geometry> *vGeometry, int start, int end);
        void DiskFree();

    public:
        cPartition(const char letter, int number);
        ~cPartition();

        bool OnlyRecording_;

        int GetNumber() const { return number_; };
        const char GetLetter() const { return letter_; };
        const char* GetUUID() { return uuid_; };
        long long int GetSize() const { return size_; };
        long long int GetUsedSpace() const { return used_; };
        long long int GetFreeSpace(); // const;
        const char* GetMountpoint() const { return mountpoint_; };
        eFileSystem GetFileSystem() const { return filesystem_; };
        bool PrimaryPartitionsFull();
        int ResizeJFS();
        int ResizeEXT3();
        bool Resizable() const { return resizable_; };
        bool GetOnlyRecording() const { return OnlyRecording_; };
        void SetOnlyRecording(bool val) { OnlyRecording_ = val; };
};

class cBlockDevice
{
    private:
        const char letter_;
        char *model_;
        long long int size_;
        bool removable_;
        std::vector<cPartition*> partition_;
        bool internal_;
        void CheckIfRootDevice(void);

    public:
        cBlockDevice(char letter);
        ~cBlockDevice();

        const char GetLetter() const { return letter_; };
        const char* GetModel() const { return model_; };
        long long int GetSize() const { return size_; };
        bool Removable() const { return removable_; };
        bool IsRootDevice() const { return internal_; };
        std::vector<cPartition*> *GetPartitions();
        void AddPartition(cPartition *partition);
};

class cRemoteBlockDevice
{
    private:
        char *service_;
        char *mountpoint_;
        char *protocol_;
        long long int size_;
        long long int used_;

    public:
        cRemoteBlockDevice(const char *service, const char *mountpoint, const char *protocol);
        ~cRemoteBlockDevice();

        bool OnlyRecording_;

        const char* GetService() const { return service_; };
        const char* GetMountpoint() const { return mountpoint_; };
        const char* GetProtocol() const { return protocol_; };
        long long int GetSize() const { return size_; };
        long long int GetUsedSpace() const { return used_; };
        long long int GetFreeSpace() const;
        bool GetOnlyRecording() const { return OnlyRecording_; };
        void SetOnlyRecording(bool val) { OnlyRecording_ = val; };
};

#endif
