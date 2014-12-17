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

// BspOsdProvider.h
 
#ifndef BSP_OSD_PROVIDER_H_INCLUDED
#define BSP_OSD_PROVIDER_H_INCLUDED

#include <vdr/osd.h>

namespace Reel
{
    class BspOsdProvider : public cOsdProvider
    {
    public:
        static void Create();
        
        BspOsdProvider();
    
        static void SetOsdScaleMode(bool on);
    
    protected:
#if APIVERSNUM >= 10509 || defined(REELVDR)
        virtual cOsd *CreateOsd(int left, int top, uint level);
#else
        virtual cOsd *CreateOsd(int left, int top);
#endif
#ifdef REELSKIN
#if APIVERSNUM >= 10509 || defined(REELVDR)
        virtual cOsd *CreateTrueColorOsd(int left, int top, uint level);
#else
        virtual cOsd *CreateTrueColorOsd(int left, int top);
#endif
#endif

    private:
        // No assigning or copying
        BspOsdProvider(const BspOsdProvider &);
        BspOsdProvider &operator=(const BspOsdProvider &);
    
        static bool scaleMode_;
    };
    
    inline void BspOsdProvider::Create()
    {
        new BspOsdProvider();
    }
    
    inline BspOsdProvider::BspOsdProvider()
    {
    }
    
    inline void BspOsdProvider::SetOsdScaleMode(bool on)
    {
        scaleMode_ = on;
    }
}

#endif // BSP_OSD_PROVIDER_H_INCLUDED
