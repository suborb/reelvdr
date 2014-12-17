/*
 * PremiereEpg plugin to VDR (C++)
 *
 * (C) 2005-2006 Stefan Huelswitt <s.huelswitt@gmx.de>
 *
 * This code is base on the commandline tool premiereepg2vdr
 * (C) 2004-2005 by Axel Katzur software@katzur.de
 * but has been rewritten from scratch
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <stdarg.h>
#include <vdr/plugin.h>
#include <vdr/filter.h>
#include <vdr/epg.h>
#include <vdr/channels.h>
#include <vdr/dvbdevice.h>
#include <vdr/i18n.h>
#include <vdr/config.h>
#include <libsi/section.h>
#include <libsi/descriptor.h>
#include "version.h"


//#define DEBUG
//#define DEBUG2

//no debug output
#define d(x) ; 
#define d2(x) ; 
/*
#ifdef DEBUG
#define d(x) { (x); }
#else
#define d(x) ; 
#endif
#ifdef DEBUG2
#define d2(x) { (x); }
#else
#define d2(x) ; 
#endif
*/

#define PMT_SCAN_TIMEOUT  10  // seconds
#define PMT_SCAN_IDLE     300 // seconds

static const char *VERSION        = "0.0.8-reel";
static const char *DESCRIPTION    = trNOOP("Parses extended Premiere EPG data");
static const char *MAINMENUENTRY  = trNOOP("Sky EPG");

#if APIVERSNUM < 10401
#error You need at least VDR API version 1.4.1 for this plugin
#endif

// --- cSetupPremiereEpg -------------------------------------------------------

const char *optPats[] = {
  "%s",
  "%s (Option %d)",
  "%s (O%d)",
  "#%2$d %1$s",
  "[%2$d] %1$s"
  };
#define NUM_PATS (sizeof(optPats)/sizeof(char *))

class cSetupPremiereEpg {
public:
  int OptPat;
  int OrderInfo;
  int RatingInfo;
  int FixEpg;
public:
  cSetupPremiereEpg(void);
  };

cSetupPremiereEpg SetupPE;

cSetupPremiereEpg::cSetupPremiereEpg(void)
{
  OptPat=1;
  OrderInfo=1;
  RatingInfo=1;
  FixEpg=1;
}

// --- cMenuSetupPremiereEpg ------------------------------------------------------------

class cMenuSetupPremiereEpg : public cMenuSetupPage {
private:
  cSetupPremiereEpg data;
  const char *optDisp[NUM_PATS];
  char buff[NUM_PATS][32];
protected:
  virtual void Store(void);
public:
  cMenuSetupPremiereEpg(void);
  };

cMenuSetupPremiereEpg::cMenuSetupPremiereEpg(void)
{
  data=SetupPE;
  SetSection(tr("PremiereEPG"));
  optDisp[0]=tr("off");
  for(unsigned int i=1; i<NUM_PATS; i++) {
    snprintf(buff[i],sizeof(buff[i]),optPats[i],"Event",1);
    optDisp[i]=buff[i];
    }
  Add(new cMenuEditStraItem(tr("Tag option events"),&data.OptPat,NUM_PATS,optDisp));
  Add(new cMenuEditBoolItem(tr("Show order information"),&data.OrderInfo));
  Add(new cMenuEditBoolItem(tr("Show rating information"),&data.RatingInfo));
  Add(new cMenuEditBoolItem(tr("Fix EPG data"),&data.FixEpg));
}

void cMenuSetupPremiereEpg::Store(void)
{
  SetupPE=data;
  SetupStore("OptionPattern",SetupPE.OptPat);
  SetupStore("OrderInfo",SetupPE.OrderInfo);
  SetupStore("RatingInfo",SetupPE.RatingInfo);
  SetupStore("FixEpg",SetupPE.FixEpg);
}

// --- CRC16 -------------------------------------------------------------------

#define POLY 0xA001 // CRC16

unsigned int crc16(unsigned int crc, unsigned char const *p, int len)
{
  while(len--) {
    crc^=*p++;
    for(int i=0; i<8; i++)
      crc=(crc&1) ? (crc>>1)^POLY : (crc>>1);
    }
  return crc&0xFFFF;
}

// --- cStrBuff ----------------------------------------------------------------

