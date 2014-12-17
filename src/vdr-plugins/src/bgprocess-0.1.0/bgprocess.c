/*
 * bgprocess.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "vdr/i18n.h"
#include <vdr/plugin.h>
#include "service.h"
#include <map>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "bgprocess.h"

//for pid-checking
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* dummy strings for gettext */
#if 0
trNOOP("Cutting");
#endif

using namespace std;
std::map < string, BgProcessData_t > bgProcessList;   // list of all BackGround Process running
std::map < string, BgProcessData_t > CompletedList;   // list of all BackGround Process finished
std::map < string, BgProcessData_t > FailedList;   // list of all BackGround Process failed
std::map < string, time_t > CompletedTimeList;  // list of all BackGround Process finished

static const char *VERSION = "0.1.0";
static const char *DESCRIPTION = trNOOP("Collects and lists background processes and their status");
static char MAINMENUENTRY[32] = trNOOP("Activity");

int nProcess = 0;
float avgPercent = 0.0;


// cBgProcessThread():

void cBgProcessThread::Init(cPluginBgprocess *plugin)
{
  plugin_ = plugin;
}

bool cBgProcessThread::PidExists(int pid)
{
  /**
   * This will be our file descriptor
   * from where we read command line
   */
  int fd;
  /**
   * Filename where command line is written to on /proc
   */
  char filename[255];
  /**
   * Buffer we read from filename (command line)
   */
  char buf[255];

  /**
   * Check if provided PID is valid
   */
  if (pid < 1 || pid > 65535){
    fprintf(stderr, "ERROR: PID number provided is not valid: %d\n", pid);
    return false;
  }

  /**
   * Build the filename
   */
  snprintf(filename, sizeof(filename), "/proc/%d/cmdline", pid);

  /**
   * Open filename for reading only
   */
  fd = open(filename, O_RDONLY);
  if (fd < 0){
    //fprintf(stderr, "ERROR: Cannot open filename: %s. Process is probably not running.\n", filename);
    close(fd);
    return false;
  }

  /**
   * Try to read contents of the file
   */
  if (read(fd, buf, sizeof(buf)) > 0){
    //printf("PID %d was found running: %s\n", pid, buf);
  }
  else {
    //fprintf(stderr, "No such process with Id: %d found in file: %s\n", pid, filename);
    return  false;
  }
  close(fd);
  return true;
}

void cBgProcessThread::Action()
{
  int num = 0;
  std::map < string, BgProcessData_t >::iterator iter;
  while (Running())
  {
    //printf("---------cBgProcessThread::Action(), while Running-----------\n");
    usleep(1000000);
    if(++num%10 == 0)
    {
      Lock();
      if(!bgProcessList.empty())
      for (iter = bgProcessList.begin(); iter != bgProcessList.end(); iter++)
      {
	  if (iter->second.pid != 0 && !PidExists(iter->second.pid) && CompletedList.find(iter->first) != FailedList.end())
	  {
	     //FailedList.insert(FailedList.end(), iter->second);
	     bgProcessList.erase(iter->first);
	  }
      }
      Unlock();
    }
  }
}

//cPluginBgProcess

const char * cPluginBgprocess::Version(void)
{
    return VERSION;
}

const char * cPluginBgprocess::Description(void)
{
    return tr(DESCRIPTION);
}

bool cPluginBgprocess::Start(void)
{
  bgProcessThread.Start();
  return true;
}

void cPluginBgprocess::Stop(void)
{
  bgProcessThread.Cancel();
}

const char * cPluginBgprocess::MainMenuEntry(void)
{
    nProcess = bgProcessList.size();
    avgPercent = 0.0;

    bgProcessThread.Lock();
    std::map < string, BgProcessData_t >::iterator it;
    for (it = bgProcessList.begin(); it != bgProcessList.end(); it++)
        if (it->second.percent > 0)
            avgPercent += it->second.percent;
    bgProcessThread.Unlock();

    if (nProcess > 0)
    {
        avgPercent /= nProcess;
        sprintf(MAINMENUENTRY, tr("%d background process (%d%%)"), nProcess, (int)avgPercent);
    }
    else
        return NULL;

    return MAINMENUENTRY;
}

bool cPluginBgprocess::HasSetupOptions(void)
{
    return false;
};


