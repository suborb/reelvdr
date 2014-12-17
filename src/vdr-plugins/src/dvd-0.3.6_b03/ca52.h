/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __VDR_cA52_H
#define __VDR_cA52_H

#include <inttypes.h>
extern "C" {
#include <a52dec/mm_accel.h>
#include <a52dec/a52.h>
}

#include <vdr/player.h>
#include "tools-dvd.h"
#include "player-dvd.h"

class A52frame {
 public:
    uint32_t pts;
    int size;
    uint8_t *frame;

    int pos;

 public:
    A52frame(int datasize, int frmsize, uint32_t apts);
    ~A52frame();
};

class A52assembler {
 private:
    A52frame *curfrm;
    uint16_t syncword;

    int parse_syncinfo(uint8_t *data);
 public:
    A52assembler();
    ~A52assembler();
    int put(uint8_t *buf, int len, uint32_t pts);
    A52frame *get(void);
    void clear(void);
    int used(void);
    bool ready(void);
};

class A52decoder {

 public:
  enum eSyncMode { ptsCopy, ptsGenerate };

 private:
  cDvdPlayer &player;

  A52assembler a52asm;
  a52_state_t * state;

  uint8_t burst[6144];

  int sample_rate;
  int flags;

  sample_t level, bias;

  eSyncMode syncMode;
  uint32_t apts;

  uchar *blk_buf;
  uchar *blk_ptr;
  int blk_size;

  void setup(void);
  void float_to_int (float * _f, int16_t * s16, int flags);
  void init_ipack(int p_size, uint32_t pktpts, uint8_t SubStreamId);
  int convertSample (int flags, a52_state_t * _state, uint32_t pktpts, uint8_t SubStreamId);
 public:
  A52decoder(cDvdPlayer &ThePlayer);
  ~A52decoder();
  void setSyncMode(eSyncMode mode) { syncMode = mode; };
  eSyncMode getSyncMode(void) { return syncMode; }
  void decode(uint8_t * start, int size, uint32_t pktpts, uint8_t SubStreamId);
  void clear();
};

#endif //__VDR_cA52_H
