/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 *                                                                         *
 ***************************************************************************
 *
 * channellistbackupmenu.h
 *
 ***************************************************************************/

#ifndef CHANNELLISTBACKUPMENU_H
#define CHANNELLISTBACKUPMENU_H

#include <vdr/osdbase.h>

#include <string>

enum eMode { eImport, eExport };

class cChannellistBackupMenu:public cOsdMenu
{
  private:
    std::vector < char *>path_;
    char *file_;
    eMode mode_;
    bool isTargetFav; // export/import favourites.conf

    bool RestoreChannellist(const char *filename);
    bool BackupChannellist();

  public:
      cChannellistBackupMenu(eMode mode_, bool isTargetFav_=false);
     ~cChannellistBackupMenu();

    void Set();
    eOSState ProcessKey(eKeys Key);
};

class cDirectoryItem:public cOsdItem
{
  private:
    std::string path_;
  public:
      cDirectoryItem(const char *path);
     ~cDirectoryItem();

    const char *Path();
};

class cFileItem:public cOsdItem
{
  public:
    cFileItem(const char *text, bool active = true);
    ~cFileItem();
};

#endif
