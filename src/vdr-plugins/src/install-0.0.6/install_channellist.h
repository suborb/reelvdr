/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 *                                                                         *
 ***************************************************************************
 *
 * install_channellist.h
 *
 ***************************************************************************/

#ifndef INSTALL_CHANNELLIST_H
#define INSTALL_CHANNELLIST_H

#include "installmenu.h"

#define HAS_DVB_INPUT(x) (x && (x->ProvidesSource(cSource::stSat) || x->ProvidesSource(cSource::stCable) || x->ProvidesSource(cSource::stTerr)))

class cInstallChannelList:public cInstallSubMenu
{
  private:
    void Set();
    bool Save();
  public:
      cInstallChannelList(bool showMsg);
    eOSState ProcessKey(eKeys Key);
};

#endif
