/*
 * VCD SPI Viewer plugin for VDR
 * viewer.h: Viewer core
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __VCD_VIEWER_H
#define __VCD_VIEWER_H

#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/remux.h>
#include <vdr/ringbuffer.h>
#include "functions.h"

#define PACK_SC     0xBA

class cVcdViewer;

class cVcdViewerControl : public cControl {
private:
  cVcdViewer *viewer;
public:
  cVcdViewerControl(int Spi, cVcd *Vcd);
  virtual ~cVcdViewerControl();
  bool Active(void);
  bool Still(void);
  int GetSpi(void);
  void Stop(void);
  bool SkipItems(int Items);
  void GotoItem(int Spi);
  void ToggleStillRes(void);
  const char *DeviceName(void);
  };

#endif //__VCD_VIEWER_H
