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

// MenuBouquets.h

#ifndef P__MENU_BOUQUETS_H
#define P__MENU_BOUQUETS_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

#include "MenuChannelItem.h"
#include "def.h"
#include "bouquets.h"

class cMenuMyBouquets:public cOsdMenu
{
  private:
    enum eEditMode editMode;
    int view_;
    bool edit;
    bool favourite;
    bool move;
    bool selectionChanched;
    int startChannel;
    int number;
    std::vector < int >channelMarked;
    cTimeMs numberTimer;
    void Setup(void);
    void SetChannels(void);
    void SetGroup(int Index);
    cChannel *GetChannel(int Index);
    void Propagate(void);
    void GetFavourite(void);
    eOSState PrevBouquet(void);
    eOSState NextBouquet(void);
    char titleBuf[128];
  protected:
    void Options(void);
    eOSState Switch(void);
    eOSState NewChannel(void);
    eOSState EditChannel(void);
    eOSState DeleteChannel(void);
    eOSState ListBouquets(void);
    eOSState Number(eKeys Key);
    void Mark( /*cMenuChannelItem *item = NULL */ );
    virtual void Move(int From, int To);
    void Move(int From, int To, bool doSwitch);
    eOSState MoveMultiple(void);
    eOSState MoveMultipleOrCurrent();
    void UnMark(cMenuMyChannelItem * p);
  public:
    cMenuMyBouquets(enum eChannellistMode view, eEditMode editMode = mode_view);
    ~cMenuMyBouquets(void);
    void CreateTitle(cChannel * channel);
    void AddFavourite(bool active);
    virtual eOSState ProcessKey(eKeys Key);
    virtual void Display(void);
    void ShowEventInfoInSideNote();
};


#endif // P__MENU_BOUQUETS
