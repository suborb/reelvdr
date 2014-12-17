/*
 * diseqc.c: DiSEqC handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: diseqc.c 1.5 2005/12/30 15:41:48 kls Exp $
 */

#include <ctype.h>
#include "tools.h"
#include "diseqc.h"
#include "sources.h"
#include "thread.h"
#include "config.h"

#define COMMANDS_MAX_LENGTH 400

#define DBG " DEBUG DiSEqC -- "
//#define DEBUG_DISEQC


#if defined DEBUG_DISEQC 
#   define DLOG(x...) dsyslog(x)
#   define DPRINT(x...) fprintf(stderr,x)
#else
# define DPRINT(x...)
# define DLOG(x...)
#endif

// -- cDiseqc ----------------------------------------------------------------

cDiseqc::cDiseqc(void)
{
  lnbType = -1;
  tuner = 0;
  satName = NULL;
  commands = NULL;
  source = 0;
  parsing = false;
  numCodes = 0;
  repeat = 0;
}

cDiseqc::cDiseqc(int Source)
:source(Source)
{
  asprintf(&satName,"%s",*cSource::ToString(source));
  lnbType = -1;
  tuner = 0;
  commands = NULL;
  parsing = false;
  numCodes = 0;
  repeat = 0;
}

cDiseqc::cDiseqc(int Source , int LnbType, int Tuner)
:source(Source),lnbType(LnbType),tuner(Tuner)
{
  asprintf(&satName,"%s",*cSource::ToString(source));
  DLOG(DBG" new DiSEqC %s", *cSource::ToString(source));
  commands = NULL;
  parsing = false;
  numCodes = 0;
  repeat = 0;
  SetLof();
}

cDiseqc::~cDiseqc()
{
  free(satName);
  free(commands);
}

void cDiseqc::SetLof()
{
  DLOG(" SetLof %d",lnbType);
  switch (lnbType)
  {
     case 0:
           lofLo = 9750;
           lofHi = 10600;
           lofThreshold = 11700;
           break;
     case 1:
           lofLo = 10750;
           lofHi = 11250;
           lofThreshold = 11700;
           break;
     case 2:
           lofLo = 0;
           lofHi = 5150;
           lofThreshold = 0;
           break;
     case 3:
           lofLo = 0;
           lofHi = 9750;
           lofThreshold = 0;
           break;
     case 4:
           lofLo = 0;
           lofHi = 106000;
           lofThreshold = 0;
           break;
     case 5:
           lofLo = 0;
           lofHi = 11250;
           lofThreshold = 0;
           break;
     case 6:
           lofLo = 0;
           lofHi = 11475;
           lofThreshold = 0;
           break;
     default:
           esyslog ("ERROR: Unknown DiSEqC Type");
           lofLo = 9750;
           lofHi = 10600;
           lofThreshold = 11700;
           break;
     }
}

bool cDiseqc::Parse(const char *s, bool Count)
{
  DLOG(DBG "PARSE \"%s\"  ", s);
  bool result = false;
  repeat = 0;
  tuner = 0;

  if (satName) {
     free(satName);
     satName = NULL;
  }

  if (strchr(s,'A') && strchr(s,'A') < strchr(s,'S'))
    tuner = atoi(strchr(s,'A')+1);

  DLOG (DBG " DisqcWaits %d  ", Diseqcs.WaitMs(tuner));
  DLOG (DBG " HAS TUNER? %s", tuner?"YES":"NO");
  
  int fields = sscanf(strchr(s,'S'), "%a[^ ] %d %c %d %a[^\n]", &satName, &lofThreshold, &polarization, &lof, &commands);

  DLOG(" Parse T:%d SatName: %s  lof: %d slof: %d Cmd: %s\n", tuner, satName, lof, lofThreshold, commands);
  if (fields == 4)
     commands = NULL; //XXX Apparently sscanf() doesn't work correctly if the last %a argument results in an empty string
  if (!strchr(commands,'W')) //  if SatPos west? 
     Diseqcs.SetWaitMs(0, tuner);

  DLOG (DBG " Parse .. commands:  %s ", commands);
  if (4 <= fields && fields <= 5) {
     source = cSource::FromString(satName);
     if (Sources.Get(source) || source == cSource::stSat) {
        DLOG (DBG " Parse .. Have Source "); 
        polarization = toupper(polarization);
        if (polarization == 'V' || polarization == 'H' || polarization == 'L' || polarization == 'R') {
           parsing = true;
           repeat = -1;
           char *CurrentAction = NULL;
           while (Execute(&CurrentAction) != daNone)
                 ;
           parsing = false;
           result = !commands || !*CurrentAction;
           }
        else
           esyslog("ERROR: unknown polarization '%c'", polarization);
        }
     else
        esyslog("ERROR: unknown source '%s'", satName);
     }
  DLOG("repeat %d\n",Diseqcs.RepeatCmd(tuner));
  DLOG("repeat %s\n",satName);

  return result;
}

