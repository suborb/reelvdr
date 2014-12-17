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


#ifndef P__MENU_FORMAT_DISK3_H
#define P__MENU_FORMAT_DISK3_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include "ScsiDevice.h"

enum formatState
{
    idle,   
    partitioning, 
    errorpartitioning,
    partitioned,
    formatting,
    errorformatting,
    formatted,
    writingfilesystem, 
    errorwritingfilesystem,
    filesystemwritten,
    integrating,
    integrated,
    
    creatingbootdisk,       // download and write the image to a device
    errorcreatingbootdisk,  // error
    createdbootdisk,        // done successfully
    
    // remaining space: create & format media partition  
    // (ie. after downloading the image and writing it to device)
    creatingmediapartition,
    errorcreatingmediapartition,
    createdmediapartition,
    
    // create media directory structure
    creatingmediadirectories,
    errorcreatingmediadirectories,
    createdmediadirectories,
    
    updatingdefaultsystemsettings,
    errorupdatingdefaultsystemsettings,
    updateddefaultsystemsettings
};


//---------------------------------------------------------------------
//----------used by cMenuFormatDisk3 and cMenuFormatDisk2--------------
//---------------------------------------------------------------------

std::string GetUuid();
std::string GetLastScriptState();
void StartPrepareHD(std::string option, std::string devicename);
void IntegrateDisk(formatState &state);
void FormatDisk(formatState &state, std::string devicename);
void WriteFileSystem(formatState &state, std::string devicename);
void MountDisk(formatState &state, std::string devicename);
void PartitionDisk(formatState &state, std::string devicename);
void UpdateDefaultSystemSettings(formatState &state, std::string devicename);
std::string GetProgressItem(std::string textbase, formatState state, formatState startstate, std::string info="");

//---------------------------------------------------------------------
//----------------------------cMenuFormatDisk3-------------------------
//---------------------------------------------------------------------

class cMenuFormatDisk3:public cOsdMenu
{
  public:
    cMenuFormatDisk3(cScsiDevice device, bool &returnToSetup, bool createBootDisk=false);
    ~cMenuFormatDisk3(){}

  private:
    cMenuFormatDisk3(const cMenuFormatDisk3 &);
    cMenuFormatDisk3 & operator=(const cMenuFormatDisk3 &);
    /*Override*/ eOSState ProcessKey(eKeys Key);
    void SetProgressItem(std::string textbase, formatState state);
    void Set();
    void SetCreateBootDisk(); // called from Set() if createBootDisk_ = true
    void GetState();

    void AddInfoText(std::string currText);

    cScsiDevice device_;
    formatState state_, lastState_;
    bool &returnToSetup_;
    bool createBootDisk_;
    std::string ongoing_str;

    /* mediatype decides which filesystem to use and
     * if some options in filesystem is turned off (ext4 journalising in SSD) */
    const char* mediaTypeStr[3];
    enum MediaType { eMediaTypeUnknown = -1, eMediaTypeHDD = 0, eMediaTypeSSD, eMediaTypeUSB };
    MediaType mediaType;
    int mediaTypeInt;

    /* offer user the option of cloning the current root !
     * User may choose to download the image from the internet(reelbox server)
     * or clone his/her root image(!)
     */
    const char* imgSourceStr[2];
    enum ImageSource { eImageSrcUnknown=-1, eImageSrcInternet=0, eImageSrcCurrentRoot };
    ImageSource imgSource;
    int imgSourceInt;
};

#endif // P__MENU_FORMAT_DISK3_H
