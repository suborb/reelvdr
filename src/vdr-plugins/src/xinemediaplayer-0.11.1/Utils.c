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

// Utils.c

#include "Utils.h"

#include <vdr/tools.h>

#include <string>

#include <stdlib.h>
#include <time.h>

namespace Reel
{
namespace XineMediaplayer
{
    std::string FormatString(char const *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        std::string ret = FormatStringV(fmt, ap);
        va_end(ap);
        return ret;
    }

    std::string FormatStringV(char const *fmt, va_list ap)
    {
        std::string ret;
        char *cstr = NULL;
        if (::vasprintf(&cstr, fmt, ap) != -1)
        {
            ret = cstr;
            ::free(cstr);
        }
        return ret;
    }

    // Generate pseudo random number
    // between 0...MaxNum, if possible avoiding "avoidNum"
    unsigned int RandomNumberLessThan( unsigned int MaxNum, unsigned int avoidNum )
    {
        // try 20 times for a new number
        unsigned int result=avoidNum;
        for ( int tries = 0; tries < 20; ++tries)
        {
            result = ( unsigned int) (MaxNum *( rand() *1.0/(RAND_MAX + 1.0) ));
            if ( result != avoidNum ) return result;
        }

        // avoidNum is returned
        return result;
    }

}
}
