/*
 * eitscan.c: EIT scanner
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: eitscan.c 2.4 2011/08/26 16:16:46 kls Exp $
 */

#include "eitscan.h"
#include <stdlib.h>
#include "channels.h"
#include "dvbdevice.h"
#include "skins.h"
#include "transfer.h"

// --- cScanData -------------------------------------------------------------

class cScanData : public cListObject {
private:
  cChannel channel;
public:
  cScanData(const cChannel *Channel);
  virtual int Compare(const cListObject &ListObject) const;
  int Source(void) const { return channel.Source(); }
  int Transponder(void) const { return channel.Transponder(); }
  const cChannel *GetChannel(void) const { return &channel; }
  };

cScanData::cScanData(const cChannel *Channel)
{
  channel = *Channel;
}

int cScanData::Compare(const cListObject &ListObject) const
{
  const cScanData *sd = (const cScanData *)&ListObject;
  int r = Source() - sd->Source();
  if (r == 0)
     r = Transponder() - sd->Transponder();
  return r;
}

// --- cScanList -------------------------------------------------------------

class cScanList : public cList<cScanData> {
public:
  void AddTransponders(cList<cChannel> *Channels);
  void AddTransponder(const cChannel *Channel);
  };

void cScanList::AddTransponders(cList<cChannel> *Channels)
{
#ifdef REELVDR
  for (cChannel *ch = Channels->First(); ch; ch = Channels->Next(ch))
      if(!Setup.EPGScanMaxChannel || (ch->Number() <= Setup.EPGScanMaxChannel))
          AddTransponder(ch);
#else
  for (cChannel *ch = Channels->First(); ch; ch = Channels->Next(ch))
      AddTransponder(ch);
  Sort();
#endif
}

void cScanList::AddTransponder(const cChannel *Channel)
{
  if (Channel->Source() && Channel->Transponder()) {
     for (cScanData *sd = First(); sd; sd = Next(sd)) {
         if (sd->Source() == Channel->Source() && ISTRANSPONDER(sd->Transponder(), Channel->Transponder()))
            return;
         }
     Add(new cScanData(Channel));
     }
}

// --- cTransponderList ------------------------------------------------------

class cTransponderList : public cList<cChannel> {
public:
  void AddTransponder(cChannel *Channel);
  };

void cTransponderList::AddTransponder(cChannel *Channel)
{
  for (cChannel *ch = First(); ch; ch = Next(ch)) {
      if (ch->Source() == Channel->Source() && ch->Transponder() == Channel->Transponder()) {
         delete Channel;
         return;
         }
      }
  Add(Channel);
}

// --- cEITScanner -----------------------------------------------------------

cEITScanner EITScanner;

cEITScanner::cEITScanner(void)
{
  lastScan = lastActivity = time(NULL);
  currentDevice = NULL;
  currentChannel = 0;
  scanList = NULL;
  transponderList = NULL;
#ifdef REELVDR
  dailyScanActive = false;
#endif
}

cEITScanner::~cEITScanner()
{
  delete scanList;
  delete transponderList;
}

void cEITScanner::AddTransponder(cChannel *Channel)
{
  if (!transponderList)
     transponderList = new cTransponderList;
  transponderList->AddTransponder(Channel);
}

void cEITScanner::ForceScan(void)
{
  lastActivity = 0;
#ifdef REELVDR
  if (Channels.Lock(false, 10)) { // make sure ::Process is not running
     if (scanList) // begin with a fresh list
        delete scanList;
     scanList = NULL;
     Channels.Unlock();
  } // if
#endif
}

void cEITScanner::Activity(void)
{
  if (currentChannel) {
     Channels.SwitchTo(currentChannel);
     currentChannel = 0;
     }
#ifdef REELVDR
  // Only clear lastActivity if not forced since Activity is also called if a key is pressed
  if (lastActivity) lastActivity = time(NULL);
#else
  lastActivity = time(NULL);
#endif
}

