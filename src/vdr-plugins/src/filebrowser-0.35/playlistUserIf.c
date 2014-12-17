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

#include "browserStatus.h"
#include "menuplaylist.h"
#include "playlistUserIf.h"

//user interface for playlist menu (state pattern)

//----------------------------------------------------------------
//-----------------cMenuPlayListBaseIf----------------------------
//----------------------------------------------------------------

eOSState cMenuPlayListBaseIf::AnyKey(cMenuPlayList *menu, eKeys Key)
{
    return menu->StandardKeyHandling(Key);
}

void cMenuPlayListBaseIf::EnterState(cMenuPlayList *menu, cMenuPlayListBaseIf *lastIf)
{
    ChangeIfType(if_playlistmenu);
    menu->SetTitleString(menu->GetTitleString());
    SetHelp(menu);
    menu->Display();
}

void cMenuPlayListBaseIf::SetHelp(cMenuPlayList *menu)
{
    menu->setHelp.SetRed("");
    menu->setHelp.SetGreen("");
    menu->setHelp.SetYellow("");
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false); 
}

//----------------------------------------------------------------
//-----------------cMenuPlayListStandardIf------------------------
//----------------------------------------------------------------

cMenuPlayListStandardIf cMenuPlayListStandardIf::instance;

eOSState cMenuPlayListStandardIf::RedKey(cMenuPlayList *menu)
{
    menu->MarkUnmarkCurrentItem();
    SetHelp(menu);
    return osContinue;
}

eOSState cMenuPlayListStandardIf::GreenKey(cMenuPlayList *menu)
{
    menu->MarkUnmarkAllFiles(); 
    SetHelp(menu);
    return osContinue;
}

eOSState cMenuPlayListStandardIf::YellowKey(cMenuPlayList *menu)
{
    menu->SetMode(browseplaylist);
    return osBack;
}

eOSState cMenuPlayListStandardIf::BlueKey(cMenuPlayList *menu)
{
    menu->userIf_ = cMenuPlayListCmdIf::Instance();
    menu->userIf_->EnterState(menu);
    return osContinue;
}

eOSState cMenuPlayListStandardIf::OkKey(cMenuPlayList *menu)
{
    return menu->SetAndOpenCurrent();
}

eOSState cMenuPlayListStandardIf::BackKey(cMenuPlayList *menu)
{
    if(GetCaller() == if_xine)
    {
        menu->ShowPlayListWithXine();
    }
    else if(GetCaller() == if_restricted || GetCaller() == if_playlistbrowser)
    {
        menu->SetMode(restricted);
        if(menu->PlayListIsDirty())
        {
            menu->userIf_ = cMenuPlayListShutdownCmdIf::Instance();
            menu->userIf_->EnterState(menu);
            return osContinue;
        }
        else
        {
            return osBack;
        }
    }
    else if(GetCaller() == if_standard)
    { 
        menu->SetMode(expert);
	    return osBack;
    } 
    return osContinue;
}

eOSState cMenuPlayListStandardIf::AnyKey(cMenuPlayList *menu, eKeys Key)
{ 
    eOSState state =  menu->StandardKeyHandling(Key);
    if(!menu->GetBrowserSubMenu())
    {
        SetHelp(menu);
    }
    return state;
}

eOSState cMenuPlayListStandardIf::FastFwdKey(cMenuPlayList *menu)
{
    menu->ShiftCurrent(true);
    return osContinue;
}

eOSState cMenuPlayListStandardIf::FastRewKey(cMenuPlayList *menu)
{
    menu->ShiftCurrent(false);
    return osContinue;
}

