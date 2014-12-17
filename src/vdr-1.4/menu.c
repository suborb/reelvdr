
/*
 * menu.c: The actual menu implementations
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: menu.c 1.446 2006/12/02 11:12:02 kls Exp $
 */

#include "menu.h"
#include <ctype.h>
#include <sys/ioctl.h> //TB: needed to determine CAM-state
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "debug.h"
#include "eitscan.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "recording.h"
#include "remote.h"
#include "sources.h"
#include "status.h"
#include "themes.h"
#include "timers.h"
#include "transfer.h"
#include "videodir.h"
#include "diseqc.h"
#include "help.h"
#include "sysconfig_vdr.h"
#include "suspend.h"
#ifdef USEMYSQL
  #include "vdrmysql.h"
#endif
#include <sys/statvfs.h>

//#include "../vdr-plugins/src/bgprocess/service.h"

struct BgProcessInfo
{
  int nProcess;
};

#include <time.h>
#include <sys/time.h>
#include <algorithm>

#define MAXWAIT4EPGINFO   3 // seconds
#define MODETIMEOUT       3 // seconds
#define DISKSPACECHEK     5 // seconds between disk space checks in the main menu
#define NEWTIMERLIMIT   120 // seconds until the start time of a new timer created from the Schedule menu,
                            // within which it will go directly into the "Edit timer" menu to allow
                            // further parameter settings

#define MAXRECORDCONTROLS (MAXDEVICES * MAXRECEIVERS)
#define MAXINSTANTRECTIME (24 * 60 - 1) // 23:59 hours
#define MAXWAITFORCAMMENU 4 // seconds to wait for the CAM menu to open
#define MINFREEDISK       300 // minimum free disk space (in MB) required to start recording
#define NODISKSPACEDELTA  300 // seconds between "Not enough disk space to start recording!" messages

#define CHNUMWIDTH  (numdigits(Channels.MaxNumber()) + 1)

// --- cMenuEditCaItem -------------------------------------------------------

/* KH: moved to menu.h
class cMenuEditCaItem : public cMenuEditIntItem {
protected:
  virtual void Set(void);
public:
  cMenuEditCaItem(const char *Name, int *Value, bool EditingBouquet);
  eOSState ProcessKey(eKeys Key);
  };
*/

/* the lite has 3 ci-slots, the avantgarde 2 */
#ifdef RBLITE
#define NUMCIS 3
#else
#define NUMCIS 2
#endif

cMenuEditCaItem::cMenuEditCaItem(const char *Name, int *Value, bool EditingBouquet)
:cMenuEditIntItem(Name, Value, 0, EditingBouquet ? NUMCIS : CA_ENCRYPTED_MAX )
{
  Set();
}

void cMenuEditCaItem::Set(void)
{
  char s[64];
  if (*value == CA_FTA)
    strcpy(s, tr("Free To Air"));
#ifdef RBLITE
  else if (*value == 3)
    sprintf(s, "%s (%s)", (cDevice::GetDevice(0))->CiHandler()->GetCamName(2) ? "Neotion" : tr("no"), tr("internal CAM"));
#endif
  else if (*value == 2) {
    if(cDevice::GetDevice(0)->CiHandler())
        sprintf(s, "%s (%s)", (cDevice::GetDevice(0))->CiHandler()->GetCamName(1) ? (cDevice::GetDevice(0))->CiHandler()->GetCamName(1) : tr("No CI at"), tr("upper slot"));
    else
        sprintf(s, "%s", tr("upper slot"));
  } else if (*value == 1) {
    if(cDevice::GetDevice(0)->CiHandler())
        sprintf(s, "%s (%s)", (cDevice::GetDevice(0))->CiHandler()->GetCamName(0) ? (cDevice::GetDevice(0))->CiHandler()->GetCamName(0) : tr("No CI at"), tr("lower slot"));
    else
        sprintf(s, "%s", tr("lower slot"));
  }

if (*value <= NUMCIS)
     SetValue(s);
  else if (*value >= CA_ENCRYPTED_MIN)
     SetValue(tr("encrypted"));
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditCaItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if ((NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight) && *value >= CA_ENCRYPTED_MIN)
        *value = CA_FTA;
     else
        return cMenuEditIntItem::ProcessKey(Key);
     Set();
     state = osContinue;
     }
  return state;
}


#if 0 //moved to menu.h
// --- cMenuEditSrcItem ------------------------------------------------------

class cMenuEditSrcItem : public cMenuEditIntItem {
private:
  const cSource *source;
protected:
  virtual void Set(void);
public:
  cMenuEditSrcItem(const char *Name, int *Value);
  eOSState ProcessKey(eKeys Key);
  };
#endif

cMenuEditSrcItem::cMenuEditSrcItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, 0)
{
  source = Sources.Get(*Value);
  Set();
}

void cMenuEditSrcItem::Set(void)
{
  if (source) {
     char *buffer = NULL;
     asprintf(&buffer, "%s - %s", *cSource::ToString(source->Code()), source->Description());
     SetValue(buffer);
     free(buffer);
     }
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditSrcItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (source && source->Prev()) {
           source = (cSource *)source->Prev();
           *value = source->Code();
           }
        }
     else if (NORMALKEY(Key) == kRight) {
        if (source) {
           if (source->Next())
              source = (cSource *)source->Next();
           }
        else
           source = Sources.First();
        if (source)
           *value = source->Code();
        }
     else
        return state; // we don't call cMenuEditIntItem::ProcessKey(Key) here since we don't accept numerical input
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditSrcEItem ------------------------------------------------------
/*
#define DBG ""

#if defined DEBUG_DISEQC
#   define DBG " DEBUG [diseqc]:  -- "
#   define DLOG(x...) dsyslog(x)
#   define DPRINT(x...) fprintf(stderr,x)
#else
# define DPRINT(x...)
# define DLOG(x...)
#endif
*/


class cMenuEditSrcEItem : public cMenuEditIntItem {
private:
  const cSource *source;
  int *Diseqc;
  int tuner; ///???
  bool HasRotor(int Tuner);
protected:
  virtual void Set(void);
public:
  cMenuEditSrcEItem(const char *Name, int *Value, int diseqc[MAXTUNERS], int Tuner);
  eOSState ProcessKey(eKeys Key);
  };

cMenuEditSrcEItem::cMenuEditSrcEItem(const char *Name, int *Value, int diseqc[MAXTUNERS], int Tuner)
:cMenuEditIntItem(Name, Value, 0)
{
  source = Sources.Get(*Value);
  DLOG (DBG " cMenuEditSrcEItem Value: %d HasGets? %s Discr  \"%s\" T: %d ", *Value, source?"YES":"NO", source->Description(), Tuner);
  Diseqc = diseqc;
  tuner = Tuner;
  Set();
}

bool cMenuEditSrcEItem::HasRotor(int Tuner)
{
  DLOG (DBG " cMenuEditSrcEItem HasRotor tuner  %d  ", Tuner );
  if (Diseqc && Tuner!=tuner) {
     bool erg =  ((Diseqc[Tuner] & (DISEQC12 | GOTOX)) && !(Diseqc[Tuner] & ROTORLNB) && cDevice::GetDevice(Tuner-1) && cDevice::GetDevice(Tuner-1)->ProvidesSource(cSource::stSat));
     DLOG (DBG " cMenuEditSrcEItem HasRotor returns %s", erg?"true":"false" );
     return erg;
     }
  else
     return false;
}

void cMenuEditSrcEItem::Set(void)
{
  DLOG (DBG " cMenuEditSrcEItem Set() ");
  if (source) {
     DLOG (DBG " Have Source ");
     char *buffer = NULL;
     asprintf(&buffer, "%s - %s", *cSource::ToString(source->Code()), source->Description());
     SetValue(buffer);
     free(buffer);
     }
  else {
     DLOG (DBG " NO Source ???");
     switch (*value) {
        case 0: {
                  char buffer[] = "Rotor - DiSEqC1.2";
                  SetValue(buffer);
                  break;
                }
        case 1: {
                  char buffer[] = "Rotor - GotoX";
                  SetValue(buffer);
                  break;
                }
        default:{
                  char *buffer = NULL;
                  asprintf(&buffer, "%s %d", tr("Rotor - shared LNB"), *value-1);
                  SetValue(buffer);
                  free(buffer);
                  break;
                }
        }
     }
}

eOSState cMenuEditSrcEItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) {
        if (source && source->Prev()) {
           source = (cSource *)source->Prev();
           *value = source->Code();
           }
        else {
           if (source) {
              source = NULL;
              *value = 0;
              }
            else if (!(*value)) {
              *value+=1;
              }
            else {
              int i;
              for (i=*value; i<=4 && !HasRotor(i); i++);
              if (i<=4)
                 *value=i+1;
              }
           }
        }
     else if (NORMALKEY(Key) == kRight) {
        if (source) {
           if (source->Next())
              source = (cSource *)source->Next();
           }
        else if (*value) {
           *value-=1;
           while (*value>=2 && !HasRotor(*value-1))
             *value-=1;
           }
        else
           source = Sources.First();
        if (source)
           *value = source->Code();
        }
     else
        return state;
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditRShItem ------------------------------------------------------

class cMenuEditRShItem : public cMenuEditIntItem {
private:
  int *Diseqc;
  bool HasRotor(int Tuner);
public:
  cMenuEditRShItem(const char *Name, int *Value, int diseqc[MAXTUNERS]);
  eOSState ProcessKey(eKeys Key);
  };

cMenuEditRShItem::cMenuEditRShItem(const char *Name, int *Value, int diseqc[MAXTUNERS])
:cMenuEditIntItem(Name, Value, 0)
{
  Diseqc = diseqc;
  while (!HasRotor(*value) && *value>0)
    *value-=1;
  while (!HasRotor(*value) && *value<MAXTUNERS)
    *value+=1;
  Set();
}

bool cMenuEditRShItem::HasRotor(int Tuner)
{
  if (Diseqc)
     return ((Diseqc[Tuner] & (DISEQC12 | GOTOX)) && !(Diseqc[Tuner] & ROTORLNB) && cDevice::GetDevice(Tuner-1) && cDevice::GetDevice(Tuner-1)->ProvidesSource(cSource::stSat));
  else
     return false;
}

eOSState cMenuEditRShItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kRight) {
        int i;
        for (i=*value+1; i<=4 && !HasRotor(i); i++);
        if (i<=4)
           *value=i;
        }
     else if (NORMALKEY(Key) == kLeft) {
        int i;
        for (i=*value-1; i && !HasRotor(i); i--);
        if (i)
           *value=i;
        }
     else
        return state;
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditMapItem ------------------------------------------------------

/* KH: moved to menu.h
class cMenuEditMapItem : public cMenuEditItem {
protected:
  int *value;
  const tChannelParameterMap *map;
  const char *zeroString;
  virtual void Set(void);
public:
  cMenuEditMapItem(const char *Name, int *Value, const tChannelParameterMap *Map, const char *ZeroString = NULL);
  virtual eOSState ProcessKey(eKeys Key);
  };
*/

cMenuEditMapItem::cMenuEditMapItem(const char *Name, int *Value, const tChannelParameterMap *Map, const char *ZeroString)
:cMenuEditItem(Name)
{
  value = Value;
  map = Map;
  zeroString = ZeroString;
  Set();
}

void cMenuEditMapItem::Set(void)
{
  int n = MapToUser(*value, map);
  if (n == 999)
     SetValue(tr("auto"));
  else if (n == 0 && zeroString)
     SetValue(zeroString);
  else if (n >= 0) {
     char buf[16];
     snprintf(buf, sizeof(buf), "%d", n);
     SetValue(buf);
     }
  else
     SetValue("???");
}

eOSState cMenuEditMapItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     int newValue = *value;
     int n = DriverIndex(*value, map);
     if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (n-- > 0)
           newValue = map[n].driverValue;
        }
     else if (NORMALKEY(Key) == kRight) {
        if (map[++n].userValue >= 0)
           newValue = map[n].driverValue;
        }
     else
        return state;
     if (newValue != *value) {
        *value = newValue;
        Set();
        }
     state = osContinue;
     }
  return state;
}

// --- cMenuEditChannel ------------------------------------------------------

class cMenuEditChannel : public cOsdMenu {
private:
  cChannel *channel;
  cChannel data;
  bool new_;
  char name[256];
  char titleBuf[128];
  void SetNew();
  void Setup(void);
public:
  cMenuEditChannel(cChannel *Channel, bool New = false);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditChannel::cMenuEditChannel(cChannel *Channel, bool New)
:cOsdMenu(tr("Edit channel"), 16)
{
  channel = Channel;
  new_ = New;
  if (strcmp(Skins.Current()->Name(), "Reel") == 0) {
     strcpy((char*)&titleBuf, "menunormalhidden$");
     strcat((char*)&titleBuf, tr("Edit channel"));
     SetTitle((const char*)&titleBuf);
  }
  if (channel) {
     data = *channel;
     if (New)
     {
        SetNew();
     }
        Setup();
     }
}

void cMenuEditChannel::SetNew()
{
    channel = NULL;
    data.nid = 0;
    data.tid = 0;
    data.rid = 0;
    data.sid = 0;
    data.apids[0] = data.apids[1] = 0;
    data.vpid = 0;
    data.tpid = 0;
    data.caids[0] = 0;
    data.ppid = 0;
    data.SetName(tr("New"), "", "");
}

void cMenuEditChannel::Setup(void)
{
  int current = Current();
  char type = **cSource::ToString(data.source);
#define ST(s) if (strchr(s, type))

  Clear();

    strn0cpy(name, data.name, sizeof(name));
    Add(new cMenuEditStrItem( tr("Name"),          name, sizeof(name), tr(FileNameChars)));
    Add(new cMenuEditSrcItem( tr("Source"),       &data.source));
    Add(new cMenuEditIntItem( tr("Frequency"),    &data.frequency));
    ST(" S ")  Add(new cMenuEditChrItem( tr("Polarization"), &data.polarization, "hvlr"));
    ST("CS ")  Add(new cMenuEditIntItem( tr("Srate"),        &data.srate));
    ST("C T")  Add(new cMenuEditMapItem( tr("CoderateH"),    &data.coderateH,    CoderateValues, tr("none")));
    ST(" S ")  Add(new cMenuEditMapItem( tr("CoderateH"),    &data.coderateH,    CoderateValuesS, tr("none")));
    ST("  T")  Add(new cMenuEditMapItem( tr("CoderateL"),    &data.coderateL,    CoderateValues, tr("none")));
    Add(new cMenuEditIntItem( tr("Vpid"),         &data.vpid,  0, 0x1FFF));
    Add(new cMenuEditIntItem( tr("Ppid"),         &data.ppid,  0, 0x1FFF));
    Add(new cMenuEditIntItem( tr("Apid1"),        &data.apids[0], 0, 0x1FFF));
    Add(new cMenuEditIntItem( tr("Apid2"),        &data.apids[1], 0, 0x1FFF));
    Add(new cMenuEditIntItem( tr("Tpid"),         &data.tpid,  0, 0x1FFF));
    Add(new cMenuEditCaItem(  "CI-Slot",           &data.caids[0], false));//XXX
    Add(new cMenuEditIntItem( tr("Sid"),          &data.sid, 1, 0xFFFF));
    ST("C T")  Add(new cMenuEditMapItem( tr("Modulation"),   &data.modulation,   ModulationValues, "QPSK"));
    ST(" S ")  Add(new cMenuEditMapItem( tr("Modulation"),   &data.modulation,   ModulationValuesS, "4"));
    ST("  T")  Add(new cMenuEditMapItem( tr("Bandwidth"),    &data.bandwidth,    BandwidthValues));
    ST("  T")  Add(new cMenuEditMapItem( tr("Transmission"), &data.transmission, TransmissionValues));
    ST("  T")  Add(new cMenuEditMapItem( tr("Guard"),        &data.guard,        GuardValues));
    ST("  T")  Add(new cMenuEditMapItem( tr("Hierarchy"),    &data.hierarchy,    HierarchyValues, tr("none")));
    ST(" S ")  Add(new cMenuEditMapItem( tr("Rolloff"),      &data.rolloff,      RolloffValues, "35"));
/*
  // Parameters for all types of sources:
  strn0cpy(name, data.name, sizeof(name));
  Add(new cMenuEditStrItem( tr("Name"),          name, sizeof(name), tr(FileNameChars)));
  Add(new cMenuEditSrcItem( tr("Source"),       &data.source));
  Add(new cMenuEditIntItem( tr("Frequency"),    &data.frequency));
  Add(new cMenuEditIntItem( tr("Vpid"),         &data.vpid,  0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Ppid"),         &data.ppid,  0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Apid1"),        &data.apids[0], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Apid2"),        &data.apids[1], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Dpid1"),        &data.dpids[0], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Dpid2"),        &data.dpids[1], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Tpid"),         &data.tpid,  0, 0x1FFF));
  Add(new cMenuEditCaItem(  tr("CA"),           &data.caids[0]));
  Add(new cMenuEditIntItem( tr("Sid"),          &data.sid, 1, 0xFFFF));
  Add(new cMenuEditIntItem( tr("Nid"),          &data.nid, 0));
  Add(new cMenuEditIntItem( tr("Tid"),          &data.tid, 0));
  Add(new cMenuEditIntItem( tr("Rid"),          &data.rid, 0));

  // Parameters for specific types of sources:
  ST(" S ")  Add(new cMenuEditChrItem( tr("Polarization"), &data.polarization, "hvlr"));
  ST("CS ")  Add(new cMenuEditIntItem( tr("Srate"),        &data.srate));
  ST("CST")  Add(new cMenuEditMapItem( tr("Inversion"),    &data.inversion,    InversionValues, tr("off")));
  ST("CST")  Add(new cMenuEditMapItem( tr("CoderateH"),    &data.coderateH,    CoderateValues, tr("none")));
  ST("  T")  Add(new cMenuEditMapItem( tr("CoderateL"),    &data.coderateL,    CoderateValues, tr("none")));
  ST("C T")  Add(new cMenuEditMapItem( tr("Modulation"),   &data.modulation,   ModulationValues, "QPSK"));
  ST("  T")  Add(new cMenuEditMapItem( tr("Bandwidth"),    &data.bandwidth,    BandwidthValues));
  ST("  T")  Add(new cMenuEditMapItem( tr("Transmission"), &data.transmission, TransmissionValues));
  ST("  T")  Add(new cMenuEditMapItem( tr("Guard"),        &data.guard,        GuardValues));
  ST("  T")  Add(new cMenuEditMapItem( tr("Hierarchy"),    &data.hierarchy,    HierarchyValues, tr("none")));
*/

  if(!new_)
  {
    SetHelp(NULL, NULL, NULL, tr("Button$New"));
  }

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuEditChannel::ProcessKey(eKeys Key)
{
  int oldSource = data.source;
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     if (Key == kOk) {
        if (Channels.HasUniqueChannelID(&data, channel)) {
           data.name = strcpyrealloc(data.name, name);
           if (channel) {
              *channel = data;
              isyslog("edited channel %d %s", channel->Number(), *data.ToText());
              state = osBack;
              }
           else {
              channel = new cChannel;
              *channel = data;
              Channels.Add(channel);
              Channels.ReNumber();
              isyslog("added channel %d %s", channel->Number(), *data.ToText());
              state = osUser1;
              }
           Channels.SetModified(true);
           }
        else {
           Skins.Message(mtError, tr("Channel settings are not unique!"));
           state = osContinue;
           }
        }
        if(Key == kBlue && !new_)
        {
            SetNew();
            Setup();
            state = osContinue;
        }
     }
  if (Key != kNone && (data.source & cSource::st_Mask) != (oldSource & cSource::st_Mask))
     Setup();
  return state;
}

// --- cMenuChannelItem ------------------------------------------------------

cMenuChannelItem::eChannelSortMode cMenuChannelItem::sortMode = csmNumber;

cMenuChannelItem::cMenuChannelItem(cChannel *Channel, eViewMode vMode)
{
  channel = Channel;
  event = NULL;
  isSet = false;
  isMarked = false;
  viewMode = vMode;
  szProgressPart[0] = '\0'; /* initialize to valid string */
  if (channel->GroupSep())
     SetSelectable(false);
  //Set();
}

int cMenuChannelItem::Compare(const cListObject &ListObject) const
{
  cMenuChannelItem *p = (cMenuChannelItem *)&ListObject;
  int r = -1;
  if (sortMode == csmProvider)
     //r = strcoll(channel->Provider(), p->channel->Provider());
     r = strcasecmp(channel->Provider(), p->channel->Provider());
  if (sortMode == csmName || r == 0)
     //r = strcoll(channel->Name(), p->channel->Name());
     r = strcasecmp(channel->Name(), p->channel->Name());
  if (sortMode == csmNumber || r == 0)
     r = channel->Number() - p->channel->Number();
  return r;
}

bool cMenuChannelItem::IsSet(void)
{
      return isSet;
}

void cMenuChannelItem::Set(void)
{
  char *buffer = NULL;
  if (!channel->GroupSep())
  {
     schedules = cSchedules::Schedules(schedulesLock);
     if (schedules && viewMode == mode_view || mode_classic)
     {
       const cSchedule *Schedule = schedules->GetSchedule(channel->GetChannelID());
       if (Schedule)
       {
          event = Schedule->GetPresentEvent();
          if (event)
          {
             char szProgress[9];
             int frac = 0;
#ifdef SLOW_FPMATH
             frac = (int)roundf( (float)(time(NULL) - event->StartTime()) / (float)(event->Duration()) * 8.0 );
             frac = min(8,max(0, frac));
#else
             frac = min(8,max(0, (int)((time(NULL) - event->StartTime())*8) / event->Duration()));
#endif
             for(int i = 0; i < frac; i++)
                szProgress[i] = '|';
             szProgress[frac]=0;
             sprintf(szProgressPart, "[%-8s]\t", szProgress);
          }
        }
     }
     if (sortMode == csmProvider)
     {
         asprintf(&buffer, "%d\t%s - %s", channel->Number(), channel->Provider(), channel->Name());
     }
     else if (viewMode == mode_classic) // full list "classic"
     {
         char tag_icon = 0;
         if (channel->Ca()) // ChannelItem() (CA)
             if (isMarked)
             {
                 tag_icon = '>'; // check mark
             }
             else
             {
                 tag_icon = 80; // encryption (key) symbol
             }
         else
             if (isMarked)
             {
                 tag_icon = '>'; // check mark
             }
             else
             {
                 tag_icon = ' ';
             }
         asprintf(&buffer, "%d\t%c\t%-.14s\t%s%-.40s", channel->Number(), tag_icon, channel->Name(),
                                     szProgressPart, event?event->Title():" ");
     }
     else if (viewMode == mode_edit)
     {
        if(isMarked)
           asprintf(&buffer, "\x87    \t%d\t%s", channel->Number(), channel->Name());
        else
           asprintf(&buffer, "\t%d\t%s", channel->Number(), channel->Name());
     }
     else if (strcmp(Skins.Current()->Name(), "Reel") == 0)
     {   // current bouquet
         char tag_icon = 0;
         if (channel->Ca())
             tag_icon = 80;
         else
             tag_icon = ' ';
         asprintf(&buffer, "%02d\t%c\t%-.14s\t%s%-.40s", channel->Number(), tag_icon, channel->Name(),
                                     szProgressPart, event?event->Title():" ");
     }
     else
     {
           //asprintf(&buffer, "%d\t\t%-.14s\t%s    %-.20s", channel->Number(), channel->Name(), szProgressPart, event?event->Title():" ");
           asprintf(&buffer, "%d\t%-.14s\t%s    %-.20s", channel->Number(), channel->Name(), szProgressPart, event?event->Title():" ");
     }
  }
  else
  {
      if (viewMode == mode_classic)
      {   // Bouquet seperator
          //asprintf(&buffer, "     %-.25s", channel->Name());
          asprintf(&buffer, "\t%-.25s", channel->Name());
      }
      else
      {
          asprintf(&buffer, "-----\t\t%-.25s\t------------------", channel->Name());
      }
  }
  event = NULL;
  isSet = true;
  SetText(buffer, false);
}

enum eViewMode { mode_view, mode_edit, mode_classic };


//-----------------------------------------------------------
//---------------cSetChannelOptionsMenu--------------------------
//-----------------------------------------------------------

class cSetChannelOptionsMenu : public cOsdMenu
{
  private:
   int onlyRadioChannels;
   int onlyDecryptedChannels;
   int onlyHDChannels;
   bool &selectionChanched;
   cSetup data;
   void Set();
   const char *channelViewModeTexts[3];

  protected:
   void Store();

  public:
    cSetChannelOptionsMenu(bool &selectionchanched, std::string title = "Select channels");
    ~cSetChannelOptionsMenu(){};

    /*override*/ eOSState ProcessKey(eKeys key);
};

cSetChannelOptionsMenu::cSetChannelOptionsMenu(bool &selectionchanched, std::string title)
:cOsdMenu(title.c_str()), onlyRadioChannels(0), onlyDecryptedChannels(0), onlyHDChannels(0), selectionChanched(selectionchanched)
{
    data = Setup;
    SetCols(20,20);
    Set();
}

eOSState cSetChannelOptionsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (Key == kOk)
    {
        Store();
        return osBack;
    }

    if(Key == kBack)
    {
        return osBack;
    }

    return state;
}

void cSetChannelOptionsMenu::Set()
{
    std::vector<std::string> options;

    channelViewModeTexts[0] = tr("channellist");
    channelViewModeTexts[1] = tr("current bouquet");
    channelViewModeTexts[2] = tr("bouquet list");


    SetTitle(tr("Channellist: set filters"));

    AddFloatingText(tr("Please select the content of the channellist"), 48);

    Add(new cOsdItem("", osUnknown, false)); // blank line

    options.push_back(tr("All channels"));
    options.push_back(tr("Only TV channels"));
    options.push_back(tr("Only radio channels"));
    options.push_back(tr("non encrypted channels"));
    //options.push_back(tr("non encrypted TV channels"));
    //options.push_back(tr("Only encrypted radio channels"));
    //options.push_back(tr("HD channels"));

    char buf[256];
    //TODO: use stringstreams
    for(uint i = 0; i < options.size(); ++i)
    {
        sprintf(buf, "%d %s", i + 1, options[i].c_str());
        Add(new cOsdItem(buf, osUnknown, true));
    }

    Add(new cOsdItem("", osUnknown, false)); // blank line
    Add(new cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"), &data.UseBouquetList, 3, channelViewModeTexts));

    SetCurrent(Get(Current()));
    Display();
}

void cSetChannelOptionsMenu::Store()
{
    //printf("--------------cSetChannelOptionsMenu::Store()-------------------\n");
    std::string option = Get(Current())->Text();

    if(option.find(tr("Only radio channels")) == 2)
    {
        data.OnlyRadioChannels = 1;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }
    else if(option.find(tr("Only TV channels")) == 2)
    {
        data.OnlyRadioChannels = 2;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }
    else if(option.find(tr("non encrypted channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 2;
        data.OnlyHDChannels = 0;
    }
    else if(option.find(tr("non encrypted TV channels")) == 2)
    {
        data.OnlyRadioChannels = 2;
        data.OnlyEncryptedChannels = 2;
        data.OnlyHDChannels = 0;
    }
    else if(option.find(tr("encrypted radio channels")) == 2)
    {
        data.OnlyRadioChannels = 1;
        data.OnlyEncryptedChannels = 1;
        data.OnlyHDChannels = 0;
    }
    else if(option.find(tr("HD channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 1;
    }
    else if(option.find(tr("All channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }

    selectionChanched = true;
    Setup = data;
    Setup.Save();
}

// --- cMenuChannels ---------------------------------------------------------

#define CHANNELNUMBERTIMEOUT 1500 //ms

class cMenuChannels : public cOsdMenu {
private:
  enum eViewMode viewMode;
  int number;
  cTimeMs numberTimer;
  void Setup(void);
  cChannel *GetChannel(int Index);
  void Propagate(void);
protected:
  eOSState Number(eKeys Key);
  eOSState Switch(void);
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  virtual void Move(int From, int To);
public:
  cMenuChannels(eViewMode mode = mode_view);
  ~cMenuChannels();
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Display(void);
  };

cMenuChannels::cMenuChannels(eViewMode mode)
:cOsdMenu(tr("Channels"), CHNUMWIDTH)
{
  number = 0;
  viewMode = mode;
  Setup();
  Channels.IncBeingEdited();
  if (strcmp(Skins.Current()->Name(), "Reel") == 0)
      SetCols(4, 2, 14, 6);
  else
      SetCols(5, 18, 6);
  DDD("viewMode: %d", viewMode);
}

cMenuChannels::~cMenuChannels()
{
  Channels.DecBeingEdited();
}

void cMenuChannels::Setup(void)
{
  cChannel *currentChannel = GetChannel(Current());
  if (!currentChannel)
     currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
  cMenuChannelItem *currentItem = NULL;
  Clear();
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
      if (!channel->GroupSep() || (cMenuChannelItem::SortMode() == cMenuChannelItem::csmNumber && *channel->Name()))
        if((channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 1))
        &&(!channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 2))
        &&(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel() || !(::Setup.OnlyEncryptedChannels == 1))
        &&(!channel->IsEncryptedRadioChannel() || !channel->IsEncryptedVideoChannel() || !(::Setup.OnlyEncryptedChannels == 2)))
        //&&(channel->IsHDChannel || !(::Setup.OnlyHDChannels == 1))
        //if(channel->IsRadioChannel())
        {
         cMenuChannelItem *item = new cMenuChannelItem(channel);
         Add(item);
         if (channel == currentChannel)
            currentItem = item;
         }
      }
  if (cMenuChannelItem::SortMode() != cMenuChannelItem::csmNumber)
     Sort();
  SetCurrent(currentItem);
  SetHelp(tr("Button$Edit"), tr("Button$New"), tr("Button$Delete"), tr("Button$Mark"));
  //Display();
}

cChannel *cMenuChannels::GetChannel(int Index)
{
  cMenuChannelItem *p = (cMenuChannelItem *)Get(Index);
  return p ? (cChannel *)p->Channel() : NULL;
}

void cMenuChannels::Propagate(void)
{
  Channels.ReNumber();
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next())
      ci->Set();
  //Display();
  Channels.SetModified(true);
}

eOSState cMenuChannels::Number(eKeys Key)
{
  if (HasSubMenu())
     return osContinue;
  if (numberTimer.TimedOut())
     number = 0;
  if (!number && Key == k0) {
     cMenuChannelItem::IncSortMode();
     Setup();
     Display();
     }
  else {
     number = number * 10 + Key - k0;
     for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next()) {
         if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number) {
            SetCurrent(ci);
            Display();
            break;
            }
         }
     numberTimer.Set(CHANNELNUMBERTIMEOUT);
     }
  return osContinue;
}

eOSState cMenuChannels::Switch(void)
{
  if (HasSubMenu())
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return cDevice::PrimaryDevice()->SwitchChannel(ch, true) ? osEnd : osContinue;
  return osEnd;
}

eOSState cMenuChannels::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return AddSubMenu(new cMenuEditChannel(ch/*, true*/));
  return osContinue;
}

eOSState cMenuChannels::New(void)
{
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cMenuEditChannel(GetChannel(Current()), true));
}

eOSState cMenuChannels::Delete(void)
{
  if (!HasSubMenu() && Count() > 0) {
     int CurrentChannelNr = cDevice::CurrentChannel();
     cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
     int Index = Current();
     cChannel *channel = GetChannel(Current());
     int DeletedChannel = channel->Number();
     // Check if there is a timer using this channel:
     if (channel->HasTimer()) {
        Skins.Message(mtError, tr("Channel is being used by a timer!"));
        return osContinue;
        }
     if (Interface->Confirm(tr("Delete channel?"))) {
        if (CurrentChannel && channel == CurrentChannel) {
           int n = Channels.GetNextNormal(CurrentChannel->Index());
           if (n < 0)
              n = Channels.GetPrevNormal(CurrentChannel->Index());
           CurrentChannel = Channels.Get(n);
           CurrentChannelNr = 0; // triggers channel switch below
           }
        Channels.Del(channel);
        cOsdMenu::Del(Index);
        Propagate();
        isyslog("channel %d deleted", DeletedChannel);
        if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
           if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
              Channels.SwitchTo(CurrentChannel->Number());
           else
              cDevice::SetCurrentChannel(CurrentChannel);
           }
        }
        Display();
     }
  return osContinue;
}

void cMenuChannels::Move(int From, int To)
{
  int CurrentChannelNr = cDevice::CurrentChannel();
  cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
  cChannel *FromChannel = GetChannel(From);
  cChannel *ToChannel = GetChannel(To);
  if (FromChannel && ToChannel) {
     int FromNumber = FromChannel->Number();
     int ToNumber = ToChannel->Number();
     Channels.Move(FromChannel, ToChannel);
     cOsdMenu::Move(From, To);
     Propagate();
     isyslog("channel %d moved to %d", FromNumber, ToNumber);
     if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
        if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
           Channels.SwitchTo(CurrentChannel->Number());
        else
           cDevice::SetCurrentChannel(CurrentChannel);
        }
     Display();
     }
}

#define LOAD_RANGE 30

void cMenuChannels::Display(void){
   int start = Current() - LOAD_RANGE;
   int end = Current() + LOAD_RANGE;
   for(int i = start; i<end; i++){
      cMenuChannelItem *p = (cMenuChannelItem *)Get(i);
      if(p) {
        if(!p->IsSet())
          p->Set();
      }
   }
   cOsdMenu::Display();
}

eOSState cMenuChannels::ProcessKey(eKeys Key)
{
  if(Key == kInfo)
  {
    //return AddSubMenu(new cSetChannelOptionsMenu);
  }

  eOSState state = cOsdMenu::ProcessKey(Key);

  static bool hadSubMenu = false;
  if(hadSubMenu && !HasSubMenu())
  {
    Clear();
    Setup();
    Display();
  }
  hadSubMenu = HasSubMenu();

  if(state == osBack && Key == kBack)
     return osEnd;

  switch (state) {
    case osUser1: {
         cChannel *channel = Channels.Last();
         if (channel) {
            Add(new cMenuChannelItem(channel), true);
            return CloseSubMenu();
            }
         }
         break;
    default:
         if (state == osUnknown) {
            switch (Key) {
              case k0 ... k9:
                            return Number(Key);
              case kOk:     return Switch();
              case kRed:    return Edit();
              case kGreen:  return New();
              case kYellow: return Delete();
              case kBlue:   if (!HasSubMenu())
                               Mark();
                            break;
              default: break;
              }
            }
    }
  return state;
}


#ifdef INTERNAL_BOUQUETS
// --- cMenuEditBouquet ------------------------------------------------------

class cMenuEditBouquet : public cOsdMenu {
private:
  cChannel *channel;
  cChannel *prevchannel;
  cChannel *&newchannel;
  cChannel data;
  char name[256];
  void Setup(void);
  int bouquetCaId;
public:
  cMenuEditBouquet(cChannel *Channel, bool New, cChannel *&newChannel);
  virtual eOSState ProcessKey(eKeys Key);
};

cMenuEditBouquet::cMenuEditBouquet(cChannel *Channel, bool New, cChannel *&newChannel)
: cOsdMenu(tr("Edit Bouquet"), 24), newchannel(newChannel)
{
  channel = Channel;
  prevchannel = Channel;
  if (channel)
    data = *channel;

  if (New) {
  SetTitle(tr("Add Bouquet"));
    channel = NULL;
    data.groupSep = true;
    data.nid = 0;
    data.tid = 0;
    data.rid = 0;
    }
  bouquetCaId = 0;

  /* TB: if all channels of the bouqet have the same CAID, show this in the menu */
  cChannel *channelE;
  int tempCaId = 0;
  bool first = 1;
  /* loop through all channels of the bouquet */
  if(!New)
  for(channelE = (cChannel*)channel->Next(); channelE && !channelE->GroupSep(); channelE = (cChannel*) channelE->Next()){
     /* remember the first CAID */
     if(first) {
        first = false;
        tempCaId = channelE->Ca();
     } else if(channelE->Ca() != tempCaId){
        /* remeber if there is one differing CAID */
        tempCaId = 0;
     }
  }

  /* we found that alle CAIDs are the same value, so we can show it */
  if(tempCaId > 0 && tempCaId < 3){
    bouquetCaId = tempCaId;
  }

  Setup();
}

void cMenuEditBouquet::Setup(void)
{
  int current = Current();

  Clear();
  strn0cpy(name, data.name, sizeof(name));
  Add(new cMenuEditStrItem( tr("Name"), name, sizeof(name), tr(FileNameChars)));
  Add(new cMenuEditCaItem( tr("CI-Slot for this Bouquet"), &bouquetCaId, true));//XXX
  Add(new cOsdItem(" ", osUnknown, false), false, NULL);
  Add(new cOsdItem(" ", osUnknown, false), false, NULL);
  Add(new cOsdItem(" ", osUnknown, false), false, NULL);
  Add(new cOsdItem(tr("Note:"), osUnknown, false), false, NULL);
  Add(new cOsdItem(tr("Select CI-Slot for current Bouquet."), osUnknown, false), false, NULL);
  Add(new cOsdItem("", osUnknown, false), false, NULL);
  SetCurrent(Get(current));
  Display();
}

eOSState cMenuEditBouquet::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
    switch (Key) {
      case kOk: {
        if (Channels.HasUniqueChannelID(&data, channel)) {
          data.name = strcpyrealloc(data.name, name);
          if (channel) {
            //printf("######### channel copy########\n");
            *channel = data;
            newchannel = channel;
            isyslog("edited bouquet %s", *data.ToText());
            state = osBack;
            }
          else {
            //printf("######### channel new########\n");
            channel = new cChannel;
            *channel = data;
            newchannel = channel;
            Channels.Ins(channel, prevchannel); //add before current bouquet
            isyslog("added bouquet %s", *data.ToText());
            state = osUser1;
            }
      if( bouquetCaId >= 0){
            cChannel *channelE;
        for(channelE = (cChannel*)channel->Next(); channelE && !channelE->GroupSep(); channelE = (cChannel*) channelE->Next()){
              int caids[2] = {bouquetCaId, 0};
          if(channelE){
        channelE->ForceCaIds((const int*)&caids);
        isyslog("editing complete bouquet: setting caid of channel %s to %i", channelE->Name(), bouquetCaId);
          }
        }
      }
          Channels.SetModified(true);
          }
        else {
          Skins.Message(mtError, tr("Channel settings are not unique!"));
          state = osContinue;
          }
        }
        break;
      default:
        state = osContinue;
      }
    }
  return state;
}

// --- cMenuBouquetItem ---------------------------------------------------------

class cMenuBouquetItem : public cOsdItem {
private:
  cChannel *bouquet;
  cSchedulesLock schedulesLock;
  const cSchedules *schedules;
  const cEvent *event;
  char szProgressPart[12];
public:
  cMenuBouquetItem(cChannel *Channel);
  cChannel *Bouquet() { return bouquet; };
  void Set(void);
  };

