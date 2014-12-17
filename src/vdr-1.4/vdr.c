/*
 * vdr.c: Video Disk Recorder main program
 *
 * Copyright (C) 2000, 2003, 2006 Klaus Schmidinger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 * The author can be reached at kls@cadsoft.de
 *
 * The project's page is at http://www.cadsoft.de/vdr
 *
 * $Id: vdr.c 1.282.1.1 2007/04/30 09:48:23 kls Exp $
 */

#include "suspend.h"
#include <getopt.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <termios.h>
#include <unistd.h>
#include <langinfo.h>
#ifndef RBLITE
#include <execinfo.h>
#endif
#include "audio.h"
#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "debug.h"
#include "device.h"
#include "diseqc.h"
#include "dvbdevice.h"
#include "eitscan.h"
#include "epg.h"
#include "i18n.h"
#include "interface.h"
#include "keys.h"
#include "libsi/si.h"
#include "lirc.h"
#include "livebuffer.h"
#include "menu.h"
#include "osdbase.h"
#include "plugin.h"
#include "rcu.h"
#include "recording.h"
#include "skinclassic.h"
#include "skinsttng.h"
#include "sources.h"
#include "themes.h"
#include "timers.h"
#include "tools.h"
#include "transfer.h"
#include "videodir.h"
#include "providercharsets.h"
#include "resumeDvd.h"

#define MINCHANNELWAIT     10 // seconds to wait between failed channel switchings
#define ACTIVITYTIMEOUT    60 // seconds before starting housekeeping
#define SHUTDOWNWAIT      180 // seconds to wait in user prompt before automatic shutdown
#define MANUALSTART       600 // seconds the next timer must be in the future to assume manual start
#define CHANNELSAVEDELTA  600 // seconds before saving channels.conf after automatic modifications
#define LASTCAMMENUTIMEOUT  3 // seconds to run the main loop 'fast' after a CAM menu has been closed
                              // in order to react on a possible new CAM menu as soon as possible
#define DEVICEREADYTIMEOUT 30 // seconds to wait until all devices are ready
#define MENUTIMEOUT       120 // seconds of user inactivity after which an OSD display is closed
#define SHUTDOWNRETRY     300 // seconds before trying again to shut down
#define TIMERCHECKDELTA    10 // seconds between checks for timers that need to see their channel
#define TIMERDEVICETIMEOUT  8 // seconds before a device used for timer check may be reused
#define TIMERLOOKAHEADTIME 60 // seconds before a non-VPS timer starts and the channel is switched if possible
#define VPSLOOKAHEADTIME   24 // hours within which VPS timers will make sure their events are up to date
#define VPSUPTODATETIME  3600 // seconds before the event or schedule of a VPS timer needs to be refreshed

#define MULTIROOM_TIMEOUT 30  // seconds till warn user about nonworking MultiRoom

#define EXIT(v) { ExitCode = (v); goto Exit; }

int Interrupted = 0;
eShutdownMode shutdownMode = standby;
#ifdef DEBUG_TIMES
struct timeval menuTimeVal1;
#endif

int ForceReloadDB = 0;
int DBCounter = 0;
int lastDBCounter = 0;
int lastReelboxModeTemp = eModeStandalone;

eOSState active_function = osUnknown;
bool requestQuickStandby = false;
char *oldVideoDirectory = NULL;
char *VideoDirectory = NULL;
bool isNetClient = false;

static bool SetUser(const char *UserName)
{
  if (UserName) {
     struct passwd *user = getpwnam(UserName);
     if (!user) {
        fprintf(stderr, "vdr: unknown user: '%s'\n", UserName);
        return false;
        }
     if (setgid(user->pw_gid) < 0) {
        fprintf(stderr, "vdr: cannot set group id %u: %s\n", (unsigned int)user->pw_gid, strerror(errno));
        return false;
        }
     if (initgroups(user->pw_name, user->pw_gid) < 0) {
        fprintf(stderr, "vdr: cannot set supplemental group ids for user %s: %s\n", user->pw_name, strerror(errno));
        return false;
        }
     if (setuid(user->pw_uid) < 0) {
        fprintf(stderr, "vdr: cannot set user id %u: %s\n", (unsigned int)user->pw_uid, strerror(errno));
        return false;
        }
     if (prctl(PR_SET_DUMPABLE, 2, 0, 0, 0) < 0) {
        fprintf(stderr, "vdr: warning - cannot set dumpable: %s\n", strerror(errno));
        // always non-fatal, and will not work with kernel < 2.6.13
        }
     }
  return true;
}

static bool SetCapSysTime(void)
{
  // drop all capabilities except cap_sys_time
  cap_t caps = cap_from_text("= cap_sys_time=ep");
  if (!caps) {
     fprintf(stderr, "vdr: cap_from_text failed: %s\n", strerror(errno));
     return false;
     }
  if (cap_set_proc(caps) == -1) {
     fprintf(stderr, "vdr: cap_set_proc failed: %s\n", strerror(errno));
     cap_free(caps);
     return false;
     }
  cap_free(caps);
  return true;
}

static bool SetKeepCaps(bool On)
{
  // set keeping capabilities during setuid() on/off
  if (prctl(PR_SET_KEEPCAPS, On ? 1 : 0, 0, 0, 0) != 0) {
     fprintf(stderr, "vdr: prctl failed\n");
     return false;
     }
  return true;
}
#ifndef RBLITE
#define BACKTRACE_BUFFER_SIZE 128
#define CRASH_DATE_SIZE       32
#define CRASH_MSG_SIZE        (CRASH_DATE_SIZE+32)

#define SIGNAL_STR(x) (\
	(SIGHUP   ==x)?"SIGHUP":\
	(SIGINT   ==x)?"SIGINT":\
	(SIGQUIT  ==x)?"SIGQUIT":\
	(SIGILL   ==x)?"SIGILL":\
	(SIGTRAP  ==x)?"SIGTRAP":\
	(SIGABRT  ==x)?"SIGABRT":\
	(SIGIOT   ==x)?"SIGIOT":\
	(SIGBUS   ==x)?"SIGBUS":\
	(SIGFPE   ==x)?"SIGFPE":\
	(SIGKILL  ==x)?"SIGKILL":\
	(SIGUSR1  ==x)?"SIGUSR1":\
	(SIGSEGV  ==x)?"SIGSEGV":\
	(SIGUSR2  ==x)?"SIGUSR2":\
	(SIGPIPE  ==x)?"SIGPIPE":\
	(SIGALRM  ==x)?"SIGALRM":\
	(SIGTERM  ==x)?"SIGTERM":\
	(SIGSTKFLT==x)?"SIGSTKFLT":\
	(SIGCHLD  ==x)?"SIGCHLD":\
	(SIGCONT  ==x)?"SIGCONT":\
	(SIGSTOP  ==x)?"SIGSTOP":\
	(SIGTSTP  ==x)?"SIGTSTP":\
	(SIGTTIN  ==x)?"SIGTTIN":\
	(SIGTTOU  ==x)?"SIGTTOU":\
	(SIGURG   ==x)?"SIGURG":\
	(SIGXCPU  ==x)?"SIGXCPU":\
	(SIGXFSZ  ==x)?"SIGXFSZ":\
	(SIGVTALRM==x)?"SIGVTALRM":\
	(SIGPROF  ==x)?"SIGPROF":\
	(SIGWINCH ==x)?"SIGWINCH":\
	(SIGIO    ==x)?"SIGIO":\
	(SIGPOLL  ==x)?"SIGPOLL":\
	(SIGPWR   ==x)?"SIGPWR":\
	(SIGSYS   ==x)?"SIGSYS":\
	(SIGUNUSED==x)?"SIGUNUSED":\
	(SIGRTMIN ==x)?"SIGRTMIN":\
	(SIGRTMAX ==x)?"SIGRTMAX":\
	"UNKNOWN")

static char crash_dtstr[CRASH_DATE_SIZE]={0};

static void SetSignalHandlerCrashTime() {
  time_t t=time(NULL);
  struct tm lt;
  localtime_r(&t, &lt);
  asctime_r(&lt, crash_dtstr);
  int len=strlen(crash_dtstr);
  if(len>0)
    crash_dtstr[len-1] =0; // remove \n
} // SetSignalHandlerCrashTime

#ifdef RBMINI
#include <unwind.h>
struct trace_arg
{
  void **array;
  int cnt, size;
};
#ifdef SHARED
static _Unwind_Reason_Code (*unwind_backtrace) (_Unwind_Trace_Fn, void *);
static _Unwind_Ptr (*unwind_getip) (struct _Unwind_Context *);
static void *libgcc_handle;

static void
init (void)
{
  libgcc_handle = __libc_dlopen ("libgcc_s.so.1");

  if (libgcc_handle == NULL)
    return;

  unwind_backtrace = __libc_dlsym (libgcc_handle, "_Unwind_Backtrace");
  unwind_getip = __libc_dlsym (libgcc_handle, "_Unwind_GetIP");
  if (unwind_getip == NULL)
    unwind_backtrace = NULL;
}
#else
# define unwind_backtrace _Unwind_Backtrace
# define unwind_getip _Unwind_GetIP
#endif
static _Unwind_Reason_Code backtrace_helper (struct _Unwind_Context *ctx, void *a) {
  struct trace_arg *arg = (trace_arg *)a;

  /* We are first called with address in the __backtrace function.
     Skip it.  */
  if (arg->cnt != -1)
    {
      arg->array[arg->cnt] = (void *) unwind_getip (ctx);

      /* Check whether we make any progress.  */
      if (arg->cnt > 0 && arg->array[arg->cnt - 1] == arg->array[arg->cnt])
    return _URC_END_OF_STACK;
    }
  if (++arg->cnt == arg->size)
    return _URC_END_OF_STACK;
  return _URC_NO_REASON;
}

int __backtrace (void **array, int size) {
  struct trace_arg arg;
  arg.array = array;
  arg.size = size;
  arg.cnt = -1;
#ifdef SHARED
  __libc_once_define (static, once);

  __libc_once (once, init);
  if (unwind_backtrace == NULL)
    return 0;
#endif
  if (size >= 1)
    unwind_backtrace (backtrace_helper, &arg);
  /* _Unwind_Backtrace on IA-64 seems to put NULL address above
     _start.  Fix it up here.  */
  if (arg.cnt > 1 && arg.array[arg.cnt - 1] == NULL)
    --arg.cnt;
  return arg.cnt != -1 ? arg.cnt : 0;
}
#endif

