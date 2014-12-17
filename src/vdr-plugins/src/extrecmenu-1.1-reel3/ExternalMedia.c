#include <stdio.h>
#include <vector>
#include <string>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/vfs.h>    /* or <sys/statfs.h> */



#define MOUNTS_FILE "/var/run/usbmount/mounts"
#define MAX_LINE_LEN 512

static std::vector<std::string> ExtMediaMountPoints;
static std::vector<std::string> ExtMediaDevPaths;
static std::vector<std::string> ExtMediaNames;

static time_t LastReadTime = 0;
void ClearAllVar()
{
        ExtMediaMountPoints.clear();
        ExtMediaDevPaths.clear();
        ExtMediaNames.clear();
}


int ChangedAfterLastRead()
{
        if (access(MOUNTS_FILE, F_OK) != 0) // does not exist
            return -1;

        // File exists, get the m_time
        struct stat fileStat;
        if (stat(MOUNTS_FILE, &fileStat) != 0) // error in getting status
            return 1; // returning, "yes changed" ... once again read the file

        return LastReadTime < fileStat.st_mtime ;
}


int ReadMountsFile()
{
    switch(ChangedAfterLastRead())
    {
        case 0: // No changes
            return 1;
        case -1: // File does not exist
            ClearAllVar();
            return 1;
        default:
            // re-read file
            break;
    }

    ClearAllVar();

    FILE *fp = fopen(MOUNTS_FILE,"r");
    if ( !fp )  return 0; 

    char devPath[MAX_LINE_LEN];
    char mountPoint[MAX_LINE_LEN]; 
    char devName[MAX_LINE_LEN];

    int flag1, flag2, flag3;
    while(1)
    {
        flag1 = fscanf(fp, "%s ", devPath);         // eg. /dev/sdb1
        flag2 = fscanf(fp, "%s ", mountPoint);      // eg. /media/Kingstons_DATA_Traveller
        flag3 = fscanf(fp, "%s ", devName);         // eg. Kingston_Data_Traveller

        if (flag1 == EOF || flag2 == EOF || flag3 == EOF) // error in reading
            break;
        else // read successful
            {
                ExtMediaMountPoints.push_back(mountPoint);
                ExtMediaDevPaths.push_back(devPath);
                ExtMediaNames.push_back(devName);
            }
    }
    
    return 1;
}

int ExtMediaCount(bool refresh = true)
{
    if (refresh)
        ReadMountsFile();

    return ExtMediaMountPoints.size();
}

void PrintFile()
{
    int i;
    for (i = 0; i<ExtMediaCount() ; ++i)
    {
        printf("%i. '%s' '%s' '%s'\n",i+1, ExtMediaDevPaths.at(i).c_str(), 
                ExtMediaMountPoints.at(i).c_str(),
                ExtMediaNames.at(i).c_str()
              )  ; 
    }

}

const std::vector<std::string> ExtMediaMountPointList() { return ExtMediaMountPoints; }

int FileSystemSizeMB(const std::string& path)
{
    
    struct statfs sfs;

    if ( statfs(path.c_str(), &sfs) ) 
        return -1; // error
   
   return  (int)( sfs.f_blocks*(sfs.f_bsize / (1024.0*1024) ));

}
