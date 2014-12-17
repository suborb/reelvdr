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

#define DBG_FN	DBG_STDOUT
//DBG_SYSLOG


/// TODO : put these functions in reel-lib

bool FileExists(cString file)
{
    return (*file && access(file, F_OK) == 0);
}

/* When no frontpanel is available,
 * '/dev/frontpanel' points to '/dev/null'
 * else to the appropriate device. eg.'/dev/ttyS2'
 *
 * easier : check for '/dev/.have_frontpanel'
 */
bool HasFrontPanel()
{
    if (FileExists("/dev/.have_frontpanel"))
        return true;

    // '/dev/frontpanel' points to '/dev/null' if reelbox does NOT
    // have a frontpanel
    char *realPath = ReadLink("/dev/frontpanel");
    if (realPath) {
        bool result = true;
        if (strcmp(realPath, "/dev/null")==0) // points to /dev/null ?
            result = false; // no front panel
        free(realPath);
        return result;
    }
    return false;
}



const tICE_TVMode SetupTVModes[]  = { ICE_TVMODE_NATIVE,
		ICE_TVMODE_NTSC, ICE_TVMODE_480I, ICE_TVMODE_480P,
		ICE_TVMODE_PAL, ICE_TVMODE_576I, ICE_TVMODE_576P,
		ICE_TVMODE_720P50, ICE_TVMODE_720P59_94,
		ICE_TVMODE_1080I50, ICE_TVMODE_1080I59_94,
		ICE_TVMODE_1080P23_98, ICE_TVMODE_1080P29_97, ICE_TVMODE_1080P50, ICE_TVMODE_1080P59_94,
		ICE_TVMODE_SOURCE};
tICE_TVMode TVModes[sizeof(SetupTVModes)/sizeof(SetupTVModes[0])];
const char *TVModesText[sizeof(TVModes)/sizeof(TVModes[0])];
int TVModesCount = 0;

const ismd_vidpproc_deinterlace_policy_t DeinterlaceModes[] = {
		ISMD_VIDPPROC_DI_POLICY_NONE, ISMD_VIDPPROC_DI_POLICY_SPATIAL_ONLY, ISMD_VIDPPROC_DI_POLICY_FILM, ISMD_VIDPPROC_DI_POLICY_VIDEO, ISMD_VIDPPROC_DI_POLICY_AUTO, ISMD_VIDPPROC_DI_POLICY_TOP_FIELD_ONLY, ISMD_VIDPPROC_DI_POLICY_NEVER };
const char *DeinterlaceModesText[sizeof(DeinterlaceModes)/sizeof(DeinterlaceModes[0])];

const ismd_vidpproc_scaling_policy_t ScaleModes[] = {
		ISMD_VIDPPROC_SCALING_POLICY_ZOOM_TO_FIT, ISMD_VIDPPROC_SCALING_POLICY_SCALE_TO_FIT, ISMD_VIDPPROC_SCALING_POLICY_NON_LINEAR_SCALE_TO_FIT, /*ISMD_VIDPPROC_SCALING_POLICY_NO_SCALING, */ ISMD_VIDPPROC_SCALING_POLICY_ZOOM_TO_FILL};
const char *ScaleModesText[sizeof(ScaleModes)/sizeof(ScaleModes[0])];

const ismd_audio_output_mode_t AudioModes[] = {
		ISMD_AUDIO_OUTPUT_INVALID, ISMD_AUDIO_OUTPUT_PCM,
		/*ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL, ISMD_AUDIO_OUTPUT_ENCODED_DTS, // must be handled in reelbox.c*/
		ISMD_AUDIO_OUTPUT_PASSTHROUGH};
const char *AudioModesText[sizeof(AudioModes)/sizeof(AudioModes[0])];

const ismd_audio_channel_config_t AudioChannels[] = {
		ISMD_AUDIO_STEREO, ISMD_AUDIO_5_1, ISMD_AUDIO_7_1};
const char *AudioChannelsText[sizeof(AudioChannels)/sizeof(AudioChannels[0])];

const gdl_hdmi_use_edid_t EdidModes[] = {
		GDL_HDMI_USE_EDID_REAL, GDL_HDMI_USE_EDID_SAFE/*, GDL_HDMI_USE_EDID_NONE*/};
const char *EdidModesText[sizeof(EdidModes)/sizeof(EdidModes[0])];

cICESetup::cICESetup(cPluginICE *Plugin, tICE_SetupMode Mode,
                     const char* Title, bool InstallWiz)
    :plugin(Plugin), mode(Mode), title(Title), InInstallWizard(InstallWiz) {
	ICE_DBG(DBG_FN, "cICESetup::cICESetup");
	isVideoPage = ((ICE_SETUP_VIDEO_AUDIO == mode) || (ICE_SETUP_VIDEO_ONLY == mode));
	isExpert = false;
	InitSettings();
	Setup();
} // cICESetup::cICESetup