static void SignalHandlerCrash(int signum) {
  void *array[BACKTRACE_BUFFER_SIZE]={0};
  size_t size=0;
  int fd=-1;
  char buf[CRASH_MSG_SIZE]={0};
  signal(signum,SIG_DFL); // Allow core dump

  fd=open("/var/log/vdr.crashlog", O_CREAT|O_APPEND|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
  if (fd != -1) {
#ifdef RBMINI
    size = __backtrace (array, BACKTRACE_BUFFER_SIZE);
#else
    size = backtrace (array, BACKTRACE_BUFFER_SIZE);
#endif
    int ret = snprintf(buf, sizeof(buf), "%s ### Crash signal %i %s ###\n", crash_dtstr, signum, SIGNAL_STR(signum));
    write(fd, buf, ret+1);
    backtrace_symbols_fd(array, size, fd);
    close(fd);
  }
} // SignalHandlerCrash

#endif
static void SignalHandler(int signum)
{
  if (signum != SIGPIPE && signum != SIGBUS) {
  PRINTF(" set interrupted to %i\n", signum);
     Interrupted = signum;
     Interface->Interrupt();
     }
  signal(signum, SignalHandler);
}

static void Watchdog(int signum)
{
  // Something terrible must have happened that prevented the 'alarm()' from
  // being called in time, so let's get out of here:
  esyslog("PANIC: watchdog timer expired - exiting!");
  exit(1);
}

#define MOUNTSH "mount.sh"

static void Eject()
{
  const char *cmd1 = MOUNTSH " eject";
#ifndef RBMINI
  Skins.Message(mtInfo, tr("ejecting media"));
#endif
  SystemExec(cmd1);
}

static void PrepareShutdownExternal ( const char *ShutdownCmd, eShutdownMode mode, bool UserShutdown = false )
{
    ///< executes external shutdown command given with -s
    cTimer *timer = NULL;
#ifdef USEMYSQL
    if (Setup.ReelboxModeTemp != eModeClient) // Don't care about wakeup timer if this Reelbox is a Client
#endif
        timer = Timers.GetNextActiveTimer();
    time_t Now   = time(NULL);
    time_t Next  = timer ? timer->StartTime() : 0;
    time_t Delta = timer ? Next - Now : 0;

    int Channel = timer ? timer->Channel()->Number() : 0;
    const char *File = timer ? timer->File() : "";

        const char *modestring = "standby";
        if(mode == deepstandby)
        {
             modestring = "deepstandby";
        }
        else if(mode == restart)
        {
             modestring = "reboot";
        }

    if (timer)
        Delta = Next - Now; // compensates for Confirm() timeout
    char *cmd;
    asprintf(&cmd, "%s %ld %ld %d \"%s\" %d %s", ShutdownCmd, Next, Delta, Channel,
                                              *strescape(File, "\"$"), UserShutdown, modestring);
    isyslog("executing '%s'", cmd);
    PRINTF("executing '%s'\n", cmd);
    SystemExec(cmd);
    free(cmd);
}


/**
Stop all timers that are running.
   ie. timers that are active and have their start-times in the past
*/
void StopAllRunningTimers()
{
if ( cRecordControls::Active() ) // if no recordings currently, no running timers too
  {
    // Stop all running timers
    cTimer *timer = Timers.GetNextActiveTimer ();
    time_t Next = timer ? timer->StartTime () : 0;
    time_t Now = time(NULL);
    while (timer && Next - Now < 0)
      {
        if (timer->IsSingleEvent ())
          timer->ClrFlags (tfActive);
        else
          timer->Skip ();
        timer->Matches ();

        cTimer *nextTimer = Timers.GetNextActiveTimer ();
        time_t nextNext = nextTimer ? nextTimer->StartTime () : 0;
        if (nextNext < Next || (nextNext == Next && nextTimer == timer))
          {
            esyslog ("Loop detected while disabling running timers");
            break;
          }
        Next = nextNext;
        timer = nextTimer;
      }
    Timers.SetModified ();
  }

}

#include <string>
#include <sstream>
#include <fstream>

bool HasDevice(const char *deviceID) {
    std::ifstream inFile("/proc/bus/pci/devices");

    if (!inFile.is_open() || !deviceID)
        return  false;

    std::string line, word;

    bool found = false;
    while (getline(inFile, line)) {
       std::istringstream splitStr(line);
       std::string::size_type pos = line.find(deviceID);
       if (pos != std::string::npos) {
          //printf (" Found Device %s \n", deviceID);
          found = true;
          break;
       }
    }
    return found;
}

int main(int argc, char *argv[])
{
  // Save terminal settings:

  struct termios savedTm;
  bool HasStdin = (tcgetpgrp(STDIN_FILENO) == getpid() || getppid() != (pid_t)1) && tcgetattr(STDIN_FILENO, &savedTm) == 0;

  // Initiate locale:

  setlocale(LC_ALL, "");

  // Command line options:

#define DEFAULTSVDRPPORT 2001
#define DEFAULTWATCHDOG     0 // seconds
#define DEFAULTPLUGINDIR PLUGINDIR
#define DEFAULTEPGDATAFILENAME "epg.data"
#define DEFAULTCHANNELSFILENAME "channels.conf"

  if (HasDevice("14f12450"))
    isNetClient = true;

  bool StartedAsRoot = false;
  const char *VdrUser = NULL;
  int SVDRPport = DEFAULTSVDRPPORT;
  const char *AudioCommand = NULL;
  const char *ConfigDirectory = NULL;
  const char *EpgDataFileName = DEFAULTEPGDATAFILENAME;
  const char *ChannelsFileName = DEFAULTCHANNELSFILENAME;
  bool DisplayHelp = false;
  bool DisplayVersion = false;
  bool DaemonMode = false;
  int SysLogTarget = LOG_USER;
  bool MuteAudio = false;
  int WatchdogTimeout = DEFAULTWATCHDOG;
  const char *Terminal = NULL;
  const char *LocaleDir = NULL;
  const char *Shutdown = NULL;
  bool TimerWakeup = false;
  bool PowerCycle = false;
  bool AutoShutdown = false;
  bool requestShutdown = false;

  //Start by Klaus
  Setup.PipIsRunning = false;
  Setup.PipIsActive = false;
  //End by Klaus

  bool UseKbd = true;
  const char *LircDevice = NULL;
  const char *RcuDevice = NULL;
  VideoDirectory = strdup(VIDEODIR);

#if !defined(REMOTE_KBD)
  UseKbd = false;
#endif
#if defined(REMOTE_LIRC)
  LircDevice = LIRC_DEVICE;
#elif defined(REMOTE_RCU)
  RcuDevice = RCU_DEVICE;
#endif
#if defined(VFAT)
  VfatFileSystem = true;
#endif
#if defined(VDR_USER)
  VdrUser = VDR_USER;
#endif

  cPluginManager PluginManager(DEFAULTPLUGINDIR);
  int ExitCode = 0;

  static struct option long_options[] = {
      { "audio",    required_argument, NULL, 'a' },
      { "buffer",   required_argument, NULL, 'b' },
      { "config",   required_argument, NULL, 'c' },
      { "daemon",   no_argument,       NULL, 'd' },
      { "device",   required_argument, NULL, 'D' },
      { "epgfile",  required_argument, NULL, 'E' },
      { "grab",     required_argument, NULL, 'g' },
      { "help",     no_argument,       NULL, 'h' },
      { "channel",  required_argument, NULL, 'k' },
      { "lib",      required_argument, NULL, 'L' },
      { "lirc",     optional_argument, NULL, 'l' | 0x100 },
      { "localedir",required_argument, NULL, 'l' | 0x200 },
      { "log",      required_argument, NULL, 'l' },
      { "mute",     no_argument,       NULL, 'm' },
      { "no-kbd",   no_argument,       NULL, 'n' | 0x100 },
      { "plugin",   required_argument, NULL, 'P' },
      { "port",     required_argument, NULL, 'p' },
      { "powercycle", no_argument,     NULL, 'p' | 0x100 },
      { "rcu",      optional_argument, NULL, 'r' | 0x100 },
      { "record",   required_argument, NULL, 'r' },
      { "shutdown", required_argument, NULL, 's' },
      { "terminal", required_argument, NULL, 't' },
      { "timerwakeup", no_argument,    NULL, 'T' },
      { "user",     required_argument, NULL, 'u' },
      { "version",  no_argument,       NULL, 'V' },
      { "vfat",     no_argument,       NULL, 'v' | 0x100 },
      { "video",    required_argument, NULL, 'v' },
      { "watchdog", required_argument, NULL, 'w' },
      { NULL }
    };

  int c;
  while ((c = getopt_long(argc, argv, "a:b:c:dD:E:g:hk:l:L:mp:P:r:s:t:Tu:v:Vw:", long_options, NULL)) != -1) {
        switch (c) {
          case 'a': AudioCommand = optarg;
                    break;
          case 'b': BufferDirectory = optarg;
                    while (optarg && *optarg && optarg[strlen(optarg) - 1] == '/')
                          optarg[strlen(optarg) - 1] = 0;
                    break;
          case 'c': ConfigDirectory = optarg;
                    break;
          case 'd': DaemonMode = true; break;
          case 'D': if (isnumber(optarg)) {
                       int n = atoi(optarg);
                       if (0 <= n && n < MAXDEVICES) {
                          cDevice::SetUseDevice(n);
                          break;
                          }
                       }
                    fprintf(stderr, "vdr: invalid DVB device number: %s\n", optarg);
                    return 2;
                    break;
          case 'E': EpgDataFileName = (*optarg != '-' ? optarg : NULL);
                    break;
          case 'g': cSVDRP::SetGrabImageDir(*optarg != '-' ? optarg : NULL);
                    break;
          case 'h': DisplayHelp = true;
                    break;
          case 'k': ChannelsFileName = (*optarg != '-' ? optarg : NULL);
                    break;
          case 'l': {
                      char *p = strchr(optarg, '.');
                      if (p)
                         *p = 0;
                      if (isnumber(optarg)) {
                         int l = atoi(optarg);
                         if (0 <= l && l <= 3) {
                            SysLogLevel = l;
                            if (!p)
                               break;
                            if (isnumber(p + 1)) {
                               int l = atoi(p + 1);
                               if (0 <= l && l <= 7) {
                                  int targets[] = { LOG_LOCAL0, LOG_LOCAL1, LOG_LOCAL2, LOG_LOCAL3, LOG_LOCAL4, LOG_LOCAL5, LOG_LOCAL6, LOG_LOCAL7 };
                                  SysLogTarget = targets[l];
                                  break;
                                  }
                               }
                            }
                         }
                    if (p)
                       *p = '.';
                    fprintf(stderr, "vdr: invalid log level: %s\n", optarg);
                    return 2;
                    }
                    break;
          case 'L': if (access(optarg, R_OK | X_OK) == 0)
                       PluginManager.SetDirectory(optarg);
                    else {
                       fprintf(stderr, "vdr: can't access plugin directory: %s\n", optarg);
                       return 2;
                       }
                    break;
          case 'l' | 0x100:
                    LircDevice = optarg ? : LIRC_DEVICE;
                    break;
          case 'l' | 0x200:
                    if (access(optarg, R_OK | X_OK) == 0)
                       LocaleDir = optarg;
                    else {
                       fprintf(stderr, "vdr: can't access locale directory: %s\n", optarg);
                       return 2;
                       }
                    break;
          case 'm': MuteAudio = true;
                    break;
          case 'n' | 0x100:
                    UseKbd = false;
                    break;
          case 'p': if (isnumber(optarg))
                       SVDRPport = atoi(optarg);
                    else {
                       fprintf(stderr, "vdr: invalid port number: %s\n", optarg);
                       return 2;
                       }
                    break;
          case 'p' | 0x100:
                    PRINTF("Got power cycle\n");
                    PowerCycle = true;
                    break;
          case 'P': PluginManager.AddPlugin(optarg);
                    break;
          case 'r' | 0x100:
                    RcuDevice = optarg ? : RCU_DEVICE;
                    break;
          case 'r': cRecordingUserCommand::SetCommand(optarg);
                    break;
          case 's': Shutdown = optarg;
                    break;
          case 't': Terminal = optarg;
                    if (access(Terminal, R_OK | W_OK) < 0) {
                       fprintf(stderr, "vdr: can't access terminal: %s\n", Terminal);
                       return 2;
                       }
                    break;
          case 'u': if (*optarg)
                       VdrUser = optarg;
                    break;
          case 'T': TimerWakeup = true;
                    break;
          case 'V': DisplayVersion = true;
                    break;
          case 'v' | 0x100:
                    VfatFileSystem = true;
                    break;
          case 'v': free(VideoDirectory);
                    VideoDirectory = strdup(optarg);
                    while (VideoDirectory && *VideoDirectory && VideoDirectory[strlen(VideoDirectory) - 1] == '/')
                          VideoDirectory[strlen(VideoDirectory) - 1] = 0;
                    break;
          case 'w': if (isnumber(optarg)) { int t = atoi(optarg);
                       if (t >= 0) {
                          WatchdogTimeout = t;
                          break;
                          }
                       }
                    fprintf(stderr, "vdr: invalid watchdog timeout: %s\n", optarg);
                    return 2;
                    break;
          default:  return 2;
          }
        }


  // Set user id in case we were started as root:

  if (VdrUser && geteuid() == 0) {
     StartedAsRoot = true;
     if (strcmp(VdrUser, "root")) {
        if (!SetKeepCaps(true))
           return 2;
        if (!SetUser(VdrUser))
           return 2;
        if (!SetKeepCaps(false))
           return 2;
        if (!SetCapSysTime())
           return 2;
        }
     }
#ifndef RBLITE
  signal(SIGILL, SignalHandlerCrash);
  signal(SIGFPE, SignalHandlerCrash);
  signal(SIGSEGV, SignalHandlerCrash);
  signal(SIGBUS, SignalHandlerCrash);
  signal(SIGABRT, SignalHandlerCrash);

  signal(SIGHUP, SignalHandlerCrash);
  signal(SIGINT, SignalHandlerCrash);
  signal(SIGQUIT, SignalHandlerCrash);
//  signal(SIGTRAP, SignalHandlerCrash);
  signal(SIGIOT, SignalHandlerCrash);
  signal(SIGKILL, SignalHandlerCrash);
//  signal(SIGUSR1, SignalHandlerCrash);
//  signal(SIGUSR2, SignalHandlerCrash);
//  signal(SIGPIPE, SignalHandlerCrash);
//  signal(SIGALRM, SignalHandlerCrash);
  signal(SIGTERM, SignalHandlerCrash);
  signal(SIGSTKFLT, SignalHandlerCrash);
//  signal(SIGCHLD, SignalHandlerCrash);
  signal(SIGCONT, SignalHandlerCrash);
  signal(SIGSTOP, SignalHandlerCrash);
  signal(SIGTSTP, SignalHandlerCrash);
//  signal(SIGTTIN, SignalHandlerCrash);
//  signal(SIGTTOU, SignalHandlerCrash);
//  signal(SIGURG, SignalHandlerCrash);
//  signal(SIGXCPU, SignalHandlerCrash);
//  signal(SIGXFSZ, SignalHandlerCrash);
//  signal(SIGVTALRM, SignalHandlerCrash);
//  signal(SIGPROF, SignalHandlerCrash);
//  signal(SIGWINCH, SignalHandlerCrash);
//  signal(SIGIO, SignalHandlerCrash);
//  signal(SIGPOLL, SignalHandlerCrash);
  signal(SIGPWR, SignalHandlerCrash);
  signal(SIGSYS, SignalHandlerCrash);

  SetSignalHandlerCrashTime();
#endif
  // Help and version info:

  if (DisplayHelp || DisplayVersion) {
     if (!PluginManager.HasPlugins())
        PluginManager.AddPlugin("*"); // adds all available plugins
     PluginManager.LoadPlugins();
     if (DisplayHelp) {
        printf("Usage: vdr [OPTIONS]\n\n"          // for easier orientation, this is column 80|
               "  -a CMD,   --audio=CMD    send Dolby Digital audio to stdin of command CMD\n"
               "  -b DIR,   --buffer=DIR   use DIR as LiveBuffer directory (default is to write\n"
               "                           it to the video directory)\n"
               "  -c DIR,   --config=DIR   read config files from DIR (default is to read them\n"
               "                           from the video directory)\n"
               "  -d,       --daemon       run in daemon mode\n"
               "  -D NUM,   --device=NUM   use only the given DVB device (NUM = 0, 1, 2...)\n"
               "                           there may be several -D options (default: all DVB\n"
               "                           devices will be used)\n"
               "  -E FILE,  --epgfile=FILE write the EPG data into the given FILE (default is\n"
               "                           '%s' in the video directory)\n"
               "                           '-E-' disables this\n"
               "                           if FILE is a directory, the default EPG file will be\n"
               "                           created in that directory\n"
               "  -g DIR,   --grab=DIR     write images from the SVDRP command GRAB into the\n"
               "                           given DIR; DIR must be the full path name of an\n"
               "                           existing directory, without any \"..\", double '/'\n"
               "                           or symlinks (default: none, same as -g-)\n"
               "  -h,       --help         print this help and exit\n"
               "  -k,       --channel=FILE use FILE as channels file (default is '%s' in \n"
               "                           the video directory\n"
               "  -l LEVEL, --log=LEVEL    set log level (default: 3)\n"
               "                           0 = no logging, 1 = errors only,\n"
               "                           2 = errors and info, 3 = errors, info and debug\n"
               "                           if logging should be done to LOG_LOCALn instead of\n"
               "                           LOG_USER, add '.n' to LEVEL, as in 3.7 (n=0..7)\n"
               "  -L DIR,   --lib=DIR      search for plugins in DIR (default is %s)\n"
               "            --lirc[=PATH]  use a LIRC remote control device, attached to PATH\n"
               "                           (default: %s)\n"
               "            --localedir=DIR search for locale files in DIR (default is\n"
               "                           %s)\n"
               "  -m,       --mute         mute audio of the primary DVB device at startup\n"
               "            --no-kbd       don't use the keyboard as an input device\n"
               "  -p PORT,  --port=PORT    use PORT for SVDRP (default: %d)\n"
               "                           0 turns off SVDRP\n"
               "  -P OPT,   --plugin=OPT   load a plugin defined by the given options\n"
               "            --rcu[=PATH]   use a remote control device, attached to PATH\n"
               "                           (default: %s)\n"
               "  -r CMD,   --record=CMD   call CMD before and after a recording\n"
               "  -s CMD,   --shutdown=CMD call CMD to shutdown the computer\n"
               "  -t TTY,   --terminal=TTY controlling tty\n"
               "  -u USER,  --user=USER    run as user USER; only applicable if started as\n"
               "                           root\n"
               "  -v DIR,   --video=DIR    use DIR as video directory (default: %s)\n"
               "  -V,       --version      print version information and exit\n"
               "            --vfat         encode special characters in recording names to\n"
               "                           avoid problems with VFAT file systems\n"
               "  -w SEC,   --watchdog=SEC activate the watchdog timer with a timeout of SEC\n"
               "                           seconds (default: %d); '0' disables the watchdog\n"
               "\n",
               DEFAULTEPGDATAFILENAME,
               DEFAULTCHANNELSFILENAME,
               DEFAULTPLUGINDIR,
               LIRC_DEVICE,
               LOCDIR,
               DEFAULTSVDRPPORT,
               RCU_DEVICE,
               VideoDirectory,
               DEFAULTWATCHDOG
               );
        }
     if (DisplayVersion)
        printf("vdr (%s/%s) - The Video Disk Recorder\n", VDRVERSION, APIVERSION);
     if (PluginManager.HasPlugins()) {
        if (DisplayHelp)
           printf("Plugins: vdr -P\"name [OPTIONS]\"\n\n");
        for (int i = 0; ; i++) {
            cPlugin *p = PluginManager.GetPlugin(i);
            if (p) {
               const char *help = p->CommandLineHelp();
               printf("%s (%s) - %s\n", p->Name(), p->Version(), p->Description());
               if (DisplayHelp && help) {
                  printf("\n");
                  puts(help);
                  }
               }
            else
               break;
            }
        }
     return 0;
     }

  // Check for UTF-8 and exit if present - asprintf() will fail if it encounters 8 bit ASCII codes
#if 0
  char *LangEnv;
  if ((LangEnv = getenv("LANG"))     != NULL && strcasestr(LangEnv, "utf") ||
      (LangEnv = getenv("LC_ALL"))   != NULL && strcasestr(LangEnv, "utf") ||
      (LangEnv = getenv("LC_CTYPE")) != NULL && strcasestr(LangEnv, "utf")) {
     fprintf(stderr, "vdr: please turn off UTF-8 before starting VDR\n");
     return 2;
     }
#endif
  // Set the system character table:

  char *CodeSet = NULL;
  if (setlocale(LC_CTYPE, ""))
     CodeSet = nl_langinfo(CODESET);
  else {
     char *LangEnv = getenv("LANG"); // last resort in case locale stuff isn't installed
     if (LangEnv) {
        CodeSet = strchr(LangEnv, '.');
        if (CodeSet)
           CodeSet++; // skip the dot
        }
     }
  if (CodeSet) {
     bool known = SI::SetSystemCharacterTable(CodeSet);
     isyslog("codeset is '%s' - %s", CodeSet, known ? "known" : "unknown");
     cCharSetConv::SetSystemCharacterTable(CodeSet);
     }

  // Initialize internationalization:

  I18nInitialize(LocaleDir);

  // Log file:

  if (SysLogLevel > 0)
     openlog("vdr", LOG_CONS, SysLogTarget); // LOG_PID doesn't work as expected under NPTL

  // Check the video directory:

  if (!DirectoryOk(VideoDirectory, true)) {
     fprintf(stderr, "vdr: can't access video directory %s\n", VideoDirectory);
     return 2;
     }


  // Check the channels file
  char *channelsFilePath = NULL;
  asprintf(&channelsFilePath,"%s",*AddDirectory(ConfigDirectory, ChannelsFileName));

  if (access(channelsFilePath, R_OK | W_OK)) {
   fprintf(stderr,"%s does not exist... creating \n",channelsFilePath);
   MakeDirs(channelsFilePath);

   int ret = creat(channelsFilePath, S_IRUSR | S_IWUSR );
    if (ret == -1 )
       fprintf(stderr,"can`t create %s! \n",channelsFilePath);
    else
       close(ret);
   }

  // Daemon mode:

  if (DaemonMode) {
     if (daemon(1, 0) == -1) {
        fprintf(stderr, "vdr: %m\n");
        esyslog("ERROR: %m");
        return 2;
        }
     }
  else if (Terminal) {
     // Claim new controlling terminal
     stdin  = freopen(Terminal, "r", stdin);
     stdout = freopen(Terminal, "w", stdout);
     stderr = freopen(Terminal, "w", stderr);
     HasStdin = true;
     }

  isyslog(VDRNAME " version %s started", VDRVERSION);
  if (StartedAsRoot && VdrUser)
     isyslog("switched to user '%s'", VdrUser);
  if (DaemonMode)
     dsyslog("running as daemon (tid=%d)", cThread::ThreadId());
  cThread::SetMainThreadId();

  // Main program loop variables - need to be here to have them initialized before any EXIT():

  cOsdObject *Menu = NULL;
  int LastChannel = 0;
  int LastTimerChannel = -1;
  int PreviousChannel[2] = { 1, 1 };
  int PreviousChannelIndex = 0;
  time_t vdrStartTime = time(NULL);
  time_t LastChannelChanged = time(NULL);
  time_t LastActivity = 0;
  time_t LastCamMenu = 0;
  int MaxLatencyTime = 0;
  bool ForceShutdown = false;
  bool UserShutdown = false;
  bool InhibitEpgScan = false;
  bool IsInfoMenu = false;
  cSkin *CurrentSkin = NULL;
  bool channelinfo_requested = false;
  bool installWizardCalled = false;
  bool netcvUpdateCalled = false;
  bool setupNetclientCalled = false;
  cPlugin *mcliPlugin = NULL;
  cOsdObject *McliMenu = NULL;
  //Start by Klaus
  bool channelinfo_was_requested = false;
  //End by Klaus

  // Config data load status
  bool loadSuccess = true;

  // TB: already seen something that delivers DVB-data?
  bool hasInputDev = false;

  // Load plugins:

  if (!PluginManager.LoadPlugins(true))
     EXIT(2);

  if(!PluginManager.GetPlugin("mcli"))
     hasInputDev = true;

  if (!BufferDirectory)
     BufferDirectory = VideoDirectory;

  // Configuration data:

  if (!ConfigDirectory)
     ConfigDirectory = VideoDirectory;

  cPlugin::SetConfigDirectory(ConfigDirectory);
  cThemes::SetThemesDirectory(AddDirectory(ConfigDirectory, "themes"));

  ResumeDvd.Read();
  Setup.Load(AddDirectory(ConfigDirectory, "setup.conf"));

  if(Setup.ReelboxMode==eModeClient)
      Setup.ReelboxModeTemp=eModeStandalone;
  else
      Setup.ReelboxModeTemp = Setup.ReelboxMode;

  // load all configuration data, even there are load failures continue loading the rest
  // ORDER Load() && loadSuccess IMPORTANT to ensure every Load() is excuted irrespective of what loadSuccess is.

  loadSuccess = Sources.Load(AddDirectory(ConfigDirectory, "sources.conf"), true, true) && loadSuccess ;
  loadSuccess =  Diseqcs.Load(AddDirectory(ConfigDirectory, "diseqc.conf"), true, Setup.DiSEqC) && loadSuccess;
  loadSuccess =  Channels.Load(AddDirectory(ConfigDirectory, ChannelsFileName ), false, true) && loadSuccess;
#ifdef USEMYSQL
  if((Setup.ReelboxMode==eModeClient)||(Setup.ReelboxMode==eModeServer))
  {
      if(Setup.ReelboxMode==eModeServer)
          loadSuccess =  Timers.LoadDB() && loadSuccess;
      else
          loadSuccess =  Timers.Load() && loadSuccess;
      Timers.SetFilename(AddDirectory(ConfigDirectory, "timers.conf"));
  }
  else
  {
      loadSuccess = Timers.Load(AddDirectory(ConfigDirectory, "timers.conf")) && loadSuccess;
      SystemExec("avahi-publish-avg-mysql.sh -2"); // -2 means kill avahi-publish
  }
#else
  loadSuccess = Timers.Load(AddDirectory(ConfigDirectory, "timers.conf")) && loadSuccess;
#endif
  loadSuccess = Commands.Load(AddDirectory(ConfigDirectory, "commands.conf"), true) && loadSuccess;
  loadSuccess = RecordingCommands.Load(AddDirectory(ConfigDirectory, "reccmds.conf"), true) && loadSuccess;
  loadSuccess = SVDRPhosts.Load(AddDirectory(ConfigDirectory, "svdrphosts.conf"), true) && loadSuccess;
  loadSuccess = Keys.Load(AddDirectory(ConfigDirectory, "remote.conf")) && loadSuccess;
  loadSuccess = KeyMacros.Load(AddDirectory(ConfigDirectory, "keymacros.conf"), true) && loadSuccess;
  loadSuccess = ProviderCharsets.Load(AddDirectory(ConfigDirectory, "providercharsets.conf"), true) &&  loadSuccess;

if (!loadSuccess)
#define RELEASE
#ifndef RELEASE
     EXIT(2);
#else
  //TODO: create new config file
  esyslog("Loading settings failed! :(");
#endif

  if (!*cFont::GetFontFileName(Setup.FontOsd)) {
     const char *msg = "no fonts available - OSD will not show any text!";
     fprintf(stderr, "vdr: %s\n", msg);
     esyslog("ERROR: %s", msg);
     }

  // Recordings:

  //Recordings.Update();
  //DeletedRecordings.Update();

  // EPG data:

  if (EpgDataFileName) {
     const char *EpgDirectory = NULL;
     if (DirectoryOk(EpgDataFileName)) {
        EpgDirectory = EpgDataFileName;
        EpgDataFileName = DEFAULTEPGDATAFILENAME;
        }
     else if (*EpgDataFileName != '/' && *EpgDataFileName != '.')
        EpgDirectory = VideoDirectory;
     if (EpgDirectory)
        cSchedules::SetEpgDataFileName(AddDirectory(EpgDirectory, EpgDataFileName));
     else
        cSchedules::SetEpgDataFileName(EpgDataFileName);
     cSchedules::Read();
     }

  // Signal handlers:

  if (signal(SIGHUP,  SignalHandler) == SIG_IGN) signal(SIGHUP,  SIG_IGN);
  if (signal(SIGINT,  SignalHandler) == SIG_IGN) signal(SIGINT,  SIG_IGN);
  if (signal(SIGTERM, SignalHandler) == SIG_IGN) signal(SIGTERM, SIG_IGN);
  if (signal(SIGPIPE, SignalHandler) == SIG_IGN) signal(SIGPIPE, SIG_IGN);
  if (signal(SIGBUS,  SignalHandler) == SIG_IGN) signal(SIGBUS,  SIG_IGN);
  if (WatchdogTimeout > 0)
     if (signal(SIGALRM, Watchdog)   == SIG_IGN) signal(SIGALRM, SIG_IGN);

  // DVB interfaces:

  cDvbDevice::Initialize();

  // Initialize plugins:

  if (!PluginManager.InitializePlugins())
     EXIT(2);

  // Primary device:

  cDevice::SetPrimaryDevice(Setup.PrimaryDVB);
  if (!cDevice::PrimaryDevice() || !cDevice::PrimaryDevice()->HasDecoder()) {
     if (cDevice::PrimaryDevice() && !cDevice::PrimaryDevice()->HasDecoder())
        isyslog("device %d has no MPEG decoder", cDevice::PrimaryDevice()->DeviceNumber() + 1);
     for (int i = 0; i < cDevice::NumDevices(); i++) {
         cDevice *d = cDevice::GetDevice(i);
         if (d && d->HasDecoder()) {
            isyslog("trying device number %d instead", i + 1);
            if (cDevice::SetPrimaryDevice(i + 1)) {
               Setup.PrimaryDVB = i + 1;
               break;
               }
            }
         }
     if (!cDevice::PrimaryDevice()) {
        const char *msg = "no primary device found - using first device!";
        fprintf(stderr, "vdr: %s\n", msg);
        esyslog("ERROR: %s", msg);
        if (!cDevice::SetPrimaryDevice(1))
           EXIT(2);
        if (!cDevice::PrimaryDevice()) {
           const char *msg = "no primary device found - giving up!";
           fprintf(stderr, "vdr: %s\n", msg);
           esyslog("ERROR: %s", msg);
           EXIT(2);
           }
        }
     }

  // User interface:

#ifndef SVDRP_THREAD
  Interface = new cInterface(SVDRPport);
#else
  Interface = new cInterface(0);
  SVDRP = new cSVDRP(SVDRPport);
#endif

  // Default skins:

  new cSkinClassic;
  new cSkinSTTNG;
  Skins.SetCurrent(Setup.OSDSkin);
  cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());
  CurrentSkin = Skins.Current();

  // Start plugins:

  if (!PluginManager.StartPlugins())
     EXIT(2);

  // Set skin and theme in case they're implemented by a plugin:

  if (!CurrentSkin || (CurrentSkin == Skins.Current() && strcmp(Skins.Current()->Name(), Setup.OSDSkin) != 0)) {
     Skins.SetCurrent(Setup.OSDSkin);
     cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());
     }

  // Remote Controls:
  if (RcuDevice)
     new cRcuRemote(RcuDevice);
  if (LircDevice)
     new cLircRemote(LircDevice);
  if (!DaemonMode && HasStdin && UseKbd)
     new cKbdRemote;
  Interface->LearnKeys();

  // External audio:

  if (AudioCommand)
     new cExternalAudio(AudioCommand);

  // Channel:

  if(!cPluginManager::GetPlugin("mcli"))
      if (!cDevice::WaitForAllDevicesReady(DEVICEREADYTIMEOUT))
         dsyslog("not all devices ready after %d seconds", DEVICEREADYTIMEOUT);
  if (Setup.InitialChannel > 0)
     Setup.CurrentChannel = Setup.InitialChannel;
  if (Setup.InitialVolume >= 0)
     Setup.CurrentVolume = Setup.InitialVolume;
  if(!cPluginManager::GetPlugin("mcli"))
    Channels.SwitchTo(Setup.CurrentChannel);
  if (MuteAudio)
     cDevice::PrimaryDevice()->ToggleMute();
  else
     cDevice::PrimaryDevice()->SetVolume(Setup.CurrentVolume, true);

  // Watchdog:

  if (WatchdogTimeout > 0) {
     dsyslog("setting watchdog timer to %d seconds", WatchdogTimeout);
     alarm(WatchdogTimeout); // Initial watchdog timer start
     }

