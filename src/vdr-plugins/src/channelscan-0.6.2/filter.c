/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia;  Author:  Markus Hahn          *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *
 ***************************************************************************
 *
 *  filter.c: collected and adapted pat sdt nit filter from VDR-core
 *
 ***************************************************************************/

#include <linux/dvb/frontend.h>
#include <map>
#include <vector>
#include <utility>

#include <vdr/s2reel_compat.h>
#include <vdr/tools.h>
#include <libsi/section.h>
#include <libsi/descriptor.h>

#include "csmenu.h"
#include "filter.h"

using std::vector;
using std::map;
using std::set;
using std::make_pair;


std::map < int, cTransponder * >transponderMap;
std::map < int, int >TblVersions;
std::map < int, const cSatTransponder > hdTransponders;
//std::list<int, cTransponder> hdTransponders;

#define DBGSDT " debug [sdt filter] "
//#define DEBUG_SDT(format, args...) printf (format, ## args)
#define DEBUG_SDT(format, args...)

#if 0
#define DEBUG_printf(format, args...) printf (format, ## args)
#else
#define DEBUG_printf(format, args...)
#endif

#define ST_TV_ONLY 0
#define ST_RADIO_ONLY 1
#define ST_HDTV_ONLY 2
#define ST_ALL 3

// --- cCaDescriptor ---------------------------------------------------------

class cCaDescriptor:public cListObject
{
  private:
    int caSystem;
    bool stream;
    int length;
    uchar *data;
  public:
    cCaDescriptor(int CaSystem, int CaPid, bool Stream, int Length, const uchar * Data);
    virtual ~ cCaDescriptor();
    bool operator==(const cCaDescriptor & arg) const;
    int CaSystem(void) const
    {
        return caSystem;
    }
    int Stream(void) const
    {
        return stream;
    }
    int Length(void) const
    {
        return length;
    }
    const uchar *Data(void) const
    {
        return data;
    }
};

cCaDescriptor::cCaDescriptor(int CaSystem, int CaPid, bool Stream, int Length, const uchar * Data)
{
    caSystem = CaSystem;
    stream = Stream;
    length = Length + 6;
    data = MALLOC(uchar, length);
    data[0] = SI::CaDescriptorTag;
    data[1] = length - 2;
    data[2] = (caSystem >> 8) & 0xFF;
    data[3] = caSystem & 0xFF;
    data[4] = ((CaPid >> 8) & 0x1F) | 0xE0;
    data[5] = CaPid & 0xFF;
    if (Length)
        memcpy(&data[6], Data, Length);
}

cCaDescriptor::~cCaDescriptor()
{
    free(data);
}

bool cCaDescriptor::operator==(const cCaDescriptor & arg) const
{
    return length == arg.length && memcmp(data, arg.data, length) == 0;
}

// --- cCaDescriptors --------------------------------------------------------

class cCaDescriptors:public cListObject
{
  private:
    int source;
    int transponder;
    int serviceId;
    int numCaIds;
    int caIds[MAXCAIDS + 1];
    cList < cCaDescriptor > caDescriptors;
    void AddCaId(int CaId);
  public:
    cCaDescriptors(int Source, int Transponder, int ServiceId);
    bool operator==(const cCaDescriptors & arg) const;
    bool Is(int Source, int Transponder, int ServiceId);
    bool Is(cCaDescriptors * CaDescriptors);
    bool Empty(void)
    {
        return caDescriptors.Count() == 0;
    }
    void AddCaDescriptor(SI::CaDescriptor * d, bool Stream);
    int GetCaDescriptors(const unsigned short *CaSystemIds, int BufSize, uchar * Data, bool & StreamFlag);
    const int *CaIds(void)
    {
        return caIds;
    }
};

cCaDescriptors::cCaDescriptors(int Source, int Transponder, int ServiceId)
{
    source = Source;
    transponder = Transponder;
    serviceId = ServiceId;
    numCaIds = 0;
    caIds[0] = 0;
}