void cEITScanner::Process(void)
{
#ifdef REELVDR
  #define EPGSacnModeText()                                                                  \
      (Setup.EPGScanMode == 0) ? (lastActivity ? "Disabled!":"Forced by user") :             \
      (Setup.EPGScanMode == 1) ? (lastActivity ? "Permanent":"Permanent (forced by user)") : \
      (Setup.EPGScanMode == 2) ? (lastActivity ? "Daily"    :"Daily (forced by user") :      \
      "Unknown scan mode"
  #define ReportEPGScanDailyNext(text) {           \
      struct tm lt;                                \
      char ltc[32];                                \
      time_t ltt = (time_t)Setup.EPGScanDailyNext; \
      localtime_r(&ltt, &lt);                      \
      asctime_r(&lt, ltc);                         \
      isyslog(text, ltc);                          \
  }
  if(Setup.EPGScanMode == 2) {
     time_t now = time(NULL);
     if(!Setup.EPGScanDailyNext || (Setup.EPGScanDailyNext > now+25*60*60)) {
        struct tm tm_r;
        if(localtime_r(&now, &tm_r)) {
           tm_r.tm_hour = Setup.EPGScanDailyTime / 100;
           tm_r.tm_min  = Setup.EPGScanDailyTime % 100;
           tm_r.tm_sec  = 0;
           Setup.EPGScanDailyNext =  mktime(&tm_r);
           if(Setup.EPGScanDailyNext <= now)
              Setup.EPGScanDailyNext += 24*60*60;
           ReportEPGScanDailyNext("Set next daily EPG scan time to %s");
        } // if
     } // if
     if(Setup.EPGScanDailyNext <= now) {
        Setup.EPGScanDailyNext += 24*60*60;
        ReportEPGScanDailyNext("Daily EPG scan triggered. Set next scan time to %s");
        dailyScanActive = true;
        if (scanList) // begin with a fresh list
           delete scanList;
        scanList = NULL;
     } // if
  } // if
  if ((Setup.EPGScanMode == 1) || dailyScanActive || !lastActivity) { // !lastActivity means a scan was forced
     if(!Setup.EPGScanTimeout) // Just make sure a value is set
        Setup.EPGScanTimeout = 4;
#else
  if ( Setup.EPGScanTimeout || !lastActivity) { // !lastActivity means a scan was forced
#endif
     time_t now = time(NULL);
     if (now - lastScan > ScanTimeout && now - lastActivity > ActivityTimeout) {
        if (Channels.Lock(false, 10)) {
           if (!scanList) {
              scanList = new cScanList;
              if (transponderList) {
                 scanList->AddTransponders(transponderList);
                 delete transponderList;
                 transponderList = NULL;
                 }
              scanList->AddTransponders(&Channels);
              }
           bool AnyDeviceSwitched = false;
#ifdef REELVDR
           int scan_count = 0;
           int scan_max   = (Setup.EPGScanMaxDevices > 0) ? Setup.EPGScanMaxDevices : MAXDEVICES;
           int busy_count = 0;
           int busy_max   = (Setup.EPGScanMaxBusyDevices > 0) ? Setup.EPGScanMaxBusyDevices : MAXDEVICES;
           for (int i = 0; (i < cDevice::NumDevices()) && (scan_count < scan_max) && (busy_count < busy_max); i++) {
               cDevice *Device = cDevice::GetDevice(i);
               if (Device && Device->ProvidesEIT() && Device->Receiving())
                   busy_count++;
           }
           for (int i = 0; (i < cDevice::NumDevices()) && (scan_count < scan_max) && (busy_count < busy_max); i++) {
               cDevice *Device = cDevice::GetDevice(i);
               if (Device && Device->ProvidesEIT()) {
                  for (cScanData *ScanData = scanList->First(); ScanData; ScanData = scanList->Next(ScanData)) {
                      const cChannel *ScanChannel = ScanData->GetChannel();
                      cChannel *Channel = new cChannel();
                      if (ScanChannel && Channel) {
                         Channel->CopyTransponderData(ScanChannel); // give mcli a chance to detect scan
#else
           for (int i = 0; i < cDevice::NumDevices(); i++) {
               cDevice *Device = cDevice::GetDevice(i);
               if (Device && Device->ProvidesEIT()) {
                  for (cScanData *ScanData = scanList->First(); ScanData; ScanData = scanList->Next(ScanData)) {
                      const cChannel *Channel = ScanData->GetChannel();
                      if (Channel) {
#endif
                         if (!Channel->Ca() || Channel->Ca() == Device->DeviceNumber() + 1 || Channel->Ca() >= CA_ENCRYPTED_MIN) {
                            if (Device->ProvidesTransponder(Channel)) {
                               if (!Device->Receiving()) {
                                  bool MaySwitchTransponder = Device->MaySwitchTransponder();
#ifdef USE_LNBSHARE
                                  if (MaySwitchTransponder && Device->GetMaxBadPriority(Channel) == -2 || (MaySwitchTransponder || Device->ProvidesTransponderExclusively(Channel)) && Device->GetMaxBadPriority(Channel) <= -1 && now - lastActivity > Setup.EPGScanTimeout * 3600) {
#else
                                  if (MaySwitchTransponder || Device->ProvidesTransponderExclusively(Channel) && now - lastActivity > Setup.EPGScanTimeout * 3600) {
#endif /* LNBSHARE */
                                     if (!MaySwitchTransponder) {
#ifdef USE_LNBSHARE
                                        if ((Device == cDevice::ActualDevice() || Device->GetMaxBadPriority(Channel) == -1) && !currentChannel) {
#else
                                        if (Device == cDevice::ActualDevice() && !currentChannel) {
#endif /* LNBSHARE */
                                           cDevice::PrimaryDevice()->StopReplay(); // stop transfer mode
                                           currentChannel = Device->CurrentChannel();
                                           Skins.Message(mtInfo, tr("Starting EPG scan"));
                                           }
                                        }
                                     currentDevice = Device;//XXX see also dvbdevice.c!!!
                                     //dsyslog("EIT scan: device %d  source  %-8s tp %5d", Device->DeviceNumber() + 1, *cSource::ToString(Channel->Source()), Channel->Transponder());
#ifdef REELVDR
                                     busy_count++;
                                     scan_count++;
                                     isyslog("EPG scan: scanning  %-8s on device %2d tp %5d remaining %d [%d/%d] (%d/%d) %s", *cSource::ToString(Channel->Source()), Device ? Device->DeviceNumber() + 1 : 0, Channel->Transponder(), scanList->Count(), scan_count, scan_max, busy_count, busy_max, EPGSacnModeText());
#endif
                                     Device->SwitchChannel(Channel, false);
                                     currentDevice = NULL;
                                     scanList->Del(ScanData);
                                     AnyDeviceSwitched = true;
                                     break;
                                     }
                                  }
                               }
                            }
#ifndef REELVDR
                         }
#else
                         }
                         if(Channel) delete Channel;
#endif
                      }
                  }
               }
#ifdef REELVDR
           if(busy_count >= busy_max) {
               isyslog("EPG scan: not scanning due to EPGScanMaxBusyDevices limits. remaining %d [%d/%d] (%d/%d) %s", scanList->Count(), scan_count, scan_max, busy_count, busy_max, EPGSacnModeText());
               AnyDeviceSwitched = true; // Don´t create new scan list if we are out of limits
           }
#endif
           if (!scanList->Count() || !AnyDeviceSwitched) {
              delete scanList;
              scanList = NULL;
#ifdef REELVDR
              dailyScanActive = false;
              if (lastActivity == 0) {// this was a triggered scan
                 // Set lastActivity to != 0 to signal end of forced scan
                 lastActivity = 1;
                 Activity();
              }
#else
              if (lastActivity == 0) // this was a triggered scan
                 Activity();
#endif
              }
           Channels.Unlock();
           }
        lastScan = time(NULL);
        }
     }
}
