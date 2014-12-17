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

#define DBG_FN   DBG_NONE
#define DBG_INFO DBG_STDOUT

#if APIVERSNUM >= 10716
#include <vdr/shutdown.h>
#endif


typedef struct imgSize {
        int slot;
        int width;
        int height;
} imgSize_t;

extern int lastThumbWidth[2];
extern int lastThumbHeight[2];

cPluginRBM::cPluginRBM(void) {
	device       = NULL;
	osd_provider = NULL;
	memset(&tv_mode,  0, sizeof(tv_mode));
	tv_mode.uHDOutputMode = RBM_OUTPUT_MODE_576P,
	tv_mode.uHDAspect     = RBM_ASPECT_16_9,
	tv_mode.uHDARMode     = RBM_ARMODE_LETTERBOX,
	tv_mode.uSDOutputMode = RBM_OUTPUT_MODE_PAL_B_WEUR,
	tv_mode.uSDAspect     = RBM_ASPECT_4_3,
	tv_mode.uSDARMode     = RBM_ARMODE_LETTERBOX,
	tv_mode.uHDMIMode     = RBM_HDMIMODE_HDMI;
	tv_mode.uSCARTMode    = RBM_SCARTMODE_RGB;
	tv_mode.uMPEG2Mode    = RBM_MPEG2_MODE_DB_DR;
	tv_mode.iMPAOffset    = 0;
	tv_mode.iAC3Offset    = 0;
} // cPluginRBM::cPluginRBM

bool cPluginRBM::Initialize(void) {
	DBG(DBG_FN,"cPluginRBM::Initialize");
	device = new cRBMDevice(this);
	if(!device) return false;
	osd_provider = new cRBMOsdProvider();
	if(!osd_provider) return false;
	return Store();
} // cPluginRBM::Initialize

cPluginRBM::~cPluginRBM() {
	//DBG(DBG_FN,"cPluginRBM::~cPluginRBM");
} // cPluginRBM::~cPluginRBM

bool cPluginRBM::SetupParse(const char *Name, const char *Value) {
	DBG(DBG_FN,"cPluginRBM::SetupParse %s=%s", Name, Value);
	bool ret = true;
	if     (!strcasecmp(Name, "HDMIOutput" )) tv_mode.uHDOutputMode    = atoi(Value);
	else if(!strcasecmp(Name, "HDMIAspect" )) tv_mode.uHDAspect        = atoi(Value);
	else if(!strcasecmp(Name, "HDMIARMode" )) tv_mode.uHDARMode        = atoi(Value);
	else if(!strcasecmp(Name, "SCARTOutput")) tv_mode.uSDOutputMode    = atoi(Value);
	else if(!strcasecmp(Name, "SCARTAspect")) tv_mode.uSDAspect        = atoi(Value);
	else if(!strcasecmp(Name, "SCARTARMode")) tv_mode.uSDARMode        = atoi(Value);
	else if(!strcasecmp(Name, "HDMIMode"   )) tv_mode.uHDMIMode        = atoi(Value);
	else if(!strcasecmp(Name, "SCARTMode"  )) tv_mode.uSCARTMode       = atoi(Value);
	else if(!strcasecmp(Name, "MPEG2Mode"  )) tv_mode.uMPEG2Mode       = atoi(Value);
	else if(!strcasecmp(Name, "MPADelay"   )) tv_mode.iMPAOffset       = atoi(Value);
	else if(!strcasecmp(Name, "AC3Delay"   )) tv_mode.iAC3Offset       = atoi(Value);
	else ret = false;
	return ret;
} // cPluginRBM::SetupParse

bool cPluginRBM::Store() {
	DBG(DBG_FN,"cPluginRBM::Store");
	SetupStore("HDMIOutput",  tv_mode.uHDOutputMode);
	SetupStore("HDMIAspect",  tv_mode.uHDAspect);
	SetupStore("HDMIARMode",  tv_mode.uHDARMode);
	SetupStore("SCARTOutput", tv_mode.uSDOutputMode);
	SetupStore("SCARTAspect", tv_mode.uSDAspect);
	SetupStore("SCARTARMode", tv_mode.uSDARMode);
	SetupStore("HDMIMode",    tv_mode.uHDMIMode);
	SetupStore("SCARTMode",   tv_mode.uSCARTMode);
	SetupStore("MPEG2Mode",   tv_mode.uMPEG2Mode);
	SetupStore("MPADelay",    tv_mode.iMPAOffset);
	SetupStore("AC3Delay",    tv_mode.iAC3Offset);
	return device ? device->SetTvMode(&tv_mode, true) : false;
} // cPluginRBM::Store

bool cPluginRBM::Service(const char *Id, void *Data) {
    DBG(DBG_FN,"cPluginRBM::Service %s", Id);

    struct service_menu {
            cOsdMenu* menu;
    };
    if (strcmp(Id, "Get AudioSetting Menu") == 0) {
        if (Data)
            ((service_menu*)Data)->menu = new cRBMSetup(this, RBM_SETUP_AUDIO_ONLY);
        return true;
    } else if (strcmp(Id, "Get VideoSetting Menu") == 0) {
        if (Data)
            ((service_menu*)Data)->menu = new cRBMSetup(this, RBM_SETUP_VIDEO_ONLY);
        return true;
    } else if (strcmp(Id, "GetUnbufferedPicSize") == 0) {
        if (Data) {
                struct imgSize *thumbSize = (struct imgSize*)Data;
                thumbSize->width = lastThumbWidth[thumbSize->slot];
                thumbSize->height = lastThumbHeight[thumbSize->slot];
        } // if
        return true;
    } else if (strcmp(Id, "ToggleVideoOutput") == 0) {
        return device ? device->SetOutput((Data && !*((int *)Data)) ? false : true) : false;
    } else if (strcmp(Id, "REINIT") == 0) {
        if(device) device->SetTvMode(&tv_mode);
        return true;
    } // if
#if APIVERSNUM > 10700 && REELVDR
    else if (strcmp(Id, "ActiveModeChanged") == 0)
    {
	if (Data)
	{
	    eActiveMode Mode = *(eActiveMode*)Data;
	    switch (Mode)
	    {
		case active:
		    {
			// order of calling these function is important
			SystemExec("quickshutdown.sh end"); // sets Front Panel LEDs
			device->SetOutput(true);
		    }
		    break;

		case poweroff:
		    {
			device->SetOutput(false);
		    }
		    break;

		case standby:
		    {
			device->SetOutput(false);
			SystemExec("quickshutdown.sh start"); // sets Front panel LEDs
		    }
		    break;

		default:
		    return false;
		    break;
	    } // switch

	} //if
	else
	    esyslog("ERROR reelbox pl: Service(\"ActiveModeChange\") Data == NULL");
    }
#endif
    return false;
} // cPluginRBM::Service

VDRPLUGINCREATOR(cPluginRBM); // Don't touch this!

