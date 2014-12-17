#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <string>
#include <vector>

using namespace std;

/// ** 
/// Playlist file : a text containing absolute paths of files
///                 one filepath per line
/// **

// read a playlist file and return a vector of paths (strings)
vector<string> ReadPlaylist(string filename);

// write the playlist vector of paths into a file
bool SavePlaylist(string filename, vector<string> playlist);


#endif /* PLAYLIST_H */
