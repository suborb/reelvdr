/*
 * timers.c: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.c 2.9 2012/04/25 09:02:03 kls Exp $
 */

#include "timers.h"
#include <ctype.h>
#include "channels.h"
#include "device.h"
#include "i18n.h"
#include "libsi/si.h"
#include "recording.h"
#include "remote.h"
#include "status.h"
#ifdef USEMYSQL
#include "vdrmysql.h"
#endif /* USEMYSQL */

#define VFAT_MAX_FILENAME 40 // same as MAX_SUBTITLE_LENGTH in recording.c

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

// --- cTimer ----------------------------------------------------------------

#ifdef USE_LIVEBUFFER
cTimer::cTimer(bool Instant, bool Pause, cChannel *Channel, int Forerun)
#else
cTimer::cTimer(bool Instant, bool Pause, cChannel *Channel)
#endif /*USE_LIVEBUFFER*/
{
#ifdef USEMYSQL
    ID = 0;
#endif
  startTime = stopTime = 0;
  lastSetEvent = 0;
  deferred = 0;
  recording = pending = inVpsMargin = false;
  flags = tfNone;
  if (Instant)
     SetFlags(tfActive | tfInstant);
  channel = Channel ? Channel : Channels.GetByNumber(cDevice::CurrentChannel());
#ifdef USE_LIVEBUFFER
  time_t t = time(NULL) - Forerun;
#else
  time_t t = time(NULL);
#endif /*USE_LIVEBUFFER*/
  struct tm tm_r;
  struct tm *now = localtime_r(&t, &tm_r);
  day = SetTime(t, 0);
  weekdays = 0;
  start = now->tm_hour * 100 + now->tm_min;
  stop = now->tm_hour * 60 + now->tm_min + Setup.InstantRecordTime;
  stop = (stop / 60) * 100 + (stop % 60);
  if (stop >= 2400)
     stop -= 2400;
  priority = Pause ? Setup.PausePriority : Setup.DefaultPriority;
  lifetime = Pause ? Setup.PauseLifetime : Setup.DefaultLifetime;
#ifdef USE_PINPLUGIN
  fskProtection = 0;
#endif /* PINPLUGIN */
  *file = 0;
  aux = NULL;
  event = NULL;
  if (Instant && channel)
     snprintf(file, sizeof(file), "%s%s", Setup.MarkInstantRecord ? "@" : "", *Setup.NameInstantRecord ? Setup.NameInstantRecord : channel->Name());
  if (VfatFileSystem && (Utf8StrLen(file) > VFAT_MAX_FILENAME)) {
     dsyslog("timer file name too long for VFAT file system: '%s'", file);
     file[Utf8SymChars(file, VFAT_MAX_FILENAME)] = 0;
     dsyslog("timer file name truncated to '%s'", file);
     }
}

cTimer::cTimer(const cEvent *Event)
{
#ifdef USEMYSQL
    ID = 0;
#endif
  startTime = stopTime = 0;
  lastSetEvent = 0;
  deferred = 0;
  recording = pending = inVpsMargin = false;
  flags = tfActive;
  if (Event->Vps() && Setup.UseVps)
     SetFlags(tfVps);
  channel = Channels.GetByChannelID(Event->ChannelID(), true);
  time_t tstart = (flags & tfVps) ? Event->Vps() : Event->StartTime();
  time_t tstop = tstart + Event->Duration();
  if (!(HasFlags(tfVps))) {
     tstop  += Setup.MarginStop * 60;
     tstart -= Setup.MarginStart * 60;
     }
  struct tm tm_r;
  struct tm *time = localtime_r(&tstart, &tm_r);
  day = SetTime(tstart, 0);
  weekdays = 0;
  start = time->tm_hour * 100 + time->tm_min;
  time = localtime_r(&tstop, &tm_r);
  stop = time->tm_hour * 100 + time->tm_min;
  if (stop >= 2400)
     stop -= 2400;
  priority = Setup.DefaultPriority;
  lifetime = Setup.DefaultLifetime;
#ifdef USE_PINPLUGIN
  fskProtection = 0;
#endif /* PINPLUGIN */
  *file = 0;
  const char *Title = Event->Title();
  if (!isempty(Title))
     Utf8Strn0Cpy(file, Event->Title(), sizeof(file));
  if (VfatFileSystem && (Utf8StrLen(file) > VFAT_MAX_FILENAME)) {
     dsyslog("timer file name too long for VFAT file system: '%s'", file);
     file[Utf8SymChars(file, VFAT_MAX_FILENAME)] = 0;
     dsyslog("timer file name truncated to '%s'", file);
     }
  aux = NULL;
  event = NULL; // let SetEvent() be called to get a log message
#ifdef USE_PINPLUGIN
  cStatus::MsgTimerCreation(this, Event);
#endif /* PINPLUGIN */
}

