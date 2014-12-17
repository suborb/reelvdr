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


#include "stringtools.h"
#include "menuBrowserBase.h" 
#include "browserItems.h"
#include "menufileinfo.h"

//----------cMenuFileInfo-----------------------

cMenuFileInfo::cMenuFileInfo(const std::string title, const cMenuFileItem &fileitem)
:cMenuFileBrowserBase(title)
{
    SetCols(14);
    SetTitle(tr("File informations"));
    Add(new cOsdItem((tr("Attributes from:") + std::string(" ") + GetLast(fileitem.GetPath())).c_str(), osUnknown, true));  
    Add(new cOsdItem("", osUnknown, false));  
    std::vector<std::string> fileInfoString = fileitem.GetFileInfoString();

    uint i;
    for(i=0; i< fileInfoString.size(); i++)
        Add(new cOsdItem(fileInfoString.at(i).c_str(), osUnknown, true));   
    //Add(new cOsdItem(fileitem.GetFileInfoString().c_str(), osUnknown, false); 
} 

eOSState cMenuFileInfo::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (Key)
    {
        case kBack:   
        case kOk: 
            return osBack;

        default:
            break;
    }

    return state;
}
