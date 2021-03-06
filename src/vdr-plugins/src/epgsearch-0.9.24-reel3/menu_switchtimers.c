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

#include "menu_switchtimers.h"
#include "epgsearchtools.h"
#include "menu_epgtimers.h"

// --- cMenuEditSwitchTimer --------------------------------------------------------
class cMenuEditSwitchTimer : public cOsdMenu {
private:
  cSwitchTimer *switchTimer;
  cSwitchTimer data;
  bool addIfConfirmed;
public:
  cMenuEditSwitchTimer(cSwitchTimer *SwitchTimer, bool New = false);
  void Set();
  virtual eOSState ProcessKey(eKeys Key);
};

cMenuEditSwitchTimer::cMenuEditSwitchTimer(cSwitchTimer* SwitchTimer, bool New)
:cOsdMenu(tr("Edit entry"),30)
{
    switchTimer = SwitchTimer;
    addIfConfirmed = New;
    if (switchTimer) 
    {
	data = *switchTimer;
	Set();
    }
}

void cMenuEditSwitchTimer::Set()
{
    int current = Current();
    Clear();

    Add(new cMenuEditIntItem(tr("Switch ... minutes before start"), &data.switchMinsBefore, 0, 99));
    cString info = cString::sprintf("%s:\t%s", tr("action at"), 
				    TIMESTRING(data.event->StartTime() - 60 * data.switchMinsBefore));
    cOsdItem* pInfoItem = new cOsdItem(info);
    pInfoItem->SetSelectable(false);
    Add(pInfoItem);
    Add(new cMenuEditBoolItem(tr("Announce only"), &data.announceOnly, trVDR("no"), trVDR("yes")));
    Add(new cMenuEditBoolItem(tr("Unmute sound"), &data.unmute, trVDR("no"), trVDR("yes")));    
    SetCurrent(Get(current));
}

eOSState cMenuEditSwitchTimer::ProcessKey(eKeys Key)
{
    int iOldMinsBefore = data.switchMinsBefore;
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (iOldMinsBefore != data.switchMinsBefore)
    {
	time_t now = time(NULL);
	if (data.event->StartTime() - 60 * data.switchMinsBefore < now)
	    data.switchMinsBefore = iOldMinsBefore;
	Set();
	Display();
    }

    if (state == osUnknown) {
	switch (Key) {
	    case kOk:
	    {
		if (switchTimer)
		{
		    *switchTimer = data;
		    cMutexLock SwitchTimersLock(&SwitchTimers);
		    if (addIfConfirmed)
			SwitchTimers.Add(switchTimer);	      
		    SwitchTimers.Save();
		}
		addIfConfirmed = false;
		return osBack;
		break;
	    } 
	    default: break;
	}
  }
    return state;
}

cMenuSwitchTimerItem::cMenuSwitchTimerItem(cSwitchTimer* SwitchTimer)
{
  switchTimer = SwitchTimer;
  Set();
}

void cMenuSwitchTimerItem::Set(void)
{
    if (!SwitchTimers.Exists(switchTimer) || !switchTimer || !switchTimer->event)
	return;

    const cEvent* event = switchTimer->event;
    time_t startTime = event->StartTime();
    char *buffer = NULL;

    char datebuf[32];
    struct tm tm_r;
    tm *tm = localtime_r(&startTime, &tm_r);
    strftime(datebuf, sizeof(datebuf), "%a %d", tm);

    cChannel* channel = Channels.GetByChannelID(event->ChannelID(),true,true);

    msprintf(&buffer, "%s\t%d\t%s\t%s\t%d\'\t%s~%s", switchTimer->announceOnly?"":">", channel?channel->Number():-1, datebuf, TIMESTRING(startTime), switchTimer->switchMinsBefore, event->Title()?event->Title():"", event->ShortText()?event->ShortText():"");
    SetText(buffer, false);

#if 0
    // icon / channel name / date / time / event info
    cString buf = cString::sprintf("%s\t%s\t%s\t%s\t%s","S  ", /* must be 3 chars long */
                                   channel?channel->Name():tr("Unknown channel"),
                                   datebuf,
                                   TIMESTRING(startTime),
                                   event->Title()?event->Title():"");
#else
    // icon / channel name / time / event info
    cString buf = cString::sprintf("%s\t%s\t%s\t%s",
                                   "S  ", /* must be 3 chars long */
                                   channel?channel->Name():tr("Unknown channel"),
                                   //datebuf, // show no date
                                   TIMESTRING(startTime),
                                   event->Title()?event->Title():"");

#endif

    SetText(buf);
}

