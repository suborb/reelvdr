/*
 * GraphLCD plugin for Video Disc Recorder
 *
 * layout.h  -  layout classes
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <list>
#include <string>

#include <glcdgraphics/font.h>

typedef enum
{
	ftFNT,
	ftFT2
} eFontTypes;

class cFontElement
{
private:
	std::string name;
	int type;
	std::string file;
	int size;
	GLCD::cFont font;
public:
	cFontElement(const std::string & fontName);
	bool Load(const std::string & url);

	const std::string & Name() const { return name; }
	int Type() const { return type; }
	const std::string & File() const { return file; }
	int Size() const { return size; }
	const GLCD::cFont * Font() const { return &font; }
};


class cFontList
{
private:
	std::list <cFontElement *> fonts;
public:
	cFontList();
	~cFontList();
	bool Load(const std::string & fileName);
	bool Parse(const std::string & line);

	const GLCD::cFont * GetFont(const std::string & name) const;
};