bool cCaDescriptors::operator==(const cCaDescriptors & arg) const
{
    cCaDescriptor *ca1 = caDescriptors.First();
    cCaDescriptor *ca2 = arg.caDescriptors.First();
    while (ca1 && ca2)
    {
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

bool cCaDescriptors::Is(cCaDescriptors * CaDescriptors)
{
    return Is(CaDescriptors->source, CaDescriptors->transponder, CaDescriptors->serviceId);
}

void cCaDescriptors::AddCaId(int CaId)
{
    if (numCaIds < MAXCAIDS)
    {
        for (int i = 0; i < numCaIds; i++)
        {
            if (caIds[i] == CaId)
                return;
        }
        caIds[numCaIds++] = CaId;
        caIds[numCaIds] = 0;
    }
}

void cCaDescriptors::AddCaDescriptor(SI::CaDescriptor * d, bool Stream)
{
    cCaDescriptor *nca = new cCaDescriptor(d->getCaType(), d->getCaPid(), Stream,
                                           d->privateData.getLength(),
                                           d->privateData.getData());
    for (cCaDescriptor * ca = caDescriptors.First(); ca; ca = caDescriptors.Next(ca))
    {
        if (*ca == *nca)
        {
            delete nca;
            return;
        }
    }
    AddCaId(nca->CaSystem());
    caDescriptors.Add(nca);
}

int cCaDescriptors::GetCaDescriptors(const unsigned short *CaSystemIds, int BufSize, uchar * Data, bool & StreamFlag)
{
    if (!CaSystemIds || !*CaSystemIds)
        return 0;
    if (BufSize > 0 && Data)
    {
        int length = 0;
        int IsStream = -1;
        for (cCaDescriptor * d = caDescriptors.First(); d; d = caDescriptors.Next(d))
        {
            const unsigned short *caids = CaSystemIds;
            do
            {
                if (*CaSystemIds == 0xFFFF || d->CaSystem() == *caids)
                {
                    if (length + d->Length() <= BufSize)
                    {
                        if (IsStream >= 0 && IsStream != d->Stream())
                            DLOG("CAM: different stream flag in CA descriptors");
                        IsStream = d->Stream();
                        memcpy(Data + length, d->Data(), d->Length());
                        length += d->Length();
                    }
                    else
                        return -1;

                }
            }
            while (*++caids);
        }
        StreamFlag = IsStream == 1;
        return length;
    }
    return -1;
}

// --- cCaDescriptorHandler --------------------------------------------------

class cCaDescriptorHandler:public cList < cCaDescriptors >
{
  private:
    cMutex mutex;
  public:
    int AddCaDescriptors(cCaDescriptors * CaDescriptors);
    // Returns 0 if this is an already known descriptor,
    // 1 if it is an all new descriptor with actual contents,
    // and 2 if an existing descriptor was changed.
    int GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar * Data, bool & StreamFlag);
};

int cCaDescriptorHandler::AddCaDescriptors(cCaDescriptors * CaDescriptors)
{
    cMutexLock MutexLock(&mutex);
    for (cCaDescriptors * ca = First(); ca; ca = Next(ca))
    {
        if (ca->Is(CaDescriptors))
        {
            if (*ca == *CaDescriptors)
            {
                delete CaDescriptors;
                return 0;
            }
            Del(ca);
            Add(CaDescriptors);
            return 2;
        }
    }
    Add(CaDescriptors);
    return CaDescriptors->Empty()? 0 : 1;
}

int cCaDescriptorHandler::GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar * Data, bool & StreamFlag)
{
    cMutexLock MutexLock(&mutex);
    StreamFlag = false;
    for (cCaDescriptors * ca = First(); ca; ca = Next(ca))
    {
        if (ca->Is(Source, Transponder, ServiceId))
            return ca->GetCaDescriptors(CaSystemIds, BufSize, Data, StreamFlag);
    }
    return 0;
}

cCaDescriptorHandler CaDescriptorHandler;

int GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar * Data, bool & StreamFlag)
{
    return CaDescriptorHandler.GetCaDescriptors(Source, Transponder, ServiceId, CaSystemIds, BufSize, Data, StreamFlag);
}

// --- PatFilter ------------------------------------------------------------
PatFilter::PatFilter()
{
    sdtFilter = NULL;
    pmtIndex = 0;
    for (int i = 0; i < MAXFILTERS; i++)
        pmtPid[i] = 0;
    numPmtEntries = 0;
    Set(0x00, 0x00);            // PAT
#ifdef REELVDR
    Set(0x1fff, 0x42);          // decrease latency
#endif
    endofScan = false;
    SetStatus(false);
    pit = pnum = 0;
    sdtfinished = false;
    for (int n = 0; n < 128; n++)
        pSid[n] = -1;
    pSidCnt = 0;
    lastFound = 0;
    waitingForGodot = 0;
}

void PatFilter::SetSdtFilter(SdtFilter * SdtFilter)
{
    sdtFilter = SdtFilter;
    SetStatus(true);
}

void PatFilter::SetStatus(bool On)
{
    cFilter::SetStatus(On);
    pmtIndex = 0;
    for (int i = 0; i < MAXFILTERS; i++)
        pmtPid[i] = 0;
    numPmtEntries = 0;
    num = 0;
    numRunning = 0;
    pit = pnum = 0;
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
    for (int i = 0; i < numPmtEntries; i++)
    {
        if ((pmtVersion[i] & 0x00000000FFFFFFFFLL) == id)
        {
            bool Changed = (pmtVersion[i] & 0x000000FF00000000LL) != v;
            if (Changed)
                pmtVersion[i] = id | v;
            return Changed;
        }
    }
    if (numPmtEntries < CMAXPMTENTRIES)
        pmtVersion[numPmtEntries++] = id | v;
    return true;
}

bool PatFilter::SidinSdt(int Sid)
{
    for (int i = 0; i < sdtFilter->numSid; i++)
        if (sdtFilter->sid[i] == Sid && sdtFilter->usefulSid[i])
        {
            for (int j = 0; j < MAXFILTERS; j++)
                if (Sids[j] == Sid)
                    return false;
            return true;
        }
    return false;
}

