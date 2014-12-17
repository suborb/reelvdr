#include "menuitems.h"
#include "menu.h"
#include "diseqc.h"


// --- Width ----------------------------------------------------------------

int Width(const char *s,int edw)
{
  if (edw>100)
    return cFont::GetFont(fontOsd)->cFont::Width(s);
  else
    return s ? strlen(s) : 0;
}

// ---  cMenuEditSatTunerItem -----------------------------------------------

cMenuEditSatTunerItem::cMenuEditSatTunerItem(const char *Name, int *Value, const char *MinString, int diseqc)
:cMenuEditItem(Name)
{
  value = Value;
  Diseqc=diseqc;
  min = 1;
  max = 9;
  minString = NULL;
  if (*value < min)
     *value = min;
  else if (*value > max)
     *value = max;
  Set();
}

bool cMenuEditSatTunerItem::hasRotor(int Tuner)
{
  return (((Diseqc & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1))) & ROTORMASK);
}

void cMenuEditSatTunerItem::Set(void)
{
  if (minString && *value == min)
     SetValue(minString);
  else {
     char buf[16];
     snprintf(buf, sizeof(buf), "%d", *value);
     SetValue(buf);
     }
}


eOSState cMenuEditSatTunerItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     Key = NORMALKEY(Key);
     switch (Key) {
       case kNone: break;
       case k0 ... k9:
            if (Key == k0 || cDevice::GetDevice(Key - k1) && cDevice::GetDevice(Key - k1)->ProvidesSource(cSource::stSat) && hasRotor(Key-k0))
              *value = Key - k0;
            break;
       case kLeft:
            {
            int tvalue = *value;
            do 
            {
              tvalue = tvalue>min ? tvalue - 1 : min;
            } while (tvalue > min && !(cDevice::GetDevice(tvalue-1) && cDevice::GetDevice(tvalue-1)->ProvidesSource(cSource::stSat) && hasRotor(tvalue)));
            if (cDevice::GetDevice(tvalue-1) && cDevice::GetDevice(tvalue-1)->ProvidesSource(cSource::stSat) && hasRotor(tvalue))
              *value = tvalue;
            break;
            }
       case kRight:
            {
            int tvalue = *value;
            do
            {
              tvalue = tvalue<max ? tvalue + 1 : max;
            } while (tvalue < max && !(cDevice::GetDevice(tvalue-1) && cDevice::GetDevice(tvalue-1)->ProvidesSource(cSource::stSat) && hasRotor(tvalue)));
            if (cDevice::GetDevice(tvalue-1) && cDevice::GetDevice(tvalue-1)->ProvidesSource(cSource::stSat) && hasRotor(tvalue))
              *value = tvalue;
            break;
            }
       default:
            if (*value < min) { *value = min; Set(); }
            if (*value > max) { *value = max; Set(); }
            return state;
       }
     state = osContinue;
     }
  return state;
}

// --- cMenuEditIntItem ------------------------------------------------------

cMenuEditintItem::cMenuEditintItem(const char *Name, int *Value, int Min, int Max, const char *MinString, const char *MaxString)
:cMenuEditItem(Name)
{
  value = Value;
  min = Min;
  max = Max;
  minString = MinString;
  maxString = MaxString;
  if (*value < min)
     *value = min;
  else if (*value > max)
     *value = max;
  Set();
}

void cMenuEditintItem::Set(void)
{
  if (minString && *value == min)
     SetValue(minString);
  else if (maxString && *value == max)
     SetValue(maxString);
  else {
     char buf[16];
     snprintf(buf, sizeof(buf), "%d", *value);
     SetValue(buf);
     }
}

eOSState cMenuEditintItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     int newValue = *value;
     Key = NORMALKEY(Key);
     switch (Key) {
       case kNone: break;
       case k0 ... k9:
            if (fresh) {
               *value = 0;
               fresh = false;
               }
            newValue = *value * 10 + (Key - k0);
            break;
       case kLeft: // TODO might want to increase the delta if repeated quickly?
            newValue = *value - 1;
            fresh = true;
            break;
       case kRight:
            newValue = *value + 1;
            fresh = true;
            break;
       default:
            if (*value < min) { *value = min; Set(); }
            if (*value > max) { *value = max; Set(); }
            return state;
       }
     if (newValue != *value && (!fresh || min <= newValue) && newValue <= max) {
        *value = newValue;
        Set();
        }
     state = osContinue;
     }
  return state;
}


// --- cMenuEditEWItem ------------------------------------------------------

cMenuEditEWItem::cMenuEditEWItem(const char *Name, int tuner):cMenuEditItem(Name)
{
  Tuner=tuner;
  movement = 0;
  Set();
};

void cMenuEditEWItem::Set()
{
  char buf[32];
  switch (movement)
  {
    case -1: snprintf(buf, sizeof(buf), "%s ->", tr("East"));
             break;
    case  0: snprintf(buf, sizeof(buf), "<- %s ->", tr("Halt"));
             break;
    case  1: snprintf(buf, sizeof(buf), "<-  %s", tr("West"));
             break; 
  }
  SetValue(buf);
}


