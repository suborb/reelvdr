/*
 * diseqc.h: DiSEqC handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: diseqc.h 1.2 2002/12/07 13:54:02 kls Exp $
 */

#ifndef __DISEQC_H
#define __DISEQC_H


#define  NONE 0
#define  MINI 1
#define  FULL 2
#define  FULL_11 3
#define  DISICON4 4
#define  ROTOR_SHARED 7
#define  DISEQC12 0x10
#define  GOTOX 0x20
#define  ROTORLNB 0x40
#define  ROTORMASK 0x70
#define  SWITCHMASK 0x0F
#define  TUNERMASK 0x7F
#define  TUNERBITS 7
#define  DIFFSETUPS 0x10000000 

#define DISEQCMOD_NONE 0
#define DISEQCMOD_FILE 1
#define DISEQCMOD_USER 2

#define DISEQCSMOD_NONE 0

#define MINLNBS 1
#define MAXLNBS 4
#define MAXTUNERS 4

#include "config.h"
#include "sources.h"





class cDiseqc : public cListObject {
public:
  enum eDiseqcActions {
    daNone,
    daToneOff,
    daToneOn,
    daVoltage13,
    daVoltage18,
    daMiniA,
    daMiniB,
    daCodes,
    };
  enum { MaxDiseqcCodes = 6 };
private:
  static cString ToText(const cDiseqc *Diseqc);
  //static cString Seperator(const cDiseqc *Diseqc);
  static char *Seperator(const cDiseqc *Diseqc);
  static const char *HeadLine();
  enum ID {A=65, B, C, D, E, F, G, H} lnbID;
  int source;
  int lnbType;
  int tuner; 
  char *satName;
  char polarization;
  int lof;
  int lofThreshold;
  int lofLo;
  int lofHi;

  char *commands;
  bool parsing;
  cString ToText(const cDiseqc diseqc);
  uchar codes[MaxDiseqcCodes];
  int numCodes;
  char *Wait(char *s);
  char *Codes(char *s);
  void SetLof(void);
  ///< Set Lof values if diseqc objects saved by cMenuSetupLNB
  int repeat; // 
  ///< counter of repeated diseqc commands
public:
  cDiseqc(void);
  cDiseqc(int Source);
  cDiseqc(int Source, int LnbType, int Tuner = 0);
  ///< this diseqc constr is used by NewLnb
  ~cDiseqc();
  cDiseqc &operator=(const cDiseqc &diseqc);
  cString ToText(void);
  bool Save(FILE *f);
  bool Parse(const char *s, bool Count = true);
  eDiseqcActions Execute(char **CurrentAction);
      // Parses the DiSEqC commands and returns the appropriate action code
      // with every call. CurrentAction must be the address of a character pointer,
      // which is initialized to NULL. This pointer is used internally while parsing
      // the commands and shall not be modified once Execute() has been called with
      // it. Call Execute() repeatedly (always providing the same CurrentAction pointer)
      // until it returns daNone. After a successful execution of all commands
      // *CurrentAction points to the value 0x00.
  const char *SatName(void) const { return  satName; } //*cSource::ToString(source); }
  int NumCodes(void) const { return numCodes; }
 
  void SetLnbType(int LnbType) { lnbType = LnbType; }
  void SetTuner(int Tuner) { tuner = Tuner; }
  int LnbType(void) const { return lnbType; }

  int Tuner(void) const { return tuner; }
  int Source(void) const { return source; }
  int Lof(void) const { return lof; }
  int Slof(void) const { return lofThreshold; }
    // HiLof or LoLof depends on  sequence

  char Polarization(void) const { return polarization; }
  const char *Commands(void) const { return commands; }

  uchar *Codes(int &NumCodes) { NumCodes = numCodes; return numCodes ? codes : NULL; }
  int DirectAddr_1_1(int LnbNr, int Line) { return (LnbNr * 4 % 16) + Line; }


  bool SetMiniDiseqCommands(int Sequence);
  bool SetFullDiseqCommands(int Sequence);
  bool SetFullDiseq1_1Commands(int Sequence);
  bool SetDisiCon4Commands(int Sequence);
  bool SetNoDiseqcCommands(int Sequence);

  };

// --- cDiseqcs -----------------------------------------------
class cDiseqcs : public cConfig<cDiseqc> {

private:
  int waitMs[MAXTUNERS+1];
  int repeatCmd[MAXTUNERS+1];
  int lnbCount[MAXTUNERS+1];
  void ConfigureLNBs();

public:
  cDiseqcs();
  void Clear();
  bool Load(const char *FileName, bool AllowComments = false, bool MustExist = false);
  void NewLnb(int DiseqcType, int Source=0, int lnbType=0, int Tuner = 0);

  void SetLnbType(int LofStat);
  ///< Set Global LnbType in setup.conf

  static int GetLnbType(int HiFreq, int LoFreq);
  ///<increment Lnb counter  
  void LnbInc(int Tuner = 0) { lnbCount[Tuner]++; } // private
  int LnbCount(int Tuner = 0) { return lnbCount[Tuner]; }

  cDiseqc *Get(int Source, int Frequency, char Polarization, int Tuner=0); // XX

  bool ProvidesSource(int Source, int Tuner);

  int WaitMs(int Tuner = 0) const { return waitMs[Tuner]; }
  int RepeatCmd(int Tuner = 0) const { return repeatCmd[Tuner]; }
  void SetWaitMs(int ms, int Tuner = 0) { waitMs[Tuner] = ms; }
  void SetRepeatCmd(int rep, int Tuner = 0) { repeatCmd[Tuner] = rep; /* dsyslog (" DiSEqC SetRepeat %d at Tuner %d ", rep, Tuner);*/  }

  bool IsUnique(int *src, int lnbs); // menu.c?? 
  };

extern cDiseqcs Diseqcs;

#endif //__DISEQC_H