cMenuBouquetItem::cMenuBouquetItem(cChannel *channel)
{
  bouquet = channel;
  event = NULL;
  szProgressPart[0] = '\0'; /* initialize to valid string */
}

void cMenuBouquetItem::Set()
{
  char *buffer = NULL;
/*  cChannel *channel = (cChannel*) bouquet->Next();

  if (channel && !channel->GroupSep()) {
     schedules = cSchedules::Schedules(schedulesLock);
     if (schedules) {
    const cSchedule *Schedule = schedules->GetSchedule(channel->GetChannelID());
    if (Schedule) {
       event = Schedule->GetPresentEvent();
       if (event) {
          char szProgress[9];
          int frac = 0;
          frac = (int)roundf( (float)(time(NULL) - event->StartTime()) / (float)(event->Duration()) * 8.0 );
          frac = min(8,max(0, frac));
          for(int i = 0; i < frac; i++)
         szProgress[i] = '|';
          szProgress[frac]=0;
          sprintf(szProgressPart, "%c%-8s%c", '[', szProgress, ']');
       }
    }
        asprintf(&buffer, "%s    -    %s   %s  %-.20s", bouquet->Name(), channel->Name(), szProgressPart, event?event->Title():" ");
     }
     else
        asprintf(&buffer, "%s    -    %s", bouquet->Name(), channel->Name());
  }
  else */
     asprintf(&buffer, "%s", bouquet->Name());
//  event = NULL;
  SetText(buffer, false);
}

// --- cMenuBouquetsList ---------------------------------------------------------

//#define CHANNELNUMBERTIMEOUT 1500 //ms

class cMenuBouquetsList : public cOsdMenu {
private:
  int bouquetMarked;
  void Setup(cChannel* channel);
  void Propagate(void);
  bool FilteredBouquetIsEmpty(cChannel* startChannel);
  cChannel *GetBouquet(int Index);
  eOSState DeleteBouquet(void);
  void Options();
  //bool showHelp;
  int mode_;
  bool HadSubMenu_;
  cChannel *newChannel_;
protected:
  eOSState Switch(void);
  eOSState ViewChannels(void);
  eOSState EditBouquet(void);
  eOSState NewBouquet(void);
  void Mark(void);
  virtual void Move(int From, int To);
public:
  cMenuBouquetsList(cChannel* channel = NULL, int mode = 1);
  ~cMenuBouquetsList(){}
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Display(void);
  };

cMenuBouquetsList::cMenuBouquetsList(cChannel* channel, int mode)
:cOsdMenu("Bouquets", CHNUMWIDTH)
{
  mode_ = mode;
  HadSubMenu_ = false;
  newChannel_ = NULL;
  bouquetMarked = -1;
  Setup(channel);
}

void cMenuBouquetsList::Setup(cChannel* channel)
{
  Clear();
  cChannel *currentChannel =  channel ? channel : Channels.GetByNumber(cDevice::CurrentChannel());
  cMenuBouquetItem *currentItem = NULL;
  while(currentChannel && !currentChannel->GroupSep()) {
    if(currentChannel->Prev())
      currentChannel= (cChannel*) currentChannel->Prev();
    else
      break;
    }
  for( cChannel *channel = (cChannel*) Channels.First(); channel; channel = (cChannel*) channel->Next()) {
    //printf("##cMenuBouquetsList::Setup, channel->Name() = %s\n", channel->Name());
    if(channel->GroupSep() && !(FilteredBouquetIsEmpty(channel) &&  mode_ == 1)) {
      cMenuBouquetItem *item = new cMenuBouquetItem(channel);
      Add(item);
      if(!currentItem && item->Bouquet() == currentChannel)
        currentItem = item;
      }
    }

  if(!currentItem)
    currentItem = (cMenuBouquetItem*) First();
  SetCurrent(currentItem);
  Options();
  Display();
}

bool cMenuBouquetsList::FilteredBouquetIsEmpty(cChannel* startChannel)
{
    //printf("-----cMenuBouquetsList::FilteredBouquetIsEmpty, startChannel->Name() = %s--------\n", startChannel->Name());
    for(cChannel *channel = startChannel; channel; channel = (cChannel*) channel->Next()) {

    if(channel !=startChannel && channel->GroupSep())
    {
        return true;
    }
    if((!channel->GroupSep())
    &&(channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 1))
    &&(!channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 2))
    &&(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel() || !(::Setup.OnlyEncryptedChannels == 1))
    &&(!(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel()) || !(::Setup.OnlyEncryptedChannels==2))
    &&(channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 1))
    &&(!channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 2)))
    {
        return false;
    }
    }
    return true;
}

void cMenuBouquetsList::Options()
{
  if (mode_ == 0)
  {
    SetHelp(NULL, NULL, NULL, NULL);
  }
  else if(mode_ == 1)
  {
    SetTitle(tr("Bouquets"));
    SetHelp(tr("Channels"), NULL, NULL, tr("Button$Customize"));
  }
  else if(mode_ == 2)
  {
    //SetStatus(tr("Select bouquet with OK"));
    SetTitle(tr("Customize Bouquets"));
    SetHelp(tr("New"), tr("Move"), tr("Delete"), tr("Button$Edit"));
  }
  Display();
}

void cMenuBouquetsList::Propagate()
{
  Channels.ReNumber();
  for (cMenuBouquetItem *ci = (cMenuBouquetItem *)First(); ci; ci = (cMenuBouquetItem *)ci->Next())
     ci->Set();
  Display();
  Channels.SetModified(true);
}

cChannel *cMenuBouquetsList::GetBouquet(int Index)
{
  if(Count() <= Index) return NULL;
  cMenuBouquetItem *p = (cMenuBouquetItem *)Get(Index);
  return p ? (cChannel *)p->Bouquet() : NULL;
}

eOSState cMenuBouquetsList::Switch()
{
  if (HasSubMenu())
     return osContinue;
  cChannel *channel = GetBouquet(Current());
  while(channel && channel->GroupSep())
    channel = (cChannel*) channel->Next();
  if (channel)
     return cDevice::PrimaryDevice()->SwitchChannel(channel, true) ? osEnd : osContinue;
  return osEnd;
}

eOSState cMenuBouquetsList::ViewChannels()
{
  if (HasSubMenu())
     return osContinue;
  cChannel *channel = GetBouquet(Current());
  if(!channel) return osEnd;
  ::Setup.CurrentChannel = channel->Index();
  return osUser5;
}

eOSState cMenuBouquetsList::EditBouquet()
{
  cChannel *channel;
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  channel = GetBouquet(Current());
  if (channel)
  {
     newChannel_ = channel;
     return AddSubMenu(new cMenuEditBouquet(channel, false, newChannel_));
  }
  return osContinue;
}

eOSState cMenuBouquetsList::NewBouquet()
{
  cChannel *channel;
  if (HasSubMenu())
     return osContinue;
  channel = GetBouquet(Current());
  newChannel_ = channel;
  return AddSubMenu(new cMenuEditBouquet(channel, true, newChannel_ ));
}

eOSState cMenuBouquetsList::DeleteBouquet()
{
  if (Interface->Confirm(tr("Delete Bouquet?"))) {
    cChannel *bouquet = GetBouquet(Current());
    cChannel *next = (cChannel*)bouquet->Next();
    /* Delete all channels up to the beginning */
    /* of the next bouquet */
    while (next && !next->GroupSep()){
    cChannel *p = next;
    next = (cChannel*)next->Next();
        Channels.Del(p);
    }
    /* Delete the bouquet itself */
    Channels.Del(bouquet);
    /* Remove the OSD-item */
    cOsdMenu::Del(Current());
    Propagate();
    if(Current() < -1) SetCurrent(0);
       return osContinue;
  }
  return osContinue;
}

void cMenuBouquetsList::Mark(void)
{

  if (Count() && bouquetMarked < 0) {
     bouquetMarked = Current();
     SetStatus(tr("Up/Dn for new location - OK to move"));
     }
}

void cMenuBouquetsList::Move(int From, int To)
{
  int CurrentChannelNr = cDevice::CurrentChannel();
  cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
  cChannel *FromChannel = GetBouquet(From);
  cChannel *ToChannel = GetBouquet(To);
  if(To > From)
    for(cChannel *channel = (cChannel*) ToChannel->Next(); channel && !channel->GroupSep(); channel = (cChannel*) channel->Next())
      ToChannel = channel;

  if (FromChannel && ToChannel && FromChannel != ToChannel) {
     int FromIndex = FromChannel->Index();
     int ToIndex = ToChannel->Index();
     cChannel *NextFromChannel = NULL;
     while (FromChannel && (!FromChannel->GroupSep() || !NextFromChannel)) {
       NextFromChannel = (cChannel*) FromChannel->Next();
       Channels.Move(FromChannel, ToChannel);
       if(To > From)
         ToChannel = FromChannel;
       FromChannel = NextFromChannel;
       }

     if (From != To)
     {
        cOsdMenu::Move(From, To);
        Propagate();
        isyslog("bouquet from %d moved to %d", FromIndex, ToIndex);
     }
     if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
        isyslog("CurrentChannelNr = %d; CurrentChannel::number = %d", CurrentChannelNr, CurrentChannel->Number());
        Channels.SwitchTo(CurrentChannel->Number());
     }
   }
}

void cMenuBouquetsList::Display(void)
{
  int start = Current() - LOAD_RANGE;
  int end = Current() + LOAD_RANGE;
  for(int i = start; i<end; i++){
      cMenuBouquetItem *p = (cMenuBouquetItem *)Get(i);
      if(p && p->Bouquet()) {
        p->Set();
      }
  }
  cOsdMenu::Display();
}

eOSState cMenuBouquetsList::ProcessKey(eKeys Key)
{
  if(Key == kBack &&  (mode_ == 2)) //go back to standard mode
  {
    cChannel *currentbouquet = GetBouquet(Current());
    if(currentbouquet && FilteredBouquetIsEmpty(currentbouquet))
    {
        for(cChannel *channel = currentbouquet; channel; channel = (cChannel*) channel->Next())
        {
            if(channel->GroupSep() && !(FilteredBouquetIsEmpty(channel)))
            {
                currentbouquet = channel;
                break;
            }
        }
    }

    mode_ = 1;
    Setup(currentbouquet);
    SetStatus(NULL);
    return osContinue;
  }

  //redraw
  if(HadSubMenu_ && !HasSubMenu())
  {
    Setup(newChannel_);
  }
  HadSubMenu_ = HasSubMenu();

  eOSState state = cOsdMenu::ProcessKey(NORMALKEY(Key));

  switch (state) {
    case osUser1: { // add bouquet
         cChannel *channel = Channels.Last();
         if (channel) {
            cMenuBouquetItem *item = new cMenuBouquetItem(channel);
            item->Set();
            Add(item, true);
            if(HasSubMenu())
              return CloseSubMenu();
            }
         break;
         }
    case osBack:
         //printf("kChanOpen: %i\n", kChanOpen);
         if(mode_ == 2)
         {
            ;
         }
         else
         {
            if(::Setup.UseBouquetList == 1 && !kChanOpen)
                return ViewChannels();
            else
                return osEnd;
         }
    default:
         if (state == osUnknown) {
            switch (Key) {
              case kOk:       if (bouquetMarked >= 0)
                              {
                                 SetStatus(NULL);
                                 if (bouquetMarked != Current() )
                                   Move(bouquetMarked , Current());
                                 bouquetMarked = -1;
                              }
                              else
                                return ViewChannels();
                              break;
                default:
                    //if (showHelp) //if helpkeys were not shown,donot act on them
                    switch(Key)
                    {
                        case kRed:
                                if(mode_ == 2)
                                {
                                    return NewBouquet();
                                }
                                else if(mode_ == 1)
                                {
                                    return ViewChannels(); //back to Channels
                                }
                                break;;
                        case kGreen:
                                if(mode_ == 2)
                                {    // Move Bouquet
                                    Mark();
                                    break;
                                }
                                break;
                        case kYellow:
                                if(mode_ == 2)
                                {   // Delete Bouquet
                                    return DeleteBouquet();
                                }
                                break;
                        case kBlue:
                                if(mode_ == 2)
                                {
                                    return EditBouquet();
                                }
                                else if(mode_ == 1)
                                {
                                    mode_ = 2;
                                    Setup( GetBouquet(Current()));
                                    return osContinue;
                                }
                                break;

                        case kGreater:
                                if(mode_ == 2)
                                {
                                    newChannel_ = GetBouquet(Current());
                                    return AddSubMenu(new cMenuChannels());
                                }
                                break;
                        default: break;
                    } // switch
                    //else PRINTF("showHelp = %i\n", showHelp);
                    break;
              }
            }
    }
  return state;
}

// --- cMenuBouquets ---------------------------------------------------------

//#define CHANNELNUMBERTIMEOUT 1000 //ms

#ifdef DEBUG_TIMES
extern struct timeval menuTimeVal1;
#endif
bool kChanOpen = false;

cMenuBouquets::cMenuBouquets(int view, enum eViewMode mode)
:cOsdMenu("", CHNUMWIDTH)
 ///< view: 0 = current bouquet
 ///<       1 = bouquet list
 ///<       2 = favourites
 ///<       3 = add to favourites
 ///<       4 = classic (full list, w/o bouquets)
 //
 ///< mode: 0 = view
 ///<       1 = edit
{
#ifdef DEBUG_TIMES
  struct timeval now;
  gettimeofday(&now, NULL);
  float secs2 = ((float)((1000000 * now.tv_sec + now.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
  PRINTF("\n======================= time since kOk: secs: %f\n", secs2);
#endif
  viewMode = mode;
  view_ = view;
  move = false;
  favourite = false;
  number = 0;
  selectionChanched = false;
  //channelMarked = -1;
  titleBuf[0] = '\0'; // Initialize to empty string
  startChannel = 0;

  if (view_ == 4)
  {  // "classic" view
      DDD("classic view");
      if (strcmp(Skins.Current()->Name(), "Reel") == 0)
          //SetCols(4, 4, 3, 14, 6);
          SetCols(4, 3, 14, 6);
      else
          SetCols(4, 2, 14, 6);
  }
  else if (viewMode == mode_view)
  {  // current bouquet or bouquet list
      DDD("current bouquet");
      if (strcmp(Skins.Current()->Name(), "Reel") == 0)
          SetCols(4, 3, 14, 6);
      else
          SetCols(5, 18, 6);
  } else
  {
      DDD("else...."); // never used???
      if (strcmp(Skins.Current()->Name(), "Reel") == 0)
          SetCols(4, 4, 14, 6);
      else
          SetCols(4, 5, 18, 6);
  }

  DDD("view: %d", view_);
  if (view_ == 1)
  {
    AddSubMenu(new cMenuBouquetsList());
  }
  else if (view_ == 2) {
    favourite = true;
    SetGroup(0);
    Options();
  }
  else if (view_ == 3) {
    favourite = true;
    SetGroup(0);
    Options();
    AddFavourite(true);
  }
  else if (view_ == 4) {
    Setup();
    Display();
    Options();
  }
  else {
    Setup();
    Options();
  }
  Channels.IncBeingEdited();

  if(viewMode == mode_edit)
     SetStatus(tr("Select channels with OK"));

#ifdef DEBUG_TIMES
  struct timeval now2;
  gettimeofday(&now2, NULL);
  secs2 = ((float)((1000000 * now2.tv_sec + now2.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
  PRINTF("======================= cMenuBouquets: time since key: secs: %f\n", secs2);
#endif
}

cMenuBouquets::~cMenuBouquets()
{
  Channels.DecBeingEdited();
  kChanOpen = false;
}

void cMenuBouquets::Setup(/*bool selectionChanched*/)
{
  if (view_ == 4)
  {
    SetChannels();
  }
  else
  {
    int Index = Current();
    if(Index >= 0)
    {
        int index = 0;
        cMenuChannelItem *currentItem = static_cast<cMenuChannelItem *>(Get(Current()));
        for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
        {
            if(currentItem->Channel() == channel)
            {
                break;
            }
            ++index;
        }
        Index = index;
    }
    else if(startChannel)
    {
        Index = Channels.GetNextNormal(startChannel);
    }

    if(Index < 0 && Channels.GetByNumber(cDevice::CurrentChannel()))
        Index = Channels.GetByNumber(cDevice::CurrentChannel())->Index();

    if(Index > -1)
        SetGroup(Index);
  }
  //Display(); //TB: not needed
}

void cMenuBouquets::CreateTitle(cChannel *channel)
{
  if(channel && channel->GroupSep()) {
    if(channel->Name() || strlen(channel->Name()) > 0){
        /*if(view_ == 4)
        {
        }*/
        if(viewMode == mode_edit && strcmp(Skins.Current()->Name(), "Reel") == 0 && view_ != 4)
        {
           strcpy((char*)&titleBuf, "menunormalhidden$");
           strncat((char*)&titleBuf, tr("Customize channels: "), 100);
           strncat((char*)&titleBuf, channel->Name(), 100);
           SetTitle((const char*)&titleBuf);
        }
        else
        {
            std::string title;
            if(view_ == 0 || view_ == 1)
            {
                title = std::string(channel->Name()) + ": ";
            }
            else
            {
                title =  "..";
                //strcpy((char*)&titleBuf, ".. ");
            }

            if(::Setup.OnlyRadioChannels == 1 && ::Setup.OnlyEncryptedChannels == 1)
            {
                title += tr("encrypted radio channels");
            }
            else if(::Setup.OnlyRadioChannels == 1 && ::Setup.OnlyEncryptedChannels == 2)
            {
                title += tr("free radio channels");
            }
            else if(::Setup.OnlyRadioChannels == 1 && ::Setup.OnlyEncryptedChannels == 0)
            {
                title += tr("radio channels");
            }
            if(::Setup.OnlyRadioChannels == 2 && ::Setup.OnlyEncryptedChannels == 1)
            {
                title += tr("encrypted TV channels");
            }
            else if(::Setup.OnlyRadioChannels == 2 && ::Setup.OnlyEncryptedChannels == 2)
            {
                title += tr("free TV channels");
            }
            else if(::Setup.OnlyRadioChannels == 2 && ::Setup.OnlyEncryptedChannels == 0)
            {
                title += tr("TV channels");
            }
            else if(::Setup.OnlyRadioChannels == 0 && ::Setup.OnlyEncryptedChannels == 1)
            {
                title += tr("encrypted channels");
            }
            else if(::Setup.OnlyRadioChannels == 0 && ::Setup.OnlyEncryptedChannels == 2)
            {
                title += tr("free channels");
            }
            else if(::Setup.OnlyHDChannels == 1)
            {
                title += tr("HD channels");
            }
            else if(::Setup.OnlyHDChannels == 2)
            {
                title += tr("non HD channels");
            }
            else if(::Setup.OnlyRadioChannels == 0 && ::Setup.OnlyEncryptedChannels == 0 && ::Setup.OnlyHDChannels == 0)
            {
                title += tr("all channels");
            }

        //printf("----------title = %s-----------\n", channel->Name());
        SetTitle(title.c_str());
        }
    }
    else
    {
      SetTitle("");
    }
  }
  else
    SetTitle("");
}

void cMenuBouquets::SetChannels(void)
{
   cMenuChannelItem *currentItem = 0;
   cChannel *currentChannel = 0;
   if(Current() >= 0)
   {
     currentItem = static_cast<cMenuChannelItem *>(Get(Current()));
   }

   if(currentItem)
   {
     currentChannel = currentItem->Channel();
   }

   if(!currentChannel)
   {
     currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
   }

   Clear();

   for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
      if ( !channel->GroupSep() || (cMenuChannelItem::SortMode() == cMenuChannelItem::csmNumber && *channel->Name()))
        if((channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 1))
        &&(!channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 2))
        &&(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel() || !(::Setup.OnlyEncryptedChannels == 1))
        &&(!(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel()) || !(::Setup.OnlyEncryptedChannels==2))
        &&(channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 1))
        &&(!channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 2)))
        {
            cMenuChannelItem *item;
            if(view_ == 4)
                item = new cMenuChannelItem(channel, cMenuChannelItem::mode_classic);
            else if(viewMode == mode_edit)
                item = new cMenuChannelItem(channel, cMenuChannelItem::mode_edit);
            else
                item = new cMenuChannelItem(channel);
            Add(item);
            item->Set();
            //printf("-----Add(item) item->Text = %s---\n", item->Text());
            if (channel == currentChannel)
                currentItem = item;

            unsigned int i;
            for (i=0; i<channelMarked.size(); i++)
            {
                if (channelMarked.at(i) == channel->Number())
                {
                    //printf("MARKING chan pos %i nr: %i name: %s\n", i, channel->Number(), channel->Name());
                    item->SetMarked(true);
                    item->Set();
                }
            }
        }
     }

     if (cMenuChannelItem::SortMode() != cMenuChannelItem::csmNumber)
        Sort();

     if(currentItem)
        SetCurrent(currentItem);
}

void cMenuBouquets::SetGroup(int Index)
{
  bool back = false;
  if(Index < 0) Index = 0;
  cChannel *currentChannel = Channels.Get(Index);
  cChannel *firstChannel = NULL;
  if(Channels.Count() == 0) return;
  if (Index == 0)
    currentChannel = Channels.First();
  else if (!currentChannel)
    currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
  else if (Current() > 0)
    back = true;
  cMenuChannelItem *currentItem = NULL;
  if (!currentChannel->GroupSep()){

    startChannel = Channels.GetPrevGroup(currentChannel->Index());
    }
  else
    startChannel = currentChannel->Index();
  if (startChannel < 0) {
    startChannel = 0;
    firstChannel = Channels.First();
    }
  else
    firstChannel = Channels.Get(startChannel + 1);
  if (!firstChannel || firstChannel->GroupSep()) {
    Clear();
    return;
    }
  if (back == true) {
    if (startChannel == 0 && !firstChannel->GroupSep())
      currentChannel = firstChannel;
    else
      ;//currentChannel = (cChannel*) (Channels.Get(startChannel)->Next());
    }

  isyslog("start: %d name of first channel: %s", startChannel, firstChannel->Name());
  isyslog("current name: %s", currentChannel->Name());
  //printf("current name: %s", currentChannel->Name());
  Clear();
  int i = 0;
  for (cChannel *channel = firstChannel; channel && !channel->GroupSep(); channel = Channels.Next(channel)) {
      if(!channel->GroupSep() || (cMenuChannelItem::SortMode() == cMenuChannelItem::csmNumber && *channel->Name())) {
            if((channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 1))
            &&(!channel->IsRadioChannel() || !(::Setup.OnlyRadioChannels == 2))
            &&(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel() || !(::Setup.OnlyEncryptedChannels == 1))
            &&(!(channel->IsEncryptedRadioChannel() || channel->IsEncryptedVideoChannel()) || !(::Setup.OnlyEncryptedChannels==2))
            &&(channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 1))
            &&(!channel->IsHDChannel() || !(::Setup.OnlyHDChannels == 2)))
            {

                cMenuChannelItem *item;
                if(viewMode == mode_edit)
                    item = new cMenuChannelItem(channel, cMenuChannelItem::mode_edit);
                else
                    item = new cMenuChannelItem(channel);
                Add(item);
                item->Set();
                //printf("-----Add(item) item->Text = %s, i = %d, Ca = %d---\n", item->Text(), i, item->Channel()->Ca());
                ++i;
                if (channel == currentChannel)
                    currentItem = item;
                unsigned int i;
                for (i=0; i<channelMarked.size(); i++) {
                    if (channelMarked.at(i) == channel->Number()){
                        //printf("MARKING chan pos %i nr: %i name: %s\n", i, channel->Number(), channel->Name());
                        item->SetMarked(true);
                        item->Set();
                    }
                }
            }
        }
    }

    if (cMenuChannelItem::SortMode() != cMenuChannelItem::csmNumber)
      Sort();

    if(currentItem)
        SetCurrent(currentItem);
}

cChannel *cMenuBouquets::GetChannel(int Index)
{
  cMenuChannelItem *p = (cMenuChannelItem *)Get(Index);
  return p ? (cChannel *)p->Channel() : NULL;
}

/* just removes the checkmark */
void cMenuBouquets::UnMark(cMenuChannelItem *p)
{
    if(p){
        p->SetMarked(false);
        p->Set();
    }
}

void cMenuBouquets::Mark()
{
  if (Count()) {
     if (viewMode == mode_edit || view_ == 4) {
        cMenuChannelItem *p = (cMenuChannelItem *)Get(Current());
    if (p) {
       if (p->IsMarked()){
              //printf("UNMARKED chan nr: %i chnr: %i name: %s\n", Current(), GetChannel(Current())->Number(), GetChannel(Current())->Name());
          /* remove checkmark */
          p->SetMarked(false);
              unsigned int i;
          /* erase from "marked channels"-list */
          for (i=0; i<channelMarked.size(); i++)
        if (channelMarked.at(i) == GetChannel(Current())->Number())
               channelMarked.erase(channelMarked.begin()+i);
          CursorDown();
       } else {
              //printf("MARKED chan nr: %i chnr: %i name: %s\n", Current(), GetChannel(Current())->Number(), GetChannel(Current())->Name() );
          /* set checkmark */
          p->SetMarked(true);
          /* put into "marked channels-list */
              channelMarked.push_back(GetChannel(Current())->Number());
          CursorDown();
           }
           p->Set();
       Display();
    }
    Options();
    }
     //SetStatus(tr("1-9 for new location - OK to move"));
     if(view_ == 4 && !move) //???
        SetStatus(tr("Select channels with blue key"));
     else if (viewMode == mode_view || (viewMode == mode_edit && move) || (view_ == 4 && move))
        SetStatus(tr("Up/Dn for new location - OK to move"));
     else if (viewMode == mode_edit)
        SetStatus(tr("Select channels with OK"));
     }
}

void cMenuBouquets::Propagate(void)
{
  Channels.ReNumber();
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next())
     ci->Set();
  Display();
  Channels.SetModified(true);
}

/*eOSState cMenuBouquets::Number(eKeys Key)
{

  if (HasSubMenu())
     return osContinue;
  if (numberTimer.TimedOut())
     number = 0;

  number = number * 10 + Key - k0;
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next()) {
       if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number) {
         SetCurrent(ci);
         Display();
         break;
       }
  }
  numberTimer.Set(CHANNELNUMBERTIMEOUT);

  return osContinue;
}*/

eOSState cMenuBouquets::Number(eKeys Key)
{
  if (HasSubMenu())
     return osContinue;
  if (numberTimer.TimedOut())
     number = 0;
  if (!number && Key == k0) {
     cMenuChannelItem::IncSortMode();
     Setup();
     Display();
     }
  else {
     number = number * 10 + Key - k0;
     for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next()) {
         if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number) {
            SetCurrent(ci);
            Display();
            break;
            }
         }
     numberTimer.Set(CHANNELNUMBERTIMEOUT);
     }
  return osContinue;
}

eOSState cMenuBouquets::Switch(void)
{
  if (HasSubMenu())
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return cDevice::PrimaryDevice()->SwitchChannel(ch, true) ? osEnd : osContinue;
  return osEnd;
}

eOSState cMenuBouquets::NewChannel(void)
{
  if (HasSubMenu())
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (!ch || favourite) {
    ch = Channels.GetByNumber(cDevice::CurrentChannel());
    favourite = false;
  }
  return AddSubMenu(new cMenuEditChannel(ch, true));
}

eOSState cMenuBouquets::EditChannel(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if(channelMarked.size() == 1) //use marked channel
  {
     ch = Channels.GetByNumber(channelMarked.at(0));
  }
  if (ch) {
     return AddSubMenu(new cMenuEditChannel(ch, false));
  }
  return osContinue;
}

eOSState cMenuBouquets::DeleteChannel(void)
{
  if (!HasSubMenu() && Count() > 0) {
     if (channelMarked.empty()) {
        int Index = Current();
        cChannel *channel = GetChannel(Current());
        int DeletedChannel = channel->Number();
        // Check if there is a timer using this channel:
        for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti)) {
            if (ti->Channel() == channel) {
               Skins.Message(mtError, tr("Channel is being used by a timer!"));
               return osContinue;
               }
            }
        //do not shwo buttons while deleting
        SetHelp(NULL, NULL, NULL, NULL);
        if (Interface->Confirm(tr("Delete channel?"))) {
           Channels.Del(channel);
           cOsdMenu::Del(Index);
           Propagate();
           //succesfully deleted
           Skins.Message(mtInfo, tr("Channel deleted"));
           Options();
           isyslog("channel %d deleted", DeletedChannel);
           }
           //redraw buttons
           Options();
     } else {
        int unsigned i;
        bool confirmed = false;
        /* sort and begin from behind: */
    /* channels must be deleted from the end, otherwise */
    /* the numbers of the channels that still have to be deleted will change */
        if(!channelMarked.empty())
       std::sort(channelMarked.begin(), channelMarked.end());
    //printf("XXX: size: %i\n", channelMarked.size());
    for (i=channelMarked.size(); i>0; i--) {
           //int Index = channelMarked.at(i);
           cChannel *channel = Channels.GetByNumber(channelMarked.at(i-1)); //GetChannel(channelMarked.at(i));
       //int Index = channel->Index();
           int DeletedChannel = channel->Number();
       //printf("XXX: i: %i at(i): %i Name: %s index: %i\n", i-1, channelMarked.at(i-1), channel->Name(), channel->Index());
           // Check if there is a timer using this channel:
           for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti)) {
               if (ti->Channel() == channel) {
                  Skins.Message(mtError, tr("Channel is being used by a timer!"));
                  //redraw buttons
                  Options();
                  return osContinue;
                  }
               }
           //do not show buttons while deleting
           SetHelp(NULL, NULL, NULL, NULL);
           if (confirmed || Interface->Confirm(tr("Delete channels?")))
           {
                confirmed = true;
                Channels.Del(channel);

                int start = Current() - LOAD_RANGE;
                int end = Current() + LOAD_RANGE;
                //cChannel *channel;
                if(Count() < end) end = Count();
                if(start < 0) start = 0;

                for(int i = start; i<end; i++){
                    cMenuChannelItem *p = (cMenuChannelItem *)Get(i);
                    if(p) {
                        if(p->IsMarked())
                        cOsdMenu::Del(i);
                    }
                }
                //cOsdMenu::Del(Index);
                Propagate();
                //succesfully deleted,
                Skins.Message(mtInfo, tr("Channels deleted"));
                isyslog("channel %d deleted", DeletedChannel);
           }
           else
           {
              //not confirmed -> go out
              break;
           }
         }
         //redraw buttons
         Options();
    if(!channelMarked.empty())
        channelMarked.clear();
     }
  }
  return osContinue;
}

eOSState cMenuBouquets::ListBouquets(void)
{
  if (HasSubMenu())
  {
    return osContinue;
  }
  if(viewMode==mode_edit || view_ == 4)
  {
    return AddSubMenu(new cMenuBouquetsList(Channels.Get(startChannel),0));
  }
  if(viewMode==mode_view)
  {
    return AddSubMenu(new cMenuBouquetsList(Channels.Get(startChannel),1));
  }
  return osContinue;
}

void cMenuBouquets::Move(int From, int To)
{
  Move(From, To, true);
}

void cMenuBouquets::Move(int From, int To, bool doSwitch)
{
  //printf("#####Move: From = %d To = %d\n", From, To);
  int CurrentChannelNr = cDevice::CurrentChannel();
  cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
  cChannel *FromChannel;
  cChannel *ToChannel;
  if (true){//viewMode == mode_view || view_ == 4) {
     FromChannel = (cChannel*) Channels.Get(From);
     ToChannel = (cChannel*) Channels.Get(To);
     //printf("#####Move: FromChannel = %d ToChannel = %d\n", FromChannel, ToChannel);
     //if(FromChannel && ToChannel)
     //   printf("#####Move: FromChannel = %s ToChannel = %s\n", FromChannel->Name(), ToChannel->Name());

  } else {
     FromChannel = (cChannel*) Channels.GetByNumber(From);
     ToChannel = (cChannel*) Channels.GetByNumber(To);
  }
  if (FromChannel && ToChannel /* && (From != To) */) {
     int FromNumber = FromChannel->Number();
     int ToNumber = ToChannel->Number();
     Channels.Move(FromChannel, ToChannel);
     if(doSwitch) {
        Propagate();
        if(view_ != 4)
        {
            SetGroup(startChannel);
        }
        Display();
     }
     if(ToNumber)
       isyslog("channel %d moved to %d", FromNumber, ToNumber);
     else
       isyslog("channel %d moved to %s", FromNumber, ToChannel->Name());
     if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr && doSwitch)
        Channels.SwitchTo(CurrentChannel->Number());
     }
}

enum directions { dir_none, dir_forward, dir_backward  };

eOSState cMenuBouquets::MoveMultipleOrCurrent(void)
{
    if(channelMarked.size() == 0)
    {
        Mark();
        return MoveMultiple();
    }
    return MoveMultiple();
}

eOSState cMenuBouquets::MoveMultiple(void)
{
    int current, currentIndex;
    unsigned int i;
    enum directions direction = dir_none;

    cChannel *playingchannel = cDevice::CurrentChannel() != 0 ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
    int playingchannelnum = cDevice::CurrentChannel();

    Current() > -1 ? current = GetChannel(Current())->Number(): current = startChannel;
    Current() > -1 ? currentIndex = GetChannel(Current())->Index(): currentIndex = startChannel;

    /* we are finishing the "move-mode" */
    move = false;
    /* sort the list of channels to change - makes it easier */
    std::sort(channelMarked.begin(), channelMarked.end());
    //bool inced = false;
           Setup();
    /* for all marked channels */
    for (i = 0; i < channelMarked.size(); i++) {
        Channels.ReNumber();
        if(!startChannel || current <= channelMarked.at(i)+1)
            direction = dir_backward;
        else
            direction = dir_forward;

        /* the channel shall be moved before the channel highlighted by the cursor */
        /* so decrease BUT NOT if "current-1" is a separator */
        /* and NOT if two channels with a distance of 1 should be swapped */
        if (direction == dir_backward /*&& current != startChannel && current > 1  && !((cChannel*)Channels.GetByNumber(current)->Prev())->GroupSep() */ && !(current-channelMarked.at(i)==1 /*&& !GetChannel(currentIndex-1)->GroupSep()*/) && i != 0){ //do not increment for first channel
            current++;
            currentIndex++;
        }

        cMenuChannelItem *p = (cMenuChannelItem *)Get(currentIndex);
        UnMark(p);
        if (channelMarked.at(i) != current || viewMode == mode_edit)
        {
            if (!Channels.Get(currentIndex)->GroupSep()) {
                if(channelMarked.at(i) != current){
                    Channels.Move(Channels.GetByNumber(channelMarked.at(i)) , Channels.GetByNumber(current));
                    unsigned int j;
                    /* if we are moving forward, the target number has to be decreased */
                    if(direction == dir_forward)
                        for(j=i; j<channelMarked.size(); j++)
                            if(channelMarked.at(j) > 1 && !Channels.GetByNumber(channelMarked.at(j)-1)->GroupSep()){
                                channelMarked[j]--;
                            }
                }
            } else {
                cChannel *chan = Channels.GetByNumber(channelMarked.at(i));
                Channels.Del(Channels.GetByNumber(channelMarked.at(i)), false);
                if(direction == dir_forward){
                    Channels.Add(chan, Channels.Get(currentIndex-1));

                int j;

                /* if we are moving forward, the target number has to be decreased */
                if(direction == dir_forward)
                    for(j=i; j<(int)channelMarked.size(); j++)
                        if(channelMarked.at(j) > 1 && !Channels.GetByNumber(channelMarked.at(j)-1)->GroupSep()){
                            channelMarked[j]--;
                        }
                    currentIndex--;
                } else {
                    Channels.Add(chan, Channels.Get(currentIndex));
                }
            }
        }
        if (viewMode == mode_edit || view_ == 4) {
            p = (cMenuChannelItem *)Get(current);
            UnMark(p);
            p = (cMenuChannelItem *)Get(channelMarked.at(i));
            UnMark(p);
            p = (cMenuChannelItem *)Get(current);
            UnMark(p);
        }
    }
    for (i = 0; i < channelMarked.size(); i++) {
        cMenuChannelItem *p = (cMenuChannelItem *)Get(channelMarked.at(i));
        UnMark(p);
    }
    channelMarked.clear();
    if(view_ != 4)
    {
        SetGroup(currentIndex);
    }
    Propagate();

    //SetStatus(tr("Select channels with OK"));
    //if(GetChannel(current))
    //    Channels.SwitchTo( GetChannel(Current())->Number() );

    if(playingchannel && playingchannelnum != playingchannel->Number()) //if position of channel beeing replayed has chanched
    {
        Channels.SwitchTo(playingchannel->Number());
    }

    if(view_ == 4)
    {
         int curPos = Current();
         Clear();
         Setup();
         SetCurrent(Get(curPos));
         SetStatus(NULL);
    }
    Display();
    Skins.Message(mtInfo, tr("Channels have been moved"));
    return osContinue;
}

void cMenuBouquets::GetFavourite(void)
{
  cChannel *channel = Channels.First();
  if(!channel->GroupSep() || 0 != strcmp(channel->Name(), tr("Favorites"))) {
    cChannel *newChannel = new cChannel();
    newChannel->groupSep = true;
    newChannel->nid = 0;
    newChannel->tid = 0;
    newChannel->rid = 0;
    strcpyrealloc(newChannel->name, tr("Favorites"));
    Channels.Add(newChannel);
    Channels.Move(newChannel, Channels.First());
    Channels.ReNumber();
    Channels.SetModified(true);
  }
}

void cMenuBouquets::AddFavourite(bool active)
{
  cChannel *channel = active ? Channels.GetByNumber(cDevice::CurrentChannel()) : GetChannel(Current());
  Display();
  if(channel && Interface->Confirm(tr("Add to Favorites?"))) {
    cChannel *newChannel = new cChannel();
    *newChannel = *channel;
    newChannel->rid = 100;
    Channels.Add(newChannel);
    GetFavourite();
    Channels.Move(newChannel, Channels.First()->Next());
    Channels.ReNumber();
    Channels.SetModified(true);
    SetGroup(0);
    Display();
    }
}

eOSState cMenuBouquets::PrevBouquet()
{
  if (HasSubMenu())
    return osContinue;
  if(startChannel > 0) {
    SetGroup(startChannel-1);
    Display();
  }
  else {
    SetGroup(Channels.Count()-1);
    Display();
    }
  return osContinue;
}

eOSState cMenuBouquets::NextBouquet()
{
  int next;
  if (HasSubMenu())
    return osContinue;
  next = Channels.GetNextGroup(startChannel);
  if(-1 < next && next < Channels.Count()) {
      SetGroup(next);
      Display();
    }
  else {
    SetGroup(0);
    Display();
    }
  return osContinue;
}