class cStrBuff {
private:
  char *buff;
  int size, pos;
public:
  cStrBuff(int Size);
  ~cStrBuff();
  void Printf(const char *fmt, ...) __attribute__ ((format (printf,2,3)));
  char *Buff(void) { return (buff && pos>0) ? strdup(buff):0; }
  };

cStrBuff::cStrBuff(int Size)
{
  size=Size; pos=0;
  buff=MALLOC(char,size);
}

cStrBuff::~cStrBuff()
{
  free(buff);
}

void cStrBuff::Printf(const char *fmt, ...)
{
  int s=size-pos;
  if(buff && s>0) {
    va_list ap;
    va_start(ap,fmt);
    int q=vsnprintf(buff+pos,s,fmt,ap);
    va_end(ap);
    if(q>0) pos+=q;
    }
}

// --- cFilterPremiereEpg ------------------------------------------------------

#define STARTTIME_BIAS (20*60)

class cFilterPremiereEpg : public cFilter {
private:
  int pmtpid, pmtsid, pmtidx, pmtnext;
  //
  void NextPmt(void);
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
public:
  cFilterPremiereEpg(void);
  virtual void SetStatus(bool On);
  void Trigger(void);
  };

cFilterPremiereEpg::cFilterPremiereEpg(void)
{
  Trigger();
  Set(0x00,0x00);
}

void cFilterPremiereEpg::Trigger(void)
{
  d(printf("trigger\n"))
  pmtpid=0; pmtidx=0; pmtnext=0;
}

void cFilterPremiereEpg::SetStatus(bool On)
{
  d(printf("setstatus %d\n",On))
  cFilter::SetStatus(On);
  Trigger();
}

void cFilterPremiereEpg::NextPmt(void)
{
  Del(pmtpid,0x02);
  pmtpid=0;
  pmtidx++;
  d(printf("PMT next\n"))
}

