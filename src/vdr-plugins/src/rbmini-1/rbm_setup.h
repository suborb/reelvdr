#ifndef RBM_SETUP_H
#define RBM_SETUP_H

class cPluginRBM; 

extern "C" {
typedef unsigned long  long u_int64;
typedef unsigned long       u_int32;
typedef long                int32;
#include <cnxt_rbm.h>
} // extern "C"

const u_int32 HDMIOutputs[]  = {
	RBM_OUTPUT_MODE_576P, RBM_OUTPUT_MODE_720P_50, RBM_OUTPUT_MODE_1080I_50, 
	RBM_OUTPUT_MODE_PAL_B_WEUR, RBM_OUTPUT_MODE_NTSC_M, RBM_OUTPUT_MODE_SECAM_L,
	RBM_OUTPUT_MODE_480P, RBM_OUTPUT_MODE_720P_60, RBM_OUTPUT_MODE_1080I_60,
	/*RBM_OUTPUT_MODE_1080P_24, RBM_OUTPUT_MODE_1080P_25, RBM_OUTPUT_MODE_1080P_30*/
	RBM_OUTPUT_MODE_SOURCE};
const u_int32 HDMIAspects[]  = {RBM_ASPECT_16_9, RBM_ASPECT_4_3};
const u_int32 HDMIARModes[]  = {RBM_ARMODE_LETTERBOX, RBM_ARMODE_PANSCAN, RBM_ARMODE_NONE};
const u_int32 HDMIModes[]    = {RBM_HDMIMODE_HDMI, RBM_HDMIMODE_PCM, RBM_HDMIMODE_DVI};
const u_int32 SCARTOutputs[] = {RBM_OUTPUT_MODE_PAL_B_WEUR, RBM_OUTPUT_MODE_NTSC_M, RBM_OUTPUT_MODE_SECAM_L};
const u_int32 SCARTAspects[] = {RBM_ASPECT_16_9, RBM_ASPECT_4_3, RBM_ASPECT_NONE, RBM_ASPECT_SOURCE};
const u_int32 SCARTARModes[] = {RBM_ARMODE_LETTERBOX, RBM_ARMODE_PANSCAN, RBM_ARMODE_NONE};
const u_int32 SCARTModes[]   = {RBM_SCARTMODE_ALL, RBM_SCARTMODE_CVBS, /*RBM_SCARTMODE_CVBS_DLY,*/ RBM_SCARTMODE_RGB, RBM_SCARTMODE_YC, RBM_SCARTMODE_YPRPB};
const u_int32 MPEG2Modes[]   = {RBM_MPEG2_MODE_NONE, RBM_MPEG2_MODE_DB, RBM_MPEG2_MODE_DB_DR};

#define RBM_SETUP_VIDEO_AUDIO 0
#define RBM_SETUP_VIDEO_ONLY  1
#define RBM_SETUP_AUDIO_ONLY  2

class cRBMSetup : public cMenuSetupPage {
protected:
	cPluginRBM *plugin;
	bool isBothMode;
	bool isVideoMode;
	bool isExpertMode;
	int HDMIOutput;
	int HDMIAspect;
	int HDMIARMode;
	int HDMIMode;
	const char *HDMIOutputText[sizeof(HDMIOutputs)/sizeof(HDMIOutputs[0])];
	const char *HDMIAspectText[sizeof(HDMIAspects)/sizeof(HDMIAspects[0])];
	const char *HDMIARModeText[sizeof(HDMIARModes)/sizeof(HDMIARModes[0])];
	const char *HDMIModeText  [sizeof(HDMIModes)  /sizeof(HDMIModes[0])];
	int SCARTOutput;
	int SCARTAspect;
	int SCARTARMode;
	int SCARTMode;
	const char *SCARTOutputText[sizeof(SCARTOutputs)/sizeof(SCARTOutputs[0])];
	const char *SCARTAspectText[sizeof(SCARTAspects)/sizeof(SCARTAspects[0])];
	const char *SCARTARModeText[sizeof(SCARTARModes)/sizeof(SCARTARModes[0])];
	const char *SCARTModeText  [sizeof(SCARTModes)  /sizeof(SCARTModes[0])];
	int MPEG2Mode;
	const char *MPEG2ModeText  [sizeof(MPEG2Modes)  /sizeof(MPEG2Modes[0])];
	int MPADelay;
	int AC3Delay;
	void Setup();
public:
	cRBMSetup(cPluginRBM *Plugin, int Mode=RBM_SETUP_VIDEO_AUDIO);
	virtual ~cRBMSetup();
	virtual eOSState ProcessKey(eKeys Key);
	virtual void Store();
	bool Parse(const char *Name, const char *Value);
}; // cRBMSetup

#endif