void cMenuBouquets::Options()
{
  if (view_ == 4)
  {
        SetHelp(tr("Button$Edit"), tr("Button$New"), tr("Button$Delete"), tr("Button$Mark"));
  }
  else if (viewMode == mode_view)
  {
        if(::Setup.ExpertNavi)
        {
            //printf("------2------\n");
            SetHelp(tr("Bouquets"), tr("Key$Back"), tr("Key$Next"), Count() ? tr("Button$Customize") : NULL);
        }
        else
        {
            //printf("------2A------\n");
            SetHelp(tr("Bouquets"), NULL, NULL, Count() ? tr("Button$Customize") : NULL);
        }
  }
  else
  {
        if (move)
        {
           //printf("------5------\n");
           SetHelp(tr("Bouquets"), tr("Button$Insert"), NULL, NULL);
        }
        else
        {
            SetStatus(tr("Select channels with OK"));
            if (channelMarked.size() > 1 )
            {
                //printf("------6A------\n");
                SetHelp(tr("Bouquets"), tr("Move"), tr("Button$Delete"), NULL);
            }
            else if (channelMarked.size() == 1 )
            {
                //printf("------6B------\n");
                SetHelp(tr("Bouquets"), tr("Move"), tr("Button$Delete"), tr("Button$Edit"));
            }
            else
            {
                //printf("------6------\n");
                SetHelp(NULL, tr("Move"), tr("Button$Delete"), tr("Button$Edit"));
            }
         }
  }
}

void cMenuBouquets::Display(void){
#ifdef DEBUG_TIMES
  struct timeval now2;
  gettimeofday(&now2, NULL);
#endif

  int start = Current() - LOAD_RANGE;
  int end = Current() + LOAD_RANGE;
  cChannel *channel;
  if(Count() < end) end = Count();
  if(start < 0) start = 0;
  for(int i = start; i<end; i++){
      cMenuChannelItem *p = (cMenuChannelItem *)Get(i);
      if(p) {
         if(!p->IsSet())
            p->Set();
      }
  }

  if(viewMode == mode_view && !view_ == 4) {
      cChannel *chan = NULL;
      cMenuChannelItem *p = (cMenuChannelItem *)Get(end-1);

      if(p)
          chan = p->Channel();
      int endNr;
      int nrCharsForChanNr = 0;

      if(chan) {
        endNr = chan->Number();
        while(endNr > 0) {
         endNr /= 10;
         nrCharsForChanNr++;
        }
      }
      if(nrCharsForChanNr < 2)
        nrCharsForChanNr = 2;
      //DDD("viewMode == mode_view && !view_ == 4");
      SetCols(nrCharsForChanNr, 1, 14, 6);
  }

  channel = Channels.Get(startChannel);
  CreateTitle(channel);
  Options();
  cOsdMenu::Display();
#ifdef DEBUG_TIMES
  struct timeval now3;
  gettimeofday(&now3, NULL);

  float secs = ((float)((1000000 * now3.tv_sec + now3.tv_usec) - (now2.tv_sec * 1000000 + now2.tv_usec))) / 1000000;
#endif
}

extern eOSState active_function;

eOSState cMenuBouquets::ProcessKey(eKeys Key)
{
  if(Key == kInfo)
  {
    return AddSubMenu(new cSetChannelOptionsMenu(selectionChanched));
  }
  //if(Key==kUp ||Key==kDown)

  //printf("current name: %s, current = %d\n", Channels.Get(Current())->Name(), Current());
  eOSState state = cOsdMenu::ProcessKey(NORMALKEY(Key));

  static bool hadSubMenu = false;
  if(hadSubMenu && !HasSubMenu() && selectionChanched)
  {
    selectionChanched = false;
    if(::Setup.UseBouquetList == 2)
    {
        return ListBouquets();
    }
    else if(::Setup.UseBouquetList == 0)
    {
        view_ = 4;
    }
    else if(::Setup.UseBouquetList == 1)
    {
        view_ = 0;
    }
    Setup();
    Display();
  }
  hadSubMenu = HasSubMenu();

  //  isyslog("ProcessKey ChannelKey: %d", Key);

  switch (state) {
    case osUser1: { // new channel
         //printf("######################new channel##########################\n");
         cChannel *channel = Channels.Last();
         if(channel)
                printf("Channel->Name = %s\n", channel->Name());
         if (channel) {
            int current;
            cChannel *currentChannel = GetChannel(Current());
            //if(currentChannel)
            //    printf("currentChannel->Name = %s\n", currentChannel->Name());
            if(currentChannel)
              current = currentChannel->Index() + 1;
            else
              current = Channels.GetNextGroup(startChannel);
            if( -1 < current && current < Channels.Count())
            {
              //printf("-------------Move---------\n");
              Move(channel->Index() , current, true);
              Setup();
              Display();
            }
            else
            {
              //printf("-------------Add---------\n");
              Add(new cMenuChannelItem(channel), true);
            }
            return CloseSubMenu();
            }
         break;
         }
    case osUser5: // view bouquet channels
         if(HasSubMenu())
           CloseSubMenu();
         view_ = 0;
         SetGroup(::Setup.CurrentChannel);
         Display();
         break;
    case osBack:
        if(kChanOpen)
        {;
            return osEnd;
        }
         if(::Setup.UseBouquetList == 2)
         {
            return ListBouquets();
         }
         else {
           /* TB: test if it was called via menu or via kOk */
       if (active_function==0) {
         move = false;
         viewMode = mode_view;
         channelMarked.clear();
             return osBack;
       } else if(viewMode == mode_edit) {
         if(move) //???
         {
            move = false;
            channelMarked.clear();
            int curPos = Current();
            if(Current() && GetChannel(Current()))
                SetGroup(GetChannel(Current())->Index());
            SetCurrent(Get(curPos));
            SetStatus(NULL);
            Display();
            return osContinue;
         }
         viewMode = mode_view;
         channelMarked.clear();
         int curPos = Current();
         /*if(Current() && GetChannel(Current()))     ----------needed?
            SetGroup(GetChannel(Current())->Index());*/
         /*if (strcmp(Skins.Current()->Name(), "Reel") == 0)
             SetCols(4, 14, 6);
         else
             SetCols(5, 18, 6);*/
         Clear();
         Setup();
         SetCurrent(Get(curPos));
         SetStatus(NULL);
         Display();
         return osContinue;
       } else
             return osEnd;
         }
         break;
    default:
         if (state == osUnknown) {
            switch (Key) {
             case k0 ... k9:
                  {
                     dsyslog("ProcessKey GetKey: %d extractVal %d", Key, Key-k0);
                     Number(Key);
                  }
                            Setup();
                            break;
              case kOk:
                           if (channelMarked.size() > 0)
                           {
                                if(view_ == 4)
                                {
                                    if(move)
                                    {
                                        return MoveMultipleOrCurrent();
                                        /*int From = channelMarked[0];
                                        int To = Current();
                                        Move(From, To);*/
                                    }
                                }
                                else if(viewMode == mode_view)
                                {
                                    SetStatus(NULL);
                                }
                                else if((viewMode == mode_edit && !move)/*||(channelMarked.empty() && viewMode == mode_view)*/) //??
                                {
                                    Mark();
                                    return osContinue;
                                }
                           }
                           else
                           {
                                if(viewMode == mode_view || view_ == 4)
                                {
                                    return Switch();
                                }
                                else
                                {
                                    Mark();
                                }
                           }
                           return osContinue;
              case kRed:
                            //printf("-------return ListBouquets()-----------\n");
                            if(view_ == 4)
                            {
                                return EditChannel();//Edit();//TODO
                            }
                            else if(viewMode == mode_view || channelMarked.size() > 0)
                            {
                                kChanOpen = false;
                                return ListBouquets();
                            }
                            return osContinue;

              case kGreen:
                             if(view_ == 4)
                             {
                                return NewChannel();
                             }
                             else if (viewMode == mode_view)
                             {
                                if(::Setup.ExpertNavi)
                                {
                                    return PrevBouquet();
                                }
                             }
                             else
                             {
                                if(!move)
                                {
                                    if(channelMarked.empty()) //mark current item for move, if no item selected
                                    {
                                        Mark();
                                    }
                                    SetStatus(tr("Move up/down for new position - then press 'Insert'"));
                                    move = true;
                                    Options();
                                }
                                else if(move)
                                {
                                  return MoveMultipleOrCurrent();
                                }
                            }
                            break;
              case kYellow:
                            if(view_ == 4 || viewMode == mode_edit)
                            {
                                if(!move)
                                { // Delete
                                     DeleteChannel();
                                }
                            }
                            else if (viewMode == mode_view)
                            {
                                if(::Setup.ExpertNavi)
                                {
                                   return NextBouquet();
                                }
                             }
                            break;
              case kBlue:
                            if(view_ == 4)
                            {
                               if(channelMarked.empty()) //mark current item for move, if no item selected
                               {
                                    Mark();
                                    SetStatus(tr("Move up/down for new position - then press 'OK'"));
                                    move = true;
                                    Options();
                               }
                               else
                               {
                                    Mark();
                               }
                            }
                            else if(viewMode == mode_view)
                            {
#if 1
                                  if ( !GetChannel(Current()))
                                  {
                                    return osContinue;
                                  }
                                  viewMode = mode_edit;
                                  int curPos = Current();
                                  SetGroup(GetChannel(Current())->Index());
                                  if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                                     SetCols(4, 4, 14, 6);
                                  else
                                     SetCols(4, 5, 18, 6);
                                  SetCurrent(Get(curPos));
                                  SetStatus(tr("Select channels with OK"));
                                  Display();


                                    /*int index = 0;
                                    cMenuChannelItem *currentItem = static_cast<cMenuChannelItem *>(Get(Current()));
                                    printf("currentItem->Text() = %s\n", currentItem->Text());
                                    for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
                                    {
                                        if(currentItem->Channel() == channel)
                                        {
                                            break;
                                        }
                                        ++index;
                                    }

                                  viewMode = mode_edit;
                                  //int curPos = Current();
                                  //SetGroup(index);
                                  if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                                     SetCols(4, 4, 14, 6);
                                  else
                                     SetCols(4, 5, 18, 6);

                                 for (cOsdItem *item = First(); item; item = Next(item))
                                 {
                                    if(static_cast<cMenuChannelItem *>(item)->Channel() == currentItem->Channel())
                                    {
                                        SetCurrent(item);
                                    }
                                 }
                                 if(currentItem)
                                    SetCurrent(currentItem);
                                  SetCurrent(Get(curPos));
                                  SetStatus(tr("Select channels with OK"));
                                  Display();*/
#else
                                  return EditChannel();
#endif
                            }
                            else
                            {
                                  return EditChannel();
                            }
                            break;
              case k2digit:  AddFavourite(false);
                         break;
              case kGreater:
                            if (view_ == 0)
                            {
                                //viewMode2 = 1;
                                view_ = 4;
                                ::Setup.UseBouquetList = 0;
                            }
                            else if (view_ == 4)
                            {
                                //viewMode2 = 0;
                                view_ = 0;
                                ::Setup.UseBouquetList = 1;
                                //printf("viewMode2 = 0: current name: %s\n", Channels.Get(Current())->Name());
                            }
                            if (view_ == 4) {
                                if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                                    SetCols(4, 3, 14, 6);
                                else
                                    SetCols(4, 3, 14, 6);
                            }
                            else if (viewMode == mode_view) {
                                if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                                    SetCols(4, 3, 14, 6);
                                else
                                    SetCols(5, 18, 6);
                            }
                            else {
                                if (strcmp(Skins.Current()->Name(), "Reel") == 0)
                                    SetCols(4, 4, 14, 6);
                                else
                                    SetCols(4, 5, 18, 6);
                            }
                            Setup();
                            Display();
                            break;
                 break;
              default: break;
              }
            }
    }
  return state;
}

#endif // INTERNAL_BOUQUETS
//------ShowBouquets-----------------------------------------------------
//for calling bouquets plugin

#if 0
cOsdMenu *GetBouquetMenu(int view, cMenuBouquets::eViewMode mode = cMenuBouquets::mode_view)
{
    //printf("----GetBouquetMenu, view = %d, mode = %d-------------\n",  view, mode);
    if(true)
    {
        struct BouquetsStartData
        {
            int view;
            cMenuBouquets::eViewMode mode;
        }
        bouquetStartData = { view, mode };

        cPluginManager::CallAllServices("Bouquets set mode", &bouquetStartData);

        cPlugin *bouquetPlugin = cPluginManager::GetPlugin("bouquets");
        return static_cast<cOsdMenu*>(bouquetPlugin->MainMenuAction());
    }
    else
    {
        return new cMenuBouquets(view, mode);
    }
}
#endif

// --- cMenuText -------------------------------------------------------------

cMenuText::cMenuText(const char *Title, const char *Text, eDvbFont Font)
:cOsdMenu(Title)
{
  text = NULL;
  font = Font;
  SetText(Text);
  Display();
}

cMenuText::~cMenuText()
{
  free(text);
}

void cMenuText::SetText(const char *Text)
{
  free(text);
  text = Text ? strdup(Text) : NULL;
}

void cMenuText::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, font == fontFix); //XXX define control character in text to choose the font???
  if (text)
     cStatus::MsgOsdTextItem(text);
}

eOSState cMenuText::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk: return osBack;
       default:  state = osContinue;
       }
     }
  return state;
}

// --- cMenuHelp --------------------------------------------------------

cMenuHelp::cMenuHelp(cHelpSection *Section, const char *Title)
:cOsdMenu(Title)
{

  text = NULL;
  helpPage = NULL;
  section = Section;
  char buffer[128];
  snprintf(buffer,128, "%s - %s",tr("Help"), Title);
  SetTitle(buffer);

  if (Section)
     helpPage = Section->GetHelpByTitle(Title);

  if (helpPage)
    SetText(helpPage->Text());

  Display();
}

cMenuHelp::~cMenuHelp()
{
  if (text) free(text);
}

void cMenuHelp::SetText(const char *Text)
{
  if (text) free(text);
  text = Text ? strdup(Text) : NULL;
}
void cMenuHelp::SetNextHelp()
{

  SetStatus(NULL);
  cHelpPage *h =  static_cast<cHelpPage *>(helpPage->cListObject::Next());
  if (h) // aviod malloc/free!
  {
    helpPage = h;
    const char *myTitle = helpPage->Title();
    SetText(helpPage->Text());
    char buffer[128];
    snprintf(buffer,128,"%s - %s",tr("Help"), myTitle);
    SetTitle(buffer);
    Display();
  }
  else
  {
     SetStatus(tr("Already first help item"));
  }
}

void cMenuHelp::SetPrevHelp()
{
  SetStatus(NULL);
  cHelpPage *h = static_cast<cHelpPage *>(helpPage->cListObject::Prev());
  if (h)
  {
    helpPage = h;
    const char *myTitle = helpPage->Title();
    SetText(helpPage->Text());

    char buffer[1024];
    snprintf(buffer,1024, "%s - %s",tr("Help"), myTitle);
    SetTitle(buffer);
    Display();
  }
  else
  {
     SetStatus(tr("Already last help item"));
  }
}

void cMenuHelp::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, font == fontFix); //XXX define control character in text to choose the font???
  if (text)
     cStatus::MsgOsdTextItem(text);
}

eOSState cMenuHelp::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:

                  SetNextHelp();
                  break;
    case kDown|k_Repeat:
    case kDown:
                  SetPrevHelp();
                  break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    case kInfo: return osBack; // XXX TOTEST
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kInfo: return osBack;
       case kOk: return osBack;
       default:  state = osContinue;
       }
     }
  return state;
}

// --- cMenuEditTimer --------------------------------------------------------

cMenuEditTimer::cMenuEditTimer(cTimer *Timer, bool New)
:cOsdMenu(tr("Edit timer"), 15)
{
  firstday = NULL;
  timer = Timer;
  addIfConfirmed = New;
  PriorityTexts[0] = tr("low");
  PriorityTexts[1] = tr("normal");
  PriorityTexts[2] = tr("high");

  if (timer) {
     data = *timer;
     if (New)
        data.SetFlags(tfActive);
     channel = data.Channel()->Number();
     tmpprio = data.priority == 10 ? 0 : data.priority == 99 ? 2 : 1;
     Add(new cMenuEditBitItem( tr("Active"),       &data.flags, tfActive));
     Add(new cMenuEditChanItem(tr("Channel"),      &channel));
     Add(new cMenuEditDateItem(tr("Day"),          &data.day, &data.weekdays));
     Add(new cMenuEditTimeItem(tr("Start"),        &data.start));
     Add(new cMenuEditTimeItem(tr("Stop"),         &data.stop));
     Add(new cMenuEditBitItem( tr("VPS"),          &data.flags, tfVps));
     Add(new cMenuEditStraItem(tr("Priority"),     &tmpprio, 3, PriorityTexts));
     Add(new cMenuEditIntItem( tr("Lifetime"),     &data.lifetime, 0, MAXLIFETIME, NULL, tr("unlimited")));
     //Add(new cMenuEditBoolItem(tr("Child protection"), &data.fskProtection));  // PIN PATCH
     // PIN PATCH
     if (cOsd::pinValid)
        Add(new cMenuEditBoolItem(tr("Child protection"),&data.fskProtection));
     else {
        char buf[64];
        snprintf(buf,64, "%s\t%s", tr("Child protection"), data.fskProtection ? tr("yes") : tr("no"));
        Add(new cOsdItem(buf));
        }

     Add(new cMenuEditStrItem( tr("File"),          data.file, sizeof(data.file), tr(FileNameChars)));
     SetFirstDayItem();
     }
  Timers.IncBeingEdited();
}

cMenuEditTimer::~cMenuEditTimer()
{
  if (timer && addIfConfirmed)
     delete timer; // apparently it wasn't confirmed
  Timers.DecBeingEdited();
}

void cMenuEditTimer::SetFirstDayItem(void)
{
  if (!firstday && !data.IsSingleEvent()) {
     Add(firstday = new cMenuEditDateItem(tr("First day"), &data.day));
     Display();
     }
  else if (firstday && data.IsSingleEvent()) {
     Del(firstday->Index());
     firstday = NULL;
     Display();
     }
}

eOSState cMenuEditTimer::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     {
                       cChannel *ch = Channels.GetByNumber(channel);
                       if (ch)
                          data.channel = ch;
                       else {
                          Skins.Message(mtError, tr("*** Invalid Channel ***"));
                          break;
                          }
                       data.priority = tmpprio == 0 ? 10 : tmpprio == 1 ? 50 : 99;
                       if (!*data.file)
                          strcpy(data.file, data.Channel()->ShortName(true));
                       if (timer) {
                          if (memcmp(timer, &data, sizeof(data)) != 0)
                             *timer = data;
                          if (addIfConfirmed)
                             if(!Timers.Add(timer))
                                Skins.Message(mtError, tr("Could not add timer"));
                          timer->SetEventFromSchedule();
                          timer->Matches();
                          Timers.SetModified();
                          isyslog("timer %s %s (%s)", *timer->ToDescr(), addIfConfirmed ? "added" : "modified", timer->HasFlags(tfActive) ? "active" : "inactive");
                          addIfConfirmed = false;
                          }
                     }
                     return osBack;
       case kRed:
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       default: break;
       }
     }
  if (Key != kNone)
     SetFirstDayItem();
  return state;
}

// --- cMenuTimerItem --------------------------------------------------------

class cMenuTimerItem : public cOsdItem {
private:
  cTimer *timer;
public:
  cMenuTimerItem(cTimer *Timer);
  virtual int Compare(const cListObject &ListObject) const;
  virtual void Set(void);
  cTimer *Timer(void) { return timer; }
  };

cMenuTimerItem::cMenuTimerItem(cTimer *Timer)
{
  timer = Timer;
  Set();
}

int cMenuTimerItem::Compare(const cListObject &ListObject) const
{
  return timer->Compare(*((cMenuTimerItem *)&ListObject)->timer);
}

void cMenuTimerItem::Set(void)
{
  cString day, name("");
  if (timer->WeekDays())
     day = timer->PrintDay(0, timer->WeekDays(), false);
  else if (timer->Day() - time(NULL) < 28 * SECSINDAY) {
     day = itoa(timer->GetMDay(timer->Day()));
     name = WeekDayName(timer->Day());
     }
  else {
     struct tm tm_r;
     time_t Day = timer->Day();
     localtime_r(&Day, &tm_r);
     char buffer[16];
     strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_r);
     day = buffer;
     }
  char *buffer = NULL;

#define TIMERMACRO_TITLE    "TITLE"
#define TIMERMACRO_EPISODE  "EPISODE"

  const char *Title = timer->Event() ? timer->Event()->Title() : "";
  const char *Subtitle = timer->Event() ? timer->Event()->ShortText() : "";

  const char *macroTITLE   = strstr(timer->File(), TIMERMACRO_TITLE);
  const char *macroEPISODE = strstr(timer->File(), TIMERMACRO_EPISODE);

  char *name_ = NULL;
  if (macroTITLE && macroEPISODE) {
     name_ = strdup(timer->File());
     name_ = strreplace(name_, TIMERMACRO_TITLE, Title ? Title : "");
     name_ = strreplace(name_, TIMERMACRO_EPISODE, Subtitle ? Subtitle : "");
     // avoid blanks at the end:
     int l = strlen(name_);
     while (l-- > 2) {
           if (name_[l] == ' ' && name_[l - 1] != '~')
              name_[l] = 0;
           else
              break;
           }
     }

  if (name_) {
  if (strcmp(Skins.Current()->Name(), "Reel") == 0) { // Here we want use channel-name instead of channel-number
      asprintf(&buffer, "%c\t%s\t%s%s%s\t%02d:%02d - %02d:%02d\t%s",
              !(timer->HasFlags(tfActive)) ? ' ' : timer->FirstDay() ? '!' : timer->Recording() ? '#' : '>',
              timer->Channel()->Name(),
              *name,
              *name && **name ? " " : "",
              *day,
              timer->Start() / 100,
              timer->Start() % 100,
              timer->Stop() / 100,
              timer->Stop() % 100,
              name_);
  } else {
      asprintf(&buffer, "%c\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
              !(timer->HasFlags(tfActive)) ? ' ' : timer->FirstDay() ? '!' : timer->Recording() ? '#' : '>',
              timer->Channel()->Number(),
              *name,
              *name && **name ? " " : "",
              *day,
              timer->Start() / 100,
              timer->Start() % 100,
              timer->Stop() / 100,
              timer->Stop() % 100,
              name_);
  }
  } else {
  if (strcmp(Skins.Current()->Name(), "Reel") == 0) { // Here we want use channel-name instead of channel-number
      asprintf(&buffer, "%c\t%s\t%s%s%s\t%02d:%02d - %02d:%02d\t%s",
              !(timer->HasFlags(tfActive)) ? ' ' : timer->FirstDay() ? '!' : timer->Recording() ? '#' : '>',
              timer->Channel()->Name(),
              *name,
              *name && **name ? " " : "",
              *day,
              timer->Start() / 100,
              timer->Start() % 100,
              timer->Stop() / 100,
              timer->Stop() % 100,
              timer->File());
  } else {
      asprintf(&buffer, "%c\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
              !(timer->HasFlags(tfActive)) ? ' ' : timer->FirstDay() ? '!' : timer->Recording() ? '#' : '>',
              timer->Channel()->Number(),
              *name,
              *name && **name ? " " : "",
              *day,
              timer->Start() / 100,
              timer->Start() % 100,
              timer->Stop() / 100,
              timer->Stop() % 100,
              timer->File());
  }
  }
  free(name_);
  SetText(buffer, false);
}

// --- cMenuTimers -----------------------------------------------------------

class cMenuTimers : public cOsdMenu {
private:
  int helpKeys;
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  eOSState OnOff(void);
  eOSState Info(void);
  cTimer *CurrentTimer(void);
  void SetHelpKeys(void);
public:
  cMenuTimers(void);
  virtual ~cMenuTimers();
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuTimers::cMenuTimers(void)
:cOsdMenu(tr("Timers"), 2, CHNUMWIDTH, 10, 6, 6)
{
  helpKeys = -1;
  for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
      timer->SetEventFromSchedule(); // make sure the event is current
      Add(new cMenuTimerItem(timer));
      }
  Sort();
  SetCurrent(First());
  SetHelpKeys();
  Timers.IncBeingEdited();
}

cMenuTimers::~cMenuTimers()
{
  Timers.DecBeingEdited();
}

cTimer *cMenuTimers::CurrentTimer(void)
{
  cMenuTimerItem *item = (cMenuTimerItem *)Get(Current());
  return item ? item->Timer() : NULL;
}

void cMenuTimers::SetHelpKeys(void)
{
  int NewHelpKeys = 0;
  cTimer *timer = CurrentTimer();
  if (timer) {
     if (timer->Event())
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     helpKeys = NewHelpKeys;
     SetHelp(helpKeys > 0 ? tr("Button$On/Off") : NULL, tr("Button$New"), helpKeys > 0 ? tr("Button$Delete") : NULL, helpKeys == 2 ? tr("Button$Info") : NULL);
     }
}

eOSState cMenuTimers::OnOff(void)
{
  if (HasSubMenu())
     return osContinue;
  cTimer *timer = CurrentTimer();
  if (timer) {
     timer->OnOff();
     timer->SetEventFromSchedule();
     RefreshCurrent();
     DisplayCurrent(true);
     if (timer->FirstDay())
        isyslog("timer %s first day set to %s", *timer->ToDescr(), *timer->PrintFirstDay());
     else
        isyslog("timer %s %sactivated", *timer->ToDescr(), timer->HasFlags(tfActive) ? "" : "de");
     Timers.SetModified();
     }
  return osContinue;
}

eOSState cMenuTimers::Edit(void)
{
    struct Epgsearch_exttimeredit_v1_0
    {
     // in
        cTimer* timer;             // pointer to the timer to edit
        bool bNew;                 // flag that indicates, if this is a new timer or an existing one
        const cEvent* event;       // pointer to the event corresponding to this timer (may be NULL)
     // out
        cOsdMenu* pTimerMenu;      // pointer to the menu of results
    };


  if (HasSubMenu() || Count() == 0)
     return osContinue;
  isyslog("editing timer %s", *CurrentTimer()->ToDescr());

  /* RC: prepared for epgsearchs timeredit. currently has the prob that timers are only
         updated when menu ist closed
  cPlugin *p = cPluginManager::GetPlugin("epgsearch");
  if (p) {
      Epgsearch_exttimeredit_v1_0 serviceData;
      serviceData.timer = CurrentTimer();
      serviceData.bNew = false;
      serviceData.event = NULL;

      p->Service("Epgsearch-exttimeredit-v1.0", &serviceData);
      if (serviceData.pTimerMenu)
          return AddSubMenu(serviceData.pTimerMenu);
      else
          Skins.Message(mtError, tr("This version of EPGSearch does not support this service!"));
  }
  */
  return AddSubMenu(new cMenuEditTimer(CurrentTimer()));
}

eOSState cMenuTimers::New(void)
{
    struct Epgsearch_exttimeredit_v1_0
    {
        // in
        cTimer* timer;             // pointer to the timer to edit
        bool bNew;                 // flag that indicates, if this is a new timer or an existing one
        const cEvent* event;       // pointer to the event corresponding to this timer (may be NULL)
        // out
        cOsdMenu* pTimerMenu;      // pointer to the menu of results
    };

  if (HasSubMenu())
     return osContinue;

  /*
  cPlugin *p = cPluginManager::GetPlugin("epgsearch");
  if (p) {
      Epgsearch_exttimeredit_v1_0 serviceData;
      serviceData.timer = new cTimer;
      serviceData.bNew = true;
      serviceData.event = NULL;

      p->Service("Epgsearch-exttimeredit-v1.0", &serviceData);
      if (serviceData.pTimerMenu)
          return AddSubMenu(serviceData.pTimerMenu);
      else
          Skins.Message(mtError, tr("This version of EPGSearch does not support this service!"));
  }
  */
  return AddSubMenu(new cMenuEditTimer(new cTimer, true));
}

eOSState cMenuTimers::Delete(void)
{
  // Check if this timer is active:
  cTimer *ti = CurrentTimer();
  if (ti) {
     if (Interface->Confirm(tr("Delete timer?"))) {
        if (ti->Recording()) {
           if (Interface->Confirm(tr("Timer still recording - really delete?"))) {
              ti->Skip();
              cRecordControls::Process(time(NULL));
              }
           else
              return osContinue;
           }
        isyslog("deleting timer %s", *ti->ToDescr());
        if(!Timers.Del(ti))
            Skins.Message(mtError, tr("Could not delete timer"));
        cOsdMenu::Del(Current());
        Timers.SetModified();
        Display();
        }
     }
  return osContinue;
}

eOSState cMenuTimers::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cTimer *ti = CurrentTimer();
  if (ti && ti->Event())
     return AddSubMenu(new cMenuEvent(ti->Event()));
  return osContinue;
}

eOSState cMenuTimers::ProcessKey(eKeys Key)
{
  int TimerNumber = HasSubMenu() ? Count() : -1;
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Edit();
       case kRed:    state = OnOff(); break; // must go through SetHelpKeys()!
       case kGreen:  return New();
       case kYellow: state = Delete(); break;
       case kBlue:   return Info();
                     break;
       default: break;
       }
     }
  if (TimerNumber >= 0 && !HasSubMenu() && Timers.Get(TimerNumber)) {
     // a newly created timer was confirmed with Ok
     Add(new cMenuTimerItem(Timers.Get(TimerNumber)), true);
     Display();
     }
  if (Key != kNone)
     SetHelpKeys();
  return state;
}

// --- cMenuEvent ------------------------------------------------------------

cMenuEvent::cMenuEvent(const cEvent *Event, bool CanSwitch, bool Buttons)
:cOsdMenu(tr("Event"))
{
  event = Event;
  if (event) {
     cChannel *channel = Channels.GetByChannelID(event->ChannelID(), true);
     if (channel) {
        SetTitle(channel->Name());
        int TimerMatch = tmNone;
        Timers.GetMatch(event, &TimerMatch);
        if (Buttons)
           SetHelp(TimerMatch == tmFull ? tr("Button$Timer") : tr("Button$Record"), NULL, NULL, CanSwitch ? tr("Button$Switch") : NULL);
        }
     }
}

void cMenuEvent::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetEvent(event);
  if (event->Description())
     cStatus::MsgOsdTextItem(event->Description());
}

eOSState cMenuEvent::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kGreen:
       case kYellow: return osContinue;
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}
// --- cMenuActiveEvent ------------------------------------------------------------

cMenuActiveEvent::cMenuActiveEvent()
:cOsdMenu(tr("ActiveEvent"))
{
  event = NULL;
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (channel) {
     SetTitle(channel->Name());
     cSchedulesLock schedulesLock;
     const cSchedules *schedules = cSchedules::Schedules(schedulesLock);
     if (schedules) {
        const cSchedule *schedule = schedules->GetSchedule(channel);
        if (schedule) {
           event = schedule->GetPresentEvent();
           }
        }
     SetHelpButtons();
     }
}

void cMenuActiveEvent::Display(void)
{
  cOsdMenu::Display();
  if (event) {
    DisplayMenu()->SetEvent(event);
    if (event->Description())
       cStatus::MsgOsdTextItem(event->Description());
    }
}

void cMenuActiveEvent::SetHelpButtons(void)
{
  int timerMatch = tmNone;
  Timers.GetMatch(event, &timerMatch);
  if (event)
     SetHelp(timerMatch == tmFull ? tr("Button$Timer") : tr("Button$Record"), NULL, NULL, NULL);
  else
     SetHelp(NULL);

}

eOSState cMenuActiveEvent::Record(void)
{
   int timerMatch = tmNone;
   Timers.GetMatch(event, &timerMatch);

   if (timerMatch == tmFull) {
     int tm = tmNone;
     cTimer *timer = Timers.GetMatch(event, &tm);
     if (timer)
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     cTimer *timer = new cTimer(event);
     cTimer *t = Timers.GetTimer(timer);
     if (t) {
        delete timer;
        timer = t;
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     else {
        if(!Timers.Add(timer))
            Skins.Message(mtError, tr("Could not add timer"));
        Timers.SetModified();
        isyslog("timer %s added (active)", *timer->ToDescr());
        if (timer->Matches(0, false, NEWTIMERLIMIT))
           return AddSubMenu(new cMenuEditTimer(timer));
        if (HasSubMenu())
           CloseSubMenu();
        }

  return osContinue;
}

eOSState cMenuActiveEvent::ProcessKey(eKeys Key)
{
  bool hadSubMenu = HasSubMenu();

  if (!hadSubMenu) {
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    case kBack:   return osEnd;
    default: break;
    }
  }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed: return Record();
       case kGreen:
       case kYellow: return osContinue;
       case kOk:     return osEnd;
       default: break;
       }
     }

  if (!hadSubMenu)
      SetHelpButtons();
  return state;

}

// --- cMenuScheduleItem -----------------------------------------------------

class cMenuScheduleItem : public cOsdItem {
public:
  enum eScheduleSortMode { ssmAllThis, ssmThisThis, ssmThisAll, ssmAllAll }; // "which event(s) on which channel(s)"
private:
  static eScheduleSortMode sortMode;
public:
  const cEvent *event;
  const cChannel *channel;
  bool withDate;
  int timerMatch;
  cMenuScheduleItem(const cEvent *Event, cChannel *Channel = NULL, bool WithDate = false);
  static void SetSortMode(eScheduleSortMode SortMode) { sortMode = SortMode; }
  static void IncSortMode(void) { sortMode = eScheduleSortMode((sortMode == ssmAllAll) ? ssmAllThis : sortMode + 1); }
  static eScheduleSortMode SortMode(void) { return sortMode; }
  virtual int Compare(const cListObject &ListObject) const;
  bool Update(bool Force = false);
  };

cMenuScheduleItem::eScheduleSortMode cMenuScheduleItem::sortMode = ssmAllThis;

cMenuScheduleItem::cMenuScheduleItem(const cEvent *Event, cChannel *Channel, bool WithDate)
{
  event = Event;
  channel = Channel;
  withDate = WithDate;
  timerMatch = tmNone;
  Update(true);
}

int cMenuScheduleItem::Compare(const cListObject &ListObject) const
{
  cMenuScheduleItem *p = (cMenuScheduleItem *)&ListObject;
  int r = -1;
  if (sortMode != ssmAllThis)
     r = strcoll(event->Title(), p->event->Title());
  if (sortMode == ssmAllThis || r == 0)
     r = event->StartTime() - p->event->StartTime();
  return r;
}

static const char *TimerMatchChars = " tT";

bool cMenuScheduleItem::Update(bool Force)
{
  bool result = false;
  int OldTimerMatch = timerMatch;
  Timers.GetMatch(event, &timerMatch);
  if (Force || timerMatch != OldTimerMatch) {
     cString buffer;
     char t = TimerMatchChars[timerMatch];
     char v = event->Vps() && (event->Vps() - event->StartTime()) ? 'V' : ' ';
     char r = event->SeenWithin(30) && event->IsRunning() ? '*' : ' ';
     const char *csn = channel ? channel->ShortName(true) : NULL;
     cString eds = event->GetDateString();
     if (channel && withDate)
        buffer = cString::sprintf("%d\t%.*s\t%.*s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
     else if (channel)
        buffer = cString::sprintf("%d\t%.*s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), t, v, r, event->Title());
     else
        buffer = cString::sprintf("%.*s\t%s\t%c%c%c\t%s", Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
     SetText(buffer);
     result = true;
     }
  return result;
}

// --- cMenuWhatsOn ----------------------------------------------------------

class cMenuWhatsOn : public cOsdMenu {
private:
  bool now;
  int helpKeys;
  int timerState;
  eOSState Record(void);
  eOSState Switch(void);
  static int currentChannel;
  static const cEvent *scheduleEvent;
  bool Update(void);
  void SetHelpKeys(void);
public:
  cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr);
  static int CurrentChannel(void) { return currentChannel; }
  static void SetCurrentChannel(int ChannelNr) { currentChannel = ChannelNr; }
  static const cEvent *ScheduleEvent(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

int cMenuWhatsOn::currentChannel = 0;
const cEvent *cMenuWhatsOn::scheduleEvent = NULL;

cMenuWhatsOn::cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr)
:cOsdMenu(Now ? tr("What's on now?") : tr("What's on next?"), CHNUMWIDTH, 7, 6, 4)
{
  now = Now;
  helpKeys = -1;
  timerState = 0;
  Timers.Modified(timerState);
  for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
      if (!Channel->GroupSep()) {
         const cSchedule *Schedule = Schedules->GetSchedule(Channel);
         if (Schedule) {
            const cEvent *Event = Now ? Schedule->GetPresentEvent() : Schedule->GetFollowingEvent();
            if (Event)
               Add(new cMenuScheduleItem(Event, Channel), Channel->Number() == CurrentChannelNr);
            }
         }
      }
  currentChannel = CurrentChannelNr;
  Display();
  SetHelpKeys();
}

bool cMenuWhatsOn::Update(void)
{
  bool result = false;
  if (Timers.Modified(timerState)) {
     for (cOsdItem *item = First(); item; item = Next(item)) {
         if (((cMenuScheduleItem *)item)->Update())
            result = true;
         }
     }
  return result;
}

void cMenuWhatsOn::SetHelpKeys(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  int NewHelpKeys = 0;
  if (item) {
     if (item->timerMatch == tmFull)
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     const char *Red[] = { NULL, tr("Button$Record"), tr("Button$Timer") };
     SetHelp(Red[NewHelpKeys], now ? tr("Button$Next") : tr("Button$Now"), tr("Button$Schedule"), tr("Button$Switch"));
     helpKeys = NewHelpKeys;
     }
}

const cEvent *cMenuWhatsOn::ScheduleEvent(void)
{
  const cEvent *ei = scheduleEvent;
  scheduleEvent = NULL;
  return ei;
}

eOSState cMenuWhatsOn::Switch(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     cChannel *channel = Channels.GetByChannelID(item->event->ChannelID(), true);
     if (channel && cDevice::PrimaryDevice()->SwitchChannel(channel, true))
        return osEnd;
     }
  Skins.Message(mtError, tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuWhatsOn::Record(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     if (item->timerMatch == tmFull) {
        int tm = tmNone;
        cTimer *timer = Timers.GetMatch(item->event, &tm);
        if (timer)
           return AddSubMenu(new cMenuEditTimer(timer));
        }
     cTimer *timer = new cTimer(item->event);
     cTimer *t = Timers.GetTimer(timer);
     if (t) {
        delete timer;
        timer = t;
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     else {
        if(!Timers.Add(timer))
            Skins.Message(mtError, tr("Could not add timer"));
        Timers.SetModified();
        isyslog("timer %s added (active)", *timer->ToDescr());
        if (timer->Matches(0, false, NEWTIMERLIMIT))
           return AddSubMenu(new cMenuEditTimer(timer));
        if (HasSubMenu())
           CloseSubMenu();
        if (Update())
           Display();
        SetHelpKeys();
        }
     }
  return osContinue;
}

eOSState cMenuWhatsOn::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRecord:
       case kRed:    return Record();
       case kYellow: state = osBack;
                     // continue with kGreen
       case kGreen:  {
                       cMenuScheduleItem *mi = (cMenuScheduleItem *)Get(Current());
                       if (mi) {
                          scheduleEvent = mi->event;
                          currentChannel = mi->channel->Number();
                          }
                     }
                     break;
       case kBlue:   return Switch();
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuScheduleItem *)Get(Current()))->event, true, true));
                     break;
       default:      break;
       }
     }
  else if (!HasSubMenu()) {
     if (HadSubMenu && Update())
        Display();
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}

