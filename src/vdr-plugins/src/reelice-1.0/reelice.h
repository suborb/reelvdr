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

#ifndef REELICE_H_INCLUDED
#define REELICE_H_INCLUDED

#include "ice_types.h"
#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/reelboxbase.h>
#include <vdr/tools.h>
#include <libgdl.h>
#include <ismd_demux.h>
#include <ismd_audio.h>
#include <ismd_viddec.h>
#include <ismd_vidpproc.h>
#include <ismd_vidrend.h>
#include <ismd_bufmon.h>

static const char *VERSION        = "1.0.0";
static const char *DESCRIPTION    = "Output device for reelbox on IntelCE";

//#define ICE_CE4100
//#define ICE_HAS_DUMMY_TUNER
//#define ICE_EXTERNAL_AUDIO_MASTER
//#define ICE_CE4100
#define ICE_REEL_OSD
#define ICE_TRIGGER_AR_ON_SET_VIDEO_FORMAT

#ifdef ICE_CE4100
#define ICE_EXTERNAL_AUDIO_MASTER
#endif

#define ICE_VID_PLANE  GDL_PLANE_ID_UPP_A
#define ICE_PIP_PLANE  GDL_PLANE_ID_UPP_D
#define ICE_OSD_PLANE  GDL_PLANE_ID_UPP_C
#define ICE_EXT_PLANE  GDL_PLANE_ID_UPP_B
#define ICE_TXT_PLANE  GDL_PLANE_ID_IAP_A

#define ICE_OUTPUT_ANALOG GEN3_HW_OUTPUT_I2S0
#define ICE_OUTPUT_SPDIF  GEN3_HW_OUTPUT_SPDIF
#define ICE_OUTPUT_HDMI   GEN3_HW_OUTPUT_HDMI

#define PIC_STREAM_TIMEOUT 3
#define PIC_INTERVAL   15
#define PIC_IMAGE_DIR "/usr/share/reel/audio/audio_%d.mpv"

#define MAX_HDMI_DEVICE 16

class cPluginICE;

class cICEOsdProvider : public cOsdProvider {
friend class cPluginICE;
protected:
	cICEOsdProvider(cPluginICE *Plugin);
public:
	virtual ~cICEOsdProvider();
protected:
	cPluginICE *plugin;
	virtual cOsd *CreateOsd(int Left, int Top, uint Level);
#ifdef ICE_REEL_OSD
	bool hasReelOsd;
	virtual cOsd *CreateTrueColorOsd(int Left, int Top, uint Level);
#endif /*ICE_REEL_OSD*/
	virtual uchar *GetOsdImage(int &SizeRow, int &SizeBpp, int &SizeX, int &SizeY);
}; // cICEOsdProvider

enum tICE_SetupMode {
	ICE_SETUP_VIDEO_AUDIO,
	ICE_SETUP_AUDIO_ONLY,
	ICE_SETUP_VIDEO_ONLY,
}; // tICE_SetupMode

class cICESetup : public cMenuSetupPage {
friend class cPluginICE;
protected:
	cICESetup(cPluginICE *Plugin, tICE_SetupMode Mode, 
              const char *Title=NULL, bool InstallWiz=false);
	void InitVideo();
	void Setup();
public:
	virtual eOSState ProcessKey(eKeys Key);
	virtual void Store();
	static ismd_vidpproc_scaling_policy_t GetNextScaleMode(ismd_vidpproc_scaling_policy_t mode);
protected:
	cPluginICE *plugin;
	tICE_SetupMode mode;
	bool isVideoPage;
	bool isExpert;
	void InitSettings(); // take settings from plugin->options and show it onscreen
	void ShowImage();
private:
	int TVMode;
	int DeinterlaceMode;
	int ScaleMode;
	int DeringingLevel;
	int GaussianLevel;
	int DegradeLevel;
	int PanScan;
	int OverrideFramePolarity;
	int HdmiAudioMode;
	int SpdifAudioMode;
	int HdmiChannelMode;
	int HdmiDelay;
	int SpdifDelay;
	int AnalogDelay;
	int EdidMode;
	int LastEdidMode;

