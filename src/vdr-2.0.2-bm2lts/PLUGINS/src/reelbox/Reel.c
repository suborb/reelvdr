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

// Reel.c

#include "Reel.h"

#include <iostream>

namespace Reel
{
    Int DebugAssertionFailed(PCSTR file, Int line)
    {
        std::cout << "!!! Debug assertion failed !!!! " << file << '(' << line << ')' << std::endl;
        return 0;
    }

    void LogException(std::exception const &e) NO_THROW
    {
        ReelException const *re = dynamic_cast<ReelException const *>(&e);

        if (re)
        {
            std::cout << "std::exception : " << re->what() << '(' << re->GetLine() << ')' << std::endl;
        }
        else
        {
            std::cout << "std::exception : " << e.what() << std::endl;
        }
    }

    void Restart() NO_THROW
    {
        std::terminate();
    }
}
