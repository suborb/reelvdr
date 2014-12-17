/*
 * channels.c: Channel handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: channels.c 2.22 2012/04/01 09:27:08 kls Exp $
 */

#include "channels.h"
#include <ctype.h>
#include "device.h"
#include "epg.h"
#include "libsi/si.h"
#include "timers.h"
#ifdef USE_ALTERNATECHANNEL
#include "tools.h"
#endif /* ALTERNATECHANNEL */
#ifdef REELVDR
#include <vector>
#endif /*REELVDR*/

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

// --- tChannelID ------------------------------------------------------------

const tChannelID tChannelID::InvalidID;

tChannelID tChannelID::FromString(const char *s)
{
  char *sourcebuf = NULL;
  int nid;
  int tid;
  int sid;
  int rid = 0;
  int fields = sscanf(s, "%a[^-]-%d-%d-%d-%d", &sourcebuf, &nid, &tid, &sid, &rid);
  if (fields == 4 || fields == 5) {
     int source = cSource::FromString(sourcebuf);
     free(sourcebuf);
     if (source >= 0)
        return tChannelID(source, nid, tid, sid, rid);
     }
  return tChannelID::InvalidID;
}

cString tChannelID::ToString(void) const
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), rid ? "%s-%d-%d-%d-%d" : "%s-%d-%d-%d", *cSource::ToString(source), nid, tid, sid, rid);
  return buffer;
}

tChannelID &tChannelID::ClrPolarization(void)
{
  while (tid > 100000)
        tid -= 100000;
  return *this;
}

// --- cChannel --------------------------------------------------------------

cChannel::cChannel(void)
{
  name = strdup("");
  shortName = strdup("");
  provider = strdup("");
  portalName = strdup("");
  memset(&__BeginData__, 0, (char *)&__EndData__ - (char *)&__BeginData__);
  parameters = "";
  modification = CHANNELMOD_NONE;
  schedule     = NULL;
  linkChannels = NULL;
  refChannel   = NULL;
}

cChannel::cChannel(const cChannel &Channel)
{
  name = NULL;
  shortName = NULL;
  provider = NULL;
  portalName = NULL;
  schedule     = NULL;
  linkChannels = NULL;
  refChannel   = NULL;
  *this = Channel;
}

cChannel::~cChannel()
{
  delete linkChannels;
  linkChannels = NULL; // more than one channel can link to this one, so we need the following loop
  for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
      if (Channel->linkChannels) {
         for (cLinkChannel *lc = Channel->linkChannels->First(); lc; lc = Channel->linkChannels->Next(lc)) {
             if (lc->Channel() == this) {
                Channel->linkChannels->Del(lc);
                break;
                }
             }
         if (Channel->linkChannels->Count() == 0) {
            delete Channel->linkChannels;
            Channel->linkChannels = NULL;
            }
         }
      }
  free(name);
  free(shortName);
  free(provider);
  free(portalName);
}

cChannel& cChannel::operator= (const cChannel &Channel)
{
  name = strcpyrealloc(name, Channel.name);
  shortName = strcpyrealloc(shortName, Channel.shortName);
  provider = strcpyrealloc(provider, Channel.provider);
  portalName = strcpyrealloc(portalName, Channel.portalName);
  memcpy(&__BeginData__, &Channel.__BeginData__, (char *)&Channel.__EndData__ - (char *)&Channel.__BeginData__);
  parameters = Channel.parameters;
  return *this;
}

int cChannel::Transponder(int Frequency, char Polarization)
{
  // some satellites have transponders at the same frequency, just with different polarization:
  switch (toupper(Polarization)) {
    case 'H': Frequency += 100000; break;
    case 'V': Frequency += 200000; break;
    case 'L': Frequency += 300000; break;
    case 'R': Frequency += 400000; break;
    default: esyslog("ERROR: invalid value for Polarization '%c'", Polarization);
    }
  return Frequency;
}

int cChannel::Transponder(void) const
{
  int tf = frequency;
  while (tf > 20000)
        tf /= 1000;
  if (IsSat()) {
     const char *p = strpbrk(parameters, "HVLRhvlr"); // lowercase for backwards compatibility
     if (p)
        tf = Transponder(tf, *p);
     }
  return tf;
}

bool cChannel::HasTimer(void) const
{
  for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer)) {
      if (Timer->Channel() == this)
         return true;
      }
  return false;
}

int cChannel::Modification(int Mask)
{
  int Result = modification & Mask;
  modification = CHANNELMOD_NONE;
  return Result;
}

void cChannel::CopyTransponderData(const cChannel *Channel)
{
  if (Channel) {
     frequency    = Channel->frequency;
     source       = Channel->source;
     srate        = Channel->srate;
     parameters   = Channel->parameters;
     }
}

bool cChannel::SetTransponderData(int Source, int Frequency, int Srate, const char *Parameters, bool Quiet)
{
  if (strchr(Parameters, ':')) {
     esyslog("ERROR: parameter string '%s' contains ':'", Parameters);
     return false;
     }
  // Workarounds for broadcaster stupidity:
  // Some providers broadcast the transponder frequency of their channels with two different
  // values (like 12551 and 12552), so we need to allow for a little tolerance here
  if (abs(frequency - Frequency) <= 1)
     Frequency = frequency;
  // Sometimes the transponder frequency is set to 0, which is just wrong
  if (Frequency == 0)
     return false;
  // Sometimes the symbol rate is off by one
  if (abs(srate - Srate) <= 1)
     Srate = srate;

  if (source != Source || frequency != Frequency || srate != Srate || strcmp(parameters, Parameters)) {
     cString OldTransponderData = TransponderDataToString();
     source = Source;
     frequency = Frequency;
     srate = Srate;
     parameters = Parameters;
     schedule = NULL;
     if (Number() && !Quiet) {
        dsyslog("changing transponder data of channel %d from %s to %s", Number(), *OldTransponderData, *TransponderDataToString());
        modification |= CHANNELMOD_TRANSP;
        Channels.SetModified();
        }
     }
  return true;
}

