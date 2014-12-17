/*
 * svdrp.c: Simple Video Disk Recorder Protocol
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * The "Simple Video Disk Recorder Protocol" (SVDRP) was inspired
 * by the "Simple Mail Transfer Protocol" (SMTP) and is fully ASCII
 * text based. Therefore you can simply 'telnet' to your VDR port
 * and interact with the Video Disk Recorder - or write a full featured
 * graphical interface that sits on top of an SVDRP connection.
 *
 * $Id: svdrp.c 1.103 2007/08/25 09:28:26 kls Exp $
 */

#include "svdrp.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "device.h"
#include "eitscan.h"
#include "keys.h"
#include "menu.h"
#include "plugin.h"
#include "remote.h"
#include "skins.h"
#include "timers.h"
#include "tools.h"
#include "videodir.h"

extern bool requestQuickStandby;   // declared in vdr.c
extern int Interrupted;            // declared in vdr.c
extern eShutdownMode shutdownMode; // declared in vdr.c

// --- cSocket ---------------------------------------------------------------

cSocket::cSocket(int Port, int Queue)
{
  port = Port;
  sock = -1;
  queue = Queue;
}

cSocket::~cSocket()
{
  Close();
}

void cSocket::Close(void)
{
  if (sock >= 0) {
     close(sock);
     sock = -1;
     }
}

bool cSocket::Open(void)
{
  if (sock < 0) {
     // create socket:
     sock = socket(PF_INET, SOCK_STREAM, 0);
     if (sock < 0) {
        LOG_ERROR;
        port = 0;
        return false;
        }
     // allow it to always reuse the same port:
     int ReUseAddr = 1;
     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ReUseAddr, sizeof(ReUseAddr));
     //
     struct sockaddr_in name;
     name.sin_family = AF_INET;
     name.sin_port = htons(port);
     name.sin_addr.s_addr = htonl(INADDR_ANY);
     if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        LOG_ERROR;
        Close();
        return false;
        }
     // make it non-blocking:
     int oldflags = fcntl(sock, F_GETFL, 0);
     if (oldflags < 0) {
        LOG_ERROR;
        return false;
        }
     oldflags |= O_NONBLOCK|FD_CLOEXEC;
     if (fcntl(sock, F_SETFL, oldflags) < 0) {
        LOG_ERROR;
        return false;
        }
     // listen to the socket:
     if (listen(sock, queue) < 0) {
        LOG_ERROR;
        return false;
        }
     }
  return true;
}

int cSocket::Accept(void)
{
  if (Open()) {
     struct sockaddr_in clientname;
     uint size = sizeof(clientname);
     int newsock = accept(sock, (struct sockaddr *)&clientname, &size);
     if (newsock > 0) {
        bool accepted = SVDRPhosts.Acceptable(clientname.sin_addr.s_addr);
        if (!accepted) {
           const char *s = "Access denied!\n";
           if (write(newsock, s, strlen(s)) < 0)
              LOG_ERROR;
           close(newsock);
           newsock = -1;
           }
        isyslog("connect from %s, port %hu - %s", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port), accepted ? "accepted" : "DENIED");
        }
     else if (errno != EINTR && errno != EAGAIN)
        LOG_ERROR;
     return newsock;
     }
  return -1;
}

// --- cPUTEhandler ----------------------------------------------------------

cPUTEhandler::cPUTEhandler(void)
{
  if ((f = tmpfile()) != NULL) {
     status = 354;
     message = "Enter EPG data, end with \".\" on a line by itself";
     }
  else {
     LOG_ERROR;
     status = 554;
     message = "Error while opening temporary file";
     }
}

cPUTEhandler::~cPUTEhandler()
{
  if (f)
     fclose(f);
}

bool cPUTEhandler::Process(const char *s)
{
  if (f) {
     if (strcmp(s, ".") != 0) {
        fputs(s, f);
        fputc('\n', f);
        return true;
        }
     else {
        rewind(f);
        if (cSchedules::Read(f)) {
           cSchedules::Cleanup(true);
           status = 250;
           message = "EPG data processed";
           }
        else {
           status = 451;
           message = "Error while processing EPG data";
           }
        fclose(f);
        f = NULL;
        }
     }
  return false;
}

// --- cSVDRP ----------------------------------------------------------------

#define MAXHELPTOPIC 10

const char *HelpPages[] = {
  "CHAN [ + | - | <number> | <name> | <id> ]\n"
  "    Switch channel up, down or to the given channel number, name or id.\n"
  "    Without option (or after successfully switching to the channel)\n"
  "    it returns the current channel number and name.",
  "CLRE\n"
  "    Clear the entire EPG list.",
  "DELC <number>\n"
  "    Delete channel.",
  "DELR <number>\n"
  "    Delete the recording with the given number. Before a recording can be\n"
  "    deleted, an LSTR command must have been executed in order to retrieve\n"
  "    the recording numbers. The numbers don't change during subsequent DELR\n"
  "    commands. CAUTION: THERE IS NO CONFIRMATION PROMPT WHEN DELETING A\n"
  "    RECORDING - BE SURE YOU KNOW WHAT YOU ARE DOING!",
  "DELT <number>\n"
  "    Delete timer.",
  "EDIT <number>\n"
  "    Edit the recording with the given number. Before a recording can be\n"
  "    edited, an LSTR command must have been executed in order to retrieve\n"
  "    the recording numbers.",
  "GRAB <filename> [ <quality> [ <sizex> <sizey> ] ]\n"
  "    Grab the current frame and save it to the given file. Images can\n"
  "    be stored as JPEG or PNM, depending on the given file name extension.\n"
  "    The quality of the grabbed image can be in the range 0..100, where 100\n"
  "    (the default) means \"best\" (only applies to JPEG). The size parameters\n"
  "    define the size of the resulting image (default is full screen).\n"
  "    If the file name is just an extension (.jpg, .jpeg or .pnm) the image\n"
  "    data will be sent to the SVDRP connection encoded in base64. The same\n"
  "    happens if '-' (a minus sign) is given as file name, in which case the\n"
  "    image format defaults to JPEG.",
  "HELP [ <topic> ]\n"
  "    The HELP command gives help info.",
  "HITK [ <key> ]\n"
  "    Hit the given remote control key. Without option a list of all\n"
  "    valid key names is given.",
  "LSTC [ :groups | <number> | <name> ]\n"
  "    List channels. Without option, all channels are listed. Otherwise\n"
  "    only the given channel is listed. If a name is given, all channels\n"
  "    containing the given string as part of their name are listed.\n"
  "    If ':groups' is given, all channels are listed including group\n"
  "    separators. The channel number of a group separator is always 0.",
  "LSTE [ <channel> ] [ now | next | at <time> ]\n"
  "    List EPG data. Without any parameters all data of all channels is\n"
  "    listed. If a channel is given (either by number or by channel ID),\n"
  "    only data for that channel is listed. 'now', 'next', or 'at <time>'\n"
  "    restricts the returned data to present events, following events, or\n"
  "    events at the given time (which must be in time_t form).",
  "LSTR [ <number> ]\n"
  "    List recordings. Without option, all recordings are listed. Otherwise\n"
  "    the information for the given recording is listed.",
  "LSTT [ <number> ] [ id ]\n"
  "    List timers. Without option, all timers are listed. Otherwise\n"
  "    only the given timer is listed. If the keyword 'id' is given, the\n"
  "    channels will be listed with their unique channel ids instead of\n"
  "    their numbers.",
  "MESG <message>\n"
  "    Displays the given message on the OSD. The message will be queued\n"
  "    and displayed whenever this is suitable.\n",
  "MODC <number> <settings>\n"
  "    Modify a channel. Settings must be in the same format as returned\n"
  "    by the LSTC command.",
  "MODT <number> on | off | <settings>\n"
  "    Modify a timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. The special keywords 'on' and 'off' can be\n"
  "    used to easily activate or deactivate a timer.",
  "MOVC <number> <to>\n"
  "    Move a channel to a new position.",
  "MOVT <number> <to>\n"
  "    Move a timer to a new position.",
  "NEWC <settings>\n"
  "    Create a new channel. Settings must be in the same format as returned\n"
  "    by the LSTC command.",
  "NEWT <settings>\n"
  "    Create a new timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. It is an error if a timer with the same channel,\n"
  "    day, start and stop time already exists.",
  "NEXT [ abs | rel ]\n"
  "    Show the next timer event. If no option is given, the output will be\n"
  "    in human readable form. With option 'abs' the absolute time of the next\n"
  "    event will be given as the number of seconds since the epoch (time_t\n"
  "    format), while with option 'rel' the relative time will be given as the\n"
  "    number of seconds from now until the event. If the absolute time given\n"
  "    is smaller than the current time, or if the relative time is less than\n"
  "    zero, this means that the timer is currently recording and has started\n"
  "    at the given time. The first value in the resulting line is the number\n"
  "    of the timer.",
  "PLAY <number> [ begin | <position> ]\n"
  "    Play the recording with the given number. Before a recording can be\n"
  "    played, an LSTR command must have been executed in order to retrieve\n"
  "    the recording numbers.\n"
  "    The keyword 'begin' plays the recording from its very beginning, while\n"
  "    a <position> (given as hh:mm:ss[.ff] or framenumber) starts at that\n"
  "    position. If neither 'begin' nor a <position> are given, replay is resumed\n"
  "    at the position where any previous replay was stopped, or from the beginning\n"
  "    by default. To control or stop the replay session, use the usual remote\n"
  "    control keypresses via the HITK command.",
  "PLUG <name> [ help | main ] [ <command> [ <options> ]]\n"
  "    Send a command to a plugin.\n"
  "    The PLUG command without any parameters lists all plugins.\n"
  "    If only a name is given, all commands known to that plugin are listed.\n"
  "    If a command is given (optionally followed by parameters), that command\n"
  "    is sent to the plugin, and the result will be displayed.\n"
  "    The keyword 'help' lists all the SVDRP commands known to the named plugin.\n"
  "    If 'help' is followed by a command, the detailed help for that command is\n"
  "    given. The keyword 'main' initiates a call to the main menu function of the\n"
  "    given plugin.\n",
  "PUTE\n"
  "    Put data into the EPG list. The data entered has to strictly follow the\n"
  "    format defined in vdr(5) for the 'epg.data' file.  A '.' on a line\n"
  "    by itself terminates the input and starts processing of the data (all\n"
  "    entered data is buffered until the terminating '.' is seen).",
  "SCAN\n"
  "    Forces an EPG scan. If this is a single DVB device system, the scan\n"
  "    will be done on the primary device unless it is currently recording.",
  "STAT disk\n"
  "    Return information about disk usage (total, free, percent).",
  "UPDT <settings>\n"
  "    Updates a timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. If a timer with the same channel, day, start\n"
  "    and stop time does not yet exists, it will be created.",
  "VOLU [ <number> | + | - | mute ]\n"
  "    Set the audio volume to the given number (which is limited to the range\n"
  "    0...255). If the special options '+' or '-' are given, the volume will\n"
  "    be turned up or down, respectively. The option 'mute' will toggle the\n"
  "    audio muting. If no option is given, the current audio volume level will\n"
  "    be returned.",
  "QSTD\n"
  "    Send vdr into Quick Standby.",
  "QUIT\n"
  "    Exit vdr (SVDRP).\n"
  "    You can also hit Ctrl-D to exit.",
  NULL
  };

