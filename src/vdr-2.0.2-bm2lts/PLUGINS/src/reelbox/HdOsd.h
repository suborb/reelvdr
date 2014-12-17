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

// HdOsd.h
 
#ifndef P__HD_OSD_H
#define P__HD_OSD_H

#include "HdCommChannel.h"
#include "Reel.h"

#include <vdr/osd.h>
#include <hdshm_user_structs.h>

namespace Reel
{
    class HdOsd : public cOsd
    {
    public:
#if APIVERSNUM >= 10509 || defined(REELVDR)
        HdOsd(int Left, int Top, uint level);
        void SetActive(bool On);
        virtual eOsdError SetAreas(const tArea *Areas, int NumAreas); // 1.6
#else
        HdOsd(int Left, int Top);
#endif
        ~HdOsd();
        
        virtual void Flush();
        void ScaleToVideoPlane();

        static HdOsd *GetInstance();

    private:
        // No assigning or copying
        HdOsd(const HdOsd &);
        HdOsd &operator=(const HdOsd &);
        
        void Clear(int x, int y, int x1, int y1);
        void FlushBitmap(cBitmap &bitmap, bool full);

        void SendOsdCmd(void const *bco, UInt bcoSize, void const *payload = 0, UInt payloadSize = 0);
        template<typename T>
        void SendOsdCmd(T const &bco, void const *payload = 0, UInt payloadSize = 0);
 
        int lPos, tPos; // Osd screen coordinates (left, top)
            
        HdCommChannel::Channel bsc_osd; // Channel for OSD commands
        int           *framebuffer; // Pointer to OSD-framebuffer
        
        unsigned int min_x,min_y;
        unsigned int max_x,max_y;
        unsigned int last_palette[256];
        int last_numColors;
        int scaleToVideoPlane_;

        static HdOsd *instance_;
    };

    template<typename T>
    inline void HdOsd::SendOsdCmd(T const &bco, void const *payload, UInt payloadSize)
    {
        return SendOsdCmd(&bco, sizeof(T), payload, payloadSize);
    }

    inline void HdOsd::ScaleToVideoPlane()
    {
        scaleToVideoPlane_ = 1;
    }

    inline HdOsd *HdOsd::GetInstance()
    {
        return instance_;
    }
}

#endif // P__HD_OSD_H