void cFilterPremiereEpg::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  int now=time(0);
  if(Pid==0 && Tid==SI::TableIdPAT) {
    if(!pmtnext || now>pmtnext) {
      if(pmtpid) NextPmt();
      if(!pmtpid) {
        SI::PAT pat(Data,false);
        if(pat.CheckCRCAndParse()) {
          SI::PAT::Association assoc;
          int idx=0;
          for(SI::Loop::Iterator it; pat.associationLoop.getNext(assoc,it);) {
            if(!assoc.isNITPid()) {
              if(idx++==pmtidx) {
                pmtpid=assoc.getPid();
                pmtsid=assoc.getServiceId();
                Add(pmtpid,0x02);
                pmtnext=now+PMT_SCAN_TIMEOUT;
                d(printf("PMT pid now 0x%04x (idx=%d)\n",pmtpid,pmtidx))
                break;
                }
              }
            }
          if(!pmtpid) {
            pmtidx=0;
            pmtnext=now+PMT_SCAN_IDLE;
            d(printf("PMT scan idle\n"))
            }
          }
        }
      }
    }
  else if(pmtpid>0 && Pid==pmtpid && Tid==SI::TableIdPMT && Source() && Transponder()) {
    SI::PMT pmt(Data,false);
    if(pmt.CheckCRCAndParse() && pmt.getServiceId()==pmtsid) {
      SI::PMT::Stream stream;
      for(SI::Loop::Iterator it; pmt.streamLoop.getNext(stream,it); ) {
        if(stream.getStreamType()==0x05) {
          SI::CharArray data=stream.getData();
          if((data[1]&0xE0)==0xE0 && (data[3]&0xF0)==0xF0) {
            bool prvData=false, usrData=false;
            SI::Descriptor *d;
            for(SI::Loop::Iterator it; (d=stream.streamDescriptors.getNext(it)); ) {
              switch(d->getDescriptorTag()) {
                case SI::PrivateDataSpecifierDescriptorTag:
                  d(printf("prv: %d %08x\n",d->getLength(),d->getData().FourBytes(2)))
                  if(d->getLength()==6 && d->getData().FourBytes(2)==0x000000be)
                    prvData=true;
                  break;
                case 0x90:
                  d(printf("usr: %d %08x\n",d->getLength(),d->getData().FourBytes(2)))
                  if(d->getLength()==6 && d->getData().FourBytes(2)==0x0000ffff)
                    usrData=true;
                  break;
                default:
                  break;
                }
              delete d;
              }
            if(prvData && usrData) {
              int pid=stream.getPid();
              d(printf("found citpid 0x%04x",pid))
              if(!Matches(pid,0xA0)) {
                Add(pid,0xA0);
                d(printf(" (added)"))
                }
              d(printf("\n"))
              }
            }
          }
        }
      NextPmt(); pmtnext=0;
      }
    }
  else if(Tid==0xA0 && Source()) {
    SI::PremiereCIT cit(Data,false);
    if(cit.CheckCRCAndParse()) {
      cSchedulesLock SchedulesLock(true,10);
      cSchedules *Schedules=(cSchedules *)cSchedules::Schedules(SchedulesLock);
      if(Schedules) {
        int nCount=0;
        SI::ExtendedEventDescriptors *ExtendedEventDescriptors=0;
        SI::ShortEventDescriptor *ShortEventDescriptor=0;
        char *order=0, *rating=0;
        {
        time_t firstTime=0;
        SI::Descriptor *d;
        bool UseExtendedEventDescriptor=false;
        int LanguagePreferenceShort=-1;
        int LanguagePreferenceExt=-1;
        for(SI::Loop::Iterator it; (d=cit.eventDescriptors.getNext(it)); ) {
          switch(d->getDescriptorTag()) {
            case 0xF0: // order information
              if(SetupPE.OrderInfo) {
		static const char *text[] = {
                  trNOOP("Ordernumber"),
                  trNOOP("Price"),
                  trNOOP("Ordering"),
                  trNOOP("SMS"),
                  trNOOP("WWW")
                  };
		cStrBuff str(1024);
                const unsigned char *data=d->getData().getData()+2;
                for(int i=0; i<5; i++) {
                  int l=data[0]; 
		  if(l>0) str.Printf("\n%s: %.*s",tr(text[i]),l,&data[1]);
                  data+=l+1;
                  }
		order=str.Buff();
                }
              break;
            case 0xF1: // parental rating
              if(SetupPE.RatingInfo) {
		cStrBuff str(1024);
                const unsigned char *data=d->getData().getData()+2;
		str.Printf("\n%s: %d %s",tr("Rating"),data[0]+3,tr("years"));
                data+=7;
                int l=data[0]; 
		if(l>0) str.Printf(" (%.*s)",l,&data[1]);
		rating=str.Buff();
                }
              break;
            case SI::PremiereContentTransmissionDescriptorTag:
              if(nCount>=0) {
                SI::PremiereContentTransmissionDescriptor *pct=(SI::PremiereContentTransmissionDescriptor *)d;
                nCount++;
                SI::PremiereContentTransmissionDescriptor::StartDayEntry sd;
                SI::Loop::Iterator it;
                if(pct->startDayLoop.getNext(sd,it)) {
                  SI::PremiereContentTransmissionDescriptor::StartDayEntry::StartTimeEntry st;
                  SI::Loop::Iterator it2;
                  if(sd.startTimeLoop.getNext(st,it2)) {
                    time_t StartTime=st.getStartTime(sd.getMJD());
                    if(nCount==1) firstTime=StartTime;
                    else if(firstTime<StartTime-5*50 || firstTime>StartTime+5*60)
                      nCount=-1;
                    }
                  }
                }
              break;
            case SI::ExtendedEventDescriptorTag:
              {
              SI::ExtendedEventDescriptor *eed=(SI::ExtendedEventDescriptor *)d;
              if(I18nIsPreferredLanguage(Setup.EPGLanguages,eed->languageCode, LanguagePreferenceExt) || !ExtendedEventDescriptors) {
                 delete ExtendedEventDescriptors;
                 ExtendedEventDescriptors=new SI::ExtendedEventDescriptors;
                 UseExtendedEventDescriptor=true;
                 }
              if(UseExtendedEventDescriptor) {
                 ExtendedEventDescriptors->Add(eed);
                 d=NULL; // so that it is not deleted
                 }
              if(eed->getDescriptorNumber()==eed->getLastDescriptorNumber())
                 UseExtendedEventDescriptor=false;
              }
              break;
            case SI::ShortEventDescriptorTag:
              {
              SI::ShortEventDescriptor *sed=(SI::ShortEventDescriptor *)d;
              if(I18nIsPreferredLanguage(Setup.EPGLanguages,sed->languageCode, LanguagePreferenceShort) || !ShortEventDescriptor) {
                 delete ShortEventDescriptor;
                 ShortEventDescriptor=sed;
                 d=NULL; // so that it is not deleted
                 }
              }
              break;
            default:
              break;
            }
          delete d;
          }
        }

        {
        bool Modified=false;
        int optCount=0;
        unsigned int crc[3];
        crc[0]=cit.getContentId();
        SI::PremiereContentTransmissionDescriptor *pct;
        for(SI::Loop::Iterator it; (pct=(SI::PremiereContentTransmissionDescriptor *)cit.eventDescriptors.getNext(it,SI::PremiereContentTransmissionDescriptorTag)); ) {
          int nid=pct->getOriginalNetworkId();
          int tid=pct->getTransportStreamId();
          int sid=pct->getServiceId();
          if(SetupPE.FixEpg) {
            if(nid==133) {
	      if     (tid==0x03 && sid==0xf0) { tid=0x02; sid=0xe0; }
	      else if(tid==0x03 && sid==0xf1) { tid=0x02; sid=0xe1; }
	      else if(tid==0x03 && sid==0xf5) { tid=0x03; sid=0xdc; }
	      else if(tid==0x04 && sid==0xd2) { tid=0x11; sid=0xe2; }
	      else if(tid==0x11 && sid==0xd3) { tid=0x11; sid=0xe3; }
	      else if(tid==0x01 && sid==0xd4) { tid=0x04; sid=0xe4; } // Service 212/228
              }
            }
          tChannelID channelID(Source(),nid,tid,sid);
          cChannel *channel=Channels.GetByChannelID(channelID,true);
          if(!channel) continue;

          cSchedule *pSchedule=(cSchedule *)Schedules->GetSchedule(channelID);
          if(!pSchedule) {
             pSchedule=new cSchedule(channelID);
             Schedules->Add(pSchedule);
             }

          optCount++;
          SI::PremiereContentTransmissionDescriptor::StartDayEntry sd;
          int index=0;
          for(SI::Loop::Iterator it; pct->startDayLoop.getNext(sd,it); ) {
            int mjd=sd.getMJD();
            SI::PremiereContentTransmissionDescriptor::StartDayEntry::StartTimeEntry st;
            for(SI::Loop::Iterator it2; sd.startTimeLoop.getNext(st,it2); ) {
              time_t StartTime=st.getStartTime(mjd);
              time_t EndTime=StartTime+cit.getDuration();
              int runningStatus=(StartTime<now && now<EndTime) ? SI::RunningStatusRunning : ((StartTime-30<now && now<StartTime) ? SI::RunningStatusStartsInAFewSeconds : SI::RunningStatusNotRunning);
              bool isOpt=false;
              if(index++==0 && nCount>1) isOpt=true;
              crc[1]=isOpt ? optCount : 0;
              crc[2]=StartTime / STARTTIME_BIAS;
              tEventID EventId=((('P'<<8)|'W')<<16) | crc16(0,(unsigned char *)crc,sizeof(crc));

              d2(printf("%s R%d %04x/%.4x %d %d/%d %s +%d ",*channelID.ToString(),runningStatus,EventId&0xFFFF,cit.getContentId(),index,isOpt,optCount,stripspace(ctime(&StartTime)),(int)cit.getDuration()/60))
              if(EndTime+Setup.EPGLinger*60<now) {
                d2(printf("(old)\n"))
                continue;
                }

              bool newEvent=false;
              cEvent *pEvent=(cEvent *)pSchedule->GetEvent(EventId,-1);
              if(!pEvent) {
                d2(printf("(new)\n"))
                pEvent=new cEvent(EventId);
                if(!pEvent) continue;
                newEvent=true;
                }
              else {
                d2(printf("(upd)\n"))
                pEvent->SetSeen();
                if(pEvent->TableID()==0x00 || pEvent->Version()==cit.getVersionNumber()) {
                  if(pEvent->RunningStatus()!=runningStatus)
                    pSchedule->SetRunningStatus(pEvent,runningStatus,channel);
                  continue;
                  }
                }
              pEvent->SetEventID(EventId);
              pEvent->SetTableID(Tid);
              pEvent->SetVersion(cit.getVersionNumber());
              pEvent->SetStartTime(StartTime);
              pEvent->SetDuration(cit.getDuration());

              if(ShortEventDescriptor) {
                char buffer[256];
#if VDRVERSNUM < 10716
                ShortEventDescriptor->name.getText(buffer,sizeof(buffer), "ISO-8859-9");
#else
//TODO: Check charset
                ShortEventDescriptor->name.getText(buffer,sizeof(buffer));
#endif
                if(isOpt) {
                  char buffer2[sizeof(buffer)+32];
                  snprintf(buffer2,sizeof(buffer2),optPats[SetupPE.OptPat],buffer,optCount);
                  pEvent->SetTitle(buffer2);
                  }
                else
                  pEvent->SetTitle(buffer);
                d2(printf("title: %s\n",pEvent->Title()))
#if VDRVERSNUM < 10716
                pEvent->SetShortText(ShortEventDescriptor->text.getText(buffer,sizeof(buffer), "ISO-8859-9"));
#else
//TODO: Check charset
                pEvent->SetShortText(ShortEventDescriptor->text.getText(buffer,sizeof(buffer)));
#endif
                }
              if(ExtendedEventDescriptors) {
                char buffer[ExtendedEventDescriptors->getMaximumTextLength(": ")+1];
#if VDRVERSNUM < 10716
                pEvent->SetDescription(ExtendedEventDescriptors->getText(buffer,sizeof(buffer), "ISO-8859-9", ": "));
#else
//TODO: Check charset
                pEvent->SetDescription(ExtendedEventDescriptors->getText(buffer,sizeof(buffer), ": "));
#endif
                }
              if(order || rating) {
                int len=(pEvent->Description() ? strlen(pEvent->Description()) : 0) +
                        (order                 ? strlen(order) : 0) +
                        (rating                ? strlen(rating) : 0);
                char buffer[len+32];
                buffer[0]=0;
                if(pEvent->Description()) strcat(buffer,pEvent->Description());
                if(rating)                strcat(buffer,rating);
                if(order)                 strcat(buffer,order);
                pEvent->SetDescription(buffer);
                }

              if(newEvent) pSchedule->AddEvent(pEvent);
              pEvent->SetComponents(NULL);
              pEvent->FixEpgBugs();
              if(pEvent->RunningStatus()!=runningStatus)
                pSchedule->SetRunningStatus(pEvent,runningStatus,channel);
              pSchedule->DropOutdated(StartTime,EndTime,Tid,cit.getVersionNumber());
              Modified=true;
              }
            }
          if(Modified) {
            pSchedule->Sort();
            Schedules->SetModified(pSchedule);
            }
          delete pct;
          }
        }
        delete ExtendedEventDescriptors;
        delete ShortEventDescriptor;
        free(order);
        free(rating);
        }
      }
    }
}

