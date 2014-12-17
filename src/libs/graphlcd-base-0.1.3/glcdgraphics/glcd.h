/*
 * GraphLCD graphics library
 *
 * glcd.h  -  GLCD file loading and saving
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_GLCD_H_
#define _GLCDGRAPHICS_GLCD_H_


namespace GLCD
{

class cImage;

class cGLCDFile
{
public:
	cGLCDFile();
	~cGLCDFile();
	bool Load(cImage & image, const std::string & fileName);
	bool Save(cImage & image, const std::string & fileName);
};

} // end of namespace

#endif
