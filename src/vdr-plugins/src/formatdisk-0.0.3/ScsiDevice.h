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


#ifndef P__SCSIDEVICE_H
#define P__SCSIDEVICE_H

#include <string>
#include <vector>

struct cPartition
{
    std::string name;
    std::string mountpoint;
};

struct cScsiDevice
{
    std::string name;
    std::string description;
    float size;
    bool hasMountedRecordingsDir;
    bool hasRecordingsDir; 
    bool isBootDevice;
    std::vector<cPartition> partitions;

    cScsiDevice();
}; 

class cScsiDevices
{
  public:
    cScsiDevices(){};
    ~cScsiDevices(){};
    void LoadDevices();
    bool GetDeviceByName(std::string name, cScsiDevice &device);
    //int NumberNotIntegrated();
    bool HasBootDevice();
    std::vector<cScsiDevice>& Get();

  private:    

    cScsiDevices(const cScsiDevices &);
    cScsiDevices & operator=(const cScsiDevices &);

    void CheckStatus2(cScsiDevice &device, std::string name);
    void CheckStatus(cScsiDevice &device);
    bool IsMounted(std::string name, std::string &mountpoint);
    bool HasRecordingsDir(std::string mountpoint);
    bool Mount(std::string name, std::string mountpoint);
    bool UMount(std::string mountpoint);
   
    std::vector<cScsiDevice> devices;
};


#endif // P__SCSIDEVICE_H