/* SVDRP Reply Codes:

 214 Help message
 215 EPG or recording data record
 216 Image grab data (base 64)
 220 VDR service ready
 221 VDR service closing transmission channel
 250 Requested VDR action okay, completed
 354 Start sending EPG data
 451 Requested action aborted: local error in processing
 500 Syntax error, command unrecognized
 501 Syntax error in parameters or arguments
 502 Command not implemented
 504 Command parameter not implemented
 550 Requested action not taken
 554 Transaction failed
 900 Default plugin reply code
 901..999 Plugin specific reply codes

*/

const char *GetHelpTopic(const char *HelpPage)
{
  static char topic[MAXHELPTOPIC];
  const char *q = HelpPage;
  while (*q) {
        if (isspace(*q)) {
           uint n = q - HelpPage;
           if (n >= sizeof(topic))
              n = sizeof(topic) - 1;
           strncpy(topic, HelpPage, n);
           topic[n] = 0;
           return topic;
           }
        q++;
        }
  return NULL;
}

const char *GetHelpPage(const char *Cmd, const char **p)
{
  if (p) {
     while (*p) {
           const char *t = GetHelpTopic(*p);
           if (strcasecmp(Cmd, t) == 0)
              return *p;
           p++;
           }
     }
  return NULL;
}

char *cSVDRP::grabImageDir = NULL;

cSVDRP::cSVDRP(int Port)
#ifndef SVDRP_THREAD
:socket(Port)
#endif
{
  inExecute=false;
  PUTEhandler = NULL;
  numChars = 0;
  length = BUFSIZ;
  cmdLine = MALLOC(char, length);
#ifdef SVDRP_THREAD
  /* clear the master and temp sets */
  fdmax = 0;
  port = Port;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  int i;
  for(i=0; i<MAXCONS; i++) {
    lastActivity[i] = 0;
    newConnection[i] = false;
  }
  SetPriority(15);
#else
  lastActivity = 0;
#endif
  isyslog("SVDRP listening on port %d", Port);
}

cSVDRP::~cSVDRP()
{
#ifndef SVDRP_THREAD
  Close(true);
#endif
  free(cmdLine);
}

void cSVDRP::Close(int Sock, bool SendReply, bool Timeout)
{
#ifndef SVDRP_THREAD
  if (file.IsOpen()) {
#endif
     if (Interrupted) {
        //TODO how can we get the *full* hostname?
        char buffer[BUFSIZ];
        gethostname(buffer, sizeof(buffer));
        Reply(Sock, 221, "%s closing connection%s", buffer, shutdownMode==standby ? " (standby)" : shutdownMode==deepstandby ? " (deepstandby)" : " (terminated)");
     } else if (SendReply) {
        //TODO how can we get the *full* hostname?
        char buffer[BUFSIZ];
        gethostname(buffer, sizeof(buffer));
        Reply(Sock, 221, "%s closing connection%s", buffer, Timeout ? " (timeout)" : "");
     }
     isyslog("closing SVDRP connection"); //TODO store IP#???
#ifndef SVDRP_THREAD
     file.Close();
#else
     close(Sock);
     FD_CLR (Sock, &master);
#endif
     DELETENULL(PUTEhandler);
#ifndef SVDRP_THREAD
     }
#endif
}

