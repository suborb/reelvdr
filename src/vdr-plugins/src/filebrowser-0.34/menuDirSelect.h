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

 
#ifndef P__MENUDIRSELECT_H
#define P__MENUDIRSELECT_H
 
#include <string>

#include <vdr/osdbase.h>

class cFileBrowserStatus;
class cFileCache;
class cImageConvert;
class cThumbConvert;

//--------------cMenuDirSelect-----------------------

class cMenuDirSelect : public cOsdMenu
{
public:
    cMenuDirSelect(const std::string title, cFileBrowserStatus *newStatus, cFileCache *cache, cImageConvert *convert, cThumbConvert *thumbConvert);   
    virtual ~cMenuDirSelect(){};
    eOSState EnterDir();  

protected:
    /*override*/ eOSState ProcessKey(eKeys Key);

private:
    void Set();

    cFileBrowserStatus *newStatus_;
    cFileCache *cache_;
    cImageConvert *convert_;
    cThumbConvert *thumbConvert_;
};

#endif //#define P__MENUDIRSELECT_H