void cChannel::SetId(int Nid, int Tid, int Sid, int Rid)
{
  if (nid != Nid || tid != Tid || sid != Sid || rid != Rid) {
     if (Number()) {
        dsyslog("changing id of channel %d from %d-%d-%d-%d to %d-%d-%d-%d", Number(), nid, tid, sid, rid, Nid, Tid, Sid, Rid);
        modification |= CHANNELMOD_ID;
        Channels.SetModified();
        Channels.UnhashChannel(this);
        }
     nid = Nid;
     tid = Tid;
     sid = Sid;
     rid = Rid;
     if (Number())
        Channels.HashChannel(this);
     schedule = NULL;
     }
}

void cChannel::SetName(const char *Name, const char *ShortName, const char *Provider)
{
  if (!isempty(Name)) {
     bool nn = strcmp(name, Name) != 0;
     bool ns = strcmp(shortName, ShortName) != 0;
     bool np = strcmp(provider, Provider) != 0;
     if (nn || ns || np) {
        if (Number()) {
           dsyslog("changing name of channel %d from '%s,%s;%s' to '%s,%s;%s'", Number(), name, shortName, provider, Name, ShortName, Provider);
           modification |= CHANNELMOD_NAME;
           Channels.SetModified();
           }
        if (nn)
           name = strcpyrealloc(name, Name);
        if (ns)
           shortName = strcpyrealloc(shortName, ShortName);
        if (np)
           provider = strcpyrealloc(provider, Provider);
        }
     }
}

void cChannel::SetPortalName(const char *PortalName)
{
  if (!isempty(PortalName) && strcmp(portalName, PortalName) != 0) {
     if (Number()) {
        dsyslog("changing portal name of channel %d from '%s' to '%s'", Number(), portalName, PortalName);
        modification |= CHANNELMOD_NAME;
        Channels.SetModified();
        }
     portalName = strcpyrealloc(portalName, PortalName);
     }
}

#ifdef USE_ALTERNATECHANNEL
void cChannel::SetAlternativeChannelID(const char *AlternativeChannelID)
{
  if (!isempty(AlternativeChannelID))
    alternativeChannelID = tChannelID::FromString(AlternativeChannelID);
}
#endif /* ALTERNATECHANNEL */

#define STRDIFF 0x01
#define VALDIFF 0x02

#ifdef REELVDR
static int IntArraysDiffer(const int *a, const int *b, const char na[][MAXLANGCODE2] = NULL, const char nb[][MAXLANGCODE2] = NULL, const int *at = NULL, const int *bt = NULL)
#else
static int IntArraysDiffer(const int *a, const int *b, const char na[][MAXLANGCODE2] = NULL, const char nb[][MAXLANGCODE2] = NULL)
#endif
{
  int result = 0;
  for (int i = 0; a[i] || b[i]; i++) {
      if (!a[i] || !b[i]) {
         result |= VALDIFF;
         break;
         }
      if (na && nb && strcmp(na[i], nb[i]) != 0)
         result |= STRDIFF;
      if (a[i] != b[i])
         result |= VALDIFF;
#ifdef REELVDR
      // Only check type if pid is valid
      if (at && bt && at[i] != bt[i])
         result |= VALDIFF;
#endif
      }
  return result;
}

static int IntArrayToString(char *s, const int *a, int Base = 10, const char n[][MAXLANGCODE2] = NULL, const int *t = NULL)
{
  char *q = s;
  int i = 0;
  while (a[i] || i == 0) {
        q += sprintf(q, Base == 16 ? "%s%X" : "%s%d", i ? "," : "", a[i]);
        const char *Delim = "=";
        if (a[i]) {
           if (n && *n[i]) {
              q += sprintf(q, "%s%s", Delim, n[i]);
              Delim = "";
              }
           if (t && t[i])
              q += sprintf(q, "%s@%d", Delim, t[i]);
           }
        if (!a[i])
           break;
        i++;
        }
  *q = 0;
  return q - s;
}