eOSState cMenuEditEWItem::ProcessKey(eKeys Key)
{
  eOSState state=osContinue;
  Key = NORMALKEY(Key);
  switch (Key) {
    case kNone           : break;
    case kLeft|k_Repeat  :
    case kRight|k_Repeat : break;
    case kLeft|k_Release :
    case kRight|k_Release:
                           DiseqcCommand(Tuner,Halt);
                           movement = 0;
                           break;
    case kLeft           : if (movement == 1)
                           {
                             DiseqcCommand(Tuner,Halt);
                             movement=0;
                           }
                           else
                           {
                             DiseqcCommand(Tuner,DriveEast);
                             movement = -1;
                           }
                           Setup.UpdateChannels=0;
                           break;
    case kRight          : if (movement == -1)
                           {
                             DiseqcCommand(Tuner,Halt);
                             movement=0;
                           }
                           else
                           {
                             DiseqcCommand(Tuner,DriveWest);
                             movement = 1;
                           }
                           Setup.UpdateChannels=0;
                           break;
    case kOk             : DiseqcCommand(Tuner,Halt);
                           movement = 0;
                           break;
    default              : movement = 0;
                           state=osUnknown;
               }
  Set();
  return state;
}

// --- cMenuEditStepsEWItem -------------------------------------------------

cMenuEditStepsEWItem::cMenuEditStepsEWItem(const char *Name, int tuner)
:cMenuEditItem(Name)
{
  Tuner=tuner;
  steps=1;
  Set();
}

void cMenuEditStepsEWItem::Set()
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%d %s %s",steps > 0 ? steps : -steps, tr("Steps"), steps>0 ? tr("West") : tr("East"));
  SetValue(buf);
}

eOSState cMenuEditStepsEWItem::ProcessKey(eKeys Key)
{
  Key = NORMALKEY(Key);
  switch (Key) {
    case kNone  : break;
    case k0...k9: if (fresh)
                  {
                    steps=0;
                    fresh=false;
                  }
                  steps= steps*10 + (Key-k0);
                  if (steps>128) steps=128;
                    char t1[50];
                  sprintf(t1,"%d %s",steps,tr("Steps"));
                  Set();
                  break;
    case kLeft  : fresh=true;
                  steps--;
                  if (!steps)
                    steps--;
                  Set();
                  break;
    case kRight : fresh=true;
                  steps++;
                  if (!steps)
                    steps++;
                  Set();
                  break;
    case kOk    : fresh=true;
                  if (steps>0)
                  {
                    DiseqcCommand(Tuner,DriveStepsWest,steps);
                    Setup.UpdateChannels=0;
                  }
                  if (steps<0)
                  {
                    DiseqcCommand(Tuner,DriveStepsEast,-steps);
                    Setup.UpdateChannels=0;
                  }
                  break;
    default     : return osUnknown;
               }
  return osContinue;
}

// --- cMenuEditLimitsItem -----------------------------------------------------

cMenuEditLimitsItem::cMenuEditLimitsItem(const char *Name, int tuner):cOsdItem(Name)
{
  Tuner=tuner;
};

eOSState cMenuEditLimitsItem::ProcessKey(eKeys Key)
{
  Key = NORMALKEY(Key);
  switch (Key) {
    case kNone : break;
    case kLeft : DiseqcCommand(Tuner,SetEastLimit);
                 break;
    case kRight: DiseqcCommand(Tuner,SetWestLimit);
                 break;
    case kOk   : DiseqcCommand(Tuner,LimitsOn);
                 break;
    default    :
                 return osUnknown;
               }
  return osContinue;
}

// --- cMenuEditFreqItem -------------------------------------------------------

cMenuEditFreqItem::cMenuEditFreqItem(const char *Name,int *Value,char *Value2, eOSState State)
:cMenuEditItem(Name)
{
  value = Value;
  value2= Value2;
  state=State;
  Set();
};

void cMenuEditFreqItem::Set()
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d %c", *value, *value2);
  SetValue(buf);
}

eOSState cMenuEditFreqItem::ProcessKey(eKeys Key)
{
  Key = NORMALKEY(Key);
  switch (Key) {
    case kNone  : break;
    case k0...k9: if (fresh)
                  {
                    *value=0;
                    fresh=false;
                  }
                  if (*value<1300)
                  *value = *value*10 + (Key-k0);
                  Set();
                  return state;
                  break;
    case kLeft  : *value2='V';
                  fresh=true;
                  Set();
                  return state;
                  break;
    case kRight : *value2='H';
                  fresh=true;
                  Set();
                  return state;
                  break;
    case kOk    :
                  fresh=true;
                  return state;
                  break;
    default     :
                  return osUnknown;
               }
  return osContinue;
}

// --- cMenuEditSatItem ------------------------------------------------------

cMenuEditSatItem::cMenuEditSatItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, 0)
{
  source = Sources.Get(*Value);
  Set();
}

