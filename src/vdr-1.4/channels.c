/*
 * channels.c: Channel handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: channels.c 1.53 2006/05/28 15:03:40 kls Exp $
 */

#include "channels.h"
#include <linux/dvb/frontend.h>
#include <ctype.h>
#include "debug.h"
#include "device.h"
#include "epg.h"
#include "s2reel_compat.h"
#include "timers.h"
#include <vector>

#include <fstream>	// for reading noepgchannels.conf

#define NOEPG_CHANNELSCONF "noepgchannels.conf"

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

// -- Channel Parameter Maps -------------------------------------------------

const tChannelParameterMap InversionValues[] = {
  {   0, INVERSION_OFF },
  {   1, INVERSION_ON },
  { 999, INVERSION_AUTO },
  { -1 }
  };

const tChannelParameterMap BandwidthValues[] = {
  {   6, BANDWIDTH_6_MHZ },
  {   7, BANDWIDTH_7_MHZ },
  {   8, BANDWIDTH_8_MHZ },
  { 999, BANDWIDTH_AUTO },
  { -1 }
  };

const tChannelParameterMap CoderateValues[] = {
  {   0, FEC_NONE },
  {  12, FEC_1_2 },
  {  23, FEC_2_3 },
  {  34, FEC_3_4 },
  {  45, FEC_4_5 },
  {  56, FEC_5_6 },
  {  67, FEC_6_7 },
  {  78, FEC_7_8 },
  {  89, FEC_8_9 },
  { 999, FEC_AUTO },
  { -1 }
  };

const tChannelParameterMap CoderateValuesS[] = {
  {   0, FEC_NONE },
  {  12, FEC_1_2 },
  {  23, FEC_2_3 },
  {  34, FEC_3_4 },
  {  45, FEC_4_5 },
  {  56, FEC_5_6 },
  {  67, FEC_6_7 },
  {  78, FEC_7_8 },
  {  89, FEC_8_9 },
#ifndef DVBAPI_V5
  {  13, FEC_1_3 }, //S2
  {  14, FEC_1_4 }, //S2
  {  25, FEC_2_5 }, //S2
#endif
  {  35, FEC_3_5 }, //S2
  {  910, FEC_9_10 },//S2
  { 999, FEC_AUTO },
  { -1 }
  };

const tChannelParameterMap RolloffValues[] = {
  {35, ROLLOFF_35},
  {25, ROLLOFF_25},
  {20, ROLLOFF_20},
  { -1 }
};

const tChannelParameterMap ModulationValues[] = {
  {   0, QPSK },
  {  16, QAM_16 },
  {  32, QAM_32 },
  {  64, QAM_64 },
  { 128, QAM_128 },
  { 256, QAM_256 },
  { 999, QAM_AUTO },
  { -1 }
  };

const tChannelParameterMap ModulationValuesS[] = {
  {   4, QPSK },
  {  42, QPSK_S2 }, // S2
  {   8, PSK_8},
  {  16, QAM_16 },
  {  32, QAM_32 },
  {  64, QAM_64 },
  { 128, QAM_128 },
  { 256, QAM_256 },
  { 999, QAM_AUTO },
  { -1 }
  };

const tChannelParameterMap TransmissionValues[] = {
  {   2, TRANSMISSION_MODE_2K },
  {   8, TRANSMISSION_MODE_8K },
  { 999, TRANSMISSION_MODE_AUTO },
  { -1 }
  };

const tChannelParameterMap GuardValues[] = {
  {   4, GUARD_INTERVAL_1_4 },
  {   8, GUARD_INTERVAL_1_8 },
  {  16, GUARD_INTERVAL_1_16 },
  {  32, GUARD_INTERVAL_1_32 },
  { 999, GUARD_INTERVAL_AUTO },
  { -1 }
  };

const tChannelParameterMap HierarchyValues[] = {
  {   0, HIERARCHY_NONE },
  {   1, HIERARCHY_1 },
  {   2, HIERARCHY_2 },
  {   4, HIERARCHY_4 },
  { 999, HIERARCHY_AUTO },
  { -1 }
  };

int UserIndex(int Value, const tChannelParameterMap *Map)
{
  const tChannelParameterMap *map = Map;
  while (map && map->userValue != -1) {
        if (map->userValue == Value)
           return map - Map;
        map++;
        }
  return -1;
}

int DriverIndex(int Value, const tChannelParameterMap *Map)
{
  const tChannelParameterMap *map = Map;
  while (map && map->userValue != -1) {
        if (map->driverValue == Value)
           return map - Map;
        map++;
        }
  return -1;
}

int MapToUser(int Value, const tChannelParameterMap *Map)
{
  int n = DriverIndex(Value, Map);
  if (n >= 0)
     return Map[n].userValue;
  return -1;
}

int MapToDriver(int Value, const tChannelParameterMap *Map)
{
  int n = UserIndex(Value, Map);
  if (n >= 0)
     return Map[n].driverValue;
  return -1;
}

