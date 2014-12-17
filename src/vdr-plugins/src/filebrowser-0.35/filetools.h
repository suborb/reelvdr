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
 
#ifndef P__FILETOOLS_H
#define P__FILETOOLS_H

#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h> 
#include <dirent.h>
#include <errno.h>

namespace Filetools
{
    struct dirEntryInfo
    {
        dirEntryInfo(std::string name, struct stat64 buf)
        : name_(name), buf_(buf) {}
        std::string name_;
        struct stat64 buf_;
    };

    bool MyS_IFLNK(mode_t mode);
    bool IsFile(const std::string &entry);
    bool IsDir(const std::string &entry);
    long long GetFileSize(const std::string &file);
    int GetRights(std::string filename, mode_t &mode);
    int SetRights(std::string filename, mode_t mode);
    int GetOwner(std::string filename, uid_t &owner, gid_t &group);
    int SetOwner(std::string filename, uid_t owner, gid_t group);
    bool FileExists(const std::string &file);
    bool DirExists(const std::string &dir);
    bool GetDirEntries(const std::string &dir, std::vector<std::string>  &entries);
    bool GetDirEntries(const std::string &dir, std::vector<dirEntryInfo> &dirEntries, std::vector<dirEntryInfo>  &fileEntries,
                                        std::vector<dirEntryInfo>  &dirLinkEntries, std::vector<dirEntryInfo>  &fileLinkEntries);
    bool GetDirEntries(const std::string &dir, std::vector<dirEntryInfo>  &dirEntries, std::vector<dirEntryInfo>  &fileEntries);
    bool GetDirFiles(const std::string &dir, std::vector<dirEntryInfo> &files);
    bool IsExecutableDir(std::string dir); 
    bool IsOnSameFileSystem(std::string file, std::string dir);
    bool CreateDir(const std::string& dir);
    bool IsSameInode(const std::string &entry1, const std::string &entry2);
    ino_t GetInodeNr(std::string path);
}

#endif //P__FILETOOLS_H
