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

#ifndef XINEMEDIAPLAYER_SERVICES_INC
#define XINEMEDIAPLAYER_SERVICES_INC

#include <string>
#include <list>

// Data structure for service "Set Xine Mode"
// Excepts NULL for Xinemediaplayer_Mode_none
enum Xinemediaplayer_Mode {Xinemediaplayer_Mode_none =0, Xinemediaplayer_Mode_filebrowser, Xinemediaplayer_Mode_shoutcast, Xinemediaplayer_Mode_mmfiles, Xinemediaplayer_Mode_youtube};
struct Xinemediaplayer_Set_Xine_Mode {
    Xinemediaplayer_Mode mode;
};

// Data structure for service "Xine Play VideoCD"
// No data expected

// Data structure for service "Xine Play AudioCD"
// No data expected

// Data structure for service "Xine Test mrl"
struct Xinemediaplayer_Xine_Test_mrl {
    char const *mrl;
    bool result;
};

// Data structure for service "Xine Play m3u"
struct Xinemediaplayer_Xine_Play_m3u {
    const char *m3u;
};

// Data structure for service "Xine Play mrl"
struct Xinemediaplayer_Xine_Play_mrl {
    char const *mrl;
    int instance; 
    bool playlist;
    std::vector<std::string> playlistEntries;
};

// Data structure for service "Xine Play mrl with name"
struct Xinemediaplayer_Xine_Play_mrl_with_name {
    char const *mrl;
    int instance; 
    bool playlist;
    std::vector<std::string> playlistEntries;
    std::vector<std::string> namelistEntries;
};

#endif /* XINEMEDIAPLAYER_SERVICES_INC */
