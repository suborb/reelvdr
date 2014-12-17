#include "Id3.h"

//id3tags : using TagLib
#include <fileref.h>
#include <tag.h>

std::string Id3Genre(const std::string& Path)
{
    TagLib::FileRef f( Path.c_str() );
    if(f.isNull() || !f.tag()) return std::string();

    std::string genreStr;

    TagLib::Tag *tag = f.tag();
    genreStr = tag->genre().stripWhiteSpace().toCString(false)  ; // unicode = false

    return genreStr;
}

std::string Id3Album(const std::string& Path)
{
    TagLib::FileRef f( Path.c_str() );
    if(f.isNull() || !f.tag()) return std::string();

    std::string albumStr;

    TagLib::Tag *tag = f.tag();
    albumStr = tag->album().stripWhiteSpace().toCString(false)  ; // unicode = false

    return albumStr;
}

std::string Id3Artist(const std::string& Path)
{
    TagLib::FileRef f( Path.c_str() );
    if(f.isNull() || !f.tag()) return std::string();

    std::string artistStr;

    TagLib::Tag *tag = f.tag();
    artistStr = tag->artist().stripWhiteSpace().toCString(false)  ; // unicode = false

    return artistStr;
}

std::string Id3Info(const std::string& songPath, int mode )
{
    std::string name;
    TagLib::FileRef f( songPath.c_str() );
    if(f.isNull() || !f.tag()) return std::string();

    TagLib::Tag *tag = f.tag();
    name  = tag->title().stripWhiteSpace().toCString(false)  ; // unicode = false

    // artist mode == 1
    if ( !name.empty() && (mode&0x1) )
    {
        std::string artist = tag->artist().stripWhiteSpace().toCString(false) ;

        if (!artist.empty())
            name  = name + " - " + artist;
    }

    // album  mode == 2
    if ( !name.empty() && (mode&0x2) )
    {
        std::string album =  tag->album().stripWhiteSpace().toCString(false) ;

        if (!album.empty())
            name = name + " - " + album;
    }

    return name;
}
