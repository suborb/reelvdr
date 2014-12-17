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

#include "userIf.h"
#include "browserStatus.h"
#include "stringtools.h"
#include "def.h"
#include "servicestructs.h"

#include "vdr/interface.h"

#include <time.h>

//-------------------------------------------------------------
//----------------cMenuBrowserBaseIf---------------------------
//-------------------------------------------------------------

void cMenuBrowserBaseIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    SetHelp(menu);
}

void cMenuBrowserBaseIf::SetHelp(cMenuFileBrowser *menu)
{
    menu->setHelp.SetRed("");
    menu->setHelp.SetGreen("");
    menu->setHelp.SetYellow("");
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false);
}

//-------------------------------------------------------------
//-----------------cMenuBrowserBlockedIf-----------------------
//-------------------------------------------------------------

cMenuBrowserBlockedIf cMenuBrowserBlockedIf::instance;

eOSState cMenuBrowserBlockedIf::AnyKey(cMenuFileBrowser *menu, eKeys Key)
{
   menu->GetFinishedTasks();
   if(!menu->HasUnfinishedTasks())
   {
       menu->userIf_ = cMenuBrowserStandardIf::Instance();
       menu->userIf_->EnterState(menu, this);
   }
   return osContinue;
}

//-------------------------------------------------------------
//----------------cMenuBrowserNavigateIf-----------------------
//-------------------------------------------------------------

eOSState cMenuBrowserNavigateIf::OkKey(cMenuFileBrowser *menu)
{
    return menu->OpenCurrent(); //do not change state
}

eOSState cMenuBrowserNavigateIf::BackKey(cMenuFileBrowser *menu)
{
    return menu->ExitDir();
}

eOSState cMenuBrowserNavigateIf::InfoKey(cMenuFileBrowser *menu)
{
    return menu->OpenCurrentInfo(); //do not change state
}

eOSState cMenuBrowserNavigateIf::AnyKey(cMenuFileBrowser *menu, eKeys Key)
{
    eOSState state = menu->StandardKeyHandling(Key);
    if(!menu->GetBrowserSubMenu())
    {
        SetHelp(menu);
#ifndef BLOCK_DURING_BKG_ACTIVITY
        //get finished background tasks
        menu->GetFinishedTasks();
#endif
        bool currentChanged = menu->CurrentChanged();
        menu->UpdateCurrent(currentChanged);
	menu->SetInfoButton(currentChanged);
    }
    return state;
}

//-------------------------------------------------------------
//----------------cMenuBrowserRequestIf------------------------
//-------------------------------------------------------------

eOSState cMenuBrowserRequestIf::AnyKey(cMenuFileBrowser *menu, eKeys Key)
{
    eOSState state = menu->StandardKeyHandling(Key);
    if(Interface->Confirm(GetRequestString().c_str()))
    {
        Action(menu);
        state = LeaveState(menu, true);
    }
    else
    {
        state = LeaveState(menu, false);
    }
    return state;
}

void cMenuBrowserRequestIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    lastIf_ = lastIf;
    defOSState_ = defOSState;
    SetHelp(menu);
}

eOSState cMenuBrowserRequestIf::LeaveStateDefault(cMenuFileBrowser *menu)
{
    if(lastIf_)
    {
        menu->userIf_ = lastIf_;
    }
    else
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    return defOSState_;
}

//-------------------------------------------------------------
//----------------cMenuBrowserDeleteRequestIf------------------
//-------------------------------------------------------------

cMenuBrowserDeleteRequestIf cMenuBrowserDeleteRequestIf::instance;

void cMenuBrowserDeleteRequestIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    SetHelp(menu);
    menu->SetTitleString(menu->GetTitleString());
    menu->Display();
}

void cMenuBrowserDeleteRequestIf::Action(cMenuFileBrowser *menu)
{
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    menu->DeleteMarkedEntries();
#else
    menu->DeleteMarkedOrCurrent();
#endif
}

eOSState cMenuBrowserDeleteRequestIf::LeaveState(cMenuFileBrowser *menu, bool ok)
{
    if(menu->GetMode() == autostart)
    {
        menu->userIf_ = cMenuBrowserAutoStartIf::Instance();
    }
    else
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
    }
    if(ok)
    {
#ifdef BLOCK_DURING_BKG_ACTIVITY
        //enter the blocked state
        menu->userIf_ = cMenuBrowserBlockedIf::Instance();
#endif

        Skins.Message(mtInfo, tr("Selection is being deleted. Please wait..."), 3);
    }
    menu->userIf_->EnterState(menu, this);
    return osContinue;
}

