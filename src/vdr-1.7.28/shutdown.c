/*
 * shutdown.c: Handling of shutdown and inactivity
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version written by Udo Richter <udo_richter@gmx.de>.
 *
 * $Id: shutdown.c 2.0 2008/02/24 10:29:00 kls Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "debug.h"
#include "dummyplayer.h"
#include "i18n.h"
#include "interface.h"
#include "menu.h"
#include "menushutdown.h"
#include "osdbase.h"
#include "plugin.h"
#include "shutdown.h"
#include "timers.h"
#include "tools.h"

cShutdownHandler ShutdownHandler;

#ifdef REELVDR
cTimeMs standbyCountdown(INT_MAX);
#define WAKEUPFILE "/tmp/vdr.wakeup"
#endif

cCountdown::cCountdown(void)
{
  timeout = 0;
  counter = 0;
  timedOut = false;
  message = NULL;
}

void cCountdown::Start(const char *Message, int Seconds)
{
  timeout = time(NULL) + Seconds;
  counter = -1;
  timedOut = false;
  message = Message;
  Update();
}

void cCountdown::Cancel(void)
{
  if (timeout) {
     timeout = 0;
     timedOut = false;
     Skins.Message(mtStatus, NULL);
     }
}

bool cCountdown::Done(void)
{
  if (timedOut) {
     Cancel();
     return true;
     }
  return false;
}

bool cCountdown::Update(void)
{
  if (timeout) {
     int NewCounter = (timeout - time(NULL) + 9) / 10;
     if (NewCounter <= 0)
        timedOut = true;
     if (counter != NewCounter) {
        counter = NewCounter;
        char time[10];
        snprintf(time, sizeof(time), "%d:%d0", counter > 0 ? counter / 6 : 0, counter > 0 ? counter % 6 : 0);
        cString Message = cString::sprintf(message, time);
        Skins.Message(mtStatus, Message);
        return true;
        }
     }
  return false;
}

cShutdownHandler::cShutdownHandler(void)
{
  activeTimeout = 0;
  retry = 0;
  shutdownCommand = NULL;
  exitCode = -1;
  emergencyExitRequested = false;
#if REELVDR
  ActiveMode = unknown;
  requestedMode = active;
#endif
}

cShutdownHandler::~cShutdownHandler()
{
  free(shutdownCommand);
}

#if REELVDR
void cShutdownHandler::RequestEmergencyExit(void)
{
    dsyslog("No emergency exit in ReelVDR");
}
#else
void cShutdownHandler::RequestEmergencyExit(void)
{
  if (Setup.EmergencyExit) {
     esyslog("initiating emergency exit");
     emergencyExitRequested = true;
     Exit(1);
     }
  else
     dsyslog("emergency exit request ignored according to setup");
}
#endif

void cShutdownHandler::CheckManualStart(int ManualStart)
{
  time_t Delta = Setup.NextWakeupTime ? Setup.NextWakeupTime - time(NULL) : 0;

  if (!Setup.NextWakeupTime || abs(Delta) > ManualStart) {
     // Apparently the user started VDR manually
     dsyslog("assuming manual start of VDR");
     // Set inactive after MinUserInactivity
     SetUserInactiveTimeout();
#if REELVDR
     RequestNewMode(active);
#endif
     }
  else {
     // Set inactive from now on
     dsyslog("scheduled wakeup time in %ld minutes, assuming automatic start of VDR", Delta / 60);
#if REELVDR
     RequestNewMode(standby);
#endif
     SetUserInactive();
     }
}

void cShutdownHandler::SetShutdownCommand(const char *ShutdownCommand)
{
  free(shutdownCommand);
  shutdownCommand = ShutdownCommand ? strdup(ShutdownCommand) : NULL;
}

void cShutdownHandler::CallShutdownCommand(time_t WakeupTime, int Channel, const char *File, bool UserShutdown)
{
  time_t Delta = WakeupTime ? WakeupTime - time(NULL) : 0;

#ifdef REELVDR
  const char* modestring = " ";
  switch (ActiveMode)
  {
      case poweroff: // deep standby
        modestring = "deepstandby";
        break;

      case restart: // reboot
        modestring = "reboot";
        break;

      default:
        modestring = " ";
        break;
  }
  cString cmd = cString::sprintf("%s %ld %ld %d \"%s\" %d %s", shutdownCommand, WakeupTime, Delta, Channel, *strescape(File, "\\\"$"), UserShutdown, modestring);
#else

  cString cmd = cString::sprintf("%s %ld %ld %d \"%s\" %d", shutdownCommand, WakeupTime, Delta, Channel, *strescape(File, "\\\"$"), UserShutdown);
#endif

  isyslog("executing '%s'", *cmd);
  int Status = SystemExec(cmd, true);
  if (!WIFEXITED(Status) || WEXITSTATUS(Status))
     esyslog("SystemExec() failed with status %d", Status);
  else {
     Setup.NextWakeupTime = WakeupTime; // Remember this wakeup time for comparison on reboot
     Setup.Save();
     }
}

void cShutdownHandler::SetUserInactiveTimeout(int Seconds, bool Force)
{
#ifdef REELVDR
  if (!Setup.EnergySaveOn || (!Setup.MinUserInactivity && !Force)) {
#else
  if (!Setup.MinUserInactivity && !Force) {
#endif
     activeTimeout = 0;
     return;
     }
  if (Seconds < 0)
     Seconds = Setup.MinUserInactivity * 60;
  activeTimeout = time(NULL) + Seconds;
}

bool cShutdownHandler::ConfirmShutdown(bool Interactive)
{
#if REELVDR
  if ((!Interactive && !cRemote::Enabled()) || (ActiveMode != active && ActiveMode != poweroff))
#else
  if (!Interactive && !cRemote::Enabled())
#endif
     return false;
  if (!shutdownCommand) {
     if (Interactive)
        Skins.Message(mtError, tr("Can't shutdown - option '-s' not given!"));
     return false;
     }
  if (cCutter::Active()) {
     if (!Interactive || !Interface->Confirm(tr("Editing - shut down anyway?")))
        return false;
     }

  cTimer *timer = Timers.GetNextActiveTimer();
  time_t Next = timer ? timer->StartTime() : 0;
  time_t Delta = timer ? Next - time(NULL) : 0;

#if USEMYSQL
if (Setup.ReelboxModeTemp != eModeClient)
{
#endif
  if (cRecordControls::Active() || (Next && Delta <= 0)) {
     // VPS recordings in timer end margin may cause Delta <= 0
     if (!Interactive || !Interface->Confirm(tr("Recording - shut down anyway?")))
        return false;
     }
  else if (Next && Delta <= Setup.MinEventTimeout * 60) {
     // Timer within Min Event Timeout
     if (!Interactive)
        return false;
     cString buf = cString::sprintf(tr("Recording in %ld minutes, shut down anyway?"), Delta / 60);
     if (!Interface->Confirm(buf))
        return false;
     }
#if USEMYSQL
}
#endif

  if (cPluginManager::Active(Interactive ? tr("shut down anyway?") : NULL))
     return false;

  cPlugin *Plugin = cPluginManager::GetNextWakeupPlugin();
  Next = Plugin ? Plugin->WakeupTime() : 0;
  Delta = Next ? Next - time(NULL) : 0;
  if (Next && Delta <= Setup.MinEventTimeout * 60) {
     // Plugin wakeup within Min Event Timeout
     if (!Interactive)
        return false;
     cString buf = cString::sprintf(tr("Plugin %s wakes up in %ld min, continue?"), Plugin->Name(), Delta / 60);
     if (!Interface->Confirm(buf))
        return false;
     }
  return true;
}

bool cShutdownHandler::ConfirmRestart(bool Interactive)
{
  if (cCutter::Active()) {
     if (!Interactive || !Interface->Confirm(tr("Editing - restart anyway?")))
        return false;
     }

  cTimer *timer = Timers.GetNextActiveTimer();
  time_t Next  = timer ? timer->StartTime() : 0;
  time_t Delta = timer ? Next - time(NULL) : 0;

  if (cRecordControls::Active() || (Next && Delta <= 0)) {
     // VPS recordings in timer end margin may cause Delta <= 0
     if (!Interactive || !Interface->Confirm(tr("Recording - restart anyway?")))
        return false;
     }

  if (cPluginManager::Active(Interactive ? tr("restart anyway?") : NULL))
     return false;

  return true;
}


bool cShutdownHandler::DoShutdown(bool Force)
{
  time_t Now = time(NULL);
#if USEMYSQL
  cTimer *timer = NULL;
  if (Setup.ReelboxModeTemp != eModeClient) // Don't care about wakeup timer if this Reelbox is in Client mode
      timer = Timers.GetNextActiveTimer();
#else
  cTimer *timer = Timers.GetNextActiveTimer();
#endif
  cPlugin *Plugin = cPluginManager::GetNextWakeupPlugin();

  time_t Next = timer ? timer->StartTime() : 0;
  time_t NextPlugin = Plugin ? Plugin->WakeupTime() : 0;
  if (NextPlugin && (!Next || Next > NextPlugin)) {
     Next = NextPlugin;
     timer = NULL;
     }
  time_t Delta = Next ? Next - Now : 0;

  if (Next && Delta < Setup.MinEventTimeout * 60) {
     if (!Force)
        return false;
     Delta = Setup.MinEventTimeout * 60;
     Next = Now + Delta;
     timer = NULL;
     dsyslog("reboot at %s", *TimeToString(Next));
     }

  if (Next && timer) {
     dsyslog("next timer event at %s", *TimeToString(Next));
     CallShutdownCommand(Next, timer->Channel()->Number(), timer->File(), Force);
     }
  else if (Next && Plugin) {
     CallShutdownCommand(Next, 0, Plugin->Name(), Force);
     dsyslog("next plugin wakeup at %s", *TimeToString(Next));
     }
  else
     CallShutdownCommand(Next, 0, "", Force); // Next should always be 0 here. Just for safety, pass it.

  return true;
}


#if REELVDR
bool cShutdownHandler::RequestNewMode(eActiveMode mode)
{
    // check for last mode?
    if (requestedMode == mode)
    {
        DLOG("Requested mode (%d) is already active. Doing nothing", mode);
        return false;
    }

    requestedMode = mode;
    return true;
}

bool cShutdownHandler::ActOnActiveMode()
{
    if (requestedMode == ActiveMode)
    {
        return false;
    }
    DLOG("ActOnActiveMode(): Active mode(%d) changed to requestedMode (%d)", ActiveMode, requestedMode);

    // check for background activity
    if (ActiveMode == active && requestedMode == poweroff && !ShutdownHandler.ConfirmShutdown(false))
    {
	DDD("There are background activities. Changing to standby first");
	ActiveMode = standby;
    }
    else
	ActiveMode = requestedMode;

    //inform plugins about mode change
    eActiveMode activeState = ActiveMode;

    /*
    ** Allow plugins to follow the changes in this function before disabling them
    ** So, call Service "ActiveModeChange" after the switch statement if ActiveMode was !active
    ** This is a quick fix for crash in GraphLCD plugin see mantis #427
    ** Replay Recording -> Hot-standby -> Normal VDR operation (kPower) -> Crash
    */
    if (ActiveMode == active)
        cPluginManager::CallAllServices("ActiveModeChanged", &activeState);

    cTimer *timer = Timers.GetNextActiveTimer();

    DDD("Acting on  new Active mode %d", ActiveMode);
    switch (ActiveMode)
    {
        case active:
            //turn back to active mode
            SetUserInactiveTimeout(-1);

            //stop dummy player
            cControl::Shutdown();
            // restore initial/last channel
#if VDRVERSNUM < 10719
            if (Setup.InitialChannel > 0)
            {
                Setup.CurrentChannel = Setup.InitialChannel;
            }
#else
            //RC: taken from vdr.c ~1023
            if (*Setup.InitialChannel)
            {
               if (isnumber(Setup.InitialChannel))
               { // for compatibility with old setup.conf files
                  if (cChannel *Channel = Channels.GetByNumber(atoi(Setup.InitialChannel)))
                      Setup.InitialChannel = Channel->GetChannelID().ToString();
                }
                if (cChannel *Channel = Channels.GetByChannelID(tChannelID::FromString(Setup.InitialChannel)))
                    Setup.CurrentChannel = Channel->Number();
            }
#endif

	    Channels.SwitchTo(Setup.CurrentChannel);

            // restore initial/last volume
            if (Setup.InitialVolume >= 0)
                Setup.CurrentVolume = Setup.InitialVolume;
            else
                Setup.CurrentVolume  = cDevice::CurrentVolume();
            cDevice::PrimaryDevice()->SetVolume(Setup.CurrentVolume, true);

            break;

        case standby:
            Setup.CurrentChannel = cDevice::CurrentChannel();   // save current active channel
            cControl::Shutdown(); // makes sure livebuffer control/player does not cause crash

            //launch the dummy player
            cControl::Launch(new cDummyPlayerControl);

	    if (Setup.EnergySaveOn && Setup.StandbyTimeout < 241) // 241mins => never poweroff
	    {
               standbyCountdown.Set(Setup.StandbyTimeout*60*1000); //(Ms) after which go into poweroff
	    }
            else
               standbyCountdown.Set(INT_MAX);

            if (timer)
            {
                //DDD("next timer at %d", (int) timer->StartTime());
                cString cmd = cString::sprintf("echo %d > %s", (int)timer->StartTime(), WAKEUPFILE);
                SystemExec(cmd);
            }
            else
                unlink(WAKEUPFILE);
            break;

        case poweroff:
            Setup.CurrentChannel = cDevice::CurrentChannel();   // save current active channel
            cControl::Shutdown(); // makes sure livebuffer control/player does not cause crash

            //launch the dummy player
            cControl::Launch(new cDummyPlayerControl);

            //TODO: tests if poweroff is possible now - background activities
            // and shutting down is done in mainloop
#if 0
            ShutdownHandler.DoShutdown(true);
            ShutdownHandler.Exit(0);
#endif
            break;

        default: break;
    }

    if (ActiveMode != active)
        cPluginManager::CallAllServices("ActiveModeChanged", &activeState);

    return true;
}

#endif
