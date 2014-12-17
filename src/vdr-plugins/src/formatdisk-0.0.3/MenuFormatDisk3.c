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

#include "CheckServer.h"
#include "MenuFormatDisk3.h"
#include "stringtools.h"

#include <vdr/debug.h>
#include "vdr/plugin.h"
#include "vdr/thread.h"
#include <vdr/interface.h>

#include <iostream>
#include <fstream>

#define MAX_LETTER_PER_LINE 46

using namespace std;

const char* const logfile = "/tmp/mylog";

std::string GetUuid()
{
    //printf("------GetUuid()------\n");
    std::string uuid = "";
    char line[256];
    ifstream is(logfile, ifstream::in);
    if(is.good())
    {
        while(!is.eof())
        {
            is.getline (line,256);
            if (StartsWith(line, "UUID"))
            {
                uuid = line;
                //printf("------GetUuid() = %s------\n", uuid.c_str());
                break;
            }
        }
    }
    return uuid;
}

std::string GetLastScriptState()
{
    std::string state = "";
    char line[256];
    ifstream is(logfile, ifstream::in);
    if(is.good())
    {
        while(!is.eof())
        {
            is.getline (line,256);
            if (!StartsWith(line, "UUID") && (std::string(line) != ""))
            {
                state = line;
            }
        }
    }
    is.close();
    return state;
}

void StartPrepareHD(std::string option, std::string devicename)
{
    //printf("-----StartPrepareHD, device_.name = %s\n", device_.name.c_str());
    std::string cmd = "preparehd.sh " + option;
    cmd += " -d /dev/" + devicename;
    cmd += " -l /tmp/mylog";
    cmd += " &";
    DDD("command: %s", cmd.c_str());
    SystemExec(cmd.c_str());
}

void IntegrateDisk(formatState &state)
{
    //DDD("-----IntegrateDisk-------");
    std::string uuid = GetUuid();

    struct
    {
        std::string id;
    } setupMediaDevice =
    {
        uuid
    };

    cPluginManager::CallAllServices("Setup set mediadevice", &setupMediaDevice);
    state = integrated;
}

void FormatDisk(formatState &state, std::string devicename)
{
    state = formatting;
    StartPrepareHD("-f", devicename);
}

void WriteFileSystem(formatState &state, std::string devicename)
{
    state = writingfilesystem;
    StartPrepareHD("-b", devicename);
}

void MountDisk(formatState &state, std::string devicename)
{
    state = writingfilesystem;
    StartPrepareHD("-m -b", devicename);
}

void PartitionDisk(formatState &state, std::string devicename)
{
    state = partitioning;
    StartPrepareHD("-p", devicename);
}

void CreateBootDisk(formatState &state, std::string devicename, bool nojournal, bool cloneLocalDisk)
{
    state = creatingbootdisk;
    std::string options = nojournal ? " -o -y ": " -y ";
    if (cloneLocalDisk)
        options = nojournal ? " -o -e " : " -e ";
    StartPrepareHD(options, devicename);
}

void CreateMediaPartition(formatState &state, std::string devicename, bool nojournal)
{
    state = creatingmediapartition;
    std::string options = nojournal ? "-a -o" : "-a";
    StartPrepareHD(options, devicename + "3"); // "/dev/sdX" becomes "/dev/sdX3"
}

void CreateMediaDirectories(formatState &state, std::string devicename)
{
    state = creatingmediadirectories;
    StartPrepareHD(" -b ", devicename + "3"); // "/dev/sdX" becomes "/dev/sdX3"
}

void UpdateDefaultSystemSettings(formatState &state, std::string devicename)
{
    state = updatingdefaultsystemsettings;
    StartPrepareHD("-y2", devicename);
}

void Reboot()
{
    //printf("-----cMenuFormatDisk3::Reboot-------\n");
    std::string cmd = "shutdown -r now";
    DDD("command: %s", cmd.c_str());
    SystemExec(cmd.c_str());
}

