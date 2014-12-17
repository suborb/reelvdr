/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __FEMONTOOLS_H
#define __FEMONTOOLS_H

#include <stdint.h>
#include <vdr/channels.h>
#include <vdr/tools.h>

#ifdef DEBUG
#define Dprintf(x...) printf(x);
#else
#define Dprintf(x...) ;
#endif

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

cString getFrontendInfo(int cardIndex = 0);
cString getFrontendName(int cardIndex = 0);
cString getFrontendStatus(int cardIndex = 0);

uint16_t getSNR(int cardIndex = 0);
uint16_t getSignal(int cardIndex = 0);

uint32_t getBER(int cardIndex = 0);
uint32_t getUNC(int cardIndex = 0);

cString getApids(const cChannel *channel);
cString getDpids(const cChannel *channel);
cString getCAids(const cChannel *channel, bool identify = false);
cString getCA(int value);
cString getCoderate(int value);
cString getTransmission(int value);
cString getBandwidth(int value);
cString getInversion(int value);
cString getHierarchy(int value);
cString getGuard(int value);
cString getModulation(int value);
cString getAspectRatio(int value);
cString getVideoFormat(int value);
cString getAC3BitStreamMode(int value, int coding);
cString getAC3AudioCodingMode(int value, int stream);
cString getAC3CenterMixLevel(int value);
cString getAC3SurroundMixLevel(int value);
cString getAC3DolbySurroundMode(int value);
cString getAC3DialogLevel(int value);
cString getFrequencyMHz(int value);
cString getAudioSamplingFreq(int value);
cString getAudioBitrate(double value, double stream);
cString getBitrateMbits(double value);
cString getBitrateKbits(double value);

#endif // __FEMONTOOLS_H
