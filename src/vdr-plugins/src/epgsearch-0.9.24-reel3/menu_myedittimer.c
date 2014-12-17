/*
Copyright (C) 2004-2008 Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The author can be reached at cwieninger@gmx.de

The project's page is at http://winni.vdr-developer.org/epgsearch
*/

#include <vector>
#include "menu_myedittimer.h"
#include "epgsearchcfg.h"
#include "epgsearchcats.h"
#include "timer_thread.h"
#include "menu_dirselect.h"
#include "menu_searchedit.h"
#include "epgsearchtools.h"
#include "recstatus.h"
#include "uservars.h"
#include "menu_deftimercheckmethod.h"
#include "timerstatus.h"
#include <math.h>
#include "switchtimer.h"
#include "switchtimer_thread.h"
#include "menuitem_event.h"

// --- cMyMenuEditDateItem -----------------------------------------------------

class cMyMenuEditDateItem : public cMenuEditItem {
private:
  static int days[];
  time_t *value;
  int *weekdays;
  time_t oldvalue;
  int dayindex;
  int FindDayIndex(int WeekDays);
  virtual void Set(void);
public:
  cMyMenuEditDateItem(const char *Name, time_t *Value, int *WeekDays = NULL);
  virtual eOSState ProcessKey(eKeys Key);
  };


static int ParseWeekDays(const char *s)
{
  time_t day;
  int weekdays;
  return cTimer::ParseDay(s, day, weekdays) ? weekdays : 0;
}

int cMyMenuEditDateItem::days[] = { ParseWeekDays("M------"),
                                  ParseWeekDays("-T-----"),
                                  ParseWeekDays("--W----"),
                                  ParseWeekDays("---T---"),
                                  ParseWeekDays("----F--"),
                                  ParseWeekDays("-----S-"),
                                  ParseWeekDays("------S"),
                                  ParseWeekDays("MTWTF--"),
                                  ParseWeekDays("MTWTFS-"),
                                  ParseWeekDays("MTWTFSS"),
                                  ParseWeekDays("-----SS"),
                                  0 };

cMyMenuEditDateItem::cMyMenuEditDateItem(const char *Name, time_t *Value, int *WeekDays)
:cMenuEditItem(Name)
{
  value = Value;
  weekdays = WeekDays;
  oldvalue = 0;
  dayindex = weekdays ? FindDayIndex(*weekdays) : 0;
  Set();
}

int cMyMenuEditDateItem::FindDayIndex(int WeekDays)
{
  for (unsigned int i = 0; i < sizeof(days) / sizeof(int); i++)
      if (WeekDays == days[i])
         return i;
  return 0;
}

void cMyMenuEditDateItem::Set(void)
{
#define DATEBUFFERSIZE 32
  char buf[DATEBUFFERSIZE];
  if (weekdays && *weekdays) {
     SetValue(cTimer::PrintDay(0, *weekdays, true));
     return;
     }
  else if (*value) {
     struct tm tm_r;
     localtime_r(value, &tm_r);
     strftime(buf, DATEBUFFERSIZE, "%Y-%m-%d ", &tm_r);
     strcat(buf, WeekDayName(tm_r.tm_wday));
     }
  else
     *buf = 0;
  SetValue(buf);
}

