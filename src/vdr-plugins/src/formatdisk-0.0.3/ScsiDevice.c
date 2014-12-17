/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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
 ***************************************************************************/

const char *const MOUNT_SH = "/usr/sbin/mount.sh";

#include "ScsiDevice.h"
#include "stringtools.h"
#include "filetools.h"
//#include "mountmanager.h"

#include <vdr/debug.h>
#include <vdr/thread.h>

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <errno.h>

using namespace std;

const char *const TMPMOUNTDIR = "/mnt/testmount";
const char *const LISTMOUNTS = "/tmp/listmounts";
const char *const LISTPARTITIONS = "/tmp/listallpartions";
 const char *const DESCRIPTION  = "/tmp/devicedescription";
const char *const VIDEODIR = "/recordings";


//---------------------------------------------------------------------
//----------------------------cScsiDevice-----------------------------
//---------------------------------------------------------------------

cScsiDevice::cScsiDevice():
name(""), description(""), size(0.0), hasMountedRecordingsDir(false), hasRecordingsDir(false), isBootDevice(false)
{
}

//---------------------------------------------------------------------
//----------------------------cScsiDevices----------------------------
//---------------------------------------------------------------------

std::vector<cScsiDevice>& cScsiDevices::Get()
{
    return devices;
}

bool cScsiDevices::GetDeviceByName(std::string name, cScsiDevice &device)
{
    for(uint i = 0; i< devices.size(); ++i)
    {
        if(devices[i].name == name)
        {
            device = devices[i];
            return true;
        }
    }
    return false;
}

void cScsiDevices::LoadDevices()
{
    //DDD("-----cScsiDevices::LoadDevices()--------\n");
    std::string cmd = "cat /proc/partitions > " + std::string(LISTPARTITIONS);
    DDD("%s", cmd.c_str());
    SystemExec(cmd.c_str());

    int m, n;
    long long size;
    char line[256];
    char name[256];

    ifstream is(LISTPARTITIONS, ifstream::in);
    if(is.good())
    {
        is.getline (line,256);
        DDD("line = %s", line);
        is.getline (line,256);
        DDD("line = %s", line);

        is.setf (ios::skipws);

        while (!is.eof())
        {
            is >> m >> n >> size >> name; 
            if(!is.fail())
            {
                DDD("device: m = %d n = %d size = %lld name = %s", m, n, size, name);
    
                if(size > 1024*1024 && name[0] == 's' && name[1] == 'd' && std::string(name).size() == 3)
                {
                    cScsiDevice newDevice;
                    newDevice.name = name;
                    newDevice.size = float(size)/(1024*1024);
                    devices.push_back(newDevice);
                    DDD("found a device %s", newDevice.name.c_str());
                }
                if(std::string(name).size() == 4 && StartsWith2(name, devices[devices.size()-1].name))
                {
                    DDD("StartsWith2=true");
                    cPartition newPartition =  {std::string(name), std::string("")};
                    DDD("cScsiDevices: new partition, %s",newPartition.name.c_str());
                    devices[devices.size()-1].partitions.push_back(newPartition);
                }
            }
        }
        is.close();
    }

    //Get vendor description
    cmd = "touch " + std::string(DESCRIPTION);
    DDD("%s", cmd.c_str());
    SystemExec(cmd.c_str());
    for (uint i = 0; i< devices.size(); ++i)
    {
        cmd = "cat /sys/block/" + devices[i].name + "/device/model > " + DESCRIPTION;
        //DDD("%s", cmd.c_str());
        SystemExec(cmd.c_str());
        ifstream is(DESCRIPTION, ifstream::in);
        if(is.good())
        {
            char line[256];
            is.getline (line,256);
            devices[i].description = line;
        }
    }

    //check for mount status and recordings dir
    cmd = "mount > " + std::string(LISTMOUNTS);
    //DDD("%s", cmd.c_str());
    SystemExec(cmd.c_str());

    cmd = "[ -d "  + std::string(TMPMOUNTDIR) + " ] || mkdir " + std::string(TMPMOUNTDIR);
    SystemExec(cmd.c_str());

    for (uint i = 0; i< devices.size(); ++i)
    {
        CheckStatus(devices[i]);
    }

    /*for (uint i = 0; i< devices.size(); ++i)
    {
        if(IsMounted(devices[i]))
        {
            devices[i].mounted = true;
        }
    }

    //check if integrated
    for (uint i = 0; i< devices.size(); ++i)
    {
        if(IsIntegrated(devices[i]))
        {
            devices[i].integrated = true;
        }
    }*/
}

