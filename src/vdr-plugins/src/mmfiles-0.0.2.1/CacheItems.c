#include "CacheItems.h"
#include "CacheTools.h"
#include "cache.h"

#include "Playlist.h"

#include "RecordingsReplayControl.h"

#include "Id3.h"

#include <sstream>
#include <iomanip>

#include <vdr/i18n.h>
#include <vdr/recording.h>

//id3tags : using TagLib
#include <fileref.h>
#include <tag.h>

#include "classify.h"

cCacheItem::cCacheItem(const std::string path, const std::string name, int type)
{ 
    path_ = path; name_ = name; type_ = type;
    length_ = -1;
    createdOn_ = 1;
    displayName_ = name;

    // set date ?
    SetDate();
    SetNotInPlaylist();
    // add to Cache list
    Cache.AddItem(this);
}

cCacheItem::~cCacheItem()
{
    // remove 'this' pointer from cache list
    Cache.RemoveItem(this);

    // Remove cache Item from Playlist
    CurrentPlaylist.Remove(this);
}

bool cCacheItem::Play()const { return false; }

std::string cCacheItem::GetDisplayString() const
{   
    return DisplayName();
}

std::string cCacheItem::SearchableText()const
{   
    return GetDisplayString();
}

void cCacheItem::SetDisplayName(std::string displayName) { displayName_ = displayName; }

void cCacheItem::SetDate()
{
    std::string full_path = FullPath();
    struct stat buf;

    if (stat(full_path.c_str(), &buf) == 0) // success
        createdOn_ = buf.st_mtime; // last modified time
    else
        createdOn_ = 0; // -1 ?
}

std::string cCacheItem::DateString(bool MonthStr)
{
    static char buf[32];
    struct tm tmstruct;

    if ( localtime_r( &createdOn_, &tmstruct) == NULL) return ""; // error

    if (MonthStr)
        strftime(buf, sizeof(buf), "%d.%b %y", &tmstruct);
    else
        strftime(buf, sizeof(buf), "%d.%m.%y", &tmstruct);

    return buf;
}

std::string cCacheItem::TimeString(bool IncludeSec)
{

    static char buf[16];
    struct tm tmstruct;

    if ( localtime_r( &createdOn_, &tmstruct) == NULL) return ""; // error

    if (IncludeSec)
        strftime(buf, sizeof(buf), "%R:%S", &tmstruct);
    else
        strftime(buf, sizeof(buf), "%R", &tmstruct);

    return buf;

}

