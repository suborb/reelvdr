#include "cache.h"
#include "classify.h"
#include "RecordingsReplayControl.h"

#include "CacheTools.h"
#include "CacheItems.h"

#include <vdr/recording.h>
#include <vdr/plugin.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include <vdr/tools.h>

#include <sstream>
#include <iomanip>



cCache::cCache(){}

void cCache::ClearCache()
{
    std::list<cCacheItem*>::iterator it = cacheList_.begin();

    // holds the cCacheItems temp
    std::vector<cCacheItem*> item_vec;

    // delete each cCacheItem*, ptr remains
    for ( ; it != cacheList_.end(); ++it)
        item_vec.push_back(*it);
    
    // remove all pointers from list
    cacheList_.clear();

    cCacheItem *citem;
    // delete each cCacheItem
    for(  unsigned int i = 0; i<item_vec.size(); ++i)
    {
        citem = item_vec.at(i);
        delete citem;
    }
        

    SetHasChanged();
}

cCache::~cCache()
{
    //delete each item and clear list 
}


std::list<std::string> cCache::FullMetaDataList(const int metadata_type)
{
    std::list<cCacheItem*>::iterator it = cacheList_.begin();
    std::list <std::string> metadata_list;

    std::string metadataStr; // tmp string

    for ( ; it != cacheList_.end(); ++it)
    {
        metadataStr = (*it)->MetaData(metadata_type);
        if (!metadataStr.empty()) metadata_list.push_back(metadataStr);
    }

    // unique
    metadata_list.sort(StringCaseCmp);
    //metadata_list.sort();
    metadata_list.unique(StringCaseEqual); // remove duplicate entries

    return metadata_list;
}

bool cCache::AddItem (cCacheItem* item)
{
    // Check if already in cache
    // XXX find a better way, resource consuming?
    std::string full_path =  item->FullPath();
    std::list<cCacheItem*>::iterator it = cacheList_.begin();
    for ( ; it != cacheList_.end(); ++it)
        if ( (*it)->FullPath() ==  full_path ) return false; 

    cacheList_.push_back(item);
    SetHasChanged(); // list has changed
    return true;
}

bool cCache::Find(const cCacheItem* item)
{
    std::list<cCacheItem*>::iterator it = cacheList_.begin();
    for ( ; it != cacheList_.end(); ++it)
        if ( (*it) ==  item ) return true;

    return false;
}

bool cCache::RemoveItem (cCacheItem* item)
{
    cacheList_.remove(item);
    SetHasChanged(); // list has changed
    return true;
}



bool cCache::RemoveItem (const std::string& fullPath)
{
    std::list<cCacheItem*>::iterator it = cacheList_.begin();
    for ( ; it != cacheList_.end(); ++it)
        if ( (*it)->FullPath() == fullPath ) 
            {
                delete *it; // removed from all CacheLists
                SetHasChanged(); // list has changed
                return true;
            }

   return false; 
}
cCache Cache;
// list of multi-media files
cCache fileList;
// list of multimedia directories
cCache dirList;
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////



