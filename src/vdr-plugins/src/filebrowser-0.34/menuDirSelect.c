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

#include "menuDirSelect.h"
#include "menuBrowser.h"

#include <vector>

//--------------cMenuDirSelect-----------------------

cMenuDirSelect::cMenuDirSelect(std::string title, cFileBrowserStatus *newStatus, cFileCache *cache, cImageConvert *convert, cThumbConvert *thumbConvert)
:cOsdMenu(title.c_str()), newStatus_(newStatus), cache_(cache), convert_(convert), thumbConvert_(thumbConvert)
{
    Set();
}  
    
eOSState cMenuDirSelect::EnterDir()
{
    if(Count())
    {
        std::string startDir = newStatus_->GetStartDirs()[Current()];
        std::string title = startDir + ": " + newStatus_->GetExternalTitleString();
        newStatus_->SetStartDir(startDir);
        return AddSubMenu(new cMenuFileBrowser(title.c_str(), newStatus_, cache_, convert_, thumbConvert_));
    }
    return osContinue;
}

void cMenuDirSelect::Set()
{
    SetTitle(newStatus_->GetExternalTitleString().c_str());

    char buf[256];
    const std::vector<std::string> &dirs = newStatus_->GetStartDirs();
    for(uint i = 0; i < dirs.size(); ++i)
    {
        sprintf(buf, "%d %s", i + 1, dirs[i].c_str());
        Add(new cOsdItem(buf, osUnknown, true));
    }
}

eOSState cMenuDirSelect::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;
 
    state = cOsdMenu::ProcessKey(Key);
    
    if(state == osUnknown)
    {
        switch(Key)
        {
            case kOk:
                state = EnterDir();
                break;
            case kBack:
                state = osBack;
                break;
            default:
                break;
        }
    }
    return state;
}