bool cSVDRP::Send(int Sock, const char *s, int length)
{
  if (length < 0)
     length = strlen(s);
#ifndef SVDRP_THREAD
  if (safe_write(file, s, length) < 0) {
#else
  if (send(Sock, s, length, 0) < 0) {
#endif
     LOG_ERROR;
#ifndef SVDRP_THREAD
     Close(Sock);
#endif
     return false;
     }
  return true;
}

void cSVDRP::Reply(int Sock, int Code, const char *fmt, ...)
{
#ifndef SVDRP_THREAD
  if (file.IsOpen()) {
#endif
     if (Code != 0) {
        va_list ap;
        va_start(ap, fmt);
        char *buffer;
        vasprintf(&buffer, fmt, ap);
        const char *s = buffer;
        while (s && *s) {
              const char *n = strchr(s, '\n');
              char cont = ' ';
              if (Code < 0 || (n && *(n + 1))) // trailing newlines don't count!
                 cont = '-';
              char number[16];
              sprintf(number, "%03d%c", abs(Code), cont);
              if (!(Send(Sock, number) && Send(Sock, s, n ? n - s : -1) && Send(Sock, "\r\n")))
                 break;
              s = n ? n + 1 : NULL;
              }
        free(buffer);
        va_end(ap);
        }
     else {
        Reply(Sock, 451, "Zero return code - looks like a programming error!");
        esyslog("SVDRP: zero return code!");
        }
#ifndef SVDRP_THREAD
     }
#endif
}

void cSVDRP::PrintHelpTopics(int Sock, const char **hp)
{
  int NumPages = 0;
  if (hp) {
     while (*hp) {
           NumPages++;
           hp++;
           }
     hp -= NumPages;
     }
  const int TopicsPerLine = 5;
  int x = 0;
  for (int y = 0; (y * TopicsPerLine + x) < NumPages; y++) {
      char buffer[TopicsPerLine * MAXHELPTOPIC + 5];
      char *q = buffer;
      q += sprintf(q, "    ");
      for (x = 0; x < TopicsPerLine && (y * TopicsPerLine + x) < NumPages; x++) {
          const char *topic = GetHelpTopic(hp[(y * TopicsPerLine + x)]);
          if (topic)
             q += sprintf(q, "%*s", -MAXHELPTOPIC, topic);
          }
      x = 0;
      Reply(Sock, -214, "%s", buffer);
      }
}

void cSVDRP::CmdCHAN(int Sock, const char *Option)
{
  if (*Option) {
     int n = -1;
     int d = 0;
     if (isnumber(Option)) {
        int o = strtol(Option, NULL, 10);
        if (o >= 1 && o <= Channels.MaxNumber())
           n = o;
        }
     else if (strcmp(Option, "-") == 0) {
        n = cDevice::CurrentChannel();
        if (n > 1) {
           n--;
           d = -1;
           }
        }
     else if (strcmp(Option, "+") == 0) {
        n = cDevice::CurrentChannel();
        if (n < Channels.MaxNumber()) {
           n++;
           d = 1;
           }
        }
     else {
        cChannel *channel = Channels.GetByChannelID(tChannelID::FromString(Option));
        if (channel)
           n = channel->Number();
        else {

            for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
                if (!channel->GroupSep()) {
                    if (strcasecmp(channel->Name(), Option) == 0) {
                        n = channel->Number();
                        break;
                    }
                }
            }
        }
        }
     if (n < 0) {
        Reply(Sock, 501, "Undefined channel \"%s\"", Option);
        return;
        }
     if (!d) {
        cChannel *channel = Channels.GetByNumber(n);
        if (channel) {
           if (!cDevice::PrimaryDevice()->SwitchChannel(channel, true)) {
              Reply(Sock, 554, "Error switching to channel \"%d\"", channel->Number());
              return;
              }
           }
        else {
           Reply(Sock, 550, "Unable to find channel \"%s\"", Option);
           return;
           }
        }
     else
        cDevice::SwitchChannel(d);
     }
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (channel)
     Reply(Sock, 250, "%d %s", channel->Number(), channel->Name());
  else
     Reply(Sock, 550, "Unable to find channel \"%d\"", cDevice::CurrentChannel());
}

void cSVDRP::CmdCLRE(int Sock, const char *Option)
{
  cSchedules::ClearAll();
  Reply(Sock, 250, "EPG data cleared");
}

void cSVDRP::CmdDELC(int Sock, const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
#ifdef SVDRP_THREAD
        if (!Channels.BeingEdited() && Channels.Lock(true, 100)) {
#else
        if (!Channels.BeingEdited()) {
#endif
           cChannel *channel = Channels.GetByNumber(strtol(Option, NULL, 10));
           if (channel) {
              for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
                  if (timer->Channel() == channel) {
                     Reply(Sock, 550, "Channel \"%s\" is in use by timer %d", Option, timer->Index() + 1);
                     return;
                     }
                  }
              int CurrentChannelNr = cDevice::CurrentChannel();
              cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
              if (CurrentChannel && channel == CurrentChannel) {
                 int n = Channels.GetNextNormal(CurrentChannel->Index());
                 if (n < 0)
                    n = Channels.GetPrevNormal(CurrentChannel->Index());
                 CurrentChannel = Channels.Get(n);
                 CurrentChannelNr = 0; // triggers channel switch below
                 }
              Channels.Del(channel);
              Channels.ReNumber();
              Channels.SetModified(true);
              isyslog("channel %s deleted", Option);
              if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
                 if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
                    Channels.SwitchTo(CurrentChannel->Number());
                 else
                    cDevice::SetCurrentChannel(CurrentChannel);
                 }
              Reply(Sock, 250, "Channel \"%s\" deleted", Option);
              }
           else
              Reply(Sock, 501, "Channel \"%s\" not defined", Option);
           }
        else
           Reply(Sock, 550, "Channels are being edited - try again later");
        }
     else
        Reply(Sock, 501, "Error in channel number \"%s\"", Option);
     }
  else
     Reply(Sock, 501, "Missing channel number");
#ifdef SVDRP_THREAD
  Channels.Unlock();
#endif
}

void cSVDRP::CmdDELR(int Sock, const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        cRecording *recording = Recordings.Get(strtol(Option, NULL, 10) - 1);
        if (recording) {
           cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
           if (!rc) {
              if (recording->Delete()) {
                 Reply(Sock, 250, "Recording \"%s\" deleted", Option);
                 ::Recordings.DelByName(recording->FileName());
                 }
              else
                 Reply(Sock, 554, "Error while deleting recording!");
              }
           else
              Reply(Sock, 550, "Recording \"%s\" is in use by timer %d", Option, rc->Timer()->Index() + 1);
           }
        else
           Reply(Sock, 550, "Recording \"%s\" not found%s", Option, Recordings.Count() ? "" : " (use LSTR before deleting)");
        }
     else
        Reply(Sock, 501, "Error in recording number \"%s\"", Option);
     }
  else
     Reply(Sock, 501, "Missing recording number");
}

void cSVDRP::CmdDELT(int Sock, const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        if (!Timers.BeingEdited()) {
           cTimer *timer = Timers.Get(strtol(Option, NULL, 10) - 1);
           if (timer) {
              if (!timer->Recording()) {
                 isyslog("deleting timer %s", *timer->ToDescr());
                 Timers.Del(timer);
                 Timers.SetModified();
                 Reply(Sock, 250, "Timer \"%s\" deleted", Option);
                 }
              else
                 Reply(Sock, 550, "Timer \"%s\" is recording", Option);
              }
           else
              Reply(Sock, 501, "Timer \"%s\" not defined", Option);
           }
        else
           Reply(Sock, 550, "Timers are being edited - try again later");
        }
     else
        Reply(Sock, 501, "Error in timer number \"%s\"", Option);
     }
  else
     Reply(Sock, 501, "Missing timer number");
}

void cSVDRP::CmdEDIT(int Sock, const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        cRecording *recording = Recordings.Get(strtol(Option, NULL, 10) - 1);
        if (recording) {
           cMarks Marks;
           if (Marks.Load(recording->FileName()) && Marks.Count()) {
              if (!cCutter::Active()) {
                 if (cCutter::Start(recording->FileName()))
                    Reply(Sock, 250, "Editing recording \"%s\" [%s]", Option, recording->Title());
                 else
                    Reply(Sock, 554, "Can't start editing process");
                 }
              else
                 Reply(Sock, 554, "Editing process already active");
              }
           else
              Reply(Sock, 554, "No editing marks defined");
           }
        else
           Reply(Sock, 550, "Recording \"%s\" not found%s", Option, Recordings.Count() ? "" : " (use LSTR before editing)");
        }
     else
        Reply(Sock, 501, "Error in recording number \"%s\"", Option);
     }
  else
     Reply(Sock, 501, "Missing recording number");
}