char *cDiseqc::Wait(char *s)
{
  char *p = NULL;
  errno = 0;
  int n = strtol(s, &p, 10);
  if (!errno && p != s && n >= 0) {
     Diseqcs.SetWaitMs(n, tuner);
     if (!parsing)
        cCondWait::SleepMs(n);
     return p;
     }
  esyslog("ERROR: invalid value for wait time in '%s'", s - 1);
  return NULL;
}

char *cDiseqc::Codes(char *s)
{
  char *e = strchr(s, ']');
  if (e) {
     numCodes = 0;
     char *t = s;
     char *p = s;
     while (t < e) {
           if (numCodes < MaxDiseqcCodes) {
              errno = 0;
              int n = strtol(t, &p, 16);
              if (!errno && p != t && 0 <= n && n <= 255) {
                 codes[numCodes++] = n;
                 t = skipspace(p);
                 }
              else {
                 esyslog("ERROR: invalid code at '%s'", t);
                 return NULL;
                 }
              }
           else {
              esyslog("ERROR: too many codes in code sequence '%s'", s - 1);
              return NULL;
              }
           }
     return e + 1;
     }
  else
     esyslog("ERROR: missing closing ']' in code sequence '%s'", s - 1);
  return NULL;
}

cDiseqc::eDiseqcActions cDiseqc::Execute(char **CurrentAction)
{
  if (!*CurrentAction)
     *CurrentAction = commands;
  //dsyslog ("[Diseqc] CurrentAction %s",commands);
  while (*CurrentAction && **CurrentAction) {
        switch (*(*CurrentAction)++) {
          case ' ': break;
          case 't': return daToneOff;
          case 'T': return daToneOn;
          case 'v': return daVoltage13;
          case 'V': return daVoltage18;
          case 'A': return daMiniA;
          case 'B': return daMiniB;
          case 'W': *CurrentAction = Wait(*CurrentAction); break;
          case '[': *CurrentAction = Codes(*CurrentAction);
          if(parsing)
              repeat++;

          Diseqcs.SetRepeatCmd(repeat, tuner);

          return *CurrentAction ? daCodes : daNone;
          default: return daNone;
          }
        }
  return daNone;
}

#if 0
cString cDiseqc::ToText(const cDiseqc *Diseqc)
{
/* Syntax:     single LNB        | mini | all            full                 mini         mini
 *   | S19.2E  | 11700 |  V  | 9750 |   t  |   v  | W15  | [E0 10 38 F0] | W15  |   A   |  W15  | t
 *   | SatCode | SLOF  | POL | LOF  | tone | volt | Wait |  diseqc code  | Wait | MinAB |  Wait | tone
 *   |
 * 1.|   --    |  low  |  V  |  low |  x   |  13  |  x   | [E0 10 38 x ]  | x    |   x   | t
 * 2.|   --    |   hi  |  V  |  hi  |  x   |  13  |  x   | [E0 10 38 x+1] | x    |   x   | T
 * 3.|   --    |  low  |  H  |  low |  x   |  18  |  x   | [E0 10 38 x+2] | x    |   x   | t
 * 3.|   --    |   hi  |  H  |  hi  |  x   |  18  |  x   | [E0 10 38 x+3] | x    |   x   | T
 */


   char *s, fullString[255];
   s = fullString;

   //dsyslog( " ToText SatName : %s /%s  Source : %d ", Diseqc->SatName(),Diseqc->SatName(), Diseqc->Source() );

   char Atuner[6] = "";
   if (tuner)
     snprintf(Atuner,6,"A%d ",tuner)

   s += sprintf(s,"%s%s  %5d ", Atuner, satName, lofThreshold);
   s += sprintf(s,"%c  %5d ", polarization, lof);
   s += sprintf(s,"%s", commands);
   s += sprintf(s,"\n");
   *s = 0;

   DLOG (DBG " schreibe %s ", fullString);
   return cString(fullString,false);
}
#endif

