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

// AudioPacketQueue.h

#ifndef AUDIO_PACKET_QUEUE_H_INCLUDED
#define AUDIO_PACKET_QUEUE_H_INCLUDED

#include "AudioPacket.h"
#include "CondVar.h"
#include "Utils.h"

#include <unistd.h>

namespace Reel
{
    UInt const NumberOfAudioPackets = 256; // FIXME

    ///< Lockfree queue of (pointers to) audio packets. Supports one concurrent reader and one concurrent writer.
    ///< The contained packets are owned by the queue and are deleted when the queue is destroyed. The packets must have
    ///< been created with new. The queue maintains a generation value.
    class AudioPacketQueue /* final */
    {
    public:
        AudioPacketQueue();
        ~AudioPacketQueue();

        bool Empty() const;
            ///< Return true iff queue is empty.
            ///< Threadsafe.

        UInt GetGeneration() const;
            ///< Return generation.

        UInt GetNumElements() const;
            ///< Return the number of elements in the queue. The returned value may be too high if concurrent reads
            ///< occur.
            ///< Threadsafe.

        UInt GetPlayTime() const;
            ///< Return the approximate total play time of all queued filled packets in ticks. The returned value may
            ///< be too high if concurrent reads occur.
            ///< Threadsafe.

        void IncrementGeneration();
            ///< Increment the generation. All subsequently queued filled audio packets will be tagged with the new
            ///< generation value.

        AudioPacket *Peek() const;
            ///< Return a pointer to the next element in the queue without removing it. Return 0 if queue is empty.

        void Pop();
            ///< Remove the next element from the queue. The queue must be nonempty.

        void PutEmptyPacket(AudioPacket *packet);
            ///< Queue a empty audio packet. Play time will not be accounted.
            ///< One writer at a time!

        void PutFilledPacket(AudioPacket *packet);
            ///< Queue a filled audio packet. Play time will be accounted.
            ///< One writer at a time!

        AudioPacket *TryGet();
            ///< Get next queued packet. Return 0 if not available.
            ///< One reader at a time!

    private:
        AudioPacketQueue(AudioPacketQueue const &); // Forbid copy construction.
        AudioPacketQueue &operator=(AudioPacketQueue const &); // Forbid copy assignment.

        enum
        {
            QueueSize = NumberOfAudioPackets + 1
        };

        AudioPacket *volatile audioPackets_[QueueSize];

        UInt volatile  playTimeRead_, playTimeWrite_;
        UInt volatile  readPos_, writePos_;
        UInt volatile  generation_;

        UInt           readInd_, writeInd_;
    };

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPacketQueue::Empty() const
    {
        return readPos_ == writePos_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacketQueue::GetGeneration() const
    {
        return generation_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacketQueue::GetNumElements() const
    {
        // Fetch the read position first, to avoid "negative" results.
        UInt read = readPos_;
        // Sequence point between volatile read accesses, ensures correct ordering.
        return writePos_ - read;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacketQueue::GetPlayTime() const
    {
        // Fetch the read play time first, to avoid "negative" results.
        UInt read = playTimeRead_;
        // Sequence point between volatile read accesses, ensures correct ordering.
        return playTimeWrite_ - read;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacketQueue::IncrementGeneration()
    {
        ++ generation_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket *AudioPacketQueue::TryGet()
    {
        AudioPacket *packet = Peek();
        if (packet)
        {
            Pop();
        }
        return packet;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket *GetPacketFromQueue(AudioPacketQueue &queue)
    {
        AudioPacket *packet;
        while (!(packet = queue.Peek()))
        {
            usleep(5000);
        }
        queue.Pop();
        return packet;
    }
}

#endif // AUDIO_PACKET_QUEUE_H_INCLUDED
