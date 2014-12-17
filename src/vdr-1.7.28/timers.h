/*
 * timers.h: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.h 2.4 2012/04/15 13:21:31 kls Exp $
 */

#ifndef __TIMERS_H
#define __TIMERS_H

#include "channels.h"
#include "config.h"
#include "epg.h"
#include "tools.h"
#ifdef USEMYSQL
#include <vector>
#endif /* USEMYSQL */

enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };
enum eTimerMatch { tmNone, tmPartial, tmFull };

class cTimer : public cListObject {
  friend class cMenuEditTimer;
#ifdef USEMYSQL
private:
  unsigned int ID;
  void UpdateTimerDB();
public:
  void SetID(unsigned int id) { ID=id; }
  unsigned int GetID(void) const { return ID; }
#endif /* USEMYSQL */
private:
  mutable time_t startTime, stopTime;
  time_t lastSetEvent;
  mutable time_t deferred; ///< Matches(time_t, ...) will return false if the current time is before this value
  bool recording, pending, inVpsMargin;
  uint flags;
  cChannel *channel;
  mutable time_t day; ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
  int weekdays;       ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  int start;
  int stop;
  int priority;
#ifdef USE_PINPLUGIN
  int fskProtection;
#endif /* PINPLUGIN */
  int lifetime;
  mutable char file[MaxFileName];
  char *aux;
  const cEvent *event;
public:
#ifdef USE_LIVEBUFFER
  cTimer(bool Instant = false, bool Pause = false, cChannel *Channel = NULL, int Forerun = 0);
#else
  cTimer(bool Instant = false, bool Pause = false, cChannel *Channel = NULL);
#endif /*USE_LIVEBUFFER*/
  cTimer(const cEvent *Event);
  cTimer(const cTimer &Timer);
  virtual ~cTimer();
  cTimer& operator= (const cTimer &Timer);
  virtual int Compare(const cListObject &ListObject) const;
  bool Recording(void) const { return recording; }
  bool Pending(void) const { return pending; }
  bool InVpsMargin(void) const { return inVpsMargin; }
  uint Flags(void) const { return flags; }
  const cChannel *Channel(void) const { return channel; }
#ifdef REELVDR
  void SetChannel(cChannel *chan) { channel = chan; }
  void ClearRecording() { recording = false; ClrFlags(tfRecording);}
#endif /* REELVDR */
  time_t Day(void) const { return day; }
  int WeekDays(void) const { return weekdays; }
  int Start(void) const { return start; }
  int Stop(void) const { return stop; }
  int Priority(void) const { return priority; }
#ifdef USE_PINPLUGIN
  int FskProtection(void) const { return fskProtection; }
#endif /* PINPLUGIN */
  int Lifetime(void) const { return lifetime; }
  const char *File(void) const { return file; }
  time_t FirstDay(void) const { return weekdays ? day : 0; }
  const char *Aux(void) const { return aux; }
  time_t Deferred(void) const { return deferred; }
  cString ToText(bool UseChannelID = false) const;
  cString ToDescr(void) const;
  const cEvent *Event(void) const { return event; }
#ifdef USEMYSQL
  bool Parse(const char *s, unsigned int id=0);
#else
  bool Parse(const char *s);
#endif /* USEMYSQL */
  bool Save(FILE *f);
  bool IsSingleEvent(void) const;
  static int GetMDay(time_t t);
  static int GetWDay(time_t t);
  bool DayMatches(time_t t) const;
  static time_t IncDay(time_t t, int Days);
  static time_t SetTime(time_t t, int SecondsFromMidnight);
  void SetFile(const char *File);
  bool Matches(time_t t = 0, bool Directly = false, int Margin = 0) const;
  int Matches(const cEvent *Event, int *Overlap = NULL) const;
  bool Expired(void) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  void SetEventFromSchedule(const cSchedules *Schedules = NULL);
  void SetEvent(const cEvent *Event);
  void SetRecording(bool Recording);
  void SetPending(bool Pending);
  void SetInVpsMargin(bool InVpsMargin);
  void SetDay(time_t Day);
  void SetWeekDays(int WeekDays);
  void SetStart(int Start);
  void SetStop(int Stop);
  void SetPriority(int Priority);
  void SetLifetime(int Lifetime);
  void SetAux(const char *Aux);
  void SetDeferred(int Seconds);
  void SetFlags(uint Flags);
#ifdef USE_PINPLUGIN
  void SetFskProtection(int aFlag);
#endif /* PINPLUGIN */
  void ClrFlags(uint Flags);
  void InvFlags(uint Flags);
  bool HasFlags(uint Flags) const;
  void Skip(void);
  void OnOff(void);
  cString PrintFirstDay(void) const;
  static int TimeToInt(int t);
  static bool ParseDay(const char *s, time_t &Day, int &WeekDays);
  static cString PrintDay(time_t Day, int WeekDays, bool SingleByteChars);
  };

class cTimers : public cConfig<cTimer> {
private:
  int state;
  int beingEdited;
  time_t lastSetEvents;
  time_t lastDeleteExpired;
#ifdef USEMYSQL
  int LastEventID;
#endif /* USEMYSQL */
public:
  cTimers(void);
#ifdef USEMYSQL
  cTimer *GetTimer(cTimer *Timer, bool CheckID=true);
#else
  cTimer *GetTimer(cTimer *Timer);
#endif
  cTimer *GetMatch(time_t t);
  cTimer *GetMatch(const cEvent *Event, int *Match = NULL);
  cTimer *GetNextActiveTimer(void);
  int BeingEdited(void) { return beingEdited; }
  void IncBeingEdited(void) { beingEdited++; }
  void DecBeingEdited(void) { if (!--beingEdited) lastSetEvents = 0; }
  void SetModified(void);
  bool Modified(int &State);
      ///< Returns true if any of the timers have been modified, which
      ///< is detected by State being different than the internal state.
      ///< Upon return the internal state will be stored in State.
  void SetEvents(void);
  void DeleteExpired(void);
#ifdef USEMYSQL
  int GetLastEventID() const { return LastEventID; };
  cTimer *GetTimerByID(unsigned int id);
  void GetInstantRecordings(std::vector<cTimer*> *InstantRecordings);
  bool LoadDB();
  void SyncData();
  bool Add(cTimer *Timer, cTimer *After = NULL);
  bool Ins(cTimer *Timer, cTimer *Before = NULL);
  bool Del(cTimer *Timer, bool DeleteObject = true);
#else
  void Add(cTimer *Timer, cTimer *After = NULL);
  void Ins(cTimer *Timer, cTimer *Before = NULL);
  void Del(cTimer *Timer, bool DeleteObject = true);
#endif /* USEMYSQL */
#if REELVDR
  void ClearAllRecordingFlags() {
      cTimer *ti = First();
      while (ti) {
          ti->ClearRecording();
          ti = Next(ti);
      }
  }
#endif
  };

extern cTimers Timers;

class cSortedTimers : public cVector<const cTimer *> {
public:
  cSortedTimers(void);
  };

#endif //__TIMERS_H