eOSState cMyMenuEditDateItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     time_t now = time(NULL);
     if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (*value && (!weekdays || !*weekdays)) {
           // Decrement single day:
           time_t v = *value;
           v -= SECSINDAY;
           if (v < now) {
              if (now <= v + SECSINDAY) { // switched from tomorrow to today
#if 0 //TB: this causes the weekday to be empty when switching from tomorrow to today - what is it for?
                 if (!weekdays)
                    v = 0;
#endif
                 }
              else if (weekdays) { // switched from today to yesterday, so enter weekdays mode
                 v = 0;
                 dayindex = sizeof(days) / sizeof(int) - 2;
                 *weekdays = days[dayindex];
                 }
              else // don't go before today
                 v = *value;
              }
           *value = v;
           }
        else {
           // Decrement weekday index:
           if (dayindex > 0)
              *weekdays = days[--dayindex];
            else if (weekdays)// cyclic
                {
                 dayindex = sizeof(days) / sizeof(int) - 2;
                 *weekdays = days[dayindex];
                }
           }
        }
     else if (NORMALKEY(Key) == kRight) {
        if (!weekdays || !*weekdays) {
           // Increment single day:
           if (!*value)
              *value = cTimer::SetTime(now, 0);
           *value += SECSINDAY;
           }
        else {
           // Increment weekday index:
           *weekdays = days[++dayindex];
           if (!*weekdays) { // was last weekday entry, so switch to today
               if (*value)
               {
                   *value = cTimer::SetTime(now, 0);
                   dayindex = 0;
               }
               else { *weekdays = 1; dayindex = 0; } // jump to first entry if "day" was 0
              }
           }
        }
     else if (weekdays) {
        if (*value && Key == k0) { // only if *value was give toggle
           // Toggle between weekdays and single day:
           if (*weekdays) {
              *value = cTimer::SetTime(oldvalue ? oldvalue : now, 0);
              oldvalue = 0;
              *weekdays = 0;
              }
           else {
              *weekdays = days[cTimer::GetWDay(*value)];
              dayindex = FindDayIndex(*weekdays);
              oldvalue = *value;
              *value = 0;
              }
           }
        else if (k1 <= Key && Key <= k7) {
           // Toggle individual weekdays:
           if (*weekdays) {
              int v = *weekdays ^ (1 << (Key - k1));
              if (v != 0)
                 *weekdays = v; // can't let this become all 0
              }
           }
        else
           return state;
        }
     else
        return state;
     Set();
     state = osContinue;
     }
  return state;
}





const char *cMenuMyEditTimer::CheckModes[3];

#if REELVDR
cMenuMyEditTimer::cMenuMyEditTimer(cSwitchTimer *SwitchTimer, bool New)
    : cOsdMenu(New?tr("New timer"):trVDR("Edit timer"), 14),
      switchTimer(SwitchTimer),
      addIfConfirmed(New)
{
    isSwitchTimer = true;
    timer         = NULL;
    file[0]       = 0;
    directory[0]  = 0;

    channel = cDevice::PrimaryDevice()->CurrentChannel(); // start with current channel
    if (switchTimer && switchTimer->event) {
        cChannel *ch = Channels.GetByChannelID(switchTimer->event->ChannelID());
        if (ch)
            channel = ch->Number();
    }

    Set();

    SetHelp(addIfConfirmed ? NULL : trVDR("Button$Delete"));
    SetStatus(tr("Please confirm changes with OK"));
}
#endif /* REELVDR */

cMenuMyEditTimer::cMenuMyEditTimer(cTimer *Timer, bool New, const cEvent* Event, const cChannel* forcechannel, bool *timerCreated)
    :cOsdMenu(New?tr("New timer"):trVDR("Edit timer"), 14)
{
    CheckModes[0] = tr("no check");
    CheckModes[UPD_CHDUR] = tr("by channel and time");
    CheckModes[UPD_EVENTID] = tr("by event ID");
#if REELVDR
    isSwitchTimer = false;
    switchTimer = NULL;
#endif
#if REELVDR && APIVERSNUM >= 10718
    EnableSideNote(true);
#endif

    strcpy(file, "");
    strcpy(directory, "");
    UserDefDaysOfWeek = 0;
    checkmode = 0;
    if (Timer)
    {
	timer = Timer;
	event = Event;
	flags = Timer->Flags();
    flags |= tfActive;
	day = Timer->Day();
	weekdays = Timer->WeekDays();
    timerCreated_ = timerCreated;

    recurrent = (weekdays != 0);
	start = Timer->Start();
	stop = Timer->Stop();
	priority = Timer->Priority();
	lifetime = Timer->Lifetime();
	strcpy(file, Timer->File());
	channel = Timer->Channel()->Number();
#ifdef USE_PINPLUGIN
    fskProtection = Timer->FskProtection();
#endif
	if (forcechannel)
	    channel = forcechannel->Number();
	SplitFile();

	addIfConfirmed = New;
	Set();
	SetHelp(addIfConfirmed ? NULL : trVDR("Button$Delete"), NULL, NULL, trVDR("Button$Folder"));
    SetStatus(tr("Please confirm changes with OK"));
    }
}

