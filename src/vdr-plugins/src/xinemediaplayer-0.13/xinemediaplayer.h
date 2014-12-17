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

#define MAINMENUENTRY trNOOP("Mediaplayer")
static char *VERSION = "0.13"

namespace Reel {

namespace XineMediaplayer {

struct LastPlayedList
{
    LastPlayedList();
    ~LastPlayedList();

    std::vector<std::string> playlist;

    void SetFilename(std::string Filename="");
    
    // replace playlist with given playlist and save the resulting playlist
    void SetPlaylist(const std::vector<std::string>& newPlaylist);

    bool Read();
    bool Save();

    std::string filename;
};

    extern LastPlayedList savedPlaylist;
    extern int remember_playlist;
    
    extern std::string currentPlaylistFilename;

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
 	    virtual const char *Version(void) { return VERSION; }
//            virtual PCSTR Version();
            virtual bool SetupParse(const char *Name, const char *Value); 
                        cString SVDRPCommand(const char*, const char*, int&);
                        const char** SVDRPHelpPages();
                        const char* MainMenuEntry() { return tr(MAINMENUENTRY) ;};
        private:
            Plugin(Plugin const &); // Forbid copy construction.
            Plugin &operator=(Plugin const &); // Forbid copy assignment.
            bool checkExtension(const char *fFile);
            std::string mExtensions;
    }; // Plugin
};
};

#endif // XINE_MEDIAPLAYER_XINE_MEDIAPLAYER_H_INCLUDED