void PatFilter::Process(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
    if (lastFound < time(NULL) && waitingForGodot < 10)
    {                           // There's something on this channel, waiting a bit longer...
        waitingForGodot++;
        lastFound++;
    }

    if (Pid == 0x00)
    {
        if (Tid == 0x00)
        {
            for (int i = 0; i < MAXFILTERS; i++)
            {
                if (pmtPid[i] && (time(NULL) - lastPmtScan[i]) > FILTERTIMEOUT)
                {
                    // x DEBUG_printf("Del %i as %i\n", pmtPid[i], i); ///DBG
                    Del(pmtPid[i], 0x02);
                    pmtPid[i] = 0;  // Note for recycling, but do not remove from feed
                    pSid[pSidCnt++] = Sids[i];
                    num++;
                    for (int k = 0; i < sdtFilter->numSid; k++)
                        if (sdtFilter->sid[k] == Sids[i])
                        {
                            sdtFilter->sid[k] = 0;
                            break;
                        }
                    Sids[i] = 0;
                    pmtIndex++;
                }
            }
            SI::PAT pat(Data, false);
            if (!pat.CheckCRCAndParse())
                return;
            SI::PAT::Association assoc;

            int tnum = 0, tSid[100];

            for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it);)
            {
                int xSid = assoc.getServiceId();

                tSid[tnum++] = xSid;
                int sidfound = 0;

                for (int n = 0; n < pSidCnt; n++)
                {
                    if (pSid[n] == xSid)
                    {
                        sidfound = 1;
                        break;
                    }
                }

                if (!sidfound && !assoc.isNITPid() && SidinSdt(assoc.getServiceId()))
                {

                    int Index = 0;
                    int foundIndex = 0;

                    // Find free filter PID
                    for (Index = 0; Index < MAXFILTERS; Index++)
                    {
                        if (pmtPid[Index] == 0)
                        {
                            foundIndex = 1;
                            break;
                        }
                    }

                    if (foundIndex)
                    {
                        pmtPid[Index] = assoc.getPid();
                        Sids[Index] = xSid;
                        lastPmtScan[Index] = time(NULL);
                        //  DEBUG_printf("ADD %i as %i, Sid %i\n",pmtPid[Index],Index,xSid);
                        Add(pmtPid[Index], 0x02);
                        pSid[pSidCnt++] = xSid;
                    }
                }
            }
        }
    }
    else if (Tid == SI::TableIdPMT && Source() && Transponder())
    {
        int Index = -1;
        for (int i = 0; i < MAXFILTERS; i++)
            if (Pid == pmtPid[i])
                Index = i;
        SI::PMT pmt(Data, false);
        if (!pmt.CheckCRCAndParse())
            return;
        if (!Channels.Lock(true, 10))
        {
            numPmtEntries = 0;  // to make sure we try again
            return;
        }

        cChannel *Channel = Channels.GetByServiceID(Source(), Transponder(),
                                                    pmt.getServiceId());
        if (Channel && Index != -1)
        {
            SI::CaDescriptor * d;
            cCaDescriptors *CaDescriptors = new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Sid());
            // Scan the common loop:
            for (SI::Loop::Iterator it; (d = (SI::CaDescriptor *) pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag));)
            {
                CaDescriptors->AddCaDescriptor(d, false);
                delete d;
            }
            // Scan the stream-specific loop:
            SI::PMT::Stream stream;
            int Vpid  = 0;
            int Vtype = 0;
            int Ppid  = 0;
            int Apids[MAXAPIDS + 1] = { 0 };
            int Dpids[MAXDPIDS + 1] = { 0 };
            int Spids[MAXSPIDS + 1] = { 0 };
            int Atypes[MAXAPIDS + 1] = { 0 };
            int Dtypes[MAXDPIDS + 1] = { 0 };
#if VDRVERSNUM >= 10332
            char ALangs[MAXAPIDS + 1][MAXLANGCODE2] = { "" };
            char DLangs[MAXDPIDS + 1][MAXLANGCODE2] = { "" };
            char SLangs[MAXSPIDS + 1][MAXLANGCODE2] = { "" };
#else
            char ALangs[MAXAPIDS + 1][4] = { "" };
            char DLangs[MAXDPIDS + 1][4] = { "" };
            char SLangs[MAXSPIDS + 1][4] = { "" };