cString cDiseqc::ToText(const cDiseqc *Diseqc)
{
/* Syntax:     single LNB        | mini | all            full                 mini         mini
 *   | S19.2E  | 11700 |  V  | 9750 |   t  |   v  | W15  | [E0 10 38 F0] | W15  |   A   |  W15  | t
 *   | SatCode | SLOF  | POL | LOF  | tone | volt | Wait |  diseqc code  | Wait | MinAB |  Wait | tone
 *   |
 * 1.|   --    |  low  |  V  |  low |  x   |  13  |  x   | [E0 10 38 x ]  | x    |   x   | t
 * 2.|   --    |   hi  |  V  |  hi  |  x   |  13  |  x   | [E0 10 38 x+1] | x    |   x   | T
 * 3.|   --    |  low  |  H  |  low |  x   |  18  |  x   | [E0 10 38 x+2] | x    |   x   | t
 * 3.|   --    |   hi  |  H  |  hi  |  x   |  18  |  x   | [E0 10 38 x+3] | x    |   x   | T
 */
   DLOG (DBG " DiseqcNr %d / %d  ToText  Count %d  ",  Diseqc->Index(), Diseqc->Source(), Diseqcs.Count());


   char *s, fullString[255];
   s = fullString;

   //dsyslog( " ToText SatName : %s /%s  Source : %d ", Diseqc->SatName(),Diseqc->SatName(), Diseqc->Source() );

   char Atuner[6] = "";
   if (Diseqc->Tuner())
     snprintf(Atuner,6,"A%d ",Diseqc->Tuner());

   s += sprintf(s,"%s%s  %5d ", Atuner, Diseqc->SatName(), Diseqc->Slof());
   s += sprintf(s,"%c  %5d ", Diseqc->polarization,Diseqc->Lof());
   s += sprintf(s,"%s",Diseqc->Commands());
   s += sprintf(s,"\n");
   *s = 0;

   //DLOG (DBG " write -> %s  ", fullString);
   
   char *buffer=NULL;
   if (Diseqc->Index() == 0) {
     DLOG (DBG " HEY  Erste Zeile, schreib was rein -> PrintHeader() ! ");
     char *sep = Seperator(Diseqc);
     //asprintf (&buffer, "%s \n%s\n%s\n",HeadLine(), *Seperator(Diseqc), fullString);  
     asprintf (&buffer, "%s \n%s\n%s",HeadLine(), sep, fullString); 
     DLOG (DBG " Diseqc ToText returns %s", buffer);
     free(sep);
     return cString(buffer, true);
     
   }
   else if (Diseqc->Source() != Diseqcs.Prev(Diseqc)->Source()) {
        char *sep = Seperator(Diseqc);
        //asprintf (&buffer, "\n%s\n%s\n", *Seperator(Diseqc), fullString);  
        asprintf (&buffer, "\n%s\n%s", sep, fullString);  
        DLOG (DBG " HO   Neuer LNB,   -> %s  ",buffer);
        free(sep);
        DLOG (DBG " Diseqc ToText returns %s", buffer);
        return cString(buffer, true);
   }

   return cString(fullString);
}

