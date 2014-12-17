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

// VideoPlayerPip.h

#ifndef VIDEO_PLAYER_PIP_H_INCLUDED
#define VIDEO_PLAYER_PIP_H_INCLUDED

#include "MpegPes.h"
#include "Reel.h"

namespace Reel
{
    class VideoPlayerPip /* abstract */
    {
    public:
        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        static VideoPlayerPip &Instance() NO_THROW;
            ///< Return the single instance.

        // All public nonstatic member functions are threadsafe.

        virtual void Clear() = 0;
            ///< Discard all buffered data.

        virtual void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false) = 0;
            ///< Play the given es packet. Return as soon as possible if data can be buffered, otherwise block.

        virtual void SetDimensions(UInt x, UInt y, UInt width, UInt height) = 0;
            ///< Set position of the pip.

        virtual void Start() = 0;
            ///< Start playback.

        virtual void Stop() = 0;
            ///< Stop playback. (black screen)

    protected:
        static VideoPlayerPip *instance_;

        VideoPlayerPip();
        virtual ~VideoPlayerPip() NO_THROW;

        VideoPlayerPip(VideoPlayerPip const &); // Forbid copy construction.
        VideoPlayerPip &operator=(VideoPlayerPip const &); // Forbid copy assignment.
        int shutdown;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPip &VideoPlayerPip::Instance() NO_THROW
    {
        REEL_ASSERT(instance_);
        return *instance_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPip::VideoPlayerPip() NO_THROW
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPip::~VideoPlayerPip() NO_THROW
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_PIP_H_INCLUDED
