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
 
#ifndef P__MOUNTMANAGER_H
#define P__MOUNTMANAGER_H

#include <string>
#include <vector>

#include "stringtools.h"

class cMountManager 
{
private:
    std::vector<std::string> mounts;
    bool DoMount(const std::string &path);
    bool DoUmount(const std::string &path);
public:
    cMountManager(){}; 
    ~cMountManager();
    
    bool IsMounted(const std::string &path);
    bool Mount(const std::string &path);
    void MountAll(const std::vector<std::string> &entries);
    void Umount();
};

#endif //P__MOUNTMANAGER_H



