/*
 * VCD Player plugin for VDR
 * player.h: Player core
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __VCD_PLAYER_H
#define __VCD_PLAYER_H

#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/remux.h>
#include <vdr/ringbuffer.h>
#include "functions.h"

#define NO_PICTURE          0
#define PICTURE_SC          0x00
#define USER_DATA_SC        0xB2
#define SEQUENCE_HEADER_SC  0xB3
#define EXTENSION_SC        0xB5
#define GROUP_OF_PICT_SC    0xB8
#define PACK_SC             0xBA
#define SYSTEM_HEADER_SC    0xBB
#define AUDIO_STREAM_SC     0xC0
#define VIDEO_STREAM_SC     0xE0
#define SC_MASK             0xE0

class cVcdPlayer;

class cVcdPlayerControl : public cControl {
private:
  cVcdPlayer *player;
public:
  cVcdPlayerControl(int Track, cVcd *Vcd);
  cVcdPlayerControl(int Track, cVcd *Vcd, int Lba, int Loop, int AutoWait);
  virtual ~cVcdPlayerControl();
  bool Active(void);
  int GetTrack(void);
  void Stop(void);
  void Pause(void);
  void Play(void);
  void Forward(void);
  void Backward(void);
  void SkipSeconds(int Seconds);
  bool SkipTracks(int Tracks);
  bool SkipEntries(int Entries);
  void Goto(int Index, bool Still = false);
  void GotoTrack(int Track);
  void GotoEntry(int Entry);
  const char *DeviceName(void);
  };

#endif //__VCD_PLAYER_H
