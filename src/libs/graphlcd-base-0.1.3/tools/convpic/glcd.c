/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  glcd.c  -  glcd logo class
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

#include <stdio.h>
#include <string.h>

#include "glcd.h"

cGraphLCDLogoExt::cGraphLCDLogoExt()
{
}

cGraphLCDLogoExt::~cGraphLCDLogoExt()
{
}

cGraphLCDLogoExt::cGraphLCDLogoExt(const cGraphLCDLogoExt& x)
{
  (*this) = x; 
}

cGraphLCDLogoExt& cGraphLCDLogoExt::operator=(const cGraphLCDLogoExt& x)
{
  clear();

  memcpy(&lhdr,&x.lhdr,sizeof(lhdr));
  nCurPicture = x.nCurPicture;
  
  bitmaps.resize(x.bitmaps.size());
  std::vector<unsigned char *>::const_iterator xi = x.bitmaps.begin();
  std::vector<unsigned char *>::const_iterator xe = x.bitmaps.end();
  std::vector<unsigned char *>::iterator i = bitmaps.begin();
  std::vector<unsigned char *>::const_iterator e = bitmaps.end();
  for(;e != i && xe != xi;++xi,++i)
  {
    (*i) = new unsigned char[lhdr.sizeY * ((lhdr.sizeX + 7) / 8)];
    if(!(*i))
      break;  
    memcpy((*i),(*xi),lhdr.sizeY * ((lhdr.sizeX + 7) / 8));
  }
  return *this;
}

bool cGraphLCDLogoExt::operator+=(const cGraphLCDLogoExt& x)
{
  if(lhdr.sizeX == 0
    || lhdr.sizeY == 0
    || lhdr.sizeX !=  x.lhdr.sizeX 
    || lhdr.sizeY !=  x.lhdr.sizeY )
  {
    fprintf(stderr, "ERROR: Can't merge images with different size\n");
    return false;
  }
  std::vector<unsigned char *>::const_iterator xi = x.bitmaps.begin();
  std::vector<unsigned char *>::const_iterator xe = x.bitmaps.end();
  for(;xe != xi;++xi)
  {
    unsigned char* bitmap = new unsigned char[x.lhdr.sizeY * ((x.lhdr.sizeX + 7) / 8)];
    if(!bitmap)
      return false;
    memcpy(bitmap,(*xi),lhdr.sizeY * ((lhdr.sizeX + 7) / 8));
    bitmaps.push_back(bitmap);
  }
  return false;
}




cGLCD::cGLCD()
{
}

cGLCD::cGLCD(const cGraphLCDLogoExt& x)
: cGraphLCDLogoExt(x) 
{
  
}

cGLCD::~cGLCD()
{
}


bool cGLCD::Save(const char * szFileName)
{
  FILE *      fOut;

  if(bitmaps.size()
     && lhdr.sizeX
     && lhdr.sizeY) {
    memcpy(lhdr.sign, lhdr.GLCDFILE_SIGN, sizeof(lhdr.sign));
    lhdr.format = bitmaps.size() == 1 ? 'D' : 'A';
    lhdr.count  = bitmaps.size();
    fOut = fopen(szFileName,"wb");
   	if(!fOut) {
  		 fprintf(stderr,"ERROR: Cannot create file: %s\n",szFileName);
   		return false;
   	}
    const unsigned int nStdHeaderSize =         
        sizeof(lhdr.sign)
      + sizeof(lhdr.format)
      + sizeof(lhdr.sizeX)
      + sizeof(lhdr.sizeY);

    if(lhdr.format == 'D')
    {
   	 if(1 != fwrite(&lhdr, nStdHeaderSize, 1, fOut))
     {  
       fprintf(stderr,"ERROR: Cannot write fileheader: %s\n",szFileName);
       fclose(fOut);
       return false;
     }
    }
    else //if(lhdr.format == 'A')
    {
   	 if(1 != fwrite(&lhdr, sizeof(GLCDHEADER), 1, fOut))
     {  
       fprintf(stderr,"ERROR: Cannot write fileheader: %s\n",szFileName);
       fclose(fOut);
       return false;
     }
    }
    for(unsigned int n=0;n<bitmaps.size();++n)
    {
     if(1 != fwrite(bitmaps[n],lhdr.sizeY*((lhdr.sizeX+7)/8), 1, fOut))
     {  
       fprintf(stderr,"ERROR: Cannot write filedata: %s\n",szFileName);
       fclose(fOut);
       return false;
     }
    }

    fclose(fOut);
  }
  return true;
}
