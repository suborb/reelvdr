/*
 * epg.c: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.c 1.81 2006/10/28 09:12:42 kls Exp $
 */

#include "epg.h"
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include "libsi/si.h"
#include "timers.h"

#define RUNNINGSTATUSTIMEOUT 30 // seconds before the running status is considered unknown

// UPC Direct / HBO strange two-character encoding. 0xC2 means acute, 0xCF caron.
// many thanks to the czechs who helped me while solving this.
void checkCzech( char *str )
{
   char *s1 = str;
   char *s2 = str;
   char nc;

   if (!str)
      return;

   while (*s1 != '\0') {
      nc = *s1;
      switch (*s1) {
         case 0xC2: // acute: á é í ó ú ý
            s1++;
            switch (*s1) {
               case 'A': nc = (char)0xC1;
                  break;
               case 'a': nc = (char)0xE1;
                  break;
               case 'E': nc = (char)0xC9;
                  break;
               case 'e': nc = (char)0xE9;
                  break;
               case 'I': nc = (char)0xCD;
                  break;
               case 'i': nc = (char)0xED;
                  break;
               case 'O': nc = (char)0xD3;
                  break;
               case 'o': nc = (char)0xF3;
                  break;
               case 'U': nc = (char)0xDA;
                  break;
               case 'u': nc = (char)0xFA;
                  break;
               case 'Y': nc = (char)0xDD;
                  break;
               case 'y': nc = (char)0xFD;
                  break;
               default:
                  s1--;
                  break;
            }
	         break;
         case 0xC6:
            s1++;
            switch (*s1) {
               case 'S': nc = (char)0xA9;
                  break;
               case 's': nc = (char)0xB9;
                  break;
               default:
                  s1--;
                  break;
            }
            break;
         case 0xC8:
            s1++;
            switch (*s1) {
               case 'A': nc = (char)0xC4;
                  break;
               case 'a': nc = (char)0xE4;
                  break;
               case 'O': nc = (char)0xD6;
                  break;
               case 'o': nc = (char)0xF6;
                  break;
               case 'U': nc = (char)0xDC;
                  break;
               case 'u': nc = (char)0xFC;
                  break;
               default:
                  s1--;
                  break;
            }
            break;
         case 0xCA: // krouzek http://de.wikipedia.org/wiki/Krouzek
            s1++;
            switch (*s1) {
               case 'U': nc = (char)0xD9;
                  break;
               case 'u': nc = (char)0xF9;
                  break;
               default:
                  s1--;
                  break;
            }
            break;
         case 0xCD:
            s1++;
            switch (*s1) {
               case 'O': nc = (char)0xD5;
                  break;
               case 'o': nc = (char)0xF5;
                  break;
               case 'U': nc = (char)0xDB;
                  break;
               case 'u': nc = (char)0xFB;
                  break;
               default:
                  s1--;
                  break;
            }
            break;
         case 0xCF: // caron
            s1++;
            switch (*s1) {
               case 'C': nc =  (char)0xC8;
                  break;
               case 'c': nc =  (char)0xE8;
                  break;
               case 'D': nc =  (char)0xCF;
                  break;
               case 'd': nc =  (char)0xEF;
                  break;
               case 'E': nc =  (char)0xCC;
                  break;
               case 'e': nc =  (char)0xEC;
                  break;
               case 'L': nc =  (char)0xC5;        // not sure if they really exist.
                  break;
               case 'l': nc =  (char)0xE5;
                  break;
               case 'N': nc =  (char)0xD2;
                  break;
               case 'n': nc =  (char)0xF2;
                  break;
               case 'R': nc =  (char)0xD8;
                  break;
               case 'r': nc =  (char)0xF8;
                  break;
               case 'S': nc =  (char)0xA9;
                  break;
               case 's': nc =  (char)0xB9;
                  break;
               case 'T': nc =  (char)0xAB;
                  break;
               case 't': nc =  (char)0xBB;
                  break;
               case 'Z': nc =  (char)0xAE;
                  break;
               case 'z': nc =  (char)0xBE;
                  break;
               default:
                  s1--;
                  break;
            }
	         break;
         default:
            break;
      }
      s1++;
      *s2 = nc;
      s2++;
   }
   *s2 = '\0';
}
// --- tComponent ------------------------------------------------------------

cString tComponent::ToString(void)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%X %02X %s %s", stream, type, language, description ? description : "");
  return buffer;
}

bool tComponent::FromString(const char *s)
{
  unsigned int Stream, Type;
  int n = sscanf(s, "%X %02X %7s %a[^\n]", &Stream, &Type, language, &description); // 7 = MAXLANGCODE2 - 1
  if (n != 4 || isempty(description)) {
     free(description);
     description = NULL;
     }
  stream = Stream;
  type = Type;
  return n >= 3;
}

// --- cComponents -----------------------------------------------------------

