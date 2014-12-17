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

#ifndef P__SERVICESTRUCTS_H
#define P__SERVICESTRUCTS_H

#include <vector>
#include <string>

enum browseMode
{
    restricted, expert, autostart, browseplaylist, browseplaylist2, 
    external, copyplaylist, selectplaylist, saveplaylistas
};

enum xineControl
{
    xine_browse, xine_edit, xine_exit, xine_copy_playlist, xine_select_playlist, 
    xine_save_playlist, xine_save_playlistas
};

enum browserCaller
{
    normal, mediaplayer, svdrp, plugin, xine
}; 

enum externalAction
{
    bla, fasel, lall
};

enum sortMode
{
    byalph, bysize
};

struct FileBrowserSetStartDir
{
    char const *dir;
    char const *file;
    int instance;
};

struct FileBrowserSetPlayList
{
    char const *file;
    int instance;
};

struct FileBrowserSetCaller
{
    browserCaller caller; 
    int instance;
};

struct FileBrowserControl
{
    xineControl cmd; 
    int instance;
};

struct FileBrowserSetPlayListEntry
{
    char const *entry; 
    int instance;
};

struct FileBrowserSetPlayListEntries
{
    std::string name;
    std::vector<std::string > entries; 
    int instance;
};

struct FileBrowserSavePlaylist
{
    std::string name; 
    int instance;
};

struct FileBrowserInstance
{
   int instance;
   bool merge;
};

const int MAX_INSTANCES = 64;

struct FileBrowserInstances
{
   int num_instances;
   int instances[MAX_INSTANCES];
};

struct FileBrowserPicPlayerInfo
{
   int active;
};

struct FileBrowserExternalActionInfo
{
    externalAction type; 
    std::string titleString;
    std::string actionString; 
    std::string returnString1;
    std::string returnString2;
    char *returnString3;

    std::vector<std::string> startDirs_;
};

struct FileBrowserExternalAction
{
    std::string startDir;
    std::vector<std::string> startDirs;
    std::string startFile;

    FileBrowserExternalActionInfo *info;
};

struct FileBrowserGetEntries
{
    std::vector<std::string> entries;
};

#endif //P__SERVICESTRUCTS_H

