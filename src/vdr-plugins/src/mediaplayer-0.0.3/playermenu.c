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

//playermenu.c

#include <vdr/osd.h>
#include <vdr/remote.h>

#include "mediaplayer.h"
#include "playermenu.h"
#include "def.h"

#include <vector>

cMediaPlayerStatus cMenuMediaPlayer::status;

bool IsMounted(const std::string &path)
{
    std::string cmd = "/usr/bin/sudo -s " + std::string(MOUNT_SH) + " status_ipod " + path;
    if (SystemExec(cmd.c_str()) == 0)
    {
	    return true;
    }
	return false;
}

cMenuMediaPlayer::cMenuMediaPlayer (const std::string title)
: cOsdMenu(title.c_str())
{
#ifdef RBLITE
    SetHelp(tr("Recordings"), tr("Music"), tr("Images"), tr("Videos"));
#else
    SetHelp(tr("Recordings"), tr("My Music"), tr("My Images"), tr("My Videos"));
#endif

    char strBuff[256];
    strBuff[0] = 131;
    strBuff[1] = ' ';

    strcpy(strBuff+2, tr("Browser"));
    Add(new cOsdItem(strBuff, osUnknown, true));

    strcpy(strBuff+2, tr("Rotor"));
    Add(new cOsdItem(strBuff, osUnknown, true));

    strcpy(strBuff+2, tr("My Images"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    strcpy(strBuff+2, tr("My Music"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    strcpy(strBuff+2, tr("My Videos"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    strcpy(strBuff+2, tr("My Recordings"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    strcpy(strBuff+2, tr("Play CD/DVD"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    strcpy(strBuff+2, tr("Internet radio"));
    Add(new cOsdItem(strBuff, osUnknown, true));
    if (IsMounted("/media/Apple_*"))
    {
        strcpy(strBuff+2, tr("Play Ipod"));
        Add(new cOsdItem(strBuff, osUnknown, true));
    }

    Init();
}

eOSState cMenuMediaPlayer::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;

    state = cOsdMenu::ProcessKey(Key);

    ReceiveBrowserAction();

    if(state == osUnknown)
    {
        
        switch (Key)
        {
            case kOk:
                printf("Text = %s\n", Get(Current())->Text());     
                if(std::string(Get(Current())->Text()).find(tr("Browser")) == 2)
                {
                    return StartBrowser();
                }     
                if(std::string(Get(Current())->Text()).find(tr("Rotor")) == 2)
                {
                    return StartRotor();
                }
                if(std::string(Get(Current())->Text()).find(tr("My Music")) == 2)
                {
                    return PlayMusic();
                }
                else if(std::string(Get(Current())->Text()).find(tr("My Images")) == 2)
                {
                    return PlayImages();
                }
                else if(std::string(Get(Current())->Text()).find(tr("My Videos")) == 2)
                {
                    return PlayMovies();
                }
                else if(std::string(Get(Current())->Text()).find(tr("My Recordings")) == 2)
                {
                    return PlayRecordings();
                }
                else if(std::string(Get(Current())->Text()).find(tr("Play CD/DVD")) == 2)
                {
                    return PlayCdDvd();
                }
                else if(std::string(Get(Current())->Text()).find(tr("Internet radio")) == 2)
                {
                    return PlayInternetRadio();
                }
                else if(std::string(Get(Current())->Text()).find(tr("Play Ipod")) == 2)
                {
                    return PlayIpod();
                }
            break;
            case kBack:
                status.Shutdown();
                state = osEnd;
                break;
            case kRed:
                return PlayRecordings();
                break;
            case kGreen:
                return PlayMusic();
                break;
            case kYellow:
                return PlayImages();
                break;
            case kBlue:
                return PlayMovies();
                break;
            default:
                state = osContinue;
                break;
        }
    }
    return state;
}

void cMenuMediaPlayer::ReceiveBrowserAction()
{
    printf("---------ReceiveBrowserAction = %s---------\n", info.returnString1.c_str());
    if(info.returnString1 != "")
    {
        Add(new cOsdItem(info.returnString1.c_str(), osUnknown, true));
        Display();
        info.returnString1 = "";
    }
}

//start filebrowser in pictures directory, external mode
eOSState cMenuMediaPlayer::StartBrowser()
{   
    FileBrowserExternalActionInfo tmp=
    {
        fasel,
        "blafasellaberlall auswählen",
        "",
        std::string(),
        std::string()
    };
    info = tmp;

    /*FileBrowserExternalAction data =
    {
        "/media/hd/pictures",
        "",
        &info
    };
    cPluginManager::CallAllServices("Filebrowser external action", &data);*/

    std::vector<std::string> dirs;    
    dirs.push_back(std::string("/media/hd/pictures/Klaus_Bilder"));
    dirs.push_back(std::string("/media/hd/pictures"));
    dirs.push_back(std::string("/media/hd/music"));

    FileBrowserExternalAction data =
    {
        "/media/hd/pictures", 
        dirs,
        "",
        &info
    };
    cPluginManager::CallAllServices("Filebrowser external action", &data);

    //cRemote::CallPlugin("filebrowser");
    //status.Shutdown();
    //return osEnd;
    cPlugin *filebrowser = cPluginManager::GetPlugin("filebrowser");
    if(filebrowser)
    {
        return AddSubMenu(static_cast<cOsdMenu*>(filebrowser->MainMenuAction()));
    }
    return osContinue;
}

//start roto plugin
eOSState cMenuMediaPlayer::StartRotor()
{    
    cRemote::CallPlugin("netcvrotor");
    status.Shutdown();
    return osEnd;
}

//start filebrowser in music directory, standard mode
eOSState cMenuMediaPlayer::PlayMusic()
{
    if (cPluginManager::CallAllServices("Show All Audio Files"))
        return osContinue;
    struct
    {
        char const *dir;
        char const *file;
    } startDirData = {
                         MUSICDIR,
                          ""
                     };

    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);

    return CallFileBrowser();
}

//start filebrowser in images directory, standard mode
eOSState cMenuMediaPlayer::PlayImages()
{
    if (cPluginManager::CallAllServices("Show All Photo Files"))
    return osContinue;
    struct
    {
        char const *dir;
        char const *file;
    } startDirData = {
                         IMAGESDIR,
                          ""
                     };

    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);

    return CallFileBrowser();
}

//start filebrowser in movies directory, standard mode
eOSState cMenuMediaPlayer::PlayMovies()
{
    if (cPluginManager::CallAllServices("Show All Video Files"))
        return osContinue;

    struct
    {
        char const *dir;
        char const *file;
    } startDirData = {
                         MOVIESDIR,
                          ""
                     };

    cPluginManager::CallAllServices("Filebrowser set startdir", &startDirData);

    return CallFileBrowser();
}

//call filebrowser
eOSState cMenuMediaPlayer::CallFileBrowser()
{
    enum browserCaller
    {
        normal, autorestart, svdrp, mediaplayer, xine
    };
    struct
    {
        browserCaller caller;
    } callerData = {
                       mediaplayer
                   };
    cPluginManager::CallAllServices("Filebrowser set caller", &callerData);

    cRemote::CallPlugin("filebrowser");
    status.Hide(Get(Current())->Text());
    return osEnd;
}

//call extrecmenu Plugin
eOSState cMenuMediaPlayer::PlayRecordings()
{
    cRemote::CallPlugin("extrecmenu");
    status.Shutdown();
    return osEnd;
}

//call mediad Plugin
eOSState cMenuMediaPlayer::PlayCdDvd()
{
#ifdef RBLITE
    cRemote::CallPlugin("vdrcd");
#else
    cRemote::CallPlugin("mediad");
#endif
    status.Shutdown();
    return osEnd;
}

//call shoutcast Plugin
eOSState cMenuMediaPlayer::PlayInternetRadio()
{
    cRemote::CallPlugin("shoutcast");
    status.Shutdown();
    return osEnd;
}

//call ipod Plugin
eOSState cMenuMediaPlayer::PlayIpod()
{
    cRemote::CallPlugin("ipod");
    status.Shutdown();
    return osEnd;
}

void cMenuMediaPlayer::Init()
{
    if(status.GetCurrent() != "")
    {
        for (int i= 0; i < Count(); i++)
	{
            if(std::string(Get(i)->Text()) == status.GetCurrent())
            {
                SetCurrent(Get(i));
                break;
            }
	}
    }
}

