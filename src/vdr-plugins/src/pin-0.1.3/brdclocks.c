/*
 * pin.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 25.01.2006, horchi
 */

//***************************************************************************
// Includes
//***************************************************************************

#include "locks.h"
#include "rep.h"
#include "def.h"

//***************************************************************************
// Statics
//***************************************************************************

const char* cLockedBroadcast::searchModes[] = 
{
   "regular expression",
   "includes",
   "exact",
   0
};

//***************************************************************************
// Object
//***************************************************************************

cLockedBroadcast::cLockedBroadcast()
   : cLockItem(yes)
{
   pattern = 0;
   searchMode = smExact;
   hasRange = no;
}

cLockedBroadcast::cLockedBroadcast(const char* aName)
   : cLockItem(aName, yes)
{
   asprintf(&pattern, "%s", aName);
   searchMode = smExact;
   hasRange = no;
}

cLockedBroadcast::~cLockedBroadcast()
{
   if (pattern) free(pattern);
}

void cLockedBroadcast::SetPattern(const char* aPattern)
{ 
   if (pattern) free(pattern);
   asprintf(&pattern, "%s", aPattern);
}

//***************************************************************************
// Parse
//***************************************************************************

bool cLockedBroadcast::Parse(char* line) 
{
   int fields;

   char* aName = 0;
   char* aPattern = 0;
   char* aSearchMode = 0;
   char* aActive = 0;

   fields = sscanf(line, "%a[^:]:%a[^:]:%a[^:]:%a[^\n]", 
                   &aName, &aPattern, &aSearchMode, &aActive);

   if (fields == 4)
   {
      if (name) free(name);
      name = aName;

      if (pattern) free(pattern);
      pattern = aPattern;

      for (int i = 0; i < smCount; i++)
      {
         if (strcmp(searchModes[i], aSearchMode) == 0)
         {
            searchMode = i;
            break;
         }
      }

      active = strcmp(aActive, "yes") == 0;
   }
   
   if (aSearchMode) free(aSearchMode);
   if (aActive) free(aActive);

   return fields == 4;
}

//***************************************************************************
// Save
//***************************************************************************

bool cLockedBroadcast::Save(FILE* file)
{
   // Format: "<name>:<pattern>:<searchMode>:<active>"

   return fprintf(file, "%s:%s:%s:%s\n", 
                  name,
                  pattern,
                  searchModes[searchMode],
                  active ? "yes" : "no") > 0;
}

//***************************************************************************
// Locked
//***************************************************************************

int cLockedBroadcast::Locked(long startTime)
{
   return active;
}

//***************************************************************************
// Match Pattern
//***************************************************************************

bool cLockedBroadcast::MatchPattern(const char* aPattern)
{
	if (searchMode == smRegexp)
		return rep(pattern, aPattern) == success;
   else if (searchMode == smExact)
      return strcmp(pattern, aPattern) == 0;
   else if (searchMode == smInclude)
      return strstr(pattern, aPattern) > 0;

   return no;
}

//***************************************************************************
// Class cLockedBroadcasts
//***************************************************************************
//***************************************************************************
// Locked
//***************************************************************************

int cLockedBroadcasts::Locked(const char* aPattern, long startTime)
{
   cLockedBroadcast* broadcast;

   if (!aPattern)
      return no;

   for (broadcast = First(); broadcast; broadcast = Next(broadcast))
   {
      if (broadcast->MatchPattern(aPattern))
         if (broadcast->Locked())
            return yes;
   }
   
   return no;
}