void cMenuMyEditTimer::SplitFile()
{
    char* tmp = strrchr(file, '~');
    if (tmp) // split file in real file and directory
    {
	if (event && !isempty(event->ShortText()))
	{
      cString eventFile = cString::sprintf("%s~%s", event->Title(), event->ShortText());
	    char* tmp2 = strstr(file, eventFile);
	    if (tmp2) // file contains title and subtitle
	    {
		if (tmp2 > file)
		{
		    *(tmp2-1) = 0;
		    strcpy(directory, file);
		    strcpy(file, tmp2);
		}
		else
		    *directory = 0;
	    }
	    else
	    {
		*tmp = 0;
		strcpy(directory, file);
		strcpy(file, tmp+1);
	    }
	}
	else
	{
	    *tmp = 0;
	    strcpy(directory, file);
	    strcpy(file, tmp+1);
	}
    }
    else
	*directory = 0;
}

#ifdef REELVDR
void cMenuMyEditTimer::SetSwitchTimer()
{
    //isSwitchTimer=true here

    cChannel *ch = Channels.GetByNumber(channel);
    if (!ch) {
        Add(new cOsdItem(tr("Channel not in list. Please choose another."), osUnknown, false));
        return;
    }


#define NO_EPG_MESG  AddFloatingText(tr("\nNo epg data available for this channel.\nSwitch to channel to get epg data."), 50)

#define NO_EPG_MESG_RETURN  do { NO_EPG_MESG; return; } while(0)

    // List all events in the channel for the user to select one

    cSchedulesLock SchedulesLock;
    const cSchedules *schedules = cSchedules::Schedules(SchedulesLock);
    if (!schedules)  NO_EPG_MESG_RETURN;

    const cSchedule *schedule = schedules->GetSchedule(ch);
    if (!schedule) NO_EPG_MESG_RETURN;

    const cList<cEvent>* events = schedule->Events();
    if (!events) NO_EPG_MESG_RETURN;

    const cEvent *e = events->First();

    // blank lines
    if (e) {
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Please select a programme for switchtimer:"), osUnknown, false));
    } else
        NO_EPG_MESG_RETURN;

    time_t now = time(NULL);
    // show epg data, starting from next programme (after 'now')
    for ( ; e ; e = events->Next(e)) {
        if (now <= e->StartTime())
            Add(new cMenuEventItem(e));
    }
}
#endif /*REELVDR*/


