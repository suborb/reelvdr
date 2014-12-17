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
 * diseqcsettingtitem.c
 *
 ***************************************************************************/

#include "diseqcsettingitem.h"

#include <vdr/remote.h>

#define AUTO_ADVANCE_TIMEOUT  1500 // ms before auto advance when entering characters via numeric keys

// class cDiseqcSettingModeItem ---------------------------------------
cDiseqcSettingModeItem::cDiseqcSettingModeItem(std::string name, std::vector<cDiseqcSettingBase*>::iterator *modeIter, std::vector<cDiseqcSettingBase*>::iterator firstIter, std::vector<cDiseqcSettingBase*>::iterator lastIter)
{
    std::string strBuff = "         ";
    strBuff.append(tr("DiSEqC mode:"));
    strBuff = strBuff + "\t" + name;
    SetText(strBuff.c_str());
    modeIter_ = modeIter;
    firstIter_ = firstIter;
    lastIter_ = lastIter;
}

cDiseqcSettingModeItem::~cDiseqcSettingModeItem()
{
}

eOSState cDiseqcSettingModeItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kLeft:
        case kLeft|k_Repeat:
            if (*modeIter_ != firstIter_)
            {
                --*modeIter_;
                return os_User;
            }
            else
                *modeIter_ = lastIter_;
            return os_User;
            break;

        case kRight:
        case kRight|k_Repeat:
            if (*modeIter_ != lastIter_)
            {
                ++*modeIter_;
                return os_User;
            }
            else
                *modeIter_ = firstIter_;
            return os_User;
            break;

        default:
            return osUnknown;
    }
}

// class cMenuEditIntpItem --------------------------------------------
    cMenuEditIntpItem::cMenuEditIntpItem(const char *Name, int *Value, int Min, int Max, const char *String1,const char *String2)
:cMenuEditItem(Name)
{
    value = Value;
    rvalue = abs(*value);
    sgn = *value>=0;
    string1 = String1;
    string2 = String2;
    min = Min;
    max = Max;
    Set();
}

void cMenuEditIntpItem::Set(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%d° %s", rvalue/10, rvalue % 10, sgn ? string1 : string2);
    SetValue(buf);
}

eOSState cMenuEditIntpItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);
    if (state == osUnknown)
    {
        int newValue = rvalue;
        Key = NORMALKEY(Key);
        switch (Key) {
            case kNone  : break;
            case k0...k9:
                          if (fresh)
                          {
                              rvalue = 0;
                              fresh = false;
                          }
                          newValue = rvalue * 10 + (Key - k0);
                          break;
            case kLeft  :
                          sgn = 0;
                          fresh = true;
                          break;
            case kRight :
                          sgn = 1;
                          fresh = true;
                          break;
            default     :
                          if (rvalue < min) { rvalue = min; Set(); }
                          if (rvalue > max) { rvalue = max; Set(); }
                          *value = rvalue * (sgn ? 1 : -1); 
                          return state;
        }
        if ((!fresh || min <= newValue) && newValue <= max)
        {
            rvalue = newValue;
            Set();
        }
        *value = rvalue * (sgn ? 1: -1);
        state = osContinue;
    }
    return state;
}

// class cMenuEditIntRefreshItem --------------------------------------
cMenuEditIntRefreshItem::cMenuEditIntRefreshItem(const char *Name, int *Value, int Min, int Max, const char *MinString, const char *MaxString) : cMenuEditItem(Name)
{
    value = Value;
    min = Min;
    max = Max;
    minString = MinString;
    maxString = MaxString;
    if (*value < min)
        *value = min;
    else if(*value > max)
        *value = max;
    Set();
}

void cMenuEditIntRefreshItem::Set(void)
{
    if(minString && *value == min)
        SetValue(minString);
    else if(maxString && *value == max)
        SetValue(maxString);
    else
    {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d", *value);
        SetValue(buf);
    }
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
            case kLeft:
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
            state = os_User; // Refresh Display
        }
        else
            state = osContinue;
    }
    return state;
}

// class cMenuEditBoolRefreshItem -------------------------------------
cMenuEditBoolRefreshItem::cMenuEditBoolRefreshItem(const char *Name, int *Value, const char *FalseString, const char *TrueString)
    :cMenuEditIntItem(Name, Value, 0, 1)
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

eOSState cMenuEditBoolRefreshItem::ProcessKey(eKeys Key)
{
    switch(Key)
    {
        case kLeft:
        case kLeft|k_Repeat:
        case kRight:
        case kRight|k_Repeat:
            if(*value == 1)
                *value = 0;
            else
                *value = 1;
            return os_User;
            break;
        default:
            return osUnknown;
    }
}

// class cMenuEditStraRefreshItem -------------------------------------
cMenuEditStraRefreshItem::cMenuEditStraRefreshItem(const char *Name, int *Value, int NumStrings, const char* const *Strings) : cMenuEditIntRefreshItem(Name, Value, 0, NumStrings - 1)
{
    strings = Strings;
    Set();
}

void cMenuEditStraRefreshItem::Set(void)
{
    SetValue(strings[*value]);
}

