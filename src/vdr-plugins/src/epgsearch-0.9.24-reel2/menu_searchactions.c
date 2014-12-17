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
#include "menu_commands.h"
#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include "menu_searchactions.h"
#include "menu_searchresults.h"
#include "menu_recsdone.h"
#include "menu_timersdone.h"
#include "menu_switchtimers.h"
#include "menu_blacklists.h"
#include "epgsearchtools.h"
#include "menu_conflictcheck.h"
#include "epgsearchcfg.h"
#include "searchtimer_thread.h"
#include "menu_searchedit.h"

using namespace std;

#define ACTION_COUNTER 11
extern int updateForced;
// --- cMenuSearchActions ---------------------------------------------------------

cMenuSearchActions::cMenuSearchActions(cSearchExt* Search, bool DirectCall)
:cOsdMenu(tr("Search actions"))
{
    directCall = DirectCall;
    SetHasHotkeys();
    
    search = Search;

    Add(new cOsdItem(hk(tr("Execute search"))));
    Add(new cOsdItem(hk(tr("Use as search timer on/off"))));
    Add(new cOsdItem(hk(tr("Trigger search timer update"))));
    Add(new cOsdItem(hk(tr("Show recordings done"))));
    Add(new cOsdItem(hk(tr("Delete created timers?"))));
    Add(new cOsdItem(hk(tr("Timer conflict check"))));
    //Add(new cOsdItem(hk(tr("Create a copy"))));

}

cMenuSearchActions::~cMenuSearchActions()
{
}


eOSState cMenuSearchActions::Search(void)
{
    cMenuTemplate* MenuTemplate = NULL;
    if (search && search->menuTemplate > 0)
	MenuTemplate = cTemplFile::GetSearchTemplateByPos(search->menuTemplate);
    if (!MenuTemplate)
	MenuTemplate = cTemplFile::GetTemplateByName("MenuSearchResults");
    return AddSubMenu(new cMenuSearchResultsForSearch(search, MenuTemplate));
}

eOSState cMenuSearchActions::OnOffSearchtimer(void)
{
   if (search) 
   {
      search->useAsSearchTimer = search->useAsSearchTimer?0:1;
#ifdef USEMYSQL
      if((Setup.ReelboxModeTemp==eModeClient)||(Setup.ReelboxModeTemp==eModeServer))
          search->UpdateDB();
      else
          SearchExts.Save();
#else
	  SearchExts.Save();
#endif
	  if (!search->useAsSearchTimer && Interface->Confirm(tr("Disable associated timers too?")))
         search->OnOffTimers(false);
      if (search->useAsSearchTimer && Interface->Confirm(tr("Activate associated timers too?")))
      {
         search->OnOffTimers(true);
         if (!EPGSearchConfig.useSearchTimers) // enable search timer thread if necessary
         {
            cSearchTimerThread::Init((cPluginEpgsearch*) cPluginManager::GetPlugin("epgsearch"), true);
            Skins.Message(mtInfo, tr("Search timers activated in setup."));			
         }
      }      
  }
  return osBack;
}


eOSState cMenuSearchActions::Execute()
{
   int current = Current();

   if (current <= ACTION_COUNTER-1)
   {
      if (current == 0)
         return Search();
      if (current == 1)
         return OnOffSearchtimer(); 
      if (current == 2)
      {
         if (!EPGSearchConfig.useSearchTimers) // enable search timer thread if necessary
         {
            cSearchTimerThread::Init((cPluginEpgsearch*) cPluginManager::GetPlugin("epgsearch"), true);
            Skins.Message(mtInfo, tr("Search timers activated in setup."));			
         }
         if (Interface->Confirm(tr("Run search timer update?")))
         {
#ifdef USEMYSQL
             if((Setup.ReelboxModeTemp==eModeClient) && Setup.NetServerIP)
             {
                 char command[128];
                 snprintf(command, 128, "svdrpsend.sh -d %s plug epgsearch UPDS", Setup.NetServerIP);
                 if(SystemExec(command))
                     Skins.Message(mtError, tr("ERROR: Could not trigger search timer update!"));
                 else
                     Skins.Message(mtInfo, tr("Search timer update triggered."));
             } 
             else
#endif
            updateForced = 2; // with message about completion
         }
         return osBack;
      }	
      if (current == 3)
         return AddSubMenu(new cMenuRecsDone(search));
      if (current == 4)
      {
         if (!Interface->Confirm(tr("Delete all timers created from this search?")))
            return osBack;
	 search->DeleteAllTimers();
	 return osBack;
     }
     if (current == 5)	 
	 return AddSubMenu(new cMenuConflictCheck());
#if 0
           if (!Interface->Confirm(tr("Copy this entry?")))
              return osBack;
           cSearchExt* copy = new cSearchExt;
           copy->CopyFromTemplate(search);
     string copyname = string(tr("Copy")) + ": " + search->search;
           strcpy(copy->search, copyname.c_str());
     cMutexLock SearchExtsLock(&SearchExts);
     copy->ID = SearchExts.GetNewID();
     SearchExts.Add(copy);
     SearchExts.Save();
     return AddSubMenu(new cMenuEditSearchExt(copy));
        }
        if (current == 6)
        {
#endif
  }
  return osContinue;
}

eOSState cMenuSearchActions::ProcessKey(eKeys Key)
{
   bool hadSubmenu = HasSubMenu();
   if (directCall && Key == k1 && !HasSubMenu())
      return Search();

   eOSState state = cOsdMenu::ProcessKey(Key);

   // jump back to calling menu, if a command was called directly with key '1' .. '9'
   if (directCall && hadSubmenu && !HasSubMenu())
      return osBack;

   if (state == osUnknown) {
      switch (Key) {
         case kGreen:
         case kYellow:
         case kBlue:
            return osContinue;
         case kOk:  
            if (!HasSubMenu())
               return Execute();
         default:   break;
      }
   }
   return state;
}
