/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  logo.h  -  logo class
 *
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

#ifndef _GRAPHLCD_LOGO_H_
#define _GRAPHLCD_LOGO_H_

#include "global.h"
#include <vector>

#define GLCDFILE_EXT  ".glcd"

#pragma pack(1)
struct GLCDHEADER
{
  static const char GLCDFILE_SIGN[]; 
  GLCDHEADER();

  char sign[3];  // = GLCDFILE_SIGN "GLC" + X
  char format;   // D - Single Image A - Animation 
  word sizeX;    // X-size
  word sizeY;    // Y-size
  // Only on animations
  word count;    // Number of Pictures
  dword delay;   // Delay ms
};
#pragma pack()

class cGraphLCDLogo 
{
protected:
  GLCDHEADER lhdr;
  unsigned int nCurPicture;
  int tLastChange;
  std::vector<unsigned char *> bitmaps;
protected:
  void clear(void);
  cGraphLCDLogo(void);
public:
  cGraphLCDLogo(const char * logoFileName);
  virtual ~cGraphLCDLogo();

  virtual bool Load(const char * fileName, bool bInvert = false);

  unsigned char * DataPtr();
  int Width()                   { return lhdr.sizeX; };
  int Height()                  { return lhdr.sizeY; };
  
  // select first image
  void First(int t)             { tLastChange=t; nCurPicture = 0; };
  // try to select next image, return false on last image
  bool Next(int t)              { tLastChange=t; ++nCurPicture;return nCurPicture < bitmaps.size(); };
  // Deliver Delay between two images on animations
  int Delay(void) const         { return lhdr.delay; }
  void SetDelay(unsigned int d) {  lhdr.delay = d; }
  // deliver last time of change
  int LastChange(void) const    { return tLastChange; }

  
  unsigned int Cnt() const      { return bitmaps.size();}
};

#endif
