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
#include <string>
#include <iomanip>
#include <sstream>

#include "epgsearchtools.h"
#include "epgsearchcfg.h"
#include "menu_search.h"
#include "menu_searchedit.h"
#include "menu_searchactions.h"
#include "recdone.h"
#include "timerdone.h"
#include "vdr/debug.h"

using namespace std;

// --- cMenuSearchExtItem ----------------------------------------------------------
class cMenuSearchExtItem : public cOsdItem {
  private:
public:
  cSearchExt* searchExt;
  cMenuSearchExtItem(cSearchExt* SearchExt);
  int Compare(const cListObject &ListObject) const;
  void Set(void);
  };

cMenuSearchExtItem::cMenuSearchExtItem(cSearchExt* SearchExt)
{
  searchExt = SearchExt;
  Set();
}

void cMenuSearchExtItem::Set(void)
{
  ostringstream line;

  if (searchExt->useAsSearchTimer)
    {
       if (searchExt->IsActiveAt(time(NULL)))
  line << ">";
      else
  line << "!";
    }

  line << "\t";
  if (searchExt->search && strlen(searchExt->search) > 0)
    line << setiosflags(ios::left) << string(searchExt->search);
  else
    line << setiosflags(ios::left) << "*";

  line << "\t";
  if (searchExt->useChannel == 1)
  {
      if (searchExt->channelMin != searchExt->channelMax)
    line << setiosflags(ios::left) << searchExt->channelMin->Number() << " - " << searchExt->channelMax->Number();
      else
  line << setiosflags(ios::left) << setw(11) << (searchExt->useChannel?CHANNELNAME(searchExt->channelMin):"");
  }
  else if (searchExt->useChannel == 2)
      line << setiosflags(ios::left) << setw(11) << searchExt->channelGroup;
  else
    line << " ";

  if (searchExt->useTime)
  {
    ostringstream timeline;
    timeline << setfill('0') << setw(2) << searchExt->startTime / 100 << ":" << setw(2) << searchExt->startTime % 100;
    timeline << "\t";
    timeline << setfill('0') << setw(2) << searchExt->stopTime / 100 << ":" << setw(2) << searchExt->stopTime % 100;
    line << timeline.str();
  }
  else
    line << "--:--\t--:--";

  SetText(strdup(line.str().c_str()), false);
}

int cMenuSearchExtItem::Compare(const cListObject &ListObject) const
{
  cMenuSearchExtItem *p = (cMenuSearchExtItem *)&ListObject;
  return strcasecmp(searchExt->search, p->searchExt->search);
}

// --- cMenuEPGSearchExt ----------------------------------------------------------
cMenuEPGSearchExt::cMenuEPGSearchExt()
:cOsdMenu("", 2, 20, 11, 6, 5)
{
    Set();
    Sort();
    UpdateTitle();
}

void cMenuEPGSearchExt::Set()
{
    int current = Current();
    cMutexLock SearchExtsLock(&SearchExts);

#ifdef USEMYSQL
    LastEventID = SearchExts.LastEventID;
#endif

    Clear();
    number_of_searchtimers_shown = 0;
    cSearchExt *SearchExt = SearchExts.First();
    while (SearchExt) {
	Add(new cMenuSearchExtItem(SearchExt));
	SearchExt = SearchExts.Next(SearchExt);
    	++number_of_searchtimers_shown ;
    }

#if REELVDR
    if ( SearchExts.Count() == 0 ) // No timers present
    {
        // make these unselectable, selectables are only cMenuTimerItem's objects
        Add(new cOsdItem(tr("No searchtimers present."),osUnknown, false));
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
        AddFloatingText(tr("Use the green button to create a new searchtimer."), MAXOSDTEXTWIDTH );
        DDD("No searchtimers");
     }
#endif
    // Display called in UpdateTitle()
    SetCurrent(Get(current));
}

void cMenuEPGSearchExt::UpdateTitle(bool ForceRebuild)
{
    SetCols(3, 20, 5, 5);
    int total=0, active=0;
    cMutexLock SearchExtsLock(&SearchExts);
    cSearchExt *SearchExt = SearchExts.First();
    while (SearchExt) {
	if (SearchExt->useAsSearchTimer) active++;
	SearchExt = SearchExts.Next(SearchExt);
	total++;
    }

#if REELVDR
    cString buffer;
    if (total == 0)
        buffer = cString::sprintf("%s", tr("Search entries"));
    else
        buffer = cString::sprintf("%s (%d/%d %s)", tr("Search entries"), active, total, tr("active"));
#else
    cString buffer = cString::sprintf("%s (%d/%d %s)", tr("Search entries"), active, total, tr("active"));
#endif
    SetTitle(buffer);

    //static eKeys ikeys[] = { kInfo, kNone };
    if (total > 0) {
	/// Delete - New - Execute - Actions
    	SetHelp(trVDR("Button$Delete"), trVDR("Button$New"), tr("Execute"), tr("Button$Actions"));
    } else {
        SetHelp(NULL,  trVDR("Button$New"), NULL,  NULL);
    }

    if (ForceRebuild || (number_of_searchtimers_shown != SearchExts.Count())) // search timers were deleted or added
        Set();
    Display();
}

