/****************************************************************************
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

#include <vector>

#include "vdr/i18n.h"
#include "vdr/player.h"
#include "vdr/remote.h"

#include "setup.h"
#include "optionsmenu.h"
#include "mediaplugin.h"


const int pluginNameTableSize = 14;
const char *pluginNameTable[][2] =
{
    {"vdr_rec",     trNOOP("Play recordings")},
    {"dmh_archive", trNOOP("Show archive")},
    {"vcd",         trNOOP("Play video CD")},
    {"mplayer",     trNOOP("Play music")},
    {"mp3",         trNOOP("Play music")},
    {"ripit",       trNOOP("Archive audio CD")},
    {"xinemediaplayer", trNOOP("Play multimedia")},
    {"music",       trNOOP("Play music")},
    {"dvd",         trNOOP("Play DVD")},
    {"pcd",         trNOOP("Play photo CD")},
    {"cdda",        trNOOP("Play audio CD")},
    {"burn",        trNOOP("Burn recordings")},
    {"dvdswitch",   trNOOP("Copy DVD")}, 
    {"filebrowser",   trNOOP("Browse data")}
};

const int pluginDescTableSize = 15;
const char *pluginDescTable[][2] =
{
   {"VDR Recording",    trNOOP("Play recordings")},
   {"DMH DVD",          trNOOP("Show archive")},
   {"SVCD",             trNOOP("Play video CD")},
   {"AVI movie disc",   trNOOP("Play video")},
   {"MPG movie disc",   trNOOP("Play video")},
   {"MP3 player",       trNOOP("Play music")},
   {"OGGVorbis player", trNOOP("Play music")},
   {"Audio CD",         trNOOP("Play audio CD")},
   {"Audio CD for ripping", trNOOP("Archive audio CD")},
   {"Video DVD",        trNOOP("Play video DVD")},
   {"Photo CD",         trNOOP("Play photo CD")},
   {"Audio CD Player",  trNOOP("Play audio CD")},
   {"DVD creation",     trNOOP("Burn recordings")},
   {"Copy DVD",         trNOOP("Copy DVD")}, 
   {"Browse data",  trNOOP("Browse data")}
};
//-----------optionsMenu---------------------------------------------

bool optionsMenu::IsCurrent(optionsItem *item)
{
    return (Get(Current()) == item);
}

optionsItem *optionsMenu::GetCurrentItem()
{
    return static_cast<optionsItem *>(Get(Current()));
}

void optionsMenu::RefreshItems()
{
    for(int i = 0; i < Count(); i++)
    {
        Get(i)->Set();
    }
    Display();
}

//-----------optionsItem---------------------------------------------

optionsItem::optionsItem(std::string name, std::string id, optionsMenu *menu)
: cOsdItem(name.c_str()), name_(name), id_(id), menu_(menu)
{
    Set();
}

void optionsItem::Set()
{
    if(menu_->IsCurrent(this))
    {
        SetText((std::string("\x87    \t") + name_).c_str());
    }
    else
    {
        SetText((std::string("\t") + name_).c_str());
    }
}

//-----------requestPluginMenu---------------------------------------

std::string requestPluginMenu::plugin_ = "";

requestPluginMenu::requestPluginMenu(std::vector<mediaplugin*> matchedPlugins, std::string title)
: optionsMenu(title), mediadStarted_(false)
{
     matchedPlugins_ = matchedPlugins;
     for(uint i = 0; i < matchedPlugins.size(); ++i)
     {
         // cout << "--requestPluginMenu, Add Description: " << matchedPlugins[i]->getDescription() << endl;
         // cout << "--requestPluginMenu, Add Name: " << matchedPlugins[i]->getName() << endl;
         std::string entryName;
         if(matchedPlugins[i]->getDescription())
         {
             entryName = tr(PluginDescToEntry(matchedPlugins[i]->getDescription()).c_str());
         }
         else
         {
             entryName = tr(PluginNameToEntry(matchedPlugins[i]->getName()).c_str()); 
         }
         std::string entryId = matchedPlugins[i]->getName();
         Add(new optionsItem(entryName, entryId, this));
     }
     SetCurrent(Get(0));
     SetCols(3);
     RefreshItems();
}

requestPluginMenu::~requestPluginMenu()
{
    if(!mediadStarted_)
    {
        cControl::Shutdown();
        Umount();
    } 
}

std::string requestPluginMenu::GetPlugin()
{
    return plugin_;
}

void requestPluginMenu::SetPlugin(std::string plugin)
{
    plugin_ = plugin;
}

eOSState requestPluginMenu::ProcessKey(eKeys Key)
{
    eOSState state = osUnknown;

    state = cOsdMenu::ProcessKey(Key);

    if(Key == kUp || Key == kDown)
    {
        RefreshItems();
    }

    if(state == osUnknown)
    {
        switch (Key)
        {
            case kOk:
                return ExecutePlugin(GetCurrentItem()->GetId());
            case kBack:
                 plugin_ = "";
                 state = osEnd;
                 break;
            default:
                break;
        }
   }
   return state;
}

void requestPluginMenu::Umount()
{
     for(uint i = 0; i < matchedPlugins_.size(); ++i)
     { 
        if( matchedPlugins_[i]->needsUmount())
        {
            matchedPlugins_[i]->preStartUmount();
            return;
        }
     }
}

eOSState requestPluginMenu::ExecutePlugin(std::string plugin)
{
    // cout << "--------requestPluginMenu::ExecutePlugin " << plugin.c_str() <<  "----------" << endl;
    mediaplugin *pPlugin = VdrcdSetup.mediaplugins.First();
    while(pPlugin)
    {
            if (pPlugin->getName() == plugin)
            {
                plugin_ = plugin;
            }
            pPlugin = VdrcdSetup.mediaplugins.Next(pPlugin);
    }
    if(plugin_ != "")
    {
        //restart mediad to actually execute mediaplugin
        mediadStarted_ = true;
        cRemote::CallPlugin("mediad");
    }
    // printf("--------------requestPluginMenu::ExecutePlugin, vor return---------------\n");
    return osEnd;
}

std::string requestPluginMenu::EntryToPluginDesc(std::string entry)
{
    for(int i = 0; i < pluginDescTableSize; ++i)
    {
        if(entry == pluginDescTable[i][1])
        {
            return pluginDescTable[i][0];
        }
    }
    return "";
}

std::string requestPluginMenu::PluginDescToEntry(std::string plugin)
{
    for(int i = 0; i < pluginDescTableSize; ++i)
    {
        if(plugin == pluginDescTable[i][0])
        {
            return pluginDescTable[i][1];
        }
    }
    return "";
}

std::string requestPluginMenu::PluginNameToEntry(std::string plugin)
{
    for(int i = 0; i < pluginNameTableSize; ++i)
    {
        if(plugin == pluginNameTable[i][0])
        {
            return pluginNameTable[i][1];
        }
    }
    return "";
}