void cSVDRP::CmdGRAB(int Sock, const char *Option)
{
  char *FileName = NULL;
  bool Jpeg = true;
  int Quality = -1, SizeX = -1, SizeY = -1;
  if (*Option) {
     char buf[strlen(Option) + 1];
     char *p = strcpy(buf, Option);
     const char *delim = " \t";
     char *strtok_next;
     FileName = strtok_r(p, delim, &strtok_next);
     // image type:
     char *Extension = strrchr(FileName, '.');
     if (Extension) {
        if (strcasecmp(Extension, ".jpg") == 0 || strcasecmp(Extension, ".jpeg") == 0)
           Jpeg = true;
        else if (strcasecmp(Extension, ".pnm") == 0)
           Jpeg = false;
        else {
           Reply(Sock, 501, "Unknown image type \"%s\"", Extension + 1);
           return;
           }
        if (Extension == FileName)
           FileName = NULL;
        }
     else if (strcmp(FileName, "-") == 0)
        FileName = NULL;
     // image quality (and obsolete type):
     if ((p = strtok_r(NULL, delim, &strtok_next)) != NULL) {
        if (strcasecmp(p, "JPEG") == 0 || strcasecmp(p, "PNM") == 0) {
           // tolerate for backward compatibility
           p = strtok_r(NULL, delim, &strtok_next);
           }
        if (p) {
           if (isnumber(p))
              Quality = atoi(p);
           else {
              Reply(Sock, 501, "Invalid quality \"%s\"", p);
              return;
              }
           }
        }
     // image size:
     if ((p = strtok_r(NULL, delim, &strtok_next)) != NULL) {
        if (isnumber(p))
           SizeX = atoi(p);
        else {
           Reply(Sock, 501, "Invalid sizex \"%s\"", p);
           return;
           }
        if ((p = strtok_r(NULL, delim, &strtok_next)) != NULL) {
           if (isnumber(p))
              SizeY = atoi(p);
           else {
              Reply(Sock, 501, "Invalid sizey \"%s\"", p);
              return;
              }
           }
        else {
           Reply(Sock, 501, "Missing sizey");
           return;
           }
        }
     if ((p = strtok_r(NULL, delim, &strtok_next)) != NULL) {
        Reply(Sock, 501, "Unexpected parameter \"%s\"", p);
        return;
        }
     // canonicalize the file name:
     char RealFileName[PATH_MAX];
     if (FileName) {
        if (grabImageDir) {
           char *s = 0;
           char *slash = strrchr(FileName, '/');
           if (!slash) {
              asprintf(&s, "%s/%s", grabImageDir, FileName);
              FileName = s;
              }
           slash = strrchr(FileName, '/'); // there definitely is one
           *slash = 0;
           char *r = realpath(FileName, RealFileName);
           *slash = '/';
           if (!r) {
              LOG_ERROR_STR(FileName);
              Reply(Sock, 501, "Invalid file name \"%s\"", FileName);
              free(s);
              return;
              }
           strcat(RealFileName, slash);
           FileName = RealFileName;
           free(s);
           if (strncmp(FileName, grabImageDir, strlen(grabImageDir)) != 0) {
              Reply(Sock, 501, "Invalid file name \"%s\"", FileName);
              return;
              }
           }
        else {
           Reply(Sock, 550, "Grabbing to file not allowed (use \"GRAB -\" instead)");
           return;
           }
        }
     // actual grabbing:
     int ImageSize;
     uchar *Image = cDevice::PrimaryDevice()->GrabImage(ImageSize, Jpeg, Quality, SizeX, SizeY);
     if (Image) {
        if (FileName) {
           int fd = open(FileName, O_WRONLY | O_CREAT | O_NOFOLLOW | O_TRUNC, DEFFILEMODE);
           if (fd >= 0) {
              if (safe_write(fd, Image, ImageSize) == ImageSize) {
                 dsyslog("grabbed image to %s", FileName);
                 Reply(Sock, 250, "Grabbed image %s", Option);
                 }
              else {
                 LOG_ERROR_STR(FileName);
                 Reply(Sock, 451, "Can't write to '%s'", FileName);
                 }
              close(fd);
              }
           else {
              LOG_ERROR_STR(FileName);
              Reply(Sock, 451, "Can't open '%s'", FileName);
              }
           }
        else {
           cBase64Encoder Base64(Image, ImageSize);
           const char *s;
           while ((s = Base64.NextLine()) != NULL)
                 Reply(Sock, -216, "%s", s);
           Reply(Sock, 216, "Grabbed image %s", Option);
           }
        free(Image);
        }
     else
        Reply(Sock, 451, "Grab image failed");
     }
  else
     Reply(Sock, 501, "Missing filename");
}

void cSVDRP::CmdHELP(int Sock, const char *Option)
{
  if (*Option) {
     const char *hp = GetHelpPage(Option, HelpPages);
     if (hp)
        Reply(Sock, 214, "%s", hp);
     else {
        Reply(Sock, 504, "HELP topic \"%s\" unknown", Option);
        return;
        }
     }
  else {
     Reply(Sock, -214, "This is "VDRNAME" version %s", VDRVERSION);
     Reply(Sock, -214, "Topics:");
     PrintHelpTopics(Sock, HelpPages);
     cPlugin *plugin;
     for (unsigned int i = 0; i < cPluginManager::PluginCount(); i++)
	     if ((plugin = cPluginManager::GetPlugin(i)) != NULL)
	     {
		     const char **hp = plugin->SVDRPHelpPages();
		     if (hp)
			     Reply(Sock, -214, "Plugin %s v%s - %s", plugin->Name(), plugin->Version(), plugin->Description());
		     PrintHelpTopics(Sock, hp);
	     }
     Reply(Sock, -214, "To report bugs in the implementation send email to");
     Reply(Sock, -214, "    vdr-bugs@cadsoft.de");
     }
  Reply(Sock, 214, "End of HELP info");
}

void cSVDRP::CmdHITK(int Sock, const char *Option)
{
  if (*Option) {
     eKeys k = cKey::FromString(Option);
     if (k != kNone) {
        cRemote::Put(k);
        Reply(Sock, 250, "Key \"%s\" accepted", Option);
        }
     else
        Reply(Sock, 504, "Unknown key: \"%s\"", Option);
     }
  else {
     Reply(Sock, -214, "Valid <key> names for the HITK command:");
     for (int i = 0; i < kNone; i++) {
         Reply(Sock, -214, "    %s", cKey::ToString(eKeys(i)));
         }
     Reply(Sock, 214, "End of key list");
     }
}

void cSVDRP::CmdLSTC(int Sock, const char *Option)
{
  bool WithGroupSeps = strcasecmp(Option, ":groups") == 0;

#ifdef SVDRP_THREAD
  if (*Option && !WithGroupSeps) {
#else
  if (*Option && !WithGroupSeps && Channels.Lock(false, 100)) {
#endif
     if (isnumber(Option)) {
        cChannel *channel = Channels.GetByNumber(strtol(Option, NULL, 10));
        if (channel)
           Reply(Sock, 250, "%d %s", channel->Number(), *channel->ToText());
        else
           Reply(Sock, 501, "Channel \"%s\" not defined", Option);
        }
     else {

        cChannel *next = NULL;

        for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
            if (!channel->GroupSep()) {
                if (strcasestr(channel->Name(), Option)) {
                    if (next)
                        Reply(Sock, -250, "%d %s", next->Number(), *next->ToText());
                    next = channel;
                }
            }
        }

        if (next)
           Reply(Sock, 250, "%d %s", next->Number(), *next->ToText());
        else
           Reply(Sock, 501, "Channel \"%s\" not defined", Option);
     }
  }
  else if (Channels.MaxNumber() >= 1) {
        for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
                if (WithGroupSeps)
                   Reply(Sock, channel->Next() ? -250: 250, "%d %s", channel->GroupSep() ? 0 : channel->Number(), *channel->ToText());
                else if (!channel->GroupSep())
                   Reply(Sock, channel->Number() < Channels.MaxNumber() ? -250 : 250, "%d %s", channel->Number(), *channel->ToText());
        }
  }
  else
     Reply(Sock, 550, "No channels defined");