#ifdef SVDRP_THREAD
  SVDRP->Start();
#endif

  if (TimerWakeup
      && !PowerCycle
      && !(Setup.Get("stepdone", "install") == NULL || atoi(Setup.Get("stepdone", "install")->Value())<16)) /* Install assistent required */
  {
      // goto Quick Standby immediately
      requestQuickStandby = true;
  }

  // switch ON Video & audio output, if normal vdr startup
  if (!requestQuickStandby)
  {
      int On = 1;
      cPluginManager::CallAllServices("ToggleVideoOutput", &On);
      cPluginManager::CallAllServices("ReInit", NULL);
  }
  else
  {
      isyslog("\n --------- vdr started with TimerWakeup=true && PowerCycle=false ----------");
  }


  // Main program loop:

#define DELETE_MENU ((IsInfoMenu &= (Menu == NULL)), delete Menu, Menu = NULL, channelinfo_requested = false)

  while (!Interrupted) {
#ifndef RBLITE
        SetSignalHandlerCrashTime();
#endif
        // Handle emergency exits:
        if (cThread::EmergencyExit()) {
           esyslog("emergency exit requested - shutting down");
           break;
           }
#ifdef DEBUGRINGBUFFERS
        cRingBufferLinear::PrintDebugRBL();
#endif
        if(Setup.ReelboxMode == eModeClient)
        {
            // If MultiRoom is en-/disabled do some actions here
            if(lastReelboxModeTemp != Setup.ReelboxModeTemp) {
                //printf("rbmt: %i rbm: %i dbc: %i\n", Setup.ReelboxModeTemp, Setup.ReelboxMode, DBCounter);
                if(Setup.ReelboxModeTemp != eModeStandalone) {
                    // load timers
                    Timers.Load(); // Clear old datas
                    Timers.LoadDB();
                    Timers.SetModified();
                    dsyslog("VDR: activating MultiRoom!");
                    PRINTF("VDR: activating MultiRoom!\n");
                    // load data of every plugins, which works with MySQL
                    cPluginManager::CallAllServices("Avahi MySQL enable", NULL);
                    lastReelboxModeTemp = Setup.ReelboxMode;
                    if (VideoDirectory) {
                        free(VideoDirectory);
                        VideoDirectory = NULL;
                    }
                    if (oldVideoDirectory)
                        VideoDirectory = strdup(oldVideoDirectory);
                    else
                        VideoDirectory = strdup("/media/reel/recordings");
                    //TouchFile(UpdateFileName());
                    Recordings.TouchUpdate();
                    //Recordings.Update();
                } else {
                    // disable MySQL on Client
                    Timers.Load(); // Clear Timers
                    Timers.SetModified();
                    dsyslog("VDR: deactivating MultiRoom!");
                    PRINTF("VDR: deactivating MultiRoom!\n");
                    cPluginManager::CallAllServices("Avahi MySQL disable", NULL);
                    lastReelboxModeTemp = eModeStandalone;
                    if (oldVideoDirectory) {
                        free(oldVideoDirectory);
                        oldVideoDirectory = NULL;
                    }
                    if (VideoDirectory) {
                        oldVideoDirectory = strdup(VideoDirectory);
                        free(VideoDirectory);
                        VideoDirectory = strdup("/tmp");
                    } else
                        oldVideoDirectory = strdup("/media/reel/recordings");
                    Recordings.StopScan();
                    Recordings.Clear();
                    //Recordings.TouchUpdate();
                    //Recordings.Update();
                }
            }

            if(ForceReloadDB)
            {
                // Reload datas
                Timers.Load(); // Clear old datas
                Timers.LoadDB();
                Timers.SetModified();
                dsyslog("VDR: Forced reload DB!");
                PRINTF("VDR: Forced reload DB!\n");
                // load data of every plugins, which works with MySQL
                cPluginManager::CallAllServices("Avahi MySQL enable", NULL);
                ForceReloadDB = 0;
            }
            else if (DBCounter != lastDBCounter && Setup.ReelboxModeTemp != eModeStandalone)
            {
                // Sync datas if DBCounter changed
                Timers.SyncData();
                cPluginManager::CallAllServices("Avahi MySQL sync", NULL); // Tell all plugins MySQL got new datas/modifications
                dsyslog("VDR: syncing DBs!");
                PRINTF("VDR: syncing DBs!\n");
                lastDBCounter = DBCounter;
            }
        }

        // Attach launched player control:
        cControl::Attach();

        time_t Now = time(NULL);

        if(!hasInputDev) {
            for(int i=0; !hasInputDev && i<cDevice::NumDevices(); i++) {
                if(cDevice::GetDevice(i)->HasInput()) {
                    hasInputDev = true;
                    Channels.SwitchTo(Setup.CurrentChannel);
                    break;
                }
            }
        } else
        // Make sure we have a visible programme in case device usage has changed:
        if (!EITScanner.Active() && cDevice::PrimaryDevice()->HasDecoder() && !cDevice::PrimaryDevice()->HasProgramme() && hasInputDev) {
           static time_t lastTime = 0;
           if (Now - lastTime > MINCHANNELWAIT) {
              cChannel *Channel = Channels.GetByNumber(cDevice::CurrentChannel());
              if (Channel && (Channel->Vpid() || Channel->Apid(0))) {
                 if (!scanning_on_receiving_device  // avoid scanning on current transponder
                     && !Channels.SwitchTo(cDevice::CurrentChannel()) // try to switch to the original channel...
                     && !(LastTimerChannel > 0 && Channels.SwitchTo(LastTimerChannel)) // ...or the one used by the last timer...
                     && !cDevice::SwitchChannel(1) // ...or the next higher available one...
                     && !cDevice::SwitchChannel(-1)) // ...or the next lower available one
                    ;
                 }
              lastTime = Now; // don't do this too often
              LastTimerChannel = -1;
              }
           }

        if (requestQuickStandby)
        {
            // launch QuickStandby
            if (!cQuickShutdownCtl::IsActive())
            {
                cControl::Shutdown();
                cControl::Launch(new cQuickShutdownCtl);
                cQuickShutdownCtl::runScript = true;
                if (!PowerCycle && TimerWakeup)
                    cQuickShutdownCtl::afterQuickShutdown = 1;
                cRemote::GetPlugin();
            }

            requestQuickStandby = false;
        }

        if(requestShutdown)
        {
            if (!cQuickShutdownCtl::IsActive())
                //printf("------------Menu = new cMenuShutdown--------------------\n");
                Menu = new cMenuShutdown(Interrupted, shutdownMode, UserShutdown, LastActivity);
            else // Active quickshutdown
            {
                switch(cQuickShutdownCtl::afterQuickShutdown)
                {
                   default:
                   case 0: // do nothing
                      break;
                   case 1: // standby
                      if (Setup.StandbyOrQuickshutdown == 0) // treat standby as requsted by user
                      {
                          shutdownMode = standby;
                          Interrupted = SIGTERM;
                      }// else: now QuickShutdown is active, let it remain active
                      else
                          DLOG("Remaining in Quickshutdown mode, after recording is complete. Instead of Standby.");
                      break;
                   case 2: // deepstandby
                      shutdownMode = deepstandby;
                      Interrupted = SIGTERM;
                      break;
                }
            }
            requestShutdown = false;
        }

        // Restart the Watchdog timer:
        if (WatchdogTimeout > 0) {
           int LatencyTime = WatchdogTimeout - alarm(WatchdogTimeout);
           if (LatencyTime > MaxLatencyTime) {
              MaxLatencyTime = LatencyTime;
              dsyslog("max. latency time %d seconds", MaxLatencyTime);
              }
           }
        // Handle channel and timer modifications:
        if (!Channels.BeingEdited() && !Timers.BeingEdited()) {
           int modified = Channels.Modified();
           static time_t ChannelSaveTimeout = 0;
           static int TimerState = 0;
           // Channels and timers need to be stored in a consistent manner,
           // therefore if one of them is changed, we save both.
           if (modified == CHANNELSMOD_USER || Timers.Modified(TimerState))
              ChannelSaveTimeout = 1; // triggers an immediate save
           else if (modified && !ChannelSaveTimeout)
              ChannelSaveTimeout = Now + CHANNELSAVEDELTA;
           bool timeout = ChannelSaveTimeout == 1 || (ChannelSaveTimeout && Now > ChannelSaveTimeout && !cRecordControls::Active());
           if ((modified || timeout) && Channels.Lock(false, 100)) {
              if (timeout) {
                 Channels.Save();
                 Timers.Save();
                 ChannelSaveTimeout = 0;
                 }
              for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
                  int chmod=Channel->Modification(CHANNELMOD_RETUNE);
                  if (chmod) {
                     cRecordControls::ChannelDataModified(Channel, chmod);
                     if (Channel->Number() == cDevice::CurrentChannel()) {
                        if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring()) {
                           if (cDevice::ActualDevice()->ProvidesTransponder(Channel) && // avoids retune on devices that don't really access the transponder
                               ((CHANNELMOD_CA != chmod) || cDevice::ActualDevice()->ProvidesCa(Channel))) { // avoids retune if only ca has changed and the device doesn´t provide it
                              isyslog("retuning due to%s%s%s modification of channel %d",
                                      (chmod & CHANNELMOD_PIDS  )?" PIDS"  :"",
                                      (chmod & CHANNELMOD_CA    )?" CA"    :"",
                                      (chmod & CHANNELMOD_TRANSP)?" TRANSP":"",
                                      Channel->Number());
                              Channels.SwitchTo(Channel->Number());
                              }
                           }
                        }
                     }
                  }
              Channels.Unlock();
              }
           }
        // Channel display:
        if (!EITScanner.Active() && cDevice::CurrentChannel() != LastChannel) {
           if (!Menu) {
              if (cControl::Control(true))
                     cControl::Control(true)->Hide();
              Menu = new cDisplayChannel(cDevice::CurrentChannel(), LastChannel >= 0 && !channelinfo_requested);
              }
           LastChannel = cDevice::CurrentChannel();
           LastChannelChanged = Now;
           }
        if (Now - LastChannelChanged >= Setup.ZapTimeout && LastChannel != PreviousChannel[PreviousChannelIndex])
           PreviousChannel[PreviousChannelIndex ^= 1] = LastChannel;
        // Timers and Recordings:
        if (TimerWakeup && Shutdown && Now - vdrStartTime > SHUTDOWNWAIT) {
           if (LastActivity == 0) {
               LastActivity = 1;
               AutoShutdown = true;
           }
           else if (LastActivity != 1)
               AutoShutdown = false;
           }
        if (!Timers.BeingEdited()) {
#ifdef USEMYSQL
           // If using MySQL then sync Data
//           if((Setup.ReelboxMode == eModeClient) || (Setup.ReelboxMode == eModeServer))
           if(Setup.ReelboxModeTemp == eModeServer)
               Timers.SyncData();

           if (Setup.ReelboxModeTemp != eModeClient) // Clients Don't do Timer-Actions
           {
#endif
               // Assign events to timers:
               Timers.SetEvents();
               // Must do all following calls with the exact same time!
               // Process ongoing recordings:
               cRecordControls::Process(Now);
               // Start new recordings:
               cTimer *Timer = Timers.GetMatch(Now);
               if (Timer && Timer->HasFlags(tfActive)) {
                   if (!cRecordControls::Start(Timer))
                       Timer->SetPending(true);
                   else
                       LastTimerChannel = Timer->Channel()->Number();
               }
               // Make sure timers "see" their channel early enough:
               static time_t LastTimerCheck = 0;
               if (hasInputDev && (Now - LastTimerCheck > TIMERCHECKDELTA)) { // don't do this too often and only if we have an input device
                   InhibitEpgScan = false;
                   static time_t DeviceUsed[MAXDEVICES] = { 0 };
                   for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer)) {
                       bool InVpsMargin = false;
                       bool NeedsTransponder = false;
                       if (Timer->HasFlags(tfActive) && !Timer->Recording()) {
                           if (Timer->HasFlags(tfVps)) {
                               if (Timer->Matches(Now, true, Setup.VpsMargin)) {
                                   InVpsMargin = true;
                                   Timer->SetInVpsMargin(InVpsMargin);
                               }
                               else if (Timer->Event()) {
                                   InVpsMargin = Timer->Event()->StartTime() <= Now && Timer->Event()->RunningStatus() == SI::RunningStatusUndefined;
                                   NeedsTransponder = Timer->Event()->StartTime() - Now < VPSLOOKAHEADTIME * 3600 && !Timer->Event()->SeenWithin(VPSUPTODATETIME);
                               }
                               else {
                                   cSchedulesLock SchedulesLock;
                                   const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
                                   if (Schedules) {
                                       const cSchedule *Schedule = Schedules->GetSchedule(Timer->Channel());
                                       InVpsMargin = !Schedule; // we must make sure we have the schedule
                                       NeedsTransponder = Schedule && !Schedule->PresentSeenWithin(VPSUPTODATETIME);
                                       //dsyslog ("[diseqc]: NeedsTransponder? %s ", NeedsTransponder?"YES":"NO");
                                   }
                               }
                               InhibitEpgScan |= InVpsMargin | NeedsTransponder;
                           }
                           else
                               NeedsTransponder = Timer->Matches(Now, true, TIMERLOOKAHEADTIME);
                       }
                       if (NeedsTransponder || InVpsMargin) {
                           // Find a device that provides the required transponder:
                           cDevice *Device = NULL;
                           bool DeviceAvailable = false;
                           for (int i = 0; i < cDevice::NumDevices(); i++) {
                               //dsyslog ("[diseqc]: diseqc %d \n",i);
                               cDevice *d = cDevice::GetDevice(i);
                               if (d && d->ProvidesTransponder(Timer->Channel())) {
                                   //dsyslog ("[diseqc]: diseqc %d Provides Tp  \n",i);
                                   if (d->IsTunedToTransponder(Timer->Channel())) {
                                       dsyslog ("[diseqc]: diseqc %d is tuned to TimerChannel  \n",i);
                                       // if any diseqc is tuned to the transponder, we're done
                                       Device = d;
                                       break;
                                   }
                                   if(!DeviceUsed[d->DeviceNumber()]) // Set initial delay to avoid using live tuner
                                       DeviceUsed[d->DeviceNumber()] = Now;
                                   bool timeout = Now - DeviceUsed[d->DeviceNumber()] > TIMERDEVICETIMEOUT; // only check other devices if they have been left alone for a while

                                   if (d->MaySwitchTransponder()) {
                                       //dsyslog ("[diseqc]: %d MaySwitchTransponder \n",i);
                                       DeviceAvailable = true; // avoids using the actual diseqc below
                                       if (timeout) {
                                           //dsyslog ("[diseqc]: diseqc %d is free timeoute == true!\n",i);
                                           Device = d; // only check other devices if they have been left alone for a while
                                       }
                                   }
                                   else if (timeout && !Device && InVpsMargin && !d->Receiving() && d->ProvidesTransponderExclusively(Timer->Channel())) {
                                       dsyslog ("[diseqc]: diseqc %d else if (timeout && !Device && InVpsMargin && ....  !\n",i);
                                       Device = d; // use this one only if no other with less impact can be found
                                   }
                               }
                           }
                           if (!Device && InVpsMargin && !DeviceAvailable) {
                               cDevice *d = cDevice::ActualDevice();
                               if(!DeviceUsed[d->DeviceNumber()]) // Set initial delay to avoid using live tuner
                                   DeviceUsed[d->DeviceNumber()] = Now;
                               if (!d->Receiving() && d->ProvidesTransponder(Timer->Channel()) && Now - DeviceUsed[d->DeviceNumber()] > TIMERDEVICETIMEOUT)
                                   Device = d; // use the actual diseqc as a last resort
                           }
                           // Switch the diseqc to the transponder:
                           if (Device) {
                               dsyslog ("[diseqc]: take  actual cardIdx %d -- diseqc %d !\n", Device->CardIndex() ,Device->DeviceNumber());
                               dsyslog ("[diseqc]: switch  to transponder \n");
                               if (!Device->IsTunedToTransponder(Timer->Channel())) {
                                   if (Device == cDevice::ActualDevice() && !Device->IsPrimaryDevice())
                                       cDevice::PrimaryDevice()->StopReplay(); // stop transfer mode

                                   dsyslog("switching device %d to channel %d", Device->DeviceNumber() + 1, Timer->Channel()->Number());
                                   Device->SwitchChannel(Timer->Channel(), false);
                                   DeviceUsed[Device->DeviceNumber()] = Now;
                               }
                               if (cDevice::PrimaryDevice()->HasDecoder() && !cDevice::PrimaryDevice()->HasProgramme()) {
                                   // the previous SwitchChannel() has switched away the current live channel
                                   Channels.SwitchTo(Timer->Channel()->Number()); // avoids toggling between old channel and black screen
                                   Skins.Message(mtInfo, tr("Upcoming VPS recording!"));
                               }
                           }
                       }
                   }
                   LastTimerCheck = Now;
               }
               // Delete expired timers:
               Timers.DeleteExpired();