// -- tChannelID -------------------------------------------------------------

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

// -- cChannel ---------------------------------------------------------------

cChannel::cChannel(void)
{
  name = strdup("");
  shortName = strdup("");
  provider = strdup("");
  portalName = strdup("");
  memset(&__BeginData__, 0, (char *)&__EndData__ - (char *)&__BeginData__);
  inversion    = INVERSION_AUTO;
  bandwidth    = BANDWIDTH_AUTO;
  coderateH    = FEC_AUTO;
  coderateL    = FEC_AUTO;
  modulation   = QAM_AUTO;
  rolloff      = ROLLOFF_35; // S2
  transmission = TRANSMISSION_MODE_AUTO;
  guard        = GUARD_INTERVAL_AUTO;
  hierarchy    = HIERARCHY_AUTO;
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
#if 1
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
#endif
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
  return *this;
}

int cChannel::Transponder(int Frequency, char Polarization)
{
  // some satellites have transponders at the same frequency, just with different polarization:
  switch (tolower(Polarization)) {
    case 'h': Frequency += 100000; break;
    case 'v': Frequency += 200000; break;
    case 'l': Frequency += 300000; break;
    case 'r': Frequency += 400000; break;
    }
  return Frequency;
}

int cChannel::Transponder(void) const
{
  int tf = frequency;
  while (tf > 20000)
        tf /= 1000;
  if (IsSat())
     tf = Transponder(tf, polarization);
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
     polarization = Channel->polarization;
     inversion    = Channel->inversion;
     bandwidth    = Channel->bandwidth;
     coderateH    = Channel->coderateH;
     coderateL    = Channel->coderateL;
     modulation   = Channel->modulation;
     rolloff      = Channel->rolloff;
     transmission = Channel->transmission;
     guard        = Channel->guard;
     hierarchy    = Channel->hierarchy;
     }
}

bool cChannel::SetSatTransponderData(int Source, int Frequency, char Polarization, int Srate, int CoderateH, int Modulation, int Rolloff)
{
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

  if (source != Source || frequency != Frequency || polarization != Polarization || srate != Srate ||
      coderateH != CoderateH || (modulation != Modulation && modulation!=QAM_AUTO) || rolloff != Rolloff) {
     if (Number()) {
        dsyslog("changing transponder data of channel %d from %s:%d:%c:%d:%d to %s:%d:%c:%d:%d", Number(), *cSource::ToString(source), frequency, polarization, srate, coderateH, *cSource::ToString(Source), Frequency, Polarization, Srate, CoderateH);
        modification |= CHANNELMOD_TRANSP;
        Channels.SetModified();
        }
     source = Source;
     frequency = Frequency;
     polarization = Polarization;
     srate = Srate;
     coderateH = CoderateH;
     modulation = Modulation;
     rolloff = Rolloff;
     schedule = NULL;
     }
  return true;
}

bool cChannel::SetCableTransponderData(int Source, int Frequency, int Modulation, int Srate, int CoderateH)
{
  if (source != Source || frequency != Frequency || modulation != Modulation || srate != Srate || coderateH != CoderateH) {
     if (Number()) {
        dsyslog("changing transponder data of channel %d from %s:%d:%d:%d:%d to %s:%d:%d:%d:%d", Number(), *cSource::ToString(source), frequency, modulation, srate, coderateH, *cSource::ToString(Source), Frequency, Modulation, Srate, CoderateH);
        modification |= CHANNELMOD_TRANSP;
        Channels.SetModified();
        }
     source = Source;
     frequency = Frequency;
     modulation = Modulation;
     srate = Srate;
     coderateH = CoderateH;
     schedule = NULL;
     }
  return true;
}