cTimer::cTimer(const cTimer &Timer)
{
#ifdef USEMYSQL
    ID = 0;
#endif
  channel = NULL;
  aux = NULL;
  event = NULL;
  flags = tfNone;
  *this = Timer;
}

cTimer::~cTimer()
{
  free(aux);
}

cTimer& cTimer::operator= (const cTimer &Timer)
{
  if (&Timer != this) {
     uint OldFlags = flags & tfRecording;
     startTime    = Timer.startTime;
     stopTime     = Timer.stopTime;
     lastSetEvent = 0;
     deferred = 0;
     recording    = Timer.recording;
     pending      = Timer.pending;
     inVpsMargin  = Timer.inVpsMargin;
     flags        = Timer.flags | OldFlags;
     channel      = Timer.channel;
     day          = Timer.day;
     weekdays     = Timer.weekdays;
     start        = Timer.start;
     stop         = Timer.stop;
     priority     = Timer.priority;
     lifetime     = Timer.lifetime;
#ifdef USE_PINPLUGIN
     fskProtection = Timer.fskProtection;
#endif /* PINPLUGIN */
     strncpy(file, Timer.file, sizeof(file));
     free(aux);
     aux = Timer.aux ? strdup(Timer.aux) : NULL;
     event = NULL;
#ifdef USEMYSQL
     ID = Timer.GetID();
#endif

     }
  return *this;
}

int cTimer::Compare(const cListObject &ListObject) const
{
  const cTimer *ti = (const cTimer *)&ListObject;
  time_t t1 = StartTime();
  time_t t2 = ti->StartTime();
  int r = t1 - t2;
  if (r == 0)
     r = ti->priority - priority;
  return r;
}

cString cTimer::ToText(bool UseChannelID) const
{
  strreplace(file, ':', '|');
  cString buffer = cString::sprintf("%u:%s:%s:%04d:%04d:%d:%d:%s:%s\n", flags, UseChannelID ? *Channel()->GetChannelID().ToString() : *itoa(Channel()->Number()), *PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux ? aux : "");
  strreplace(file, '|', ':');
  return buffer;
}

cString cTimer::ToDescr(void) const
{
  return cString::sprintf("%d (%d %04d-%04d %s'%s')", Index() + 1, Channel()->Number(), start, stop, HasFlags(tfVps) ? "VPS " : "", file);
}

int cTimer::TimeToInt(int t)
{
  return (t / 100 * 60 + t % 100) * 60;
}

bool cTimer::ParseDay(const char *s, time_t &Day, int &WeekDays)
{
  // possible formats are:
  // 19
  // 2005-03-19
  // MTWTFSS
  // MTWTFSS@19
  // MTWTFSS@2005-03-19

  Day = 0;
  WeekDays = 0;
  s = skipspace(s);
  if (!*s)
     return false;
  const char *a = strchr(s, '@');
  const char *d = a ? a + 1 : isdigit(*s) ? s : NULL;
  if (d) {
     if (strlen(d) == 10) {
        struct tm tm_r;
        if (3 == sscanf(d, "%d-%d-%d", &tm_r.tm_year, &tm_r.tm_mon, &tm_r.tm_mday)) {
           tm_r.tm_year -= 1900;
           tm_r.tm_mon--;
           tm_r.tm_hour = tm_r.tm_min = tm_r.tm_sec = 0;
           tm_r.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
           Day = mktime(&tm_r);
           }
        else
           return false;
        }
     else {
        // handle "day of month" for compatibility with older versions:
        char *tail = NULL;
        int day = strtol(d, &tail, 10);
        if ((tail && *tail) || day < 1 || day > 31)
           return false;
        time_t t = time(NULL);
        int DaysToCheck = 61; // 61 to handle months with 31/30/31
        for (int i = -1; i <= DaysToCheck; i++) {
            time_t t0 = IncDay(t, i);
            if (GetMDay(t0) == day) {
               Day = SetTime(t0, 0);
               break;
               }
            }
        }
     }
  if (a || !isdigit(*s)) {
     if ((a && a - s == 7) || strlen(s) == 7) {
        for (const char *p = s + 6; p >= s; p--) {
            WeekDays <<= 1;
            WeekDays |= (*p != '-');
            }
        }
     else
        return false;
     }
  return true;
}