cComponents::cComponents(void)
{
  numComponents = 0;
  components = NULL;
}

cComponents::~cComponents(void)
{
  for (int i = 0; i < numComponents; i++)
      free(components[i].description);
  free(components);
}

void cComponents::Clear(void)
{
  for (int i = 0; i < numComponents; i++)
      free(components[i].description);
  free(components);
  components = NULL;
  numComponents = 0;
}

void cComponents::Realloc(int Index)
{
  if (Index >= numComponents) {
     int n = numComponents;
     numComponents = Index + 1;
     components = (tComponent *)realloc(components, numComponents * sizeof(tComponent));
     memset(&components[n], 0, sizeof(tComponent) * (numComponents - n));
     }
}

void cComponents::SetComponent(int Index, const char *s)
{
  Realloc(Index);
  components[Index].FromString(s);
}

void cComponents::SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description)
{
  Realloc(Index);
  tComponent *p = &components[Index];
  p->stream = Stream;
  p->type = Type;
  strn0cpy(p->language, Language, sizeof(p->language));
  char *q = strchr(p->language, ',');
  if (q)
     *q = 0; // strips rest of "normalized" language codes
  p->description = strcpyrealloc(p->description, !isempty(Description) ? Description : NULL);
}

tComponent *cComponents::GetComponent(int Index, uchar Stream, uchar Type)
{
  for (int i = 0; i < numComponents; i++) {
      // In case of an audio stream the 'type' check actually just distinguishes between "normal" and "Dolby Digital":
      if (components[i].stream == Stream && (Stream != 2 || (components[i].type < 5) == (Type < 5))) {
         if (!Index--)
            return &components[i];
         }
      }
  return NULL;
}

// --- cEvent ----------------------------------------------------------------

cEvent::cEvent(tEventID EventID)
{
  schedule = NULL;
  eventID = EventID;
  tableID = 0;
  version = 0xFF; // actual version numbers are 0..31
  runningStatus = SI::RunningStatusUndefined;
  title = NULL;
  shortText = NULL;
  description = NULL;
  components = NULL;
  startTime = 0;
  duration = 0;
  vps = 0;
  SetSeen();
}

cEvent::~cEvent()
{
  free(title);
  free(shortText);
  free(description);
  delete components;
}

int cEvent::Compare(const cListObject &ListObject) const
{
  cEvent *e = (cEvent *)&ListObject;
  return startTime - e->startTime;
}

tChannelID cEvent::ChannelID(void) const
{
  return schedule ? schedule->ChannelID() : tChannelID();
}

void cEvent::SetEventID(tEventID EventID)
{
  if (eventID != EventID) {
     if (schedule)
        schedule->UnhashEvent(this);
     eventID = EventID;
     if (schedule)
        schedule->HashEvent(this);
     }
}

void cEvent::SetTableID(uchar TableID)
{
  tableID = TableID;
}

void cEvent::SetVersion(uchar Version)
{
  version = Version;
}

void cEvent::SetRunningStatus(int RunningStatus, cChannel *Channel)
{
  if (Channel && runningStatus != RunningStatus && (RunningStatus > SI::RunningStatusNotRunning || runningStatus > SI::RunningStatusUndefined) && Channel->HasTimer())
     isyslog("channel %d (%s) event %s status %d", Channel->Number(), Channel->Name(), *ToDescr(), RunningStatus);
  runningStatus = RunningStatus;
}

void cEvent::SetTitle(const char *Title)
{
  if(Title==NULL || strlen(Title)==0) {
    if(title)
      free(title);
    title = NULL;
  }
  else
    title = strcpyrealloc(title, Title);
}

void cEvent::SetShortText(const char *ShortText)
{
  if(ShortText==NULL || strlen(ShortText)==0) {
    if(shortText)
      free(shortText);
    shortText = NULL;
  }
  else
    shortText = strcpyrealloc(shortText, ShortText);
}

void cEvent::SetDescription(const char *Description)
{
  description = strcpyrealloc(description, Description);
}

void cEvent::SetComponents(cComponents *Components)
{
  delete components;
  components = Components;
}

void cEvent::SetStartTime(time_t StartTime)
{
  if (startTime != StartTime) {
     if (schedule)
        schedule->UnhashEvent(this);
     startTime = StartTime;
     if (schedule)
        schedule->HashEvent(this);
     }
}

void cEvent::SetDuration(int Duration)
{
  duration = Duration;
}

void cEvent::SetVps(time_t Vps)
{
  vps = Vps;
}

void cEvent::SetSeen(void)
{
  seen = time(NULL);
}

cString cEvent::ToDescr(void) const
{
  char vpsbuf[64] = "";
  if (Vps())
     sprintf(vpsbuf, "(VPS: %s) ", *GetVpsString());
  return cString::sprintf("%s %s-%s %s'%s'", *GetDateString(), *GetTimeString(), *GetEndTimeString(), vpsbuf, Title());
}

