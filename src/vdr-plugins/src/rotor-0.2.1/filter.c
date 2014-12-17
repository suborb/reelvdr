#include "filter.h"
#include <malloc.h>
#include <libsi/section.h>
#include <libsi/descriptor.h>
#include <vdr/channels.h>

// --- cCaDescriptor ---------------------------------------------------------

class cCaDescriptor : public cListObject {
private:
  int caSystem;
  bool stream;
  int length;
  uchar *data;
public:
  cCaDescriptor(int CaSystem, int CaPid, bool Stream, int Length, const uchar *Data);
  virtual ~cCaDescriptor();
  bool operator== (const cCaDescriptor &arg) const;
  int CaSystem(void) { return caSystem; }
  int Stream(void) { return stream; }
  int Length(void) const { return length; }
  const uchar *Data(void) const { return data; }
  };

cCaDescriptor::cCaDescriptor(int CaSystem, int CaPid, bool Stream, int Length, const uchar *Data)
{
  caSystem = CaSystem;
  stream = Stream;
  length = Length + 6;
  data = MALLOC(uchar, length);
  data[0] = SI::CaDescriptorTag;
  data[1] = length - 2;
  data[2] = (caSystem >> 8) & 0xFF;
  data[3] =  caSystem       & 0xFF;
  data[4] = ((CaPid   >> 8) & 0x1F) | 0xE0;
  data[5] =   CaPid         & 0xFF;
  if (Length)
     memcpy(&data[6], Data, Length);
}

cCaDescriptor::~cCaDescriptor()
{
  free(data);
}

bool cCaDescriptor::operator== (const cCaDescriptor &arg) const
{
  return length == arg.length && memcmp(data, arg.data, length) == 0;
}

// --- cCaDescriptors --------------------------------------------------------

class cCaDescriptors : public cListObject {
private:
  int source;
  int transponder;
  int serviceId;
  int numCaIds;
  int caIds[MAXCAIDS + 1];
  cList<cCaDescriptor> caDescriptors;
  void AddCaId(int CaId);
public:
  cCaDescriptors(int Source, int Transponder, int ServiceId);
  bool operator== (const cCaDescriptors &arg) const;
  bool Is(int Source, int Transponder, int ServiceId);
  bool Is(cCaDescriptors * CaDescriptors);
  bool Empty(void) { return caDescriptors.Count() == 0; }
  void AddCaDescriptor(SI::CaDescriptor *d, bool Stream);
  int GetCaDescriptors(const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag);
  const int *CaIds(void) { return caIds; }
  };

cCaDescriptors::cCaDescriptors(int Source, int Transponder, int ServiceId)
{
  source = Source;
  transponder = Transponder;
  serviceId = ServiceId;
  numCaIds = 0;
  caIds[0] = 0;
}

bool cCaDescriptors::operator== (const cCaDescriptors &arg) const
{
  cCaDescriptor *ca1 = caDescriptors.First();
  cCaDescriptor *ca2 = arg.caDescriptors.First();
  while (ca1 && ca2) {
        if (!(*ca1 == *ca2))
           return false;
        ca1 = caDescriptors.Next(ca1);
        ca2 = arg.caDescriptors.Next(ca2);
        }
  return !ca1 && !ca2;
}

bool cCaDescriptors::Is(int Source, int Transponder, int ServiceId)
{
  return source == Source && transponder == Transponder && serviceId == ServiceId;
}

bool cCaDescriptors::Is(cCaDescriptors *CaDescriptors)
{
  return Is(CaDescriptors->source, CaDescriptors->transponder, CaDescriptors->serviceId);
}

void cCaDescriptors::AddCaId(int CaId)
{
  if (numCaIds < MAXCAIDS) {
     for (int i = 0; i < numCaIds; i++) {
         if (caIds[i] == CaId)
            return;
         }
     caIds[numCaIds++] = CaId;
     caIds[numCaIds] = 0;
     }
}

