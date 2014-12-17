#ifndef ROTOR_TOOLS_H
#define ROTOR_TOOLS_H

#include <linux/dvb/frontend.h>
#include <vdr/sources.h>

enum rotorStatus { R_NOT_MOVING = 0, R_MOVE_TO_CACHED_POS = 1, R_MOVE_TO_UNKNOWN_POS = 2, R_AUTOFOCUS = 3 };

struct RotorRequestData {
	int available;
	int minpos;
	int maxpos;
    int tunerNr;
};

void MoveRotor(int cardIndex, int source);
bool IsWithinConfiguredBorders(int currentTuner, const cSource *source);
bool WaitForRotorMovement(int cardIndex);
bool TunerIsRotor(int nrTuner);
bool RotorGetMinMax(int nrTuner, int *min, int *max);
bool GetRotorStatus(int cardIndex, rotorStatus *rStat);
bool GetSignal(int cardIndex, fe_status_t *status);
//bool getLocked(int cardIndex);
uint16_t getSNR(int cardIndex);
uint16_t getSignalStrength(int cardIndex);
bool GetCurrentRotorPosition(int tuner, char *pos);
bool GetCurrentRotorDiff(int tuner, double *diff);

#endif

