/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
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

#include "rbmini.h"
#include "rbm_setup.h"

#define DBG_FN   DBG_STDOUT
#define DBG_INFO DBG_STDOUT
#define MAX_LETTER_PER_LINE 48

cRBMSetup::cRBMSetup(cPluginRBM *Plugin, int Mode) {
    DBG(DBG_FN,"cRBMSetup::cRBMSetup");
    plugin       = Plugin;
    isBothMode   = RBM_SETUP_VIDEO_AUDIO == Mode;
    isVideoMode  = isBothMode ? true : (Mode == RBM_SETUP_VIDEO_ONLY) ? true : false;
    isExpertMode = false;
    if(isVideoMode)
       SetTitle(tr("Setup - Video"));
    else
       SetTitle(tr("Setup - Audio"));
    HDMIOutput = 0;
    for(unsigned int i=0;i<(sizeof(HDMIOutputText)/sizeof(HDMIOutputText[0]));i++) {
        HDMIOutputText[i] = tr(RBM_OUTPUT_MODE(HDMIOutputs[i]));
        if(HDMIOutputs[i] == plugin->tv_mode.uHDOutputMode)
            HDMIOutput = i;
    } // for
    for(unsigned int i=0;i<(sizeof(HDMIAspectText)/sizeof(HDMIAspectText[0]));i++) {
        HDMIAspectText[i] = tr(RBM_ASPECT(HDMIAspects[i]));
        if(HDMIAspects[i] == plugin->tv_mode.uHDAspect)
            HDMIAspect = i;
    } // if
    HDMIARMode = 0;
    for(unsigned int i=0;i<(sizeof(HDMIARModeText)/sizeof(HDMIARModeText[0]));i++) {
        HDMIARModeText[i] = tr(RBM_ARMODE(HDMIARModes[i]));
        if(HDMIARModes[i] == plugin->tv_mode.uHDARMode)
            HDMIARMode = i;
    } // if
    HDMIMode = 0;
    for(unsigned int i=0;i<(sizeof(HDMIModeText)/sizeof(HDMIModeText[0]));i++) {
        HDMIModeText[i] = tr(RBM_HDMIMODE(HDMIModes[i]));
        if(HDMIModes[i] == plugin->tv_mode.uHDMIMode)
            HDMIMode = i;
    } // if
    SCARTOutput = 0;
    for(unsigned int i=0;i<(sizeof(SCARTOutputText)/sizeof(SCARTOutputText[0]));i++) {
        SCARTOutputText[i] = tr(RBM_OUTPUT_MODE(SCARTOutputs[i]));
        if(SCARTOutputs[i] == plugin->tv_mode.uSDOutputMode)
            SCARTOutput = i;
    } // for
    SCARTAspect = 0;
    for(unsigned int i=0;i<(sizeof(SCARTAspectText)/sizeof(SCARTAspectText[0]));i++) {
        SCARTAspectText[i] = tr(RBM_ASPECT(SCARTAspects[i]));
        if(SCARTAspects[i] == plugin->tv_mode.uSDAspect)
            SCARTAspect = i;
    } // if
    SCARTARMode = 0;
    for(unsigned int i=0;i<(sizeof(SCARTARModeText)/sizeof(SCARTARModeText[0]));i++) {
        SCARTARModeText[i] = tr(RBM_ARMODE(SCARTARModes[i]));
        if(SCARTARModes[i] == plugin->tv_mode.uSDARMode)
            SCARTARMode = i;
    } // if
    SCARTMode = 0;
    for(unsigned int i=0;i<(sizeof(SCARTModeText)/sizeof(SCARTModeText[0]));i++) {
        SCARTModeText[i] = tr(RBM_SCARTMODE(SCARTModes[i]));
        if(SCARTModes[i] == plugin->tv_mode.uSCARTMode)
            SCARTMode = i;
    } // if
    MPEG2Mode = 0;
    for(unsigned int i=0;i<(sizeof(MPEG2ModeText)/sizeof(MPEG2ModeText[0]));i++) {
        MPEG2ModeText[i] = tr(RBM_MPEG2_MODE(MPEG2Modes[i]));
        if(MPEG2Modes[i] == plugin->tv_mode.uMPEG2Mode)
            MPEG2Mode = i;
    } // if
    MPADelay = plugin->tv_mode.iMPAOffset / 100;
    AC3Delay = plugin->tv_mode.iAC3Offset / 100;
    Setup();
} // cRBMSetup::cRBMSetup

cRBMSetup::~cRBMSetup() {
    DBG(DBG_FN,"cRBMSetup::cRBMSetup");
} // cRBMSetup::~cRBMSetup

