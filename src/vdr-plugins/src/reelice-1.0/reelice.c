/**************************************************************************
*   Copyright (C) 2011 by Reel Multimedia                                 *
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

#include "reelice.h"
#include <vdr/shutdown.h>
#include <vdr/thread.h>
#include <vdr/dummyplayer.h>

#define DBG_FN	DBG_NONE
#define DBG_GDL	DBG_STDOUT
#define DBG_SMD	DBG_STDOUT
//DBG_SYSLOG

#include "ice_device.h"

//#define MUTE_AUDIO_AFTER_CHANGE
//#define ENABLE_AUDIO_TIMEOUT_MS 1000
#define SET_DISPLAY_INFO_RETRY 100

extern int lastThumbWidth[2];
extern int lastThumbHeight[2];

cICEBgPic::cICEBgPic(cICEDevice *fDevice) /*: cThread("ICEBgPic")*/ {
	device  = fDevice;
	nextPic = 0;
} // cICEBgPic::cICEBgPic

void cICEBgPic::Action() {
	nextPic++;
	cString filename = cString::sprintf(PIC_IMAGE_DIR, nextPic);
	FILE *file = ::fopen(filename, "rb");
	if(file) {
		struct stat filestat;
		::stat(filename, &filestat);
		if(filestat.st_size > 0) {
			Buffer.resize(filestat.st_size);
			if(filestat.st_size == (off_t)::fread(&Buffer[0], 1, filestat.st_size, file))
				if(device && device->ShowBgPic())
					device->StillPicture(&Buffer[0], filestat.st_size);
		} // if
		::fclose(file);
	} else if (nextPic != 1) {
		nextPic = 0;
		Action();
	} // if
} // cICEBgPic::Action

cPluginICE::cPluginICE(void)
           :device(NULL), osd_provider(NULL), aud_prc(AUDIO_INVALID_HANDLE) {
	ICE_DBG(DBG_FN, "cPluginICE::cPluginICE");
	enableOutputs = 0;
	gdl_ret_t ret = GDL_SUCCESS;
//	cl_app_sven_initialize();
	gdl_init(0);
	GDLCHECK(gdl_event_register(GDL_APP_EVENT_HOTPLUG_DETECT, on_gdl_event, this));
	GDLCHECK(gdl_event_register(GDL_APP_EVENT_HOTPLUG_LOSS, on_gdl_event, this));
#ifdef APP_EVENT_MODE_DISP
	GDLCHECK(gdl_event_register(GDL_APP_EVENT_MODE_DISP_0, on_gdl_event, this));
	GDLCHECK(gdl_event_register(GDL_APP_EVENT_MODE_DISP_1, on_gdl_event, this));
#endif /*APP_EVENT_MODE_DISP*/
 } // cPluginICE::cPluginICE

cPluginICE::~cPluginICE() {
	ICE_DBG(DBG_FN, "cPluginICE::~cPluginICE");
	if(osd_provider) delete osd_provider;
	osd_provider=NULL;
	if(device) delete device;
	device=NULL;
	gdl_ret_t ret = GDL_SUCCESS;
	if(AUDIO_INVALID_HANDLE != aud_prc) ismd_audio_close_processor(aud_prc);
	aud_prc = AUDIO_INVALID_HANDLE;
	GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_HOTPLUG_DETECT));
	GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_HOTPLUG_LOSS));
#ifdef APP_EVENT_MODE_DISP
	GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_MODE_DISP_0));
	GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_MODE_DISP_1));
#endif /*APP_EVENT_MODE_DISP*/
	gdl_close();
} // cPluginICE::~cPluginICE

bool cPluginICE::Initialize(void) {
	ICE_DBG(DBG_FN, "cPluginICE::Initialize");
	SystemExec("killall splash");   
	device = new cICEDeviceSMD(this);
	if(!device) return false;
	osd_provider = new cICEOsdProvider(this);
	if(!osd_provider) return false;
	return true;
} // cPluginICE::Initialize

bool cPluginICE::SetupParse(const char *Name, const char *Value) {
	ICE_DBG(DBG_FN, "cPluginICE::SetupParse %s=%s", Name, Value);
	int iVal = Value ? atoi(Value) : -1;
	if(     !strcasecmp(Name, "tv_mode"                ) && (iVal > ICE_TVMODE_NONE)                            && (iVal < ICE_TVMODE_COUNT)                  ) options.tv_mode                 = (tICE_TVMode)iVal;
	else if(!strcasecmp(Name, "deinterlace_mode"       ) && (iVal >= ISMD_VIDPPROC_DI_POLICY_NONE)              && (iVal < ISMD_VIDPPROC_DI_POLICY_COUNT)     ) options.deinterlace_mode        = (ismd_vidpproc_deinterlace_policy_t)iVal;
	else if(!strcasecmp(Name, "scale_mode"             ) && (iVal >= ISMD_VIDPPROC_SCALING_POLICY_SCALE_TO_FIT) && (iVal < ISMD_VIDPPROC_SCALING_POLICY_COUNT)) options.scale_mode              = (ismd_vidpproc_scaling_policy_t)iVal;
	else if(!strcasecmp(Name, "deringing_level"        ) && (iVal >= INVALID_LEVEL)                             && (iVal <= MAX_DERINGNING_LEVEL)             ) options.deringing_level         = iVal;
	else if(!strcasecmp(Name, "gaussian_level"         ) && (iVal >= INVALID_LEVEL)                             && (iVal <= MAX_GAUSSIAN_LEVEL)               ) options.gaussian_level          = iVal;
	else if(!strcasecmp(Name, "degrade_level"          ) && (iVal >= INVALID_LEVEL)                             && (iVal <= MAX_DEGRADE_LEVEL)                ) options.degrade_level           = iVal;
	else if(!strcasecmp(Name, "pan_scan"               )                                                                                                      ) options.pan_scan                = (1==iVal);
	else if(!strcasecmp(Name, "override_frame_polarity")                                                                                                      ) options.override_frame_polarity = (1==iVal);
	else if(!strcasecmp(Name, "hdmi_audio"             ) && (iVal >= ISMD_AUDIO_OUTPUT_INVALID)                 && (iVal <= ISMD_AUDIO_OUTPUT_ENCODED_DTS)    ) options.hdmi_audio              = (ismd_audio_output_mode_t)iVal;
	else if(!strcasecmp(Name, "hdmi_channels"          ) && (iVal > ISMD_AUDIO_CHAN_CONFIG_INVALID)             && (iVal <= ISMD_AUDIO_7_1)                   ) options.hdmi_channels           = (ismd_audio_channel_config_t)iVal;
	else if(!strcasecmp(Name, "hdmi_delay"             ) && (iVal >= 0)                                         && (iVal <= AUDIO_MAX_OUTPUT_STREAM_DELAY)    ) options.hdmi_delay              = iVal;
	else if(!strcasecmp(Name, "spdif_audio"            ) && (iVal >= ISMD_AUDIO_OUTPUT_INVALID)                 && (iVal <= ISMD_AUDIO_OUTPUT_ENCODED_DTS)    ) options.spdif_audio             = (ismd_audio_output_mode_t)iVal;
	else if(!strcasecmp(Name, "spdif_channels"         ) && (iVal > ISMD_AUDIO_CHAN_CONFIG_INVALID)             && (iVal <= ISMD_AUDIO_7_1)                   ) options.spdif_channels          = (ismd_audio_channel_config_t)iVal;
	else if(!strcasecmp(Name, "spdif_delay"            ) && (iVal >= 0)                                         && (iVal <= AUDIO_MAX_OUTPUT_STREAM_DELAY)    ) options.spdif_delay             = iVal;
	else if(!strcasecmp(Name, "analog_delay"           ) && (iVal >= 0)                                         && (iVal <= AUDIO_MAX_OUTPUT_STREAM_DELAY)    ) options.analog_delay            = iVal;
	else if(!strcasecmp(Name, "lockOsdToSd"            )                                                                                                      ) options.lockOsdToSd             = (1==iVal);
	else if(!strcasecmp(Name, "edid_mode"              ) && (iVal >= GDL_HDMI_USE_EDID_NONE)                    && (iVal <= GDL_HDMI_USE_EDID_SAFE)           ) options.edid_mode               = (gdl_hdmi_use_edid_t)iVal;
	else if(!strcasecmp(Name, "video_system"           ) && (iVal >= vsPAL)                                     && (iVal <= vsNTSC)                           ) options.video_system            = (eVideoSystem)iVal;
	else if(!strcasecmp(Name, "hdmi_device"            )                                                                                                      ) Utf8Strn0Cpy(options.hdmi_device, Value, sizeof(options.hdmi_device));
	else if(!strcasecmp(Name, "hdmi_device_check"      )                                                                                                      ) options.hdmi_device_check       = (1==iVal);
	else return false;
	return true;
} // cPluginICE::SetupParse

