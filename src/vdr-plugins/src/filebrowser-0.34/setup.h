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

#ifndef FILEBROWSER_SETUP_H
#define FILEBROWSER_SETUP_H

#include <vdr/plugin.h>

struct cFileBrowserSetup 
{
    cFileBrowserSetup();
    bool SetupParse(const char *Name, const char *Value); 
    //slide show options
    int displayFileNames;
    int slideShow;
    int random;
    int cyclic;
    int switchTime;
    //playlist options
    int playAll;
    std::string AudioFileTypes;
    std::string VideoFileTypes;
    std::string PictureFileTypes;
};

extern cFileBrowserSetup fileBrowserSetup;

class cFileBrowserSetupPage: public cMenuSetupPage
{
public:
    cFileBrowserSetupPage(cFileCache *cache);
    /*override*/ ~cFileBrowserSetupPage(){};

protected: 
    /*override*/ eOSState ProcessKey(eKeys Key);
    /*override*/ void Store();

private:
    cFileCache *cache_;
    cFileBrowserSetup data_;
    void Set();
    void ClearCache();
};

#endif // FILEBROWSER_SETUP_H
