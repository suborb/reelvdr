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


#ifndef P__MENU_FORMAT_DISK2_H
#define P__MENU_FORMAT_DISK2_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

#include "ScsiDevice.h"
#include "MenuFormatDisk3.h"

/*enum formatState2
{
    idle2,   
    writingfilesystem2, 
    errorwritingfilesystem2,
    filesystemwritten2,
    integrating2,
    integrated2
};*/
enum formatState;

class cMenuFormatDisk2:public cOsdMenu
{
  public:
    cMenuFormatDisk2(cScsiDevice device, bool &returnToSetup);
    ~cMenuFormatDisk2();


  private:
    cMenuFormatDisk2(const cMenuFormatDisk2 &);
    cMenuFormatDisk2 & operator=(const cMenuFormatDisk2 &);
    /*Override*/ eOSState ProcessKey(eKeys Key);
    void Set();
    void SetProgressItem(std::string textbase, formatState state);
    void StartPrepareHD(std::string option);
    eOSState StartFormatDiskMenu();
    eOSState StartCreateBootDiskMenu();
    
    void GetState();

    /* Is the 'device_' a mediadevice ?
     * Do this check before creating boot disk.
     */
    bool IsMediadevice();

    /* If 'device_' is Media device, stop livebuffer and all recordings
     * before starting to create boot disk
     * (else boot disk creation would fail because umount fails)
     */
    void StopActivityOnMediadevice();

    cScsiDevice device_;
    formatState state_, lastState_;
    bool &returnToSetup_;
    std::string uuid_;
};

#endif // P__MENU_FORMAT_DISK2_H
