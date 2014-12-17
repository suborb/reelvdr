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
 * install_video.h
 *
 ***************************************************************************/

#ifndef INSTALL_VIDEO_H
#define INSTALL_VIDEO_H

#include "installmenu.h"

extern "C"
{
    typedef unsigned long long u_int64;
    typedef unsigned long u_int32;
    typedef long int32;
#include "../rbmini/include/cnxt_rbm.h"
}                               // extern "C"

const u_int32 HDMIOutputs[] = {
    RBM_OUTPUT_MODE_576P, RBM_OUTPUT_MODE_720P_50, RBM_OUTPUT_MODE_1080I_50,
    RBM_OUTPUT_MODE_PAL_B_WEUR, RBM_OUTPUT_MODE_NTSC_M, RBM_OUTPUT_MODE_SECAM_L,
    RBM_OUTPUT_MODE_480P, RBM_OUTPUT_MODE_720P_60, RBM_OUTPUT_MODE_1080I_60,
    RBM_OUTPUT_MODE_1080P_24, RBM_OUTPUT_MODE_1080P_25, RBM_OUTPUT_MODE_1080P_30,
    /*RBM_OUTPUT_MODE_1080P_24, RBM_OUTPUT_MODE_1080P_25, RBM_OUTPUT_MODE_1080P_30 */
    RBM_OUTPUT_MODE_SOURCE
};
const u_int32 HDMIAspects[] = { RBM_ASPECT_16_9, RBM_ASPECT_4_3 };
const u_int32 HDMIARModes[] = { RBM_ARMODE_LETTERBOX, RBM_ARMODE_PANSCAN, RBM_ARMODE_NONE };
const u_int32 HDMIModes[] = { RBM_HDMIMODE_HDMI, RBM_HDMIMODE_DVI };
const u_int32 SCARTOutputs[] = { RBM_OUTPUT_MODE_PAL_B_WEUR, RBM_OUTPUT_MODE_NTSC_M, RBM_OUTPUT_MODE_SECAM_L };
const u_int32 SCARTAspects[] = { RBM_ASPECT_16_9, RBM_ASPECT_4_3, RBM_ASPECT_NONE, RBM_ASPECT_SOURCE };
const u_int32 SCARTARModes[] = { RBM_ARMODE_LETTERBOX, RBM_ARMODE_PANSCAN, RBM_ARMODE_NONE };
const u_int32 SCARTModes[] = { RBM_SCARTMODE_ALL, RBM_SCARTMODE_CVBS, /*RBM_SCARTMODE_CVBS_DLY, */ RBM_SCARTMODE_RGB, RBM_SCARTMODE_YC, RBM_SCARTMODE_YPRPB };

class cInstallVidoutMini:public cInstallSubMenu
{
  private:
    void Set();
    bool Save();
    const char *HDMIOutputText[sizeof(HDMIOutputs) / sizeof(HDMIOutputs[0])];
    const char *HDMIAspectText[sizeof(HDMIAspects) / sizeof(HDMIAspects[0])];
    const char *HDMIARModeText[sizeof(HDMIARModes) / sizeof(HDMIARModes[0])];
    const char *HDMIModeText[sizeof(HDMIModes) / sizeof(HDMIModes[0])];
    int HDMIOutput;
    int oldHDMIOutput;
    int HDMIAspect;
    int oldHDMIAspect;
    int HDMIARMode;
    int oldHDMIARMode;
    int HDMIMode;
    int oldHDMIMode;
    const char *SCARTOutputText[sizeof(SCARTOutputs) / sizeof(SCARTOutputs[0])];
    const char *SCARTAspectText[sizeof(SCARTAspects) / sizeof(SCARTAspects[0])];
    const char *SCARTARModeText[sizeof(SCARTARModes) / sizeof(SCARTARModes[0])];
    const char *SCARTModeText[sizeof(SCARTModes) / sizeof(SCARTModes[0])];
    int SCARTOutput;
    int oldSCARTOutput;
    int SCARTAspect;
    int oldSCARTAspect;
    int SCARTARMode;
    int oldSCARTARMode;
    int SCARTMode;
    int oldSCARTMode;

    const char *showDisplayType[2];
    const char *masterMode[4];
    int HDdisplay_type;
    int mastermode;
    int oldMastermode;
    bool expertmode;
  public:
      cInstallVidoutMini();
     ~cInstallVidoutMini();
    eOSState ProcessKey(eKeys Key);
};

#endif
