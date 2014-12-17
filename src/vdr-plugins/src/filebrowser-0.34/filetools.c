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

#include "stringtools.h"
#include "filetools.h"

#include <sys/stat.h>
#include <sys/types.h> 
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
//#include <ifstream>

using namespace std;

namespace Filetools
{
    bool MyS_IFLNK(mode_t mode)
    {
        bool val = (mode & S_IFMT) == S_IFLNK;
        return val;
    }

    bool IsFile(const std::string &entry)
    {
        struct stat64 statBuf;
        stat64(entry.c_str(), &statBuf);
        if(S_ISREG(statBuf.st_mode)) //regular file
        {
            return true;
        }
        return false;
    }

    bool IsDir(const std::string &entry)
    {
        struct stat64 statBuf;
        stat64(entry.c_str(), &statBuf);
        if(S_ISDIR(statBuf.st_mode)) //regular file
        {
            return true;
        }
        return false; 
    }

    long long GetFileSize(const std::string &file)
    {
        struct stat64 statBuf;
        stat64(file.c_str(), &statBuf);
        if(S_ISREG(statBuf.st_mode)) //regular file
        {
            return statBuf.st_size;
        }
        return 0;
    }

    int GetRights(std::string filename, mode_t &mode)
    {
        struct stat64 statBuf;
        int ret = stat64(filename.c_str(), &statBuf);
        mode = statBuf.st_mode;
        return ret;
    }

    int SetRights(std::string filename, mode_t mode)
    {
        return chmod(filename.c_str(), mode);
    }

    int GetOwner(std::string filename, uid_t &owner, gid_t &group)
    {
        struct stat64 statBuf;
        int ret = stat64(filename.c_str(), &statBuf);
        owner = statBuf.st_uid;
        group = statBuf.st_gid;
        return ret;
    }

    int SetOwner(std::string filename, uid_t owner, gid_t group)
    {
        return chown(filename.c_str(), owner, group);
    }

    bool FileExists(const std::string &file)
    {
        FILE *fd = fopen64(file.c_str(), "r");
        if(fd)
        { 
            fclose(fd);
            return true;
        }
        return false;
    }

    bool DirExists(const std::string &dir)
    {
        DIR *d = opendir(dir.c_str());
        if (d == NULL && errno == ENOENT) 
        { 
            return false;
        }
        closedir(d);
        return true;
    }

    bool GetDirEntries(const std::string &dir, std::vector<std::string>  &entries)
    {
        DIR *dirp;
        struct dirent64 *dp; 

        if ((dirp = opendir(dir.c_str())) == NULL) 
        {
            printf("couldn't open %s\n", dir.c_str()); 
            return false;
        }

        while((dp = readdir64(dirp)) != NULL)
        {
            std::string name = dir + "/" + dp->d_name; 
            entries.push_back(name);
        }
        closedir(dirp);
        return true;
    } 

    bool GetDirEntries(const std::string &dir, std::vector<dirEntryInfo> &dirEntries, std::vector<dirEntryInfo>  &fileEntries,
                                        std::vector<dirEntryInfo>  &dirLinkEntries, std::vector<dirEntryInfo>  &fileLinkEntries)
    { 
        //printf("------------------GetDirEntries--------%s------\n", dir.c_str());
        DIR *dirp;
        struct dirent64 *dp; 
        struct stat64 buf, lbuf;

        if ((dirp = opendir(dir.c_str())) == NULL) 
        {
            printf("couldn't open %s\n", dir.c_str()); 
            return false;
        }

        while((dp = readdir64(dirp)) != NULL)
        {
            std::string path = dir + "/" + dp->d_name; 
            int statSuccess = stat64(path.c_str(), &buf); 
            int lstatSuccess = lstat64(path.c_str(), &lbuf);

            if((lstatSuccess == 0) && MyS_IFLNK(lbuf.st_mode))
            {
                if((statSuccess == 0) && S_ISDIR(buf.st_mode))
                {
                    dirLinkEntries.push_back(dirEntryInfo(path, buf));
                }
                else if((statSuccess == 0) && S_ISREG(buf.st_mode))
                {
                    fileEntries.push_back(dirEntryInfo(path, buf));
                }
                else if(statSuccess != 0)
                {
                    dirEntries.push_back(dirEntryInfo(path, buf));
                }
            }
            else if(statSuccess == 0)
            {
                if(S_ISDIR(buf.st_mode))
                { 
                    dirEntries.push_back(dirEntryInfo(path, buf));
                }
                else if(S_ISREG(buf.st_mode))
                {
                    fileEntries.push_back(dirEntryInfo(path, buf));
                }
            }
        }
        closedir(dirp);
        return true;
    }

    bool GetDirEntries(const std::string &dir, std::vector<dirEntryInfo>  &dirEntries, std::vector<dirEntryInfo>  &fileEntries)
    { 
        std::vector<dirEntryInfo> dirLinkEntries;
        std::vector<dirEntryInfo> fileLinkEntries;
        return GetDirEntries(dir, dirEntries, fileEntries, dirLinkEntries, fileLinkEntries);
    }   

    bool GetDirFiles(const std::string &dir, std::vector<dirEntryInfo>  &files)
    { 
        std::vector<dirEntryInfo> dirEntries;
        return GetDirEntries(dir, dirEntries, files);
    }
  

    bool IsExecutableDir(std::string dir)
    {
        mode_t mode;
        int ret = GetRights(dir, mode);
        if(ret == 0 && mode&0000100)
        {
            return true;
        }   
        return false;
    }


    bool IsOnSameFileSystem(std::string path, std::string dir)
    {
        std::string mountfile = "/tmp/mountinfo";
        std::string cmd = "sudo mount > " + mountfile;
        ::system(cmd.c_str());

        char device[256]; 
        char on[265];
        char mountpoint[256];
        char type[265];
        char fs[256];
        char ops[256];

        std::vector<std::string> mountpoints;

        ifstream is(mountfile.c_str(), ifstream::in); 
        is.setf (ios::skipws);
        if(is.good())
        {
            while (!is.eof())
            {
                is >> device >> on >> mountpoint >> type >> fs >> ops;
                mountpoints.push_back(mountpoint);
            }
        }
        is.close();

        int lendir = 0;
        int lenpath= 0;
        int posdir= 0;
        int pospath = 0;        
        for(uint i = 0; i < mountpoints.size(); ++i)
        {
            if (dir.find(mountpoints[i]) != dir.npos)
            {
                if(lendir < mountpoints[i].size())
                {
                    lendir = mountpoints[i].size();
                    posdir = i;
                }
            } 
            if (path.find(mountpoints[i]) != path.npos)
            {
                if(lenpath < mountpoints[i].size())
                {
                    lenpath = mountpoints[i].size();
                    pospath = i;
                }
            }
            //printf("mountpoints[%d] = %s\n", i, mountpoints[i].c_str());
        }

        if (posdir == pospath)
        {
            return true;
        }
        return false;
    }


    bool InSameFileSystem(const std::string& path1, const std::string& path2)
    {
        struct stat stat1,stat2;
        stat(path1.c_str(),&stat1);
        stat(path2.c_str(),&stat2);
    
        return stat1.st_dev==stat2.st_dev;
    }
}
