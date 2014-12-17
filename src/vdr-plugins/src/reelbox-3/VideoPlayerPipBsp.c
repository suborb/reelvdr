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

// VideoPlayerPipBsp.c

#include "VideoPlayerPipBsp.h"

#include <climits>
#include <cstring>

namespace Reel
{
    namespace
    {
        UInt const PACKET_FLAG_STOP          = 0x00000004;
        UInt const PACKET_FLAG_WAKE          = 0x00000008;
    }

    //--------------------------------------------------------------------------------------------------------------

    VideoPlayerPipBsp::VideoPlayerPipBsp()
    :   generation_(1),
        streamPos_(0),
        bspChannel_(Bsp::BspCommChannel::Instance().bsc_pes2),
        bspVideoPlayer_(Bsp::BspCommChannel::Instance().bspd->video_player[1])
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::Create()
    {
        if (!instance_)
        {
            instance_ = new VideoPlayerPipBsp;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::Clear()
    {
        MutexLocker lock(mutex_);

        // We simply increase the generation and the bsp will discard all out of date packets.
        ++ generation_;

        // Send a wake packet
        VideoPacketHeader header = {PACKET_FLAG_WAKE, generation_, streamPos_, 0};
        SendVideoPacket(header, 0, 0, 2000);
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::PlayPacket(Mpeg::EsPacket const &esPacket, bool still)
    {
        MutexLocker lock(mutex_);

        streamPos_ += esPacket.GetDataLength();
        VideoPacketHeader header = {0, generation_, streamPos_, 0};
        SendVideoPacket(header, esPacket.GetData(), esPacket.GetDataLength(), 0);
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::SendVideoPacket(VideoPacketHeader const &packetHeader, Byte const *data, UInt size, Int timeout)
    {
        // Set shared generation variable first.
        bspVideoPlayer_.stream_generation = packetHeader.generation_;

        UInt const fullDataLength = sizeof(VideoPacketHeader) + size;

        // Request buffer.
        void     *buffer;

        static UInt sentData = 0;

        int bufferSize = bsp_channel_write_start(bspChannel_, &buffer, fullDataLength, 0);

        if (bufferSize)
        {
            // Here: bufferSize >= fullDataLength.

            sentData += fullDataLength;

            // Write packet header.
            VideoPacketHeader *header = static_cast<VideoPacketHeader *>(buffer);
            *header = packetHeader;
            buffer = header + 1;

            // Write payload data.
            std::memcpy(buffer,
                        data,
                        size);

            // Send packet.
            bsp_channel_write_finish(bspChannel_, fullDataLength);
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::SetDimensions(UInt x, UInt y, UInt width, UInt height)
    {
        bspVideoPlayer_.x = x;
        bspVideoPlayer_.y = y;
        bspVideoPlayer_.w = width;
        bspVideoPlayer_.h = height;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::Start()
    {
        bspVideoPlayer_.use_xywh = 0;
        bspVideoPlayer_.changed++;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerPipBsp::Stop()
    {
        Clear();

        MutexLocker lock(mutex_);

        // Send stop packet to blank the screen.
        VideoPacketHeader header = {PACKET_FLAG_STOP, generation_, streamPos_, 0};
        SendVideoPacket(header, 0, 0, 2000);
    }
}
