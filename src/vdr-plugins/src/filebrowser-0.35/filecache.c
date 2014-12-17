/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <vdr/thread.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef NEWUBUNTU
#include <unistd.h>
#endif
#include "filecache.h"
#include "stringtools.h"
#include "filetools.h"

//-----------------------------------------------
//----------cFileCache-----------------------
//-----------------------------------------------

bool cFileCache::Open(const std::string &cacheDir, uint maxCacheSize)
{
     if(RemoveName(cacheDir) != "")
         if(mkdir(RemoveName(cacheDir).c_str(), S_IRUSR | S_IWUSR) == -1 && errno != EEXIST)       
             Open(RemoveName(cacheDir), maxCacheSize);

     if(mkdir(cacheDir.c_str(), S_IRUSR | S_IWUSR) == -1)
     {
         if(errno != EEXIST)
         {
             return false;
         }
     }
     cacheDir_ = cacheDir;
     maxCacheSize_ = maxCacheSize;
     return true;
}

bool cFileCache::PutInCache(const std:: string &item, const std::vector<unsigned char> &buffer, std::string ending, bool keyless)
{ 
    //printf("-------cFileCache::PutInCache, item = %s-----\n", item.c_str());
    if(!LimitCacheSize())
        return false;

    std::string filename = BuildKey(item, ending, keyless); 
    if (filename == "")
        return false;

    FILE *cacheFile = ::fopen64(filename.c_str(), "wb");
    bool success = false;
    if(cacheFile)
    { 
        if (buffer.size() > 0)
        {   
            const unsigned char* buf = &buffer[0];
            uint r = ::fwrite(buf, 1, buffer.size(), cacheFile);
            if (r == buffer.size())
                success = true;
            ::fclose(cacheFile);
        } 
    } 
    return success;
}

bool cFileCache::FetchFromCache(const std::string &item, std::vector<unsigned char> &buffer, std::string ending, bool keyless) const
{ 
   // printf("------------ cFileCache::FetchFromCache, item = %s-------------\n", item.c_str());
    std::string filename = BuildKey(item, ending);
    if (filename == "")
        return false;

    FILE *cacheFile = ::fopen64(filename.c_str(), "rb");
    bool success = false;
    if (cacheFile)
    {
        struct stat64 fileStat;
        ::stat64(filename.c_str(), &fileStat);
        int fileSize = fileStat.st_size;

        if (fileSize > 0)
        {
            buffer.resize(fileSize);
            unsigned char* buf = &buffer[0];
            int r = ::fread(buf, 1, fileSize, cacheFile);
            if (r == fileSize)
            {
                success = true;
            }
            ::fclose(cacheFile);
        } 
    } 
    return success;
}

bool cFileCache::FetchFromCache(const std::string &item, std::string &file, std::string ending, bool keyless) const
{
    file = BuildKey(item, ending, keyless);
    return Filetools::FileExists(file);
}

bool cFileCache::InCache(const std::string &item, std::string ending, bool keyless) const
{
    std::string filename = BuildKey(item, ending, keyless);
    return Filetools::FileExists(filename);
}

void cFileCache::RemoveFromCache(std::string item, std::string ending, bool keyless)
{ 
    std::string filename = BuildKey(item, ending, keyless);
    unlink(filename.c_str());
    //std::string cmd = std::string("rm -f ") + "\"" + filename + "\"";
    //SystemExec(cmd.c_str());
}

void cFileCache::ClearCache()
{
    std::string cmd = "rm -f " + cacheDir_ + "/*";
    SystemExec(cmd.c_str());
}

std::string cFileCache::BuildKey(const std::string &item, std::string ending, bool keyless) const
{
    //printf("------cFileCache::BuildKey--, item = %s\n", item.c_str());
    if(keyless)
    {
        return cacheDir_ + "/" + GetLast(item) + ending;
    }

    char const *path = item.c_str();
    struct stat64 stbuf;

    if (::stat64(path, &stbuf) == 0)
    {
        char const *baseName = rindex(path, '/');
        if (baseName)
        {
            baseName += 1;
        }
        else
        {
            baseName = path;
        }
        char key[512];
        ::sprintf(key, "%s_%u_%u", baseName,
          static_cast<unsigned int>(stbuf.st_size),
          static_cast<unsigned int>(stbuf.st_mtime));
        return cacheDir_ + "/" + key + ending;
    }
    else
    {
        return std::string(); // error
    }
} 

