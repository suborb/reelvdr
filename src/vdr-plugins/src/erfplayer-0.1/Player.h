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

#ifndef ERFPLAYER_PLAYER_H_INCLUDED
#define ERFPLAYER_PLAYER_H_INCLUDED

#include <vdr/player.h>
#include <string>
#include <map>
#include "Thread.h"
#include "rawplayer.h"

namespace ERFPlayer {
	class Player;
	class Control : public ::cControl {
		public:
			Control(char const *filePath, const char *defLang);
			~Control();
			virtual void Hide();
			virtual ::cOsdObject * GetInfo();
			virtual eOSState ProcessKey(eKeys key);
		protected:
			Control(Control const &);
			Control & operator=(Control const &);
			void ProgressOff();
			void ProgressOn();
			void ShowProgress();
			void Stop();
			void UpdateProgress();
		
			Player *mPlayer;
			::cSkinDisplayReplay * mSkinDisplayReplay;
			::cSkinDisplayTracks * mSkinDisplayTracks;
			bool mShowProgress;
			std::string mTitle;
			int mDispReplCurrent, mDispReplTotal, mDispReplSpeed, mDisplTrack;
			bool mDispReplPlay, mDispReplDir;
	}; // class Control 
	class Player:public ::cPlayer {
		public:
			Player(const char *filePath, const char *defLang);
			~Player();
			virtual void Activate(bool on);
			virtual bool GetIndex(int &current, int &total, bool snapToIFrame);
			virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
			RawPlayer *mPlayer;
		protected:
			Player(Player const &);
			Player & operator=(Player const &);
			std::string const filePath_;
			std::string const defLang_;
	}; // class Player
} // namespace ERFPlayer

#endif // ERFPLAYER_PLAYER_H_INCLUDED
