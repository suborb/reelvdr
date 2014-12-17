#include "DefaultParams.h"

// constructor
cDefaultParams::cDefaultParams()
{
    Reset();
}


// Instance(): Use this to create a object of cDefaultParams
cDefaultParams& cDefaultParams::Instance()
{
    static cDefaultParams inst;

    return inst ;
}

// Reset
void cDefaultParams::Reset()
{
    Title_ = "mmfiles" ;

    BrowseMode_ = FileType_ = CacheType_ = SortType_ = 0 ;
}

// Menu Title 
void cDefaultParams::SetMenuTitle(const std::string& title) 
{
    Title_ = title ;
}

std::string cDefaultParams::MenuTitle()const 
{
    return Title_ ;
}


// Browse Mode 
void cDefaultParams::SetBrowseMode(int mode)
{
    BrowseMode_ = mode ;
}

int cDefaultParams::BrowseMode()const
{
    return BrowseMode_ ;
}


// File Type
void cDefaultParams::SetFileType(int type)
{
    FileType_ = type ;
}

int cDefaultParams::FileType()const 
{
    return FileType_ ;
}


// Cache Type
void cDefaultParams::SetCacheType(int type)
{
    CacheType_ = type ;
}

int cDefaultParams::CacheType()const
{
    return CacheType_ ;
}


// Sort Type
void cDefaultParams::SetSortType(int type)
{
    SortType_ = type ;
}

int cDefaultParams::SortType()const 
{
    return SortType_ ;
}