void cCaDescriptors::AddCaDescriptor(SI::CaDescriptor *d, bool Stream)
{
  cCaDescriptor *nca = new cCaDescriptor(d->getCaType(), d->getCaPid(), Stream, d->privateData.getLength(), d->privateData.getData());
  for (cCaDescriptor *ca = caDescriptors.First(); ca; ca = caDescriptors.Next(ca)) {
      if (*ca == *nca) {
         delete nca;
         return;
         }
      }
  AddCaId(nca->CaSystem());
  caDescriptors.Add(nca);
}

int cCaDescriptors::GetCaDescriptors(const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag)
{
  if (!CaSystemIds || !*CaSystemIds)
     return 0;
  if (BufSize > 0 && Data) {
     int length = 0;
     int IsStream = -1;
     for (cCaDescriptor *d = caDescriptors.First(); d; d = caDescriptors.Next(d)) {
         const unsigned short *caids = CaSystemIds;
         do {
            if (*CaSystemIds == 0xFFFF || d->CaSystem() == *caids) {
               if (length + d->Length() <= BufSize) {
                  if (IsStream >= 0 && IsStream != d->Stream())
                     dsyslog("CAM: different stream flag in CA descriptors");
                  IsStream = d->Stream();
                  memcpy(Data + length, d->Data(), d->Length());
                  length += d->Length();
                  }
               else
                  return -1;
               }
            } while (*++caids);
         }
     StreamFlag = IsStream == 1;
     return length;
     }
  return -1;
}

// --- cCaDescriptorHandler --------------------------------------------------

class cCaDescriptorHandler : public cList<cCaDescriptors> {
private:
  cMutex mutex;
public:
  int AddCaDescriptors(cCaDescriptors *CaDescriptors);
      // Returns 0 if this is an already known descriptor,
      // 1 if it is an all new descriptor with actual contents,
      // and 2 if an existing descriptor was changed.
  int GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag);
  };

int cCaDescriptorHandler::AddCaDescriptors(cCaDescriptors *CaDescriptors)
{
  cMutexLock MutexLock(&mutex);
  for (cCaDescriptors *ca = First(); ca; ca = Next(ca)) {
      if (ca->Is(CaDescriptors)) {
         if (*ca == *CaDescriptors) {
            delete CaDescriptors;
            return 0;
            }
         Del(ca);
         Add(CaDescriptors);
         return 2;
         }
      }
  Add(CaDescriptors);
  return CaDescriptors->Empty() ? 0 : 1;
}

int cCaDescriptorHandler::GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag)
{
  cMutexLock MutexLock(&mutex);
  StreamFlag = false;
  for (cCaDescriptors *ca = First(); ca; ca = Next(ca)) {
      if (ca->Is(Source, Transponder, ServiceId))
         return ca->GetCaDescriptors(CaSystemIds, BufSize, Data, StreamFlag);
      }
  return 0;
}

cCaDescriptorHandler CaDescriptorHandler;

int GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag)
{
  return CaDescriptorHandler.GetCaDescriptors(Source, Transponder, ServiceId, CaSystemIds, BufSize, Data, StreamFlag);
}

// --- PatFilter ------------------------------------------------------------

PatFilter::PatFilter(cMenuScan* MENU)
{
  Menu=MENU;
  sdtFilter=NULL;
  pmtIndex = 0;
  for (int i=0; i<MAXFILTERS; i++)
    pmtPid[i] = 0;
  numPmtEntries = 0;
  Set(0x00, 0x00);  // PAT
  endofScan=false;
  SetStatus(false);
  pit=pnum=0;
}

void PatFilter::SetSdtFilter(SdtFilter *SdtFilter)
{
  sdtFilter=SdtFilter;
  SetStatus(true);
}

void PatFilter::SetStatus(bool On)
{
  cFilter::SetStatus(On);
  pmtIndex = 0;
  for (int i=0; i<MAXFILTERS; i++)
    pmtPid[i] = 0;
  numPmtEntries = 0;
  num=0;
  pit=pnum=0;
}

void PatFilter::Trigger(void)
{
  numPmtEntries = 0;
}