void cMenuMyEditTimer::Set()
{
    SetCols(22, 6);
    int current = Current();
    PriorityTexts[0] = tr("low");
    PriorityTexts[1] = tr("normal");
    PriorityTexts[2] = tr("high");
    Clear();

    if (!isempty(file))
    {
        std::string title = string(trVDR("Edit timer")) + ": " + file;
        SetTitle(title.c_str());
    }

    cSearchExt* search = TriggeredFromSearchTimer(timer);
    tmpprio = priority == 10 ? 0 : priority == 99 ? 2 : 1;
#ifndef REELVDR
    Add(new cMenuEditBitItem( trVDR("Active"),       &flags, tfActive));
#endif

#if REELVDR
    cOsdItem *item = NULL;
    Add(item = new cMenuEditBoolItem(tr("Timer type"), &isSwitchTimer, tr("Recording"), tr("SwitchTimer")));
    if (item && !addIfConfirmed)    // editing already existing timer
        item->SetSelectable(false); // then do not offer to change timer type
#endif

//#ifdef USE_PINPLUGIN
#if 0 // disregard PIN for REELVDR
    //if (cOsd::pinValid) Add(new cMenuEditChanItem(trVDR("Channel"), &channel)); //TB: changing the channel is only possible when pin is activated?!
    cSetupLine *setupLine;
    bool pinDefined = false;
    if ((setupLine = Setup.Get("pinCode", "pin")))
        pinDefined = strlen(setupLine->Value()) && strcmp(setupLine->Value(), "0000");
    if (cOsd::pinValid || !pinDefined) Add(new cMenuEditChanItem(trVDR("Channel"), &channel));
    else {
      cString buf = cString::sprintf("%s:\t%i %s", tr("Channel"), Channels.GetByNumber(channel)->Number(), Channels.GetByNumber(channel)->Name());
      Add(new cOsdItem(buf));
    }
#else
    Add(new cMenuEditChanItem(trVDR("Channel"),      &channel));
#endif
#if REELVDR
    if(isSwitchTimer) {
        SetSwitchTimer();
        SetCurrent(Get(current));
        return;
    }
#endif

#if REELVDR
    if (IsSingleEvent())
    {
	weekdays  = 0;
	Add(new cMyMenuEditDateItem(trVDR("Date"),          &day));
    }
    else
        Add(new cMyMenuEditDateItem(tr("Next date"), &day));  //only days
#else
    Add(new cMenuEditBoolItem ( tr("Repeat"), &recurrent));
    if (IsSingleEvent())
    {
	    weekdays  = 0;
	    Add(new cMyMenuEditDateItem(trVDR("Date"),          &day));
    }
    else
    {
        if ( !weekdays ) weekdays = 1;
        static time_t day1 = 0;
        Add(new cMyMenuEditDateItem(tr("  Day of Week"),        &day1, &weekdays)); // only weekdays
        Add(new cMyMenuEditDateItem(tr("First day"), &day));  //only days
    }
#endif

    Add(new cMenuEditTimeItem(trVDR("Start"),        &start));
    Add(new cMenuEditTimeItem(trVDR("Stop"),         &stop));
    Add(new cMenuEditBitItem( trVDR("VPS"),          &flags, tfVps));
    Add(new cMenuEditStraItem(trVDR("Priority"),     &tmpprio, 3, PriorityTexts));
#ifndef REELVDR
    Add(new cMenuEditIntItem( trVDR("Lifetime"),     &lifetime, 0, MAXLIFETIME, NULL, tr("unlimited")));
#endif
#ifdef USE_PINPLUGIN
    if (cOsd::pinValid || !fskProtection) Add(new cMenuEditBoolItem(tr("Child protection"),&fskProtection));
    else {
      cString buf = cString::sprintf("%s\t%s", tr("Child protection"), fskProtection ? trVDR("yes") : trVDR("no"));
      Add(new cOsdItem(buf));
    }
#endif
#ifdef REELVDR
    Add(new cMenuEditBoolItem ( tr("Repeat"), &recurrent));
    if (!IsSingleEvent())
    {
        if ( !weekdays ) weekdays = 1;
        static time_t day1 = 0;
        Add(new cMyMenuEditDateItem(tr("  Day of Week"),        &day1, &weekdays)); // only weekdays
        Add(new cMyMenuEditDateItem(tr("First day"), &day));  //only days
    }
#endif

    //Add(new cMenuEditStrItem( trVDR("File"), file, MaxFileName, trVDR(FileNameChars)));
    Add(new cMenuEditStrItem( tr("Directory"), directory, MaxFileName, NULL, tr("[Standard]")));

    if (search)
    {
	cMenuEditStrItem* searchtimerItem = new cMenuEditStrItem( tr("Search timer"), search->search, MaxFileName);
	searchtimerItem->SetSelectable(false);
	Add(searchtimerItem);
    }
    else if (IsSingleEvent() && event)
    {
	checkmode = DefTimerCheckModes.GetMode(Channels.GetByNumber(channel));
	char* checkmodeAux = GetAuxValue(timer, "update");
	if (checkmodeAux)
	{
	    checkmode = atoi(checkmodeAux);
	    free(checkmodeAux);
	}
	Add(new cMenuEditStraItem(tr("Timer check"), &checkmode, 3, CheckModes));
    }

    int deviceNr = gl_recStatusMonitor->TimerRecDevice(timer);
    if (deviceNr > 0)
    {
	cOsdItem* pInfoItem = new cOsdItem("");
	pInfoItem->SetSelectable(false);
	Add(pInfoItem);
#ifndef REELVDR
	cString info = cString::sprintf("%s: %d/%d", tr("recording with device"), deviceNr, cDevice::NumDevices());
	pInfoItem = new cOsdItem(info);
	pInfoItem->SetSelectable(false);
	Add(pInfoItem);
#endif
    }

    if (current > -1)
    {
	SetCurrent(Get(current));
        RefreshCurrent();
    }
    Display();
}