bool cPluginICE::Store() {
	ICE_DBG(DBG_FN, "cPluginICE::Store");
	SetupStore("tv_mode",                 options.tv_mode);
	SetupStore("deinterlace_mode",        options.deinterlace_mode);
	SetupStore("scale_mode",              options.scale_mode);
	SetupStore("deringing_level",         options.deringing_level);
	SetupStore("gaussian_level",          options.gaussian_level);
	SetupStore("degrade_level",           options.degrade_level);
	SetupStore("pan_scan",                options.pan_scan);
	SetupStore("override_frame_polarity", options.override_frame_polarity);
	SetupStore("hdmi_audio",              options.hdmi_audio);
	SetupStore("hdmi_channels",           options.hdmi_channels);
	SetupStore("hdmi_delay",              options.hdmi_delay);
	SetupStore("spdif_audio",             options.spdif_audio);
	SetupStore("spdif_channels",          options.spdif_channels);
	SetupStore("spdif_delay",             options.spdif_delay);
	SetupStore("analog_delay",            options.analog_delay);
//	SetupStore("lockOsdToSd",             options.lockOsdToSd);
	SetupStore("edid_mode",               options.edid_mode);
	SetupStore("video_system",            options.video_system);
	SetupStore("hdmi_device",             options.hdmi_device);
	SetupStore("hdmi_device_check",       options.hdmi_device_check);
	Setup.Save(); // TODO: Or wait till vdr saves config?
	return updateVideo();
} // cPluginICE::Store

bool cPluginICE::Service(char const *Id, void *Data) {
	ICE_DBG(DBG_FN, "cPluginICE::Service %s", Id);
	if (strcmp(Id, "ActiveModeChanged") == 0) {
		if (Data) {
			eActiveMode Mode = *(eActiveMode*)Data;
			switch(Mode) {
				case active: {
					enableOutput(true);
					SystemExec("quickshutdown.sh end"); // sets Front Panel LEDs
					return true;
				} // active
				case standby:
					SystemExec("quickshutdown.sh start"); // sets Front panel LEDs
					// fall through
				case poweroff: {
					enableOutput(false);
					return true;
				} // poweroff|standby
				default:
					esyslog("ERROR cPluginICE::Service(\"%s\")  invalid mode %d", Id, Mode);
			} // switch
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "Get AudioSetting Menu") == 0) {
		struct audio_service_menu { // TODO: remove this ugly local hack/definition
			cOsdMenu *menu;
		};
		if(Data) {
			((audio_service_menu *)Data)->menu = new cICESetup(this, ICE_SETUP_AUDIO_ONLY);
			return true;
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "Get VideoSetting Menu") == 0) {
		struct video_service_menu { // TODO: remove this ugly local hack/definition
			cOsdMenu *menu;
		};
		if(Data) {
			((video_service_menu *)Data)->menu = new cICESetup(this, ICE_SETUP_VIDEO_ONLY);
			return true;
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "Get VideoSetting InstallWizard Menu") == 0) {
		struct video_service_installmenu { // TODO: remove this ugly local hack/definition
			cOsdMenu *menu;
			cString title;
		};
		video_service_installmenu *installmenu = ((video_service_installmenu *)Data);
		if(installmenu) {
			installmenu->menu = new cICESetup(this, ICE_SETUP_VIDEO_ONLY, 
								installmenu->title, true);
			return true;
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "LockOSDToSD") == 0) {
		if(Data) {
			options.lockOsdToSd=*((bool *)Data);
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "IsLockOSDToSD") == 0) {
		if(Data) {
			*((bool *)Data) = options.lockOsdToSd;
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "EnableAudioPicPlayer") == 0) {
		options.show_bg_pic = true;
	} else if (strcmp(Id, "DisableAudioPicPlayer") == 0) {
		options.show_bg_pic = false;
	} else if (strcmp(Id, "IsOutputPlugin") == 0) {
		return true;
	} else if (strcmp(Id, "GetDisplayWidth") == 0) {
		if(Data) {
			gdl_tvmode_t mode;
			if(GetCurrentTVMode(mode)) {
				*((int *)Data) = mode.width;
				return true;
			} else
				esyslog("ERROR cPluginICE::Service(\"%s\") Unable to get current tv mode", Id);
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "GetDisplayHeight") == 0) {
		if(Data) {
			gdl_tvmode_t mode;
			if(GetCurrentTVMode(mode)) {
				*((int *)Data) = mode.height;
				return true;
			} else
				esyslog("ERROR cPluginICE::Service(\"%s\") Unable to get current tv mode", Id);
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "GetDisplayAspect") == 0) {
		if(Data) {
			*((double *)Data) = 1.0;
			return true;
		} else
			esyslog("ERROR cPluginICE::Service(\"%s\") Data == NULL", Id);
	} else if (strcmp(Id, "GetUnbufferedPicSize") == 0) {
		if (Data) {
			struct imgSize { // TODO: remove this ugly local hack/definition
				int slot;
				int width;
				int height;
			};
			struct imgSize *thumbSize = (struct imgSize*)Data;
			thumbSize->width = lastThumbWidth[thumbSize->slot];
			thumbSize->height = lastThumbHeight[thumbSize->slot];
		} // if
		return true;
	} else if (strcmp("switch to xbmc", Id) == 0) {
		int replyCode = 500;
		SVDRPCommand("OFF", NULL, replyCode);
		return true;
	} // if
	return false;
} // cPluginICE::Service

