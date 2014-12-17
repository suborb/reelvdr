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
 
#include "mountmanager.h"
#include "def.h"
#include <vdr/thread.h>

cMountManager::~cMountManager()
{
    //Umount();
}

void cMountManager::MountAll(const std::vector<std::string> &entries)
{
    bool mountDvd = false;
    for (uint i = 0; i < entries.size(); i++)
    {
        if(StartsWith(entries[i], dvdPath))
            mountDvd = true;
    }
    if(mountDvd)
    {
        if(DoMount(dvdPath))
            mounts.push_back(dvdPath);
    }
}

bool cMountManager::Mount(const std::string &path)
{
    if(path == dvdPath)
    {
        if(DoMount(dvdPath))
            mounts.push_back(dvdPath);
        else
            return false;
    }
    return true;
}

void cMountManager::Umount()
{ 
    for (uint i = 0; i < mounts.size(); i++)
        DoUmount(mounts[i]);
}

bool  cMountManager::IsMounted(const std::string &path)
{
    if(path != dvdPath)
        return false;

    std::string cmd = "sudo -s " + std::string(MOUNT_SH) + " status " + path;
    if (SystemExec(cmd.c_str()) == 0)
    {
        return true;
    }
    return false;
}

bool cMountManager::DoUmount(const std::string &path)
{
    std::string cmd = "sudo -s " + std::string(MOUNT_SH) + " unmount " + path;
    if (SystemExec(cmd.c_str()) == 0)
    {
        return true;
    }
    return false;    
}

bool cMountManager::DoMount(const std::string &path)
{
    if (IsMounted(path))
    {
        return true;
    }
    else
    {
        std::string cmd = "sudo -s " + std::string(MOUNT_SH) + " mount " + path;
        if (SystemExec(cmd.c_str()) == 0)
        {
            return true;    
        }
    }
    return false;
}