cMenuMyEditTimer::~cMenuMyEditTimer()
{
  if (timer && addIfConfirmed)
     delete timer; // apparently it wasn't confirmed
}

void cMenuMyEditTimer::HandleSubtitle()
{
    const char* ItemText = Get(Current())->Text();
    if (strstr(ItemText, tr("File")) != ItemText)
	return;
    if (InEditMode(ItemText, tr("File"), file))
	return;

    if (!event || (event && !event->ShortText()))
	return;
    char* tmp = strchr(file, '~');
    if (tmp)
    {
	*tmp = 0;
	SetHelp(addIfConfirmed?NULL:trVDR("Button$Delete"), NULL, trVDR("Button$Reset"), tr("Button$With subtitle"));
    }
    else
    {
	strcat(file, "~");
	strcat(file, event->ShortText());
	SetHelp(addIfConfirmed?NULL:trVDR("Button$Delete"), NULL, trVDR("Button$Reset"), tr("Button$Without subtitle"));
    }
    RefreshCurrent();
    Display();
}

bool cMenuMyEditTimer::IsSingleEvent(void) const
{
  //return !weekdays;
  return !recurrent;
}

eOSState cMenuMyEditTimer::DeleteTimer()
{
    // Check if this timer is active:
    if (timer) {
        if (Interface->Confirm(trVDR("Delete timer?"))) {
            if (timer->Recording()) {
                if (Interface->Confirm(trVDR("Timer still recording - really delete?"))) {
                    timer->Skip();
                    cRecordControls::Process(time(NULL));
                }
                else
                    return osContinue;
            }
            LogFile.iSysLog("deleting timer %s", *timer->ToDescr());
            if(!Timers.Del(timer, addIfConfirmed)) // Don't destroy object -> it's needed by parrent menu to redraw item
                Skins.Message(mtError, trVDR("Could not delete timer"));
            timer = NULL;
            gl_timerStatusMonitor->SetConflictCheckAdvised();
            cOsdMenu::Del(Current());
            Timers.SetModified();
            Display();
            return osBack;
        }
    }
    return osContinue;
}

char* cMenuMyEditTimer::SetFskProtection(int fskProtection, char* aux)                 // PIN PATCH
{
   char* p;
   char* tmp = 0;

   if (fskProtection && (!aux || !strstr(aux, "<pin-plugin><protected>yes</protected></pin-plugin>")))
   {
      // add protection info to aux
      if (aux) { tmp = strdup(aux); free(aux); }
      asprintf(&aux,"%s<pin-plugin><protected>yes</protected></pin-plugin>", tmp ? tmp : "");
   }
   else if (!fskProtection && aux && (p = strstr(aux, "<pin-plugin><protected>yes</protected></pin-plugin>")))
   {
      // remove protection info to aux
      asprintf(&tmp, "%.*s%s", p-aux, aux, p+strlen("<pin-plugin><protected>yes</protected></pin-plugin>"));
      free(aux);
      aux = strdup(tmp);
   }

   if (tmp)
      free(tmp);

   return aux;
}