const char **cPluginICE::SVDRPHelpPages(void) {
	static const char *HelpPages[] = {
		"STV [ mode ]\n"
		"    Set current tv mode\n",
		"GTV\n"
		"    Get current tv mode\n",
		NULL
	};
	return HelpPages;
} // cPluginICE::SVDRPHelpPages

cString cPluginICE::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
	if(!strcasecmp("STV", Command)) {
		if(Option) {
			int mode = atoi(Option);
			if((ICE_TVMODE_NONE < mode) && (ICE_TVMODE_COUNT > mode)) {
				options.tv_mode = (tICE_TVMode)mode;
				if(updateVideo()) {
					return cString::sprintf(tr("Set current tv mode to %d %s"), mode, TVMODE_STR(mode));
				} else {
					ReplyCode = 950;
					return cString::sprintf(tr("Failed to set current tv mode to %d %s"), mode, TVMODE_STR(mode));
				} // if
			} else {
				ReplyCode = 901;
				return cString::sprintf(tr("Invalid mode parameter %d (Allowed %d..%d)"), mode, ICE_TVMODE_NONE+1, ICE_TVMODE_COUNT-1);
			} // if
		} else {
			ReplyCode = 901;
			return cString(tr("No mode parameter"));
		} // if
	} else if(!strcasecmp("GTV", Command)) {
		gdl_tvmode_t mode;
		if(!GetCurrentTVMode(mode)) {
			ReplyCode = 901;
			return cString(tr("Unable to get current tv mode"));
		} // if
		return cString::sprintf("%d %s (%dx%d%s%s)", options.tv_mode, TVMODE_STR(options.tv_mode), mode.width, mode.height, mode.interlaced?"i":"p", REFRESH_STR(mode.refresh));
	} else if(!strcasecmp("OFF", Command)) {
		cControl::Shutdown();
		cControl::Launch(new cDummyPlayerControl(pmExtern_THIS_SHOULD_BE_AVOIDED));
		return cString::sprintf(tr("Output disabled"));
	} else if (!strcasecmp("ON", Command)) {
		// TODO: find if reelice plugin is "OFF" before 
		// executing the following
		cControl::Shutdown();
		
		// tune to current channel
		int Channel = cDevice::CurrentChannel();
		
		if (Channel > 0) 
			Channels.SwitchTo(Channel);
		else if (Setup.CurrentChannel > 0)
			Channels.SwitchTo(Setup.CurrentChannel);
		else 
			Channels.SwitchTo(1); //couldn't find the curr. channel, tune to channelNr 1
		return "Ok";
	} else
		return NULL;
} // cPluginICE::SVDRPCommand

uchar *cPluginICE::GrabImage(int &Size, bool Jpeg, int Quality, int SizeX, int SizeY) {
	int osdRow = 0, osdBpp = 0, osdX = 0, osdY = 0;
	const uchar *osdData = osd_provider ? osd_provider->GetOsdImage(osdRow, osdBpp, osdX, osdY) : NULL;
	Size = 0;
	if(!osdData) return NULL;
	SizeX=osdX; // Until we support scaling
	SizeY=osdY; // Until we support scaling
	if (Quality<0) Quality=100;
	uint8_t *picture = (uchar*) malloc( 32 + 3*SizeX*SizeY );
	if(!picture) return NULL;
	if (!Jpeg) { // write PNM header:
		char buf[32];
		snprintf(buf, sizeof(buf), "P6\n%d\n%d\n255\n", SizeX, SizeY);
		int l = strlen(buf);
		memcpy(picture, buf, l);
		Size+=l;
	} // if
	for (int y=0; y<osdY; y++) {
		for (int x=0; x<osdX; x++) {
			picture[Size++] = osdData[y*osdRow+x*osdBpp+2];
			picture[Size++] = osdData[y*osdRow+x*osdBpp+1];
			picture[Size++] = osdData[y*osdRow+x*osdBpp+0];
		} // for
	} // for
	if (Jpeg) {
		uint8_t *jpeg_picture = RgbToJpeg(picture, SizeX, SizeY, Size, Quality);
		free(picture);
		if (!jpeg_picture) {
			esyslog("[softdevice] failed to convert image to JPEG");
			return NULL;
		} // if
		return jpeg_picture;
	} // if
	return picture;
} // cPluginICE::GrabImage

bool cPluginICE::SetCurrentTVMode(gdl_tvmode_t &mode) {
	ICE_DBG(DBG_FN, "cPluginICE::SetCurrentTVMode %dx%d%s%s", mode.width, mode.height, mode.interlaced?"i":"p", REFRESH_STR(mode.refresh));
	gdl_ret_t ret = GDL_SUCCESS;
	bool set_display_first=true;
	gdl_tvmode_t old;
	memset(&old, 0, sizeof(old));
	gdl_display_info_t di_hd;
	memset(&di_hd, 0, sizeof(di_hd));
	di_hd.id          = GDL_DISPLAY_ID_0;
	di_hd.tvmode      = mode;
	di_hd.bg_color    = 0;
	di_hd.gamma       = GDL_GAMMA_LINEAR;
	di_hd.color_space = (mode.height > 576) ? GDL_COLOR_SPACE_BT709 : GDL_COLOR_SPACE_BT601;
	isyslog("Video [HDMI] %dx%d%s%s", di_hd.tvmode.width, di_hd.tvmode.height, di_hd.tvmode.interlaced?"i":"p", REFRESH_STR(di_hd.tvmode.refresh));
	gdl_display_info_t di_sd;
	memset(&di_sd, 0, sizeof(di_sd));
	di_sd.id = GDL_DISPLAY_ID_1;
	switch(mode.refresh) {
		case GDL_REFRESH_29_97 :
		case GDL_REFRESH_30    :
		case GDL_REFRESH_59_94 :
		case GDL_REFRESH_60    :
			di_sd.tvmode.width      = 720;
			di_sd.tvmode.height     = 480;
			di_sd.tvmode.refresh    = GDL_REFRESH_59_94; 
			di_sd.tvmode.interlaced = GDL_TRUE;
			break;
		case GDL_REFRESH_25 :
		case GDL_REFRESH_50 :
		default             :
			di_sd.tvmode.width      = 720;
			di_sd.tvmode.height     = 576;
			di_sd.tvmode.refresh    = GDL_REFRESH_50;
			di_sd.tvmode.interlaced = GDL_TRUE;
			break;
	} // switch
	di_sd.tvmode.stereo_type = GDL_STEREO_NONE;
	di_sd.bg_color           = 0;
	di_sd.gamma              = GDL_GAMMA_LINEAR;
	di_sd.color_space        = GDL_COLOR_SPACE_BT601;
	isyslog("Video [ANALOG] %dx%d%s%s", di_sd.tvmode.width, di_sd.tvmode.height, di_sd.tvmode.interlaced?"i":"p", REFRESH_STR(di_sd.tvmode.refresh));

	if(GetCurrentTVMode(old, GDL_DISPLAY_ID_0))
		if(old.height > mode.height) set_display_first = false;
	RemoveOutput(ICE_OUTPUT_HDMI);
	if(set_display_first) {
		int retry=SET_DISPLAY_INFO_RETRY;
		while(retry--) {
			GDLCHECK_IGN(GDL_ERR_DISPLAY_CONFLICT, gdl_set_display_info(&di_hd));
			if(GDL_ERR_DISPLAY_CONFLICT!=ret) break;
			cCondWait::SleepMs(100);
		} // while
		GDLCHECK_IGN(GDL_ERR_DISPLAY_CONFLICT, gdl_set_display_info(&di_sd));
	} // if

	gdl_vid_policy_t policy = GDL_VID_POLICY_BLACK_FRAME;//GDL_VID_POLICY_RESIZE; Don't use resize since dst_rect will be changed and may cause a conflict in gdl_set_display_info
	gdl_rectangle_t dst_rect = {origin:{x:0, y:0}, width:di_hd.tvmode.width, height:di_hd.tvmode.height};
	GDLCHECK(gdl_plane_reset(ICE_VID_PLANE));
	GDLCHECK(gdl_plane_config_begin(ICE_VID_PLANE));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_VID_MISMATCH_POLICY, &policy));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dst_rect));
	GDLCHECK(gdl_plane_config_end(GDL_FALSE));

	policy = GDL_VID_POLICY_RESIZE;// But it is still needed for pip
	GDLCHECK(gdl_plane_reset(ICE_PIP_PLANE));
	GDLCHECK(gdl_plane_config_begin(ICE_PIP_PLANE));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_VID_MISMATCH_POLICY, &policy));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dst_rect));
	GDLCHECK(gdl_plane_config_end(GDL_FALSE));

	policy = GDL_VID_POLICY_RESIZE;// But it is still needed for pip
	GDLCHECK(gdl_plane_reset(ICE_EXT_PLANE));
	GDLCHECK(gdl_plane_config_begin(ICE_EXT_PLANE));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_VID_MISMATCH_POLICY, &policy));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dst_rect));
	GDLCHECK(gdl_plane_config_end(GDL_FALSE));

	if(device) device->UpdateScreen(mode);