void cChannel::SetPids(int Vpid, int Ppid, int Vtype, int *Apids, int *Atypes, char ALangs[][MAXLANGCODE2], int *Dpids, int *Dtypes, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid)
{
  int mod = CHANNELMOD_NONE;
#ifdef REELVDR
  if (vpid != Vpid || ppid != Ppid || vtype != Vtype)
#else
  if (vpid != Vpid || ppid != Ppid || vtype != Vtype || tpid != Tpid)
#endif
     mod |= CHANNELMOD_PIDS;
#ifdef REELVDR
  int m = IntArraysDiffer(apids, Apids, alangs, ALangs, atypes, Atypes) | IntArraysDiffer(dpids, Dpids, dlangs, DLangs, dtypes, Dtypes) | IntArraysDiffer(spids, Spids, slangs, SLangs);
#else
  int m = IntArraysDiffer(apids, Apids, alangs, ALangs) | IntArraysDiffer(atypes, Atypes) | IntArraysDiffer(dpids, Dpids, dlangs, DLangs) | IntArraysDiffer(dtypes, Dtypes) | IntArraysDiffer(spids, Spids, slangs, SLangs);
#endif
  if (m & STRDIFF)
     mod |= CHANNELMOD_LANGS;
  if (m & VALDIFF)
     mod |= CHANNELMOD_PIDS;
  if (mod) {
     const int BufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2 + 5) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod@type', +10: paranoia
     char OldApidsBuf[BufferSize];
     char NewApidsBuf[BufferSize];
     char *q = OldApidsBuf;
     q += IntArrayToString(q, apids, 10, alangs, atypes);
     if (dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, dpids, 10, dlangs, dtypes);
        }
     *q = 0;
     q = NewApidsBuf;
     q += IntArrayToString(q, Apids, 10, ALangs, Atypes);
     if (Dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, Dpids, 10, DLangs, Dtypes);
        }
     *q = 0;
     const int SBufferSize = MAXSPIDS * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod', +10: paranoia
     char OldSpidsBuf[SBufferSize];
     char NewSpidsBuf[SBufferSize];
     q = OldSpidsBuf;
     q += IntArrayToString(q, spids, 10, slangs);
     *q = 0;
     q = NewSpidsBuf;
     q += IntArrayToString(q, Spids, 10, SLangs);
     *q = 0;
     if (Number())
        dsyslog("changing pids of channel %d from %d+%d=%d:%s:%s:%d to %d+%d=%d:%s:%s:%d", Number(), vpid, ppid, vtype, OldApidsBuf, OldSpidsBuf, tpid, Vpid, Ppid, Vtype, NewApidsBuf, NewSpidsBuf, Tpid);
     vpid = Vpid;
     ppid = Ppid;
     vtype = Vtype;
     for (int i = 0; i < MAXAPIDS; i++) {
         apids[i] = Apids[i];
         atypes[i] = Atypes[i];
         strn0cpy(alangs[i], ALangs[i], MAXLANGCODE2);
         }
     apids[MAXAPIDS] = 0;
     for (int i = 0; i < MAXDPIDS; i++) {
         dpids[i] = Dpids[i];
         dtypes[i] = Dtypes[i];
         strn0cpy(dlangs[i], DLangs[i], MAXLANGCODE2);
         }
     dpids[MAXDPIDS] = 0;
     for (int i = 0; i < MAXSPIDS; i++) {
         spids[i] = Spids[i];
         strn0cpy(slangs[i], SLangs[i], MAXLANGCODE2);
         }
     spids[MAXSPIDS] = 0;
     tpid = Tpid;
     modification |= mod;
     Channels.SetModified();
     }
}

void cChannel::SetSubtitlingDescriptors(uchar *SubtitlingTypes, uint16_t *CompositionPageIds, uint16_t *AncillaryPageIds)
{
  if (SubtitlingTypes) {
     for (int i = 0; i < MAXSPIDS; i++)
         subtitlingTypes[i] = SubtitlingTypes[i];
     }
  if (CompositionPageIds) {
     for (int i = 0; i < MAXSPIDS; i++)
         compositionPageIds[i] = CompositionPageIds[i];
     }
  if (AncillaryPageIds) {
     for (int i = 0; i < MAXSPIDS; i++)
         ancillaryPageIds[i] = AncillaryPageIds[i];
     }
}

#ifdef USE_TTXTSUBS
void cChannel::SetTeletextSubtitlePages(tTeletextSubtitlePage pages[], int numberOfPages)
{
  int mod = CHANNELMOD_NONE;
  if (totalTtxtSubtitlePages != (fixedTtxtSubtitlePages + numberOfPages))
     mod |= CHANNELMOD_PIDS;
  totalTtxtSubtitlePages = fixedTtxtSubtitlePages;
  for (int i = 0; (i < numberOfPages) && (totalTtxtSubtitlePages < MAXTXTPAGES); i++) {
      if (teletextSubtitlePages[totalTtxtSubtitlePages].ttxtMagazine != pages[i].ttxtMagazine ||
          teletextSubtitlePages[totalTtxtSubtitlePages].ttxtPage != pages[i].ttxtPage ||
          teletextSubtitlePages[totalTtxtSubtitlePages].ttxtType != pages[i].ttxtType ||
          strcmp(teletextSubtitlePages[totalTtxtSubtitlePages].ttxtLanguage, pages[i].ttxtLanguage)) {
         mod |= CHANNELMOD_PIDS;
         teletextSubtitlePages[totalTtxtSubtitlePages] = pages[i];
         }
      totalTtxtSubtitlePages++;
      }
  modification |= mod;
  Channels.SetModified();
}

#endif /* TTXTSUBS */

void cChannel::SetCaIds(const int *CaIds)
{
  if (caids[0] && caids[0] <= CA_USER_MAX)
     return; // special values will not be overwritten

#ifdef REELVDR // seperates SetCaIds into two functions SetCaIds and ForceCaIds
  ForceCaIds(CaIds);
}

void cChannel::ForceCaIds(const int *CaIds)
{
#endif

  if (IntArraysDiffer(caids, CaIds)) {
#ifdef USE_MCLI
     bool fta_chg = ((0==caids[0]) || (CA_ENCRYPTED_MAX==caids[0])) != ((0==CaIds[0]) || (CA_ENCRYPTED_MAX==CaIds[0]));
#endif /*USE_MCLI*/
     char OldCaIdsBuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
     char NewCaIdsBuf[MAXCAIDS * 5 + 10];
     IntArrayToString(OldCaIdsBuf, caids, 16);
     IntArrayToString(NewCaIdsBuf, CaIds, 16);
     if (Number())
        dsyslog("changing caids of channel %d from %s to %s", Number(), OldCaIdsBuf, NewCaIdsBuf);
     for (int i = 0; i <= MAXCAIDS; i++) { // <= to copy the terminating 0
         caids[i] = CaIds[i];
         if (!CaIds[i])
            break;
         }
     modification |= CHANNELMOD_CA;
#ifdef USE_MCLI
     if(fta_chg) modification |= CHANNELMOD_RETUNE; // Not correct, but we need to force a retune for mcli here (and avoid it if only ca changed)
#endif /*USE_MCLI*/
     Channels.SetModified();
     }
}

