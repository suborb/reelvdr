/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  tiff.c  -  tiff logo class
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

#include "tiff.h"

#pragma pack(1)
typedef struct TIFFT{
  unsigned short  tag;
  unsigned short  type;
  unsigned long   length;
  /*     1 = BYTE.      8-bit unsigned integer.                                                                  */
  /*     2 = ASCII.     8-bit bytes that store ASCII codes; the last byte must be null.                          */
  /*     3 = SHORT.     A 16-bit (2-byte) unsigned integer.                                                      */
  /*     4 = LONG.      A 32-bit (4-byte) unsigned integer.                                                      */
  /*     5 = RATIONAL.  Two LONGs: the first represents the numerator of a fraction, the second the denominator. */
  unsigned long   off_val;
} TIFFTAG;
#pragma pack()

#define GETANDCHECK { t=fgetc(fIN);if(t==EOF) {fclose(fIN);return false;};}

cTIFF::cTIFF()
{
}

cTIFF::cTIFF(const cGraphLCDLogoExt& x)
 :cGraphLCDLogoExt(x) 
{
  
}

cTIFF::~cTIFF()
{
}

bool cTIFF::Load(const char* szFileName, bool bInvert) {

  FILE         *fIN;
  TIFFTAG       tifftag;
  unsigned int  tiff_header, tiff_anztags, tiff_data;
  unsigned char cl,ch,y,i;
  unsigned char height, width, strip, invert;
  unsigned char fLittleEndian=0;
  unsigned int  j;
  int t;
  unsigned char *bitmap = NULL;

  if(szFileName) {
    fIN = fopen(szFileName, "rb");
    if(fIN) {

  //    isyslog("graphlcd plugin: try to load logo %s.", szFileName);
      if(fseek(fIN, 0, SEEK_SET)==EOF)
      { 
        fclose(fIN); 
        return false; 
      }
      GETANDCHECK; cl=(unsigned char)t;
      GETANDCHECK; ch=(unsigned char)t;
      if((cl==0x49) && (ch==0x49)) {
        fLittleEndian=1;
      }

      if(fseek(fIN, 4, SEEK_SET)==EOF)
      { 
        fclose(fIN); 
        return false; 
      }
      GETANDCHECK; cl=(unsigned char)t;
      GETANDCHECK; ch=(unsigned char)t;
      tiff_header = cl+256*ch;
  //printf("tiff_header:%d %x\n", tiff_header, tiff_header);

      if(fseek(fIN, tiff_header, SEEK_SET)==EOF)
      { 
        fclose(fIN); 
        return false; 
      }
        
      GETANDCHECK; cl=(unsigned char)t;
      GETANDCHECK; ch=(unsigned char)t;
      tiff_anztags = cl+256*ch;
  //printf("tiff_anztags:%d %x\n", tiff_anztags, tiff_anztags);

      height=0;
      width=0;
      strip=0;
      invert=0;
      for(i=0; (i<tiff_anztags)&&(!height||!width||!strip||!invert); i++) {
        if(fread(&tifftag, sizeof(tifftag), 1, fIN)!=1)
        { 
          fclose(fIN); 
          return false; 
        }
        if(tifftag.tag==0x0100) width=tifftag.off_val;
        if(tifftag.tag==0x0101) height=tifftag.off_val;
        if(tifftag.tag==0x0111) strip=tifftag.off_val;
        if(tifftag.tag==0x0106) invert=tifftag.off_val+1;
  //printf("tag%d: %d %d %ld %ld\n", i,tifftag.tag, tifftag.type, tifftag.length, tifftag.off_val );
      }

      if(fseek(fIN,strip, SEEK_SET)==EOF)
      { 
        fclose(fIN); 
        return false; 
      }
      GETANDCHECK; cl=(unsigned char)t;
      GETANDCHECK; ch=(unsigned char)t;
      tiff_data = cl+256*ch;
  //printf("tiff_data:%d %x\n", tiff_data, tiff_data);

      if(fseek(fIN, tiff_data, SEEK_SET)==EOF)
      { 
        fclose(fIN); 
        return false; 
      }


      if(bitmaps.size()==0) {
        lhdr.sizeX = width;
        lhdr.sizeY = height;
        bitmap = new unsigned char[lhdr.sizeY * ((lhdr.sizeX + 7) / 8)];
        if(bitmap) {
          if(fread(bitmap, lhdr.sizeY*((lhdr.sizeX+7)/8), 1, fIN)!=1) {
            delete [] bitmap;
            fclose(fIN); 
            return false; 
          }

          if(invert-1==1) bInvert = !bInvert; // 'Black is zero'
          if(bInvert) {
            for(j=0; j<(unsigned int)lhdr.sizeY*(((unsigned int)lhdr.sizeX+7)/8); j++) {
              (*(bitmap+j)) = (*(bitmap+j))^0xff;
            }
          }

          // cut the rest of the line
          if(lhdr.sizeX%8) {
            for(y=1; y<=lhdr.sizeY; y++) {
              j=y*((lhdr.sizeX+7)/8)-1;
              (*(bitmap+j)) = ((*(bitmap+j))>>(8-lhdr.sizeX%8))<<(8-lhdr.sizeX%8);
            }
          }
        } else {
          fprintf(stderr, "ERROR: cannot allocate memory\n");
        }
      } else {
        fprintf(stderr, "ERROR: only one image are allowed.\n");
      }
      fclose(fIN);
    } else {
      fprintf(stderr, "ERROR: cannot open picture %s\n", szFileName);
    }
  } else {
    fprintf(stderr, "ERROR: no szFileName given!\n");
  }

  bitmaps.push_back(bitmap);
  return true;
}