std::string cMenuBrowserDeleteRequestIf::GetRequestString()
{
    return tr("Really Delete?");
}

//-------------------------------------------------------------
//----------------cMenuBrowserPlaylistRequestIf----------------
//-------------------------------------------------------------

cMenuBrowserPlaylistRequestIf cMenuBrowserPlaylistRequestIf::instance;

eOSState cMenuBrowserPlaylistRequestIf::LeaveState(cMenuFileBrowser *menu, bool ok)
{
    if(ok)
    {
        menu->userIf_ = cMenuBrowserPlaylistOptionsIf::Instance();
        menu->userIf_->EnterState(menu, lastIf_, defOSState_);
        return osContinue;
    }
    else
    {
        menu->ClearPlayList();
        return LeaveStateDefault(menu);
    }
}

std::string cMenuBrowserPlaylistRequestIf::GetRequestString()
{
    return tr("Save/copy current playlist?");
}

//-------------------------------------------------------------
//----------------cMenuBrowserShortPlaylistRequestIf----------------
//-------------------------------------------------------------

cMenuBrowserShortPlaylistRequestIf cMenuBrowserShortPlaylistRequestIf::instance;

eOSState cMenuBrowserShortPlaylistRequestIf::LeaveState(cMenuFileBrowser *menu, bool ok)
{
    if(ok)
    {
        std::string path;
        menu->SavePlayList(path);
        std::string msg = std::string(tr("Playlist saved as ")) + menu->GetPreTitle() + GetLast(path);
        Skins.Message(mtInfo, msg.c_str(), 3);
        return LeaveStateDefault(menu);
    }
    else
    {
        menu->ClearPlayList();
        return LeaveStateDefault(menu);
    }
}

//-------------------------------------------------------------
//----------------cMenuBrowserRequestAndNavigateIf-------------
//-------------------------------------------------------------

eOSState cMenuBrowserRequestAndNavigateIf::RedKey(cMenuFileBrowser *menu)
{
    LeaveState(menu, false);
    return osContinue;
}

eOSState cMenuBrowserRequestAndNavigateIf::GreenKey(cMenuFileBrowser *menu)
{
    Action(menu);
    LeaveState(menu, true);
    return osContinue;
}

void cMenuBrowserRequestAndNavigateIf::LeaveState(cMenuFileBrowser *menu, bool ok)
{
    if(menu->GetMode() == autostart)
    {
        menu->userIf_ = cMenuBrowserAutoStartIf::Instance();
    }
    else
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
    }
    if(ok)
    {
#ifdef BLOCK_DURING_BKG_ACTIVITY
        //enter the blocked state
        menu->userIf_ = cMenuBrowserBlockedIf::Instance();
#endif
        SetMessage();
    }
    menu->userIf_->EnterState(menu, this);
}

void cMenuBrowserRequestAndNavigateIf::SetMessage()
{
    Skins.Message(mtInfo, tr("Working"), 3);
}

//-------------------------------------------------------------
//----------------cMenuBrowserExternalIf-----------------------
//-------------------------------------------------------------

cMenuBrowserExternalIf cMenuBrowserExternalIf::instance;

eOSState cMenuBrowserExternalIf::OkKey(cMenuFileBrowser *menu)
{
    return menu->OpenCurrentDir(); //do not change state
}

eOSState cMenuBrowserExternalIf::RedKey(cMenuFileBrowser *menu)
{
    return osBack;
}

eOSState cMenuBrowserExternalIf::GreenKey(cMenuFileBrowser *menu)
{
    Action(menu);
    return osBack;
}

void cMenuBrowserExternalIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    SetHelp(menu);
    menu->SetTitleString(menu->GetTitleString());
    menu->Display();
}

void cMenuBrowserExternalIf::SetHelp(cMenuFileBrowser *menu)
{
    menu->setHelp.SetRed(trNOOP("Cancel"));
    menu->setHelp.SetGreen(trNOOP("Execute"));
    //menu->setHelp.SetYellow(trNOOP("Create dir"));
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false);
}

/*void cMenuBrowserExternalIf::SetMessage()
{
    Skins.Message(mtInfo, tr("Working"), 3);
}*/

void cMenuBrowserExternalIf::Action(cMenuFileBrowser *menu)
{
    //printf("------cMenuBrowserExternalIf::Action = %s-------\n", menu->GetCurrentDirEntryItem()->GetPath().c_str());
    FileBrowserExternalActionInfo *info = menu->GetExternalActionInfo();
    info->returnString1 = menu->GetCurrentDirEntryItem()->GetPath();
    strcpy(info->returnString3, menu->GetCurrentDirEntryItem()->GetPath().c_str());
}

