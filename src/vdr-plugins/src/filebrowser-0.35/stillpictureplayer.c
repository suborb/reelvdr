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

#include <vector>

#include "vdr/remote.h"

#include "stringtools.h"
#include "filecache.h"
#include "playlist.h"
#include "setup.h"
#include "servicestructs.h"

#include "stillpictureplayer.h" 

//---------cStillPicturePlayer----------------------------------------

cStillPicturePlayer::cStillPicturePlayer(int total)
:cPlayer(pmVideoOnly), total_(total), current_(0)
{
}

void cStillPicturePlayer::ShowPicture(unsigned char *Image, int Len, int current)
{
    current_ = current;
    DeviceStillPicture(Image, Len);
}

bool cStillPicturePlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame) 
{ 
    Current = current_ + 1;
    Total = total_;
    //printf("-----------------cStillPicturePlayer::GetIndex, Current = %d, Total = %d---------------\n", Current, Total);
    return true; 
}

//----------------------cStillPictureControl--------------------------
 

cStillPictureControl::cStillPictureControl(cPlayList *Playlist, cFileCache *Cache, cImageConvert *Convert, int browserInstance)
:cControl(new cStillPicturePlayer(Playlist->Size()), false), playlist_(Playlist), convert_(Convert), cache_(Cache), browserInstance_(browserInstance),
state_(plInvalid), msg_(""), slideShow_(fileBrowserSetup.slideShow), normalExit_(false)
{
    player_ = static_cast<cStillPicturePlayer*>(player);
    UpdateImage();
    cStatus::MsgReplaying(this, NULL, NULL, true);
    std::string msg = std::string("[image] ") + GetLast(playlist_->GetCurrent().GetPath());
    cStatus::MsgReplaying(this, msg.c_str(),NULL , true);
}

cStillPictureControl::~cStillPictureControl()
{ 
    Shutdown();
}

void cStillPictureControl::Shutdown()
{   
    ::cStatus::MsgReplaying(this, NULL, NULL, false);
    delete player_;
    if(convert_)
    {
        convert_->ClearConvertedFiles();
    }

    //go back to filebrowser
    if(normalExit_)
    {
        CallFilebrowser();
    }
}              

eOSState cStillPictureControl::ProcessKey(eKeys Key)
{
    UpdateShow();

    eOSState state = osUnknown;

    state = ProcessKey2(Key);

    if(state == osUnknown && slideShow_ && timer_.TimedOut())
    {
        state = ShowNext(true, fileBrowserSetup.cyclic, fileBrowserSetup.random);
    }

    if (UpdateMessage())
    {
        state = ShowMessage();
    }

    if(state == osBack)
    {
        normalExit_ = true;
        return osBack;
    }
    else
    {
        return osUser1; //never handle this call in supermenus        //????
    }
}    

eOSState cStillPictureControl::ShowMessage()
{
    eKeys key = Skins.Message(mtInfo,msg_.c_str());
    return ProcessKey2(key);
}

eOSState cStillPictureControl::ProcessKey2(eKeys Key)
{
    eOSState state = osUnknown ;
    switch (Key)
    {
        case kBack:
            return osBack;
        case kUp:
        case kRight:
            state = ShowNext(true, fileBrowserSetup.cyclic);
            break;
        case kDown:
        case kLeft:
            state = ShowNext(false, fileBrowserSetup.cyclic);
            break;
        case kPlay:
            ToggleSlideShow(true);
            state = osContinue;
            break;
        case kStop:
        case kPause:
            ToggleSlideShow(false);
            state = osContinue;
            break;
        case kRed:
            ToggleSlideShowMode();
            state = osContinue;
        default:
            break;
    }
    return state;
}

void cStillPictureControl::Show()
{
    if(player_)
    {
        player_->ShowPicture(&(convertedFile_.buffer[0]), convertedFile_.buffer.size(), playlist_->GetPos(playlist_->GetCurrent())); 
        std::string msg = std::string("[image] ") + GetLast(playlist_->GetCurrent().GetPath());
        cStatus::MsgReplaying(this, msg.c_str(),NULL , true);
    }
    timer_.Start();
    state_ = (state_ == plLast) ? plLast : plValid;
}