bool cDiseqc::SetFullDiseqCommands(int Line)
{
/*
# S19.2E  11700 V  9750 t v [E0 10 38 F0] t    lnb 0 +line 0
# S19.2E  99999 V 10600 t v [E0 10 38 F1] T X  lnb 0 +line 1
# S19.2E  11700 H  9750 t V [E0 10 38 F2] t    lnb 0 +line 2
# S19.2E  99999 H 10600 t V [E0 10 38 F3] T X  lnb 0 +line 3  || if lofHi == 0   lnb 0 + 1  F1
#
# S21.5E  11700 V  9750 t v [E0 10 38 F4]     lnb1 *4  +line 0
# S21.5E  99999 V 10600 t v [E0 10 38 F5]  X  lnb1 *4  +line 1
# S21.5E  11700 H  9750 t V [E0 10 38 F6]     lnb1 *4  +line 2
# S21.5E  99999 H 10600 t V [E0 10 38 F7]  X  lnb1 *4  *line 3  || lofHi == 0 lnb 1 *4 +1

# S28.3E  11700 V  9750 t [E0 10 38 F8]     lnb2 *4 + line 0
# S28.3E  99999 V 10600 t [E0 10 38 F9]  X
# S28.3E  11700 H  9750 t [E0 10 38 FA]
# S28.3E  99999 H 10600 t [E0 10 38 FB]  X


# EchoStar 7 - 119W - Port 1
S119.0W 99999 V 11250 t v W15 [E0 10 38 F1]
S119.0W 99999 H 11250 t V W15 [E0 10 38 F1]

# EchoStar 6/8 - 110W - Port 2
S110.0W 99999 V 11250 t v W15 [E0 10 38 F5]
S110.0W 99999 H 11250 t V W15 [E0 10 38 F5]

# Nimiq 1 - 91W - Port 3
S91.0W 99999 V 11250 t v W15 [E0 10 38 F9]
S91.0W 99999 H 11250 t V W15 [E0 10 38 F9]

*/

  DLOG(" SetFullDiseqCommands line %d LnbNum %d",Line, Diseqcs.LnbCount());

  int line  = Line;

  char waitStr[6];
  if (Diseqcs.WaitMs(tuner) != 0)
    snprintf(waitStr,5,  "W%d  ", Diseqcs.WaitMs(tuner));
  else
    strcpy(waitStr,"");


  int lnbNr = Diseqcs.LnbCount(tuner) -1;


  char buffer[COMMANDS_MAX_LENGTH];
  char buf[COMMANDS_MAX_LENGTH/4];
  char Atuner[6] = "";
  if (tuner)
     snprintf(Atuner,6,"A%d ",tuner);
  switch (Line) {
     case 0:
           if (lofLo == 0) {
              DLOG(" Single frequence band LNB ");
              return false;
           }
           snprintf(buffer,100, "%s%s  %d V  %d  t v %s[E0 10 38 F%X]", Atuner, satName, lofThreshold, lofLo, waitStr, lnbNr*4+line);
           for (int i =0; i< Diseqcs.RepeatCmd(tuner); i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 38 F%X]", waitStr, lnbNr*4+line);
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
               }

           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s t", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           Parse(buffer);
           return true;

     case 1:
           snprintf(buffer,100, "%s%s  99999 V  %d t v %s[E0 10 38 F%X]",Atuner, satName, lofHi, waitStr, lnbNr*4+line);
           for (int i =0; i< Diseqcs.RepeatCmd(tuner); i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 38 F%X]", waitStr, lnbNr*4+line);
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
             }
           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s T", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           Parse(buffer);
           return true;

     case 2:
           if (lofLo == 0) return false;
           snprintf(buffer,100, "%s%s  %d H  %d  t V %s[E0 10 38 F%X]",Atuner, satName, lofThreshold, lofLo, waitStr, lnbNr*4+line);
           for (int i =0; i< Diseqcs.RepeatCmd(tuner); i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 38 F%X]", waitStr, lnbNr*4+line);
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
              }
           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s t", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           Parse(buffer);
           return true;

     case 3:
           snprintf(buffer,100, "%s%s  99999 H  %d  t V %s[E0 10 38 F%X]",Atuner, satName, lofHi, waitStr, lnbNr*4+line);
           for (int i =0; i< Diseqcs.RepeatCmd(tuner); i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 38 F%X]", waitStr, lnbNr*4+line);
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
              }
           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s T", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           Parse(buffer);
           return true;
     default:
           esyslog ("ERROR: check  DiSEqC  handling");
    }
    return false;

}

bool cDiseqc::SetDisiCon4Commands(int Line)
{
/*
  Only DisiCon
# S19.2E  99999 H 10560 t v
# S19.2E  12110 V 11080 t v
# S19.2E  99999 V 10720 t v
* */

  DLOG("SetDisiCon4Commands Line %d", Line);

  char buffer[100];
  char Atuner[6] = "";
  if (tuner)
    snprintf(Atuner,6,"A%d ",tuner);
  switch (Line) {
    case 0:
           snprintf(buffer,100,"%s%s 99999 H 10560 t v",Atuner,satName);
           Parse(buffer);
           break;
    case 1:
           snprintf(buffer,100,"%s%s 12110 V 11080 t v",Atuner,satName);
           Parse(buffer);
           break;
    case 2:
           snprintf(buffer,100,"%s%s 99999 V 10720 t v",Atuner,satName);
           Parse(buffer);
           break;
    default:
           esyslog ("ERROR: check  DiSEqC  handling");
    }
  return true;
}

bool cDiseqc::SetMiniDiseqCommands(int Line)
{

/*
  Mini Diseq
S19.2E  11700 V  9750  t v W15 A W15 t
S19.2E  99999 V 10600  t v W15 A W15 T
S19.2E  11700 H  9750  t V W15 A W15 t
S19.2E  99999 H 10600  t V W15 A W15 T

S119.0W 99999 V 11250  t v W15 B W15 T
S119.0W 99999 H 11250  t V W15 B W15 T

# EchoStar 7 - 119W - Port 1
S119.0W 99999 V 11250 t v W15 A W15 T
S119.0W 99999 H 11250 t V W15 A W15 T
*/
 
  DLOG("SetMiniCommands Line %d LNB: %d, Count %d ", Line, Diseqcs.LnbCount(), Diseqcs.Count());

  /*
  char satName[10];
  snprintf("%s",10, *cSource::ToString(source));
  */


  char AB = Diseqcs.LnbCount(tuner)==1 ?'A':'B';
  int wait = Diseqcs.WaitMs(tuner)?Diseqcs.WaitMs(tuner):15; // XXX
  dsyslog ("SetMiniCommands SatName  %s  wait W%d Line %d  lofHi %d ",satName , wait, Line, lofHi);

  char buffer[100];
  char Atuner[6] = "";
  if (tuner)
    snprintf(Atuner,6,"A%d ",tuner);
  switch (Line) {
    case 0:
           if (lofLo == 0) return false;
           snprintf(buffer,100,"%s%s  %d V  %d  t v W%d %c W%d t", Atuner, satName, lofThreshold, lofLo,wait,AB,wait);
           Parse(buffer);
           return true;
    case 1:
           snprintf(buffer,100,"%s%s  99999 V  %d  t v W%d %c W%d T", Atuner,  satName, lofHi, wait, AB, wait);
           Parse(buffer);
           return true;
    case 2:
           if (lofLo == 0) return false;
           snprintf(buffer,100,"%s%s  %d H  %d  t V W%d %c W%d t", Atuner, satName, lofThreshold, lofLo, wait, AB,  wait);
           Parse(buffer);
           return true;
    case 3:
           snprintf(buffer,100,"%s%s  99999 H  %d  t V W%d %c W%d T", Atuner, satName, lofHi, wait, AB, wait);
           Parse(buffer);
           return true;
    default:
           esyslog ("ERROR: check  DiSEqC  handling");
           return false;
    }
   return false;
}