void cChannel::SetCaDescriptors(int Level)
{
  if (Level > 0) {
     modification |= CHANNELMOD_CA;
     Channels.SetModified();
     if (Number() && Level > 1)
        dsyslog("changing ca descriptors of channel %d", Number());
     }
}

void cChannel::SetLinkChannels(cLinkChannels *LinkChannels)
{
  if (!linkChannels && !LinkChannels)
     return;
  if (linkChannels && LinkChannels) {
     cLinkChannel *lca = linkChannels->First();
     cLinkChannel *lcb = LinkChannels->First();
     while (lca && lcb) {
           if (lca->Channel() != lcb->Channel()) {
              lca = NULL;
              break;
              }
           lca = linkChannels->Next(lca);
           lcb = LinkChannels->Next(lcb);
           }
     if (!lca && !lcb) {
        delete LinkChannels;
        return; // linkage has not changed
        }
     }
  char buffer[((linkChannels ? linkChannels->Count() : 0) + (LinkChannels ? LinkChannels->Count() : 0)) * 6 + 256]; // 6: 5 digit channel number plus blank, 256: other texts (see below) plus reserve
  char *q = buffer;
  q += sprintf(q, "linking channel %d from", Number());
  if (linkChannels) {
     for (cLinkChannel *lc = linkChannels->First(); lc; lc = linkChannels->Next(lc)) {
         lc->Channel()->SetRefChannel(NULL);
         q += sprintf(q, " %d", lc->Channel()->Number());
         }
     delete linkChannels;
     }
  else
     q += sprintf(q, " none");
  q += sprintf(q, " to");
  linkChannels = LinkChannels;
  if (linkChannels) {
     for (cLinkChannel *lc = linkChannels->First(); lc; lc = linkChannels->Next(lc)) {
         lc->Channel()->SetRefChannel(this);
         q += sprintf(q, " %d", lc->Channel()->Number());
         //dsyslog("link %4d -> %4d: %s", Number(), lc->Channel()->Number(), lc->Channel()->Name());
         }
     }
  else
     q += sprintf(q, " none");
  if (Number())
     dsyslog("%s", buffer);
}

void cChannel::SetRefChannel(cChannel *RefChannel)
{
  refChannel = RefChannel;
}

cString cChannel::TransponderDataToString(void) const
{
  if (cSource::IsTerr(source))
     return cString::sprintf("%d:%s:%s", frequency, *parameters, *cSource::ToString(source));
  return cString::sprintf("%d:%s:%s:%d", frequency, *parameters, *cSource::ToString(source), srate);
}

cString cChannel::ToText(const cChannel *Channel)
{
  char FullName[strlen(Channel->name) + 1 + strlen(Channel->shortName) + 1 + strlen(Channel->provider) + 1 + 10]; // +10: paranoia
  char *q = FullName;
  q += sprintf(q, "%s", Channel->name);
  if (!isempty(Channel->shortName))
     q += sprintf(q, ",%s", Channel->shortName);
  else if (strchr(Channel->name, ','))
     q += sprintf(q, ",");
  if (!isempty(Channel->provider))
     q += sprintf(q, ";%s", Channel->provider);
  *q = 0;
  strreplace(FullName, ':', '|');
  cString buffer;
  if (Channel->groupSep) {
     if (Channel->number)
        buffer = cString::sprintf(":@%d %s\n", Channel->number, FullName);
     else
        buffer = cString::sprintf(":%s\n", FullName);
     }
  else {
     char vpidbuf[32];
     char *q = vpidbuf;
     q += snprintf(q, sizeof(vpidbuf), "%d", Channel->vpid);
     if (Channel->ppid && Channel->ppid != Channel->vpid)
        q += snprintf(q, sizeof(vpidbuf) - (q - vpidbuf), "+%d", Channel->ppid);
     if (Channel->vpid && Channel->vtype)
        q += snprintf(q, sizeof(vpidbuf) - (q - vpidbuf), "=%d", Channel->vtype);
     *q = 0;
     const int ABufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2 + 5) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod@type', +10: paranoia
     char apidbuf[ABufferSize];
     q = apidbuf;
     q += IntArrayToString(q, Channel->apids, 10, Channel->alangs, Channel->atypes);
     if (Channel->dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, Channel->dpids, 10, Channel->dlangs, Channel->dtypes);
        }
     *q = 0;
#ifdef USE_TTXTSUBS
     const int TBufferSize = (MAXTXTPAGES * MAXSPIDS) * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod', +10: paranoia and tpid
#else
     const int TBufferSize = MAXSPIDS * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod', +10: paranoia and tpid
#endif /* TTXTSUBS */
     char tpidbuf[TBufferSize];
     q = tpidbuf;
     q += snprintf(q, sizeof(tpidbuf), "%d", Channel->tpid);
#ifdef USE_TTXTSUBS
     if (Channel->fixedTtxtSubtitlePages > 0) {
        *q++ = '+';
        for (int i = 0; i < Channel->fixedTtxtSubtitlePages; ++i) {
            tTeletextSubtitlePage page = Channel->teletextSubtitlePages[i];
            q += snprintf(q, sizeof(tpidbuf) - (q - tpidbuf), "%d=%s", page.PageNumber(), page.ttxtLanguage);
            }
        }
#endif /* TTXTSUBS */
     if (Channel->spids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, Channel->spids, 10, Channel->slangs);
        }
     char caidbuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
     q = caidbuf;
