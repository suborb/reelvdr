/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Tobias Bratfisch     *
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

#include "install_video_mini.h"
#include "../reelbox/ReelBoxMenu.h"
#include <vdr/plugin.h>

extern int savedMainMode;
extern int wasMainModeSaved;

cInstallVidoutMini::cInstallVidoutMini() : cInstallSubMenu(tr("Connection"))
{
    loopMode = false;           /* fallback just to be sure */
    expertmode = false;

    showDisplayType[0] = "4:3";
    showDisplayType[1] = "16:9";

    masterMode[HD_OUTPUT_HDMI] = "HDMI";
    masterMode[HD_OUTPUT_DVI] = "DVI";
    masterMode[HD_OUTPUT_SCART] = "SCART";

    cSetupLine *setupLine;
    unsigned int tempVal = 0;

    if (wasMainModeSaved)
        mastermode = oldMastermode = savedMainMode;
    else
        mastermode = oldMastermode = 0; // default to "HDMI"

    HDdisplay_type = 1;         // default to "16:9"

    if ((setupLine = Setup.Get("HDMIOutput", "rbmini")))
        tempVal = atoi(setupLine->Value());
    HDMIOutput = 0;
    for (unsigned int i = 0; i < (sizeof(HDMIOutputText) / sizeof(HDMIOutputText[0])); i++)
    {
        HDMIOutputText[i] = tr(RBM_OUTPUT_MODE(HDMIOutputs[i]));
        if (HDMIOutputs[i] == tempVal)
            HDMIOutput = i;
    }                           // for
    if ((setupLine = Setup.Get("HDMIAspect", "rbmini")))
        tempVal = atoi(setupLine->Value());
    HDMIAspect = 0;
    for (unsigned int i = 0; i < (sizeof(HDMIAspectText) / sizeof(HDMIAspectText[0])); i++)
    {
        HDMIAspectText[i] = tr(RBM_ASPECT(HDMIAspects[i]));
        if (HDMIAspects[i] == tempVal)
            HDMIAspect = i;
    }                           // if
    if ((setupLine = Setup.Get("HDMIARMode", "rbmini")))
        tempVal = atoi(setupLine->Value());
    HDMIARMode = 0;
    for (unsigned int i = 0; i < (sizeof(HDMIARModeText) / sizeof(HDMIARModeText[0])); i++)
    {
        HDMIARModeText[i] = tr(RBM_ARMODE(HDMIARModes[i]));
        if (HDMIARModes[i] == tempVal)
            HDMIARMode = i;
    }                           // if
    if ((setupLine = Setup.Get("HDMIMode", "rbmini")))
        tempVal = atoi(setupLine->Value());
    HDMIMode = 0;
    for (unsigned int i = 0; i < (sizeof(HDMIModeText) / sizeof(HDMIModeText[0])); i++)
    {
        HDMIModeText[i] = tr(RBM_HDMIMODE(HDMIModes[i]));
        if (HDMIModes[i] == tempVal)
            HDMIMode = i;
    }                           // if
    if ((setupLine = Setup.Get("SCARTOutput", "rbmini")))
        tempVal = atoi(setupLine->Value());
    SCARTOutput = 0;
    for (unsigned int i = 0; i < (sizeof(SCARTOutputText) / sizeof(SCARTOutputText[0])); i++)
    {
        SCARTOutputText[i] = tr(RBM_OUTPUT_MODE(SCARTOutputs[i]));
        if (SCARTOutputs[i] == tempVal)
            SCARTOutput = i;
    }                           // for
    if ((setupLine = Setup.Get("SCARTAspect", "rbmini")))
        tempVal = atoi(setupLine->Value());
    SCARTAspect = 0;
    for (unsigned int i = 0; i < (sizeof(SCARTAspectText) / sizeof(SCARTAspectText[0])); i++)
    {
        SCARTAspectText[i] = tr(RBM_ASPECT(SCARTAspects[i]));
        if (SCARTAspects[i] == tempVal)
            SCARTAspect = i;
    }                           // if
    if ((setupLine = Setup.Get("SCARTARMode", "rbmini")))
        tempVal = atoi(setupLine->Value());
    SCARTARMode = 0;
    for (unsigned int i = 0; i < (sizeof(SCARTARModeText) / sizeof(SCARTARModeText[0])); i++)
    {
        SCARTARModeText[i] = tr(RBM_ARMODE(SCARTARModes[i]));
        if (SCARTARModes[i] == tempVal)
            SCARTARMode = i;
    }                           // if
    if ((setupLine = Setup.Get("SCARTMode", "rbmini")))
        tempVal = atoi(setupLine->Value());
    SCARTMode = 0;
    for (unsigned int i = 0; i < (sizeof(SCARTModeText) / sizeof(SCARTModeText[0])); i++)
    {
        SCARTModeText[i] = tr(RBM_SCARTMODE(SCARTModes[i]));
        if (SCARTModes[i] == tempVal)
            SCARTMode = i;
    }                           // if

    oldHDMIMode = HDMIMode;
    oldHDMIAspect = HDMIAspect;
    oldHDMIARMode = HDMIARMode;
    oldHDMIOutput = HDMIOutput;
    oldSCARTMode = SCARTMode;
    oldSCARTAspect = SCARTAspect;
    oldSCARTARMode = SCARTARMode;
    oldSCARTOutput = SCARTOutput;

    Set();
}

