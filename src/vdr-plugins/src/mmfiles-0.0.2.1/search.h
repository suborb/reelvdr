
#ifndef MMFILES_SEARCH_CACHE_H
#define MMFILES_SEARCH_CACHE_H

#include <time.h>
#include <string>

#include "cache.h"
#include "classify.h"
#include "Enums.h"

// singleton class
class cSearchCache
{
    private:
        cCache searchCache_;
            // contents of this cache is showed on OSD
            
        int cacheType_; // Type of cache: Video/Audio/DVD/VdrRec ...
                        // 0: MainCache!


        int searchType_; // search for length/Date/Text
        int fileType_;  // search for file type

        // search params
        std::string searchStr_;
        std::string searchPathStr_;
        std::string searchMetaDataStr_;

        int metaDataType_;
        long searchLength_;
        time_t searchDate_;

        int sortType_;   // 0, no sort order as in main cache


        int ParamChanged_;
        bool searchInSearchCache;


        // contructor, copy constructor, operator
        cSearchCache();
        cSearchCache(const cSearchCache&);
        cSearchCache& operator=(const cSearchCache&);;


    public:
        static cSearchCache& Instance();

        inline void ResetParamChanged() { ParamChanged_ = false; }
        inline void SetParamChanged() { ParamChanged_ = true; }

        // Search inside results
        inline void SetSearchInSearch() {searchInSearchCache=true;}
        inline void ResetSearchInSearch() {searchInSearchCache=false;}
        inline bool SearchInSearch() const{ return searchInSearchCache; }


        void SetCacheType       (int);
        int CacheType           () const;

        int FileType            () const;
        void SetFileType        (int);

        void SetSearchType      (int);
        int  SearchType         ()const;

        int MetaDataType        ()const;
        std::string MetaDataStr() const;

        bool HasParamsChanged   ()const;
        bool HasCacheChanged    ()const; // true if the cache corresponding to cacheType_ has changed

        void SetSearchString    (const std::string&);
        void SetSearchLength    (const int&);
        void SetSearchDate      (const time_t&);

        // search for path
        void SetSearchPathStartsWith(const std::string& str);
        void SetSearchPath(const std::string& str);

        // search for meta data
        void SetSearchMetaData(const std::string& str, int meta_data_type);
        void SetSearchGenre(const std::string& str);


        bool doesMatchSearchParam(const cCacheItem *) const;

        // call this function after setting all search params
        void RunSearch();

        std::string GCD(); // Greated Common Directory

        cCache& SearchResult();

        void ResetSearchResult();

        // Sets the state of Cache to ummodified
        void ResetChanged();

        // sort functions
        void SortByName(bool order);
        void SortByLength(bool order);
        void SortByDate(bool order);

        std::list<std::string> MetaDataList(const int);
        inline unsigned int CacheSize()const { return searchCache_.size(); }
};


// file type to cache type
inline int FileType2CacheType(int fileType)
{
    switch(fileType)
    {
        case eAudioFile:     return eAudioCache;
        case eVideoFile:     return eVideoCache;
        case ePictureFile:   return ePictureCache;

        case eVdrRecDir:     return eVdrRecCache;
        case eDvdDir:        return eDvdCache;

        default:
                             return eMainCache;
    }
    return eMainCache;
}


#endif

