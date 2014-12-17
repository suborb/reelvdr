#ifndef CACHE_ITEM_H
#define CACHE_ITEM_H

#include <string>
#include <time.h>

#include <vdr/tools.h>

class cCacheItem
{
    private:
        int             type_;

        std::string     path_;
        std::string     name_, displayName_;


    protected:
        int             length_; // secs
        time_t          createdOn_;
        bool            inPlaylist;

    public:
        cCacheItem(const std::string path, const std::string name, int type);
        virtual ~cCacheItem();

        virtual bool Play() const;

        inline bool InPlaylist()const { return inPlaylist; }
        inline void SetInPlaylist() { inPlaylist = true; }
        inline void SetNotInPlaylist() { inPlaylist = false; }

        inline std::string FullPath() const    { return path_ + "/" + name_; }
        inline std::string Path()const         { return path_; }

        inline std::string Name()const         { return name_; }
        inline std::string DisplayName()const  { return displayName_; }

        inline int         Type() const         { return type_; }
        inline int         Length() const       { return length_; }
        inline time_t      Date()  const        { return createdOn_; }

        virtual cString MetaData(int metadata_type)const { return ""; } // cString because of UTF

        inline std::string LengthString()const 
        { 
            return length_ < 0 ? "" : *cString::sprintf("%02i:%02i", length_/60, length_ - 60*(length_/60) );
        }

        virtual std::string DateString(bool MonthStr = false); 
        virtual std::string TimeString(bool IncludeSec = false); 

        virtual std::string GetDisplayString()const;

        //should return string containing all search able texts, name + album + artist etc...
        virtual std::string SearchableText()const; 

        virtual void SetDisplayName(std::string displayName);
        virtual void SetDate(); // creation date / last access date

        virtual void SetLength(int len=-1);
};



class cFileItem : public cCacheItem
{
    private:
        // name, ide3 info etc
        virtual void SetLength(int len=-1);

    public:
        cFileItem(const std::string& path, const std::string& name, int type);
        virtual ~cFileItem();

        std::string GetDisplayString()const;
        std::string SearchableText()const;

        virtual cString MetaData(int metadata_type)const; // cString because of UTF

        virtual bool Play()const ; // call xinemediaplayer ?
};



class cDirItem : public cCacheItem
{
    private:
        // name, ide3 info etc
        virtual void SetLength(int len=-1);

    public:
        cDirItem(const std::string& path, const std::string& name, int type);
        virtual ~cDirItem();

        std::string GetDisplayString()const;
        virtual cString MetaData(int metadata_type)const; // cString because of UTF

        virtual bool Play()const; // call xinemediaplayer ?
};

#endif

