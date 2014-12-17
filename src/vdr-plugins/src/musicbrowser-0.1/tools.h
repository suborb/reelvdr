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
 
#ifndef P__TOOLS_H
#define P__TOOLS_H

#include <string>

inline bool IsEmptyString(std::string s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end())
    {
        if (!isspace(*it)) return false;
        ++it;
    }
    return true;
}

inline bool IsNonEmptyStringOfSpaces(std::string s)
{
    return !s.empty() && IsEmptyString(s); 
}


inline std::string MakeProgressBar(float percent, unsigned int len)
{
    if (percent < 0) return std::string();
    if (percent >= 100) percent = 100;

    if (len <= 2)
        return std::string();

    len -= 2; // two chars for [ and ]

    unsigned int p = (percent*len)/100;
    if (!p) return std::string("[") + std::string(len, ' ') + std::string("]");

    if (p >= len) return std::string("[") + std::string(len, '|') + std::string("]");

    return std::string("[") + std::string(p, '|')
            + std::string(len-p, ' ') + std::string("]");
}

#endif //P__TOOLS_H
