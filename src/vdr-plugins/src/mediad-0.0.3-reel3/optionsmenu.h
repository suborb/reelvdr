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

#ifndef __OPTIONS_MENU_H
#define __OPTIONS_MENU_H

#include <string>

#include <vdr/menuitems.h>
#include <vdr/menu.h>

#include "mediaplugin.h"

class optionsItem;

//list options, display checkmark next to current item, only entries of type optionsItem allowed
class optionsMenu: 
    public cOsdMenu 
{
    public:
        optionsMenu(std::string title);
        /*override*/ ~optionsMenu(){}
        bool IsCurrent(optionsItem *item);
        optionsItem *GetCurrentItem();

    protected:
        void RefreshItems();
};

inline optionsMenu::optionsMenu(std::string title)
: cOsdMenu(title.c_str())
{
    SetCols(5);
}

class optionsItem :
public cOsdItem
{ 
    public:
            optionsItem(std::string name, std::string id, optionsMenu *menu);
            /*override*/ ~optionsItem(){};
            /*override*/ void Set(); 
            std::string GetName();
            std::string GetId();
    
    private:
            std::string name_;
            std::string id_;
            optionsMenu *menu_;
    
            //forbid copying
            optionsItem(const optionsItem &);
            optionsItem &operator=(const optionsItem &);
};

inline std::string optionsItem::GetName()
{
    return name_;
}

inline std::string optionsItem::GetId()
{
    return id_;
}

//list matched plugins
class requestPluginMenu: 
    public optionsMenu 
{
    public:
        requestPluginMenu(std::vector<mediaplugin*> matchedPlugins, std::string title);
        /*override*/ ~requestPluginMenu();
        /*override*/ eOSState ProcessKey(eKeys Key);

        static std::string GetPlugin();
        static void SetPlugin(std::string plugin);

    private:
        eOSState ExecutePlugin(std::string plugin);
        void Umount();
        std::string EntryToPluginDesc(std::string entry);
        std::string PluginDescToEntry(std::string plugin);
        std::string PluginNameToEntry(std::string plugin);
        bool mediadStarted_;
        static std::string plugin_;
        std::vector<mediaplugin*> matchedPlugins_;
};

//list options vor dvd
/*class requestDvdModeMenu:
    public optionsMenu 
{
    public:
        requestDvdModeMenu();
        ~requestDvdModeMenu();
        eOSState ProcessKey(eKeys Key);
    private:
        eOSState PlayDvd();
        eOSState CopyDvd();
        bool playerStarted_;
};*/

#endif //__OPTIONS_H


