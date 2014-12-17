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

// HdOsdProvider.h
 
#ifndef HD_OSD_PROVIDER_H_INCLUDED
#define HD_OSD_PROVIDER_H_INCLUDED

#include <vdr/osd.h>

namespace Reel
{
    class HdOsdProvider : public cOsdProvider
    {
    public:
        static void Create();
        
        HdOsdProvider();
        ~HdOsdProvider();
    
        static void SetOsdScaleMode(bool on);
    
    protected:
#if APIVERSNUM >= 10509 || defined(REELVDR)
        virtual cOsd *CreateOsd(int left, int top, uint level);
        virtual cOsd *CreateTrueColorOsd(int left, int top, uint level);
#else
        virtual cOsd *CreateOsd(int left, int top);
        virtual cOsd *CreateTrueColorOsd(int left, int top);
#endif

    private:
        // No assigning or copying
        HdOsdProvider(const HdOsdProvider &);
        HdOsdProvider &operator=(const HdOsdProvider &);
    
        static bool scaleMode_;
    };
    
    inline void HdOsdProvider::Create()
    {
        new HdOsdProvider();
    }
    
    inline HdOsdProvider::HdOsdProvider()
    {
    }
    
    inline void HdOsdProvider::SetOsdScaleMode(bool on)
    {
        scaleMode_ = on;
    }
}

#endif // HD_OSD_PROVIDER_H_INCLUDED
