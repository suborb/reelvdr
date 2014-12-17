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

// VideoPlayer.h

#ifndef VIDEO_PLAYER_H_INCLUDED
#define VIDEO_PLAYER_H_INCLUDED

#include "AVSyncListener.h"
#include "MpegPes.h"
#include "Reel.h"

namespace Reel
{
    class VideoPlayer : public AVSyncListener /* abstract */
    {
    public:
        static VideoPlayer &Instance() NO_THROW;
            ///< Return the single instance.

        static void Destroy() NO_THROW;
            ///< Destroy the single instance if it does exist.

        // All public nonstatic member functions are threadsafe.

        virtual void Clear() = 0;
            ///< Discard all buffered data.

        virtual bool Flush() = 0;
            ///< Return true if all data has been processed and displayed. Return immediately without blocking, even
            ///< if other member functions are currently executing.

        virtual void Freeze() = 0;
            ///< Freeze playback.

        virtual void Play() = 0;
            ///< Turn off trickmode (if active) and prepare for standard AV-synchronized playback.

        virtual void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false) = 0;
            ///< Play the given es packet. Return as soon as possible if data can be buffered, otherwise block.

        virtual void PlayTsPacket(void *packet, int length, unsigned char *PATPMT) = 0;

	virtual void PlayPesPacket(void *data, int length, int av) {};

        virtual bool Poll() = 0;
            ///< Return true if further data is needed. Return immediately without blocking, even if other member
            ///< functions are currently executing.

        virtual bool PollVideo() = 0;

        virtual void SetStc(bool valid, UInt stc) = 0;

        virtual void StillPicture(Mpeg::EsPacket const esPackets[], UInt packetCount) = 0;
            ///< Display a sequence of es packets as a still picture.

        virtual void Start() = 0;
            ///< Start playback.

        virtual void Stop() = 0;
            ///< Stop playback. (black screen)

        virtual void Trickmode(UInt trickSpeed) = 0;
            ///< Turn on trickmode. Display all following frames trickSpeed times. trickSpeed >= 1.

		virtual int AproxFramesInQueue(void) = 0;
	protected:
        static VideoPlayer *instance_;

        VideoPlayer();
        virtual ~VideoPlayer() NO_THROW;

        VideoPlayer(VideoPlayer const &); // Forbid copy construction.
        VideoPlayer &operator=(VideoPlayer const &); // Forbid copy assignment.
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayer &VideoPlayer::Instance() NO_THROW
    {
        REEL_ASSERT(instance_);
        return *instance_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayer::VideoPlayer()
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayer::~VideoPlayer() NO_THROW
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_H_INCLUDED