cInstallVidoutMini::~cInstallVidoutMini()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
}

eOSState cInstallVidoutMini::ProcessKey(eKeys Key)
{
    eOSState result = cOsdMenu::ProcessKey(Key);

    if (Key == kBlue)
    {
        if (!expertmode)
        {
            cPluginManager::CallAllServices("setThumb", NULL);
            expertmode = true;
        }
        else
            expertmode = false;
    }


    if (!expertmode && (Key == kRight || Key == kLeft || Key == kOk))
    {
        if (mastermode == HD_OUTPUT_HDMI)
        {                       // HDMI
            HDMIMode = 0;       // HDMI
            HDMIOutput = 2;     // 1080i50
            HDMIARMode = 2;     // none
            /* set SCART to a proper default: */
            SCARTMode = 2;      // RGB
            SCARTOutput = 0;    // PAL
            SCARTARMode = 2;    // none
        }
        else if (mastermode == HD_OUTPUT_DVI)
        {                       // DVI
            HDMIMode = 0;       // DVI
            HDMIOutput = 2;     // 1080i50
            HDMIARMode = 2;     // none
            /* set SCART to a proper default: */
            SCARTMode = 2;      // RGB
            SCARTOutput = 0;    // PAL
            SCARTARMode = 2;    // none
        }
        else if (mastermode == HD_OUTPUT_SCART)
        {                       // SCART
            SCARTMode = 2;      // RGB
            SCARTOutput = 0;    // PAL
            SCARTARMode = 2;    // none
            /* set HDMI to a proper default: */
            HDMIMode = 0;       // HDMI
            HDMIOutput = 2;     // 1080i50
            HDMIARMode = 2;     // none
        }
        if (mastermode != oldMastermode)
        {
            if (mastermode == HD_OUTPUT_SCART)
                HDdisplay_type = 0;
            else
                HDdisplay_type = 1;
            oldMastermode = mastermode;
        }
        if (HDdisplay_type == 0)
        {                       //4:3
            HDMIAspect = 1;     // 4:3
            SCARTAspect = 1;    // 4:3
        }
        else
        {
            HDMIAspect = 0;     // 16:9
            SCARTAspect = 0;    // 16:9
        }
    }

    switch (Key)
    {
    case kOk:
        Save();
        sleep(1);               // Wait while the VidOut is being reinited
        Key = Skins.Message(mtWarning, tr("Keep these settings?"), 20);
        if (kOk != Key)
        {
            HDMIMode = oldHDMIMode;
            HDMIAspect = oldHDMIAspect;
            HDMIARMode = oldHDMIARMode;
            HDMIOutput = oldHDMIOutput;
            SCARTMode = oldSCARTMode;
            SCARTAspect = oldSCARTAspect;
            SCARTARMode = oldSCARTARMode;
            SCARTOutput = oldSCARTOutput;
            Save();
            Skins.Message(mtWarning, tr("Using old settings"), 10);
            return osContinue;
        }
        else
            return osUser1;
    default:
        break;
    }
    if (Key != kNone)
        Set();
    return result;
}