bool cPluginBgprocess::GetProcessProgress(BgProcessData_t *pData)
{
    /*BgProcessData_t data =
    {
      pData->processName,
      pData->processDesc,
      pData->startTime,
      pData->percent,
    };

    string key = GenKey(data);*/
    bgProcessThread.Lock();
    map<string, BgProcessData_t>::iterator iter = bgProcessList.begin();

    for (; iter != bgProcessList.end(); iter++)
    {
      if(iter->second.processName == pData->processName &&
	 iter->second.processDesc == pData->processDesc)
      {
	pData->percent = iter->second.percent;
	return true;
      }
    }
    bgProcessThread.Unlock();

    /*for (; iter != CompletedList.end(); iter++)
    {
      if(iter->second.processName == pData->processName &&
	 iter->second.processDesc == pData->processDesc)
      {
	pData->percent = 1.0;
	return true;
      }
    }*/

    /*if(bgProcessList.find(key) != bgProcessList.end())
    {
      pData->percent = bgProcessList.find(key)->second.percent;
    }*/
    /*else if(CompletedList.find(key) != CompletedList.end())
    {
      pData->percent = 1.0;
    }*/
    pData->percent = 100.0;

    return false;
}

bool cPluginBgprocess::AddProcess(BgProcessData_t pData)
{
    string key = GenKey(pData);
    /*map<string, BgProcessData>::iterator iter = bgProcessList.find(key);
       if (iter == bgProcessList.end()) // not found
       return false; */

    bgProcessThread.Lock();
    bgProcessList[key] = pData;
    bgProcessThread.Unlock();

    return true;
}

bool cPluginBgprocess::RemoveProcess(BgProcessData_t pData)
{
    string key = GenKey(pData);

    bgProcessThread.Lock();
    map < string, BgProcessData_t >::iterator iter = bgProcessList.find(key);
    if (iter == bgProcessList.end())    // not found
        return false;
    bgProcessList.erase(iter);

    // add to completed list
    CompletedList[key] = pData;
    CompletedTimeList[key] = time(NULL);
    bgProcessThread.Unlock();

    // compute new average for Display
    return true;
}

bool cPluginBgprocess::HasProcessFromCaller(BgProcessCallerInfo_t *pData)
{
  bgProcessThread.Lock();
  //printf("HasProcessFromCaller, pData->callerName) = %s\n", pData->callerName.c_str());
  map < string, BgProcessData_t >::iterator iter = bgProcessList.begin();
  for (; iter != bgProcessList.end(); iter++)
  {
    if(iter->second.caller == pData->callerName)
    {
      pData->calledFrom = true;
      return true;
    }
  }
  pData->calledFrom = false;
  return true;

  bgProcessThread.Unlock();
}

string cPluginBgprocess::GenKey(BgProcessData_t pData)
{
    std::ostringstream oss;
    oss << pData.processName << " " << pData.startTime; // converts name + time to a string
    std::string key(oss.str());
    return key;
}

cPluginBgprocess::cPluginBgprocess(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!

    bgProcessThread.Lock();
    bgProcessList.clear();      // start fresh
    bgProcessThread.Unlock();

    bgProcessThread.Init(this);
}

cPluginBgprocess::~cPluginBgprocess()
{
    // Clean up after yourself!
    bgProcessThread.Lock();
    bgProcessList.clear();
    bgProcessThread.Unlock();
}

const char *cPluginBgprocess::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginBgprocess::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginBgprocess::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

void cPluginBgprocess::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginBgprocess::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginBgprocess::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

cOsdObject *cPluginBgprocess::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    //return NULL;
    return new BgProcessMenu(&bgProcessThread);
}

cMenuSetupPage *cPluginBgprocess::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return NULL;
}

bool cPluginBgprocess::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return false;
}