#endif
            int Tpid = 0;
            int NumApids = 0;
            int NumDpids = 0;
            for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it);)
            {
                //  DEBUG_printf("sid: %5d pid %5d str_t%2X \n", pmt.getServiceId(), stream.getPid(), stream.getStreamType());

                switch (stream.getStreamType())
                {
                case 1:    // STREAMTYPE_11172_VIDEO
                case 2:    // STREAMTYPE_13818_VIDEO
                case 0x10: // 14496-2 Visual MPEG-4
                case 0x1b: // 14496-10 Video h.264
                    Vtype = stream.getStreamType();
                    Vpid  = stream.getPid();
                    Ppid  = pmt.getPCRPid();
                    break;
                case 3:    // STREAMTYPE_11172_AUDIO
                case 4:    // STREAMTYPE_13818_AUDIO
                case 0x11: // STREAMTYPE_14496_AUDIO (AAC)
                    {
                        if (NumApids < MAXAPIDS)
                        {
                            Apids[NumApids] = stream.getPid();
                            Atypes[NumApids] = stream.getStreamType();
                            SI::Descriptor * d;
                            for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));)
                            {
                                switch (d->getDescriptorTag())
                                {
                                case SI::ISO639LanguageDescriptorTag:
                                    {
                                        SI::ISO639LanguageDescriptor * ld = (SI::ISO639LanguageDescriptor *) d;
                                        if (*ld->languageCode != '-')
                                        {   // some use "---" to indicate "none"
                                            strn0cpy(ALangs[NumApids], I18nNormalizeLanguageCode(ld->languageCode), 4);
                                            ALangs[NumApids][4] = 0;
                                        }
                                    }
                                    break;
                                default:;
                                }
                                delete d;
                            }
                            NumApids++;
                        }
                    }
                    break;
                case 5:        // STREAMTYPE_13818_PRIVATE
                case 6:        // STREAMTYPE_13818_PES_PRIVATE
                    //XXX case 8: // STREAMTYPE_13818_DSMCC
                    {
                        int dpid = 0;
                        char lang[4] = { 0 };
                        SI::Descriptor * d;
                        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));)
                        {
                            switch (d->getDescriptorTag())
                            {
                            case SI::AC3DescriptorTag:
                                dpid = stream.getPid();
                                break;
                            case SI::TeletextDescriptorTag:
                                Tpid = stream.getPid();
                                break;
                            case SI::ISO639LanguageDescriptorTag:
                                {
                                    SI::ISO639LanguageDescriptor * ld = (SI::ISO639LanguageDescriptor *) d;
                                    strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), 4);
                                }
                                break;
                            default:;
                            }
                            delete d;
                        }
                        if (dpid)
                        {
                            if (NumDpids < MAXDPIDS)
                            {
                                Dpids[NumDpids] = dpid;
                                Dtypes[NumDpids] = stream.getStreamType();
                                strn0cpy(DLangs[NumDpids], lang, 4);
                                NumDpids++;
                            }
                        }
                    }
                    break;
                }
                for (SI::Loop::Iterator it; (d = (SI::CaDescriptor *) stream.streamDescriptors.getNext(it, SI::CaDescriptorTag));)
                {
                    CaDescriptors->AddCaDescriptor(d, true);
                    delete d;
                }
            }
#if VDRVERSNUM >= 10716
            Channel->SetPids(Vpid, Vpid ? Ppid : 0, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
#elif APIVERSNUM >= 10509 || defined(REELVDR)
            Channel->SetPids(Vpid, Vpid ? Ppid : 0, Apids, ALangs, Dpids, DLangs, Spids, SLangs, Tpid);
#else
            Channel->SetPids(Vpid, Vpid ? Ppid : 0, Apids, ALangs, Dpids, DLangs, Tpid);
#endif
            //DEBUG_printf("#### %i %s %i %i SID  %i\n",num,Channel->Name(),Vpid, Apids[0], Channel->Sid());
            Channel->SetCaIds(CaDescriptors->CaIds());
            Channel->SetCaDescriptors(CaDescriptorHandler.AddCaDescriptors(CaDescriptors));

            //DEBUG_printf("Del %i as %i\n", pmtPid[Index], Index);
            Del(pmtPid[Index], 0x02);
            pmtPid[Index] = 0;

            num++;
            numRunning++;
            lastFound = time(NULL);
        }
#if 1
        if (Index != -1)
            lastPmtScan[Index] = 0; // this triggers the next scan
#endif
        Channels.Unlock();
    }
    if (sdtfinished && num >= sdtFilter->numUsefulSid)
    {
        //DEBUG_printf("#### num %i sid %i  EOS \n", num,sdtFilter->numSid);
        endofScan = true;
    }
}

void PatFilter::GetFoundNum(int &current, int &total)
{
    current = numRunning;
    total = (sdtFilter ? sdtFilter->numSid : 0);
    if (total > 1000 || total < 0)
        total = 0;
}

// --- cSdtFilter ------------------------------------------------------------

SdtFilter::SdtFilter(PatFilter * PatFilter)
{
    patFilter = PatFilter;
    numSid = 0;
    numUsefulSid = 0;
    Set(0x11, 0x42);            // SDT
#ifdef REELVDR
    Set(0x1fff, 0x42);
#endif
    AddServiceType = ScanSetup.ServiceType;
}

void SdtFilter::SetStatus(bool On)
{
    cFilter::SetStatus(On);
    sectionSyncer.Reset();
}

