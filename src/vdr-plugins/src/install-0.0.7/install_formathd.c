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
 * install_formathd.c
 *
 ***************************************************************************/

#include "install_formathd.h"
#include <vdr/sysconfig_vdr.h>

/********************************************************************************
 * Tools
 ********************************************************************************/

std::string MakeProgressBarStr(float percent, int length)
{
    // makes [|||||    ] for given percent and length of string

    if (length <= 0)
    {
        esyslog("[xinemediaplayer] (%s:%i) MakeProgressBarStr(percent=%f, length=%i)", __FILE__, __LINE__, percent, length);
        return "";
    }

    if (percent > 100)
        percent = 100;
    else if (percent < 0)
        percent = 0;

    int p = (int)(percent / 100 * length);
    std::string str = "[";
    str.insert(str.size(), length, ' ');    // fill with ' '

    if (p > 0)
        str.replace(1, p, p, '|');  // replace with appropriate '|'

    str.append("]");

    return str;
}

#include <unistd.h>
#include <stdlib.h>

/**
 * @brief PartitionHasRecordingsDir
 * check for '/recordings' directory in the given device
 * mount the device to a temporary location and check
 * for 'MOUNT_POINT/recordings'
 *
 * @param dev : the partition device
 * @return true if '/recordings' directory is found in the partition
 */
bool PartitionHasRecordingsDir(const char* dev)
{
   char tempdir_template [] = "/tmp/mount_XXXXXX"; // template
   char* tempdir = mkdtemp(tempdir_template);

   if (!tempdir) return false;
   printf("temp dir: '%s' %p template '%s' %p\n", tempdir, tempdir,
          tempdir_template, tempdir_template);

   
   cString cmd;
   
   /** run filesystem check before trying to mount the device; 
    *  non-interactive (-a) and do not run fsck on mounted filesystems (-M)*/
   cmd = cString::sprintf("fsck -a -M  %s", dev);
   SystemExec(cmd);
   
   /** mount the given device on to the temp dir */
   cmd = cString::sprintf("mount %s %s", dev, tempdir);
   SystemExec(cmd);


   /** check for MOUNTPOINT/recordings/. : makes sure there is a /recordings
    *  dir instead of a file*/
   cString rec_dir = cString::sprintf("%s/%s/.", tempdir/*mount point*/, "recordings" );
   printf("checking for recording directory .... '%s'\n", *rec_dir);
   bool result = (access(rec_dir, R_OK|W_OK|X_OK) == 0);
   printf("... result = %d\n", result);

   /** clean up */

   printf("--- lets umount '%s' %p\n", tempdir, tempdir);

  // unmount

   /// XXX TODO Why does this result in a segfault!!
   /*
   cmd = cString::sprintf("umount %s", tempdir);
  printf("'%s'\n", *cmd); */


  char umount_cmd[64];
  snprintf(umount_cmd, 63, "umount %s", tempdir);
  SystemExec(umount_cmd);

  // remove temp directory
  remove(tempdir);

 return result;
}



/**
 * @brief IsSuitableFileSystem
 *   check given filesystem string against supported/suitable filesystems
 *   jfs, ext3, ext4, reiserfs
 * @param fs : filesystem type eg. 'jfs'
 * @return true if filesystem is supported
 */

bool IsSuitableFileSystem(std::string fs)
{
    if ("jfs" == fs || "ext3" == fs || "ext4" == fs || "reiserfs" == fs)
        return true;

    return false;
}


/**
 * @brief IsDeviceMediadevice : check if the given device is used as for
 *        media or recordings in reelbox.
 *   Checks sysconfig's MEDIADEVICE variable
 * @param dev : device '/dev/sdb1'
 * @return true is sysconfig's MEDIADEVICE's UUID matches given parameter 'dev's UUID
 */
bool IsMediaDevice(std::string dev)
{
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();

    const char * device = sysconfig.GetVariable("MEDIADEVICE");
    printf("MEDIADEVICE: '%s'\n", device);
    /* no mediadevice set, so no recordings or maybe no harddisk */
    if (!device) return false;

    std::string mediadev_uuid = device;

    /* if it contains 'UUID=', remove this prefix */
    const std::string UUID_PREFIX = "UUID=";
    std::string::size_type idx = mediadev_uuid.find(UUID_PREFIX);
    if (idx != std::string::npos)
        mediadev_uuid = mediadev_uuid.substr(idx + UUID_PREFIX.size());

    std::string uuid = GetUUID(dev);
    printf("GetUUID('%s') == '%s'\n", dev.c_str(), uuid.c_str());
    printf("mediadevice_uuid == '%s'\n", mediadev_uuid.c_str());

    return uuid == mediadev_uuid;
} // IsMediaDevice()