// returns the first day of the week encoded by bits of w
//  LSB that is 1
int DayOfTheWeek(const int w, int starting_day=1)
{
    if (w<0) return -1; // error

    int i=1;
    int N = 1;
    int first_bitpos = -1;
    N=1; i=1;

    if (starting_day == 0) starting_day = 7; // sunday comes last here
    while ( i <= 7 )
    {
        if ( w & N )
        {
            if (first_bitpos < 0) first_bitpos=i;
            if ( starting_day <= i) return i;
        }
        N = N<<1;
        ++i;
    }

    return first_bitpos;
}

eOSState cMenuMyEditTimer::ProcessKey(eKeys Key)
{
    bool bWasSingleEvent = IsSingleEvent();
#if REELVDR
    int bWasIsSwitchTimer = isSwitchTimer;
#endif
    int bChannel = channel;
    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (bWasSingleEvent != IsSingleEvent() 
#if REELVDR
        || bWasIsSwitchTimer != isSwitchTimer
            || (isSwitchTimer && bChannel != channel)
#endif 
            )
    {
	Set();
	Display();
    }
    //printf("WeekDay %i\n", weekdays);

#if REELVDR && APIVERSNUM >= 10718
  if (Key != kNone && state != osUnknown) {
      ShowEventDetailsInSideNote();
  }
#endif

    static int prev_weekdays = weekdays;
    if ( weekdays != prev_weekdays) // has weekdays changed
    {
       // PRINTF("WeekDay (%i / %i)\n", weekdays, prev_weekdays);
        prev_weekdays = weekdays;

        struct tm *tm_r;
        time_t tt = time(NULL);
        tm_r = localtime(&tt);
        //printf("%ld (%i)", day,  tm_r->tm_wday);
        int day_of_the_week = DayOfTheWeek(weekdays, tm_r->tm_wday);
        //printf("\t\tWeekDay  %i (%i)\n", weekdays, day_of_the_week);
        if (day_of_the_week >= 0)
        {
            day = tt + SECSINDAY * ( (7 - tm_r->tm_wday + day_of_the_week ) %7);
            //printf("%ld \n", day);
            Set();
            Display();
        }
        //else PRINTF("(%s:%d) Error: day of the week\n", __FILE__, __LINE__);
    }

    int iOnDirectoryItem = 0;
    int iOnFileItem = 0;
    int iOnDayItem = 0;
    const char* ItemText = Get(Current())->Text();
    if (!HasSubMenu() && ItemText && strlen(ItemText)>0)
    {
	if (strstr(ItemText, tr("Week Day")) == ItemText)
	{
	    if (!IsSingleEvent())
	    {
		SetHelp(trVDR("Button$Edit"));
		iOnDayItem = 1;
	    }
	    else
		SetHelp(addIfConfirmed?NULL:trVDR("Button$Delete"), NULL, NULL, NULL);
	}
	else if (strstr(ItemText, tr("Directory")) == ItemText)
	{
	    if (!InEditMode(ItemText, tr("Directory"), directory))
		SetHelp(addIfConfirmed?NULL:trVDR("Button$Delete"), NULL, trVDR("Button$Reset"), tr("Button$Select"));
	    iOnDirectoryItem = 1;
	}
	else if (strstr(ItemText, tr("File")) == ItemText)
	{
	    if (!InEditMode(ItemText, tr("File"), file))
	    {
		if (event && event->ShortText())
		{
		    if (strchr(file, '~'))
			SetHelp(addIfConfirmed?NULL:tr("Button$Delete"), NULL, tr("Button$Reset"), tr("Button$Without subtitle"));
		    else
			SetHelp(addIfConfirmed?NULL:tr("Button$Delete"), NULL, tr("Button$Reset"), tr("Button$With subtitle"));
		}
		else
		    SetHelp(addIfConfirmed?NULL:tr("Button$Delete"), NULL, tr("Button$Reset"), NULL);
	    }
	    iOnFileItem = 1;
	}
	else
	    SetHelp(addIfConfirmed?NULL:tr("Button$Delete"), NULL, NULL, trVDR("Button$Folder"));
    }
#if REELVDR
    if (isSwitchTimer) SetHelp(NULL);
#endif

    if ((Key == kYellow) && ((iOnDirectoryItem && !InEditMode(ItemText, tr("Directory"), directory)) ||
			     (iOnFileItem && !InEditMode(ItemText, tr("File"), file))))
    {
	if (iOnDirectoryItem)
	    strcpy(directory, "");
	if (iOnFileItem && event)
	    strn0cpy(file, event->Title(),sizeof(file));
	RefreshCurrent();
	Display();
    }

    if (state == osUnknown)
    {
	switch (Key)
	{
	    case kOk:
	    {
		cChannel *ch = Channels.GetByNumber(channel);
		if (!ch)
		{
		    Skins.Message(mtError, tr("*** Invalid Channel ***"));
		    break;
		}

#if REELVDR
                // add switchtimer and return
                if (isSwitchTimer) {
                    cMenuEventItem *item = dynamic_cast<cMenuEventItem*> (Get(Current()));
                    if (!item || !item->event) {
                        Skins.Message(mtInfo,
                                      tr("Please, select a program to set switch timer"));
                        return osContinue;
                    }

                    const cEvent *pEvent = item->event;
                    if (!SwitchTimers.InSwitchList(pEvent))
                    {
                        if (switchTimer)
                            switchTimer->event = pEvent;
                        else
                            switchTimer = new cSwitchTimer(pEvent);

                        if (addIfConfirmed) { // add switchtimer to list
                            cMutexLock SwitchTimersLock(&SwitchTimers);
                            SwitchTimers.Add(switchTimer);
                            SwitchTimers.Save();
                        }
                       cSwitchTimerThread::Init();
                    } else {
                        Skins.Message(mtError, tr("Switchtimer to this programme already present"));
                        return osContinue;
                    }

                    return osBack;
                }
#endif /*REELVDR*/

		string fullaux = "";
		string aux = "";
		char* tmpSummary = NULL;
		if (timer && timer->Aux())
		    fullaux = timer->Aux();

		int bstart = 0, bstop = 0; // calculate margins
		if (event && IsSingleEvent())
		{
		    time_t startTime = 0, stopTime = 0;;
		    int begin  = cTimer::TimeToInt(start); // seconds from midnight
		    int length = cTimer::TimeToInt(stop) - begin;
		    if (length < 0)
			length += SECSINDAY;
		    startTime = cTimer::SetTime(day, begin);
		    stopTime = startTime + length;
		    bstart = event->StartTime() - startTime;
		    bstop = stopTime - event->EndTime();

		    char* epgsearchaux = GetAuxValue(timer, "epgsearch");
		    if (epgsearchaux)
		    {
			aux = epgsearchaux;
			free(epgsearchaux);
		    }
		    aux = UpdateAuxValue(aux, "channel", NumToString(ch->Number()) + " - " + CHANNELNAME(ch));
		    aux = UpdateAuxValue(aux, "update", checkmode);
		    aux = UpdateAuxValue(aux, "eventid", event->EventID());
		    aux = UpdateAuxValue(aux, "bstart", bstart);
		    aux = UpdateAuxValue(aux, "bstop", bstop);
		    fullaux = UpdateAuxValue(fullaux, "epgsearch", aux);
		}

#ifdef USE_PINPLUGIN
        aux = "";
        aux = UpdateAuxValue(aux, "protected", fskProtection ? "yes" : "no");
        fullaux = UpdateAuxValue(fullaux, "pin-plugin", aux);
#endif

		char* tmpFile = strdup(file);
		tmpFile = strreplace(tmpFile, ':', '|');
		char* tmpDir = strdup(directory);
		tmpDir = strreplace(tmpDir, ':', '|');
		if (strlen(tmpFile) == 0)
		    tmpFile = strdup(CHANNELNAME(ch));
		priority = tmpprio == 0 ? 10 : tmpprio == 1 ? 50 : 99;
#ifdef USE_PINPLUGIN
		tmpSummary = SetFskProtection(fskProtection, tmpSummary);        // PIN PATCH
#endif
		if (timer)
		{
                    cString cmdbuf;
                    cmdbuf = cString::sprintf("%d:%d:%s:%04d:%04d:%d:%d:%s%s%s:%s",
                    flags,
                    ch->Number(),
#if VDRVERSNUM < 10503 && !defined(REELVDR)
                 PRINTDAY(day, weekdays),
#else
                 PRINTDAY(day, weekdays, true),
#endif
                    start,
                    stop,
                    priority,
                    lifetime,
			strlen(tmpDir)>0?tmpDir:"",
		        (strlen(tmpDir)>0 && strlen(tmpFile)>0)?"~":"", tmpFile, tmpSummary?tmpSummary:"");
        	        timer->Parse(cmdbuf);
#ifdef USEMYSQL
                        // DONOT check ID of timer because the 'timer' object
                        // here created has yet no vaild ID
                        cTimer *t = Timers.GetTimer(timer, false);
#else
                        cTimer *t = Timers.GetTimer(timer);
#endif
        	if (t)
        	    t->Parse(cmdbuf);
            else if (!t /*TB && addIfConfirmed */)
    		    Timers.Add(timer);
		    Timers.SetModified();
		    free(tmpFile);
		    free(tmpDir);
		    free(tmpSummary);
		    addIfConfirmed = false;
            if(timerCreated_ != NULL)
            {
                *timerCreated_ = true;
            }
		}
	    }
	    return osBack;
	    case kRed:
		if (HasSubMenu())
		    return osContinue;
		else
		    if (iOnDayItem)
			return AddSubMenu(new cMenuEditDaysOfWeek(&weekdays,1,false));
		    else
			{
			    return addIfConfirmed ? osContinue : DeleteTimer();
			}
		break;
	    case kGreen:
	    case kYellow:
		return osContinue;
	    case kBlue:
                if (HasSubMenu() || isSwitchTimer)
		    return osContinue;
		//if (iOnDirectoryItem && !InEditMode(ItemText, tr("Directory"), directory))
		    state = AddSubMenu(new cMenuDirSelect(directory));
		if (iOnFileItem)
		{
		    HandleSubtitle();
		    return osContinue;
		}
		break;
	    default: break;
	}
    }

    if (!HasSubMenu() && HadSubMenu) {
	Set();
    }

    if (Key != kNone && !HasSubMenu())
    {
	if (iOnDirectoryItem && !InEditMode(ItemText, tr("Directory"), directory))
	    ReplaceDirVars();
    }
    return state;
}

void cMenuMyEditTimer::ReplaceDirVars()
{
    if (!strchr(directory,'%') || !event)
	return;

    cVarExpr varExpr(directory);
    strcpy(directory, varExpr.Evaluate(event).c_str());
    if (strchr(directory, '%') != NULL) // only set directory to new value if all categories could have been replaced
	*directory = 0;
    if (varExpr.DependsOnVar("%title%", event) || varExpr.DependsOnVar("%subtitle%", event))
    {
	strcpy(file, directory);
	*directory = 0;
	SplitFile();
    }

    RefreshCurrent();
    // update title too
    if (Current() > 0)
	Get(Current()-1)->Set();
    Display();
    return;
}

void cMenuMyEditTimer::ShowEventDetailsInSideNote() {

#if REELVDR && APIVERSNUM >= 10718

    if (!HasSubMenu()) {
        cMenuEventItem *item = dynamic_cast<cMenuEventItem*> (Get(Current()));

        if (item)
            SideNote(item->event);
        else
            SideNote((const cEvent*)NULL);
    }
#endif /* REELVDR && APIVERSNUM >= 10718 */
}