cString cTimer::PrintDay(time_t Day, int WeekDays, bool SingleByteChars)
{
#define DAYBUFFERSIZE 64
  char buffer[DAYBUFFERSIZE];
  char *b = buffer;
  if (WeekDays) {
     // TRANSLATORS: the first character of each weekday, beginning with monday
     const char *w = trNOOP("MTWTFSS");
     if (!SingleByteChars)
        w = tr(w);
     while (*w) {
           int sl = Utf8CharLen(w);
           if (WeekDays & 1) {
              for (int i = 0; i < sl; i++)
                  b[i] = w[i];
              b += sl;
              }
           else
              *b++ = '-';
           WeekDays >>= 1;
           w += sl;
           }
     if (Day)
        *b++ = '@';
     }
  if (Day) {
     struct tm tm_r;
     localtime_r(&Day, &tm_r);
     b += strftime(b, DAYBUFFERSIZE - (b - buffer), "%Y-%m-%d", &tm_r);
     }
  *b = 0;
  return buffer;
}

cString cTimer::PrintFirstDay(void) const
{
  if (weekdays) {
     cString s = PrintDay(day, weekdays, true);
     if (strlen(s) == 18)
        return *s + 8;
     }
  return ""; // not NULL, so the caller can always use the result
}

#ifdef USEMYSQL
bool cTimer::Parse(const char *s, unsigned int id)
#else
bool cTimer::Parse(const char *s)
#endif /* USEMYSQL */
{
#ifdef USEMYSQL
  bool UpdateDB = false;
  if(ID)
    UpdateDB = true;
  else
    ID = id;
#endif /* USEMYSQL */

  char *channelbuffer = NULL;
  char *daybuffer = NULL;
  char *filebuffer = NULL;
  free(aux);
  aux = NULL;
  //XXX Apparently sscanf() doesn't work correctly if the last %a argument
  //XXX results in an empty string (this first occured when the EIT gathering
  //XXX was put into a separate thread - don't know why this happens...
  //XXX As a cure we copy the original string and add a blank.
  //XXX If anybody can shed some light on why sscanf() failes here, I'd love
  //XXX to hear about that!
  char *s2 = NULL;
  int l2 = strlen(s);
  while (l2 > 0 && isspace(s[l2 - 1]))
        l2--;
  if (s[l2 - 1] == ':') {
     s2 = MALLOC(char, l2 + 3);
     strcat(strn0cpy(s2, s, l2 + 1), " \n");
     s = s2;
     }
  bool result = false;
  if (8 <= sscanf(s, "%u :%a[^:]:%a[^:]:%d :%d :%d :%d :%a[^:\n]:%a[^\n]", &flags, &channelbuffer, &daybuffer, &start, &stop, &priority, &lifetime, &filebuffer, &aux)) {
#ifdef USEMYSQL
     if(Setup.ReelboxModeTemp == eModeClient || Setup.ReelboxModeTemp == eModeServer) // Client & Server (see comment below)
     {
         if(HasFlags(tfRecording) && HasFlags(tfActive))
         {
             recording = true;
             SetFlags(tfRecording);
         }
         else
         {
             recording = false;
             if(!UpdateDB)
                 ClrFlags(tfRecording);
         }
     }

     /* Why clear recording flag when in Server mode ?
      * Since the server also reads its own updates from db!
      * So, server set the recording-flag, updates timer in db and since entry
      * into vdr_event is triggered, it reads its own update from db(!) and
      * parses the 'changes' to timer. Clearing recording flag stops the
      * recording of the timer immediately after it starts. Hence issue #453 */

     /*else if (Setup.ReelboxModeTemp == eModeServer)
     {
         if(!UpdateDB)
         {
             recording = false;
             ClrFlags(tfRecording);
         }
     }*/
     else
         ClrFlags(tfRecording);

     /*
     if(UpdateDB && Setup.DatabaseHost && HasFlags(tfRecording) && HasFlags(tfActive))
     {
        recording = true;
        SetFlags(tfRecording);
     }
     else if(!UpdateDB)
     {
        recording = false;
        ClrFlags(tfRecording);
     }
     */
#else
     ClrFlags(tfRecording);
#endif /* USEMYSQL */
     if (aux && !*skipspace(aux)) {
        free(aux);
        aux = NULL;
        }
     //TODO add more plausibility checks
     result = ParseDay(daybuffer, day, weekdays);
     if (VfatFileSystem) {
        char *p = strrchr(filebuffer, FOLDERDELIMCHAR);
        if (p)
           p++;
        else
           p = filebuffer;
        if (Utf8StrLen(p) > VFAT_MAX_FILENAME) {
           dsyslog("timer file name too long for VFAT file system: '%s'", p);
           p[Utf8SymChars(p, VFAT_MAX_FILENAME)] = 0;
           dsyslog("timer file name truncated to '%s'", p);
           }
        }
     Utf8Strn0Cpy(file, filebuffer, sizeof(file));
     strreplace(file, '|', ':');
     if (isnumber(channelbuffer))
        channel = Channels.GetByNumber(atoi(channelbuffer));
     else
        channel = Channels.GetByChannelID(tChannelID::FromString(channelbuffer), true, true);
     if (!channel) {
        esyslog("ERROR: channel %s not defined", channelbuffer);
        result = false;
        }
     }
#ifdef USE_PINPLUGIN
  fskProtection = aux && strstr(aux, "<pin-plugin><protected>yes</protected></pin-plugin>");
#endif /* PINPLUGIN */
#ifdef USEMYSQL
  if(UpdateDB && ((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer))) // Client & Server
      UpdateTimerDB();
#endif /* USEMYSQL */
  free(channelbuffer);
  free(daybuffer);
  free(filebuffer);
  free(s2);
  return result;
}

