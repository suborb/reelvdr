#include "scanDir.h"
#include <iostream>

static std::vector<int> types;
int ClassifyDirContents(const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs, std::vector<int>);
void AddDirContentsToCache(const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& subdirs);


cScanDir::cScanDir()
{
    followLink_ = false;
    recursive_ = true;
}

bool cScanDir::DirContents(const std::string& path,  std::vector<std::string> &files, 
                         std::vector<std::string>& subdirs)
{
    DIR *dp;
    struct dirent *dirp;
    std::string dirEntry;

    dp = opendir(path.c_str());
    if ( !dp ) return false;

    while((dirp = readdir(dp)) != NULL)
    {
        dirEntry = dirp->d_name;
        //if ( ExcludeFile(dirEntry)) continue;
        if ( dirEntry.empty() || dirEntry[0]=='.' ) continue; // exclude all hidden files

        if (IsDir(path + "/" + dirEntry))     subdirs.push_back(dirEntry);
        if (IsFile(path + "/" + dirEntry))    files.push_back(dirEntry);
        // the rest are ignored
    }
   
    closedir(dp);
    return true;
}

bool cScanDir::ScanDirTree(const std::string &path)
{
    std::vector <std::string> files, subdirs;

    // get contents of this Dir
    if ( ! DirContents( path, files, subdirs) )
        return false;

    // send the contents of Dir to FileCache
    //PrintDirContents(path, files, subdirs);
    //ClassifyDirContents(path, files, subdirs, types);
    AddDirContentsToCache(path, files, subdirs);

    if (Recursive())
        for (unsigned int i=0; i<subdirs.size(); ++i)
        {
            ScanDirTree(path + "/" + subdirs.at(i));
        }

    return true;
}

void cScanDir::PrintDirContents(const std::string& path,  const std::vector<std::string> &files,
                         const std::vector<std::string>& subdirs) const
{
    std::cout<<path<<": ("<<files.size()<<","<<subdirs.size()<<")"<<std::endl;

    unsigned int i=0;
    for (i=0; i<files.size(); ++i)
        std::cout<<files.at(i)<<" ";
    std::cout<<std::endl;
    for (i=0; i<subdirs.size(); ++i)
        std::cout<<subdirs.at(i)<<"/ ";
    std::cout<<std::endl;

}

bool cScanDir::Recursive() const { return recursive_; }
void cScanDir::SetRecursive(bool recursive)  {  recursive_ = recursive; }

bool cScanDir::IsDir(const std::string& path) const
{
    if (path.empty()) return false;

    struct stat64 buf;
    if(stat64(path.c_str(), &buf) != 0) return false; // stat error

    return S_ISDIR(buf.st_mode);
}

bool cScanDir::IsFile(const std::string& path) const 
{
    if (path.empty()) return false;

    struct stat64 buf;
    if(stat64(path.c_str(), &buf) != 0) return false; // stat error

    return S_ISREG(buf.st_mode);
}

bool cScanDir::IsLink(const std::string& path) const
{
    if (path.empty()) return false;

    struct stat64 buf;
    if(stat64(path.c_str(), &buf) != 0) return false; // stat error

    return S_ISLNK(buf.st_mode);
}