// --- cMenuSchedule ---------------------------------------------------------

class cMenuSchedule : public cOsdMenu {
private:
  cSchedulesLock schedulesLock;
  const cSchedules *schedules;
  bool now, next;
  int otherChannel;
  int helpKeys;
  int timerState;
  eOSState Number(void);
  eOSState Record(void);
  eOSState Switch(void);
  void PrepareScheduleAllThis(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleThisThis(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleThisAll(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleAllAll(const cEvent *Event, const cChannel *Channel);
  bool Update(void);
  void SetHelpKeys(void);
public:
  cMenuSchedule(void);
  virtual ~cMenuSchedule();
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSchedule::cMenuSchedule(void)
:cOsdMenu("")
{
  now = next = false;
  otherChannel = 0;
  helpKeys = -1;
  timerState = 0;
  Timers.Modified(timerState);
  cMenuScheduleItem::SetSortMode(cMenuScheduleItem::ssmAllThis);
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (channel) {
     cMenuWhatsOn::SetCurrentChannel(channel->Number());
     schedules = cSchedules::Schedules(schedulesLock);
     PrepareScheduleAllThis(NULL, channel);
     SetHelpKeys();
     }
}

cMenuSchedule::~cMenuSchedule()
{
  cMenuWhatsOn::ScheduleEvent(); // makes sure any posted data is cleared
}

void cMenuSchedule::PrepareScheduleAllThis(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(7, 6, 4);
  char *buffer = NULL;
  asprintf(&buffer, tr("Schedule - %s"), Channel->Name());
  SetTitle(buffer);
  free(buffer);
  if (schedules && Channel) {
     const cSchedule *Schedule = schedules->GetSchedule(Channel);
     if (Schedule) {
        const cEvent *PresentEvent = Event ? Event : Schedule->GetPresentEvent();
        time_t now = time(NULL) - Setup.EPGLinger * 60;
        for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
            if (ev->EndTime() > now || ev == PresentEvent)
               Add(new cMenuScheduleItem(ev), ev == PresentEvent);
            }
        }
     }
}

void cMenuSchedule::PrepareScheduleThisThis(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(7, 6, 4);
  char *buffer = NULL;
  asprintf(&buffer, tr("This event - %s"), Channel->Name());
  SetTitle(buffer);
  free(buffer);
  if (schedules && Channel && Event) {
     const cSchedule *Schedule = schedules->GetSchedule(Channel);
     if (Schedule) {
        time_t now = time(NULL) - Setup.EPGLinger * 60;
        for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
            if ((ev->EndTime() > now || ev == Event) && !strcmp(ev->Title(), Event->Title()))
               Add(new cMenuScheduleItem(ev), ev == Event);
            }
        }
     }
}

void cMenuSchedule::PrepareScheduleThisAll(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(CHNUMWIDTH, 7, 7, 6, 4);
  SetTitle(tr("This event - all channels"));
  if (schedules && Event) {
     for (cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch)) {
         const cSchedule *Schedule = schedules->GetSchedule(ch);
         if (Schedule) {
            time_t now = time(NULL) - Setup.EPGLinger * 60;
            for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
                if ((ev->EndTime() > now || ev == Event) && !strcmp(ev->Title(), Event->Title()))
                   Add(new cMenuScheduleItem(ev, ch, true), ev == Event && ch == Channel);
                }
            }
         }
     }
}

void cMenuSchedule::PrepareScheduleAllAll(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(CHNUMWIDTH, 7, 7, 6, 4);
  SetTitle(tr("All events - all channels"));
  if (schedules) {
     for (cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch)) {
         const cSchedule *Schedule = schedules->GetSchedule(ch);
         if (Schedule) {
            time_t now = time(NULL) - Setup.EPGLinger * 60;
            for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
                if (ev->EndTime() > now || ev == Event)
                   Add(new cMenuScheduleItem(ev, ch, true), ev == Event && ch == Channel);
                }
            }
         }
     }
}

bool cMenuSchedule::Update(void)
{
  bool result = false;
  if (Timers.Modified(timerState)) {
     for (cOsdItem *item = First(); item; item = Next(item)) {
         if (((cMenuScheduleItem *)item)->Update())
            result = true;
         }
     }
  return result;
}

void cMenuSchedule::SetHelpKeys(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  int NewHelpKeys = 0;
  if (item) {
     if (item->timerMatch == tmFull)
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     const char *Red[] = { NULL, tr("Button$Record"), tr("Button$Timer") };
     SetHelp(Red[NewHelpKeys], tr("Button$Now"), tr("Button$Next"));
     helpKeys = NewHelpKeys;
     }
}

eOSState cMenuSchedule::Number(void)
{
  cMenuScheduleItem::IncSortMode();
  cMenuScheduleItem *CurrentItem = (cMenuScheduleItem *)Get(Current());
  const cChannel *Channel = NULL;
  const cEvent *Event = NULL;
  if (CurrentItem) {
     Event = CurrentItem->event;
     Channel = Channels.GetByChannelID(Event->ChannelID(), true);
     }
  else
     Channel = Channels.GetByNumber(cDevice::CurrentChannel());
  switch (cMenuScheduleItem::SortMode()) {
    case cMenuScheduleItem::ssmAllThis:  PrepareScheduleAllThis(Event, Channel); break;
    case cMenuScheduleItem::ssmThisThis: PrepareScheduleThisThis(Event, Channel); break;
    case cMenuScheduleItem::ssmThisAll:  PrepareScheduleThisAll(Event, Channel); break;
    case cMenuScheduleItem::ssmAllAll:   PrepareScheduleAllAll(Event, Channel); break;
    }
  CurrentItem = (cMenuScheduleItem *)Get(Current());
  Sort();
  SetCurrent(CurrentItem);
  Display();
  return osContinue;
}

eOSState cMenuSchedule::Record(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     if (item->timerMatch == tmFull) {
        int tm = tmNone;
        cTimer *timer = Timers.GetMatch(item->event, &tm);
        if (timer)
           return AddSubMenu(new cMenuEditTimer(timer));
        }
     cTimer *timer = new cTimer(item->event);
     cTimer *t = Timers.GetTimer(timer);
     if (t) {
        delete timer;
        timer = t;
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     else {
        if(!Timers.Add(timer))
            Skins.Message(mtError, tr("Could not add timer"));
        Timers.SetModified();
        isyslog("timer %s added (active)", *timer->ToDescr());
        if (timer->Matches(0, false, NEWTIMERLIMIT))
           return AddSubMenu(new cMenuEditTimer(timer));
        if (HasSubMenu())
           CloseSubMenu();
        if (Update())
           Display();
        SetHelpKeys();
        }
     }
  return osContinue;
}

eOSState cMenuSchedule::Switch(void)
{
  if (otherChannel) {
     if (Channels.SwitchTo(otherChannel))
        return osEnd;
     }
  Skins.Message(mtError, tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuSchedule::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case k0:      return Number();
       case kRecord:
       case kRed:    return Record();
       case kGreen:  if (schedules) {
                        if (!now && !next) {
                           int ChannelNr = 0;
                           if (Count()) {
                              cChannel *channel = Channels.GetByChannelID(((cMenuScheduleItem *)Get(Current()))->event->ChannelID(), true);
                              if (channel)
                                 ChannelNr = channel->Number();
                              }
                           now = true;
                           return AddSubMenu(new cMenuWhatsOn(schedules, true, ChannelNr));
                           }
                        now = !now;
                        next = !next;
                        return AddSubMenu(new cMenuWhatsOn(schedules, now, cMenuWhatsOn::CurrentChannel()));
                        }
       case kYellow: if (schedules)
                        return AddSubMenu(new cMenuWhatsOn(schedules, false, cMenuWhatsOn::CurrentChannel()));
                     break;
       case kBlue:   if (Count() && otherChannel)
                        return Switch();
                     break;
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuScheduleItem *)Get(Current()))->event, otherChannel, true));
                     break;
       default:      break;
       }
     }
  else if (!HasSubMenu()) {
     now = next = false;
     const cEvent *ei = cMenuWhatsOn::ScheduleEvent();
     if (ei) {
        cChannel *channel = Channels.GetByChannelID(ei->ChannelID(), true);
        if (channel) {
           cMenuScheduleItem::SetSortMode(cMenuScheduleItem::ssmAllThis);
           PrepareScheduleAllThis(NULL, channel);
           if (channel->Number() != cDevice::CurrentChannel()) {
              otherChannel = channel->Number();
              SetHelp(Count() ? tr("Button$Record") : NULL, tr("Button$Now"), tr("Button$Next"), tr("Button$Switch"));
              }
           Display();
           }
        }
     else if (HadSubMenu && Update())
        Display();
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}

// --- cMenuCommands ---------------------------------------------------------

class cMenuCommands : public cOsdMenu {
private:
  cCommands *commands;
  char *parameters;
  eOSState Execute(void);
public:
  cMenuCommands(const char *Title, cCommands *Commands, const char *Parameters = NULL);
  virtual ~cMenuCommands();
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuCommands::cMenuCommands(const char *Title, cCommands *Commands, const char *Parameters)
:cOsdMenu(Title)
{
  SetHasHotkeys();
  commands = Commands;
  parameters = Parameters ? strdup(Parameters) : NULL;
  for (cCommand *command = commands->First(); command; command = commands->Next(command))
      Add(new cOsdItem(hk(command->Title())));
}

cMenuCommands::~cMenuCommands()
{
  free(parameters);
}

eOSState cMenuCommands::Execute(void)
{
  cCommand *command = commands->Get(Current());
  if (command) {
     char *buffer = NULL;
     bool confirmed = true;
     if (command->Confirm()) {
        asprintf(&buffer, "%s?", command->Title());
        confirmed = Interface->Confirm(buffer);
        free(buffer);
        }
     if (confirmed) {
        asprintf(&buffer, "%s...", command->Title());
        Skins.Message(mtStatus, buffer);
        free(buffer);
        const char *Result = command->Execute(parameters);
        Skins.Message(mtStatus, NULL);
        if (Result)
           return AddSubMenu(new cMenuText(command->Title(), Result, fontFix));
        return osEnd;
        }
     }
  return osContinue;
}

eOSState cMenuCommands::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       case kOk:     return Execute();
       default:      break;
       }
     }
  return state;
}

// --- cMenuCam --------------------------------------------------------------

cMenuCam::cMenuCam(cCiMenu *CiMenu)
:cOsdMenu("")
{
  dsyslog("CAM: Menu ------------------");
  ciMenu = CiMenu;
  selected = false;
  offset = 0;
  if (ciMenu->Selectable())
     SetHasHotkeys();
  SetTitle(*ciMenu->TitleText() ? ciMenu->TitleText() : "CAM");
  dsyslog("CAM: '%s'", ciMenu->TitleText());
  if (*ciMenu->SubTitleText()) {
     dsyslog("CAM: '%s'", ciMenu->SubTitleText());
     AddMultiLineItem(ciMenu->SubTitleText());
     offset = Count();
     }
  for (int i = 0; i < ciMenu->NumEntries(); i++) {
      Add(new cOsdItem(hk(ciMenu->Entry(i)), osUnknown, ciMenu->Selectable()));
      dsyslog("CAM: '%s'", ciMenu->Entry(i));
      }
  if (*ciMenu->BottomText()) {
     AddMultiLineItem(ciMenu->BottomText());
     dsyslog("CAM: '%s'", ciMenu->BottomText());
     }
  Display();
}

cMenuCam::~cMenuCam()
{
  if (!selected)
     ciMenu->Abort();
  delete ciMenu;
}

void cMenuCam::AddMultiLineItem(const char *s)
{
  while (s && *s) {
        const char *p = strchr(s, '\n');
        int l = p ? p - s : strlen(s);
        cOsdItem *item = new cOsdItem;
        item->SetSelectable(false);
        item->SetText(strndup(s, l), false);
        Add(item);
        s = p ? p + 1 : p;
        }
}

eOSState cMenuCam::Select(void)
{
  if (ciMenu->Selectable()) {
     ciMenu->Select(Current() - offset);
     dsyslog("CAM: select %d", Current() - offset);
     }
  else
     ciMenu->Cancel();
  selected = true;
  return osEnd;
}

eOSState cMenuCam::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Select();
       default: break;
       }
     }
  else if (state == osBack) {
     ciMenu->Cancel();
     selected = true;
     return osEnd;
     }
  if (ciMenu->HasUpdate()) {
     selected = true;
     return osEnd;
     }
  return state;
}

// --- cMenuCamEnquiry -------------------------------------------------------

cMenuCamEnquiry::cMenuCamEnquiry(cCiEnquiry *CiEnquiry)
:cOsdMenu("", 1)
{
  ciEnquiry = CiEnquiry;
  int Length = ciEnquiry->ExpectedLength();
  input = MALLOC(char, Length + 1);
  *input = 0;
  replied = false;
  SetTitle("CAM");
  Add(new cOsdItem(ciEnquiry->Text(), osUnknown, false));
  Add(new cOsdItem("", osUnknown, false));
  Add(new cMenuEditNumItem("", input, Length, ciEnquiry->Blind()));
  Display();
}

cMenuCamEnquiry::~cMenuCamEnquiry()
{
  if (!replied)
     ciEnquiry->Abort();
  free(input);
  delete ciEnquiry;
}

eOSState cMenuCamEnquiry::Reply(void)
{
  if (ciEnquiry->ExpectedLength() < 0xFF && int(strlen(input)) != ciEnquiry->ExpectedLength()) {
     char buffer[64];
     snprintf(buffer, sizeof(buffer), tr("Please enter %d digits!"), ciEnquiry->ExpectedLength());
     Skins.Message(mtError, buffer);
     return osContinue;
     }
  ciEnquiry->Reply(input);
  replied = true;
  return osEnd;
}

eOSState cMenuCamEnquiry::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Reply();
       default: break;
       }
     }
  else if (state == osBack) {
     ciEnquiry->Cancel();
     replied = true;
     return osEnd;
     }
  return state;
}

// --- CamControl ------------------------------------------------------------

cOsdObject *CamControl(void)
{
  for (int d = 0; d < cDevice::NumDevices(); d++) {
      cDevice *Device = cDevice::GetDevice(d);
      if (Device) {
         cCiHandler *CiHandler = Device->CiHandler();
         if (CiHandler && CiHandler->HasUserIO()) {
            cCiMenu *CiMenu = CiHandler->GetMenu();
            if (CiMenu)
               return new cMenuCam(CiMenu);
            else {
               cCiEnquiry *CiEnquiry = CiHandler->GetEnquiry();
               if (CiEnquiry)
                  return new cMenuCamEnquiry(CiEnquiry);
               }
            }
         }
      }
  return NULL;
}

// --- cMenuRecording --------------------------------------------------------

class cMenuRecording : public cOsdMenu {
private:
  const cRecording *recording;
  bool withButtons;

  bool CheckForPicture(const cRecording *Recording);
  bool bPictureAvailable;
  bool bPicIsZoomed;
public:
  cMenuRecording(const cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};

bool cMenuRecording::CheckForPicture(const cRecording *Recording)
{
  std::string PngPath = Recording->FileName();
  PngPath += "/vdr.png";
  struct stat results;
  if ( stat ( PngPath.c_str(), &results ) == 0 )
    return(true);
  else
    return false;
}

cMenuRecording::cMenuRecording(const cRecording *Recording, bool WithButtons)
:cOsdMenu(tr("Recording info"))
{
  recording = Recording;
  withButtons = WithButtons;

  bPictureAvailable = CheckForPicture(Recording);

  if (withButtons)
     SetHelp(tr("Button$Play"), tr("Button$Rewind"), bPictureAvailable ? tr("Scale Picture") : NULL);
  else if(bPictureAvailable)
     SetHelp(NULL, NULL, tr("Scale Picture"));

  bPicIsZoomed = false;
}

void cMenuRecording::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetRecording(recording);
  if (recording->Info()->Description())
     cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState cMenuRecording::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    if (withButtons)
                        Key = kOk; // will play the recording, even if recording commands are defined
       case kGreen:  if (!withButtons)
                        break;
                     cRemote::Put(Key, true);
                     // continue with osBack to close the info menu and process the key
      case kYellow:
	{
	    if(bPictureAvailable)
	    {
	    	if(bPicIsZoomed)
		{
		    Display(); // Repaint -> Clear zoomed picture
		    bPicIsZoomed = false;
		} else
		{
		    	// Construct Recording entry with information necessary to display Picture for recording
		    	// Will be displayed in zoomed mode
    		    	cRecording *rec = new cRecording(recording->FileName(), recording->BaseDir());
    		    	rec->SetTitle("ZOOMPIC$", recording->Info()->Title());
    		    	DisplayMenu()->SetRecording(rec);

    		    	delete rec;
		    	state = osContinue;
		    	bPicIsZoomed = true;
		}
	    }
	    break;
	}
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}

// --- cMenuRecordingItem ----------------------------------------------------

class cMenuRecordingItem : public cOsdItem {
private:
  char *fileName;
  char *name;
  int totalEntries, newEntries;
public:
  cMenuRecordingItem(cRecording *Recording, int Level);
  ~cMenuRecordingItem();
  void IncrementCounter(bool New);
  const char *Name(void) { return name; }
  const char *FileName(void) { return fileName; }
  bool IsDirectory(void) { return name != NULL; }
  };

cMenuRecordingItem::cMenuRecordingItem(cRecording *Recording, int Level)
{
  fileName = strdup(Recording->FileName());
  name = NULL;
  totalEntries = newEntries = 0;
  SetText(Recording->Title('\t', true, Level));
  if (*Text() == '\t')
     name = strdup(Text() + 2); // 'Text() + 2' to skip the two '\t'
}

cMenuRecordingItem::~cMenuRecordingItem()
{
  free(fileName);
  free(name);
}

void cMenuRecordingItem::IncrementCounter(bool New)
{
  totalEntries++;
  if (New)
     newEntries++;
  char *buffer = NULL;
  asprintf(&buffer, "%d\t%d\t%s", totalEntries, newEntries, name);
  SetText(buffer, false);
}

// --- cMenuRecordings -------------------------------------------------------

cMenuRecordings::cMenuRecordings(const char *Base, int Level, bool OpenSubMenus)
:cOsdMenu(Base ? Base : tr("Recordings"), 9, 7)
{
  base = Base ? strdup(Base) : NULL;
  level = Setup.RecordingDirs ? Level : -1;
  Recordings.StateChanged(recordingsState); // just to get the current state
  helpKeys = -1;
  Display(); // this keeps the higher level menus from showing up briefly when pressing 'Back' during replay
  Set();
  if (Current() < 0)
     SetCurrent(First());
  else if (OpenSubMenus && cReplayControl::LastReplayed() && Open(true))
     return;
  Display();
  SetHelpKeys();
}

cMenuRecordings::~cMenuRecordings()
{
  helpKeys = -1;
  free(base);
}

void cMenuRecordings::SetHelpKeys(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  int NewHelpKeys = 0;
  if (ri) {
     if (ri->IsDirectory())
        NewHelpKeys = 1;
     else {
        NewHelpKeys = 2;
        cRecording *recording = GetRecording(ri);
        if (recording && recording->Info()->Title())
           NewHelpKeys = 3;
        }
     }
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(tr("Button$Open")); break;
       case 2:
       case 3: SetHelp(RecordingCommands.Count() ? tr("Commands") : tr("Button$Play"), tr("Button$Rewind"), tr("Button$Delete"), NewHelpKeys == 3 ? tr("Button$Info") : NULL);
       }
     helpKeys = NewHelpKeys;
     }
}

void cMenuRecordings::Set(bool Refresh)
{
  const char *CurrentRecording = cReplayControl::LastReplayed();
  cMenuRecordingItem *LastItem = NULL;
  char *LastItemText = NULL;
  cThreadLock RecordingsLock(&Recordings);
  if (Refresh) {
     cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
     if (ri) {
        cRecording *Recording = Recordings.GetByName(ri->FileName());
        if (Recording)
           CurrentRecording = Recording->FileName();
        }
     }
  Clear();
  Recordings.Sort();
  for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
      if (!base || (strstr(recording->Name(), base) == recording->Name() && recording->Name()[strlen(base)] == '~')) {
         cMenuRecordingItem *Item = new cMenuRecordingItem(recording, level);
         if (*Item->Text() && (!LastItem || strcmp(Item->Text(), LastItemText) != 0)) {
            Add(Item);
            LastItem = Item;
            free(LastItemText);
            LastItemText = strdup(LastItem->Text()); // must use a copy because of the counters!
            }
         else
            delete Item;
         if (LastItem) {
            if (CurrentRecording && strcmp(CurrentRecording, recording->FileName()) == 0)
               SetCurrent(LastItem);
            if (LastItem->IsDirectory())
               LastItem->IncrementCounter(recording->IsNew());
            }
         }
      }
  free(LastItemText);
  if (Refresh)
     Display();
}

cRecording *cMenuRecordings::GetRecording(cMenuRecordingItem *Item)
{
  cRecording *recording = Recordings.GetByName(Item->FileName());
  if (!recording)
     Skins.Message(mtError, tr("Error while accessing recording!"));
  return recording;
}

bool cMenuRecordings::Open(bool OpenSubMenus)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && ri->IsDirectory()) {
     const char *t = ri->Name();
     char *buffer = NULL;
     if (base) {
        asprintf(&buffer, "%s~%s", base, t);
        t = buffer;
        }
     AddSubMenu(new cMenuRecordings(t, level + 1, OpenSubMenus));
     free(buffer);
     return true;
     }
  return false;
}

eOSState cMenuRecordings::Play(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri) {
     if (cStatus::MsgReplayProtected(GetRecording(ri), ri->Name(), base,
                                     ri->IsDirectory()) == true)    // PIN PATCH
        return osContinue;                                          // PIN PATCH
     if (ri->IsDirectory())
        Open();
     else {
        cRecording *recording = GetRecording(ri);
        if (recording) {
           cReplayControl::SetRecording(recording->FileName(), recording->Title());
           return osReplay;
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Rewind(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
     cResumeFile ResumeFile(ri->FileName());
     ResumeFile.Delete();
     return Play();
     }
  return osContinue;
}

eOSState cMenuRecordings::Delete(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     if (Interface->Confirm(tr("Delete recording?"))) {
        cRecordControl *rc = cRecordControls::GetRecordControl(ri->FileName());
        if (rc) {
           if (Interface->Confirm(tr("Timer still recording - really delete?"))) {
              cTimer *timer = rc->Timer();
              if (timer) {
                 timer->Skip();
                 cRecordControls::Process(time(NULL));
                 if (timer->IsSingleEvent()) {
                    isyslog("deleting timer %s", *timer->ToDescr());
                    if(!Timers.Del(timer))
                        Skins.Message(mtError, tr("Could not delete timer"));
                    }
                 Timers.SetModified();
                 }
              }
           else
              return osContinue;
           }
        cRecording *recording = GetRecording(ri);
        if (recording) {
           if (recording->Delete()) {
              cReplayControl::ClearLastReplayed(ri->FileName());
              Recordings.DelByName(ri->FileName());
              cOsdMenu::Del(Current());
              SetHelpKeys();
              Display();
              if (!Count())
                 return osBack;
              }
           else
              Skins.Message(mtError, tr("Error while deleting recording!"));
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording && recording->Info()->Title())
        return AddSubMenu(new cMenuRecording(recording, true));
     }
  return osContinue;
}

eOSState cMenuRecordings::Commands(eKeys Key)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording) {
        char *parameter = NULL;
        asprintf(&parameter, "\"%s\"", *strescape(recording->FileName(), "\"$"));
        cMenuCommands *menu;
        eOSState state = AddSubMenu(menu = new cMenuCommands(tr("Recording commands"), &RecordingCommands, parameter));
        free(parameter);
        if (Key != kNone)
           state = menu->ProcessKey(Key);
        return state;
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Play();
       case kRed:    return (helpKeys > 1 && RecordingCommands.Count()) ? Commands() : Play();
       case kGreen:  return Rewind();
       case kYellow: return Delete();
       case kBlue:   return Info();
       case k1...k9: return Commands(Key);
       case kNone:   if (Recordings.StateChanged(recordingsState))
                        Set(true);
                     break;
       default: break;
       }
     }
  if (Key == kYellow && HadSubMenu && !HasSubMenu()) {
     // the last recording in a subdirectory was deleted, so let's go back up
     cOsdMenu::Del(Current());
     if (!Count())
        return osBack;
     Display();
     }
  if (!HasSubMenu() && Key != kNone)
     SetHelpKeys();
  return state;
}

// --- cMenuSetupBase --------------------------------------------------------

class cMenuSetupBase : public cMenuSetupPage {
protected:
  cSetup data;
  virtual void Store(void);
public:
  cMenuSetupBase(void);
  int tmpScrollBarWidth;      // hw ScrollBarWidth
};

cMenuSetupBase::cMenuSetupBase(void)
{
  data = Setup;
  //printf("\n%s\n",__PRETTY_FUNCTION__);
  switch (data.OSDScrollBarWidth) {              // hw move setup to tmpsrcollbarwidth
    case 5:  {tmpScrollBarWidth = 0;
      break;
    }
    case 7:   {tmpScrollBarWidth = 1;
              break;
    }
    case 9:   {tmpScrollBarWidth = 2;
              break;
    }
    case 11:  {tmpScrollBarWidth = 3;
              break;
    }
    case 13:  {tmpScrollBarWidth = 4;
              break;
    }
    case 15:  {tmpScrollBarWidth = 5;
              break;
    }
    default : tmpScrollBarWidth = 3;
  }
}

void cMenuSetupBase::Store(void)
{
  switch (tmpScrollBarWidth) {              // hw move tmpsrcollbarwidth to setup
    case 0:  {data.OSDScrollBarWidth = 5;
              break;
    }
    case 1:  {data.OSDScrollBarWidth = 7;
              break;
    }
    case 2:  {data.OSDScrollBarWidth = 9;
              break;
    }
    case 3:  {data.OSDScrollBarWidth = 11;
              break;
    }
    case 4:  {data.OSDScrollBarWidth = 13;
              break;
    }
    case 5:  {data.OSDScrollBarWidth = 15;
              break;
    }
  }
  Setup = data;
  //printf("\n Setup.Save() %s\n",__PRETTY_FUNCTION__);
  Setup.Save();
}

// --- cMenuSetupOSD ---------------------------------------------------------

class cMenuSetupOSD : public cMenuSetupBase {
private:
  const char *useSmallFontTexts[3];
  int osdLanguageIndex;
  const char *FontSizesTexts[3];
  const char *channelViewModeTexts[3];
  const char *ScrollBarWidthTexts[6];

  int numSkins;
  bool showSkinSelection;
  int originalSkinIndex;
  int skinIndex;
  const char **skinDescriptions;
  cThemes themes;
  int originalThemeIndex;
  int themeIndex;
  cFileNameList fontNames;
  cStringList fontOsdNames, fontSmlNames, fontFixNames;
  int fontOsdIndex, fontSmlIndex, fontFixIndex;
  virtual void Set(void);
  void ExpertMenu(void);            // ExpertMenu for OSD
  void DrawExpertMenu(void);        // Draw ExpertMenu
  bool expert;

// Language option
  int originalNumLanguages;
  int numLanguages;
  int optLanguages;
  char oldOSDLanguage[8];
  int oldOSDLanguageIndex;
  //int osdLanguageIndex;
  int ExpertOptions_old;
  bool stored;

public:
  cMenuSetupOSD(void);
  virtual ~cMenuSetupOSD();
  virtual eOSState ProcessKey(eKeys Key);

// Language option
  eOSState LangProcessKey(eKeys Key); // if language changed call this processkey()
  };

cMenuSetupOSD::cMenuSetupOSD(void)
{
  //printf("\n%s\n",__PRETTY_FUNCTION__);
  osdLanguageIndex = I18nCurrentLanguage();
  numSkins = Skins.Count();
  skinIndex = originalSkinIndex = Skins.Current()->Index();
  skinDescriptions = new const char*[numSkins];
  themes.Load(Skins.Current()->Name());
  themeIndex = originalThemeIndex = Skins.Current()->Theme() ? themes.GetThemeIndex(Skins.Current()->Theme()->Description()) : 0;
  cFont::GetAvailableFontNames(&fontOsdNames);
  cFont::GetAvailableFontNames(&fontSmlNames);
  cFont::GetAvailableFontNames(&fontFixNames, true);
  fontOsdNames.Insert(strdup(DefaultFontOsd));
  fontSmlNames.Insert(strdup(DefaultFontSml));
  fontFixNames.Insert(strdup(DefaultFontFix));
  fontOsdIndex = max(0, fontOsdNames.Find(Setup.FontOsd));
  fontSmlIndex = max(0, fontSmlNames.Find(Setup.FontSml));
  fontFixIndex = max(0, fontFixNames.Find(Setup.FontFix));
  showSkinSelection = 0;
  expert = false;

  // Langugage

  for (numLanguages = 0; numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0; numLanguages++)
      ;
  originalNumLanguages = numLanguages;
  optLanguages = originalNumLanguages-1;
  //optLanguages = 1;
  //oldOSDLanguage = Setup.OSDLanguage;
  strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
  oldOSDLanguageIndex = osdLanguageIndex = I18nCurrentLanguage();
  stored = false;
  ExpertOptions_old   = data.ExpertOptions;

  Set();
}

cMenuSetupOSD::~cMenuSetupOSD()
{
  //DDD("%s",__PRETTY_FUNCTION__);
  delete[] skinDescriptions;

  // Language
  if(!stored)
  {
    strn0cpy(Setup.OSDLanguage, oldOSDLanguage, 6);
    Setup.EPGLanguages[0] = oldOSDLanguageIndex;
    I18nSetLanguage(Setup.EPGLanguages[0]);
    //DrawMenu();
    //Set();
  }

}