bool PatFilter::PmtVersionChanged(int PmtPid, int Sid, int Version)
{
  uint64_t v = Version;
  v <<= 32;
  uint64_t id = (PmtPid | (Sid << 16)) & 0x00000000FFFFFFFFLL;
  for (int i = 0; i < numPmtEntries; i++) {
      if ((pmtVersion[i] & 0x00000000FFFFFFFFLL) == id) {
         bool Changed = (pmtVersion[i] & 0x000000FF00000000LL) != v;
         if (Changed)
            pmtVersion[i] = id | v;
         return Changed;
         }
      }
  if (numPmtEntries < MAXPMTENTRIES)
     pmtVersion[numPmtEntries++] = id | v;
  return true;
}

bool PatFilter::SidinSdt(int Sid)
{
  for (int i=0; i<sdtFilter->numSid; i++)
    if (sdtFilter->sid[i]==Sid)
    {
      for (int j=0; j<MAXFILTERS; j++)
        if (Sids[j]==Sid)
          return false;
      return true;
    }
  return false;
}

void PatFilter::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (Pid == 0x00) {
     if (Tid == 0x00) {
        for (int i=0; i<MAXFILTERS; i++)
          if (pmtPid[i] && time(NULL) - lastPmtScan[i] > FILTERTIMEOUT) {
             Del(pmtPid[i], 0x02);
             pmtPid[i] = 0;
             for (int k=0; i<sdtFilter->numSid; k++)
               if (sdtFilter->sid[k]==Sids[i])
               {
                 num++;
                 sdtFilter->sid[k]=0;
                 break;
               }
             Sids[i]=0;
             pmtIndex++;
            // lastPmtScan = time(NULL);
             }
           SI::PAT pat(Data, false);
           if (!pat.CheckCRCAndParse())
              return;
           SI::PAT::Association assoc;
           int Index = 0;
           while (pmtPid[Index] && Index<MAXFILTERS) Index++;
           int tnum=0,tSid[100];
           for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
               tSid[tnum++]=assoc.getServiceId();
               if (!assoc.isNITPid() &&  SidinSdt(assoc.getServiceId())) {
                  if (Index<MAXFILTERS) 
                  {
                    pmtPid[Index] = assoc.getPid();
                    Sids[Index] = assoc.getServiceId();
                    lastPmtScan[Index] = time(NULL);
                    Add(pmtPid[Index], 0x02);
                    while (pmtPid[Index] && Index<MAXFILTERS) Index++;
                  }
                  if (Index==MAXFILTERS)
                    break;
                  }
               }
             for (int i=0; i<tnum; i++)
             {
               bool found=false;
               for (int k=0; k<pnum; k++)
               {
                 if (tSid[i]==pSid[k])
                   found=true;
               }
               if (!found)
                  pSid[pnum++]=tSid[i];
             }
             pit++;              
             for (int i=0; pit>4 && i<sdtFilter->numSid; i++)
             {
               bool found=false;
               for (int j=0; j<pnum; j++)
                 if (!sdtFilter->sid[i] || sdtFilter->sid[i]==pSid[j])
                   found=true;
               if (!found)
               {
                 sdtFilter->sid[i]=0;
                 num++;
               }
             }
        }
     }
  else if (Tid == SI::TableIdPMT && Source() && Transponder()) {
     int Index=0;
     for (int i=0; i<MAXFILTERS; i++)
       if (Pid==pmtPid[i])
          Index=i;
     SI::PMT pmt(Data, false);
     if (!pmt.CheckCRCAndParse())
        return;
//     if (!Channels.Lock(true, 10)) {
//        numPmtEntries = 0; // to make sure we try again
//        return;
//        }
     cChannel *Channel = Menu->GetChannel(pmt.getServiceId());
     if (Channel) {
        SI::CaDescriptor *d;
        cCaDescriptors *CaDescriptors = new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Sid());
        // Scan the common loop:
        for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag)); ) {
            CaDescriptors->AddCaDescriptor(d, false);
            delete d;
            }
        // Scan the stream-specific loop:
        SI::PMT::Stream stream;
        int Vpid = 0;
        int Ppid = pmt.getPCRPid();
        int Apids[MAXAPIDS + 1] = { 0 };
        int Dpids[MAXDPIDS + 1] = { 0 };
#if VDRVERSNUM >= 10332
        char ALangs[MAXAPIDS + 1][MAXLANGCODE2] = { "" };
        char DLangs[MAXDPIDS + 1][MAXLANGCODE2] = { "" };
