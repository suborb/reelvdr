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


#ifndef P__STILLPICTUREPLAYER_H
#define P__STILLPICTUREPLAYER_H

#include <string>
#include <sys/time.h>

#include "filecache.h"
#include "convert.h"
#include "menuBrowserBase.h"
#include "setup.h"

class cImageConvert;
class cMenuFileItem;
class cFileCache;
class cPlayList;
class cStillPicturePlayer;

class  cStillPicturePlayer : public cPlayer
{
public:
    cStillPicturePlayer(int total);
    /*override*/ ~cStillPicturePlayer(){}
    void ShowPicture(unsigned char *Image, int Len, int current);
    /*override*/ bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);   

private:
    int total_;
    int current_;
};

class cStillPictureControl : public cControl 
{
public:
    //cStillPictureControl(cStillPicturePlayer *&player, bool hidden = false);
    /*override*/ void Hide(){} //???

    cStillPictureControl(cPlayList *playlist, cFileCache *cache, cImageConvert *convert, int browserInstance);
    /*override*/ ~cStillPictureControl();
    /*override*/ void Shutdown();

private:
    //forbid copying
    cStillPictureControl(cStillPictureControl &);
    cStillPictureControl &operator=(const cStillPictureControl &);

    /*override*/ eOSState ProcessKey(eKeys Key);
    eOSState ProcessKey2(eKeys Key);
    void Show();
    void UpdateShow();
    eOSState ShowNext(bool forward, bool cyclic, bool random = false);
    eOSState ShowMessage();
    bool UpdateMessage();
    bool UpdateImage();
    void StartConversion();
    //void StartPlayer();
    void ToggleSlideShow(bool on);
    void ToggleSlideShowMode();
    void CallFilebrowser();

    class cTimer
    {  
    public:
        cTimer()
        : oldtime_(0), timeout_(0)
        {
        }
        bool TimedOut()
        {
            gettimeofday(&time, &tz);
            uint newtime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
            if(oldtime_ && newtime - oldtime_ > timeout_)
            {
                return true;
            }
            return false;
        }
        void Start(int timeout = 0)
        {
            gettimeofday(&time, &tz);
            oldtime_ = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
            timeout_ = timeout;
            if(timeout == 0)
            {
                timeout_ = static_cast<uint>(fileBrowserSetup.switchTime) * 1000;
            }
        } 
    private:
        struct timeval time;
        struct timezone tz;
        uint oldtime_;
        uint timeout_;
    }
    timer_;

    cStillPicturePlayer *player_;
    cPlayList *playlist_;
    OutListElement convertedFile_;
    cImageConvert *convert_;
    cFileCache *cache_; 
    int browserInstance_;
    enum displayState{ plError, plInvalid, plValid, plLast }; 
    displayState state_;
    std::string msg_;
    bool slideShow_;
    bool normalExit_;
};

#endif //P__STILLPICTUREPLAYER_H



