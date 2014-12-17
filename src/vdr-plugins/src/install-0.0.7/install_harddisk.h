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
 * install_harddisk.h
 *
 ***************************************************************************/

#ifndef INSTALL_HARDDISK_H
#define INSTALL_HARDDISK_H

#include "installmenu.h"
#include <string>

class cInstallHarddisk:public cInstallSubMenu
{
  private:
    bool HarddiskAvailable_;
    bool StorageAVG_;
      std::string UUID_;
    void Set();
    bool Save();
  public:
      cInstallHarddisk(bool HarddiskAvailable);
     ~cInstallHarddisk();
    eOSState ProcessKey(eKeys Key);
};

#endif