#include "disk_tools.h"
#include <sstream>
/**
 * @brief HDDHasMediaDevice : find if any partition is a used as mediadevice
 * @param device  : eg '/dev/sdc'
 * @return
 */
bool HDDHasMediaDevice(std::string device)
{
    cPartitions partitions;
    bool result = GetPartitions(device, partitions);

    // error finding partition info
    if (!result)
        return false;

    result = false;

    cPartitions::const_iterator it = partitions.begin();
    while ( partitions.end() != it && !result)
    {
        // find partitions device name '/dev/sdc3'
        std::stringstream stream;
        stream << it->deviceName << it->number ;

        /* is it mediadevice ?*/
        result = IsMediaDevice(stream.str());

        ++it;
    } // while

    return result;
}


/**
 * @brief IsReelboxHarddisk : check partitions of given harddisk for /recordings dir
 * @param device : harddisk device '/dev/sdc'
 * @return true if the largest partition is in a compatible filesystem and
 *         has /recordings directory and the partition is copied to 'recording_partition'
 */
bool IsReelboxHarddisk(std::string device, std::string& recordings_partition)
{
    cPartitions partitions;
    bool result = GetPartitions(device, partitions);

    // error finding partition info
    if (!result)
    {
        esyslog("%s:%d GetPartitions('%s') failed", __FILE__, __LINE__, device.c_str());
        return false;
    }
    isyslog("%s:%d GetPartitions('%s') got %d partitions",
            __FILE__, __LINE__, device.c_str(), partitions.size());

    cPartitions::const_iterator it;
    it = FindLargestMountablePartition(partitions);

    if (it == partitions.end())
    {
        esyslog("%s:%d Could not find largest partition of device '%s' : partitions count %d",
                __FILE__, __LINE__, device.c_str(), partitions.size());
        return false;
    }
    isyslog("%s:%d Largest partition of device '%s'  is %s",
            __FILE__, __LINE__, device.c_str(), it->ToString().c_str());

    printf("Largest partition: %s\n", it->ToString().c_str());

    // suitable filesystem ?
    if ( !IsSuitableFileSystem(it->filesystem) )
    {
        esyslog("%s:%d '%s' is not a suitable filesystem", __FILE__, __LINE__,
                it->filesystem.c_str());
        return false;
    }
    isyslog("%s:%d Largest partition has a 'nice'' filesystem '%s' ", __FILE__, __LINE__,
            it->filesystem.c_str());

    // find partitions device name '/dev/sdc3'
    std::stringstream stream;
    stream << it->deviceName << it->number ;

    std::string partition = stream.str();
    printf(" --- test partition has recordings dir: '%s'\n", partition.c_str());
    result = PartitionHasRecordingsDir(partition.c_str());

    isyslog("Partition '%s' has rec dir? %d", partition.c_str(), result);

    if (result)
        recordings_partition = partition;

    return result;
} // IsReelboxHarddisk()



// converts bytes to the appropriate higher unit
// input: 10982 output: 10 KB
int convert_from_bytes(long long int inp, char *const units)
{
#define UNIT 1000
    const char *Units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
    int neg = 0;
    if (inp < 0)
    {
        inp = -inp;
        neg = 1;
    }

    int count = 0;
    while (inp > UNIT)
    {
        ++count;
        inp /= UNIT;
    }

    if (count >= 0)
        strcpy(units, Units[count]);

    return neg ? -(int)inp : (int)inp;
}                               // convert_from_bytes