bool cPluginBgprocess::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    if (strcmp(Id, "bgprocess-data") == 0)
    {
        if (!Data)
            return true;

        BgProcessData_t *newBgProcess = (BgProcessData_t *) Data;

        if (newBgProcess->percent > 100)    // process to be removed
            RemoveProcess(*newBgProcess);

        else if (newBgProcess->percent <= 100)
            AddProcess(*newBgProcess);  // should also update process info if 0 <= percent <= 100

#ifdef DEBUG
        // debugging START
        ofstream of;
        of.open("/tmp/bglist", ios::app);
        of << "Name:" << newBgProcess->processName << " Desc:" << newBgProcess->processDesc << " %:" << newBgProcess->percent << " startTime:" << newBgProcess->startTime << endl;
        of.close();
        // debugging END
#endif
        return true;
    }
    else if(strcmp(Id, "bgprocess-getprogress") == 0)
    {
        if (!Data)
            return true;

	GetProcessProgress((BgProcessData_t *) Data);
	return true;
    }
    else if(strcmp(Id, "bgprocess-testifcalledfromplugin") == 0)
    {
        if (!Data)
            return true;

        //BgProcessData2_t *newBgProcess = (BgProcessData2_t *) Data;

	HasProcessFromCaller((BgProcessCallerInfo_t *) Data);

	return true;
    }
    return false;
}

const char **cPluginBgprocess::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    static const char *HelpPages[] = {
        "PROCESS\n",
        NULL
    };
    return HelpPages;
}

cString cPluginBgprocess::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    //printf("cPluginBgprocess::SVDRPCommand################################################\n\n");
    //printf("cPluginBgprocess::SVDRPCommand# Option = %s\n", Option);
    // Process SVDRP commands this plugin implements
    if (strcasecmp(Command, "PROCESS") != 0)
        return NULL;


    int pid = 0;
    char caller[512];
    char name[512];             // plugin/process name
    char name2[512];
    char desc[512];             // description
    long long int t;            // time
    int perCent;
    int isSuccess = 1;


    int flag = sscanf(Option, "\"%s %s %lld %d %d %s %[^\n] ", name, name2, &t, &perCent, &pid, caller, desc);
    if (flag == 7)
    {
        strcat(name, " ");
        name2[strlen(name2) - 1] = '\0';
        strcat(name, name2);
    }
    else
    {
      flag = sscanf(Option, "\"%s %lld %d %d %s %[^\n] ", name, &t, &perCent, &pid, caller, desc);
      if (flag == 6)
      {
	  ;
      }
      else
      {
	caller[0]= 0;
	pid =0;
	flag = sscanf(Option, "\"%s %s %lld %d %[^\n] ", name, name2, &t, &perCent,desc);
	if (flag == 5)
	{
	  strcat(name, " ");
	  name2[strlen(name2) - 1] = '\0';
	  strcat(name, name2);
	}
	else
	{
	  flag = sscanf(Option, "%s %lld %d %[^\n] ", name, &t, &perCent, desc);
	  if (flag != 4)
	      isSuccess = 0;
	}
      }
    }
    name[511] = 0;
    desc[511] = 0;
    caller[511] = 0;

    BgProcessData_t *newBgProcess = new BgProcessData_t;
    newBgProcess->processName = name;
    newBgProcess->processDesc = desc;
    newBgProcess->percent = perCent;
    newBgProcess->startTime = t;
    newBgProcess->pid = pid;
    newBgProcess->caller = caller;

    if (isSuccess)
        isSuccess &= Service("bgprocess-data", newBgProcess);

    delete(newBgProcess);
    return isSuccess ? "OK" : "FAILED";
    //return cString::sprintf("%s %lld %d %s (%d)", name, t, perCent, desc, flag) ;
}


// ---- BgProcessMenu --------------------------------------

BgProcessMenu::BgProcessMenu(cBgProcessThread *bgProcessThread)
:bgProcessThread_(bgProcessThread), cOsdMenu(tr("Background processes"), 4, 10, 13)
{
    ShowBgProcesses();
    // Change this to last added
    //if (bgProcessList.size()) showMode = showRunning;
    //else showMode = showCompleted;
}