bool cDiseqc::SetNoDiseqcCommands(int Line)
{
/*
  Only LNB settings
# S19.2E  11700 V  9750 t v
# S19.2E  99999 V 10600 T v
# S19.2E  12110 H  9750 t V
# S19.2E  99999 H 10600 T V
* */

  char buffer[100];
  char Atuner[6] = "";
  if (tuner)
    snprintf(Atuner,6,"A%d ",tuner);
  switch (Line) {
    case 0:
           snprintf(buffer,100,"%s%s %d V %d t v",Atuner,satName,lofThreshold, lofLo);
           Parse(buffer);
           break;
    case 1:
           snprintf(buffer,100,"%s%s 99999 V %d T v",Atuner,satName, lofHi);
           Parse(buffer);
           break;
    case 2:
           snprintf(buffer,100,"%s%s %d H %d t V",Atuner,satName, lofThreshold,lofLo);
           Parse(buffer);
           break;
    case 3:
           snprintf(buffer,100,"%s%s 99999 H %d T V",Atuner,satName, lofHi);
           Parse(buffer);
           break;
    default:
           esyslog ("ERROR: check  DiSEqC  handling");
    }
  return true;
}



bool cDiseqc::SetFullDiseq1_1Commands(int Sequence)
{

/*

input 1 - Astra 1 19.2E - uncommited switch input 1 every 4th Lnb  F0 +1
                    immer [t], [vV]                    this wait 3xlonger
S19.2E  11700 V  9750  t v W15 [E0 10 39 F0] W45 [E0 10 38 F0] W15 t  || every LNB +4
S19.2E  99999 V 10600  t v W15 [E0 10 39 F0] W45 [E0 10 38 F0] W15 T
S19.2E  11700 H  9750  t V W15 [E0 10 39 F0] W45 [E0 10 38 F0] W15 t
S19.2E  99999 H 10600  t V W15 [E0 10 39 F0] W45 [E0 10 38 F0] W15 T

# Input 2 - Hostbird 13E - uncommited switch input 1

S13E  11700 V  9750  t v W15 [E0 10 39 F0] W45 [E0 10 38 F4] W15 t
S13E  99999 V 10600  t v W15 [E0 10 39 F0] W45 [E0 10 38 F4] W15 T
S13E  11700 H  9750  t V W15 [E0 10 39 F0] W45 [E0 10 38 F4] W15 t
S13E  99999 H 10600  t V W15 [E0 10 39 F0] W45 [E0 10 38 F4] W15 T

*/

  DLOG(" SetFullDiseq_1_1Commands line %d LnbNum %d",Sequence, Diseqcs.LnbCount());

  int line  = Sequence;
  char waitStr[6] ="";
  char longWaitStr[6] = "";
  int rep = Diseqcs.RepeatCmd(tuner);

  dsyslog (" LNB repeat   %d ", rep); 
  rep = 0; //FIXME
  
  if (Diseqcs.WaitMs(tuner) > 0)
  {
    snprintf(waitStr,5, "W%-4d  ", Diseqcs.WaitMs(tuner));
    snprintf(longWaitStr,5, "W%-4d  ", 3 * Diseqcs.WaitMs(tuner));
  }
  else
  {
    strcpy(waitStr,"W15");
    strcpy(longWaitStr,"W45");
  }


  int lnbNr = Diseqcs.LnbCount(tuner) -1;

  char buffer[COMMANDS_MAX_LENGTH];
  char buf[COMMANDS_MAX_LENGTH/4];
  char Atuner[6] = "";
  if (tuner)
     snprintf(Atuner,6,"A%d ",tuner);

  switch (Sequence) {
     case 0:
           if (lofLo == 0) {

              dsyslog (" Single frequence band LNB ");
              return false;
           }
           snprintf(buffer,COMMANDS_MAX_LENGTH , "%s%s  %d V  %d  t v %s[E0 10 39 F%X] %s [E0 10 38 F%X] ", 
                           Atuner, satName, lofThreshold, lofLo, waitStr, lnbNr / 4, longWaitStr, DirectAddr_1_1(lnbNr,line));
           for (int i=0; i< rep; i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 39 F%X]  %s [E0 10 38 F%X]", waitStr, lnbNr/4, longWaitStr, DirectAddr_1_1(lnbNr,line));
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
               }

           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s t", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           DLOG(" Parse Buffer Line 1  %s \n", buffer);
           Diseqcs.SetRepeatCmd(0, tuner);  
           Parse(buffer);
           return true;

     case 1:
           snprintf(buffer,COMMANDS_MAX_LENGTH, "%s%s  99999 V  %d t v %s[E0 10 39 F%X] %s [E0 10 38 F%X] ", 
                                  Atuner, satName, lofHi, waitStr, lnbNr / 4, longWaitStr, DirectAddr_1_1(lnbNr,line));
           for (int i =0; i< rep; i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 39 F%X]  %s [E0 10 38 F%X]", waitStr, lnbNr/4, longWaitStr, DirectAddr_1_1(lnbNr,line));
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
             }
           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s T", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
           DLOG(" Parse Buffer Line 2  %s \n", buffer);
           Parse(buffer);
           return true;

     case 2:
           if (lofLo == 0) return false;

           snprintf(buffer,COMMANDS_MAX_LENGTH , "%s%s  %d H  %d  t V %s[E0 10 39 F%X] %s [E0 10 38 F%X] ", 
                                  Atuner, satName, lofThreshold, lofLo, waitStr, lnbNr / 4, longWaitStr, DirectAddr_1_1(lnbNr,line));
           for (int i=0; i< rep; i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 39 F%X]  %s [E0 10 38 F%X]", waitStr, lnbNr/4, longWaitStr, DirectAddr_1_1(lnbNr,line));
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
               }

           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s t", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);

           DLOG (" Parse Buffer  Line 3 %s \n", buffer);
           Parse(buffer);
           return true;

     case 3:
           snprintf(buffer,COMMANDS_MAX_LENGTH, "%s%s  99999 H  %d t V %s[E0 10 39 F%X] %s [E0 10 38 F%X] ", 
                                  Atuner, satName, lofHi, waitStr, lnbNr / 4, longWaitStr, DirectAddr_1_1(lnbNr,line));
           for (int i =0; i< rep; i++) {
               snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s[E1 10 39 F%X]  %s [E0 10 38 F%X]", waitStr, lnbNr/4, longWaitStr, DirectAddr_1_1(lnbNr,line));
               strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
             }
           snprintf(buf, COMMANDS_MAX_LENGTH/4, " %s T", waitStr);
           strncat(buffer, buf, COMMANDS_MAX_LENGTH - strlen(buffer) - 1);
           DLOG (" Parse Buffer Line 4 %s \n", buffer);
           Parse(buffer);
           return true;
    }
    return false;
}