void cMenuSetupOSD::Set(void)
{
    SetCols(23);
    SetSection(tr("OSD & Language"));
    int current = Current();

    useSmallFontTexts[0] = tr("never");
    useSmallFontTexts[1] = tr("skin dependent");
    useSmallFontTexts[2] = tr("always");

    FontSizesTexts[FONT_SIZE_USER] = tr("User defined");
    FontSizesTexts[FONT_SIZE_SMALL] = tr("Small");
    FontSizesTexts[FONT_SIZE_NORMAL] = tr("FontSize$Normal");
    //FontSizesTexts[FONT_SIZE_LARGE] = tr("Large");

    channelViewModeTexts[0] = tr("channellist");
    channelViewModeTexts[1] = tr("current bouquet");
    channelViewModeTexts[2] = tr("bouquet list");

    Clear();

// Language
    Add(new cMenuEditStraItem(tr("Setup.OSD$Language"),                &data.EPGLanguages[0], I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));

    Add(new cOsdItem("", osUnknown, false)); // blank line
    Add(new cMenuEditBoolItem(tr("Setup.DVB$Display subtitles"),       &data.DisplaySubtitles));
    Add(new cOsdItem("", osUnknown, false)); // blank line

// OSD Settings
    Add(new cMenuEditStraItem(tr("Setup.OSD$OSD color"),               &themeIndex, themes.NumThemes(), themes.Descriptions()));

    Add(new cMenuEditStraItem(tr("Setup.OSD$OSD Font Size"),           &data.FontSizes, sizeof(FontSizesTexts)/sizeof(char*), FontSizesTexts));
    if (data.FontSizes == FONT_SIZE_USER) {
        Add(new cMenuEditIntItem( tr("Setup.OSD$OSD font size (pixel)"),  &data.FontOsdSize, MINFONTSIZE, MAXFONTSIZE));
        //Add(new cMenuEditIntItem( tr("Setup.OSD$Small font size (pixel)"),&data.FontSmlSize, MINFONTSIZE, MAXFONTSIZE));
        //Add(new cMenuEditIntItem( tr("Setup.OSD$Fixed font size (pixel)"),&data.FontFixSize, MINFONTSIZE, MAXFONTSIZE));
    }

    if (!strcmp(Skins.Current()->Name(), "Reel") == 0)
        Add(new cMenuEditStraItem(tr("Setup.OSD$Use small font"),      &data.UseSmallFont, 3, useSmallFontTexts));
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Scroll pages"),            &data.MenuScrollPage));
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Scroll wraps"),            &data.MenuScrollWrap));
    Add(new cMenuEditIntItem( tr("Setup.OSD$Channel info time (s)"),   &data.ChannelInfoTime, 1, 60));
    Add(new cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"), &data.UseBouquetList, 3, channelViewModeTexts));

    Add(new cOsdItem("", osUnknown, false)); // blank line

    Add(new cMenuEditBoolItem(tr("Setup.OSD$Show expert menu options"),    &data.ExpertOptions));
    if (data.ExpertOptions)
    {
        Add(new cMenuEditBoolItem(tr("Setup.OSD$  Use extended navigation keys"),  &data.ExpertNavi));
        Add(new cMenuEditBoolItem(tr("Setup.OSD$  Ok key in TV mode"),             &data.WantChListOnOk, tr("channelinfo"), tr("Standard")));
        Add(new cMenuEditBoolItem(tr("Setup.OSD$  Program +/- keys"),              &data.ChannelUpDownKeyMode, tr("Standard"), tr("Channell./Bouquets")));
    }

/* sent to expertmenu
    // new items
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Ok shows"),                 &data.WantChListOnOk, tr("channelinfo"), tr("channellist")));
    Add(new cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"),  &data.UseBouquetList, 3, channelViewModeTexts));
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Channel +/- keys"),  &data.ChannelUpDownKeyMode, tr("Standard"), tr("Bouq./chan. list") ));
*/

    //Add(new cMenuEditStraItem(tr("Setup.OSD$Language"),               &data.OSDLanguage, I18nNumLanguages, I18nLanguages()));
    //Add(new cMenuEditIntItem( tr("Setup.EPG$Preferred languages"),    &numLanguages, 1, I18nNumLanguages));
    //for (int i = 1; i < numLanguages; i++) {
    //Add(new cMenuEditStraItem(tr(" Setup.EPG$Preferred language"),     &data.EPGLanguages[i], I18nNumLanguages, I18nLanguages()));
    // }
    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Timeout requested channel info"), &data.TimeoutRequChInfo));
    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Menu button closes"),     &data.MenuButtonCloses));
    //Add(new cMenuEditStraItem(tr("Setup.OSD$Use small font"),         &data.UseSmallFont, 3, useSmallFontTexts));


    if (data.ExpertOptions)
        SetHelp(NULL, NULL, NULL, tr("Expertmenu"));
    else
        SetHelp(NULL);
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSetupOSD::ProcessKey(eKeys Key)
{
  bool ModifiedApperance = false;

  if (Key == kBlue) {
      expert = true;
      ExpertMenu();
  }
  else if (Key == kYellow) {
      showSkinSelection = !showSkinSelection;
      if (expert)
          DrawExpertMenu();
      //Set();
  }
  else if (Key == kOk) {
     I18nSetLocale(data.OSDLanguage);
#ifdef RBLITE
    Skins.SetCurrent("Reel");
#else
    if (skinIndex != originalSkinIndex) {
      cSkin *Skin = Skins.Get(skinIndex);
      if (Skin) {
        Utf8Strn0Cpy(data.OSDSkin, Skin->Name(), sizeof(data.OSDSkin));
        Skins.SetCurrent(Skin->Name());
        ModifiedApperance = true;
      }
    }
#endif
    if (themes.NumThemes() && Skins.Current()->Theme()) {
      // hw data.UseSmallFont=2;
      //Skins.SetCurrent("Reel");
      Skins.Current()->Theme()->Load(themes.FileName(themeIndex));
      Utf8Strn0Cpy(data.OSDTheme, themes.Name(themeIndex), sizeof(data.OSDTheme));
        ModifiedApperance |= themeIndex != originalThemeIndex;
        }
     if (data.OSDLeft != Setup.OSDLeft || data.OSDTop != Setup.OSDTop || data.OSDWidth != Setup.OSDWidth || data.OSDHeight != Setup.OSDHeight) {
        data.OSDWidth &= ~0x07; // OSD width must be a multiple of 8
        ModifiedApperance = true;
        }
     if (data.UseSmallFont != Setup.UseSmallFont || data.AntiAlias != Setup.AntiAlias)
        ModifiedApperance = true;
     Utf8Strn0Cpy(data.FontOsd, fontOsdNames[fontOsdIndex], sizeof(data.FontOsd));
     Utf8Strn0Cpy(data.FontSml, fontSmlNames[fontSmlIndex], sizeof(data.FontSml));
     Utf8Strn0Cpy(data.FontFix, fontFixNames[fontFixIndex], sizeof(data.FontFix));
     if (strcmp(data.FontOsd, Setup.FontOsd) || data.FontOsdSize != Setup.FontOsdSize) {
        cFont::SetFont(fontOsd, data.FontOsd, data.FontOsdSize);
        ModifiedApperance = true;
        }
     if (strcmp(data.FontSml, Setup.FontSml) || data.FontSmlSize != Setup.FontSmlSize) {
        cFont::SetFont(fontSml, data.FontSml, data.FontSmlSize);
        ModifiedApperance = true;
        }
     if (strcmp(data.FontFix, Setup.FontFix) || data.FontFixSize != Setup.FontFixSize) {
        cFont::SetFont(fontFix, data.FontFix, data.FontFixSize);
        ModifiedApperance = true;
    }
  }

  int oldSkinIndex = skinIndex;
  int oldOsdLanguageIndex = osdLanguageIndex;
  int oldFontSizes = data.FontSizes;
  eOSState state = LangProcessKey(Key);//cMenuSetupBase::ProcessKey(Key);

  if (ModifiedApperance)
     SetDisplayMenu();

 if (Key == kLeft || Key == kRight) {
  if (skinIndex != oldSkinIndex) {
    cSkin *Skin = Skins.Get(skinIndex);
    if (Skin) {
      char *d = themes.NumThemes() ? strdup(themes.Descriptions()[themeIndex]) : NULL;
      themes.Load(Skin->Name());
      if (skinIndex != oldSkinIndex)
        themeIndex = d ? themes.GetThemeIndex(d) : 0;
      free(d);
    }
    Set();
    SetHelp(NULL);                                      // clear HelpKey
    Clear();                                            // Clear OSD
    SetSection(tr("OSD - Expertmenu"));                 // Title OSD
    DrawExpertMenu();                                   // Draw New OSD
  }

  if (osdLanguageIndex != oldOsdLanguageIndex/* || skinIndex != oldSkinIndex*/) {
     strn0cpy(data.OSDLanguage, I18nLocale(osdLanguageIndex), sizeof(data.OSDLanguage));
     int OriginalOSDLanguage = I18nCurrentLanguage();
     I18nSetLanguage(osdLanguageIndex);

     Set();
     I18nSetLanguage(OriginalOSDLanguage);
     }

  if (data.FontSizes != oldFontSizes) {
      oldFontSizes = data.FontSizes;
      Set();
  }

  if (data.ExpertOptions != ExpertOptions_old)
  {
      ExpertOptions_old = data.ExpertOptions;
      if (data.ExpertOptions == 0)
      {
          // Reset special modes to default
          data.ExpertNavi = 0;
          data.ExpertOptions = 0;
          data.WantChListOnOk = 1;
          data.ChannelUpDownKeyMode = 0;
      }
      Set();
  }


#if 0
  if (oldFontSizes != data.FontSizes) {
    switch (data.FontSizes) {
           case 1:  /* Large */
             data.FontOsdSize = 20;
             data.FontSmlSize = 16;
             data.FontFixSize = 18;
             break;
           case 2: /* Small */
             data.FontOsdSize = 16;
             data.FontSmlSize = 12;
             data.FontFixSize = 14;
             break;
           case 3: /* Normal */
             data.FontOsdSize = 18;
             data.FontSmlSize = 14;
             data.FontFixSize = 16;
             break;
           default: break;
        }
        cFont::SetFont(fontOsd, data.FontOsd, data.FontOsdSize);
        cFont::SetFont(fontSml, data.FontSml, data.FontSmlSize);
        cFont::SetFont(fontFix, data.FontFix, data.FontFixSize);
        Set();
  }
#endif
  } // kLeft || kRight
  else if (Key == kOk) {
        Clear();
        Set();
    }

  return state;
}

eOSState cMenuSetupOSD::LangProcessKey(eKeys Key)
{
  bool Modified = false; //(optLanguages+1 != originalNumLanguages || osdLanguageIndex != data.EPGLanguages[0]);

  int oldnumLanguages = optLanguages+1;
  int oldPrefLanguage = data.EPGLanguages[0];

  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (Key != kNone) {
     if (optLanguages+1 != oldnumLanguages) {
        Modified=true;
        for (int i = oldnumLanguages; i <= optLanguages; i++) {
        Modified = true;
            data.EPGLanguages[i] = 0;
            for (int l = 0; l < I18nNumLanguages; l++) {
                int k;
                for (k = 0; k < oldnumLanguages; k++) {
                    if (data.EPGLanguages[k] == l)
                       break;
                    }
                if (k >= oldnumLanguages) {
                   data.EPGLanguages[i] = l;
                   break;
                   }
                }
            }
        data.EPGLanguages[optLanguages+1] = -1;
     }

     if (oldPrefLanguage != data.EPGLanguages[0]) {
        //Modified = true;
    strn0cpy(data.OSDLanguage, I18nLocale(data.EPGLanguages[0]), sizeof(data.OSDLanguage));
        strn0cpy(Setup.OSDLanguage, I18nLocale(data.EPGLanguages[0]), sizeof(data.OSDLanguage));
        data.SubtitleLanguages[0] = data.EPGLanguages[0];
        data.AudioLanguages[0]    = data.EPGLanguages[0];
        stored = false;
        //int OriginalOSDLanguage = I18nCurrentLanguage();
        I18nSetLanguage(data.EPGLanguages[0]);
        //DrawMenu();
        Set();

    //I18nSetLanguage(OriginalOSDLanguage);
     }

     if (!Modified) {
        for (int i = 1; i <= optLanguages; i++) {
            if (data.EPGLanguages[i] != ::Setup.EPGLanguages[i]) {
               Modified = true;
               break;
            }
        }
     }

     if (Modified) {
        for (int i = 0; i <= I18nNumLanguages ; i++) {
            data.AudioLanguages[i] = data.EPGLanguages[i];
            data.SubtitleLanguages[i] = data.EPGLanguages[i];
        if (data.EPGLanguages[i] == -1)
            break;
    }
    if (Key == kOk)
           cSchedules::ResetVersions();
    else
    {
           DrawExpertMenu();
           //Set();
        }
     }
  }
  if(Key == kOk)
  {
    strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
    stored = true;
  }
  return state;
}

// --- OSD ExpertMenu Init ---------------------------------------------------------

void cMenuSetupOSD::ExpertMenu(void)
{
    Set();
    SetHelp(NULL);                                      // clear HelpKey
    Clear();                                            // Clear OSD
    SetSection(tr("OSD - Expertmenu"));                 // Title OSD
    DrawExpertMenu();                                   // Draw New OSD
}

// --- OSD ExpertMenu Draw ---------------------------------------------------------

void cMenuSetupOSD::DrawExpertMenu(void)
{
    ScrollBarWidthTexts[0] = "5";
    ScrollBarWidthTexts[1] = "7";
    ScrollBarWidthTexts[2] = "9";
    ScrollBarWidthTexts[3] = "11";
    ScrollBarWidthTexts[4] = "13";
    ScrollBarWidthTexts[5] = "15";


    int current = Current();

    Clear();

    for (cSkin *Skin = Skins.First(); Skin; Skin = Skins.Next(Skin))
         skinDescriptions[Skin->Index()] = Skin->Description();

    cSkin *Skin = Skins.Get(skinIndex);
    if (Skin && strcmp(Skin->Name(), "Reel") )
        showSkinSelection = 1;

    if (themes.NumThemes())
      if (showSkinSelection == 1) {
        Add(new cMenuEditStraItem(tr("Setup.OSD$Skin"),               &skinIndex, numSkins, skinDescriptions));
      }

    //Add(new cMenuEditStraItem(tr("Setup.OSD$Skin color"),            &themeIndex, themes.NumThemes(), themes.Descriptions()));
    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Anti-alias"),             &data.AntiAlias));

    /* font & freetype settings */
    /* don't show these buttons with skinreelng/skinreel3, these settings are done in the skin in this case */
    if (showSkinSelection == 1) {
      Add(new cMenuEditStraItem(tr("Setup.OSD$OSD font name"),          &fontOsdIndex, fontOsdNames.Size(), &fontOsdNames[0]));
      Add(new cMenuEditStraItem(tr("Setup.OSD$Small font name"),        &fontSmlIndex, fontSmlNames.Size(), &fontSmlNames[0]));
      Add(new cMenuEditStraItem(tr("Setup.OSD$Fixed font name"),        &fontFixIndex, fontFixNames.Size(), &fontFixNames[0]));
      Add(new cMenuEditIntItem( tr("Setup.OSD$OSD font size (pixel)"),  &data.FontOsdSize, 10, MAXFONTSIZE));
      Add(new cMenuEditIntItem( tr("Setup.OSD$Small font size (pixel)"),&data.FontSmlSize, 10, MAXFONTSIZE));
      Add(new cMenuEditIntItem( tr("Setup.OSD$Fixed font size (pixel)"),&data.FontFixSize, 10, MAXFONTSIZE));

#if 0
      Add(new cMenuEditIntItem(tr("Setup.OSD$Left"),                &data.OSDLeft, 0, MAXOSDWIDTH));
      Add(new cMenuEditIntItem(tr("Setup.OSD$Top"),                 &data.OSDTop, 0, MAXOSDHEIGHT));
      Add(new cMenuEditIntItem(tr("Setup.OSD$Width"),               &data.OSDWidth, MINOSDWIDTH, MAXOSDWIDTH));
      Add(new cMenuEditIntItem(tr("Setup.OSD$Height"),              &data.OSDHeight, MINOSDHEIGHT, MAXOSDHEIGHT));
#endif
    }

    Add(new cOsdItem("", osUnknown, false)); // blank line
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Random menu placement"),   &data.OSDRandom));

    /*
    Add(new cOsdItem("", osUnknown, false)); // blank line
    //Key Behaviour
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Ok shows"),                 &data.WantChListOnOk, tr("channelinfo"), tr("channellist")));
    Add(new cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"),  &data.UseBouquetList, 3, channelViewModeTexts));
    Add(new cMenuEditBoolItem(tr("Setup.OSD$Channel +/- keys"),  &data.ChannelUpDownKeyMode, tr("Standard"), tr("Bouq./chan. list") ));
    */

    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Remain Time"),              &data.OSDRemainTime));
    //Add(new cMenuEditBoolItem(tr("Setup.OSD$Use Symbol"),         &data.OSDUseSymbol));
    //Add(new cMenuEditStraItem(tr("Setup.OSD$ScrollBar Width"),    &tmpScrollBarWidth, 6, ScrollBarWidthTexts));

    Add(new cOsdItem("", osUnknown, false)); // blank line

    Add(new cMenuEditIntItem( tr("Setup.OSD$Optional languages"),     &optLanguages, 0, I18nLanguages()->Size(), tr("none"), NULL));
    for (int i = 1; i <= optLanguages; i++)
         Add(new cMenuEditStraItem(tr("Setup.OSD$ Optional language"),   &data.EPGLanguages[i], I18nLanguages()->Size(), &I18nLanguages()->At(0)));

    SetCurrent(Get(current));
    Display();
}


// Languages are in OSD menu
#if 1
// --- cMenuSetupLang ---------------------------------------------------------

class cMenuSetupLang : public cMenuSetupBase {
private:
  int originalNumLanguages;
  int numLanguages;
  int optLanguages;
  char oldOSDLanguage[8];
  int oldOSDLanguageIndex;
  int osdLanguageIndex;
  bool stored;
  void DrawMenu(void);
public:
  cMenuSetupLang(void);
  ~cMenuSetupLang(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupLang::cMenuSetupLang(void)
{
  //printf("\n%s\n",__PRETTY_FUNCTION__);
  for (numLanguages = 0; numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0; numLanguages++)
      ;
  originalNumLanguages = numLanguages;
  optLanguages = originalNumLanguages-1;
  //oldOSDLanguage = Setup.OSDLanguage;
  strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
  oldOSDLanguageIndex = osdLanguageIndex = I18nCurrentLanguage();
  stored = false;
  //SetSection(tr("Setup.OSD$Language"));
  //SetHelp(tr("Button$Scan"));
  DrawMenu();
}

cMenuSetupLang::~cMenuSetupLang(void)
{
  if(!stored)
  {
    strn0cpy(Setup.OSDLanguage, oldOSDLanguage, 6);
    Setup.EPGLanguages[0] = oldOSDLanguageIndex;
    I18nSetLanguage(Setup.EPGLanguages[0]);
    DrawMenu();
  }
}

void cMenuSetupLang::DrawMenu(void)
{
  SetCols(25);
  SetSection(tr("Setup.OSD$Language"));
  int current = Current();

  Clear();

  Add(new cMenuEditStraItem(tr("Setup.OSD$Language"),               &data.EPGLanguages[0], I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Optional languages"),     &optLanguages, 0, I18nLanguages()->Size(), tr("none"), NULL));
  for (int i = 1; i <= optLanguages; i++)
     Add(new cMenuEditStraItem(tr("Setup.OSD$ Optional language"),   &data.EPGLanguages[i], I18nLanguages()->Size(), &I18nLanguages()->At(0)));
  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupLang::ProcessKey(eKeys Key)
{
  bool Modified = (optLanguages+1 != originalNumLanguages || osdLanguageIndex != data.EPGLanguages[0]);

  int oldnumLanguages = optLanguages+1;
  int oldPrefLanguage = data.EPGLanguages[0];

  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (Key != kNone) {
     if (optLanguages+1 != oldnumLanguages) {
        Modified=true;
        for (int i = oldnumLanguages; i <= optLanguages; i++) {
        Modified = true;
            data.EPGLanguages[i] = 0;
            for (int l = 0; l < I18nNumLanguages; l++) {
                int k;
                for (k = 0; k < oldnumLanguages; k++) {
                    if (data.EPGLanguages[k] == l)
                       break;
                    }
                if (k >= oldnumLanguages) {
                   data.EPGLanguages[i] = l;
                   break;
                   }
                }
            }
        data.EPGLanguages[optLanguages+1] = -1;
     }

     if (oldPrefLanguage != data.EPGLanguages[0]) {
        Modified = true;
    strn0cpy(data.OSDLanguage, I18nLocale(data.EPGLanguages[0]), sizeof(data.OSDLanguage));
        strn0cpy(Setup.OSDLanguage, I18nLocale(data.EPGLanguages[0]), sizeof(data.OSDLanguage));
        stored = false;
        //int OriginalOSDLanguage = I18nCurrentLanguage();
        I18nSetLanguage(data.EPGLanguages[0]);
        DrawMenu();

    //I18nSetLanguage(OriginalOSDLanguage);
     }

     if (!Modified) {
        for (int i = 0; i <= optLanguages; i++) {
            if (data.EPGLanguages[i] != ::Setup.EPGLanguages[i]) {
               Modified = true;
               break;
            }
        }
     }

     if (Modified) {
        for (int i = 0; i <= I18nNumLanguages ; i++) {
            data.AudioLanguages[i] = data.EPGLanguages[i];
        if (data.EPGLanguages[i] == -1)
            break;
    }
    if (Key == kOk)
           cSchedules::ResetVersions();
    else
           DrawMenu();
     }
  }
  if(Key == kOk)
  {
    strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
    stored = true;
  }
  return state;
}
#endif

// --- cMenuSetupEPG ---------------------------------------------------------

class cMenuSetupEPG : public cMenuSetupBase {
private:
  //int originalNumLanguages;
  //int numLanguages;
  void Setup(void);
public:
  cMenuSetupEPG(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupEPG::cMenuSetupEPG(void)
{
  /*for (numLanguages = 0; numLanguages < I18nNumLanguages && data.EPGLanguages[numLanguages] >= 0; numLanguages++)
      ;
  originalNumLanguages = numLanguages;
  */
  SetSection(tr("EPG"));
  SetHelp(tr("Button$Scan"));
  Setup();
}

void cMenuSetupEPG::Setup(void)
{
  int current = Current();

  Clear();
  /*
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG scan timeout (h)"),      &data.EPGScanTimeout));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG bugfix level"),          &data.EPGBugfixLevel, 0, MAXEPGBUGFIXLEVEL));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG linger time (min)"),     &data.EPGLinger, 0));
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Set system time"),           &data.SetSystemTime));
  if (data.SetSystemTime)
     Add(new cMenuEditTranItem(tr("Setup.EPG$Use time from transponder"), &data.TimeTransponder, &data.TimeSource));
  Add(new cMenuEditIntItem( tr("Setup.EPG$Preferred languages"),       &numLanguages, 0, I18nNumLanguages));
  for (int i = 0; i < numLanguages; i++)
     Add(new cMenuEditStraItem(tr("Setup.EPG$Preferred language"),     &data.EPGLanguages[i], I18nNumLanguages, I18nLanguages()));
  */
  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupEPG::ProcessKey(eKeys Key)
{
  /*
  if (Key == kOk) {
     bool Modified = numLanguages != originalNumLanguages;
     if (!Modified) {
        for (int i = 0; i < numLanguages; i++) {
            if (data.EPGLanguages[i] != ::Setup.EPGLanguages[i]) {
               Modified = true;
               break;
               }
            }
        }
     if (Modified)
        cSchedules::ResetVersions();
     }

  int oldnumLanguages = numLanguages;
  */
  int oldSetSystemTime = data.SetSystemTime;

  eOSState state = cMenuSetupBase::ProcessKey(Key);
  if (Key != kNone) {
     if (data.SetSystemTime != oldSetSystemTime) {
        /*for (int i = oldnumLanguages; i < numLanguages; i++) {
            data.EPGLanguages[i] = 0;
            for (int l = 0; l < I18nNumLanguages; l++) {
                int k;
                for (k = 0; k < oldnumLanguages; k++) {
                    if (data.EPGLanguages[k] == l)
                       break;
                    }
                if (k >= oldnumLanguages) {
                   data.EPGLanguages[i] = l;
                   break;
                   }
                }
            }
        data.EPGLanguages[numLanguages] = -1;
    */
        Setup();
        }
     if (Key == kRed) {
        EITScanner.ForceScan();
        return osEnd;
        }
     }
  return state;
}

// --- cMenuSetupDVB ---------------------------------------------------------

class cMenuSetupDVB : public cMenuSetupBase {
private:
  int originalNumAudioLanguages;
  int numAudioLanguages;
  int originalNumSubtitleLanguages;
  int numSubtitleLanguages;
  void Setup(void);
/*
  const char *videoDisplayFormatTexts[3];
  const char *updateChannelsTexts[6];
*/

public:
  cMenuSetupDVB(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupDVB::cMenuSetupDVB(void)
{

/*
  for (numAudioLanguages = 0; numAudioLanguages < I18nNumLanguages && data.AudioLanguages[numAudioLanguages] >= 0; numAudioLanguages++);
  for (numSubtitleLanguages = 0; numSubtitleLanguages < I18nLanguages()->Size() && data.SubtitleLanguages[numSubtitleLanguages] >= 0; numSubtitleLanguages++);

  originalNumAudioLanguages = numAudioLanguages;
  originalNumSubtitleLanguages = numSubtitleLanguages;

  videoDisplayFormatTexts[0] = tr("pan&scan");
  videoDisplayFormatTexts[1] = tr("letterbox");
  videoDisplayFormatTexts[2] = tr("center cut out");
  updateChannelsTexts[0] = tr("no");
  updateChannelsTexts[1] = tr("names only");
  updateChannelsTexts[2] = tr("PIDs only");
  updateChannelsTexts[3] = tr("names and PIDs");
  updateChannelsTexts[4] = tr("add new channels");
  updateChannelsTexts[5] = tr("add new transponders");
  SetSection(tr("DVB"));
*/
  SetSection(tr("Audio"));
  Setup();
}

void cMenuSetupDVB::Setup(void)
{
  int current = Current();

  Clear();
/*
  Add(new cMenuEditIntItem( tr("Setup.DVB$Primary DVB interface"), &data.PrimaryDVB, 1, cDevice::NumDevices()));
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Video format"),          &data.VideoFormat, "4:3", "16:9"));
  if (data.VideoFormat == 0)
     Add(new cMenuEditStraItem(tr("Setup.DVB$Video display format"), &data.VideoDisplayFormat, 3, videoDisplayFormatTexts));

  Add(new cMenuEditIntItem( tr("Setup.DVB$Audio languages"),       &numAudioLanguages, 0, I18nNumLanguages));
  for (int i = 0; i < numAudioLanguages; i++)
      Add(new cMenuEditStraItem(tr("Setup.DVB$Audio language"),    &data.AudioLanguages[i], I18nNumLanguages, I18nLanguages()));
*/
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Display subtitles"),     &data.DisplaySubtitles));
#if 0
  if (data.DisplaySubtitles) {
      Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle languages"), &numSubtitleLanguages, 0, I18nNumLanguages));
      for (int i = 0; i < numSubtitleLanguages; i++)
          Add(new cMenuEditStraItem(tr("Setup.DVB$Subtitle language"), &data.SubtitleLanguages[i], I18nNumLanguages, I18nLanguages()));
      Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle offset"), &data.SubtitleOffset, -100, 100));
      Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle foreground transparency"), &data.SubtitleFgTransparency, 0, 9));
      Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle background transparency"), &data.SubtitleBgTransparency, 0, 10));
  }
#endif
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Use Dolby Digital"),     &data.UseDolbyDigital));
  if (data.UseDolbyDigital) {
     Add(new cMenuEditIntItem(tr(" Delay ac3 (10ms)"),               &data.ReplayDelay, 0, 80));
     Add(new cMenuEditBoolItem(tr(" Prefer ac3 over HDMI"),          &data.Ac3OverHdmi));
  }
  Add(new cMenuEditIntItem(tr("Delay Stereo (10ms)"),              &data.MP2Delay, 0, 80));
  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupDVB::ProcessKey(eKeys Key)
{
  /*
  int oldPrimaryDVB = ::Setup.PrimaryDVB;
  int oldVideoDisplayFormat = ::Setup.VideoDisplayFormat;
  bool oldVideoFormat = ::Setup.VideoFormat;
  bool newVideoFormat = data.VideoFormat;
  int oldnumAudioLanguages = numAudioLanguages;
  */

  bool oldUseDD = data.UseDolbyDigital;

  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (Key != kNone) {
     if (oldUseDD != data.UseDolbyDigital) {
        Setup();
     }
  }
  return state;

  /*
  if (Key != kNone) {
     bool DoSetup = data.VideoFormat != newVideoFormat;
     if (numAudioLanguages != oldnumAudioLanguages) {
        for (int i = oldnumAudioLanguages; i < numAudioLanguages; i++) {
            data.AudioLanguages[i] = 0;
            for (int l = 0; l < I18nNumLanguages; l++) {
                int k;
                for (k = 0; k < oldnumAudioLanguages; k++) {
                    if (data.AudioLanguages[k] == l)
                       break;
                    }
                if (k >= oldnumAudioLanguages) {
                   data.AudioLanguages[i] = l;
                   break;
                   }
                }
            }
        data.AudioLanguages[numAudioLanguages] = -1;
        DoSetup = true;
        }
     if (DoSetup)
        Setup();
     }
  if (state == osBack && Key == kOk) {
     if (::Setup.PrimaryDVB != oldPrimaryDVB)
        state = osSwitchDvb;
     if (::Setup.VideoDisplayFormat != oldVideoDisplayFormat)
        cDevice::PrimaryDevice()->SetVideoDisplayFormat(eVideoDisplayFormat(::Setup.VideoDisplayFormat));
     if (::Setup.VideoFormat != oldVideoFormat)
     }

  return state;
  */
}

// --- cMenuSetupLNB ---------------------------------------------------------
// #define  DBG "DEBUG [diseqc]: "

class cMenuSetupLNB : public cMenuSetupBase {
private:
  void Setup(void);
  void SetHelpKeys(void);
  bool IsUnique(int Tuner = 0, int Source=0);
  void LoadActuall();
  void AddDefault();
  void ResetLnbs();
  void Init();

  // holds only unique Sources
  struct tLnbType {
     int source;
     int lnbType;
     int flag;
     } /* keep this */ ;
  //tLnbType lnbTypesAtTuner[MAXTUNERS+1][MAXLNBS]; //XXX
  tLnbType lnbTypesAtTuner[MAXTUNERS+1][64];
  int lnbNumberAtTuner[MAXTUNERS+1];
  //int lnbNumber[MAXTUNERS+1];  // number of diffrent LNBs/sources


  int tuner;
  int DiSEqC[MAXTUNERS+1];
  int Diseqc[MAXTUNERS+1]; // DiSEqFlags
  int RotorLNBTuner[MAXTUNERS+1];
  int diffSetups;
  cOsdMenu *smenu;
  static int IntCmp(const void *a, const void *b);
  void LoadTmpSources();

  bool extended;
  const char *useDiSEqcTexts[8];
  const char *lofTexts[7];
  int oldLnbNumber;
  int currentChannel;
  bool circular; // XXX
  int waitMs[MAXTUNERS+1];
  int repeat[MAXTUNERS+1];
  void DumpDiseqcs(bool All = false);

public:
  cMenuSetupLNB(void);
  virtual eOSState ProcessKey(eKeys Key);
  eOSState Save();
  };

cMenuSetupLNB::cMenuSetupLNB(void)
{
  SetSection(tr("Dish settings"));
  //DLOG( " LNB DEBUG  cMenuSetupLNB Constr  data.DiSEqC: %d  ", data.DiSEqC );

  SetCols(19);
  extended = false;
  circular = 0;
  oldLnbNumber = 0;
  tuner = (::Setup.DiSEqC & DIFFSETUPS) == DIFFSETUPS;
  diffSetups = tuner;

  DLOG (DBG " Have Diff Tuner Setup? %s ", diffSetups?"YES":"NO");


  Init();


  Setup();
}

void cMenuSetupLNB::Setup(void)
{
  DLOG (DBG " cMenuSetupLNB::Setup()    ");
  int current = Current();
  Clear();

  useDiSEqcTexts[0] = tr("DiSEqC disabled");
  useDiSEqcTexts[1] = "mini DiSEqC";
  useDiSEqcTexts[2] = "DiSEqC 1.0";
  useDiSEqcTexts[3] = "DiSEqC 1.1";
  useDiSEqcTexts[4] = "DisiCon 4";
  useDiSEqcTexts[5] = "Rotor - DiSEqC 1.2";
  useDiSEqcTexts[6] = "Rotor - GotoX";
  useDiSEqcTexts[7] = tr("Rotor - shared LNB");

  lofTexts[0] = "9750/10600 MHz";
  lofTexts[1] = "10750/11250 MHz";
  lofTexts[2] = "5150 MHz";
  lofTexts[3] = "9750 MHz";
  lofTexts[4] = "10600 Mhz";
  lofTexts[5] = "11250 MHz";
  lofTexts[6] = "11475 MHz";

  char buffer[16];
  char LnbC = 'A';

  bool hasRotor = false;
  for (int i = diffSetups; i < diffSetups*MAXTUNERS + 1; i++) { // ???
     if (i!=tuner && ((DiSEqC[i] & ROTORMASK) == GOTOX ||
                 (DiSEqC[i] & ROTORMASK) == DISEQC12) &&
                 (!i || (cDevice::GetDevice(i-1) &&
                  cDevice::GetDevice(i-1)->ProvidesSource(cSource::stSat))))
     {
        hasRotor = true;
     }
  }

  if (extended)
     Add(new cMenuEditBoolItem(tr("Different Setups"),  &diffSetups));
  if (tuner)
     Add(new cMenuEditSatTunItem(tr("Tuner"), &tuner));
  if ((Diseqc[tuner])==ROTOR_SHARED && !hasRotor)
     Diseqc[tuner]=DiSEqC[tuner]=NONE;

  Add(new cMenuEditStraItem(tr("DiSEqC Type"),     &Diseqc[tuner], hasRotor ? 8 : 7, useDiSEqcTexts));


  //if (!(Diseqc[tuner] == DISICON4  || extended) || Diseqc[tuner] >= 4)
  // Add(new cMenuEditStraItem(tr("LNB Type"), &lnbTypesAtTuner[tuner][0].lnbType, 7, lofTexts));

  DLOG (DBG " switch Diseqc[%X] @ tuner: %d ", Diseqc[tuner], tuner);
  switch (Diseqc[tuner]) {
     case NONE:
            lnbNumberAtTuner[tuner] = 1;
            if (tuner)
                    Add(new cMenuEditSrcItem(tr("Satellite"), &lnbTypesAtTuner[tuner][0].source));
           if (extended)
                   Add(new cMenuEditStraItem(tr("   LNB Type"), &lnbTypesAtTuner[tuner][0].lnbType, 7, lofTexts));
            break;
     case MINI :
            lnbNumberAtTuner[tuner] = 2;
            for (int i=0; i < lnbNumberAtTuner[tuner];i++) {
               snprintf(buffer, sizeof(buffer), "LNB %c",LnbC+i);
               Add(new cMenuEditSrcEItem(buffer, &lnbTypesAtTuner[tuner][i].source, DiSEqC, tuner));
               if (extended)
                     Add(new cMenuEditStraItem(tr("   LNB Type"), &lnbTypesAtTuner[tuner][i].lnbType, 7, lofTexts));
               }

             if (extended)
                Add(new cMenuEditIntItem(tr("Delay (ms)"), &waitMs[tuner], 15, 100));
             break;
     case FULL:
             Add(new cMenuEditIntItem(tr("Number of LNBs"), &lnbNumberAtTuner[tuner],MINLNBS,MAXLNBS));

             for (int i=0;i < lnbNumberAtTuner[tuner];i++) {
                 snprintf(buffer, sizeof(buffer), "LNB %c",LnbC+i);
                 Add(new cMenuEditSrcEItem(buffer, &lnbTypesAtTuner[tuner][i].source, DiSEqC, tuner));
                 if (extended)
                    Add(new cMenuEditStraItem(tr("   LNB Type"), &lnbTypesAtTuner[tuner][i].lnbType, 7, lofTexts));
               }

              if (extended) {
                 Add(new cMenuEditIntItem(tr("Delay (ms)"), &waitMs[tuner], 15, 100));
                 Add(new cMenuEditIntItem(tr("Repeat"), &repeat[tuner], 0, 3));
                 }
              else if (((data.DiSEqC & (SWITCHMASK << (tuner ? (tuner-1)*TUNERBITS : 0))) >> (tuner ? (tuner-1)*TUNERBITS : 0))==MINI)
                 waitMs[tuner] = 0;
              break;
     case FULL_11:
             Add(new cMenuEditIntItem(tr("Number of LNBs"), &lnbNumberAtTuner[tuner],MINLNBS,16));

             for (int i=0;i < lnbNumberAtTuner[tuner];i++) {
                 snprintf(buffer, sizeof(buffer), "LNB %c",LnbC+i);
                 Add(new cMenuEditSrcEItem(buffer, &lnbTypesAtTuner[tuner][i].source, DiSEqC, tuner));
                 if (extended)
                    Add(new cMenuEditStraItem(tr("   LNB Type"), &lnbTypesAtTuner[tuner][i].lnbType, 7, lofTexts));
               }

              if (extended) {
                 Add(new cMenuEditIntItem(tr("Delay (ms)"), &waitMs[tuner], 15, 100));
                 Add(new cMenuEditIntItem(tr("Repeat"), &repeat[tuner], 0, 3));
                 }
              else if (((data.DiSEqC & (SWITCHMASK << (tuner ? (tuner-1)*TUNERBITS : 0))) >> (tuner ? (tuner-1)*TUNERBITS : 0))==MINI)
                 waitMs[tuner] = 0;
              break;
     case DISICON4:
             lnbNumberAtTuner[tuner] = 1;
             Add(new cMenuEditSrcEItem(tr("Satellite"), &lnbTypesAtTuner[tuner][0].source, DiSEqC, tuner));
             break;
     case ROTOR_SHARED:
             lnbNumberAtTuner[tuner] = 7;
             Add(new cMenuEditRShItem(tr("Rotor on tuner"), &RotorLNBTuner[tuner], DiSEqC));
             break;
     default:
            if (extended)
                    Add(new cMenuEditStraItem(tr("   LNB Type"), &lnbTypesAtTuner[tuner][0].lnbType, 7, lofTexts));
             lnbNumberAtTuner[tuner] = 1;

     }

  SetHelp(extended? tr("Normal") : tr("Expert"), (DiSEqC[tuner] & ROTORMASK) ? tr("Rotor Settings") : NULL);
  SetCurrent(Get(current));
  Display();
}

int cMenuSetupLNB::IntCmp(const void *a, const void *b)
{
  return (* (int *)a - *(int *)b);
}

void cMenuSetupLNB::Init()
{

  // if settings changed we have to send diseqc commands by switching channel
  currentChannel = cDevice::CurrentChannel();
  if (currentChannel > Channels.Count())
     currentChannel = 1;

  if (tuner) {
     for (int t=0; t<MAXTUNERS; t++)
        DiSEqC[t+1]= (data.DiSEqC & (TUNERMASK << (TUNERBITS * t))) >> (TUNERBITS * t);
     }
  else
  {
     DLOG (DBG " for indiff Tuner setup set  DiSEqC[0] to %d ", data.DiSEqC);
     DiSEqC[0]= data.DiSEqC;
  }
  DLOG( " LNB DEBUG  cMenuSetupLNB Constr  DiSEqC[%d]: %d  ", tuner, DiSEqC[tuner]);

  for (int t=0; t<=MAXTUNERS; t++) {
     if (DiSEqC[t] & SWITCHMASK)
        Diseqc[t]=DiSEqC[t] & SWITCHMASK;
     else if (DiSEqC[t] & ROTORLNB)
        Diseqc[t] = 7;
     else if (DiSEqC[t] & GOTOX)
        Diseqc[t] = 6;
     else if (DiSEqC[t] & DISEQC12)
        Diseqc[t] = 5;
     else
        Diseqc[t] = 0;
     }

  for (int t=1; t<=MAXTUNERS; t++) {
     RotorLNBTuner[t]=(DiSEqC[t] & ROTORLNB) ? (DiSEqC[t] & 0x30) >> 4 : 0;
  }

  DLOG (DBG " Restet LNB Types & LnbNubers  ");

  for (int t=0; t<MAXTUNERS+1; t++) {
     lnbNumberAtTuner[t] = 0;
     waitMs[t] = Diseqcs.WaitMs(t);
     repeat[t] = Diseqcs.RepeatCmd(t);
     for (int lnb=0;lnb<64;lnb++) {  // MAXLNBs
        lnbTypesAtTuner[t][lnb].source = 0;
        lnbTypesAtTuner[t][lnb].lnbType = 0;
        }
     }

  DLOG (DBG " Load Actuall ");
  int index = 0;

  for (cDiseqc *d = Diseqcs.First(); d; d = Diseqcs.Next(d)) {
     int t = d->Tuner();
     //DLOG (DBG " %d.) T: %d;Source: %d ",++i , t, d->Source());
     if (index == 0) {
        lnbNumberAtTuner[t] = 1;
        lnbTypesAtTuner[t][index].source = d->Source();
        lnbTypesAtTuner[t][index].lnbType = d->LnbType();
        index++;
        DLOG (DBG " Count Lnb %d @ Tuner %d add Source %d  ", lnbNumberAtTuner[t], t, d->Source());
     }
     else if (index > 0) {
        if (lnbTypesAtTuner[t][lnbNumberAtTuner[t]-1].source != d->Source()) {
          lnbTypesAtTuner[t][lnbNumberAtTuner[t]].source = d->Source();
          lnbTypesAtTuner[t][lnbNumberAtTuner[t]].lnbType = d->LnbType();
          lnbNumberAtTuner[t]++;
          index++;
          DLOG (DBG " Count Lnb %d @ Tuner %d add Source %d  ", lnbNumberAtTuner[t], t, d->Source());
          }
        }
     }   /// Tested OK  for indifferent Settings
  for (int k=0; k<=MAXTUNERS; k++)
    for (int i=0; i<lnbNumberAtTuner[k]; i++) {
       if (lnbTypesAtTuner[k][i].source == cSource::stSat)
          switch (DiSEqC[k] & ROTORMASK) {
             case DISEQC12: lnbTypesAtTuner[k][i].source=0;
                            break;
             case GOTOX:    lnbTypesAtTuner[k][i].source=1;
                            break;
             case ROTORLNB: lnbTypesAtTuner[k][i].source = 1 + ((DiSEqC[k] & 0x30) >> 4);
                            break;
             }
       }
   DumpDiseqcs(true);
   AddDefault();
   DumpDiseqcs(true);
}


void cMenuSetupLNB::DumpDiseqcs(bool all)
{
  dsyslog(DBG  " Dump Diseqc () D.Count() %d D.LnbCount() %d ALL %c", Diseqcs.Count(), Diseqcs.LnbCount(), all?'y':'n');
  //Loading already configured LnbTypes to LnbStruct
  int limit = 0;

  for (int t = 0;t<MAXTUNERS+1;t++)
  {
     if (all)
       limit = 20;  // vor save
     else
       limit = lnbNumberAtTuner[t];
      // Count Lnb 0 @ T: 1 S: 4

      DLOG(DBG " Count Lnb %d @ T: %d ",  lnbNumberAtTuner[t], t);
      for (int lnb = 0;lnb<limit;lnb++) {
         DLOG(DBG " LNB %d points at  %d   ",lnb, lnbTypesAtTuner[t][lnb].source );
      }
  }
}

void cMenuSetupLNB::AddDefault()
{
   DLOG (DBG " AddDefault() ");

  tLnbType initTypes[] = {
   /// TODO we need to know right LNB types for each satelite
     { 35008, 0, 0 },
     { 34946, 0, 0 },
     { 35031, 0, 0 },
     { 35076, 0, 0 },
     { 35051, 0, 0 },
     { 35098, 0, 0 },
     { 35121, 0, 0 },
     { 35129, 0, 0 },
     { 32838, 0, 0 },
     { 32878, 0, 0 },
     { 34916, 0, 0 },
     { 34866, 0, 0 },
     { 34976, 0, 0 },
     { 32778, 0, 0 },
     { 35176, 0, 0 },
     { 35236, 0, 0 }
   };
  // fill up with default values to avoid string "0"  in EditSrcItem
  for (int t=0; t<MAXTUNERS+1; t++) {
     int cnt = lnbNumberAtTuner[t];
     DLOG (DBG " Procceding  through tuner %d LNBs: %d ",t, cnt);
     for (int i=0;i<16;i++) {  // runs initTypes // XXX MAXLNBS
        for(int lnb=0;lnb<cnt;lnb++) { // loop though  actuall lnbs
           //DLOG (DBG " check %d. InitSource %d vs. lnb %d s:%d   ", i, initTypes[i].source, lnb, lnbTypesAtTuner[t][lnb].source);
           if (lnbTypesAtTuner[t][lnb].source == initTypes[i].source) {
             initTypes[i].flag = 1;
             DLOG (DBG " check %d. mark  s:%d found @ lnb %d", i, initTypes[i].source, lnb);
             break; //next lnb
             }
           }
        } /* end for  LNBs */
        DLOG (DBG " Tuner %d add defaults  ", t);
#if DEBUG_DISEQC
        for (int x = 0;x < 16;x++)
        {
           if (initTypes[x].flag == 1)
             DLOG(DBG " %d.) %d  marked as found   ",x, initTypes[x].source);
        }
#endif
        int lnb = lnbNumberAtTuner[t];
        for (int i=0;i<16;i++) { // loop though  actuall lnbs ///XXX MAXLNBS
            if (initTypes[i].flag == 0 || lnbNumberAtTuner[t] == 0) {
               lnbTypesAtTuner[t][lnb].source = initTypes[i].source;
               if (lnb>=16) // MAXLNB
                  break;
               lnb++;
               }
            }
    } /* end for MAXTUNERS */ //OK testet
}

bool cMenuSetupLNB::IsUnique(int Tuner, int Source)
{
  // actual, we do not call this function
  DLOG (DBG "IsUnique Source %d @ T: %d ",Source, Tuner);
  if(Source)  {
     for (int i=0;i<0;i++) {
        if (lnbTypesAtTuner[Tuner][i].source == Source)
           return false;
        }
     return true;
     }

  int tmp[MAXLNBS] = { 0 };

  for (int i=0;i<MAXLNBS;i++) // ?? +1
     tmp[0]=lnbTypesAtTuner[Tuner][i].source;

  qsort(tmp, MAXLNBS ,sizeof(int), IntCmp);

  for (int i= 1; i< MAXLNBS;i++) {
     if (tmp[i] == tmp[i-1]&& tmp[i]!= 0)
        return false;
     }

  return true;
}

eOSState cMenuSetupLNB::Save()
{

  DLOG (DBG " cMenuSetupLNB::Save()    ");
  eOSState state = osContinue; ///???

  bool isUnique = true;
  if (!tuner)
     isUnique = IsUnique();
  else {
     for (int k=1; k<=MAXTUNERS; k++) {
        if (!cDevice::GetDevice(k-1) || !(cDevice::GetDevice(k-1)->ProvidesSource(cSource::stSat)))
           continue;
        if (!IsUnique(k))
           isUnique = false;
        }
     }
  if (!isUnique) {
     Skins.Message(mtError, tr("Sat positions must be unique!"));
     return osContinue;
     }

  if (!extended) {
     DLOG(DBG " LNB DEBUG  Not Extended ");
     for (int k = 0; k<=MAXTUNERS; k++) {
        for (int i = 1; i<lnbNumberAtTuner[k];i++) {
           lnbTypesAtTuner[k][i].lnbType = lnbTypesAtTuner[k][0].lnbType;
           }
        }
     }

  // ask user to rewrite diseqc.conf
  if (Interface->Confirm(tr("Overwrite DiSEqC.conf?"))) {

  DLOG(DBG " LNB DEBUG  Not Extended ");
  for (int i=0; i<=MAXTUNERS; i++)
     if (Diseqc[i]>4)
        lnbTypesAtTuner[i][0].source = cSource::stSat;

  //XXX
  // Diseqcs.SetLnbType(lnbTypesAtTuner[0].lnbType);

  // dsyslog ("DBG LNB_TYPE: DiSEqC: %d", data.DiSEqC);
  dsyslog ("delete all Diseqs");
  Diseqcs.Clear();

  if (!tuner) {
     Diseqcs.SetRepeatCmd(repeat[0],0);
     Diseqcs.SetWaitMs(waitMs[0],0);

     for (int i=0;i<lnbNumberAtTuner[0];i++) {
        Diseqcs.NewLnb(DiSEqC[0] & SWITCHMASK, lnbTypesAtTuner[0][i].source, lnbTypesAtTuner[0][i].lnbType);
        }
     }
 else {
    for (int k=1; k<=MAXTUNERS; k++) {
       if (!cDevice::GetDevice(k-1) || !(cDevice::GetDevice(k-1)->ProvidesSource(cSource::stSat)))
          continue;
       Diseqcs.SetRepeatCmd(repeat[k],k);
       Diseqcs.SetWaitMs(waitMs[k],k);

       for (int i=0;i<lnbNumberAtTuner[k];i++) {
          //dsyslog ("for  lnbNumberAtTuner %d newLnb(dyseqType: %d, source: %d, lnbType %d", lnbNumberAtTuner, data.DiSEqC, lnbTypesAtTuner[i].source, lnbTypesAtTuner[i].lnbType);
          Diseqcs.NewLnb(DiSEqC[k] & SWITCHMASK, lnbTypesAtTuner[k][i].source, lnbTypesAtTuner[k][i].lnbType, k);
          }
       }
    }
  // update current Setup  Object
     data.DiSEqC= tuner ? (DIFFSETUPS | (DiSEqC[4]<<(TUNERBITS*3)) | (DiSEqC[3]<<(TUNERBITS*2)) | (DiSEqC[2]<<TUNERBITS) | (DiSEqC[1])): DiSEqC[0];

    Store();
    if (data.DiSEqC) {
       Diseqcs.Save();

       // workaround to trigger diseqc codes
       Channels.SwitchTo(currentChannel+2);
       Channels.SwitchTo(currentChannel);
       }
    Skins.Message(mtInfo, tr("Changes done"),1);
    state = osUnknown;

    }
  return state;
}

eOSState cMenuSetupLNB::ProcessKey(eKeys Key)
{

  oldLnbNumber = lnbNumberAtTuner[tuner];
  int oldDiSEqC = Diseqc[tuner];
  int oldDiffSetups = diffSetups;

  if (HasSubMenu()) {
    eOSState state = smenu->ProcessKey(Key);
    if (state == osBack)
       return CloseSubMenu();
    return state;
    }

  if (Key == kOk) {
     Key = kNone;
     SetStatus(NULL);
     return Save();
  }

  eOSState state = cMenuSetupBase::ProcessKey(Key);

  //dsyslog (" lnbNumberAtTuner < oldLnbNumber  %d < %d", lnbNumberAtTuner, oldLnbNumber);

  if (Key == kGreen && (DiSEqC[tuner] & ROTORMASK)) {
     cPlugin *p = cPluginManager::GetPlugin("rotor");
     if (p) {
        int oldDiSEqC = data.DiSEqC;
        ::Setup.DiSEqC= tuner ? (DIFFSETUPS | (DiSEqC[4]<<(TUNERBITS*3)) | (DiSEqC[3]<<(TUNERBITS*2)) | (DiSEqC[2]<<TUNERBITS) | (DiSEqC[1])) : DiSEqC[0];
        AddSubMenu(smenu = (cOsdMenu *) p->MainMenuAction());
        ::Setup.DiSEqC = oldDiSEqC;
        }
     }

  if ((Key != kNone)) {
     if (Diseqc[tuner] != oldDiSEqC || lnbNumberAtTuner[tuner] != oldLnbNumber) {
        switch (Diseqc[tuner]) {
           case 5: DiSEqC[tuner] = DISEQC12;
                   break;
           case 6: DiSEqC[tuner] = GOTOX;
                   break;
           case 7: DiSEqC[tuner] = ROTORLNB + (RotorLNBTuner[tuner] << 4);
                   break;
          default: DiSEqC[tuner] = Diseqc[tuner];
                   break;
           }
        }
     if (Diseqc[tuner]<4)
        for (int i=0; i<lnbNumberAtTuner[tuner]; i++) {
            if ((lnbTypesAtTuner[tuner][i].source & cSource::st_Mask) == cSource::stNone) {
               switch (lnbTypesAtTuner[tuner][i].source) {
                  case 0: DiSEqC[tuner] = DISEQC12 | Diseqc[tuner];
                          break;
                  case 1: DiSEqC[tuner] = GOTOX | Diseqc[tuner];
                          break;
                 default: DiSEqC[tuner] = (ROTORLNB + ((lnbTypesAtTuner[tuner][i].source - 1) << 4))  | Diseqc[tuner];
                          break;
                  }
               }
            else
               DiSEqC[tuner] = Diseqc[tuner];
         }
      if (oldDiffSetups != diffSetups) {
         if (diffSetups)
            tuner=1;
         else
            tuner=0;
         }
      if (Key == kRed)
         extended = extended?false:true;

     Setup();
      } // endif other key as kOk

  return state;
}
/*
void cMenuSetupLNB::LoadActuall()
{
  dsyslog(" LoadActuall() D.Count() %d D.LnbCount() %d", Diseqcs.Count(), Diseqcs.LnbCount());
  //Loading already configured LnbTypes to LnbStruct
  for (int t=0; t<=MAXTUNERS; t++) {
     lnbNumberAtTuner[t] = 0;
     }

  if (Diseqcs.Count() == 0) {
     return;
     }

  DLOG (" LoadActuall()  ");

  /// FIXME
  for (cDiseqc *diseqc = Diseqcs.First(); diseqc; diseqc = Diseqcs.Next(diseqc)) {
     DLOG (DBG " Source: %d ", diseqc->Source());
     bool found=false;

     for (int k=0; k<lnbNumberAtTuner[diseqc->tuner()]; k++)
        if (lnbTypesAtTuner[diseqc->Tuner()][k].source == diseqc->Source())
           found=true;
     if (!found) {
        lnbTypesAtTuner[diseqc->Tuner()][lnbNumberAtTuner[diseqc->Tuner()]].source = diseqc->Source();
        lnbTypesAtTuner[diseqc->Tuner()][lnbNumberAtTuner[diseqc->Tuner()]].lnbType = diseqc->LnbType();
        lnbNumberAtTuner[diseqc->Tuner()]++;
        if (!diseqc->Tuner()) {
           for (int i=1; i<=MAXTUNERS; i++) {
              lnbTypesAtTuner[i][lnbNumberAtTuner[i]].source = diseqc->Source();
              lnbTypesAtTuner[i][lnbNumberAtTuner[i]].lnbType = diseqc->LnbType();
              lnbNumberAtTuner[i]++;
              }
           }
        }
    }

  for (int t=0; t<=MAXTUNERS; t++) {
     for (int lnb=0; lnb<lnbNumberAtTuner[t]; lnb++) {
        if (lnbTypesAtTuner[t][lnb].source == cSource::stSat) {
           switch (DiSEqC[t] & ROTORMASK) {
              case DISEQC12: lnbTypesAtTuner[t][lnb].source=0;
                             break;
              case GOTOX:    lnbTypesAtTuner[t][lnb].source=1;
                             break;
              default:       lnbTypesAtTuner[t][lnb].source = 1 + ((DiSEqC[t] & 0x30) >> 4);
              }
           }
        }
     }

  if (Tuner) {
     for (int t=1; t<=MAXTUNERS; t++) {
        for (int lnb=1; lnb<lnbNumberAtTuner[t]; lnb++) {
           if (lnbTypesAtTuner[t][lnb].lnbType!=lnbTypesAtTuner[t][0].lnbType) {
              extended=true;
              }
           }
       }
     }
  else {
     for (int lnb=1; lnb<lnbNumberAtTuner[0]; lnb++) {
        if (lnbTypesAtTuner[0][lnb].lnbType!=lnbTypesAtTuner[0][0].lnbType)
           extended=true;
        }
  }

#if 1
      dsyslog ("load actuall ");
     for (int t=1; t<=MAXTUNERS; t++) {
        for (int lnb=0;lnb<16;lnb++) { // MAXXXLNBS
          dsyslog ("found  %d LNBs @Tuner %d", lnbNumberAtTuner[t],t);
          dsyslog ("lnbTypesAtTuner[%d].source %d", lnb, lnbTypesAtTuner[t][lnb].source);
          }
       }
#endif

} */

// --- cMenuSetupCICAM -------------------------------------------------------

bool camFound[3]; //TB: has the CAM been found successfully?!
time_t firstTimeChecked[3]; //TB: first time we did check - used to determine if we assume an error

class cMenuSetupCICAMItem : public cOsdItem {
private:
  cCiHandler *ciHandler;
  int slot;
  int device;
public:
  cMenuSetupCICAMItem(int Device, cCiHandler *CiHandler, int Slot, int state, int enabled);
  cCiHandler *CiHandler(void) { return ciHandler; }
  int Slot(void) { return slot; }
  int Device(void) { return device; }
  };

cMenuSetupCICAMItem::cMenuSetupCICAMItem(int Device, cCiHandler *CiHandler, int Slot, int state, int enabled)
{
  int slot_tmp=Slot;
  ciHandler = CiHandler;
  slot = Slot;
  char buffer[64];
  const char *CamName = CiHandler->GetCamName(slot);
#if !defined(RBLITE) && !defined(CAM_NEW)
  slot=Device;
#endif
  device=Device;
  if (!CamName) {
    if (state&(3<<(16+2*slot))){
#ifdef RBLITE
      if((firstTimeChecked[slot] == 0) || ((time(NULL) - firstTimeChecked[slot]) < 10)){
#else
      if((firstTimeChecked[slot] == 0) || ((time(NULL) - firstTimeChecked[slot]) < 25)){
#endif
    CamName = tr("Init");
    if(!firstTimeChecked[slot])
      firstTimeChecked[slot] = time(NULL);
      } else {
    CamName = tr("Error");
      }
    } else {
      CamName = "-";
      camFound[slot] = 1;
      firstTimeChecked[slot] = 0;
    }
  } else {
    camFound[slot] = 1;
    firstTimeChecked[slot] = 0;
  }

  // GA: CAM enable/disable
  if (!(enabled&(1<<slot))) {
    if (state&(1<<slot))
      CamName = tr("Disabled");
    else
      CamName = tr("--OFF--");
  }

  switch(slot){
    case 0:
            snprintf(buffer, sizeof(buffer), "%s:\t%s", tr("lower slot"), CamName);
        break;
    case 1:
            snprintf(buffer, sizeof(buffer), "%s:\t%s", tr("upper slot"), CamName);
        break;
    case 2:
        snprintf(buffer, sizeof(buffer), "%s:\t%s", tr("internal CAM"), CamName);
        break;
    default:
        break;
  }
  slot=slot_tmp;
  SetText(buffer);
}

class cMenuSetupCICAM : public cMenuSetupBase {
private:
  eOSState Menu(void);
  eOSState Reset(void);
  eOSState Switch(void);
  void Update(int);
public:
  cMenuSetupCICAM(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

#define IOCTL_REEL_CI_GET_STATE _IOR('d', 0x45, int)

void cMenuSetupCICAM::Update(int cur) {
  SetCols(23);
#ifdef RBLITE
  int numDevices=1;
  SetCols(20);
  int fd,state=0;
  fd = open("/dev/reelfpga0", O_RDWR);
  if (fd) {
    state=ioctl(fd,IOCTL_REEL_CI_GET_STATE,0);
    close(fd);
  }
#else
  int numDevices=cDevice::NumDevices();
  int state=(5<<16)|3;
#endif
  SetSection(tr("Common Interface"));
  for (int d = 0; d < numDevices; d++) {
    cDevice *Device = cDevice::GetDevice(d);
    if (Device) {
      cCiHandler *CiHandler = Device->CiHandler();
      if (CiHandler) {
    for (int Slot = CiHandler->NumSlots()-1; Slot >= 0; Slot--)
      Add(new cMenuSetupCICAMItem(Device->CardIndex(), CiHandler, Slot, state, data.CAMEnabled));
      }
    }
  }
  // GA: Best way without knowing object
  for(int i=0;i<=cur;i++)
    CursorDown();
#ifdef RBLITE
  SetHelp(tr("Reset"), NULL, NULL, tr("On/Off"));
#else
  SetHelp(tr("Reset"), NULL, NULL, NULL);
#endif
}

cMenuSetupCICAM::cMenuSetupCICAM(void)
{
  Update(0);
  Display();
}

eOSState cMenuSetupCICAM::Menu(void)
{
  cMenuSetupCICAMItem *item = (cMenuSetupCICAMItem *)Get(Current());
  if (item) {
     if (item->CiHandler()->EnterMenu(item->Slot())) {
        Skins.Message(mtWarning, tr("Opening CAM menu..."));
        time_t t = time(NULL);
        while (time(NULL) - t < MAXWAITFORCAMMENU && !item->CiHandler()->HasUserIO())
              item->CiHandler()->Process();
        return osEnd; // the CAM menu will be executed explicitly from the main loop
        }
     else
        Skins.Message(mtError, tr("Can't open CAM menu!"));
     }
  return osContinue;
}

eOSState cMenuSetupCICAM::Reset(void)
{
  cMenuSetupCICAMItem *item = (cMenuSetupCICAMItem *)Get(Current());
  if (item) {
     Skins.Message(mtWarning, tr("Resetting CAM..."));
     int slot = item->Slot();
     if (item->CiHandler()->Reset(slot)) {
        Skins.Message(mtInfo, tr("CAM has been reset"));
#if defined(RBLITE) || defined(CAM_NEW)
        camFound[slot] = 0;
#else
        int device = item->Device();
    camFound[device] = 0;
#endif
        return osContinue;
        }
     else
        Skins.Message(mtError, tr("Can't reset CAM!"));
     }
  return osContinue;
}

eOSState cMenuSetupCICAM::Switch(void)  // GA
{
  cMenuSetupCICAMItem *item = (cMenuSetupCICAMItem *)Get(Current());
  if (item){
    int slot = item->Slot();
    data.CAMEnabled^=(1<<slot);

    if(!(data.CAMEnabled&(1<<slot))){
      camFound[slot] = 0;
      firstTimeChecked[slot] = 0;
    }
  }
  return osContinue;
}

time_t timeLastRefresh = 0;

eOSState cMenuSetupCICAM::ProcessKey(eKeys Key)
{
  if(Key == kOk)
    return Menu();

  eOSState state = cMenuSetupBase::ProcessKey(Key);
  int cur;

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    return Reset();
#ifdef RBLITE
       case kBlue:
             cur=Current();
             Switch();
             Clear();
             Update(cur);
             Display();
             Store();
             return osContinue;
#endif
       default: break;
       }
     }

  if((timeLastRefresh == 0) || ((time(NULL) - timeLastRefresh) >= 1)){ //TB: refresh every second
    cur=Current();
    Clear();
    Update(cur);
    Display();
    timeLastRefresh = time(NULL);
  }

  return state;
}

// --- cMenuSetupRecord ------------------------------------------------------

class cMenuSetupRecord : public cMenuSetupBase {
private:
    const char *PriorityTexts[3];
    int tmpprio, tmppauseprio;
    virtual void Store(void);
    int NoAd;
public:
    cMenuSetupRecord(void);
};

cMenuSetupRecord::cMenuSetupRecord(void)
{
  SetCols(27);
  PriorityTexts[0] = tr("low");
  PriorityTexts[1] = tr("normal");
  PriorityTexts[2] = tr("high");

  tmpprio = data.DefaultPriority == 10 ? 0 : data.DefaultPriority == 99 ? 2 : 1;
  tmppauseprio = data.PausePriority == 10 ? 0 : data.PausePriority == 99 ? 2 : 1;

  // Auto No Advt
  cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");
  NoAd = 0;
  const char* buf = cSysConfig_vdr::GetInstance().GetVariable("AUTO_NOAD");
  if (buf && strncmp(buf,"yes",3)==0)
      NoAd = 1;

  SetSection(tr("Recording"));
  //Add(new cMenuEditBoolItem(tr("Setup.Recording$Record Digital Audio"),         &data.UseDolbyInRecordings));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Timer Buffer at Start (min)"),  &data.MarginStart));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Timer Buffer at End (min)"),    &data.MarginStop));
  Add(new cMenuEditStraItem(tr("Setup.Recording$Default Priority"),             &tmpprio, 3, PriorityTexts));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default Recording Lifetime"),   &data.DefaultLifetime, 0, MAXLIFETIME, NULL, tr("unlimited")));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use VPS"),                      &data.UseVps));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Mark Instant Recording"),       &data.MarkInstantRecord));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Instant Recording Time (min)"), &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Instant Recording Lifetime"),   &data.PauseLifetime, 0, MAXLIFETIME, NULL, tr("unlimited")));
  Add(new cMenuEditStraItem( tr("Setup.Recording$Instant Recording Priority"),  &tmppauseprio, 3, PriorityTexts));
  //Add(new cMenuEditBoolItem(tr("Setup.OSD$Recording directories"),               &data.RecordingDirs));
  //Add(new cMenuEditBoolItem(tr("Setup.Recording$Split edited files"),            &data.SplitEditedFiles));
  Add(new cMenuEditBoolItem( tr("Setup.Recording$Mark Blocks after Recording"),  &NoAd));

  Display();
  };

/*
  SetSection(tr("Recording"));
  Add(new cMenuEditBoolItem( tr("Setup.Recording$Record Dolby"),         &data.UseDolbyInRecordings));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at start (min)"),     &data.MarginStart));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at stop (min)"),      &data.MarginStop));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Primary limit"),             &data.PrimaryLimit, 0, MAXPRIORITY));
  Add(new cMenuEditStraItem(tr("Setup.Recording$Default priority"),          &tmpprio, 3, PriorityTexts));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default lifetime (d)"),      &data.DefaultLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditStraItem( tr("Setup.Recording$Pause priority"),           &tmppauseprio, 3, PriorityTexts));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Pause lifetime (d)"),        &data.PauseLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use episode name"),          &data.UseSubtitle));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use VPS"),                   &data.UseVps));
  Add(new cMenuEditIntItem( tr("Setup.Recording$VPS margin (s)"),            &data.VpsMargin, 0));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Mark instant recording"),    &data.MarkInstantRecord));
  Add(new cMenuEditStrItem( tr("Setup.Recording$Name instant recording"),     data.NameInstantRecord, sizeof(data.NameInstantRecord), tr(FileNameChars)));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Instant rec. time (min)"),   &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Max. video file size (MB)"), &data.MaxVideoFileSize, MINVIDEOFILESIZE, MAXVIDEOFILESIZE));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Split edited files"),        &data.SplitEditedFiles));
}
*/

void cMenuSetupRecord::Store(void)
{
    data.DefaultPriority = tmpprio == 0 ? 10 : tmpprio == 1 ? 50 : 99;
    data.PausePriority   = tmppauseprio == 0 ? 10 : tmppauseprio == 1 ? 50 : 99;
    cMenuSetupBase::Store();

    // Store Sysconfig
    cSysConfig_vdr::GetInstance().SetVariable("AUTO_NOAD", NoAd?"yes":"no");
    cSysConfig_vdr::GetInstance().Save();

    cMenuSetupBase::Store();
};

// --- cMenuSetupReplay ------------------------------------------------------

class cMenuSetupReplay : public cMenuSetupBase {
protected:
  virtual void Store(void);
public:
  cMenuSetupReplay(void);
  };

cMenuSetupReplay::cMenuSetupReplay(void)
{
  SetCols(27);
  SetSection(tr("Replay"));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Multi speed mode"), &data.MultiSpeedMode));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Show replay mode"), &data.ShowReplayMode));
  //Add(new cMenuEditIntItem(tr("Setup.Replay$Resume ID"), &data.ResumeID, 0, 99));
  Add(new cMenuEditIntItem(tr("Setup.Replay$Jump width for green/yellow (min)"), &data.JumpWidth, 0, 10, tr("intelligent")));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Jump over cutting-mark"), &data.PlayJump));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Resume play after jump"), &data.JumpPlay));
  //Add(new cMenuEditBoolItem(tr("Setup.Replay$Pause at last mark"), &data.PauseLastMark));
  //Add(new cMenuEditBoolItem(tr("Setup.Replay$Reload marks"), &data.ReloadMarks));
  Display();
}

void cMenuSetupReplay::Store(void)
{
  if (Setup.ResumeID != data.ResumeID)
     Recordings.ResetResume();
  cMenuSetupBase::Store();
}

// --- cMenuSetupMisc --------------------------------------------------------

class cMenuSetupMisc : public cMenuSetupBase {
private:
  const char *updateChannelsTexts[3];
  const char *AddNewChannelsTexts[2];
  const char *ShutDownModes[3];
  int tmpUpdateChannels;
  int tmpAddNewChannels;
  int UpdateCheck;
  int initialVolume; // volume to be shown in percentage
  virtual void Store(void);
  void Setup(void);
public:
  cMenuSetupMisc(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupMisc::cMenuSetupMisc(void)
{
  updateChannelsTexts[0] = tr("off");
  updateChannelsTexts[1] = tr("update"); // 3
  updateChannelsTexts[2] = tr("add"); // 5

  AddNewChannelsTexts[0] = tr("at the end");
  AddNewChannelsTexts[1] = tr("to bouquets");

  tmpUpdateChannels = (int) data.UpdateChannels / 2;
  tmpAddNewChannels = data.AddNewChannels; //XXX change

  if (data.InitialVolume > 0)
      initialVolume = (int) (data.InitialVolume/2.55);
  else
      initialVolume = data.InitialVolume;
  //printf("inital volume :%i \t data.InitialVolume :%i \n", initialVolume, data.InitialVolume);
  // Update reminder
  cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");

  UpdateCheck = 0;
  const char* buf = cSysConfig_vdr::GetInstance().GetVariable("UPDATECHECK_ENABLE");
  if (buf && strncmp(buf,"yes",3)==0)
      UpdateCheck = 1;

  SetCols(26);
  SetSection(tr("Background Services"));
  Setup();
}

void cMenuSetupMisc::Setup(void)
{
  ShutDownModes[0] = tr("Request");
  ShutDownModes[1] = tr("Standby");
  ShutDownModes[2] = tr("Power off");

  int current = Current();

  Clear();

  SetHelp(tr("Button$Scan EPG"));
  //Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. event timeout (min)"),   &data.MinEventTimeout, 0, INT_MAX, tr("off")));
  //Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. user inactivity (min)"), &data.MinUserInactivity, 0, INT_MAX, tr("off")));
  //Add(new cMenuEditStraItem(tr("Setup.Miscellaneous$Power On/Off option"), &data.RequestShutDownMode, 3, ShutDownModes));
  //Add(new cMenuEditBoolItem(tr("Setup.Miscellaneous$Standby Mode"), &data.StandbyOrQuickshutdown,tr("Standard"), tr("Option$Quick")));
  //Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$SVDRP timeout (s)"),          &data.SVDRPTimeout));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Zap timeout (s)"),  &data.ZapTimeout, 0, INT_MAX, tr("off")));
  Add(new cMenuEditStraItem(tr("Setup.DVB$Autom. update channels"),            &tmpUpdateChannels, 3, updateChannelsTexts));
  if (tmpUpdateChannels==2)
      Add(new cMenuEditStraItem(tr("  Update mode"),                 &tmpAddNewChannels, 2, AddNewChannelsTexts));
  Add(new cMenuEditChanItem(tr("Setup.Miscellaneous$Initial channel"),  &data.InitialChannel, tr("Setup.Miscellaneous$as before")));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Initial volume (%)"),   &initialVolume, -1, 100, tr("Setup.Miscellaneous$as before")));

  if (::Setup.ReelboxModeTemp == eModeStandalone || ::Setup.ReelboxModeTemp == eModeServer)
     Add(new cMenuEditIntItem( tr("Setup.EPG$EPG scan timeout (h)"),      &data.EPGScanTimeout, 0, INT_MAX, tr("off")));

  Add(new cMenuEditBoolItem( tr("Setup.Miscellaneous$Update-check on system start"),   &UpdateCheck));

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupMisc::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupBase::ProcessKey(Key);
     if (Key == kRed) {
        EITScanner.ForceScan();
        return osEnd;
     } else if (Key == kLeft || Key == kRight) {
        Setup();
     }
  return state;
}

void cMenuSetupMisc::Store(void)
{

   //printf("\ninitial volume(user selected): %i\n", initialVolume);
   if (initialVolume > 0)
   data.InitialVolume = (int) (2.55*(initialVolume + 0.5)); // round, since (int)(2.55*100) gives 254 probably is 254.9999
   else
   data.InitialVolume = initialVolume;

   //printf("initial volume (after percent to vdr conversion): %i\n\n", data.InitialVolume);

  if ( tmpUpdateChannels == 0 )
     data.UpdateChannels = 0;
  else if  ( tmpUpdateChannels == 1 )
     data.UpdateChannels = 3;
  else if  ( tmpUpdateChannels == 2 )
     data.UpdateChannels = 5;

  data.AddNewChannels = tmpAddNewChannels;

  // Store Sysconfig
  //cSysConfig_vdr::GetInstance().SetVariable("AUTO_NOAD", NoAd?"yes":"no");
  cSysConfig_vdr::GetInstance().SetVariable("UPDATECHECK_ENABLE", UpdateCheck?"yes":"no");
  cSysConfig_vdr::GetInstance().Save();

  //
  /*
  // Check for change
  //
  const char* buf = cSysConfig_vdr::Instance().GetVariable("AUTO_NOAD");
  if (NoAd)
  {
      if ( !buf || strncmp(buf,"yes") !=0)
      {
          cSysConfig_vdr::Instance().SetVariable("AUTO_NOAD", "yes");
          cSysConfig_vdr::Instance().Save()
      }
  }
  else
  {
      if ( !buf || strncmp(buf,"no") !=0)
      {
          cSysConfig_vdr::Instance().SetVariable("AUTO_NOAD", "no");
          cSysConfig_vdr::Instance().Save()
      }
  }
  */

  cMenuSetupBase::Store();
}


// --- cMenuSetupLiveBuffer --------------------------------------------------

class cMenuSetupLiveBuffer : public cMenuSetupBase {
private:
  void Setup();
#if 1
  bool hasHD;
#endif
public:
  eOSState ProcessKey(eKeys Key);
  cMenuSetupLiveBuffer(void);
  };

cMenuSetupLiveBuffer::cMenuSetupLiveBuffer(void)
{
  SetSection(tr("Permanent Timeshift"));
  SetCols(25);

  static const char *cmd = "cat /proc/mounts |awk '{ print $2 }'|grep -q '/media/hd'";
  hasHD = !SystemExec(cmd);

  Setup();
}

void cMenuSetupLiveBuffer::Setup(void)
{
  int current=Current();
  Clear();

  if (hasHD)
  {
        Add(new cMenuEditBoolItem(tr("Permanent Timeshift"),                   &data.LiveBuffer));
        if (data.LiveBuffer)
        {
            Add(new cMenuEditIntItem(tr("Setup.LiveBuffer$Buffer (min)"),      &data.LiveBufferSize, 1, 60));
        }
  } else
  {
        std::string buf = std::string(tr("Permanent Timeshift")) + ":\t" + tr("no");
        Add(new cOsdItem(buf.c_str(), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Info:"), osUnknown, false));
        if (data.LiveBuffer)  // is currently on
        {
            data.LiveBuffer = 0;
            Store();
            AddFloatingText(tr("Permanent Timeshift has been disabled."), 48);
        }
        AddFloatingText(tr("Permanent Timeshift can only be activated with a harddisk."), 48);
  }

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupLiveBuffer::ProcessKey(eKeys Key)
{
  int oldLiveBuffer = data.LiveBuffer;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  static const char *command = "reelvdrd --livebufferdir";
  char *strBuff = NULL;
  FILE *process;
  bool dirok;

  if (Key != kNone && (data.LiveBuffer != oldLiveBuffer))
  {
     Setup();
  }


  if (Key == kOk && data.LiveBuffer == 1)
  {
        Store();

	process = popen(command, "r");
	if (process)
	{
		cReadLine readline;
		//DDD("strBuff = readline...");
		strBuff = readline.Read(process);
		if (strBuff != NULL)
		    BufferDirectory = strdup(strBuff);
		DDD("Permanent timeshift on, BufferDirectory: %s", BufferDirectory);
	}

	dirok = !pclose(process);

  }

  // TODO: re-tune to activate/deactivate Live-Buffer?

  return state;
}


// --- cMenuSetupPluginItem --------------------------------------------------

class cMenuSetupPluginItem : public cOsdItem {
private:
  int pluginIndex;
public:
  cMenuSetupPluginItem(const char *Name, int Index);
  int PluginIndex(void) { return pluginIndex; }
  };

cMenuSetupPluginItem::cMenuSetupPluginItem(const char *Name, int Index)
:cOsdItem(Name)
{
  pluginIndex = Index;
}

// --- cMenuSetupPlugins -----------------------------------------------------

class cMenuSetupPlugins : public cMenuSetupBase {
public:
  cMenuSetupPlugins(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupPlugins::cMenuSetupPlugins(void)
{
    //printf("\n%s\n",__PRETTY_FUNCTION__);
    SetSection(tr("System Settings"));
    SetHasHotkeys();
    for (unsigned int i = 0; i < cPluginManager::PluginCount() ; i++) {
        cPlugin *p = cPluginManager::GetPlugin(i);
        if (p && p->HasSetupOptions()){
            char *buffer = NULL;
            if(!p->MenuSetupPluginEntry())
                asprintf(&buffer, "%s", p->MainMenuEntry());
            else
                asprintf(&buffer, "%s", p->MenuSetupPluginEntry());
            Add(new cMenuSetupPluginItem(hk(buffer), i));
            free(buffer);
        } else
            //break;
            esyslog("cMenuSetupPlugins: (%s,%i) error getting plugin #%i", __FILE__, __LINE__, i);
    }
    Display();
}

eOSState cMenuSetupPlugins::ProcessKey(eKeys Key)
{

  //printf("\n%s :  \t HasSubMenu()= %d\n",__PRETTY_FUNCTION__, HasSubMenu());
  eOSState state = HasSubMenu() ? cMenuSetupBase::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (Key == kOk) {
     if (state == osUnknown) {
        cMenuSetupPluginItem *item = (cMenuSetupPluginItem *)Get(Current());
        if (item) {
           cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
           if (p) {
              cMenuSetupPage *menu = p->SetupMenu();
              if (menu) {
                 menu->SetPlugin(p);
                 return AddSubMenu(menu);
                 }
              Skins.Message(mtInfo, tr("This plugin has no setup parameters!"));
              }
           }
        }
     else if (state == osContinue)
        {
//  Store();
    }
     }
  return state;
}

// --- cMenuSetup ------------------------------------------------------------

class cMenuSetup : public cOsdMenu {
private:
  virtual void Set(void);
  eOSState Restart(void);
public:
  cMenuSetup(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetup::cMenuSetup(void)
:cOsdMenu("")
{
  Set();
  //printf("\n%s\n", __PRETTY_FUNCTION__);
}

void cMenuSetup::Set(void)
{
  Clear();
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s - VDR %s", tr("Title$Setup"), VDRVERSION);
  SetTitle(buffer);
  SetHasHotkeys();
  Add(new cOsdItem(hk(tr("LNB")),           osUser4));
  Add(new cOsdItem(hk(tr("OSD")),           osUser1));
  Add(new cOsdItem(hk(tr("EPG")),           osUser2));
  Add(new cOsdItem(hk(tr("DVB")),           osUser3));
  Add(new cOsdItem(hk(tr("CICAM")),         osUser5));
  Add(new cOsdItem(hk(tr("Recording settings")),     osUser6));
  Add(new cOsdItem(hk(tr("Replay settings")),        osUser7));
  Add(new cOsdItem(hk(tr("Miscellaneous")), osUser8));
  Add(new cOsdItem(hk(tr("Permanent Timeshift")), osLiveBuffer));
  if (cPluginManager::HasPlugins())
  Add(new cOsdItem(hk(tr("Plugins")),       osUser9));
  Add(new cOsdItem(hk(tr("Restart")),       osUser10));
}

eOSState cMenuSetup::Restart(void)
{
  if (Interface->Confirm(tr("Really restart?"))
     && (!cRecordControls::Active() || Interface->Confirm(tr("Recording - restart anyway?")))
     && !cPluginManager::Active(tr("restart anyway?"))) {
     cThread::EmergencyExit(true);
     return osEnd;
     }
  return osContinue;
}

eOSState cMenuSetup::ProcessKey(eKeys Key)
{
  //int osdLanguage = Setup.OSDLanguage;
  eOSState state = cOsdMenu::ProcessKey(Key);
  //DDD("%s", __PRETTY_FUNCTION__);
  switch (state) {
    case osUser1: return AddSubMenu(new cMenuSetupOSD);
    case osUser2: return AddSubMenu(new cMenuSetupEPG);
    case osUser3: return AddSubMenu(new cMenuSetupDVB);
    case osUser4: return AddSubMenu(new cMenuSetupLNB);
    case osUser5: return AddSubMenu(new cMenuSetupCICAM);
    case osUser6: return AddSubMenu(new cMenuSetupRecord);
    case osUser7: return AddSubMenu(new cMenuSetupReplay);
    case osUser8: return AddSubMenu(new cMenuSetupMisc);
    case osUser9: return AddSubMenu(new cMenuSetupPlugins);
    case osUser10: return Restart();
    case osLiveBuffer: return AddSubMenu(new cMenuSetupLiveBuffer);
    default: ;
    }
#if 0
  if (Setup.OSDLanguage != osdLanguage) {
     Set();
     if (!HasSubMenu())
        Display();
     }
#endif
  return state;
}

// --- cMenuPluginItem -------------------------------------------------------

class cMenuPluginItem : public cOsdItem {
private:
  int pluginIndex;
  std::string Link_;
public:
  cMenuPluginItem(const char *Name, int Index, std::string Link);
  int PluginIndex(void) { return pluginIndex; }
  std::string Link() { return Link_; }
  };

cMenuPluginItem::cMenuPluginItem(const char *Name, int Index, std::string Link)
:cOsdItem(Name, osPlugin)
{
  Link_ = Link;
  pluginIndex = Index;
  std::string strBuff;
  strBuff.assign(Link, 0, std::string(Link).length() - std::string(Name).length());
  strBuff.append(tr(Name));
  SetText(strBuff.c_str());
}

// --- cMenuMain -------------------------------------------------------------

#define STOP_RECORDING tr(" Stop recording ")

cOsdObject *cMenuMain::pluginOsdObject = NULL;

cMenuMain::cMenuMain(eOSState State)
:cOsdMenu("")
{

  /* TB: no help pages in channels-menu */
  if (State != osChannels && State != osTimers) {
     // Load Menu Configuration
     HelpMenus.Load();
  }

  /* TB: no need to parse vdr-menu.xml for channels- or setup-menu */
  if (State != osChannels && State != osSetup && State != osTimers) {
     char menuXML[256];
     snprintf(menuXML, 256, "%s/setup/vdr-menu.xml", cPlugin::ConfigDirectory());
     subMenu.LoadXml(menuXML);
  }

  if(!subMenu.isTopMenu()) {
    int level = 0;
    bool ret;
    do {
      ret = subMenu.Up(&level);
    } while (ret && level != 0);
  }

  nrDynamicMenuEntries=0;

  lastDiskSpaceCheck = 0;
  lastFreeMB = 0;
  replaying = false;
  stopReplayItem = NULL;
  cancelEditingItem = NULL;
  stopRecordingItem = NULL;
  recordControlsState = 0;

  // Initial submenus:
  switch (State) {
    case osSchedule:   AddSubMenu(new cMenuSchedule); break;
    case osActiveBouquet:
    case osAddFavourite:
    case osBouquets:
    case osFavourites:
    case osChannels:   //AddSubMenu(new cMenuChannels()); break;
                       DDD("osChannels before osChannelsHook");
#ifdef INTERNAL_BOUQUETS

                       if (Setup.UseBouquetList==1)
                           AddSubMenu(new cMenuBouquets(0));
                       else if(Setup.UseBouquetList==2)
                           AddSubMenu(new cMenuBouquets(1));
                       else
                           AddSubMenu(new cMenuBouquets(4));
                       break;
#else
                       struct osChannelsHook_Data
                       {
                           eChannellistMode Function;      /*IN*/
                           cOsdMenu        *pResultMenu;   /*OUT*/
                       } data;
                       memset(&data, 0, sizeof(data));
                       switch (State)
                       {
                           case osChannels:      data.Function = Default; break;
                           case osActiveBouquet: data.Function = currentBouquet; break;
                           case osBouquets:      data.Function = bouquets; break;
                           case osFavourites:    data.Function = favourites; break;
                           case osAddFavourite:  data.Function = addToFavourites; break;
                           default: break;
                       }
                       if (!cPluginManager::CallFirstService("osChannelsHook", &data))
                           AddSubMenu(new cMenuChannels());
                       else
                           AddSubMenu(data.pResultMenu);
                       break;
#endif

    case osTimers:     AddSubMenu(new cMenuTimers); break;
    case osRecordings: AddSubMenu(new cMenuRecordings(NULL, 0, true)); break;
    case osSetup:      AddSubMenu(new cMenuSetup); break;
    case osCommands:   AddSubMenu(new cMenuCommands(tr("Commands"), &Commands)); break;
    case osOSDSetup:   AddSubMenu(new cMenuSetupOSD); break;
    case osLanguage:   AddSubMenu(new cMenuSetupLang); break;
    case osTimezone:
                       {
                           cPlugin *SetupTimePlugin = cPluginManager::GetPlugin("setup");
                           if (SetupTimePlugin)
                           {
                               struct {
                                   cOsdMenu* menu;
                               } data;

                               SetupTimePlugin->Service("Setup Timezone", &data);

                               AddSubMenu(data.menu);
                           }
                       }
                       break;
    case osTimeshift:  AddSubMenu(new cMenuSetupLiveBuffer); break;
    case osActiveEvent:AddSubMenu(new cMenuActiveEvent); break;
    default: Set(); break;
    }
}

cOsdObject *cMenuMain::PluginOsdObject(void)
{
  cOsdObject *o = pluginOsdObject;
  pluginOsdObject = NULL;
  return o;
}

void cMenuMain::Set(int current)
{
  Clear();
  //TB SetTitle("VDR");
  SetHasHotkeys();

// *** START PATCH SETUP
  stopReplayItem = NULL;
  cancelEditingItem = NULL;
  stopRecordingItem = NULL;

  // remember initial dynamic MenuEntries added
  int index = 0;
  nrDynamicMenuEntries = Count();
  int icon_number = 0;
  for (cSubMenuNode *node = subMenu.GetMenuTree()->First(); node;
       node = subMenu.GetMenuTree()->Next(node))
  {
    cSubMenuNode::Type type = node->GetType();
  if(node->GetIconNumber())
  {
      icon_number = atoi(node->GetIconNumber());
  }
  else icon_number = 0;

    if(type != cSubMenuNode::UNDEFINED)
    {
      if (!HasSubMenu() && current == index)
      {
         SetStatus(node->GetInfo());
      }
    }

    if(type==cSubMenuNode::PLUGIN)
    {
        const char *item = node->GetPluginMainMenuEntry();

        char *link = NULL;
        if(item)
        {
            if (icon_number)
            {
                asprintf(&link, "%c %s",char(icon_number), item);
                Add(new cMenuPluginItem(item, node->GetPluginIndex(), link ));
                free(link);
            }
            else
                Add(new cMenuPluginItem(item, node->GetPluginIndex(), hk(item)));
        }
    }
    else if(type==cSubMenuNode::MENU)
    {
      char *item = NULL;
      if( icon_number)
      {
          asprintf(&item, "%c %s%s", icon_number, tr(node->GetName()), subMenu.GetMenuSuffix());
          Add(new cOsdItem(item));
      }
      else
      {
          asprintf(&item, "%s%s", tr(node->GetName()), subMenu.GetMenuSuffix());
          Add(new cOsdItem(hk(item)));
      }
      free(item);
    }
    else
      if(type==cSubMenuNode::COMMAND)
    {
      char *link=NULL;
      if (icon_number)
      {
          asprintf(&link, "%c %s", icon_number, tr(node->GetName()));
          Add(new cOsdItem(link));
          free(link);
      }
      else
          Add(new cOsdItem(hk(tr(node->GetName()))));
    }
    else
      if(type==cSubMenuNode::SYSTEM)
    {
      const char *item = node->GetName();
      char *link = NULL;
      if (icon_number)
          asprintf(&link, "%c %s", icon_number, tr(item));
      else
          asprintf(&link, "%s",  hk(tr(item)));

      if(strcmp(item, "Schedule") == 0)
          //Add(new cOsdItem(hk(tr("Schedule")),   osSchedule));
          Add(new cOsdItem(link,   osSchedule));
      else
        if(strcmp(item, trNOOP("Channel list")) == 0)
          //Add(new cOsdItem(hk(tr("Channels Editor")),   osChannels));
          Add(new cOsdItem(link,   osChannels));
      else
        if(strcmp(item, trNOOP("Edit Channels")) == 0)
          //Add(new cOsdItem(hk(tr("Channels Editor")),   osChannels));
          Add(new cOsdItem(link,   osChannels));
      else
        if(strcmp(item, "Timers") == 0)
          //Add(new cOsdItem(hk(tr("Timers")),   osTimers));
          Add(new cOsdItem(link,   osTimers));
      else
        if(strcmp(item, "Recordings") == 0)
          //Add(new cOsdItem(hk(tr("Recordings")),   osRecordings));
          Add(new cOsdItem(link,   osRecordings));
      else
        if(strcmp(item, "Setup") == 0)
          //Add(new cOsdItem(hk(tr("Title$Setup")),   osSetup));
          Add(new cOsdItem(link,   osSetup));
      else
        if(strcmp(item, "Commands") == 0 && Commands.Count()>0)
          //Add(new cOsdItem(hk(tr("Commands")),   osCommands));
          Add(new cOsdItem(link,   osCommands));
      else
        if(strcmp(item, "OSD Settings") == 0)
          //Add(new cOsdItem(hk(tr("OSD Settings")),   osOSDSetup));
          Add(new cOsdItem(link,   osOSDSetup));
      else
        if(strcmp(item, "Language Settings") == 0)
          //Add(new cOsdItem(hk(tr("Language Settings")),   osLanguage));
          Add(new cOsdItem(link,   osLanguage));
      else
        if(strcmp(item, "Timezone Settings") == 0)
          //Add(new cOsdItem(hk(tr("Clock Settings")),   osTimezone));
          Add(new cOsdItem(link,   osTimezone));
      else
        if(strcmp(item, "Permanent Timeshift") == 0)
          //Add(new cOsdItem(hk(tr("Permanent Timeshift")),   osTimeshift));
          Add(new cOsdItem(link,   osTimeshift));
    }
    index++;
  }
  if(current >=0 && current<Count())
  {
    SetCurrent(Get(current));
  }


// *** END PATCH SETUP

/* original
  // Basic menu items:

  Add(new cOsdItem(hk(tr("Schedule")),   osSchedule));
  Add(new cOsdItem(hk(tr("Channels")),   osChannels));
  Add(new cOsdItem(hk(tr("Timers")),     osTimers));
  Add(new cOsdItem(hk(tr("Recordings")), osRecordings));

  // Plugins:

  for (int i = 0; ; i++) {
      cPlugin *p = cPluginManager::GetPlugin(i);
      if (p) {
         const char *item = p->MainMenuEntry();
         if (item)
            Add(new cMenuPluginItem(hk(item), i));
         }
      else
         break;
      }

  // More basic menu items:

  Add(new cOsdItem(hk(tr("Title$Setup")),      osSetup));
  if (Commands.Count())
     Add(new cOsdItem(hk(tr("Commands")),  osCommands));

----- end origiginal */

  Update(true);

  Display();
}

#define MB_PER_MINUTE 25.75 // this is just an estimate!

bool cMenuMain::Update(bool Force)
{
  bool result = false;
  cOsdItem *fMenu = NULL;
  if( Force && subMenu.isTopMenu())
  {
    fMenu = First();
    nrDynamicMenuEntries = 0;
  }

  if( subMenu.isTopMenu())
  {

#ifdef RBLITE
  // Title with disk usage:
  if (Force || time(NULL) - lastDiskSpaceCheck > DISKSPACECHEK) {
     int FreeMB;
     int Percent = VideoDiskSpace(&FreeMB);
     if (Force || FreeMB != lastFreeMB) {
        int Minutes = int(double(FreeMB) / MB_PER_MINUTE);
        int Hours = Minutes / 60;
        Minutes %= 60;
        char buffer[60];

        snprintf(buffer, sizeof(buffer), "%s - %s %d%% %2d:%02dh %s", tr("Title$Menu"), tr("Disk"), Percent, Hours, Minutes, tr("free"));
        SetTitle(buffer);
        result = true;
        }
     lastDiskSpaceCheck = time(NULL);
     }

#else
#if 0
        if (strcmp(Skins.Current()->Name(), "Reel") == 0)
        {
            //percent string
            char percent_str[13];
            percent_str[0] = '[';  percent_str[13] = 0;  percent_str[12] = ']';
            for ( int j= 1; j < 12; ++j)
                if ( j*10 < Percent )
                    percent_str[j] = '|';
                else percent_str[j] = ' ';

            snprintf(buffer, sizeof(buffer), "%s - %s %s  %2d:%02dh %s", tr("Title$Main Menu"), tr("Disk"), percent_str, Hours, Minutes, tr("free"));
        }
        else
            snprintf(buffer, sizeof(buffer), "%s - %s %d%% %2d:%02dh %s", tr("Title$Main Menu"), tr("Disk"), Percent, Hours, Minutes, tr("free"));
        //XXX -> skin function!!!
#endif

        SetTitle(tr("Title$Main Menu"));

#endif
   } else {
     SetTitle(tr(subMenu.GetParentMenuTitel()));
     return(true);
   }

  bool NewReplaying = cControl::Control() != NULL;
  if (Force || NewReplaying != replaying) {
     replaying = NewReplaying;
     // Replay control:
     if (replaying && !stopReplayItem)
        Add(stopReplayItem = new cOsdItem(tr(" Stop replaying"), osStopReplay));
     else if (stopReplayItem && !replaying) {
        Del(stopReplayItem->Index());
        stopReplayItem = NULL;
        }
     // Color buttons:
//     SetHelp(!replaying ? tr("Button$Record") : NULL, tr("Button$Audio"), replaying ? NULL : tr("Button$Pause"), replaying ? tr("Button$Stop") : cReplayControl::LastReplayed() ? tr("Button$Resume") : NULL);
     if(!replaying)
         SetHelp(tr("Button$Recordings"), tr("EPG"), tr("Favorites"), tr("Button$Multifeed"));
     else
         SetHelp(NULL, NULL, NULL, NULL);
     result = true;
     }

  // Editing control:
  bool CutterActive = cCutter::Active();
  if (CutterActive && !cancelEditingItem) {
     Add(cancelEditingItem = new cOsdItem(tr(" Cancel editing"), osCancelEdit));
     result = true;
     }
  else if (cancelEditingItem && !CutterActive) {
     Del(cancelEditingItem->Index());
     cancelEditingItem = NULL;
     result = true;
     }

  // Record control:
  if ( 1 || cRecordControls::StateChanged(recordControlsState)) { // balaji: this condition does not redraw recordings-items on mainmenu when a submenu is entered into and returned back to main-menu via kExit
     while (stopRecordingItem) {
           cOsdItem *it = Next(stopRecordingItem);
           Del(stopRecordingItem->Index());
           stopRecordingItem = it;
           }
     const char *s = NULL;
     int count = 4; // show only 4 current-recordings
                    // GetInstantId(s,true) true == in the reverse, Last started is give out first
#ifdef USEMYSQL
     if(Setup.ReelboxModeTemp != eModeServer)
     {
#endif
         //printf("Last recording %s\n", cRecordControls::GetInstantId(NULL,false));
         while (count && (s = cRecordControls::GetInstantId(s,true)) != NULL) {
             char *buffer = NULL;
             asprintf(&buffer, "%s%s", STOP_RECORDING, s);
             cOsdItem *item = new cOsdItem(osStopRecord);
             --count;
             item->SetText(buffer, false);
             Add(item);
             if (!stopRecordingItem)
                 stopRecordingItem = item;
         }
#ifdef USEMYSQL
     }
     if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer))
     {
         std::vector<cTimer*> InstantRecordings;
         Timers.GetInstantRecordings(&InstantRecordings);
         if(InstantRecordings.size())
         {
             std::vector<cTimer*>::iterator TimerIterator = InstantRecordings.begin();
             while(count && TimerIterator != InstantRecordings.end())
             {
                 if((*TimerIterator)->Recording())
                 {
                     char *buffer = NULL;
                     asprintf(&buffer, "%s%s", STOP_RECORDING, (*TimerIterator)->Channel()->Name());
                     cOsdItem *item = new cOsdItem(osStopRecord);
                     --count;
                     item->SetText(buffer, false);
                     Add(item);
                     if (!stopRecordingItem)
                         stopRecordingItem = item;
                 }
                 ++TimerIterator;
             }
         }
     }
#endif
     result = true;
     }

  // adjust nrDynamicMenuEntries
  if( fMenu != NULL)
     nrDynamicMenuEntries = fMenu->Index();

  return result;
}


eOSState cMenuMain::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);
  HadSubMenu |= HasSubMenu();

  switch (state) {
    case osSchedule:   return AddSubMenu(new cMenuSchedule);
    case osEditChannels:
    case osChannels:
                       //DDD("before new cMenuChannels");
                       //return AddSubMenu(new cMenuChannels);
                       struct osChannelsHook_Data
                       {
                           eChannellistMode Function;      /*IN*/
                           cOsdMenu        *pResultMenu;   /*OUT*/
                       } data;
                       memset(&data, 0, sizeof(data));
                       data.Function = Default;
                       if (cPluginManager::CallFirstService("osChannelsHook", &data))
                           return AddSubMenu(data.pResultMenu);
                       else
                           return AddSubMenu(new cMenuChannels());
                       break;
    case osTimers:     return AddSubMenu(new cMenuTimers);
    case osRecordings: return AddSubMenu(new cMenuRecordings);
    case osSetup:      return AddSubMenu(new cMenuSetup);
    case osCommands:   return AddSubMenu(new cMenuCommands(tr("Commands"), &Commands));
    case osOSDSetup:   return AddSubMenu(new cMenuSetupOSD);
    case osLanguage:   return AddSubMenu(new cMenuSetupLang);
    case osTimezone:
                       {
                           cPlugin *SetupTimePlugin = cPluginManager::GetPlugin("setup");
                           if (SetupTimePlugin)
                           {
                               struct {
                                   cOsdMenu* menu;
                               } data;

                               SetupTimePlugin->Service("Setup Timezone", &data);

                               return AddSubMenu(data.menu);
                           }
                       }
    case osTimeshift:  return AddSubMenu(new cMenuSetupLiveBuffer);
    case osStopRecord: if (Interface->Confirm(tr("Stop recording?"))) {
                          cOsdItem *item = Get(Current());
                          if (item) {
#ifdef USEMYSQL
                              if((Setup.ReelboxModeTemp == eModeClient) || (Setup.ReelboxModeTemp == eModeServer)) { // Client & Server
                                bool SetServer = false;
                                bool res = false;
                                // Remove Timer from Database
                                cTimersMysql *TimersMysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
                                if((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP)) // Client
                                  SetServer = TimersMysql->SetServer(Setup.NetServerIP);
                                else
                                  SetServer = TimersMysql->SetServer("localhost");
                                if(SetServer)
                                {
                                    std::vector<cTimer*> InstantRecordings;
                                    Timers.GetInstantRecordings(&InstantRecordings);
                                    bool found = false;
                                    unsigned int i=0;
                                    while(!found && (i < InstantRecordings.size()))
                                    {
                                        if(!strcmp(InstantRecordings.at(i)->Channel()->Name(), item->Text() + strlen(STOP_RECORDING)))
                                        {
                                            res = TimersMysql->DeleteTimer(InstantRecordings.at(i)->GetID());
                                            found = true;
                                        }
                                        ++i;
                                    }
                                }
                                delete TimersMysql;
                                return osEnd;
                              } else
#endif
                             cRecordControls::Stop(item->Text() + strlen(STOP_RECORDING));
                             return osEnd;
                             }
                          }
                       break;
    case osCancelEdit: if (Interface->Confirm(tr("Cancel editing?"))) {
                          cCutter::Stop();
                          return osEnd;
                          }
                       break;
    case osPlugin:     {
                         cMenuPluginItem *item = (cMenuPluginItem *)Get(Current());
                         if (item) {
                            cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
                            if (p) {
                               if ( strcmp(p->Name(), "setup") == 0)
                               {
                                   /* eg. '  3  link name'
                                    * looking for the second word */
                                   const char *tmp = strchr(first_non_space_char(item->Link().c_str()),' ');
                                   tmp = first_non_space_char(tmp);
                                   //printf("(%s:%i)\033[0;91mlink='%s'\033[0m '%s'\n",  __FILE__,__LINE__, tmp,item->Link().c_str());
                                   p->Service("link", (void *)tmp);
                               }
                               if (!cStatus::MsgPluginProtected(p)) {  // PIN PATCH
                               cOsdObject *menu = p->MainMenuAction();
                               if (menu) {
                                  if (menu->IsMenu())
                                     return AddSubMenu((cOsdMenu *)menu);
                                  else {
                                     pluginOsdObject = menu;
                                     return osPlugin;
                                     }
                                  }
                               }
                            }
                         }
                         state = osEnd;
                       }
                       break;
    case osBack:   {
                    int newCurrent =0;
                    if (subMenu.Up(&newCurrent) )
                    {
                     Set(newCurrent);
                     return osContinue;
                    }
                    else
                        return osEnd;
                    }
                    break;
    default: switch (Key) {

               //case kInfo: if (!HadSubMenu) return DisplayHelp((Current() - nrDynamicMenuEntries));
               case kRecord:
               case kRed:    if (!HadSubMenu)
                             {
                                 if(!replaying)
                                     cRemote::CallPlugin("extrecmenu");
                                 state = osContinue;
                             }
                             break;
               case kGreen:  if (!HadSubMenu)
                             {
                                 if(!replaying)
                                     cRemote::CallPlugin("epgsearch");
                                 state = osContinue;
                             }
                             break;
               case kYellow: /*if (!HadSubMenu)
                             {
                                 if(!replaying)
                                 {
                                     //AddSubMenu(new cMenuBouquets(2));
                                     AddSubMenu(GetBouquetMenu(2));
                                 }
                                 state = osContinue;
                             }
                             */
                             ///TODO: modifier: Favourites
                             state = osChannels;
                             break;
               case kBlue:   if (!HadSubMenu)
                             {
                                 if(!replaying)
                                     cRemote::CallPlugin("arghdirector");
                                 state = osContinue;
//                                state = replaying ? osStopReplay : cReplayControl::LastReplayed() ? osReplay : osContinue;
                             }
                             break;
               case kOk:    if(state == osUnknown)
                            {
                              int index = Current()-nrDynamicMenuEntries;
                              cSubMenuNode *node = subMenu.GetNode(index);

                              if ( node != NULL)
                              {
                                if(node->GetType() == cSubMenuNode::MENU)
                                {
                                  subMenu.Down(index);
                                }
                                else
                                  if(node->GetType() == cSubMenuNode::COMMAND)
                                {
                                  char *buffer = NULL;
                                  bool confirmed = true;
                                  if( node->CommandConfirm())
                                  {
                                    asprintf(&buffer, "%s?", node->GetName());
                                    confirmed = Interface->Confirm(buffer);
                                    free(buffer);
                                  }
                                  if (confirmed)
                                  {
                                    asprintf(&buffer, "%s...", node->GetName());
                                    Skins.Message(mtStatus, buffer);
                                    free(buffer);
                                    const char *Result = subMenu.ExecuteCommand(node->GetCommand());
                                    Skins.Message(mtStatus, NULL);
                                    if (Result)
                                      return AddSubMenu(new cMenuText(node->GetName(), Result, fontFix));

                                    return osEnd;
                                  }
                                }
                              }

                              Set();
                              return state;
                            }
               break;
               default:  break;
        }
    }
#if 0 //TB: why this??? this slows down rendering of submenus extremely
  if (!HasSubMenu() && Update(HadSubMenu))
     Display();
#endif
  if (Key != kNone) {

      int index = Current() - nrDynamicMenuEntries;
      cSubMenuNode *node = subMenu.GetNode(index);

      if (!HasSubMenu() && node)
      {
         if (node->GetInfo())
              SetStatus(node->GetInfo());
         else
           SetStatus(NULL);
      }
  }
  return state;
}

// --- SetTrackDescriptions --------------------------------------------------

static void SetTrackDescriptions(int LiveChannel)
{
  cDevice::PrimaryDevice()->ClrAvailableTracks(true);
  const cComponents *Components = NULL;
  cSchedulesLock SchedulesLock;
  if (LiveChannel) {
     cChannel *Channel = Channels.GetByNumber(LiveChannel);
     if (Channel) {
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
        if (Schedules) {
           const cSchedule *Schedule = Schedules->GetSchedule(Channel);
           if (Schedule) {
              const cEvent *Present = Schedule->GetPresentEvent();
              if (Present)
                 Components = Present->Components();
              }
           }
        }
     }
  else if (cReplayControl::NowReplaying()) {
     cThreadLock RecordingsLock(&Recordings);
     cRecording *Recording = Recordings.GetByName(cReplayControl::NowReplaying());
     if (Recording)
        Components = Recording->Info()->Components();
     }
  if (Components) {
     int indexAudio = 0;
     int indexDolby = 0;
     int indexSubtitle = 0;
     for (int i = 0; i < Components->NumComponents(); i++) {
         const tComponent *p = Components->Component(i);
         switch (p->stream)
         {
             case 2:
                 if (p->type == 0x05)
                     cDevice::PrimaryDevice()->SetAvailableTrack(ttDolby, indexDolby++, 0, LiveChannel ? NULL : p->language, p->description);
                 else
                     cDevice::PrimaryDevice()->SetAvailableTrack(ttAudio, indexAudio++, 0, LiveChannel ? NULL : p->language, p->description);
                 break;
             case 3:
                 cDevice::PrimaryDevice()->SetAvailableTrack(ttSubtitle, indexSubtitle++, 0, LiveChannel ? NULL : p->language, p->description);
                 break;
            }
         }
     }
}

// --- cDisplayChannel -------------------------------------------------------

#define DIRECTCHANNELTIMEOUT 1500 //ms

cDisplayChannel *cDisplayChannel::currentDisplayChannel = NULL;

cDisplayChannel::cDisplayChannel(int Number, bool Switched)
:cOsdObject(true)
{
  currentDisplayChannel = this;
  group = -1;
  withInfo = !Switched || Setup.ShowInfoOnChSwitch;
  displayChannel = Skins.Current()->DisplayChannel(withInfo);
  number = 0;
  timeout = Switched || Setup.TimeoutRequChInfo;
  channel = Channels.GetByNumber(Number);
  lastPresent = lastFollowing = NULL;
  if (channel) {
     DisplayChannel();
     DisplayInfo();
     displayChannel->Flush();
     }
  lastTime.Set();
}

cDisplayChannel::cDisplayChannel(eKeys FirstKey)
:cOsdObject(true)
{
  currentDisplayChannel = this;
  group = -1;
  number = 0;
  timeout = true;
  lastPresent = lastFollowing = NULL;
  lastTime.Set();
  withInfo = Setup.ShowInfoOnChSwitch;
  displayChannel = Skins.Current()->DisplayChannel(withInfo);
  channel = Channels.GetByNumber(cDevice::CurrentChannel());
  ProcessKey(FirstKey);
}

cDisplayChannel::~cDisplayChannel()
{
  delete displayChannel;
  cStatus::MsgOsdClear();
  currentDisplayChannel = NULL;
}

void cDisplayChannel::DisplayChannel(void)
{
  displayChannel->SetChannel(channel, number);
  cStatus::MsgOsdChannel(ChannelString(channel, number));
  lastPresent = lastFollowing = NULL;
}

void cDisplayChannel::DisplayInfo(void)
{
  if (withInfo && channel) {
     cSchedulesLock SchedulesLock;
     const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
     if (Schedules) {
        const cSchedule *Schedule = Schedules->GetSchedule(channel);
        if (Schedule) {
           const cEvent *Present = Schedule->GetPresentEvent();
           const cEvent *Following = Schedule->GetFollowingEvent();
           if (Present != lastPresent || Following != lastFollowing) {
              SetTrackDescriptions(channel->Number());
              displayChannel->SetEvents(Present, Following);
              cStatus::MsgOsdProgramme(Present ? Present->StartTime() : 0, Present ? Present->Title() : NULL, Present ? Present->ShortText() : NULL, Following ? Following->StartTime() : 0, Following ? Following->Title() : NULL, Following ? Following->ShortText() : NULL);
              lastPresent = Present;
              lastFollowing = Following;
              }
           }
        }
     }
}

void cDisplayChannel::Refresh(void)
{
  DisplayChannel();
  displayChannel->SetEvents(NULL, NULL);
}

cChannel *cDisplayChannel::NextAvailableChannel(cChannel *Channel, int Direction)
{
  if (Direction) {
     while (Channel) {
           Channel = Direction > 0 ? Channels.Next(Channel) : Channels.Prev(Channel);
    if (cStatus::MsgChannelProtected(0, Channel) == false)                     // PIN PATCH
           if (Channel && !Channel->GroupSep() && (cDevice::PrimaryDevice()->ProvidesChannel(Channel, Setup.PrimaryLimit) || cDevice::GetDevice(Channel, 0)))
              return Channel;
           }
     }
  return NULL;
}

eOSState cDisplayChannel::ProcessKey(eKeys Key)
{
  cChannel *NewChannel = NULL;
  if (Key != kNone)
     lastTime.Set();
  switch (Key) {
    case k0:
         if (number == 0) {
            // keep the "Toggle channels" function working
            cRemote::Put(Key);
            return osEnd;
            }
    case k1 ... k9:
         group = -1;
         if (number >= 0) {
            if (number > Channels.MaxNumber())
               number = Key - k0;
            else
               number = number * 10 + Key - k0;
            channel = Channels.GetByNumber(number);
            Refresh();
            withInfo = false;
            // Lets see if there can be any useful further input:
            int n = channel ? number * 10 : 0;
            int m = 10;
            cChannel *ch = channel;
            while (ch && (ch = Channels.Next(ch)) != NULL) {
                  if (!ch->GroupSep()) {
                     if (n <= ch->Number() && ch->Number() < n + m) {
                        n = 0;
                        break;
                        }
                     if (ch->Number() > n) {
                        n *= 10;
                        m *= 10;
                        }
                     }
                  }
            if (n > 0) {
               // This channel is the only one that fits the input, so let's take it right away:
               NewChannel = channel;
               withInfo = true;
               number = 0;
               Refresh();
               }
            }
         break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
    case kNext|k_Repeat:
    case kNext:
    case kPrev|k_Repeat:
    case kPrev:
         withInfo = false;
         number = 0;
         if (group < 0) {
            cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
            if (channel)
               group = channel->Index();
            }
         if (group >= 0) {
            int SaveGroup = group;
            if (NORMALKEY(Key) == kRight || NORMALKEY(Key) == kNext)
               group = Channels.GetNextGroup(group) ;
            else
               group = Channels.GetPrevGroup(group < 1 ? 1 : group);
            if (group < 0)
               group = SaveGroup;
            channel = Channels.Get(group);
            if (channel) {
               Refresh();
               if (!channel->GroupSep())
                  group = -1;
               }
            }
         break;
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kChanUp|k_Repeat:
    case kChanUp:
    case kChanDn|k_Repeat:
    case kChanDn: {
         eKeys k = NORMALKEY(Key);
         cChannel *ch = NextAvailableChannel(channel, (k == kUp || k == kChanUp) ? 1 : -1);
         if (ch)
            channel = ch;
         else if (channel && channel->Number() != cDevice::CurrentChannel())
            Key = k; // immediately switches channel when hitting the beginning/end of the channel list with k_Repeat
         }
         // no break here
    case kUp|k_Release:
    case kDown|k_Release:
    case kChanUp|k_Release:
    case kChanDn|k_Release:
    case kNext|k_Release:
    case kPrev|k_Release:
         if (!(Key & k_Repeat) && channel && channel->Number() != cDevice::CurrentChannel())
            NewChannel = channel;
         withInfo = true;
         group = -1;
         number = 0;
         Refresh();
         break;
    case kNone:
         if (number && lastTime.Elapsed() > DIRECTCHANNELTIMEOUT) {
            channel = Channels.GetByNumber(number);
            if (channel)
               NewChannel = channel;
            withInfo = true;
            number = 0;
            Refresh();
            lastTime.Set();
            }
         break;
    //TODO
    //XXX case kGreen:  return osEventNow;
    //XXX case kYellow: return osEventNext;
    case kOk:
         if (group >= 0) {
            channel = Channels.Get(Channels.GetNextNormal(group));
            if (channel)
               NewChannel = channel;
            withInfo = true;
            group = -1;
            Refresh();
            }
         else if (number > 0) {
            channel = Channels.GetByNumber(number);
            if (channel)
               NewChannel = channel;
            withInfo = true;
            number = 0;
            Refresh();
            }
         else
            return osEnd;
         break;
    default:
         if ((Key & (k_Repeat | k_Release)) == 0) {
            cRemote::Put(Key);
            return osEnd;
            }
    };
  if (!timeout || lastTime.Elapsed() < (uint64_t)(Setup.ChannelInfoTime * 1000)) {
     if (Key == kNone && !number && group < 0 && !NewChannel && channel && channel->Number() != cDevice::CurrentChannel()) {
        // makes sure a channel switch through the SVDRP CHAN command is displayed
        channel = Channels.GetByNumber(cDevice::CurrentChannel());
        Refresh();
        lastTime.Set();
        }
     DisplayInfo();
     displayChannel->Flush();
     if (NewChannel) {
        SetTrackDescriptions(NewChannel->Number()); // to make them immediately visible in the channel display
        Channels.SwitchTo(NewChannel->Number());
        SetTrackDescriptions(NewChannel->Number()); // switching the channel has cleared them
        channel = NewChannel;
        }
     return osContinue;
     }
  return osEnd;
}

// --- cDisplayVolume --------------------------------------------------------

#define VOLUMETIMEOUT 1000 //ms
#define MUTETIMEOUT   5000 //ms

cDisplayVolume *cDisplayVolume::currentDisplayVolume = NULL;

cDisplayVolume::cDisplayVolume(void)
:cOsdObject(true)
{
  currentDisplayVolume = this;
  timeout.Set(cDevice::PrimaryDevice()->IsMute() ? MUTETIMEOUT : VOLUMETIMEOUT);
  displayVolume = Skins.Current()->DisplayVolume();
  Show();
}

cDisplayVolume::~cDisplayVolume()
{
  delete displayVolume;
  currentDisplayVolume = NULL;
}

void cDisplayVolume::Show(void)
{
  displayVolume->SetVolume(cDevice::CurrentVolume(), MAXVOLUME, cDevice::PrimaryDevice()->IsMute());
}

cDisplayVolume *cDisplayVolume::Create(void)
{
  if (!currentDisplayVolume)
     new cDisplayVolume;
  return currentDisplayVolume;
}

void cDisplayVolume::Process(eKeys Key)
{
  if (currentDisplayVolume)
     currentDisplayVolume->ProcessKey(Key);
}

eOSState cDisplayVolume::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kVolUp|k_Repeat:
    case kVolUp:
    case kVolDn|k_Repeat:
    case kVolDn:
         Show();
         timeout.Set(VOLUMETIMEOUT);
         break;
    case kMute:
         if (cDevice::PrimaryDevice()->IsMute()) {
            Show();
            timeout.Set(MUTETIMEOUT);
            }
         else
            timeout.Set();
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0) {
                cRemote::Put(Key);
                return osEnd;
                }
    }
  return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cDisplayTracks --------------------------------------------------------

#define TRACKTIMEOUT 5000 //ms

cDisplayTracks *cDisplayTracks::currentDisplayTracks = NULL;

cDisplayTracks::cDisplayTracks(void)
:cOsdObject(true)
{
  cDevice::PrimaryDevice()->EnsureAudioTrack();
  SetTrackDescriptions(!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring() ? cDevice::CurrentChannel() : 0);
  currentDisplayTracks = this;
  numTracks = track = 0;
  audioChannel = cDevice::PrimaryDevice()->GetAudioChannel();
  eTrackType CurrentAudioTrack = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
      const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         types[numTracks] = eTrackType(i);
// Use better description for DD-Test
//         descriptions[numTracks] = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : IS_DOLBY_TRACK(types[numTracks]) ? "Dolby Digital" :*itoa(i));
         descriptions[numTracks] = strdup(cString::sprintf("%s%s%s%s%s%s%s%s",
                                          *TrackId->description ? TrackId->description : "",
                                          *TrackId->description ? " " : "",
                                          *TrackId->language ? "(" : "",
                                          *TrackId->language ? TrackId->language : "",
                                          *TrackId->language ? ") " : "",
                                          (*TrackId->description||*TrackId->language) ? "" : IS_DOLBY_TRACK(types[numTracks]) ? "Dolby Digital" : tr("Audio"),
                                          (*TrackId->description||*TrackId->language) ? "" : " ",
                                          (*TrackId->description||*TrackId->language||(ttDolby==i)) ? "" : *itoa((i>ttDolby) ? (i-ttDolby+1) : i)));
         if (i == CurrentAudioTrack)
            track = numTracks;
         numTracks++;
         }
      }
  timeout.Set(TRACKTIMEOUT);
  displayTracks = Skins.Current()->DisplayTracks(tr("Button$Audio"), numTracks, descriptions);
  Show();
}

cDisplayTracks::~cDisplayTracks()
{
  delete displayTracks;
  currentDisplayTracks = NULL;
  for (int i = 0; i < numTracks; i++)
      free(descriptions[i]);
  cStatus::MsgOsdClear();
}

void cDisplayTracks::Show(void)
{
  int ac = IS_AUDIO_TRACK(types[track]) ? audioChannel : -1;
  displayTracks->SetTrack(track, descriptions);
  displayTracks->SetAudioChannel(ac);
  displayTracks->Flush();
  cStatus::MsgSetAudioTrack(track, descriptions);
  cStatus::MsgSetAudioChannel(ac);
}

cDisplayTracks *cDisplayTracks::Create(void)
{
  if (cDevice::PrimaryDevice()->NumAudioTracks() > 0) {
     if (!currentDisplayTracks)
        new cDisplayTracks;
     return currentDisplayTracks;
     }
  Skins.Message(mtWarning, tr("No audio available!"));
  return NULL;
}

void cDisplayTracks::Process(eKeys Key)
{
  if (currentDisplayTracks)
     currentDisplayTracks->ProcessKey(Key);
}

eOSState cDisplayTracks::ProcessKey(eKeys Key)
{
  int oldTrack = track;
  int oldAudioChannel = audioChannel;
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
         if (NORMALKEY(Key) == kUp && track > 0)
            track--;
         else if (NORMALKEY(Key) == kDown && track < numTracks - 1)
            track++;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight: if (IS_AUDIO_TRACK(types[track])) {
                    static int ac[] = { 1, 0, 2 };
                    audioChannel = ac[cDevice::PrimaryDevice()->GetAudioChannel()];
                    if (NORMALKEY(Key) == kLeft && audioChannel > 0)
                       audioChannel--;
                    else if (NORMALKEY(Key) == kRight && audioChannel < 2)
                       audioChannel++;
                    audioChannel = ac[audioChannel];
                    timeout.Set(TRACKTIMEOUT);
                    }
         break;
    case kAudio|k_Repeat:
    case kAudio:
         if (++track >= numTracks)
            track = 0;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kOk:
         if (track != cDevice::PrimaryDevice()->GetCurrentAudioTrack())
            oldTrack = -1; // make sure we explicitly switch to that track
         timeout.Set();
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0)
                return osEnd;
    }
  if (track != oldTrack || audioChannel != oldAudioChannel)
     Show();
  if (track != oldTrack) {
     cDevice::PrimaryDevice()->SetCurrentAudioTrack(types[track]);
     Setup.CurrentDolby = IS_DOLBY_TRACK(types[track]);
     }
  if (audioChannel != oldAudioChannel)
     cDevice::PrimaryDevice()->SetAudioChannel(audioChannel);
  return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cDisplaySubtitleTracks ------------------------------------------------

cDisplaySubtitleTracks *cDisplaySubtitleTracks::currentDisplayTracks = NULL;

cDisplaySubtitleTracks::cDisplaySubtitleTracks(void) : cOsdObject(true)
{
    if ( !cDevice::PrimaryDevice()->PlayerCanHandleSubtitles() )
        SetTrackDescriptions( (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring()) ? cDevice::CurrentChannel() : 0); ////

    currentDisplayTracks = this;
    numTracks = track = 0;
    types[numTracks] = ttNone;
    descriptions[numTracks] = strdup(tr("No subtitles"));
    numTracks++;

    eTrackType CurrentSubtitleTrack;
    if ( !cDevice::PrimaryDevice()->PlayerCanHandleSubtitles() )
    {
        CurrentSubtitleTrack = cDevice::PrimaryDevice()->GetCurrentSubtitleTrack();

        for (int i = ttSubtitleFirst; i <= ttSubtitleLast; i++) {
            const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
            if (TrackId && TrackId->id) {
                types[numTracks] = eTrackType(i);
                descriptions[numTracks] = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i));
                if (i == CurrentSubtitleTrack)
                    track = numTracks;
                numTracks++;
            }
        }

    }
    else
    {
        // player can handle subtitles
        CurrentSubtitleTrack = eTrackType( ttSubtitleFirst +  cDevice::PrimaryDevice()->PlayerGetCurrentSubtitleTrack()) ;
        for (int i=0; i <  cDevice::PrimaryDevice()->PlayerNumSubtitleTracks(); ++i)
        {
            types[numTracks] =  eTrackType( ttSubtitleFirst + i);
            descriptions[numTracks] =  strdup(cDevice::PrimaryDevice()->PlayerSubtitleTrackName(i));

            if (strlen(descriptions[numTracks])==0) break;

            if (  eTrackType(  ttSubtitleFirst + i) == CurrentSubtitleTrack )
                track = numTracks;
            numTracks++;
        }

    }

    descriptions[numTracks] = NULL;
    timeout.Set(TRACKTIMEOUT);
    displayTracks = Skins.Current()->DisplayTracks(tr("Button$Subtitles"), numTracks, descriptions);
    Show();
}

cDisplaySubtitleTracks::~cDisplaySubtitleTracks()
{
    delete displayTracks;
    currentDisplayTracks = NULL;
    for (int i = 0; i < numTracks; i++)
        free(descriptions[i]);
    cStatus::MsgOsdClear();
}

void cDisplaySubtitleTracks::Show(void)
{
    displayTracks->SetTrack(track, descriptions);
    displayTracks->Flush();
    cStatus::MsgSetSubtitleTrack(track, descriptions);
}

cDisplaySubtitleTracks *cDisplaySubtitleTracks::Create(void)
{
    if (cDevice::PrimaryDevice()->PlayerNumSubtitleTracks() > 0 || cDevice::PrimaryDevice()->NumSubtitleTracks() > 0) { ////
        if (!currentDisplayTracks)
            new cDisplaySubtitleTracks;
        return currentDisplayTracks;
    }
    Skins.Message(mtWarning, tr("No subtitles available!"));
    return NULL;
}

void cDisplaySubtitleTracks::Process(eKeys Key)
{
    if (currentDisplayTracks)
        currentDisplayTracks->ProcessKey(Key);
}

eOSState cDisplaySubtitleTracks::ProcessKey(eKeys Key)
{
    int oldTrack = track;
    switch (Key) {
        case kUp|k_Repeat:
        case kUp:
        case kDown|k_Repeat:
        case kDown:
            if (NORMALKEY(Key) == kUp && track > 0)
                track--;
            else if (NORMALKEY(Key) == kDown && track < numTracks - 1)
                track++;
            timeout.Set(TRACKTIMEOUT);
            break;
        case /*kSubtitles*/ kGreater|k_Repeat:
        case /*kSubtitles*/ kGreater:
            if (++track >= numTracks)
                track = 0;
            timeout.Set(TRACKTIMEOUT);
            break;
        case kOk:
            if (types[track] != cDevice::PrimaryDevice()->GetCurrentSubtitleTrack() ||
                    types[track] != cDevice::PrimaryDevice()->PlayerGetCurrentSubtitleTrack()) ////
                oldTrack = -1; // make sure we explicitly switch to that track
            timeout.Set();
            break;
        case kNone: break;
        default: if ((Key & k_Release) == 0)
                     return osEnd;
    }
    if (track != oldTrack) {
        Show();

        if (  cDevice::PrimaryDevice()->PlayerCanHandleSubtitles() )
            cDevice::PrimaryDevice()->PlayerSetCurrentSubtitleTrack(track);  ////
        else
            cDevice::PrimaryDevice()->SetCurrentSubtitleTrack(types[track], true);
    }
    return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cRecordControl --------------------------------------------------------

cRecordControl *cRecordControls::RecordControls[MAXRECORDCONTROLS] = { NULL };
int cRecordControls::state = 0;

cRecordControl::cRecordControl(cDevice *Device, cTimer *Timer, bool Pause)
{
//  PRINTF("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  // We're going to manipulate an event here, so we need to prevent
  // others from modifying any EPG data:
  cSchedulesLock SchedulesLock;
  cSchedules::Schedules(SchedulesLock);

  event = NULL;
  instantId = NULL;
  fileName = NULL;
  recorder = NULL;
  device = Device;
  usrActive=false;
  if (!device) device = cDevice::PrimaryDevice();//XXX
  timer = Timer;
  if (!timer) {
     timer = new cTimer(true, Pause);
     if(!Timers.Add(timer))
        Skins.Message(mtError, tr("Could not add timer"));
     Timers.SetModified();
     asprintf(&instantId, cDevice::NumDevices() > 1 ? "%s - %d" : "%s", timer->Channel()->Name(), device->CardIndex() + 1);
     }
  else if (timer->HasFlags(tfInstant))
     asprintf(&instantId, cDevice::NumDevices() > 1 ? "%s - %d" : "%s", timer->Channel()->Name(), device->CardIndex() + 1);
  timer->SetPending(true);
  timer->SetRecording(true);
  event = timer->Event();

  if (event || GetEvent())
     dsyslog("Title: '%s' Subtitle: '%s'", event->Title(), event->ShortText());
  cRecording Recording(timer, event);
  fileName = strdup(Recording.FileName());

#if 0 //TB: we need a better idea for that: recording to a subdirectoy, which is on another filesystem with free space won't work if the HD is full
  int MBFree = FreeDiskSpaceMB("/media/reel/recordings", NULL);
  if (MBFree < 512) { // if not at least 512MB free disk space
      esyslog("not starting recordings because there are only %i free MBytes left", MBFree);
      PRINTF("not starting recordings because there are only %i free MBytes left\n", MBFree);
      Skins.Message(mtWarning, tr("Not enough disk space to start recording!"));
      if (Timer) {
        timer->SetPending(false);
        timer->SetRecording(false);
        timer->OnOff();
        if(!Timers.Del(timer))
            Skins.Message(mtError, tr("Could not delete timer"));
        Timers.SetModified();
      } else {
        if(!Timers.Del(timer))
          Skins.Message(mtError, tr("Could not delete timer"));
        Timers.SetModified();
      }
      timer = NULL;
      return;
  }
#endif

  // crude attempt to avoid duplicate recordings:
  if (cRecordControls::GetRecordControl(fileName)) {
     isyslog("already recording: '%s'", fileName);
     if (Timer) {
        timer->SetPending(false);
        timer->SetRecording(false);
        timer->OnOff();
        if(!Timers.Del(timer))
            Skins.Message(mtError, tr("Could not delete timer"));
        Timers.SetModified();
     } else {
        if(!Timers.Del(timer))
            Skins.Message(mtError, tr("Could not delete timer"));
        Timers.SetModified();
        if (!cReplayControl::LastReplayed()) // an instant recording, maybe from cRecordControls::PauseLiveVideo()
           cReplayControl::SetRecording(fileName, Recording.Name());
     }
     timer = NULL;
     return;
     }

  /* TB: only one instant recording per channel */
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
    if (cRecordControls::RecordControls[i] && cRecordControls::RecordControls[i] != this) {
       if(instantId && cRecordControls::RecordControls[i]->InstantId() && strcmp(instantId, cRecordControls::RecordControls[i]->InstantId()) == 0){
          isyslog("an instant recording is already running '%s'", fileName);
          if(!Interface->Confirm(tr("recording already running - Start another?"))){
            if (Timer) {
              timer->SetPending(false);
              timer->SetRecording(false);
              timer->OnOff();
              if(!Timers.Del(timer))
                Skins.Message(mtError, tr("Could not delete timer"));
              Timers.SetModified();
            } else {
              if(!Timers.Del(timer))
                Skins.Message(mtError, tr("Could not delete timer"));
              Timers.SetModified();
            }
           timer = NULL;
           return;
          }
       }
    }
  }

  cRecordingUserCommand::InvokeCommand(RUC_BEFORERECORDING, fileName);
  usrActive=true;
  isyslog("record %s", fileName);
  if (MakeDirs(fileName, true)) {
     const cChannel *ch = timer->Channel();
     int startFrame=-1;
     int endFrame=0;
     cLiveBuffer *liveBuffer = cLiveBufferManager::InLiveBuffer(timer, &startFrame, &endFrame);
     if (liveBuffer) {
        timer->SetFlags(tfhasLiveBuf);
        liveBuffer->SetStartFrame(startFrame);
        if (endFrame) {
           liveBuffer->CreateIndexFile(fileName, 0, endFrame);
           if(!Timers.Del(timer))
              Skins.Message(mtError, tr("Could not delete timer"));
           Timers.SetModified();
           timer = NULL;
           Recording.WriteInfo();
           Recordings.AddByName(fileName);
           return;
           }
     }
     recorder = new cRecorder(fileName, ch->Ca(), timer->Priority(), ch->Vpid(), ch->Ppid(), ch->Apids(), ch->Dpids(), ch->Spids(), liveBuffer);
     if (device->AttachReceiver(recorder, true)) {
        time_t start_t=time(0);
        while(recorder->GetRemux()->SFmode()==SF_UNKNOWN && (time(0)-start_t)<=2)
           usleep(50*1000); //TB: give recorder's remux a chance to detect mode
        if(recorder->GetRemux()->SFmode()==SF_H264){
          Recording.SetIsHD(true);
        }
        if(recorder->GetRemux()->TSmode()==rTS){
          Recording.SetIsTS(true);
        }
        Recording.WriteInfo();
        cStatus::MsgRecording(device, Recording.Name(), Recording.FileName(), true, ch->Number());
        if (!Timer || (Timer->HasFlags(tfInstant) && !cReplayControl::LastReplayed())) // an instant recording, maybe from cRecordControls::PauseLiveVideo()
           cReplayControl::SetRecording(fileName, Recording.Name());
        Recordings.AddByName(fileName);
        return;
        }
     else
        DELETENULL(recorder);
     }
  if (!Timer) {
     if(!Timers.Del(timer))
        Skins.Message(mtError, tr("Could not delete timer"));
     Timers.SetModified();
     timer = NULL;
     }
}

cRecordControl::~cRecordControl()
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  Stop();
  free(instantId);
  free(fileName);
}

#define INSTANT_REC_EPG_LOOKAHEAD 300 // seconds to look into the EPG data for an instant recording

bool cRecordControl::GetEvent(void)
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  const cChannel *channel = timer->Channel();
  time_t Time = timer->HasFlags(tfInstant) ? timer->StartTime() + INSTANT_REC_EPG_LOOKAHEAD : timer->StartTime() + (timer->StopTime() - timer->StartTime()) / 2;
  for (int seconds = 0; seconds <= MAXWAIT4EPGINFO; seconds++) {
      {
        cSchedulesLock SchedulesLock;
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
        if (Schedules) {
           const cSchedule *Schedule = Schedules->GetSchedule(channel);
           if (Schedule) {
              event = Schedule->GetEventAround(Time);
              if (event) {
                 if (seconds > 0)
                    dsyslog("got EPG info after %d seconds", seconds);
                 return true;
                 }
              }
           }
      }
      if (seconds == 0)
         dsyslog("waiting for EPG info...");
      sleep(1);
      }
  dsyslog("no EPG info available");
  return false;
}