#ifdef SVDRP_THREAD
  Channels.Unlock();
#endif
}

void cSVDRP::CmdLSTE(int Sock, const char *Option)
{
  cSchedulesLock SchedulesLock;
  const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
  if (Schedules) {
     const cSchedule* Schedule = NULL;
     eDumpMode DumpMode = dmAll;
     time_t AtTime = 0;
     if (*Option) {
        char buf[strlen(Option) + 1];
        strcpy(buf, Option);
        const char *delim = " \t";
        char *strtok_next;
        char *p = strtok_r(buf, delim, &strtok_next);
        while (p && DumpMode == dmAll) {
              if (strcasecmp(p, "NOW") == 0)
                 DumpMode = dmPresent;
              else if (strcasecmp(p, "NEXT") == 0)
                 DumpMode = dmFollowing;
              else if (strcasecmp(p, "AT") == 0) {
                 DumpMode = dmAtTime;
                 if ((p = strtok_r(NULL, delim, &strtok_next)) != NULL) {
                    if (isnumber(p))
                       AtTime = strtol(p, NULL, 10);
                    else {
                       Reply(Sock, 501, "Invalid time");
                       return;
                       }
                    }
                 else {
                    Reply(Sock, 501, "Missing time");
                    return;
                    }
                 }
              else if (!Schedule) {
                 cChannel* Channel = NULL;
                 if (isnumber(p))
                    Channel = Channels.GetByNumber(strtol(Option, NULL, 10));
                 else
                    Channel = Channels.GetByChannelID(tChannelID::FromString(Option));
                 if (Channel) {
                    Schedule = Schedules->GetSchedule(Channel);
                    if (!Schedule) {
                       Reply(Sock, 550, "No schedule found");
                       return;
                       }
                    }
                 else {
                    Reply(Sock, 550, "Channel \"%s\" not defined", p);
                    return;
                    }
                 }
              else {
                 Reply(Sock, 501, "Unknown option: \"%s\"", p);
                 return;
                 }
              p = strtok_r(NULL, delim, &strtok_next);
              }
        }
#ifdef SVDRP_THREAD
     int fd = dup(Sock);
#else
     int fd = dup(file);
#endif
     if (fd) {
        FILE *f = fdopen(fd, "w");
        if (f) {
           if (Schedule)
              Schedule->Dump(f, "215-", DumpMode, AtTime);
           else
              Schedules->Dump(f, "215-", DumpMode, AtTime);
           fflush(f);
           Reply(Sock, 215, "End of EPG data");
           fclose(f);
           }
        else {
           Reply(Sock, 451, "Can't open file connection");
           close(fd);
           }
        }
     else
        Reply(Sock, 451, "Can't dup stream descriptor");
     }
  else
     Reply(Sock, 451, "Can't get EPG data");
}

void cSVDRP::CmdLSTR(int Sock, const char *Option)
{
#ifdef SVDRP_THREAD
  int recordings = Recordings.Count();
#else
  bool recordings = Recordings.Update(true);
#endif

  if (*Option) {
     if (isnumber(Option)) {
        cRecording *recording = Recordings.Get(strtol(Option, NULL, 10) - 1);
        if (recording) {
#ifdef SVDRP_THREAD
           FILE *f = fdopen(Sock, "w");
#else
           FILE *f = fdopen(file, "w");
#endif
           if (f) {
              recording->Info()->Write(f, "215-");
              fflush(f);
              Reply(Sock, 215, "End of recording information");
              // don't 'fclose(f)' here!
              }
           else
              Reply(Sock, 451, "Can't open file connection");
           }
        else
           Reply(Sock, 550, "Recording \"%s\" not found", Option);
        }
     else
        Reply(Sock, 501, "Error in recording number \"%s\"", Option);
     }
  else if (recordings > 0) {
     cRecording *recording = Recordings.First();
     if(!recording) {
       Reply(Sock, 550, "No recordings available");
       return;
     }
     while (recording) {
           Reply(Sock, recording == Recordings.Last() ? 250 : -250, "%d %s", recording->Index() + 1, recording->Title(' ', true));
           recording = Recordings.Next(recording);
           cCondWait::SleepMs(3);
           }
     }
}

void cSVDRP::CmdLSTT(int Sock, const char *Option)
{
  int Number = 0;
  bool Id = false;
  if (*Option) {
     char buf[strlen(Option) + 1];
     strcpy(buf, Option);
     const char *delim = " \t";
     char *strtok_next;
     char *p = strtok_r(buf, delim, &strtok_next);
     while (p) {
           if (isnumber(p))
              Number = strtol(p, NULL, 10);
           else if (strcasecmp(p, "ID") == 0)
              Id = true;
           else {
              Reply(Sock, 501, "Unknown option: \"%s\"", p);
              return;
              }
           p = strtok_r(NULL, delim, &strtok_next);
           }
     }
  if (Number) {
     cTimer *timer = Timers.Get(Number - 1);
     if (timer)
        Reply(Sock, 250, "%d %s", timer->Index() + 1, *timer->ToText(Id));
     else
        Reply(Sock, 501, "Timer \"%s\" not defined", Option);
     }
  else if (Timers.Count()) {
     for (int i = 0; i < Timers.Count(); i++) {
         cTimer *timer = Timers.Get(i);
        if (timer)
           Reply(Sock, i < Timers.Count() - 1 ? -250 : 250, "%d %s", timer->Index() + 1, *timer->ToText(Id));
        else
           Reply(Sock, 501, "Timer \"%d\" not found", i + 1);
         }
     }
  else
     Reply(Sock, 550, "No timers defined");
}

void cSVDRP::CmdMESG(int Sock, const char *Option)
{
#ifdef SVDRP_THREAD
  Lock();
#endif
  if (*Option) {
     isyslog("SVDRP message: '%s'", Option);
     Skins.QueueMessage(mtInfo, Option);
     Reply(Sock, 250, "Message sent");
     }
  else
     Reply(Sock, 501, "Missing message");
#ifdef SVDRP_THREAD
  Unlock();
#endif
}