#if 0	
	GDLCHECK(gdl_plane_reset(ICE_OSD_PLANE));
	GDLCHECK(gdl_plane_config_begin (ICE_OSD_PLANE));
	GDLCHECK(gdl_plane_set_rect (GDL_PLANE_DST_RECT, &rect));
	GDLCHECK(gdl_plane_config_end (GDL_FALSE));
#else
	GDLCHECK(gdl_plane_reset(ICE_OSD_PLANE));
	GDLCHECK(gdl_plane_config_begin (ICE_OSD_PLANE));
	gdl_rectangle_t src_rect = {origin:{x:0, y:0}, width:di_hd.tvmode.width, height:di_hd.tvmode.height};
	GetOsdSize(src_rect.width, src_rect.height);
#ifdef CE4100
	gdl_boolean_t scale = GDL_FALSE;
	if((src_rect.width != dst_rect.width) || (src_rect.height != dst_rect.height))
		scale = GDL_TRUE;
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_SCALE, &scale));
	GDLCHECK(gdl_plane_set_rect (GDL_PLANE_SRC_RECT, &src_rect));
#else
	//4200 Does NOT support SCALE??
	//Center output...
/*
	dst_rect.origin.x = (dst_rect.width -src_rect.width )>>1;
	dst_rect.origin.y = (dst_rect.height-src_rect.height)>>1;
	dst_rect.width    = src_rect.width;
	dst_rect.height   = src_rect.height;
*/
/*
	gdl_boolean_t scale = GDL_FALSE;
	if((src_rect.width != dst_rect.width) || (src_rect.height != dst_rect.height))
		scale = GDL_TRUE;
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_SCALE, &scale));
	GDLCHECK(gdl_plane_set_rect (GDL_PLANE_SRC_RECT, &src_rect));
*/
	gdl_boolean_t scale = GDL_FALSE;
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_SCALE, &scale));
#endif
	GDLCHECK(gdl_plane_set_rect (GDL_PLANE_DST_RECT, &dst_rect));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_PIXEL_FORMAT, GDL_PF_ARGB_32));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_SRC_COLOR_SPACE, GDL_COLOR_SPACE_RGB));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_REVERSE_GAMMA_TYPE, GDL_GAMMA_LINEAR));
//gdl_vid_policy_t policy = GDL_VID_POLICY_RESIZE;
//GDLCHECK(gdl_plane_set_attr(GDL_PLANE_VID_MISMATCH_POLICY, &policy));
	GDLCHECK(gdl_plane_config_end (GDL_FALSE));
#endif
	GDLCHECK(gdl_plane_reset(ICE_TXT_PLANE));
	GDLCHECK(gdl_plane_config_begin (ICE_TXT_PLANE));
	GDLCHECK(gdl_plane_set_attr(GDL_PLANE_SCALE, &scale));
	GDLCHECK(gdl_plane_set_rect (GDL_PLANE_DST_RECT, &dst_rect));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_PIXEL_FORMAT, GDL_PF_ARGB_8));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_SRC_COLOR_SPACE, GDL_COLOR_SPACE_RGB));
	GDLCHECK(gdl_plane_set_uint (GDL_PLANE_REVERSE_GAMMA_TYPE, GDL_GAMMA_LINEAR));
	GDLCHECK(gdl_plane_config_end (GDL_FALSE));

	if(!set_display_first) {
		int retry=SET_DISPLAY_INFO_RETRY;
		while(retry--) {
			GDLCHECK_IGN(GDL_ERR_DISPLAY_CONFLICT, gdl_set_display_info(&di_hd));
			if(GDL_ERR_DISPLAY_CONFLICT!=ret) break;
			cCondWait::SleepMs(100);
		} // while
		GDLCHECK_IGN(GDL_ERR_DISPLAY_CONFLICT, gdl_set_display_info(&di_sd));
	} // if
	updateAudio();
	return (GDL_SUCCESS==ret);
} // cPluginICE::SetCurrentTVMode

bool cPluginICE::GetCurrentTVMode(gdl_tvmode_t &mode, gdl_display_id_t id) {
	ICE_DBG(DBG_FN, "cPluginICE::GetCurrentTVMode %d", id);
	gdl_ret_t ret = GDL_SUCCESS;
	gdl_display_info_t di;
	GDLCHECK_IGN(GDL_ERR_TVMODE_UNDEFINED, gdl_get_display_info(id, &di));
	if(GDL_SUCCESS != ret) return false;
	mode = di.tvmode;
	return true;
} //  cPluginICE::GetCurrentTVMode