	cString title;
	bool InInstallWizard;
}; // cICESetup

class cICEDevice;
class cICEBgPic : public cThread {
protected:
	int nextPic;
	cICEDevice *device;
	std::vector<uchar> Buffer;
public:
	cICEBgPic(cICEDevice *fDevice);
	virtual void Action();
}; // cICEBgPic

struct tICE_Options {
	tICE_TVMode tv_mode;
	ismd_vidpproc_deinterlace_policy_t deinterlace_mode;
	ismd_vidpproc_scaling_policy_t scale_mode;
	int deringing_level; // INVALID_LEVEL=disable / values from MIN_DERINGNING_LEVEL..MAX_DERINGNING_LEVEL
	int gaussian_level;  // INVALID_LEVEL=disable / values from MIN_GAUSSIAN_LEVEL..MAX_GAUSSIAN_LEVEL
	int degrade_level;  // INVALID_LEVEL=disable / values from MIN_DEGRADE_LEVEL..MAX_DEGRADE_LEVEL
	bool pan_scan;
	bool override_frame_polarity;
	ismd_audio_output_mode_t hdmi_audio;
	ismd_audio_channel_config_t hdmi_channels;
	int hdmi_delay;   // 0..AUDIO_MAX_OUTPUT_STREAM_DELAY
	ismd_audio_output_mode_t spdif_audio;
	ismd_audio_channel_config_t spdif_channels;
	int spdif_delay;  // 0..AUDIO_MAX_OUTPUT_STREAM_DELAY
	int analog_delay; // 0..AUDIO_MAX_OUTPUT_STREAM_DELAY
	bool lockOsdToSd;
	gdl_hdmi_use_edid_t edid_mode;
	eVideoSystem video_system;
	char hdmi_device[MAX_HDMI_DEVICE];
	bool hdmi_device_check;
	bool show_bg_pic;
	tICE_Options() :
		tv_mode(ICE_TVMODE_NATIVE),
		deinterlace_mode(ISMD_VIDPPROC_DI_POLICY_AUTO),
		scale_mode(ISMD_VIDPPROC_SCALING_POLICY_ZOOM_TO_FIT),
		deringing_level(4),
		gaussian_level(1),
		degrade_level(25),
		pan_scan(true),
		override_frame_polarity(true),
		hdmi_audio(ISMD_AUDIO_OUTPUT_PCM),
		hdmi_channels(ISMD_AUDIO_5_1),
		hdmi_delay(0),
		spdif_audio(ISMD_AUDIO_OUTPUT_PASSTHROUGH),
		spdif_channels(ISMD_AUDIO_STEREO),
		spdif_delay(0),
		analog_delay(0),
		lockOsdToSd(false),
		edid_mode(GDL_HDMI_USE_EDID_REAL),
		video_system(vsPAL),
		hdmi_device_check(true),
		show_bg_pic(true)
		{memset(&hdmi_device, 0, sizeof(hdmi_device));};
}; // tICE_Options

class cPluginICE : public cPlugin {
friend class cICEDeviceGST;
friend class cICEDeviceSMD;
friend class cICESetup;
public:
	cPluginICE(void);
	virtual ~cPluginICE();
	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void) { return DESCRIPTION; }
	virtual const char *CommandLineHelp(void) { return NULL; }
	virtual bool ProcessArgs(int argc, char *argv[]) { return true; }
	virtual bool Initialize(void);
	virtual bool Start(void) { return true; }
	virtual void Housekeeping(void) {}
	virtual const char *MainMenuEntry(void) { return NULL; }
	virtual cOsdObject *MainMenuAction(void) { return NULL; }
	virtual cMenuSetupPage *SetupMenu(void) { return new cICESetup(this, ICE_SETUP_VIDEO_AUDIO); }
	virtual bool SetupParse(const char *Name, const char *Value);
	virtual bool Service(char const *Id, void *Data);
	virtual const char **SVDRPHelpPages(void);
	virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
	virtual uchar *GrabImage(int &Size, bool Jpeg, int Quality, int SizeX, int SizeY);