void SdtFilter::Process(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
    const time_t tt = time(NULL);
    char *strDate;
    asprintf(&strDate, "%s", asctime(localtime(&tt)));
    DEBUG_printf("\nSdtFilter::Process IN %s", strDate);

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
    for (SI::Loop::Iterator it; sdt.serviceLoop.getNext(SiSdtService, it);)
    {
        cChannel *channel = Channels.GetByChannelID(tChannelID(Source(), sdt.getOriginalNetworkId(),
                                                               sdt.getTransportStreamId(), SiSdtService.getServiceId()));
        if (!channel)
            channel = Channels.GetByChannelID(tChannelID(Source(), 0, Transponder(), SiSdtService.getServiceId()));

        cLinkChannels *LinkChannels = NULL;
        SI::Descriptor * d;

        for (SI::Loop::Iterator it2; (d = SiSdtService.serviceDescriptors.getNext(it2));)
        {


            switch (d->getDescriptorTag())
            {
            case SI::ServiceDescriptorTag:
                {
                    SI::ServiceDescriptor * sd = (SI::ServiceDescriptor *) d;
                    char NameBufDeb[1024];
                    char ShortNameBufDeb[1024];
                    sd->serviceName.getText(NameBufDeb, ShortNameBufDeb, sizeof(NameBufDeb), sizeof(ShortNameBufDeb));
                    DEBUG_SDT(DBGSDT " \033[0;43m  Name %s --  ServiceType: %X: AddServiceType %d, Sid %i, running %i \033[0m\n", NameBufDeb, sd->getServiceType(), AddServiceType, SiSdtService.getServiceId(), SiSdtService.getRunningStatus());


                    switch (sd->getServiceType())
                    {
                    case 0x01: // digital television service
                    case 0x02: // digital radio sound service
                    case 0x03: // DVB Subtitles
                    case 0x04: // NVOD reference service
                    case 0x05: // NVOD time-shifted service
                    case 0x11: // digital television service MPEG-2 HD  ??? never seen !
                    case 0x19: // digital television service MPEG-4 HD
                    case 0xC3: // some french channels like kiosk
                    case 0x86: // ?? Astra 28.2 MPEG-4 HD

                        {

                            // esyslog("%s sd->getServiceType()=%d  AddServiceType=%d",__FUNCTION__, sd->getServiceType(), AddServiceType);
                            if (!(sd->getServiceType() == 0x11 || sd->getServiceType() == 0x19) && AddServiceType == ST_HDTV_ONLY)  // (! HD TV && HDOnly) break
                            {
                                DEBUG_SDT(DBGSDT "  \033[0;48m +++++++++++++++  NO Found HD CHANNEL: Skip %s +++++++++++++++ \033[0m \n", NameBufDeb);
                                break;
                            }


                            // Add only radio
                            if ((sd->getServiceType() == 1 || sd->getServiceType() == 0x11 || sd->getServiceType() == 0x19 || sd->getServiceType() == 0xC3) && AddServiceType == ST_RADIO_ONLY) // (TV && radioOnly) break
                            {
                                DEBUG_SDT(DBGSDT " Add nur Radio  aber nur TV Sender gefunden  SID skip %d \n", sd->getServiceType());
                                break;
                            }
                            // Add only tv
                            if (sd->getServiceType() == 2 && (AddServiceType == ST_TV_ONLY || AddServiceType == ST_HDTV_ONLY))  // RadioSender && (TVonly || HDTvonly) break
                            {
                                DEBUG_SDT(DBGSDT " Add nur TV  aber nur RadioSender gefunden  SID skip %d \n", sd->getServiceType());
                                break;
                            }
                            char NameBuf[1024];
                            char ShortNameBuf[1024];
                            char ProviderNameBuf[1024];
                            sd->serviceName.getText(NameBuf, ShortNameBuf, sizeof(NameBuf), sizeof(ShortNameBuf));
                            char *pn = compactspace(NameBuf);
                            char *ps = compactspace(ShortNameBuf);
                            sd->providerName.getText(ProviderNameBuf, sizeof(ProviderNameBuf));
                            char *pp = compactspace(ProviderNameBuf);

                            if (SiSdtService.getRunningStatus() > SI::RunningStatusNotRunning || SiSdtService.getRunningStatus() == SI::RunningStatusUndefined) // see DVB BlueBook A005 r5, section 4.3
                            {

                                mutexNames.Lock();
                                switch (sd->getServiceType())
                                {
                                case 0x1:
                                case 0x11:
                                case 0x19:
                                case 0xC3:
                                case 0x86:
                                    tvChannelNames.push_back(NameBuf);  // if service wanted
                                    break;
                                case 0x2:
                                    radioChannelNames.push_back(NameBuf);   // if service wanted
                                    break;
                                default:;
                                    //dataChannelNames.push_back(NameBuf);
                                }
                                mutexNames.Unlock();
                                usefulSid[numSid] = 1;
                                numUsefulSid++;
                            }
                            else
                                usefulSid[numSid] = 0;

                            sid[numSid++] = SiSdtService.getServiceId();

                            if (channel)
                            {
                                DEBUG_SDT(DBGSDT " \033[0;42m---------------------- Channelscan Add Chanel pn %s ps %s pp %s --------------\033[0m\n", pn, ps, pp);
                                channel->SetId(sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId(), channel->Rid());
                                //if (Setup.UpdateChannels >= 1)
                                channel->SetName(pn, ps, pp);
#if defined(REELVDR)
#if VDRVERSNUM < 10716
                                channel->SetServiceType(sd->getServiceType());
#endif
#endif
                                // Using SiSdtService.getFreeCaMode() is no good, because some
                                // tv stations set this flag even for non-encrypted channels :-(
                                // The special value 0xFFFF was supposed to mean "unknown encryption"
                                // and would have been overwritten with real CA values later:
                                // channel->SetCa(SiSdtService.getFreeCaMode() ? 0xFFFF : 0);
                            }
                            else if (*pn)
                            {
                                channel = Channels.NewChannel(Channel(), pn, ps, pp, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
#if defined(REELVDR)
#if VDRVERSNUM < 10716

                                channel->SetServiceType(sd->getServiceType());
#endif
#endif
                                patFilter->Trigger();
                                if (SiSdtService.getServiceId() == 0x12)
                                {
                                    DEBUG_SDT(DBGSDT "-------- found ServiceID for PremiereDirekt!  %s - %s - %s --------- \n", pn, ps, pp);
                                    //eitFilter->Trigger();
                                }
                            }
                        }       // end case Digital TV services
                    }
                }
                break;
            case SI::NVODReferenceDescriptorTag:
                {
                    SI::NVODReferenceDescriptor * nrd = (SI::NVODReferenceDescriptor *) d;
                    SI::NVODReferenceDescriptor::Service Service;
                    for (SI::Loop::Iterator it; nrd->serviceLoop.getNext(Service, it);)
                    {
                        cChannel *link = Channels.GetByChannelID(tChannelID(Source(),
                                                                            Service.getOriginalNetworkId(),
                                                                            Service.getTransportStream(),
                                                                            Service.getServiceId()));
                        if (!link)
                        {

                            usefulSid[numSid] = 0;

                            sid[numSid++] = SiSdtService.getServiceId();
                            link = Channels.NewChannel(Channel(), "NVOD", "", "", Service.getOriginalNetworkId(), Service.getTransportStream(), Service.getServiceId());
                            patFilter->Trigger();
                        }

                        if (link)
                        {
                            if (!LinkChannels)
                                LinkChannels = new cLinkChannels;
                            LinkChannels->Add(new cLinkChannel(link));
                        }
                    }
                }
                break;
            default:;
            }
            delete d;
        }
        if (LinkChannels)
        {
            if (channel)
                channel->SetLinkChannels(LinkChannels);
            else
                delete LinkChannels;
        }

    }
    Channels.Unlock();
    if (sdt.getSectionNumber() == sdt.getLastSectionNumber())
    {
        patFilter->SdtFinished();
        SetStatus(false);
    }
    const time_t ttout = time(NULL);
    asprintf(&strDate, "%s", asctime(localtime(&ttout)));
    DEBUG_printf("\n\nSdtFilter::Process OUT :%4.1fsec: %s\n", (float)difftime(ttout, tt), strDate);
}


// --- NitFilter  ---------------------------------------------------------

#ifdef DBG
# undef DBG
#endif
#define DBGNIT  " DEBUG [cs-nit]: "
//#define DEBUG_NIT(format, args...) printf (format, ## args)
#define DEBUG_NIT(format, args...)


NitFilter::NitFilter()
{
    numNits = 0;
    networkId = 0;
    lastCount = 0;
    found_ = endofScan = false;
    Set(0x10, 0x40);            // NIT
    //Set(0x10, 0x41);  // other NIT
#ifdef REELVDR
    Set(0x1fff, 0x42);          // decrease latency
#endif
    vector < int >tmp(64, 0);
    sectionSeen_ = tmp;
}

void NitFilter::SetStatus(bool On)
{
    cFilter::SetStatus(true);
    numNits = 0;
    networkId = 0;
    sectionSyncer.Reset();
    lastCount = 0;
    found_ = endofScan = false;
    vector < int >tmp(64, 0);
    sectionSeen_ = tmp;
}

NitFilter::~NitFilter()
{
}

bool NitFilter::Found()
{
    return found_;

}

void NitFilter::Process(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
    SI::NIT nit(Data, false);
    if (!nit.CheckCRCAndParse())
        return;

    // TODO
    // if versionsMap(nit.getVersionNumber()) == false)
    //  return;


    DEBUG_NIT(DBGNIT " ++ %s NIT ID  %d\n", __PRETTY_FUNCTION__, nit.getNetworkId());
    //DEBUG_NIT(DBGNIT " ++ Version %d \n", nit.getVersionNumber());
    DEBUG_NIT(DBGNIT " ++ SectionNumber  %d\n", nit.getSectionNumber());
    DEBUG_NIT(DBGNIT " ++ LastSectionNumber  %d\n", nit.getLastSectionNumber());
    DEBUG_NIT(DBGNIT " ++ moreThanOneSection %d\n", nit.moreThanOneSection());
    DEBUG_NIT(DBGNIT " ++ num Nits  %d\n", numNits);

    int getTransponderNum = 0;
    //
    // return if we have seen the Table already
    int cnt = ++TblVersions[nit.getVersionNumber()];
    if (cnt > nit.getLastSectionNumber() + 1)
    {
        DEBUG_NIT(DBGNIT "DEBUG [nit]: ++  NIT Version %d found %d times \n", cnt, nit.getVersionNumber());
        endofScan = true;
        return;
    }
    // Some broadcasters send more than one NIT, with no apparent way of telling which
    // one is the right one to use. This is an attempt to find the NIT that contains
    // the transponder it was transmitted on and use only that one:
    //int transportstreamId = SI::NIT::TransportStream::getTransportStreamId();
    found_ = endofScan = false;
    bool insert = false;
    int ThisNIT = -1;
    //if (!networkId)
    if (true)
    {
        for (int i = 0; i < numNits; i++)
        {
            DEBUG_NIT(DBGNIT " ++ num Nits  %d\n", numNits);
            if (nits[i].networkId == nit.getNetworkId())
            {
                if (nit.getSectionNumber() == 0)
                {
                    // all NITs have passed by
                    for (int j = 0; j < numNits; j++)
                    {
                        DEBUG_NIT(DBGNIT " --  nits[%d] has Transponder ? %s \n", j, nits[j].hasTransponder ? "YES" : "NO");
                        if (nits[j].hasTransponder)
                        {
                            networkId = nits[j].networkId;
                            DEBUG_NIT(DBGNIT " take  NIT with network ID %d\n", networkId);
                            //XXX what if more than one NIT contains this transponder???
                            break;
                        }
                    }
                    if (!networkId)
                    {
                        DEBUG_NIT(DBGNIT "none of the NITs contains transponder %d\n", Transponder());
                        return;
                    }
                }
                else
                {
                    DEBUG_NIT(DBGNIT " ----------- ThisNIT: %d --------------  \n", i);
                    ThisNIT = i;
                    break;
                }
            }
        }
        if (!networkId && ThisNIT < 0 && numNits < MAXNITS)
        {
            if (nit.getSectionNumber() == 0)
            {
                *nits[numNits].name = 0;
                SI::Descriptor * d;
                //  DEBUG_NIT(DBGNIT" INFO [nit] ----------- loop common descriptors:  --------------  \n");

                for (SI::Loop::Iterator it; (d = nit.commonDescriptors.getNext(it));)
                {
                    switch (d->getDescriptorTag())
                    {
                    case SI::NetworkNameDescriptorTag:
                        {
                            SI::NetworkNameDescriptor * nnd = (SI::NetworkNameDescriptor *) d;
                            nnd->name.getText(nits[numNits].name, MAXNETWORKNAME_CS);
                            DEBUG_NIT(DBGNIT " ----------- Get Name %s   --------------  \n", nits[numNits].name);
                        }
                        break;
                    default:;
                    }
                    delete d;
                }
                nits[numNits].networkId = nit.getNetworkId();
                nits[numNits].hasTransponder = false;
                DEBUG_NIT(DBGNIT " ---- NIT[%d] ID: %5d Proivider '%s'\n", numNits, nits[numNits].networkId, nits[numNits].name);
                ThisNIT = numNits;
                numNits++;
            }
        }
    }
    else if (networkId != nit.getNetworkId())
    {
        DEBUG_NIT(DBGNIT "found !!!! OTHER  NIT !!!!!  %d previos NIT %d \n", nit.getNetworkId(), networkId);
        //return; // ignore all other NITs
    }
    else if (!sectionSyncer.Sync(nit.getVersionNumber(), nit.getSectionNumber(), nit.getLastSectionNumber()))
        return;


    sectionSeen_[nit.getSectionNumber()]++;

    //DEBUG_NIT(DBGNIT " -- SectionNumber %d   \n", nit.getSectionNumber());

    SI::NIT::TransportStream ts;
    for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it);)
    {
        insert = false;
        //DEBUG_NIT(DBGNIT " -- found TS_ID %d\n",  ts.getTransportStreamId());

        SI::Descriptor * d;

        SI::Loop::Iterator it2;
        SI::FrequencyListDescriptor * fld = (SI::FrequencyListDescriptor *) ts.transportStreamDescriptors.getNext(it2, SI::FrequencyListDescriptorTag);
        int NumFrequencies = fld ? fld->frequencies.getCount() + 1 : 1;
        int Frequencies[NumFrequencies];
        if (fld)
        {
            int ct = fld->getCodingType();
            if (ct > 0)
            {
                int n = 1;
                for (SI::Loop::Iterator it3; fld->frequencies.hasNext(it3);)
                {
                    int f = fld->frequencies.getNext(it3);
                    switch (ct)
                    {
                    case 1:
                        f = BCD2INT(f) / 100;
                        break;
                    case 2:
                        f = BCD2INT(f) / 10;
                        break;
                    case 3:
                        f = f * 10;
                        break;
                    }
                    Frequencies[n++] = f;
                }
            }
            else
                NumFrequencies = 1;
        }
        delete fld;

        for (SI::Loop::Iterator it2; (d = ts.transportStreamDescriptors.getNext(it2));)
        {
            switch (d->getDescriptorTag())
            {
            case SI::SatelliteDeliverySystemDescriptorTag:
                {
                    SI::SatelliteDeliverySystemDescriptor * sd = (SI::SatelliteDeliverySystemDescriptor *) d;
                    //int Source = cSource::FromData(cSource::stSat, BCD2INT(sd->getOrbitalPosition()), sd->getWestEastFlag());
                    int Frequency = Frequencies[0] = BCD2INT(sd->getFrequency()) / 100;
                    getTransponderNum++;
                    //DEBUG_NIT(DBGNIT  "  -- %s:%d DEBUG [nit]:  Get  Sat DSDT f: %d --   num %d   \n", __FILE__, __LINE__, Frequency, getTransponderNum++);
                    static char Polarizations[] = { 'h', 'v', 'l', 'r' };
                    char Polarization = Polarizations[sd->getPolarization()];


                    // orig
                    /* static int CodeRates[] = { FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4, FEC_4_5, FEC_5_6, FEC_7_8, FEC_8_9,
                       FEC_3_5, FEC_9_10, FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_NONE }; */

                    // same as in channels.c
                    static int CodeRates[] = { FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4, FEC_4_5, FEC_5_6, 
                                               FEC_6_7, FEC_7_8, FEC_8_9,    // DVB-S
#ifndef DVBAPI_V5
                                               FEC_1_3, FEC_1_4, FEC_2_5, 
#endif
                                               FEC_3_5, FEC_9_10, FEC_AUTO, 
                                               FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_AUTO, FEC_NONE
                                             };          // DVB-S2





                    int CodeRate = CodeRates[sd->getFecInner()];
                    int SymbolRate = BCD2INT(sd->getSymbolRate()) / 10;
                    for (int n = 0; n < NumFrequencies; n++)
                    {

                        cChannel *Channel = new cChannel;
                        Channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                        printf("# %d Mhz  TSID %5d  orig NIT %6d \n", Frequencies[n], ts.getTransportStreamId(), ts.getOriginalNetworkId());

                        static int Rolloffs[] = { ROLLOFF_35, ROLLOFF_25, ROLLOFF_20, ROLLOFF_35 };


                        int Modulation = QPSK;

                        /*
                           static int Modulations[] = { QPSK, QPSK, QPSK, QPSK,      // DVB-S
                           QPSK, QPSK_S2, PSK8, QPSK };
                         */

#if VDRSERSNUM < 10716
                        static int ModulationsS2[] = { QPSK, QPSK_S2, PSK_8, QPSK, QPSK, QPSK, QPSK, QPSK }; // DVB-S2
#else
                        static int ModulationsS2[] = { QAM_AUTO, QPSK, PSK_8, QPSK, QPSK, QPSK, QPSK, QPSK }; // DVB-S2
#endif

                        int RollOff = Rolloffs[sd->getRollOff()];

                        if (sd->getModulationSystem() == 1)
                        {       // HD
                            if (Frequency == 11914)
                                Modulation = ModulationsS2[sd->getModulationType()];

                            if (Frequency == 11914)
                                printf("11914 SI f %d sr:%d mod:SI %d->%d code:SI%d->%d ro:xx->%d \n", Frequency, SymbolRate, sd->getModulationType(), Modulation, sd->getFecInner(), CodeRate, RollOff);
                        }

                        /*  let the tuner work
                           else {
                           Modulation = Modulations[sd->getModulationType()];
                           }
                         */

                        cSatTransponder *t = new cSatTransponder(Frequency, Polarization,
                                                                 SymbolRate, Modulation, CodeRate, RollOff, sd->getModulationSystem());

                        if (!t)
                        {
                            esyslog("FatalError new cSatTransponder %d failed", Frequency);
                        }
                        else
                        {
                            mapRet ret = transponderMap.insert(make_pair(Frequency, t));
                            if (ret.second)
                                printf(" New transponder f: %d  p: %c sr: %d (mod_si: %d  mod: %d, ro %d )    \n", Frequency, Polarization, SymbolRate, sd->getModulationType(), Modulation, RollOff);
                            else
                                delete t;
                        }
                    }
                }
                break;
            default:;
            }

            delete d;
        }
    }
    found_ = insert;

    DEBUG_NIT(DBGNIT " ++  End of ProcessLoop MapSize: %d  lastCount %d   \n", (int)transponderMap.size(), lastCount);

    DEBUG_NIT(DBGNIT " -- NIT ID  %d\n", nit.getNetworkId());
    DEBUG_NIT(DBGNIT " -- Version %d \n", nit.getVersionNumber());
    DEBUG_NIT(DBGNIT " -- moreThanOneSection %d\n", nit.moreThanOneSection());
    DEBUG_NIT(DBGNIT " -- SectionNumber  %d\n", nit.getSectionNumber());
    DEBUG_NIT(DBGNIT " -- LastSectionNumber  %d\n", nit.getLastSectionNumber());
    DEBUG_NIT(DBGNIT " -- moreThanOneSection %d\n", nit.moreThanOneSection());

    if (!nit.moreThanOneSection())
    {
        endofScan = true;
    }
    else
    {
        endofScan = true;
        DEBUG_NIT(DBGNIT "DEBUG [nit]:  -- LastSectionNumber  %d\n", nit.getLastSectionNumber());
        //for (int i = 0; i<sectionSeen.size();i++)
        for (int i = 0; i < nit.getLastSectionNumber() + 1; i++)
        {
            DEBUG_NIT(DBGNIT "DEBUG [nit]:  -- Seen[%d] %s\n", i, sectionSeen_[i] ? "YES" : "NO");
            if (sectionSeen_[i] == 0)
            {
                endofScan = false;
                break;
            }
        }
    }

    if (endofScan == true)
    {
        //printf ("DEBUG [channescan ]: filter.c  End of ProcessLoop newMap size: %d  \n", (int)transponderMap.size());
        vector < int >tmp(64, 0);
        sectionSeen_ = tmp;
    }
    found_ = insert = false;
    lastCount = transponderMap.size();
    //printf("DEBUG [channescan ]: set endofScan %s \n", endofScan ? "TRUE" : "FALSE");
}
