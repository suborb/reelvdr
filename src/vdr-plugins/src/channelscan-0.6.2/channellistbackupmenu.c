/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 parts rewritten by Tobias Bratfisch                *
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
 *                                                                         *
 ***************************************************************************
 *
 * channellistbackupmenu.c
 *
 ***************************************************************************/

#include <vector>
#include <sstream>

#include "channellistbackupmenu.h"
#include <vdr/plugin.h>

#define START_PATH "/media/"
#define CHANNELS_CONF "/etc/vdr/channels.conf"
#define FAV_CONF "/etc/vdr/plugins/reelchannellist/favourites.conf" //avoid hard-coding the path

bool ReloadFavourites()
{
    cPlugin *p = cPluginManager::GetPlugin("reelchannellist");
    if (!p) return false;

    char dummy;
    return p->Service("reload favourites list", &dummy);
}

// class cChannellistBackupMenu ---------------------------------------
cChannellistBackupMenu::cChannellistBackupMenu(eMode mode, bool isTargetFav_):cOsdMenu(tr("Channel lists")), isTargetFav(isTargetFav_)
{
    file_ = NULL;
    mode_ = mode;

    // show vdr's default channels directory when importing channellist
    if (mode_ == eImport)
        path_.push_back(strdup("channels"));

    Set();
    if (mode_ == eExport)
        SetHelp(tr("Backup"));
}

cChannellistBackupMenu::~cChannellistBackupMenu()
{
    if (file_)
        free(file_);
    for (int i = 0; i < (int)path_.size(); ++i)
        free(path_.at(i));
    path_.clear();
}

void cChannellistBackupMenu::Set()
{
    char *buffer;
    char *path;
    DIR *dp, *testdp;
    struct dirent *dirp;
    std::vector < char *>files;
    std::vector < char *>directories;

    Clear();
    SetCols(2);

    if (mode_ == eExport)
        Add(new cOsdItem(tr("Backup channel list to..."), osUnknown, false));
    else
        Add(new cOsdItem(tr("Restore channel list from..."), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    // Get current path, beginning from start-path
    path = strdup(START_PATH);
    for (int i = 0; i < (int)path_.size(); ++i)
    {
        asprintf(&buffer, "%s%s/", path, path_.at(i));
        free(path);
        path = strdup(buffer);
        free(buffer);
    }

    if ((dp = opendir(path)) != NULL)
    {
        while ((dirp = readdir(dp)) != NULL)
        {
            if (*(dirp->d_name) != '.')
            {
                if ((dirp->d_type == DT_DIR) || (dirp->d_type == DT_LNK))
                    directories.push_back(strdup(dirp->d_name));
                else
                {
                    // Test if it's a directory since symbolic link is not always working (why?)
                    asprintf(&buffer, "%s%s", path, dirp->d_name);
                    testdp = opendir(buffer);
                    free(buffer);
                    if (testdp)
                    {
                        directories.push_back(strdup(dirp->d_name));
                        closedir(testdp);
                    }
                    else
                        files.push_back(strdup(dirp->d_name));
                }
            }
        }
        closedir(dp);
    }
    free(path);

    if ((int)path_.size() > 0)
    {
        asprintf(&buffer, "%c\t..", 128);
        Add(new cOsdItem(buffer));
        free(buffer);
    }

    for (int i = 0; i < (int)directories.size(); ++i)
        Add(new cDirectoryItem(directories.at(i)));

    for (int i = 0; i < (int)files.size(); ++i)
    {
        if (mode_ == eExport)
            Add(new cFileItem(files.at(i), false));
        else
            Add(new cFileItem(files.at(i)));
    }

    for (int i = 0; i < (int)directories.size(); ++i)
        free(directories.at(i));
    directories.clear();
    for (int i = 0; i < (int)files.size(); ++i)
        free(files.at(i));
    files.clear();

    Display();
}

bool cChannellistBackupMenu::RestoreChannellist(const char *filename)
{
    bool result;
    std::string buffer;
    std::string path = std::string(START_PATH);

    for (int i = 0; i < (int)path_.size(); ++i)
    {
        buffer = path + path_.at(i) + "/";
        path = buffer;
    }

    buffer = std::string("cp ") + path + filename + " " + std::string(isTargetFav?FAV_CONF:CHANNELS_CONF);
    result = SystemExec(buffer.c_str());

    if (result)
        Skins.Message(mtError, tr("Restore failed!"));
    else
    {
        if (isTargetFav)
        {
            if(ReloadFavourites())
                Skins.Message(mtInfo, tr("Favourites List successfully loaded"));
            else
                Skins.Message(mtError, tr("Invalid file"));
        }
        else if (Channels.Reload(CHANNELS_CONF, false, true))
            Skins.Message(mtInfo, tr("Channel List successfully loaded"));
        else
            Skins.Message(mtError, tr("Invalid file"));
    }

    return !result;
}

bool cChannellistBackupMenu::BackupChannellist()
{
    bool result;
    std::string buffer;
    std::string path = std::string(START_PATH);

    for (int i = 0; i < (int)path_.size(); ++i)
    {
        buffer = path + path_.at(i) + "/";
        path = buffer;
    }

    buffer = std::string("cp ") + std::string(isTargetFav?FAV_CONF:CHANNELS_CONF) + " " + path;
    result = SystemExec(buffer.c_str());

    if (result)
        Skins.Message(mtError, tr("Backup failed!"));
    else
        Skins.Message(mtInfo, tr("Channel List successfully stored"));

    return !result;
}

eOSState cChannellistBackupMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    switch (Key)
    {
    case kOk:
        {
            cDirectoryItem *directoryitem = dynamic_cast < cDirectoryItem * >(Get(Current()));
            if (directoryitem)
            {
                path_.push_back(strdup(directoryitem->Path()));
                Set();
                return osContinue;
            }

            cFileItem *fileitem = dynamic_cast < cFileItem * >(Get(Current()));
            if (fileitem)
            {
                if (RestoreChannellist(fileitem->Text()))
                    return osBack;
                else
                    return osContinue;
            }

            cOsdItem *osditem = Get(Current());
            if (osditem && strcmp(osditem->Text(), ".."))
            {
                free(path_.at(path_.size() - 1));
                path_.pop_back();
                Set();
                return osContinue;
            }

        }
        break;
    case kBack:
        {
            if ((int)path_.size() > 0)
            {
                free(path_.at(path_.size() - 1));
                path_.pop_back();
                Set();
                return osContinue;
            }
        }
        break;
    case kRed:
        if (mode_ == eExport)
        {
            if (BackupChannellist())
                return osBack;
            else
                return osContinue;
        }
        break;
    default:
        break;
    }
    return state;
}

// class cDirectoryItem -----------------------------------------------
cDirectoryItem::cDirectoryItem(const char *path)
{
    std::stringstream buffer;
    buffer << (char)130 << "\t" << path;
    SetText(buffer.str().c_str());
    path_ = path;
}

cDirectoryItem::~cDirectoryItem()
{
}

const char *cDirectoryItem::Path()
{
    return path_.c_str();
};

// class cFileItem ----------------------------------------------------
cFileItem::cFileItem(const char *filename, bool selectable):cOsdItem(filename)
{
    SetSelectable(selectable);
}

cFileItem::~cFileItem()
{
}