void cInstallVidoutMini::Set()
{
    int current = Current();
    Clear();
    SetCols(22);

    Add(new cOsdItem(tr("Select a video mode for your screen"), osUnknown, false), false, NULL);
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);

    if (!expertmode)
    {
        Add(new cMenuEditStraItem(tr("TV connected by"), &mastermode, 3, masterMode));
        Add(new cMenuEditStraItem(tr("Display Type"), &HDdisplay_type, 2, showDisplayType));
    }
    else
    {
        Add(new cOsdItem(tr("HDMI settings"), osUnknown, false));
        Add(new cMenuEditStraItem(tr("Mode"), &HDMIMode, sizeof(HDMIModeText) / sizeof(HDMIModeText[0]), HDMIModeText));
        Add(new cMenuEditStraItem(tr("Output"), &HDMIOutput, sizeof(HDMIOutputText) / sizeof(HDMIOutputText[0]), HDMIOutputText));
        Add(new cMenuEditStraItem(tr("Aspect"), &HDMIAspect, sizeof(HDMIAspectText) / sizeof(HDMIAspectText[0]), HDMIAspectText));
        Add(new cMenuEditStraItem(tr("Aspect ratio"), &HDMIARMode, sizeof(HDMIARModeText) / sizeof(HDMIARModeText[0]), HDMIARModeText));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("SCART settings"), osUnknown, false));
        Add(new cMenuEditStraItem(tr("Mode"), &SCARTMode, sizeof(SCARTModeText) / sizeof(SCARTModeText[0]), SCARTModeText));
        Add(new cMenuEditStraItem(tr("Output"), &SCARTOutput, sizeof(SCARTOutputText) / sizeof(SCARTOutputText[0]), SCARTOutputText));
        Add(new cMenuEditStraItem(tr("Aspect"), &SCARTAspect, sizeof(SCARTAspectText) / sizeof(SCARTAspectText[0]), SCARTAspectText));
        Add(new cMenuEditStraItem(tr("Aspect ratio"), &SCARTARMode, sizeof(SCARTARModeText) / sizeof(SCARTARModeText[0]), SCARTARModeText));
    }
    Add(new cOsdItem("", osUnknown, false), false, NULL);
    AddFloatingText(tr("You will have to confirm changes to mode or output. If your display does not support the new settings, just wait for 20 seconds or press Exit to cancel the changes."), 46);

    if (expertmode)
        SetHelp(NULL, tr("Back"), tr("Skip"), tr("Normal"));
    else
        SetHelp(NULL, tr("Back"), tr("Skip"), tr("Expert"));

    SetCurrent(Get(current));

    if (!expertmode)
    {
        // Draw Image
        struct StructImage Image;

        Image.blend = true;
        Image.y = 280;
        Image.h = 0;
        Image.w = 0;
        Image.slot = 0;
        if (mastermode == HD_OUTPUT_HDMI || mastermode == HD_OUTPUT_DVI)
        {                       // HDMI or DVI
            Image.x = 336;
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "NC-Connections-HDMI.png");
        }
        else
        {                       // SCART
            Image.x = 284;
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "NC-Connections-SCART.png");
        }
        cPluginManager::CallAllServices("setThumb", (void *)&Image);
    }
    else
    {
        /** TB: a trick to prevent the skin to clear the last picture */
        struct StructImage Image;
        Image.x = 1;
        Image.y = 1;
        Image.w = Image.h = 0;
        Image.blend = true;
        Image.slot = 0;
        strcpy(Image.path, "");
        cPluginManager::CallAllServices("setThumb", (void *)&Image);
        Image.slot = 1;
        cPluginManager::CallAllServices("setThumb", (void *)&Image);
    }

    Display();
}

bool cInstallVidoutMini::Save()
{
    if (cPlugin * rbminiPlugin = cPluginManager::GetPlugin("rbmini"))
    {
        char tempString[16];

        snprintf(tempString, 16, "%li", HDMIOutputs[HDMIOutput]);
        rbminiPlugin->SetupStore("HDMIOutput", HDMIOutputs[HDMIOutput]);
        rbminiPlugin->SetupParse("HDMIOutput", tempString);

        snprintf(tempString, 16, "%li", HDMIAspects[HDMIAspect]);
        rbminiPlugin->SetupStore("HDMIAspect", HDMIAspects[HDMIAspect]);
        rbminiPlugin->SetupParse("HDMIAspect", tempString);

        snprintf(tempString, 16, "%li", HDMIARModes[HDMIARMode]);
        rbminiPlugin->SetupStore("HDMIARMode", HDMIARModes[HDMIARMode]);
        rbminiPlugin->SetupParse("HDMIARMode", tempString);

        snprintf(tempString, 16, "%li", HDMIModes[HDMIMode]);
        rbminiPlugin->SetupStore("HDMIMode", HDMIModes[HDMIMode]);
        rbminiPlugin->SetupParse("HDMIMode", tempString);

        snprintf(tempString, 16, "%li", SCARTOutputs[SCARTOutput]);
        rbminiPlugin->SetupStore("SCARTOutput", SCARTOutputs[SCARTOutput]);
        rbminiPlugin->SetupParse("SCARTOutput", tempString);

        snprintf(tempString, 16, "%li", SCARTAspects[SCARTAspect]);
        rbminiPlugin->SetupStore("SCARTAspect", SCARTAspects[SCARTAspect]);
        rbminiPlugin->SetupParse("SCARTAspect", tempString);

        snprintf(tempString, 16, "%li", SCARTARModes[SCARTARMode]);
        rbminiPlugin->SetupStore("SCARTARMode", SCARTARModes[SCARTARMode]);
        rbminiPlugin->SetupParse("SCARTARMode", tempString);

        snprintf(tempString, 16, "%li", SCARTModes[SCARTMode]);
        rbminiPlugin->SetupStore("SCARTMode", SCARTModes[SCARTMode]);
        rbminiPlugin->SetupParse("SCARTMode", tempString);

        rbminiPlugin->Service("REINIT", NULL);
    }
    else
        esyslog("error: rbmini-plugin not found");

    if (!expertmode)
    {
        cPluginManager::GetPlugin("install")->SetupStore("wasMainModeSaved", 1);
        cPluginManager::GetPlugin("install")->SetupStore("savedMainMode", mastermode);
        wasMainModeSaved = 1;
        savedMainMode = mastermode;
    }
    else
    {
        wasMainModeSaved = 0;
        cPluginManager::GetPlugin("install")->SetupStore("wasMainModeSaved", 0);
    }

    Setup.Save();

    return true;
}
