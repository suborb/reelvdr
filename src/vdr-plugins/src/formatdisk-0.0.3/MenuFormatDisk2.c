/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedium                                 *
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

#include "MenuFormatDisk2.h"
#include "stringtools.h"

#include <iostream>
#include <fstream>

#include "vdr/plugin.h"
#include "vdr/thread.h"
#include "vdr/sysconfig_vdr.h"
#include "vdr/menu.h"
#include "vdr/interface.h"
#include <time.h>

#define VIDEODIR "/recordings"
#define MAX_LETTER_PER_LINE 46


using namespace std;

/* Give uuid string return device name
 * eg. "ad73c128-bd08-4cd6-ab6c-a4524165c271" return "/dev/sda4"
 */
string UUID2Device(const string& uuid)
{
    char path[PATH_MAX];
    string uuidPath = "/dev/disk/by-uuid/" + uuid;
    string device;

    if (realpath( uuidPath.c_str(), path) != NULL)
        device = path;

    return device;
}


/* detect if the given partition belongs to given device
 *   detection algorithm is just substring matching
 *   'device' must be a substring of 'partition'
 *
 *   eg. device := /dev/sda  partition := /dev/sda12  returns true
 */
bool PartitionOfDevice(const string& device, const string& partition)
{
    isyslog("%s device:%s partition:%s", __FUNCTION__, device.c_str(), partition.c_str());
    return partition.find(device) != string::npos;
}


bool IsReelIce()
{
    // TODO: must do better, use reellib and check HasDevice() etc...
    return cPluginManager::GetPlugin("reelice");
}

//---------------------------------------------------------------------
//----------------------------cMenuFormatDisk2-------------------------
//---------------------------------------------------------------------

cMenuFormatDisk2::cMenuFormatDisk2(cScsiDevice device, bool &returnToSetup)
:cOsdMenu(tr("Prepare disk: confirm formatting")), state_(idle), returnToSetup_(returnToSetup)
{
    SetCols(3, 15, 10);
    device_ = device;
    Set();
}

cMenuFormatDisk2::~cMenuFormatDisk2()
{
}

