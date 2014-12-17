/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  bmp.c  -  bmp logo class
 *
 *  (C) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (C) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
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

#include "bmp.h"


#pragma pack(1)
typedef struct BMPH {
  word    bmpIdentifier;
  dword   bmpFileSize;
  dword   bmpReserved;
  dword   bmpBitmapDataOffset;
  dword   bmpBitmapHeaderSize;
  dword   bmpWidth;
  dword   bmpHeight;
  word    bmpPlanes;
  word    bmpBitsPerPixel;
  dword   bmpCompression;
  dword   bmpBitmapDataSize;
  dword   bmpHResolution;
  dword   bmpVResolution;
  dword   bmpColors;
  dword   bmpImportantColors;
} BMPHEADER;  // 54 bytes

typedef struct RGBQ {
    byte    rgbBlue;
    byte    rgbGreen;
    byte    rgbRed;
    byte    rgbReserved;
} RGBQUAD;    // 4 bytes
#pragma pack()


byte bitmask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
byte bitmaskl[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
byte bitmaskr[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

cBMP::cBMP()
{
}

cBMP::cBMP(const cGraphLCDLogoExt& x)
:cGraphLCDLogoExt(x) 
{
  
}

cBMP::~cBMP()
{
}

bool cBMP::Load(const char* szFileName, bool bInvert) {

  FILE         *fIN;
  BMPHEADER     bmpHeader;
  RGBQUAD      *pPalette;
  char         *pByte;
  char          Dummy;
  long          iNumColors;
  long          iSize;
  dword         x, y;
  word          iRead;
  unsigned char *bitmap = NULL;

  if(szFileName) {
    fIN = fopen(szFileName, "rb");
    if(fIN) {

      if(fread(&bmpHeader, sizeof(BMPHEADER), 1, fIN)!=1) 
      { 
        fclose(fIN); 
        return false; 
      }

      // check for Windows BMP
      if(bmpHeader.bmpBitmapHeaderSize != 0x00000028 ) {
        fprintf(stderr, "ERROR: only Windows BMP images are allowed.\n");
        fclose(fIN); 
        return false; 
      }

      // check for 2 color
      iNumColors = (1 << bmpHeader.bmpBitsPerPixel);
      if(iNumColors != 2) {
        fprintf(stderr, "ERROR: the image has %ld colors, but only images with 2 colors are allowed.\n", iNumColors);
        fclose(fIN); 
        return false; 
      }


      iSize = bmpHeader.bmpHeight * bmpHeader.bmpWidth;

      pPalette = (RGBQUAD *) malloc( iNumColors*sizeof(RGBQUAD));
      if(!pPalette) {
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        fclose(fIN); 
        return false; 
      }

      if(fread( pPalette, iNumColors*sizeof(RGBQUAD), 1, fIN)!=1) {
        free(pPalette);
        fclose(fIN); 
        return false; 
      }

      // check colors
      if(pPalette->rgbBlue+pPalette->rgbGreen+pPalette->rgbRed <
         (pPalette+1)->rgbBlue+(pPalette+1)->rgbGreen+(pPalette+1)->rgbRed ) {
        // index 0 represents 'black', index 1 'white'
        bInvert = !bInvert;
      } else {
        // index 0 represents 'white', index 1 'black'
      }

      if(fseek(fIN, bmpHeader.bmpBitmapDataOffset, SEEK_SET)==EOF) {
        free(pPalette);
        fclose(fIN); 
        return false; 
      }

      switch (bmpHeader.bmpCompression) {
        case 0: // BI_RGB       no compression
      if(bitmaps.size()==0) {
          lhdr.sizeY = bmpHeader.bmpHeight;
          lhdr.sizeX = bmpHeader.bmpWidth;

          bitmap = new unsigned char[lhdr.sizeY * ((lhdr.sizeX + 7) / 8)];
          if(!bitmap) {
            fprintf(stderr, "ERROR: cannot allocate memory\n");
            free(pPalette);
            fclose(fIN); 
            return false; 
          }

          for(y=bmpHeader.bmpHeight; y>0; y--) {
            pByte = (char*)bitmap + (y-1)*((bmpHeader.bmpWidth+7)/8);
            iRead = 0;
            for(x=0; x<bmpHeader.bmpWidth/8; x++) {

              if(fread( pByte , sizeof(char), 1, fIN)!=1) {
                delete [] bitmap;
                
                free(pPalette);
                fclose(fIN); 
                return false; 
              }
              iRead++;
              if(bInvert) *pByte = *pByte^0xff;
              pByte++;
            }

            if(bmpHeader.bmpWidth%8) {
              if(fread( pByte , sizeof(char), 1, fIN)!=1) {
                delete [] bitmap;
                
                free(pPalette);
                fclose(fIN); 
                return false; 
              }
              iRead++;
              if(bInvert) *pByte = *pByte^0xff;
              *pByte = *pByte & bitmaskl[bmpHeader.bmpWidth%8];
              pByte++;
            }

            // Scan line must be 4-byte-alligned
            while (iRead%4) {
              if(fread(&Dummy, sizeof(char), 1, fIN)!=1) {
                delete [] bitmap;
                
                free(pPalette);
                fclose(fIN); 
                return false; 
              }
              iRead++;
            }
          }
          break;
        }
        else {
          fprintf(stderr, "ERROR: only one image are allowed.\n");
          
          free(pPalette);
          fclose(fIN); 
          return false; 
        }
        case 1: // BI_RLE4      RLE 4bit/pixel
        case 2: // BI_RLE8      RLE 8bit/pixel
        case 3: // BI_BITFIELDS
        default:
          fprintf(stderr, "ERROR: only uncompressed RGB images are allowed.\n");
          
          free(pPalette);
          fclose(fIN); 
          return false; 
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


bool cBMP::Save(const char* szFileName) 
{
  FILE         *fOut;
  BMPHEADER     bmpHeader;
  RGBQUAD       bmpColor1, bmpColor2;
  dword         dBDO, dBDSx, dBDS;
  char         *pByte;
  char          Dummy = 0x00;
  dword         x, y;
  word          iWrote;

  unsigned char* bitmap = DataPtr();

  if(bitmap
     && lhdr.sizeX
     && lhdr.sizeY) {
    memset(&bmpHeader, 0, sizeof(BMPHEADER));

    dBDO = sizeof(BMPHEADER)+2*sizeof(RGBQUAD);
    dBDSx = ((lhdr.sizeX+7)/8+3) & 0xfffffffc;
    dBDS = dBDSx * lhdr.sizeY;

    bmpHeader.bmpIdentifier       = 0x4d42; // "BM"
    bmpHeader.bmpFileSize         = dBDO + dBDS;
    bmpHeader.bmpBitmapDataOffset = dBDO;
    bmpHeader.bmpBitmapHeaderSize = 0x28;
    bmpHeader.bmpWidth            = lhdr.sizeX;
    bmpHeader.bmpHeight           = lhdr.sizeY;
    bmpHeader.bmpPlanes           = 0x01;
    bmpHeader.bmpBitsPerPixel     = 0x01;
    bmpHeader.bmpCompression      = 0x00;
    bmpHeader.bmpBitmapDataSize   = dBDS;
    bmpHeader.bmpHResolution      = 0xb13;  // 72dpi
    bmpHeader.bmpVResolution      = 0xb13;  // 72dpi
    bmpHeader.bmpColors           = 0x02;
    bmpHeader.bmpImportantColors  = 0x02;

    bmpColor1.rgbBlue     = 0x00;
    bmpColor1.rgbGreen    = 0x00;
    bmpColor1.rgbRed      = 0x00;
    bmpColor1.rgbReserved = 0x00;
    bmpColor2.rgbBlue     = 0xff;
    bmpColor2.rgbGreen    = 0xff;
    bmpColor2.rgbRed      = 0xff;
    bmpColor2.rgbReserved = 0x00;


    fOut = fopen(szFileName,"wb");
   	if(!fOut) {
  		fprintf(stderr,"Cannot create file: %s\n",szFileName);
   		return false;
   	}
   	fwrite(&bmpHeader, sizeof(BMPHEADER), 1, fOut);
    fwrite(&bmpColor1, sizeof(RGBQUAD), 1, fOut);
    fwrite(&bmpColor2, sizeof(RGBQUAD), 1, fOut);
    
    for(y=lhdr.sizeY; y>0; y--) {
      pByte = (char*)bitmap + (y-1)*((lhdr.sizeX+7)/8);
      iWrote = 0;
      for(x=0; x<(dword) lhdr.sizeX/8; x++) {

        *pByte = *pByte^0xff;
        if(fwrite( pByte , sizeof(char), 1, fOut)!=1) {
          fclose(fOut);
          return false;
        }
        iWrote++;
        pByte++;
      }
      // Scan line must be 4-byte-alligned
      while (iWrote%4) {
        if(fwrite(&Dummy, sizeof(char), 1, fOut)!=1) {
          fclose(fOut);
          return 3;
        }
        iWrote++;
      }
    }
    fclose(fOut);
  }
  return true;
}
