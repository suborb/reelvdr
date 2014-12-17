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

#ifndef P__MENU_BOUQUETITEM_H
#define P__MENU_BOUQUETITEM_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

class cMenuBouquetItem:public cOsdItem
{
  private:
    cChannel * bouquet;
    cSchedulesLock schedulesLock;
    const cSchedules *schedules;
    const cEvent *event;
    char szProgressPart[12];
  public:
      cMenuBouquetItem(cChannel * Channel);
    cChannel *Bouquet()
    {
        return bouquet;
    };
    void Set(void);
};

#endif // P__MENU_BOUQUETITEM
