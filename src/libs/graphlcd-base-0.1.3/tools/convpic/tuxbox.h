/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  tuxbox.h  -  tuxbox logo class
 *
 *  (c) 2004 Andreas Brachold <vdr04 AT deltab de>
 **/

/***************************************************************************
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
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/
#ifndef _TUXBOX_H_
#define _TUXBOX_H_

#include "glcd.h"

class cTuxBox : public cGraphLCDLogoExt 
{
  void vert2horz(const unsigned char* source, unsigned char* dest);
  void horz2vert(const unsigned char* source, unsigned char* dest);

public:
  cTuxBox();
  cTuxBox(const cGraphLCDLogoExt& x);
  virtual ~cTuxBox();
  virtual bool Load(const char * fileName, bool bInvert);
  virtual bool Save(const char * fileName);
  virtual bool CanMergeImage() const { return true; }
};

#endif
