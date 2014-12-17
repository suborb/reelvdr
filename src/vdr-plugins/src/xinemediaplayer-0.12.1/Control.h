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

// Control.h
 
#ifndef XINE_MEDIAPLAYER_CONTROL_H_INCLUDED
#define XINE_MEDIAPLAYER_CONTROL_H_INCLUDED

#include "Reel.h"

#ifndef USE_PLAYPES
#include "XineLib.h"
#endif

#include "SpuDecode.h"
#include "xineOsdInfoMenu.h"

#include <vdr/player.h>

#include <string>
#include <vector>

namespace Reel
{
namespace XineMediaplayer
{
    class Player;

    class Control : public ::cControl /* final */
    {
    public:
        Control(char const *mrl, bool playlist, std::vector<std::string> playlistEntries) NO_THROW;
        ~Control() NO_THROW;

        /* override */ void Hide() NO_THROW;

        /* override */ eOSState ProcessKey(eKeys key) NO_THROW;

        /* override */ cOsdObject* GetInfo() ;

        bool ShowProgress(bool Initial);

    void PlayerPositionInfo(int &plNr, int &curr, int &total); 
    // show the position on the playlist AND position of the file being played
    void SetPlayList( std::vector<std::string> pl);
    bool PlayFileNumber(int n);
    void SetPlayMode(int);
    void StopPlayback();
        int GetPlayMode();
        void Show();

        bool IsPlaying();
        bool IsPlayingVideo();
        bool HasActiveVideo();
        bool IsPaused();

        bool OsdVisible(void *me);
        ///< retrurn true if "this" Control has opend an OSD which is still visible 

        bool OsdTaken(void *me);
        ///< returns true if osd is open and owned by some other object 

        bool TakeOsd(void *obj);
        ///< TakeOsd((void *) -1) takes osd in any case or returns false 
        ///< if osd not owner of obj 
 
       std::string GetDisplayHeaderLine() const;

    private:
        Control(Control const &); // No copying.
        Control &operator=(Control const &); // No assigning.

        eOSState ProcessKey2(eKeys key) THROWS;
        void Stop() NO_THROW;

        Player *player_;
        void   *osdTaker_;
        //  points to current osd owner 
        //  TODO sync with spu 

        void OsdOpen();
        void HideOsd();
        void OsdClose();
        void UpdateShow();
        //void UpdateShow(bool force = false);
        void ShowMode();

        std::string lastTitleBuffer_; // weg
         
        // osd progress display
        cSkinDisplayReplay  *displayReplay_;
        const char          *title_; // check this 
        // Progress stats XXX review stats

        int     lastCurrent_;
        int     lastTotal_;
        bool    visible_;
        bool    modeOnly_;
        bool    shown_;
        bool    lastPlay_;
        bool    lastForward_;
        int     lastSpeed_;
        bool    displayFrames_;
        //bool    needsFastResponse_;
        time_t  timeoutShow_;
        bool timeSearchActive, timeSearchHide;
        int timeSearchTime, timeSearchPos;
        void TimeSearchDisplay(void);
        void TimeSearchProcess(eKeys Key);
        void TimeSearch(void);
    };

}
}
#endif // #define XINE_MEDIAPLAYER_CONTROL_H_INCLUDED