#ifdef REELVDR
     if((Channel->caids[0] >= CA_DVB_MIN) && (Channel->caids[0] <= CA_USER_MAX))
        *q++ = 'S';
#endif
     q += IntArrayToString(q, Channel->caids, 16);
     *q = 0;
     buffer = cString::sprintf("%s:%d:%s:%s:%d:%s:%s:%s:%s:%d:%d:%d:%d\n", FullName, Channel->frequency, *Channel->parameters, *cSource::ToString(Channel->source), Channel->srate, vpidbuf, apidbuf, tpidbuf, caidbuf, Channel->sid, Channel->nid, Channel->tid, Channel->rid);
     }
  return buffer;
}

cString cChannel::ToText(void) const
{
  return ToText(this);
}

bool cChannel::Parse(const char *s)
{
  bool ok = true;
  if (*s == ':') {
     groupSep = true;
     if (*++s == '@' && *++s) {
        char *p = NULL;
        errno = 0;
        int n = strtol(s, &p, 10);
        if (!errno && p != s && n > 0) {
           number = n;
           s = p;
           }
        }
     name = strcpyrealloc(name, skipspace(s));
     strreplace(name, '|', ':');
     }
  else {
     groupSep = false;
     char *namebuf = NULL;
     char *sourcebuf = NULL;
     char *parambuf = NULL;
     char *vpidbuf = NULL;
     char *apidbuf = NULL;
     char *tpidbuf = NULL;
     char *caidbuf = NULL;
     int fields = sscanf(s, "%a[^:]:%d :%a[^:]:%a[^:] :%d :%a[^:]:%a[^:]:%a[^:]:%a[^:]:%d :%d :%d :%d ", &namebuf, &frequency, &parambuf, &sourcebuf, &srate, &vpidbuf, &apidbuf, &tpidbuf, &caidbuf, &sid, &nid, &tid, &rid);
     if (fields >= 9) {
        if (fields == 9) {
           // allow reading of old format
           sid = atoi(caidbuf);
           delete caidbuf;
           caidbuf = NULL;
           if (sscanf(tpidbuf, "%d", &tpid) != 1)
              return false;
           caids[0] = tpid;
           caids[1] = 0;
           tpid = 0;
           }
        vpid = ppid = 0;
        vtype = 0;
        apids[0] = 0;
        atypes[0] = 0;
        dpids[0] = 0;
        dtypes[0] = 0;
        spids[0] = 0;
        ok = false;
        if (parambuf && sourcebuf && vpidbuf && apidbuf) {
           parameters = parambuf;
           ok = (source = cSource::FromString(sourcebuf)) >= 0;

           char *p;
           if ((p = strchr(vpidbuf, '=')) != NULL) {
              *p++ = 0;
              if (sscanf(p, "%d", &vtype) != 1)
                 return false;
              }
           if ((p = strchr(vpidbuf, '+')) != NULL) {
              *p++ = 0;
              if (sscanf(p, "%d", &ppid) != 1)
                 return false;
              }
           if (sscanf(vpidbuf, "%d", &vpid) != 1)
              return false;
           if (!ppid)
              ppid = vpid;
           if (vpid && !vtype)
              vtype = 2; // default is MPEG-2

           char *dpidbuf = strchr(apidbuf, ';');
           if (dpidbuf)
              *dpidbuf++ = 0;
           p = apidbuf;
           char *q;
           int NumApids = 0;
           char *strtok_next;
           while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                 if (NumApids < MAXAPIDS) {
                    atypes[NumApids] = 4; // backwards compatibility
                    char *l = strchr(q, '=');
                    if (l) {
                       *l++ = 0;
                       char *t = strchr(l, '@');
                       if (t) {
                          *t++ = 0;
                          atypes[NumApids] = strtol(t, NULL, 10);
                          }
                       strn0cpy(alangs[NumApids], l, MAXLANGCODE2);
                       }
                    else
                       *alangs[NumApids] = 0;
                       if ((apids[NumApids] = strtol(q, NULL, 10)) != 0)
                          NumApids++;
                    }
                 else
                    esyslog("ERROR: too many APIDs!"); // no need to set ok to 'false'
                 p = NULL;
                 }
           apids[NumApids] = 0;
           atypes[NumApids] = 0;
           if (dpidbuf) {
              char *p = dpidbuf;
              char *q;
              int NumDpids = 0;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (NumDpids < MAXDPIDS) {
                       dtypes[NumDpids] = SI::AC3DescriptorTag; // backwards compatibility
                       char *l = strchr(q, '=');
                       if (l) {
                          *l++ = 0;
                          char *t = strchr(l, '@');
                          if (t) {
                             *t++ = 0;
                             dtypes[NumDpids] = strtol(t, NULL, 10);
                             }
                          strn0cpy(dlangs[NumDpids], l, MAXLANGCODE2);
                          }
                       else
                          *dlangs[NumDpids] = 0;
                       if ((dpids[NumDpids] = strtol(q, NULL, 10)) != 0)
                          NumDpids++;
                       }
                    else
                       esyslog("ERROR: too many DPIDs!"); // no need to set ok to 'false'
                    p = NULL;
                    }
              dpids[NumDpids] = 0;
              dtypes[NumDpids] = 0;
              }
           int NumSpids = 0;
           if ((p = strchr(tpidbuf, ';')) != NULL) {
              *p++ = 0;
              char *q;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (NumSpids < MAXSPIDS) {
                       char *l = strchr(q, '=');
                       if (l) {
                          *l++ = 0;
                          strn0cpy(slangs[NumSpids], l, MAXLANGCODE2);
                          }
                       else
                          *slangs[NumSpids] = 0;
                       spids[NumSpids++] = strtol(q, NULL, 10);
                       }
                    else
                       esyslog("ERROR: too many SPIDs!"); // no need to set ok to 'false'
                    p = NULL;
                    }
              spids[NumSpids] = 0;
              }
#ifdef USE_TTXTSUBS
           fixedTtxtSubtitlePages = 0;
           if ((p = strchr(tpidbuf, '+')) != NULL) {
              *p++ = 0;
              char *q;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (fixedTtxtSubtitlePages < MAXTXTPAGES) {
                       int page;
                       char *l = strchr(q, '=');
                       if (l)
                          *l++ = 0;
                       if (sscanf(q, "%d", &page) == 1) {
                          teletextSubtitlePages[fixedTtxtSubtitlePages] = tTeletextSubtitlePage(page);
                          if (l)
                             strn0cpy(teletextSubtitlePages[fixedTtxtSubtitlePages].ttxtLanguage, l, MAXLANGCODE2);
                          fixedTtxtSubtitlePages++;
                          }
                       else
                          esyslog("ERROR: invalid Teletext page!"); // no need to set ok to 'false'
                       }
                    else
                       esyslog("ERROR: too many Teletext pages!"); // no need to set ok to 'false'
                    p = NULL;
                    }
              totalTtxtSubtitlePages = fixedTtxtSubtitlePages;
              }
