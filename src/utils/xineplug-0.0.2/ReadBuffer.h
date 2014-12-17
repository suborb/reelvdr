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

/* ReadBuffer.h */

#ifndef XINEPLUG_REELBOX_READ_BUFFER_H_INCLUDED
#define XINEPLUG_REELBOX_READ_BUFFER_H_INCLUDED

#include <vector>

#define nullptr 0
    // Note: NULL pointer constant bug in gcc (3.3.6 and earlier)!

template<typename T1, typename T2>
inline T1 implicit_cast(T2 t2)
{
    return t2;
}

typedef unsigned char Byte;

class ReadBuffer /* final */
{
public:
    ReadBuffer();

    void        Advance(int length);
    void        Clear();
    Byte const *Read(int min, /*out */ int &length);
    void        Set(Byte const *data, int length);
    void        Unset();

private:
    void  AppendToBuffer(int length);

    std::vector<Byte> buffer_;

    Byte const *readRange_;
    int         readRangeLength_, readRangeOrigLength_;
    int         bufferPos_;
};


#endif /* XINEPLUG_REELBOX_READ_BUFFER_H_INCLUDED */
