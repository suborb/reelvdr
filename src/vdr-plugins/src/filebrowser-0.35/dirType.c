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

#include <stdio.h>
#include <cstring>

#include <vdr/plugin.h>
#include <vdr/remote.h>

#include "menuBrowserBase.h"
#include "stringtools.h"
#include "dirType.h"
#include "filetools.h"


//----------cDirType-----------------------

cDirType::cDirType(edirtype dtype)
{
    GetDirType(dtype);
}

cDirType::cDirType(std::string path)
{
    GetDirType(path);
}

edirtype cDirType::GetType() const
{
    return type_->GetType();
}

std::string cDirType::GetSymbol() const
{
    return type_->GetSymbol();
}

eOSState cDirType::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return type_->Open(item, menu, asPlaylist);
}

eOSState cDirType::Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return type_->Play(item, menu, asPlaylist);
}

eOSState cDirType::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged) const
{
    return type_->Update(item, menu, currentChanged);
}

bool cDirType::IsPlayable() const
{
    return type_->IsPlayable();
}

bool cDirType::IsXinePlayable() const
{
    return type_->IsXinePlayable();
}

bool cDirType::IsImage() const
{
    return type_->IsImage();
}

void cDirType::GetDirType(std::string path)
{
    //printf (" cDirType::GetDirType %s \n", path.c_str());
    if(cDirTypeBaseVdrRec::IsVdrRec(path))
        type_ = &cDirTypeBaseVdrRec::Instance();
    else if(cDirTypeBaseDvd::IsDvd(path))
        type_ = &cDirTypeBaseDvd::Instance();
    else if(cDirTypeBaseDvdDir::IsDvdDir(path))
        type_ = &cDirTypeBaseDvdDir::Instance();
    else
        type_ = &cDirTypeBase::Instance();
}

void cDirType::GetDirType(edirtype type)
{
    //printf (" cDirType::GetDirType %d \n", type);
    if(type == tp_vdr_rec)
        type_ = &cDirTypeBaseVdrRec::Instance();
    else if(type == tp_dvd)
        type_ = &cDirTypeBaseDvd::Instance();
    else if(type == tp_dvd_dir)
	type_ = &cDirTypeBaseDvdDir::Instance();
    else
        type_ = &cDirTypeBase::Instance();
}

//----------cDirTypeBase-----------------------

cDirTypeBase cDirTypeBase::instance;

std::string cDirTypeBase::GetSymbol() const
{
    return "\x82\t";
}

eOSState cDirTypeBase::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->EnterSubDir(item.GetPath());
}

eOSState cDirTypeBase::Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return Open(item, menu);
}

eOSState cDirTypeBase::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged) const
{
    return osContinue;
}

bool cDirTypeBase::IsPlayable() const
{
    return false;
}

bool cDirTypeBase::IsXinePlayable() const
{
    return false;
}

bool cDirTypeBase::IsImage() const
{
    return false;
}

//----------cDirTypeBaseVdrRec---------------------------

cDirTypeBaseVdrRec cDirTypeBaseVdrRec::instance;

eOSState cDirTypeBaseVdrRec::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->EnterSubDir(item.GetPath());
}

eOSState cDirTypeBaseVdrRec::Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist)  const
{
    cRemote::CallPlugin("extrecmenu");
    return osEnd;
}

bool cDirTypeBaseVdrRec::IsPlayable() const
{
    return true;
}

bool cDirTypeBaseVdrRec::IsVdrRec(std::string path)
{
    if(path != "/media/hd/recordings")
        return false;
    return true;
}

//----------cDirTypeBaseDvd---------------------------
cDirTypeBaseDvd cDirTypeBaseDvd::instance;

eOSState cDirTypeBaseDvd::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist)  const
{
    return menu->EnterSubDir(item.GetPath());
}

eOSState cDirTypeBaseDvd::Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->EnterSubDir(item.GetPath());
    //printf("----- call mediad-----\n");
    //cRemote::CallPlugin("mediad");
    //return osEnd;
}

bool cDirTypeBaseDvd::IsPlayable() const
{
    return true;
}

bool cDirTypeBaseDvd::IsXinePlayable() const
{
    return true;
}

bool cDirTypeBaseDvd::IsDvd(std::string path)
{
    if(path != "/media/dvd")
        return false;
    return true;
}

//----------cDirTypeBaseDvdDir---------------------------

cDirTypeBaseDvdDir cDirTypeBaseDvdDir::instance;

std::string cDirTypeBaseDvdDir::GetSymbol() const
{
    return "\x81\t";
}

eOSState cDirTypeBaseDvdDir::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->EnterSubDir(item.GetPath());
}

eOSState cDirTypeBaseDvdDir::Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    std::string path = item.GetPath();
    
    if(strcmp(GetLast(path).c_str(), "VIDEO_TS"))
	path += "/VIDEO_TS";

    //const cPlayListItem *item = new cPlayListItem(path);
    return menu->ShowDvdWithXine(item, false);
}

bool cDirTypeBaseDvdDir::IsPlayable() const
{
    return true;
}  

bool cDirTypeBaseDvdDir::IsDvdDir(std::string path)
{
    /* directory itself is a "VIDEO_TS"-directory */
    if(!strcmp(GetLast(path).c_str(), "VIDEO_TS")){
	//printf("\"%s\" IS DVD DIR :-)\n", path.c_str());
        return true;
    }

    std::string test_path;
    test_path = path + "/VIDEO_TS";
    if (access(test_path.c_str(), R_OK|X_OK) == 0)
        return true;
    test_path = path + "/video_ts";
    if (access(test_path.c_str(), R_OK|X_OK) == 0)
        return true;

    return false;
}
