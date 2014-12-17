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

// VideoPlayerPipHd.h

#ifndef VIDEO_PLAYER_PIP_HD_H_INCLUDED
#define VIDEO_PLAYER_PIP_HD_H_INCLUDED

#include "VideoPlayerPip.h"

#define DEFAULT_X 272   // Gives nice effect on startup...
#define DEFAULT_Y 218
#define DEFAULT_WIDTH 176
#define DEFAULT_HEIGHT 140

namespace Reel
{
    class VideoPlayerPipHd : public VideoPlayerPip /* final */
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
        VideoPlayerPipHd();
        virtual ~VideoPlayerPipHd() NO_THROW;

        VideoPlayerPipHd(VideoPlayerPipHd const &); // Forbid copy construction.
        VideoPlayerPipHd &operator=(VideoPlayerPipHd const &); // Forbid copy assignment.

    private:
	class SWDecoder *decoder;
	UInt xpos,ypos,width,height;
	
    };

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPipHd::VideoPlayerPipHd() NO_THROW
    {
	    decoder=NULL;
	    xpos=DEFAULT_X;
	    ypos=DEFAULT_Y;
	    width=DEFAULT_WIDTH;
	    height=DEFAULT_HEIGHT;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline VideoPlayerPipHd::~VideoPlayerPipHd() NO_THROW
    {
        // Do nothing.
    }

    //--------------------------------------------------------------------------------------------------------------
}

#endif // def VIDEO_PLAYER_PIP_HD_H_INCLUDED