void cPluginICE::GetOsdSize(gdl_uint32 &Width, gdl_uint32 &Height) {
//	ICE_DBG(DBG_FN,"cPluginICE::GetOsdSize %s %p", Skins.Current()?Skins.Current()->Name():"NULL", device);
#ifdef ICE_REEL_OSD
	// The CreateTrueColorOsd of the reel osd is only designed/tested for this resolution
	if(Skins.Current() && !strcmp("Reel", Skins.Current()->Name())) {
		Width  = 720; 
		Height = 576;
		return;
	} // if
#endif
	Width  = 720;
	Height = device ? (vsNTSC==device->GetVideoSystem()) ? 480 : 576 : 576;
	if(options.lockOsdToSd) return;
	gdl_tvmode_t mode;
	if(GetCurrentTVMode(mode) || GetTVMode(options.tv_mode, mode, false)) {
		Width  = mode.width;
		Height = mode.height;
	} // if
} // cPluginICE::GetOsdSize

bool cPluginICE::GetTVMode(tICE_TVMode in, gdl_tvmode_t &out, bool check) {
	memset(&out, 0, sizeof(out));
	switch(in) {
		case ICE_TVMODE_NTSC       : // no break;
		case ICE_TVMODE_480I       : out.width =  720; out.height =  480; out.refresh = GDL_REFRESH_59_94; out.interlaced  = GDL_TRUE;  break;
		case ICE_TVMODE_480P       : out.width =  720; out.height =  480; out.refresh = GDL_REFRESH_59_94; out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_PAL        : // no break;
		case ICE_TVMODE_576I       : out.width =  720; out.height =  576; out.refresh = GDL_REFRESH_50;    out.interlaced  = GDL_TRUE;  break;
		case ICE_TVMODE_576P       : out.width =  720; out.height =  576; out.refresh = GDL_REFRESH_50;    out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_720P59_94  : out.width = 1280; out.height =  720; out.refresh = GDL_REFRESH_59_94; out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_720P50     : out.width = 1280; out.height =  720; out.refresh = GDL_REFRESH_50;    out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_1080I59_94 : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_59_94; out.interlaced  = GDL_TRUE;  break;
		case ICE_TVMODE_1080I50    : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_50;    out.interlaced  = GDL_TRUE;  break;
		case ICE_TVMODE_1080P23_98 : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_23_98; out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_1080P25    : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_25;    out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_1080P29_97 : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_29_97; out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_1080P50    : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_50;    out.interlaced  = GDL_FALSE; break;
		case ICE_TVMODE_1080P59_94 : out.width = 1920; out.height = 1080; out.refresh = GDL_REFRESH_59_94; out.interlaced  = GDL_FALSE; break;
		default:
			if(check) esyslog("cPluginICE::GetTVMode: unhandled mode %d %s", in, TVMODE_STR(in));
			return false;
	} // switch
	out.stereo_type = GDL_STEREO_NONE;
	if(check && (GDL_SUCCESS != gdl_check_tvmode(GDL_DISPLAY_ID_0, &out))) {
		isyslog("Video [HDMI] %dx%d%s%s not supported", out.width, out.height, out.interlaced?"i":"p",REFRESH_STR(out.refresh));
		return false;
	} // if
	return true;
} //  cPluginICE::GetTVMode

void cPluginICE::CheckHDMIDeviceChange() {
	if(!options.hdmi_device_check) return;
	gdl_hdmi_sink_info_t info;
	if(GDL_SUCCESS==gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_SINK_INFO, &info, sizeof(info))) {
		if(strlen((const char *)info.name) && strcmp(options.hdmi_device, (const char *)info.name)) {
			if(strlen(options.hdmi_device)) {
				isyslog("HDMI device changed from \"%s\" to \"%s\": using native mode for new device", options.hdmi_device, info.name);
				options.tv_mode = ICE_TVMODE_NATIVE;
			} // if
			strncpy(options.hdmi_device, (const char *)info.name, sizeof(options.hdmi_device));
		} // if
	} // if
} // cPluginICE::CheckHDMIDeviceChange

bool cPluginICE::updateVideo(bool force) {
	updateEDID();
	CheckHDMIDeviceChange();
	if(device) device->UpdateOptions();
	gdl_tvmode_t mode;
	if(ICE_TVMODE_NATIVE==options.tv_mode) {
		if(!GetNativeTVMode(mode)) goto default_mode;
		if(device) {
			gdl_refresh_t old = mode.refresh;
			mode.refresh = (vsNTSC==device->GetVideoSystem()) ? GDL_REFRESH_59_94 : GDL_REFRESH_50;
			if(GDL_SUCCESS != gdl_check_tvmode(GDL_DISPLAY_ID_0, &mode)) {
				isyslog("Video [HDMI] %dx%d%s%s not supported", mode.width, mode.height, mode.interlaced?"i":"p",REFRESH_STR(mode.refresh));
				mode.refresh = old;
				if((720 < mode.height) && !mode.interlaced) {
					mode.refresh = (vsNTSC==device->GetVideoSystem()) ? GDL_REFRESH_29_97 : GDL_REFRESH_25;
					if(GDL_SUCCESS != gdl_check_tvmode(GDL_DISPLAY_ID_0, &mode)) {
						isyslog("Video [HDMI] %dx%d%s%s not supported", mode.width, mode.height, mode.interlaced?"i":"p",REFRESH_STR(mode.refresh));
						mode.refresh = old;
					} // if
				} // if
			} // if
		} // if
	} else if(ICE_TVMODE_SOURCE==options.tv_mode) {
		if(!device) goto default_mode;
		int Width, Height;
		double Aspect;
		device->GetVideoSize(Width, Height, Aspect);
		//TODO: Also handle frequency fallback
		if(Height <= 0) {
			goto default_mode;
		} else if(Height <= 480) {
			if(!GetTVMode(ICE_TVMODE_480I, mode)) 
				if(!GetTVMode(ICE_TVMODE_480P, mode)) goto default_mode;
		} else if(Height <= 576) {
			if(!GetTVMode(ICE_TVMODE_576I, mode))
				if(!GetTVMode(ICE_TVMODE_576P, mode)) goto default_mode;
		} else if(Height <= 720) {
			if(!GetTVMode((vsNTSC==device->GetVideoSystem()) ? ICE_TVMODE_720P59_94 : ICE_TVMODE_720P50, mode)) goto default_mode;
		} else {
			if(!GetTVMode((vsNTSC==device->GetVideoSystem()) ? ICE_TVMODE_1080I59_94 : ICE_TVMODE_1080I50, mode))
				if(!GetTVMode((vsNTSC==device->GetVideoSystem()) ? ICE_TVMODE_1080P59_94 : ICE_TVMODE_1080P50, mode))
					if(!GetTVMode((vsNTSC==device->GetVideoSystem()) ? ICE_TVMODE_1080P29_97 : ICE_TVMODE_1080P25, mode))
						if(!GetTVMode((vsNTSC==device->GetVideoSystem()) ? ICE_TVMODE_720P59_94 : ICE_TVMODE_720P50, mode)) goto default_mode;
		} // if
	} else if(!GetTVMode(options.tv_mode, mode)) goto default_mode;
	gdl_tvmode_t curr_mode;
	if(force || !GetCurrentTVMode(curr_mode) || memcmp(&curr_mode, &mode, sizeof(mode)))
		return SetCurrentTVMode(mode);
	return updateAudio();
default_mode:
	if(!GetCurrentTVMode(mode))
		if(!GetNativeTVMode(mode))
			if(!GetTVMode(vsNTSC==device->GetVideoSystem() ? ICE_TVMODE_480P : ICE_TVMODE_576P, mode))
				return false;
	return SetCurrentTVMode(mode);
} //  cPluginICE::updateVideo