void cMenuFormatDisk2::Set()
{
    int current = Current();
    Clear();

    if(state_ == idle)
    {
        std::string text = std::string(tr("The storage medium ")) + "'" + device_.description + "'" +
        tr(" will be prepared. Please press the red key or choose another action:");
        AddFloatingText(text.c_str(), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("",osUnknown, false));
        cOsdItem *firstSelectableItem = new cOsdItem(tr("\tFormat disk"));
        Add(firstSelectableItem);
        Add(new cOsdItem(tr("\tMount harddisk")));
        
        /* available only on AVG 3 and NCL 2 */
        if (IsReelIce())
        {
            Add(new cOsdItem("",osUnknown, false));
            Add(new cOsdItem(tr("\tCreate boot disk")));
        }

        //Add(new cOsdItem(tr("\tCancel")));

        Add(new cOsdItem("",osUnknown, false));
        Add(new cOsdItem("",osUnknown, false));
        Add(new cOsdItem("____________________________________________________________",osUnknown, false));
        Add(new cOsdItem("",osUnknown, false));

        Add(new cOsdItem(tr("Info:"), osContinue, false));

        SetCurrent(Get(current));
        if(Current() == -1)
        {
            SetCurrent(firstSelectableItem );
        }

        //printf("Get(Current())->Text() = %s\n", Get(Current())->Text());
        if(std::string(Get(Current())->Text()) == tr("\tFormat disk"))
        {
            AddFloatingText(tr("The storage medium is going to be formatted.\nAll data will be destroyed."), MAX_LETTER_PER_LINE);
        }
        else if(std::string(Get(Current())->Text()) == tr("\tMount harddisk"))
        {
            AddFloatingText(tr("The storage medium will not be formatted but integrated into the system to be used as the medium disk."), MAX_LETTER_PER_LINE);
        }
        else if(std::string(Get(Current())->Text()) == tr("\tCreate boot disk"))
        {
            AddFloatingText(tr("CAUTION: All data will be destroyed.\nA fresh copy of the operating system will be installed. Needs internet connection."), MAX_LETTER_PER_LINE);
        }
        else if(std::string(Get(Current())->Text()) == tr("\tCancel"))
        {
            AddFloatingText(tr("With pressing 'OK' you will cancel this process."), MAX_LETTER_PER_LINE);
        }
    }
    else
    {
        SetCols(2, 20, 10);
        //AddFloatingText(tr("The storage medium will not be formatted but integrated into the system to be used as the medium disk."), MAX_LETTER_PER_LINE);
        AddFloatingText(tr("Please wait, the setup for the storage media is running"), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("",osUnknown, false));

        std::string text = "";
        if(state_ >= writingfilesystem)
        {
            //writing basic directory structure
            text = GetProgressItem(tr("installing basic structure"), state_, writingfilesystem);
            if(text != "")
            {
                Add(new cOsdItem(text.c_str(),osUnknown, false));
            }
        }

        std::string textbase = tr("integrating disk");
        text = "";
        if(state_ == integrating)
        {
            text = "\t" + textbase + "...";
            Add(new cOsdItem(text.c_str(), osUnknown, false));
        }
        if(state_ == integrated)
        {
            text = "\t" + textbase + " \t>";
            Add(new cOsdItem(text.c_str(), osUnknown, false));
            Add(new cOsdItem("",osUnknown, false));
            AddFloatingText(tr("Process done."), MAX_LETTER_PER_LINE);
            Add(new cOsdItem("",osUnknown, false));
            AddFloatingText(tr("The storage medium has been successfully integrated."
                               " Please press the red key to continue"), MAX_LETTER_PER_LINE);
        }
        if(state_ == errorwritingfilesystem)
        {
            Add(new cOsdItem("",osUnknown, false));
            Add(new cOsdItem("",osUnknown, false));
            AddFloatingText(tr("The integration of the storage medium has failed"), MAX_LETTER_PER_LINE);
        }
    }

    SetHelp(tr("Button$Continue"), NULL , NULL, NULL);

    Display();
}

eOSState cMenuFormatDisk2::StartFormatDiskMenu()
{
    //printf("-----cMenuFormatDisk2::FormatDisk()-------\n");
    return AddSubMenu(new cMenuFormatDisk3(device_, returnToSetup_));
}

eOSState cMenuFormatDisk2::StartCreateBootDiskMenu()
{
    return AddSubMenu(new cMenuFormatDisk3(device_, returnToSetup_, true));
}


bool cMenuFormatDisk2::IsMediadevice()
{
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    const char* uuid_mediadevice = sysconfig.GetVariable("MEDIADEVICE");

    isyslog("%s sysconfig MEDIADEVICE uuid: %s", __FUNCTION__, uuid_mediadevice);
    if (!uuid_mediadevice)
        return false;

    string uuid = uuid_mediadevice;

    /* uuid is of the form 'UUID="XXXXXXXXX"'
     * remove quotes(") and 'UUID='
     */
    if (uuid.find("UUID=") != 0) // does not start with 'UUID='
        return false;

    uuid = uuid.substr(5); // remove 'UUID='

    // uuid may or may not be quoted!
    if (uuid[0] == '\"')
        uuid = uuid.substr(1); // remove first char (")

    if (uuid[uuid.size()] == '\"') // remove last char (")
        uuid = uuid.substr(0, uuid.size()-1);

    isyslog("%s stripped uuid: %s", __FUNCTION__, uuid.c_str());
    string partition = UUID2Device(uuid);

    isyslog("%s uuid: %s partition: %s", __FUNCTION__, uuid.c_str(), partition.c_str());
    return PartitionOfDevice(device_.name, partition);
}


