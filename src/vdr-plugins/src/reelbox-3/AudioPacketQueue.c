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

// AudioPacketQueue.c

#include "AudioPacketQueue.h"

namespace Reel
{
    AudioPacketQueue::AudioPacketQueue()
    :   playTimeRead_(0), playTimeWrite_(0),
        readPos_(0), writePos_(0),
        generation_(0),
        readInd_(0), writeInd_(0)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    AudioPacketQueue::~AudioPacketQueue()
    {
        // Delete contained packets.
        AudioPacket *p;
        while ((p = TryGet()))
        {
            delete p;
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    AudioPacket *AudioPacketQueue::Peek() const
    {
        AudioPacket *packet = 0;
        if (!Empty())
        {
            packet = audioPackets_[readInd_];
            MemoryBarrier(); // For the packet data.
        }
        return packet;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPacketQueue::Pop()
    {
        REEL_ASSERT(!Empty());

        AudioPacket *packet = audioPackets_[readInd_++];
        if (readInd_ == QueueSize)
        {
            readInd_ = 0;
        }
        MemoryBarrier(); // For the packet data.
        playTimeRead_ += AudioPacketPlayTime(*packet);
        ++ readPos_;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPacketQueue::PutEmptyPacket(AudioPacket *packet)
    {
        packet->SetNumFrames(0); // packet is empty.
        audioPackets_[writeInd_++] = packet;
        if (writeInd_ == QueueSize)
        {
            writeInd_ = 0;
        }
        MemoryBarrier(); // For the packet data.
        ++ writePos_;
    }

    //--------------------------------------------------------------------------------------------------------------

    void AudioPacketQueue::PutFilledPacket(AudioPacket *packet)
    {
        packet->SetGeneration(generation_); // Attach generation.
        audioPackets_[writeInd_++] = packet;
        if (writeInd_ == QueueSize)
        {
            writeInd_ = 0;
        }
        MemoryBarrier(); // For the packet data.
        playTimeWrite_ += AudioPacketPlayTime(*packet);
        ++ writePos_;
    }

    //--------------------------------------------------------------------------------------------------------------
}
