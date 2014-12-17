#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string>

#ifndef PROFILE_H
#define PROFILE_H

class cProfile
{
private:
    uint starttime;
    uint currenttime;
    uint timediff;
    uint startusage_u;
    uint currentusage_u;
    uint usagediff_u; 
    uint startusage_s;
    uint currentusage_s;
    uint usagediff_s;
    
    std::string name;
    
    struct timeval time;
    struct timezone tz;
    struct rusage usage;
    struct timeval ru_utime;
    struct timeval ru_stime;
    
public:
    cProfile(const char* name);
    cProfile();
    ~cProfile();
    void Init(const char* name);
    void Start();
    void Stop();
    void Update();
    void Print();
    void PrintAll();
    static void PrintTime(const char* name);
};


inline cProfile::cProfile(const char* name_)
: starttime (0), currenttime (0), timediff(0), startusage_u(0), currentusage_u(0), usagediff_u(0), 
  startusage_s(0), currentusage_s(0), usagediff_s(0), name (name_)
{
    Start();
}

inline cProfile::cProfile()
: starttime (0), currenttime (0), timediff(0), startusage_u(0), currentusage_u(0), usagediff_u(0), 
  startusage_s(0), currentusage_s(0), usagediff_s(0)
{
}

inline cProfile::~cProfile()
{
    Update();
    Print();
    //PrintAll();
}

inline void cProfile::Init(const char* name_)
{
   name  = name_;
} 

inline void cProfile::Start()
{
    gettimeofday(&time, &tz);
    starttime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
    currenttime = starttime;
     
    getrusage(RUSAGE_SELF, &usage);
    startusage_u = (usage.ru_utime.tv_sec%1000000)*1000 + usage.ru_utime.tv_usec/1000;
    startusage_s = (usage.ru_stime.tv_sec%1000000)*1000 + usage.ru_stime.tv_usec/1000;
    currentusage_u = startusage_u;
    currentusage_s = currentusage_s;
}

inline void cProfile::Stop()
{
    Update();
    Print();
    //PrintAll();
}

inline void cProfile::Update()
{
    gettimeofday(&time, &tz);
    currenttime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
    timediff += currenttime - starttime;
    //if (timediff >= 1000000)
    //    printf("%s: diff:%d, start:%d, current:%d, Pid=%d\n",name.c_str(),timediff, starttime, currenttime, ::getpid());
    
    getrusage(RUSAGE_SELF, &usage);
    currentusage_u = (usage.ru_utime.tv_sec%1000000)*1000 + usage.ru_utime.tv_usec/1000;
    currentusage_s = (usage.ru_stime.tv_sec%1000000)*1000 + usage.ru_stime.tv_usec/1000;
    usagediff_u += currentusage_u - startusage_u;
    usagediff_s += currentusage_s - startusage_s;       
}

inline void cProfile::Print()
{
    printf("%s, time = %d, timediff=%d\n", name.c_str(), currenttime, timediff);
}

inline void cProfile::PrintAll()
{
    printf("\n%s: PID = %d\n", name.c_str(), ::getpid());
    printf("Zeitdifferenz = %u\n", timediff);
    printf("Rechenzeit, User = %u, Kernel = %u\n", usagediff_u, usagediff_s); 
    printf("\n");
}

inline void cProfile::PrintTime(const char* name)
{
     struct timeval time;
     struct timezone tz;
     gettimeofday(&time, &tz);
     int currenttime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
     printf("%s, time = %d\n", name, currenttime);
}


/*
class cProfileThread : public cThread{
protected:
  void Action(void);
public:
  bool Start(void);
  bool Active(void);
  };
  
inline  void  cProfileThread::Action(void)
{
   for (int i=0; i<10; i++)
   

}
  
inline   void  cProfileThread::Start(void)
{


} 

 inline   void  cProfileThread::Active(void)
{


}*/

#endif  //PROFILE_H


