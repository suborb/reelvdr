#include "cache.h"
#include "classify.h"

cCache::cCache(){}
cCache::~cCache()
{
    //delete each item and clear list 
}
bool cCache::AddItem (cCacheItem* item)
{
    cacheList_.push_back(item);
    return true;
}
bool cCache::RemoveItem (cCacheItem* item)
{
    cacheList_.remove(item);
    return true;
}
cCache Cache;
// list of multi-media files
cCache fileList;
// list of multimedia directories
cCache dirList;
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

cCacheItem::cCacheItem(const std::string path, const std::string name, int type)
{ 
    path_ = path; name_ = name; type_ = type;

    // add to Cache list
    Cache.AddItem(this);
}

cCacheItem::~cCacheItem()
{
    // remove 'this' pointer from cache list
    Cache.RemoveItem(this);
}

bool cCacheItem::Play()const { return false; }


void cCacheItem::SetDisplayName(std::string displayName) { displayName_ = displayName; }

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


cFileItem::cFileItem(const std::string& path, const std::string& name, int type) : cCacheItem(path, name, type)
{
    // Add to filelist
    fileList.AddItem(this);
}
cFileItem::~cFileItem()
{
    // remove from file list
    fileList.RemoveItem(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


cDirItem::cDirItem(const std::string& path, const std::string& name, int type) : cCacheItem(path, name, type)
{
    // Add to filelist
    dirList.AddItem(this);
}
cDirItem::~cDirItem()
{
    // remove from file list
    dirList.RemoveItem(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
 * creates appropriate cCacheItem
 * and therefore gets added to the correct cache list
 */
bool AddToCache(std::string full_path, int type)
{

    std::string name     = GetLast(full_path);
    std::string parent   = RemoveName(full_path);

    switch( type )
    {
        case eVdrRecDir:
        case eDvdDir:
            new cDirItem(parent, name, type); // donot hold the pointer
            break;

        case eAudioFile:
        case eVideoFile:
            new cFileItem(parent, name, type);
            break;

        default:
            // uknown type
            new cCacheItem(parent, name, eUnknownFile);
            break;

    }

    return true;
}

