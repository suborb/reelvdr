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

#include "MenuFormatDisk.h"
#include "MenuFormatDisk2.h"
#include "CheckServer.h"

#include "vdr/remote.h"
#include "vdr/thread.h"

#include <sstream>

#define MAX_LETTER_PER_LINE 46

//---------------------------------------------------------------------
//----------------------------cScsiDeviceItem-------------------------
//---------------------------------------------------------------------

cScsiDeviceItem::cScsiDeviceItem(std::string name, const char *Text, eOSState State, bool Selectable)
:cOsdItem(Text, State, Selectable), name_(name)
{
}

//---------------------------------------------------------------------
//----------------------------cMenuFormatDisk-------------------------
//---------------------------------------------------------------------

cMenuFormatDisk::cMenuFormatDisk()
:cOsdMenu(tr("Prepare disk: Choose disk")), returnToSetup_(false)
{
    checkServer.Start();
    SetCols(3, 2, 17, 10, 10);
    devices_.LoadDevices();
    Set();
    //printf("devices_.Get().size() = %d devices_.HasBootDevice() = %d\n", devices_.Get().size(), devices_.HasBootDevice());

    if (devices_.Get().size() == 2 &&  devices_.HasBootDevice() ||
        devices_.Get().size() == 1 && !devices_.HasBootDevice() )
    {
        cRemote::Put(kOk);
    }

    // turn off livebuffer
    /*::Setup.LiveBuffer = 0;
    Channels.SwitchTo(::Setup.CurrentChannel);

    if (procinfo != NULL)
    {
        int length = fread(hdname, 1, 256, procinfo);
        fclose(procinfo);
        if (length > 0)
            hdpresent = true;
    }
    if (!hdpresent)
        ::strcpy(pt, tr("none"));
    hdnames[0] = hdname;*/
}

cMenuFormatDisk::~cMenuFormatDisk()
{
    checkServer.Stop();
    //::Setup.LiveBuffer = LiveBufferTmp;
}

void cMenuFormatDisk::Set()
{
    if(devices_.Get().size() == 1 &&  devices_.HasBootDevice() ||
        devices_.Get().size() == 0 && !devices_.HasBootDevice() )
    {
        AddFloatingText(tr("Sorry. No applicable storage medium was detected."), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("", osContinue, false)); //blank line;
    }
    else
    {
        AddFloatingText(tr("The following storage media have been detected. Please select the medium to be configured:"), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("", osContinue, false)); //blank line;
    }

    for (uint i = 0; i< devices_.Get().size(); ++i)
    {
        std::ostringstream out;
        out << devices_.Get()[i].size;

        if(devices_.Get()[i].isBootDevice)
        {
            continue; //do not show boot device!
        }

        std::string name = "\t" + devices_.Get()[i].description + "\t" + out.str() + " GB";
        if(devices_.Get()[i].hasMountedRecordingsDir)
        {
            name = std::string("\t>") + name;
            //name += "\t";
            //name += devices_.Get()[i]. mountpoint;
        }
        else
            name = std::string("\t") + name;

        if(!devices_.Get()[i].hasRecordingsDir)
        {
            name += "\t";
            name += char(132);
        }
        cScsiDeviceItem *item = new cScsiDeviceItem(devices_.Get()[i].name, name.c_str());
        Add(item);
    }

    Add(new cOsdItem("",osUnknown, false));
    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    Add(new cOsdItem(tr("Info:"), osContinue, false));

    std::string option = std::string(">\t - ") + tr("is already mounted on the system");
    Add(new cOsdItem(option.c_str(), osContinue, false));
    option = char(132) + std::string("\t - ") + tr("has not yet been configured for the ReelBox");
    Add(new cOsdItem(option.c_str(), osContinue, false));
}

eOSState cMenuFormatDisk::FormatDisk()
{
    if(Current())
    {
        std:: string name = static_cast<cScsiDeviceItem *>(Get(Current()))->name_;
        cScsiDevice device;
        if(devices_.GetDeviceByName(name, device))
        {
            return AddSubMenu(new cMenuFormatDisk2(device, returnToSetup_));
        }
    }
    return osContinue;
}

eOSState cMenuFormatDisk::ProcessKey(eKeys Key)
{
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
            case kOk:
                return FormatDisk();

            default:
                break;
            }
        }
    }
    return state;
}