// returns size of disk got from /sys
// input: sdb output: 1029222 //in blocks
long long int size_of_disk(const char *hdd)
{
    //read "/sys/block/<hdd>/size"
    if (!hdd)
        return 0;

    char buffer[32];
    snprintf(buffer, 31, "/sys/block/%s/size", hdd);
    buffer[31] = 0;

    FILE *fp = fopen(buffer, "r");

    if (!fp)
    {
        printf("Error opening file: %s\n", buffer);
        return 0;
    }

    long long int s = 0;
    if (fscanf(fp, " %lld", &s) == EOF)
    {
        printf("Error reading file: %s\n", buffer);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return s;
}                               //size_of_disk


//returns size as "192 KB"
const char *size_of_disk_str(const char *const hdd)
{
    static char buffer[32];
    char size_units[8];
    size_units[0] = 0;

    long long int sizeOfDisk = size_of_disk(hdd);   // in blocks

    int sizeOfDisk_compacted = convert_from_bytes(sizeOfDisk * 512, size_units);

    snprintf(buffer, 31, "%d %s", sizeOfDisk_compacted, size_units);
    return buffer;
}

/*****************************************************************************/

enum
{ eNotFormatting, eFormatOnGoing, eFormatComplete, eFormatError };
#define FORMAT_STATUS_LOG "/tmp/disk_format_status.log"
int CheckFormatStatus()
{

    FILE *fp = fopen(FORMAT_STATUS_LOG, "r");

    if (!fp)
        return eNotFormatting;

    cReadLine readline;

    const char *str = readline.Read(fp);

    if (str)
    {
        printf("CheckFormatState: '%s'\n", str);
        if (strstr(str, "complete"))
            return eFormatComplete;
        else if (strstr(str, "ongoing"))
            return eFormatOnGoing;
        else if (strstr(str, "error"))
            return eFormatError;
    }

    return eNotFormatting;
}

cInstallFormatHD::cInstallFormatHD(std::string harddiskDevice_) :
    cInstallSubMenu(tr("Harddisk"))
{
    if (harddiskDevice_.find("/dev/") != std::string::npos)
        harddiskDevice = harddiskDevice_.substr(5);
    else {
        esyslog("harddiskDevice_ wrong? '%s'", harddiskDevice_.c_str());
        harddiskDevice = "";
    }

    hddState = eUnknownHDD;

    rec_partition = "";
    bool isUnusedReelHDD = IsReelboxHarddisk(harddiskDevice_, rec_partition);
    if (isUnusedReelHDD)
        hddState = eUnusedReelHDD;

    printf("\033[0;91mcInstallFormatHD: xxxx Hardidisk: '%s' is Old reelHDD ? %d rec_partition '%s'\033[0m\n", harddiskDevice.c_str(),
           isUnusedReelHDD, rec_partition.c_str());


    bool isHDDMediadevice = HDDHasMediaDevice(harddiskDevice_);
    if (isHDDMediadevice)
        hddState = eAlreadyInUseHDD;
    printf("\n\n\033[7;92mDoes harddisk '%s' have mediadevice? %d\033[0m\n", harddiskDevice_.c_str(), isHDDMediadevice);

    std::string strBuff;

    // Get device name
    Modelname_ = "Unknown";

    strBuff = std::string("/sys/block/") + harddiskDevice + "/device/model";
    FILE *file = fopen(strBuff.c_str(), "r");
    if (file)
    {
        cReadLine readline;
        strBuff = readline.Read(file);
        if (strBuff.size())
            Modelname_ = strBuff;
        fclose(file);
    }

    SecureLock_ = true;
    Running_ = false;
    Finish_ = false;
    Set();
}

cInstallFormatHD::~cInstallFormatHD()
{
}

void cInstallFormatHD::Set()
{
    switch (hddState)
    {
    case eUnknownHDD:
        if (SecureLock_)
            SetUnknownHarddisk();
        else
            SetAskUserForConfirmation();
        break;

    case eUnusedReelHDD:
        SetUnusedReelHardisk();
        break;

    case eFormatInProgressHDD:
        SetHarddiskFormatting();
        break;

    case eAlreadyInUseHDD:
        SetHardiskAlreadyInUse();
        break;

    default:
        // error
        break;
    }
}

#define MAX_LETTERS_PER_LINE 50
void cInstallFormatHD::SetUnknownHarddisk()
{
    Clear();

    AddFloatingText(tr("Internal harddisk found."), MAX_LETTERS_PER_LINE);
    AddFloatingText(tr("Press OK to format now, otherwise the yellow buttown 'skip'."), MAX_LETTERS_PER_LINE);
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));


    Add(new cOsdItem(tr("Note:"), osUnknown, false));
    AddFloatingText(tr("If this is the first usage of the harddisk, it must be formatted now. "
                        "Formatting will DELETE ALL data on the harddisk.\n "
                        "If you have used this harddisk before, you could skip this step."), MAX_LETTERS_PER_LINE);

    Display();
}

#define osUseWithoutFormatting osUser4
#define osUseAfterFormatting   osUser5
#define osDonotUseHDD          osUser6

