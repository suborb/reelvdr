/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  logolist.h  -  logo list class
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

#ifndef _GRAPHLCD_LOGOLIST_H_
#define _GRAPHLCD_LOGOLIST_H_

#include <list>

#include "logo.h"

enum ePicType
{
	ptPictureFixed,
	ptLogoSmall,
	ptLogoMedium,
	ptLogoLarge
};

class cGraphLCDLogoList
{
private:
	struct tAliasListElement
	{
		std::string channelID;
		std::string fileName;
	};

	std::string logoDir;
	std::list <cGraphLCDLogo *> logoList;
	std::list <tAliasListElement *> aliasList;

	std::string CreateFullFileName(const std::string & baseName, ePicType type);

public:
	cGraphLCDLogoList(const std::string & logodir, const std::string & cfgdir);
	~cGraphLCDLogoList();

	cGraphLCDLogo * GetLogo(const std::string & chID, ePicType type);
};

#endif