bool cPluginICE::GetNativeTVMode(gdl_tvmode_t &mode, gdl_pd_id_t id) {
	ICE_DBG(DBG_FN, "cPluginICE::GetNativeTVMode %d", id);
	gdl_ret_t ret = GDL_SUCCESS;
	switch(id) {
		case GDL_PD_ID_HDMI:
			GDLCHECK(gdl_port_recv(id, GDL_PD_RECV_HDMI_NATIVE_MODE, &mode, sizeof(mode))); 
			//isyslog("HDMI Native-Mode %dx%d%s%s", mode.width, mode.height, mode.interlaced?"i":"p",REFRESH_STR(mode.refresh));
			break;
		case GDL_PD_ID_INTTVENC:
			memset(&mode, 0, sizeof(mode));
			if(vsNTSC==device->GetVideoSystem()) {
				mode.width       = 720;
				mode.height      = 480;
				mode.refresh     = GDL_REFRESH_59_94;
				mode.interlaced  = GDL_TRUE;
			} else {
				mode.width       = 720;
				mode.height      = 576;
				mode.refresh     = GDL_REFRESH_50;
				mode.interlaced  = GDL_TRUE;
			} // if
			mode.stereo_type = GDL_STEREO_NONE;
			//isyslog("TVENC Native-Mode %dx%d%s%s", mode.width, mode.height, mode.interlaced?"i":"p",REFRESH_STR(mode.refresh));
			break; 
		default: return false;
	} // switch
	return (GDL_SUCCESS==ret);
} // cPluginICE::GetNativeTVMode

void cPluginICE::enableOutput(bool Enable) {
	ICE_DBG(DBG_FN, "cPluginICE::enableOutput %d", Enable);
	if(Enable) GDLSETPORT_BOOL(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP, GDL_FALSE);
	GDLSETPORT_BOOL(GDL_PD_ID_HDMI,               GDL_PD_ATTR_ID_POWER, Enable ? GDL_TRUE : GDL_FALSE);
	GDLSETPORT_BOOL(GDL_PD_ID_INTTVENC,           GDL_PD_ATTR_ID_POWER, Enable ? GDL_TRUE : GDL_FALSE);
	GDLSETPORT_BOOL(GDL_PD_ID_INTTVENC_COMPONENT, GDL_PD_ATTR_ID_POWER, Enable ? GDL_TRUE : GDL_FALSE);
	if(Enable) updateVideo(true);
} // cPluginICE::enableOutput

static bool audio_output_from_hdmi_audio( gdl_hdmi_audio_cap_t in, const ismd_audio_stream_info_t *max, ismd_audio_output_config_t *out) {
	if(!out) return false;
	switch(in.max_channels) {
		case 2: out->ch_config = ISMD_AUDIO_STEREO; break;
		case 6: out->ch_config = ISMD_AUDIO_5_1; break;
		case 8: out->ch_config = ISMD_AUDIO_7_1; break;
		default: isyslog("Invalid channel %d", in.max_channels); return false;
	} // switch
	//TODO: Find a way to select video/audio modes at max sample rate
	if((in.fs & GDL_HDMI_AUDIO_FS_192_KHZ       ) && max && max->sample_rate && (max->sample_rate >= 192000))
		out->sample_rate = 192000;
	else if((in.fs & GDL_HDMI_AUDIO_FS_176_4_KHZ) && max && max->sample_rate && (max->sample_rate >= 176400))
		out->sample_rate = 176400;
	else if((in.fs & GDL_HDMI_AUDIO_FS_96_KHZ   ) && max && max->sample_rate && (max->sample_rate >= 96000))
		out->sample_rate =  96000;
	else if((in.fs & GDL_HDMI_AUDIO_FS_88_2_KHZ ) && max && max->sample_rate && (max->sample_rate >= 88200))
		out->sample_rate =  88200;
	else if((in.fs & GDL_HDMI_AUDIO_FS_48_KHZ   ) && max && max->sample_rate && (max->sample_rate >= 48000))
		out->sample_rate =  48000;
	else if((in.fs & GDL_HDMI_AUDIO_FS_44_1_KHZ ) && max && max->sample_rate && (max->sample_rate >= 44100))
		out->sample_rate =  44100;
	else if((in.fs & GDL_HDMI_AUDIO_FS_32_KHZ   ) && max && max->sample_rate && (max->sample_rate >= 32000))
		out->sample_rate =  32000;
	else { isyslog("Invalid sample rate %d", in.fs); 
		return false;
	}
	if(in.ss_bitrate & GDL_HDMI_AUDIO_SS_UNDEFINED)
		out->sample_size = 0;
	else if((in.ss_bitrate & GDL_HDMI_AUDIO_SS_24) && max && max->sample_size && (max->sample_size >= 24))
		out->sample_size = 24;
	else if((in.ss_bitrate & GDL_HDMI_AUDIO_SS_20) && max && max->sample_size && (max->sample_size >= 20))
		out->sample_size = 20;
	else if((in.ss_bitrate & GDL_HDMI_AUDIO_SS_16) && max && max->sample_size && (max->sample_size >= 16))
		out->sample_size = 16;
	else {
		if(in.format == GDL_HDMI_AUDIO_FORMAT_PCM)
			return false;
		out->sample_size = 16;
	}
	out->stream_delay = 0;
	return true;
} // audio_output_from_hdmi_audio

static void set_best_audio_output (ismd_audio_output_config_t in, ismd_audio_output_config_t *out, ismd_audio_channel_config_t max_ch = ISMD_AUDIO_CHAN_CONFIG_INVALID, int max_sr = 0, int max_ss = 0) {
	if(!out) return;
	if((in.ch_config   >= out->ch_config  ) && ((max_ch != ISMD_AUDIO_CHAN_CONFIG_INVALID) ? (in.ch_config   <= max_ch) : true) &&
	   (in.sample_rate >= out->sample_rate) && (max_sr                                     ? (in.sample_rate <= max_sr) : true) &&
	   (in.sample_size >= out->sample_size) && (max_ss                                     ? (in.sample_size <= max_ss) : true))
		*out = in;
//	else
//		isyslog("rejected (%d < %d) (%d < %d) (%d < %d)", in.ch_config, out->ch_config, in.sample_rate, out->sample_rate, in.sample_size, out->sample_size);
} // set_best_audio_output

void cPluginICE::MainThreadHook() {
#ifdef MUTE_AUDIO_AFTER_CHANGE
	if( enableOutputs && (enableOutputs < cTimeMs::Now())) {
		EnableOutput(ICE_OUTPUT_ANALOG, true);
		EnableOutput(ICE_OUTPUT_SPDIF, true);
		EnableOutput(ICE_OUTPUT_HDMI, true);
		enableOutputs = 0;
	} // if
#endif
} // cPluginICE::MainThreadHook

