/*
 * pin.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/interface.h>

#include "menu.h"
#include "pin.h"

//***************************************************************************
// Pin Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPinMenu::cPinMenu(const char* title, cLockItems* theChannels,
                   cLockedBroadcasts* theBroadcasts, cLockItems* thePlugins)
   : cOsdMenu(title)
{
   cLockItem* plgSetup;
   int setupLocked = no;

   lockedChannels = theChannels;
   lockedBroadcasts = theBroadcasts;
   lockedPlugins = thePlugins;
   
   SetHelp(tr("Recordings"), NULL, NULL, NULL);
   
   if ((plgSetup = lockedPlugins->FindByName("setup")))
      setupLocked = plgSetup->GetActive();
   cOsdMenu::Add(new cPinMenuItem(tr("1 Activate childlock"), miLock));
   cOsdMenu::Add(new cPinMenuItem(tr("2 Add current channel to protection list"), miAddCurrentChannel));
   cOsdMenu::Add(new cPinMenuItem(tr("3 Add current broadcast to protection list"), miAddCurrentBroadcast));
   cOsdMenu::Add(new cPinMenuItem(tr("4 Edit channel locks"), miEditChannelList));
   cOsdMenu::Add(new cPinMenuItem(tr("5 Edit broadcast locks"), miEditBroadcastList));
   //TB cOsdMenu::Add(new cPinMenuItem(tr("6 Edit plugin locks"), miEditPluginList));

   if (plgSetup)
      cOsdMenu::Add(new cPinMenuItem(setupLocked ? tr("6 Unlock setup") : tr("6 Lock setup"), miProtectSetup));

   Add(new cOsdItem(" ", osUnknown, false), false, NULL);
   Add(new cOsdItem(" ", osUnknown, false), false, NULL);	
   Add(new cOsdItem(tr("Note:"), osUnknown, false), false, NULL);
   AddFloatingText(tr("Please activate Childlock by pressing (1)."), 45);

   Display();
}

cPinMenu::~cPinMenu()
{
   // Aenderungen speichern

   lockedChannels->Save();
   lockedBroadcasts->Save();
   lockedPlugins->Save();
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cPinMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);
   int action;

   if (state == osUnknown)
   {
       switch (key)
       {
          case k1:  action = miLock;                break;
          case k2:  action = miAddCurrentChannel;   break;
          case k3:  action = miAddCurrentBroadcast; break;
          case k4:  action = miEditChannelList;     break;
          case k5:  action = miEditBroadcastList;   break;
          //TB case k6:  action = miEditPluginList;      break;
          //TB case k7:  action = miProtectSetup;        break;
          case k6:  action = miProtectSetup;        break;
          case kRed: action = cRemote::CallPlugin("extrecmenu"); return osEnd;

          case kOk: action = currentAction();       break;
          default: return state;
       }

       switch (action)
       {
          case miLock:
          {
             if (*cPinPlugin::pinCode)
             {
                cOsd::pinValid = false;
                Skins.Message(mtInfo, tr("Activated pin protection"));
             }
             else
             {
                Skins.Message(mtInfo, tr("Please setup a pin code first!"));
             }

             return osEnd;
          }

          case miAddCurrentChannel:   addCurrentChannel();      break;
          case miAddCurrentBroadcast: addCurrentBroadcast();    break;

          case miEditChannelList:
             state = AddSubMenu(new cLockMenu(tr("Protected channels"),
                                              lockedChannels, ltChannels));
             break;
          case miEditBroadcastList:
             state = AddSubMenu(new cLockMenu(tr("Protected broadcasts"),
                                              (cLockItems*)lockedBroadcasts, ltBroadcasts));
             break;
          case miEditPluginList:
             state = AddSubMenu(new cLockMenu(tr("Plugin protections"),
                                              lockedPlugins, ltPlugins));
             break;
          case miProtectSetup:
             int locked = protectSetup();
             //TB ((cPinMenuItem*)Get(7-1))->SetText(
                //TB locked ? tr("7 Unlock setup") : tr("7 Lock setup"), /*copy*/ true);
             ((cPinMenuItem*)Get(6-1))->SetText(
                locked ? tr("6 Unlock setup") : tr("6 Lock setup"), /*copy*/ true);
		    
	    Display();
             break;
       }
    }
    return state;
}

//***************************************************************************
// Protect Setup
//***************************************************************************

