/*
 * VCD Player plugin for VDR
 * psdcontrol.h: PSD control of VCD replay
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#include "functions.h"
#include "viewer.h"
#include "player.h"


// --- cPsdSpiControl -----------------------------------------------------------

class cPsdSpiControl : public cVcdViewerControl {
private:
  static int spi;
  static cVcd *vcd;
  static union psd_vcd *psdVcd;
  static int playInit;
  static int playTime;
  static int waitInit;
  static int waitTime;
  bool GotoListOffset(__u16 Offs);
public:
  cPsdSpiControl(void);
  cPsdSpiControl(int Loop);
  virtual ~cPsdSpiControl();
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Hide(void) {}
  static void SetItem(int Spi, cVcd *Vcd, union psd_vcd *PsdVcd);
  };


// --- cPsdVcdControl --------------------------------------------------------

class cPsdVcdControl : public cVcdPlayerControl {
private:
  static int track;
  static cVcd *vcd;
  static union psd_vcd *psdVcd;
  static int playInit;
  static int playTime;
  static int waitInit;
  static int waitTime;
  bool GotoListOffset(__u16 Offs);
public:
  cPsdVcdControl(void);
  cPsdVcdControl(int Lba, int Loop, int AutoWait);
  virtual ~cPsdVcdControl();
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Hide(void) {}
  static void SetTrack(int Track, cVcd *Vcd, union psd_vcd *PsdVcd);
  static void SetEntry(int Entry, cVcd *Vcd, union psd_vcd *PsdVcd);
  };