bool cPluginICE::setMasterClock(int rate) {
	unsigned int freq;
	switch(rate) {
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
		default:
			freq = 36864000;
			break;
		case 44100:
		case 88200:
		case 176400:
			freq = 33868800;
			break;
		case 32000:
			freq = 24576000;
			break;
	} // switch
	ismd_result_t ret = ISMD_SUCCESS;
	if(AUDIO_INVALID_HANDLE==aud_prc) SMDCHECK_CLEANUP(ismd_audio_open_global_processor(&aud_prc));
#ifdef ICE_EXTERNAL_AUDIO_MASTER
	SMDCHECK_CLEANUP(ismd_audio_configure_master_clock(aud_prc, freq, ISMD_AUDIO_CLK_SRC_EXTERNAL));
	isyslog("Using external audio master clock (%d)", freq);
#else
	SMDCHECK_CLEANUP(ismd_audio_configure_master_clock(aud_prc, freq, ISMD_AUDIO_CLK_SRC_INTERNAL));
	isyslog("Using internal audio master clock (%d)", freq);
#endif
	return true;
cleanup:
	return false;
} // cPluginICE::setMasterClock

bool cPluginICE::updateAudio() {
	ICE_DBG(DBG_FN, "cPluginICE::updateAudio");
	ismd_audio_format_t      aud_codec = ISMD_AUDIO_MEDIA_FMT_PCM;
	ismd_audio_stream_info_t aud_info  = {bitrate:0, sample_rate:48000, sample_size:16, channel_config:0, channel_count:2, algo:ISMD_AUDIO_MEDIA_FMT_INVALID};
	cICEDeviceSMD *smd_device = dynamic_cast<cICEDeviceSMD *>(device);
	if(smd_device) {
		aud_codec = smd_device->aud_codec;
		if(smd_device->aud_prop.algo!=ISMD_AUDIO_MEDIA_FMT_INVALID)
			aud_info = smd_device->aud_prop;
	} // if
	setMasterClock(aud_info.sample_rate);
#ifdef MUTE_AUDIO_AFTER_CHANGE
	enableOutputs = cTimeMs::Now() + ENABLE_AUDIO_TIMEOUT_MS;
	EnableOutput(ICE_OUTPUT_ANALOG, false);
	EnableOutput(ICE_OUTPUT_SPDIF, false);
	EnableOutput(ICE_OUTPUT_HDMI, false);
#endif
	ismd_audio_output_config_t cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.out_mode     = ISMD_AUDIO_OUTPUT_PCM;
	cfg.ch_config    = ISMD_AUDIO_STEREO;
	cfg.stream_delay = options.analog_delay;
	cfg.sample_size  = aud_info.sample_size;
	cfg.sample_rate  = aud_info.sample_rate;
	InsertOutput(ICE_OUTPUT_ANALOG, &cfg);

	memset(&cfg, 0, sizeof(cfg));
	switch(aud_codec) {
		case ISMD_AUDIO_MEDIA_FMT_DTS : // no break;
		case ISMD_AUDIO_MEDIA_FMT_DD  : cfg.out_mode  = options.spdif_audio; 
		                                cfg.ch_config = options.spdif_channels;
		                                if(ISMD_AUDIO_OUTPUT_PASSTHROUGH == cfg.out_mode) {
		                                	switch(aud_info.channel_count) {
		                                		case 6 : cfg.ch_config = ISMD_AUDIO_5_1; break;
		                                		case 8 : cfg.ch_config = ISMD_AUDIO_7_1; break;
		                                		default: cfg.ch_config = ISMD_AUDIO_STEREO;
		                                	} // switch
		                                } // if
		                                break;
		default                       : cfg.out_mode  = ISMD_AUDIO_OUTPUT_PCM;
		                                cfg.ch_config = ISMD_AUDIO_STEREO;
	} // switch
	cfg.stream_delay = options.spdif_delay;
	cfg.sample_size  = aud_info.sample_size;//16;
	cfg.sample_rate  = aud_info.sample_rate;//48000;
	if(ISMD_AUDIO_OUTPUT_INVALID==options.spdif_audio) RemoveOutput(ICE_OUTPUT_SPDIF);
	else InsertOutput(ICE_OUTPUT_SPDIF, &cfg);

	if(!updateEDID()) goto cleanup;
	if(ISMD_AUDIO_OUTPUT_INVALID==options.hdmi_audio) goto cleanup;

	ismd_audio_output_config_t pass_cfg, pcm_cfg;
	memset(&pass_cfg, 0, sizeof(pass_cfg));
	memset(&pcm_cfg, 0, sizeof(pcm_cfg));
	pass_cfg.out_mode = pcm_cfg.out_mode = ISMD_AUDIO_OUTPUT_INVALID;
	gdl_hdmi_audio_ctrl_t ctrl;
	ctrl.cmd_id = GDL_HDMI_AUDIO_GET_CAPS;
	ctrl.data._get_caps.index = 0;
	while(GDL_SUCCESS == gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_AUDIO_CTRL, &ctrl, sizeof(ctrl))) {
		ismd_audio_output_config_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		if(aud_codec == HDMI_AUDIO_TO_MEDIA_FORMAT(ctrl.data._get_caps.cap.format)) {
			cfg.out_mode     = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
			cfg.stream_delay = options.hdmi_delay;
			if(audio_output_from_hdmi_audio( ctrl.data._get_caps.cap, &aud_info, &cfg))
				set_best_audio_output(cfg, &pass_cfg/*, options.hdmi_channels*/);
		} else if (GDL_HDMI_AUDIO_FORMAT_PCM == ctrl.data._get_caps.cap.format) {
			cfg.out_mode     = ISMD_AUDIO_OUTPUT_PCM;
			cfg.stream_delay = options.hdmi_delay;
			if(audio_output_from_hdmi_audio( ctrl.data._get_caps.cap, &aud_info, &cfg))
				set_best_audio_output(cfg, &pcm_cfg/*, options.hdmi_channels*/);
		} // if
		ctrl.data._get_caps.index++;
	} // while
	if(GDL_HDMI_USE_EDID_REAL != options.edid_mode) pcm_cfg.ch_config = ISMD_AUDIO_STEREO; // Don´t allow more than 2 channels if real edid is not used
	if((ISMD_AUDIO_OUTPUT_INVALID!=pass_cfg.out_mode) && (ISMD_AUDIO_OUTPUT_PASSTHROUGH==options.hdmi_audio)) {
		if(!InsertOutput(ICE_OUTPUT_HDMI, &pass_cfg)) goto cleanup;
	} else if(ISMD_AUDIO_OUTPUT_INVALID!=pcm_cfg.out_mode) {
		if(!InsertOutput(ICE_OUTPUT_HDMI, &pcm_cfg)) goto cleanup;
	} else {
		isyslog("No valid HDMI audio output available!");
		goto cleanup;
	} // if
	return true;