int cPinMenu::protectSetup()
{
   cLockItem* plgSetup;
   char* buf;

   if (!(plgSetup = lockedPlugins->FindByName("setup")))
      lockedPlugins->Add(plgSetup = new cLockItem("setup"));

   plgSetup->SetActive(!plgSetup->GetActive());

   asprintf(&buf, "Setup %s", plgSetup->GetActive() ? tr("locked") : tr("unlocked"));
   Skins.Message(mtInfo, buf);
   free(buf);

   return plgSetup->GetActive();
}

//***************************************************************************
// Add Current Channel
//***************************************************************************

int cPinMenu::addCurrentChannel()
{
   char* buf;
   cChannel* channel = Channels.GetByNumber(cDevice::CurrentChannel());

   if (!channel || channel->GroupSep())
      return ignore;

   if (lockedChannels->FindByName(channel->Name()))
   {
      asprintf(&buf, "%s - %s", channel->Name(), tr("already in list"));
      Skins.Message(mtInfo, buf);
      free(buf);

      return ignore;
   }

   asprintf(&buf, "%s - %s", channel->Name(), tr("added to protection list"));
   Skins.Message(mtInfo, buf);
   free(buf);

   // append channel to lock list

   lockedChannels->Add(new cLockItem(channel->Name()));

   return success;
}

//***************************************************************************
// Add Current Broadcast
//***************************************************************************

int cPinMenu::addCurrentBroadcast()
{
   cSchedulesLock schedLock;
   const cSchedules* scheds;
   const cSchedule *sched;
   const cEvent* event;
   char* buf;

   cChannel* channel = Channels.GetByNumber(cDevice::CurrentChannel());

   if (channel && !channel->GroupSep())
   {
      if (!(scheds = cSchedules::Schedules(schedLock)))
         return done;

      if (!(sched = scheds->GetSchedule(channel->GetChannelID())))
         return done;

      if (!(event = sched->GetPresentEvent()))
         return done;

      // Info

      asprintf(&buf, "%s - %s", event->Title(), tr("added to protection list"));
      Skins.Message(mtInfo, buf);
      free(buf);

      // append event to lock list

      lockedBroadcasts->Add(new cLockedBroadcast(event->Title()));
   }

   return success;
}

//***************************************************************************
// Pin Menu Item
//***************************************************************************
//***************************************************************************
// Process Key
//***************************************************************************

cPinMenuItem::cPinMenuItem(const char* title, int aAction)
   : cOsdItem()
{
   action = aAction;
   SetText(title, /*copy*/ true);
}

//***************************************************************************
// Lock Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cLockMenu::cLockMenu(const char* title, cLockItems* theItems, ListType theType)
   : cOsdMenu(title)
{
   items = theItems;
   type = theType;

   Set();
}

void cLockMenu::Set(void) {
   Clear();

   if (items->Count() == 0) {
      switch (type) {
        case ltChannels:
            Add(new cOsdItem(tr("No locked channels present."),osUnknown, false));
            AddFloatingText(tr("Use the green button to lock channels."), 45);
            break;
        case ltBroadcasts:
            Add(new cOsdItem(tr("No locked broadcasts present."),osUnknown, false));
            AddFloatingText(tr("Use the green button to lock broadcasts."), 45);
            break;
        case ltPlugins:
            break;
        default:
            break;
      }
   }

   cLockItem* item;

   for (item = items->First(); item; item = items->Next(item))
      cOsdMenu::Add(new cLockMenuItem(item));

   SetHelpKeys();
 
   Display();
}

