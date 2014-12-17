#ifndef MENU_H
#define MENU_H
#include "diseqc.h"
#include "filter.h"
#include "rotor.h"
#include <vdr/plugin.h>

class config {
public:
  int ActualSource[4];
  int AByte;
  int Lat;
  int Long;
  bool sw;
  config() {AByte=49; Lat=0; Long=0; sw=0;};
};

class cRotorPos : public cListObject {
private:
  int code;
  int pos[4];
public:
  cRotorPos(int Code = 0, int Pos0 = 0, int Pos1 = 0, int Pos2 = 0, int Pos3 = 0);
  int Code(void) { return code; }
  int Pos(int Tuner) { return pos[Tuner-1]; }
  void SetPos(int Pos, int Tuner) { pos[Tuner-1]=Pos; }
  bool Parse(const char *s);
  bool Save(FILE *f);
};

class cRotorPositions : public cConfig<cRotorPos> {
public:
  cRotorPos *GetfromSource(int Code);
  cRotorPos *GetfromPos(int Pos, int Tuner);
};

class cSatAngle : public cListObject {
private:
  int code;
  int e_code[4];
public:
  cSatAngle(int Code = 0);
  int Code(int Tuner) { return e_code[Tuner-1]; }
  int R_Code(void) { return code; }
  void SetCode(int Code, int Tuner) { e_code[Tuner-1]=Code; }
  bool Parse(const char *s);
  bool Save(FILE *f);
};

class cSatAngles : public cConfig<cSatAngle> {
public:
  cSatAngle *GetfromSource(int Code);
};

extern cRotorPositions RotorPositions;
extern cSatAngles SatAngles;

class cConfigureMenuRotor : public cOsdMenu {
public:
  cConfigureMenuRotor(void);
};

class cMainMenuRotor : public cOsdMenu {
private:
  cPluginRotor* plugin;
  enum { mUser, mInstaller };
  int Fd_Frontend;
int achsw;
  int mode;
  int cmdnr;
  int Angle;
  bool satActive;
  int Tuner;
  int Setup_DiSEqC;
  int a_source;
  const char* texts[4];
  const char* commands[10];
  int oldupdate,Position,Frequenz,Symbolrate,edw;
  bool first_time;
  cRotorPos *RotorPos;
  cSatAngle *SatAngle;
  char Pol;
  cChannel *OldChannel;
  bool HasSwitched;
  char SS[700];
  char SNR[700];
  char Status[700];
  char Type[200];  
  void Set();
  void AddMenuItems();
  cFont::tPixelData CharData[eDvbFontSize][256*35];
  void UpdateStatus(void);
public:
  cMainMenuRotor(cPluginRotor *Plugin);
  ~cMainMenuRotor();
  virtual eOSState ProcessKey(eKeys Key);
};

class PatFilter;
class SdtFilter;

class cMenuScan : public cOsdMenu {
private:
  int Tuner;
  int num;
  cChannel Channel[64];
  PatFilter *PFilter;
  SdtFilter *SFilter;
  cOsdItem* n[64];
public:
  cMenuScan(int tuner);
  ~cMenuScan();
  virtual eOSState ProcessKey(eKeys Key);
  void AddChannel(int Num);
  void NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid);
#if VDRVERSNUM>=10509 || defined(REELVDR)
  void SetPids(int Sid,int Vpid, int Ppid, int *Apids, char ALangs[][MAXLANGCODE2], int *Dpids, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid);
#else
  void SetPids(int Sid,int Vpid, int Ppid, int *Apids, char ALangs[][MAXLANGCODE2], int *Dpids, char DLangs[][MAXLANGCODE2], int Tpid);
#endif
  void SetCaIds(int Sid,const int *CaIds);
  void SetCaDescriptors(int Sid,int Level);
  cChannel* GetChannel(int Sid);
  void display(int num);
};

extern config data;

#endif