#else
        char ALangs[MAXAPIDS + 1][4] = { "" };
        char DLangs[MAXDPIDS + 1][4] = { "" };
#endif
#if VDRVERSNUM>=10509 || defined(REELVDR)
        int Spids[MAXSPIDS + 1] = { 0 };
        char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
#endif
        int Tpid = 0;
        int NumApids = 0;
        int NumDpids = 0;
        for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); ) {
            switch (stream.getStreamType()) {
              case 1: // STREAMTYPE_11172_VIDEO
              case 2: // STREAMTYPE_13818_VIDEO
                      Vpid = stream.getPid();
                      break;
              case 3: // STREAMTYPE_11172_AUDIO
              case 4: // STREAMTYPE_13818_AUDIO
                      {
                      if (NumApids < MAXAPIDS) {
                         Apids[NumApids] = stream.getPid();
                         SI::Descriptor *d;
                         for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                             switch (d->getDescriptorTag()) {
                               case SI::ISO639LanguageDescriptorTag: {
                                    SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                    if (*ld->languageCode != '-') { // some use "---" to indicate "none"
                                       strn0cpy(ALangs[NumApids], I18nNormalizeLanguageCode(ld->languageCode), 4);
                                       ALangs[NumApids][4] = 0;
                                       }
                                    }
                                    break;
                               default: ;
                               }
                             delete d;
                             }
                         NumApids++;
                         }
                      }
                      break;
              case 5: // STREAMTYPE_13818_PRIVATE
              case 6: // STREAMTYPE_13818_PES_PRIVATE
              //XXX case 8: // STREAMTYPE_13818_DSMCC
                      {
                      int dpid = 0;
                      char lang[4] = { 0 };
                      SI::Descriptor *d;
                      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                          switch (d->getDescriptorTag()) {
                            case SI::AC3DescriptorTag:
                                 dpid = stream.getPid();
                                 break;
                            case SI::TeletextDescriptorTag:
                                 Tpid = stream.getPid();
                                 break;
                            case SI::ISO639LanguageDescriptorTag: {
                                 SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                 strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), 4);
                                 }
                                 break;
                            default: ;
                            }
                          delete d;
                          }
                      if (dpid) {
                         if (NumDpids < MAXDPIDS) {
                            Dpids[NumDpids] = dpid;
                            strn0cpy(DLangs[NumDpids], lang, 4);
                            NumDpids++;
                            }
                         }
                      }
                      break;
              //default: printf("PID: %5d %5d %2d %3d %3d\n", pmt.getServiceId(), stream.getPid(), stream.getStreamType(), pmt.getVersionNumber(), Channel->Number());//XXX
              }
            for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)stream.streamDescriptors.getNext(it, SI::CaDescriptorTag)); ) {
                CaDescriptors->AddCaDescriptor(d, true);
                delete d;
                }
            }
#if VDRVERSNUM>=10509 || defined(REELVDR)
        Menu->SetPids(pmt.getServiceId(),Vpid, Vpid ? Ppid : 0, Apids, ALangs, Dpids, DLangs, Spids, SLangs, Tpid);
#else
        Menu->SetPids(pmt.getServiceId(),Vpid, Vpid ? Ppid : 0, Apids, ALangs, Dpids, DLangs, Tpid);
#endif
        Menu->SetCaIds(pmt.getServiceId(),CaDescriptors->CaIds());
        Menu->SetCaDescriptors(pmt.getServiceId(),CaDescriptorHandler.AddCaDescriptors(CaDescriptors));
        }
     lastPmtScan[Index] = 0; // this triggers the next scan
//     Channels.Unlock();
     }
  if (num && num>=sdtFilter->numSid)
    endofScan=true;
}


// --- cSdtFilter ------------------------------------------------------------

SdtFilter::SdtFilter(PatFilter *PatFilter,cMenuScan* MENU)
{
  Menu=MENU;
  patFilter = PatFilter;
  numSid=0;
  Set(0x11, 0x42);  // SDT
}