// class cSatlistItem -------------------------------------------------
cSatlistItem::cSatlistItem(std::vector<cSatList*>::iterator satlistIter, cDiseqcSetting *DiseqcSetting, const char* Name, const char *Allowed)
: cMenuEditItem(Name)
{
    satlistIter_ = satlistIter;
    DiseqcSetting_ = DiseqcSetting;

    orgValue = NULL;
    value = (*satlistIter)->Name_;
    length = SATLIST_STRING_LENGTH;
    allowed = strdup(Allowed ? Allowed : "");
    pos = -1;
    insert = uppercase = false;
    newchar = true;
    charMap = tr(" 0\t-.#~,/_@1\tabc2\tdef3\tghi4\tjkl5\tmno6\tpqrs7\ttuv8\twxyz9");
    currentChar = NULL;
    lastKey = kNone;
    Set();
}

cSatlistItem::~cSatlistItem()
{
    free(orgValue);
    free(allowed);
}

void cSatlistItem::SetHelpKeys(void)
{
    if (InEditMode())
        cSkinDisplay::Current()->SetButtons(trVDR("Button$ABC/abc"), insert ? trVDR("Button$Overwrite") : trVDR("Button$Insert"), trVDR("Button$Delete"));
    else
        cSkinDisplay::Current()->SetButtons(tr("Save"), NULL, trVDR("Button$Delete"));
}

void cSatlistItem::AdvancePos(void)
{
    if (pos < length - 2 && pos < int(strlen(value)) ) {
        if (++pos >= int(strlen(value))) {
            if (pos >= 2 && value[pos - 1] == ' ' && value[pos - 2] == ' ')
                pos--; // allow only two blanks at the end
            else {
                value[pos] = ' ';
                value[pos + 1] = 0;
            }
        }
    }
    newchar = true;
    if (!insert && isalpha(value[pos]))
        uppercase = isupper(value[pos]);
}

void cSatlistItem::Set(void)
{
    char buf[1000];

    if (InEditMode()) {
        // This is an ugly hack to make editing strings work with the 'skincurses' plugin.
        const cFont *font = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->GetTextAreaFont(false);
        if (!font || font->Width("W") != 1) // all characters have with == 1 in the font used by 'skincurses'
            font = cFont::GetFont(fontOsd);
        strncpy(buf, value, pos);
        snprintf(buf + pos, sizeof(buf) - pos - 2, insert && newchar ? "[]%c%s" : "[%c]%s", *(value + pos), value + pos + 1);
        int width = cSkinDisplay::Current()->EditableWidth();
        if (font->Width(buf) <= width) {
            // the whole buffer fits on the screen
            SetValue(buf);
            return;
        }
        width -= font->Width('>'); // assuming '<' and '>' have the same width
        int w = 0;
        int i = 0;
        int l = strlen(buf);
        while (i < l && w <= width)
            w += font->Width(buf[i++]);
        if (i >= pos + 4) {
            // the cursor fits on the screen
            buf[i - 1] = '>';
            buf[i] = 0;
            SetValue(buf);
            return;
        }
        // the cursor doesn't fit on the screen
        w = 0;
        if (buf[i = pos + 3]) {
            buf[i] = '>';
            buf[i + 1] = 0;
        }
        else
            i--;
        while (i >= 0 && w <= width)
            w += font->Width(buf[i--]);
        buf[++i] = '<';
        SetValue(buf + i);
    }
    else
        SetValue(value);
}

char cSatlistItem::Inc(char c, bool Up)
{
    const char *p = strchr(allowed, c);
    if (!p)
        p = allowed;
    if (Up) {
        if (!*++p)
            p = allowed;
    }
    else if (--p < allowed)
        p = allowed + strlen(allowed) - 1;
    return *p;
}

