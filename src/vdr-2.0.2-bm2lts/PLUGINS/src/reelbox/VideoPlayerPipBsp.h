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

// VideoPlayerPipBsp.h

#ifndef VIDEO_PLAYER_PIP_BSP_H_INCLUDED
#define VIDEO_PLAYER_PIP_BSP_H_INCLUDED

#include "BspCommChannel.h"
#include "Mutex.h"
#include "VideoPlayerPip.h"

namespace Reel
{
    class VideoPlayerPipBsp : public VideoPlayerPip /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.

        /* override */ void Clear();

        /* override */ void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false);

        /* override */ void SetDimensions(UInt x, UInt y, UInt width, UInt height);

        /* override */ void Start();

        /* override */ void Stop();

    protected:
        VideoPlayerPipBsp();
        virtual ~VideoPlayerPipBsp() NO_THROW;

        VideoPlayerPipBsp(VideoPlayerPipBsp const &); // Forbid copy construction.
        VideoPlayerPipBsp &operator=(VideoPlayerPipBsp const &); // Forbid copy assignment.

    private:

        struct VideoPacketHeader
        {
            UInt flags_;
            UInt generation_;
            UInt streamPos_;
            UInt timestamp_;
        };

        void SendVideoPacket(VideoPacketHeader const &packetHeader, Byte const *data, UInt size, Int timeout);

        UInt                           generation_; // For implementing Clear().
        UInt volatile                  streamPos_; // The total numer of bytes played (mod 2^32).
        bsp_channel_t                 *bspChannel_;
        bsp_video_player_t volatile   &bspVideoPlayer_; // Bsp shared mem structs.
        Mutex                          mutex_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPipBsp::~VideoPlayerPipBsp() NO_THROW
    {
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_PIP_BSP_H_INCLUDED
