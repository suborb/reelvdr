#include <vector>
#include <math.h>
#include <string>

#include "menu_epgtimers.h"
#include "menu_myedittimer.h"
#include "recstatus.h"
#include "uservars.h"
#include "menu_switchtimers.h"
#include <vdr/debug.h>


cMenuEpgTimers::cMenuEpgTimers(void)
    :cOsdMenu(tr("Timers"), 2, CHNUMWIDTH, 10, 6, 6), SwitchTimersLock(&SwitchTimers)
{

  if (strcmp(Skins.Current()->Name(), "Reel") == 0)
#if APIVERSNUM < 10700
      SetCols(3, 10, 6, 10, 6);
#else

#if REELVDR && APIVERSNUM >= 10718
      SetCols(3, 12, 7, 6);
#else
      SetCols(3, 12, 7, 12, 7);
#endif /* REELVDR && APIVERSNUM >= 10718 */

#endif
#if REELVDR && APIVERSNUM >= 10718
  EnableSideNote(true);
#endif
  helpKeys = -1;
  Set();
  SetCurrent(First());
  SetHelpKeys();

  /* get timer state */
  TimerState_=0;
  Timers.Modified(TimerState_);
#ifdef USEMYSQL
  LastEventID_ = -1;
  LastEventID_ = Timers.GetLastEventID();
#endif


}

cMenuEpgTimers::~cMenuEpgTimers()
{
}

void cMenuEpgTimers::Set(void)
{
  int current = Current();
  Clear();
  int i = 0;
  for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
      timer->SetEventFromSchedule(); // make sure the event is current
      ++i;
      //printf (" ## -- %d.) t: %s \n", i, *timer->ToDescr());
      Add(new cMenuTimerItem(timer));
      }
  Sort();
#if REELVDR

  cSwitchTimer* switchTimer = SwitchTimers.First();
  for (; switchTimer; switchTimer = SwitchTimers.Next(switchTimer)) {
      if (!switchTimer->event) continue;
      ++i;
      Add(new cMenuSwitchTimerItem(switchTimer));
  }

  if ( i == 0 ) // No timers present
  {
    // make these unselectable, selectables are only cMenuTimerItem's objects
    Add(new cOsdItem(tr("No timers present."),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false) );
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem((""),osUnknown, false));
    Add(new cOsdItem(tr("Info:"),osUnknown, false));
    AddFloatingText(tr("Use the green button to create a new timer."), MAXOSDTEXTWIDTH );
    SetTitle(tr("Timers"));
  }
  else
      SetTitle(cString::sprintf("%s ", tr("Timers")));
#endif
  //Sort();
  SetCurrent(Get(current));
  Display();
  lastSet = time(0);
}

cTimer *cMenuEpgTimers::CurrentTimer(void)
{
  cMenuTimerItem *item = dynamic_cast<cMenuTimerItem *>(Get(Current()));
  return item && item->Selectable() ? item->Timer() : NULL;
}

void cMenuEpgTimers::SetHelpKeys(void)
{
  int NewHelpKeys = 0;
  cTimer *timer = CurrentTimer();
  bool isCurrItemSwitchTimer = false;
  if (timer) {
     if (timer->Event())
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
  } else {
      cMenuSwitchTimerItem *item = dynamic_cast<cMenuSwitchTimerItem*>(Get(Current()));
      if (item) {
          NewHelpKeys = 1;
          isCurrItemSwitchTimer = true;
      }
  }

  if (NewHelpKeys != helpKeys) {
     helpKeys = NewHelpKeys;

    bool on = (timer && timer->HasFlags(tfActive)
               || isCurrItemSwitchTimer) ? true : false;
    static eKeys ikeys[] = { kInfo, kNone };

    /// Löschen | Neu | Ein/Aus | (frei) | Info
    if(on)
    {
        SetHelp(
            helpKeys > 0 ? tr("Button$Delete") : NULL,
            tr("Button$New"),
            (!isCurrItemSwitchTimer && helpKeys > 0) ? tr("Button$Off") : NULL,
            NULL //helpKeys == 2 ? tr("Button$Info") : NULL,
#if REELVDR && APIVERSNUM >= 10718
            , helpKeys == 2 ? ikeys : NULL
#endif
            );
    }
    else
    {
        SetHelp(
            helpKeys > 0 ? tr("Button$Delete") : NULL,
            tr("Button$New"),
            helpKeys > 0 ? tr("Button$On") : NULL,
            NULL //helpKeys == 2 ? tr("Button$Info") : NULL,
#if REELVDR && APIVERSNUM >= 10718
            , helpKeys == 2 ? ikeys : NULL
#endif
            );
    }
  }
}

eOSState cMenuEpgTimers::OnOff(void)
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
     //update helpkeys
     helpKeys = -1;
     }
  return osContinue;
}

eOSState cMenuEpgTimers::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;

  cMenuSwitchTimerItem *item = dynamic_cast<cMenuSwitchTimerItem*> (Get(Current()));
  if (item && item->switchTimer)
      return AddSubMenu(new cMenuMyEditTimer(item->switchTimer, false));

  cTimer *timer = CurrentTimer();

  if (!timer) return osContinue; // nothing to edit here

  isyslog("editing timer %s", *timer->ToDescr());
  return AddSubMenu(new cMenuMyEditTimer(timer, false, NULL));

}


eOSState cMenuEpgTimers::New(void)
{

  if (HasSubMenu())
     return osContinue;

  return AddSubMenu(new cMenuMyEditTimer(new cTimer, true, NULL));
  //return AddSubMenu(new cMenuEditTimer(new cTimer, true));
}

