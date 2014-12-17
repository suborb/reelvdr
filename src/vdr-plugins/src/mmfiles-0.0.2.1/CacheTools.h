#ifndef CACHE_TOOLS_H
#define CACHE_TOOLS_H

#include "cache.h"
#include <string>

// cPluginManager

// finds 'base' directory of this recording
// Assumming all recordings are under '.../recordings/' directory
std::string VdrRecBaseDir(const std::string& filename);

// Adds Item to cache, creates appropriate 
// cCacheItem (cFileItem / cDirItem)
bool AddToCache(std::string full_path, int type);

// Deletes the item with this path, therefore removed from every cache
bool RemoveFromCache(const std::string& full_path);

// Calls xinemediaplayer's service() with 'playlist'
bool CallXinemediaplayer(const std::vector<std::string>& playlist);

#endif
