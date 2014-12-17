#ifndef REELMENU_H
#define REELMENU_H

#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/config.h>

#define HD_DMODE_DVI   0
#define HD_DMODE_HDMI  1

#define HD_AMODE_AUTO  0
#define HD_AMODE_YUV   1
#define HD_AMODE_RGB   2
#define HD_AMODE_YC    3
#define HD_AMODE_OFF   4

#define HD_APORT_AUTO  0
#define HD_APORT_SCART 1
#define HD_APORT_MDIN  2
#define HD_APORT_SCART_V1 3
#define HD_APORT_SCART_V0 4

#define HD_VM_RESOLUTION_1080  0
#define HD_VM_RESOLUTION_720   1
#define HD_VM_RESOLUTION_576   2
#define HD_VM_RESOLUTION_480   3

#define HD_VM_NORM_50HZ   0
#define HD_VM_NORM_60HZ   1

#define HD_OUTPUT_HDMI    0
#define HD_OUTPUT_DVI     1
#define HD_OUTPUT_SCART   2
#define HD_OUTPUT_MINIDIN 3
#define HD_OUTPUT_BOTH    4

namespace Reel
{

    class ReelBoxSetup
    {
        public:
            int brightness;
            int contrast;
            int colour;
            int sharpness;
            int gamma;
            int flicker;

            // Videomodes BSP
            int main_mode;
            int sub_mode;
            int display_type;
            int aspect;
            int framerate;
            int norm;
            int resolution;
            int scart_mode;
            int deint;
            int usehdext;

            // Videomodes eHD
            int HDdmode;
            int HDamode;
            int HDaport;
            int HDdisplay_type;
            int HDaspect;
            int HDresolution;
            int HDintProg;
            int HDnorm;
            int HDfb_dbd;
            int HDfb_force;
            int HDdeint;
	    int HDauto_format;
	    int HDswitch_voltage;
	    int HDoutput; // high level wrapper for HD*mode and HDaport
            int HDoverscan;

            // Audiomodes HDMI
            int audio_over_hdmi;
            int ac3;

            // generic audio
            int delay_ac3;
            int delay_stereo;

	    // Avantgarde
	    int audiomix;
            int audio_over_hd;
    };


    class cMenuVideoMode: public cMenuSetupPage
    {
        private:
            cSetup data;
            cPlugin *plugin;
            ReelBoxSetup rbSetup_;
            bool useHdAtStart; // ??
            bool liteHasHdext_;
            int oldUseHDext_;
            int oldAudioOverHdmi_;
            int oldAudioOverHd_;
	    enum menu_modes {video_mode, audio_mode}; // toggles between audio- and video-settings-mode
	    enum menu_modes menu_mode;
	    bool expert_mode;
	    bool expert_mode_changed;
	    int old_output;
	    bool output_changed;
	    int last_normal_output;

            const char *showMainMode[3];
            const char *showSubMode[6];
	    const char *showHDDMode[2];
	    const char *showHDAMode[5];
	    const char *showHDAPort[5];
            const char *showDisplayType[2];
            const char *showAspect[3];
            const char *showResolution[9];
            const char *showOsd[2];
            const char *showIntProg[2];
            const char *showAutoFormat[2];
            const char *deint[3];
	    const char *switchVoltage[3];
            const char *output[5];

            void Setup(void);
            void PostStore();
            bool HaveDevice(const char *deviceID);
        public:
            cMenuVideoMode(cPlugin *Plugin, bool isVideoMode=true);
            virtual ~cMenuVideoMode();
            virtual eOSState ProcessKey(eKeys Key);
            virtual void Store();
    };
    extern ReelBoxSetup RBSetup;
}


#endif