/**
 * @brief MakeProgressBar
 * @param percent a string that may contain a number followed by '%' sign
 * @param result  if percent string was valid, then it contains a "[|||   ]"
 * @return
 */
bool MakeProgressBar(const std::string& percent, std::string& result)
{
    // handle '0%' seperately as atoi returns 0 for invalid/non-digit inputs
    if (percent == "0%")
    {
        result = "[" + std::string(100, ' ') +"]";
        return true;
    }

    int p = atoi(percent.c_str());
    if (p <= 0 || p > 100)
        return false;

    result = "[" + std::string(p, '|') + std::string(100-p, ' ') + "]";
    return true;
}


std::string GetProgressItem(std::string textbase, formatState state, formatState startstate, std::string info)
{
    std::string text = "";
    if(state == startstate)
    {
        std::string bar;
        bool isProgress = MakeProgressBar(info, bar);
        text = "\t" + textbase + (isProgress? "      " + info + "\t" + bar : "...");
    }
    else if(state == startstate + 1)
    {
        text = "\t" + textbase + "...\t[Error]";
    }
    else if(state >= startstate + 2 )
    {
        text = "\t" + textbase + " \t>";
    }
    return text;
}

//---------------------------------------------------------------------
//----------------------------cMenuFormatDisk3-------------------------
//---------------------------------------------------------------------

#define MEDIATYPE_STR tr("\tType of target disk")
#define IMGSRC_STR    tr("\tSource of software")

/* must match enum MediaType size and order */
//const char* cMenuFormatDisk3::mediaTypeStr[3];

cMenuFormatDisk3::cMenuFormatDisk3(cScsiDevice device, bool &returnToSetup, bool createBootDisk)
:cOsdMenu(tr("Prepare disk: perform actions")), state_(idle), lastState_(idle),
  returnToSetup_(returnToSetup), createBootDisk_(createBootDisk),
  mediaType(eMediaTypeUnknown), mediaTypeInt(1),
  imgSource(eImageSrcInternet), imgSourceInt(0)
{
    mediaTypeStr[0] = tr("Harddisk");
    mediaTypeStr[1] = tr("SSD");
    mediaTypeStr[2] = tr("USB Flash Drive");

    imgSourceStr[0] = tr("Internet");
    imgSourceStr[1] = tr("Local disk");

    SetCols(2, 28, 15);
    device_ = device;
    Set();
}

void cMenuFormatDisk3::AddInfoText(std::string currText)
{
    if (currText.empty()) return;

    Add(new cOsdItem("",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));
    AddFloatingText("___________________________________________", MAX_LETTER_PER_LINE);
    Add(new cOsdItem("",osUnknown, false));

    if (currText.find(MEDIATYPE_STR) != std::string::npos)
    {
	Add(new cOsdItem(tr("Info:"), osContinue, false));
        currText = std::string(tr("Some optimizations are made according to the type of the installation media. "
                                  "So please select the correct type of the device.")); // to be changed
        AddFloatingText(currText.c_str(), MAX_LETTER_PER_LINE);
        //AddFloatingText(cString::sprintf("chosen media type:%d", mediaTypeInt), MAX_LETTER_PER_LINE);
    }
    else if (currText.find(IMGSRC_STR) != std::string::npos)
    {
	Add(new cOsdItem(tr("Info:"), osContinue, false));
        currText = std::string(tr("If you select to install the image from local source, the "
				  "local harddisk oder USB-device will be copied. Select 'Internet' to "
				  "download and install a fresh software image from the net.")); // to be changed
        AddFloatingText(currText.c_str(), MAX_LETTER_PER_LINE);
        //AddFloatingText(cString::sprintf("chosen image source:%d", imgSourceInt), MAX_LETTER_PER_LINE);
    }
}

