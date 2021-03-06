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

// MenuBouquetsList.h

#ifndef P__MENU_EDITBOUQUET_H
#define P__MENU_EDITBOUQUET_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

class cMenuEditBouquet:public cOsdMenu
{
  private:
    cChannel * channel;
    cChannel *prevchannel;
    cChannel *&newchannel;
    cChannel data;
    char name[256];
    void Setup(void);
    int bouquetCaId;
    int number;
    int min_chan;
  public:
      cMenuEditBouquet(cChannel * Channel, bool New, cChannel * &newChannel);
    virtual eOSState ProcessKey(eKeys Key);
};

#endif // P__MENU_EDITBOUQUET