eOSState cSatlistItem::ProcessKey(eKeys Key)
{
    bool SameKey = NORMALKEY(Key) == lastKey;
    if (Key != kNone)
        lastKey = NORMALKEY(Key);
    else if (!newchar && k0 <= lastKey && lastKey <= k9 && autoAdvanceTimeout.TimedOut()) {
        AdvancePos();
        newchar = true;
        currentChar = NULL;
        Set();
        return osContinue;
    }
    switch (Key)
    {
        case kRed:   // Switch between upper- and lowercase characters
            if (InEditMode()) {
                if (!insert || !newchar) {
                    uppercase = !uppercase;
                    value[pos] = uppercase ? toupper(value[pos]) : tolower(value[pos]);
                }
            }
            else
            {
                DiseqcSetting_->SaveSetting();
                return osContinue;
            }
            break;
        case kGreen: // Toggle insert/overwrite modes
            if (InEditMode()) {
                insert = !insert;
                newchar = true;
                SetHelpKeys();
            }
            else
                return osUnknown;
            break;
        case kYellow|k_Repeat:
        case kYellow: // Remove the character at current position; in insert mode it is the character to the right of cursor
            if (InEditMode()) {
                if (strlen(value) > 1) {
                    if (!insert || pos < int(strlen(value)) - 1)
                        memmove(value + pos, value + pos + 1, strlen(value) - pos);
                    else if (insert && pos == int(strlen(value)) - 1)
                        value[pos] = ' '; // in insert mode, deleting the last character replaces it with a blank to keep the cursor position
                    // reduce position, if we removed the last character
                    if (pos == int(strlen(value)))
                        pos--;
                }
                else if (strlen(value) == 1)
                    value[0] = ' '; // This is the last character in the string, replace it with a blank
                if (isalpha(value[pos]))
                    uppercase = isupper(value[pos]);
                newchar = true;
            }
            else
            {
                DiseqcSetting_->GetSatlistVector()->erase(satlistIter_);
                return os_User;
            }
            //                return osUnknown;
            break;
        case kBlue|k_Repeat:
        case kBlue:  // consume the key only if in edit-mode
            if (!InEditMode())
                return osUnknown;
            break;
        case kLeft|k_Repeat:
        case kLeft:
            if (pos > 0) {
                if (!insert || newchar)
                    pos--;
                newchar = true;
            }
            if (!insert && isalpha(value[pos]))
                uppercase = isupper(value[pos]);
            break;
        case kRight|k_Repeat:
        case kRight:
            AdvancePos();
            if (pos == 0) {
                orgValue = strdup(value);
                SetHelpKeys();
            }
            break;
        case kUp|k_Repeat:
        case kUp:
        case kDown|k_Repeat:
        case kDown:
            if (InEditMode()) {
                if (insert && newchar) {
                    // create a new character in insert mode
                    if (int(strlen(value)) < length - 1) {
                        memmove(value + pos + 1, value + pos, strlen(value) - pos + 1);
                        value[pos] = ' ';
                    }
                }
                if (uppercase)
                    value[pos] = toupper(Inc(tolower(value[pos]), NORMALKEY(Key) == kUp));
                else
                    value[pos] =         Inc(        value[pos],  NORMALKEY(Key) == kUp);
                newchar = false;
            }
            else
                return cMenuEditItem::ProcessKey(Key);
            break;
        case k0|k_Repeat ... k9|k_Repeat:
        case k0 ... k9:
            {
                if (InEditMode()) {
                    if (!SameKey) {
                        if (!newchar)
                            AdvancePos();
                        currentChar = NULL;
                    }
                    if (!currentChar || !*currentChar || *currentChar == '\t') {
                        // find the beginning of the character map entry for Key
                        int n = NORMALKEY(Key) - k0;
                        currentChar = charMap;
                        while (n > 0 && *currentChar) {
                            if (*currentChar++ == '\t')
                                n--;
                        }
                        // find first allowed character
                        while (*currentChar && *currentChar != '\t' && !strchr(allowed, *currentChar))
                            currentChar++;
                    }
                    if (*currentChar && *currentChar != '\t') {
                        if (insert && newchar) {
                            // create a new character in insert mode
                            if (int(strlen(value)) < length - 1) {
                                memmove(value + pos + 1, value + pos, strlen(value) - pos + 1);
                                value[pos] = ' ';
                            }
                        }
                        value[pos] = *currentChar;
                        if (uppercase)
                            value[pos] = toupper(value[pos]);
                        // find next allowed character
                        do {
                            currentChar++;
                        } while (*currentChar && *currentChar != '\t' && !strchr(allowed, *currentChar));
                        newchar = false;
                        autoAdvanceTimeout.Set(AUTO_ADVANCE_TIMEOUT);
                    }
                }
                else
                    return cMenuEditItem::ProcessKey(Key);
            }
            break;
        case kBack:
            if(!InEditMode())
                return osBack;
        case kOk:
            if (InEditMode()) {
                if (Key == kBack && orgValue) {
                    strcpy(value, orgValue);
                    free(orgValue);
                    orgValue = NULL;
                }
                pos = -1;
                newchar = true;
                stripspace(value);
                SetHelpKeys();
                break;
            }
            else if(kOk)
            {
                (*satlistIter_)->bShowSettings_ = !(*satlistIter_)->bShowSettings_;
                return os_User;
            }
            // run into default
        default:
            if (InEditMode() && BASICKEY(Key) == kKbd) {
                int c = KEYKBD(Key);
                if (c <= 0xFF) {
                    const char *p = strchr(allowed, tolower(c));
                    if (p) {
                        int l = strlen(value);
                        if (insert && l < length - 1)
                            memmove(value + pos + 1, value + pos, l - pos + 1);
                        value[pos] = c;
                        if (pos < length - 2)
                            pos++;
                        if (pos >= l) {
                            value[pos] = ' ';
                            value[pos + 1] = 0;
                        }
                    }
                    else {
                        switch (c) {
                            case 0x7F: // backspace
                                if (pos > 0) {
                                    pos--;
                                    return ProcessKey(kYellow);
                                }
                                break;
                        }
                    }
                }
                else {
                    switch (c) {
                        case kfHome: pos = 0; break;
                        case kfEnd:  pos = strlen(value) - 1; break;
                        case kfIns:  return ProcessKey(kGreen);
                        case kfDel:  return ProcessKey(kYellow);
                    }
                }
            }
            else
                return cMenuEditItem::ProcessKey(Key);
    }
    Set();
    return osContinue;
}