cString cDiseqc::ToText(void)
{
   return ToText(this);
}

bool cDiseqc::Save(FILE *f)
{
   dsyslog ("Save(FILE): %s ", *ToText());
   return fprintf(f, "%s", *ToText()) > 0;

} 


const char *cDiseqc::HeadLine()
{

return 
  "# DiSEqC configuration for VDR\n"
  "#\n"
  "# Format:\n"
  "#\n"
  "# satellite slof polarization lof command...\n"
  "#\n"
  "# satellite:      one of the 'S' codes defined in sources.conf\n"
  "# slof:           switch frequency of LNB; the first entry with\n"
  "#                 an slof greater than the actual transponder\n"
  "#                 frequency will be used\n"
  "# polarization:   V = vertical, H = horizontal, L = Left circular, R = Right circular\n"
  "# lof:            the local oscillator frequency to subtract from\n"
  "#                 the actual transponder frequency\n"
  "# command:\n"
  "#   t         tone off\n"
  "#   T         tone on\n"
  "#   v         voltage low (13V)\n"
  "#   V         voltage high (18V)\n"
  "#   A         mini A\n"
  "#   B         mini B\n"
  "#   Wnn       wait nn milliseconds (nn may be any positive integer number)\n"
  "#   [xx ...]  hex code sequence (max. 6)\n"
  "#\n"
  "# The 'command...' part is optional.\n"
  "#\n";

}

