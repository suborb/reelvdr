#include "stringtools.h"
#include <vector>
#include <iostream>

#include "classify.h"
#include "cache.h"

/** Recognized Audio and Video Extensions */
static std::string AudioExt = "mp3 ogg wav";
static std::string VideoExt = "avi mpg mpeg divx xvid";

/*extensions of contents in a Vdr Recording Directory*/
static std::string VdrDirContentsExt = "vdr logo fsk";

/*extensions of contents in a DVD Directory*/
static std::string DvdDirContentsExt = "VIDEO_TS ";

/* list of dirs to be hidden*/
static std::string ExcludeDirs = ". ..";



bool IsAudioFile(const std::string& file) 
{
    return IsInsideStringChain(AudioExt, GetEnding(file), false);
}

bool IsVideoFile(const std::string& file)
{
    return IsInsideStringChain(VideoExt, GetEnding(file), false);
}

/** VDR dir can have 
 * *.vdr
 * *.logo
 * *.fsk
 */
bool IsVdrRecDir(const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs)
{
    unsigned int i=0;
    std::string extStr; // file extension ".vdr" / ".txt" etc.

    for(i=0; i<files.size(); ++i)
    {
        extStr = GetEnding(files.at(i));
        if (!IsInsideStringChain( VdrDirContentsExt, extStr, false)) 
            return false;
    }

    // is Vdr Rec dir only if dir was non empty
    return files.size() > 0;
}

bool IsEmptyDir( const std::string &path, const std::vector<std::string>& files)
{
    return files.size() == 0;
}


bool IsDvdDir( const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs)
{
    unsigned int  i=0;
    for( i=0; i<subdirs.size(); ++i)
    {
        if ( !IsInsideStringChain(DvdDirContentsExt, subdirs.at(i), false)) return false;
    }

    return subdirs.size() > 0;
}

bool IsHiddenDir( const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs)
{
    if (path.empty() || path[0] == '.') return true;

    return false;
}

bool IsExcludedDir( const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs)
{
    return IsInsideStringChain( ExcludeDirs, GetLast(path), false);
}

int ClassifyFile( const std::string & filename)
{
    if (IsAudioFile( filename ))        return eAudioFile;
    if (IsVideoFile( filename ))        return eVideoFile;

    return eUnknownFile;
}

int ClassifyDir(const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs)
{
    if ( IsHiddenDir    (path, files, subdirs) || 
            IsExcludedDir  ( path, files, subdirs) )   return eNotShownDir;

    if ( IsVdrRecDir    (path, files, subdirs))     return eVdrRecDir;
    if ( IsDvdDir       (path, files, subdirs))     return eDvdDir;
    if ( IsEmptyDir     (path, files))              return eEmptyDir;

    return eNormalDir;
}
/*
 * returns directory type
 * VDR Rec / DVD / unknown dir=normal
 *
 * subdirs are not used, only provided for classifying current directory (represented by path)
 */

/*
   int ClassifyDirContents(const std::string& path, const std::vector<std::string>& files,
   const std::vector<std::string>& subdirs, std::vector<int> types)
   {
   unsigned int i = 0;

//Decide on Directory
// if a special directory like VDR Rec / DVD 
// then donot probe files
std::string result;
switch (ClassifyDir( path, files, subdirs))
{
case eNotShownDir:  result = "not shown/ hidden directory"; break;
case eVdrRecDir:    result = "Vdr Recording directory";     break;    
//    case eEmptyDir:     result = "empty directory";             break;
case eDvdDir:       result = "Dvd directory";               break;

default:    result = "";
break;
}
if ( !result.empty()) {std::cout <<path<<" : "<<result<<std::endl;
return 1;}
// classify files
types.clear();

for (i=0; i<files.size(); ++i)
{
if ( IsAudioFile(files.at(i)))
std::cout<<path+"/"+files.at(i)<<" Audio file"<<std::endl;
else
if ( IsVideoFile(files.at(i)))
std::cout<<path+"/"+files.at(i)<<" Video file"<<std::endl;
}
}
*/

/* subdirs are not used, only provided for classifying current directory (represented by path)
*/
void AddDirContentsToCache(const std::string& path, const std::vector<std::string>& files,
        const std::vector<std::string>& subdirs)
{

    //Decide on Directory
    // if a special directory like VDR Rec / DVD 
    // then donot probe files
    int type = ClassifyDir( path, files, subdirs);

    switch( type)
    {
        case eVdrRecDir:   
        case eDvdDir:     
            AddToCache(path, type); // dir
            return;
            break;

        default:    
            break;
    }

    // classify files
    unsigned int i = 0;
    for (i=0; i<files.size(); ++i)
    {
        if ( IsAudioFile(files.at(i)))
            AddToCache( path + "/" + files.at(i), eAudioFile);
        else
            if ( IsVideoFile(files.at(i)))
                AddToCache( path + "/" + files.at(i), eVideoFile);
    }
}

