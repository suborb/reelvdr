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
#include "convert.h"
#include <iostream>
#include <sstream>

#include "def.h"
#include "mountmanager.h"
#include "browserItems.h"

//-----------cMenuBrowserItem------------------------------

eOSState cMenuBrowserItem::Update(cMenuFileBrowserBase *menu, bool currentChanged) 
{
    menu->ShowThumbnail(cPlayListItem(), true, currentChanged); 
    menu->ShowID3TagInfo(cPlayListItem(), true, currentChanged);
    return osContinue;
}

//-----------cMenuBrowserEditStrItem------------------------------

cMenuBrowserEditStrItem::cMenuBrowserEditStrItem(cMenuFileBrowserBase* menu,
const char *Name, char *Value, int Length, const char *Allowed)
:  cMenuBrowserItem(menu), cMenuEditStrItem(Name, Value, Length, Allowed)
{
}

//-----------cMenuEmptyItem---------------------

cMenuEmptyItem::cMenuEmptyItem(cMenuFileBrowserBase* menu)
:cMenuBrowserItem(menu), cOsdItem("", osUnknown, false)
{
}

//-----------cMenuDirEntryItem------------------------------

cMenuDirEntryItem::cMenuDirEntryItem(std::string path, cMenuFileBrowserBase* Menu)
: cMenuBrowserItem(Menu), id_(path)
{ 
}

cMenuDirEntryItem::cMenuDirEntryItem(const itemId &id, cMenuFileBrowserBase* Menu)
: cMenuBrowserItem(Menu), id_(id)
{ 
}

std::string cMenuDirEntryItem::GetPath() const
{
    return  id_.path_;
}

itemId cMenuDirEntryItem::GetId() const
{
    return  id_;
}

void cMenuDirEntryItem::Mark()
{
    isMarked = true;
    Set();
}

void cMenuDirEntryItem::UnMark()
{
    isMarked = false;
    Set();
}

void cMenuDirEntryItem::Flag()
{
    isFlagged = true;
    Set();
}

void cMenuDirEntryItem::UnFlag()
{
    isFlagged = false;
    Set();
}

//-----------cMenuDirItem------------------------------

cMenuDirItem::cMenuDirItem(std::string path, cMenuFileBrowserBase* menu)
: cMenuDirEntryItem(path, menu), type_(path)
{
}

eOSState cMenuDirItem::Open(cMenuFileBrowserBase *menu, bool asPlayListItem)
{
    return type_.Open(GetPath(), menu);
}

eOSState cMenuDirItem::Play(cMenuFileBrowserBase *menu)
{
    return type_.Play(GetPath(), menu);
}

eOSState cMenuDirItem::OpenInfo() const
{
    return osContinue;
}

void cMenuDirItem::Set()
{
    std::string name;

    if(GetLast(GetPath()) == "..")
        name = std::string("\x80");
    else
        name = type_.GetSymbol() + menu->GetAdjustedEntryName(GetPath());

    if(isMarked)
    {
        name = std::string(">\t") + name;
    }

    SetText(name.c_str());
}

//-----------cMenuFileItem------------------------------

cMenuFileItem::cMenuFileItem(std::string path, cMenuFileBrowserBase *menu)
: cMenuDirEntryItem(path, menu), item_(path)
{
} 

cMenuFileItem::cMenuFileItem(cPlayListItem item, cMenuFileBrowserBase *menu)
: cMenuDirEntryItem(item.GetId(), menu), item_(item)
{
}

cMenuFileItem::~cMenuFileItem()
{
}

eOSState cMenuFileItem::Open(cMenuFileBrowserBase *menu, bool asPlayListItem)
{
    return item_.Open(menu, asPlayListItem);
}

eOSState cMenuFileItem::Play(cMenuFileBrowserBase *menu)
{
    return item_.Open(menu);
}

eOSState cMenuFileItem::Update(cMenuFileBrowserBase *menu, bool currentChanched)
{
    return item_.Update(menu, currentChanched);
}

eOSState cMenuFileItem::OpenInfo() const
{
    return menu->OpenFileInfo(*this);
}

void cMenuFileItem::Set()
{
    //printf("-----cMenuFileItem::Set %s-----------\n", GetPath().c_str()); 
    std:: string name = item_.GetFileType().GetSymbol() + menu->GetAdjustedEntryName(GetPath());

    if(isFlagged)
    {
        name = std::string("\x88\t") + name;
    }
    if(isMarked)
    {
        name = std::string(">\t") + name;
    }

    /*const char *nameptr = name.c_str();
    while(*nameptr)
    {
        printf("cMenuFileItem::Set(): %08x : %c\n", *nameptr, *nameptr);
        ++nameptr;
    }*/

    SetText(name.c_str());
}

std::vector<std::string> cMenuFileItem::GetFileInfoString() const
{
    //std::string info;
    //std::ostringstream o;
    //o << statbuf_.st_size;
    //info += tr("Filesize:") + o.str() +"  ";
    //info += "Last Modification Date: 
    //off_tbuf.st_size  //File Size in Bytes
    //time_t buf.st_mtime //Last Modification Time 
    //return info;
    return item_.GetFileType().GetFileInfo(GetPath());
}

//-----------cMenuDirLinkItem------------------------------
cMenuDirLinkItem::cMenuDirLinkItem(std::string path, cMenuFileBrowserBase* menu, bool isDead)
: cMenuDirItem(path, menu), isDead_(isDead)
{
}

void cMenuDirLinkItem::Set()
{
    std::string name = menu->GetAdjustedEntryName(GetPath());
    if(isDead_)
    {
        name = "! " + name;
    }
    else
    {
        name = type_.GetSymbol() + menu->GetAdjustedEntryName(GetPath());
    }
    if(isMarked)
    {
        name = std::string(">\t") + name;
    }
    SetText(name.c_str());
}

//-----------cMenuFileLinkItem------------------------------

cMenuFileLinkItem::cMenuFileLinkItem(std::string path, cMenuFileBrowserBase* menu, bool isDead)
: cMenuFileItem(path, menu), isDead_(isDead)
{
}

void cMenuFileLinkItem::Set()
{
    std:: string name = item_.GetFileType().GetSymbol() + menu->GetAdjustedEntryName(GetPath());
    if(isDead_)
    {
        name = "! " + name;
    }
    if(isFlagged)
    {
        name = std::string("\x88\t") + name;
    }
    if(isMarked)
    {
        name = std::string(">\t") + name;
    }

    SetText(name.c_str());
}

