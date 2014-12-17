#ifndef __MMFILES_CACHE_H__
#define __MMFILES_CACHE_H__

#include <string>
#include <list>

#include "stringtools.h"

class cCacheItem;

class cCache
{
    private:
        //std::list<cCacheItem*> cacheList_;

    public:
        std::list<cCacheItem*> cacheList_;

        cCache();
        ~cCache();
        bool AddItem        (cCacheItem*);
        bool RemoveItem     (cCacheItem*);

        // Search
};


class cCacheItem
{
    private:
        int             type_;

        std::string     path_;
        std::string     name_, displayName_;
        
        time_t          createdOn_;
        int             length_;
    public:
        cCacheItem(const std::string path, const std::string name, int type);
        virtual ~cCacheItem();

        virtual bool Play() const;

        inline std::string Path()const         { return path_; }
        inline std::string Name()const         { return name_; }
        inline std::string DisplayName()const  { return displayName_; }
        inline int         Type()const         { return type_; }

        virtual void SetDisplayName(std::string displayName);
};



class cFileItem : public cCacheItem
{
    private:
        // name, ide3 info etc
    public:
        cFileItem(const std::string& path, const std::string& name, int type);
        virtual ~cFileItem();
        //virtual bool Play()const ; // call xinemediaplayer ?
};



class cDirItem : public cCacheItem
{
    private:
        // name, ide3 info etc
    public:
        cDirItem(const std::string& path, const std::string& name, int type);
        virtual ~cDirItem();
        //virtual bool Play()const; // call xinemediaplayer ?
};


/* creates appropriate cCacheItem
 * and therefore gets added to the correct cache list
 **/
bool AddToCache(std::string full_path, int type);

extern cCache Cache;
// list of multi-media files
extern cCache fileList;
// list of multimedia directories
extern cCache dirList;

#endif