void cRecordControl::Stop(void)
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  if (timer) {
     DELETENULL(recorder);
     timer->SetRecording(false);
     cStatus::MsgRecording(device, NULL, fileName, false, timer->Channel()->Number());
     timer = NULL;
     // Now handled by usrActive
     //cRecordingUserCommand::InvokeCommand(RUC_AFTERRECORDING, fileName);
     }

     /*
      * RUC_BEFORERECORDING command was invoked regardless of whether timer==NULL
      run RUN_AFTERRECORDING similiarly, also it keeps /tmp/vdr.records in
      the correct state ==> LED glows correctly
      */
  if(usrActive)
    cRecordingUserCommand::InvokeCommand(RUC_AFTERRECORDING, fileName);
  usrActive = false;
}

bool cRecordControl::Process(time_t t)
{
  if (!recorder || !timer || !timer->Matches(t))
     return false;
  AssertFreeDiskSpace(timer->Priority());
  return true;
}

// --- cRecordControls -------------------------------------------------------
bool cRecordControls::Start(cTimer *Timer, bool Pause)
{
#ifdef RBMINI
  if (Setup.ReelboxModeTemp == eModeClient && Active()) { // a limit of 1 recordings at a time on the rbmini in client-mode
    Skins.Message(mtError, tr("A recording is already active!"));
    return false;
  }
#endif
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  static time_t LastNoDiskSpaceMessage = 0;
  int FreeMB = 0;
  if (Timer) {
     AssertFreeDiskSpace(Timer->Priority(), !Timer->Pending());
     Timer->SetPending(true);
     }
  VideoDiskSpace(&FreeMB);
  if(Setup.ReelboxModeTemp != eModeClient) // Don't check disk usage on Clients
  {
  if (FreeMB < MINFREEDISK) {
     if (!Timer || time(NULL) - LastNoDiskSpaceMessage > NODISKSPACEDELTA) {
        isyslog("not enough disk space to start recording%s%s", Timer ? " timer " : "", Timer ? *Timer->ToDescr() : "");
        Skins.Message(mtWarning, tr("Not enough disk space to start recording!"));
        LastNoDiskSpaceMessage = time(NULL);
        }
     return false;
     }
  LastNoDiskSpaceMessage = 0;

  ChangeState();
  }
  int ch = Timer ? Timer->Channel()->Number() : cDevice::CurrentChannel();
  cChannel *channel = Channels.GetByNumber(ch);

  if (channel) {
     bool NeedsDetachReceivers = false;
     int Priority = Timer ? Timer->Priority() : Pause ? Setup.PausePriority : Setup.DefaultPriority;
     cDevice *device = cDevice::GetDevice(channel, Priority, &NeedsDetachReceivers);
     if (device) {
        if (NeedsDetachReceivers) {
           Stop(device);
           if (device == cTransferControl::ReceiverDevice())
              cControl::Shutdown(); // in case this device was used for Transfer Mode
           }
        dsyslog("switching device %d to channel %d", device->DeviceNumber() + 1, channel->Number());
        if (!device->SwitchChannel(channel, false)) {
           cThread::EmergencyExit(true);
           return false;
           }
        if (!Timer || Timer->Matches() || cLiveBufferManager::InLiveBuffer(Timer)) {
#ifdef USEMYSQL
            // On Clients, just create new timer since AVG will record
            if(Setup.ReelboxModeTemp == eModeClient)
            {
                if (!Timer)
                {
                    std::vector<cTimer*> InstantRecordings;
                    Timers.GetInstantRecordings(&InstantRecordings);
                    bool InstantRecordingExists = false;

                    if(InstantRecordings.size())
                    {
                        unsigned int i = 0;
                        while(!InstantRecordingExists && i < InstantRecordings.size())
                        {
                            if(InstantRecordings.at(i)->Channel() == channel)
                                InstantRecordingExists = true;
                            ++i;
                        }
                    }

                    // Don't record if there is already a running recording
                    if(InstantRecordingExists)
                    {
                        isyslog("an instant recording is already running '%s'", channel->Name());
                        Skins.Message(mtWarning, tr("An instant recording is already active!"));
                        Timer = NULL;
                    }
                    else
                        Timer = new cTimer(true, Pause);

                    if(Timer)
                    {
                        if(!Timers.Add(Timer))
                        {
                            Skins.Message(mtError, tr("Could not add timer"));
                            delete Timer;
                            return false;
                        }
                        Timers.SetModified();
                        return true;
                    }
                }
            }
            else
#endif
            {
                for(int i = 0; i < MAXRECORDCONTROLS; i++)
                {
                    if(!RecordControls[i])
                    {
                        RecordControls[i] = new cRecordControl(device, Timer, Pause);
                        cStatus::MsgRecordingFile(RecordControls[i]->FileName());  // PIN PATCH
                        return RecordControls[i]->Process(time(NULL));
                    }
                }
            }
        }
     }
     else if (!Timer || (Timer->Priority() >= Setup.PrimaryLimit && !Timer->Pending())) {
        isyslog("no free DVB device to record channel %d!", ch);
        Skins.Message(mtError, tr("No free DVB device to record!"));
        }
     }
  else
     esyslog("ERROR: channel %d not defined!", ch);
  return false;
}

