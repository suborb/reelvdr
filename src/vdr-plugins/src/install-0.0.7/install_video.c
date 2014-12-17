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
 * install_video.c
 *
 ***************************************************************************/

#include "install_video.h"
#include "../reelbox/ReelBoxMenu.h"

cInstallVidoutAva::cInstallVidoutAva():cInstallSubMenu(tr("Connection"))
{
    loopMode = false;           /* fallback just to be sure */
    expertmode = false;

    showDisplayType[0] = "4:3";
    showDisplayType[1] = "16:9";

    showHDDMode[0] = "DVI";
    showHDDMode[1] = "HDMI";

    showHDAMode[0] = tr("Auto");
    showHDAMode[1] = "YUV";
    showHDAMode[2] = "RGB";
    showHDAMode[3] = "YC";

    showHDAPort[0] = tr("Auto");
    showHDAPort[1] = "SCART";
    showHDAPort[2] = "Mini-DIN";

    showIntProg[0] = tr("Progressive");
    showIntProg[1] = tr("Interlaced");

    deint[0] = tr("Off");
    deint[1] = tr("Auto");
    deint[2] = tr("On");

    masterMode[HD_OUTPUT_HDMI] = "HDMI";
    masterMode[HD_OUTPUT_DVI] = "DVI";
    masterMode[HD_OUTPUT_SCART] = "SCART";
    masterMode[HD_OUTPUT_MINIDIN] = tr("Component");

    cSetupLine *setupLine;

    if ((setupLine = Setup.Get("HDDM", "reelbox")))
        HDdmode = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDAM", "reelbox")))
        HDamode = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDAPort", "reelbox")))
        old_HDaport = HDaport = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDDisplayType", "reelbox")))
        HDdisplay_type = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDAspect", "reelbox")))
        HDaspect = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDResolution", "reelbox")))
        HDresolution = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDIntProg", "reelbox")))
        HDintProg = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("HDnorm", "reelbox")))
        HDnorm = atoi(setupLine->Value());

    // Master mode set
    /**
      mastermode                 di.output  An.output  An.Port  Resolution
     */

    mastermode = 0;             // default
    if (HDdmode == 1 && HDamode == 0 && HDaport == 0)   // HDMI, auto, auto 1080
        mastermode = HD_OUTPUT_HDMI;    // HDMI

    if (HDdmode == 0 && HDamode == 0 && HDaport == 0)   // DVI, auto, auto 1080
        mastermode = HD_OUTPUT_DVI; // DVI

    if (HDdmode == 1 && HDamode == 1 && HDaport == 2)   // HDMI, YUV, Mini-Din 1080
        mastermode = HD_OUTPUT_MINIDIN; // Component

    if (HDamode == 2 && HDaport == 1)   // any, RGB, SCART, any
        mastermode = HD_OUTPUT_SCART;   // SCART
#if 0
    // hack: if HDAPort == Scart then change resolution to 576 */
    if (HDaport == 1)
        HDresolution = 2;
#endif
    Set();
}

cInstallVidoutAva::~cInstallVidoutAva()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
}

eOSState cInstallVidoutAva::ProcessKey(eKeys Key)
{
    eOSState result = cOsdMenu::ProcessKey(Key);

    if (Key == kBlue)
    {
        if (expertmode)
            expertmode = false;
        else
            expertmode = true;
    }


    if (expertmode)
    {
        if (HDaport != old_HDaport) /* if analog port changed */
        {
            if (HDaport == 1)   /* if scart */
                HDresolution = 2;   /* "auto" */
            old_HDaport = HDaport;
        }
    }
    else
    {
        if (mastermode == HD_OUTPUT_HDMI)   // HDMI
        {
            HDresolution = 0;   // 1080
            HDintProg = 1;      // interlaced
            HDdmode = 1;        // HDMI
            HDaport = 0;        // aport: auto
            HDamode = 0;        // auto
        }
        else if (mastermode == HD_OUTPUT_DVI)   // DVI
        {
            HDresolution = 0;   // 1080
            HDintProg = 1;      // interlaced
            HDdmode = 0;        // DVI
            HDamode = 0;        // auto
            HDaport = 0;        // aport: auto
        }
        else if (mastermode == HD_OUTPUT_MINIDIN)   // component
        {
            HDresolution = 0;   // 1080
            HDintProg = 1;      // interlaced
            HDdmode = 1;        // HDMI
            HDamode = 1;        // YUV
            HDaport = 2;        // Mini-Din
        }
        else if (mastermode == HD_OUTPUT_SCART) // SCART
        {
            HDresolution = 2;   // 576
            HDintProg = 1;      // interlaced
            HDaport = 1;        // scart
            HDamode = 2;        // RGB
            HDdmode = 1;        // HDMI
        }
        if (HDdisplay_type == 0)    //4:3
            HDaspect = 1;       // fill to aspect/auto
        else
            HDaspect = 0;
    }

    switch (Key)
    {
    case kOk:
        result = osUser1;
        Save();
        sleep(3);               // Wait while the VidOut is being reinited
        return result;
    default:
        break;
    }
    if (Key != kNone)
        Set();
    return result;
}

