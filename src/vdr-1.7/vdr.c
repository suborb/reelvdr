/*
 * vdr.c: Video Disk Recorder main program
 *
 * Copyright (C) 2000, 2003, 2006, 2008 Klaus Schmidinger
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
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * The author can be reached at kls@tvdr.de
 *
 * The project's page is at http://www.tvdr.de
 *
 * $Id: vdr.c 2.23 2011/08/15 12:42:39 kls Exp $
 */

#include <getopt.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <termios.h>
#include <unistd.h>
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
#include "menu.h"
#include "menushutdown.h"
#include "osdbase.h"
#include "plugin.h"
#include "rcu.h"
#include "recording.h"
#include "shutdown.h"
#include "skinclassic.h"
#include "skinsttng.h"
#include "sourceparams.h"
#include "sources.h"
#include "themes.h"
#include "timers.h"
#include "tools.h"
#include "transfer.h"
#include "videodir.h"

#ifdef USE_PROVIDERCHARSET
#include "providercharsets.h"
#endif

#define MINCHANNELWAIT        10 // seconds to wait between failed channel switchings
#define ACTIVITYTIMEOUT       60 // seconds before starting housekeeping
#define SHUTDOWNWAIT         300 // seconds to wait in user prompt before automatic shutdown
#define SHUTDOWNRETRY        360 // seconds before trying again to shut down
#define SHUTDOWNFORCEPROMPT    5 // seconds to wait in user prompt to allow forcing shutdown
#define SHUTDOWNCANCELPROMPT   5 // seconds to wait in user prompt to allow canceling shutdown
#define RESTARTCANCELPROMPT    5 // seconds to wait in user prompt before restarting on SIGHUP
#define MANUALSTART          600 // seconds the next timer must be in the future to assume manual start
#define CHANNELSAVEDELTA     600 // seconds before saving channels.conf after automatic modifications
#define DEVICEREADYTIMEOUT    30 // seconds to wait until all devices are ready
#define MENUTIMEOUT          120 // seconds of user inactivity after which an OSD display is closed
#define TIMERCHECKDELTA       10 // seconds between checks for timers that need to see their channel
#define TIMERDEVICETIMEOUT     8 // seconds before a device used for timer check may be reused
#define TIMERLOOKAHEADTIME    60 // seconds before a non-VPS timer starts and the channel is switched if possible
#define VPSLOOKAHEADTIME      24 // hours within which VPS timers will make sure their events are up to date
#define VPSUPTODATETIME     3600 // seconds before the event or schedule of a VPS timer needs to be refreshed

#define EXIT(v) { ShutdownHandler.Exit(v); goto Exit; }

#ifdef USEMYSQL
int ForceReloadDB = 0;
int DBCounter = 0;
int lastDBCounter = 0;
#endif /* USEMYSQL */

#ifdef REELVDR
eOSState LastMenuMainFunc = osEnd;
int lastReelboxModeTemp = eModeStandalone;

#ifdef RBMINI
bool isNetClient = true;
#else
bool isNetClient = false;
#endif
#endif


static int LastSignal = 0;

#ifdef USE_CRASHLOG
#include <execinfo.h>

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
    if(write(fd, buf, ret+1)){};
    backtrace_symbols_fd(array, size, fd);
    close(fd);
  }
} // SignalHandlerCrash

#endif /* USE_CRASHLOG */

static bool SetUser(const char *UserName, bool UserDump)//XXX name?
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
     if (UserDump && prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0)
        fprintf(stderr, "vdr: warning - cannot set dumpable: %s\n", strerror(errno));
     }
  return true;
}

