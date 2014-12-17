//***************************************************************************
/*
 * commpon.hpp - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef VDRVERSNUM
#  define WITH_VDR
#endif

//***************************************************************************
// Includes
//***************************************************************************

#include <string.h>

//***************************************************************************
// Definitions
//***************************************************************************

#define TB 1

//***************************************************************************
// Return Values
//***************************************************************************

enum Return
{
   fail = -1,
   ignore = fail,
   na = fail,

   success = 0,
   ok = success,
   done = success
};

//***************************************************************************
// Switches
//***************************************************************************

enum Switches
{
   // true/false and similiar switches

   no       = false,    yes   = !no,
   off      = false,    on    = !off,
   invalid  = false,    valid = !invalid,
   unknown  = false,    known = !unknown,

   // more switches

   absolute = 0, relative = 1
};

//***************************************************************************
// Common Sizes
//***************************************************************************

enum CommonSize
{
   sizePath     = 500
};

//***************************************************************************
// Common Service
//***************************************************************************

class Cs
{
   public:

      enum Eloquence
      {
         eloDebug  = 0,              // tell to syslog
         eloDetail,                  //
         eloAlways,                  // tell always ..

         eloScreen,                  // tell to screen (in addition)

         eloStatus = eloScreen,      //   "       "
         eloInfo,                    //   "       "
         eloWarning,                 //   "       "
         eloError                    //   "       "
      };

      enum Device
      {
         dScreen  = 1,
         dSyslog  = 2,
         dConsole = 4,

         dDefault = dScreen + dSyslog
      };

      static int isEmpty(const char* s)        { return !s || !*s; };

      // tell stuff

      static void setEloquence(int aEloquence)       { eloquence = aEloquence; }
      static void setTellDevice(int aDevice)         { device = aDevice; }
      static void setTellPrefix(const char* aPrefix) { sprintf(tellPrefix, "[%s] ", aPrefix); }
      static const char* getTellPrefix()             { return tellPrefix; }

      static int tell(int elo = eloInfo, const char* format = "", ...);

      // data
      
      static int eloquence;
      static int device;
      static char tellBuffer[1000+TB];
      static char tellPrefix[100+TB];
};

//***************************************************************************
#endif // __COMMON_H__