void cSVDRP::CmdMODC(int Sock, const char *Option)
{
#ifdef SVDRP_THREAD
  if (*Option && Channels.Lock(true, 100)) {
#else
  if (*Option) {
#endif
     char *tail;
     int n = strtol(Option, &tail, 10);
     if (tail && tail != Option) {
        tail = skipspace(tail);
        if (!Channels.BeingEdited()) {
           cChannel *channel = Channels.GetByNumber(n);
           if (channel) {
              cChannel ch;
              if (ch.Parse(tail)) {
                 if (Channels.HasUniqueChannelID(&ch, channel)) {
                    *channel = ch;
                    Channels.ReNumber();
                    Channels.SetModified(true);
                    isyslog("modifed channel %d %s", channel->Number(), *channel->ToText());
                    Reply(Sock, 250, "%d %s", channel->Number(), *channel->ToText());
                    }
                 else
                    Reply(Sock, 501, "Channel settings are not unique");
                 }
              else
                 Reply(Sock, 501, "Error in channel settings");
              }
           else
              Reply(Sock, 501, "Channel \"%d\" not defined", n);
           }
        else
           Reply(Sock, 550, "Channels are being edited - try again later");
        }
     else
        Reply(Sock, 501, "Error in channel number");
     }
  else
     Reply(Sock, 501, "Missing channel settings");
#ifdef SVDRP_THREAD
  Channels.Unlock();
#endif
}

void cSVDRP::CmdMODT(int Sock, const char *Option)
{
  if (*Option) {
     char *tail;
     int n = strtol(Option, &tail, 10);
     if (tail && tail != Option) {
        tail = skipspace(tail);
        if (!Timers.BeingEdited()) {
           cTimer *timer = Timers.Get(n - 1);
           if (timer) {
              cTimer t = *timer;
              if (strcasecmp(tail, "ON") == 0)
                 t.SetFlags(tfActive);
              else if (strcasecmp(tail, "OFF") == 0)
                 t.ClrFlags(tfActive);
              else if (!t.Parse(tail)) {
                 Reply(Sock, 501, "Error in timer settings");
                 return;
                 }
              *timer = t;
              Timers.SetModified();
              isyslog("timer %s modified (%s)", *timer->ToDescr(), timer->HasFlags(tfActive) ? "active" : "inactive");
              Reply(Sock, 250, "%d %s", timer->Index() + 1, *timer->ToText());
              }
           else
              Reply(Sock, 501, "Timer \"%d\" not defined", n);
           }
        else
           Reply(Sock, 550, "Timers are being edited - try again later");
        }
     else
        Reply(Sock, 501, "Error in timer number");
     }
  else
     Reply(Sock, 501, "Missing timer settings");
}

void cSVDRP::CmdMOVC(int Sock, const char *Option)
{
#ifdef SVDRP_THREAD
  if (*Option && Channels.Lock(true, 100)) {
#else
  if (*Option) {
#endif
     if (!Channels.BeingEdited() && !Timers.BeingEdited()) {
        char *tail;
        int From = strtol(Option, &tail, 10);
        if (tail && tail != Option) {
           tail = skipspace(tail);
           if (tail && tail != Option) {
              int To = strtol(tail, NULL, 10);
              int CurrentChannelNr = cDevice::CurrentChannel();
              cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
              cChannel *FromChannel = Channels.GetByNumber(From);
              if (FromChannel) {
                 cChannel *ToChannel = Channels.GetByNumber(To);
                 if (ToChannel) {
                    int FromNumber = FromChannel->Number();
                    int ToNumber = ToChannel->Number();
                    if (FromNumber != ToNumber) {
                       Channels.Move(FromChannel, ToChannel);
                       Channels.ReNumber();
                       Channels.SetModified(true);
                       if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
                          if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
                             Channels.SwitchTo(CurrentChannel->Number());
                          else
                             cDevice::SetCurrentChannel(CurrentChannel);
                          }
                       isyslog("channel %d moved to %d", FromNumber, ToNumber);
                       Reply(Sock, 250,"Channel \"%d\" moved to \"%d\"", From, To);
                       }
                    else
                       Reply(Sock, 501, "Can't move channel to same postion");
                    }
                 else
                    Reply(Sock, 501, "Channel \"%d\" not defined", To);
                 }
              else
                 Reply(Sock, 501, "Channel \"%d\" not defined", From);
              }
           else
              Reply(Sock, 501, "Error in channel number");
           }
        else
           Reply(Sock, 501, "Error in channel number");
        }
     else
        Reply(Sock, 550, "Channels or timers are being edited - try again later");
     }
  else
     Reply(Sock, 501, "Missing channel number");
#ifdef SVDRP_THREAD
  Channels.Unlock();
#endif
}

void cSVDRP::CmdMOVT(int Sock, const char *Option)
{
  //TODO combine this with menu action
  Reply(Sock, 502, "MOVT not yet implemented");
}

void cSVDRP::CmdNEWC(int Sock, const char *Option)
{
#ifdef SVDRP_THREAD
  if (*Option && Channels.Lock(true, 100)) {
#else
  if (*Option) {
#endif
     cChannel ch;
     if (ch.Parse(Option)) {
        if (Channels.HasUniqueChannelID(&ch)) {
           cChannel *channel = new cChannel;
           *channel = ch;
           Channels.Add(channel);
           Channels.ReNumber();
           Channels.SetModified(true);
           isyslog("new channel %d %s", channel->Number(), *channel->ToText());
           Reply(Sock, 250, "%d %s", channel->Number(), *channel->ToText());
           }
        else
           Reply(Sock, 501, "Channel settings are not unique");
        }
     else
        Reply(Sock, 501, "Error in channel settings");
     }
  else
     Reply(Sock, 501, "Missing channel settings");
#ifdef SVDRP_THREAD
  Channels.Unlock();
#endif
}

void cSVDRP::CmdNEWT(int Sock, const char *Option)
{
  if (*Option) {
     cTimer *timer = new cTimer;
     if (timer->Parse(Option)) {
        cTimer *t = Timers.GetTimer(timer);
        if (!t) {
           Timers.Add(timer);
           Timers.SetModified();
           isyslog("timer %s added", *timer->ToDescr());
           Reply(Sock, 250, "%d %s", timer->Index() + 1, *timer->ToText());
           return;
           }
        else
           Reply(Sock, 550, "Timer already defined: %d %s", t->Index() + 1, *t->ToText());
        }
     else
        Reply(Sock, 501, "Error in timer settings");
     delete timer;
     }
  else
     Reply(Sock, 501, "Missing timer settings");
}

void cSVDRP::CmdNEXT(int Sock, const char *Option)
{
  cTimer *t = Timers.GetNextActiveTimer();
  if (t) {
     time_t Start = t->StartTime();
     int Number = t->Index() + 1;
     if (!*Option)
        Reply(Sock, 250, "%d %s", Number, *TimeToString(Start));
     else if (strcasecmp(Option, "ABS") == 0)
        Reply(Sock, 250, "%d %ld", Number, Start);
     else if (strcasecmp(Option, "REL") == 0)
        Reply(Sock, 250, "%d %ld", Number, Start - time(NULL));
     else
        Reply(Sock, 501, "Unknown option: \"%s\"", Option);
     }
  else
     Reply(Sock, 550, "No active timers");
}

void cSVDRP::CmdPLAY(int Sock, const char *Option)
{
  if (*Option) {
     char *opt = strdup(Option);
     char *num = skipspace(opt);
     char *option = num;
     while (*option && !isspace(*option))
           option++;
     char c = *option;
     *option = 0;
     if (isnumber(num)) {
        cRecording *recording = Recordings.Get(strtol(num, NULL, 10) - 1);
        if (recording) {
           if (c)
              option = skipspace(++option);
           cReplayControl::SetRecording(NULL, NULL);
           cControl::Shutdown();
           if (*option) {
              int pos = 0;
              if (strcasecmp(option, "BEGIN") != 0) {
                 int h, m = 0, s = 0, f = 1;
                 int x = sscanf(option, "%d:%d:%d.%d", &h, &m, &s, &f);
                 if (x == 1)
                    pos = h;
                 else if (x >= 3)
                    pos = (h * 3600 + m * 60 + s) * FRAMESPERSEC + f - 1;
                 }
              cResumeFile resume(recording->FileName());
              if (pos <= 0)
                 resume.Delete();
              else
                 resume.Save(pos);
              }
           cReplayControl::SetRecording(recording->FileName(), recording->Title());
           cControl::Launch(new cReplayControl);
           cControl::Attach();
           Reply(Sock, 250, "Playing recording \"%s\" [%s]", num, recording->Title());
           }
        else
           Reply(Sock, 550, "Recording \"%s\" not found%s", num, Recordings.Count() ? "" : " (use LSTR before playing)");
        }
     else
        Reply(Sock, 501, "Error in recording number \"%s\"", num);
     free(opt);
     }
  else
     Reply(Sock, 501, "Missing recording number");
}

void cSVDRP::CmdPLUG(int Sock, const char *Option)
{
  if (*Option) {
     char *opt = strdup(Option);
     char *name = skipspace(opt);
     char *option = name;
     while (*option && !isspace(*option))
        option++;
     char c = *option;
     *option = 0;
     cPlugin *plugin = cPluginManager::GetPlugin(name);
     if (plugin) {
        if (c)
           option = skipspace(++option);
        char *cmd = option;
        while (*option && !isspace(*option))
              option++;
        if (*option) {
           *option++ = 0;
           option = skipspace(option);
           }
        if (!*cmd || strcasecmp(cmd, "HELP") == 0) {
           if (*cmd && *option) {
              const char *hp = GetHelpPage(option, plugin->SVDRPHelpPages());
              if (hp) {
                 Reply(Sock, -214, "%s", hp);
                 Reply(Sock, 214, "End of HELP info");
                 }
              else
                 Reply(Sock, 504, "HELP topic \"%s\" for plugin \"%s\" unknown", option, plugin->Name());
              }
           else {
              Reply(Sock, -214, "Plugin %s v%s - %s", plugin->Name(), plugin->Version(), plugin->Description());
              const char **hp = plugin->SVDRPHelpPages();
              if (hp) {
                 Reply(Sock, -214, "SVDRP commands:");
                 PrintHelpTopics(Sock, hp);
                 Reply(Sock, 214, "End of HELP info");
                 }
              else
                 Reply(Sock, 214, "This plugin has no SVDRP commands");
              }
           }
        else if (strcasecmp(cmd, "MAIN") == 0) {
           if (cRemote::CallPlugin(plugin->Name()))
              Reply(Sock, 250, "Initiated call to main menu function of plugin \"%s\"", plugin->Name());
           else
              Reply(Sock, 550, "A plugin call is already pending - please try again later");
           }
        else {
           int ReplyCode = 900;
           cString s = plugin->SVDRPCommand(cmd, option, ReplyCode);
           if (s)
              Reply(Sock, abs(ReplyCode), "%s", *s);
           else
              Reply(Sock, 500, "Command unrecognized: \"%s\"", cmd);
           }
        }
     else
        Reply(Sock, 550, "Plugin \"%s\" not found (use PLUG for a list of plugins)", name);
     free(opt);
     }
  else {
     Reply(Sock, -214, "Available plugins:");
     cPlugin *plugin;
     for (unsigned int i = 0; i < cPluginManager::PluginCount(); ++i)
	     if ((plugin = cPluginManager::GetPlugin(i)) != NULL)
		     Reply(Sock, -214, "%s v%s - %s", plugin->Name(), plugin->Version(), plugin->Description());

     Reply(Sock, 214, "End of plugin list");
     }
}

void cSVDRP::CmdPUTE(int Sock, const char *Option)
{
  delete PUTEhandler;
  PUTEhandler = new cPUTEhandler;
  Reply(Sock, PUTEhandler->Status(), "%s", PUTEhandler->Message());
  if (PUTEhandler->Status() != 354)
     DELETENULL(PUTEhandler);
}

void cSVDRP::CmdSCAN(int Sock, const char *Option)
{
  EITScanner.ForceScan();
  Reply(Sock, 250, "EPG scan triggered");
}

void cSVDRP::CmdSTAT(int Sock, const char *Option)
{
  if (*Option) {
     if (strcasecmp(Option, "DISK") == 0) {
        int FreeMB, UsedMB;
        int Percent = VideoDiskSpace(&FreeMB, &UsedMB);
        Reply(Sock, 250, "%dMB %dMB %d%%", FreeMB + UsedMB, FreeMB, Percent);
        }
     else
        Reply(Sock, 501, "Invalid Option \"%s\"", Option);
     }
  else
     Reply(Sock, 501, "No option given");
}

void cSVDRP::CmdUPDT(int Sock, const char *Option)
{
  if (*Option) {
     cTimer *timer = new cTimer;
     if (timer->Parse(Option)) {
        if (!Timers.BeingEdited()) {
           cTimer *t = Timers.GetTimer(timer);
           if (t) {
              t->Parse(Option);
              delete timer;
              timer = t;
              isyslog("timer %s updated", *timer->ToDescr());
              }
           else {
              Timers.Add(timer);
              isyslog("timer %s added", *timer->ToDescr());
              }
           Timers.SetModified();
           Reply(Sock, 250, "%d %s", timer->Index() + 1, *timer->ToText());
           return;
           }
        else
           Reply(Sock, 550, "Timers are being edited - try again later");
        }
     else
        Reply(Sock, 501, "Error in timer settings");
     delete timer;
     }
  else
     Reply(Sock, 501, "Missing timer settings");
}

void cSVDRP::CmdVOLU(int Sock, const char *Option)
{
  if (*Option) {
     if (isnumber(Option))
        cDevice::PrimaryDevice()->SetVolume(strtol(Option, NULL, 10), true);
     else if (strcmp(Option, "+") == 0)
        cDevice::PrimaryDevice()->SetVolume(VOLUMEDELTA);
     else if (strcmp(Option, "-") == 0)
        cDevice::PrimaryDevice()->SetVolume(-VOLUMEDELTA);
     else if (strcasecmp(Option, "MUTE") == 0)
        cDevice::PrimaryDevice()->ToggleMute();
     else {
        Reply(Sock, 501, "Unknown option: \"%s\"", Option);
        return;
        }
     }
  if (cDevice::PrimaryDevice()->IsMute())
     Reply(Sock, 250, "Audio is mute");
  else
     Reply(Sock, 250, "Audio volume is %d", cDevice::CurrentVolume());
}

#define CMD(c) (strcasecmp(Cmd, c) == 0)

void cSVDRP::Execute(char *Cmd)
{
  Execute(Cmd, -1);
}

void cSVDRP::Execute(char *Cmd, int Sock)
{
  // handle PUTE data:
  if (PUTEhandler) {
     if (!PUTEhandler->Process(Cmd)) {
        Reply(Sock, PUTEhandler->Status(), "%s", PUTEhandler->Message());
        DELETENULL(PUTEhandler);
        }
     return;
     }
  // skip leading whitespace:
  Cmd = skipspace(Cmd);
  // find the end of the command word:
  char *s = Cmd;
  while (*s && !isspace(*s))
        s++;
  if (*s)
     *s++ = 0;
  s = skipspace(s);
  if      (CMD("CHAN"))  CmdCHAN(Sock, s);
  else if (CMD("CLRE"))  CmdCLRE(Sock, s);
  else if (CMD("DELC"))  CmdDELC(Sock, s);
  else if (CMD("DELR"))  CmdDELR(Sock, s);
  else if (CMD("DELT"))  CmdDELT(Sock, s);
  else if (CMD("EDIT"))  CmdEDIT(Sock, s);
  else if (CMD("GRAB"))  CmdGRAB(Sock, s);
  else if (CMD("HELP"))  CmdHELP(Sock, s);
  else if (CMD("HITK"))  CmdHITK(Sock, s);
  else if (CMD("LSTC"))  CmdLSTC(Sock, s);
  else if (CMD("LSTE"))  CmdLSTE(Sock, s);
  else if (CMD("LSTR"))  CmdLSTR(Sock, s);
  else if (CMD("LSTT"))  CmdLSTT(Sock, s);
  else if (CMD("MESG"))  CmdMESG(Sock, s);
  else if (CMD("MODC"))  CmdMODC(Sock, s);
  else if (CMD("MODT"))  CmdMODT(Sock, s);
  else if (CMD("MOVC"))  CmdMOVC(Sock, s);
  else if (CMD("MOVT"))  CmdMOVT(Sock, s);
  else if (CMD("NEWC"))  CmdNEWC(Sock, s);
  else if (CMD("NEWT"))  CmdNEWT(Sock, s);
  else if (CMD("NEXT"))  CmdNEXT(Sock, s);
  else if (CMD("PLAY"))  CmdPLAY(Sock, s);
  else if (CMD("PLUG"))  CmdPLUG(Sock, s);
  else if (CMD("PUTE"))  CmdPUTE(Sock, s);
  else if (CMD("SCAN"))  CmdSCAN(Sock, s);
  else if (CMD("STAT"))  CmdSTAT(Sock, s);
  else if (CMD("UPDT"))  CmdUPDT(Sock, s);
  else if (CMD("VOLU"))  CmdVOLU(Sock, s);
  else if (CMD("QSTD"))  { requestQuickStandby = true; Reply(Sock, 250, "Going into Quick Standby"); }
  else if (CMD("QUIT"))  { /*file.Close(); FD_CLR (Sock, &master);*/ Close(Sock, true); }
  else                   Reply(Sock, 500, "Command unrecognized: \"%s\"", Cmd);
}

#ifdef SVDRP_THREAD
bool cSVDRP::OpenSock(int *sock)
{
     int queue = 0;
     // create socket:
     *sock = socket(PF_INET, SOCK_STREAM, 0);
     if (*sock < 0) {
        LOG_ERROR;
        port = 0;
        return false;
        }
     // allow it to always reuse the same port:
     int ReUseAddr = 1;
     setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &ReUseAddr, sizeof(ReUseAddr));
     //
     struct sockaddr_in name;
     name.sin_family = AF_INET;
     name.sin_port = htons(port);
     name.sin_addr.s_addr = htonl(INADDR_ANY);
     if (bind(*sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        LOG_ERROR;
        //Close();
        return false;
        }
     // make it non-blocking:
     int oldflags = fcntl(*sock, F_GETFL, 0);
     if (oldflags < 0) {
        LOG_ERROR;
        return false;
        }
     oldflags |= O_NONBLOCK|FD_CLOEXEC;
     if (fcntl(*sock, F_SETFL, oldflags) < 0) {
        LOG_ERROR;
        return false;
        }
     // listen to the socket:
     if (listen(*sock, queue) < 0) {
        LOG_ERROR;
        return false;
        }
     /* add the listener to the master set */
     FD_SET(*sock, &master);
     /* keep track of the biggest file descriptor */
     fdmax = *sock; /* so far, it's this one*/
  return true;
}

int cSVDRP::AcceptSock(int *sock)
{
     struct sockaddr_in clientname;
     uint size = sizeof(clientname);
     int newsock = accept(*sock, (struct sockaddr *)&clientname, &size);
     if (newsock > 0) {
        bool accepted = SVDRPhosts.Acceptable(clientname.sin_addr.s_addr);
        if (!accepted) {
           const char *s = "Access denied!\n";
           if (write(newsock, s, strlen(s)) < 0)
              LOG_ERROR;
           close(newsock);
           newsock = -1;
           }
        isyslog("connect from %s, port %hu - %s", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port), accepted ? "accepted" : "DENIED");
        PRINTF("connect from %s, port %hu - %s on socket %i\n", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port), accepted ? "accepted" : "DENIED", newsock);
                      if(newsock!= -1) {
		        FD_SET (newsock, &master);	/* add to master set */
		        if (newsock > fdmax) {	/* keep track of the maximum */
			  fdmax = newsock;
		        }
                      }
        }
     else if (errno != EINTR && errno != EAGAIN)
        LOG_ERROR;
     return newsock;
}
#endif

