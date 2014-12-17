#include "search.h"
#include "DefaultParams.h"
#include <string.h>

/// compare functions
//
// by Name
bool CompareNameAsc( const cCacheItem* a, const cCacheItem*b)
{
    return strcasecmp(a->DisplayName().c_str(),b->DisplayName().c_str()) <= 0;
}

bool CompareNameDesc(const cCacheItem* a, const cCacheItem* b)
{
    //return a->DisplayName() < b->DisplayName();
    return strcasecmp(a->DisplayName().c_str(),b->DisplayName().c_str()) >= 0;
}


// by length
bool CompareLengthAsc( const cCacheItem* a, const cCacheItem*b)
{
    return a->Length() <= b->Length();
}

bool CompareLengthDesc(const cCacheItem* a, const cCacheItem* b)
{
    return a->Length() > b->Length();
}

// by Date
bool CompareDateAsc( const cCacheItem* a, const cCacheItem*b)
{
    return a->Date() <= b->Date();
}

bool CompareDateDesc(const cCacheItem* a, const cCacheItem* b)
{
    return a->Date() > b->Date();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

cSearchCache::cSearchCache()
{
    ResetSearchResult();
    RunSearch();
}


void cSearchCache::ResetSearchResult()
{
   cacheType_   = eMainCache;
   searchType_  = st_none;
   fileType_    = cDefaultParams::Instance().FileType();
   sortType_    = no_sort;

   SetParamChanged();
}

void cSearchCache::ResetChanged()
{
    cSearchCache::Instance().SearchResult().ResetChanged();
}

cCache& cSearchCache::SearchResult() { return searchCache_; }

void cSearchCache::SortByName(bool order) // false desc, true asc
{
    if (order)
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareNameAsc); 
    else
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareNameDesc); 

    cSearchCache::Instance().searchCache_.SetHasChanged();
}


void cSearchCache::SortByLength(bool order) // false desc, true asc
{
    if (order)
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareLengthAsc); 
    else
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareLengthDesc); 

    cSearchCache::Instance().searchCache_.SetHasChanged();
}


void cSearchCache::SortByDate(bool order) // false desc, true asc
{
    if (order)
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareDateAsc); 
    else
        cSearchCache::Instance().SearchResult().cacheList_.sort( CompareDateDesc); 

    cSearchCache::Instance().searchCache_.SetHasChanged();
}


void cSearchCache::RunSearch()
{
   if ( !HasParamsChanged())  return; 

    cCache *tmpCache=NULL;
    
    // select the correct cache
    switch( CacheType())
    {
        case eAudioCache:
        case eVideoCache:
                    tmpCache = &fileList;
                    break;
        
        case eVdrRecCache:
        case eDvdCache:
                    tmpCache = &dirList;
                    break;

        case eMainCache:
        default:
                    tmpCache = &Cache;
                    break;

    } // switch

    cCache OldSearchCache;
    if (SearchInSearch())
    {
        OldSearchCache = searchCache_;
        tmpCache = &OldSearchCache;
        ResetSearchInSearch();
    }
    searchCache_.cacheList_.clear();
    
    // tmpCache != NULL
    std::list<cCacheItem*>::iterator it = tmpCache->cacheList_.begin();
    
    for( ; it != tmpCache->cacheList_.end(); ++it)
    {
        if (doesMatchSearchParam(*it)) searchCache_.AddItem(*it);
    } //for

    // Seen the changes
    tmpCache->ResetChanged(); 

    // donot run search more than once on the same params
    ResetParamChanged(); 

    // Search Cache changed even if nothing is found
    searchCache_.SetHasChanged();
}

