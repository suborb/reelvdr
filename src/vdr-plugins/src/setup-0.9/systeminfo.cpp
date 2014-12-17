/********************************************************************************
* systeminfo.c about menu part
* writen by markus Hahn @ reel-multimedia
*
* See REDME for copyright information and
* how to reach the authors.
*
********************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <string>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#ifndef DEVICE_ATTRIBUTES
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>             //??
#include <ctype.h>              //??
#include <errno.h>              //??

#include <vdr/tools.h>          ///??
#include <vdr/plugin.h>

#include "menusysteminfo.h"
#include "systeminfo.h"

#ifdef DEVICE_ATTRIBUTES
#include "../mcli/mcli_service.h"
#endif

#define IOCTL_REEL_TUNER_ID _IOR('d', 0x42, int)
#define TUNER_DEVICE  "/dev/reelfpga0"
#define FRONTEND_DEVICE  "/dev/dvb/adapter%d/frontend0"
#define MAX_TUNERS 6

using std::string;
using std::vector;
using std::ifstream;

// --- Class cSysInfo ----------------------------------------------

cSysInfo::cSysInfo() :build_("NA"), release_("NA"), version_("NA"), buildDate_("NA"), distribution_("NA")
{
    InitDvbInfo();
    InitReleaseInfo();
#ifdef RBLITE
    InitHDExtension();
#else
    InitRbAvantInfo();
#endif
#if 0
    std::cout << "distribution: " <<  distribution_ << std::endl;
    std::cout << "version:      " <<  version_ << std::endl;
    std::cout << "release:      " <<  release_ << std::endl;
    std::cout << "buildDate:    " <<  buildDate_ << std::endl;
    std::cout << "build:        " <<  build_ << std::endl;
#endif
#ifndef RBLITE
    InitAvantSystemHealth();
#endif
}

cSysInfo::~cSysInfo()
{
}

void cSysInfo::InitDvbInfo()
{
    //printf (" %s \n", __PRETTY_FUNCTION__);
    id1_ = 0;
    id2_ = 0;

    tunerstrings_[0] = "Single DVB-S";
    tunerstrings_[1] = "Single DVB-C";
    tunerstrings_[2] = "Single DVB-T";
    tunerstrings_[3] = "Single DVB-S2";
    tunerstrings_[4] = "?";
    tunerstrings_[5] = "?";
    tunerstrings_[6] = "?";
    tunerstrings_[7] = "?";
    tunerstrings_[8] = "Dual DVB-S";
    tunerstrings_[9] = "Dual DVB-S";
    tunerstrings_[10] = "?";
    tunerstrings_[11] = "?";
    tunerstrings_[12] = "?";
    tunerstrings_[13] = "?";
    tunerstrings_[14] = "?";
    tunerstrings_[15] = "?";
    tunerstrings_[16] = "?";
    tunerstrings_[17] = "?";
    tunerstrings_[18] = "?";
    tunerstrings_[19] = "?";
    tunerstrings_[20] = "?";
    tunerstrings_[21] = "?";
    tunerstrings_[22] = "?";
    tunerstrings_[23] = "?";
    tunerstrings_[24] = "?";
    tunerstrings_[25] = "?";
    tunerstrings_[26] = "?";
    tunerstrings_[27] = "?";
    tunerstrings_[28] = "?";
    tunerstrings_[29] = "?";
    tunerstrings_[30] = "?";
    tunerstrings_[31] = "-";

    // SetDevice();
#ifdef RBLITE
    int fd = open(TUNER_DEVICE, O_RDWR);
    if (fd > 0)
    {
        int ids = 0;
        ioctl(fd, IOCTL_REEL_TUNER_ID, &ids);
        id1_ = (ids) & 31;
        id2_ = (ids >> 8) & 31;

        close(fd);
    }
#else
    //printf("\033[0;41m  Get Device description \033[0m\n");
#ifndef DEVICE_ATTRIBUTES
    for (int t = 0; t < cDevice::NumDevices(); t++) {
        char fe[64];
        struct dvb_frontend_info info;
        snprintf(fe,64,FRONTEND_DEVICE,t);
        int  frontend = open(fe, O_RDONLY | O_NONBLOCK);
        if (frontend>= 0) {
             if (ioctl(frontend, FE_GET_INFO, &info) < 0) {
                printf("ERROR: cFemonOsd::Show() cannot read frontend info.\n");
             } else {
                frontendType_[t] = info.type;
                //printf(" %s Info %d name %s  \n", fe, info.type, info.name);
                if ( strstr(info.name, trNOOP("Unconfigured")) )
                    tunerDescription_.push_back(tr(info.name));
                else
                    tunerDescription_.push_back(info.name);
             }
        }
        close(frontend);
        frontend = -1;
     }
#else
    cPlugin *plug = cPluginManager::GetPlugin("mcli");
    if (plug && dynamic_cast<cThread*>(plug) && dynamic_cast<cThread*>(plug)->Active()) {
        mclituner_info_t info;
        for(int i=0; i<MAX_TUNERS_IN_MENU; i++)
            info.name[i][0] = '\0';
	    plug->Service("GetTunerInfo", &info);
        int cntrTuners = 0;
        for(int i = 0; i < MAX_TUNERS_IN_MENU; i++) {
            if(strlen(info.name[i]) == 0)
                break;
            else {
                frontendType_[cntrTuners] = (fe_type_t)info.type[cntrTuners];
                if (info.preference[i] == -1)
                    tunerDescription_.push_back(std::string("(") + tr(info.name[cntrTuners]) + ")");
                else
                    tunerDescription_.push_back(tr(info.name[cntrTuners]));
                cntrTuners++;
            }
        }
    } else {
        for (int t = 0; t < cDevice::NumDevices(); t++) {
            cDevice *cdev = cDevice::GetDevice(t);
            if (cdev) {
                uint64_t v=0;
                char name[54];
                int ret1 = cdev->GetAttribute("fe.name",name, sizeof(name));
                int ret2 = cdev->GetAttribute("fe.type",&v);
                if (ret1 == -1 || ret2 == -1)
                    continue;
                frontendType_[t] = (fe_type_t)v;
                if ( strstr(name, trNOOP("Unconfigured")) )
                    tunerDescription_.push_back(tr(name));
                else
                    tunerDescription_.push_back(name);
            }
        }
    }
#endif
#endif
}

// TODO test avantgarde handling on RB LITE   and remove old  version
const char *cSysInfo::TunerDescription(int TunerID) const
{
    //printf (" %s \n", __PRETTY_FUNCTION__);
#ifdef RBLITE
    switch (TunerID) {
        case 1:
            return tunerstrings_[id1_];
        case 2:
            return tunerstrings_[id2_];
        default:
            return "unknown tuner";  // XXX tr()
    }
#else
   if ((uint)TunerID < tunerDescription_.size()) {
      return tunerDescription_[TunerID].c_str();
   } else {
      esyslog("Setup: tunerDescription at tuner %d \n", TunerID);
      return "??";
   }

#endif
}

void cSysInfo::InitRbAvantInfo()
{
    //printf (" %s \n", __PRETTY_FUNCTION__);
    ifstream inFile("/etc/issue");

    string line, word;

    while (getline(inFile, line)) {
        //std::cout << "Distrie: " << line << std::endl;

        string::size_type pos = line.find("\\n");
        if (pos != string::npos) {
            distribution_ = line.substr(0, pos - 1);
            //std::cout << "Distribution: " << distribution_ << std::endl;
            pos = string::npos;
        }
    }

    inFile.close();

    inFile.open("/var/lib/dpkg/status");

    bool found = false;

    while (getline(inFile, line)) {
        string::size_type pos = line.find("Package: reelvdr-bin");

        if (pos != string::npos) {
            found = true;
            //std::cout << "line: " << line << std::endl;
            continue;
        }

        if (found) {
            string::size_type pos = line.find("Version:");
            if (pos != string::npos) {
                version_ = line.substr(string("Version:").size() +1);
                pos = string::npos;
                found = false;
                break;
            }
        }
    }
}

void cSysInfo::InitReleaseInfo()
{
    //printf (" %s \n", __PRETTY_FUNCTION__);

#ifdef RBLITE
    ifstream inFile("/etc/release");
#else
    ifstream inFile("/etc/reel/release");
#endif

    if (!inFile.is_open()) {
        esyslog("Setup: No \"/etc/reel/release\" found!");
        return;
    }

    string line, word;

    while (getline(inFile, line)) {
        std::istringstream splitStr(line);

        string::size_type pos = line.find("Distribution");
        if (pos != string::npos) {
            string::size_type pos = line.find("=");
            distribution_ = line.substr(pos + 1);
            pos = string::npos;
        }

        pos = line.find("Release");
        if (pos != string::npos) {
            string::size_type pos = line.find("=");
            release_ = line.substr(pos + 1);
            pos = string::npos;
        }
        pos = line.find("VersionString");
        if (pos != string::npos) {
            string::size_type pos = line.find("=");
            version_ = line.substr(pos + 1);
            pos = string::npos;
        }
        pos = line.find("Build");
        if (pos != string::npos) {
            //std::cout << " found  Build in line  " << line << std::endl;
            pos = line.find("BuildDate");
            if (pos != string::npos) {
                string::size_type pos = line.find("=");
                buildDate_ = line.substr(pos + 1);
                pos = string::npos;
            } else {
                string::size_type pos = line.find("=");
                build_ = line.substr(pos + 1);
                pos = string::npos;
            }
        }
    }
}

void cSysInfo::InitHDExtension() {
    //printf (" %s \n", __PRETTY_FUNCTION__);
    hasHDExtension_ = false;
    if (HasDevice("19058100")) {
        hasHDExtension_ = true;
        isyslog("Setup: HD-Extension found!\n");
    }
}

int cSysInfo::GetAvantTemperature(int mode)
{
        int fd,t;
        char buf[256];
        std::string sensor_mainboard;
        std::string sensor_cpu;

        if (HasDevice("808627a0")) {
            sensor_mainboard = string("/sys/devices/platform/w83627hf.2560/temp1_input");
            sensor_cpu       = string("/sys/devices/platform/w83627hf.2560/temp2_input");
        } else if (HasDevice("10027910")) {
            //sensor_mainboard = string("/sys/devices/pci0000:00/0000:00:18.3/temp2_input");
            sensor_mainboard = string("/sys/devices/platform/w83627ehf.2576/temp1_input");
            sensor_cpu       = string("/sys/devices/pci0000:00/0000:00:18.3/temp1_input");
        } else if (access("/sys/devices/platform/coretemp.0/temp1_input", R_OK != 0)) {
	    DDD("opening sensor device success");
	    sensor_cpu       = string("/sys/devices/platform/coretemp.0/temp1_input");
	    //sensor_mainboard = string(NULL);
	}
	else
	    DDD("opening sensor failed");

        if (!mode)
            fd = open(sensor_mainboard.c_str(), O_RDONLY);
        else
            fd = open(sensor_cpu.c_str(), O_RDONLY);

        if (fd < 0)
            return 0;

        if (read(fd,buf,256)) {
            t = atoi(buf);
            t = t/1000;
            close(fd);
            return t;
        }
        close(fd);
        return 0;
}

void cSysInfo::InitAvantSystemHealth(void)
{
    temps[0]=GetAvantTemperature(0);  //MB
    temps[1]=GetAvantTemperature(1);  //CPU
    // TODO: HD temp via smartctl, fan RPMs via reelboxctrld
}