void cICESetup::InitSettings() {
	TVMode = 0;
	TVModes[TVMode] = plugin->options.tv_mode;
	TVModesCount = 0;
	DeinterlaceMode = 0;
	for(unsigned int i=0; i < (sizeof(DeinterlaceModesText)/sizeof(DeinterlaceModesText[0])); i++) {
		DeinterlaceModesText[i] = tr(DEINTERLACE_MODE_STR(DeinterlaceModes[i]));
		if(DeinterlaceModes[i] == plugin->options.deinterlace_mode)
			DeinterlaceMode = i;
	} // for
	ScaleMode = 0;
	for(unsigned int i=0; i < (sizeof(ScaleModesText)/sizeof(ScaleModesText[0])); i++) {
		ScaleModesText[i] = tr(SCALE_MODE_STR(ScaleModes[i]));
		if(ScaleModes[i] == plugin->options.scale_mode)
			ScaleMode = i;
	} // for
	HdmiAudioMode = 0;
	SpdifAudioMode = 0;
	for(unsigned int i=0; i < (sizeof(AudioModesText)/sizeof(AudioModesText[0])); i++) {
		AudioModesText[i] = tr(AUDIO_OUTPUT_STR(AudioModes[i]));
		if(ISMD_AUDIO_OUTPUT_INVALID==AudioModes[i]) AudioModesText[i] = tr("Disabled");
		if(AudioModes[i] == plugin->options.hdmi_audio)
			HdmiAudioMode = i;
		if(AudioModes[i] == plugin->options.spdif_audio)
			SpdifAudioMode = i;
	} // for
	HdmiChannelMode = 0;
	for(unsigned int i=0; i < (sizeof(AudioChannelsText)/sizeof(AudioChannelsText[0])); i++) {
		AudioChannelsText[i] = AUDIO_CHANNEL_STR(AudioChannels[i]);
		if(AudioChannels[i] == plugin->options.hdmi_channels)
			HdmiChannelMode = i;
	} // for
	EdidMode = 0;
	for(unsigned int i=0; i < (sizeof(EdidModesText)/sizeof(EdidModesText[0])); i++) {
		EdidModesText[i] = (GDL_HDMI_USE_EDID_SAFE==EdidModes[i])?tr("Manual") :
		                   (GDL_HDMI_USE_EDID_REAL==EdidModes[i])?tr("Automatic" ) :
		                   tr("Unknown");
		if(EdidModes[i] == plugin->options.edid_mode)
			EdidMode = i;
	} // for
	LastEdidMode = EdidMode;

	DeringingLevel        = plugin->options.deringing_level;
	GaussianLevel         = plugin->options.gaussian_level;
	DegradeLevel          = plugin->options.degrade_level;
	PanScan               = plugin->options.pan_scan;
	OverrideFramePolarity = plugin->options.override_frame_polarity;
	HdmiDelay             = plugin->options.hdmi_delay;
	SpdifDelay            = plugin->options.spdif_delay;
	AnalogDelay           = plugin->options.analog_delay;
} // cICESetup::InitSettings

ismd_vidpproc_scaling_policy_t cICESetup::GetNextScaleMode(ismd_vidpproc_scaling_policy_t mode) {
	unsigned int ret = 0;
	for(unsigned int i=0; !ret && (i < (sizeof(ScaleModesText)/sizeof(ScaleModesText[0]))); i++) {
		if(ScaleModes[i] == mode)
			ret = i;
	} // for
	ret++;
	if(ret >= (sizeof(ScaleModesText)/sizeof(ScaleModesText[0])))
	    ret=0;
	return ScaleModes[ret];
} // cICESetup::GetNextScaleMode

struct StructImage
{
	int x;
	int y;
	char path[255];
	bool blend;
	int slot;
	int w;
	int h;
};

void cICESetup::ShowImage() {
	// Draw Image
	struct StructImage Image;

	Image.x = 209;
	Image.y = 284;
	Image.w = 0;
	Image.h = 0;
	Image.blend = true;
	Image.slot = 0;
	bool isClient = !HasFrontPanel();

	if (isClient) {
		Image.x = 310; Image.y = 200; // centre image
		snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/NetClient2", "NetClient2-HDMI.png");
	}
	else
		snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-left-plain.png");
	cPluginManager::CallAllServices("setThumb", (void *)&Image);

	if (isClient) return; // has only one image

	Image.x = 371;
	Image.h = 116;
	Image.w = 200;
	Image.slot = 1;
	// HDMI
	snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", "Reelbox3-right-HDMI.png");
	cPluginManager::CallAllServices("setThumb", (void *)&Image);
} // cICESetup::ShowImage

