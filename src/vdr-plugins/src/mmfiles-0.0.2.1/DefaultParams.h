#ifndef DEFAULT_PARAMS_H
#define DEFAULT_PARAMS_H

#include <string>

// singleton class; create obj using cDefaultParams::Instance()
class cDefaultParams
{
    private:
        std::string Title_;

        int BrowseMode_;
        int FileType_;
        int CacheType_;
        int SortType_;


        // constructor
        cDefaultParams();

        // copy constructor
        cDefaultParams(const cDefaultParams&);

    public:

        static cDefaultParams& Instance();

        // Reset params
        void Reset();

        // Title
        void SetMenuTitle(const std::string&);
        std::string MenuTitle()const;

        // Browse Mode
        void SetBrowseMode(int);
        int  BrowseMode()const;

        // File Type
        void SetFileType(int);
        int  FileType()const;

        // Cache Type
        void SetCacheType(int);
        int CacheType()const;

        // Sort Type
        void SetSortType(int);
        int SortType()const;


};

#endif

