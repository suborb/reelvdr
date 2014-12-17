/*
 * VCD Player plugin for VDR
 * menu.h: OSD menus
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


#ifndef __VCD_MENU_H
#define __VCD_MENU_H


#include <vdr/osd.h>
#include "functions.h"


class cMenu : public cOsdMenu {
protected:
  cVcd *vcd;
  bool validLabel;
  char vcdTitle[17];
  eOSState Eject(void);
  void SetHelp(const char *Red, const char *Green, const char *Blue);
public:
  cMenu(cVcd *Vcd);
  ~cMenu();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuSpi : public cMenu {
private:
  bool validItems;
  bool ListItems(void);
  eOSState Play(void);
public:
  cMenuSpi(cVcd *Vcd);
  ~cMenuSpi();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuVcd : public cMenu {
private:
  bool validTracks;
  bool spi;
  bool ListTracks(void);
  eOSState Play(void);
public:
  cMenuVcd(cVcd *Vcd);
  ~cMenuVcd();
  virtual eOSState ProcessKey(eKeys Key);
  };


#endif //__VCD_MENU_H