//-------------------------------------------------------------
//----------------cMenuBrowserCopyRequestIf--------------------
//-------------------------------------------------------------

cMenuBrowserCopyRequestIf cMenuBrowserCopyRequestIf::instance;

eOSState cMenuBrowserCopyRequestIf::YellowKey(cMenuFileBrowser *menu)
{
    menu->userIf_ = cMenuBrowserCreateDirIf::Instance();
    menu->userIf_->EnterState(menu, this);
    return osContinue;
}

void cMenuBrowserCopyRequestIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    SetHelp(menu);
    menu->SetTitleString(menu->GetTitleString());
    menu->Display();
    Skins.Message(mtInfo, tr("Paste selected entries"));
}

void cMenuBrowserCopyRequestIf::SetHelp(cMenuFileBrowser *menu)
{
    menu->setHelp.SetRed(trNOOP("Cancel"));
    menu->setHelp.SetGreen(trNOOP("Insert"));
    menu->setHelp.SetYellow(trNOOP("Create dir"));
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false);
}

void cMenuBrowserCopyRequestIf::Action(cMenuFileBrowser *menu)
{
    menu->InsertCopiedEntries();
}

void cMenuBrowserCopyRequestIf::SetMessage()
{
    Skins.Message(mtInfo, tr("Selection is copied in background..."), 3);
}

//----------------------------------------------------------------
//----------------cMenuBrowserMoveRequestIf-----------------------
//----------------------------------------------------------------

cMenuBrowserMoveRequestIf cMenuBrowserMoveRequestIf::instance;

void cMenuBrowserMoveRequestIf::Action(cMenuFileBrowser *menu)
{
    menu->InsertMovedEntries();
}

void cMenuBrowserMoveRequestIf::SetMessage()
{
    Skins.Message(mtInfo, tr("Selection is being moved in background..."), 3); //Moving
}

//-------------------------------------------------------------
//----------------cMenuBrowserCopyToHdRequestIf--------------------
//-------------------------------------------------------------

cMenuBrowserCopyToHdRequestIf cMenuBrowserCopyToHdRequestIf::instance;

eOSState cMenuBrowserCopyToHdRequestIf::BlueKey(cMenuFileBrowser *menu)
{
    menu->SetMode(expert);
    menu->Refresh();
    return osContinue;
}

void cMenuBrowserCopyToHdRequestIf::SetHelp(cMenuFileBrowser *menu)
{
    menu->setHelp.SetRed(trNOOP("Cancel"));
    menu->setHelp.SetGreen(trNOOP("Insert"));
    menu->setHelp.SetYellow(trNOOP("Create dir"));
    menu->setHelp.SetBlue(trNOOP("Expert mode"));
    menu->setHelp.FlushButtons(false);
}

void cMenuBrowserCopyToHdRequestIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    SetHelp(menu);
    menu->SetTitleString(menu->GetTitleString());
    if(lastIf == cMenuBrowserAutoStartIf::Instance())
    {
        menu->ChangeDir(MEDIA_DIR);
    }
    menu->Display();
    Skins.Message(mtInfo, tr("Paste selected entries"));
}

//-------------------------------------------------------------
//----------------cMenuBrowserMoveToHdRequestIf--------------------
//-------------------------------------------------------------

cMenuBrowserMoveToHdRequestIf cMenuBrowserMoveToHdRequestIf::instance;

void cMenuBrowserMoveToHdRequestIf::SetMessage()
{
    Skins.Message(mtInfo, tr("Selection is being moved in background..."), 3); // Moving
}

//----------------------------------------------------------------
//----------------cMenuBrowserStandardIf--------------------------
//----------------------------------------------------------------

cMenuBrowserStandardIf cMenuBrowserStandardIf::instance;

eOSState cMenuBrowserStandardIf::OkKey(cMenuFileBrowser *menu)
{
    //printf("--------cMenuBrowserStandardIf::OkKey----------\n");
    cMenuDirEntryItem *currentItem = menu->GetCurrentDirEntryItem();
    bool playlistEntered = false;
    eOSState state = osContinue;
    if(currentItem && currentItem->IsDir() && !menu->IsMarked(currentItem))
    {
        state = menu->OpenCurrent(false);
    }
    else if(menu->NothingIsMarked() && currentItem && currentItem->IsFile())
    {
        state = menu->OpenCurrent(false);
    }
    else
    {
        state =  menu->OpenMarked(playlistEntered);
    }
    if(playlistEntered)
    {
        menu->SetMode(browseplaylist);
    }
    return state;
}