void cLockMenu::SetHelpKeys()
{
    if (type == ltPlugins)
    {
        if (items->Count())
            SetHelp(tr("Edit"), 0, 0, tr("Lock / Unlock"));
        else
            SetHelp(0, 0, 0, 0);
    }
    else
    {
        if (items->Count())
            SetHelp(tr("Edit"), trVDR("New"), tr("Delete"), tr("On/Off"));
        else
            SetHelp(0, trVDR("New"), 0,0);
    }
}
cLockMenu::~cLockMenu()
{
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cLockMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

   if (state == osUnknown)
   {
      switch (key)
      {
         case kOk:
         case kRed:                          // Edit
         {
            if (!items->Count() || !Get(Current())) return osContinue;
               //break;

            if (items->GetListType() == cPinPlugin::ltBroadcasts)
               state = AddSubMenu(new cBroadcastEditMenu((cLockedBroadcast*)
                                                         (((cLockMenuItem*)Get(Current()))->GetLockItem())));
            else
               state = AddSubMenu(new cLockEditMenu(((cLockMenuItem*)Get(Current()))->GetLockItem()));

            return osContinue;
            break;
         }

         case kGreen:                         // New
         {
            cLockItem* item;

            if (type != ltPlugins)
            {
               if (items->GetListType() == cPinPlugin::ltBroadcasts)
                  items->Add(item = new cLockedBroadcast(tr("- new -")));
               else
                  items->Add(item = new cLockItem(tr("- new -")));

               cOsdMenu::Add(new cLockMenuItem(item));

               Set();
            }

            return osContinue;
            break;
         }

         case kYellow:                        // Delete
         {
            if (type != ltPlugins)
            {
               if (!items->Count() || !Get(Current()))
                    return osContinue;
               //   break;

               if (Interface->Confirm(tr("Remove entry?")))
               {
                  // remove from list

                  items->Del(((cLockMenuItem*)Get(Current()))->GetLockItem());
                  cOsdMenu::Del(Current());
               }

               Set();
            }

            return osContinue;
            break;
         }

         case kBlue:                        // Toggle
         {
            if (!items->Count() || !Get(Current())) return osContinue;
            cLockItem* p = ((cLockMenuItem*)Get(Current()))->GetLockItem();

            p->SetActive(!p->GetActive());
            ((cLockMenuItem*)Get(Current()))->Set();

            Set();

            return osContinue;
            break;
         }

         default:  break;
      }
   }

   return state;
}

//***************************************************************************
// Lock Menu Item
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cLockMenuItem::cLockMenuItem(cLockItem* aItem)
   : cOsdItem()
{
   item = aItem;
   Set();
}

cLockMenuItem::~cLockMenuItem()
{
}

//***************************************************************************
// Set
//***************************************************************************

void cLockMenuItem::Set()
{
   char* tmp;
   asprintf(&tmp, "%c %s", item->GetActive() ? '>' : ' ', GetTitle());
   SetText(tmp, /*copy*/ true);
   free(tmp);
}

//***************************************************************************
// Get Title
//***************************************************************************

const char* cLockMenuItem::GetTitle()
{ 
   if (item->GetTitle())
      return item->GetTitle();

   return item->GetName();
}

//***************************************************************************
// Lock Edit Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cLockEditMenu::cLockEditMenu(cLockItem* aItem)
{
   char strBuff[512];
   item = aItem;
   strncpy(name, item->GetName(), sizeNameMax);
   name[sizeNameMax] = 0;
   active = item->GetActive();
   
//   Add(new cMenuEditStrItem(tr("Name"), name, sizeNameMax, editCharacters));
   Add(new cMenuEditBoolItem(tr("Lock active"), &active));

   if (item->HasRange())
   {
      long l;

      l = item->GetStart();
      start = (l/60/60) * 100 + l/60%60;
      l = item->GetEnd();
      end = (l/60/60) * 100 + l/60%60;

      Add(new cMenuEditTimeItem(tr("Start"), &start));
      Add(new cMenuEditTimeItem(tr("Stop"), &end));
   }

   sprintf(strBuff, " %s", item->GetTitle() ? item->GetTitle() : item->GetName());
   SetTitle(strBuff);
}

//***************************************************************************
// Store
//***************************************************************************

void cLockEditMenu::Store(void)
{
   item->SetName(name);
   item->SetActive(active);

   if (item->HasRange())
   {
      item->SetStart((start/100)*60*60 + (start%100)*60);
      item->SetEnd((end/100)*60*60 + (end%100)*60);
   }
}

//***************************************************************************
// Broadcast Edit Menu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cBroadcastEditMenu::cBroadcastEditMenu(cLockedBroadcast* aItem)
   : cLockEditMenu((cLockItem*)aItem)
{
   static const char* trSearchModes[cLockedBroadcast::smCount] = { 0 };

   strncpy(pattern, aItem->GetPattern(), sizePatternMax);
   pattern[sizePatternMax] = 0;
   searchMode = aItem->SearchMode();

   // translate search modes

   if (!trSearchModes[0])
      for (int i = 0; i < cLockedBroadcast::smCount; i++)
         trSearchModes[i] = tr(cLockedBroadcast::searchModes[i]);

   // add menu items

   Add(new cMenuEditStrItem(tr("Keyword"), pattern, sizePatternMax, editCharacters));
	Add(new cMenuEditStraItem(tr("Search mode"), &searchMode, cLockedBroadcast::smCount, trSearchModes));
}

//***************************************************************************
// Store
//***************************************************************************

void cBroadcastEditMenu::Store(void)
{
   cLockEditMenu::Store();

   ((cLockedBroadcast*)item)->SetPattern(pattern);
   ((cLockedBroadcast*)item)->SetSearchMode(searchMode);
}