void cRecordControls::Stop(const char *InstantId)
{
    //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  ChangeState();
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         const char *id = RecordControls[i]->InstantId();
         if (id && strcmp(id, InstantId) == 0) {
            cTimer *timer = RecordControls[i]->Timer();
            RecordControls[i]->Stop();
            if (timer) {
               isyslog("deleting timer %s", *timer->ToDescr());
               if(!Timers.Del(timer))
                  Skins.Message(mtError, tr("Could not delete timer"));
               Timers.SetModified();
               }
            break;
            }
         }
      }
}

void cRecordControls::Stop(cDevice *Device)
{
    //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  ChangeState();
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Device() == Device) {
            isyslog("stopping recording on DVB device %d due to higher priority", Device->CardIndex() + 1);
            RecordControls[i]->Stop();
            }
         }
      }
}

bool cRecordControls::PauseLiveVideo(bool fromLiveBuffer)
{
    //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  Skins.Message(mtStatus, tr("Pausing live video..."));
  cReplayControl::SetRecording(NULL, NULL); // make sure the new cRecordControl will set cReplayControl::LastReplayed()
  cTimer *timer = NULL;
  if (fromLiveBuffer)
     timer = cLiveBufferManager::Timer(-1);
  if (Start(timer, true)) {
     sleep(2); // allow recorded file to fill up enough to start replaying
     cReplayControl *rc = new cReplayControl;
     cControl::Launch(rc);
     cControl::Attach();
     sleep(1); // allow device to replay some frames, so we have a picture
     Skins.Message(mtStatus, NULL);
     rc->ProcessKey(kPause); // pause, allowing replay mode display
     return true;
     }
  Skins.Message(mtStatus, NULL);
  return false;
}

