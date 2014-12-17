#ifndef Rotor_Diseqc_h
#define Rotor_Diseqc_h

#include <vdr/dvbdevice.h>

enum { Halt, LimitsOff, SetEastLimit, SetWestLimit, DriveEast, DriveStepsEast, DriveStepsWest, DriveWest, Store, Goto, Recalc, LimitsOn, Gotox};;

void DiseqcCommand(int Tuner, int KNr, int p1=0, int p2=0);

void GotoX(int Tuner, int Source);

#endif
