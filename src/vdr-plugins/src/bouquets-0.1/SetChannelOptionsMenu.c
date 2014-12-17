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

#include "SetChannelOptionsMenu.h"

//-----------------------------------------------------------
//---------------cSetChannelOptionsMenu----------------------
//-----------------------------------------------------------

cSetChannelOptionsMenu::cSetChannelOptionsMenu(bool & selectionchanched, std::string title):cOsdMenu(title.c_str()), onlyRadioChannels(0), onlyDecryptedChannels(0),
onlyHDChannels(0),
selectionChanched(selectionchanched)
{
    data = Setup;
    SetCols(20, 20);
    Set();
}

eOSState cSetChannelOptionsMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (Key == kOk)
    {
        Store();
        return osBack;
    }

    if (Key == kBack)
    {
        return osBack;
    }

    return state;
}

void cSetChannelOptionsMenu::Set()
{
    std::vector < std::string > options;

    channelViewModeTexts[0] = tr("channellist");
    channelViewModeTexts[1] = tr("current bouquet");
    channelViewModeTexts[2] = tr("bouquet list");


    SetTitle(tr("Channellist: set filters"));

    AddFloatingText(tr("Please select the content of the channellist"), 48);

    Add(new cOsdItem("", osUnknown, false));    // blank line

    options.push_back(tr("All channels"));
    options.push_back(tr("Only TV channels"));
    options.push_back(tr("Only radio channels"));
    options.push_back(tr("non encrypted channels"));
    //options.push_back(tr("non encrypted TV channels"));
    //options.push_back(tr("Only encrypted radio channels"));
    //options.push_back(tr("HD channels"));

    char buf[256];
    //TODO: use stringstreams
    for (uint i = 0; i < options.size(); ++i)
    {
        sprintf(buf, "%d %s", i + 1, options[i].c_str());
        Add(new cOsdItem(buf, osUnknown, true));
    }

    Add(new cOsdItem("", osUnknown, false));    // blank line
    Add(new
        cMenuEditStraItem(tr("Setup.OSD$Channellist starts with"), &data.UseBouquetList,
                          3, channelViewModeTexts));

    SetCurrent(Get(Current()));
    Display();
}

void cSetChannelOptionsMenu::Store()
{
    //printf("--------------cSetChannelOptionsMenu::Store()-------------------\n");
    std::string option = Get(Current())->Text();

    if (option.find(tr("Only radio channels")) == 2)
    {
        data.OnlyRadioChannels = 1;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }
    else if (option.find(tr("Only TV channels")) == 2)
    {
        data.OnlyRadioChannels = 2;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }
    else if (option.find(tr("non encrypted channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 2;
        data.OnlyHDChannels = 0;
    }
    else if (option.find(tr("non encrypted TV channels")) == 2)
    {
        data.OnlyRadioChannels = 2;
        data.OnlyEncryptedChannels = 2;
        data.OnlyHDChannels = 0;
    }
    else if (option.find(tr("encrypted radio channels")) == 2)
    {
        data.OnlyRadioChannels = 1;
        data.OnlyEncryptedChannels = 1;
        data.OnlyHDChannels = 0;
    }
    else if (option.find(tr("HD channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 1;
    }
    else if (option.find(tr("All channels")) == 2)
    {
        data.OnlyRadioChannels = 0;
        data.OnlyEncryptedChannels = 0;
        data.OnlyHDChannels = 0;
    }

    selectionChanched = true;
    Setup = data;
    Setup.Save();
}
