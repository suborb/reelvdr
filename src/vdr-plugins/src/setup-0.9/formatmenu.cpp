#include "formatmenu.h"
#include <stdio.h>
#include <vdr/debug.h>
#include <vdr/menuitems.h>
#include <vdr/interface.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>

#define FORMAT_CMD "preparehd.sh"

#define FLOATING_WIDTH 60

std::string cFsDevice::LogFile()
{
    return logFileName_;
}

enum eFormattingStatus { eNotFormatting,
                         eFormattingInProgress,
                         eFormattingSuccess,
                         eFormattingFailure
};

/** reads log file of a particular device and returns
    formatting status
*/
int cFsDevice::IsFormatting()
{
    FILE* fp = fopen(LogFile().c_str(), "r");
    int state  = eNotFormatting;

    if (!fp)
        return state;

    cReadLine ReadLine;
    char *s = NULL;

    while((s = ReadLine.Read(fp)) != NULL)
    {
        if(strncasecmp(s, "start", 5) == 0)
            state = eFormattingInProgress;
        else if(strncasecmp(s, "success", 7) == 0)
            state = eFormattingSuccess;
        else if(strncasecmp(s, "failure", 7) == 0)
            state = eFormattingFailure;
    }

    fclose(fp);

    /*delete log file if success or failure*/
    if (state == eFormattingSuccess || state == eFormattingFailure)
    {
        if (unlink(LogFile().c_str()) == 0)
            dsyslog("unlinked (removed) format-log %s", LogFile().c_str());
        else
            dsyslog("unlinking format-log %s Failed", LogFile().c_str());
    }

    return state;
}

//------struct cFsDevice-------------------------------------------------------
void cFsDevice::SetProp(std::string dev, long long int size, long long int free)
{
    dev_        = dev;

    //partition type
#ifndef NO_JFS
    type_       = eRecFs; // jfs as default
#else
    type_       = eStandardFs;
#endif


    /** get disk model */
    /// /sys/block/sdX/device/model

    FILE *file = NULL;
    std::string command = std::string("/sys/block/sd") + dev_.at(7) + "/device/model";

    file = fopen(command.c_str(), "r");
    if (file)
    {
        cReadLine readline;
        char *line = readline.Read(file);
        if (line)
        {
            diskmodel_ = std::string(line);
            DDD("device model name: %s", diskmodel_.c_str());
        }
        fclose(file);
    }


    size_t pos = dev_.find("999");
    if (pos != std::string::npos) {
        availableSpace_ = size_ = 0;

        partitionMustBeCreated = true;
        dev_.erase(pos, 3);

        /** get disk size */
        //std::string
        command = std::string("/sys/block/sd") + dev_.at(pos-1) + "/size";
        file = fopen(command.c_str(), "r");
        if(file)
        {
            cReadLine readline;
            char *line = readline.Read(file);
            if(line)
                availableSpace_ = size_ = atoll(line)/(2*1024*1024);
            fclose(file);
        }
    } else {
        partitionMustBeCreated = false;
        size_       = size;
        availableSpace_ = free;
    }

    //set logfile name
    //construct the format log file, should be unique for each device
    std::string dev_name(dev_);
    //replace all '/' to '_'
    for (unsigned int i = 0 ; i < dev_name.length(); ++i)
        if(dev_name[i] == '/')
            dev_name[i] = '_';

    logFileName_ = std::string("/tmp/format") + dev_name + std::string(".log");

}

std::string cFsDevice::Type()const
{
    switch(type_)
    {
        case eStandardFs:       return "ext3";
#ifndef NO_JFS
        case eRecFs:            return "jfs";
#else
        case eRecFs:            return "xfs";
#endif
        case eWinCompatFs:      return "vfat";

        default:                return std::string(); //empty string
    }
}


//------class cFormatMenu-----------------------------------------------------
cFormatMenu::cFormatMenu(const char* deviceName, long long int size, long long int free) : cOsdMenu("Format")
{
    //DDD("%s deviceName %s", __PRETTY_FUNCTION__ ,deviceName);
    pageNum_    = eFormatStartPage;
    PartitionTypes_.Append(strdup(tr("Standard (ext3)")));
#ifndef NO_JFS
    PartitionTypes_.Append(strdup(tr("Recording (jfs)")));
#else
    PartitionTypes_.Append(strdup(tr("Recording (xfs)")));
#endif
    PartitionTypes_.Append(strdup(tr("Windows Compatible (vFat)")));

    fsDevice_.SetProp(deviceName, size, free);
    newPartitionType_ = fsDevice_.type_ ;

    Set();
}