void cMenuFormatDisk3::SetCreateBootDisk()
{
    std::string currText;
    cOsdItem *curr = Get(Current());
    if (curr && curr->Text())
        currText = curr->Text();

    Clear();

    cOsdItem *mediaTypeItem;
    cOsdItem *imgSrcItem;

    bool online = checkServer.IsOnline();
    // if not online, image source defaults to local root disk
    if (!online)
    {
        imgSourceInt = 1; // local root
        imgSource = eImageSrcCurrentRoot;
    }

    AddFloatingText(tr("Please select the image source"), MAX_LETTER_PER_LINE);
    Add(imgSrcItem = new cMenuEditStraItem(IMGSRC_STR, &imgSourceInt, 2, imgSourceStr));

    // not online user has no choice but to clone the local disk
    if (!online)
        imgSrcItem->SetSelectable(false);

    Add(new cOsdItem("",osUnknown, false));
    AddFloatingText(tr("Please select your media type"), MAX_LETTER_PER_LINE);
    Add(mediaTypeItem = new cMenuEditStraItem(MEDIATYPE_STR, &mediaTypeInt, 3, mediaTypeStr));

    if (currText.find(MEDIATYPE_STR) != std::string::npos || !online)
        SetCurrent(mediaTypeItem);
    else
        SetCurrent(imgSrcItem);

    if (mediaType == eMediaTypeUnknown)
    {
        // refresh currText, it may have changed if network connection was lost during
        // menu redraw
        curr = Get(Current());
        if (curr && curr->Text())
            currText = curr->Text();
        else
            currText.clear();

        AddInfoText(currText);
        Display();
        return;
    }
    // else
    // user has choosen media type already, let it not be selectable anymore
    Clear(); // start a fresh osd...

    Add(mediaTypeItem = new cMenuEditStraItem(MEDIATYPE_STR, &mediaTypeInt, 3, mediaTypeStr));
    mediaTypeItem->SetSelectable(false);
    Add(imgSrcItem = new cMenuEditStraItem(IMGSRC_STR, &imgSourceInt, 2, imgSourceStr));
    imgSrcItem->SetSelectable(false);

    Add(new cOsdItem("",osUnknown, false));
    AddFloatingText(tr("Please wait, creating boot disk can take a while (up to 30mins). Do not close this menu."), MAX_LETTER_PER_LINE);
    Add(new cOsdItem("",osUnknown, false));

    std::string text;

    if (eImageSrcCurrentRoot == imgSource)
    {
        text = std::string("\t") + tr("checking devices") + std::string("\t>"); // dummy osditem; assuming no problems with devices
    }
    else if (online)
    {
        text = std::string("\t") + tr("connected to internet") + std::string("\t>");
    }
    else
    {
        text = std::string("\t") + tr("check internet connection!") + std::string("\t#");
    }

    Add(new cOsdItem(text.c_str(),osUnknown, false));

    text = GetProgressItem(tr("creating boot disk"), state_, creatingbootdisk, ongoing_str);
    if(!text.empty())
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    text = GetProgressItem(tr("creating media partitions"), state_, creatingmediapartition);
    if(!text.empty())
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    text = GetProgressItem(tr("creating media directories"), state_, creatingmediadirectories);
    if(!text.empty())
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    text = GetProgressItem(tr("Updating system settings"), state_, updatingdefaultsystemsettings);
    if(!text.empty())
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    if (state_ == updateddefaultsystemsettings)
    {
        Add(new cOsdItem("",osUnknown, false));
        AddFloatingText(tr("Boot disk created successfully.\nTo use the new boot disk please shutdown and "
                           "REMOVE the old USB stick/harddisk and start the ReelBox again."), MAX_LETTER_PER_LINE);
    }

    if(state_ == errorcreatingbootdisk || state_ == errorcreatingmediapartition
            || state_ == errorcreatingmediadirectories || state_ == errorupdatingdefaultsystemsettings)
    {
        Add(new cOsdItem("",osUnknown, false));
        Add(new cOsdItem("",osUnknown, false));
        AddFloatingText(tr("Creating boot disk failed. Please try again"), MAX_LETTER_PER_LINE);

        if (errorcreatingbootdisk)
        {
            AddFloatingText(tr("Please make sure the ReelBox is connected to the internet."), MAX_LETTER_PER_LINE);
        }

        SetHelp(NULL, NULL , NULL, NULL);
    }

    if (state_ == updateddefaultsystemsettings)
        SetHelp(tr("Shutdown"));
    Display();
}