void SdtFilter::SetStatus(bool On)
{
  cFilter::SetStatus(On);
  sectionSyncer.Reset();
}

void SdtFilter::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (!(Source() && Transponder()))
     return;
  SI::SDT sdt(Data, false);
  if (!sdt.CheckCRCAndParse())
     return;
  if (!sectionSyncer.Sync(sdt.getVersionNumber(), sdt.getSectionNumber(), sdt.getLastSectionNumber()))
     return;
  if (!Channels.Lock(true, 10))
     return;
  SI::SDT::Service SiSdtService;
  for (SI::Loop::Iterator it; sdt.serviceLoop.getNext(SiSdtService, it); ) {
      cChannel *channel = NULL; // Channels.GetByServiceID(Source(),Transponder(), SiSdtService.getServiceId());
      SI::Descriptor *d;
      for (SI::Loop::Iterator it2; (d = SiSdtService.serviceDescriptors.getNext(it2)); ) {
          switch (d->getDescriptorTag()) {
            case SI::ServiceDescriptorTag: {
                 SI::ServiceDescriptor *sd = (SI::ServiceDescriptor *)d;
                 switch (sd->getServiceType()) {
                   case 0x01: // digital television service
                   case 0x02: // digital radio sound service
                   case 0x04: // NVOD reference service
                   case 0x05: // NVOD time-shifted service
                        {
                        char NameBuf[1024];
                        char ShortNameBuf[1024];
                        char ProviderNameBuf[1024];
                        sd->serviceName.getText(NameBuf, ShortNameBuf, sizeof(NameBuf), sizeof(ShortNameBuf));
                        char *pn = compactspace(NameBuf);
                        char *ps = compactspace(ShortNameBuf);
                        sd->providerName.getText(ProviderNameBuf, sizeof(ProviderNameBuf));
                        char *pp = compactspace(ProviderNameBuf);
                        sid[numSid++]=SiSdtService.getServiceId();
                        if (channel) {
                           channel->SetId(sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId(),channel->Rid());
                           if (Setup.UpdateChannels >= 1)
                              channel->SetName(pn, ps, pp);
                           // Using SiSdtService.getFreeCaMode() is no good, because some
                           // tv stations set this flag even for non-encrypted channels :-(
                           // The special value 0xFFFF was supposed to mean "unknown encryption"
                           // and would have been overwritten with real CA values later:
                           // channel->SetCa(SiSdtService.getFreeCaMode() ? 0xFFFF : 0);
                           }
                        else if (*pn) {
                           Menu->NewChannel(Channel(), pn, ps, pp, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
                        //   channel = Channels.NewChannel(Channel(), pn, ps, pp, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
                           patFilter->Trigger();
                           }
                        }
                   }
                 }
                 break;
            case SI::NVODReferenceDescriptorTag: {
                 SI::NVODReferenceDescriptor *nrd = (SI::NVODReferenceDescriptor *)d;
                 SI::NVODReferenceDescriptor::Service Service;
                 for (SI::Loop::Iterator it; nrd->serviceLoop.getNext(Service, it); ) {
                 //    cChannel *link = Channels.GetByChannelID(tChannelID(Source(), Service.getOriginalNetworkId(), Service.getTransportStream(), Service.getServiceId()));
                     sid[numSid++]=SiSdtService.getServiceId();
                   //  if (!link) {
                      //  link = Channels.NewChannel(Channel(), "NVOD", "", "", Service.getOriginalNetworkId(), Service.getTransportStream(), Service.getServiceId());
                        patFilter->Trigger();
                      //  }
 /*                    if (link) {
                        if (!LinkChannels)
                           LinkChannels = new cLinkChannels;
                        LinkChannels->Add(new cLinkChannel(link));
                        }*/
//                 Menu->NewChannel(Channel(), pn, ps, pp, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
 
                     }
                 }
                 break;
            default: ;
            }
          delete d;
          }
/*      if (LinkChannels) {
         if (channel)
            channel->SetLinkChannels(LinkChannels);
         else
            delete LinkChannels;
         }*/
      }
  Channels.Unlock();
 if (sdt.getSectionNumber() == sdt.getLastSectionNumber()) { 
  SetStatus(false);
    }
}