const char *cRecordControls::GetInstantId(const char *LastInstantId, bool LIFO)
{
    //printf("\033[1;91m%s\033[0m '%s'\n",__PRETTY_FUNCTION__, LastInstantId?LastInstantId:"");
    for (int i = LIFO?MAXRECORDCONTROLS-1:0; LIFO? i>=0:i < MAXRECORDCONTROLS; LIFO?--i:i++) {//printf(".");
      if (RecordControls[i]) {
         if (!LastInstantId && RecordControls[i]->InstantId()) { //printf("%i Y \n",i);
            return RecordControls[i]->InstantId();}
         if (LastInstantId && LastInstantId == RecordControls[i]->InstantId())
            LastInstantId = NULL;
            //printf("%i N '%s' ",i, RecordControls[i]->InstantId());
         }
      }
  return NULL;
}

cRecordControl *cRecordControls::GetRecordControl(const char *FileName)
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i] && strcmp(RecordControls[i]->FileName(), FileName) == 0)
         return RecordControls[i];
      }
  return NULL;
}

void cRecordControls::Process(time_t t)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (!RecordControls[i]->Process(t)) {
            DELETENULL(RecordControls[i]);
            ChangeState();
            }
         }
      }
}

void cRecordControls::ChannelDataModified(cChannel *Channel, int chmod)
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Timer() && RecordControls[i]->Timer()->Channel() == Channel) {
            if (RecordControls[i]->Device()->ProvidesTransponder(Channel) && // avoids retune on devices that don't really access the transponder
                ((CHANNELMOD_CA != chmod) || RecordControls[i]->Device()->ProvidesCa(Channel))) { // avoids retune if only ca has changed and the device doesn't provide it
               isyslog("stopping recording due to modification of channel %d", Channel->Number());
               RecordControls[i]->Stop();
               // This will restart the recording, maybe even from a different
               // device in case conditional access has changed.
               ChangeState();
               }
            }
         }
      }
}

bool cRecordControls::Active(void)
{
  //printf("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]){
          //printf("%d\n", i);
         return true;
      }
      }
  return false;
}

void cRecordControls::Shutdown(void)
{
  //printf("\033[1;91mCalled cRecordControls::Shutdown\033[0m\n");
  for (int i = 0; i < MAXRECORDCONTROLS; i++)
      DELETENULL(RecordControls[i]);
  ChangeState();
}

bool cRecordControls::StateChanged(int &State)
{
  //PRINTF("\033[1;91m%s\033[0m\n",__PRETTY_FUNCTION__);
  int NewState = state;
  bool Result = State != NewState;
  State = state;
  return Result;
}

// --- cReplayControl --------------------------------------------------------

cReplayControl *cReplayControl::currentReplayControl = NULL;
char *cReplayControl::fileName = NULL;
char *cReplayControl::title = NULL;

cReplayControl::cReplayControl(void)
:cDvbPlayerControl(fileName), marks(fileName)
{
  currentReplayControl = this;
  displayReplay = NULL;
  visible = modeOnly = shown = displayFrames = false;
  lastCurrent = lastTotal = -1;
  lastPlay = lastForward = false;
  lastSpeed = -2;
  lastJump = 0;
  jumpWidth = (Setup.JumpWidth * 60);
  timeoutShow = 0;
  timeSearchActive = false;
  cRecording Recording(fileName);

  cStatus::MsgReplaying(this, Recording.Name(), Recording.FileName(), true);
  SetTrackDescriptions(false);

  // Initializing variables for intelligent Positioning
  m_LastKeyPressed = 0;
  m_LastKey = kNone;
  m_ActualStep = m_FirstStep = (7*60);

}

cReplayControl::~cReplayControl()
{
  Hide();
  cStatus::MsgReplaying(this, NULL, fileName, false);
  Stop();
  if (currentReplayControl == this)
     currentReplayControl = NULL;
}

void cReplayControl::SetRecording(const char *FileName, const char *Title)
{
  free(fileName);
  free(title);
  fileName = FileName ? strdup(FileName) : NULL;
  title = Title ? strdup(Title) : NULL;
}

const char *cReplayControl::NowReplaying(void)
{
  return currentReplayControl ? fileName : NULL;
}

const char *cReplayControl::NowReplayingTitle(void)
{
  return currentReplayControl ? title : NULL;
}

const char *cReplayControl::LastReplayed(void)
{
  return fileName;
}

void cReplayControl::ClearLastReplayed(const char *FileName)
{
  if (fileName && FileName && strcmp(fileName, FileName) == 0) {
     free(fileName);
     fileName = NULL;
     }
}

void cReplayControl::ShowTimed(int Seconds)
{
  if (modeOnly)
     Hide();
  if (!visible) {
     shown = ShowProgress(true);
     timeoutShow = (shown && Seconds > 0) ? time(NULL) + Seconds : 0;
     }
}

void cReplayControl::Show(void)
{
  ShowTimed();
}

void cReplayControl::Hide(void)
{
  if (visible) {
     delete displayReplay;
     displayReplay = NULL;
     SetNeedsFastResponse(false);
     visible = false;
     modeOnly = false;
     lastPlay = lastForward = false;
     lastSpeed = -2; // an invalid value
     timeSearchActive = false;
     }
}

void cReplayControl::ShowMode(void)
{
  if (visible || (Setup.ShowReplayMode && !cOsd::IsOpen())) {
     bool Play, Forward;
     int Speed;
     if (GetReplayMode(Play, Forward, Speed) && (!visible || Play != lastPlay || Forward != lastForward || Speed != lastSpeed)) {
        bool NormalPlay = (Play && Speed == -1);

        if (!visible) {
           if (NormalPlay)
              return; // no need to do indicate ">" unless there was a different mode displayed before
           visible = modeOnly = true;
           displayReplay = Skins.Current()->DisplayReplay(modeOnly);
           }

        if (modeOnly && !timeoutShow && NormalPlay)
           timeoutShow = time(NULL) + MODETIMEOUT;
        displayReplay->SetMode(Play, Forward, Speed);
        lastPlay = Play;
        lastForward = Forward;
        lastSpeed = Speed;
        }
     }
}

void cReplayControl::Jump(eKeys xkey)
{
  // Reset step width after 30 seconds
  if (time(NULL) - m_LastKeyPressed > 30)
  {
      m_ActualStep = m_FirstStep;
  }
  else
  {
      // direction has changed -> reduce step width
      if (xkey != m_LastKey)
      {
          // min 4 seconds step remaining
          if (m_ActualStep > 8)
              m_ActualStep /= 2;
      }
  }

  // Now jump the computed step width.
  if (xkey==kYellow)
      SkipSeconds(m_ActualStep);
  else
      SkipSeconds(-1 * m_ActualStep);

  m_LastKeyPressed = time(NULL);
  m_LastKey = xkey;
}

static struct timeval lastUpdate = { 0 };

bool cReplayControl::ShowProgress(bool Initial)
{
  int Current, Total;
  struct timeval now = { 0 };
  gettimeofday(&now, NULL);

  /* TB: if "lastUpdate" is not set (first call), fill it */
  if(lastUpdate.tv_sec == 0 && lastUpdate.tv_usec == 0)
       lastUpdate = now;

  /* TB: 4 times/second is really enough */
  if(!Initial && (now.tv_sec*1000*1000 + now.tv_usec) - (lastUpdate.tv_sec*1000*1000 + lastUpdate.tv_usec) < 100*1000 ) {
       return true;
  }
  lastUpdate = now;

  if (GetIndex(Current, Total) && Total > 0) {
     if (!visible) {
        displayReplay = Skins.Current()->DisplayReplay(modeOnly);
        displayReplay->SetMarks(&marks);
        SetNeedsFastResponse(true);
        visible = true;
        }
     if (Initial) {
        if (title)
           displayReplay->SetTitle(title);
        lastCurrent = lastTotal = -1;
        }
     if (Total != lastTotal) {
        displayReplay->SetTotal(IndexToHMSF(Total));
        if (!Initial)
           displayReplay->Flush();
        }
     if (Current != lastCurrent || Total != lastTotal) {
        displayReplay->SetProgress(Current, Total);
        if (!Initial)
           displayReplay->Flush();
        displayReplay->SetCurrent(IndexToHMSF(Current, displayFrames));
        displayReplay->Flush();
        lastCurrent = Current;
        }
     lastTotal = Total;
     ShowMode();
     return true;
     }
  return false;
}

void cReplayControl::TimeSearchDisplay(void)
{
  char buf[64];
  strcpy(buf, tr("Jump: "));
  int len = strlen(buf);
  char h10 = '0' + (timeSearchTime >> 24);
  char h1  = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
  char m10 = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
  char m1  = '0' + (timeSearchTime & 0x000000FF);
  char ch10 = timeSearchPos > 3 ? h10 : '-';
  char ch1  = timeSearchPos > 2 ? h1  : '-';
  char cm10 = timeSearchPos > 1 ? m10 : '-';
  char cm1  = timeSearchPos > 0 ? m1  : '-';
  sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
  displayReplay->SetJump(buf);
}

void cReplayControl::TimeSearchProcess(eKeys Key)
{
#define STAY_SECONDS_OFF_END 10
  int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
  int Current = (lastCurrent / FRAMESPERSEC);
  int Total = (lastTotal / FRAMESPERSEC);
  switch (Key) {
    case k0 ... k9:
         if (timeSearchPos < 4) {
            timeSearchTime <<= 8;
            timeSearchTime |= Key - k0;
            timeSearchPos++;
            TimeSearchDisplay();
            }
         break;
    case kFastRew:
    case kLeft:
    case kFastFwd:
    case kRight: {
         int dir = ((Key == kRight || Key == kFastFwd) ? 1 : -1);
         if (dir > 0)
            Seconds = min(Total - Current - STAY_SECONDS_OFF_END, Seconds);
         SkipSeconds(Seconds * dir);
         timeSearchActive = false;
         }
         break;
    case kPlay:
    case kUp:
    case kPause:
    case kDown:
    case kOk:
         Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
         Goto(Seconds * FRAMESPERSEC, Key == kDown || Key == kPause || Key == kOk);
         timeSearchActive = false;
         break;
    default:
         timeSearchActive = false;
         break;
    }

  if (!timeSearchActive) {
     if (timeSearchHide)
        Hide();
     else
        displayReplay->SetJump(NULL);
     ShowMode();
     }
}

void cReplayControl::TimeSearch(void)
{
  timeSearchTime = timeSearchPos = 0;
  timeSearchHide = false;
  if (modeOnly)
     Hide();
  if (!visible) {
     Show();
     if (visible)
        timeSearchHide = true;
     else
        return;
     }
  timeoutShow = 0;
  TimeSearchDisplay();
  timeSearchActive = true;
}

void cReplayControl::MarkToggle(void)
{
  int Current, Total;
  if (GetIndex(Current, Total, true)) {
     cMark *m = marks.Get(Current);
     lastCurrent = -1; // triggers redisplay
     if (m)
        marks.Del(m);
     else {
        marks.Add(Current);
        ShowTimed(2);
        bool Play, Forward;
        int Speed;
        if (GetReplayMode(Play, Forward, Speed) && !Play) {
           Goto(Current, true);
           displayFrames = true;
           }
        }
     marks.Save();
     }
}

void cReplayControl::MarkJump(bool Forward)
{
  if (marks.Count()) {
     int Current, Total;
     if (GetIndex(Current, Total)) {
        cMark *m = Forward ? marks.GetNext(Current) : marks.GetPrev(Current);
        if (m) {
           bool Play2, Forward2;
           int Speed;
           if (Setup.JumpPlay && GetReplayMode(Play2, Forward2, Speed) &&
               Play2 && Forward && m->position < Total - SecondsToFrames(3)) {
              Goto(m->position);
              Play();
              }
           else {
              Goto(m->position, true);
              displayFrames = true;
              }
           }
        }
     }
}

void cReplayControl::MarkMove(bool Forward)
{
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (m) {
        displayFrames = true;
        int p = SkipFrames(Forward ? 1 : -1);
        cMark *m2;
        if (Forward) {
           if ((m2 = marks.Next(m)) != NULL && m2->position <= p)
              return;
           }
        else {
           if ((m2 = marks.Prev(m)) != NULL && m2->position >= p)
              return;
           }
        Goto(m->position = p, true);
        marks.Save();
        }
     }
}

void cReplayControl::EditCut(void)
{
  if (fileName) {
     Hide();
     if (!cCutter::Active()) {
        if (!marks.Count())
           Skins.Message(mtError, tr("No editing marks defined!"));
        else if (!cCutter::Start(fileName))
           Skins.Message(mtError, tr("Can't start editing process!"));
        else
           Skins.Message(mtInfo, tr("Editing process started"));
        }
     else
        Skins.Message(mtError, tr("Editing process already active!"));
     ShowMode();
     }
}

void cReplayControl::EditTest(void)
{
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (!m)
        m = marks.GetNext(Current);
     if (m) {
        if ((m->Index() & 0x01) != 0 && !Setup.PlayJump)
           m = marks.Next(m);
        if (m) {
           Goto(m->position - SecondsToFrames(3));
           Play();
           }
        }
     }
}

cOsdObject *cReplayControl::GetInfo(void)
{
  cRecording *Recording = Recordings.GetByName(cReplayControl::LastReplayed());
  if (Recording)
     return new cMenuRecording(Recording, false);
  return NULL;
}

eOSState cReplayControl::ProcessKey(eKeys Key)
{
  if (!Active())
     return osEnd;
  marks.Reload();
  if (visible) {
     if (timeoutShow && time(NULL) > timeoutShow) {
        Hide();
        ShowMode();
        timeoutShow = 0;
        }
     else if (modeOnly)
        ShowMode();
     else
        shown = ShowProgress(!shown) || shown;
     }
  bool DisplayedFrames = displayFrames;
  displayFrames = false;
  if (timeSearchActive && Key != kNone) {
     TimeSearchProcess(Key);
     return osContinue;
     }
  bool DoShowMode = true;
  switch (Key) {
    // Positioning:
    case kPlay:
                   Play(); break;
    case kPause:
                   Pause(); break;
    case kLeft:
    case kLeft|k_Repeat:
                   SkipSeconds( -5); break;
    case kFastRew|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kFastRew:
                   Backward(); break;
    case kRight:   bool playing, forwarding;
                   int speed;
                   if (GetReplayMode(playing, forwarding, speed) && !playing) {
                       Play();
                       break;
                   }
    case kRight|k_Repeat:
                   SkipSeconds( 5); break;

    case kFastFwd|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kFastFwd:
                   Forward(); break;
    case kRed:     TimeSearch(); break;
    case k1|k_Repeat:
    case k1:       SkipSeconds(-20); break;
    case k3|k_Repeat:
    case k3:       SkipSeconds( 20); break;

    case kGreen|k_Repeat:
    case kGreen:   Setup.JumpWidth ? SkipSeconds(-jumpWidth) : Jump(Key) ; break;
    case kYellow|k_Repeat:
    case kYellow:  Setup.JumpWidth ? SkipSeconds(jumpWidth) : Jump(Key) ; break;

    case kStop:
    case kBlue:    Hide();
                   Stop();
                   return osEnd;
    default: {
      DoShowMode = false;
      switch (Key) {
        // Editing:
        case kMarkToggle:      MarkToggle(); break;
        case kPrev|k_Repeat:
        case kPrev:
        case kDown:
        case kMarkJumpBack|k_Repeat:
        case kMarkJumpBack:    MarkJump(false); break;
        case kNext|k_Repeat:
        case kNext:
        case kUp:
        case kMarkJumpForward|k_Repeat:
        case kMarkJumpForward: MarkJump(true); break;
        case kMarkMoveBack|k_Repeat:
        case kMarkMoveBack:    MarkMove(false); break;
        case kMarkMoveForward|k_Repeat:
        case kMarkMoveForward: MarkMove(true); break;
        case kEditCut:         EditCut(); break;
        case kEditTest:        EditTest(); break;
        default: {
          displayFrames = DisplayedFrames;
          switch (Key) {
            // Menu control:
            case kOk:      if (visible && !modeOnly) {
                              Hide();
                              DoShowMode = true;
                              }
                           else
                              Show();
                           break;
            case kBack:    if (visible && !modeOnly) {
                              Hide();
                              DoShowMode = true;
                              }
                           else {
                              //return osRecordings;
                              cRemote::CallPlugin("extrecmenu");
                              return osEnd;
                           }
                           break;
            default:       return osUnknown;
            }
          }
        }
      }
    }
  if (DoShowMode)
     ShowMode();
  return osContinue;
}

// --- cMenuShutdown --------------------------------------------------------
#define SHUTDOWNMENU_TIMEOUT (10*1000) // 10s
cMenuShutdown::cMenuShutdown(int &interrupted, eShutdownMode &shutdownMode, bool &userShutdown,time_t &lastActivity)
:cOsdMenu(tr("Power Off ReelBox")), interrupted_(interrupted), shutdownMode_(shutdownMode), userShutdown_(userShutdown),lastActivity_(lastActivity), shutdown_(false)
{
    //printf("\033[1;92m-----------cMenuShutdown::cMenuShutdown--------------\033[0m\n");
    SetNeedsFastResponse(true);
    expert = false;
    quickShutdown_ = false;
    standbyAfterRecording_ = false;

    Set();
    timer_.Start(SHUTDOWNMENU_TIMEOUT);
}

cMenuShutdown::~cMenuShutdown()
{
    //printf("------------------cMenuShutdown::~cMenuShutdown()-----------------\n");
    if(!shutdown_)
    {
        CancelShutdown();
    }

#if 0
    bool ret = false;
    if(cRecordControls::Active())
        ret = Interface->Confirm(tr("Recording - Activating QuickStandby"), 2, true, true);
    else
        ret = Interface->Confirm(tr("Activating QuickStandby"), userShutdown_ ? 2 : 60, false, true);

#endif
    //printf("ret: %i qs: %i csa: %i\n", ret, quickShutdown_, cRecordControls::Active());

    if(quickShutdown_ )
    {
        if (!cQuickShutdownCtl::IsActive())
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
                Skins.Message(mtInfo, tr("Recording - Activating QuickStandby"));

            if(nProcess > 0 )
                Skins.Message(mtInfo, tr("Running Process - Activating QuickStandby"));


            cControl::Shutdown();
            cControl::Launch(new cQuickShutdownCtl);
            //printf("\033[1;92m Launching QuickShutdown\033[0m\n");

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

    if (Key != kNone)
        timer_.Start(SHUTDOWNMENU_TIMEOUT); // restart timeout Timer

    if(timer_.TimedOut()) // if recording then quick shutdown; else standby
    {
    /* if recording running, then go into quickshutdown and
     *  after the recording is over go into standby*/

        if (cRecordControls::Active() || Setup.StandbyOrQuickshutdown == 1) // StandbyOrQuickshutdown == 1 ==> no Standby
        {
            quickShutdown_ = true;
            standbyAfterRecording_ = true;
            return osEnd;
        }
        else
            return Shutdown(standby);
    }

    if(state == osUnknown)
    {
        switch (Key)
        {
            case kOk:
                if(std::string(Get(Current())->Text()).find(tr("Quick Standby")) == 2)
                {
                    quickShutdown_  = true;
                    shutdown_       = false; // No full shutdown

                    /* closing this OSD triggers 'quick shutdown' */
                    //return state      = osEnd;     // close Menu/OSD
                    return Shutdown(quickstandby);
                }
                else if(std::string(Get(Current())->Text()).find(tr("Standby")) == 2)
                {
                    int nProcess = 0;
                    BgProcessInfo info;
                    memset(&info, 0, sizeof(info));
                    if(cPluginManager::CallAllServices("get bgprocess info", &info))
                    {
                        nProcess = info.nProcess;
                    }
                    if (!cRecordControls::Active() && Setup.StandbyOrQuickshutdown == 0 && nProcess == 0)
                        return Shutdown(standby);

                    quickShutdown_ = true;
                    standbyAfterRecording_ = true;
                    cQuickShutdownCtl::afterQuickShutdown = 1; // standby
                    return osEnd;

                }
                else if(std::string(Get(Current())->Text()).find(tr("Power off")) == 2)
                {
                    if (!cRecordControls::Active())
                        return Shutdown(deepstandby);

                    quickShutdown_ = true;
                    standbyAfterRecording_ = true;
                    cQuickShutdownCtl::afterQuickShutdown = 2; // deepstandby
                    return osEnd;
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
                else if(std::string(Get(Current())->Text()).find(tr("Pause Live-TV")) == 2)
                {
                    userShutdown_ = false; //let k_Plugin work
                    cRemote::CallPlugin("streamdev-server");
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

eOSState cMenuShutdown::Shutdown(eShutdownMode mode)
{
    std::string msg;
    if(mode == standby)
    {
        shutdownMode_ = standby;
        msg = tr("Activating standby");
    }
    else if(mode == deepstandby)
    {
        shutdownMode_ = deepstandby;
        msg = tr("Activating deep standby");
    }
    else if(mode == quickstandby)
    {
        shutdownMode_ = quickstandby;
        msg = tr("Activating quick standby");
    }
    else if(mode == restart)
    {
        shutdownMode_ =  restart;
        msg = tr("Activating reboot");
    }
    if (cQuickShutdownCtl::IsActive())
        cQuickShutdownCtl::runScript = false;

    // if recording and user says NO then cancel shutdown
    // or if a Timer is near and user says NO then cancel shutdown
    // else perform shutdown

    if ( ( cRecordControls::Active()&&!Interface->Confirm(tr("Recording - shut down anyway?")) )
        || ( IsTimerNear() && !Interface->Confirm(tr("A recording starts soon, shut down anyway?")) ))

    {
        shutdown_ = false;
        cQuickShutdownCtl::runScript = true;
        // CancelShutdown is run in the destructor
    }
    else if (Interface->Confirm(msg.c_str(), userShutdown_ ? 2 : 180, true, true))
    {
        //printf("\033[1;92mSetting to SIGTERM\033[0m\n");
        interrupted_ = SIGTERM;
        shutdown_ = true;
        cControl::Shutdown(); //?? really neccessary?
    }
    return osEnd;
}

void cMenuShutdown::Set()
{
    Clear();
    char buf[256];
    sprintf(buf, "%d %s", 1, tr("Standby"));
    Add(new cOsdItem(buf, osUnknown, true));

    sprintf(buf, "%d %s", 2, tr("Power off"));
    Add(new cOsdItem(buf, osUnknown, true));

    sprintf(buf, "%d %s", 3, tr("Set Sleeptimer"));
    Add(new cOsdItem(buf, osUnknown, true));

    if (expert) {
        sprintf(buf, "%d %s", 4, tr("Reboot"));
        Add(new cOsdItem(buf, osUnknown, true));
    }

    // donot offer this option to users explicitly
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
    char *cmd;
    asprintf(&cmd, "shutdownwd.sh cancel");
    isyslog("executing '%s' (%s)", cmd, msg);
    SystemExec(cmd);
    free(cmd);
}

/*
class cMenuEditChannels : public cMenuSetupBase {
private:
public:
  cMenuEditChannels();
};

cMenuEditChannels::cMenuEditChannels() {
  cMenuChannels::cMenuChannels(mode_edit);
}
*/


bool IsTimerNear()
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

