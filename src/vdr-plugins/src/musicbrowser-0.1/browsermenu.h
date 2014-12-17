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

#ifndef P__PLAYERMENU_H
#define P__PLAYERMENUH

#include <string>
#include <vector>

#include <vdr/osdbase.h>
#include <vdr/status.h>
#include <reel/searchapi.h>

#include "browserItems.h"
#include "browserbase.h"
#include "musicbrowser.h"
//#include "userIf.h"


class cMp3BrowserStatus;
class cBrowserBaseIf;
//class FileCache;
//class cThumbConvert;


//-----------------cVolumeStatus--------------------

class cVolumeStatus : public  cStatus
{
public:
    cVolumeStatus(cBrowserMenu* m);
    bool ClearVolume();
private:
    cBrowserMenu *menu;
    bool volumeSet;
    cTimeMs volumeTimeout;
protected:
    void SetVolume(int Volume, bool Absolute);
};



extern std::string dbPath; // to be set by Plugin's ProcessArgs()
//----------cBrowserMenu-----------------------

class cBrowserMenu : public cBrowserBase
{
    friend class cVolumeStatus;
public:
    cBrowserMenu(std::string title, cMp3BrowserStatus *newstatus);
    ~cBrowserMenu();
  
    /*override*/  eOSState ProcessKey(eKeys Key); 

    //functions are called from menuitems
    // eOSState ShowThumbnail(cPlayListItem item, bool hide, bool currentChanged) const;
    //eOSState ShowID3TagInfo(cPlayListItem item, bool hide, bool currentChanched);
    
    eOSState ShowFileWithXine(const cPlayListItem &item, bool asPlaylist = false, bool passInstance = true, bool showImmediately = false);
    
    eOSState StandardKeyHandling(eKeys Key);
    void SetItems(std::vector<cOsdItem*> osdItems);
    void SetColumns(int c0, int c1=0, int c2=0, int c3=0, int c4=0);
    void SetIf(cBrowserBaseIf *userIf, cBrowserBaseIf *lastIf);
    void SetIf(cBrowserBaseIf *userIf);
    void ClearOsd(){ Clear(); }
    void SetTitleString(std::string title) { title = std::string(tr(MAINMENUENTRY)) + ": " + title; SetTitle(title.c_str()); }
    const char *Hk(const char *s) { return hk(s); } //make hotkeys available for user ifs
    void Display();
    void DrawSideNote(cStringList *list);
    cPlayList *GetPlaylist() { return &playlist_; }
    cPlayList *GetFavlist() { return &favlist_; }
    cPlayList *GetLivelist() { return &livelist_; }
    void LoadLivelist();

protected:
    cVolumeStatus volStatus;
    void  ClearVolume();
    //void SetStandardCols();

private:

    cMp3BrowserStatus *status;

    static cPlayList playlist_;
    static cPlayList favlist_;
    cPlayList livelist_;
    
    cSearchDatabase search_;


    //forbid copying
    cBrowserMenu(const cBrowserMenu &);
    cBrowserMenu &operator=(const cBrowserMenu &);

    //void InsertItemsInConversionList(cImageConvert *convert) const;
    //void RemoveItemsFromConversionList(cImageConvert *convert) const;
    //void LoadPlayList(cPlayListItem item);
    //void MarkFlagTagItems();
};

#endif //P__PLAYERMENU_H
