


#ifndef __EPG_TIMERS_H
#define __EPG_TIMERS_H

#include <vdr/menuitems.h>

class cMenuTimerItem : public cOsdItem {
private:
  cTimer *timer;
public:
  cMenuTimerItem(cTimer *Timer);
  virtual int Compare(const cListObject &ListObject) const;
  virtual void Set(void);
  cTimer *Timer(void) { return timer; }
  };


class cMenuEpgTimers : public cOsdMenu {
private:
#ifdef USEMYSQL
  int LastEventID_;
#endif
  int TimerState_;
  int helpKeys;
  time_t lastSet;
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  eOSState OnOff(void);
  eOSState Info(void);
  cTimer *CurrentTimer(void);
  void SetHelpKeys(void);
#if REELVDR
  cMutexLock SwitchTimersLock;
#endif
public:
  void Set();
  cMenuEpgTimers(void);
  virtual ~cMenuEpgTimers();
  virtual eOSState ProcessKey(eKeys Key);

  virtual void Display();
  void ShowEventDetailsInSideNote();
  };

#endif //__EPG_TIMERS_H

