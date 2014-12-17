#ifndef __RIPIT_STATUS_H__
#define __RIPIT_STATUS_H__
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <vdr/thread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vdr/plugin.h>

using namespace std;

inline void NotifyBgProcess(std::string name, std::string desc, time_t st_time, int percent_done)
{
    /***
     * for background process list
     * **/
    struct
    {
        std::string name;
        std::string desc;
        time_t t;
        float percent;
    } AProcess;

    AProcess.name = name;
    AProcess.desc = desc;
    AProcess.t = st_time;       // start time
    AProcess.percent = percent_done;

    cPlugin *Plugin = cPluginManager::GetPlugin("bgprocess");
    if (Plugin)
    {
        Plugin->Service("bgprocess-data", &AProcess);
        //cout << "Sent bgprocess-data " << desc << " " << percent_done << "%" << endl;
    }
}


class cRipitStatus:public cThread
{
    string logFileName, description;
    struct stat fstats;
    time_t old_time, startTime;
    int percentage_done;
    int ripCount, encodeCount;
    int totalTracks;
  public:
      cRipitStatus(string filename, string desc);
    void Action();
    int StatusRip();
    void Abort();
};

#endif