bool cTimer::Save(FILE *f)
{
  return fprintf(f, "%s", *ToText(true)) > 0;
}

bool cTimer::IsSingleEvent(void) const
{
  return !weekdays;
}

int cTimer::GetMDay(time_t t)
{
  struct tm tm_r;
  return localtime_r(&t, &tm_r)->tm_mday;
}

int cTimer::GetWDay(time_t t)
{
  struct tm tm_r;
  int weekday = localtime_r(&t, &tm_r)->tm_wday;
  return weekday == 0 ? 6 : weekday - 1; // we start with Monday==0!
}

bool cTimer::DayMatches(time_t t) const
{
  return IsSingleEvent() ? SetTime(t, 0) == day : (weekdays & (1 << GetWDay(t))) != 0;
}

time_t cTimer::IncDay(time_t t, int Days)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_mday += Days; // now tm_mday may be out of its valid range
  int h = tm.tm_hour; // save original hour to compensate for DST change
  tm.tm_isdst = -1;   // makes sure mktime() will determine the correct DST setting
  t = mktime(&tm);    // normalize all values
  tm.tm_hour = h;     // compensate for DST change
  return mktime(&tm); // calculate final result
}

time_t cTimer::SetTime(time_t t, int SecondsFromMidnight)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_hour = SecondsFromMidnight / 3600;
  tm.tm_min = (SecondsFromMidnight % 3600) / 60;
  tm.tm_sec =  SecondsFromMidnight % 60;
  tm.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
  return mktime(&tm);
}

void cTimer::SetFile(const char *File)
{
  if (!isempty(File))
     Utf8Strn0Cpy(file, File, sizeof(file));
}

#define EITPRESENTFOLLOWINGRATE 10 // max. seconds between two occurrences of the "EIT present/following table for the actual multiplex" (2s by the standard, using some more for safety)

bool cTimer::Matches(time_t t, bool Directly, int Margin) const
{
  startTime = stopTime = 0;
  if (t == 0)
     t = time(NULL);

  int begin  = TimeToInt(start); // seconds from midnight
  int length = TimeToInt(stop) - begin;
  if (length < 0)
     length += SECSINDAY;

  if (IsSingleEvent()) {
     startTime = SetTime(day, begin);
     stopTime = startTime + length;
     }
  else {
     for (int i = -1; i <= 7; i++) {
         time_t t0 = IncDay(day ? max(day, t) : t, i);
         if (DayMatches(t0)) {
            time_t a = SetTime(t0, begin);
            time_t b = a + length;
            if ((!day || a >= day) && t < b) {
               startTime = a;
               stopTime = b;
               break;
               }
            }
         }
     if (!startTime)
        startTime = IncDay(t, 7); // just to have something that's more than a week in the future
     else if (!Directly && (t > startTime || t > day + SECSINDAY + 3600)) // +3600 in case of DST change
        day = 0;
     }

  if (t < deferred)
     return false;
  deferred = 0;

  if (HasFlags(tfActive)) {
     if (HasFlags(tfVps) && event && event->Vps()) {
        if (Margin || !Directly) {
           startTime = event->StartTime();
           stopTime = event->EndTime();
           if (!Margin) { // this is an actual check
              if (event->Schedule()->PresentSeenWithin(EITPRESENTFOLLOWINGRATE)) // VPS control can only work with up-to-date events...
                 return event->IsRunning(true);
              else
                 return startTime <= t && t < stopTime; // ...otherwise we fall back to normal timer handling
              }
           }
        }
     return startTime <= t + Margin && t < stopTime; // must stop *before* stopTime to allow adjacent timers
     }
  return false;
}

#define FULLMATCH 1000

