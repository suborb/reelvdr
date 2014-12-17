#ifndef __MMFILES_CACHE_H__
#define __MMFILES_CACHE_H__

#include <string>
#include <vector>
#include <list>

#include "Enums.h"

#include "stringtools.h"
#include <vdr/tools.h>

#include "CacheItems.h"

class cCache
{
    private:
        //std::list<cCacheItem*> cacheList_;
    bool hasChanged;
    public:
        std::list<cCacheItem*> cacheList_;

        cCache();
        ~cCache();

        inline void SetHasChanged() {   hasChanged = true; }
        inline void ResetChanged() {    hasChanged = false; } 

        inline bool HasChanged() const { return hasChanged; }

        bool AddItem        (cCacheItem*);
        bool RemoveItem     (cCacheItem*);
        bool RemoveItem     (const std::string& fullPath);

        // Search
        cCacheItem* SearchItem(const std::string& fullPath);
        bool Find(const cCacheItem*);
        std::list<std::string> FullMetaDataList(int);

        inline unsigned int size() const { return cacheList_.size(); }

        void ClearCache();
};


extern cCache Cache;

// list of multi-media files
extern cCache fileList;

// list of multimedia directories
extern cCache dirList;

#endif

