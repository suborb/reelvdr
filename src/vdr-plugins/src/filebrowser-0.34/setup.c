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

#include <vdr/config.h>

#include "filecache.h"
#include "setup.h"

//----------------------------------------------------
//--------------cFileBrowserSetup---------------------
//----------------------------------------------------

cFileBrowserSetup fileBrowserSetup;

cFileBrowserSetup::cFileBrowserSetup()
:displayFileNames(1), slideShow(1), random(0), switchTime(5), playAll(false)
{
#ifdef RBMINI
    AudioFileTypes = "mp3 pls";
    VideoFileTypes = "";
    PictureFileTypes = "jpeg jpg";
#else
    AudioFileTypes = "flac m4a mp2 mp3 ogg pcm wav pls"; //FIXME: pls is not an audio filetype - workaround 
    VideoFileTypes = "avi divx iso mkv mpeg mpg mp4 mov pes rec ts mts m2ts vob vcd vdr pls";
    PictureFileTypes = "bmp jpeg jpg tif tiff png";
#endif
}

bool cFileBrowserSetup::SetupParse(const char *Name, const char *Value)
{
    if(std::string(Name) == "DisplayFileNames")
    {
        displayFileNames = atoi(Value);
    }
    else if(std::string(Name) == "SlideShow")
    {
        slideShow  = atoi(Value);
    }
    else if(std::string(Name) == "Random")
    {
        random  = atoi(Value);
    }
    else if(std::string(Name) == "Cyclic")
    {
        cyclic  = atoi(Value);
    }
    else if(std::string(Name) == "SwitchTime")
    {
        switchTime = atoi(Value);
    }
    /*else if(std::string(Name) == "PlayAll")
    {
        switchTime = atoi(Value);
    }*/
    else if(std::string(Name) == "AudioFileTypes")
    {
        //AudioFileTypes = Value;
    }
    else if(std::string(Name) == "VideoFileTypes")
    {
        //VideoFileTypes = Value;
    }
    else if(std::string(Name) == "PictureFileTypes")
    {
        //PictureFileTypes = Value;
    }
    else
    { 
        return false;
    }
    return true;
}

//----------------------------------------------------
//--------------cFileBrowserSetupPage-----------------
//----------------------------------------------------

cFileBrowserSetupPage::cFileBrowserSetupPage(cFileCache *cache)
:cache_(cache), data_(fileBrowserSetup)
{ 
    SetCols(33);
    Set();
}

void cFileBrowserSetupPage::Set()
{
    Clear();

    Add(new cOsdItem(tr("Options for image viewer"), osUnknown , false));
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cMenuEditBoolItem(tr("Display image file names"), &data_.displayFileNames, tr("no"), tr("yes")));
    Add(new cMenuEditBoolItem(tr("Slide show"), &data_.slideShow, tr("no"), tr("yes"))); 
    Add(new cMenuEditBoolItem(tr("Random choice"), &data_.random, tr("no"), tr("yes")));
    Add(new cMenuEditBoolItem(tr("Cyclic display"), &data_.cyclic, tr("no"), tr("yes")));
    Add(new cMenuEditIntItem(tr("Switch time (s)"), &data_.switchTime, 1, 60)); 
    //Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    //Add(new cOsdItem(tr("Playlist Options"), osUnknown , false));  
    //Add(new cMenuEditBoolItem(tr("Play all items"), &data_.playAll, tr("no"), tr("yes")));

    SetHelp(tr("Clear cache"));
    SetCurrent(Get(Current()));
    Display();
}

eOSState cFileBrowserSetupPage::ProcessKey(eKeys Key)
{
    eOSState state = cMenuSetupPage::ProcessKey(Key);

    if (state == osUnknown)
    {
        switch (Key)
        {
            case kRed:
            ClearCache();
            break;
        default:
            break;
        }
    }
    return state;
}

void cFileBrowserSetupPage::Store(void)
{
    fileBrowserSetup = data_;

    SetupStore("DisplayFileNames", fileBrowserSetup.displayFileNames);
    SetupStore("SlideShow", fileBrowserSetup.slideShow); 
    SetupStore("Random", fileBrowserSetup.random);
    SetupStore("Cyclic", fileBrowserSetup.cyclic);
    SetupStore("SwitchTime", fileBrowserSetup.switchTime);
    //SetupStore("PlayAll", fileBrowserSetup.playAll);
    SetupStore("AudioFileTypes", fileBrowserSetup.AudioFileTypes.c_str());
    SetupStore("VideoFileTypes", fileBrowserSetup.VideoFileTypes.c_str());
    SetupStore("PictureFileTypes", fileBrowserSetup.PictureFileTypes.c_str());
}

void cFileBrowserSetupPage::ClearCache()
{
    cache_->ClearCache();
    Skins.Message(mtInfo, tr("Cache cleared"));
}

