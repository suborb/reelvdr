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

// BspOsd.h
 
#ifndef P__BSP_OSD_H
#define P__BSP_OSD_H

#include "BspCommChannel.h"
#include "Reel.h"

#include <vdr/osd.h>
#include <bspchannel.h>

namespace Reel
{
    class BspOsd : public cOsd
    {
    public:
#if APIVERSNUM >= 10509 || defined(REELVDR)
        BspOsd(int Left, int Top, uint level);
#else
        BspOsd(int Left, int Top);
#endif
        ~BspOsd();
        
        virtual void Flush();
        void ScaleToVideoPlane();

        static BspOsd *GetInstance();

    private:
        // No assigning or copying
        BspOsd(const BspOsd &);
        BspOsd &operator=(const BspOsd &);
        
        void Clear(int x, int y, int x1, int y1);
        void FlushBitmap(cBitmap &bitmap, bool full);
        
        int lPos, tPos; // Osd screen coordinates (left, top)
            
        bsp_channel_t *bsc_osd; // Channel for OSD commands
        int           *framebuffer; // Pointer to OSD-framebuffer
        
        unsigned int min_x,min_y;
        unsigned int max_x,max_y;
        unsigned int last_palette[256];
        int last_numColors;
        int scaleToVideoPlane_;

        static BspOsd *instance_;
    };

    inline void BspOsd::ScaleToVideoPlane()
    {
        scaleToVideoPlane_ = 1;
    }

    inline BspOsd *BspOsd::GetInstance()
    {
        return instance_;
    }
}

#endif // P__BSP_OSD_H
