#include "xineOsdFunction.h"
#include "xineOsd.h"
#include "xinemediaplayer.h"
#include <vdr/skins.h>

//#include "../filebrowser/servicestructs.h"

// items on xineOsdFunction menu's screen
#define DELETE_SELECTED             tr("Delete selected item")
#define CLEAR_PLAYLIST              tr("Clear playlist")
#define SAVE_PLAYLIST               tr("Save playlist")
#define SAVE_PLAYLIST_AS            tr("Save playlist as...")
#define PLAYLIST_TO_CD              tr("Burn playlist to CD...")
#define PLAYLIST_IN_NEW_FOLDER      tr("Copy all playlist items to new folder...")
#define SELECT_PLAYLIST             tr("Select a playlist...")


cXineOsdFunction::cXineOsdFunction(int pl_pos):
    cOsdMenu(tr("Mediaplayer Functions")),
    playlist_pos(pl_pos)
{
    Set();
}


void cXineOsdFunction::Set()
{
    Clear();

    SetHasHotkeys(true);

    Add( new cOsdItem( hk( DELETE_SELECTED)       ));
    Add( new cOsdItem( hk( CLEAR_PLAYLIST)        ));
    Add( new cOsdItem( hk( SAVE_PLAYLIST)         ));
    Add( new cOsdItem( hk( SAVE_PLAYLIST_AS)      ));
    Add( new cOsdItem( hk( PLAYLIST_TO_CD)        ));
    Add( new cOsdItem( hk( PLAYLIST_IN_NEW_FOLDER)));
    Add( new cOsdItem( hk( SELECT_PLAYLIST)       ));

    Display();

} // Set()



eOSState cXineOsdFunction::ProcessKey(eKeys Key)
{

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown)
        switch (Key) {

        case kOk: {
            bool closeMenu;
            OnOk(Get(Current())->Text(), closeMenu);

            state = closeMenu?osBack:osContinue;
        }
            break;

        default:
            break;
        } // switch

    return state;

} // ProcessKey()



void cXineOsdFunction::OnOk(const char* Text, bool& closeMenu)
{
    if (!Text) return;

    closeMenu = true;
    if (strstr(Text, DELETE_SELECTED)) {
        // delete current item from playlist, and set the "new" playlist

        Skins.Message(mtInfo, tr("Deleting playlist item..."), 1);
        bool success = Reel::XineMediaplayer::cXineOsd::DeletePlaylistItem(playlist_pos);
        
        if (success) {
        // service call to filebrowser, indicating change in playlist
            if (Reel::XineMediaplayer::cXineOsd::browserInstance == -1 ) {
                Reel::FileBrowserSetPlayListEntries data;
                data.name    = Reel::XineMediaplayer::currentPlaylistFilename;
                data.entries = Reel::XineMediaplayer::cXineOsd::playListEntries;
                data.instance = Reel::XineMediaplayer::cXineOsd::browserInstance;
                cPluginManager::CallAllServices("Filebrowser set playlist entries", &data);
            }
            
            Reel::XineMediaplayer::savedPlaylist.SetPlaylist(Reel::XineMediaplayer::cXineOsd::playListEntries);
            Skins.Message(mtInfo, tr("Playlist item deleted"), 1);
        }
        else
        Skins.Message(mtError, tr("Deleting playlist item: failed"), 1);
        
    } else if (strstr(Text, CLEAR_PLAYLIST)) {
        // similiar to above
        bool success = Reel::XineMediaplayer::cXineOsd::ClearPlaylist();
        if (success) {
        // service call to filebrowser, indicating change in playlist
             Reel::XineMediaplayer::currentPlaylistFilename.clear();
             
            if (Reel::XineMediaplayer::cXineOsd::browserInstance == -1 ) {
                Reel::FileBrowserSetPlayListEntries data;
                data.name    = Reel::XineMediaplayer::currentPlaylistFilename;
                data.entries = Reel::XineMediaplayer::cXineOsd::playListEntries;
                data.instance = Reel::XineMediaplayer::cXineOsd::browserInstance;
                cPluginManager::CallAllServices("Filebrowser set playlist entries", &data);
            }
            
            Reel::XineMediaplayer::savedPlaylist.SetPlaylist(Reel::XineMediaplayer::cXineOsd::playListEntries);
        }

    } else if (strstr(Text, SAVE_PLAYLIST)) {
        // Service call to filebrowser to save playlist
        
        if (Reel::XineMediaplayer::cXineOsd::browserInstance==-1) {
            Reel::FileBrowserSetPlayListEntries data;
            data.name    = Reel::XineMediaplayer::currentPlaylistFilename;
            data.entries = Reel::XineMediaplayer::cXineOsd::playListEntries;
            data.instance = Reel::XineMediaplayer::cXineOsd::browserInstance;
            cPluginManager::CallAllServices("Filebrowser set playlist entries", &data);
        }
        
        {
            Reel::FileBrowserSavePlaylist data;
            data.instance = Reel::XineMediaplayer::cXineOsd::browserInstance;
            data.name = Reel::XineMediaplayer::currentPlaylistFilename;
            cPluginManager::CallAllServices("Filebrowser save playlist", &data);
            
            bool changed = (Reel::XineMediaplayer::currentPlaylistFilename != data.name);
            printf("currentplaylist name changed? %d --> '%s'\n", changed,
                   data.name.c_str() );
            Reel::XineMediaplayer::currentPlaylistFilename = data.name;
        }
        //Reel::XineMediaplayer::CallFilebrowser(Reel::xine_save_playlist);
    } else if (strstr(Text, SAVE_PLAYLIST_AS)) {
        Reel::XineMediaplayer::CallFilebrowser(Reel::xine_save_playlistas);
    } else if (strstr(Text, PLAYLIST_TO_CD)) {
        // Service call to filebrowser to burn playlist to CD
        Reel::FileBrowserInstance data;
        data.instance = Reel::XineMediaplayer::cXineOsd::browserInstance;
        cPluginManager::CallAllServices("Filebrowser burn playlist", &data);
    } else if (strstr(Text, PLAYLIST_IN_NEW_FOLDER)) {
        // Service call to filebrowser to save playlist items in new folder
        Reel::XineMediaplayer::CallFilebrowser(Reel::xine_copy_playlist);
    } else if (strstr(Text, SELECT_PLAYLIST)) {
        // Service call to filebrowser to
        // open playlist folder to select a new playlist
        Reel::XineMediaplayer::CallFilebrowser(Reel::xine_select_playlist);
    } else {
        esyslog("OnOk(const char*) got %s: unknown item text", Text);
        closeMenu = false;
    }

} // OnOk()