void cMenuFormatDisk3::Set()
{
    if (createBootDisk_)
    {
        SetCreateBootDisk();
        return;
    }

    //printf("-------------cMenuFormatDisk2::Set()------------------\n");
    Clear();

    AddFloatingText(tr("Please wait, the setup for the storage media is running"), MAX_LETTER_PER_LINE);
    Add(new cOsdItem("",osUnknown, false));

    std::string text = "";

    //partitioning
    text = GetProgressItem(tr("partitioning"), state_, partitioning);
    if(text != "")
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    //formatting media
    text = GetProgressItem(tr("formatting"), state_, formatting);
    if(text != "")
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    //writing basic directory structure
    text = GetProgressItem(tr("installing basic structure"), state_, writingfilesystem);
    if(text != "")
    {
        Add(new cOsdItem(text.c_str(),osUnknown, false));
    }

    std::string textbase = tr("integrating disk");
    text = "";
    if(state_ == integrating)
    {
        text = "\t" + textbase + "...";
        Add(new cOsdItem(text.c_str(), osUnknown, false));
    }
    else if(state_ == integrated)
    {
        text = "\t" + textbase + " \t>";
        Add(new cOsdItem(text.c_str(), osUnknown, false));
        Add(new cOsdItem("",osUnknown, false));
        AddFloatingText(tr("Process done."), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("",osUnknown, false));
        AddFloatingText(tr("The storage medium has been sucessfully integrated."
                           " The harddisk is ready for use now."), MAX_LETTER_PER_LINE);
    }

    if(state_ == errorpartitioning || state_ == errorformatting || state_ == errorwritingfilesystem)
    {
        Add(new cOsdItem("",osUnknown, false));
        Add(new cOsdItem("",osUnknown, false));
        AddFloatingText(tr("The integration of the storage medium has failed. Please try again after rebooting the system."), MAX_LETTER_PER_LINE);
        SetHelp(tr("Button$Reboot"), NULL , NULL, NULL);
    }

    if(state_ == integrated)
    {
        //SetHelp(tr("Button$Reboot"), NULL , NULL, NULL);
        SetHelp(NULL, NULL , NULL, NULL);
    }
    Display();
}

void cMenuFormatDisk3::GetState()
{
    //state_ = integrating;  //test
    //return;//test

    std::string state = GetLastScriptState();

    switch (state_)
    {
    case formatting:
        if(state == "success")
        {
            state_ = formatted;
        }
        else if(state == "failure")
        {
            state_ = errorformatting;
        }
        break;
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
    case partitioning:
        if(state == "success")
        {
            state_ = partitioned;
        }
        else if(state == "failure")
        {
            state_ = errorpartitioning;
        }
        break;
    case creatingbootdisk:
        if(state == "success")
        {
            state_ = createdbootdisk;
        }
        else if(state == "failure")
        {
            state_ = errorcreatingbootdisk;
        }
        else if (state.find("ongoing") == 0)
        {
            // remain in creatingbootdisk state
            // boot disk creation on going
            // percent complete
            ongoing_str = state.substr(8);
        }
        //printf("creatingbootdisk: state='%s' ongoing_str='%s'\n", state.c_str(), ongoing_str.c_str());
        break;
     case creatingmediapartition:
        if(state == "success")
        {
            state_ = createdmediapartition;
        }
        else if(state == "failure")
        {
            state_ = errorcreatingmediapartition;
        }
        break;
     case creatingmediadirectories:
        if(state == "success")
        {
            state_ = createdmediadirectories;
        }
        else if(state == "failure")
        {
            state_ = errorcreatingmediadirectories;
        }
        break;
     case updatingdefaultsystemsettings:
        if(state == "success")
        {
            state_ = updateddefaultsystemsettings;
        }
        else if(state == "failure")
        {
            state_ = errorupdatingdefaultsystemsettings;
        }
            break;
    default:
        break;
    }
}

