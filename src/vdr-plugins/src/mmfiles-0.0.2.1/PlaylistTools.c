#include "Playlist.h"
#include "PlaylistTools.h"
#include "Enums.h"

#include "CacheTools.h"

#include <vdr/plugin.h>

// Adds if item not in Playlist, if already in Playlist, removes it
bool TogglePlaylistItem(cCacheItem* item)
{
    if (item)
    {
        if (!item->InPlaylist())
        {
            CurrentPlaylist.Add(item);
            return true;
        }
        else 
            return CurrentPlaylist.Remove(item);
    }
    else return false;
}

bool CallXineWithPlaylist()
{
    cPlugin *p = cPluginManager::GetPlugin("xinemediaplayer");
    if(!p) return false;

    std::vector<std::string> xine_playlist;

    xine_playlist = Playlist2XinePlaylist(CurrentPlaylist);

    return CallXinemediaplayer(xine_playlist);
}

std::vector<std::string> Playlist2XinePlaylist(cPlaylist& pl)
{
    // xine playlist
    std::vector<std::string> xplaylist;
    xplaylist.clear();
    
    std::string mrl = "";
    bool toAdd = false;

    cCacheItem *item = NULL;
    unsigned int i = 0;

    for (; i < pl.Size(); ++i)
    {
        if (item = pl.At(i)) // '=',assignment
        {
            mrl = "";
            toAdd = true;
            
            // add right MRL prefix to path
            switch(item->Type())
            {
                case eAudioFile:
                case eVideoFile:
                        //mrl = "file://";
                        break;

                case eDvdDir:
                        //mrl = "dvd://";
                        break;

                default:
                case eVdrRecDir:
                            toAdd = false;
                            break;
            } // switch
            
            if (toAdd)
            {
                mrl += item->FullPath();
                printf("%s\n", mrl.c_str());
                xplaylist.push_back(mrl);
            }

        }// if (item...)

    }// for
    
    return xplaylist;
} // end func