void cInstallFormatHD::SetUnusedReelHardisk()
{
    Clear();
    SetCols(5);
    AddFloatingText(cString::sprintf(tr("The disk '%s' was already used before and may contain recordings"
                                        " and multimedia files."), Modelname_.c_str()), MAX_LETTERS_PER_LINE);
    Add(new cOsdItem("", osUnknown, false)); // blank line
    AddFloatingText(tr("Do you want to mount it and use it as a device for your media files?"), MAX_LETTERS_PER_LINE);

    Add(new cOsdItem("", osUnknown, false)); // blank line
    Add(new cOsdItem(tr("\tYes, mount it, preserve all recordings"), osUseWithoutFormatting, true));
//    Add(new cOsdItem(tr("   - format and use"), osUseAfterFormatting, true));

//    Add(new cOsdItem("", osUnknown, false)); // blank line
    Add(new cOsdItem(tr("\tNo, do not use the harddisk at all"), osDonotUseHDD, true));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    AddFloatingText(tr("Note:"), MAX_LETTERS_PER_LINE);
    AddFloatingText(tr("You should mount this disk now if you want to use it for permanent timeshift"
                       " even if you are not going to use it for your media files."), MAX_LETTERS_PER_LINE);

    Display();
}

void cInstallFormatHD::SetHardiskAlreadyInUse()
{
    Clear();
    AddFloatingText(cString::sprintf(tr("The disk '%s' is already in use for media files."), Modelname_.c_str()), MAX_LETTERS_PER_LINE);
    AddFloatingText(tr("No action needs to be taken."), MAX_LETTERS_PER_LINE);

    Display();
}


void cInstallFormatHD::SetAskUserForConfirmation()
{
    Clear();
    char strBuff[64];
    snprintf(strBuff, 63, "%s: %s (%s)", tr("Harddisk"), Modelname_.c_str(), size_of_disk_str(harddiskDevice.c_str()));
    Add(new cOsdItem(strBuff, osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    AddFloatingText(tr("Press red key, if you want to format this harddisk:"), MAX_LETTERS_PER_LINE);
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));


    Add(new cOsdItem(tr("CAUTION:"), osUnknown, true));
    AddFloatingText(tr("If you choose to format, ALL data on the internal harddisk will be deleted!"), MAX_LETTERS_PER_LINE);



    SetHelp(tr("Format"), tr("Back"), tr("Skip"), NULL);

    Display();
}