bool cEvent::HasTimer(void) const
{
  for (cTimer *t = Timers.First(); t; t = Timers.Next(t)) {
      if (t->Event() == this)
         return true;
      }
  return false;
}

bool cEvent::IsRunning(bool OrAboutToStart) const
{
  return runningStatus >= (OrAboutToStart ? SI::RunningStatusStartsInAFewSeconds : SI::RunningStatusPausing);
}

cString cEvent::GetDateString(void) const
{
  return DateString(startTime);
}

cString cEvent::GetTimeString(void) const
{
  return TimeString(startTime);
}

cString cEvent::GetEndTimeString(void) const
{
  return TimeString(startTime + duration);
}

cString cEvent::GetVpsString(void) const
{
  char buf[25];
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%d.%m %R", localtime_r(&vps, &tm_r));
  return buf;
}

void cEvent::Dump(FILE *f, const char *Prefix, bool InfoOnly) const
{
  if (InfoOnly || startTime + duration + Setup.EPGLinger * 60 >= time(NULL)) {
     fprintf(f, "%sE %u %ld %d %X %X\n", Prefix, eventID, startTime, duration, tableID, version);
     if (!isempty(title))
        fprintf(f, "%sT %s\n", Prefix, title);
     if (!isempty(shortText))
        fprintf(f, "%sS %s\n", Prefix, shortText);
     if (!isempty(description)) {
        strreplace(description, '\n', '|');
        fprintf(f, "%sD %s\n", Prefix, description);
        strreplace(description, '|', '\n');
        }
     if (components) {
        for (int i = 0; i < components->NumComponents(); i++) {
#ifdef SVDRP_THREAD
            if(i%10==0)
               cCondWait::SleepMs(3);
#endif
            tComponent *p = components->Component(i);
            if (!Setup.UseDolbyDigital && p->stream == 0x02 && p->type == 0x05)
               continue;
            fprintf(f, "%sX %s\n", Prefix, *p->ToString());
            }
        }
     if (vps)
        fprintf(f, "%sV %ld\n", Prefix, vps);
     if (!InfoOnly)
        fprintf(f, "%se\n", Prefix);
     }
}

bool cEvent::Parse(char *s)
{
  char *t = skipspace(s + 1);
  switch (*s) {
    case 'T': SetTitle(t);
              break;
    case 'S': SetShortText(t);
              break;
    case 'D': strreplace(t, '|', '\n');
              SetDescription(t);
              break;
    case 'X': if (!components)
                 components = new cComponents;
              components->SetComponent(components->NumComponents(), t);
              break;
    case 'V': SetVps(atoi(t));
              break;
    default:  esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
              return false;
    }
  return true;
}

bool cEvent::Read(FILE *f, cSchedule *Schedule)
{
  if (Schedule) {
     cEvent *Event = NULL;
     char *s;
     int line = 0;
     cReadLine ReadLine;
     while ((s = ReadLine.Read(f)) != NULL) {
           line++;
           char *t = skipspace(s + 1);
           switch (*s) {
             case 'E': if (!Event) {
                          unsigned int EventID;
                          time_t StartTime;
                          int Duration;
                          unsigned int TableID = 0;
                          unsigned int Version = 0xFF; // actual value is ignored
                          int n = sscanf(t, "%u %ld %d %X %X", &EventID, &StartTime, &Duration, &TableID, &Version);
                          if (n >= 3 && n <= 5) {
                             Event = (cEvent *)Schedule->GetEvent(EventID, StartTime);
                             cEvent *newEvent = NULL;
                             if (Event)
                                DELETENULL(Event->components);
                             if (!Event) {
                                Event = newEvent = new cEvent(EventID);
                                Event->seen = 0;
                                }
                             if (Event) {
                                Event->SetTableID(TableID);
                                Event->SetStartTime(StartTime);
                                Event->SetDuration(Duration);
                                if (newEvent)
                                   Schedule->AddEvent(newEvent);
                                }
                             }
                          }
                       break;
             case 'e': if (Event && !Event->Title())
                          Event->SetTitle(tr("No title"));
                       Event = NULL;
                       break;
             case 'c': // to keep things simple we react on 'c' here
                       return true;
             default:  if (Event && !Event->Parse(s)) {
                          esyslog("ERROR: EPG data problem in line %d", line);
                          return false;
                          }
             }
           }
     esyslog("ERROR: unexpected end of file while reading EPG data");
     }
  return false;
}

#define MAXEPGBUGFIXSTATS 13
#define MAXEPGBUGFIXCHANS 100
struct tEpgBugFixStats {
  int hits;
  int n;
  tChannelID channelIDs[MAXEPGBUGFIXCHANS];
  tEpgBugFixStats(void) { hits = n = 0; }
  };

