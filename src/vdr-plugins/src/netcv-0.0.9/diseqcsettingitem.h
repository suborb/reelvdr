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
 * diseqcsettingtitem.h
 *
 ***************************************************************************/

#ifndef DISEQCSETTINGITEM_H
#define DISEQCSETTINGITEM_H

#include <vector>
#include <string>

#include <vdr/menuitems.h>

#include "diseqcsetting.h"

class cMenuEditIntRefreshItem : public cMenuEditItem
{
    protected:
        int *value;
        int min, max;
        const char *minString, *maxString;
        virtual void Set(void);

    public:
        cMenuEditIntRefreshItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *MinString = NULL, const char *MaxString = NULL);
        virtual eOSState ProcessKey(eKeys Key);
};

class cMenuEditBoolRefreshItem : public cMenuEditIntItem
{
    protected:
        const char *falseString, *trueString;

        virtual void Set(void);

    public:
        cMenuEditBoolRefreshItem(const char *Name, int *Value, const char *FalseString = NULL, const char *TrueString = NULL);
        eOSState ProcessKey(eKeys Key);
};

class cMenuEditStraRefreshItem : public cMenuEditIntRefreshItem {
    private:
        const char* const *strings;
    protected:
        virtual void Set(void);
    public:
        cMenuEditStraRefreshItem(const char *Name, int *Value, int NumStrings, const char* const *Strings);
};

class cSatlistItem : public cMenuEditItem
{
    private:
        bool *bShow_;
        std::vector<cSatList*>::iterator satlistIter_;
        cDiseqcSetting *DiseqcSetting_;

        char *orgValue;
        char *value;
        int length;
        char *allowed;
        int pos;
        bool insert, newchar, uppercase;
        const char *charMap;
        const char *currentChar;
        eKeys lastKey;
        cTimeMs autoAdvanceTimeout;
        void AdvancePos(void);
        virtual void Set(void);
        char Inc(char c, bool Up);

    protected:
        bool InEditMode(void) { return pos >= 0; }

    public:
        cSatlistItem(std::vector<cSatList*>::iterator satlistIter, cDiseqcSetting *DiseqcSetting, const char *Name, const char *Allowed);
        ~cSatlistItem();

        void SetHelpKeys(void);
        virtual eOSState ProcessKey(eKeys Key);
};

class cDiseqcSettingItem : public cOsdItem
{
    private:
        bool *bShow_;

    public:
        cDiseqcSettingItem(bool *bShow);
        ~cDiseqcSettingItem();

        eOSState ProcessKey(eKeys Key);
};

class cDiseqcSettingModeItem : public cOsdItem
{
    public:
        std::vector<cDiseqcSettingBase*>::iterator *modeIter_;
        std::vector<cDiseqcSettingBase*>::iterator firstIter_;
        std::vector<cDiseqcSettingBase*>::iterator lastIter_;

        cDiseqcSettingModeItem(std::string name, std::vector<cDiseqcSettingBase*>::iterator *modeIter, std::vector<cDiseqcSettingBase*>::iterator firstIter, std::vector<cDiseqcSettingBase*>::iterator lastIter);
        ~cDiseqcSettingModeItem();

        eOSState ProcessKey(eKeys Key);
};

class cMenuEditIntpItem : public cMenuEditItem
{
    protected:
        virtual void Set(void);
        int min, max;
        const char *string1, *string2;
        int sgn, rvalue;
        int *value;
    public:
        cMenuEditIntpItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *String1 = "", const char *String2 = NULL);
        virtual eOSState ProcessKey(eKeys Key);
};

#endif
