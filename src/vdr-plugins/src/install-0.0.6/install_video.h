/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
 *                                                                         *
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
 ***************************************************************************
 *
 * install_video.h
 *
 ***************************************************************************/

#ifndef INSTALL_VIDEO_H
#define INSTALL_VIDEO_H

#include "installmenu.h"

class cInstallVidoutAva:public cInstallSubMenu
{
  private:
    void Set();
    bool Save();
    const char *showMainMode[2];
    const char *showSubMode[4];
    const char *showDisplayType[2];
    const char *showAspect[3];
    const char *showHDDMode[2];
    const char *showHDAMode[4];
    const char *showHDAPort[3];
    const char *showIntProg[2];
    const char *deint[3];
    const char *showResolution[4];
    const char *showOsd[3];
    const char *masterMode[4];
    int HDdmode;
    int HDamode;
    int HDaport;
    int old_HDaport;
    int HDaspect;
    int HDresolution;
    int HDnorm;
    int HDintProg;
    int HDdisplay_type;
    int mastermode;
    bool expertmode;
  public:
      cInstallVidoutAva();
     ~cInstallVidoutAva();
    eOSState ProcessKey(eKeys Key);
};

#endif
