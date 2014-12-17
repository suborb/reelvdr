/*
*  C Implementation: pngutil.h
*
* Description: 
*
*
* Author: RealQuick <realquick@doublequick.de>, (C) 2009
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef __PNGUTILS_H
#define __PNGUTILS_H

#include <png.h>
#include <vdr/plugin.h>

#define FILEHEADER_TAG "cPnG"

class cPngUtils
{
private:
  int bilinear_interpolation_read(double x, double y, int colour);

  /*typedef*/ struct imgheader
  { 
       char tag[4];
       int xres;
       int yres;
  };

public:
  cPngUtils();
  cPngUtils(int w, int h);
  cPngUtils(unsigned char *bmp, int w, int h);

  int downloadURL ( const char *URL, const char *filename );

  ~cPngUtils();

  int Width() {return width;};
  int Height() {return height;};
  int Rowsize() {return rowsize;};

  int ReadJpeg(const char *sPath);
  int DumpRaw(const char *sPath, int step=1);

  int WriteFile(unsigned char nTransparenz, int xpos, int ypos,const char *sPath, bool bHasTransparency=false, int rel_width=0, int rel_height=0, int rel_rowsize=0);
  int WriteFileOsd(cOsd *Osd, unsigned char nTransparenz, int x, int y, int show_lines, int *nImageIdPos, bool bHasTransparency=false);

  void DrawBitmap(cOsd *Osd, int xpos, int ypos, int step=1, int maxheight=0);

//  bool DrawRawBitmap(cOsd *Osd, char *path, int xpos, int ypos, int maxheight=0);

  bool ReadPng(const char *path);
  bool ReadRawBitmap(const char *path);
  bool PngIsReadable(const char *path);

  bool resize_image(int newWidth, int newHeight = 0);	// default filter bilinear
  bool resize_image_nearest_neighbour(int new_rwidth);	// better performance, less quality

  int read(int intx,int inty,int colour);

  int nImageIdPos;

  int nMaxPNGWidth;
  int nMaxPNGHeight;

  unsigned char *get_bitmap() {return bmp;};

protected:
  unsigned char *bmp;
  bool HasTransparency;
  int width;
  int height;
  int rowsize;

};

#endif //__PNGUTILS_H