eOSState cMenuEpgTimers::Delete(void)
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
            Skins.Message(mtError, trVDR("Could not delete timer"));
        //cOsdMenu::Del(Current());
        Timers.SetModified();
        //Display();
        }
  } else {
      cMenuSwitchTimerItem *item = dynamic_cast<cMenuSwitchTimerItem*> (Get(Current()));
      if (item && item->switchTimer &&
              Interface->Confirm(tr("Delete switchtimer?"))) {
          cMutexLock SwitchTimersLock(&SwitchTimers);
          SwitchTimers.Del(item->switchTimer);
          SwitchTimers.Save();
          Del(Current());
      }
  }
  return osContinue;
}

eOSState cMenuEpgTimers::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cTimer *ti = CurrentTimer();
  if (ti && ti->Event())
     return AddSubMenu(new cMenuEvent(ti->Event()));
  return osContinue;
}

void cMenuEpgTimers::Display()
{
    cOsdMenu::Display();
    ShowEventDetailsInSideNote();
}

/* compute the directory in which the timer will be stored */
std::string TimerDirectory(const cTimer* timer)
{
    if (!timer) return "";

    char *file = strdup(timer->File());
    const cEvent *event = timer->Event();
    std::string dir;

    char *tmp = strrchr(file, '~'); // last dir
    printf("file: '%s'\ntmp:'%s'\n", file, tmp);

    if (tmp)
    {
#if 0
        if (event && !isempty(event->ShortText()))
        {
            cString eventFile = cString::sprintf("%s~%s", event->Title(), event->ShortText());

            char* tmp2 = strstr(file, eventFile);
            if (tmp2)
            {
                printf("title~episode tmp2:'%s'\n", tmp2);

                if (tmp2 > file)
                {
                    *(tmp2-1) = 0;
                    dir = file;
                    free(file);
                    return dir;
                } //if

            } // if
            else
            {
                *tmp = 0;
                dir = file;
                free(file);
                return dir;
            } // else

        } // if
        else
#endif
        {
            // every directory apart from the last directory
            *tmp = 0;
            dir = file;
            free(file);
            return dir;
        } // else

    } // if

    free(file);
    return dir;
}


void cMenuEpgTimers::ShowEventDetailsInSideNote() {

#if REELVDR && APIVERSNUM >= 10718

    if (!HasSubMenu()) {
        cTimer *currTimer = CurrentTimer();

        if (!currTimer) // check if it is switchtimer
        {
            cMenuSwitchTimerItem *item = dynamic_cast <cMenuSwitchTimerItem*> (Get(Current()));
            if (item &&  item->switchTimer)
                SideNote(item->switchTimer->event);
            else
                SideNote((const cEvent*)NULL);

            return;
        }

        printf("currentTimer %p Event? %p dir:'%s'\n", currTimer,
               currTimer?(currTimer->Event()):NULL, TimerDirectory(currTimer).c_str());

        std::string subDir = TimerDirectory(currTimer);
        cStringList sList;
        if (subDir.size() > 0) {

            std::string directory = tr("Directory");
            directory += ":";
            sList.Append(strdup(directory.c_str()));
            sList.Append(strdup(subDir.c_str()));
            SideNote(&sList);
        } // if

        const cEvent *event =  currTimer?currTimer->Event():NULL;
        SideNote(event, &sList);
    } // if !HasSubMenu()

#endif /* REELVDR && APIVERSNUM >= 10718 */
}

eOSState cMenuEpgTimers::ProcessKey(eKeys Key)
{
  //int TimerNumber = HasSubMenu() ? Count() : -1;

  eOSState state = cOsdMenu::ProcessKey(Key);

#if REELVDR && APIVERSNUM >= 10718
  if (Key != kNone && state != osUnknown)
      ShowEventDetailsInSideNote();
#endif

  if (state == osUnknown) {
     switch (Key) {
       /// Löschen | Neu | Ein/Aus | (frei) | Info
       case kOk:     Timers.Count() ? state = Edit() : state = New(); break;
       case kRed:    state = Delete(); break; // must go through SetHelpKeys()!
       case kGreen:  return New(); break;
       case kYellow: state = OnOff(); break;
       case kBlue:   return osContinue; break;
       case kInfo:   return Info(); break;
       default: break;
       }
     }
/* redraw osd *ONLY* if Timers were modified and it is not a submenu, not with everykey stroke! */
#if 0
#ifdef USEMYSQL
if (!HasSubMenu() && (Timers.Modified(TimerState_) || (Timers.GetLastEventID() != LastEventID_)))
#else
if (!HasSubMenu() && (Timers.Modified(TimerState_) || (lastSet != time(0))))
#endif
#else
// We want to see changes in timers flag so we need to poll...
if (!HasSubMenu() && (lastSet != time(0)))
#endif
{
	Set();
	SetHelpKeys();
	Display();
#ifdef USEMYSQL
    LastEventID_ = Timers.GetLastEventID();
#endif
}
else if (!HasSubMenu())
    switch(Key) // update the info buttons
    {
        default: break;
        case kUp:
        case kUp|k_Repeat:
        case kDown:
        case kDown|k_Repeat:
        case kRight:
        case kRight|k_Repeat:
        case kLeft:
        case kLeft|k_Repeat:
            //activity of current timer may have chanched
            helpKeys = -1;
            SetHelpKeys();
            break;
    }
/*
  if (TimerNumber != -1 && TimerNumber != Timers.Count()) {
     Set();
     SetHelpKeys();
     Display();
  } else if (Key != kNone && !HasSubMenu()) {
     Set();
     SetHelpKeys();
     //Display();
  }
*/

  return state;
}