eOSState cMenuBrowserStandardIf::RedKey(cMenuFileBrowser *menu)
{
     menu->MarkUnmarkCurrentItem();
     SetHelp(menu);
     return osContinue;
}

eOSState cMenuBrowserStandardIf::GreenKey(cMenuFileBrowser *menu)
{
    menu->MarkUnmarkAllFiles();
    SetHelp(menu);
    return osContinue;
}

eOSState cMenuBrowserStandardIf::YellowKey(cMenuFileBrowser *menu)
{
   //Insert entries in playlist
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    bool success = menu->InsertMarkedEntriesInPlayList();
#else
    bool success = menu->InsertMarkedOrCurrentInPlayList();
#endif
    if(!success)
    {
        Skins.Message(mtError, tr("Too many entries for playlist!"));
    }
    return menu->EnterPlayList();
}

eOSState cMenuBrowserStandardIf::BlueKey(cMenuFileBrowser *menu)
{
     //enter command mode
#ifdef USE_KEYS_FOR_COMMANDS
     menu->userIf_ = cMenuBrowserCommandIf::Instance();
#else
     menu->userIf_ = cMenuBrowserCmdOptionsIf::Instance();
#endif
     menu->userIf_->EnterState(menu, this);
     return osContinue;
}

void cMenuBrowserStandardIf::SetHelp(cMenuFileBrowser *menu)
{
    SetHelpMarkUnmark(menu);
    menu->setHelp.SetYellow(trNOOP("Playlist"));
    menu->setHelp.SetBlue(trNOOP("Functions"));
    menu->setHelp.FlushButtons(false);
}

void cMenuBrowserStandardIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    ChangeIfType(if_standard);
    menu->SetTitleString(menu->GetTitleString());
    SetHelp(menu);
    menu->Display();
}

void cMenuBrowserStandardIf::SetHelpMarkUnmark(cMenuFileBrowser *menu)
{
    if(menu->NoFiles())
    {
	    menu->setHelp.SetGreen("");
    }
    else if(menu->AllFilesAreMarked())
    {
	    menu->setHelp.SetGreen(trNOOP("Deselect all"));
    }
    else
    {
	    menu->setHelp.SetGreen(trNOOP("Select all"));
    }

    cMenuBrowserItem *item = menu->GetCurrentItem();
    if(item && menu->IsMarked(item))
    {
	    menu->setHelp.SetRed(trNOOP("Deselect"));
    }
    else
    {
	    menu->setHelp.SetRed(trNOOP("Select"));
    }
}

void cMenuBrowserStandardIf::SetMode(cMenuFileBrowser *menu, browseMode mode, bool force)
{
    if(mode == restricted)
    {
        menu->userIf_ = cMenuBrowserRestrictedIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == browseplaylist)
    {
        menu->userIf_ = cMenuBrowserBrowsePlayListIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == expert && force)
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
}

//----------------------------------------------------------------
//----------------cMenuBrowserRestrictedIf--------------------------
//----------------------------------------------------------------

cMenuBrowserRestrictedIf cMenuBrowserRestrictedIf::instance;

eOSState cMenuBrowserRestrictedIf::OkKey(cMenuFileBrowser *menu)
{
   // printf("--------cMenuBrowserRestrictedIf::OkKey----------\n");
    cMenuDirEntryItem *currentItem = menu->GetCurrentDirEntryItem();
    bool playlistEntered = false;
    eOSState state = osContinue;
    if(currentItem && currentItem->IsDir() && !menu->IsMarked(currentItem))
    {
        //state = menu->OpenCurrent(false);
        state = menu->PlayCurrent();
    }
    else if(menu->NothingIsMarked() && currentItem && currentItem->IsFile())
    {
        state = menu->OpenCurrent(false);
    }
    else
    {
        state =  menu->OpenMarked(playlistEntered);
    }
    if(playlistEntered)
    {
        menu->SetMode(browseplaylist);
    }
    return state;
}

eOSState cMenuBrowserRestrictedIf::YellowKey(cMenuFileBrowser *menu)
{
    return osContinue;
}

void cMenuBrowserRestrictedIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    ChangeIfType(if_restricted);
    menu->SetTitleString(menu->GetTitleString());
    SetHelp(menu);
    menu->Display();
}