tEpgBugFixStats EpgBugFixStats[MAXEPGBUGFIXSTATS];

static void EpgBugFixStat(int Number, tChannelID ChannelID)
{
  if (0 <= Number && Number < MAXEPGBUGFIXSTATS) {
     tEpgBugFixStats *p = &EpgBugFixStats[Number];
     p->hits++;
     int i = 0;
     for (; i < p->n; i++) {
         if (p->channelIDs[i] == ChannelID)
            break;
         }
     if (i == p->n && p->n < MAXEPGBUGFIXCHANS)
        p->channelIDs[p->n++] = ChannelID;
     }
}

void ReportEpgBugFixStats(bool Reset)
{
  if (Setup.EPGBugfixLevel > 0) {
     bool GotHits = false;
     char buffer[1024];
     for (int i = 0; i < MAXEPGBUGFIXSTATS; i++) {
         const char *delim = "\t";
         tEpgBugFixStats *p = &EpgBugFixStats[i];
         if (p->hits) {
            bool PrintedStats = false;
            char *q = buffer;
            *buffer = 0;
            for (int c = 0; c < p->n; c++) {
                cChannel *channel = Channels.GetByChannelID(p->channelIDs[c], true);
                if (channel) {
                   if (!GotHits) {
                      dsyslog("=====================");
                      dsyslog("EPG bugfix statistics");
                      dsyslog("=====================");
                      dsyslog("IF SOMEBODY WHO IS IN CHARGE OF THE EPG DATA FOR ONE OF THE LISTED");
                      dsyslog("CHANNELS READS THIS: PLEASE TAKE A LOOK AT THE FUNCTION cEvent::FixEpgBugs()");
                      dsyslog("IN VDR/epg.c TO LEARN WHAT'S WRONG WITH YOUR DATA, AND FIX IT!");
                      dsyslog("=====================");
                      dsyslog("Fix\tHits\tChannels");
                      GotHits = true;
                      }
                   if (!PrintedStats) {
                      q += snprintf(q, sizeof(buffer) - (q - buffer), "%d\t%d", i, p->hits);
                      PrintedStats = true;
                      }
                   q += snprintf(q, sizeof(buffer) - (q - buffer), "%s%s", delim, channel->Name());
                   delim = ", ";
                   if (q - buffer > 80) {
                      q += snprintf(q, sizeof(buffer) - (q - buffer), "%s...", delim);
                      break;
                      }
                   }
                }
            if (*buffer)
               dsyslog("%s", buffer);
            }
         if (Reset)
            p->hits = p->n = 0;
         }
     if (GotHits)
        dsyslog("=====================");
     }
}