#ifndef SVDRP_THREAD
bool cSVDRP::Process(void)
#else
void cSVDRP::Action(void)
#endif
{
#ifdef SVDRP_THREAD
  bool sleep = true;
  int sock;
  if(!OpenSock(&sock))
	printf("error\n");

  while(Running()) {
#else
  bool NewConnection = !file.IsOpen();
  bool SendGreeting = NewConnection;
#endif
  //printf("%s:%d fdmax: %i new: %i\n", __func__, __LINE__, fdmax, NewConnection);

  int i = 0;
#ifdef SVDRP_THREAD
  read_fds = master;
  static struct timeval tv = { 0, 0};
  if (select (fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
    PRINTF ("Server-select() error !\n");
  }
  sleep = true;
  for (i = 0; i <= fdmax; i++) {
    if (FD_ISSET (i, &read_fds)) {			/* we got one... */
         sleep = false;
         PRINTF("ready: %i listener?: %i\n", i, (i==sock));
         if(i == sock) {
           int newsock = AcceptSock(&sock);
           if(newsock > 0) {
             newConnection[newsock] = true;
             char buffer[BUFSIZ];
             gethostname(buffer, sizeof(buffer));
             time_t now = time(NULL);
             lastActivity[newsock] = now;
             Reply(newsock, 220, "%s SVDRP VideoDiskRecorder %s; %s", buffer, VDRVERSION, *TimeToString(now));
           }
         } else {
#endif

#ifndef SVDRP_THREAD
  if (file.IsOpen() || file.Open(socket.Accept())) {
     if(inExecute) return false; // Wait till execute is processed
     if (SendGreeting) {
        //TODO how can we get the *full* hostname?
        char buffer[BUFSIZ];
        gethostname(buffer, sizeof(buffer));
        time_t now = time(NULL);
        Reply(i, 220, "%s SVDRP VideoDiskRecorder %s; %s", buffer, VDRVERSION, *TimeToString(now));
        }
        if(SendGreeting)
         SendGreeting = false;
     if (NewConnection) {
        lastActivity = time(NULL);
#else
     if (newConnection[i]) {
        lastActivity[i] = time(NULL);
        newConnection[i] = false;
#endif
     }
#ifndef SVDRP_THREAD
     while (file.Ready(false)) {
#endif
           unsigned char c;
#ifndef SVDRP_THREAD
           int r = safe_read(file, &c, 1);
#else
           int r = recv(i, &c, 1, 0);
#endif
           if (r > 0) {
              if (c == '\n' || c == 0x00) {
                 // strip trailing whitespace:
                 while (numChars > 0 && strchr(" \t\r\n", cmdLine[numChars - 1]))
                       cmdLine[--numChars] = 0;
                 // make sure the string is terminated:
                 cmdLine[numChars] = 0;
                 // showtime!
                 inExecute=true;
                 Execute(cmdLine, i);
                 inExecute=false;
                 numChars = 0;
                 if (length > BUFSIZ) {
                    free(cmdLine); // let's not tie up too much memory
                    length = BUFSIZ;
                    cmdLine = MALLOC(char, length);
                    }
                 return true; // Process other cmds on next run (Fixes "HITK setup\r\nHITK 2" problem)
                 }
              else if (c == 0x04 && numChars == 0) {
                 // end of file (only at beginning of line)
                 Close(i, true);
                 }
              else if (c == 0x08 || c == 0x7F) {
                 // backspace or delete (last character)
                 if (numChars > 0)
                    numChars--;
                 }
              else if (c <= 0x03 || c == 0x0D) {
                 // ignore control characters
                 }
              else {
                 if (numChars >= length - 1) {
                    length += BUFSIZ;
                    cmdLine = (char *)realloc(cmdLine, length);
                    }
                 cmdLine[numChars++] = c;
                 cmdLine[numChars] = 0;
                 }
#ifdef SVDRP_THREAD
              lastActivity[i] = time(NULL);
#else
              lastActivity = time(NULL);
#endif
              }
           else if (r <= 0) {
              isyslog("lost connection to SVDRP client");
              Close(i);
#ifdef SVDRP_THREAD
              break;
#endif
              }
           }
#ifndef SVDRP_THREAD
     if (Setup.SVDRPTimeout && time(NULL) - lastActivity > Setup.SVDRPTimeout) {
        isyslog("timeout on SVDRP connection");
        Close(i, true, true);
        }
     return true;
     }
  return false;
#else
     }
  }

  for(int j = 0; j<MAXCONS; j++) {
     time_t now = time(NULL);
     if (lastActivity[j] && Setup.SVDRPTimeout && now - lastActivity[j] > Setup.SVDRPTimeout) {
        isyslog("timeout on SVDRP connection on socket %i", j);
        Close(j, true, true);
        lastActivity[j] = 0;
        }
  }
  if(sleep)
    cCondWait::SleepMs(50);

  }
#endif
}

void cSVDRP::SetGrabImageDir(const char *GrabImageDir)
{
  free(grabImageDir);
  grabImageDir = GrabImageDir ? strdup(GrabImageDir) : NULL;
}