void cCacheItem::SetLength(int len)
{
    length_ = len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

cFileItem::cFileItem(const std::string& path, const std::string& name, int type) : cCacheItem(path, name, type)
{
    // Add to filelist
    fileList.AddItem(this);
    //Get Id3 info?
    std::string str = Id3Info(path +"/"+ name,0x3);
    SetDisplayName(str.empty()?RemoveEnding(name):str);

    if (type == eAudioFile)
        SetLength();
    else SetLength(-1);

}
cFileItem::~cFileItem()
{
    // remove from file list
    fileList.RemoveItem(this);
}

bool cFileItem::Play()const 
{ 
    // Call xinemediaplayer
    switch( Type())
    {
        case eAudioFile:
        case eVideoFile:
            {
                std::vector<std::string> playlistEntries;
                playlistEntries.push_back( FullPath() );

                return CallXinemediaplayer( playlistEntries);
            }
            break;

        case ePictureFile:
            printf("Cannot show IMAGE files\n");
        default:
            printf("UNKNOWN file type, not playing it\n");
            return false;
    }

}

cString cFileItem::MetaData(int metadata_type) const
{

    if(Type()==eVideoFile) return tr("Video");
    if(Type()==ePictureFile) return tr("Photo");
    // Function pointer
    std::string (*metadata_func)(const std::string&) = NULL;

    switch(metadata_type)
    {
        case meta_genre:
            //return Id3Genre(FullPath());
            metadata_func = &Id3Genre;
            break;

        case meta_album:
            //return Id3Album(FullPath());
            metadata_func = &Id3Album;
            break;

        case meta_artist:
            //return Id3Artist(FullPath());
            metadata_func = &Id3Artist;
            break;

        default:
            return "";
    }

    std::string retStr = (*metadata_func)(FullPath());

    return retStr.empty()?tr("Unknown"): retStr.c_str();
}


void cFileItem::SetLength(int len)
{
    if (len >= 0) { length_ = len; return;}

    // calculate length of file

    TagLib::FileRef f( FullPath().c_str() );
    if( f.isNull() ) return;

    length_ = f.audioProperties()->length();
}



std::string cFileItem::GetDisplayString() const
{   
    std::stringstream str;
    if (InPlaylist())
        str << ">";
    else str<< " ";
    str<<"\t";

    switch( Type())
    {
        case eAudioFile:
            str<< char(139)<<" "<<DisplayName()<<"  \t "<< LengthString()<<(LengthString().length()>0?"'":"");
            break;
        case eVideoFile:
            // id3 tag
            //str<< DateString() << "  "<< LengthString()<<"'  "<<DisplayName();
            //    str<< DateString() << "\t"<< LengthString()<<(LengthString().length()>0?"'":"")<<"\t"<<DisplayName();
            //    break;

            str<< char(141)<<" "<<DisplayName()<<"  \t "<< LengthString()<<(LengthString().length()>0?"'":"");
            break;
        default:
            str<<Name()<<"\t";
            break;
    }
    return str.str();;
}
std::string cFileItem::SearchableText()const
{
    switch(Type())
    {
        case eAudioFile:
        case eVideoFile:
            return DisplayName() + " " + Id3Artist(FullPath()) + " " + Id3Album(FullPath());

        default:
            return Name();
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
 * removes @ , % and ~ from recording title
 */
std::string BeautifyName(const std::string& str )
{
    std::string returnString;
    // ~
    size_t idx = str.rfind('~');
    if (idx != std::string::npos) returnString = str.substr(idx+1);
    else returnString = str;

    // remove @ / %
    idx = returnString.rfind('~');
    if (idx != std::string::npos) returnString = returnString.substr(idx+1);

    idx = returnString.rfind('%');
    if (idx != std::string::npos) returnString = returnString.substr(idx+1);

    return returnString;
}




cDirItem::cDirItem(const std::string& path, const std::string& name, int type) : cCacheItem(path, name, type)
{
    // Add to filelist
    dirList.AddItem(this);

    // Set Display name
    if (type == eVdrRecDir)
    {
        std::string filename = path + "/" +name;
        std::string baseDir = VdrRecBaseDir(filename);

        cRecording *recording = new cRecording(filename.c_str() , baseDir.empty()?path.c_str(): baseDir.c_str());

        Recordings.Add(recording); // add to VDR Rec list
        Recordings.ChangeState();

        std::string tmpStr;
        if (recording->Info())
            tmpStr = recording->Info()->Title()? recording->Info()->Title():"";

        // try Name
        if (tmpStr.empty())
            tmpStr = recording->Name()?recording->Name():tr("Unknown VdrRecording");

        // remove %, ~, @
        tmpStr = BeautifyName(tmpStr); 

        SetDisplayName( tmpStr );

        // set date
        createdOn_ = recording->start; 

    }
    else if ( type == eDvdDir)
    {
        SetDate(); 
        SetDisplayName( name );
    }
}



cDirItem::~cDirItem()
{
    // remove from file list
    dirList.RemoveItem(this);
}

void cDirItem::SetLength(int len)
{
    if (len >= 0) { length_ = len; return;}
}


bool cDirItem::Play() const
{
    if (Type() == eVdrRecDir)
    {
        std::string filename = Path() + "/" + Name();
        std::string baseDir = VdrRecBaseDir(filename);

        cRecording *recording = new cRecording(filename.c_str(), baseDir.empty()?Path().c_str(): baseDir.c_str() );

        cVdrRecDirReplayControl::SetRecording(recording->FileName(), recording->Title(' ', false, recording->HierarchyLevels()));
        cControl::Shutdown();
        cControl::Launch(new cVdrRecDirReplayControl);

        delete recording; // leak?

        return true;
    }
    else if ( Type() == eDvdDir)
    {
        std::string mrl = "dvd://";
        mrl += FullPath() + "/VIDEO_TS";

        std::vector<std::string> playlistEntries;
        playlistEntries.push_back( mrl );

        return CallXinemediaplayer( playlistEntries);


    }

    return false;
}


cString cDirItem::MetaData(int metadata_type) const
{
    return tr("Video");
}

std::string cDirItem::GetDisplayString() const
{   
    std::stringstream str;
    if (InPlaylist())
    str<< ">";
    else str<<" ";
    str<< "\t";
    switch( Type() )
    {
        case eVdrRecDir:
            str << char(140)<< " "<<DisplayName();
            break;

        case eDvdDir:
            str << char(129) <<" "<<Name();
            break;
        default:
            str <<Name() ;

            break;
    }
    return str.str();
}

