/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * menueditrefreshitems.h
 *
 ***************************************************************************/

#ifndef MENUEDITREFRESHITEMS_H
#define MENUEDITREFRESHITEMS_H

#include <vdr/menuitems.h>

class cMenuEditIntRefreshItem : public cMenuEditIntItem
{
    public:
        cMenuEditIntRefreshItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *MinString = NULL, const char *MaxString = NULL);
        eOSState ProcessKey(eKeys Key);
};

class cMenuEditStraRefreshItem : public cMenuEditIntRefreshItem
{
    private:
        const char * const *strings;
    protected:
        virtual void Set(void);
    public:
        cMenuEditStraRefreshItem(const char *Name, int *Value, int NumStrings, const char * const *Strings);
};

class cMenuEditBoolRefreshItem : public cMenuEditIntRefreshItem
{
    protected:
        const char *falseString, *trueString;
        virtual void Set(void);
    public:
        cMenuEditBoolRefreshItem(const char *Name, int *Value, const char *FalseString = NULL, const char *TrueString = NULL);
};

#endif

