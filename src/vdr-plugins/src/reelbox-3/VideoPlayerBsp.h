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

// VideoPlayerBsp.h

#ifndef VIDEO_PLAYER_BSP_H_INCLUDED
#define VIDEO_PLAYER_BSP_H_INCLUDED

#include "BspCommChannel.h"
#include "Mutex.h"
#include "VideoPlayer.h"

namespace Reel
{
    class VideoPlayerBsp : public VideoPlayer /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.

        /* override */ void Clear();

        /* override */ bool Flush();

        /* override */ void Freeze();

        /* override */ void Play();

        /* override */ void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false);
    void PlayTsPacket(void *data, int length, unsigned char* PATPMT){};

        /* override */ bool Poll();

        /* override */ bool PollVideo(){return true;}

        /* override */ void SetStc(bool stcValid, UInt stc);

        /* override */ void StillPicture(Mpeg::EsPacket const esPackets[], UInt packetCount);

        /* override */ void Start();

        /* override */ void Stop();

        /* override */ void Trickmode(UInt trickSpeed);

		/* override */ int AproxFramesInQueue(void) { return 0;};
		protected:
        VideoPlayerBsp();
        virtual ~VideoPlayerBsp() NO_THROW;

        VideoPlayerBsp(VideoPlayerBsp const &); // Forbid copy construction.
        VideoPlayerBsp &operator=(VideoPlayerBsp const &); // Forbid copy assignment.

    private:

        struct VideoPacketHeader
        {
            UInt flags_;
            UInt generation_;
            UInt streamPos_;
            UInt timestamp_;
        };

        void SendVideoPacket(VideoPacketHeader const &packetHeader, Byte const *data, UInt size, Int timeout);

        bool volatile                  freeze_;
        UInt                           generation_; // For implementing Clear().
        UInt                           stillPictureId_; // Id of the displayed still picture.
        UInt volatile                  streamPos_; // The total numer of bytes played (mod 2^32).
        bsp_channel_t                 *bspChannel_;
        bsp_video_player_t volatile   &bspVideoPlayer_; // Bsp shared mem struct.
        Mutex                          mutex_;
        bool volatile                  pollPacketSent_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerBsp::~VideoPlayerBsp() NO_THROW
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void VideoPlayerBsp::SetStc(bool stcValid, UInt stc)
    {
        bspVideoPlayer_.stc_valid = stcValid;
        bspVideoPlayer_.stc = stc;
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_BSP_H_INCLUDED
