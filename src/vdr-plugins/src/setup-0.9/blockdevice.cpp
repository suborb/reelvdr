/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * blockdevice.cpp
 *
 ***************************************************************************/

#include "blockdevice.h"

#include <vdr/debug.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

#include <fstream>
#include <sstream>
#include <algorithm>

#include <unistd.h>
#define SCRIPT_PATH "/var/spool/vdr/"

// --- Class cPartition --------------------------------------------------------------
cPartition::cPartition(char letter, int number) : letter_(letter)
{
    number_ = number;
    resizable_ = false;
    mountpoint_ = NULL;
    uuid_ = NULL;
    size_ = 0;
    used_ = -1;
    std::stringstream command;
    FILE *file = NULL;
    char *buffer = NULL;
    //char buffer2[256];
    char trash[512]; // for strings that are not needed
    filesystem_ = eUnknownFS;
    OnlyRecording_ = false;

    DiskFree();

    // get information about size if it is still not available
    if((mountpoint_ == NULL) && (size_ == 0) && ((int)used_ == -1))
    {
        command << "/sys/block/sd" << letter << "/sd" << letter << number << "/size";
        file = fopen(command.str().c_str(), "r");
        command.str("");
        if(file)
        {
            cReadLine readline;
            size_ = atoll(readline.Read(file))/2;
            fclose(file);
        }
    }

    if(size_ == 0) // TB: very strange things have happened, no need to continue
        return;

    // check if this partition is a swap
    if(!mountpoint_)
    {
        command << "LANG=\"C\" cat /proc/swaps | grep sd" << letter << number;
        file = popen(command.str().c_str(), "r");
        command.str("");
        if(file)
        {
            cReadLine readline;
            buffer = readline.Read(file);
            if(buffer)
            {
                mountpoint_ = strdup("SWAP");
                sscanf(buffer, "%s %s %llu %llu", trash, trash, &size_, &used_);
            }
            buffer = NULL;
        }
        pclose(file);
    }

    // Get information about filesystem
    command << "LANG=\"C\" blkid /dev/sd" << letter << number;
    file = popen(command.str().c_str(), "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
        {
            static const char *keyword = " TYPE=\"";
            if(strstr(buffer, keyword))
            {
                char *type = strstr(buffer, keyword) + strlen(keyword);
                *(strchr(type, '\"')) = '\0';
                if(strcmp(type, "jfs") == 0)
                    filesystem_ = eJFS;
                else if(strcmp(type, "ext2") == 0)
                    filesystem_ = eEXT2;
                else if(strcmp(type, "ext3") == 0)
                    filesystem_ = eEXT3;
                else if(strcmp(type, "swap") == 0)
                    filesystem_ = eSWAP;
                else
                    filesystem_ = eUnknownFS;
            }
            else
                filesystem_ = eUnknownFS;

            static const char *keyword2 = " UUID=\"";
            if(strstr(buffer, keyword2)) {
                char *uuid = strstr(buffer, keyword2) + strlen(keyword2);
                *(strchr(uuid, '\"')) = '\0';
                uuid_ = strdup(uuid);
            } else
              uuid_ = NULL;
        }
        buffer = NULL;
    }
    pclose(file);

    // Check if partition is resizable
    if(filesystem_ == eJFS || filesystem_ == eEXT3)
        CheckResizable();
}


cPartition::~cPartition()
{
    free(uuid_);
    free(mountpoint_);
}

void cPartition::DiskFree()
{
    FILE *file = NULL;
    std::stringstream command;
    char *buffer = NULL;
    char buffer2[256];
    char trash[512]; // for strings that are not needed

    /** get information about used space if available */
    command << "LANG=\"C\" df -P /dev/sd" << letter_ << number_ << " | grep ^/dev/sd" << letter_ << number_;
    file = popen(command.str().c_str(), "r");
    command.str("");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
        {
            sscanf(buffer, "%s %llu %llu %s %s %s", trash, &size_, &used_, trash, trash, buffer2);
            mountpoint_ = strdup(buffer2);
        }
        else
        {
            //size_ = 0;
            used_ = -1;
            mountpoint_ = NULL;
        }
        buffer = NULL;
    }
    pclose(file);
    DDD("size_: %llu, used_: %llu", size_, used_);
}

long long int cPartition::GetFreeSpace() //const
{
    DiskFree();
    if(used_ == -1)
        return -1;
    else
        return size_ - used_;
}

