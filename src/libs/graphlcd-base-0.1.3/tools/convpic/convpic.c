/**
 *  convpic.c  -  a tool to convert images to
 *                own proprietary format of the logos and pictures
 *                for graphlcd plugin
 *
 *  (C) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (C) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glcd.h"
#include "bmp.h"
#include "tiff.h"
#include "tuxbox.h"

static const char *prgname = "convpic";
static const char *VERSION = "0.1.0";

int SysLogLevel = 3;
unsigned int delay   = 250;

typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned int    dword;


#define FREENULL(p) (free (p), p = NULL)





enum PicFormat { undef, TIFF, BMP, GLCD, TUXBOX };

void      usage(void);

PicFormat getFormat(const char* szFile)
{
  static const 
  struct tagformats{const char* szExt; PicFormat picformat; }
  formats [] = 
  {
    { ".tiff",   TIFF   },
    { ".tif",    TIFF   },
    { ".bmp",    BMP    },
    { ".glcd",   GLCD   },
    { ".ani",    TUXBOX }
  };
  
  PicFormat pf = undef;

  if(szFile) {
    for(int i=strlen(szFile)-1;i>=0;--i)
    {
      if(*(szFile+i) == '.' && strlen(szFile+i+1)) {
        for(unsigned int n = 0;n < sizeof(formats)/sizeof(*formats);++n)
        {
          if(!strcasecmp((szFile+i), formats[n].szExt)) {
            return formats[n].picformat;
          }
        }
      }
    }
  }
  return pf;
}


int main(int argc, char *argv[]) {
  PicFormat   inFormat  = undef;
  PicFormat   outFormat = undef;
  char*       inFile  = NULL;
  char*       nextinfile  = NULL;
  char*       outFile = NULL;
  cGraphLCDLogoExt*   pInBitmap = NULL;
  cGraphLCDLogoExt*   pNextBitmap = NULL;
  cGraphLCDLogoExt*   pOutBitmap = NULL;
  bool        bError = false;
  char*       pszTmp;
  bool        bInvert = false;
  bool        bDelay  = false;


  static struct option long_options[] = {
    { "invert",         no_argument, NULL, 'n'},
    { "infile",   required_argument, NULL, 'i'},
    { "outfile",  required_argument, NULL, 'o'},
    { "delay",    required_argument, NULL, 'd'},
    { NULL}
  };


  int c, option_index = 0;
  while((c=getopt_long(argc,argv,"ni:o:d:",long_options, &option_index))!=-1) {
    switch(c) {
      case 'n':
        bInvert = true;
        break;

      case 'i':
        inFile  = strdup(optarg);
        break;

      case 'o':
        outFile = strdup(optarg);
        break;

      case 'd':
        delay = atoi(optarg);
        bDelay = true;
        if(delay < 10)
        {
          fprintf(stderr, "Warning: You have specify a to short delay, minimum are 10 ms\n");
          delay = 10;
        }
        break;

      default:
        return 1;
    }
  }


  if(!inFile) {
    fprintf(stderr, "ERROR: You have to specify the infile (-i filename)\n");
    bError = true;
  }

  if(undef == (inFormat = getFormat(inFile))) {
    fprintf(stderr, "ERROR: You have to specify a correct extension for the %s\n", inFile);
    bError = true;
  }

  if(!outFile) {
    fprintf(stderr, "ERROR: You have to specify the outfile (-o filename)\n");
    bError = true;
  }

  if(undef == (outFormat = getFormat(outFile))) {
    fprintf(stderr, "ERROR: You have to specify a correct extension for the %s \n", outFile);
    bError = true;
  }


  if(bError) {
    usage();
    return 1;
  }


  // Load Picture
  switch(inFormat) {
    case TIFF:
      pInBitmap = new cTIFF;
      break;

    case BMP:
      pInBitmap = new cBMP;
      break;

    case GLCD:
      pInBitmap = new cGLCD;
      break;

    case TUXBOX:
      pInBitmap = new cTuxBox;
      break;

    default:
      return 2;
  }

  if(!pInBitmap) 
    return 3;

  fprintf(stdout, "loading %s\n", inFile);
  bError = !pInBitmap->Load(inFile,bInvert);
  if(!bError)
  {
    // Save Picture
    switch(outFormat) {
      case TIFF:
        fprintf(stderr, "SORRY: conversion to TIFF is not implemented at the moment.\n");
        //pOutBitmap = new cTIFF(*pInBitmap);
        break;
  
      case BMP:
        pOutBitmap = new cBMP(*pInBitmap);
        break;
  
      case GLCD:
        pOutBitmap = new cGLCD(*pInBitmap);
        break;

      case TUXBOX:
        pOutBitmap = new cTuxBox(*pInBitmap);
        break;
  
      default:
        return 4;
    }
  
    if(!pOutBitmap) 
      return 5;

    if(pOutBitmap->Cnt() > 0)
    {
      // Load more in files
      while(optind < argc && !bError) {
        nextinfile = argv[optind++];
  
        // Load next Picture
        switch(getFormat(nextinfile)) {
          case TIFF:
            pNextBitmap = new cTIFF;
            break;
      
          case BMP:
            pNextBitmap = new cBMP;
            break;
      
          case GLCD:
            pNextBitmap = new cGLCD;
            break;

          case TUXBOX:
            pNextBitmap = new cTuxBox;
            break;
      
          default:
            fprintf(stderr, "ERROR: You have to specify a correct extension for the %s\n",nextinfile);
            bError = true;
            break;
          }    
        if(!pNextBitmap) 
          break;
      
        fprintf(stdout, "loading %s\n", nextinfile);
        if(pNextBitmap->Load(nextinfile,bInvert) 
            && pNextBitmap->Cnt() > 0)
          *pOutBitmap += *pNextBitmap;
        delete pNextBitmap;
      }
      
      if(bDelay)
        pOutBitmap->SetDelay(delay);
      
      if(pOutBitmap->CanMergeImage() || pOutBitmap->Cnt() == 1)
      {
        fprintf(stdout, "saving %s\n", outFile);
        bError = !pOutBitmap->Save(outFile);
      }
      else
      {
        int n = 0;
        pOutBitmap->First(0);
        do
        {
          asprintf(&pszTmp,"%s.%d",outFile,++n);
          if(!pszTmp)
              break;
          fprintf(stdout, "saving %s\n", pszTmp);
          bError = !pOutBitmap->Save(pszTmp);
          
          FREENULL(pszTmp);
        } while(!bError && pOutBitmap->Next(0));
      }
    } 
    delete pOutBitmap;
  }
    
  delete pInBitmap;
  FREENULL(inFile);
  FREENULL(outFile);

  if(bError) {
    return 6;
  }

  fprintf(stdout, "conversion compeleted successfully.\n\n");

  return 0;
}









void
usage(void) {
  fprintf(stdout, "\n");
  fprintf(stdout, "%s v%s\n", prgname, VERSION);
  fprintf(stdout, "%s is a tool to convert images to a simple format (*.glcd)\n", prgname);
  fprintf(stdout, "        that is used by the graphlcd plugin for VDR.\n\n");
  fprintf(stdout, "  Usage: %s [-n] -i file[s...] -o outfile \n\n", prgname);
  fprintf(stdout, "  -n  --invert      inverts the output (default: none)\n");
  fprintf(stdout, "  -i  --infile      specifies the name of the input file[s]\n");
  fprintf(stdout, "  -o  --outfile     specifies the name of the output file\n");
  fprintf(stdout, "  -d  --delay       specifies the delay between multiple images [Default: %d ms] \n",delay);
  fprintf(stdout, "\n" );
  fprintf(stdout, "  example: %s -i vdr-logo.bmp -o vdr-logo.glcd \n", prgname );
}
