#ifndef MMFILES_PLAYLIST_H
#define MMFILES_PLAYLIST_H

#include <string>
#include <vector>

#include "CacheItems.h"


// A playlist that 
// holds cCacheItem* items
class cPlaylist
{
    private:
        std::vector<cCacheItem*> list_;
        bool hasChanged;

    public:

        cPlaylist();

        //add items to list
        void Add(cCacheItem*);


        // Remove item from list
        bool Remove(cCacheItem*);

        // Remove item at index
        bool Remove(unsigned int);

        //remove item by full path
        bool Remove(const std::string& path);

        // remove items under a directory
        bool RemoveDirTree(const std::string& dirpath);


        unsigned int Size()const;

        // return pointer to item: Can return NULL
        cCacheItem* At(unsigned int);

        // given a "item" returns its position 0...size-1; 
        // return -1 if not found
        int Find(const cCacheItem*);


        // clear playlist
        // set each Item 'not' to be a playlist item
        // CAUTION : cCacheItem* is not to be deleted
        void Clear();

        inline bool HasChanged() const { return hasChanged; }
        inline void SetChanged() {hasChanged = true;}
        inline void ResetChanged() {hasChanged = false;}
};

extern cPlaylist CurrentPlaylist;
#endif