void cScsiDevices::CheckStatus2(cScsiDevice &device, std::string name)
{
    std::string mountpoint;
    if(IsMounted(name, mountpoint))
    {
        DDD("mountpoint: %s", mountpoint.c_str());
        if(mountpoint == "/")
        {
            device.isBootDevice = true;
        }
        if(HasRecordingsDir(mountpoint))
        {
            device.hasRecordingsDir = true;
            device.hasMountedRecordingsDir = true;
        }
    }
    else
    {
        if(Mount(name, TMPMOUNTDIR))
        {
            if(HasRecordingsDir(TMPMOUNTDIR))
            {
                device.hasRecordingsDir = true;
            }
            UMount(TMPMOUNTDIR);
        }
    }
}


void cScsiDevices::CheckStatus(cScsiDevice &device)
{
    //printf("-----------CheckStatus, device.name = %s-----\n", device.name.c_str());
    //device
    CheckStatus2(device, device.name);

    //partitions
    for (uint i = 0; i< device.partitions.size(); ++i)
    {
        CheckStatus2(device, device.partitions[i].name);
    }
}

bool cScsiDevices::IsMounted(std::string name, std::string &mountpoint)
{
    ifstream is(LISTMOUNTS, ifstream::in);
    if(is.good())
    {
        std::string line;
        while(!is.eof ( ))
        {
            getline (is, line);
            if(line.find(name) != line.npos)
            {
                int start = line.find (" on ") + 4;
                int stop = line.substr(start).find_first_of(" ");
                mountpoint = line.substr(start, stop);
                //printf("----mountpoint = %s-----------\n", mountpoint.c_str());
                is.close();
                return true;
            }
        }
        is.close();
    }
    return false;
}

bool cScsiDevices::Mount(std::string name, std::string mountpoint)
{
    std::string cmd = "mount -t auto /dev/" + name + " " + mountpoint;
    DDD("%s", cmd.c_str());
    return (SystemExec(cmd.c_str()) == 0);
}

bool cScsiDevices::UMount(std::string mountpoint)
{
    std::string cmd = "umount " + mountpoint;
    return (SystemExec(cmd.c_str()) == 0);
}

bool cScsiDevices::HasRecordingsDir(std::string mountpoint)
{
    std::string recordingsDir = mountpoint + VIDEODIR;
   // printf("---cScsiDevices::HasRecordingsDir, recordingsDir = %s----------\n", recordingsDir.c_str());
    if(Filetools::DirExists(recordingsDir))
    {
        return true;
    }
    return false;
}

/*bool cScsiDevices::IsIntegrated(cScsiDevice &device)
{
    std::string recordingsDir = device.mountpoint + VIDEODIR;
    //printf("---cScsiDevices::IsIntegrated, recordingsDir = %s----------\n", recordingsDir.c_str());
    if(Filetools::DirExists(recordingsDir))
    {
        return true;
    }
    for (uint i = 0; i< device.partitions.size(); ++i)
    {
        DDD("device.partitions[i].mountpoint = %s\n", device.partitions[i].mountpoint.c_str());
        recordingsDir = device.partitions[i].mountpoint + VIDEODIR;
        if(Filetools::DirExists(recordingsDir))
        {
            return true;
        }
    }
    return false;
}*/

/*int cScsiDevices::NumberNotIntegrated()
{
    int num = 0;
    for (uint i = 0; i< devices.size(); ++i)
    {
        if(!devices[i].integrated)
        {
            ++num;
        }
    }
    return num;
}*/

bool cScsiDevices::HasBootDevice()
{
    int num = 0;
    for (uint i = 0; i< devices.size(); ++i)
    {
        if(devices[i].isBootDevice)
        {
            return true;
        }
    }
    return false;
}
