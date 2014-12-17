/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  glcd.h  -  glcd logo class
 *
 *  (c) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
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

#ifndef _GLCD_H_
#define _GLCD_H_

#include "logo.h"

class cGraphLCDLogoExt : public cGraphLCDLogo 
{
public:
  cGraphLCDLogoExt();
  virtual ~cGraphLCDLogoExt();

  cGraphLCDLogoExt(const cGraphLCDLogoExt& x);
  cGraphLCDLogoExt& operator=(const cGraphLCDLogoExt& x);
  bool operator+=(const cGraphLCDLogoExt& x);
  
  virtual bool Save(const char * fileName) = 0;
  virtual bool CanMergeImage() const = 0;
};

class cGLCD : public cGraphLCDLogoExt 
{
public:
  cGLCD();
  cGLCD(const cGraphLCDLogoExt& x);
  virtual ~cGLCD();
  virtual bool Save(const char * fileName);
  virtual bool CanMergeImage() const { return true; }
};
#endif