eOSState cRBMSetup::ProcessKey(eKeys Key) {
//  DBG(DBG_FN,"cRBMSetup::ProcessKey");
    if(kOk == Key) {
        rbm_tv_mode old = plugin->tv_mode;
        Store();
        if((old.uHDOutputMode != plugin->tv_mode.uHDOutputMode) || (old.uHDMIMode  != plugin->tv_mode.uHDMIMode) ||
           (old.uSDOutputMode != plugin->tv_mode.uSDOutputMode) || (old.uSCARTMode != plugin->tv_mode.uSCARTMode)) { // Check for resolution or mode change
            Key = Skins.Message(mtWarning, tr("Keep these settings?"), 20);
            if(kOk != Key) {
                plugin->tv_mode = old;
                plugin->Store();
                Skins.Message(mtWarning, tr("Using old settings"), 10);
            } else
                return osContinue;
        } // if
    } else if ((kRed == Key) && isBothMode) {
        isVideoMode = !isVideoMode;
        Setup();
        return osContinue;
    } else if ((kBlue == Key) && isVideoMode) {
        isExpertMode = !isExpertMode;
        Setup();
        return osContinue;
    } // if
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    if (Key == kRed || Key == kGreen || Key == kYellow || Key == kBlue)
    {
	//ignore color keys
	state = osContinue;
    }
    else if (Key == kNone)
	state = osUnknown;
    return state;
} // cRBMSetup::ProcessKey

void cRBMSetup::Store() {
    DBG(DBG_FN,"cRBMSetup::Store");
    plugin->tv_mode.uHDOutputMode   = HDMIOutputs[HDMIOutput];
    plugin->tv_mode.uHDAspect       = HDMIAspects[HDMIAspect];
    plugin->tv_mode.uHDARMode       = HDMIARModes[HDMIARMode];
    plugin->tv_mode.uHDMIMode       = HDMIModes[HDMIMode];
    plugin->tv_mode.uSDOutputMode   = SCARTOutputs[SCARTOutput];
    plugin->tv_mode.uSDAspect       = SCARTAspects[SCARTAspect];
    plugin->tv_mode.uSDARMode       = SCARTARModes[SCARTARMode];
    plugin->tv_mode.uSCARTMode      = SCARTModes[SCARTMode];
    plugin->tv_mode.uMPEG2Mode      = MPEG2Modes[MPEG2Mode];
    plugin->tv_mode.iMPAOffset      = 100 * MPADelay;
    plugin->tv_mode.iAC3Offset      = 100 * AC3Delay;
#ifndef TRIGGER_AR_ON_SET_VIDEO_FORMAT
    TODO:
    Setup.VideoFormat        =
    Setup.VideoDisplayFormat =
#endif
    plugin->Store();
} // cRBMSetup::Store

void cRBMSetup::Setup() {
    DBG(DBG_FN,"cRBMSetup::Setup");
    int current = Current();
    Clear();
    SetCols(29);

    if (isVideoMode) {
        SetHelp(isBothMode ? tr("Audio") : NULL, NULL, NULL, isExpertMode ? tr("Normal") : tr("Experts"));
        Add(new cOsdItem(tr("HDMI settings"), osUnknown, false));
        Add(new cMenuEditStraItem(tr("Mode"), &HDMIMode, sizeof(HDMIModeText)/sizeof(HDMIModeText[0]), HDMIModeText));
        Add(new cMenuEditStraItem(tr("Output"), &HDMIOutput, sizeof(HDMIOutputText)/sizeof(HDMIOutputText[0]), HDMIOutputText));
        Add(new cMenuEditStraItem(tr("Aspect"), &HDMIAspect, sizeof(HDMIAspectText)/sizeof(HDMIAspectText[0]), HDMIAspectText));
        Add(new cMenuEditStraItem(tr("Aspect ratio"), &HDMIARMode, sizeof(HDMIARModeText)/sizeof(HDMIARModeText[0]), HDMIARModeText));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("SCART settings"), osUnknown, false));
        Add(new cMenuEditStraItem(tr("Mode"), &SCARTMode, sizeof(SCARTModeText)/sizeof(SCARTModeText[0]), SCARTModeText));
        Add(new cMenuEditStraItem(tr("Output"), &SCARTOutput, sizeof(SCARTOutputText)/sizeof(SCARTOutputText[0]), SCARTOutputText));
        Add(new cMenuEditStraItem(tr("Aspect"), &SCARTAspect, sizeof(SCARTAspectText)/sizeof(SCARTAspectText[0]), SCARTAspectText));
        Add(new cMenuEditStraItem(tr("Aspect ratio"), &SCARTARMode, sizeof(SCARTARModeText)/sizeof(SCARTARModeText[0]), SCARTARModeText));
        if(isExpertMode) {
            Add(new cOsdItem("", osUnknown, false), false, NULL);
            Add(new cOsdItem(tr("Mpeg2 settings"), osUnknown, false));
            Add(new cMenuEditStraItem(tr("Postprocessing"), &MPEG2Mode, sizeof(MPEG2ModeText)/sizeof(MPEG2ModeText[0]), MPEG2ModeText));
        } else {
            Add(new cOsdItem("", osUnknown, false), false, NULL);
            AddFloatingText(tr("You will have to confirm changes to mode or output. If your display does not support the new settings, just wait for 20 seconds or press Exit to cancel the changes."), 46);
        }
    } else {
        SetHelp(isBothMode ? tr("Video") : NULL);
        Add(new cOsdItem(tr("Audio settings"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cMenuEditIntItem(tr("Analog offset"), &MPADelay, -25, 25));
        Add(new cMenuEditIntItem(tr("Dolby Digital offset"), &AC3Delay, -25, 25));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem("_____________________________________________",osUnknown, false));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("Note:"), osUnknown, false));
        AddFloatingText(tr("The adjustable range is 1/100 seconds."), MAX_LETTER_PER_LINE);
    }
    SetCurrent(Get(current));
    Display();
} // cRBMSetup::Setup