cFormatMenu::~cFormatMenu()
{
}

void cFormatMenu::Set()
{
    switch(pageNum_)
    {
        case 0:
            // start page
            ShowStartPage();
            break;
        case 1:
            //confirm page
            ShowConfirmPage();
            break;

        case 2:
            // Format progress page

        case 3:
            //Format Finished page
            ShowFinishedPage();
            break;
        default:
            esyslog("Error: pageNum_ should not be %i", pageNum_);
            printf("Error: pageNum_ should not be %i\n", pageNum_);
            break;

    }
}

void cFormatMenu::ShowFinishedPage() {}

void cFormatMenu::ShowConfirmPage()
{
    Clear();

    SetTitle(cString::sprintf("%s: %s", tr("Format"), tr("Confirm formatting")));
    SetCols(20);


    char buffer[128];
    // device model
    snprintf(buffer, 128, "%s\t%s", tr("Name:"), fsDevice_.diskmodel_.c_str());
    Add(new cOsdItem(buffer, osUnknown, false));

    // device name
    snprintf(buffer, 128, "%s\t%s", tr("Device:"), fsDevice_.dev_.c_str());
    Add(new cOsdItem(buffer, osUnknown, false));

    if (fsDevice_.availableSpace_) // free space is reported as 0, if device is not mounted
    {
        snprintf(buffer, 128, "%s\t%llu GB", tr("Free space:"), fsDevice_.availableSpace_);
        Add(new cOsdItem(buffer, osUnknown, false));
    }
    snprintf(buffer, 128, "%s\t%llu GB", tr("Partition size:"), fsDevice_.size_);
    Add(new cOsdItem(buffer, osUnknown, false));
    snprintf(buffer, 128, "%s\t%s", tr("Filesystem:"), PartitionTypes_.At(newPartitionType_));
    Add(new cOsdItem(buffer, osUnknown, false));

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Warning:"), osUnknown, false));
    snprintf(buffer, 128, tr("Formatting will destroy all data on \"%s\"!"), fsDevice_.dev_.c_str());
    AddFloatingText(buffer, FLOATING_WIDTH);

    Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr("Press 'Red' to start formatting now or 'Exit' to cancel formatting."), FLOATING_WIDTH);

    Display();
    SetHelp(tr("Format"), tr("Back"), NULL, NULL);
}

void cFormatMenu::ShowStartPage()
{
    Clear();

    SetTitle(cString::sprintf("%s: %s", tr("Format"), tr("Select Filesystem Type")));
    SetCols(20);

    char buffer[128];

    // drive name OR volume name

    // device model
    snprintf(buffer, 128, "%s\t%s", tr("Name:"), fsDevice_.diskmodel_.c_str());
    Add(new cOsdItem(buffer, osUnknown, false));

    // device name
    snprintf(buffer, 128, "%s\t%s", tr("Device:"), fsDevice_.dev_.c_str());
    Add(new cOsdItem(buffer, osUnknown, false));


    if (fsDevice_.availableSpace_) // free space is reported as 0, if device is not mounted
    {
        snprintf(buffer, 128, "%s\t%llu GB", tr("Free space:"), fsDevice_.availableSpace_);
        Add(new cOsdItem(buffer, osUnknown, false));
    }
    snprintf(buffer, 128, "%s\t%llu GB", tr("Partition size:"), fsDevice_.size_);
    Add(new cOsdItem(buffer, osUnknown, false));

    Add(new cOsdItem("", osUnknown, false));

    Add(new cMenuEditStraItem(tr("Filesystem"), (int*)&newPartitionType_, eLastFs, &PartitionTypes_.At(0)));

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    //Add(new cOsdItem(tr("Warning:"), osUnknown, false));
    //AddFloatingText(tr("All files and folders will be lost after formatting"), FLOATING_WIDTH);
    AddFloatingText(tr("Please select the filesystem according to the porpuse of the device."), FLOATING_WIDTH);

    Add(new cOsdItem("", osUnknown, false));

    AddFloatingText(tr("Press 'Ok' to continue or 'Exit' to cancel now."), FLOATING_WIDTH);

    Display();
    SetHelp(NULL, NULL, NULL, NULL);
}