void cInstallVidoutAva::Set()
{
    int current = Current();
    Clear();
    SetCols(26);

    Add(new cOsdItem(tr("Select a video mode for your screen"), osUnknown, false), false, NULL);
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);

    if (!expertmode)
    {
        Add(new cMenuEditStraItem(tr("TV connected by"), &mastermode, 4, masterMode));
        Add(new cMenuEditStraItem(tr("Display Type"), &HDdisplay_type, 2, showDisplayType));
    }
    else
    {
        Add(new cMenuEditStraItem(tr("Digital Output"), &HDdmode, 2, showHDDMode));
        Add(new cMenuEditStraItem(tr("Analog Output"), &HDamode, 4, showHDAMode));
        Add(new cMenuEditStraItem(tr("Analog Port"), &HDaport, 3, showHDAPort));
        Add(new cMenuEditStraItem(tr("Display Type"), &HDdisplay_type, 2, showDisplayType));

        showAspect[0] = tr("fill to screen");
        showAspect[1] = tr("fill to aspect");
        showAspect[2] = tr("crop to fill");
        Add(new cMenuEditStraItem(tr("Aspect Ratio Ctrl."), &HDaspect, 3, showAspect));

        showResolution[0] = "1080";
        showResolution[1] = "720";
        showResolution[2] = "576";
        showResolution[3] = "480";

        Add(new cMenuEditStraItem(tr("Resolution"), &HDresolution, 4, showResolution));

        if (HDresolution != HD_VM_RESOLUTION_576 && HDresolution != HD_VM_RESOLUTION_480)
        {
            static const char *showNorm[2];
            showNorm[0] = "50 Hz";
            showNorm[1] = "60 Hz";
            Add(new cMenuEditStraItem(tr("Norm/Framerate"), &HDnorm, 2, showNorm));
        }

        if (HDresolution != HD_VM_RESOLUTION_1080)  // not 1080
        {
            if (HDresolution != HD_VM_RESOLUTION_720)
                Add(new cMenuEditStraItem(tr("Progressive/Interlaced"), &HDintProg, 2, showIntProg));
        }
        else
        {
            HDintProg = 1;      // no 1080p yet
        }

    }

    if (expertmode)
        SetHelp(NULL, tr("Back"), tr("Skip"), tr("Normal"));
    else
        SetHelp(NULL, tr("Back"), tr("Skip"), tr("Expert"));

    SetCurrent(Get(current));

    // Draw Image
    struct StructImage Image;

    Image.x = 209;
    Image.y = 284;
    Image.w = 0;
    Image.h = 0;
    Image.blend = true;
    Image.slot = 0;
    snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-left.png");
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    Image.x = 371;
    Image.h = 116;
    Image.w = 200;
    Image.slot = 1;
    if (expertmode)
        snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-All.png");
    else
    {
        if (mastermode == HD_OUTPUT_HDMI)   // HDMI
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-HDMI.png");
        else if (mastermode == HD_OUTPUT_DVI)   // DVI
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-DVI.png");
        else if (mastermode == HD_OUTPUT_MINIDIN)   // Component
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-YUV.png");
        else                    // SCART
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "TV-Connections-SCART.png");
    }
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    Display();
}

bool cInstallVidoutAva::Save()
{
    if (cPlugin * reelboxPlugin = cPluginManager::GetPlugin("reelbox"))
    {
        char tempString[16];

        snprintf(tempString, 16, "%i", HDdmode);
        reelboxPlugin->SetupStore("HDDM", HDdmode);
        reelboxPlugin->SetupParse("HDDM", tempString);

        snprintf(tempString, 16, "%i", HDamode);
        reelboxPlugin->SetupStore("HDAM", HDamode);
        reelboxPlugin->SetupParse("HDAM", tempString);

        snprintf(tempString, 16, "%i", HDaport);
        reelboxPlugin->SetupStore("HDAPort", HDaport);
        reelboxPlugin->SetupParse("HDAPort", tempString);

        snprintf(tempString, 16, "%i", HDdisplay_type);
        reelboxPlugin->SetupStore("HDDisplayType", HDdisplay_type);
        reelboxPlugin->SetupParse("HDDisplayType", tempString);

        snprintf(tempString, 16, "%i", HDaspect);
        reelboxPlugin->SetupStore("HDAspect", HDaspect);
        reelboxPlugin->SetupParse("HDAspect", tempString);

        snprintf(tempString, 16, "%i", HDresolution);
        reelboxPlugin->SetupStore("HDResolution", HDresolution);
        reelboxPlugin->SetupParse("HDResolution", tempString);

        snprintf(tempString, 16, "%i", HDintProg);
        reelboxPlugin->SetupStore("HDIntProg", HDintProg);
        reelboxPlugin->SetupParse("HDIntProg", tempString);

        snprintf(tempString, 16, "%i", HDnorm);
        reelboxPlugin->SetupStore("HDnorm", HDnorm);
        reelboxPlugin->SetupParse("HDnorm", tempString);

        snprintf(tempString, 16, "%i", mastermode);
        reelboxPlugin->SetupStore("HDoutput", mastermode);
        reelboxPlugin->SetupParse("HDoutput", tempString);

        int replyCode;
        reelboxPlugin->SVDRPCommand("REINIT", NULL, replyCode);
    }
    else
        esyslog("error: reelbox-plugin not found");

//    char tempString[3];
//    sprintf(tempString, "%d", stepNumber);
//    // printf (" debug  SetupStore Video %d \n", stepNumber);
//    cPluginManager::GetPlugin("install")->SetupStore("stepdone", stepNumber);
//    cPluginManager::GetPlugin("install")->SetupParse("stepdone", tempString);
    Setup.Save();

    return true;
}