void cStillPictureControl::UpdateShow()
{
#ifdef RBMINI
    if(state_ != plValid && state_ != plLast)
#endif
    if(state_ != plValid)
    {
        if(cache_ && cache_->FetchFromCache(playlist_->GetCurrent().GetPath(), convertedFile_.buffer))
        { 
            Show();
        }
        else if(convert_ && convert_->GetConvertedFile(playlist_->GetCurrent().GetPath(), convertedFile_))
        { 
            if(convertedFile_.valid)
            {
                Show();
            }
            else
            { 
                state_ = plError;
                timer_.Start(1);
            }
        }
    }
}

eOSState cStillPictureControl::ShowNext(bool forward, bool cyclic, bool random)
{
    bool next = true;
    if((state_ == plValid || state_ == plError || state_ == plLast) && !playlist_->IsEmpty())
    {
        if(random)
        {
            playlist_->NextRandom();
        }
        else if(cyclic)
        {
            playlist_->NextCyclic(forward);
        }
        else
        {
            next = playlist_->Next(forward);
        }

        if(next && UpdateImage())
        { 
            state_ = plValid;
            Show();
        }
    }
    if(!next) 
    {
       state_ = plLast;
       UpdateMessage();
       return ShowMessage();
    }
    return osContinue;
}

bool cStillPictureControl::UpdateMessage()
{
    std::string msg = "";
    if(state_ == plValid && fileBrowserSetup.displayFileNames)
    { 
        msg = GetLast(playlist_->GetCurrent().GetPath());
    } 
    else if(state_ == plInvalid && fileBrowserSetup.displayFileNames)
    {
        msg = GetLast(playlist_->GetCurrent().GetPath()) + tr(" ...opening file");
    } 
    else if (state_ == plError)
    {
        msg = std::string(tr("Cannot open file:")) + " " + GetLast(playlist_->GetCurrent().GetPath());
    }    
    else if (state_ == plLast)
    {
        msg = std::string(tr("No more images"));
    }
    if(msg_ != msg)
    {
        msg_= msg;
        if(msg_ != "")
        {
            return true;
        }
    }
    return false;
}

bool cStillPictureControl::UpdateImage()
{
    if(!cache_ || !cache_->FetchFromCache(playlist_->GetCurrent().GetPath(), convertedFile_.buffer))
    {
        if(convert_)
        {
            StartConversion(); 
            state_ = plInvalid;
        }
        else
        {
            state_ = plError; 
            timer_.Start(1);
        }
        return false;
    }
    return true;
}

void cStillPictureControl::StartConversion()
{
    convert_->InsertInConversionList(playlist_->GetCurrent().GetPath(), true);
    convert_->StartConversion();
}

/*void cStillPictureControl::StartPlayer()
{
    player_ = new cStillPicturePlayer;
    cControl *control = new cStillPictureControl(player_, false); //will not receive kPause key when hidden. Other problems?
    cControl::Launch(control);
}*/

void cStillPictureControl::ToggleSlideShow(bool on)
{
    if(!slideShow_)
    {
        {
            ShowNext(true, fileBrowserSetup.cyclic);
            slideShow_ = true;
            Skins.Message(mtInfo,tr("Slide show started")); //TODO: find out if this call really crashes remote in scarce situations
        }
    }
    else
    {
        if(slideShow_)
        {
            slideShow_ = false; 
            Skins.Message(mtInfo,tr("Slide show halted"));
        }
    }
}

void cStillPictureControl::ToggleSlideShowMode()
{
    if(fileBrowserSetup.random)
    {
        fileBrowserSetup.random = false;
        Skins.Message(mtInfo,tr("Standard play mode")); 
    }
    else
    {
        fileBrowserSetup.random = true;
        Skins.Message(mtInfo,tr("Random play mode")); 
    }
}

void cStillPictureControl::CallFilebrowser()
{ 
    //printf("cStillPictureControl: CallFilebrowser, browserInstance = %d\n", browserInstance_);        
    FileBrowserInstance instanceData = {
                                            browserInstance_,
                                            true
                                       };
    cPluginManager::CallAllServices("Filebrowser set instance", &instanceData);

    cRemote::CallPlugin("filebrowser");
}