void cMenuFormatDisk2::StopActivityOnMediadevice()
{
    /* 1. disable live buffer
     * 2. stop all recordings
     * 3. stop all recording playback
     * 4. restart nfs server! ** lsof does not show any activity in 'device_' **
     */

#ifdef USE_LIVEBUFFER
    // disable livebuffer
    Setup.LiveBuffer = 0;
    // stop livebuffer
    cRecordControls::SetLiveChannel(NULL, NULL);
#endif /*USE_LIVEBUFFER*/

    // stop all recordings;
    // also skip any timer that is going to start soon (30mins) ?
    time_t now = time(NULL);
    for (cTimer *timer = Timers.First();
         timer;
         timer = Timers.Next(timer) )
    {
        // stop recordings
        if (timer->Recording())
            timer->Skip();

        // timer starting soon ? Skip it so that it does not
        // create problems using hdd in the subsequent steps
        if (timer->StartTime() >= now
                && timer->StartTime() < now + 30*60) // starts in the next 30mins?
            timer->Skip();
    }
    cRecordControls::Process(time(NULL));
    Timers.SetModified();

    // stop recording playback
    cControl *control = dynamic_cast<cReplayControl*> (cControl::Control());
    if (control)
        cControl::Shutdown();

    // restart nfs server, so that /media/hd can be umounted!
    SystemExec("/etc/init.d/nfs-kernel-server restart");
}

void cMenuFormatDisk2::GetState()
{
    //if(state_ != idle) //test
    //state_ = filesystemwritten;  //test
    //return; //test

    std::string state = GetLastScriptState();

    switch (state_)
    {
    case writingfilesystem:
        if(state == "success")
        {
            state_ = filesystemwritten;
        }
        else if(state == "failure")
        {
            state_ = errorwritingfilesystem;
        }
        break;
    default:
        break;
    }
}

eOSState cMenuFormatDisk2::ProcessKey(eKeys Key)
{
    if(!HasSubMenu())
    {
	GetState();
	if(state_ != lastState_ )
	{
		if(state_ == formatted)
		{
			MountDisk(state_, device_.name);
		}
		else if(state_ == filesystemwritten)
		{
			IntegrateDisk(state_);
		}
		Set();
		lastState_ = state_;
	}
    }

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (returnToSetup_)
    {
        return osBack;
    }

    if (!HasSubMenu())
    {
        if (state == osUnknown)
        {
            switch (Key)
            {
            case kRed:
                //if(Current() != -1 && std::string(Get(Current())->Text()) == std::string("\t") + tr("Start formatting"))
                if(Current() != -1 && std::string(Get(Current())->Text()) == tr("\tFormat disk"))
                {
                    return StartFormatDiskMenu();
                }
                else if(Current() != -1 && std::string(Get(Current())->Text()) == tr("\tCreate boot disk"))
                {
                    if (IsMediadevice())
                    {
                        if(!Interface->Confirm(tr("All recordings will be lost! Continue anyway ?")))
                            return osContinue;

                        StopActivityOnMediadevice();
                    }

                    return StartCreateBootDiskMenu();
                }
                else if(state_ == integrated)
                {
                    returnToSetup_ = true;
                    return osBack;
                }
                //fall through
            case kOk:
                if(Current() != -1 && (std::string(Get(Current())->Text()) == tr("\tFormat disk") || 
                                       std::string(Get(Current())->Text()) == tr("\tCreate boot disk")) )
                {
                    Skins.Message(mtInfo, tr("Press the red key to continue"));
                }
                else if(Current() != -1 && std::string(Get(Current())->Text()) == tr("\tMount harddisk"))
                {
                    state_ = formatted; //will trigger WriteFileSystem
                }
                else if(Current() != -1 && std::string(Get(Current())->Text()) == tr("\tCancel"))
                {
                    return osBack;
                }
                break;

            default:
                if (state_ != idle)
                    state = osContinue;
                break;
            }
        }
        if(state == osContinue)
        {
            switch (Key)
            {
                case kUp:
                case kDown:
                case kRight:
                case kLeft:
                    Set();

                default:
                    break;
            }
        }
    }
    return state;
}