#endif /* TTXTSUBS */
           if (sscanf(tpidbuf, "%d", &tpid) != 1)
              return false;
           if (caidbuf) {
              char *p = caidbuf;
              char *q;
              int NumCaIds = 0;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (NumCaIds < MAXCAIDS) {
                       caids[NumCaIds++] = strtol(q, NULL, 16) & 0xFFFF;
#ifdef REELVDR
                       if((NumCaIds == 1) && (caids[0] <= CA_USER_MAX)) {
                          if('S'==q[0])
                             caids[0] = strtol(&q[1], NULL, 16) & 0xFFFF;
                          else if ((caids[0] >= CA_DVB_MIN) && (caids[0] <= CA_DVB_MAX)) // Old ca assignment
                             caids[0] += CA_USER_MIN;
                          break;
                       } // if
#else
                       if (NumCaIds == 1 && caids[0] <= CA_USER_MAX)
                          break;
#endif
                       }
                    else
                       esyslog("ERROR: too many CA ids!"); // no need to set ok to 'false'
                    p = NULL;
                    }
              caids[NumCaIds] = 0;
              }
           }
        strreplace(namebuf, '|', ':');

        char *p = strchr(namebuf, ';');
        if (p) {
           *p++ = 0;
           provider = strcpyrealloc(provider, p);
           }
        p = strrchr(namebuf, ','); // long name might contain a ',', so search for the rightmost one
        if (p) {
           *p++ = 0;
           shortName = strcpyrealloc(shortName, p);
           }
        name = strcpyrealloc(name, namebuf);

        free(parambuf);
        free(sourcebuf);
        free(vpidbuf);
        free(apidbuf);
        free(tpidbuf);
        free(caidbuf);
        free(namebuf);
        if (!GetChannelID().Valid()) {
           esyslog("ERROR: channel data results in invalid ID!");
           return false;
           }
        }
     else
        return false;
     }
  return ok;
}

bool cChannel::Save(FILE *f)
{
  return fprintf(f, "%s", *ToText()) > 0;
}

// --- cChannelSorter --------------------------------------------------------

class cChannelSorter : public cListObject {
public:
  cChannel *channel;
  tChannelID channelID;
  cChannelSorter(cChannel *Channel) {
    channel = Channel;
    channelID = channel->GetChannelID();
    }
  virtual int Compare(const cListObject &ListObject) const {
    cChannelSorter *cs = (cChannelSorter *)&ListObject;
    return memcmp(&channelID, &cs->channelID, sizeof(channelID));
    }
  };

// --- cChannels -------------------------------------------------------------

cChannels Channels;

cChannels::cChannels(void)
{
  maxNumber = 0;
  maxChannelNameLength = 0;
  maxShortChannelNameLength = 0;
  modified = CHANNELSMOD_NONE;
}

void cChannels::DeleteDuplicateChannels(void)
{
  cList<cChannelSorter> ChannelSorter;
  for (cChannel *channel = First(); channel; channel = Next(channel)) {
      if (!channel->GroupSep())
         ChannelSorter.Add(new cChannelSorter(channel));
      }
  ChannelSorter.Sort();
  cChannelSorter *cs = ChannelSorter.First();
  while (cs) {
        cChannelSorter *next = ChannelSorter.Next(cs);
        if (next && cs->channelID == next->channelID) {
           dsyslog("deleting duplicate channel %s", *next->channel->ToText());
           Del(next->channel);
           }
        cs = next;
        }
}

