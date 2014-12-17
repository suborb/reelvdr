#include "ExpertMenu.h"
#include "filesMenu.h"
#include "PlaylistMenu.h"

#include "search.h"
#include "cache.h"

#include <vdr/interface.h>
#include <vdr/menuitems.h>



#define SHOW_PLAYLIST_STR tr("Show Playlist")
#define EDIT_PLAYLIST_STR tr("Edit Playlist")
#define CLEAR_PLAYLIST_STR tr("Clear Playlist")

#define SEARCH_SHOW_ALL tr("Show All")
#define BROWSE_MODE_STR tr("Browse Mode")

#define LIST_ALL_STR tr("List All")
#define SHOW_DIR_STR tr("Directory")
#define SHOW_GENRE_STR tr("Genre")
#define SHOW_ARTIST_STR tr("Artist")
#define SHOW_ALBUM_STR tr("Album")



void cExpertMenu::MenuIdx2BrowseMode(int idx)
{
   switch(idx)
   {
    case 0: // list mode
        cFilesMenu::browse_mode = eNormalMode;
        break;

    case 1: // dir mode
        cFilesMenu::browse_mode = eDirBrowseMode;
        break;

    case 2: // Genre
        cFilesMenu::browse_mode = eGenreMode;
        cFilesMenu::meta_data_mode = meta_genre;
        break;
    case 3: // artist
        cFilesMenu::browse_mode = eGenreMode;
        cFilesMenu::meta_data_mode = meta_artist;
        break;
    case 4: // album
        cFilesMenu::browse_mode = eGenreMode;
        cFilesMenu::meta_data_mode = meta_album;
        break;
   }
}

int cExpertMenu::BrowseMode2MenuIdx()
{
    switch(cFilesMenu::browse_mode)
    {
        case eNormalMode: // list mode
                return 0;

        case eGenreMode:  // Genre / meta data mode
            switch(cFilesMenu::meta_data_mode)
            {
                case meta_genre: return 2; 

                case meta_artist: return 3;

                case meta_album: return 4;

                default:
                    return 0;
            }
            break;

        case eDirBrowseMode: return 1;

        default:
            return 0;
    }

    return 0;
}

cExpertMenu::cExpertMenu():cOsdMenu(tr("Expert Menu"),20)
{
    SetHasHotkeys();

    mode = BrowseMode2MenuIdx();
    ModeTexts[0] =  LIST_ALL_STR;
    ModeTexts[1] = SHOW_DIR_STR;
    ModeTexts[2] = SHOW_GENRE_STR;
    ModeTexts[3] = SHOW_ARTIST_STR;
    ModeTexts[4] = SHOW_ALBUM_STR;

    ShowOptions();
}

void cExpertMenu::ShowOptions()
{
    Clear();

    Add(new cOsdItem( hk( SHOW_PLAYLIST_STR) ));
    Add(new cOsdItem( hk( EDIT_PLAYLIST_STR) ));
    Add(new cOsdItem( hk( CLEAR_PLAYLIST_STR) ));

    Add(new cOsdItem("", osUnknown, false ));
    Add(new cOsdItem( hk( SEARCH_SHOW_ALL) ));

    Add(new cOsdItem("", osUnknown, false ));

    printf("%s %s\n",BROWSE_MODE_STR, ModeTexts[mode] );
    Add(new cMenuEditStraItem( BROWSE_MODE_STR, &mode, 5, ModeTexts ));

    Display();
    SetHelp(NULL);
}

eOSState cExpertMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if( HasSubMenu() && state == osUser2) // go back to filesMenu
        return osBack;

    if(state == osUnknown)
    {
        switch(key)
        {
            case kOk:
                {
                    cOsdItem* item = Get(Current());
                    std::string text = item->Text();

                    // Open playlist
                    if ( strstr(text.c_str(), SHOW_PLAYLIST_STR))
                        return AddSubMenu(new cPlaylistMenu);

                    // edit playlist
                    else if ( strstr(text.c_str(), EDIT_PLAYLIST_STR))
                        return AddSubMenu(new cPlaylistMenu);
                    
                    // clear playlist
                    else if ( strstr(text.c_str(), CLEAR_PLAYLIST_STR))
                    {
                        // ask a Q
                        bool confirm = Interface->Confirm(tr("Clear Playlist ?"));
                        if (confirm)
                        {
                            CurrentPlaylist.Clear();
                            cFilesMenu::RefreshMenu = true;

                            Skins.Message(mtInfo, tr("Playlist Cleared"));
                        }
                        return osContinue;
                    }
                    
                    // Show all
                    else if ( strstr(text.c_str(), SEARCH_SHOW_ALL))
                    {
                        if ( cSearchCache::Instance().CacheSize() < Cache.size())
                        {
                            cSearchCache::Instance().ResetSearchInSearch();
                            cSearchCache::Instance().ResetSearchResult();
                            cSearchCache::Instance().RunSearch();
                            //Skins.Message(mtInfo, tr("Showing all ..."));
                        }
                        return osBack;
                    }
                    else if ( strstr(text.c_str(), BROWSE_MODE_STR ) )
                    {
                        MenuIdx2BrowseMode(mode);
                        cFilesMenu::RefreshMenu = true;
                        return osBack;
                    }

                    else return osContinue; // default

                }// case kOk
                break;

            default:
                break;
        }// switch

    } // if

    return state;
}