void cMenuEditSatItem::Set()
{
  if (source) {
     char *buffer = NULL;
     asprintf(&buffer, "%s - %s", *cSource::ToString(source->Code()), source->Description());
     SetValue(buffer);
     free(buffer);
     }
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditSatItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) { 
        const cSource* tsource=source;
        if (tsource && tsource->Prev()) 
           tsource = (cSource *)tsource->Prev();
        while (tsource && (tsource->Code() & cSource::st_Mask)!=cSource::stSat && tsource->Prev())
           tsource = (cSource *)tsource->Prev();
        if (tsource && (tsource->Code() & cSource::st_Mask)==cSource::stSat) {
           source = tsource;           
           *value = source->Code();
           }
        }
     else if (NORMALKEY(Key) == kRight) {
        const cSource* tsource=source;
        if (tsource) {
           if (tsource->Next())
              tsource = (cSource *)tsource->Next();
           }
        else
           tsource = Sources.First();
        while (tsource && (tsource->Code() & cSource::st_Mask)!=cSource::stSat && tsource->Next())
           tsource = (cSource *)tsource->Next();
        if (tsource && (tsource->Code() & cSource::st_Mask)==cSource::stSat) {
           source = tsource;
           *value = source->Code();
           }
        }
     else
        return state; // we don't call cMenuEditIntItem::ProcessKey(Key) here since we don't accept numerical input
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditSymbItem ----------------------------------------------------

cMenuEditSymbItem::cMenuEditSymbItem(const char *Name, int *Value, int Min, int Max, eOSState State):cMenuEditIntItem(Name,Value,Min,Max)
{
  name=strdup(Name);
  state=State;
  Set();
}

void cMenuEditSymbItem::Set()
{
  char buf[50];
  snprintf(buf,sizeof(buf),"%d",*value);
  SetValue(buf);
}

eOSState cMenuEditSymbItem::ProcessKey(eKeys Key)
{
  eOSState pstate = cMenuEditIntItem::ProcessKey(Key);
  Key = NORMALKEY(Key);
  if (Key >= k0 && Key <= k9 || Key == kLeft || Key == kRight || Key == kOk)
    return state;
  return pstate;
}


// --- cMenuEditCmdsItem ----------------------------------------------------

cMenuEditCmdsItem::cMenuEditCmdsItem(const char *Name, int *Value, int NumStrings, const char * const *Strings, eOSState State)
:cMenuEditStraItem(Name,Value,NumStrings,Strings)
{
  state=State;
}

eOSState cMenuEditCmdsItem::ProcessKey(eKeys Key)
{
  if (Key == kOk) return state;
  else return cMenuEditStraItem::ProcessKey(Key);
}

// --- cMenuEditIntpItem ----------------------------------------------------

cMenuEditIntpItem::cMenuEditIntpItem(const char *Name, int *Value, int Min, int Max, const char *String1,const char *String2)
:cMenuEditItem(Name)
{
  value = Value;
  rvalue = abs(*value);
  sgn = *value>=0;
  string1 = String1;
  string2 = String2;
  min = Min;
  max = Max;
  Set();
}

void cMenuEditIntpItem::Set(void)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d.%d %s", rvalue/10, rvalue % 10, sgn ? string1 : string2);
  SetValue(buf);
}

eOSState cMenuEditIntpItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);
  if (state == osUnknown)
  {
    int newValue = rvalue;
    Key = NORMALKEY(Key);
    switch (Key) {
      case kNone  : break;
      case k0...k9:
                    if (fresh)
                    {
                      rvalue = 0;
                      fresh = false;
                    }
                    newValue = rvalue * 10 + (Key - k0);
                    break;
      case kLeft  :
                    sgn = 0;
                    fresh = true;
                    break;
      case kRight :
                    sgn = 1;
                    fresh = true;
                    break;
      default     :
                    if (rvalue < min) { rvalue = min; Set(); }
                    if (rvalue > max) { rvalue = max; Set(); }
                    *value = rvalue * (sgn ? 1 : -1); 
                    return state;
                 }
    if ((!fresh || min <= newValue) && newValue <= max)
    {
      rvalue = newValue;
      Set();
    }
  *value = rvalue * (sgn ? 1: -1);
  state = osContinue;
  }
  return state;
}

// --- cMenuEditAngleItem ----------------------------------------------------

cMenuEditAngleItem::cMenuEditAngleItem(const char *Name, int *Value, int Min, int Max, const char *String1,const char *String2, bool *SA)
:cMenuEditIntpItem(Name, Value, Min, Max, String1, String2)
{
  sa=SA;
  if (!*sa)
    sgn=-1;
  Set();
}

void cMenuEditAngleItem::Set(void)
{
  char buf[16];
  if (sgn>=0)
    snprintf(buf, sizeof(buf), "%d.%d %s", rvalue/10, rvalue % 10, sgn ? string1 : string2);
  else
    snprintf(buf, sizeof(buf), "%s", tr("inactive"));
  SetValue(buf);
}

eOSState cMenuEditAngleItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditIntpItem::ProcessKey(Key);
  if (state == osUnknown)
  {
    Key = NORMALKEY(Key);
    switch (Key) {
      case kNone  : break;
      case kRed   :
                    sgn=-1;
                    fresh = true;
                    Set();
                    break;
      default     : 
                    return state;
    }
    state = osContinue;
  }
  *sa=sgn!=-1;
  return state;
}