eOSState cMenuFormatDisk3::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    //printf("\033[94mState=%d Key=%d\033[0m, mediaType %d mediaTypeInt %d\n", state, Key, (int)mediaType, mediaTypeInt);

    // do not close the menu when a boot disk is being created
    if (state == osBack && (state_ == creatingbootdisk
                            || state_ == creatingmediapartition
                            || state_ == creatingmediadirectories
                            || state_ == updatingdefaultsystemsettings))
    {
        state = osContinue; // do not return osBack
        isyslog("ignoring key %d, in the process of creating boot disk", Key);
        Key = kNone;        // so that subsequently osBack is not returned from this function

        // do not return here; go through the state changes with GetState() etc...
    }

    GetState();
    if(state_ != lastState_ || state_ == idle || state_ == creatingbootdisk /* redrawn menu to show progress */)
    {
       if(state_ == idle)
       {
           if (createBootDisk_)
           {
               if (mediaType != eMediaTypeUnknown)
               {
                   CreateBootDisk(state_,
                                  device_.name,
                                  mediaType == eMediaTypeSSD || mediaType == eMediaTypeUSB,
                                  imgSource == eImageSrcCurrentRoot);
                   //printf("=========== \033[0;92mStarting to create boot disk mediaType %d\033[0m\n", (int)mediaType);
               }

           }
           else
           {
               PartitionDisk(state_, device_.name);
           }
       }
       else if(state_ == partitioned)
       {
           FormatDisk(state_, device_.name);
       }
       else if(state_ == formatted)
       {
           WriteFileSystem(state_, device_.name);
       }
       else if(state_ == filesystemwritten)
       {
           IntegrateDisk(state_);
       }
       else if (state_ == createdbootdisk)
       {
           CreateMediaPartition(state_, device_.name, mediaType == eMediaTypeSSD || mediaType == eMediaTypeUSB);
       }
       else if (state_ == createdmediapartition)
       {
           CreateMediaDirectories(state_, device_.name);
       }
       else if (state_ == createdmediadirectories)
       {
           UpdateDefaultSystemSettings(state_, device_.name);
       }

       // redraw menu always (to display info text and progress)
       //if (!createBootDisk_ || (createBootDisk_ && mediaType != eMediaTypeUnknown))
       Set();
       lastState_ = state_;
    }

    if (state_ == updateddefaultsystemsettings && Key == kRed
            && Interface->Confirm(tr("Shutdown now?")))
    {
        // message "please remove usb stick after shutdown"
        Skins.Message(mtInfo, tr("Please wait, shutting down"));

        // shutdown
        SystemExec("sync; shutdown -h now"); // vdr's shutdown menu ?
        return osContinue;
    }


    if (state == osUnknown)
    {
        switch (Key)
        {
        case kOk:
        {
            std::string text;
            if (Get(Current()) && Get(Current())->Text()) text = Get(Current())->Text();

            if (text.find(MEDIATYPE_STR) != std::string::npos
                    || text.find(IMGSRC_STR) != std::string::npos)
            {
                // set the mediatype, when redrawing the osd, creating boot disk starts
                mediaType = (MediaType) mediaTypeInt;
                imgSource = (ImageSource) imgSourceInt;
                return osContinue;
            }
            //printf("\033[0;91mMediaTypeInt %d\033[0m\n", mediaTypeInt);
        }
            returnToSetup_ = true;
            state = osBack;
            break;
        case kRed:
            //RC: disabled because reboot causes box to hang
            break;
            if (state_ == integrated || state_ == errorpartitioning || state_ == errorformatting || state_ == errorwritingfilesystem)
            {
                Reboot();
            }
            break;
        case kYellow:
            //returnToSetup_ = true;
            //state = osBack;
            break;
        default:
            state = osContinue;
            break;
        }
    }
    else
    {
        switch (Key)
        {
        case kBack:
            returnToSetup_ = true;
            state = osBack;
            break;
        default:
            break;
        }
    }
    return state;
}
