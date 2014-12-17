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


#include <vdr/status.h>
#include <vdr/remote.h>

#include "playerStatus.h"
#include "playermenu.h"


//----------cMediaPlayerStatus--------------------

cMediaPlayerStatus::cMediaPlayerStatus()
: /*active(false), running(false), fileBrowserStarted(false),*/ current_("")
{
}

/*void cFileBrowserStatus::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
    if(!On && active && !running)
    {
        RestartMediaPlayer();
    }
}*/

/*void cMediaPlayerStatus::RestartMediaPlayer()
{
    if (!cOsd::IsOpen())
    {
        cRemote::CallPlugin("mediaplayer");
    }
}*/

void cMediaPlayerStatus::Shutdown()
{
   SetCurrent(""); 
}

void cMediaPlayerStatus::Hide(std::string current)
{
   SetCurrent(current);
}

std::string cMediaPlayerStatus::GetCurrent()
{
    return current_;
}

void cMediaPlayerStatus::SetCurrent(std::string current)
{
   current_ = current; 
}
