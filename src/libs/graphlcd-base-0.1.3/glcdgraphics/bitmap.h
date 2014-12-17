/*
 * GraphLCD graphics library
 *
 * bitmap.h  -  cBitmap class
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_BITMAP_H_
#define _GLCDGRAPHICS_BITMAP_H_

#include <string>

namespace GLCD
{

enum eColor
{
	clrBlack,
	clrWhite
};

class cFont;

class cBitmap
{
protected:
	int width;
	int height;
	int lineSize;
	unsigned char * bitmap;

public:
	cBitmap(int width, int height, unsigned char * data = NULL);
	~cBitmap();

	int Width() const { return width; }
	int Height() const { return height; }
	int LineSize() const { return lineSize; }
	const unsigned char * Data() const { return bitmap; }

	void Clear();
	void DrawPixel(int x, int y, eColor color);
	void Draw8Pixels(int x, int y, unsigned char pixels, eColor color);
	void DrawLine(int x1, int y1, int x2, int y2, eColor color);
	void DrawHLine(int x1, int y, int x2, eColor color);
	void DrawVLine(int x, int y1, int y2, eColor color);
	void DrawRectangle(int x1, int y1, int x2, int y2, eColor color, bool filled);
	void DrawRoundRectangle(int x1, int y1, int x2, int y2, eColor color, bool filled, int size);
	void DrawEllipse(int x1, int y1, int x2, int y2, eColor color, bool filled, int quadrants);
	void DrawBitmap(int x, int y, const cBitmap & bitmap, eColor color);
	int DrawText(int x, int y, int xmax, const std::string & text, const cFont * font,
	             eColor color = clrBlack, bool proportional = true, int skipPixels = 0);
	int DrawCharacter(int x, int y, int xmax, int c, const cFont * font,
	                  eColor color = clrBlack, int skipPixels = 0);

	cBitmap * SubBitmap(int x1, int y1, int x2, int y2) const;
	unsigned char GetPixel(int x, int y) const;

	bool LoadPBM(const std::string & fileName);
	void SavePBM(const std::string & fileName);
};

#ifdef HAVE_FREETYPE2
class cBitmapFt2: public cBitmap
{
private:
protected:
    int charcode;
public:
    //cBitmapFt2(void):cBitmap(1,1) {charcode=0;};
    cBitmapFt2(int width, int height, int ch, unsigned char * data = NULL);
    ~cBitmapFt2();
    int GetCharcode( void ) const;

};

class cBitmapCache
{
private:
protected:
public:
	cBitmapCache *start; // start bitmap
	cBitmapCache *next;  // next  bitmap
	cBitmapCache *last;  // last  bitmap
	void *pointer;
	
	cBitmapCache();
	cBitmapCache( void * );
	~cBitmapCache();
	
	bool PushBack( void * );
};
#endif

} // end of namespace

#endif
