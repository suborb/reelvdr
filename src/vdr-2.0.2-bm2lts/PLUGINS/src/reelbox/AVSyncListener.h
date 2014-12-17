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

// AVSyncListener.h

#ifndef A_V_SYNC_LISTENER_H_INCLUDED
#define A_V_SYNC_LISTENER_H_INCLUDED

#include "Reel.h"

namespace Reel
{
    class AVSyncListener /* interface */
    {
    public:

        // Note: The callback functions in this interface will be called from within the audio playback realtime thread.
        //       In order to avoid priority inversion, they must never call any potentially blocking operations (like
        //       lock mutexes, wait on conditions, block on io, sleep etc.)!

        virtual ~AVSyncListener();

        virtual void SetStc(bool stcValid, UInt stc) = 0;
            ///< Inform listener about the stc.
            ///< stcValid = true: Set the stc.
            ///< stcValid = false: Invalidate stc.
    };

    inline AVSyncListener::~AVSyncListener()
    {
        // Do nothing.
    }
}

#endif // def A_V_SYNC_LISTENER_H_INCLUDED