#ifdef REELVDR
	virtual bool HasSetupOptions(void) { return false; }
#endif
	virtual bool Store();
	virtual bool ShowBgPic() { return options.show_bg_pic; }
	virtual void MainThreadHook();
protected:
	bool SetCurrentTVMode(gdl_tvmode_t &mode);
	bool GetCurrentTVMode(gdl_tvmode_t &mode, gdl_display_id_t id = GDL_DISPLAY_ID_0);
	void GetOsdSize(gdl_uint32 &Width, gdl_uint32 &Height);
	bool GetTVMode(tICE_TVMode in, gdl_tvmode_t &out, bool check = true);
	void CheckHDMIDeviceChange();
	bool updateVideo(bool force = false);
	bool GetNativeTVMode(gdl_tvmode_t &mode, gdl_pd_id_t id = GDL_PD_ID_HDMI);
//	void setOutput();
	void enableOutput(bool Enable);
	bool setMasterClock(int rate);
	bool updateAudio();
	bool updateEDID();
	bool InsertOutput(int output, ismd_audio_output_config_t *cfg);
	bool EnableOutput(int output, bool enable);
	void RemoveOutput(int output);
	void gdlEvent(gdl_app_event_t Event);
	static void on_gdl_event(gdl_app_event_t event, void *user_data) {((cPluginICE *)user_data)->gdlEvent(event);};
protected:
	cICEDevice *device;
	cICEOsdProvider *osd_provider;
	tICE_Options options;
	ismd_audio_processor_t aud_prc;
	uint64_t enableOutputs;
}; // cPluginICE

class cICEDevice : public cDevice {
friend class cPluginICE;
friend class cICEBgPic;
protected:
	cPluginICE *plugin;
	ePlayMode playMode;
	cICEBgPic bgPic;
	int lastVideo;
	int lastAudio;
	int lastBgPic;
protected:
	cICEDevice(cPluginICE *Plugin):plugin(Plugin),playMode(pmNone),bgPic(this){};
	virtual ~cICEDevice() {
		while(bgPic.Active()) cCondWait::SleepMs(100); // Should end soon...
	} // ~cICEDevice
	virtual void UpdateOptions(){};
	virtual void UpdateScreen(const gdl_tvmode_t &mode){};
	virtual bool SetPlayMode(ePlayMode PlayMode) {
		playMode = PlayMode;
		lastVideo = lastAudio = ::time(0);
		lastBgPic = 0;
		return true;
	} // ResetTimer
	virtual bool ShowBgPic() {
		if(plugin && !plugin->ShowBgPic()) return false;
		if((pmAudioVideo != playMode) && (pmAudioOnly != playMode)) return false; // Not in bg pic insert mode
		int now = ::time(0);
		if((now <= lastVideo+PIC_STREAM_TIMEOUT) || (now >= lastAudio+PIC_STREAM_TIMEOUT)) return false; // Still waiting
		if(IsPlayingVideo()) return false; // We already got some video data
		return true;
	} // ShowBgPic
	virtual void CheckBgPic() {
		if(!ShowBgPic()) return;
		int now = ::time(0);
		if(now >= lastBgPic + PIC_INTERVAL) {
			lastBgPic = now;
			bgPic.Start();
		} // if
	} // CheckBgPic
	virtual uchar *GrabImage(int &Size, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1) {
		return plugin ? plugin->GrabImage(Size, Jpeg, Quality, SizeX, SizeY) : NULL; 
	} // GrabImage
}; // cICEDevice

#endif /*REELICE_H_INCLUDED*/