/* shown when the formatting is done by the script preparehd.sh in the background */
void cInstallFormatHD::SetFormatting()
{
    Clear();
    long long int time_elapsed = time(NULL) - start_time;

    char strBuff[64];
    SetTitle(tr("Formatting in progress"));


    snprintf(strBuff, 63, "%s %s:", tr("Formatting"), tr("Harddisk"));
    Add(new cOsdItem(strBuff, osUnknown, false));

    snprintf(strBuff, 63, "%s (%s)", Modelname_.c_str(), size_of_disk_str(harddiskDevice.c_str()));
    Add(new cOsdItem(strBuff, osUnknown, false));

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    long long int sizeOfDisk = size_of_disk(harddiskDevice.c_str());  // in blocks, not bytes

    // Estimate formatting time on NetClient knowning 250B takes about 3 mins
    long long int estimatedTime = sizeOfDisk / (10000L);
    estimatedTime = 35;
    float percent = time_elapsed * 100.0 / estimatedTime;

    // since the percentage is an estimate, let us wait at 90% for the script to finish its work
    // just indicating that still something is going on
    if (percent > 90)
        percent = 90;

    snprintf(strBuff, 63, "%s %s", tr("Formatting in progress."), tr("Please wait ..."));
    Add(new cOsdItem(strBuff, osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(MakeProgressBarStr(percent, 50).c_str(), osUnknown, false));

    snprintf(strBuff, 63, "%s: %lld s / %lld s = %.2f %%", tr("Time elapsed"), time_elapsed, estimatedTime, percent);
    //snprintf(strBuff, 63, "%s: %lld s", tr("Time elapsed"), time_elapsed);
    Add(new cOsdItem(strBuff, osUnknown, false));

    SetHelp(NULL, NULL, NULL, NULL);
    Display();
}


void cInstallFormatHD::SetSuccessful()
{
    Clear();
    remove(FORMAT_STATUS_LOG);

    SetTitle(tr("Formatting complete"));

    Add(new cOsdItem(tr("Formatting was successful."), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr("The harddisk is now prepared for recording."), MAX_LETTERS_PER_LINE);
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr("Press 'Next' to proceed with the installation."), MAX_LETTERS_PER_LINE);

    SetHelp(NULL, NULL, tr("Next"), NULL);

    Display();
}


void cInstallFormatHD::SetNotFormating()
{
    Clear();
    remove(FORMAT_STATUS_LOG);

    Add(new cOsdItem(tr("Not Formatting!"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr("Please try again."), MAX_LETTERS_PER_LINE);

    SetHelp(NULL, tr("Back"), tr("Next"), NULL);
    Display();
}

void cInstallFormatHD::SetFormatingError()
{
    Clear();
    remove(FORMAT_STATUS_LOG);

    Add(new cOsdItem(tr("Formatting failed!"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    AddFloatingText(tr("Please try again."), MAX_LETTERS_PER_LINE);

    SetHelp(NULL, tr("Back"), tr("Next"), NULL);
    Display();
}

bool cInstallFormatHD::Save()
{
    return true;
}

void cInstallFormatHD::SetHarddiskFormatting()
{
    if (Running_)
    {
        //check if formatting is complete
        switch (CheckFormatStatus())
        {
        case eFormatComplete: {
            // goes into processKey
            char command[64];
            snprintf(command, 64, "preparehd.sh -s -m -d /dev/%s1", harddiskDevice.c_str());
            SystemExec(command);
            SystemExec("setup-mediadir restart");
            // formatting complete
            SetSuccessful();
            Finish_ = true;
            Running_ = false;
        }
            break;

        case eFormatOnGoing:
            SetFormatting();
            break;

        case eFormatError:
            SetFormatingError();

            Finish_ = true;
            Running_ = false;
            break;

        case eNotFormatting:
            SetNotFormating();
            break;
        } // switch
    } // if
}

eOSState cInstallFormatHD::ProcessKey(eKeys Key)
{
    switch(hddState)
    {
    case eUnknownHDD:
        return ProcessKey_UnknownHDD(Key);

    case eFormatInProgressHDD:
        return ProcessKey_formatting(Key);

    case eUnusedReelHDD:
        return ProcessKey_UnusedReelHDD(Key);

    case eAlreadyInUseHDD:
        return ProcessKey_HDDAlreadyInUse(Key);
    }
}


eOSState cInstallFormatHD::ProcessKey_UnknownHDD(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (NORMALKEY(Key) == kOk) {
        if (SecureLock_) // ask user for confirmation
        {
            SecureLock_ = false;
            Set();
        }
        return osContinue;
    } // kok

    if (!SecureLock_ && !Running_ && NORMALKEY(Key) == kRed) {
        // start formating

        hddState = eFormatInProgressHDD;
        Running_ = true;
        start_time = time(NULL);
        char command[64];

        // Format Harddisk
        snprintf(command, 64, "preparehd.sh --all -t jfs -d /dev/%s &", harddiskDevice.c_str());
        if (!SystemExec(command))   // 0 = no error
        {
            //Set();
            return osContinue;
        }
        else
        {
            Skins.Message(mtError, tr("Formating failed!"));
            return osContinue;
        }
    } // if kRed

    return state;
}

eOSState cInstallFormatHD::ProcessKey_HDDAlreadyInUse(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (NORMALKEY(Key) == kOk)
        state = osUser1; // next menu

    return state;
}

eOSState cInstallFormatHD::ProcessKey_UnusedReelHDD(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (NORMALKEY(Key) == kOk)
    {
        // formatting chosen
        if (state == osUseAfterFormatting)
        {
            hddState = eUnknownHDD;
            SecureLock_ = false;
            Set();
            state = osContinue;
        }
        else if (state == osUseWithoutFormatting)
        {
            // integrate without formatting
            // call prepare hdd
            char command[64];
            snprintf(command, 64, "preparehd.sh -s -m -d %s", rec_partition.c_str());
            SystemExec(command);
            SystemExec("setup-mediadir restart");
            Skins.Message(mtInfo, tr("Mounting harddisk ..."));
            
            state = osUser1; //next menu
        }
        else if (state == osDonotUseHDD)
        {
            state = osUser1; // next menu
        }

    } // if kOk

    return state;
}

eOSState cInstallFormatHD::ProcessKey_formatting(eKeys Key)
{

    eOSState state = cOsdMenu::ProcessKey(Key);

    // refresh screen to show progress bar
    Set();

    if (state == osUnknown)
    {
        if (Finish_)
        {
            if (Key == kOk)
                return osUser1; // Next menu
            else
                return state;
        }

    }
    else
        return osContinue;

    return osUnknown;
}