void cMenuBrowserRestrictedIf::SetHelp(cMenuFileBrowser *menu)
{
    SetHelpMarkUnmark(menu);
    menu->setHelp.SetYellow("");
    menu->setHelp.SetBlue(trNOOP("Functions"));
    menu->setHelp.FlushButtons(false);
}

void cMenuBrowserRestrictedIf::SetMode(cMenuFileBrowser *menu, browseMode mode, bool force)
{
    if(mode == expert)
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == browseplaylist)
    {
        menu->userIf_ = cMenuBrowserBrowsePlayListIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == restricted && force)
    {
        menu->userIf_ = cMenuBrowserRestrictedIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
}

//----------------------------------------------------------------
//----------------cMenuBrowserBrowsePlaylistIf--------------------
//----------------------------------------------------------------

cMenuBrowserBrowsePlayListIf cMenuBrowserBrowsePlayListIf::instance;

eOSState cMenuBrowserBrowsePlayListIf::OkKey(cMenuFileBrowser *menu)
{
    cMenuDirEntryItem *currentItem = menu->GetCurrentDirEntryItem();
    bool playlistEntered = false;
    eOSState state = osContinue;
    if(currentItem && currentItem->IsDir() && !menu->IsMarked(currentItem))
    {
        return menu->OpenCurrent(false);
    }
    else if(menu->NothingIsMarked() && currentItem && currentItem->IsFile())
    {
        menu->InsertCurrentInPlayList();
        if(GetCaller() == if_xine)
        {
            state = menu->OpenCurrent(true);
        }
        else if(GetCaller() == if_playlistmenu)
        {
            //go back to playlistmenu
            state = menu->EnterPlayList();
        }
    }
    else
    {
        if(GetCaller() == if_xine)
        {
            //go back to xine
            state =  menu->OpenMarked(playlistEntered);
        }
        else if(GetCaller() == if_playlistmenu)
        {
            //go back to playlistmenu
            menu->InsertMarkedEntriesInPlayList();
            state = menu->EnterPlayList();
        }
    }
    return state;
}

eOSState cMenuBrowserBrowsePlayListIf::BackKey(cMenuFileBrowser *menu)
{
    eOSState state = menu->ExitDir();
    if(state == osBack && !menu->PlayListIsEmpty()) //exit in browseplaylist mode?
    {
        menu->userIf_ = cMenuBrowserPlaylistRequestIf::Instance();
        menu->userIf_->EnterState(menu, this, osBack); //shutdown browser after request
        return osContinue;
    }
    return state;
}

eOSState cMenuBrowserBrowsePlayListIf::YellowKey(cMenuFileBrowser *menu)
{
    if(GetCaller() == if_xine)
    {
       //go back to xine
       return menu->ShowPlayListWithXine();
    }
    else if(GetCaller() == if_playlistmenu)
    {
       //go back to playlistmenu
       return menu->EnterPlayList();
    }
    return osContinue;
}

void cMenuBrowserBrowsePlayListIf::SetHelp(cMenuFileBrowser *menu)
{
    SetHelpMarkUnmark(menu);
    if(GetCaller() == if_xine)
    {
        menu->setHelp.SetYellow(trNOOP("Back"));
    }
    else if(GetCaller() == if_playlistmenu)
    {
        menu->setHelp.SetYellow(trNOOP("Back"));
    }
    menu->setHelp.SetBlue("");
    menu->setHelp.FlushButtons(false);
}

void cMenuBrowserBrowsePlayListIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    //cUserIfBase::PrintIfTypes();
    ChangeIfType(if_playlistbrowser);
    menu->SetTitleString(menu->GetTitleString());
    SetHelp(menu);
    menu->Display();
}

void cMenuBrowserBrowsePlayListIf::SetMode(cMenuFileBrowser *menu, browseMode mode, bool force)
{
    if(mode == restricted)
    {
        menu->userIf_ = cMenuBrowserRestrictedIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == expert)
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
    else if(mode == browseplaylist && force)
    {
        menu->userIf_ = cMenuBrowserBrowsePlayListIf::Instance();
        menu->userIf_->EnterState(menu, this);
    }
}

//----------------------------------------------------------------
//----------------cMenuBrowserAutoStartIf--------------------------
//----------------------------------------------------------------

cMenuBrowserAutoStartIf cMenuBrowserAutoStartIf::instance;

eOSState cMenuBrowserAutoStartIf::YellowKey(cMenuFileBrowser *menu)
{
    menu->CopyMarkedOrCurrent();
    menu->userIf_ = cMenuBrowserCopyToHdRequestIf::Instance();
    menu->userIf_->EnterState(menu, this);
    return osContinue;
}