int cTimer::Matches(const cEvent *Event, int *Overlap) const
{
  // Overlap is the percentage of the Event's duration that is covered by
  // this timer (based on FULLMATCH for finer granularity than just 100).
  // To make sure a VPS timer can be distinguished from a plain 100% overlap,
  // it gets an additional 100 added, and a VPS event that is actually running
  // gets 200 added to the FULLMATCH.
  if (HasFlags(tfActive) && channel->GetChannelID() == Event->ChannelID()) {
     bool UseVps = HasFlags(tfVps) && Event->Vps();
     Matches(UseVps ? Event->Vps() : Event->StartTime(), true);
     int overlap = 0;
     if (UseVps)
        overlap = (startTime == Event->Vps()) ? FULLMATCH + (Event->IsRunning() ? 200 : 100) : 0;
     if (!overlap) {
        if (startTime <= Event->StartTime() && Event->EndTime() <= stopTime)
           overlap = FULLMATCH;
        else if (stopTime <= Event->StartTime() || Event->EndTime() <= startTime)
           overlap = 0;
        else
           overlap = (min(stopTime, Event->EndTime()) - max(startTime, Event->StartTime())) * FULLMATCH / max(Event->Duration(), 1);
        }
     startTime = stopTime = 0;
     if (Overlap)
        *Overlap = overlap;
     if (UseVps)
        return overlap > FULLMATCH ? tmFull : tmNone;
     return overlap >= FULLMATCH ? tmFull : overlap > 0 ? tmPartial : tmNone;
     }
  return tmNone;
}

#define EXPIRELATENCY 60 // seconds (just in case there's a short glitch in the VPS signal)

bool cTimer::Expired(void) const
{
  return IsSingleEvent() && !Recording() && StopTime() + EXPIRELATENCY <= time(NULL) && (!HasFlags(tfVps) || !event || !event->Vps());
}

time_t cTimer::StartTime(void) const
{
  if (!startTime)
     Matches();
  return startTime;
}

time_t cTimer::StopTime(void) const
{
  if (!stopTime)
     Matches();
  return stopTime;
}

#define EPGLIMITBEFORE   (1 * 3600) // Time in seconds before a timer's start time and
#define EPGLIMITAFTER    (1 * 3600) // after its stop time within which EPG events will be taken into consideration.

void cTimer::SetEventFromSchedule(const cSchedules *Schedules)
{
  cSchedulesLock SchedulesLock;
  if (!Schedules) {
     lastSetEvent = 0; // forces setting the event, even if the schedule hasn't been modified
     if (!(Schedules = cSchedules::Schedules(SchedulesLock)))
        return;
     }
  const cSchedule *Schedule = Schedules->GetSchedule(Channel());
  if (Schedule && Schedule->Events()->First()) {
     time_t now = time(NULL);
     if (!lastSetEvent || Schedule->Modified() >= lastSetEvent) {
        lastSetEvent = now;
        const cEvent *Event = NULL;
        if (HasFlags(tfVps) && Schedule->Events()->First()->Vps()) {
           if (event && Recording())
              return; // let the recording end first
           // VPS timers only match if their start time exactly matches the event's VPS time:
           for (const cEvent *e = Schedule->Events()->First(); e; e = Schedule->Events()->Next(e)) {
               if (e->StartTime() && e->RunningStatus() != SI::RunningStatusNotRunning) { // skip outdated events
                  int overlap = 0;
                  Matches(e, &overlap);
                  if (overlap > FULLMATCH) {
                     Event = e;
                     break; // take the first matching event
                     }
                  }
               }
           if (!Event && event && (now <= event->EndTime() || Matches(0, true)))
              return; // stay with the old event until the timer has completely expired
           }
        else {
           // Normal timers match the event they have the most overlap with:
           int Overlap = 0;
           // Set up the time frame within which to check events:
           Matches(0, true);
           time_t TimeFrameBegin = StartTime() - EPGLIMITBEFORE;
           time_t TimeFrameEnd   = StopTime()  + EPGLIMITAFTER;
           for (const cEvent *e = Schedule->Events()->First(); e; e = Schedule->Events()->Next(e)) {
               if (e->EndTime() < TimeFrameBegin)
                  continue; // skip events way before the timer starts
               if (e->StartTime() > TimeFrameEnd)
                  break; // the rest is way after the timer ends
               int overlap = 0;
               Matches(e, &overlap);
               if (overlap && overlap >= Overlap) {
                  if (Event && overlap == Overlap && e->Duration() <= Event->Duration())
                     continue; // if overlap is the same, we take the longer event
                  Overlap = overlap;
                  Event = e;
                  }
               }
           }
        SetEvent(Event);
        }
     }
}

void cTimer::SetEvent(const cEvent *Event)
{
  if (event != Event) { //XXX TODO check event data, too???
     if (Event)
        isyslog("timer %s set to event %s", *ToDescr(), *Event->ToDescr());
     else
        isyslog("timer %s set to no event", *ToDescr());
     event = Event;
     }
}

void cTimer::SetRecording(bool Recording)
{
  uint old_flags = flags;

  recording = Recording;
  if (recording)
     SetFlags(tfRecording);
  else
     ClrFlags(tfRecording);
  isyslog("timer %s %s", *ToDescr(), recording ? "start" : "stop");
#ifdef USEMYSQL
  /* if nothing was modified, do not update db */
  if (old_flags == flags) 
     return;

  // Maybe not working :(
  if(Setup.ReelboxMode == eModeServer) // Only Server should write state of recording into DB
      UpdateTimerDB();
  //TODO: Timers.SetModified() NEEDS TESTING!!! It may cause problem if a timer is enabled/disabled while running record
  if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer)) // Client & Server
      Timers.SetModified(); // Needed for sync with Database
