/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 * install_connections.c
 *
 ***************************************************************************/

#include "install_connections.h"
#include "search_netceiver.h"

//#######################################################################
// cInstallConnectionsAVG
//#######################################################################

cInstallConnectionsAVG::cInstallConnectionsAVG(): cInstallSubMenu(tr("Reception"))
{
    countdown_started = false;
    NetCeiverFound = false;
    vlan_id = 1; // default vlan
    
    // start looking for netceiver in the background
    cSearchNetCeiver::GetInstance().Start(); 
    
    Set();
}

cInstallConnectionsAVG::~cInstallConnectionsAVG()
{
    cSearchNetCeiver::GetInstance().Stop();
    
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
}

void cInstallConnectionsAVG::Set()
{
    Clear();
    
    if (NetCeiverFound) 
        SetNetCeiverFound();
    else if (status.finished && !NetCeiverFound) 
        SetNoNetCeiverFound();
    else
        SetSearchingForNetCeiver();
    
    Display();
}

void MakeProgressBar(char* const s, int len, float percent)
{
    char *p = s;
    int total_number_of_bars = len-3; // first and last chars are '[', ']' resp and null excluded
    int number_of_bars = (int)(total_number_of_bars*percent);
    int i;
    
    for (i=0; i < number_of_bars; ++i)      p[1+i] = '|';
    for ( ; i < total_number_of_bars; ++i)  p[1+i] = ' ';
    
    p[0]     = '[';
    p[len-2] = ']';
    
    p[len-1] = 0;
}

void ShowNetceiverConnectionImage(int h)
{
    struct StructImage Image;
    Image.x = 209;
    Image.y = h;
    Image.w = Image.h = 0;
    Image.blend = true;
    Image.slot = 0;
    bool isICE = (cPluginManager::GetPlugin("reelice") != NULL);
    
    if (IsClient()) {
        Image.x = 290; // centre the image
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/NetClient2", "NetClient2-Ethernet.png");
    }
    else if (IsAvantgarde()) {
        if (isICE)
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-left-NetCeiver-connection.png");
        else
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-System-left.png");
    } else {
        esyslog("(%s:%d) Neither Client not Avantgarde?", __FILE__, __LINE__);
    }
    
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    
    if (IsClient()) // has one image to show
        return;
    
    Image.x = 370;
    Image.w = 200;
    Image.h = 116;
    Image.slot = 1;
    
    if (IsClient()) {
        
    } else if (IsAvantgarde()) {
        if (isICE)
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-right-plain.png");
        else
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-System-right.png");
    } else {
        esyslog("(%s:%d) Neither Client not Avantgarde?", __FILE__, __LINE__);
    }
    
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    
}

void cInstallConnectionsAVG::SetSearchingForNetCeiver()
{
    SetTitle(tr("Reception"));
    SetHelp(NULL, tr("Back"), tr("Skip"));
    
    AddFloatingText(tr("Searching for NetCeiver in network. Please wait..."), MAX_LETTER_PER_LINE);
    
    ShowNetceiverConnectionImage(150);
    
    for (int i=0; i < 8; i++)
    Add(new cOsdItem("", osUnknown, false));
    
    char bar[16] = {0};
    MakeProgressBar(bar, 16, vlan_id*1.0/MAX_VLAN_ID);
    if (bar[0]) 
        Add(new cOsdItem(bar, osUnknown, false));
    
    cString text;
    if (status.local_search)
        text = tr("Searching for directly connected NetCeiver...");
    else
        text = cString::sprintf(tr("Searching for NetCeiver in VLAN %d"),
                                vlan_id);
    
    AddFloatingText(text, MAX_LETTER_PER_LINE);
}

void cInstallConnectionsAVG::SetNetCeiverFound()
{
    SetSearchingForNetCeiver();
    
    SetTitle(tr("Reception"));
    
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    
    SetCols(3);

    cString text;
    if (status.local_search)
        text = cString::sprintf(">\t%s", tr("Internal NetCeiver found"));
    else
        text = cString::sprintf(">\t%s %d", tr("NetCeiver found in VLAN"), vlan_id);
    
    AddFloatingText(text, MAX_LETTER_PER_LINE);
}

void cInstallConnectionsAVG::SetNoNetCeiverFound()
{
    SetTitle(tr("System error!"));
    
    Add(new cOsdItem(tr("No NetCeiver found!"), osUnknown, false));
    AddFloatingText(tr("Please check if the system cable is connected properly."), MAX_LETTER_PER_LINE);

    // Draw Image
    ShowNetceiverConnectionImage(315);
    
    SetHelp(tr("Rescan"), tr("Back"), tr("Skip"));
    //Display();
}

bool cInstallConnectionsAVG::Save()
{
    return false;
}

eOSState cInstallConnectionsAVG::ProcessKey(eKeys Key)
{
    //Display();                  // Redraw since the VideoOut-Menu switchs Video-Mode and the following menus may disapears
    
    cSearchNetCeiver::GetInstance().SearchStatus(status, NetCeiverFound);
    vlan_id = status.vlan_id;
    Set(); // update menu to show status
    
    if (NORMALKEY(Key) == kOk && NetCeiverFound)
        return osUser1;
    
    if (NetCeiverFound) {
        if (!countdown_started) {
            wait_after_netcv_found.Set(2000); // 2 seconds
            countdown_started = true;
        } else if (wait_after_netcv_found.TimedOut()) 
            return osUser1; // go to next step
    } // if
    else if (!NetCeiverFound && status.finished) { // netceiver not found
        // start looking for netceiver in the background again
        if (NORMALKEY(Key) == kRed)
            cSearchNetCeiver::GetInstance().Start();
    }
    
    return osUnknown;
}
