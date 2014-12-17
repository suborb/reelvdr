#ifndef CFILTER_H
#define CFILTER_H

#include <vdr/filter.h>
#include <stdint.h>
#include "menu.h"

#define MAXPMTENTRIES 64
#define MAXFILTERS 10
#define FILTERTIMEOUT 10

class SdtFilter;

class cMenuScan;

class PatFilter : public cFilter {
private:
  int pSid[100];
  time_t lastPmtScan[MAXFILTERS];
  int pmtIndex;
  int pmtPid[MAXFILTERS];
  int Sids[MAXFILTERS];
  uint64_t pmtVersion[MAXPMTENTRIES];
  int numPmtEntries;
  bool PmtVersionChanged(int PmtPid, int Sid, int Version);
  int num,pit,pnum;
  SdtFilter *sdtFilter;
  bool endofScan;
  bool SidinSdt(int Sid);
  cMenuScan* Menu;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
public:
  PatFilter(cMenuScan* MENU);
  void SetSdtFilter(SdtFilter* SdtFilter);
  virtual void SetStatus(bool On);
  bool EndOfScan() {return endofScan;};
  void Trigger(void);
  };

int GetCaDescriptors(int Source, int Transponder, int ServiceId, const unsigned short *CaSystemIds, int BufSize, uchar *Data, bool &StreamFlag);

class SdtFilter : public cFilter {
friend class PatFilter;
private:
  int numSid,sid[100];
  cSectionSyncer sectionSyncer;
  PatFilter *patFilter;
  cMenuScan* Menu;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
public:
  SdtFilter(PatFilter *PatFilter,cMenuScan* MENU);
  virtual void SetStatus(bool On);
  };

#endif
