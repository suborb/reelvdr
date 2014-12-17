#ifndef PLAYLIST_TOOLS_H
#define PLAYLIST_TOOLS_H

#include "CacheItems.h"
#include "Playlist.h"

#include <string>
#include <vector>

// Adds if item not in Playlist; if already in Playlist, removes it
bool TogglePlaylistItem(cCacheItem* item);

// calls xinemediaplayer plugin with playlist got from cPlaylist (CurrentPlaylist)
bool CallXineWithPlaylist();

// converts cCacheItem* playlist to std::string playlist understandable to xinemediaplayer
std::vector<std::string> Playlist2XinePlaylist(cPlaylist& pl);


#endif
