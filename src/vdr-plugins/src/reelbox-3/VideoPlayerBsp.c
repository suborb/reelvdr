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

// VideoPlayerBsp.c
 
#include "VideoPlayerBsp.h"

#include <climits>
#include <cstring>

namespace Reel
{
    namespace
    {
        UInt const PACKET_FLAG_HAS_TIMESTAMP = 0x00000001;
        UInt const PACKET_FLAG_STILL_PICTURE = 0x00000002;
        UInt const PACKET_FLAG_STOP          = 0x00000004;
        UInt const PACKET_FLAG_WAKE          = 0x00000008;
    }

    //--------------------------------------------------------------------------------------------------------------

    VideoPlayerBsp::VideoPlayerBsp()
    :   generation_(1),
        stillPictureId_(0),
        streamPos_(0),
        bspChannel_(Bsp::BspCommChannel::Instance().bsc_pes1),
        bspVideoPlayer_(Bsp::BspCommChannel::Instance().bspd->video_player[0])
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Create()
    {
        if (!instance_)
        {
            instance_ = new VideoPlayerBsp;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Clear()
    {
        MutexLocker lock(mutex_);

        // We simply increase the generation and the bsp will discard all out of date packets.
        ++ generation_;

        // Send a wake packet
        VideoPacketHeader header = {PACKET_FLAG_WAKE, generation_, streamPos_, 0};
        SendVideoPacket(header, 0, 0, 2000);
    }

    //--------------------------------------------------------------------------------------------------------------

    bool VideoPlayerBsp::Flush()
    {
        return bspVideoPlayer_.idle;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Freeze()
    {
        bspVideoPlayer_.trickspeed = 0; // All currently buffered trickframes will be displayed with normal speed.
        freeze_                    = true;
        bspVideoPlayer_.av_sync    = 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Play()
    {
        bspVideoPlayer_.trickspeed = 0; // All currently buffered trickframes will be displayed with normal speed.
        freeze_                    = false;
        bspVideoPlayer_.av_sync    = 100;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::PlayPacket(Mpeg::EsPacket const &esPacket, bool still)
    {
        MutexLocker lock(mutex_);

        streamPos_ += esPacket.GetDataLength();
        UInt const flags = esPacket.HasPts() ? PACKET_FLAG_HAS_TIMESTAMP : 0;
        VideoPacketHeader header = {flags, generation_, streamPos_, esPacket.GetPts()};
        SendVideoPacket(header, esPacket.GetData(), esPacket.GetDataLength(), 2000);
    }

    //--------------------------------------------------------------------------------------------------------------

    bool VideoPlayerBsp::Poll()
    {
	//Int bytesNeeded = bspVideoPlayer_.stream_pos;// - streamPos_; (yellow and green button bug)
	//return !freeze_ && bytesNeeded;
	UInt bytesNeeded = bspVideoPlayer_.stream_pos;
	return bytesNeeded;
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::SendVideoPacket(VideoPacketHeader const &packetHeader, Byte const *data, UInt size, Int timeout)
    {
        // Set shared generation variable first.
        bspVideoPlayer_.stream_generation = packetHeader.generation_;

        UInt const fullDataLength = sizeof(VideoPacketHeader) + size;

        // Request buffer.
        void     *buffer;

        static UInt sentData = 0;

        int bufferSize = bsp_channel_write_start(bspChannel_, &buffer, fullDataLength, 10);
        if (!bufferSize)
        {
            for (int n = 0; n < 2000000 && !bufferSize; n += 5000)
            {
                ::usleep(5000);
                bufferSize = bsp_channel_write_start(bspChannel_, &buffer, fullDataLength, 10);
            }
        }

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

    void VideoPlayerBsp::StillPicture(Mpeg::EsPacket const esPackets[], UInt packetCount)
    {
        MutexLocker lock(mutex_);

        // Send only if the player has not yet displayed the picture.
        ++ stillPictureId_;

        int count = 0;
        while (stillPictureId_ != implicit_cast<UInt>(bspVideoPlayer_.still_picture_id))
        {
            if (count >= 2)
            {
                ::usleep(30000); // sleep 30 ms.
            }            
            if (stillPictureId_ == implicit_cast<UInt>(bspVideoPlayer_.still_picture_id))
            {
                break;
            }

            for (UInt n = 0; n < packetCount; ++n)
            {
                Mpeg::EsPacket const &esPacket = esPackets[n];
                VideoPacketHeader header = {PACKET_FLAG_STILL_PICTURE, generation_,
                                            streamPos_, stillPictureId_};
                SendVideoPacket(header, esPacket.GetData(), esPacket.GetDataLength(), 2000);
            }

            // Send a wake packet
            VideoPacketHeader header = {PACKET_FLAG_WAKE, generation_, streamPos_, 0};
            SendVideoPacket(header, 0, 0, 2000);

            ++ count;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Start()
    {
        bspVideoPlayer_.use_xywh = 0;
        bspVideoPlayer_.changed++;
        Bsp::BspCommChannel::Instance().bspd->need_focus_sync=3; // Focus sync after 3 sec.

        Play(); // Turn off trickmode /freeze.
        // Clear(); No Clear() here !!!
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Stop()
    {
        Play(); // Turn off trickmode /freeze.
        Clear();

        MutexLocker lock(mutex_);

        // Send stop packet to blank the screen.
        VideoPacketHeader header = {PACKET_FLAG_STOP, generation_, streamPos_, 0};
        SendVideoPacket(header, 0, 0, 2000);
    }

    //--------------------------------------------------------------------------------------------------------------

    void VideoPlayerBsp::Trickmode(UInt trickSpeed)
    {
        bspVideoPlayer_.trickspeed = trickSpeed;
        freeze_                    = false;
    }
}