bool cSearchCache::doesMatchSearchParam(const cCacheItem* item) const
{

    // main cache contains all file types
    if ( CacheType() != eMainCache &&  CacheType() != FileType2CacheType(item->Type()) ) 
        return false;

    // check for file type
    int item_fType = item->Type();
    
    switch(FileType())
    {
        case 0: // all
            break;
        
        case 1: // audio
            if (item_fType != eAudioFile) return false;
            break;
        
        case 2: // video
            if ( item_fType != eVideoFile && item_fType != eDvdDir && item_fType != eVdrRecDir) return false;
            break;

        case 3: // photos
            if ( item_fType != ePictureFile) return false;
            break;
        
        default:
            return false;
    }

    switch(SearchType())
    {
        case st_none:
            return true;

        case st_metadata: // Genre/Album/Artist
            return StringCaseEqual(*item->MetaData(metaDataType_), searchMetaDataStr_);

        // path begins with
        case st_path_starts_with: 
            return item->Path().find(searchPathStr_) == 0;

        // anywhere in the path
        case st_path:
            return strstr( item->Path().c_str(), searchPathStr_.c_str()) != NULL;

        case st_text:
            //return  item->DisplayName().find(searchStr_) != std::string::npos ;
            return  strcasestr(item->SearchableText().c_str(), searchStr_.c_str()) != NULL ;
        
        case st_date:   //XXX
        case st_length: //XXX
        default:
            return true;
    }
}

cSearchCache& cSearchCache::Instance()
{
    static cSearchCache searchCacheObj;

    return searchCacheObj;
}

// cache type
void cSearchCache::SetCacheType(int ct) { cacheType_ = ct; SetParamChanged(); }
int cSearchCache::CacheType()const { return cacheType_;}
void cSearchCache::SetFileType(int ft) { fileType_ = ft; SetParamChanged();}
int cSearchCache::FileType() const { return fileType_; }
// search type
void cSearchCache::SetSearchType(int st) { searchType_ = st; SetParamChanged(); }
int cSearchCache::SearchType()const { return searchType_;}

int cSearchCache::MetaDataType        ()const { return metaDataType_; }
std::string cSearchCache::MetaDataStr() const { return searchMetaDataStr_;}

bool cSearchCache::HasParamsChanged()const { return ParamChanged_; }


bool cSearchCache::HasCacheChanged()const 
{
    switch( CacheType())
    {
        case eVideoCache:
        case eAudioCache:
                        // check fileCache
                        return fileList.HasChanged();
                        break;

        case eVdrRecCache:
        case eDvdCache:
                        // check DirCache
                        return dirList.HasChanged();
                        break;

        case eMainCache:
        default:
                        // check main cache
                        return Cache.HasChanged();
                        break;

    }
}

void cSearchCache::SetSearchMetaData(const std::string& str, int meta_data_type)
{
    searchMetaDataStr_ = str;
    metaDataType_ = meta_data_type;
    searchType_ = st_metadata;

    SetParamChanged();
}

void cSearchCache::SetSearchGenre(const std::string& str)
{
    searchMetaDataStr_ = str;
    metaDataType_ = meta_genre;
    searchType_ = st_metadata;

    SetParamChanged();
}

void cSearchCache::SetSearchString(const std::string& str)
{
    searchStr_ = str;
    searchType_ = st_text;

    SetParamChanged();
}

void cSearchCache::SetSearchPathStartsWith(const std::string& str)
{
    searchPathStr_ = str;
    searchType_ = st_path_starts_with;

    SetParamChanged();
}

void cSearchCache::SetSearchPath(const std::string& str)
{
    searchPathStr_ = str;
    searchType_ = st_path;

    SetParamChanged();
}

void cSearchCache::SetSearchDate(const time_t& t)
{
    searchDate_ = t;
    searchType_ = st_date;

    SetParamChanged();
}



void cSearchCache::SetSearchLength(const int& len)
{
    searchLength_ = len;
    searchType_ = st_length;

    SetParamChanged();
}

std::string cSearchCache::GCD()  // Greated common directory
{
    std::list<cCacheItem*>::iterator it = SearchResult().cacheList_.begin();
    std::string gcd ="";

    if (searchCache_.size())
        gcd = (*it)->FullPath();

    for ( ; it != SearchResult().cacheList_.end(); ++it)
        gcd = StringOverlap( (*it)->FullPath(), gcd);

    return gcd;
}

std::list<std::string> cSearchCache::MetaDataList(const int metadata_type)
{
    std::list<cCacheItem*>::iterator it = SearchResult().cacheList_.begin();
    std::list <std::string> metadata_list;
    std::string metadataStr; // tmp string

    for ( ; it != SearchResult().cacheList_.end(); ++it)
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

