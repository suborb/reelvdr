/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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

// xinemediaplayer.h
 
#ifndef XINE_MEDIAPLAYER_XINE_MEDIAPLAYER_H_INCLUDED
#define XINE_MEDIAPLAYER_XINE_MEDIAPLAYER_H_INCLUDED

#include <vdr/plugin.h>
#include <string>
#include "xineOsd.h"

#include "Reel.h"

namespace Reel
{
namespace XineMediaplayer 
{
    class Plugin : public ::cPlugin {
        public:
            Plugin();
            ~Plugin();
            virtual PCSTR Description();
            virtual bool HasSetupOptions();
            cOsdObject* MainMenuAction();
            virtual bool Initialize();
            virtual bool Service(char const *id, void *data);
            virtual bool Start();
            virtual PCSTR Version();
            virtual bool SetupParse(const char *Name, const char *Value); 
                        cString SVDRPCommand(const char*, const char*, int&);
                        const char** SVDRPHelpPages();
        private:
            Plugin(Plugin const &); // Forbid copy construction.
            Plugin &operator=(Plugin const &); // Forbid copy assignment.
            bool checkExtension(const char *fFile);
            std::string mExtensions;
    }; // Plugin
    bool Plugin::HasSetupOptions() {
        return false;
    } // Plugin::HasSetupOptions
}
}

#endif // XINE_MEDIAPLAYER_XINE_MEDIAPLAYER_H_INCLUDED
