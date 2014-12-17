
#ifndef BG_PROCESS_SERVICE_H
#define BG_PROCESS_SERVICE_H

#include <string>
#include <time.h>
#include <map>

using namespace std;

struct BgProcessData_t
{
    std::string processName;
    std::string processDesc;
    time_t startTime;
    float percent;
    uint pid;
    std::string caller;
  /**
   * percent completed
   *  < 0 don't/cannot know the percentage
   *  0-100 normal percent meaning
   *  >100  process finished and to be removed from the Back Ground Process list
   */
};

struct BgProcessCallerInfo_t
{
    std::string callerName;
    bool calledFrom;
};

inline void NotifyBgProcess(std::string name, std::string desc, time_t st_time, int percent_done, uint pid = 0, std::string  caller = "")
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
	uint pid; 
	std::string  caller;
    } AProcess;

    AProcess.name = name;
    AProcess.desc = desc;
    AProcess.t = st_time;       // start time
    AProcess.percent = percent_done;
    AProcess.pid = pid;
    AProcess.caller = caller;

    cPlugin *Plugin = cPluginManager::GetPlugin("bgprocess");
    if (Plugin)
    {
        Plugin->Service("bgprocess-data", &AProcess);
        //cout << "Sent bgprocess-data " << desc << " " << percent_done << "%" << endl;
    }
}

/*inline void GetBgProcessProgress(std::string name, std::string desc, time_t st_time, float &percent_done)
{
    struct
    {
        std::string name;
        std::string desc;
        time_t t;
        float &percent;
    } 
    AProcess =
    {
	name,
	desc,
	st_time,       // start time
	percent_done
    };

    cPlugin *Plugin = cPluginManager::GetPlugin("bgprocess");
    if (Plugin)
    {
        Plugin->Service("bgprocess-getprogress", &AProcess);
        //cout << "Sent bgprocess-data " << desc << " " << percent_done << "%" << endl;
    }  
}*/  


inline void GetBgProcessProgress(std::string name, std::string desc, std::string caller, /*time_t st_time,*/ float &percent_done)
{
    time_t st_time;
    
    struct
    {
        std::string name;
        std::string desc;
        time_t t;
        float percent;
	uint pid; 
	std::string  caller;
    } AProcess =
    {
	name,
	desc,
	st_time,
	0.0,
        0,
        caller
    } ; 

    cPlugin *Plugin = cPluginManager::GetPlugin("bgprocess");
    if (Plugin)
    {
        Plugin->Service("bgprocess-getprogress", &AProcess);
        //cout << "Sent bgprocess-data " << desc << " " << percent_done << "%" << endl;
    }
    //printf("GetBgProcessProgress, percent_done= %f\n", AProcess.percent);
    percent_done = AProcess.percent;
}

inline bool HasProcessFromCaller(std::string caller)
{
    struct
    {
	std::string caller;
        bool called;
    } 
    AProcess =
    {
	caller,
	false
    };
    
    cPlugin *Plugin = cPluginManager::GetPlugin("bgprocess");
    if (Plugin)
    {
        Plugin->Service("bgprocess-testifcalledfromplugin", &AProcess);
        //cout << "Sent bgprocess-data " << desc << " " << percent_done << "%" << endl;
    }  
    return AProcess.called;
}  
#endif