#endif /* USEMYSQL */
}

void cTimer::SetPending(bool Pending)
{
  pending = Pending;
}

void cTimer::SetInVpsMargin(bool InVpsMargin)
{
  if (InVpsMargin && !inVpsMargin)
     isyslog("timer %s entered VPS margin", *ToDescr());
  inVpsMargin = InVpsMargin;
}

void cTimer::SetDay(time_t Day)
{
  day = Day;
}

void cTimer::SetWeekDays(int WeekDays)
{
  weekdays = WeekDays;
}

void cTimer::SetStart(int Start)
{
  start = Start;
}

void cTimer::SetStop(int Stop)
{
  stop = Stop;
}

void cTimer::SetPriority(int Priority)
{
  priority = Priority;
}

void cTimer::SetLifetime(int Lifetime)
{
  lifetime = Lifetime;
}

void cTimer::SetAux(const char *Aux)
{
  free(aux);
  aux = strdup(Aux);
}

void cTimer::SetDeferred(int Seconds)
{
  deferred = time(NULL) + Seconds;
  isyslog("timer %s deferred for %d seconds", *ToDescr(), Seconds);
}

void cTimer::SetFlags(uint Flags)
{
  flags |= Flags;
}

void cTimer::ClrFlags(uint Flags)
{
  flags &= ~Flags;
}

void cTimer::InvFlags(uint Flags)
{
  flags ^= Flags;
}

bool cTimer::HasFlags(uint Flags) const
{
  return (flags & Flags) == Flags;
}

void cTimer::Skip(void)
{
  day = IncDay(SetTime(StartTime(), 0), 1);
  startTime = 0;
  SetEvent(NULL);
}

void cTimer::OnOff(void)
{
  if (IsSingleEvent())
     InvFlags(tfActive);
  else if (day) {
     day = 0;
     ClrFlags(tfActive);
     }
  else if (HasFlags(tfActive))
     Skip();
  else
     SetFlags(tfActive);
  SetEvent(NULL);
  Matches(); // refresh start and end time
#ifdef USEMYSQL
  if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxMode == eModeServer)) // Client
      UpdateTimerDB();
#endif /* USEMYSQL */
}

#ifdef USEMYSQL
void cTimer::UpdateTimerDB()
{
    bool SetServer = false;
    // Update Timer in Database if neccessary
    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
        SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    else
        SetServer = TimersMysql->SetServer("localhost");
    if(SetServer)
        TimersMysql->UpdateTimer(this);
    delete TimersMysql;
}
#endif /* USEMYSQL */

#ifdef USE_PINPLUGIN
void cTimer::SetFskProtection(int aFlag)
{
   char* p;
   char* tmp = 0;

   fskProtection = aFlag;

   if (fskProtection && (!aux || !strstr(aux, "<pin-plugin><protected>yes</protected></pin-plugin>")))
   {
      // add protection info to aux

      if (aux) { tmp = strdup(aux); free(aux); }
      if (asprintf(&aux,"%s<pin-plugin><protected>yes</protected></pin-plugin>", tmp ? tmp : "") < 0 )
         aux = NULL;
   }
   else if (!fskProtection && aux && (p = strstr(aux, "<pin-plugin><protected>yes</protected></pin-plugin>")))
   {
      // remove protection info to aux

      if (asprintf(&tmp, "%.*s%s", p-aux, aux, p+strlen("<pin-plugin><protected>yes</protected></pin-plugin>")) >= 0 ) {
         free(aux);
         aux = strdup(tmp);
         }
   }

   if (tmp)
      free(tmp);
}
#endif /* PINPLUGIN */

// --- cTimers ---------------------------------------------------------------

cTimers Timers;

cTimers::cTimers(void)
{
  state = 0;
  beingEdited = 0;;
  lastSetEvents = 0;
  lastDeleteExpired = 0;
#ifdef USEMYSQL
  LastEventID = 0;
#endif /* USEMYSQL */
}

#ifdef USEMYSQL
cTimer *cTimers::GetTimer(cTimer *Timer, bool CheckID)
{
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      if ((!CheckID || (ti->GetID() == Timer->GetID())) && ti->Channel() == Timer->Channel() &&
          ((ti->WeekDays() && ti->WeekDays() == Timer->WeekDays()) || (!ti->WeekDays() && ti->Day() == Timer->Day())) &&
          ti->Start() == Timer->Start() &&
          ti->Stop() == Timer->Stop())
         return ti;
      }
  return NULL;
}
#else
cTimer *cTimers::GetTimer(cTimer *Timer)
{
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      if (ti->Channel() == Timer->Channel() &&
          (ti->WeekDays() && ti->WeekDays() == Timer->WeekDays() || !ti->WeekDays() && ti->Day() == Timer->Day()) &&
          ti->Start() == Timer->Start() &&
          ti->Stop() == Timer->Stop())
         return ti;
      }
  return NULL;
}
#endif