eOSState cFormatMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);
    if (state == osUnknown)
        switch(pageNum_)
        {
            case 0: return ProcessKeyStartPage(key);
            case 1: return ProcessKeyConfirmPage(key);
            case 2:
            case 3: return ProcessKeyFinishPage(key);
            default:
                return osContinue;
        }

    return state;
}

eOSState cFormatMenu::ProcessKeyStartPage(eKeys key)
{
    switch(key)
    {
        case kOk:
            fsDevice_.type_ = newPartitionType_;
            pageNum_++;
            Set();
            break;

        default:
            break;
    }
    return osContinue;
}

bool UserConfirmFormat()
{
    const char *msg = tr("Starting format in 3 seconds");

    //guard timegap of 3s
#if VDRVERSNUM < 10716
    eKeys user_key = Skins.Message(mtWarning, msg, 3, false);
#else
    eKeys user_key = Skins.Message(mtWarning, msg, 3);
#endif

    // Proceed with formatting by default, no response == YES!
    return  (user_key == kNone || user_key == kOk);
}

eOSState cFormatMenu::ProcessKeyConfirmPage(eKeys key)
{
    // read format log file and find out if formatting is complete
    int isFormatting = fsDevice_.IsFormatting();
    //printf("Formatting status %i\n", isFormatting);

    if (isFormatting == eFormattingInProgress) // formatting started
        SetStatus(tr("Formatting in progress, please wait..."));
    else
        SetStatus(NULL);
    switch (isFormatting)
    {
        case eFormattingInProgress:
            SetStatus(tr("Formatting in progress, please wait..."));
            break;
        case eFormattingSuccess:
            Skins.Message(mtInfo, tr("Formatting completed successfully."));
            return osBack;
            break;
        case eFormattingFailure:
            Skins.Message(mtError, tr("Error: formatting failed"));
            return osBack;
            break;
        default: break;
    }

    switch(key)
    {
        case kGreen:
            pageNum_--;
            Set();
            break;

        case kRed:
            {
            std::string path = std::string("/usr/sbin/") + FORMAT_CMD;
            if (access(path.c_str(), X_OK) != 0)
            {
                Skins.Message(mtError, tr("Unable to format. Missing command"));
                printf("Missing file '%s' or file not executeable\n", FORMAT_CMD);
                esyslog("Missing file '%s' or file not executeable", FORMAT_CMD);

                return osContinue;
            }

            if(isFormatting != eFormattingInProgress)
            {
                bool answer = UserConfirmFormat();
                if (answer)
                {

                    if (fsDevice_.partitionMustBeCreated)
                    {
                        //std::string command = std::string("LANG=\"C\" echo \"n\np\n1\n\n\nw\nq\n\" | fdisk 2>/dev/null 1>/dev/null ") + fsDevice_.dev_.c_str();
                        std::string command = std::string("/usr/sbin/") + FORMAT_CMD + " -p -d " + fsDevice_.dev_.c_str();
                        printf("command: \"%s\"\n", command.c_str());
                        SystemExec(command.c_str());
                        fsDevice_.dev_ += "1";
                    }

                    // format_device.sh /dev/sdxN ext3 /tmp/format_dev_sdxN.log
                    std::string format_cmd = std::string(FORMAT_CMD)
                        + " -d " + fsDevice_.dev_
                        + " -t " + fsDevice_.Type()
                        + " -f -b -l " + fsDevice_.LogFile() +" &";

                    printf("\033[7;91m'%s'\033[0m\n", format_cmd.c_str());
                    SystemExec(format_cmd.c_str());

                    Skins.QueueMessage(mtInfo, tr("Formatting started"));

                    //return osEnd;
                    break;
                    //return osContinue;
                }
                else
                    Skins.Message(mtInfo, tr("Canceled"));
            }
            break;
            }
        default:
            break;

    }

    return osContinue;
}

eOSState cFormatMenu::ProcessKeyFinishPage(eKeys key)
{
    return osContinue;
}