void cEvent::FixEpgBugs(void)
{
  if (isempty(title)) {
     // we don't want any "(null)" titles
     title = strcpyrealloc(title, tr("No title"));
     EpgBugFixStat(12, ChannelID());
     }

  if (Setup.EPGBugfixLevel == 0)
     goto Final;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the ShortText in quotes and use either the ShortText
  // or the Description field, depending on how long the string is:
  //
  // Title
  // "ShortText". Description
  //
  if ((shortText == NULL) != (description == NULL)) {
     char *p = shortText ? shortText : description;
     if (*p == '"') {
        const char *delim = "\".";
        char *e = strstr(p + 1, delim);
        if (e) {
           *e = 0;
           char *s = strdup(p + 1);
           char *d = strdup(e + strlen(delim));
           free(shortText);
           free(description);
           shortText = s;
           description = d;
           EpgBugFixStat(1, ChannelID());
           }
        }
     }

  // Some channels put the Description into the ShortText (preceded
  // by a blank) if there is no actual ShortText and the Description
  // is short enough:
  //
  // Title
  //  Description
  //
  if (shortText && !description) {
     if (*shortText == ' ') {
        memmove(shortText, shortText + 1, strlen(shortText));
        description = shortText;
        shortText = NULL;
        EpgBugFixStat(2, ChannelID());
        }
     }

  // Sometimes they repeat the Title in the ShortText:
  //
  // Title
  // Title
  //
  if (shortText && strcmp(title, shortText) == 0) {
     free(shortText);
     shortText = NULL;
     EpgBugFixStat(3, ChannelID());
     }

  // Some channels put the ShortText between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "ShortText"[.]
  //
  if (shortText && *shortText == '"') {
     int l = strlen(shortText);
     if (l > 2 && (shortText[l - 1] == '"' || (shortText[l - 1] == '.' && shortText[l - 2] == '"'))) {
        memmove(shortText, shortText + 1, l);
        char *p = strrchr(shortText, '"');
        if (p)
           *p = 0;
        EpgBugFixStat(4, ChannelID());
        }
     }

  if (Setup.EPGBugfixLevel <= 1)
     goto Final;

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  title = compactspace(title);
  shortText = compactspace(shortText);
  description = compactspace(description);

#define MAX_USEFUL_EPISODE_LENGTH 40
  // Some channels put a whole lot of information in the ShortText and leave
  // the Description totally empty. So if the ShortText length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
  // instead:
  if (!isempty(shortText) && isempty(description)) {
     if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
        free(description);
        description = shortText;
        shortText = NULL;
        EpgBugFixStat(6, ChannelID());
        }
     }

  // Some channels put the same information into ShortText and Description.
  // In that case we delete one of them:
  if (shortText && description && strcmp(shortText, description) == 0) {
     if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
        free(shortText);
        shortText = NULL;
        }
     else {
        free(description);
        description = NULL;
        }
     EpgBugFixStat(7, ChannelID());
     }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  strreplace(title, '`', '\'');
  strreplace(shortText, '`', '\'');
  strreplace(description, '`', '\'');

  if (Setup.EPGBugfixLevel <= 2)
     goto Final;

  // The stream components have a "description" field which some channels
  // apparently have no idea of how to set correctly:
  if (components) {
     for (int i = 0; i < components->NumComponents(); i++) {
         tComponent *p = components->Component(i);
         switch (p->stream) {
           case 0x01: { // video
                if (p->description) {
                   if (strcasecmp(p->description, "Video") == 0 ||
                        strcasecmp(p->description, "Bildformat") == 0) {
                      // Yes, we know it's video - that's what the 'stream' code
                      // is for! But _which_ video is it?
                      free(p->description);
                      p->description = NULL;
                      EpgBugFixStat(8, ChannelID());
                      }
                   }
                if (!p->description) {
                   switch (p->type) {
                     case 0x01:
                     case 0x05: p->description = strdup("4:3"); break;
                     case 0x02:
                     case 0x03:
                     case 0x06:
                     case 0x07: p->description = strdup("16:9"); break;
                     case 0x04:
                     case 0x08: p->description = strdup(">16:9"); break;
                     case 0x09:
                     case 0x0D: p->description = strdup("HD 4:3"); break;
                     case 0x0A:
                     case 0x0B:
                     case 0x0E:
                     case 0x0F: p->description = strdup("HD 16:9"); break;
                     case 0x0C:
                     case 0x10: p->description = strdup("HD >16:9"); break;
                     }
                   EpgBugFixStat(9, ChannelID());
                   }
                }
                break;
           case 0x02: { // audio
                if (p->description) {
                   if (strcasecmp(p->description, "Audio") == 0) {
                      // Yes, we know it's audio - that's what the 'stream' code
                      // is for! But _which_ audio is it?
                      free(p->description);
                      p->description = NULL;
                      EpgBugFixStat(10, ChannelID());
                      }
                   }
                if (!p->description) {
                   switch (p->type) {
                     case 0x05: p->description = strdup("Dolby Digital"); break;
                     // all others will just display the language
                     }
                   EpgBugFixStat(11, ChannelID());
                   }
                }
                break;
           }
         }
     }

Final:

  // VDR can't usefully handle newline characters in the title and shortText of EPG
  // data, so let's always convert them to blanks (independent of the setting of EPGBugfixLevel):
  strreplace(title, '\n', ' ');
  strreplace(shortText, '\n', ' ');
  /* TODO adapt to UTF-8
  // Same for control characters:
  strreplace(title, '\x86', ' ');
  strreplace(title, '\x87', ' ');
  strreplace(shortText, '\x86', ' ');
  strreplace(shortText, '\x87', ' ');
  strreplace(description, '\x86', ' ');
  strreplace(description, '\x87', ' ');
  XXX*/

  // Check for some strange czech characters :)
  checkCzech( title );
  checkCzech( shortText );
  checkCzech( description );
}

// --- cSchedule -------------------------------------------------------------

cSchedule::cSchedule(tChannelID ChannelID)
{
  channelID = ChannelID;
  hasRunning = false;
  modified = 0;
  presentSeen = 0;
}

cEvent *cSchedule::AddEvent(cEvent *Event)
{
  events.Add(Event);
  Event->schedule = this;
  HashEvent(Event);
  return Event;
}

void cSchedule::DelEvent(cEvent *Event)
{
  if (Event->schedule == this) {
     if (hasRunning && Event->IsRunning())
        ClrRunningStatus();
     UnhashEvent(Event);
     events.Del(Event);
     }
}

void cSchedule::HashEvent(cEvent *Event)
{
  eventsHashID.Add(Event, Event->EventID());
  if (Event->StartTime() > 0) // 'StartTime < 0' is apparently used with NVOD channels
     eventsHashStartTime.Add(Event, Event->StartTime());
}

void cSchedule::UnhashEvent(cEvent *Event)
{
  eventsHashID.Del(Event, Event->EventID());
  if (Event->StartTime() > 0) // 'StartTime < 0' is apparently used with NVOD channels
     eventsHashStartTime.Del(Event, Event->StartTime());
}

