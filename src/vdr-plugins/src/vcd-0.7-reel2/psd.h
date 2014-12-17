/*
 * VCD Player plugin for VDR
 * psd.h: PSD controlled replay
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __VCD_PSD_H
#define __VCD_PSD_H


#include <vdr/osdbase.h>
#include "functions.h"


class cVcdPsd : public cOsdMenu {
protected:
  cVcd *vcd;
  bool validList;
  eOSState Eject(void);
  eOSState Play(void);
public:
  cVcdPsd(cVcd *Vcd);
  ~cVcdPsd();
  virtual eOSState ProcessKey(eKeys Key);
  };

#endif //__VCD_PSD_H
