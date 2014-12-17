/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia;  Author:  Markus Hahn          *
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
 *
 ***************************************************************************
 *
 *   dirfiles.h: low-level filebrowser for config files.  Reelbox special
 *
 ***************************************************************************/

#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <vdr/channels.h>
#include <vdr/device.h>
#if VDRVERSNUM < 10716
#include <vdr/submenu.h>        // Filefactory
#else
#include <vdr/config.h>         // Filefactory
#endif

#include "transponders.h"
#include "channelscan.h"
#include "csmenu.h"
#include "debug.h"


using std::string;
using std::cout;
using std::endl;

#define DIRFILE   ?'d':'f'


cDirectoryFiles DirectoryFiles;

// -- Class cDirectoryEntry --------------------------------------
cDirectoryEntry::cDirectoryEntry(const std::string & FileName, bool IsDir)
{
    fileName_ = FileName;
    titleBuffer_ = fileName_.substr(FileName.find_last_of('/') + 1);
    isDirectory_ = IsDir;
    toEdit_ = false;            // Backup or New
    StripEnding();              // einstelbar?
}

void cDirectoryEntry::StripEnding()
{
    string::size_type pos = titleBuffer_.find('.');
    if (pos != titleBuffer_.npos)
        titleBuffer_.erase(pos);
}

int cDirectoryEntry::Compare(const cListObject & ListObject) const
{
    cDirectoryEntry *de = (cDirectoryEntry *) & ListObject;
    if (de->IsDirectory() == IsDirectory())
    {
        string title = Title();
        int ret = strcasecmp(title.c_str(), de->Title());
        return ret;
    }
    return IsDirectory()? -100 : 100;
}

// ----------- Class cDirectoryFiles ------------------------

cDirectoryFiles::cDirectoryFiles()
{
}

bool cDirectoryFiles::Load(bool oldPath)
{

    if (!oldPath)
    {
        path_ = setup::FileNameFactory("channels");
        return Load(path_);
    }
    else
    {
        string tmp = oldPath_;
        oldPath_ = string(oldPath_, oldPath_.find_last_of('/'));
        return Load(tmp);
    }
}

bool cDirectoryFiles::Load(string & Path)
{
    Clear();

    DIR *dirp;
    struct dirent *dp;
    struct stat buf;
    if (!Path.empty())
    {
        oldPath_ = path_;
        path_ = Path;
    }
    else                        // reload
        Path = path_;

    if ((dirp = opendir(Path.c_str())) == NULL)
    {
        esyslog(" error open %s ", Path.c_str());
        return false;
    }

    while ((dp = readdir(dirp)) != NULL)
    {
        string dName = dp->d_name;
        if (dName.find('.') == 0)
            continue;

        string path = Path + "/" + dName;

        if (stat(path.c_str(), &buf) != 0)
        {
            int err = errno;
            esyslog(" Error: %s errno %d ", path.c_str(), err);
        }

        if (S_ISDIR(buf.st_mode))
            Add(new cDirectoryEntry(path, true));

        if (S_ISREG(buf.st_mode))
        {
            if (dName.find(".conf") != string::npos)
                Add(new cDirectoryEntry(path, false));
            else
                isyslog("Channelscan: no (channels).config file %s \n", dName.c_str());
        }
    }

    closedir(dirp);
    Sort();
    return true;
}