void cMenuPlayListStandardIf::SetHelp(cMenuPlayList *menu)
{
    cMenuBrowserItem *item = menu->GetCurrentItem();
    if(item && menu->IsMarked(item))
    {
        menu->setHelp.SetRed("Deselect");
    }
    else if(!menu->NoFiles()) /** TB: "Select" doesn't make sense with no entries in the list */
    {
        menu->setHelp.SetRed("Select");
    }
    if (!menu->NoFiles()) /** TB: "select all" and "deselect all" only make sense with entries in the list */
    {
        if(menu->AllFilesAreMarked())
        {
            menu->setHelp.SetGreen("Deselect all");
        }
        else
        {
            menu->setHelp.SetGreen("Select all");
        }
    }
    menu->setHelp.SetYellow("Insert");
    menu->setHelp.SetBlue("Functions");
    menu->setHelp.FlushButtons(false);
}

//----------------------------------------------------------------
//----------------cMenuPlayListOptionsIf--------------------------
//----------------------------------------------------------------

cMenuPlayListOptionsIf cMenuPlayListOptionsIf::instance;

eOSState cMenuPlayListOptionsIf::OkKey(cMenuPlayList *menu)
{
    std::string option = menu->Get(menu->Current())->Text();
    menu->Hide(false);
    return ExecuteOptions(menu, option);
}

eOSState cMenuPlayListOptionsIf::BackKey(cMenuPlayList *menu)
{ 
    menu->Hide(false);
    return LeaveStateDefault(menu);
}

void cMenuPlayListOptionsIf::EnterState(cMenuPlayList *menu, cMenuPlayListBaseIf *lastIf)
{
    lastIf_ = lastIf;
    CreateOptions(menu);
    menu->Hide(true);
    AddOptions(menu);
    SetHelp(menu);
    menu->Display();
}

eOSState cMenuPlayListOptionsIf::LeaveStateDefault(cMenuPlayList *menu)
{
    if(lastIf_)
    {
        menu->userIf_ = lastIf_;
    }
    else
    {
        menu->userIf_ = cMenuPlayListStandardIf::Instance();
    }
    menu->userIf_->EnterState(menu, this);
    return osContinue;
}

void cMenuPlayListOptionsIf::AddOptions(cMenuPlayList *menu)
{
    char buf[256];
    //TODO: use stringstreams
    for(uint i = 0; i < options_.size(); ++i)
    {
        sprintf(buf, "%d %s", i + 1, options_[i].c_str());
        menu->Add(new cOsdItem(buf, osUnknown, true));
    }
}

//----------------------------------------------------------------
//-----------------cPlayListCmdIf--------------------------
//----------------------------------------------------------------

cMenuPlayListCmdIf cMenuPlayListCmdIf::instance;

eOSState cMenuPlayListCmdIf::ExecuteOptions(cMenuPlayList *menu, std::string option)
{
    //printf("#####--------cMenuPlayListCmdIf::ExecuteOptions = %s----------------\n", option.c_str());
    if(option.find(tr("Delete entry")) == 2)
    {
        return DeleteEntries(menu);
    }
    else if(option.find(tr("Close playlist")) == 2)
    {
        return ClosePlayList(menu);
    }
    else if(option.find(tr("Save playlist as")) == 2)
    {
        return SavePlayListAs(menu);
    }
    else if(option.find(tr("Save playlist")) == 2)
    {
        return SavePlayList(menu);
    }
    else if(option.find(tr("Copy playlist to CD")) == 2)
    {
        return CopyPlayListToCD(menu);
    }
    return osContinue;
}

void cMenuPlayListCmdIf::CreateOptions(cMenuPlayList *menu)
{
    menu->SetTitleString(tr("Playlist Commands"));
    options_.clear();
    if(!menu->NoFiles()) /** TB: deleting an entry only makes sense if there is an entry */
        options_.push_back(tr("Delete entry"));
    options_.push_back(tr("Close playlist"));
    options_.push_back(tr("Save playlist"));
    options_.push_back(tr("Save playlist as"));
    options_.push_back(tr("Copy playlist to CD"));
}

eOSState cMenuPlayListCmdIf::DeleteEntries(cMenuPlayList *menu)
{
    menu->DeleteMarkedEntries();
    return LeaveStateDefault(menu);
}