void cICESetup::InitVideo() {
	ICE_DBG(DBG_FN, "cICESetup::InitVideo");
	tICE_TVMode tv_mode = TVModes[TVMode];
	TVMode = -1;
	int defaultMode = -1;
	TVModesCount = 0;
	for(unsigned int i=0; i < (sizeof(SetupTVModes)/sizeof(SetupTVModes[0])); i++) {
		gdl_tvmode_t mode;
		if(((ICE_TVMODE_NATIVE==SetupTVModes[i]) && (GDL_HDMI_USE_EDID_REAL==EdidModes[EdidMode])) || (ICE_TVMODE_SOURCE==SetupTVModes[i]) || plugin->GetTVMode(SetupTVModes[i], mode)) {
			TVModes[TVModesCount] = SetupTVModes[i];
			TVModesText[TVModesCount] = tr(TVMODE_STR(TVModes[TVModesCount]));
			if(TVModes[TVModesCount] == tv_mode)
				TVMode = TVModesCount;
			if(TVModes[TVModesCount] == ICE_TVMODE_576P) defaultMode = TVModesCount;
			TVModesCount++;
		} // if
	} // for
	if((LastEdidMode != EdidMode) && (GDL_HDMI_USE_EDID_REAL==EdidModes[EdidMode])) TVMode = 0;
	if(TVMode < 0) TVMode = defaultMode;
	if(TVMode < 0) TVMode = 0;
	LastEdidMode = EdidMode;
} // cICESetup::InitVideo

void cICESetup::Setup() {
	ICE_DBG(DBG_FN, "cICESetup::Setup");
	int current = Current();
	Clear();
	SetCols(26);
	if(isVideoPage) {
		InitVideo();
		if (*title)
		    SetTitle(title);
		else
		    SetTitle(tr("Setup - Video"));
		SetHelp((ICE_SETUP_VIDEO_AUDIO==mode)?tr("Audio"):NULL,
		InInstallWizard?tr("Back"):NULL, InInstallWizard?tr("Skip"):NULL, /*isExpert ? tr("Normal"):tr("Experts"));*/ NULL);
		Add(new cOsdItem(tr("HDMI settings"), osUnknown, false));
		Add(new cOsdItem("", osUnknown, false));
		Add(new cMenuEditStraItem(tr("Output"), &TVMode, TVModesCount, TVModesText));
		Add(new cMenuEditStraItem(tr("Scale"), &ScaleMode, sizeof(ScaleModesText)/sizeof(ScaleModesText[0]), ScaleModesText));
		if (!InInstallWizard) { //(isExpert) {
//			Add(new cOsdItem("", osUnknown, false));
			Add(new cMenuEditStraItem(tr("Display detection"), &EdidMode, sizeof(EdidModesText)/sizeof(EdidModesText[0]), EdidModesText));
			Add(new cOsdItem("", osUnknown, false));
			Add(new cOsdItem(tr("Postprocessing"), osUnknown, false));
//			Add(new cMenuEditStraItem(tr("Deinterlacer"), &DeinterlaceMode, sizeof(DeinterlaceModesText)/sizeof(DeinterlaceModesText[0]), DeinterlaceModesText));
//			Add(new cMenuEditIntItem(tr("Deringing level"), &DeringingLevel, INVALID_LEVEL, MAX_DERINGNING_LEVEL, tr("Off")));
//			Add(new cMenuEditIntItem(tr("Gaussian level"), &GaussianLevel, INVALID_LEVEL, MAX_GAUSSIAN_LEVEL, tr("Off")));
			Add(new cMenuEditIntItem(tr("Degrade level"), &DegradeLevel, INVALID_LEVEL, MAX_DEGRADE_LEVEL, tr("Off")));
//			Add(new cMenuEditBoolItem(tr("Pan scan"), &PanScan, tr("Disable"), tr("Enable")));
//			Add(new cMenuEditBoolItem(tr("Override frame polarity"), &OverrideFramePolarity, tr("Disable"), tr("Enable")));
		} // if
	} else {  // Audio page
		if (*title)
		    SetTitle(title);
		else
		    SetTitle(tr("Setup - Audio"));
		SetHelp((ICE_SETUP_VIDEO_AUDIO==mode)?tr("Video"):NULL,
		InInstallWizard?tr("Back"):NULL, InInstallWizard?tr("Skip"):NULL, isExpert ? tr("Normal"):tr("Experts"));

		Add(new cOsdItem(tr("HDMI settings"), osUnknown, false));
		Add(new cOsdItem("", osUnknown, false));
		Add(new cMenuEditStraItem(tr("Output"), &HdmiAudioMode, sizeof(AudioModesText)/sizeof(AudioModesText[0]), AudioModesText));
#if 0		// Has to be handled in reelbox.c
		Add(new cMenuEditStraItem(tr("Channels"), &HdmiChannelMode, sizeof(AudioChannelsText)/sizeof(AudioChannelsText[0]), AudioChannelsText));
#endif
		if(isExpert) {
			Add(new cMenuEditIntItem(tr("Delay (ms)"), &HdmiDelay, 0, AUDIO_MAX_OUTPUT_STREAM_DELAY));
			//Add(new cMenuEditStraItem(tr("Edid mode"), &EdidMode, sizeof(EdidModesText)/sizeof(EdidModesText[0]), EdidModesText));
		} // if
		Add(new cOsdItem("", osUnknown, false));
		Add(new cOsdItem("", osUnknown, false));
		Add(new cOsdItem(tr("S/PDIF settings"), osUnknown, false));
		Add(new cOsdItem("", osUnknown, false));
		Add(new cMenuEditStraItem(tr("Output"), &SpdifAudioMode, sizeof(AudioModesText)/sizeof(AudioModesText[0]), AudioModesText));
		if(isExpert) {
			Add(new cMenuEditIntItem(tr("Delay (ms)"), &SpdifDelay, 0, AUDIO_MAX_OUTPUT_STREAM_DELAY));
			Add(new cOsdItem("", osUnknown, false));
			Add(new cOsdItem(tr("Analog settings"), osUnknown, false));
			Add(new cMenuEditIntItem(tr("Delay (ms)"), &AnalogDelay, 0, AUDIO_MAX_OUTPUT_STREAM_DELAY));
		} // if
	} // if
	SetCurrent(Get(current));

	if (InInstallWizard)
		ShowImage();

	Display();
} //  cICESetup::Setup

