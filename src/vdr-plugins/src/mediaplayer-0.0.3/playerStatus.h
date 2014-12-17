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


#ifndef P__PLAYERSTATUS_H
#define P__PLAYERSTATUS_H

#include <string>

#include <vdr/status.h>

//----------cMediaPlayerStatus-------------------- 


class cMediaPlayerStatus //: public cStatus
{
public:
    cMediaPlayerStatus();
    //virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On); 
    //void RestartMediaPlayer();
    void Shutdown();
    void Hide(std::string current);
    std::string GetCurrent();

    private:
    //bool active;
    //bool running;
    //bool fileBrowserStarted; 
    void SetCurrent(std::string current);
    std::string current_;
};

#endif //P__PLAYERSTATUS_H
