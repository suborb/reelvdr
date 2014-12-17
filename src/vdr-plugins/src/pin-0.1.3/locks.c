/*
 * pin.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 11.04.05 - 27.12.05, horchi
 */

//***************************************************************************
// Includes
//***************************************************************************

#include "locks.h"
#include "def.h"

//***************************************************************************
// Object
//***************************************************************************

cLockItem::cLockItem(int aActive)
{
   name = 0;
   title = 0;
   active = aActive;
   hasRange = yes;
   start = 0;
   end = 0;
}

cLockItem::cLockItem(const char* aName, int aActive, const char* aTitle)
{
   asprintf(&name, "%s", aName);
   active = aActive;
   hasRange = yes;
   title = 0;
   start = DT::hhmm2Int("0000");
   end = DT::hhmm2Int("2359");

   if (aTitle)
      asprintf(&title, "%s", aTitle);
}

cLockItem::~cLockItem()
{
   if (name) free(name);
   if (title) free(title);
}

//***************************************************************************
// Name
//***************************************************************************

void cLockItem::SetName(const char* aName)
{ 
   if (!aName) return;

   if (name) free(name);
   asprintf(&name, "%s", aName);
}

//***************************************************************************
// Title
//***************************************************************************

void cLockItem::SetTitle(const char* aTitle)
{ 
   if (!aTitle) return;

   if (title) free(title);
   asprintf(&title, "%s", aTitle);
}

//***************************************************************************
// Parse
//***************************************************************************

bool cLockItem::Parse(char* line) 
{
   int fields;

   char* aName = 0;
   char* aActive = 0;
   char* aStart = 0;
   char* aEnd = 0;

   fields = sscanf(line, "%a[^:]:%a[^:]:%a[^:]:%a[^\n]", 
                   &aName, &aActive, &aStart, &aEnd);

   if (fields == 4)
   {
      if (name) free(name);
      name = aName;
      active = strcmp(aActive, "yes") == 0;
      start = DT::hhmm2Int(aStart);
      end = DT::hhmm2Int(aEnd);
   }
   
   if (aActive) free(aActive);
   if (aStart)  free(aStart);
   if (aEnd)    free(aEnd);

   return fields == 4;
}

//***************************************************************************
// Save
//***************************************************************************

bool cLockItem::Save(FILE* file)
{
   char aStart[sizeHHMM+TB];
   char aEnd[sizeHHMM+TB];

   DT::int2Hhmm(start, aStart);
   DT::int2Hhmm(end, aEnd);

   // Format: "<name>:<active>:<start>:<end>"

   return fprintf(file, "%s:%s:%s:%s\n", 
                  name, 
                  active ? "yes" : "no", 
                  aStart, aEnd) > 0;
}

//***************************************************************************
// Locked
//***************************************************************************

int cLockItem::Locked(long startTime)
{
   long sec;
   char ct[sizeHHMM+TB];
   long theTime;

   if (startTime)
      theTime = startTime;
   else
      theTime = DT::lNow();        // get now in seconds

   // get seconds from today 00:00 in actual timezone

   #ifdef __DEBUG__
     dsyslog("Pin: Checking protection at '%s'", DT::int2Hhmm(theTime, ct));
   #endif

   sec = DT::hhmm2Int(DT::int2Hhmm(theTime, ct));

   if (sec >= start && sec <= end && active)
      return yes;
   
   return no;
}

//***************************************************************************
// Has Name
//***************************************************************************

bool cLockItem::HasName(const char* aName)
{
   return strcmp(name, aName) == 0;
}

//***************************************************************************
// Class cLockItems
//***************************************************************************
//***************************************************************************
// Locked
//***************************************************************************

int cLockItems::Locked(const char* aName, long startTime)
{
   cLockItem* item = FindByName(aName);

   if (!aName)
      return no;

   if (item && item->Locked(startTime))
      return yes;
   
   return no;
}