const cEvent *cSchedule::GetPresentEvent(void) const
{
  const cEvent *pe = NULL;
  time_t now = time(NULL);
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      if (p->StartTime() <= now)
         pe = p;
      else if (p->StartTime() > now + 3600)
         break;
      if (p->SeenWithin(RUNNINGSTATUSTIMEOUT) && p->RunningStatus() >= SI::RunningStatusPausing)
         return p;
      }
  return pe;
}

const cEvent *cSchedule::GetFollowingEvent(void) const
{
  const cEvent *p = GetPresentEvent();
  if (p)
     p = events.Next(p);
  else {
     time_t now = time(NULL);
     for (p = events.First(); p; p = events.Next(p)) {
         if (p->StartTime() >= now)
            break;
         }
     }
  return p;
}

const cEvent *cSchedule::GetEvent(tEventID EventID, time_t StartTime) const
{
  // Returns the event info with the given StartTime or, if no actual StartTime
  // is given, the one with the given EventID.
  if (StartTime > 0) // 'StartTime < 0' is apparently used with NVOD channels
     return eventsHashStartTime.Get(StartTime);
  else
     return eventsHashID.Get(EventID);
}

const cEvent *cSchedule::GetEventAround(time_t Time) const
{
  const cEvent *pe = NULL;
  time_t delta = INT_MAX;
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      time_t dt = Time - p->StartTime();
      if (dt >= 0 && dt < delta && p->EndTime() >= Time) {
         delta = dt;
         pe = p;
         }
      }
  return pe;
}

void cSchedule::SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel)
{
  hasRunning = false;
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      if (p == Event) {
         if (p->RunningStatus() > SI::RunningStatusNotRunning || RunningStatus > SI::RunningStatusNotRunning) {
            p->SetRunningStatus(RunningStatus, Channel);
            break;
            }
         }
      else if (RunningStatus >= SI::RunningStatusPausing && p->StartTime() < Event->StartTime())
         p->SetRunningStatus(SI::RunningStatusNotRunning);
      if (p->RunningStatus() >= SI::RunningStatusPausing)
         hasRunning = true;
      }
}

void cSchedule::ClrRunningStatus(cChannel *Channel)
{
  if (hasRunning) {
     for (cEvent *p = events.First(); p; p = events.Next(p)) {
         if (p->RunningStatus() >= SI::RunningStatusPausing) {
            p->SetRunningStatus(SI::RunningStatusNotRunning, Channel);
            hasRunning = false;
            break;
            }
         }
     }
}

void cSchedule::ResetVersions(void)
{
  for (cEvent *p = events.First(); p; p = events.Next(p))
      p->SetVersion(0xFF);
}

void cSchedule::Sort(void)
{
  events.Sort();
  // Make sure there are no RunningStatusUndefined before the currently running event:
  if (hasRunning) {
     for (cEvent *p = events.First(); p; p = events.Next(p)) {
         if (p->RunningStatus() >= SI::RunningStatusPausing)
            break;
         p->SetRunningStatus(SI::RunningStatusNotRunning);
         }
     }
}

void cSchedule::DropOutdated(time_t SegmentStart, time_t SegmentEnd, uchar TableID, uchar Version)
{
  if (SegmentStart > 0 && SegmentEnd > 0) {
     for (cEvent *p = events.First(); p; p = events.Next(p)) {
         if (p->EndTime() > SegmentStart) {
            if (p->StartTime() < SegmentEnd) {
               // The event overlaps with the given time segment.
               if (p->TableID() > TableID || (p->TableID() == TableID && p->Version() != Version)) {
                  // The segment overwrites all events from tables with higher ids, and
                  // within the same table id all events must have the same version.
                  // We can't delete the event right here because a timer might have
                  // a pointer to it, so let's set its id and start time to 0 to have it
                  // "phased out":
                  if (hasRunning && p->IsRunning())
                     ClrRunningStatus();
                  UnhashEvent(p);
                  p->eventID = 0;
                  p->startTime = 0;
                  }
               }
            else
               break;
            }
         }
     }
}

void cSchedule::Cleanup(void)
{
  Cleanup(time(NULL));
}

void cSchedule::Cleanup(time_t Time)
{
  cEvent *Event;
  while ((Event = events.First()) != NULL) {
        if (!Event->HasTimer() && Event->EndTime() + Setup.EPGLinger * 60 + 3600 < Time) // adding one hour for safety
           DelEvent(Event);
        else
           break;
        //TB cCondWait::SleepMs(10); /* TB: sleep a bit... */
        }
}

