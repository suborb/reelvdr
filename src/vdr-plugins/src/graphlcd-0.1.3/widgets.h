/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  widgets.h  -  display widgets
 *
 *  (c) 2004 Andreas Regel <andreas.regel AT powarman de>
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

#ifndef _GRAPHLCD_WIDGETS_H_
#define _GRAPHLCD_WIDGETS_H_

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>

class cScroller
{
private:
	int x;
	int y;
	int xmax;
	const GLCD::cFont * font;
	std::string text;
	bool active;
	bool update;
	int position;
	int increment;
	unsigned long long int lastUpdate;
public:
	cScroller();
	const std::string & Text() const { return text; }
	bool NeedsUpdate();
	void Reset();
	void Init(int x, int y, int xmax, const GLCD::cFont * font, const std::string & text);
	void Draw(GLCD::cBitmap * bitmap);
};

#endif

