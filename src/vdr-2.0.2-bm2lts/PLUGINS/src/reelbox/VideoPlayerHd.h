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

// VideoPlayerHd.h

#ifndef VIDEO_PLAYER_HD_H_INCLUDED
#define VIDEO_PLAYER_HD_H_INCLUDED

#include "VideoPlayer.h"

#include "HdCommChannel.h"

namespace Reel
{
    class VideoPlayerHd : public VideoPlayer /* final */
    {
    public:
        static void Create();
            ///< Create the single instance if it does not exist.

        /* override */ void Clear();

        /* override */ bool Flush();

        /* override */ void Freeze();

        /* override */ void Play();

        /* override */ void PlayPacket(Mpeg::EsPacket const &esPacket, bool still = false);

                       void PlayTsPacket(void *data, int length, unsigned char *PATPMT);

		       void PlayPesPacket(void *data, int length, int av);

        /* override */ bool Poll();

        //Start by Klaus
        /* override */ bool PollVideo()
        {
            //printf("---------VideoPlayerHd:PollVideo, video_delay = %d------------\n", hdPlayer_.video_delay);
            return (hdPlayer_.video_delay < 1000000);
        }
        //End by Klaus

        /* override */ void SetStc(bool stcValid, UInt stc);

        /* override */ void StillPicture(Mpeg::EsPacket const esPackets[], UInt packetCount);

        /* override */ void Start();

        /* override */ void Stop();

        /* override */ void Trickmode(UInt trickSpeed);
	   virtual int AproxFramesInQueue(void);

           void IncGen(void) { generation_++; }
    protected:
        VideoPlayerHd(); // Allow construction only through Create().
        virtual ~VideoPlayerHd() NO_THROW; // Allow destruction only through Destroy().

        VideoPlayerHd(VideoPlayerHd const &); // Forbid copy construction.
        VideoPlayerHd &operator=(VideoPlayerHd const &); // Forbid copy assignment.

    private:
        ::hd_player_t volatile &hdPlayer_; // Hd shared mem struct.
        UInt          generation_;
        bool volatile freeze_;
        Int pts_shift_;
        Int ac3_pts_shift_;

        template<typename T>
        void SendPacket(T const &header, void const *data, size_t dataSize);

        void SendPacket(void const *header, size_t headerSize, void const *data, size_t dataSize);
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerHd::~VideoPlayerHd() NO_THROW
    {
        printf ("\033[0;36m %s \033[0m\n", __PRETTY_FUNCTION__);
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void VideoPlayerHd::SetStc(bool stcValid, UInt stc)
    {
        hdPlayer_.stc_valid = stcValid;
        if (stcValid)
        {
            hdPlayer_.stc = stc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T>
    inline void VideoPlayerHd::SendPacket(T const &header, void const *data, size_t dataSize)
    {
        SendPacket(&header, sizeof(T), data, dataSize);
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_HD_H_INCLUDED
