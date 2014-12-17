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

// mpegplayer.h

#ifndef ERFPLAYER_H_INCLUDED
#define ERFPLAYER_H_INCLUDED

#include <vdr/plugin.h>

#include <string>

namespace ERFPlayer {
	class Plugin : public::cPlugin {
		public:
			Plugin();
			~Plugin();
			virtual const char *Description();
			virtual bool HasSetupOptions();
			virtual bool Initialize();
			virtual bool Service(char const *, void *);
			virtual bool Start();
			virtual const char * Version();
			virtual bool SetupParse(const char *Name, const char *Value); 
		protected:
			Plugin(Plugin const &);   // Forbid copy construction.
			Plugin & operator=(Plugin const &);       // Forbid copy assignment.
			bool checkExtension(const char *fFile);
			std::string mExtensions;
			std::string mDefLang;
	}; // Plugin
} // namespace ERFPlayer

#endif // ERFPLAYER_H_INCLUDED
