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

// BspOsdProvider.c

#include "BspOsdProvider.h"

#include "BspOsd.h"
#ifdef REELSKIN
#include "BspTrueColorOsd.h"
#endif

namespace Reel
{
#if APIVERSNUM >= 10509 || defined(REELVDR)
    cOsd *BspOsdProvider::CreateOsd(int left, int top, uint level)
    {
        BspOsd *bspOsd = new BspOsd(left, top, level);
#else
    cOsd *BspOsdProvider::CreateOsd(int left, int top)
    {
        BspOsd *bspOsd = new BspOsd(left, top);
#endif
    
        if (scaleMode_)
        {
            bspOsd->ScaleToVideoPlane();
        }
        return bspOsd; 
    }
    
#ifdef REELSKIN
#if APIVERSNUM >= 10509 || defined(REELVDR)
    cOsd *BspOsdProvider::CreateTrueColorOsd(int left, int top, uint level)
    {
        return new BspTrueColorOsd(left, top, level);
    }
#else
    cOsd *BspOsdProvider::CreateTrueColorOsd(int left, int top)
    {
        return new BspTrueColorOsd(left, top);
    }
#endif
#endif
    
    bool BspOsdProvider::scaleMode_ = false;
}
