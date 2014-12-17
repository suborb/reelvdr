
//vdr includes
#include "interface.h"
#include "debug.h"
#include "menu.h"
#include "menushutdown.h"
#include "plugin.h"
#include "remote.h"
#include "shutdown.h"

//system includes
#include <limits.h>
#include <signal.h>


// --- cMenuShutdown --------------------------------------------------------
#define SHUTDOWNMENU_TIMEOUT (10*1000) // 10s

cMenuShutdown::cMenuShutdown
  ()
  //(int &interrupted, eShutdownMode &shutdownMode, bool &userShutdown,time_t &lastActivity)
  //(int &interrupted)
:cOsdMenu(tr("Power Off ReelBox"))
  //,userShutdown_ (userShutdown), lastActivity_ (lastActivity), shutdown_(false)
{
    printf("\033[1;92m-----------cMenuShutdown::cMenuShutdown--------------\033[0m\n");
    //SetNeedsFastResponse(true);
    expert = false;
    quickShutdown_ = false;
    standbyAfterRecording_ = false;

    Set();
    timer_.Start(SHUTDOWNMENU_TIMEOUT);
}

cMenuShutdown::~cMenuShutdown()
{
    printf("------------------cMenuShutdown::~cMenuShutdown()-----------------\n");
    if(!shutdown_)
    {
        CancelShutdown();
    }

    if(quickShutdown_ )
    {
        //if (!cQuickShutdownCtl::IsActive())
        if (ShutdownHandler.IsUserInactive())
        {
            if (cRecordControls::Active() && standbyAfterRecording_)
            {
                userShutdown_ = true; lastActivity_ = 1;
            } // needs to be done (again) since, CancelShutdown() resets userShutdown_

            int nProcess = 0;
	    BgProcessInfo info;
            memset(&info, 0, sizeof(info));
            //DL Haven't seen a plugin that handles this call!?!
            //DL But I have also implemented this in vdr.c checking for kPower without this menu
            if(cPluginManager::CallAllServices("get bgprocess info", &info))
            {
                nProcess = info.nProcess;
            }
            if(nProcess > 0 )
            {
                userShutdown_ = true; lastActivity_ = 1;
            }

            //Skins.Message(mtInfo, tr("Activating QuickStandby"));
            if(cRecordControls::Active())
                Skins.Message(mtInfo, tr("Recording - activating Standby"));

            if(nProcess > 0 )
                Skins.Message(mtInfo, tr("Running Processes - activating Standby"));


            cControl::Shutdown();
            //cControl::Launch(new cQuickShutdownCtl);
	    //TODO: bool On=true; cPluginManager::CallAllServices("SuspendMode", &On);

            /* clean the callPlugin "cache" from cRemote class
             * so that later calls to cRemote::CallPlugin() are not blocked
             */
            cRemote::GetPlugin();
        }
        //else
            //printf("\033[1;91m QuickShutdown Already active \033[0m\n");
    }
    //printf("\033[1;91m-----------cMenuShutdown::~cMenuShutdown--------------\033[0m\n");
}

eOSState cMenuShutdown::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;
    state = cOsdMenu::ProcessKey(Key);
    DDD("state: %d key: %d", state, Key);

    if (Key != kNone)
        timer_.Start(SHUTDOWNMENU_TIMEOUT); // restart timeout Timer

    if(timer_.TimedOut()) // if recording then quick shutdown; else standby
    {
    /* if recording running, then go into quickshutdown and
     *  after the recording is over go into standby*/

       /*  //standby is hot-standby : so no need to wait for recordings or timers or any background process
        if (cRecordControls::Active() || Setup.StandbyOrQuickshutdown == 1) // StandbyOrQuickshutdown == 1 ==> no Standby
        {
            //TODO
            DDD("quickShutdown_: true");
            quickShutdown_ = true;
            standbyAfterRecording_ = true;
            return osEnd;
        }
        else */
            return Shutdown(standby);
    }

    if(state == osUnknown)
    {
        switch (Key)
        {
            case kOk:
                if(std::string(Get(Current())->Text()).find(tr("Standby")) == 2)
                {
                    int nProcess = 0;
                    BgProcessInfo info;
                    memset(&info, 0, sizeof(info));
                    if(cPluginManager::CallAllServices("get bgprocess info", &info))
                    {
                        nProcess = info.nProcess;
                    }
                    //if (!cRecordControls::Active() && Setup.StandbyOrQuickshutdown == 0 && nProcess == 0)
                        return Shutdown(standby);

                    quickShutdown_ = true;
                    standbyAfterRecording_ = true;
                    //cQuickShutdownCtl::afterQuickShutdown = 1; // standby
                    return osEnd;

                }
                else if(std::string(Get(Current())->Text()).find(tr("Power off")) == 2)
                {
                   // if (!cRecordControls::Active())
                    return Shutdown(poweroff);

                    /*
                    quickShutdown_ = true;
                    standbyAfterRecording_ = true;
                    //cQuickShutdownCtl::afterQuickShutdown = 2; // deepstandby
		    return osEnd;
                    */
                }
                else if(std::string(Get(Current())->Text()).find(tr("Reboot")) == 2)
                {
                    return Shutdown(restart);
                }
                else if(std::string(Get(Current())->Text()).find(tr("Set Sleeptimer")) == 2)
                {
                    userShutdown_ = false; //let k_Plugin work
                    cRemote::CallPlugin("sleeptimer");
                }
                else if(std::string(Get(Current())->Text()).find(tr("Maintenance Mode")) == 2)
                {
                    DDD("requested: Maintenence, going to end VDR");
                    return Shutdown(maintenance);
                }
            break;

            case kBack:
                state = osEnd;
            case kBlue:
                expert = !expert;
                Set();
            default:
                state = osContinue;
                break;
        }
    }
    return state;
}