bool cPartition::PrimaryPartitionsFull()
{
    char command[64];
    char *buffer = NULL;
    FILE *file = NULL;
    char *blockdevicename = NULL;
    bool PartitionExists[4] = { false, false, false, false };

    snprintf(command, 64, "ls /dev/sd%c*", letter_);
    file = popen(command, "r");
    if (file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer)
        {
            blockdevicename = strrchr(buffer, '/') + 1;
            if(strlen(blockdevicename) == 4)
            {
                //check if this is a logical partition, if yes skip it!
                int number = atoi(blockdevicename + 3);
                if(number < 5)
                    PartitionExists[number-1] = true;
            }
            buffer = readline.Read(file);
        }
        buffer = NULL;
    }
    pclose(file);

    return (PartitionExists[0] && PartitionExists[1] && PartitionExists[2] && PartitionExists[3]);
}

int cPartition::ResizeEXT3()
{
    int result = 0;
    char command[256];
    char fdisk[256];
    char path[256];
    std::ofstream file;

    if(number_ < 5)
    {
        if(PrimaryPartitionsFull())
            snprintf(fdisk, 256, "d\n%i\nn\np\n\n\nw\n", number_);
        else
            snprintf(fdisk, 256, "d\n%i\nn\np\n%i\n\n\nw\n", number_, number_);
    }
    else
    {
        if(PrimaryPartitionsFull())
            snprintf(fdisk, 256, "d\n%i\nn\n\n\nx\nf\nw\n", number_);
        else
            snprintf(fdisk, 256, "d\n%i\nn\nl\n\n\nx\nf\nw\n", number_);
    }
    snprintf(command, 256, "LANG=\"C\" echo \"%s\" | fdisk /dev/sd%c 1>/dev/null 2>/dev/null", fdisk, letter_);
    //printf("\033[0;93m %s \033[0m\n", command);
    result = SystemExec(command);

    mkdir(SCRIPT_PATH, 0755);
    snprintf(path, 256, "%splugin-setup-resize.sh", SCRIPT_PATH);
    file.open(path);

    file << "#!/bin/sh\n\n";

    snprintf(command, 256, "resize2fs /dev/sd%c%i", letter_, number_);
    file << command;

    file.close();
    chmod(path, 0755);

    return 0;
}

int cPartition::ResizeJFS()
{
    char path[256];
    int result = 0;
    char command[256];
    char fdisk[256];
    std::ofstream file;

    if(number_ < 5)
    {
        if(PrimaryPartitionsFull())
            snprintf(fdisk, 256, "d\n%i\nn\np\n\n\nw\n", number_);
        else
            snprintf(fdisk, 256, "d\n%i\nn\np\n%i\n\n\nw\n", number_, number_);
    }
    else
    {
        if(PrimaryPartitionsFull())
            snprintf(fdisk, 256, "d\n%i\nn\n\n\nx\nf\nw\n", number_);
        else
            snprintf(fdisk, 256, "d\n%i\nn\nl\n\n\nx\nf\nw\n", number_);
    }
    snprintf(command, 256, "LANG=\"C\" echo \"%s\" | fdisk /dev/sd%c 1>/dev/null 2>/dev/null", fdisk, letter_);
//printf("\033[0;93m %s \033[0m\n", command);
    result = SystemExec(command);

//    sprintf(command, "mount -o remount,resize %s", mountpoint_); // This won't work before the box is rebooted! :(
//printf("\033[0;93m %s \033[0m\n", command);
//    SystemExec(command);

#if 0
//commented out since it won't work. uuid is updated after rebooting :/
    // update uuid
    sprintf(command, "blkid /dev/sd%c%i", letter_, number_);
    file = popen(command, "r");
    free(command);
    if(file)
    {
        cReadLine readline;
        char *buffer = readline.Read(file);
        if(buffer)
        {
            char *keyword = " UUID=\"";
            if(strstr(buffer, keyword))
            {
                char *uuid = strstr(buffer, keyword) + strlen(keyword);
                *(strchr(uuid, '\"')) = '\0';
                strcpy(uuid_, uuid);
            }
        }
        buffer = NULL;
    }
    pclose(file);
#endif

//commented out: This won't work since uuid/partition-letter may change after rebooting :/
//    if(result == 0)
//    {
    // build a script for resizing JFS-partition after rebooting
    mkdir(SCRIPT_PATH, 0755);
    snprintf(path, 256, "%splugin-setup-resize.sh", SCRIPT_PATH);
    file.open(path);

    file << "#!/bin/sh\n\n";
    file << "MP=/media/$$\n";
    file << "mkdir $MP\n";

    snprintf(command, 256, "mount /dev/disk/by-uuid/%s $MP\n\n", uuid_);
    file << command;

    file << "mount -o remount,resize $MP\n";
    file << "umount $MP\n";
    file << "rmdir $MP\n";
    file << "rm $0\n";

    file.close();
    chmod(path, 0755);
//    }

//    return result;
    return 0;
}


/*
 * returns true if the given device has partition table type as GPT
 * else returns false
 */
