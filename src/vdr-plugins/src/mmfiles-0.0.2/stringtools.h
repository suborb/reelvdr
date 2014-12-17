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

#include <string>
#include <algorithm> 
#include <cstring>

#ifndef P__STRINGTOOLS_H
#define P__STRINGTOOLS_H

//return string after the last '/' character
inline std::string GetLast(const std::string &path)
{
    return path.substr(path.find_last_of("/") + 1, path.size());
}

//return string after the last '.' character
inline std::string GetEnding(const std::string &path)
{
    return path.substr(path.find_last_of(".") + 1, path.size());
}

//remove substring after the last '.' character
inline std::string RemoveEnding(const std::string &path)
{
    return path.substr(0, path.find_last_of("."));
}

//remove substring after the last '/' character
inline std::string RemoveName(const std::string &path)
{
    return path.substr(0, path.find_last_of("/"));
}

//return substring containing the last N characters
inline std::string GetLastN(const std::string &path, int n)
{
    int length = path.size();
    int start = std::max(0, length - n);
    return path.substr(start, path.size());
}

//string "path" starts with string "start" 
inline bool StartsWith(const std::string &path, const std::string &start)
{
    return (path.find(start) == 0) ? true : false;
}

//string "path" starts with string "start" and is actually longer 
inline bool StartsWith2(const std::string &path, const std::string &start)
{
    return StartsWith(path, start) && (path.size() > start.size());
}

//string "str" occurs in "chain" (seperated by blank spaces an not longer than 256 chars)
inline bool IsInsideStringChain(const std::string chain, const std::string str, bool caseSensitive)
{
    char cchain[256];
    strcpy (cchain, chain.c_str());
    char *token = strtok(cchain, " ");
    while(token)
    {
        if(!caseSensitive && !strcasecmp(str.c_str(), token) || !strcmp(str.c_str(), token))
        {
            return true;
        }
        token = strtok(NULL, " ");
    }
    return false;
}


// returns the subdir of path given the full_path "dir structure"
//  path SHOULD end with '/'
inline std::string GetSubDir(const std::string &full_path, const std::string &path)
{
   if ( !StartsWith(full_path, path)) return std::string("!"); // error

   std::string tmp = full_path.substr(path.length(),std::string::npos);
   size_t idx = tmp.find("/");

   if (idx != std::string::npos)
        return tmp.substr(0,idx+1);

   return std::string();
}

// returns the parent dir of a given path
// Does *NOT* check for validity of path
std::string ParentDir(const std::string& pwd);

std::string StringOverlap(const std::string& str1, const std::string& str2); 

inline bool StringCaseCmp(const std::string& str1, const std::string& str2) 
{
    return strcasecmp(str1.c_str(), str2.c_str()) < 0; 
}

inline bool StringCaseEqual(const std::string& str1, const std::string& str2)
{
    return strcasecmp(str1.c_str(), str2.c_str()) == 0;
}

#endif //P__STRINGTOOLS_H