void cSchedule::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime) const
{
  cChannel *channel = Channels.GetByChannelID(channelID, true);
  if (channel) {
     fprintf(f, "%sC %s %s\n", Prefix, *channel->GetChannelID().ToString(), channel->Name());
     const cEvent *p;
     switch (DumpMode) {
       case dmAll: {
            for (p = events.First(); p; p = events.Next(p)) {
                p->Dump(f, Prefix);
#ifdef SVDRP_THREAD
                cCondWait::SleepMs(3);
#endif
                }
            }
            break;
       case dmPresent: {
            if ((p = GetPresentEvent()) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       case dmFollowing: {
            if ((p = GetFollowingEvent()) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       case dmAtTime: {
            if ((p = GetEventAround(AtTime)) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       }
     fprintf(f, "%sc\n", Prefix);
     }
}

bool cSchedule::Read(FILE *f, cSchedules *Schedules)
{
  if (Schedules) {
     cReadLine ReadLine;
     char *s;
     while ((s = ReadLine.Read(f)) != NULL) {
           if (*s == 'C') {
              s = skipspace(s + 1);
              char *p = strchr(s, ' ');
              if (p)
                 *p = 0; // strips optional channel name
              if (*s) {
                 tChannelID channelID = tChannelID::FromString(s);
                 if (channelID.Valid()) {
                    cSchedule *p = Schedules->AddSchedule(channelID);
                    if (p) {
                       if (!cEvent::Read(f, p))
                          return false;
                       p->Sort();
                       Schedules->SetModified(p);
                       }
                    }
                 else {
                    esyslog("ERROR: invalid channel ID: %s", s);
                    return false;
                    }
                 }
              }
           else {
              esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
              return false;
              }
           }
     return true;
     }
  return false;
}

// --- cSchedulesLock --------------------------------------------------------

cSchedulesLock::cSchedulesLock(bool WriteLock, int TimeoutMs)
{
  locked = cSchedules::schedules.rwlock.Lock(WriteLock, TimeoutMs);
}

cSchedulesLock::~cSchedulesLock()
{
  if (locked)
     cSchedules::schedules.rwlock.Unlock();
}

// --- cSchedules ------------------------------------------------------------

// MQ
// initialize Cleanup Thread-Variable
cCleanSchedulesThread *CleanSchedulesThread = NULL;

cSchedules cSchedules::schedules;
const char *cSchedules::epgDataFileName = NULL;
time_t cSchedules::lastCleanup = time(NULL);
time_t cSchedules::lastDump = time(NULL);
time_t cSchedules::modified = 0;

// MQ
cCleanSchedulesThread *cSchedules::CleanSchedulesThread = NULL;

const cSchedules *cSchedules::Schedules(cSchedulesLock &SchedulesLock)
{
  return SchedulesLock.Locked() ? &schedules : NULL;
}

void cSchedules::SetEpgDataFileName(const char *FileName)
{
  delete epgDataFileName;
  epgDataFileName = FileName ? strdup(FileName) : NULL;
}

void cSchedules::SetModified(cSchedule *Schedule)
{
  Schedule->SetModified();
  modified = time(NULL);
}


/* start a thread that Cleans up schedule/epg data
 * and writes it to disk
 */
void cSchedules::Cleanup(bool Force)
{
 /* Force = true
  *
  * write the epg.data NOW.If cleanup loop is running, exit it.
  */
  if (Force)
     lastDump = 0; //stops cleanup(incase thread is not active does not start cleanup loop)
                   //and makes sure epg.data is written by thread

  // delete thread if not active
  if (CleanSchedulesThread && !CleanSchedulesThread->Active())
  {
     delete CleanSchedulesThread;
     CleanSchedulesThread = NULL;
  }

  /* Clean up is done every 60 mins.
  *  Writing of epg data is done every 10 mins
  *
  *  So, the thread is started every 10mins
  */

  time_t now = time(NULL);
  if (now - lastDump > 600) {
     if(!CleanSchedulesThread)
     {
         // Start cleaningschedules thread
         CleanSchedulesThread = new cCleanSchedulesThread(&schedules, epgDataFileName, &lastCleanup, &lastDump, &modified); // lastDump = 0, => no cleanup, only epg.data dump
         CleanSchedulesThread->Start();

     }
  }

  // force = true (during shutdown)
  // wait for epg data to be written
  if (Force && CleanSchedulesThread && CleanSchedulesThread->Active())
     cCondWait::SleepMs(50);

} //end cSchedules::Cleanup()


void cSchedules::ResetVersions(void)
{
  cSchedulesLock SchedulesLock(true);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     for (cSchedule *Schedule = s->First(); Schedule; Schedule = s->Next(Schedule))
         Schedule->ResetVersions();
     }
}

bool cSchedules::ClearAll(void)
{
  cSchedulesLock SchedulesLock(true, 1000);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
         Timer->SetEvent(NULL);
     for (cSchedule *Schedule = s->First(); Schedule; Schedule = s->Next(Schedule))
         Schedule->Cleanup(INT_MAX);
     return true;
     }
  return false;
}

bool cSchedules::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime)
{
  cSchedulesLock SchedulesLock;
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     for (cSchedule *p = s->First(); p; p = s->Next(p))
         p->Dump(f, Prefix, DumpMode, AtTime);
     return true;
     }
  return false;
}

bool cSchedules::Read(FILE *f)
{
  cSchedulesLock SchedulesLock(true, 1000);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     bool OwnFile = f == NULL;
     if (OwnFile) {
        if (epgDataFileName && access(epgDataFileName, R_OK) == 0) {
           dsyslog("reading EPG data from %s", epgDataFileName);
           if ((f = fopen(epgDataFileName, "r")) == NULL) {
              LOG_ERROR;
              return false;
              }
           }
        else
           return false;
        }
     bool result = cSchedule::Read(f, s);
     if (OwnFile)
        fclose(f);
     if (result) {
        // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
        for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel))
            s->GetSchedule(Channel);
        }
     return result;
     }
  return false;
}