//cString cDiseqc::Seperator(const cDiseqc *Diseqc)
char *cDiseqc::Seperator(const cDiseqc *Diseqc)
{
  
   int code = Diseqc->Source();
   DLOG (DBG " Seperator %d ", code);

   DLOG (DBG " %s ", *cSource::ToString(Diseqc->Source()));
   //DLOG (DBG " Seperator return %s "  );
      
   char *buffer;
   asprintf(&buffer, "# Input %d - %s  \n", Diseqc->Index(), *cSource::ToString(code));
   DLOG (DBG " Seperator return %s ", buffer);
   return buffer; // free me
   //return cString(buffer, true); // todo
}



// -- cDiseqcs ---------------------------------------------------------------

cDiseqcs Diseqcs;

cDiseqcs::cDiseqcs()
{
   for (int i=0; i<=MAXTUNERS; i++) {
      waitMs[i] = 0;
      repeatCmd[i] = 0;
      lnbCount[i] = 0;
      }
}

cDiseqc *cDiseqcs::Get(int Source, int Frequency, char Polarization, int Tuner)
{
  for (cDiseqc *p = First(); p; p = Next(p)) {
      if (p->Source() == Source && p->Slof() > Frequency && p->Polarization() == toupper(Polarization) && (!Tuner || !p->Tuner() || p->Tuner() == Tuner))
        return p;
      }
  return NULL;
}

bool cDiseqcs::ProvidesSource(int Source, int Tuner)
{
  for (cDiseqc *p = First(); p; p = Next(p)) {
      if (p->Source() == Source && (p->Tuner() == Tuner || !p->Tuner()))
        return true;
      }
  return false;
}

bool cDiseqcs::Load(const char *FileName, bool AllowComments, bool MustExist)
{
  DLOG(" Diseqc Load file %s  Comments? %s   exists? %s ", FileName, AllowComments?"YES":"NO", MustExist?"YES":"NO");
  for (int i=0; i<=MAXTUNERS; i++) 
      lnbCount[i] = 0;
  if (cConfig<cDiseqc>::Load(FileName, AllowComments, MustExist)) {
    ConfigureLNBs();
   //dsyslog (" DEBUG vdr-diseqc: LOAD...  lnbs %d  fertig", Diseqcs.LnbCount());
    return true;
   }
  else {
     esyslog ("No diseqc.conf, disabling Diseqc!");
     ::Setup.DiSEqC=0;
  }
  return true;
}

void cDiseqcs::ConfigureLNBs()
{
   DLOG (" DEBUG vdr-diseqc:  ConfigureLNBs");
   DLOG (" Diseqcs.Count %d, Diseqcs.First %s", Diseqcs.Count(), Diseqcs.First()?"yes":"no");

   if (Diseqcs.First()==NULL) {
           esyslog ("No entries in diseqc.conf, disabling Diseqc!");
           ::Setup.DiSEqC=0;
           return;
   }
   if (Diseqcs.First()->LnbType() == -1)
   {
       for(cDiseqc *diseqc = Diseqcs.First(); diseqc; diseqc=Diseqcs.Next(diseqc))
       {
          if (diseqc != Diseqcs.First() && diseqc->Source() == Diseqcs.Prev(diseqc)->Source() && diseqc->Tuner() == Diseqcs.Prev(diseqc)->Tuner()) {
             diseqc->SetLnbType(GetLnbType(diseqc->Lof(), Diseqcs.Prev(diseqc)->Lof()));
             continue;
          }
          else
          {
            lnbCount[diseqc->Tuner()]++;
          }
      }
   }
   for(cDiseqc *diseqc = Diseqcs.First(); diseqc; diseqc=Diseqcs.Next(diseqc))
   {
      if (diseqc->LnbType() == -1)
      {
          //dsyslog (" if Diseqcs.Next(diseqc) ");
          if (Diseqcs.Next(diseqc))
            diseqc->SetLnbType(Diseqcs.Next(diseqc)->LnbType());
      }
   }

#ifdef DEBUG_DISEQC
  dsyslog (" DEBUG vdr-diseqc: print all diseqcs  ");
  int i = 0;
  for(cDiseqc *diseqc = Diseqcs.First(); diseqc; diseqc=Diseqcs.Next(diseqc))
  {
     dsyslog (" DEBUG  Conf LNBs diseqc[%d]: name:%s type %d lof %d  Slof %d ",i, diseqc->SatName(), diseqc->LnbType(),
                                                  diseqc->Lof(), diseqc->Slof());
     i++;
  }
#endif

}

void cDiseqcs::Clear()
{
  for (int i=0; i<=MAXTUNERS; i++) {
     lnbCount[i]=0;
  }

  while (Diseqcs.Count()) {
     cDiseqc *p = Diseqcs.First();
     Diseqcs.Del(p);
     }
}