// --- cPluginPremiereEpg ------------------------------------------------------

class cPluginPremiereEpg : public cPlugin {
private:
  struct {
    cFilterPremiereEpg *filter;
    cDevice *device;
    } epg[MAXDVBDEVICES];
public:
  cPluginPremiereEpg(void);
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual bool Start(void);
  virtual void Stop(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  };

cPluginPremiereEpg::cPluginPremiereEpg(void)
{
  memset(epg,0,sizeof(epg));
}

bool cPluginPremiereEpg::Start(void)
{
  for(int i=0; i<MAXDVBDEVICES; i++) {
    cDevice *dev=cDevice::GetDevice(i);
    if(dev) {
      epg[i].device=dev;
      dev->AttachFilter(epg[i].filter=new cFilterPremiereEpg);
      isyslog("Attached premiere EPG filter to device %d",i);
      }
    }
  return true;
}

void cPluginPremiereEpg::Stop(void)
{
  for(int i=0; i<MAXDVBDEVICES; i++) {
    cDevice *dev=epg[i].device;
    if(dev) dev->Detach(epg[i].filter);
    delete epg[i].filter;
    epg[i].device=0;
    epg[i].filter=0;
    }
}

cMenuSetupPage *cPluginPremiereEpg::SetupMenu(void)
{
  return new cMenuSetupPremiereEpg;
}

bool cPluginPremiereEpg::SetupParse(const char *Name, const char *Value)
{
  if      (!strcasecmp(Name, "OptionPattern")) SetupPE.OptPat     = atoi(Value);
  else if (!strcasecmp(Name, "OrderInfo"))     SetupPE.OrderInfo  = atoi(Value);
  else if (!strcasecmp(Name, "RatingInfo"))    SetupPE.RatingInfo = atoi(Value);
  else if (!strcasecmp(Name, "FixEpg"))        SetupPE.FixEpg     = atoi(Value);
  else return false;
  return true;
}

VDRPLUGINCREATOR(cPluginPremiereEpg); // Don't touch this!