bool IsGPT(std::string device)
{
    char command[512];
    snprintf(command, 512, "LANG=\"C\" fdisk -l %s | grep GPT", device.c_str());
    int ret = SystemExec(command);

    isyslog("SystemExec('%s') returns %d", command, ret);

    if (ret == 0) {
        isyslog("device: %s has GPT partition table", device.c_str());
        return true;
    }

    isyslog("device: %s does not have GPT partition table", device.c_str());
    return false;
}

void cPartition::CheckResizable()
{
    char command[512];

#if 1 // handle GPT partition table
    /** Handle GPT partitions
     * XXX Stop-gap fix. since the rest of the code depends on fdisk which does not work with GPT
     * do not allow resizing in those cases.
     *
     * XXX What if the partition table type is neither GPT nor msdos ?
     *
     * TODO: allow GPT partitions to be resized
     */
    snprintf(command, 512, "/dev/sd%c", letter_);
    if (IsGPT(command)) {
        resizable_ = false;
        isyslog("assuming /dev/sd%c is not resizeable because it contains GPT type partition table", letter_);
        return;
    }
#endif // handle GPT partition table

    FILE *file = NULL;
    char *buffer = NULL;
    char buffer2[512];
    char trash[512];
    std::vector<geometry> vGeometry;
    geometry tempGeometry;
    int cylinders, start, end, ext_start = 0, ext_end = 0;
    // Get information about geometry and check if the partition is resizable
    snprintf(command, 512, "LANG=\"C\" fdisk -l /dev/sd%c | grep /dev/sd%c | grep -v \"Disk /dev/sd%c\"", letter_, letter_, letter_);
    //printf("Executing: \"%s\"\n", command);
    file = popen(command, "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer)
        {
            start = 0;
            end = 0;
            buffer2[0] = '\0';

            if(strchr(buffer, '*'))
                sscanf(buffer, "%s %s %i %i", buffer2, trash, &start, &end);
            else
                sscanf(buffer, "%s %i %i", buffer2, &start, &end);

            if(strstr(buffer, "Extended"))
            {
                ext_start = start;
                ext_end = end;
            }

            tempGeometry.device = strdup(buffer2);
            tempGeometry.start = start;
            tempGeometry.end = end;

            vGeometry.push_back(tempGeometry);
            buffer = readline.Read(file);
        }
        buffer = NULL;
    }
    pclose(file);

    // Get numbers of cylinders of the harddisk
    snprintf(command, 256, "LANG=\"C\" fdisk -l /dev/sd%c | grep heads | grep sectors | grep cylinders", letter_);
    //printf("Executing: \"%s\"\n", command);
    file = popen(command, "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
            sscanf(buffer, "%s %s %s %s %i", trash, trash, trash, trash, &cylinders);
        else
            cylinders = 0;
    }
    pclose(file);

    if((cylinders > 0) && (vGeometry.size() > 0))
    {
        if(number_ < 5)
            resizable_ = DetectFreePartitionSpace(&vGeometry, 1, cylinders);
        else
            resizable_ = DetectFreePartitionSpace(&vGeometry, ext_start, ext_end);
    }
    else
        resizable_ = false;

    // free memory
    for(unsigned int i=0; i < vGeometry.size(); ++i)
        free(vGeometry.at(i).device);
}

bool cPartition::DetectFreePartitionSpace(std::vector<geometry> *vGeometry, int start, int end)
{
    unsigned int i=0;
    char device[32];
    snprintf(device, 32, "/dev/sd%c%i", letter_, number_);
    while((i < vGeometry->size()) && !(strstr(vGeometry->at(i).device, device)))
        ++i;

    // Check if there is gap before the partition
    if((i == 0) || ( number_ == 5))
    {
        if(vGeometry->at(i).start > start)
            return true;
    }
    else if (vGeometry->at(i).start > vGeometry->at(i-1).end + 1)
        return true;

    // Check if there is gap after the partition
    if(i == vGeometry->size()-1)
    {
        if(vGeometry->at(i).end < end)
            return true;
    }
    else if (vGeometry->at(i).end < vGeometry->at(i+1).start - 1)
        return true;

    return false;
}

