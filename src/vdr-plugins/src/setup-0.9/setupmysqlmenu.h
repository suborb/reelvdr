/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * setupmysqlmenu.h
 *
 ***************************************************************************/

#ifndef SETUPMYSQLMENU_H
#define SETUPMYSQLMENU_H

#include <vdr/osdbase.h>

class cSetupMysqlMenu : public cOsdMenu
{
    private:
        bool LocalDBEnabled_;
        void Set();
        eOSState ProcessKey(eKeys Key);
        void ClearDatabase();
    public:
        cSetupMysqlMenu(const char *title);
        ~cSetupMysqlMenu();
};

#endif