cSchedule *cSchedules::AddSchedule(tChannelID ChannelID)
{
  ChannelID.ClrRid();
  cSchedule *p = (cSchedule *)GetSchedule(ChannelID);
  if (!p) {
     p = new cSchedule(ChannelID);
     Add(p);
     cChannel *channel = Channels.GetByChannelID(ChannelID);
     if (channel)
        channel->schedule = p;
     }
  return p;
}

const cSchedule *cSchedules::GetSchedule(tChannelID ChannelID) const
{
  ChannelID.ClrRid();
  for (cSchedule *p = First(); p; p = Next(p)) {
      if (p->ChannelID() == ChannelID)
         return p;
      }
  return NULL;
}

const cSchedule *cSchedules::GetSchedule(const cChannel *Channel, bool AddIfMissing) const
{
  // This is not very beautiful, but it dramatically speeds up the
  // "What's on now/next?" menus.
  static cSchedule DummySchedule(tChannelID::InvalidID);
  if (!Channel->schedule)
     Channel->schedule = GetSchedule(Channel->GetChannelID());
  if (!Channel->schedule)
     Channel->schedule = &DummySchedule;
  if (Channel->schedule == &DummySchedule && AddIfMissing) {
     cSchedule *Schedule = new cSchedule(Channel->GetChannelID());
     ((cSchedules *)this)->Add(Schedule);
     Channel->schedule = Schedule;
     }
  return Channel->schedule != &DummySchedule? Channel->schedule : NULL;
}

cCleanSchedulesThread::cCleanSchedulesThread(cSchedules *xschedules, const char *xepgDataFileName, time_t *xlastCleanup, time_t *xlastDump, time_t *xmodified) :cThread("CleanSchedules")
{
#ifdef TRUE // DEBUG
	esyslog("cCleanSchedulesThread::cCleanSchedulesThread\n" );
#endif

// Initialize member variables
	schedules = xschedules;
	epgDataFileName = xepgDataFileName;
	lastCleanup = xlastCleanup;
	lastDump = xlastDump; // *lastDump == 0 indicates no cleanup only write epg.data
	modified = xmodified;
}


void cCleanSchedulesThread::Action(void) {
#ifdef TRUE // DEBUG
       dsyslog("cCleanSchedulesThread::Action\n" );
#endif
       time_t now = time(NULL);
       struct tm tm_r;
       struct tm *ptm = localtime_r(&now, &tm_r);

       cSchedulesLock *SchedulesLock;
       SchedulesLock = new cSchedulesLock(true, 1000);

       // run cleanup loop once every 60mins
       if (schedules && now - *lastCleanup > 3600) {
          // Set lastcleanup immediately to avoid repeatedly checks of main thread
          *lastCleanup = now;

          isyslog("cleaning up schedules (epg) data");

          for (cSchedule *p = schedules->First(); p && Running() && (lastDump && *lastDump); p = schedules->Next(p))
             // loop through all schedules until Cancel(by setting *lastDump  =0 from cSchedules::Cleanup()) or finished
             p->Cleanup(now);
          isyslog("schedules cleanup done");
       }
       delete SchedulesLock;

       if (ptm->tm_hour == 5)
          ReportEpgBugFixStats(true);

       /*write epg.data in this thread, this thread is called every 10 mins*/
       if (epgDataFileName /*&& now - *lastDump > 600*/) {
          cSafeFile f(epgDataFileName);

          isyslog("Writing EPG data to file");

          if (f.Open()) {
              cSchedules::Dump(f);
              f.Close();
              //printf("wrote epg date to '%s'\n", epgDataFileName);
          }
          else
              LOG_ERROR;

          *lastDump = now;
       }

       // Thread end
#ifdef TRUE // DEBUG
       dsyslog("cCleanSchedulesThread Terminated" );
#endif

}