static bool DropCaps(void)
{
  // drop all capabilities except selected ones
  cap_t caps = cap_from_text("= cap_sys_nice,cap_sys_time=ep");
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

static void SignalHandler(int signum)
{
  switch (signum) {
    case SIGPIPE:
         break;
    case SIGHUP:
         LastSignal = signum;
         break;
    default:
         LastSignal = signum;
         Interface->Interrupt();
         ShutdownHandler.Exit(0);
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

#if REELVDR
static void Eject()
{
    #define MOUNTSH "mount.sh"
    const char *cmd1 = MOUNTSH " eject";
#ifndef RBMINI
    Skins.Message(mtInfo, tr("ejecting media"));
#endif
    SystemExec(cmd1);
}
#endif

int main(int argc, char *argv[])
{
  // Save terminal settings:
  struct termios savedTm;
  bool HasStdin = (tcgetpgrp(STDIN_FILENO) == getpid() || getppid() != (pid_t)1) && tcgetattr(STDIN_FILENO, &savedTm) == 0;

  // Initiate locale:

  setlocale(LC_ALL, "");

  // Command line options:

#define DEFAULTSVDRPPORT 6419
#define DEFAULTWATCHDOG     0 // seconds
#define DEFAULTCONFDIR CONFDIR
#define DEFAULTPLUGINDIR PLUGINDIR
#define DEFAULTEPGDATAFILENAME "epg.data"

  bool StartedAsRoot = false;
  const char *VdrUser = NULL;
  bool UserDump = false;
  int SVDRPport = DEFAULTSVDRPPORT;
  const char *AudioCommand = NULL;
  const char *ConfigDirectory = NULL;
  const char *EpgDataFileName = DEFAULTEPGDATAFILENAME;
  bool DisplayHelp = false;
  bool DisplayVersion = false;
  bool DaemonMode = false;
  int SysLogTarget = LOG_USER;
  bool MuteAudio = false;
  int WatchdogTimeout = DEFAULTWATCHDOG;
  const char *Terminal = NULL;
  const char *LocaleDir = NULL;

#ifdef REELVDR
  bool installWizardCalled = false;
  bool netcvUpdateCalled = false;
  //eActiveMode prevActiveMode = poweroff; // TODO add unknown mode?
#endif

  bool UseKbd = true;
  const char *LircDevice = NULL;
  const char *RcuDevice = NULL;
#if !defined(REMOTE_KBD)
  UseKbd = false;
#endif
#if defined(REMOTE_LIRC)
  LircDevice = LIRC_DEVICE;
#elif defined(REMOTE_RCU)
  RcuDevice = RCU_DEVICE;
#endif
#if defined(VDR_USER)
  VdrUser = VDR_USER;
#endif

  cPluginManager PluginManager(DEFAULTPLUGINDIR);

  static struct option long_options[] = {
      { "audio",    required_argument, NULL, 'a' },
#ifdef USE_LIVEBUFFER
      { "buffer",   required_argument, NULL, 'b' },
      { "bufferminfree",   required_argument, NULL, 'B' },
#endif /* USE_LIVEBUFFER */
      { "config",   required_argument, NULL, 'c' },
      { "daemon",   no_argument,       NULL, 'd' },
      { "device",   required_argument, NULL, 'D' },
      { "edit",     required_argument, NULL, 'e' | 0x100 },
      { "epgfile",  required_argument, NULL, 'E' },
      { "filesize", required_argument, NULL, 'f' | 0x100 },
      { "genindex", required_argument, NULL, 'g' | 0x100 },
      { "grab",     required_argument, NULL, 'g' },
      { "help",     no_argument,       NULL, 'h' },
#ifdef REELVDR
      { "channel",  required_argument, NULL, 'k' },
#endif /*REELVDR*/
      { "instance", required_argument, NULL, 'i' },
      { "lib",      required_argument, NULL, 'L' },
      { "lirc",     optional_argument, NULL, 'l' | 0x100 },
      { "localedir",required_argument, NULL, 'l' | 0x200 },
      { "log",      required_argument, NULL, 'l' },
      { "mute",     no_argument,       NULL, 'm' },
      { "no-kbd",   no_argument,       NULL, 'n' | 0x100 },
      { "plugin",   required_argument, NULL, 'P' },
      { "port",     required_argument, NULL, 'p' },
#ifdef REELVDR
      { "powercycle", no_argument,     NULL, 'p' | 0x100 },
#endif /*REELVDR*/
      { "rcu",      optional_argument, NULL, 'r' | 0x100 },
      { "record",   required_argument, NULL, 'r' },
      { "shutdown", required_argument, NULL, 's' },
      { "split",    no_argument,       NULL, 's' | 0x100 },
      { "terminal", required_argument, NULL, 't' },
#ifdef REELVDR
      { "timerwakeup", no_argument,    NULL, 'T' | 0x100 },
#endif /*REELVDR*/
      { "user",     required_argument, NULL, 'u' },
      { "userdump", no_argument,       NULL, 'u' | 0x100 },
      { "version",  no_argument,       NULL, 'V' },
      { "vfat",     no_argument,       NULL, 'v' | 0x100 },
      { "video",    required_argument, NULL, 'v' },
      { "watchdog", required_argument, NULL, 'w' },
      { NULL,       no_argument,       NULL,  0  }
    };

  int c;
#ifdef REELVDR
  while ((c = getopt_long(argc, argv, "a:b:B:c:dD:e:E:g:hk:i:l:L:mp:P:r:s:t:Tu:v:Vw:", long_options, NULL)) != -1) {
#else
  while ((c = getopt_long(argc, argv, "a:c:dD:e:E:g:hi:l:L:mp:P:r:s:t:u:v:Vw:", long_options, NULL)) != -1) {
#endif /*REELVDR*/
        switch (c) {
          case 'a': AudioCommand = optarg;
                    break;
#ifdef USE_LIVEBUFFER
          case 'b': BufferDirectory = optarg;
                    if(optarg && *optarg && optarg[strlen(optarg)-1] == '/')
                       optarg[strlen(optarg)-1] = 0;
                    break;
          case 'B': if (isnumber(optarg)) BufferDirectoryMinFree = atoi(optarg);
                    break;
#endif /* USE_LIVEBUFFER */
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
          case 'e' | 0x100:
                    return CutRecording(optarg) ? 0 : 2;
          case 'E': EpgDataFileName = (*optarg != '-' ? optarg : NULL);
                    break;
          case 'f' | 0x100:
                    Setup.MaxVideoFileSize = StrToNum(optarg) / MEGABYTE(1);
                    if (Setup.MaxVideoFileSize < MINVIDEOFILESIZE)
                       Setup.MaxVideoFileSize = MINVIDEOFILESIZE;
                    if (Setup.MaxVideoFileSize > MAXVIDEOFILESIZETS)
                       Setup.MaxVideoFileSize = MAXVIDEOFILESIZETS;
                    break;
          case 'g' | 0x100:
                    return GenerateIndex(optarg) ? 0 : 2;
          case 'g': cSVDRP::SetGrabImageDir(*optarg != '-' ? optarg : NULL);
                    break;
          case 'h': DisplayHelp = true;
                    break;
#ifdef REELVDR
          case 'k': isyslog("IGNORED deprecated param channel (-k)");
                    break;
#endif /*REELVDR*/
          case 'i': if (isnumber(optarg)) {
                       InstanceId = atoi(optarg);
                       if (InstanceId >= 0)
                          break;
                       }
                    fprintf(stderr, "vdr: invalid instance id: %s\n", optarg);
                    return 2;
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
                    LircDevice = optarg ? optarg : LIRC_DEVICE;
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
#ifdef REELVDR
          case 'p' | 0x100:
                    isyslog("IGNORED deprecated param powercycle (-p|0x100)");
                    break;
#endif
          case 'P': PluginManager.AddPlugin(optarg);
                    break;
          case 'r' | 0x100:
                    RcuDevice = optarg ? : RCU_DEVICE;
                    break;
          case 'r': cRecordingUserCommand::SetCommand(optarg);
                    break;
          case 's': ShutdownHandler.SetShutdownCommand(optarg);
                    break;
          case 's' | 0x100:
                    Setup.SplitEditedFiles = 1;
                    break;
          case 't': Terminal = optarg;
                    if (access(Terminal, R_OK | W_OK) < 0) {
                       fprintf(stderr, "vdr: can't access terminal: %s\n", Terminal);
                       return 2;
                       }
                    break;
#ifdef REELVDR
          case 'T': isyslog("IGNORED deprecated param timerwakeup (-T)");
                    break;
#endif
          case 'u': if (*optarg)
                       VdrUser = optarg;
                    break;
          case 'u' | 0x100:
                    UserDump = true;
                    break;
          case 'V': DisplayVersion = true;
                    break;
          case 'v' | 0x100:
                    VfatFileSystem = true;
                    break;
          case 'v': VideoDirectory = optarg;
                    while (optarg && *optarg && optarg[strlen(optarg) - 1] == '/')
                          optarg[strlen(optarg) - 1] = 0;
                    break;
          case 'w': if (isnumber(optarg)) {
                       int t = atoi(optarg);
                       if (t >= 0) {
                          WatchdogTimeout = t;
                          break;
                          }
                       }
                    fprintf(stderr, "vdr: invalid watchdog timeout: %s\n", optarg);
                    return 2;
                    break;
#ifdef REELVDR
          default:  esyslog("Warning: unknown option -%c", c);
#else
          default:  return 2;
#endif
          }
        }

  // Set user id in case we were started as root:

  if (VdrUser && geteuid() == 0) {
     StartedAsRoot = true;
     if (strcmp(VdrUser, "root")) {
        if (!SetKeepCaps(true))
           return 2;
        if (!SetUser(VdrUser, UserDump))
           return 2;
        if (!SetKeepCaps(false))
           return 2;
        if (!DropCaps())
           return 2;
        }
     }

#ifdef USE_CRASHLOG
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
#endif /* USE_CRASHLOG */

  // Help and version info:

  if (DisplayHelp || DisplayVersion) {
     if (!PluginManager.HasPlugins())
        PluginManager.AddPlugin("*"); // adds all available plugins
     PluginManager.LoadPlugins();
     if (DisplayHelp) {
        printf("Usage: vdr [OPTIONS]\n\n"          // for easier orientation, this is column 80|
               "  -a CMD,   --audio=CMD    send Dolby Digital audio to stdin of command CMD\n"
#ifdef USE_LIVEBUFFER
               "  -b DIR,   --buffer=DIR   use DIR as LiveBuffer directory\n"
               "  -B SIZE,  --bufferminfree=SIZE "
               "                           leave at least SIZE free space (in MB) on"
               "                           buffer disk (default: 512MB). 0 means default\n"
#endif /*USE_LIVEBUFFER*/
               "  -c DIR,   --config=DIR   read config files from DIR (default: %s)\n"
               "  -d,       --daemon       run in daemon mode\n"
               "  -D NUM,   --device=NUM   use only the given DVB device (NUM = 0, 1, 2...)\n"
               "                           there may be several -D options (default: all DVB\n"
               "                           devices will be used)\n"
               "            --edit=REC     cut recording REC and exit\n"
               "  -E FILE,  --epgfile=FILE write the EPG data into the given FILE (default is\n"
               "                           '%s' in the video directory)\n"
               "                           '-E-' disables this\n"
               "                           if FILE is a directory, the default EPG file will be\n"
               "                           created in that directory\n"
               "            --filesize=SIZE limit video files to SIZE bytes (default is %dM)\n"
               "                           only useful in conjunction with --edit\n"
               "            --genindex=REC generate index for recording REC and exit\n"
               "  -g DIR,   --grab=DIR     write images from the SVDRP command GRAB into the\n"
               "                           given DIR; DIR must be the full path name of an\n"
               "                           existing directory, without any \"..\", double '/'\n"
               "                           or symlinks (default: none, same as -g-)\n"
               "  -h,       --help         print this help and exit\n"
               "  -i ID,    --instance=ID  use ID as the id of this VDR instance (default: 0)\n"
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
               "            --split        split edited files at the editing marks (only\n"
               "                           useful in conjunction with --edit)\n"
               "  -t TTY,   --terminal=TTY controlling tty\n"
               "  -u USER,  --user=USER    run as user USER; only applicable if started as\n"
               "                           root\n"
               "            --userdump     allow coredumps if -u is given (debugging)\n"
               "  -v DIR,   --video=DIR    use DIR as video directory (default: %s)\n"
               "  -V,       --version      print version information and exit\n"
               "            --vfat         encode special characters in recording names to\n"
               "                           avoid problems with VFAT file systems\n"
               "  -w SEC,   --watchdog=SEC activate the watchdog timer with a timeout of SEC\n"
               "                           seconds (default: %d); '0' disables the watchdog\n"
               "\n",
               DEFAULTCONFDIR,
               DEFAULTEPGDATAFILENAME,
               MAXVIDEOFILESIZEDEFAULT,
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

  // Log file:

  if (SysLogLevel > 0)
     openlog("vdr", LOG_CONS, SysLogTarget); // LOG_PID doesn't work as expected under NPTL

  // Check the video directory:

  if (!DirectoryOk(VideoDirectory, true)) {
     fprintf(stderr, "vdr: can't access video directory %s\n", VideoDirectory);
     return 2;
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
     tcgetattr(STDIN_FILENO, &savedTm);
     }

  isyslog("VDR version %s started", VDRVERSION);
  if (StartedAsRoot && VdrUser)
     isyslog("switched to user '%s'", VdrUser);
  if (DaemonMode)
     dsyslog("running as daemon (tid=%d)", cThread::ThreadId());
  cThread::SetMainThreadId();

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
  setlocale(LC_NUMERIC, "C"); // makes sure any floating point numbers written use a decimal point

  // Initialize internationalization:

  I18nInitialize(LocaleDir);

  // Main program loop variables - need to be here to have them initialized before any EXIT():

  cOsdObject *Menu = NULL;
  int LastChannel = 0;
  int LastTimerChannel = -1;
  int PreviousChannel[2] = { 1, 1 };
  int PreviousChannelIndex = 0;
  time_t LastChannelChanged = time(NULL);
  time_t LastInteract = 0;
  int MaxLatencyTime = 0;
  bool InhibitEpgScan = false;
#ifdef REELVDR
  bool tmpInhibitEpgScan = false;
#endif
  bool IsInfoMenu = false;
  bool CheckHasProgramme = false;
  cSkin *CurrentSkin = NULL;
#ifdef USEMYSQL
  bool loadSuccess = true;
#endif

  // Load plugins:

  if (!PluginManager.LoadPlugins(true))
     EXIT(2);
  // Configuration data:

#ifdef USE_LIVEBUFFER
  if (!BufferDirectory)
     BufferDirectory = VideoDirectory;
#endif /*USE_LIVEBUFFER*/
  if (!ConfigDirectory)
     ConfigDirectory = DEFAULTCONFDIR;

  cPlugin::SetConfigDirectory(ConfigDirectory);
  cThemes::SetThemesDirectory(AddDirectory(ConfigDirectory, "themes"));

  Setup.Load(AddDirectory(ConfigDirectory, "setup.conf"));
#ifdef REELVDR
  if (Setup.ReelboxMode == eModeClient)
      Setup.ReelboxModeTemp = eModeStandalone;
  else
      Setup.ReelboxModeTemp = Setup.ReelboxMode;
#endif

  Sources.Load(AddDirectory(ConfigDirectory, "sources.conf"), true, true);
  Diseqcs.Load(AddDirectory(ConfigDirectory, "diseqc.conf"), true, Setup.DiSEqC);
  Channels.Load(AddDirectory(ConfigDirectory, "channels.conf"), false, true);

#ifdef USEMYSQL
  if ((Setup.ReelboxMode == eModeClient) || (Setup.ReelboxMode == eModeServer))
  {
      if (Setup.ReelboxMode == eModeServer) {
          loadSuccess = Timers.LoadDB() && loadSuccess;
          Timers.ClearAllRecordingFlags(); //so that all interrupted recordings are started
      }
      else
          loadSuccess = Timers.Load() && loadSuccess;
      //Timers.SetFilename(AddDirectory(ConfigDirectory, "timers.conf"));
  }
  else
  {
      loadSuccess = Timers.Load(AddDirectory(ConfigDirectory, "timers.conf")) && loadSuccess;
      SystemExec("avahi-publish-avg-mysql.sh -2"); // -2 means kill avahi-publish
  }
#else
  loadSuccess = Timers.Load(AddDirectory(ConfigDirectory, "timers.conf")) && loadSuccess;
#endif

  Commands.Load(AddDirectory(ConfigDirectory, "commands.conf"));
  RecordingCommands.Load(AddDirectory(ConfigDirectory, "reccmds.conf"));
  SVDRPhosts.Load(AddDirectory(ConfigDirectory, "svdrphosts.conf"), true);
  Keys.Load(AddDirectory(ConfigDirectory, "remote.conf"));

#ifdef USE_PROVIDERCHARSET
  ProviderCharsets.Load(AddDirectory(ConfigDirectory, "providercharsets.conf"), true);
#endif
#ifdef USE_ALTERNATECHANNEL
  KeyMacros.Load(AddDirectory(ConfigDirectory, "keymacros.conf"), true) &&
    Channels.LoadAlternativeChannels(AddDirectory(ConfigDirectory, "channel_alternative.conf"));
#else
  KeyMacros.Load(AddDirectory(ConfigDirectory, "keymacros.conf"), true);
#endif /* ALTERNATECHANNEL */
  Folders.Load(AddDirectory(ConfigDirectory, "folders.conf"));

  if (!*cFont::GetFontFileName(Setup.FontOsd)) {
     const char *msg = "no fonts available - OSD will not show any text!";
     fprintf(stderr, "vdr: %s\n", msg);
     esyslog("ERROR: %s", msg);
     }

  // Recordings:

  Recordings.Update();
  DeletedRecordings.Update();

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

  // Check for timers in automatic start time window:

  ShutdownHandler.CheckManualStart(MANUALSTART);

  // User interface:

  Interface = new cInterface(SVDRPport);

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

  if (!CurrentSkin
      || (CurrentSkin == Skins.Current() && strcmp(Skins.Current()->Name(), Setup.OSDSkin) != 0)) {
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

  if (!cDevice::WaitForAllDevicesReady(DEVICEREADYTIMEOUT))
     dsyslog("not all devices ready after %d seconds", DEVICEREADYTIMEOUT);
#if ! REELVDR
     // we don't need this in Reelvdr since initial channel/volume is restored in
     // cShutdownHandler::ActOnActiveMode when Mode becomes "active"
  if (*Setup.InitialChannel) {
     if (isnumber(Setup.InitialChannel)) { // for compatibility with old setup.conf files
        if (cChannel *Channel = Channels.GetByNumber(atoi(Setup.InitialChannel)))
           Setup.InitialChannel = Channel->GetChannelID().ToString();
        }
     if (cChannel *Channel = Channels.GetByChannelID(tChannelID::FromString(Setup.InitialChannel)))
        Setup.CurrentChannel = Channel->Number();
     }
  if (Setup.InitialVolume >= 0)
     Setup.CurrentVolume = Setup.InitialVolume;
  Channels.SwitchTo(Setup.CurrentChannel);
  if (MuteAudio)
     cDevice::PrimaryDevice()->ToggleMute();
  else
     cDevice::PrimaryDevice()->SetVolume(Setup.CurrentVolume, true);
#endif

  // Signal handlers:

  if (signal(SIGHUP,  SignalHandler) == SIG_IGN) signal(SIGHUP,  SIG_IGN);
  if (signal(SIGINT,  SignalHandler) == SIG_IGN) signal(SIGINT,  SIG_IGN);
  if (signal(SIGTERM, SignalHandler) == SIG_IGN) signal(SIGTERM, SIG_IGN);
  if (signal(SIGPIPE, SignalHandler) == SIG_IGN) signal(SIGPIPE, SIG_IGN);
  if (WatchdogTimeout > 0)
     if (signal(SIGALRM, Watchdog)   == SIG_IGN) signal(SIGALRM, SIG_IGN);

  // Watchdog:

  if (WatchdogTimeout > 0) {
     dsyslog("setting watchdog timer to %d seconds", WatchdogTimeout);
     alarm(WatchdogTimeout); // Initial watchdog timer start
     }

  // Main program loop:

#define DELETE_MENU ((IsInfoMenu &= (Menu == NULL)), delete Menu, Menu = NULL)

  while (!ShutdownHandler.DoExit()) {
#ifdef USE_CRASHLOG
        SetSignalHandlerCrashTime();
#endif /* USE_CRASHLOG */
#ifdef DEBUGRINGBUFFERS
        cRingBufferLinear::PrintDebugRBL();
#endif

#if USEMYSQL
        if (Setup.ReelboxMode == eModeClient)
        {
            //printf("rbModeTemp %i rbm: %i dbc: %i\n", Setup.ReelboxModeTemp, Setup.ReelboxMode, DBCounter);
            // If MultiRoom is en-/disabled do some actions here
            if (lastReelboxModeTemp != Setup.ReelboxModeTemp) {
                printf("rbmt: %i rbm: %i dbc: %i\n", Setup.ReelboxModeTemp, Setup.ReelboxMode, DBCounter);
                if (Setup.ReelboxModeTemp != eModeStandalone) {

                    // load timers
                    printf("calling cTimers::LoadDB\n");
                    Timers.Load(); // Clear old datas
                    Timers.LoadDB();
                    Timers.SetModified();
                    DLOG("activating MultiRoom");

                    // load data of every plugins, which works with MySQL
                    cPluginManager::CallAllServices("Avahi MySQL enable", NULL);
                    lastReelboxModeTemp = Setup.ReelboxMode;

                    //TouchFile(UpdateFileName());
                    Recordings.TouchUpdate();
                    //Recordings.Update();
                } else {
                    // disable MySQL on Client
                    Timers.Load(); // Clear Timers
                    Timers.SetModified();
                    DLOG("deactivating MultiRoom");
                    cPluginManager::CallAllServices("Avahi MySQL disable", NULL);
                    lastReelboxModeTemp = eModeStandalone;
                    //Recordings.StopScan();
                    Recordings.Clear();
                    //Recordings.TouchUpdate();
                    //Recordings.Update();
                }
            }

            if (ForceReloadDB)
            {
                // Reload datas
                Timers.Load(); // Clear old datas
                Timers.LoadDB();
                Timers.SetModified();
                DLOG("forced reload of database");
                // load data of every plugins, which works with MySQL
                cPluginManager::CallAllServices("Avahi MySQL enable", NULL);
                ForceReloadDB = 0;
            }
            else if (DBCounter != lastDBCounter && Setup.ReelboxModeTemp != eModeStandalone)
            {
                // Sync datas if DBCounter changed
                Timers.SyncData();
                cPluginManager::CallAllServices("Avahi MySQL sync", NULL); // Tell all plugins MySQL got new datas/modifications
                DLOG("syncing database");
                lastDBCounter = DBCounter;
            }
        }
#endif
        // Attach launched player control:
        cControl::Attach();

        time_t Now = time(NULL);

        // Make sure we have a visible programme in case device usage has changed:
        if (!EITScanner.Active() && cDevice::PrimaryDevice()->HasDecoder() && !cDevice::PrimaryDevice()->HasProgramme()) {
           static time_t lastTime = 0;
#ifdef REELVDR
           if ((!Menu || CheckHasProgramme) && (Now - lastTime > MINCHANNELWAIT) && (unknown != ShutdownHandler.GetActiveMode())) { // !Menu to avoid interfering with the CAM if a CAM menu is open
#else
           if ((!Menu || CheckHasProgramme) && Now - lastTime > MINCHANNELWAIT) { // !Menu to avoid interfering with the CAM if a CAM menu is open
#endif
              cChannel *Channel = Channels.GetByNumber(cDevice::CurrentChannel());
              if (Channel && (Channel->Vpid() || Channel->Apid(0))) {
                 if (!Channels.SwitchTo(cDevice::CurrentChannel()) // try to switch to the original channel...
                     && !(LastTimerChannel > 0 && Channels.SwitchTo(LastTimerChannel))) // ...or the one used by the last timer...
                    ;
                 }
              lastTime = Now; // don't do this too often
              LastTimerChannel = -1;
              CheckHasProgramme = false;
              }
           }
        // Update the OSD size:
        {
          static time_t lastOsdSizeUpdate = 0;
          if (Now != lastOsdSizeUpdate) { // once per second
             cOsdProvider::UpdateOsdSize();
             lastOsdSizeUpdate = Now;
             }
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
           bool timeout = ChannelSaveTimeout == 1
                          || (ChannelSaveTimeout && Now > ChannelSaveTimeout
                              && !cRecordControls::Active());
           if ((modified || timeout) && Channels.Lock(false, 100)) {
              if (timeout) {
                 Channels.Save();
                 Timers.Save();
                 ChannelSaveTimeout = 0;
                 }
              for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
#ifdef USE_MCLI
                  int chmod = Channel->Modification(CHANNELMOD_RETUNE);
                  if (chmod) {
                     cRecordControls::ChannelDataModified(Channel, chmod);
                     if (Channel->Number() == cDevice::CurrentChannel()) {
                        if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring()) {
                           if (cDevice::ActualDevice()->ProvidesTransponder(Channel) &&                    // avoids retune on devices that don't really access the transponder
                               ((CHANNELMOD_CA != chmod) || !((Channel->Ca() >= CA_MCLI_MIN) && (Channel->Ca() <= CA_MCLI_MAX) && cDevice::ActualDevice()->HasInternalCam()))) { // avoids retune if only ca has changed and the device has an internal cam
                              isyslog("retuning due to%s%s%s%s%s%s modification of channel %d%s",
                                      (chmod & CHANNELMOD_NAME  )?" NAME"  :"",
                                      (chmod & CHANNELMOD_PIDS  )?" PIDS"  :"",
                                      (chmod & CHANNELMOD_ID    )?" ID"    :"",
                                      (chmod & CHANNELMOD_CA    )?" CA"    :"",
                                      (chmod & CHANNELMOD_TRANSP)?" TRANSP":"",
                                      (chmod & CHANNELMOD_LANGS )?" LANGS" :"",
                                      Channel->Number(),
                                      ((Channel->Ca() >= CA_MCLI_MIN) && (Channel->Ca() <= CA_MCLI_MAX) && cDevice::ActualDevice()->HasInternalCam())?" (InternalCam)":"");
                              Channels.SwitchTo(Channel->Number());
                              }
                           }
                        }
                     }
#else
                  if (Channel->Modification(CHANNELMOD_RETUNE)) {
                     cRecordControls::ChannelDataModified(Channel);
                     if (Channel->Number() == cDevice::CurrentChannel()) {
                        if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring()) {
                           if (cDevice::ActualDevice()->ProvidesTransponder(Channel)) { // avoids retune on devices that don't really access the transponder
                              isyslog("retuning due to modification of channel %d", Channel->Number());
                              Channels.SwitchTo(Channel->Number());
                              }
                           }
                        }
                     }
#endif /*USE_MCLI*/
                  }
              Channels.Unlock();
              }
           }
        // Channel display:
        if (!EITScanner.Active() && cDevice::CurrentChannel() != LastChannel) {
	    //&& ShutdownHandler.GetActiveMode() != unknown) {
           if (!Menu)
              Menu = new cDisplayChannel(cDevice::CurrentChannel(), LastChannel >= 0);
           LastChannel = cDevice::CurrentChannel();
           LastChannelChanged = Now;
           }
        if (Now - LastChannelChanged >= Setup.ZapTimeout && LastChannel != PreviousChannel[PreviousChannelIndex])
           PreviousChannel[PreviousChannelIndex ^= 1] = LastChannel;
        // Timers and Recordings:
        if (!Timers.BeingEdited()) {
#if USEMYSQL
        // If using MySQL then sync Data
          if (Setup.ReelboxModeTemp == eModeServer)
              Timers.SyncData();

          if (Setup.ReelboxModeTemp != eModeClient) // Clients don't do timer-actions
          {
#endif
           // Assign events to timers:
           Timers.SetEvents();
           // Must do all following calls with the exact same time!
           // Process ongoing recordings:
           cRecordControls::Process(Now);
           // Start new recordings:
           cTimer *Timer = Timers.GetMatch(Now);
           if (Timer) {
              if (!cRecordControls::Start(Timer))
                 Timer->SetPending(true);
              else
                 LastTimerChannel = Timer->Channel()->Number();
              }
           // Make sure timers "see" their channel early enough:
           static time_t LastTimerCheck = 0;
           if (Now - LastTimerCheck > TIMERCHECKDELTA) { // don't do this too often
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
                              }
                           }
#if REELVDR
                        InhibitEpgScan |= InVpsMargin | NeedsTransponder;
			if (tmpInhibitEpgScan != InhibitEpgScan) {
			    if (InhibitEpgScan)
			    {
				dsyslog("Disabling EPG scan. InVpsMargin %d, NeedsTransponder %d", InVpsMargin, NeedsTransponder);
			    }
			    else
				dsyslog("Enabling EPG scan");
			    tmpInhibitEpgScan = InhibitEpgScan;
			}
#else
                        InhibitEpgScan |= InVpsMargin | NeedsTransponder;
#endif
                        }
                     else
                        NeedsTransponder = Timer->Matches(Now, true, TIMERLOOKAHEADTIME);
                     }
                  if (NeedsTransponder || InVpsMargin) {
                     // Find a device that provides the required transponder:
#ifdef REELVDR
                     cDevice *UseableDevice = NULL;
#endif
                     cDevice *Device = NULL;
                     bool DeviceAvailable = false;
                     for (int i = 0; i < cDevice::NumDevices(); i++) {
                         cDevice *d = cDevice::GetDevice(i);
                         if (d && d->ProvidesTransponder(Timer->Channel())) {
                            if (d->IsTunedToTransponder(Timer->Channel())) {
                               // if any device is tuned to the transponder, we're done
                               Device = d;
                               break;
                               }
                            bool timeout = Now - DeviceUsed[d->DeviceNumber()] > TIMERDEVICETIMEOUT; // only check other devices if they have been left alone for a while
                            if (d->MaySwitchTransponder()) {
                               DeviceAvailable = true; // avoids using the actual device below
                               if (timeout)
                                  Device = d; // only check other devices if they have been left alone for a while
                               }
                            else if (timeout && !Device && InVpsMargin && !d->Receiving() && d->ProvidesTransponderExclusively(Timer->Channel()))
                               Device = d; // use this one only if no other with less impact can be found
#ifdef REELVDR
                            else if (timeout && !Device && InVpsMargin && !d->Receiving())
                               UseableDevice = d; // use this one only if no other with less impact can be found
#endif
                            }
                         }
                     if (!Device && InVpsMargin && !DeviceAvailable) {
#ifdef REELVDR
                        if(cDevice::PrimaryDevice() == cDevice::ActualDevice()) { // No Live-TV so use last known good device since our PrimaryDevice has no tuner
                            Device = UseableDevice;
                        } else {
#endif
                        cDevice *d = cDevice::ActualDevice();
                        //DDD("ActualDevice InVpsMargin %d DeviceAvailable %d Receiving %d ProvidesTransponder %d Channel %s devtimeout %d", InVpsMargin, DeviceAvailable, d->Receiving(), d->ProvidesTransponder(Timer->Channel()),
			//                              Timer->Channel()->Name(),
			//                              Now - DeviceUsed[d->DeviceNumber()] > TIMERDEVICETIMEOUT);
                        if (!d->Receiving() && d->ProvidesTransponder(Timer->Channel()) && Now - DeviceUsed[d->DeviceNumber()] > TIMERDEVICETIMEOUT)
                           Device = d; // use the actual device as a last resort
#ifdef REELVDR
                        }
#endif
                        }
                     // Switch the device to the transponder:
                     if (Device) {
#ifdef REELVDR
                        bool hadProgramme = cDevice::PrimaryDevice()->HasProgramme();
#endif /*REELVDR*/
                        if (!Device->IsTunedToTransponder(Timer->Channel())) {
                           if (Device == cDevice::ActualDevice() && !Device->IsPrimaryDevice())
                              cDevice::PrimaryDevice()->StopReplay(); // stop transfer mode
                           dsyslog("switching device %d to channel %d", Device->DeviceNumber() + 1, Timer->Channel()->Number());
                           Device->SwitchChannel(Timer->Channel(), false);
                           DeviceUsed[Device->DeviceNumber()] = Now;
                           }
#ifdef REELVDR
                        if (cDevice::PrimaryDevice()->HasDecoder() && !cDevice::PrimaryDevice()->HasProgramme() && hadProgramme) {
#else
                        if (cDevice::PrimaryDevice()->HasDecoder() && !cDevice::PrimaryDevice()->HasProgramme()) {
#endif /*REELVDR*/
                           // the previous SwitchChannel() has switched away the current live channel
                           cDevice::SetAvoidDevice(Device);
                           if (!Channels.SwitchTo(cDevice::CurrentChannel())) // try to switch to the original channel on a different device...
                              Channels.SwitchTo(Timer->Channel()->Number()); // ...or avoid toggling between old channel and black screen
                           Skins.Message(mtInfo, tr("Upcoming recording!"));
                           }
                        }
                     }
                  }
              LastTimerCheck = Now;
              }
           // Delete expired timers:
           Timers.DeleteExpired();
           }
#if USEMYSQL
        }
#endif
        if (!Menu && Recordings.NeedsUpdate()) {
           Recordings.Update();
           DeletedRecordings.Update();
           }

#ifdef REELVDR // from reel vdr-1.4

    /* show netcvupdate / install menus only in active mode */
    if(ShutdownHandler.GetActiveMode() == active) {

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
 } // active mode

#endif

#ifdef USE_MCLI
        cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
        if (mcliPlugin) {
           if (!ShutdownHandler.countdown) { // if kPower has been pressed, cMenuShutdown takes precedence over other menus
              cOsdObject *MyMenu = mcliPlugin->AltMenuAction();
              if (MyMenu) { // is there any cam-menu waiting?
                 DELETE_MENU;
                 if (cControl::Control())
                    cControl::Control()->Hide();
                    Menu = MyMenu;
                    Menu->Show();
                 }
              }
           }
#endif /* MCLI */
        // CAM control:
        if (!Menu && !cOsd::IsOpen())
           Menu = CamControl();

        // Queued messages:
#ifdef REELVDR
        /* Show Queued messages even when an OSD is open
        ** Get keys and use it if key != { kNone, kOk, kBack }
        */
        eKeys key = Skins.ProcessQueuedMessages();

        // User Input:
        cOsdObject *Interact = Menu ? Menu : cControl::Control();

        if (key == kNone || key == kOk || key == kBack)
            key = Interface->GetKey(!Interact || !Interact->NeedsFastResponse());

#else
        if (!Skins.IsOpen())
           Skins.ProcessQueuedMessages();

        // User Input:
        cOsdObject *Interact = Menu ? Menu : cControl::Control();

        eKeys key = Interface->GetKey(!Interact || !Interact->NeedsFastResponse());
#endif /* REELVDR */

#if REELVDR
        // Make sure requested mode has been acted up before ignoring keys
        // eg. during startup (unknown mode) k_Plugin from netcvupdate is missed
        ShutdownHandler.ActOnActiveMode();

        if (ShutdownHandler.GetActiveMode() != active)
        {
            switch (key)
            {
                case kPower:
                     DDD("Hot-Standby - Power pressed");
                     SystemExec("shutdownwd.sh cancel");
                     ShutdownHandler.RequestNewMode(active);
                     break;

                case kBack:
                     DDD("Hot-Standby - Exit pressed");
                     ShutdownHandler.RequestNewMode(poweroff);
                     break;

                default:
                    if (key != kNone)
                        DDD("Hot-Standby - key %d ignored", key);
                    break;
            } // switch
            key = kNone;

            switch (ShutdownHandler.GetActiveMode())
            {
                case standby: // check for power Saving time out to poweroff
                    if(Setup.EnergySaveOn && standbyCountdown.TimedOut()
                            && Setup.StandbyTimeout < 241) // 241mins => never poweroff
                       ShutdownHandler.RequestNewMode(poweroff);
                    break;

                case poweroff:// maybe in "standby mode" waiting for background
                              //processes to finish, so check with ConfirmShutdown

                  if (ShutdownHandler.ConfirmShutdown(false))
                  {
                     ShutdownHandler.DoShutdown(true);
                     ShutdownHandler.Exit(0);
                  }
                  else
                     DDD("ActiveState: poweroff  ConfirmShutdown() returns false");
                  break;

               case maintenance: // no check, fallthrough
               case restart: // no check
                    ShutdownHandler.DoShutdown(true);
                    ShutdownHandler.Exit(0);
                    break;

               default: break;
            }// switch

        } //if (ShutdownHandler.GetActiveMode() != active)
        else if ((Now - LastInteract) > ACTIVITYTIMEOUT) {
          //vdr in ActiveMode = active


            // start Standby countdown after the user has been inactive
            // for Setup.MinUserInactivity minutes (unlike vanilla vdr 1.7, which looks ahead)
            if (ShutdownHandler.IsUserInactive(Now) && !ShutdownHandler.countdown) {
                    ShutdownHandler.countdown.Start(tr("ReelBox will standby in %s minutes"), SHUTDOWNWAIT);
            }

            // Countdown run down to 0?
            if (ShutdownHandler.countdown.Done()) {
                    ShutdownHandler.RequestNewMode(standby);
            }
        }

        // changes in Mode are handled here immediately. Eg. standby and poweroff
        ShutdownHandler.ActOnActiveMode();

        if (key == kMenu && cDevice::PrimaryDevice()->Replaying()
              && cDevice::PrimaryDevice()->PlayerCanHandleMenuCalls()
              && !Menu && !cDevice::PrimaryDevice()->DVDPlayerIsInMenuDomain())
            key = k3;
#endif /* REELVDR */

        if (ISREALKEY(key)) {
#ifdef USE_PINPLUGIN
           cStatus::MsgUserAction(key, Interact);
#endif /* PINPLUGIN */
           EITScanner.Activity();
           // Cancel shutdown countdown:
           if (ShutdownHandler.countdown)
              ShutdownHandler.countdown.Cancel();
           // Set user active for MinUserInactivity time in the future:
           ShutdownHandler.SetUserInactiveTimeout();
           }
        // Keys that must work independent of any interactive mode:
        switch (int(key)) {
          // Menu control:
#ifndef REELVDR
          case kMenu: {
               key = kNone; // nobody else needs to see this key
               bool WasOpen = Interact != NULL;
               bool WasMenu = Interact && Interact->IsMenu();
               if (Menu)
                  DELETE_MENU;
               else if (cControl::Control()) {
                  if (cOsd::IsOpen())
                     cControl::Control()->Hide();
                  else
                     WasOpen = false;
                  }
               if (!WasOpen || !WasMenu && !Setup.MenuKeyCloses)
                  Menu = new cMenuMain;
               }
               break;
          // Info:
          case kInfo: {
               if (IsInfoMenu) {
                  key = kNone; // nobody else needs to see this key
                  DELETE_MENU;
                  }
               else if (!Menu) {
                  IsInfoMenu = true;
                  if (cControl::Control()) {
                     cControl::Control()->Hide();
                     Menu = cControl::Control()->GetInfo();
                     if (Menu)
                        Menu->Show();
                     else
                        IsInfoMenu = false;
                     }
                  else {
                     cRemote::Put(kOk, true);
                     cRemote::Put(kSchedule, true);
                     }
                  key = kNone; // nobody else needs to see this key
                  }
               }
               break;
#endif /* !REELVDR */

#ifdef REELVDR
          // Direct main menu functions:

          #define DirectMainFunction(function)                         \
                  {                                                    \
                     if(Menu)                                          \
                       DELETE_MENU;                                    \
                     else                                              \
                       LastMenuMainFunc = osEnd;                       \
                     if (cControl::Control())                          \
                        cControl::Control()->Hide();                   \
                     if (LastMenuMainFunc != function)  \
                     {                                                 \
                        LastMenuMainFunc = function;                   \
                        CREATE_MENU_MAIN(function,Menu);               \
                     }                                                 \
                  }                                                    \
                  key = kNone; // nobody else needs to see this key

            // kMultimedia
          case kMultimedia: {
            cPlugin *p = cPluginManager::GetPlugin("setup");
            static const char* multimedia = "Multimedia";
            if (p)
            {
                // close any open menu
                if(Menu)
                    DELETE_MENU;

                p->Service("link", (void*) multimedia );
                key = kNone; // nobody else needs to see this key

                cRemote::CallPlugin("setup");
            }
          }
            break;

          // Info:
          case kInfo: {
            // handle kInfo
            // show channel information when live TV and livebuffer
            // show recording information when replaying recordings
            // shows channellist options (Not here, see code below) when
            //   channellist osd (Menu!=NULL) is open

            if (IsInfoMenu) {
                key = kNone; // nobody else needs to see this key
                DELETE_MENU;
                IsInfoMenu=false;
                }
            else if (!Menu) {

                if (cControl::Control()
      #ifdef USE_LIVEBUFFER
                        && cRecordControls::GetLiveIndex(cReplayControl::NowReplaying()) == NULL /*not live buffer*/
      #endif
                        ) {
                    IsInfoMenu = true;

                   cControl::Control()->Hide();
                   Menu = cControl::Control()->GetInfo();

                   if (Menu)
                      Menu->Show();
                   else
                      IsInfoMenu = false;
                   }
                else {
                    /* //XXX why show EPG ?
                    cRemote::Put(kOk, true);
                    cRemote::Put(kSchedule, true);
                    */
                    if ((Setup.WantChListOnOk && (key == kOk)) ||
                        (!Setup.WantChListOnOk && (key == kInfo))) {
                        DirectMainFunction(osChannels);
                    } else
                        LastChannel = -1;
                   }
                key = kNone; // nobody else needs to see this key
            }
          }
          break;

          case kMenu: {
               key = kNone; // nobody else needs to see this key
               bool WasOpen = Interact != NULL;
               bool WasMenu = Interact && Interact->IsMenu();
               if (Menu)
                  DELETE_MENU;
               else if (cControl::Control()) {
                  if (cOsd::IsOpen())
                     cControl::Control()->Hide();
                  else
                     WasOpen = false;
                  }
               if (!WasOpen || !WasMenu )//&& !Setup.MenuKeyCloses)
                   //Menu = new cMenuMain;
                   DirectMainFunction(osUnknown);
               }
               break;
          case kSearch:     DirectMainFunction(osSearchtimers); break;
          case kHelp:       DirectMainFunction(osActiveEvent);  break;
          case kAddFavorite:
          case k2digit:     DirectMainFunction(osAddFavourite); break;
          case kFavourites: DirectMainFunction(osFavourites); break;
          case kReel: if(!Menu) break;
              cOsd::SetOsd3D(!cOsd::Osd3D());
              continue;
          // Aspect ratio
          case kAspect:
              ::Setup.VideoFormat == 0 ? ::Setup.VideoFormat=1: ::Setup.VideoFormat=0;
              cDevice::PrimaryDevice()->SetVideoFormat(::Setup.VideoFormat);
              key = kNone;
              break;
          case kEject:      Eject(); key = kNone; break;
#else
          #define DirectMainFunction(function)\
            DELETE_MENU;\
            if (cControl::Control())\
               cControl::Control()->Hide();\
            Menu = new cMenuMain(function);\
            key = kNone; // nobody else needs to see this key
#endif /* REELVDR */
          case kSchedule:   DirectMainFunction(osSchedule); break;
          case kChannels:   DirectMainFunction(osChannels); break;
          case kTimers:     DirectMainFunction(osTimers); break;
          case kRecordings: DirectMainFunction(osRecordings); break;
          case kSetup:
            {
#ifdef USE_PINPLUGIN
                cPlugin *plugin = cPluginManager::GetPlugin("setup");
                if (plugin && !cStatus::MsgPluginProtected(plugin))
                {
#endif /* PINPLUGIN */

                    DirectMainFunction(osSetup);

#ifdef USE_PINPLUGIN
                }
#endif /* PINPLUGIN */
                    break;
            }
          case kCommands:   DirectMainFunction(osCommands); break;
          case kUser0 ... kUser9: cRemote::PutMacro(key); key = kNone; break;
          case k_Plugin: {
               const char *PluginName = cRemote::GetPlugin();
               if (PluginName) {
                  DELETE_MENU;
                  if (cControl::Control())
                     cControl::Control()->Hide();
                  cPlugin *plugin = cPluginManager::GetPlugin(PluginName);
                  if (plugin) {
#ifdef USE_PINPLUGIN
                  if (!cStatus::MsgPluginProtected(plugin)) {
#endif /* PINPLUGIN */
                     Menu = plugin->MainMenuAction();
                     if (Menu)
                        Menu->Show();
#ifdef USE_PINPLUGIN
                     }
#endif /* PINPLUGIN */
                  }
                  else
                     esyslog("ERROR: unknown plugin '%s'", PluginName);
                  }
               key = kNone; // nobody else needs to see these keys
               }
               break;
          // Channel up/down:
          case kChanUp|k_Repeat:
          case kChanUp:
          case kChanDn|k_Repeat:
          case kChanDn:
#ifdef REELVDR
             {
               eOSState state = osUnknown;
               if (Setup.ChannelUpDownKeyMode == 1 && (!Menu || cDisplayChannel::IsOpen()))
               // only if a menu is not open
               //  and user has opted for a different key behavior
               {
                   if (cDisplayChannel::IsOpen() || cControl::Control())
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
               else
               if (!Interact)
                  Menu = new cDisplayChannel(NORMALKEY(key));
               // for PiP pi, always do the ProcessKey()
               else {
                  state = Interact->ProcessKey(key);
                  if (state == osEnd) { // cDisplayVolume returns osEnd
                      DELETE_MENU;
                      continue;
                  } else
                  if (state != osUnknown)
                      continue;
                  else
                  // if key is not handled lets switch channel
                  cDevice::SwitchChannel(NORMALKEY(key) == kChanUp ? 1 : -1);
               }
               /// channel switch already happens inside cDisplayChannel constructor
               /// donot do it again here
               //if (state == osUnknown)
               //   cDevice::SwitchChannel(NORMALKEY(key) == kChanUp ? 1 : -1);
            }
#else
               if (!Interact)
                  Menu = new cDisplayChannel(NORMALKEY(key));
               else if (cDisplayChannel::IsOpen() || cControl::Control()) {
                  Interact->ProcessKey(key);
                  continue;
                  }
               else
                  cDevice::SwitchChannel(NORMALKEY(key) == kChanUp ? 1 : -1);
#endif
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
               if (cControl::Control())
                  cControl::Control()->Hide();
               if (!cDisplayTracks::IsOpen()) {
                  DELETE_MENU;
                  Menu = cDisplayTracks::Create();
                  }
               else
                  cDisplayTracks::Process(key);
               key = kNone;
               break;
          // Subtitle track control:
          case kSubtitles:
#ifdef REELVDR
          case kGreater:
               if(!Menu || cDisplaySubtitleTracks::IsOpen()) {
#endif
               if (cControl::Control())
                  cControl::Control()->Hide();
               if (!cDisplaySubtitleTracks::IsOpen()) {
                  DELETE_MENU;
                  Menu = cDisplaySubtitleTracks::Create();
                  }
               else
                  cDisplaySubtitleTracks::Process(key);
               key = kNone;
#ifdef REELVDR
              }
#endif
               break;
#ifdef USE_LIVEBUFFER
          case kFastRew:
               if (!Interact) {
                  DELETE_MENU;
                  if(cRecordControls::StartLiveBuffer(key))
                     key = kNone;
               } // if
               break;
#endif /*USE_LIVEBUFFER*/
          // Pausing live video:
          case kPause:
#ifdef REELVDR
               if (!cControl::Control()) {
//                if((Setup.ReelboxModeTemp==eModeClient) || (Setup.ReelboxModeTemp == eModeStandalone && isNetClient) || (Setup.LiveBuffer==0))
//                if((Setup.ReelboxModeTemp==eModeClient || (Setup.ReelboxModeTemp == eModeStandalone && isNetClient)) && (Setup.LiveBuffer==0))
                  if (Setup.LiveBuffer == 0 && Setup.PauseKeyHandling == 0)
                     Skins.Message(mtWarning, tr("Timeshift is not available!"));
                  else
                  {
                     DELETE_MENU;
                     if (Setup.LiveBuffer == 1
                         || Setup.PauseKeyHandling == 2
                         || (Setup.PauseKeyHandling == 1 && Interface->Confirm(tr("Pause live video?")))
                        ) {
                         if (!cRecordControls::PauseLiveVideo())
                             Skins.Message(mtError, tr("No free tuner to record!"));
                     }
                  }
                  key = kNone; // nobody else needs to see this key
               }
               break;
#else
               if (!cControl::Control()) {
                  DELETE_MENU;
                  if (Setup.PauseKeyHandling) {
                     if (Setup.PauseKeyHandling > 1 || Interface->Confirm(tr("Pause live video?"))) {
                        if (!cRecordControls::PauseLiveVideo())
                           Skins.Message(mtError, tr("No free DVB device to record!"));
                        }
                     }
                  key = kNone; // nobody else needs to see this key
                  }
               break;
#endif /*REELVDR*/
          // Instant recording:
          case kRecord:
               if (!cControl::Control()) {
                  if (cRecordControls::Start())
                     Skins.Message(mtInfo, tr("Recording started"));
                  key = kNone; // nobody else needs to see this key
                  }
               break;
          // Power off:
          case kPower:
               isyslog("Power button pressed");
               DELETE_MENU;
#ifdef REELVDR
               // close player/control menus, if any
               // if not shutdown menu 'opens' but is not visible on the OSD
               if (cControl::Control())
                   cControl::Control()->Hide();

               SystemExec("shutdownwd.sh cancel"); // donot auto kill vdr

               //< Look into setup.RequestShutDownMode
               //< 0 = request 1 = standby 2 = poweroff
               switch(Setup.RequestShutDownMode)
               {
                   case 0: // opens menu
                       Menu = new cMenuShutdown();
                       break;

                   case 1: // standby
                       ShutdownHandler.RequestNewMode(standby);
                       break;

                   case 2: // poweroff
                       ShutdownHandler.RequestNewMode(poweroff);
                       break;

                   default:
                       isyslog("Setup.RequestShutdownMode: %d not understood. Ignored.",
                           Setup.RequestShutDownMode);
                       break;
               } // switch


               //ReelVDR: shutdown script does not send a kill so we need:
               //ShutdownHandler.Exit(0);
#else
               // Check for activity, request power button again if active:
               if (!ShutdownHandler.ConfirmShutdown(false) && Skins.Message(mtWarning, tr("ReelBox will shut down later - press Power to force"), SHUTDOWNFORCEPROMPT) != kPower) {
                  // Not pressed power - set VDR to be non-interactive and power down later:
                  ShutdownHandler.SetUserInactive();
                  break;
                  }
               // No activity or power button pressed twice - ask for confirmation:
               if (!ShutdownHandler.ConfirmShutdown(true)) {
                  // Non-confirmed background activity - set VDR to be non-interactive and power down later:
                  ShutdownHandler.SetUserInactive();
                  break;
                  }
               // Ask the final question:
               if (!Interface->Confirm(tr("Press any key to cancel shutdown"), SHUTDOWNCANCELPROMPT, true))
                  // If final question was canceled, continue to be active:
                  break;
               // Ok, now call the shutdown script:
               ShutdownHandler.DoShutdown(true);
               // Set VDR to be non-interactive and power down again later:
               ShutdownHandler.SetUserInactive();
               // Do not attempt to automatically shut down for a while:
               ShutdownHandler.SetRetry(SHUTDOWNRETRY);
#endif
               break;
          default: break;
          }
//#ifdef REELVDR
//        }
//#endif

        Interact = Menu ? Menu : cControl::Control(); // might have been closed in the mean time
        if (Interact) {
           LastInteract = Now;
           eOSState state = Interact->ProcessKey(key);
           if (state == osUnknown && Interact != cControl::Control()) {
              if (ISMODELESSKEY(key) && cControl::Control()) {
                 state = cControl::Control()->ProcessKey(key);
                 if (state == osEnd) {
                    // let's not close a menu when replay ends:
                    cControl::Shutdown();
                    continue;
                    }
                 }
              else if (Now - cRemote::LastActivity() > MENUTIMEOUT)
                 state = osEnd;
#ifdef REELVDR
              } else cRemote::TriggerLastActivity();
#else
              }
#endif
           switch (state) {
             case osPause:  DELETE_MENU;
                            cControl::Shutdown(); // just in case
                            if (!cRecordControls::PauseLiveVideo())
                               Skins.Message(mtError, tr("No free DVB device to record!"));
                            break;
             case osRecord: DELETE_MENU;
                            if (cRecordControls::Start())
                               Skins.Message(mtInfo, tr("Recording started"));
                            break;
             case osRecordings:
                            DELETE_MENU;
                            cControl::Shutdown();
#ifdef REELVDR
                            DirectMainFunction(osRecordings);
                            if (Menu)
                               Menu->Show();
#else
                            Menu = new cMenuMain(osRecordings);
#endif
                            CheckHasProgramme = true; // to have live tv after stopping replay with 'Back'
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
             case osEnd:    if (Interact == Menu)
                               DELETE_MENU;
                            else
                               cControl::Shutdown();
                            break;
#ifdef USE_LIVEBUFFER
             case osSwitchChannel:
                            switch (key) {
                                // Toggle channels:
                                case kChanPrev:
                                case k0: {
                                    if (PreviousChannel[PreviousChannelIndex ^ 1] == LastChannel
                                        || (LastChannel != PreviousChannel[0] && LastChannel != PreviousChannel[1]))
                                        PreviousChannelIndex ^= 1;
                                    Channels.SwitchTo(PreviousChannel[PreviousChannelIndex ^= 1]);
                                    break;
                                }
                                case k1 ... k9:
                                    DELETE_MENU;
                                    cControl::Shutdown();
                                    Menu = new cDisplayChannel(NORMALKEY(key));
                                    break;
                                default:
                                    break;
                            } // switch
                            break;
#endif /*USE_LIVEBUFFER*/
#ifdef REELVDR
             case osUnknown: // Key functions in "normal" viewing mode:
                            if (key != kNone && KeyMacros.Get(key)) {
                               cRemote::PutMacro(key);
                               key = kNone;
                            }
#endif /*REELVDR*/
             default:       ;
             }
           }
        else {
           // Key functions in "normal" viewing mode:
           if (key != kNone && KeyMacros.Get(key)) {
              cRemote::PutMacro(key);
              key = kNone;
              }
           switch (int(key)) {
             // Toggle channels:
             case kChanPrev:
             case k0: {
                  if (PreviousChannel[PreviousChannelIndex ^ 1] == LastChannel
                      || (LastChannel != PreviousChannel[0] && LastChannel != PreviousChannel[1]))
                     PreviousChannelIndex ^= 1;
                  Channels.SwitchTo(PreviousChannel[PreviousChannelIndex ^= 1]);
                  break;
                  }
#ifdef USE_VOLCTRL
             // Left/Right volume control
#else
             // Direct Channel Select:
             case k1 ... k9:
             // Left/Right rotates through channel groups:
#endif /* VOLCTRL */
             case kLeft|k_Repeat:
             case kLeft:
             case kRight|k_Repeat:
             case kRight:
#ifdef USE_VOLCTRL
                  if (Setup.LRVolumeControl && Setup.LRChannelGroups < 2) {
                     cRemote::Put(NORMALKEY(key) == kLeft ? kVolDn : kVolUp, true);
                     break;
                     }
                  // else fall through
             // Direct Channel Select:
             case k1 ... k9:
#endif /* VOLCTRL */
             // Previous/Next rotates through channel groups:
             case kPrev|k_Repeat:
             case kPrev:
             case kNext|k_Repeat:
             case kNext:
             // Up/Down Channel Select:
             case kUp|k_Repeat:
             case kUp:
             case kDown|k_Repeat:
             case kDown:
                  Menu = new cDisplayChannel(NORMALKEY(key));
                  break;
             // Viewing Control:
#ifdef REELVDR
             case kInfo:
             case kOk:  if ((Setup.WantChListOnOk && (key == kOk))
                            || (!Setup.WantChListOnOk && (key == kInfo)) )
                        {
                            switch (Setup.WantChListOnOk)
                            {
                            case 3:
                                DirectMainFunction(osBouquets);
                                break;
                            case 2:
                                cRemote::PutMacro(kYellow); // open favourites
                                break;
                            default:
                                DirectMainFunction(osChannels);
                                break;
                            }
                        }
                        else
                        {
                            LastChannel = -1;
                        }
                        break;
#else
             case kOk:   LastChannel = -1; break; // forces channel display
#endif
             // Instant resume of the last viewed recording:
             case kPlay:
                  if (cReplayControl::LastReplayed()) {
#ifdef USE_PINPLUGIN
                     if (cStatus::MsgReplayProtected(0, cReplayControl::LastReplayed(), 0, false) == false) {
#endif /* PINPLUGIN */
#if REELVDR // Do not replayback timeshifts/livebuffer
                         cReplayControl::SetRecording(
                                     cReplayControl::LastReplayed(),
                                     cReplayControl::LastReplayedTitle()
                                     );
#endif /* REELVDR */
                     cControl::Shutdown();
                     cControl::Launch(new cReplayControl);
                     }
#ifdef USE_PINPLUGIN
                     }
#endif /* PINPLUGIN */
                  break;
#if REELVDR
             case kStop:
                 DDD("pressed kStop");
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
#endif /*USEMYSQL*/
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
#endif /*REELVDR*/
             default:    break;
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

        // SIGHUP shall cause a restart:
        if (LastSignal == SIGHUP) {
           if (ShutdownHandler.ConfirmRestart(true) && Interface->Confirm(tr("Press any key to cancel restart"), RESTARTCANCELPROMPT, true))
              EXIT(1);
           LastSignal = 0;
           }

        // Update the shutdown countdown:
        if (ShutdownHandler.countdown && ShutdownHandler.countdown.Update()) {
           if (!ShutdownHandler.ConfirmShutdown(false))
              ShutdownHandler.countdown.Cancel();
           //DDD("ShutdownHandler.countdown.Cancel()");
           }
#if 0
        if (ShutdownHandler.IsUserInactive())
                  { DDD("non-interactive mode"); }
        else
                  { DDD("interactive (user) mode"); }
#endif

        if ((Now - LastInteract) > ACTIVITYTIMEOUT && !cRecordControls::Active() && !cCutter::Active()
             && !Interface->HasSVDRPConnection() && (Now - cRemote::LastActivity()) > ACTIVITYTIMEOUT) {
           // Handle housekeeping tasks

           // Shutdown:
#ifdef REELVDR
           // countdown for standby is done above,
           // before cShutdownHandler::ActOnActiveMode()
#else
           // Check whether VDR will be ready for shutdown in SHUTDOWNWAIT seconds:
           time_t Soon = Now + SHUTDOWNWAIT;
           if (ShutdownHandler.IsUserInactive(Soon) && ShutdownHandler.Retry(Soon) && !ShutdownHandler.countdown) {
              if (ShutdownHandler.ConfirmShutdown(false))
                 // Time to shut down - start final countdown:
                 ShutdownHandler.countdown.Start(tr("ReelBox will shut down in %s minutes"), SHUTDOWNWAIT); // the placeholder is really %s!
              // Dont try to shut down again for a while:
              ShutdownHandler.SetRetry(SHUTDOWNRETRY);
              }
           // Countdown run down to 0?
           if (ShutdownHandler.countdown.Done()) {
              // Timed out, now do a final check:
              if (ShutdownHandler.IsUserInactive() && ShutdownHandler.ConfirmShutdown(false))
                 ShutdownHandler.DoShutdown(false);
              // Do this again a bit later:
              ShutdownHandler.SetRetry(SHUTDOWNRETRY);
              }
#endif

           // Disk housekeeping:
           RemoveDeletedRecordings();
           cSchedules::Cleanup();
           // Plugins housekeeping:
           PluginManager.Housekeeping();
           }
        // Main thread hooks of plugins:
        PluginManager.MainThreadHook();
        }

  if (ShutdownHandler.EmergencyExitRequested())
     esyslog("emergency exit requested - shutting down");

Exit:

  // Reset all signal handlers to default before Interface gets deleted:
  signal(SIGHUP,  SIG_DFL);
  signal(SIGINT,  SIG_DFL);
  signal(SIGTERM, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);
  signal(SIGALRM, SIG_DFL);

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
  SourceParams.Clear();
  if (ShutdownHandler.GetExitCode() != 2) {
     Setup.CurrentChannel = cDevice::CurrentChannel();
     Setup.CurrentVolume  = cDevice::CurrentVolume();
     Setup.Save();

#ifdef REELVDR
     Timers.Save(AddDirectory(ConfigDirectory, "timers.conf"));
#endif
     }
  cDevice::Shutdown();
  PluginManager.Shutdown(true);
  cSchedules::Cleanup(true);
  ReportEpgBugFixStats();
  if (WatchdogTimeout > 0)
     dsyslog("max. latency time %d seconds", MaxLatencyTime);
  if (LastSignal)
     isyslog("caught signal %d", LastSignal);
  if (ShutdownHandler.EmergencyExitRequested())
     esyslog("emergency exit!");
  isyslog("exiting, exit code %d", ShutdownHandler.GetExitCode());
  if (SysLogLevel > 0)
     closelog();
  if (HasStdin)
     tcsetattr(STDIN_FILENO, TCSANOW, &savedTm);
  return ShutdownHandler.GetExitCode();
}
