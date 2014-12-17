#ifndef ROTOR_MENUITEM_H
#define ROTOR_MENUITEM_H

#include <vdr/menuitems.h>

int Width(const char *s, int edw);

class cMenuEditSatTunerItem : public cMenuEditItem {
private:
  bool hasRotor(int Tuner);
protected:
  int *value;
  int min, max;
  int Diseqc; 
  const char *minString;
  virtual void Set(void);
public:
  cMenuEditSatTunerItem(const char *Name, int *Value, const char *MinString, int diseqc);
  eOSState ProcessKey(eKeys Key);
};

class cMenuEditintItem : public cMenuEditItem {
protected:
  int *value;
  int min, max;
  const char *minString, *maxString;
  virtual void Set(void);
public:
  cMenuEditintItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *MinString = NULL, const char *MaxString = NULL);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditEWItem : public cMenuEditItem {
private:
  int Tuner;
  int movement;
  void Set();
public:
  cMenuEditEWItem(const char *Name, int tuner);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditStepsEWItem : public cMenuEditItem {
private:
  int steps;
  int Tuner;
  void Set();
public:
  cMenuEditStepsEWItem(const char *Name, int tuner);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditLimitsItem : public cOsdItem {
private:
  int Tuner;
public:
  cMenuEditLimitsItem(const char *Name, int tuner);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditFreqItem : public cMenuEditItem {
private:
  int *value;
  char *value2;
  eOSState state;
public:
  cMenuEditFreqItem(const char *Name,int *Value,char *Value2, eOSState State = osUnknown);
  void Set();
  virtual eOSState ProcessKey(eKeys Key);
};

class cRotorPos;

class cMenuEditSatItem : public cMenuEditIntItem {
private:
  const cSource *source;
protected:
  virtual void Set(void);
public:
  cMenuEditSatItem(const char *Name, int *Value);
  eOSState ProcessKey(eKeys Key);
  };

class cMenuEditSymbItem : public cMenuEditIntItem {
protected:
  char *name;
  virtual void Set();
  eOSState state;
public:
  cMenuEditSymbItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, eOSState State = osUnknown);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditCmdsItem : public cMenuEditStraItem {
private:
  eOSState state;
public:
  cMenuEditCmdsItem(const char *Name, int *Value, int NumStrings, const char * const *Strings, eOSState State);
  virtual eOSState ProcessKey(eKeys Key); 
};

class cMenuEditIntpItem : public cMenuEditItem {
protected:
  virtual void Set(void);
  int min, max;
  const char *string1, *string2;
  int sgn, rvalue;
  int *value;
public:
  cMenuEditIntpItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *String1 = "", const char *String2 = NULL);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditAngleItem : public cMenuEditIntpItem {
private:
  bool *sa;
protected:
  virtual void Set(void);
public:
  cMenuEditAngleItem(const char *Name, int *Value, int Min, int Max, const char *String1, const char *String2, bool *SA);
  virtual eOSState ProcessKey(eKeys Key);
};

#endif