#ifdef USEMYSQL
           }
#endif
        }
#if 0
        if (Recordings.NeedsUpdate()) {
           Recordings.Update();
           DeletedRecordings.Update();
           }
#endif

    bool netcvUpdateReady = false;
    if (!netcvUpdateCalled) {
        cPlugin *p = cPluginManager::GetPlugin("netcvupdate");
        if (p)
            p->Service("ready", &netcvUpdateReady);
    }

    if ((Setup.Get("stepdone", "install") == NULL || atoi(Setup.Get("stepdone", "install")->Value())<16)
         && !installWizardCalled)
    {
            installWizardCalled = true;
            cRemote::CallPlugin("install");
            cRemote::Put(kBack); // just kill Shutdown-Watchdog ;)
    }
    else if (!installWizardCalled && !netcvUpdateCalled && netcvUpdateReady)
    {
        netcvUpdateCalled = true;
        cRemote::CallPlugin("netcvupdate");
    }
//#ifdef RBMINI
    else if(!installWizardCalled && !setupNetclientCalled)
    {
        if((Now-vdrStartTime>MULTIROOM_TIMEOUT) && (Now-vdrStartTime<=MULTIROOM_TIMEOUT+5) && (Setup.ReelboxMode==eModeClient) && (Setup.ReelboxModeTemp==eModeStandalone))
        {
            setupNetclientCalled = true;
            cPlugin *p=cPluginManager::GetPlugin("setup");
            if(p)
            {
                DELETE_MENU;
                p->Service("Setup MultiRoom AVG not found", &Menu);
            }
        }
        else if(Now-vdrStartTime>MULTIROOM_TIMEOUT+5)
            setupNetclientCalled = true; // Only checks in the first $MULTIROOM_TIMEOUT+5 seconds
    }