#ifdef USEMYSQL
cTimer *cTimers::GetTimerByID(unsigned int id)
{
    for(cTimer *ti=First(); ti; ti=Next(ti))
    {
        if(ti->GetID() == id)
            return ti;
    }
    return NULL;
}

void cTimers::GetInstantRecordings(std::vector<cTimer*> *InstantRecordings)
{
    bool SetServer = false;
    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
        SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    else if(Setup.ReelboxModeTemp == eModeServer)
        SetServer = TimersMysql->SetServer("localhost");
    if(SetServer)
        TimersMysql->GetInstantRecordings(InstantRecordings);
    delete TimersMysql;
}
#endif /* USEMYSQL */

cTimer *cTimers::GetMatch(time_t t)
{
  static int LastPending = -1;
  cTimer *t0 = NULL;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      if (!ti->Recording() && ti->Matches(t)) {
         if (ti->Pending()) {
            if (ti->Index() > LastPending)
               LastPending = ti->Index();
            else
               continue;
            }
         if (!t0 || ti->Priority() > t0->Priority())
            t0 = ti;
         }
      }
  if (!t0)
     LastPending = -1;
  return t0;
}

cTimer *cTimers::GetMatch(const cEvent *Event, int *Match)
{
  cTimer *t = NULL;
  int m = tmNone;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      int tm = ti->Matches(Event);
      if (tm > m) {
         t = ti;
         m = tm;
         if (m == tmFull)
            break;
         }
      }
  if (Match)
     *Match = m;
  return t;
}

cTimer *cTimers::GetNextActiveTimer(void)
{
  cTimer *t0 = NULL;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      ti->Matches();
      if ((ti->HasFlags(tfActive)) && (!t0 || (ti->StopTime() > time(NULL) && ti->Compare(*t0) < 0)))
         t0 = ti;
      }
  return t0;
}

void cTimers::SetModified(void)
{
  cStatus::MsgTimerChange(NULL, tcMod);
  state++;
}

#ifdef USEMYSQL
extern int DBCounter;
#endif /* USEMYSQL */

#ifdef USEMYSQL
bool cTimers::Add(cTimer *Timer, cTimer *After)
#else
void cTimers::Add(cTimer *Timer, cTimer *After)
#endif /* USEMYSQL */
{
#ifdef USEMYSQL
  if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer)) // Client & Server
  {
    bool SetServer = false;
    bool res = false;
    // Add Timer to Database
    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
      SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    else
      SetServer = TimersMysql->SetServer("localhost");
    if(SetServer)
      res = TimersMysql->InsertTimer(Timer);
    if(res)
      DBCounter++;
    delete TimersMysql;
    if(res)
    {
        cConfig<cTimer>::Add(Timer, After);
        cStatus::MsgTimerChange(Timer, tcAdd);
    }
    return res;
  }
  else if((Setup.ReelboxMode == eModeClient) && (Setup.ReelboxModeTemp == eModeStandalone))
  {
      Skins.QueueMessage(mtError, tr("Database not available!"));
      esyslog("ERROR: Database not available!");
      return false;
  }
  else
  {
    cConfig<cTimer>::Add(Timer, After);
    cStatus::MsgTimerChange(Timer, tcAdd);
  }
  return true;
#else
  cConfig<cTimer>::Add(Timer, After);
  cStatus::MsgTimerChange(Timer, tcAdd);
#endif /* USEMYSQL */
}

#ifdef USEMYSQL
bool cTimers::Ins(cTimer *Timer, cTimer *Before)
#else
void cTimers::Ins(cTimer *Timer, cTimer *Before)
#endif /* USEMYSQL */
{
#ifdef USEMYSQL
  if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer)) // Client & Server
  {
    bool SetServer = false;
    bool res = false;
    // Add Timer to Database
    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
      SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    else
      SetServer = TimersMysql->SetServer("localhost");
    if(SetServer)
      res = TimersMysql->InsertTimer(Timer);
    delete TimersMysql;
    if(res)
    {
        cConfig<cTimer>::Ins(Timer, Before);
        cStatus::MsgTimerChange(Timer, tcAdd);
    }
    return res;
  }
  else if((Setup.ReelboxMode == eModeClient) && (Setup.ReelboxModeTemp == eModeStandalone))
  {
      Skins.QueueMessage(mtError, tr("Database not available!"));
      esyslog("ERROR: Database not available!");
      return false;
  }
  else
  {
    cConfig<cTimer>::Ins(Timer, Before);
    cStatus::MsgTimerChange(Timer, tcAdd);
  }
  return true;