// --- Class cBlockDevice ------------------------------------------------------------
cBlockDevice::cBlockDevice(char letter) : letter_(letter)
{
    model_ = NULL;
    std::stringstream command;
    FILE *file = NULL;
    DIR *dir = NULL;
    struct dirent *dp = NULL;
    char *buffer = NULL;
    size_ = 0;
    std::vector<int> partnumbers;
    internal_ = false;
    CheckIfRootDevice();

    /** get disk size */
    command << "/sys/block/sd" << letter << "/size";
    file = fopen(command.str().c_str(), "r");
    command.str("");
    if(file)
    {
        cReadLine readline;
        char *line = readline.Read(file);
        if(line)
            size_ = atoll(line)/2;
        fclose(file);
    }

    if(size_ == 0)
        return; // TB: devices with size 0 (e.g. sd-cardreader without card) won't be displayed, so we can return right now

    /** get partitions */
    dir = opendir("/dev/");
    if (dir) {
        std::string diskname = "sd";
        diskname += letter;
        do {
            errno = 0;
            if ((dp = readdir(dir)) != NULL) {
                if ((strncmp(dp->d_name, diskname.c_str(), 3) != 0) || (strlen(dp->d_name) != 4))
                    continue;

                //check if this is a logical partition, if yes skip it!
                std::stringstream path2;
                char trash[64];
                unsigned long long blocks = 0;

                path2 << "LANG=\"C\" cat /proc/partitions | grep sd" << letter_ << atoi(dp->d_name+3);
                file = popen(path2.str().c_str(), "r");
                path2.str("");
                if(file)
                {
                    cReadLine readline;
                    buffer = readline.Read(file);
                    if(buffer)
                        sscanf(buffer, "%s %s %llu", trash, trash, &blocks);
                    buffer = NULL;
                }
                pclose(file);
                if(blocks > 1)
                    partnumbers.push_back(atoi(dp->d_name+3));
            }
        } while (dp != NULL);
    }
    sort(partnumbers.begin(), partnumbers.end());
    for(unsigned int i = 0; i < partnumbers.size(); ++i)
        partition_.push_back(new cPartition(letter_, partnumbers.at(i)));

    command << "/sys/block/sd" << letter << "/device/model";
    file = fopen(command.str().c_str(), "r");
    command.str("");
    if(file)
    {
        cReadLine readline;
        char *line = readline.Read(file);
        if(line)
            model_ = strdup(line);
        fclose(file);
    }

    command << "/sys/block/sd" << letter << "/removable";
    file = fopen(command.str().c_str(), "r");
    command.str("");
    if(file)
    {
        char c = '0';
        if(fread(&c, 1, 1, file) == 1) {
            if(c == '1')
                removable_ = true;
            else
                removable_ = false;
        } else
            removable_ = false;
        fclose(file);
    }
}

cBlockDevice::~cBlockDevice()
{
    free(model_);
    for(std::vector<cPartition*>::iterator partIter=partition_.begin(); partIter != partition_.end(); ++partIter)
        delete (*partIter);
    partition_.clear();
}

std::vector<cPartition*> *cBlockDevice::GetPartitions()
{
    return &partition_;
}

void cBlockDevice::AddPartition(cPartition *partition)
{
    partition_.push_back(partition);
}

void cBlockDevice::CheckIfRootDevice(void)
{
#ifdef RBMINI
    /** the internal usb stick is on usb bus nr. 2 and is device nr. 4 */
    dsyslog("setup: checking if \"/dev/sd%c\" is the internal usb-stick: %i\n", letter_, internal_);
    char buf[256];
    std::string path = std::string("/sys/block/sd") + letter_ + "/device";
    if (readlink(path.c_str(), buf, 256) != -1) {
        if (strncmp(buf, "../../devices/platform/cnxt1/usb2/2-1/2-1.2/2-1.2:1.0/host1/target1:0:0/1:0:0:0", 44) == 0)
                internal_ = true;
    } else
        esyslog("Error reading target of \"%s\": error \"%s\"", path.c_str(), strerror(errno));
#else
    internal_ = false;
    char buf[256];
    //std::string path = std::string("/sys/block/sd") + _ + "/device";
    if (readlink("/dev/root", buf, sizeof(buf)) == -1)
        esyslog("This device does not have /dev/root. Cannot find the root device");
    else if (buf[2] == letter_)
    {
        isyslog("Device sd%c is the root device", letter_);
        internal_ = true;
    }
#endif
}

// --- Class cRemoteBlockDevice ------------------------------------------------------
cRemoteBlockDevice::cRemoteBlockDevice(const char *service, const char *mountpoint, const char *protocol)
{
    service_ = NULL;
    mountpoint_ = NULL;
    protocol_ = NULL;
    char command[64];
    FILE *file = NULL;
    char *buffer = NULL;
    size_ = 0;
    used_ = 0;
    OnlyRecording_ = false;

    service_ = strdup(service);
    mountpoint_ = strdup(mountpoint);
    protocol_ = strdup(protocol);

    // get information about size and used space if available
    snprintf(command, 64, "df -P | grep %s$", mountpoint_);
    file = popen(command, "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
        {
            char trash[128];
            sscanf(buffer, "%s %llu %llu", trash, &size_, &used_);
        }
        else
            used_ = -1;
        buffer=NULL;
    }
    pclose(file);
}

cRemoteBlockDevice::~cRemoteBlockDevice()
{
    free(protocol_);
    free(mountpoint_);
    free(service_);
}

long long int cRemoteBlockDevice::GetFreeSpace() const
{
    if(used_ == -1)
        return -1;
    else
        return size_ - used_;
}

