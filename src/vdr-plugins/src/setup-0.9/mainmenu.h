/***************************************************************************
 *   Copyright (C) 2010 by Reel Multimedia                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/

#ifndef MAINMENU_H
#define MAINMENU_H

#include <vdr/config.h>
#include <vdr/menu.h>
#include "vdrsetupclasses.h"

#if VDRVERSNUM >= 10716
#ifndef USE_SETUP
#error Needs USE_SETUP patch!
#endif
#ifndef USE_LIEMIEXT
#error Needs USE_LIEMIEXT patch
#endif

/*
class cMenuSetupLang : public cMenuSetupBase {
private:
  int originalNumLanguages;
  int numLanguages;
  int optLanguages;
  char oldOSDLanguage[8];
  int oldOSDLanguageIndex;
  int osdLanguageIndex;
  bool stored;
  void DrawMenu(void);
public:
  cMenuSetupLang(void);
  ~cMenuSetupLang(void);
  virtual eOSState ProcessKey(eKeys Key);
};
*/

#if 0
class cMenuSetupLiveBuffer:public cMenuSetupBase
{
  private:
    void Setup();
#if 1
    bool hasHD;
#endif
  public:
      eOSState ProcessKey(eKeys Key);
      cMenuSetupLiveBuffer(void);
};
#endif

class cMainMenu:public cMenuMain
{
  public:
    cMainMenu(eOSState State);
    eOSState ProcessKey(eKeys Key);
  private:
      time_t lastDiskSpaceCheck;
    int lastFreeMB;
    void Set(int current = 0);
    bool Update(bool Force = false);
};                              // cMainMenu

#endif /* VDRVERSNUM */
#endif /* MAINMENU_H */