//#endif

    if (mcliPlugin == NULL)
       mcliPlugin = cPluginManager::GetPlugin("mcli");
    else {
       if (!UserShutdown) { // if kPower has been pressed, cMenuShutdown takes precedence over other menus
          if(!Menu && !McliMenu)
              McliMenu = mcliPlugin->AltMenuAction();
          if (!Menu && McliMenu) { // is there any cam-menu waiting?
              DELETE_MENU;
              if(cControl::Control(true))
                  cControl::Control(true)->Hide();
              Menu = McliMenu;
              Menu->Show();
              McliMenu = NULL;
          }
       }
    }
    // CAM control:
        if (!Menu && !cOsd::IsOpen()) {
           Menu = CamControl();
           if (Menu)
              LastCamMenu = 0;
           else if (!LastCamMenu)
              LastCamMenu = Now;
           }
        // Queued messages:
        eKeys key = Skins.ProcessQueuedMessages();
        // User Input:


        cOsdObject *Interact = Menu ? Menu : cControl::Control(true);
        if((key == kNone) || (key == kOk) || (key == kBack))
            key = Interface->GetKey((!Interact || !Interact->NeedsFastResponse()) && Now - LastCamMenu > LASTCAMMENUTIMEOUT);
        Interact = Menu ? Menu : cControl::Control();