eOSState cMenuPlayListCmdIf::ClosePlayList(cMenuPlayList *menu)
{
    menu->ClosePlayList(); 
    return LeaveStateDefault(menu);
}

eOSState cMenuPlayListCmdIf::SavePlayList(cMenuPlayList *menu)
{
    menu->SavePlayList(); 
    return LeaveStateDefault(menu);
}

eOSState cMenuPlayListCmdIf::SavePlayListAs(cMenuPlayList *menu)
{
    menu->userIf_ = cMenuPlayListRenamePlayListIf::Instance();
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuPlayListCmdIf::CopyPlayListToCD(cMenuPlayList *menu)
{
    menu->CopyPlayListToCD();
    return LeaveStateDefault(menu);
}


//----------------------------------------------------------------
//----------------cMenuPlaylistShutdownCmdIf-------------------
//----------------------------------------------------------------

cMenuPlayListShutdownCmdIf cMenuPlayListShutdownCmdIf::instance;

eOSState cMenuPlayListShutdownCmdIf::BackKey(cMenuPlayList *menu)
{
    return ClosePlayList(menu);
}

eOSState cMenuPlayListShutdownCmdIf::LeaveStateDefault(cMenuPlayList *menu)
{
    return osBack;
}

eOSState cMenuPlayListShutdownCmdIf::ExecuteOptions(cMenuPlayList *menu, std::string option)
{
    if(option.find(tr("Close playlist")) == 2)
    {
        return ClosePlayList(menu);
    }
    if(option.find(tr("Save playlist as")) == 2)
    {
        return SavePlayListAs(menu);
    }
    else if(option.find(tr("Save playlist")) == 2)
    {
        return SavePlayList(menu);
    }
    else if(option.find(tr("Copy playlist to CD")) == 2)
    {
        return SavePlayList(menu);
    }
    return osContinue;
}

void cMenuPlayListShutdownCmdIf::CreateOptions(cMenuPlayList *menu)
{
    menu->SetTitleString(tr("Playlist Commands"));
    options_.clear(); 
    options_.push_back(tr("Close playlist"));
    options_.push_back(tr("Save playlist"));
    options_.push_back(tr("Save playlist as"));
    options_.push_back(tr("Copy playlist to CD"));
}

//----------------------------------------------------------------
//----------------cMenuPlaylistRenamePlaylistIf-------------------
//----------------------------------------------------------------

cMenuPlayListRenamePlayListIf cMenuPlayListRenamePlayListIf::instance;

eOSState cMenuPlayListRenamePlayListIf::OkKey(cMenuPlayList *menu)
{
    if(!RenamePlayList(menu))
    {
        Skins.Message(mtInfo, tr("Entry already exists"));
    }
    menu->SetCols(2,2,2);
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}

void cMenuPlayListRenamePlayListIf::CreateOptions(cMenuPlayList *menu)
{
    menu->SetTitleString(tr("Save playlist as"));
    oldname_ = (GetLast(menu->GetName())).c_str();
    strcpy(name_, oldname_.c_str());
}

void cMenuPlayListRenamePlayListIf::AddOptions(cMenuPlayList *menu)
{
    menu->SetCols(15);
    menu->Add(new cOsdItem(tr("Change the name and then press 'OK'"), osUnknown, false));
    menu->Add(new cOsdItem("", osUnknown, false));
    menu->Add(new cMenuBrowserEditStrItem(menu,tr("Playlist name"), name_, sizeof(name_), trVDR(FileNameChars)));
}

bool cMenuPlayListRenamePlayListIf::RenamePlayList(cMenuPlayList *menu)
{
    menu->SetName(RemoveName(menu->GetName()) + "/" + name_);
    menu->SavePlayList();
    return true;
}

eOSState cMenuPlayListRenamePlayListIf::BackKey(cMenuPlayList *menu)
{
    menu->SetCols(2,2,2);    
    menu->Hide(false); 
    LeaveStateDefault(menu);
    return osContinue;
}

