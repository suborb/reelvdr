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
 * menueditrefreshitems.cpp
 *
 ***************************************************************************/

#include "menueditrefreshitems.h"

// class cMenuEditIntRefreshItem --------------------------------------
cMenuEditIntRefreshItem::cMenuEditIntRefreshItem(const char *Name, int *Value, int Min, int Max, const char *MinString, const char *MaxString) : cMenuEditIntItem(Name, Value, Min, Max, MinString, MaxString)
{
    value = Value;
    min = Min;
    max = Max;
    minString = MinString;
    maxString = MaxString;
    if(*value < min)
        *value = min;
    else if(*value > max)
        *value = max;
    Set();
}

eOSState cMenuEditIntRefreshItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    if(state == osUnknown)
    {
        int newValue = *value;
        bool IsRepeat = Key & k_Repeat;
        Key = NORMALKEY(Key);
        switch(Key)
        {
            case kNone:
                break;
            case k0 ... k9:
                if(fresh)
                    {
                        newValue = 0;
                        fresh = false;
                    }
                newValue = newValue * 10 + (Key - k0);
                break;
            case kLeft: // TODO might want to increase the delta if repeated quickly?
                newValue = *value - 1;
                fresh = true;
                if(!IsRepeat && newValue < min && max != INT_MAX)
                    newValue = max;
                break;
            case kRight:
                newValue = *value + 1;
                fresh = true;
                if(!IsRepeat && newValue > max && min != INT_MIN)
                    newValue = min;
                break;
            default:
                if(*value < min) { *value = min; Set(); }
                if(*value > max) { *value = max; Set(); }
                return state;
        }
        if(newValue != *value && (!fresh || min <= newValue) && newValue <= max)
        {
            *value = newValue;
            Set();
            return os_User; // This custom return value tells osd to redraw menu
        }
        state = osContinue;
    }
    return state;
}

// class cMenuEditStraRefreshItem -------------------------------------
cMenuEditStraRefreshItem::cMenuEditStraRefreshItem(const char *Name, int *Value, int NumStrings, const char *const *Strings)
    : cMenuEditIntRefreshItem(Name, Value, 0, NumStrings - 1)
{
    strings = Strings;
    Set();
}

void cMenuEditStraRefreshItem::Set(void)
{
    SetValue(tr(strings[*value]));
}

// class cMenuEditBoolRefreshItem -------------------------------------
cMenuEditBoolRefreshItem::cMenuEditBoolRefreshItem(const char *Name, int *Value, const char *FalseString, const char *TrueString) : cMenuEditIntRefreshItem(Name, Value, 0, 1)
{
    falseString = FalseString ? FalseString : tr("no");
    trueString = TrueString ? TrueString : tr("yes");
    Set();
}

void cMenuEditBoolRefreshItem::Set(void)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%s", *value ? trueString : falseString);
    SetValue(buf);
}


