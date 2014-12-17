/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  logo.c  -  logo class
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

#include <stdio.h>
#include <string.h>

#include "global.h"
#include "logo.h"

const char GLCDHEADER::GLCDFILE_SIGN[] = "GLC"; 

GLCDHEADER::GLCDHEADER()
: format('\0')       // 'D' -> Single Image  or 'A' -> Animation 
, sizeX(0)           // X-size
, sizeY(0)           // Y-size
  // Only on Animation 
, count(1)           // Number of picture
, delay(250)         // default delay ms
{

}

cGraphLCDLogo::cGraphLCDLogo(const char* logoFileName)
: nCurPicture(0)         // Number of current picture
, tLastChange(0)
{
  Load(logoFileName);
}

cGraphLCDLogo::cGraphLCDLogo()
: nCurPicture(0)         // Number of current picture
, tLastChange(0)
{
  
}

cGraphLCDLogo::~cGraphLCDLogo()
{
  clear();
}

void cGraphLCDLogo::clear(void)
{
  std::vector<unsigned char *>::iterator i = bitmaps.begin();
  std::vector<unsigned char *>::const_iterator e = bitmaps.end();
  for(;e != i;++i)
  {
    if(*i)
      delete[] (*i);
    (*i) = NULL;
  }
  bitmaps.clear();
} 


bool cGraphLCDLogo::Load(const char * fileName, bool bInvert)
{
  bool ret = false;
  FILE * inFile;
  long fileLen;

  inFile = fopen(fileName, "rb");
  if (inFile)
  {
    // get len of file
    if (fseek(inFile, 0, SEEK_END))
    {
      fclose(inFile);
      return false;
    }
    fileLen = ftell(inFile);

    // rewind and get Header
    if (fseek(inFile, 0, SEEK_SET))
    {
      fclose(inFile);
      return false;
    }
    const unsigned int nStdHeaderSize =         
        sizeof(lhdr.sign)
      + sizeof(lhdr.format)
      + sizeof(lhdr.sizeX)
      + sizeof(lhdr.sizeY);

    // Read standard header
    if (fread(&lhdr, nStdHeaderSize, 1, inFile) != 1)
    {
      fclose(inFile);
      return false;
    }

    // check Header
    if (strncmp(lhdr.sign, lhdr.GLCDFILE_SIGN, sizeof(lhdr.sign)) ||
        !lhdr.sizeX || !lhdr.sizeY)
    {
      fprintf(stderr, "convpic: load %s failed, wrong header (cGraphLCDLogo::Load).",fileName);
      fclose(inFile);
      return false;
    }

    switch(lhdr.format)
    { 
      case 'D':
      {
        // check file length
        if ((fileLen != (long) (lhdr.sizeY * ((lhdr.sizeX + 7) / 8) + nStdHeaderSize)))
        {
          fprintf(stderr, "convpic: load %s failed, wrong size (cGraphLCDLogo::Load).",fileName);
          fclose(inFile);
          return false;
        }
        break;
      }
      case 'A':
      {
        // rewind and get Header
        if (fseek(inFile, 0, SEEK_SET))
        {
          fclose(inFile);
          return false;
        }
        if (fread(&lhdr, sizeof(lhdr), 1, inFile) != 1)
        {
          fprintf(stderr, "convpic: load %s failed, wrong header (cGraphLCDLogo::Load).",fileName);
          fclose(inFile);
          return false;
        }
        // check file length
        if (!lhdr.count 
          || (fileLen != (long) ( (lhdr.count * (lhdr.sizeY * ((lhdr.sizeX + 7) / 8))) + sizeof(lhdr))))
        {
          fprintf(stderr, "convpic: load %s failed, wrong size (cGraphLCDLogo::Load).",fileName);
          fclose(inFile);
          return false;
        }
        // Set minimal limit for next image 
        if(lhdr.delay < 10)
          lhdr.count = 10;
        break;
      }
    }
    clear();
    for(unsigned int n=0;n<lhdr.count;++n)
    {
      unsigned char *bitmap = new unsigned char[lhdr.sizeY * ((lhdr.sizeX + 7) / 8)];
      if (bitmap)
      {
        if (fread(bitmap, lhdr.sizeY * ((lhdr.sizeX + 7) / 8), 1, inFile) != 1)
        {
          delete[] bitmap;
          fclose(inFile);
          clear();
          return false;
        }

        if(bInvert)
        {
          for(int x=0;x<(lhdr.sizeY * ((lhdr.sizeX + 7) / 8));++x)
            bitmap[x] ^= 0xFF;
        }
          
        bitmaps.push_back(bitmap);
        ret = true;
      }
      else
      {
        fprintf(stderr, "convpic: malloc failed (cGraphLCDLogo::Load).");
        break;
      }
    }
    fclose(inFile);
  }
  if(ret)
    fprintf(stdout, "convpic: logo %s loaded.", fileName);
  return ret;
}

unsigned char * cGraphLCDLogo::DataPtr() 
{ 
  if(nCurPicture < bitmaps.size())
    return bitmaps[nCurPicture]; 
  return NULL;
};