bool cChannels::Load(const char *FileName, bool AllowComments, bool MustExist)
{
#if REELVDR
  if (cConfig<cChannel>::Load(FileName, AllowComments, MustExist)||Count()) {
      //Load() returns false even if there was error in parsing only one line,
      //vdr must not "hang" due to error in parsing of just one channel's data
#else
  if (cConfig<cChannel>::Load(FileName, AllowComments, MustExist)) {
#endif
     DeleteDuplicateChannels();
     ReNumber();
     return true;
     }
  return false;
}

#ifdef REELVDR
bool cChannels::LoadWithDuplicates(const char *FileName, bool AllowComments, bool MustExist)
{
  if (cConfig<cChannel>::Load(FileName, AllowComments, MustExist)||Count()) {
     ReNumber();
     return true;
     }
  return false;
}

bool cChannels::Reload(const char *FileName, bool AllowComments, bool MustExist)
{

  Timers.Save();

  std::vector<tChannelID> channelIDs; //TB: remember the channelIDs of the timers, they don't survive loading the channellist

  cTimer *timer;
  for (timer = Timers.First(); timer; timer = Timers.Next(timer)) {
      if(timer->Channel())
        channelIDs.push_back(timer->Channel()->GetChannelID());
      //printf("timer: \"%s\" on channel nr %i: \"%s\"\n", timer->Event() ? timer->Event()->Title() : "(NULL)", timer->Channel() ? timer->Channel()->Number() : -1, timer->Channel() ? timer->Channel()->Name() : "(NULL)");
  }

  // remember current channel's ID, switch to this channel after loading the new
  // channel list, if possible
  tChannelID currChannelId;
  cChannel *currChannel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (currChannel) currChannelId = currChannel->GetChannelID();

  if (cConfig<cChannel>::Load(FileName, false, true)||Count()) {
     DeleteDuplicateChannels();
     ReNumber();
     if (Count()) {
         // update current channel, if we had one
         if (currChannel)
             currChannel = Channels.GetByChannelID(currChannelId);

         //prev current channel is present in new channel list, switch to it
         if (currChannel)
             cDevice::PrimaryDevice()->SwitchChannel(currChannel, true);
         else
            Channels.SwitchTo(1); // switch to first channel

        //CheckTimers(tmpTimers);
        if(Setup.ReelboxModeTemp == eModeStandalone)
        //    Timers.Load(); // XXX Deletes all
        for (int i = 0; i<Timers.Count(); i++) {
            cTimer *timer = Timers.Get(i);
            cChannel *chan = Channels.GetByChannelID(channelIDs.at(i));
            if (chan) {
                timer->SetChannel(chan);
            } else {
                chan = Channels.GetByChannelID(channelIDs.at(i), true);
                if (chan) {
                    timer->SetChannel(chan);
                } else if(Setup.ReelboxMode != eModeClient) { //TB: do not delete the timer in client-mode, it could have a valid channel on the "AVG-server"
                    Timers.Del(timer);
                }
            }
            //printf("timer: \"%s\" on channel nr %i: \"%s\"\n", timer->Event() ? timer->Event()->Title() : "(NULL)", timer->Channel() ? timer->Channel()->Number() : -1, timer->Channel() ? timer->Channel()->Name() : "(NULL)");
        }
#ifdef NOEPG_CHANNELSCONF
        // build path to noepgchannels.conf from channels.conf path
        std::string sPath=FileName; int nPos = sPath.rfind('/'); sPath.erase(nPos + 1); sPath += NOEPG_CHANNELSCONF;
        SyncNoEPGChannels(sPath.c_str());       // After reading channels.conf synchronize n¦epg list
#endif /* NOEPG_CHANNELSCONF */

        return true;
        }
     }

  return false;

}
#endif /*REELVDR*/

#ifdef USE_ALTERNATECHANNEL
bool cChannels::LoadAlternativeChannels(const char *FileName)
{
  FILE *fp;
  char *line;
  cReadLine ReadLine;
  cChannel *origChannel;
  tChannelID channelID;
  if ((fp = fopen(FileName,"r"))==NULL)
  {
    esyslog("Can't open Alternative Channels-File <%s>",FileName);
    return false;
  }
  while ((line = ReadLine.Read(fp)) != NULL)
  {
    if (line[0] != '#')
    {
      line=strtok(line, ";");
      if (line != NULL)
      {
        channelID = tChannelID::FromString(line);
        if (channelID == tChannelID::InvalidID)
          dsyslog("Skipping invalid channel ID <%s>",line);
        else {
          origChannel = Channels.GetByChannelID(channelID);
          if (!origChannel)
            dsyslog("Skipping unknown channel ID <%s>",line);
          else {
            line=strtok(NULL, ";");
            channelID = tChannelID::FromString(line);
            if (channelID == tChannelID::InvalidID || !Channels.GetByChannelID(channelID))
              dsyslog("Skipping invalid/unknown alternative channel ID <%s>",line);
            else
              origChannel->SetAlternativeChannelID(line);
          }
        }
      }
    }
  } while (line != NULL);
  fclose(fp);
  return true;
}
#endif /* ALTERNATECHANNEL */

void cChannels::HashChannel(cChannel *Channel)
{
  channelsHashSid.Add(Channel, Channel->Sid());
}

void cChannels::UnhashChannel(cChannel *Channel)
{
  channelsHashSid.Del(Channel, Channel->Sid());
}

int cChannels::GetNextGroup(int Idx)
{
  cChannel *channel = Get(++Idx);
  while (channel && !(channel->GroupSep() && *channel->Name()))
        channel = Get(++Idx);
  return channel ? Idx : -1;
}

int cChannels::GetPrevGroup(int Idx)
{
  cChannel *channel = Get(--Idx);
  while (channel && !(channel->GroupSep() && *channel->Name()))
        channel = Get(--Idx);
  return channel ? Idx : -1;
}

int cChannels::GetNextNormal(int Idx)
{
  cChannel *channel = Get(++Idx);
  while (channel && channel->GroupSep())
        channel = Get(++Idx);
  return channel ? Idx : -1;
}

int cChannels::GetPrevNormal(int Idx)
{
  cChannel *channel = Get(--Idx);
  while (channel && channel->GroupSep())
        channel = Get(--Idx);
  return channel ? Idx : -1;
}

void cChannels::ReNumber(void)
{
  channelsHashSid.Clear();
  maxNumber = 0;
  int Number = 1;
  for (cChannel *channel = First(); channel; channel = Next(channel)) {
      if (channel->GroupSep()) {
         if (channel->Number() > Number)
            Number = channel->Number();
         }
      else {
         HashChannel(channel);
         maxNumber = Number;
         channel->SetNumber(Number++);
         }
      }
}

cChannel *cChannels::GetByNumber(int Number, int SkipGap)
{
  cChannel *previous = NULL;
  for (cChannel *channel = First(); channel; channel = Next(channel)) {
      if (!channel->GroupSep()) {
         if (channel->Number() == Number)
            return channel;
         else if (SkipGap && channel->Number() > Number)
            return SkipGap > 0 ? channel : previous;
         previous = channel;
         }
      }
  return NULL;
}

cChannel *cChannels::GetByServiceID(int Source, int Transponder, unsigned short ServiceID)
{
  cList<cHashObject> *list = channelsHashSid.GetList(ServiceID);
  if (list) {
     for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
         cChannel *channel = (cChannel *)hobj->Object();
         if (channel->Sid() == ServiceID && channel->Source() == Source && ISTRANSPONDER(channel->Transponder(), Transponder))
            return channel;
         }
     }
  return NULL;
}