eOSState cMenuShutdown::Shutdown(eActiveMode mode)
{
    bool doChangeActiveMode = true;
    bool bgProcessActive  = !ShutdownHandler.ConfirmShutdown(false);
    userShutdown_ = true;  //FIXME: should come from extern

    std::string msg;
    if (mode == standby)
    {
        DDD("requested: Standby, entering non-interactive mode");
        shutdownMode_ = standby;
        msg = tr("Activating standby");
    }
    else if (mode == poweroff)
    {
        DDD("requested: Power off, going to shut down");
        shutdownMode_ = poweroff;
        msg = tr("Activating deep standby");
        if (bgProcessActive)
        {
            msg = tr("PowerOff will happen after background processes are complete");
        }
    }
    else if (mode == maintenance)
    {
        DDD("requested: Maintenance, going to shut down");
        msg = tr("Entering maintenance mode");
        shutdownMode_ = maintenance;
    }
    else if (mode == restart)
    {
        DDD("requested: Restart, going to shut down");
        shutdownMode_ =  restart;
        msg = tr("Activating reboot");
    }

    if (mode == maintenance || mode == restart)
    {
        DDD("mode maintenance/restart: lets ask some questions");
        // if background process are active, ask user questions
        if (!ShutdownHandler.ConfirmShutdown(true))
            doChangeActiveMode = false;  // user cancelled

    }

    /*
    if ( ( cRecordControls::Active() && !Interface->Confirm(tr("Recording - shut down anyway?")) )
        || ( IsTimerNear() && !Interface->Confirm(tr("A recording starts soon, shut down anyway?")) ))

    {
        shutdown_ = false;
        doChangeActiveMode = false;
        //TODO: cQuickShutdownCtl::runScript = true;
        // CancelShutdown is run in the destructor
    }
    //else if (Interface->Confirm(msg.c_str(), userShutdown_ ? 2 : 180, true, true))
    //TODO: apply r6017 ignorePluginKey???
    else */
    if (doChangeActiveMode && Interface->Confirm(msg.c_str(), userShutdown_ ? 2 : 180, true))
    {
        shutdown_ = true;
        doChangeActiveMode = true;
    }
    else
        doChangeActiveMode = false;


    if(doChangeActiveMode)
    {
        ShutdownHandler.RequestNewMode(mode);

        //Turn off TV
        ShutdownHandler.SetUserInactive(); //SetActiveMode(standby)

        //prevent the countdown from appearing
        ShutdownHandler.SetRetry(INT_MAX);

    }
    else
    {
        // cancelled
        shutdownMode_ = active;
    }


DDD("return osEnd");
    return osEnd;
}

void cMenuShutdown::Set()
{
    Clear();
    
    char buf[256];
    sprintf(buf, "%d %s", Count()+1, tr("Standby"));
    Add(new cOsdItem(buf, osUnknown, true));

    // do not accidentally shutdown a MultiRoom Server
    if (expert || Setup.ReelboxMode != eModeServer) 
    {
        sprintf(buf, "%d %s", Count()+1, tr("Power off"));
        Add(new cOsdItem(buf, osUnknown, true));
    }
    
    sprintf(buf, "%d %s", Count()+1, tr("Set Sleeptimer"));
    Add(new cOsdItem(buf, osUnknown, true));
    
    if (expert) 
    {
        sprintf(buf, "%d %s", Count()+1, tr("Maintenance Mode"));
        Add(new cOsdItem(buf, osUnknown, true));

        sprintf(buf, "%d %s", Count()+1, tr("Reboot"));
        Add(new cOsdItem(buf, osUnknown, true));
    }

    // do not offer this option to users explicitly
    //sprintf(buf, "%d %s", 5, tr("Quick Standby"));
    //Add(new cOsdItem(buf, osUnknown, true));

    Display();
}

void cMenuShutdown::CancelShutdown()
{
    userShutdown_ = false;
    CancelShutdownScript();
}

void cMenuShutdown::CancelShutdownScript(const char *msg)
{
    // cancel external running shutdown watchdog
    //printf("----------cMenuShutdown::CancelShutdown-----------\n");
    char cmd[31];
    snprintf(cmd, 30, "shutdownwd.sh cancel");
    isyslog("executing '%s' (%s)", cmd, msg);
    SystemExec(cmd);
    //free(cmd);
}

bool cMenuShutdown::IsTimerNear()
/**
returns false if a recording is already running
    or if the box is a client: clients donot manage timers

returns true if next active timer starts within Setup.MinEventTimeout*60 secs

*/
{
    if ( !cRecordControls::Active() && Setup.ReelboxModeTemp != eModeClient )
    // If not recordings are active and if not a client:
    // clients donot manage timers, so safe to shut them down
    {
        cTimer *timer = Timers.GetNextActiveTimer ();
        time_t Next = timer ? timer->StartTime () : 0;
        time_t Delta = timer ? Next - time(NULL) : 0;

        return  Next && Delta <= Setup.MinEventTimeout * 60 ;
    }
    else
        return false;

}
