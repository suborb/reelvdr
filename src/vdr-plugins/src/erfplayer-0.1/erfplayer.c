/***************************************************************************
 *   Copyright (C) 2007 by Dirk Leber                                      *
 *                                                                         *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "erfplayer.h"

#include "Player.h"

namespace ERFPlayer {
	static const char *VERSION     = "0.1";
	static const char *DESCRIPTION = "ERF Player plugin";
	static const char *DEFAULT_EXTENSIONS = "avi";
	static int mUseHDE = 0;

	Plugin::Plugin() : mExtensions(DEFAULT_EXTENSIONS) {
	} // Plugin::Plugin
	Plugin::~Plugin() {
	} // Plugin::~Plugin
	const char *Plugin::Version() {
		return VERSION;
	} // Plugin::Version
	const char * Plugin::Description() {
		return DESCRIPTION;
	} // Plugin::Description
	bool Plugin::HasSetupOptions() {
		return false;
	} // Plugin::HasSetupOptions
	bool Plugin::SetupParse(const char *Name, const char *Value) {
		if (!strcasecmp(Name, "Extensions")) mExtensions = Value;
		else if (!strcasecmp(Name, "Language")) mDefLang = Value;
		else
			return false;
		return true;
	} // Plugin::SetupParse
	bool Plugin::Initialize() {
		return true;
	} // Plugin::Initialize
	bool Plugin::Service(char const *id, void *data) {
		struct ERFPlayData {
			char const *filePath;
			bool result;
		};
		ERFPlayData *lData = static_cast <ERFPlayData * >(data);
		cSetupLine *lLine = Setup.Get("UseHdExt", "reelbox"); 
		if (lLine && lLine->Value())
			mUseHDE = atoi(lLine->Value());	
		if (!data || mUseHDE < 1)
			return false;
		if (std::strcmp(id, "ERF Play file") == 0) {
			//printf("ERFplayer: playing\n");
			::cControl * control = new Control(lData->filePath, mDefLang.c_str());
			::cControl::Launch(control);
			return lData->result = true;
		} else if (std::strcmp(id, "ERF Test file") == 0) {
			char const *lPath = lData->filePath;
			return lData->result = checkExtension(lPath);
		} // if
		return false;
	} // Plugin::Service
	bool Plugin::Start() {
		return true;
	} // Plugin::Start
	bool Plugin::checkExtension(const char *fFile) {
		//printf("Plugin::checkExtension: fFile: %s\n", fFile);
		bool lRet = true;
		if(!fFile)
			return false;
		if (!strcmp(mExtensions.c_str(), "*"))
			return true;
		const char *lExt = strrchr(fFile, '.');
		if(!lExt)
			return false;
		lExt++;
		if(!*lExt)
			return false;
#ifndef RBLITE
		// AVG always uses erfplayer for playback of vdr files
		if(!strcasecmp(lExt, "vdr"))
			return true;
#endif
		if (!strncmp(mExtensions.c_str(), "! ", 2))
			lRet = false;
		int lLen = strlen(lExt);
		const char *lPos = mExtensions.c_str();
		const char *lNext = strchr(lPos, ' ');
		while (lPos) {
			if (lNext) {
				if ((lLen == (lNext - lPos)) && !strncasecmp(lExt, lPos, lNext - lPos))
					return lRet;
				lPos = ++lNext;
				lNext = strchr(lPos, ' ');
			} else {
				if (!strcasecmp(lExt, lPos))
					return lRet;
				lPos = lNext;
			} // if
		} // while
		//printf("Plugin::checkExtension: return: %d\n", (int) !lRet);
		return !lRet;
	} // Plugin::checkExtension
} // namespace ERFPlayer

VDRPLUGINCREATOR(ERFPlayer::Plugin);     // Don't touch this!