bool cChannel::SetTerrTransponderData(int Source, int Frequency, int Bandwidth, int Modulation, int Hierarchy, int CoderateH, int CoderateL, int Guard, int Transmission)
{
  if (source != Source || frequency != Frequency || bandwidth != Bandwidth || modulation != Modulation || hierarchy != Hierarchy || coderateH != CoderateH || coderateL != CoderateL || guard != Guard || transmission != Transmission) {
     if (Number()) {
        dsyslog("changing transponder data of channel %d from %s:%d:%d:%d:%d:%d:%d:%d:%d to %s:%d:%d:%d:%d:%d:%d:%d:%d", Number(), *cSource::ToString(source), frequency, bandwidth, modulation, hierarchy, coderateH, coderateL, guard, transmission, *cSource::ToString(Source), Frequency, Bandwidth, Modulation, Hierarchy, CoderateH, CoderateL, Guard, Transmission);
        modification |= CHANNELMOD_TRANSP;
        Channels.SetModified();
        }
     source = Source;
     frequency = Frequency;
     bandwidth = Bandwidth;
     modulation = Modulation;
     hierarchy = Hierarchy;
     coderateH = CoderateH;
     coderateL = CoderateL;
     guard = Guard;
     transmission = Transmission;
     schedule = NULL;
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

#define STRDIFF 0x01
#define VALDIFF 0x02

static int IntArraysDiffer(const int *a, const int *b, const char na[][MAXLANGCODE2] = NULL, const char nb[][MAXLANGCODE2] = NULL)
{
  int result = 0;
  for (int i = 0; a[i] || b[i]; i++) {
      if (a[i] && na && nb && strcmp(na[i], nb[i]) != 0)
         result |= STRDIFF;
      if (a[i] != b[i])
         result |= VALDIFF;
      if (!a[i] || !b[i])
         break;
      }
  return result;
}

static int IntArrayToString(char *s, const int *a, int Base = 10, const char n[][MAXLANGCODE2] = NULL)
{
  char *q = s;
  int i = 0;
  while (a[i] || i == 0) {
        q += sprintf(q, Base == 16 ? "%s%X" : "%s%d", i ? "," : "", a[i]);
        if (a[i] && n && *n[i])
           q += sprintf(q, "=%s", n[i]);
        if (!a[i])
           break;
        i++;
        }
  *q = 0;
  return q - s;
}

void cChannel::SetServiceType(int Stype)
{
  stype=Stype;
  SetPids(vpid, ppid, apids, alangs, dpids, dlangs, spids, slangs, tpid);
}

void cChannel::SetPids(int Vpid, int Ppid, int *Apids, char ALangs[][MAXLANGCODE2], int *Dpids, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid)
{
  int mod = CHANNELMOD_NONE;
  if(0x02==stype) // digital radio sound service
    Vpid = 0;     // ignore video
  if (vpid != Vpid || ppid != Ppid /*|| tpid != Tpid*/) /* TB: don't trigger a retune if tpid has changed */
     mod |= CHANNELMOD_PIDS;

  int m = IntArraysDiffer(apids, Apids, alangs, ALangs) | IntArraysDiffer(dpids, Dpids, dlangs, DLangs);
  if (Setup.DisplaySubtitles)    //TB: we don't want to retune if we don't use subtitles
      m |= IntArraysDiffer(spids, Spids, slangs, SLangs);

  if (m & STRDIFF)
     mod |= CHANNELMOD_LANGS;
  if (m & VALDIFF)
     mod |= CHANNELMOD_PIDS;
  if (mod) {
     const int BufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod', +10: paranoia
     char OldApidsBuf[BufferSize];
     char NewApidsBuf[BufferSize];
     char *q = OldApidsBuf;
     q += IntArrayToString(q, apids, 10, alangs);
     if (dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, dpids, 10, dlangs);
        }
     *q = 0;
     q = NewApidsBuf;
     q += IntArrayToString(q, Apids, 10, ALangs);
     if (Dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, Dpids, 10, DLangs);
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
     dsyslog("changing pids (spid) of channel %d from %d+%d:%s:%s:%d to %d+%d:%s:%s:%d", Number(), vpid, ppid, OldApidsBuf, OldSpidsBuf, tpid, Vpid, Ppid, NewApidsBuf, NewSpidsBuf, Tpid);

     vpid = Vpid;
     ppid = Ppid;
     for (int i = 0; i < MAXAPIDS; i++) {
         apids[i] = Apids[i];
         strn0cpy(alangs[i], ALangs[i], MAXLANGCODE2);
         }
     apids[MAXAPIDS] = 0;
     for (int i = 0; i < MAXDPIDS; i++) {
         dpids[i] = Dpids[i];
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

void cChannel::SetCaIds(const int *CaIds){
  if (caids[0] && caids[0] <= 0x00FF)
     return;
  ForceCaIds(CaIds);
}

void cChannel::ForceCaIds(const int *CaIds)
{
  if (IntArraysDiffer(caids, CaIds)) {
     bool fta_chg = (!caids[0] != !CaIds[0]); // Do we have a change between fta and encoded?
     char OldCaIdsBuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
     char NewCaIdsBuf[MAXCAIDS * 5 + 10];
     IntArrayToString(OldCaIdsBuf, caids, 16);
     IntArrayToString(NewCaIdsBuf, CaIds, 16);
     dsyslog("changing caids of channel %d from %s to %s %s", Number(), OldCaIdsBuf, NewCaIdsBuf, fta_chg?"FORCING RETUNE":"");
     for (int i = 0; i <= MAXCAIDS; i++) { // <= to copy the terminating 0
         caids[i] = CaIds[i];
         if (!CaIds[i])
            break;
         }
     modification |= CHANNELMOD_CA;
     if(fta_chg) modification |= CHANNELMOD_RETUNE; /*Not correct, but we need to force a retune for NC here (and avoid a retune on CA only)*/
     Channels.SetModified();
     }
}

void cChannel::SetCaDescriptors(int Level)
{
  if (Level > 0) {
     modification |= CHANNELMOD_CA;
     Channels.SetModified();
     if (Level > 1)
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
  dsyslog(buffer);
}

void cChannel::SetRefChannel(cChannel *RefChannel)
{
  refChannel = RefChannel;
}

static int PrintParameter(char *p, char Name, int Value)
{
  return Value >= 0 && Value != 999 ? sprintf(p, "%c%d", Name, Value) : 0;
}

cString cChannel::ParametersToString(void) const
{
  char type = **cSource::ToString(source);
  if (isdigit(type))
     type = 'S';
#define ST(s) if (strchr(s, type))
  char buffer[64];
  char *q = buffer;
  *q = 0;
  ST(" S ")  q += sprintf(q, "%c", polarization);
  ST("CST")  q += PrintParameter(q, 'I', MapToUser(inversion, InversionValues));
  ST("C T")  q += PrintParameter(q, 'C', MapToUser(coderateH, CoderateValues));
  ST(" S ")  q += PrintParameter(q, 'C', MapToUser(coderateH, CoderateValuesS));
  //if (rolloff != FE_ROLLOFF_35) // markus
  if (modulation != QPSK && modulation != QAM_AUTO)
  {
      ST(" S ")  q += PrintParameter(q, 'E', MapToUser(rolloff, RolloffValues));
  }
  ST("  T")  q += PrintParameter(q, 'D', MapToUser(coderateL, CoderateValues));
  ST("C T")  q += PrintParameter(q, 'M', MapToUser(modulation, ModulationValues));
  if (modulation != QPSK && modulation != QAM_AUTO)
  {
      ST(" S ")  q += PrintParameter(q, 'M', MapToUser(modulation, ModulationValuesS));
  }
   // ST(" S ")  q += PrintParameter(q, 'M', MapToUser(modulation, ModulationValuesS));
  ST("  T")  q += PrintParameter(q, 'B', MapToUser(bandwidth, BandwidthValues));
  ST("  T")  q += PrintParameter(q, 'T', MapToUser(transmission, TransmissionValues));
  ST("  T")  q += PrintParameter(q, 'G', MapToUser(guard, GuardValues));
  ST("  T")  q += PrintParameter(q, 'Y', MapToUser(hierarchy, HierarchyValues));

  return buffer;
}

static const char *ParseParameter(const char *s, int &Value, const tChannelParameterMap *Map)
{
  if (*++s) {
     char *p = NULL;
     errno = 0;
     int n = strtol(s, &p, 10);
     if (!errno && p != s) {
        Value = MapToDriver(n, Map);
        if (Value >= 0)
           return p;
        }
     }
  esyslog("ERROR: invalid value for parameter '%c'", *(s - 1));
  return NULL;
}

bool cChannel::StringToParameters(const char *s)
{
  while (s && *s) {
        switch (toupper(*s)) {
          case 'B': s = ParseParameter(s, bandwidth, BandwidthValues); break;
          case 'C': s = ParseParameter(s, coderateH, CoderateValuesS); break;
          case 'D': s = ParseParameter(s, coderateL, CoderateValues); break;
          case 'E': s = ParseParameter(s, rolloff, RolloffValues); break;
          case 'G': s = ParseParameter(s, guard, GuardValues); break;
          case 'H': polarization = *s++; break;
          case 'I': s = ParseParameter(s, inversion, InversionValues); break;
          case 'L': polarization = *s++; break;
          case 'M': s = ParseParameter(s, modulation, ModulationValuesS); break;
          case 'R': polarization = *s++; break;
          case 'T': s = ParseParameter(s, transmission, TransmissionValues); break;
          case 'V': polarization = *s++; break;
          case 'Y': s = ParseParameter(s, hierarchy, HierarchyValues); break;
          default: esyslog("ERROR: unknown parameter key '%c'", *s);
                   return false;
          }
        }
  return true;
}

cString cChannel::ToText(const cChannel *Channel)
{
  char FullName[strlen(Channel->name) + 1 + strlen(Channel->shortName) + 1 + strlen(Channel->provider) + 1 + 10]; // +10: paranoia
  char *q = FullName;
  q += sprintf(q, "%s", Channel->name);
  if (!isempty(Channel->shortName))
     q += sprintf(q, ",%s", Channel->shortName);
  if (!isempty(Channel->provider))
     q += sprintf(q, ";%s", Channel->provider);
  *q = 0;
  strreplace(FullName, ':', '|');
  char *buffer;
  if (Channel->groupSep) {
     if (Channel->number)
        asprintf(&buffer, ":@%d %s\n", Channel->number, FullName);
     else
        asprintf(&buffer, ":%s\n", FullName);
     }
  else {
     char vpidbuf[32];
     char *q = vpidbuf;
     q += snprintf(q, sizeof(vpidbuf), "%d", Channel->vpid);
     if (Channel->ppid && Channel->ppid != Channel->vpid)
        q += snprintf(q, sizeof(vpidbuf) - (q - vpidbuf), "+%d", Channel->ppid);
     *q = 0;
     const int BufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod', +10: paranoia
     char apidbuf[BufferSize];
     q = apidbuf;
     q += IntArrayToString(q, Channel->apids, 10, Channel->alangs);
     if (Channel->dpids[0]) {
        *q++ = ';';
        q += IntArrayToString(q, Channel->dpids, 10, Channel->dlangs);
        }
     *q = 0;
     char caidbuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
     q = caidbuf;
     q += IntArrayToString(q, Channel->caids, 16);
     *q = 0;
     asprintf(&buffer, "%s:%d:%s:%s:%d:%s:%s:%d:%s:%d:%d:%d:%d\n", FullName, Channel->frequency, *Channel->ParametersToString(), *cSource::ToString(Channel->source), Channel->srate, vpidbuf, apidbuf, Channel->tpid, caidbuf, Channel->sid, Channel->nid, Channel->tid, Channel->rid);
     }
  return cString(buffer, true);
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
     char *caidbuf = NULL;
     int fields = sscanf(s, "%a[^:]:%d :%a[^:]:%a[^:] :%d :%a[^:]:%a[^:]:%d :%a[^:]:%d :%d :%d :%d ", &namebuf, &frequency, &parambuf, &sourcebuf, &srate, &vpidbuf, &apidbuf, &tpid, &caidbuf, &sid, &nid, &tid, &rid);
     if (fields >= 9) {
        if (fields == 9) {
           // allow reading of old format
           sid = atoi(caidbuf);
           delete caidbuf;
           caidbuf = NULL;
           caids[0] = tpid;
           caids[1] = 0;
           tpid = 0;
           }
        vpid = ppid = 0;
        apids[0] = 0;
        dpids[0] = 0;
        ok = false;
        if (parambuf && sourcebuf && vpidbuf && apidbuf) {
           ok = StringToParameters(parambuf) && (source = cSource::FromString(sourcebuf)) >= 0;

           char *p = strchr(vpidbuf, '+');
           if (p)
              *p++ = 0;
           if (sscanf(vpidbuf, "%d", &vpid) != 1)
              return false;
           if (p) {
              if (sscanf(p, "%d", &ppid) != 1)
                 return false;
              }
           else
              ppid = vpid;

           char *dpidbuf = strchr(apidbuf, ';');
           if (dpidbuf)
              *dpidbuf++ = 0;
           p = apidbuf;
           char *q;
           int NumApids = 0;
           char *strtok_next;
           while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                 if (NumApids < MAXAPIDS) {
                    char *l = strchr(q, '=');
                    if (l) {
                       *l++ = 0;
                       strn0cpy(alangs[NumApids], l, MAXLANGCODE2);
                       }
                    else
                       *alangs[NumApids] = 0;
                    apids[NumApids++] = strtol(q, NULL, 10);
                    }
                 else
                    esyslog("ERROR: too many APIDs!"); // no need to set ok to 'false'
                 p = NULL;
                 }
           apids[NumApids] = 0;
           if (dpidbuf) {
              char *p = dpidbuf;
              char *q;
              int NumDpids = 0;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (NumDpids < MAXDPIDS) {
                       char *l = strchr(q, '=');
                       if (l) {
                          *l++ = 0;
                          strn0cpy(dlangs[NumDpids], l, MAXLANGCODE2);
                          }
                       else
                          *dlangs[NumDpids] = 0;
                       dpids[NumDpids++] = strtol(q, NULL, 10);
                       }
                    else
                       esyslog("ERROR: too many DPIDs!"); // no need to set ok to 'false'
                    p = NULL;
                    }
              dpids[NumDpids] = 0;
              }

           if (caidbuf) {
              char *p = caidbuf;
              char *q;
              int NumCaIds = 0;
              char *strtok_next;
              while ((q = strtok_r(p, ",", &strtok_next)) != NULL) {
                    if (NumCaIds < MAXCAIDS) {
                       caids[NumCaIds++] = strtol(q, NULL, 16) & 0xFFFF;
                       if (NumCaIds == 1 && caids[0] <= 0x00FF)
                          break;
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
        p = strchr(namebuf, ',');
        if (p) {
           *p++ = 0;
           shortName = strcpyrealloc(shortName, p);
           char *q = strchr(shortName, ',');
           if (q) {
                *q = 0;
            }
           }
        name = strcpyrealloc(name, namebuf);

        free(parambuf);
        free(sourcebuf);
        free(vpidbuf);
        free(apidbuf);
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

// -- cChannelSorter ---------------------------------------------------------

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

// -- cChannels --------------------------------------------------------------

cChannels Channels;

cChannels::cChannels(void)
{
  maxNumber = 0;
  modified = CHANNELSMOD_NONE;
}

bool cChannels::SyncNoEPGChannels(const char *FileName)
{
   std::fstream file;
   std::string strBuff;
   tChannelID ChannelID;

   // initialize list of no epg channels from channels.conf
   noEPGChannels.clear();
   for (int i=0;i<Channels.MaxNumber();i++)
   {
      noEPGChannels.push_back(true);	// Default: EPG from DVB is allowed for all channels
   }

   // load noepg list channels
   file.open(FileName, std::ios::in);
   if (file)
   {
      getline(file, strBuff);
      while (!file.eof())
      {
         // lines beginning with # are comment lines
         if (strBuff.size() && strBuff[0] != '#')
         {
            // channel ids mapped to white list begins with +
            // channel ids mapped to black list begins with -
            // missing channel ids are treated as being in white list -> will be feeded with epg data from DVB
            if (strBuff[0] == '-' || strBuff[0] == '+')
            {
               // Find channel id from channel string
               ChannelID = tChannelID::FromString(strBuff.substr(1).c_str());
               if (ChannelID == tChannelID::InvalidID)
               {
                   esyslog("noepgchannels.conf: not a valid channel id [%s]. Check your %s", strBuff.c_str(), NOEPG_CHANNELSCONF);
               } else
               {
                  // Find channel
                  cChannel *noepg_chan =  Channels.GetByChannelID(ChannelID);
                  if (noepg_chan)
                  {
                     int nChannelNumber = noepg_chan->Number();

                     // set marker of noepg channel to be evaluated in eit.c ( "-" means no EPG from DVB. "+" or not listed will do)
                     noEPGChannels[nChannelNumber - 1] = (strBuff[0] != '-');
                  }
               }
            } else
               esyslog("noepgchannels.conf: not a valid mode [%c] for channel id [%s]. Check your %s! Only \"-\" for Blacklist or \"-\" for whitelist allowed!", strBuff[0], strBuff.substr(1).c_str(), NOEPG_CHANNELSCONF);

         }
         getline(file, strBuff);
      }
   }
   file.close();

   return true;
};

bool cChannels::isEPGAllowed(tChannelID ChannelID)
{
  // Get channel with tChannelID
  cChannel *noepg_chan =  Channels.GetByChannelID(ChannelID);
  if (noepg_chan)
  {
     // Get channel number (position in channel.conf)
     int nChannelNumber = noepg_chan->Number();

     // check if there are entries in noepg channels list
     if (nChannelNumber <= noEPGChannels.size())
     {
        // Does channel belongs to black list?  --> NO EPG FROM DVB WANTED
        return noEPGChannels[nChannelNumber - 1];
     } //else if (noEPGChannels.size())
          //esyslog("noepg: Channel number(%d) exceeds MaxNumber(%d)! Channel id=%s", nChannelNumber, noEPGChannels.size(), (const char *)ChannelID.ToString());
  } else
      esyslog("noepg: Channel not found! Channel id=%s", (const char *)ChannelID.ToString());
  return true;
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
  if (cConfig<cChannel>::Load(FileName, AllowComments, MustExist)) {
     DeleteDuplicateChannels();
     ReNumber();

     // build path to noepgchannels.conf from channels.conf path
     std::string sPath=FileName; int nPos = sPath.rfind('/'); sPath.erase(nPos + 1); sPath += NOEPG_CHANNELSCONF;
     SyncNoEPGChannels(sPath.c_str());	// After reading channels.conf synchronize nóepg list

     return true;
     }
  return false;
}

bool cChannels::Reload(const char *FileName, bool AllowComments, bool MustExist)
{

  Timers.Save();

  std::vector<tChannelID> channelIDs; //TB: remember the channelIDs of the timers, they don't survive loading the channellist

  for (int i = 0; i<Timers.Count(); i++) {
      cTimer *timer = Timers.Get(i);
      if(timer->Channel())
        channelIDs.push_back(timer->Channel()->GetChannelID());
      //printf("timer: \"%s\" on channel nr %i: \"%s\"\n", timer->Event() ? timer->Event()->Title() : "(NULL)", timer->Channel() ? timer->Channel()->Number() : -1, timer->Channel() ? timer->Channel()->Name() : "(NULL)");
  }

  cChannel *channel = new cChannel;
  channel->Parse("VDR:1:H:0:27500:0:0:0:0:1:0:0:0");

  cDevice::PrimaryDevice()->SwitchChannel(channel, true);

  if (cConfig<cChannel>::Load(FileName, false, true)) {
     DeleteDuplicateChannels();
     ReNumber();
     if (Count()) {
        cDevice::PrimaryDevice()->SwitchChannel(1);
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

        // build path to noepgchannels.conf from channels.conf path
        std::string sPath=FileName; int nPos = sPath.rfind('/'); sPath.erase(nPos + 1); sPath += NOEPG_CHANNELSCONF;
        SyncNoEPGChannels(sPath.c_str());	// After reading channels.conf synchronize nóepg list

        return true;
        }
     }

  return false;

}

// currently not called
void cChannels::CheckTimers(cTimers& tmpTimers)
{
   PRINTF ("DEBUG [vdr]: Check Timers \n");
   PRINTF ("DEBUG [vdr]: Timers (tmpTimers) Count() %d  \n",tmpTimers.Count());

   for (cTimer *ti = tmpTimers.First(); ti; ti = tmpTimers.Next(ti)) {
      if (ti->HasFlags(tfActive)) {
          const cChannel *channel = ti->Channel();
          if (channel) {
             cChannel *ch = Channels.GetByServiceID(channel->Source(), channel->Transponder(), channel->Sid());
             if (ch) {
                    cTimer *t = new cTimer(ti);
                    if (t) {
                      PRINTF (" DEBUG [vdr]: ping 7 \n");
                      Timers.Add(t);
                      }
                    else
                      PRINTF (" DEBUG [vdr]: ping 8 \n");
                    }
             }
          else { //  no Channel ID found
            PRINTF (" DEBUG [vdr]: no ChannelID reactive   \n");
            //ti->ClrFlags(tfActive);
            //Timers.Add(ti);
            }
        }
        Timers.Add(ti);
     }
   PRINTF ("  DEBUG [vdr]: Check Timers end \n");
}



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

void cChannels::ReNumber( void )
{
  channelsHashSid.Clear();

  int Number = 1;
  maxNumber = 0; // when empty channels.conf
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
         if (channel && channel->Sid() == sid && channel->GetChannelID() == ChannelID)
            return channel;
         }
     if (TryWithoutRid) {
        ChannelID.ClrRid();
        for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
            cChannel *channel = (cChannel *)hobj->Object();
            if (channel && channel->Sid() == sid && channel->GetChannelID().ClrRid() == ChannelID)
               return channel;
            }
        }
     if (TryWithoutPolarization) {
        ChannelID.ClrPolarization();
        for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj)) {
            cChannel *channel = (cChannel *)hobj->Object();
            if (channel && channel->Sid() == sid && channel->GetChannelID().ClrPolarization() == ChannelID)
               return channel;
            }
        }
     }
  return NULL;
}
cChannel *cChannels::GetByTransponderID(tChannelID ChannelID)
{
  int source = ChannelID.Source();
  int nid = ChannelID.Nid(); // networkd
  int tid = ChannelID.Tid(); // transponderId

     for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
        if (channel->Tid() == tid &&  channel->Nid() == nid && channel->Source() == source)
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

void cChannels::SetModified(bool ByUser)
{
  modified = ByUser ? CHANNELSMOD_USER : !modified ? CHANNELSMOD_AUTO : modified;
}

int cChannels::Modified(void)
{
  int Result = modified;
  modified = CHANNELSMOD_NONE;
  return Result;
}

// start Balaji
cChannel *cChannels::InsBouquet(char* b_name, cChannel*before)
{

    cChannel *newBouquet = new cChannel;
    newBouquet->SetGroupSep(true);
    newBouquet->SetName(b_name,"","");
    newBouquet->SetId(0,0,0,0);

    Ins(newBouquet, before);
    return newBouquet;
}

cChannel *cChannels::AddBouquet(char* b_name, cChannel*after)
{

    cChannel *newBouquet = new cChannel;
    newBouquet->SetGroupSep(true);
    newBouquet->SetName(b_name,"","");
    newBouquet->SetId(0,0,0,0);

    Add(newBouquet, after);
    return newBouquet;
}

// Adds channel to the end of the bouquet
void cChannels::AddToBouquet(cChannel* NewChannel, cChannel* c_bouquet)
{
    int idx = GetNextGroup(c_bouquet->Index() );

    // "(auto added)" is the last bouquet
    if (idx==-1)
        Add(NewChannel,NULL);
    else
    {
        // got to the next group
        c_bouquet = Get(idx);
        Ins(NewChannel, c_bouquet);//add the channel before it
    }
    ReNumber();
}
//end Balaji

cChannel *cChannels::NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid, int Rid)
{
    PRINTF("\t\t adding new channel \033[91;m'%s'\033[0m\n", Name);
  if (Transponder) {
     dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", Name, ShortName, Provider, *cSource::ToString(Transponder->Source()), Transponder->Transponder(), Nid, Tid, Sid, Rid);
     cChannel *NewChannel = new cChannel;
     NewChannel->CopyTransponderData(Transponder);
     NewChannel->SetId(Nid, Tid, Sid, Rid);
     NewChannel->SetName(Name, ShortName, Provider);

     //started by Balaji
     //printf("Name = %s/%s \t\t vpid() = %d\n",Name, Provider, Transponder->Vpid());

     // store current channel info
     // adding new bouquet / channels should not switch the channel!
     int oldCurrentChannelNr = cDevice::CurrentChannel();
     cChannel *currChannel = Channels.GetByNumber(oldCurrentChannelNr);

     // Add to appropriate bouquet
     cChannel *ch=NULL, *c_bouquet=NULL;


     char dummy[64];
      snprintf(dummy, 63, "%s - (auto added)", tr("New Channels"));
     char *auto_added = dummy;
     char bouquetName[128];

     if (Setup.AddNewChannels != 0) // 0 : add at the end, 1: add in bouquets
         snprintf(bouquetName,127, ".. %s", strlen(Provider)>0? Provider:"Unknown Provider"); // XXX translate ???
     // if no provider, add under unknown provider
     //else
         //snprintf(bouquetName,127,auto_added);

     int size = 0;
     char *favorites = strdup(tr("Favorites"));
     // start check for "Favorites"
     ch = First();
     if ( ch== NULL || (ch && strcmp(ch->Name(), favorites) != 0)) // "Favorites is always the first entry
     {
         InsBouquet(favorites, ch);
     }
     // end check for "favorites"

     // check for "auto added" bouquet
    for (ch=First(); ch ; ch = Next(ch))
    {
        if (ch->GroupSep() && strstr(ch->Name(), "(auto added)") )
            break;
    }
    if (ch==NULL) // no auto added bouquet
    {
        PRINTF("(%s:%d) adding bouquet %s\n",__FILE__,__LINE__, auto_added);
        ch = AddBouquet(auto_added, NULL); // at the end

    }

    // if Add to "(auto added)"
    // now ch points to "(auto added)" bouquet
    if (Setup.AddNewChannels == 0)
    {
        // add channel at the end of the bouquet
        AddToBouquet(NewChannel, ch);
        return NewChannel;
    }


    // from here on, AutoAddedChannels != 0
    // find the correct place/bouquet to add the channel
     for (; ch ; ch = Next(ch) )
     {
         size++;

         // size > 1 => skip the first channel : Favorites
         // ch->GroupSep() only look at bouquets

         if (Setup.AddNewChannels !=0 && size > 1 && ch->GroupSep() )
         {
             char *c = bouquetName;
             /*
              * if (strcasestr(ch->Name(),bouquetName)) ch->SetName(c,"","");
              else if (strcasestr(bouquetName,ch->Name()))
              {
              strncpy(bouquetName,ch->Name(), 127);
              int len = strlen(ch->Name() );
              len = len>127? 127:len;
              bouquetName[ len ]=0;
              }*/

             int cmp_result = strncasecmp(ch->Name(), bouquetName,6);// first 3 chars are '.' , '.' and a ' '
             //printf(" cmp_result=%d (%s/%s)\n", cmp_result, ch->Name(), bouquetName);
             if ( cmp_result  > 0)  break;
             if (cmp_result == 0) // the 3 chars are equal, then copy the smaller one on to both ch->Name & bouquetName
             {
                 int len1 = strlen(bouquetName);
                 int len2 = strlen(ch->Name() );
                 if (len1 == len2 )
                 {
                     // add to existing bouquet
                     strncpy(bouquetName,ch->Name(), 127 );
                     bouquetName[ len2<128?len2:127 ]=0;
                     break;
                 }

                 if (len1 < len2 ) // bouquetName smaller
                     ch->SetName(c,"","");
                 else
                 {
                     strncpy(bouquetName,ch->Name(), 127 );
                     bouquetName[ len2<128?len2:127 ]=0;
                 }

                 break;
             }

         }

         // "auto added" is the last bouquet always
         //if (strstr(ch->Name(), "(auto added)")) break;
     } // end for

     if (ch==NULL) // bouquet not found
     {
         // should not reach here. as there is always the last bouquet "auto added"
         if (Setup.AddNewChannels != 0) // always true
             {
                 c_bouquet = AddBouquet(bouquetName,NULL); // add at the end
                 //AddBouquet(auto_added, NULL);
             }
        // else
         //    c_bouquet = AddBouquet(auto_added,NULL); //add "am Ende"
     }
     else
     {
         if ( Setup.AddNewChannels !=0)
         {
             if ( strcasecmp(ch->Name(), bouquetName) == 0 )
                 c_bouquet = ch;
             else // strcmp returns greater than
                 c_bouquet = InsBouquet(bouquetName,ch);
         }
         else
         // ch != NULL and AddNewChannels == 0 means "(auto added)" was found at ch
             c_bouquet = ch;
     }

    PRINTF("adding %s to %s\n Channels.BeingEdited() == %i \n", NewChannel->Name(), c_bouquet->Name(), Channels.BeingEdited() );
     AddToBouquet(NewChannel,c_bouquet);

     ReNumber();
     Channels.SetModified(false); // auto modification

     // switch to "real current channel" if it has changed
     if ( !Channels.BeingEdited() &&  currChannel && currChannel->Number() != oldCurrentChannelNr)
     {
       Channels.SwitchTo( currChannel->Number() ); // STOPS channelscan!!
     }

     if(favorites)      free(favorites);
     //End by Balaji
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
