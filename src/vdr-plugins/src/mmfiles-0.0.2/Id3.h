#ifndef PLUGINS_IDE3_H
#define PLUGINS_IDE3_H

#include <string>

//returns Genre
std::string Id3Genre(const std::string& Path);

// returns album name
std::string Id3Album(const std::string& Path);

// returns Artist name
std::string Id3Artist(const std::string& Path);

// returns string with Title + artist + album  
// mode = 0 (title),1 (title+artist), 2 (title+album), 3(all)
std::string Id3Info(const std::string& songPath, int mode=0 );

#endif