cChannel *cChannels::GetByChannelID(tChannelID ChannelID, bool TryWithoutRid, bool TryWithoutPolarization)
{
  int sid = ChannelID.Sid();
  cList<cHashObject> *list = channelsHashSid.GetList(sid);
  if (list) {
     for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
         cChannel *channel = (cChannel *)hobj->Object();
#if REELVDR
         if (channel && channel->Sid() == sid && channel->GetChannelID() == ChannelID)
#else
         if (channel->Sid() == sid && channel->GetChannelID() == ChannelID)
#endif
            return channel;
         }
     if (TryWithoutRid) {
        ChannelID.ClrRid();
        for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
            cChannel *channel = (cChannel *)hobj->Object();
#if REELVDR
            if (channel && channel->Sid() == sid && channel->GetChannelID().ClrRid() == ChannelID)
#else
            if (channel->Sid() == sid && channel->GetChannelID().ClrRid() == ChannelID)
#endif
               return channel;
            }
        }
     if (TryWithoutPolarization) {
        ChannelID.ClrPolarization();
        for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
            cChannel *channel = (cChannel *)hobj->Object();
#if REELVDR
            if (channel && channel->Sid() == sid && channel->GetChannelID().ClrPolarization() == ChannelID)
#else
            if (channel->Sid() == sid && channel->GetChannelID().ClrPolarization() == ChannelID)
#endif
               return channel;
            }
        }
     }
  return NULL;
}
cChannel *cChannels::GetByTransponderID(tChannelID ChannelID)
{
  int source = ChannelID.Source();
  int nid = ChannelID.Nid();
  int tid = ChannelID.Tid();
  for (cChannel *channel = First(); channel; channel = Next(channel)) {
      if (channel->Tid() == tid && channel->Nid() == nid && channel->Source() == source)
         return channel;
      }
  return NULL;
}

bool cChannels::HasUniqueChannelID(cChannel *NewChannel, cChannel *OldChannel)
{
  tChannelID NewChannelID = NewChannel->GetChannelID();
  for (cChannel *channel = First(); channel; channel = Next(channel)) {
      if (!channel->GroupSep() && channel != OldChannel && channel->GetChannelID() == NewChannelID)
         return false;
      }
  return true;
}

bool cChannels::SwitchTo(int Number)
{
  cChannel *channel = GetByNumber(Number);
  return channel && cDevice::PrimaryDevice()->SwitchChannel(channel, true);
}

int cChannels::MaxChannelNameLength(void)
{
  if (!maxChannelNameLength) {
     for (cChannel *channel = First(); channel; channel = Next(channel)) {
         if (!channel->GroupSep())
            maxChannelNameLength = max(Utf8StrLen(channel->Name()), maxChannelNameLength);
         }
     }
  return maxChannelNameLength;
}

int cChannels::MaxShortChannelNameLength(void)
{
  if (!maxShortChannelNameLength) {
     for (cChannel *channel = First(); channel; channel = Next(channel)) {
         if (!channel->GroupSep())
            maxShortChannelNameLength = max(Utf8StrLen(channel->ShortName(true)), maxShortChannelNameLength);
         }
     }
  return maxShortChannelNameLength;
}

void cChannels::SetModified(bool ByUser)
{
  modified = ByUser ? CHANNELSMOD_USER : !modified ? CHANNELSMOD_AUTO : modified;
  maxChannelNameLength = maxShortChannelNameLength = 0;
}

int cChannels::Modified(void)
{
  int Result = modified;
  modified = CHANNELSMOD_NONE;
  return Result;
}

cChannel *cChannels::NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid, int Rid)
{
  if (Transponder) {
     dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", Name, ShortName, Provider, *cSource::ToString(Transponder->Source()), Transponder->Transponder(), Nid, Tid, Sid, Rid);
     cChannel *NewChannel = new cChannel;
     NewChannel->CopyTransponderData(Transponder);
     NewChannel->SetId(Nid, Tid, Sid, Rid);
     NewChannel->SetName(Name, ShortName, Provider);
     Add(NewChannel);
     ReNumber();
     return NewChannel;
     }
  return NULL;
}

cString ChannelString(const cChannel *Channel, int Number)
{
  char buffer[256];
  if (Channel) {
     if (Channel->GroupSep())
        snprintf(buffer, sizeof(buffer), "%s", Channel->Name());
     else
        snprintf(buffer, sizeof(buffer), "%d%s  %s", Channel->Number(), Number ? "-" : "", Channel->Name());
     }
  else if (Number)
     snprintf(buffer, sizeof(buffer), "%d-", Number);
  else
     snprintf(buffer, sizeof(buffer), "%s", tr("*** Invalid Channel ***"));
  return buffer;
}