cSearchExt *cMenuEPGSearchExt::CurrentSearchExt(void)
{
    cMenuSearchExtItem *item = (cMenuSearchExtItem *)Get(Current());
    if (item && SearchExts.Exists(item->searchExt))
	return item->searchExt;
    return NULL;
}


eOSState cMenuEPGSearchExt::New(void)
{
  if (HasSubMenu())
    return osContinue;
  return AddSubMenu(new cMenuEditSearchExt(new cSearchExt, true));
}
void cMenuEPGSearchExt::ShowNew(const char *fText)
{
  if (HasSubMenu())
    return;
  cSearchExt* pNew = new cSearchExt;
  strcpy(pNew->search, fText);
  AddSubMenu(new cMenuEditSearchExt(pNew, true));
}

eOSState cMenuEPGSearchExt::Delete(void)
{
    cSearchExt *curSearchExt = CurrentSearchExt();
    if (curSearchExt) {
	if (Interface->Confirm(tr("Edit$Delete search?"))) {
	    int DelID = curSearchExt->ID;
	    if (Interface->Confirm(tr("Delete all timers created from this search?")))
	    {
		curSearchExt->DeleteAllTimers();
		cMutexLock TimersDoneLock(&TimersDone);
		TimersDone.DelAllSearchID(DelID);
		TimersDone.Save();
	    }
	    LogFile.Log(1,"search timer %s (%d) deleted", curSearchExt->search, curSearchExt->ID);
	    cMutexLock SearchExtsLock(&SearchExts);
	    SearchExts.Del(curSearchExt);
	    SearchExts.Save();
	    RecsDone.RemoveSearchID(DelID);
	    cOsdMenu::Del(Current());
	    Display();
	    Set(); // show new timers list
	    UpdateTitle();
	}
    }
    return osContinue;
}

eOSState cMenuEPGSearchExt::Actions(eKeys Key)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cSearchExt* search = CurrentSearchExt();
  if (search) {
      cMenuSearchActions *menu;
      eOSState state = AddSubMenu(menu = new cMenuSearchActions(search, true));
      if (Key != kNone)
	  state = menu->ProcessKey(Key);
      return state;
  }
  return osContinue;
}


eOSState cMenuEPGSearchExt::ProcessKey(eKeys Key)
{
  int SearchNumber = HasSubMenu() ? Count() : -1;
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
	case k0:
	    if (HasSubMenu())
		return osContinue;
	    if (CurrentSearchExt())
		state = AddSubMenu(new cMenuSearchActions(CurrentSearchExt()));
	    else
		state = osContinue;
	    break;
	case k1...k9:
	    return Actions(Key);
	/// Delete - New - Execute - Actions - ?
	case kRed:
	    state = Delete(); break;
	case kGreen:
	    state = New(); break;
	case kYellow:
	    state = Actions(k1); break;
	case kBlue:
	    if (HasSubMenu())
		return osContinue;
	    if (CurrentSearchExt())
		state = AddSubMenu(new cMenuSearchActions(CurrentSearchExt()));
	    else
		state = osContinue;
	    break;
	case kOk:
	  if (HasSubMenu())
	    return osContinue;
	  if (CurrentSearchExt())
	      state = AddSubMenu(new cMenuEditSearchExt(CurrentSearchExt()));
	  else
	      state = osContinue;
	  break;
	default: break;
    }
  }
  if (SearchNumber >= 0 && !HasSubMenu())
  {
      cMutexLock SearchExtsLock(&SearchExts);
      cSearchExt* search = SearchExts.Get(SearchNumber);
      if (search)       // a newly created search was confirmed with Ok
	  Add(new cMenuSearchExtItem(search));
      else
	  search = CurrentSearchExt();
      // always update all entries, since channel group names may have changed and affect other searches
      Sort();
      for(int i=0; i<Count(); i++)
      {
	  cMenuSearchExtItem *item = (cMenuSearchExtItem *)Get(i);
	  if (item)
	  {
	      item->Set();
	      if (item->searchExt == search)
		  SetCurrent(item);
	  }
      }
      Display();
      UpdateTitle();
  }

#ifdef USEMYSQL
  if(!HasSubMenu() && (LastEventID != SearchExts.LastEventID))
  {
      UpdateTitle(true);
      LastEventID = SearchExts.LastEventID;
  }
#endif

  return state;
}
