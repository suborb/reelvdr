/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "ttxtsubschannelsettings.h"

#include <vdr/menuitems.h>
#include <vdr/device.h>

#include "ttxtsubspagemenu.h"
#include "ttxtsubsglobals.h"

const char * MODE_CHOICES[4] = {NULL, NULL, NULL, NULL};

cTtxtsubsPageMenu::~cTtxtsubsPageMenu()
{
}

cTtxtsubsPageMenu::cTtxtsubsPageMenu()
  :cOsdMenu(tr("Subtitle Page Settings"), 30)
{
    cDevice *device = cDevice::ActualDevice();
    channel = Channels.GetByNumber(device->CurrentChannel());
    setting = TtxtSubsChannelSettings.Get(channel);
    if (setting) {
       origPageMode = mPageMode = setting->PageMode();
       origPageNumber = mPageNumber = setting->PageNumber();
    } else {
      origPageMode = mPageMode = 0;
      origPageNumber = mPageNumber = 0;   
    }
    DoDisplay=globals.mRealDoDisplay;
     
    MODE_CHOICES[0] = tr("Auto");
    MODE_CHOICES[1] = tr("Manual");
    MODE_CHOICES[2] = tr("Disabled");
    MODE_CHOICES[3] = NULL;

    Add(new cMenuEditBoolItem(tr("Display Subtitles"), &DoDisplay));
    Add(new cMenuEditStraItem(tr("Subtitle Mode"), &mPageMode, 3, MODE_CHOICES ));
    Add(new cMenuEditIntItem(tr("Manual Subtitle Page"), &mPageNumber, 100, 899));

    Display();
};

eOSState cTtxtsubsPageMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) 
    {
        if (Key == kOk) 
        {
            if(origPageMode!=mPageMode || origPageNumber!=mPageNumber) {
              if (setting) setting->Set(mPageMode, mPageNumber);
                     else  TtxtSubsChannelSettings.Add(new cTtxtSubsChannelSetting(channel, mPageMode, mPageNumber));
              TtxtSubsChannelSettings.Save();
            }
            globals.mRealDoDisplay=DoDisplay;   
            state = osEnd;
         }
     }

    return state;
}
