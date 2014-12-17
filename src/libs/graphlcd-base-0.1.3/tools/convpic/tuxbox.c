/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  tuxbox.c  -  tuxbox logo class
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

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "tuxbox.h"

#pragma pack(1)
struct ani_header {
	unsigned char magic[4];	// = "LCDA"
	unsigned short format;	// Format
	unsigned short width;		// Breite
	unsigned short height;	// Höhe
	unsigned short count;		// Anzahl Einzelbilder
	unsigned long delay;		// µs zwischen Einzelbildern
};
#pragma pack()

cTuxBox::cTuxBox()
{
}

cTuxBox::cTuxBox(const cGraphLCDLogoExt& x)
: cGraphLCDLogoExt(x) 
{
  
}

cTuxBox::~cTuxBox()
{
}

bool cTuxBox::Load(const char * szFileName, bool bInvert)
{
  bool ret = false;
  FILE * fIN;
  long fileLen;
	struct ani_header header;
    
  fIN = fopen(szFileName, "rb");
  if (fIN)
  {
    // get len of file
    if (fseek(fIN, 0, SEEK_END))
    {
      fclose(fIN);
      return false;
    }
    fileLen = ftell(fIN);

    // rewind and get Header
    if (fseek(fIN, 0, SEEK_SET))
    {
      fclose(fIN);
      return false;
    }

    // Read header
    if (fread(&header, sizeof(header), 1, fIN) != 1)
    {
      fclose(fIN);
      return false;
    }
    
    lhdr.sizeX = ntohs(header.width);
    lhdr.sizeY = ntohs(header.height);
    lhdr.count = ntohs(header.count); 
    lhdr.delay = ntohl(header.delay) / 1000; 
    
    // check Header
    if (strncmp((const char*)header.magic, "LCDA", sizeof(header.magic)) ||
        !lhdr.sizeX || !lhdr.sizeY || ntohs(header.format) != 0)
    {
      fprintf(stderr, "ERROR: load %s failed, wrong header.\n",szFileName);
      fclose(fIN);
      return false;
    }

    //fprintf(stderr,"%d %dx%d (%d %d) %d\n",lhdr.count,lhdr.sizeX,lhdr.sizeY,fileLen, ( (lhdr.count * (lhdr.sizeX * ((lhdr.sizeY + 7) / 8))) + sizeof(header)),lhdr.delay);

    // check file length
    if (!lhdr.count 
      || (fileLen != (long) ( (lhdr.count * (lhdr.sizeX * ((lhdr.sizeY + 7) / 8))) + sizeof(header))))
    {
      fprintf(stderr, "ERROR: load %s failed, wrong size.\n",szFileName);
      fclose(fIN);
      return false;
    }
    // Set minimal limit for next image 
    if(lhdr.delay < 10)
      lhdr.count = 10;
    clear();
    for(unsigned int n=0;n<lhdr.count;++n)
    {
      ret = false;
      unsigned int nBmpSize = lhdr.sizeY * ((lhdr.sizeX + 7) / 8);
      unsigned char *bitmap = new unsigned char[nBmpSize];
      if (!bitmap)
      {
        fprintf(stderr, "ERROR: malloc failed.");
        break;
      }
      unsigned int nAniSize = lhdr.sizeX * ((lhdr.sizeY + 7) / 8);
      unsigned char *pAni = new unsigned char[nAniSize];
      if (!pAni)
      {
        delete[] bitmap;
        fprintf(stderr, "ERROR: malloc failed.");
        break;
      }
        
      if (1 != fread(pAni, nAniSize, 1, fIN))
      {
        fprintf(stderr,"ERROR: Cannot read filedata: %s\n",szFileName);
        delete[] bitmap;
        delete[] pAni;
        break;
      }
      
      vert2horz(pAni,bitmap);
      delete[] pAni;

      if(bInvert)
        for(unsigned int i=0;i<nBmpSize;++i)
          bitmap[i] ^= 0xFF;
          
      bitmaps.push_back(bitmap);
      ret = true;
    }
    fclose(fIN);
    if(!ret)
      clear();
  }
  return ret;
}

  
bool cTuxBox::Save(const char * szFileName)
{
  FILE *      fOut;
	struct ani_header header;
  bool bRet = false;
    
  if(bitmaps.size()
     && lhdr.sizeX
     && lhdr.sizeY) {
       
    memcpy(header.magic, "LCDA", 4);
    header.format = htons(0);
    header.width = htons(lhdr.sizeX);
    header.height = htons(lhdr.sizeY);
    header.count = htons(bitmaps.size());
    header.delay = htonl(lhdr.delay * 1000); 
       
       
    if(lhdr.sizeX != 120 || lhdr.sizeY != 64)
    {
  		fprintf(stderr,"WARNING: Maybe wrong image dimension (for all I know is 120x64 wanted) %s\n",szFileName);
    }
       
    fOut = fopen(szFileName,"wb");
   	if(!fOut) {
  		fprintf(stderr,"ERROR: Cannot create file: %s\n",szFileName);
   		return false;
   	}
    
    if(1 != fwrite(&header, sizeof(header), 1, fOut))
    {  
      fprintf(stderr,"ERROR: Cannot write fileheader: %s\n",szFileName);
      fclose(fOut);
      return false;
    }

    for(unsigned int n=0;n<bitmaps.size();++n)
    {
      bRet = false;
      unsigned int nAniSize = lhdr.sizeX * ((lhdr.sizeY + 7) / 8);
      unsigned char *pAni = new unsigned char[nAniSize];
      if (!pAni)
      {
        fprintf(stderr, "ERROR: malloc failed.");
        break;
      }
      horz2vert(bitmaps[n], pAni);
      
      if(1 != fwrite(pAni, nAniSize, 1,  fOut))
      {  
        delete [] pAni;
        fprintf(stderr,"ERROR: Cannot write filedata: %s\n",szFileName);
        break;
      }
      delete [] pAni;
      bRet = true;
    }

    fclose(fOut);
  }
  return bRet;
}

/** Translate memory alignment from vertical to horizontal
rotate from {Byte} to {Byte}
{o}[o][o][o][o][o][o][o] => { oooooooo } 
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]*/
void cTuxBox::vert2horz(const unsigned char* source, unsigned char* dest) {
	int x, y, off;
  memset(dest,0,lhdr.sizeY*((lhdr.sizeX+7)/8));

	for (y=0; y<lhdr.sizeY; ++y) {
		for (x=0; x<lhdr.sizeX; ++x) {
      off = x + ((y/8) * lhdr.sizeX);
      if (source[off] & (0x1 << (y % 8)))
      {
        off = (x / 8) + (y * ((lhdr.sizeX+7)/8));
        dest[off] |= (unsigned char)(0x80 >> (x % 8));   
      }
		}
	}
}

/** Translate memory alignment from horizontal to vertical (rotate byte)
rotate from {Byte} to {Byte}
{ oooooooo } => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]*/
void cTuxBox::horz2vert(const unsigned char* source, unsigned char* dest) {
	int x, y, off;
  memset(dest,0,lhdr.sizeX*((lhdr.sizeY+7)/8));

	for (y=0; y<lhdr.sizeY; ++y) {
		for (x=0; x<lhdr.sizeX; ++x) {
      off = (x / 8) + ((y) * ((lhdr.sizeX+7)/8));
      if (source[off] & (0x80 >> (x % 8)))
      {
        off = x + ((y/8) * lhdr.sizeX);
        dest[off] |= (unsigned char)(0x1 << (y % 8));   
      }
		}
	}
}
