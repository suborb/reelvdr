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

// ReelSkin.h

#ifndef REEL_SKIN_H_INCLUDED
#define REEL_SKIN_H_INCLUDED

#include "Reel.h"

#include <vdr/skins.h>

namespace Reel
{

    void ResetImagePaths();

    class ReelSkin : public cSkin
    {
    public:

        ReelSkin();
        /* override */ char const *Description();
        /* override */ cSkinDisplayChannel *DisplayChannel(bool withInfo);
        /* override */ cSkinDisplayMenu *DisplayMenu();
        /* override */ cSkinDisplayReplay *DisplayReplay(bool modeOnly);
        /* override */ cSkinDisplayVolume *DisplayVolume();
        /* override */ cSkinDisplayTracks *DisplayTracks(char const *title, int numTracks, char const *const *tracks);
        /* override */ cSkinDisplayMessage *DisplayMessage();

    private:

        ReelSkin(ReelSkin const &); // Forbid copy construction.
        ReelSkin &operator=(ReelSkin const &); // Forbid copy assignment.
    };
}

#endif // REEL_SKIN_H_INCLUDED