#ifdef DEBUG_TIMES
        if(key != kNone)
            gettimeofday(&menuTimeVal1, NULL);
#endif
        // Quick Shutdown Mode : Start
        //if (key == kPower)  PRINTF ("\033[1;91mGot kPower \033[0m \t in Quick Shutdown Mode? %i\n",cQuickShutdownCtl::IsActive() );
        if (cQuickShutdownCtl::IsActive()) // in suspend mode, capture all keys, except kExit and kPower
        {
            if (key == kBack || key == kPower) // Exit suspend mode, only with kPower or kExit
            {
                if (key == kPower)
                {
                    //printf ("turning kPower to kBack for cQuickShutdownCtl to handle\n");

                    /* needed not to trigger shutdown from script.
                     */
                    SystemExec("shutdownwd.sh cancel");
                }
                key = kBack; // kPower is not handled by cQuickShutdownCtl
                /* User has chosen to end QuickShutdown.So,
                 * must run quickshutdown script to have audio and video*/
                cQuickShutdownCtl::runScript = true;
                cQuickShutdownCtl::afterQuickShutdown = 0; // do nothing

                /*end of quickshutdown signals cancelling of "Standby-after recording done" state as well*/
                UserShutdown = false;
            }
            else
            {
                if (NORMALKEY(key) == k_Plugin)
                {
                    const char* plugin = cRemote::GetPlugin(); // donot act on CallPlugin()
                    isyslog(" QStandby active --- k_Plugin got. Removed \"%s\" from \"cache\"---", plugin);
                }

                key = kNone; // No reaction to any other key while in "quick-shutdown mode"
            }
        }
        // Quick Shutdown Mode : End



        //Start by Klaus
        if (key == kMenu && cDevice::PrimaryDevice()->Replaying() && cDevice::PrimaryDevice()->PlayerCanHandleMenuCalls())
        {
            if (!Menu && !cDevice::PrimaryDevice()->DVDPlayerIsInMenuDomain())
                key = k3;
        }
        else if(key == kStop)
        {
            //shutdown filebrowser while replaying playlists
            struct
            {
                int cmd;
            }   FileBrowserControl=
            {
                1
            };
            cPluginManager::CallAllServices("Filebrowser control", &FileBrowserControl);
        }
        else if(key == kBack)
        {
            //halt filebrowser while replaying playlists
            struct
            {
                int cmd;
            }   FileBrowserControl=
            {
                2
            };
            cPluginManager::CallAllServices("Filebrowser control", &FileBrowserControl);
        }
        //End by Klaus
        if (NORMALKEY(key) != kNone) {
           cStatus::MsgUserAction(key, Interact);          // PIN PATCH
           EITScanner.Activity();
           if(NORMALKEY(key) != k_Plugin) //ignore auto call of install plugin, dirty!
               LastActivity = Now;
           }
        // Keys that must work independent of any interactive mode:
        switch (key) {
          // Menu control:
          case kMenu: {
               key = kNone; // nobody else needs to see this key
               bool WasOpen = Interact != NULL;
               bool WasMenu = Interact && Interact->IsMenu();
               if (Menu) {
                  //Start by Klaus
                  DELETE_MENU;
                  active_function = osUnknown;
                  }
                  //End by Klaus
               else if (cControl::Control(true)) {
                  if (cOsd::IsOpen())
                     cControl::Control(true)->Hide();
                  else
                     WasOpen = false;
                  }
               if (!WasOpen || (!WasMenu && !Setup.MenuButtonCloses))
                  Menu = new cMenuMain;
               }
               break;
          // Direct main menu functions:
          /*#define DirectMainFunction(function)\
            DELETE_MENU;\
            if (cControl::Control(true))\
               cControl::Control(true)->Hide();\
            Menu = new cMenuMain(function);\
            key = kNone; // nobody else needs to see this key
          */
          //Start by Klaus
          #define DirectMainFunction(function)\
            printf("--------------DirectMainFunction, function = %d----------\n", active_function);\
            DELETE_MENU;\
            if (function != active_function) {\
                        if (cControl::Control(true))\
                            cControl::Control(true)->Hide();\
                        active_function = function;\
                        Menu = new cMenuMain(function);\
                   }\
                   else {\
                           active_function = osUnknown;\
                        }\
            key = kNone; // nobody else needs to see this key
          //End by Klaus
          case kInfo: {
               if (!channelinfo_requested /*Start by Klaus*/&& !channelinfo_was_requested/*End by Klaus*/) {
                       // we do nothing if channelInfo _was_  requested;
                     bool WasInfoMenu = IsInfoMenu;
                     if (!WasInfoMenu) {
                        IsInfoMenu = true;
                        if (cControl::Control()) {
                           DELETE_MENU;
                           cControl::Control()->Hide();
                           Menu = cControl::Control()->GetInfo();
                           if (Menu) {
                              key = kNone;
                              Menu->Show();
                              }
                           else {
                             IsInfoMenu = false;
                             }
                           }
                        }
                     }
                   else /*Start by Klaus*/if(channelinfo_requested && !channelinfo_was_requested) /*End by Klaus*/{

                     if (LastChannel == cDevice::CurrentChannel()) { //Start by Klaus
                       channelinfo_was_requested = true;
                       channelinfo_requested = false;
                       }
                     //End by Klaus
                     }
                  } break;
          case kHelp:       IsInfoMenu = true;
                            DirectMainFunction(osActiveEvent); break;
          case kSchedule:   DirectMainFunction(osSchedule); break;
          case kChannels:   DirectMainFunction(osChannels); break;
          //case kTimers:     DirectMainFunction(osTimers); break;
          case kSearch:     cRemote::CallPlugin("epgsearchonly"); break;
          case kRecordings: DirectMainFunction(osRecordings); break;
          case kSetup:      DELETE_MENU;
                            if (active_function != osSetup) {
                               cRemote::CallPlugin("setup");
                               active_function = osSetup;
                               }
                            else
                                active_function = osUnknown;
                            break;
          case kEject:      Eject(); key = kNone; break;
          case kCommands:   DirectMainFunction(osCommands); break;
          case kTT:
          case kDVB:
          case kDVD:
          case kPVR:
          case kReel:
          case kPiP:
          //case kEject:
          case kTimers:
          case kUser1 ... kUser9:
                            //Start by Klaus
                            if (key==kUser6 && Setup.PipIsRunning)
                            {
                                Interact->ProcessKey(key);
                            }
                            else
                            {
                                eOSState state = static_cast<enum eOSState> ((key - kUser1) + os_User + 1);
                                if (state != active_function)
                                {
                                    if (cRemote::PutMacro(key))
                                    	active_function = state;
                                }
                                else
                                {
                                    active_function = osUnknown;
                                    DELETE_MENU;
                                }
                                key = kNone;
                            }
                            break;
                            //End by Klaus
                            //cRemote::PutMacro(key); key = kNone; break;
          case k_Plugin: {
               const char *PluginName = cRemote::GetPlugin();
               //printf("PluginName: \033[1;91m %s \033[0m\n", PluginName);
               if (!UserShutdown && PluginName) { // if kPower has been pressed, cMenuShutdown takes precedence over other menus
                  DELETE_MENU;
                  if (cControl::Control(true))
                     cControl::Control(true)->Hide();
                  cPlugin *plugin = cPluginManager::GetPlugin(PluginName);
                  if (plugin) {
                   if (!cStatus::MsgPluginProtected(plugin)) {    // PIN PATCH
                     //printf("#########PluginName= %s, Setup.PipSwitchedToTV = %d\n", PluginName, Setup.PipSwitchedToTV);
                     if(std::string(PluginName) != "extrecmenu" || Setup.PipSwitchedToTV == 0){
                        Menu = plugin->MainMenuAction();
                        if (Menu && strcmp(PluginName, "epgsearch"))
                            Menu->Show();
                        }
                        else
                           Setup.PipSwitchedToTV = 0;
                     }
                   }
                  else
                     esyslog("ERROR(%s:%i): unknown plugin '%s'",__FILE__,__LINE__, PluginName);
                  }
               key = kNone; // nobody else needs to see these keys
               }
               break;
          // Channel up/down:
          case kChanUp|k_Repeat:
          case kChanUp:
          case kChanDn|k_Repeat:
          case kChanDn:
               // only if a menu is not open
               //  and user has opted for a different key behavior

               if (Setup.ChannelUpDownKeyMode == 1 && (!Menu || cDisplayChannel::IsOpen()))
#ifdef INTERNAL_BOUQUETS
               {
                   // channellist : cMenuBouquets(0)
                   // Bouquetlist : cMenuBouquets(1)
                   if (cDisplayChannel::IsOpen() || cControl::Control(true))
                   {
                       DELETE_MENU;
                   }
                   cMenuBouquets *menuBouquet = new cMenuBouquets(NORMALKEY(key) == kChanDn);
                   Menu = menuBouquet;
                   menuBouquet->Display();
                   active_function = osChannels;
                   kChanOpen = true; // used in menu.c
                   key = kNone; // nobody else sees these keys
                   break;
               }
#else
               {
                   if (cDisplayChannel::IsOpen() || cControl::Control(true))
                   {
                       DELETE_MENU;
                   }
                   if (key == kChanDn)
                   {
                       DirectMainFunction(osBouquets);
                   }
                   else
                   {
                       DirectMainFunction(osActiveBouquet);
                   }
                   key = kNone; // nobody else sees these keys
                   break;
               }
#endif

               if (!Interact) {
                  if (cControl::Control(true))
                     cControl::Control(true)->Hide();
                  Menu = new cDisplayChannel(NORMALKEY(key));
                  }
               else if (cDisplayChannel::IsOpen() || cControl::Control() /*Start by Klaus*/|| Setup.PipIsRunning /*End by Klaus*/) {
                  Interact->ProcessKey(key);
                  continue;
                  }
               else
                  cDevice::SwitchChannel(NORMALKEY(key) == kChanUp ? 1 : -1);
               key = kNone; // nobody else needs to see these keys
               break;
          // Volume control:
          case kVolUp|k_Repeat:
          case kVolUp:
          case kVolDn|k_Repeat:
          case kVolDn:
          case kMute:
               if (key == kMute) {
                  if (!cDevice::PrimaryDevice()->ToggleMute() && !Menu) {
                     key = kNone; // nobody else needs to see these keys
                     break; // no need to display "mute off"
                     }
                  }
               else
                  cDevice::PrimaryDevice()->SetVolume(NORMALKEY(key) == kVolDn ? -VOLUMEDELTA : VOLUMEDELTA);
               if (!Menu && !cOsd::IsOpen())
                  Menu = cDisplayVolume::Create();
               cDisplayVolume::Process(key);
               key = kNone; // nobody else needs to see these keys
               break;
          // Audio track control:
          case kAudio:
           {
               // kAudio is also used for toggling (lower/upper) case
               eOSState audioState = osUnknown;
               if( Interact )                 // if Osd open, call its processKey
                   audioState = Interact-> ProcessKey(key);

               if(audioState != osUnknown)        // Osd has already processed kAudio, so donot open tracks
               {
                   key = kNone;               // Osd ProcessKey() is called after switch-case, to avoid double call
                   break; // kAudio already used
               }

               if (cControl::Control(true))
                   cControl::Control(true)->Hide();
               if (!cDisplayTracks::IsOpen()) {
                   DELETE_MENU;
                   Menu = cDisplayTracks::Create();
               }
               else
                   cDisplayTracks::Process(key);
               key = kNone;
           } //  {} needed for eOSState audioState definition
           break;

          // Aspect ratio
             case kAspect:
               ::Setup.VideoFormat == 0 ? ::Setup.VideoFormat=1: ::Setup.VideoFormat=0;
               cDevice::PrimaryDevice()->SetVideoFormat(::Setup.VideoFormat);
               key = kNone;
               break;
          // Subtitle track control:
          case /*kSubtitles*/ kGreater:
               if(!Menu || cDisplaySubtitleTracks::IsOpen()) {
                   if (cControl::Control())
                       cControl::Control()->Hide();
                   if (!cDisplaySubtitleTracks::IsOpen()) {
                       DELETE_MENU;
                       Menu = cDisplaySubtitleTracks::Create();
                   }
                   else
                       cDisplaySubtitleTracks::Process(key);
                   key = kNone;
               }
               break;

          // Pausing live video:
          case kPause:
               if (!cControl::Control() && !cLiveBufferManager::GetLiveBufferControl()) {
                   // Don't allow Pause if LiveBuffer is disabled on NetClient
                   if((Setup.ReelboxModeTemp==eModeClient || (Setup.ReelboxModeTemp == eModeStandalone && isNetClient)) && (Setup.LiveBuffer==0))
                       Skins.Message(mtWarning, tr("Timeshift is not available!"));
                   else
                   {
                       DELETE_MENU;
                       if (!cRecordControls::PauseLiveVideo())
                           Skins.Message(mtError, tr("No free tuner to record!"));
                           //Skins.Message(mtError, tr("No free DVB device to record!"));
                   }
                  key = kNone; // nobody else needs to see this key
                  }
               break;
          // Instant recording:
          case kRecord:
               if (!cControl::Control() && !cLiveBufferManager::GetLiveBufferControl()) {
                  if (cRecordControls::Start())
                     Skins.Message(mtInfo, tr("Recording started"));
                  key = kNone; // nobody else needs to see this key
                  }
               break;
          // Power off:
          case kPower: {
                           isyslog("Power button pressed");
                           DELETE_MENU;

                           if (!Shutdown) {
                               Skins.Message(mtError, tr("Can't shutdown - option '-s' not given!"));
                               break;
                           }


                           // open cMenuShutdown and ask user what is to be done
                           cMenuShutdown::CancelShutdownScript("kPower");
                           LastActivity = 1;

#if 0
                           if (cRecordControls::Active() && Setup.RequestShutDownMode != 0) { // Setup.RequestShutDownMode == 0 => request shutdown menu!
                               //printf("In requestshutdownmode %i\n",  Setup.RequestShutDownMode);
                               if (!UserShutdown) {
                                   //Skins.Message(mtInfo, tr("Activated standby after current recording"));
                                   UserShutdown = true;
                                   if (!cQuickShutdownCtl::IsActive())
                                   {
                                       cControl::Shutdown();
                                       cControl::Launch(new cQuickShutdownCtl);
                                       /*donot runscript since the video and audio should not return back between
                                        * shuting down of cQuickShutdownCtl and vdr standby/shutdown*/
                                       cQuickShutdownCtl::runScript = false;
                                       cRemote::GetPlugin();
                                   }

                                   break;
                               }
                           }
#else
                           int nProcess = 0;
                           struct BgProcessInfo {
                               int nProcess;
                           } info;
                           memset(&info, 0, sizeof(info));
                           if(cPluginManager::CallAllServices("get bgprocess info", &info))
                               nProcess = info.nProcess;
                           if((cRecordControls::Active() || (nProcess > 0)) && (Setup.RequestShutDownMode != 0)) {
                               if (!UserShutdown) {
                                   UserShutdown = true;
                                   if(cRecordControls::Active())
                                       Skins.Message(mtInfo, tr("Recording - Activating QuickStandby"));
                                   else if(nProcess > 0 )
                                       Skins.Message(mtInfo, tr("Running Process - Activating QuickStandby"));
                                   else // Should not happen
                                       Skins.Message(mtInfo, tr("Activating standby"));
                                   if(Setup.RequestShutDownMode==1) // standby
                                       cQuickShutdownCtl::afterQuickShutdown = Setup.StandbyOrQuickshutdown ? 1 : 0;
                                   else if(Setup.RequestShutDownMode==2) // deep standby
                                       cQuickShutdownCtl::afterQuickShutdown = 2;
                                   if (!cQuickShutdownCtl::IsActive())
                                   {
                                       cControl::Shutdown();
                                       cControl::Launch(new cQuickShutdownCtl);
                                       /*donot runscript since the video and audio should not return back between
                                        * shuting down of cQuickShutdownCtl and vdr standby/shutdown*/
                                       cQuickShutdownCtl::runScript = false;
                                       cRemote::GetPlugin();
                                       continue;
                                   }
                                   break;
                               }
                           }
#endif
                           UserShutdown = true;
                           ForceShutdown = true;
                       } break;
        /*
               if (!Shutdown) {
                  Skins.Message(mtError, tr("Can't shutdown - option '-s' not given!"));
                  break;
                  }
               LastActivity = 1; // not 0, see below!
               cMenuShutdown::CancelShutdownScript("kPower"); //RC - we assume its ok to cancel the shutdown here as
                                         //     if vdr reaches the switch/case it's still running ok
               if (cRecordControls::Active()) {
              if (!UserShutdown) {
                 Skins.Message(mtInfo, tr("Activated standby after current recording"));
                     UserShutdown = true;
             break;
          } else
                  if (!Interface->Confirm(tr("Recording - shut down anyway?")))
                     break;
               }
               //  -- Shutdown at active playback by moviemax
               if (cControl::Control()) {
                   cControl::Control()->Shutdown();
                   //Skins.Message(mtInfo, tr(" Activated standby "));
                   UserShutdown = true;
                   PRINTF("\033[0;91m UserShutdown is True \033[0m\n");
                   break;
               }
               //  -- Shutdown at active playback by moviemax end

               UserShutdown = true;
               if (cPluginManager::Active(tr("shut down anyway?")))
                  break;
               if (cRecordControls::Active()) {
                  // Stop all running timers
                  cTimer *timer = Timers.GetNextActiveTimer();
                  time_t Next = timer ? timer->StartTime() : 0;
                  while (timer && Next - Now < 0) {
                        if (timer->IsSingleEvent())
                           timer->ClrFlags(tfActive);
                        else
                           timer->Skip();
                        timer->Matches();

                        cTimer *nextTimer = Timers.GetNextActiveTimer();
                        time_t nextNext = nextTimer ? nextTimer->StartTime() : 0;
                        if (nextNext < Next || (nextNext == Next && nextTimer == timer)) {
                           esyslog("Loop detected while disabling running timers");
                           break;
                           }
                        Next=nextNext;
                        timer=nextTimer;
                        }
                  Timers.SetModified();
           }
#ifdef USEMYSQL
           else if(Setup.ReelboxModeTemp != eModeClient) { // Clients don't cares about timer
#else
           else {
#endif
                  cTimer *timer = Timers.GetNextActiveTimer();
                  time_t Next  = timer ? timer->StartTime() : 0;
                  time_t Delta = timer ? Next - Now : 0;
                  if (Next && Delta <= Setup.MinEventTimeout * 60 && !AutoShutdown) {
                     char *buf;
                     asprintf(&buf, tr("Recording in %ld minutes, shut down anyway?"), Delta / 60);
                     bool confirm = Interface->Confirm(buf);
                     free(buf);
                     if (!confirm)
                        break;
                     }
                  }
               ForceShutdown = true;
               break;
               }*/
          default: break;
          }
        //if (!ForceShutdown)
        //    cMenuShutdown::CancelShutdownScript(); //RC
        Interact = Menu ? Menu : cControl::Control(); // might have been closed in the mean time
        if (Interact) {
           eOSState state = Interact->ProcessKey(key);

           //dirty hack: pass key to filebrowser when pip and picture player are active
           if (state == osUser10)
           {
                Interact = cControl::Control();
                if(Interact)
                {
                    state = Interact->ProcessKey(key);
                }
           }

           if (state == osUnknown && Interact != cControl::Control()) {
              if (ISMODELESSKEY(key) && cControl::Control()) {
                 state = cControl::Control()->ProcessKey(key);
                 if (state == osEnd) {
                    // let's not close a menu when replay ends:
                    cControl::Shutdown();
                    continue;
                    }
                 }
             else if ((LastActivity > 1) && (Now - LastActivity > MENUTIMEOUT))
             {
                state = osEnd;
             }
           }
         // TODO make the CAM menu stay open in case of automatic updates and have it return osContinue; then the following two lines can be removed again
           else if (state == osEnd && LastActivity > 1)
              LastActivity = Now;

           switch (state) {
             case osPause:  if (cLiveBufferManager::GetLiveBufferControl()) {
                               if (cControl::Control(true))
                                  cControl::Control(true)->ProcessKey(kPause);
                               break;
                               }
                            DELETE_MENU;
                            cControl::Shutdown(); // just in case
                            if (!cRecordControls::PauseLiveVideo())
                               Skins.Message(mtError, tr("No free DVB device to record!"));
                            break;
             case osRecord: if (cLiveBufferManager::GetLiveBufferControl()) {
                               if (cControl::Control(true))
                                  cControl::Control(true)->ProcessKey(kRecord);
                               break;
                               }
                            DELETE_MENU;
                            if (cRecordControls::Start())
                               Skins.Message(mtInfo, tr("Recording started"));
                            break;
             case osRecordings:
                            DELETE_MENU;
                            cControl::Shutdown();
                            DirectMainFunction(osRecordings);
                            // Menu = new cMenuMain(osRecordings);
                            break;
             case osReplay: DELETE_MENU;
                            cControl::Shutdown();
                            cControl::Launch(new cReplayControl);
                            break;
             case osStopReplay:
                            DELETE_MENU;
                            cControl::Shutdown();
                            break;
             case osSwitchDvb:
                            DELETE_MENU;
                            cControl::Shutdown();
                            Skins.Message(mtInfo, tr("Switching primary DVB..."));
                            cDevice::SetPrimaryDevice(Setup.PrimaryDVB);
                            break;
             case osPlugin: DELETE_MENU;
                            Menu = cMenuMain::PluginOsdObject();
                            if (Menu)
                               Menu->Show();
                            break;
             case osBack:
             case osEnd:
                    //Start by Klaus
                    active_function = osUnknown;
                    //End by Klaus
                    if (const char *returnToPlugin = cRemote::GetReturnToPlugin()) {
                    cPlugin *plugin = cPluginManager::GetPlugin(returnToPlugin);
                    cRemote::SetReturnToPlugin(NULL);
                    DELETE_MENU;
                    //active_function = osUnknown; // needed for change by Klaus
                    if (plugin) {
                        //Menu = Temp = plugin->MainMenuAction(); // PIN patch?
                        Menu = plugin->MainMenuAction();
                        if (Menu) {
                            Menu->Show();
                            if (Menu->IsMenu())
                                ((cOsdMenu*)Menu)->Display();
                        }
                    }
                }
                if (Interact == Menu)
                               DELETE_MENU;
                            else
                               cControl::Shutdown();
                            break;
             default:       ;
             }
           }
        else {
           eOSState state = osUnknown;
           if (cLiveBufferManager::GetLiveBufferControl())
             state = cLiveBufferManager::GetLiveBufferControl()->ProcessKey(key);
           if (state == osPause) {
              DELETE_MENU;
           //   cControl::Shutdown(); Don't do that here!
              if (!cRecordControls::PauseLiveVideo(true))
                 Skins.Message(mtError, tr("No free DVB device to record!"));
              }
           if (state == osUnknown) {
           // Key functions in "normal" viewing mode:
           if (key != kNone && KeyMacros.Get(key)) {
              cRemote::PutMacro(key);
              key = kNone;
              }
           switch (key) {
             // Toggle channels:
             case kChanPrev:
             case k0: {
                  if (PreviousChannel[PreviousChannelIndex ^ 1] == LastChannel || (LastChannel != PreviousChannel[0] && LastChannel != PreviousChannel[1]))
                     PreviousChannelIndex ^= 1;
                  Channels.SwitchTo(PreviousChannel[PreviousChannelIndex ^= 1]);
                  break;
                  }
             // Direct Channel Select:
             case k1 ... k9:
             // Left/Right rotates through channel groups:
             case kLeft|k_Repeat:
             case kLeft:
             case kRight|k_Repeat:
             case kRight:
             // Up/Down Channel Select:
             case kUp|k_Repeat:
             case kUp:
             case kDown|k_Repeat:
             case kDown:
                  if (cControl::Control(true))
                     cControl::Control(true)->Hide();
                  Menu = new cDisplayChannel(NORMALKEY(key));
                  break;
             case kInfo:
                  //Start by Klaus
                  if(channelinfo_was_requested)
                     channelinfo_was_requested = false;
                  else
                  //End by Klaus
                  if (Setup.WantChListOnOk) { /// kInfo osdDown
                      LastChannel = -1;
                      channelinfo_requested = true; // forces channel display
                  }
                  else
                  {
                      DirectMainFunction(osChannels);
                  }
                  break;
             // Viewing Control:
             case kOk:   //LastChannel = -1; break; // forces channel display
                         if (Setup.WantChListOnOk) {
                              DirectMainFunction(osChannels);
                         }
                         else {
                             LastChannel = -1;
                             channelinfo_requested = true; // forces channel display
                         }
                         break;

             // Instant resume of the last viewed recording:
             case kPlay:
                  if (cReplayControl::LastReplayed()) {
                     cControl::Shutdown();
                     cControl::Launch(new cReplayControl);
                     }
                  break;
             case kStop:
#ifdef USEMYSQL
                  if(Setup.ReelboxModeTemp == eModeClient)
                  {
                      std::vector<cTimer*> InstantRecording;
                      Timers.GetInstantRecordings(&InstantRecording);
                      if(InstantRecording.size())
                      {
                          cTimer *Timer = InstantRecording.at(InstantRecording.size()-1);
                          if(Timer)
                          {
                              char *buffer;
                              asprintf(&buffer,"%s \"%s\"?",tr("End recording"),Timer->Channel()->Name());
                              if (Interface->Confirm(buffer))
                                  Timers.Del(Timer);
                              free(buffer);
                          }
                      }
                  }
                  else
#endif
                  {
                  const char *s = NULL;
                  const char *last = NULL;
                  while ((s = cRecordControls::GetInstantId(s)) != NULL) {
                      if(s)
                        last = s;
                  }
                  if(last) {
                     char *buffer;
                     asprintf(&buffer,"%s \"%s\"?",tr("End recording"),last);
                     if (Interface->Confirm(buffer)) {
                        cRecordControls::Stop(last);
                        }
                     free(buffer);
                     }
                  }
                  break;
         case k2digit:
#ifdef INTERNAL_BOUQUETS
            Menu = new cMenuBouquets(3);
#else
            DirectMainFunction(osAddFavourite);
#endif
            break;
         case kYellow: {
#ifdef INTERNAL_BOUQUETS
            cMenuBouquets *menu = new cMenuBouquets(2);
            menu->Display();
            Menu = menu;
#else
            DirectMainFunction(osFavourites);
#endif
            break;
            }
         default:    break;
             }
           }
           }
        if (!Menu) {
           if (!InhibitEpgScan)
              EITScanner.Process();
           if (!cCutter::Active() && cCutter::Ended()) {
              if (cCutter::Error())
                 Skins.Message(mtError, tr("Editing process failed!"));
              else
                 Skins.Message(mtInfo, tr("Editing process finished"));
              }
           }

    /* allow cMenuShutdown to be called even if there is a cControl active(eg. Replay / xinemediaplayer is running) */
         if (/* (!Interact|| cQuickShutdownCtl::IsActive()) && */((!cRecordControls::Active() && !cCutter::Active() && (!Interface->HasSVDRPConnection() || UserShutdown)) || ForceShutdown)) {
           if (Now - LastActivity > ACTIVITYTIMEOUT) {
              // Shutdown:
              if (Shutdown && (Setup.MinUserInactivity || LastActivity == 1) && Now - LastActivity > Setup.MinUserInactivity * 60) {
                 cTimer *timer = Timers.GetNextActiveTimer();
                 time_t Next  = timer ? timer->StartTime() : 0;
                 time_t Delta = timer ? Next - Now : 0;
                 if (!LastActivity) {
                    if (!timer || Delta > MANUALSTART) {
                       // Apparently the user started VDR manually
                       dsyslog("assuming manual start of VDR");
                       LastActivity = Now;
                       continue; // don't run into the actual shutdown procedure below
                       }
                    else
                       LastActivity = 1;
                    }
                 /* This one manipulates starttime, we don't want that
         if (timer && Delta < Setup.MinEventTimeout * 60 && ForceShutdown) {
                    Delta = Setup.MinEventTimeout * 60;
                    Next = Now + Delta;
                    timer = NULL;
                    dsyslog("reboot at %s", *TimeToString(Next));
                    }
         */
                 if (!ForceShutdown && cPluginManager::Active()) {
                    LastActivity = Now - Setup.MinUserInactivity * 60 + SHUTDOWNRETRY; // try again later
                    continue;
         }
                 if (!Next || Delta > Setup.MinEventTimeout * 60 || ForceShutdown || (AutoShutdown && Delta > Setup.MinEventTimeout * 60)) {
                    ForceShutdown = false;
                    AutoShutdown = false;
                    if (timer)
                       dsyslog("next timer event at %s", *TimeToString(Next));
                    if (WatchdogTimeout > 0)
                       signal(SIGALRM, SIG_IGN);
                    if(Setup.RequestShutDownMode == 0)
                    {
                        requestShutdown = true;
                        if (!cRecordControls::Active()) LastActivity = Now;
                        //neccessary??
                        if (WatchdogTimeout > 0) {
                            alarm(WatchdogTimeout);
                            if (signal(SIGALRM, Watchdog) == SIG_IGN)
                            signal(SIGALRM, SIG_IGN);
                            }
                        continue;
                    }
                    else
                    {
                        if (Interface->Confirm(tr("Activating standby"), UserShutdown ? 2 : SHUTDOWNWAIT, true, true)) {
                            cControl::Shutdown();
                            /* RC: moved downwards to be executed on every shutdown, not just
                                    user shutdown. See PrepareShutdownExternal().
                            int Channel = timer ? timer->Channel()->Number() : 0;
                            const char *File = timer ? timer->File() : "";
                            if (timer)
                                Delta = Next - Now; // compensates for Confirm() timeout
                            char *cmd;
                            asprintf(&cmd, "%s %ld %ld %d \"%s\" %d", Shutdown, Next, Delta, Channel, *strescape(File, "\"$"), UserShutdown);
                            isyslog("executing '%s'", cmd);
                            SystemExec(cmd);
                            free(cmd);
                            */
                            if(Setup.RequestShutDownMode == 1)
                            {
                                shutdownMode = standby;
                            }
                            else
                            {
                                shutdownMode = deepstandby;
                            }
                            PRINTF("\033[1;91m In vdr.c Setting Interrupted == SIGTERM\033[0m\n");
                            if(shutdownMode == standby && Setup.StandbyOrQuickshutdown == 1)
                            {
                                //Start quickshutdown
                                if (Interact == Menu)
                                    DELETE_MENU; // close open menus
                                if (!cQuickShutdownCtl::IsActive())
                                {
                                    cControl::Shutdown();
                                    cControl::Launch(new cQuickShutdownCtl);
                                    cQuickShutdownCtl::runScript = true;
                                    cRemote::GetPlugin();
                                }

                            }
                            else
                                Interrupted = SIGTERM; // GA
                            //LastActivity = Now - Setup.MinUserInactivity * 60 + SHUTDOWNRETRY; // try again later ???
                        }
                        else
                        {
                            LastActivity = Now;
                            if (WatchdogTimeout > 0) {
                                alarm(WatchdogTimeout);
                                if (signal(SIGALRM, Watchdog) == SIG_IGN)
                                signal(SIGALRM, SIG_IGN);
                                }
                            cMenuShutdown::CancelShutdownScript("after SHUTDOWNWAIT"); //RC
                        }
                        UserShutdown = false;
                        continue; // skip the rest of the housekeeping for now
                    }
                    }
                 }
              // Disk housekeeping:
              RemoveDeletedRecordings();
              cSchedules::Cleanup();
              // Plugins housekeeping:
              PluginManager.Housekeeping();
              }
           }

    // Auto-quickshutdown if the user has choosen to have no liveTV On Avantgarde
    if ( Now - vdrStartTime > 120 &&  // donot immediately quickshutdown when vdr restarts
        !Setup.LiveTvOnAvg && (!UserShutdown && !ForceShutdown && !AutoShutdown)
        /*these are set when kPower is pressed and LastActivity is modified*/
        && Now - LastActivity > 120 /*wait 2 mins*/ )
        {
            // go into quickshutdown
            if (!cQuickShutdownCtl::IsActive())
            {
                DELETE_MENU;
                cControl::Shutdown();
                Skins.Message(mtInfo, tr("Switching to server-mode"),4);
                sleep(1); // give reelbox plugin time to delete the Menu from OSD
                cControl::Launch(new cQuickShutdownCtl);
                cQuickShutdownCtl::runScript = true;
                cRemote::GetPlugin(); // clear "cache" so that next k_Plugin will work
            }
        }
        // Main thread hooks of plugins:
        PluginManager.MainThreadHook();
        }

  if (Interrupted == SIGTERM) {
    //printf("\033[0;91mInterrupted == SIGTERM \033[0m%i\n",Interrupted);
    PrepareShutdownExternal( Shutdown, shutdownMode, UserShutdown );
  }

