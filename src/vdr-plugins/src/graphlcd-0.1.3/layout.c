/*
 * GraphLCD plugin for Video Disc Recorder
 *
 * layout.c  -  layout classes
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <syslog.h>

#include <fstream>

#include "global.h"
#include "layout.h"
#include "strfct.h"

#include <vdr/config.h>
#include <vdr/i18n.h>
#include <vdr/plugin.h>


cFontElement::cFontElement(const std::string & fontName)
:	name(fontName),
	type(0),
	file(""),
	size(0)
{
}

bool cFontElement::Load(const std::string & url)
{
	if (url.find("fnt:") == 0)
	{
		type = ftFNT;
		if (url[4] == '/')
			file = url.substr(4);
		else
		{
			file = cPlugin::ConfigDirectory(PLUGIN_NAME);
			file += "/fonts/";
			file += url.substr(4);
		}
		size = 0;
		return font.LoadFNT(file);
	}
	else if (url.find("ft2:") == 0)
	{
		type = ftFT2;
		std::string::size_type pos = url.find(":", 4);
		if (pos == std::string::npos)
		{
			syslog(LOG_ERR, "cFontElement::Load(): No font size specified in %s\n", url.c_str());
			return false;
		}
		std::string tmp = url.substr(pos + 1);
		size = atoi(tmp.c_str());
		if (url[4] == '/')
			file = url.substr(4, pos - 4);
		else
		{
			file = cPlugin::ConfigDirectory(PLUGIN_NAME);
			file += "/fonts/";
			file += url.substr(4, pos - 4);
		}
#if 0
		return font.LoadFT2(file, I18nCharSets()[Setup.OSDLanguage], size);
#else
                return font.LoadFT2(file, cCharSetConv::SystemCharacterTable()? cCharSetConv::SystemCharacterTable() :"UTF-8", size);
#endif
	}
	else
	{
		syslog(LOG_ERR, "cFontElement::Load(): Unknown font type in %s\n", url.c_str());
		return false;
	}
}


cFontList::cFontList()
{
}

cFontList::~cFontList()
{
	std::list <cFontElement *>::iterator it;
	cFontElement * elem;

	for (it = fonts.begin(); it != fonts.end(); it++)
	{
		elem = *it;
		delete elem;
	}
	fonts.clear();
}

bool cFontList::Load(const std::string & fileName)
{
	std::fstream file;
	char readLine[1000];
	std::string line;

#if (__GNUC__ < 3)
	file.open(fileName.c_str(), std::ios::in);
#else
	file.open(fileName.c_str(), std::ios_base::in);
#endif
	if (!file.is_open())
		return false;

	while (!file.eof())
	{
		file.getline(readLine, 1000);
		line = trim(readLine);
		if (line.length() == 0)
			continue;
		if (line[0] == '#')
			continue;
		Parse(line);
	}

	file.close();
	return true;
}

bool cFontList::Parse(const std::string & line)
{
	std::string::size_type pos;
	std::string fontName;
	std::string fontUrl;
	cFontElement * elem;

	pos = line.find("=");
	if (pos == std::string::npos)
		return false;
	fontName = trim(line.substr(0, pos));
	fontUrl = trim(line.substr(pos + 1));
	//printf("%s = %s\n", fontName.c_str(), fontUrl.c_str());

	elem = new cFontElement(fontName);
	if (elem->Load(fontUrl))
	{
		fonts.push_back(elem);
		return true;
	}
	else
	{
		delete elem;
		return false;
	}
}

const GLCD::cFont * cFontList::GetFont(const std::string & name) const
{
	std::list <cFontElement *>::const_iterator it;
	cFontElement * elem;

	for (it = fonts.begin(); it != fonts.end(); it++)
	{
		elem = *it;
		if (elem->Name() == name)
		{
			return elem->Font();
		}
	}
	return NULL;
}

