#ifndef PLAYLIST_MENU_H
#define PLAYLIST_MENU_H

#include <vdr/osdbase.h>
#include <string>

#include "Playlist.h"
#include "search.h"

#include "CacheItems.h"
#include "filesMenu.h" // for cFileOsdItem

/* 
 * Display cPlaylist's object CurrentPlaylist
 * Allow for edit/clear/add
 */
class cPlaylistMenu : public cOsdMenu
{
    private:
        cPlaylist* plPtr;
        int currentElt;

    public:
        cPlaylistMenu(cPlaylist *pl = &CurrentPlaylist);
        void ShowPlaylist();
        void SetHelpButtons();
        void SetCurrentElt();

        eOSState ProcessKey(eKeys key);
};

#endif