int cMenuSwitchTimerItem::Compare(const cListObject &ListObject) const
{
    const cMenuSwitchTimerItem *p = dynamic_cast<const cMenuSwitchTimerItem *>(&ListObject);
    if (p && p->switchTimer && p->switchTimer->event)
        return switchTimer->event->StartTime() - p->switchTimer->event->StartTime();

    //cMenuMyTimerItem
    const cMenuMyTimerItem *item = dynamic_cast<const cMenuMyTimerItem*> (&ListObject);
    if (item)
        return switchTimer->event->StartTime() - item->Timer()->StartTime();

    return 0;
}

// --- cMenuSwitchTimers ----------------------------------------------------------
cMenuSwitchTimers::cMenuSwitchTimers()
:cOsdMenu(tr("Switch list"), 2, 4, 6, 6, 4)
{
    Set();
    Display();
}

void cMenuSwitchTimers::Set()
{
    Clear();
    cMutexLock SwitchTimersLock(&SwitchTimers);
    cSwitchTimer* switchTimer = SwitchTimers.First();
    while (switchTimer) {
	if (switchTimer->event)
	    Add(new cMenuSwitchTimerItem(switchTimer));
	switchTimer = SwitchTimers.Next(switchTimer);
    }
    Display();
    SetHelp(trVDR("Button$Edit"), tr("Button$Delete all"), trVDR("Button$Delete"), NULL);
    Sort();
}

cSwitchTimer* cMenuSwitchTimers::CurrentSwitchTimer(void)
{
    cMenuSwitchTimerItem *item = (cMenuSwitchTimerItem *)Get(Current());
    if (item && SwitchTimers.Exists(item->switchTimer))
	return item->switchTimer;
    return NULL;
}

eOSState cMenuSwitchTimers::Delete(void)
{
    cSwitchTimer *curSwitchTimer = CurrentSwitchTimer();
    if (curSwitchTimer) {
	if (Interface->Confirm(tr("Edit$Delete entry?"))) {
	    cMutexLock SwitchTimersLock(&SwitchTimers);
	    SwitchTimers.Del(curSwitchTimer);
	    SwitchTimers.Save();
	    cOsdMenu::Del(Current());
	    Display();
	}
    }
    return osContinue;
}

eOSState cMenuSwitchTimers::DeleteAll(void)
{
    if (Interface->Confirm(tr("Edit$Delete all entries?"))) 
    {
	cMutexLock SwitchTimersLock(&SwitchTimers);
	while (SwitchTimers.First()) 
	    SwitchTimers.Del(SwitchTimers.First());
	SwitchTimers.Save();
	Set();
    }

    return osContinue;
}

eOSState cMenuSwitchTimers::Summary(void)
{
    if (HasSubMenu() || Count() == 0)
	return osContinue;
    cSwitchTimer *curSwitchTimer = CurrentSwitchTimer();
    if (curSwitchTimer && !isempty(curSwitchTimer->event->Description()))
	return AddSubMenu(new cMenuText(tr("Summary"), curSwitchTimer->event->Description()));
    return osContinue;
}

eOSState cMenuSwitchTimers::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
	case kOk:
	    state = Summary();
	    break;
	case kGreen:
	    state = DeleteAll();
	    break;
	case kYellow:
	    state = Delete();
	    break;
	case kRed:
	    if (HasSubMenu())
		return osContinue;
	    if (CurrentSwitchTimer())
		state = AddSubMenu(new cMenuEditSwitchTimer(CurrentSwitchTimer()));
	    else
		state = osContinue;
	    break;
	case k0:
	    if (CurrentSwitchTimer())
	    {
		cSwitchTimer* switchTimer = CurrentSwitchTimer();
	        switchTimer->announceOnly = 1 - switchTimer->announceOnly;
		cMutexLock SwitchTimersLock(&SwitchTimers);
		SwitchTimers.Save();
		RefreshCurrent();
		Display();
	    }
	    break;
      default: break;
    }
  }

  return state;
}