Exit:
  isyslog("caught signal %d", Interrupted);
  if(!Interrupted) Interrupted = SIGTERM;    // Need this to signal svdrp that the vdr is terminating
  if(Interface) Interface->TerminateSVDRP(); // Stop current & listening svdrp connection

    char *cmd;
    asprintf(&cmd, "shutdownwd.sh");
    isyslog("executing '%s'", cmd);
    SystemExec(cmd);
    free(cmd);

  StopAllRunningTimers();
  cLiveBufferManager::Shutdown();
  PluginManager.StopPlugins();
  cRecordControls::Shutdown();
  cCutter::Stop();
  delete Menu;
  cControl::Shutdown();
  delete Interface;
  cOsdProvider::Shutdown();
  Remotes.Clear();
  Audios.Clear();
  Skins.Clear();
  //if(UserShutdownActive)
  //{
      Skins.Shutdown(); //Close open osd object (dirty hack)
  //}
  //End by Klaus
  if (ExitCode != 2) {
     Setup.CurrentChannel = cDevice::CurrentChannel();
     Setup.CurrentVolume  = cDevice::CurrentVolume();
     Setup.Save();
     Timers.Save();
     Channels.Save();
     }
  cDevice::Shutdown();
  cSchedules::Cleanup(true);
  PluginManager.Shutdown(true);
  ReportEpgBugFixStats();
  if (WatchdogTimeout > 0)
     dsyslog("max. latency time %d seconds", MaxLatencyTime);
  isyslog("exiting");
  if (HasStdin) {
     dsyslog("HasStdin - tcsetattr");
     tcsetattr(STDIN_FILENO, TCSANOW, &savedTm);
  }
  if (cThread::EmergencyExit()) {
    if (SysLogLevel > 0) {
        dsyslog("closing log");
        closelog();
    }
     esyslog("emergency exit!");
     return 1;
  }
  if (SysLogLevel > 0) {
     dsyslog("closing log");
     closelog();
  }
  return ExitCode;
}
