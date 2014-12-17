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

 
#ifndef P__USERIFBASE_H
#define P__USERIFBASE_H

#include <vdr/osdbase.h>

#include <vector>

//callers for user If
enum eIftype
{
    if_none,
    if_standard,
    if_restricted,
    if_playlistbrowser,
    if_playlistmenu,
    if_xine
};

class cUserIfBase
{
    public:
    virtual ~cUserIfBase(){};
    static void SetHistory(std::vector<eIftype> *ifTypes); 
    static eIftype GetCaller();
    static void EraseIfTypes();
    static void ChangeIfType(eIftype nextType);
    static void RemoveLastIf(); 
    static eIftype GetPreviousIf();
    static void PrintIfTypes();

    protected:
    cUserIfBase(){};
    //static std::vector<eIftype> ifTypes;
    static std::vector<eIftype> *ifTypes;
};

#endif //P__USERIFBASE_H
