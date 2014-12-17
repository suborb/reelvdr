//***************************************************************************
/*
 * common.cc: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include "common.hpp"

#ifdef WITH_VDR
#  include <vdr/skins.h>
#endif

//***************************************************************************
// Statics
//***************************************************************************

int Cs::eloquence = eloScreen;
int Cs::device = dDefault;
char Cs::tellBuffer[] = "";
char Cs::tellPrefix[] = "";

//***************************************************************************
// Tell
//***************************************************************************

// eKeys Message(eMessageType Type, const char *s, int Seconds = 0);
// enum eMessageType { mtStatus = 0, mtInfo, mtWarning, mtError }; 

int Cs::tell(int elo, const char* format, ...)
{
   va_list more;

   if (isEmpty(format) || (elo < eloAlways && elo < eloquence))
      return done;

   // build buffer

   va_start(more, format);
   sprintf(tellBuffer, "%s", tellPrefix);
   vsprintf(tellBuffer+strlen(tellBuffer), format, more);

#ifdef WITH_VDR

   // show on VDR screen

   if (elo >= eloScreen && device & dScreen)
      Skins.Message((eMessageType)(elo-eloScreen), tellBuffer+strlen(tellPrefix));

   // Im syslog ausgeben
   // mtInfo und mtError werden bereits von Message(..) im syslog ausgegeben,
   // mtWarning und mtStatus hingegen nicht.
   // Warum das da so durcheinander ist ... ?

   if (device & dSyslog && elo != eloError && elo != eloInfo)
      syslog(elo > eloDetail ? LOG_INFO : LOG_DEBUG, tellBuffer);

   // show on console

   if (device & dConsole)
      printf("%s\n", tellBuffer);

#else
   
   printf("%s\n", tellBuffer);

#endif

   return done;
}
