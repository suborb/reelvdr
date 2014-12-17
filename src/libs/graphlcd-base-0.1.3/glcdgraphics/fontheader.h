/*
 * GraphLCD graphics library
 *
 * fontheader.h  -  font header definition
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_FONTHEADER_H_
#define _GLCDGRAPHICS_FONTHEADER_H_

namespace GLCD
{

static const char * kFontFileSign = "FNT3";

#pragma pack(1)
struct tFontHeader
{
	char sign[4];          // = FONTFILE_SIGN
	unsigned short height; // total height of the font
	unsigned short ascent; // ascender of the font
	unsigned short line;   // line height
	unsigned short reserved;
	unsigned short space;  // space between characters of a string
	unsigned short count;  // number of chars in this file
};

struct tCharHeader
{
	unsigned short character;
	unsigned short width;
};
#pragma pack()

} // end of namespace

#endif