cleanup:
	RemoveOutput(ICE_OUTPUT_HDMI);
	return false;
} // cPluginICE::updateAudio

bool cPluginICE::updateEDID() {
	GDLSETPORT_BOOL(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_DVI, GDL_FALSE);
	gdl_hdmi_use_edid_t use_edid = GDL_HDMI_USE_EDID_NONE;
	gdl_pd_attribute_t attr;
	if(GDL_SUCCESS == gdl_port_get_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_USE_EDID, &attr))
		use_edid = (gdl_hdmi_use_edid_t)attr.content._uint.value;
	if(options.edid_mode!=use_edid) {
		use_edid=options.edid_mode;
		if(GDL_SUCCESS != gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_USE_EDID, &use_edid)) {
			isyslog("Unable to use edid -> force DVI mode!");
			GDLSETPORT_BOOL(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_DVI, GDL_TRUE);
			goto cleanup;
		} // if
		PRINT_HDMI_INFO(this);
	} // if
	return (GDL_HDMI_USE_EDID_NONE!=use_edid);
cleanup:
	use_edid = GDL_HDMI_USE_EDID_NONE;
	gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_USE_EDID, &use_edid);
	return false;
} // cPluginICE::updateEDID

bool cPluginICE::InsertOutput(int output, ismd_audio_output_config_t *cfg) {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_audio_output_t aud_out;
	if(AUDIO_INVALID_HANDLE==aud_prc) SMDCHECK_CLEANUP(ismd_audio_open_global_processor(&aud_prc));
	SMDCHECK_IGN(ISMD_ERROR_OPERATION_FAILED, ismd_audio_output_get_handle_by_hw_id(aud_prc, output, &aud_out));
	if(ISMD_SUCCESS==ret) {
		ismd_audio_output_config_t cur = *cfg;
		SMDCHECK(ismd_audio_output_get_mode          (aud_prc, aud_out, &cur.out_mode));
		SMDCHECK(ismd_audio_output_get_channel_config(aud_prc, aud_out, &cur.ch_config));
		SMDCHECK(ismd_audio_output_get_sample_rate   (aud_prc, aud_out, &cur.sample_rate));
		SMDCHECK(ismd_audio_output_get_sample_size   (aud_prc, aud_out, &cur.sample_size));
		SMDCHECK(ismd_audio_output_get_delay         (aud_prc, aud_out, &cur.stream_delay));
		if(!memcmp(&cur, cfg, sizeof(cur))) {
			PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Unchanged:", cfg);
			return true;
		} // if
#if 0
		SMDCHECK(ismd_audio_output_disable(aud_prc, aud_out));
		SMDCHECK(ismd_audio_output_set_mode          (aud_prc, aud_out, cfg->out_mode));
		SMDCHECK(ismd_audio_output_set_channel_config(aud_prc, aud_out, cfg->ch_config));
		SMDCHECK(ismd_audio_output_set_sample_rate   (aud_prc, aud_out, cfg->sample_rate));
		SMDCHECK(ismd_audio_output_set_sample_size   (aud_prc, aud_out, cfg->sample_size));
		SMDCHECK(ismd_audio_output_set_delay         (aud_prc, aud_out, cfg->stream_delay));
		SMDCHECK(ismd_audio_output_enable(aud_prc, aud_out));
		PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Changed:", cfg);
		return true;
	} else {
		SMDCHECK_CLEANUP(ismd_audio_add_phys_output(aud_prc, output, *cfg, &aud_out));
		SMDCHECK_CLEANUP(ismd_audio_output_enable(aud_prc, aud_out));
		PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Created:", cfg);
		return true;
	} // if
#else
		SMDCHECK(ismd_audio_remove_output(aud_prc, aud_out));
	} // if
	if(ISMD_AUDIO_OUTPUT_INVALID == cfg->out_mode) {
		PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Disabled:", cfg);
		return true;
	} // if
	SMDCHECK_CLEANUP(ismd_audio_add_phys_output(aud_prc, output, *cfg, &aud_out));
#ifdef MUTE_AUDIO_AFTER_CHANGE
	SMDCHECK_CLEANUP(ismd_audio_output_mute(aud_prc, aud_out, true, 0));
#endif
	SMDCHECK_CLEANUP(ismd_audio_output_enable(aud_prc, aud_out));
#endif
	PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Changed:", cfg);
	return true;
cleanup:
	PRINT_AUDIO_OUTPUT_CFG(AUDIO_HW_OUTPUT_STR(output), "Failed:", cfg);
	return false;
} // cPluginICE::InsertOutput

bool cPluginICE::EnableOutput(int output, bool enable) {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_audio_output_t aud_out;
	if(AUDIO_INVALID_HANDLE==aud_prc) SMDCHECK_CLEANUP(ismd_audio_open_global_processor(&aud_prc));
	SMDCHECK_IGN(ISMD_ERROR_OPERATION_FAILED, ismd_audio_output_get_handle_by_hw_id(aud_prc, output, &aud_out));
	if(ISMD_SUCCESS!=ret) goto cleanup;
	if(enable) {
		SMDCHECK_CLEANUP(ismd_audio_output_mute(aud_prc, aud_out, false, ISMD_AUDIO_RAMP_MS_MAX));
	} else {
		SMDCHECK_CLEANUP(ismd_audio_output_mute(aud_prc, aud_out, true, 0));
	}
	return true;
cleanup:
	return false;
} // cPluginICE::EnableOutput

void cPluginICE::RemoveOutput(int output) {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_audio_output_t aud_out;
	if(AUDIO_INVALID_HANDLE==aud_prc) SMDCHECK(ismd_audio_open_global_processor(&aud_prc));
	SMDCHECK_IGN(ISMD_ERROR_OPERATION_FAILED, ismd_audio_output_get_handle_by_hw_id(aud_prc, output, &aud_out));
	if(ISMD_SUCCESS==ret) SMDCHECK(ismd_audio_remove_output(aud_prc, aud_out));
} // cPluginICE::RemoveOutput

void cPluginICE::gdlEvent(gdl_app_event_t event) {
	switch(event) {
		case GDL_APP_EVENT_HOTPLUG_DETECT:
			isyslog("cPluginICE::gdlEvent GDL_APP_EVENT_HOTPLUG_DETECT");
			updateVideo();
			PRINT_HDMI_INFO(this);
//			updateAudio();
			break;
		case GDL_APP_EVENT_HOTPLUG_LOSS:
			isyslog("cPluginICE::gdlEvent GDL_APP_EVENT_HOTPLUG_LOSS");
			RemoveOutput(ICE_OUTPUT_HDMI);
			break;
		case GDL_APP_EVENT_MODE_DISP_0:
			isyslog("cPluginICE::gdlEvent GDL_APP_EVENT_MODE_DISP_0");
			break;
		case GDL_APP_EVENT_MODE_DISP_1:
			isyslog("cPluginICE::gdlEvent GDL_APP_EVENT_MODE_DISP_1");
			break;
		default:
			break;
	} // switch
} // cPluginICE::gdlEvent

VDRPLUGINCREATOR(cPluginICE);