bool cFileCache::LimitCacheSize()
{
    return true; //test!!!!

    DIR *dirp;
    struct dirent64 *dp;
    struct stat64 stbuf;  
    uint cacheSize = 0;
    uint lasttime = 0xFFFFFFFF;
    std::string oldestFile;

    if ((dirp = opendir(cacheDir_.c_str())) == NULL) 
    {
        printf("filecache cannot be opened %s", cacheDir_.c_str());
        return false;
    }

    do
    {
        while((dp = readdir64(dirp)) != NULL)
        {
            std::string file = cacheDir_ + "/" + dp->d_name; 
            stat64(file.c_str(), &stbuf);
            cacheSize += static_cast<unsigned int>(stbuf.st_size);
            if (static_cast<unsigned int>(stbuf.st_mtime) < lasttime)
            {
                oldestFile = file;
                lasttime = static_cast<unsigned int>(stbuf.st_mtime);
            }
        }
        unlink(oldestFile.c_str());
    }
    while(cacheSize > maxCacheSize_);

    closedir(dirp);
    return true;
}

//-----------------------------------------------
//----------cTempFileCache-----------------------
//-----------------------------------------------

bool cTempFileCache::PutInCache(const std:: string &item, int size, const std::vector<unsigned char> &buffer, std::string ending)
{ 
    //printf("-------cFileCache::PutInCache, item = %s-----\n", item.c_str());
    if(!LimitCacheSize())
        return false;

    std::string filename = BuildKey(item, size, ending); 
    if (filename == "")
        return false;

    FILE *cacheFile = ::fopen64(filename.c_str(), "wb");
    bool success = false;
    if(cacheFile)
    { 
        if (buffer.size() > 0)
        {   
            const unsigned char* buf = &buffer[0];
            uint r = ::fwrite(buf, 1, buffer.size(), cacheFile);
            if (r == buffer.size())
                success = true;
            ::fclose(cacheFile);
        } 
    } 
    return success;
}

bool cTempFileCache::FetchFromCache(const std::string &item, std::string &file, int filesize, std::string ending) const
{
    file = BuildKey(item, filesize, ending);
    return Filetools::FileExists(file);
}

bool cTempFileCache::InCache(const std::string &item, int filesize, std::string ending) const
{
    std::string filename = BuildKey(item, filesize, ending);
    return Filetools::FileExists(filename);
}

void cTempFileCache::RemoveFromCache(std::string item, int filesize, std::string ending)
{ 
    std::string filename = BuildKey(item, filesize, ending);
    unlink(filename.c_str());
    //std::string cmd = std::string("rm -f ") + "\"" + filename + "\"";
    //SystemExec(cmd.c_str());
}

std::string cTempFileCache::BuildKey(const std::string &item, std::string ending, bool keyless) const
{
    char const *path = item.c_str();
    struct stat64 stbuf;

    if (::stat64(path, &stbuf) == 0)
    {
        char const *baseName = rindex(path, '/');
        if (baseName)
        {
            baseName += 1;
        }
        else
        {
            baseName = path;
        }
        char key[512];
        ::sprintf(key, "%s_%u", baseName, static_cast<unsigned int>(stbuf.st_size));
        return cacheDir_ + "/" + key + ending;
    }
    else
    {
        return std::string(); // error
    }
}

std::string cTempFileCache::BuildKey(const std::string &item, int filesize, std::string ending) const
{
    //printf("------cTempFileCache::BuildKey----------\n");
    char const *path = item.c_str();
    char const *baseName = rindex(path, '/');
    if (baseName)
    {
        baseName += 1;
    }
    else
    {
        baseName = path;
    }
    char key[512];
    ::sprintf(key, "%s_%u", baseName, filesize);
    return cacheDir_ + "/" + key + ending;
}