eOSState cICESetup::ProcessKey(eKeys Key) {
	if(kNone != Key) ICE_DBG(DBG_FN,"cICESetup::ProcessKey %d", Key);
	bool settingsSaved = false;
	if(kOk == Key) {
		tICE_Options old = plugin->options;
		Store();
		settingsSaved = true;
		if(old.tv_mode != plugin->options.tv_mode) {
			eKeys K = Skins.Message(mtWarning, tr("Keep these settings?"), 20);
			if(kOk != K) {
				plugin->options = old;
				plugin->Store();
				InitSettings();
				Setup();
				settingsSaved = false;
				Skins.Message(mtWarning, tr("Using old settings"), 10);
			}
		} // if
	} else if ((kRed == Key) && (ICE_SETUP_VIDEO_AUDIO==mode)) {
		isVideoPage = !isVideoPage;
		Setup();
		return osContinue;
	} else if (kBlue == Key) {
		isExpert = !isExpert;
		Setup();
		return osContinue;
	} // if

	// ignore kOk, because cMenuSetupPage::ProcessKey(kOk) calls Store()!
	if (kOk == Key && !settingsSaved)
		 Key = kNone;

	eOSState state = cMenuSetupPage::ProcessKey(Key);
	if(LastEdidMode != EdidMode) {
		plugin->options.edid_mode = EdidModes[EdidMode];
		plugin->updateEDID();
	} // if
	if(kNone != Key) Setup();

	if(InInstallWizard && (Key == kGreen || Key == kYellow ))
		state = osUnknown; // install wizard handles green and yellow keys
	else if (Key == kRed || Key == kGreen || Key == kYellow || Key == kBlue)
		state = osContinue;
	else if (Key == kNone)
		state = osUnknown;
	else if (InInstallWizard && Key == kOk)
		state = settingsSaved?osUser1:osContinue;
	return state;
} // cICESetup::ProcessKey

void cICESetup::Store() {
	ICE_DBG(DBG_FN, "cICESetup::Store");
	plugin->options.tv_mode                 = TVModes[TVMode];
	plugin->options.deinterlace_mode        = DeinterlaceModes[DeinterlaceMode];
	plugin->options.scale_mode              = ScaleModes[ScaleMode];
	plugin->options.deringing_level         = DeringingLevel;
	plugin->options.gaussian_level          = GaussianLevel;
	plugin->options.degrade_level           = DegradeLevel;
	plugin->options.pan_scan                = PanScan;
	plugin->options.override_frame_polarity = OverrideFramePolarity;
	plugin->options.hdmi_audio              = AudioModes[HdmiAudioMode];
	plugin->options.spdif_audio             = AudioModes[SpdifAudioMode];
	plugin->options.hdmi_channels           = AudioChannels[HdmiChannelMode];
	plugin->options.hdmi_delay              = HdmiDelay;
	plugin->options.spdif_delay             = SpdifDelay;
	plugin->options.analog_delay            = AnalogDelay;
	plugin->options.edid_mode               = EdidModes[EdidMode];
	plugin->Store();
} // cICESetup::Store

