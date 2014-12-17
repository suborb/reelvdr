/*
 * GraphLCD graphics library
 *
 * image.h  -  image and animation handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_IMAGE_H_
#define _GLCDGRAPHICS_IMAGE_H_

#include <vector>

namespace GLCD
{

class cBitmap;

class cImage
{
friend class cGLCDFile;

private:
	unsigned int width;
	unsigned int height;
	unsigned int delay;
	unsigned int curBitmap;
	unsigned long long lastChange;
	std::vector <cBitmap *> bitmaps;
public:
	cImage();
	~cImage();

	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }
	unsigned int Count() const { return bitmaps.size(); }
	unsigned int Delay() const { return delay; }
	unsigned long long LastChange() const { return lastChange; }
	void First(unsigned long long t) { lastChange = t; curBitmap = 0; }
	bool Next(unsigned long long t) { lastChange = t; curBitmap++; return curBitmap < bitmaps.size(); }
	void SetDelay(unsigned int d) { delay = d; }
	const cBitmap * GetBitmap(unsigned int nr) const;
	const cBitmap * GetBitmap() const;
	void Clear();
};

} // end of namespace

#endif