void BgProcessMenu::ShowCompleted()
{
#if APIVERSNUM < 10700
    SetCols(4, 15, 7);
#else
    SetCols(4, 14, 13);
#endif
    std::map < string, BgProcessData_t >::iterator iter = CompletedList.begin();
    std::map < string, time_t >::iterator iterTime = CompletedTimeList.begin();
    int i = 0;
    Clear();
    if (iter == CompletedList.end())
    {
        Add(new cOsdItem(" ", osUnknown, false));
        Add(new cOsdItem(tr(" No process completed"), osUnknown, false));
    }
    for (; iter != CompletedList.end(); iterTime++, iter++)
    {
        i++;
        std::ostringstream oss;
        struct tm *time_tm = localtime(&iter->second.startTime);
        oss << i << "\t" << tr(iter->second.processName.c_str()) << "\t" << time_tm->tm_hour << ":" << time_tm->tm_min;
        time_tm = localtime(&iterTime->second);
        oss << " - " << time_tm->tm_hour << ":" << time_tm->tm_min << "\t" << iter->second.processDesc;

        Add(new cOsdItem(oss.str().c_str(), osUnknown, false));
    }
    SetTitle(tr("Completed processes"));

    if (!CompletedList.empty())
        cOsdMenu::SetHelp(tr("Clear list"), NULL, NULL, tr("Running"));
    else
        cOsdMenu::SetHelp(NULL, NULL, NULL, tr("Running"));

    Display();
}


void BgProcessMenu::ShowBgProcesses()
{
    SetCols(4, 15, 7, 7);
    // loop through the list and display
    std::map < string, BgProcessData_t >::iterator iter = bgProcessList.begin();
    int i = 0;
    Clear();
    if (iter == bgProcessList.end() && CompletedList.size() == 0)
    {
        Add(new cOsdItem(" "), osUnknown, false);
        Add(new cOsdItem(tr(" No background process running"), osUnknown, false));
    }
    for (; iter != bgProcessList.end(); iter++)
    {
        i++;
        if (iter->second.percent < 0)   // donot show process with -ve percentage_done
            continue;

        // make the progress bar string [|||||    ]
        std::ostringstream oss;
        char per_str[100 + 3];
        per_str[102] = '\0';
        per_str[0] = '[';
        per_str[101] = ']';
        for (int j = 0; j < 100; j++)
            per_str[1 + j] = ' ';
        for (int j = 0; j < iter->second.percent; j++)
            per_str[1 + j] = '|';

        // calc hour and minute
        //struct tm *time_tm = localtime(&iter->second.startTime);

        //  oss<< i <<"\t"<<iter->second.processName<<"\t"<<per_str<<"\t"<<time_tm->tm_hour<<":"<<time_tm->tm_min<<"\t"<<iter->second.processDesc;
        oss << i << "\t" << tr(iter->second.processName.c_str()) << "\t" << per_str << "\t" << iter->second.percent << "%\t" << iter->second.processDesc;

        Add(new cOsdItem(oss.str().c_str(), osUnknown, false));
    }
    SetTitle(tr("Running processes"));

/** Completed process */
    iter = CompletedList.begin();
    std::map < string, time_t >::iterator iterTime = CompletedTimeList.begin();

    for (; iter != CompletedList.end(); iter++, iterTime++)
    {
        i++;
        std::ostringstream oss;
        struct tm *time_tm = localtime(&iter->second.startTime);
        oss << i << "\t" << tr(iter->second.processName.c_str()) << "\t" << time_tm->tm_hour << ":" << setw(2) << setfill('0') << time_tm->tm_min;
        time_tm = localtime(&iterTime->second);
        oss << " - " << time_tm->tm_hour << ":" << setw(2) << setfill('0') << time_tm->tm_min << "\t\t" << tr("Completed");

        Add(new cOsdItem(oss.str().c_str(), osUnknown, false));
    }


    //cOsdMenu::SetHelp(NULL,tr("Completed"), NULL , NULL);
    cOsdMenu::SetHelp(NULL, NULL, NULL, NULL);
    Display();
}

eOSState BgProcessMenu::ProcessKey(eKeys k)
{
    // if (showMode == showRunning) ShowBgProcesses();
    //else ShowCompleted();

    bgProcessThread_->Lock();
    ShowBgProcesses();
    bgProcessThread_->Unlock();
    eOSState state = cOsdMenu::ProcessKey(k);
//  switch (k)
//  {
//   case kBlue: showMode = showRunning; break;
//   case kGreen: showMode = showCompleted; break;
//   case kRed: if(showMode == showCompleted) CompletedList.clear(); break;
//   default: break;
//  }
    return state;
}

VDRPLUGINCREATOR(cPluginBgprocess); // Don't touch this!
