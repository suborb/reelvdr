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

// MenuSetupReel.h
 
#ifndef P__MENU_SETUP_REEL_H
#define P__MENU_SETUP_REEL_H

#include <vdr/device.h>
#include <vdr/menuitems.h>

class cMenuSetupReel : public cMenuSetupPage
{
public:
    cMenuSetupReel();

protected:
    virtual void Store();

private:
    cMenuSetupReel(const cMenuSetupReel &);
    cMenuSetupReel &operator=(const cMenuSetupReel &);

    int newTestValue;
};

extern int testValue;

#endif // P__MENU_SETUP_REEL_H