void cMenuBrowserAutoStartIf::SetHelp(cMenuFileBrowser *menu)
{
    SetHelpMarkUnmark(menu);
    menu->setHelp.SetYellow(trNOOP("Copy"));
    menu->setHelp.SetBlue(trNOOP("Functions"));
    menu->setHelp.FlushButtons(false);
}

//----------------------------------------------------------------
//----------------cMenuBrowserOptionsIf---------------------------
//----------------------------------------------------------------

cMenuBrowserOptionsIf cMenuBrowserOptionsIf::instance;

eOSState cMenuBrowserOptionsIf::OkKey(cMenuFileBrowser *menu)
{
    std::string option = menu->Get(menu->Current())->Text();
    menu->Hide(false);
    return ExecuteOptions(menu, option);
}

eOSState cMenuBrowserOptionsIf::BackKey(cMenuFileBrowser *menu)
{
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}

void cMenuBrowserOptionsIf::EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState)
{
    lastIf_ = lastIf;
    defOSState_ = defOSState;
    CreateOptions(menu);
    menu->Hide(true);
    AddOptions(menu);
    SetHelp(menu);
    menu->Display();
}

eOSState cMenuBrowserOptionsIf::LeaveStateDefault(cMenuFileBrowser *menu)
{
    if(lastIf_)
    {
        menu->userIf_ = lastIf_;
    }
    else
    {
        menu->userIf_ = cMenuBrowserStandardIf::Instance();
    }
    menu->userIf_->EnterState(menu, this);
    return defOSState_;
}

void cMenuBrowserOptionsIf::AddOptions(cMenuFileBrowser *menu)
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
//----------------cMenuBrowserCmdOptionsIf------------------------
//----------------------------------------------------------------

cMenuBrowserCmdOptionsIf cMenuBrowserCmdOptionsIf::instance;

eOSState cMenuBrowserCmdOptionsIf::ExecuteOptions(cMenuFileBrowser *menu, std::string option)
{
    if(option.find(tr("Copy")) == 2)
    {
        return Copy(menu);
    }
    else if(option.find(tr("Move")) == 2)
    {
        return Move(menu);
    }
    else if(option.find(tr("Delete")) == 2)
    {
        return Delete(menu);
    }
    else if(option.find(tr("Rename entry")) == 2)
    {
        return Rename(menu);
    }
    else if(option.find(tr("Create directory")) == 2)
    {
        return CreateDir(menu);
    }
    else if(option.find(tr("Insert in playlist")) == 2)
    {
        return InsertInPlayList(menu);
    }
    else if(option.find(tr("Show playlist")) == 2)
    {
        return ShowPlayList(menu);
    }
#if 0
    else if(option.find(tr("Convert to UTF8")) == 2)
    {
        return ConvertToUTF8(menu);
    }
#endif
    else if(option.find(tr("Burn ISO-File")) == 2)
    {
        return BurnIso(menu);
    }
    else if(option.find(tr("Make And burn ISO-File")) == 2)
    {
        return MakeAndBurnIso(menu);
    }
    return osContinue;
}

void cMenuBrowserCmdOptionsIf::CreateOptions(cMenuFileBrowser *menu)
{
    menu->SetTitleString(tr("Filebrowser Commands"));
    options_.clear();
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    if(menu->HasMarkedDirEntryItems())
#else
    if(menu->GetCurrentDirEntryItem() && GetLast(menu->GetCurrentDirEntryItem()->GetPath()) != ".." || menu->HasMarkedDirEntryItems())
#endif
    {
        options_.push_back(tr("Delete"));
        options_.push_back(tr("Copy"));
        options_.push_back(tr("Move"));
    }
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    if(menu->GetSingleMarkedDirEntryItem())
#else
    if(menu->GetSingleMarkedDirEntryItem()
    || menu->GetCurrentDirEntryItem() && GetLast(menu->GetCurrentDirEntryItem()->GetPath()) != ".." && menu->NothingIsMarked())
#endif
    {
        options_.push_back(tr("Rename entry"));
    }
    options_.push_back(tr("Create directory"));
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    if(menu->HasMarkedDirEntryItems())
#else
    if(menu->GetCurrentDirEntryItem() && GetLast(menu->GetCurrentDirEntryItem()->GetPath()) != ".." || menu->HasMarkedDirEntryItems())
#endif
    {
        options_.push_back(tr("Insert in playlist"));
    }
    if(!menu->PlayListIsEmpty())
    {
        options_.push_back(tr("Show playlist"));
    }
#if 0
    options_.push_back(tr("Convert to UTF8"));
#endif
    if(menu->GetSingleMarkedFileItem() && menu->GetSingleMarkedFileItem()->GetFileType().GetType() == tp_iso)
    {
        options_.push_back(tr("Burn ISO-File"));
    }
    else if(!menu->NothingIsMarked())
    {
        ;//options_.push_back(tr("Make and burn ISO-File"));
    }
}