#else
  cConfig<cTimer>::Ins(Timer, Before);
  cStatus::MsgTimerChange(Timer, tcAdd);
#endif /* USEMYSQL */
}

#ifdef USEMYSQL
bool cTimers::Del(cTimer *Timer, bool DeleteObject)
#else
void cTimers::Del(cTimer *Timer, bool DeleteObject)
#endif
{
#ifdef USEMYSQL
  if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer)) // Client & Server
  {
    bool SetServer = false;
    bool res = false;
    // Remove Timer from Database
    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
      SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    else
      SetServer = TimersMysql->SetServer("localhost");
    if(SetServer)
      res = TimersMysql->DeleteTimer(Timer->GetID());
    delete TimersMysql;
    cStatus::MsgTimerChange(Timer, tcDel);
    cConfig<cTimer>::Del(Timer, DeleteObject);
    return res;
  }
  else
  {
      cStatus::MsgTimerChange(Timer, tcDel);
      cConfig<cTimer>::Del(Timer, DeleteObject);
  }
  return true;
#else
  cStatus::MsgTimerChange(Timer, tcDel);
  cConfig<cTimer>::Del(Timer, DeleteObject);
#endif /* USEMYSQL */
}

#ifdef USEMYSQL
bool cTimers::LoadDB()
{
  bool SetServer = false;

  cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
 
  if(Setup.ReelboxModeTemp == eModeClient) // Client
  {
      if(Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
          SetServer = TimersMysql->SetServer(Setup.NetServerIP);
      else
          SetServer = false;
  }
  else
    SetServer = TimersMysql->SetServer("localhost");

  if(SetServer)
  {
    std::vector<StringTimer> StringTimers;
    TimersMysql->LoadDB(&StringTimers, &LastEventID);

    for(unsigned int i=0; i < StringTimers.size(); ++i)
    {
      cTimer *timer = new cTimer;
      if(timer->Parse(StringTimers.at(i).s, StringTimers.at(i).ID))
        cConfig<cTimer>::Add(timer);
      else
        delete timer;
      free(StringTimers.at(i).s); // clear memory
    }
    delete TimersMysql;
    return true;
  }

  delete TimersMysql;
  esyslog("ERROR (%s,%i): LoadDB failed\n", __FILE__, __LINE__);
  return false;
}

void cTimers::SyncData()
{
    bool SetServer = false;

    static int TimerState = 0;
    int oldState = state;
    bool modified = Modified(TimerState);

    cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");

    if(Setup.ReelboxModeTemp == eModeClient) // Client
    {
        if(Setup.NetServerIP && strlen(Setup.NetServerIP)) // Got NetServerIP?
            SetServer = TimersMysql->SetServer(Setup.NetServerIP);
    }
    else
        SetServer = TimersMysql->SetServer("localhost");

    if(SetServer)
    {
        TimersMysql->Sync(&LastEventID);
// UpdateDB() is obsolete
//        if(modified)
//            TimersMysql->UpdateDB();
//        else
        if(!modified)
            state = oldState; // undo change in state, caused by TimersMysql->Sync()
    }

    delete TimersMysql;
}
#endif /* USEMYSQL */

bool cTimers::Modified(int &State)
{
  bool Result = state != State;
  State = state;
  return Result;
}

void cTimers::SetEvents(void)
{
  if (time(NULL) - lastSetEvents < 5)
     return;
  cSchedulesLock SchedulesLock(false, 100);
  const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
  if (Schedules) {
     if (!lastSetEvents || Schedules->Modified() >= lastSetEvents) {
        for (cTimer *ti = First(); ti; ti = Next(ti)) {
            if (cRemote::HasKeys())
               return; // react immediately on user input
            ti->SetEventFromSchedule(Schedules);
            }
        }
     }
  lastSetEvents = time(NULL);
}

void cTimers::DeleteExpired(void)
{
  if (time(NULL) - lastDeleteExpired < 30)
     return;
  cTimer *ti = First();
  while (ti) {
        cTimer *next = Next(ti);
        if (ti->Expired()) {
           isyslog("deleting timer %s", *ti->ToDescr());
           Del(ti);
           SetModified();
           }
        ti = next;
        }
  lastDeleteExpired = time(NULL);
}

// --- cSortedTimers ---------------------------------------------------------

static int CompareTimers(const void *a, const void *b)
{
  return (*(const cTimer **)a)->Compare(**(const cTimer **)b);
}

cSortedTimers::cSortedTimers(void)
:cVector<const cTimer *>(Timers.Count())
{
  for (const cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
      Append(Timer);
  Sort(CompareTimers);
}