void cDiseqcs::NewLnb(int DiseqcType, int Source, int LnbType, int Tuner)
{

  lnbCount[Tuner]++;
  //dsyslog (" cDiseqcs::NewLnb lnbCount++ %d", lnbCount[Tuner]);
  if ((Source & cSource::st_Mask) != cSource::stSat)
     Source = cSource::stSat;
  switch (DiseqcType) {
    case DISICON4: {
         DLOG ("DISICON-4");
         for (int line = 0; line<3; line++) {
            cDiseqc *d = new cDiseqc(Source);
            d->SetTuner(Tuner);
            d->SetDisiCon4Commands(line);
            Diseqcs.Add(d);
           }
         }
         break;
      case MINI:  {
           DLOG("MINI DiSEqC   LnbType %d:", LnbType);
           for (int line=0;line<4;line++) {
              cDiseqc *d = new cDiseqc(Source,LnbType);
              d->SetTuner(Tuner);
              if(d->SetMiniDiseqCommands(line))
                Diseqcs.Add(d);
              else
                delete d;
            }
           }
           break;
      case FULL:  {
           DLOG("FULL DiSEqC");
           for (int line=0;line<4;line++) {
              cDiseqc *d = new cDiseqc(Source,LnbType);
              d->SetTuner(Tuner);
              if(d->SetFullDiseqCommands(line))
                Diseqcs.Add(d);
              else
                delete d;
            }
           }
           break;
      case FULL_11:  {
           DLOG ("FULL DiSEqC 1.1 up to 16 LNBs ");
           for (int line=0;line<4;line++) {
              dsyslog (" cDiseqcs::NewLnb  new cDiseqc(Source %d Type %d, Tuner %d ) ", Source, LnbType, Tuner);
              cDiseqc *d = new cDiseqc(Source, LnbType);
              d->SetTuner(Tuner); // in Constr
              if (d->SetFullDiseq1_1Commands(line)) 
              {
                DLOG (" cDiseqcs::NewLnb  SetCommands (%d)  succeeds ", line);
                Diseqcs.Add(d);
              }
              else {
                DLOG (" cDiseqcs::NewLnb  SetCommands (%d)  !! failed !! ", line);
                delete d;
              }
            }
           }
           break;
      case NONE:   {
           for (int line=0;line<4;line++) {
              cDiseqc *d = new cDiseqc(Source,LnbType);
              d->SetTuner(Tuner);
              d->SetNoDiseqcCommands(line);
              Diseqcs.Add(d);
              }
           }
           break;
     default:
          ; //XXX

     }
}

void cDiseqcs::SetLnbType(int LofStat)
{
  // Add SLOF
  switch (LofStat)
  {
     case 0:
           ::Setup.LnbFrequLo = 9750;
           ::Setup.LnbFrequHi = 10600;
           break;
     case 1:
           ::Setup.LnbFrequLo = 10750;
           ::Setup.LnbFrequHi = 11250;
           break;
     case 2:
           ::Setup.LnbFrequLo = 0;
           ::Setup.LnbFrequHi = 5150 ;
           break;
     case 3:
           ::Setup.LnbFrequLo = 0;
           ::Setup.LnbFrequHi = 9750;
           break;
     case 4:
           ::Setup.LnbFrequLo = 0;
           ::Setup.LnbFrequHi = 10600;
           break;
     case 5:
           ::Setup.LnbFrequLo = 0;
           ::Setup.LnbFrequHi = 11250;
           break;
     case 6:
           ::Setup.LnbFrequLo = 0;
           ::Setup.LnbFrequHi = 11475;
           break;
     default:
           esyslog ("ERROR: Unknown DiSEqC Type");
           ::Setup.LnbFrequLo = 9750;
           ::Setup.LnbFrequHi = 10600;
           break;
     }
}

int cDiseqcs::GetLnbType(int Freq1, int Freq2)
{

  int FrequLo = 0;
  int FrequHi = 0;

  if (Freq1<Freq2)
  {
     FrequLo = Freq1;
     FrequHi = Freq2;
  }
  else
  {
     FrequHi = Freq1;
     FrequLo = Freq2;
  }

  switch (FrequLo) {
    case 9750:
         if (FrequHi == 10600)
             return 0;
         else
             return 3;
    case 10750:
               return 1;
    case 5150:
              return 2;
    case  10600:
              return 4;
    case  11250:
              return 5;
    case  11475:
              return 6;
    case 10560:
    case 11080:
              return 7; 
    default:
              esyslog (" error in \"diseqc.conf\". Assuming Universal LNB (KU-Band)");
              return 0;
   }
   return 0;
}

bool cDiseqcs::IsUnique(int *Src, int Lnbs)
{
   for(int i = 0; i<Lnbs*4; i+=4){
      int k = i+4;
      while (k<Lnbs*4){
          if (Src[i] == Src[k]){
             return false;
          }
          k+=4;
      }
   }
   return true;
}