eOSState cMenuBrowserCmdOptionsIf::Copy(cMenuFileBrowser *menu)
{
    //Copy entries
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    menu->CopyMarkedEntries();
#else
    menu->CopyMarkedOrCurrent();
#endif
    if(menu->GetMode() == autostart)
    {
        menu->userIf_ = cMenuBrowserCopyToHdRequestIf::Instance();
    }
    else
    {
        menu->userIf_ = cMenuBrowserCopyRequestIf::Instance();
    }
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::Move(cMenuFileBrowser *menu)
{
    //Copy entries (for move)
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    menu->CopyMarkedEntries();
#else
    menu->CopyMarkedOrCurrent();
#endif
    if(menu->GetMode() == autostart)
    {
        menu->userIf_ = cMenuBrowserMoveToHdRequestIf::Instance();
    }
    else
    {
        menu->userIf_ = cMenuBrowserMoveRequestIf::Instance();
    }
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::Delete(cMenuFileBrowser *menu)
{
    //Delete entries
    menu->userIf_ = cMenuBrowserDeleteRequestIf::Instance();
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::Rename(cMenuFileBrowser *menu)
{
    //Rename Entry
    menu->userIf_ = cMenuBrowserRenameEntryIf::Instance();
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::CreateDir(cMenuFileBrowser *menu)
{
    //Create Directory
    menu->userIf_ = cMenuBrowserCreateDirIf::Instance();
    menu->userIf_->EnterState(menu, lastIf_);
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::InsertInPlayList(cMenuFileBrowser *menu)
{
    //Insert entries in playlist
#ifdef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    bool success = menu->InsertMarkedEntriesInPlayList();
#else
    bool success = menu->InsertMarkedOrCurrentInPlayList();
#endif
    if(!success)
    {
        Skins.Message(mtError, tr("Too many entries for playlist!"));
    }
    LeaveStateDefault(menu);
    return menu->EnterPlayList();
}

eOSState cMenuBrowserCmdOptionsIf::ShowPlayList(cMenuFileBrowser *menu)
{
    //Open playlist without inserting items
    LeaveStateDefault(menu);
    return menu->EnterPlayList();
}

eOSState cMenuBrowserCmdOptionsIf::ConvertToUTF8(cMenuFileBrowser *menu)
{
    Skins.Message(mtInfo, tr("Started converting"));
    menu->ConvertToUTF8();
    Skins.Message(mtInfo, tr("Finished converting"));
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::BurnIso(cMenuFileBrowser *menu)
{
    menu->BurnIso();
    return osContinue;
}

eOSState cMenuBrowserCmdOptionsIf::MakeAndBurnIso(cMenuFileBrowser *menu)
{
    menu->MakeAndBurnIso();
    return osEnd;
}

//----------------------------------------------------------------
//----------------cMenuBrowserShutdownOptionsIf-------------------
//----------------------------------------------------------------

cMenuBrowserPlaylistOptionsIf cMenuBrowserPlaylistOptionsIf::instance;

eOSState cMenuBrowserPlaylistOptionsIf::BackKey(cMenuFileBrowser *menu)
{
    menu->Hide(false);
    return DiscardPlayList(menu);
}

eOSState cMenuBrowserPlaylistOptionsIf::ExecuteOptions(cMenuFileBrowser *menu, std::string option)
{
    if(option.find(tr("Save playlist to disk")) == 2)
    {
        return SavePlayList(menu);
    }
    else if(option.find(tr("Copy playlist to usb stick")) == 2)
    {
        return CopyPlayListToUSB(menu);
    }
    else if(option.find(tr("Copy playlist to CD")) == 2)
    {
        return CopyPlayListToDVD(menu);
    }
    return osContinue;
}

void cMenuBrowserPlaylistOptionsIf::CreateOptions(cMenuFileBrowser *menu)
{
    menu->SetTitleString(tr("Playlist actions"));
    options_.clear();
    {
        options_.push_back(tr("Save playlist to disk"));
        //options_.push_back(tr("Copy playlist to usb stick"));
        options_.push_back(tr("Copy playlist to CD"));
    }
}

eOSState cMenuBrowserPlaylistOptionsIf::KeepPlayList(cMenuFileBrowser *menu)
{
    return LeaveStateDefault(menu);
}

eOSState cMenuBrowserPlaylistOptionsIf::DiscardPlayList(cMenuFileBrowser *menu)
{
    menu->ClearPlayList();
    return LeaveStateDefault(menu);
}

eOSState cMenuBrowserPlaylistOptionsIf::SavePlayList(cMenuFileBrowser *menu)
{
    std::string path;
    menu->SavePlayList(path);
    return LeaveStateDefault(menu);
}

eOSState cMenuBrowserPlaylistOptionsIf::CopyPlayListToUSB(cMenuFileBrowser *menu)
{
    menu->CopyPlayListToUsb();
    return LeaveStateDefault(menu);
}

eOSState cMenuBrowserPlaylistOptionsIf::CopyPlayListToDVD(cMenuFileBrowser *menu)
{
    menu->CopyPlayListToDvd();
    return osContinue;;
}

//----------------------------------------------------------------
//----------------cMenuBrowserCreateDirIf-------------------------
//----------------------------------------------------------------

cMenuBrowserCreateDirIf cMenuBrowserCreateDirIf::instance;

eOSState cMenuBrowserCreateDirIf::OkKey(cMenuFileBrowser *menu)
{
    if(!CreateDir(menu))
    {
        Skins.Message(mtInfo, tr("Directory already exists"));
    }
    menu->SetCols(2,2,2);
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}

void cMenuBrowserCreateDirIf::CreateOptions(cMenuFileBrowser *menu)
{
    menu->SetTitleString(tr("Create directory"));
    const char *const defEntryName = trNOOP("New");
    strcpy(name_, tr(defEntryName));
}

void cMenuBrowserCreateDirIf::AddOptions(cMenuFileBrowser *menu)
{
    menu->SetCols(15);
    menu->Add(new cOsdItem(tr("To create a new directory press 'OK'"), osUnknown, false));
    menu->Add(new cOsdItem("", osUnknown, false));
    menu->Add(new cMenuBrowserEditStrItem(menu,tr("Directory Name"), name_, sizeof(name_), trVDR(FileNameChars)));
}

bool cMenuBrowserCreateDirIf::CreateDir(cMenuFileBrowser *menu)
{
    return menu->CreateDirectory(name_);
}

eOSState cMenuBrowserCreateDirIf::BackKey(cMenuFileBrowser *menu)
{
    menu->SetCols(2,2,2);
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}

//----------------------------------------------------------------
//----------------cMenuBrowserRenameEntryIf-----------------------
//----------------------------------------------------------------

cMenuBrowserRenameEntryIf cMenuBrowserRenameEntryIf::instance;

eOSState cMenuBrowserRenameEntryIf::OkKey(cMenuFileBrowser *menu)
{
    if(!RenameEntry(menu))
    {
        Skins.Message(mtInfo, tr("Entry already exists"));
    }
    menu->SetCols(2,2,2);
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}

void cMenuBrowserRenameEntryIf::CreateOptions(cMenuFileBrowser *menu)
{
    menu->SetTitleString(tr("Rename entry"));
    if(menu->GetSingleMarkedDirEntryItem())
    {
        oldname_ = GetLast(menu->GetSingleMarkedDirEntryItem()->GetPath()).c_str();
    }
#ifndef APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES
    else if(menu->GetCurrentDirEntryItem())
    {
        oldname_ = GetLast(menu->GetCurrentDirEntryItem()->GetPath()).c_str();
    }
#endif
    strcpy(name_, oldname_.c_str());
}

void cMenuBrowserRenameEntryIf::AddOptions(cMenuFileBrowser *menu)
{
    menu->SetCols(15);
    menu->Add(new cOsdItem(tr("Change the name and then press 'OK'"), osUnknown, false));
    menu->Add(new cOsdItem("", osUnknown, false));

    menu->Add(new cMenuBrowserEditStrItem(menu,tr("Rename to"), name_, sizeof(name_), trVDR(FileNameChars)));
}

bool cMenuBrowserRenameEntryIf::RenameEntry(cMenuFileBrowser *menu)
{
    return menu->RenameEntry(oldname_, name_);
}

eOSState cMenuBrowserRenameEntryIf::BackKey(cMenuFileBrowser *menu)
{
    menu->SetCols(2,2,2);
    menu->Hide(false);
    LeaveStateDefault(menu);
    return osContinue;
}
