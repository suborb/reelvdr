/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  logolist.c  -  logo list class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
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

#include <fstream>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>
#include <glcdgraphics/glcd.h>

#include "logolist.h"
#include "strfct.h"

#include <vdr/tools.h>

const char * kGLCDFileExtension = ".glcd";
const char * kAliasFileName = "logonames.alias";

cGraphLCDLogoList::cGraphLCDLogoList(const std::string & logodir, const std::string & cfgdir)
{
	std::fstream file;
	char readLine[1000];
	std::string line;
	std::string aliasFileName;
	std::string::size_type pos;
	tAliasListElement * newAlias;

	logoDir = logodir;
	aliasFileName = AddDirectory(cfgdir.c_str(), kAliasFileName);

#if (__GNUC__ < 3)
	file.open(aliasFileName.c_str(), std::ios::in);
#else
	file.open(aliasFileName.c_str(), std::ios_base::in);
#endif
	if (!file.is_open())
		return;

	while (!file.eof())
	{
		file.getline(readLine, 1000);
		line = trim(readLine);
		if (line.length() == 0)
			continue;
		if (line[0] == '#')
			continue;
		pos = line.find(":");
		if (pos == std::string::npos)
			continue;
		newAlias = new tAliasListElement;
		if (newAlias)
		{
			newAlias->channelID = trim(line.substr(0, pos));
			newAlias->fileName = trim(line.substr(pos + 1));
			aliasList.push_back(newAlias);
		}
	}
	file.close();
/*
	std::list <tAliasListElement *>::iterator it;
	for (it = aliasList.begin(); it != aliasList.end(); it++)
	{
		newAlias = *it;
		printf(">>>>>> AliasList:  >%s< : >%s<\n", newAlias->channelID.c_str(), newAlias->fileName.c_str());
	}
*/
}

cGraphLCDLogoList::~cGraphLCDLogoList()
{
	std::list <tAliasListElement *>::iterator itAlias;
	std::list <cGraphLCDLogo *>::iterator itLogo;

	for (itAlias = aliasList.begin(); itAlias != aliasList.end(); itAlias++)
	{
		delete *itAlias;
	}
	aliasList.empty();

	for (itLogo = logoList.begin(); itLogo != logoList.end(); itLogo++)
	{
		delete *itLogo;
	}
	logoList.empty();
}

std::string cGraphLCDLogoList::CreateFullFileName(const std::string & baseName, ePicType type)
{
	std::string tmp;

	tmp = AddDirectory(logoDir.c_str(), baseName.c_str());

	switch (type)
	{
		case ptPictureFixed:  // do not attach anything
			break;
		case ptLogoSmall:
			tmp += "_s";
			break;
		case ptLogoMedium:
			tmp += "_m";
			break;
		case ptLogoLarge:
			tmp += "_l";
			break;
	}
	tmp += kGLCDFileExtension;

	return tmp;
}

cGraphLCDLogo * cGraphLCDLogoList::GetLogo(const std::string & chID, ePicType type)
{
	std::list <cGraphLCDLogo *>::iterator itLogo;
	std::list <tAliasListElement *>::iterator itAlias;
	std::string logoFileName = "";
	cGraphLCDLogo * newLogo;
	GLCD::cGLCDFile glcd;

	for (itLogo = logoList.begin(); itLogo != logoList.end(); itLogo++)
	{
		if ((*itLogo)->ID() == chID)
		{
			return *itLogo;
		}
	}

	for (itAlias = aliasList.begin(); itAlias != aliasList.end(); itAlias++)
	{
		if ((*itAlias)->channelID == chID)
		{
			logoFileName = CreateFullFileName((*itAlias)->fileName, type);
			break;
		}
	}
	if (itAlias == aliasList.end())
		logoFileName = CreateFullFileName(chID, type);

	// try to load logo
	newLogo = new cGraphLCDLogo(chID);
	if (glcd.Load(*newLogo, logoFileName))
	{
		logoList.push_back(newLogo);
		return newLogo;
	}
	else
	{
		delete newLogo;
		return NULL;
	}
	return NULL;
}
