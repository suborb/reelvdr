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

#ifndef P__MENU_BOUQUETSLIST_H
#define P__MENU_BOUQUETSLIST_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

extern bool calledFromkChan;

class cMenuBouquetsList:public cOsdMenu
{
  private:
    int bouquetMarked;
    void Setup(cChannel * channel);
    void Propagate(void);
    bool FilteredBouquetIsEmpty(cChannel * startChannel);
    cChannel *GetBouquet(int Index);
    eOSState DeleteBouquet(void);
    void Options();
    //bool showHelp;
    int mode_;
    bool HadSubMenu_;
    cChannel *newChannel_;
  protected:
      eOSState Switch(void);
    eOSState ViewChannels(void);
    eOSState EditBouquet(void);
    eOSState NewBouquet(void);
    void Mark(void);
    virtual void Move(int From, int To);
  public:
      cMenuBouquetsList(cChannel * channel = NULL, int mode = 1);
     ~cMenuBouquetsList()
    {
    }
    virtual eOSState ProcessKey(eKeys Key);
    virtual void Display(void);
};

#endif // P__MENU_BOUQUETSLIST
