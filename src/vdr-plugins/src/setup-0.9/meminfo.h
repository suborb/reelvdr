/*
 *
 *
 */


#ifndef __MEMINFO_H
#define __MEMINFO_H


#include <fstream>
#include <iostream>
#include <sstream>              // zeicheketten streams
#include <string>
#include <vector>

#include <vdr/tools.h>          // provides esyslog
#include <vdr/config.h>

#define PROC_MEMINFO "/proc/meminfo"
#define PROC_IDE_DEV "/proc/ide/hd%c/model"
#define PROC_SCSI_DEV "/proc/scsi/scsi"

class cMemInfo
{
  private:
    std::string memory;
    std::vector < std::string > driveInfo;
    const std::string cleanMemSize(const std::string);
    std::string SubString(const std::string& line, const char *start, const char *end);
    void AddValidSCSI(const std::string& line);
    void GetDriveInfo(); 
  public:
    cMemInfo();
    const std::string Memory();
    std::vector < std::string > &DriveModels();
    int NumDrives();
};

// inline Funktions
inline const std::string cMemInfo::Memory()
{
    return memory;
}

inline std::vector<std::string> &cMemInfo::DriveModels()
{
    //printf("%s\n", __PRETTY_FUNCTION__);
    //printf(" Num Drives  %d \n", driveInfo.size());    //cast
    return driveInfo;
}

inline int cMemInfo::NumDrives()
{;
    return driveInfo.size();    //cast
}

#endif // __MEMINFO_H
