/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 * install_formathd.h
 *
 ***************************************************************************/

#ifndef INSTALL_FORMATHD_H
#define INSTALL_FORMATHD_H

#include "installmenu.h"

#include <string>

enum HardDiskState { eUnknownHDD, /* format it and use it as mediadevice */
                     eFormatInProgressHDD,
                     eUnusedReelHDD, /* use it as mediadevice without formatting */
                     eAlreadyInUseHDD /* nothing to do */
                   };

class cInstallFormatHD:public cInstallSubMenu
{
  private:

    HardDiskState hddState;

    bool SecureLock_;
    bool Running_;
    bool Finish_;
    time_t start_time;          // start time of formatting
    std::string Modelname_;
    std::string harddiskDevice;
    std::string rec_partition;

    void Set();

    void SetUnknownHarddisk();
    void SetHarddiskFormatting();

    /** If the harddisk was identified as reelbox-harddisk, then offer user the option to
    * use the harddisk as mediadevice, without formatting
    */
    void SetUnusedReelHardisk();

    /** show the user the information that the harddisk being used as mediadevice
    */
    void SetHardiskAlreadyInUse();


    void SetAskUserForConfirmation();
    void SetSuccessful();
    void SetFormatingError();
    void SetNotFormating();
    void SetFormatting();       // shows bar indicating formatting progress

    bool Save();

    eOSState ProcessKey_formatting(eKeys Key);
    eOSState ProcessKey_UnusedReelHDD(eKeys Key);
    eOSState ProcessKey_HDDAlreadyInUse(eKeys Key);
    eOSState ProcessKey_UnknownHDD(eKeys Key);

  public:
      cInstallFormatHD(std::string harddiskDevice);
     ~cInstallFormatHD();
    eOSState ProcessKey(eKeys Key);
};


/**
 * @brief IsReelboxHarddisk : check partitions of given harddisk for /recordings dir
 * @param device : harddisk device '/dev/sdc'
 * @return true if the largest partition is in a compatible filesystem and
 *         has /recordings directory and the partition is copied to 'recording_partition'
 */
bool IsReelboxHarddisk(std::string device, std::string &recordings_partition);

#endif
