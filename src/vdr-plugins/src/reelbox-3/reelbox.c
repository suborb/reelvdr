/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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

// reelbox.c

#include "reelbox.h"
#include "setupmenu.h"

#include "BspOsdProvider.h"
#include "HdOsdProvider.h"
#include "HdCommChannel.h"
#include "ReelBoxDevice.h"
#include "fs453settings.h"
#include "ReelBoxMenu.h"
#include "config.h"
#include "VdrXineMpIf.h"
#include "fs453settings.h"

#include <vdr/config.h>
#if APIVERSNUM >= 10716
  #include <vdr/shutdown.h>
#endif

#ifdef REELSKIN
#include "ReelSkin.h"

Reel::ReelSkin *skin;
#endif

typedef struct imgSize {
        int slot;
	int width;
	int height;
} imgSize_t;

namespace Reel
{

    extern int lastThumbWidth[2];
    extern int lastThumbHeight[2];
    int useFb = 1;
    const char *fbdev = FB_DEFAULT_DEVICE;

    namespace
    {
        PCSTR VersionString     =  VERSION;
        PCSTR DescriptionString = "ReelBox plugin";
    }

    Plugin::Plugin() NO_THROW
    {
#ifdef RBLITE
        RBSetup.usehdext = 0;
#else
        RBSetup.usehdext = 1;
        RBSetup.audiomix = 1;
#endif
        if (!RBSetup.usehdext) {
           RBSetup.brightness = fs453_defaultval_tab0_BSP;
           RBSetup.contrast   = fs453_defaultval_tab1_BSP;
           RBSetup.colour     = fs453_defaultval_tab2_BSP;
           RBSetup.sharpness  = fs453_defaultval_tab3_BSP;
           RBSetup.gamma      = fs453_defaultval_tab4_BSP;
           RBSetup.flicker    = fs453_defaultval_tab5_BSP;
        } else {
           RBSetup.brightness = fs453_defaultval_tab0_HD;
           RBSetup.contrast   = fs453_defaultval_tab1_HD;
           RBSetup.colour     = fs453_defaultval_tab2_HD;
           RBSetup.sharpness  = fs453_defaultval_tab3_HD;
           RBSetup.gamma      = fs453_defaultval_tab4_HD;
           RBSetup.flicker    = fs453_defaultval_tab5_HD;
        }

        RBSetup.main_mode  = BSP_VMM_SD;
        RBSetup.sub_mode   = BSP_VSM_YUV;
        RBSetup.display_type = BSP_VM_DISPLAY_43;
        RBSetup.aspect     = BSP_VM_ASPECT_WSS;
        RBSetup.framerate  = BSP_VM_FRATE_AUTO1;
        RBSetup.norm       = BSP_VM_NORM_PAL50_60;
        RBSetup.resolution = BSP_VM_RESOLUTION_800x600;
        RBSetup.scart_mode = 0;
        RBSetup.deint      = 0;
        RBSetup.HDdmode      = HD_DMODE_HDMI;
        RBSetup.HDamode      = HD_AMODE_AUTO;
        RBSetup.HDaport      = HD_APORT_AUTO;
        RBSetup.HDresolution    = HD_VM_RESOLUTION_1080; // 1080
        RBSetup.HDnorm          = HD_VM_NORM_50HZ; // 50Hz
        RBSetup.HDintProg       = 1; // interlaced
        RBSetup.HDdisplay_type  = 1; // 16:9
        RBSetup.HDdeint  = HD_VM_DEINT_AUTO;
	RBSetup.HDauto_format = HD_VM_AUTO_OFF;
	RBSetup.HDoverscan      = 1;
        RBSetup.audio_over_hdmi = 1;
        RBSetup.audio_over_hd   = 1;
        RBSetup.ac3             = 0;
        RBSetup.delay_ac3      = 0;
        RBSetup.delay_stereo   = 0;
        RBSetup.HDswitch_voltage = 0;
   }

    Plugin::~Plugin() NO_THROW
    {
        //printf (" \033[0;44m %s \033[0m \n", __PRETTY_FUNCTION__);
        if (RBSetup.usehdext)
        {
            HdCommChannel::Exit();
        }
        else
        {
            Bsp::BspCommChannel::Destroy();
        }
        //printf (" \033[0;44m %s END \033[0m \n", __PRETTY_FUNCTION__);
    }

    PCSTR Plugin::Version() NO_THROW
    {
        return VersionString;
    }

    PCSTR Plugin::Description() NO_THROW
    {
        return DescriptionString;
    }

    PCSTR Plugin::MainMenuEntry() NO_THROW
    {
	//return NULL;
        return tr("Picture Settings");
    }

    cOsdObject* Plugin::MainMenuAction(void) NO_THROW
    {
        return new cFs453Settings(this);
    }

    cMenuSetupPage* Plugin::SetupMenu(void)
    {
        //return new cMenuVideoMode(this);
        return new cMenuReelBoxSetup(this);
    }

    bool Plugin::Initialize() NO_THROW
    {
        // Open communications to the BSP

        if (RBSetup.usehdext)
        {
            HdCommChannel::Init();
            //Reel::HdCommChannel::SetVideomode();
	    Reel::HdCommChannel::SetPicture(&RBSetup);
	    Reel::HdCommChannel::SetHWControl(&RBSetup);
	    Reel::HdCommChannel::hda->plane[2].enable=1; // avoid init race
	    Reel::HdCommChannel::hda->plane[2].alpha=255;

#if 0 //REELVDR
        Reel::HdCommChannel::hda->hdp_enable = 0;
        Reel::HdCommChannel::hda->hd_shutdown = 1;
        /* disable video output and audio output
         * so that no OSD can be shown
         * later enabled with "ReInit" service, ie. SetVideomode() */
        Reel::HdCommChannel::hda->video_mode.outputd=HD_VM_OUTD_OFF;
        Reel::HdCommChannel::hda->video_mode.outputa=HD_VM_OUTD_OFF;
#else
	//Reel::HdCommChannel::hda->hdp_enable = 1;
        //Reel::HdCommChannel::hda->hd_shutdown = 0;
#endif

#ifdef REELSKIN
            Reel::ResetImagePaths();
#endif
        }
        else
        {
            Bsp::BspCommChannel::Create();
            Bsp::BspCommChannel::SetVideomode();
            Bsp::BspCommChannel::SetPicture();
        }

        // Create the device and the BSP-OSD provider
        Reel::ReelBoxDevice::Create();

        if (RBSetup.usehdext)
        {
            //HdOsdProvider::Create();
        }
        else
        {
            BspOsdProvider::Create();
        }

#ifdef REELSKIN
        skin = new ReelSkin;
#endif

        return true;
    }

    const char *Plugin::CommandLineHelp(void)
    {
        // return a string that describes all known command line options.
        return "  --nofb           Do not use the framebuffer-based OSD-implementation.\n"
               "  --fbdev <dev>   Use <dev> as the framebuffer device (/dev/fb0)\n";
    }

    bool Plugin::ProcessArgs(int argc, char *argv[]) NO_THROW
    {
        for (int n = 0; n < argc; ++n)
        {
            if(strcmp(argv[n], "--nofb")==0) {
                useFb = 0;
            }
            else if(strcmp(argv[n], "--fbdev")==0) {
                fbdev = strdup(argv[n+1]);
                n++;
            }
        }
        return true;
    }

    bool Plugin::Start() NO_THROW
    {
        return true;
    }


    bool Plugin::Service(const char *Id, void *Data) NO_THROW
    {
        struct service_menu
        {
            cOsdMenu* menu;
        };

    	if (strcmp(Id,"ReelSetAudioTrack") == 0) // for skin !!!
	{
		if (!Data) return false;
		int *index = (int*) Data;

		Reel::ReelBoxDevice::Instance()->SetAudioTrack(*index );
		return true;
	}
        // Xine output interface.
        else if (strcmp(Id, "GetVdrXineMpIf") == 0)
        {
            ::VdrXineMpIf const *GetVdrXineMpIf();

            *static_cast<VdrXineMpIf const **>(Data) = GetVdrXineMpIf();

            //::printf("VdrXineMpIf = %u\n", (unsigned int)*static_cast<VdrXineMpIf const **>(Data));
            return true;
        }

#ifdef REELSKIN
        else if (strcmp(Id, "ResetImagePaths") == 0)
        {
            //if (strcmp(Skins.Current()->Name(), "Reel") == 0) // not needed?!
            Reel::ResetImagePaths();
            return true;
        }
#endif
        else if (strcmp(Id, "ReInit") == 0)
        {
            if (!RBSetup.usehdext)
            {
                Reel::Bsp::BspCommChannel::SetVideomode();
            }
	    else
	    {
		Reel::HdCommChannel::SetVideomode();
	    }
		return true;
        }
        else if (strcmp(Id, "ToggleVideoOutput") == 0)
        {
            if (RBSetup.usehdext) {
                Reel::HdCommChannel::hda->hdp_enable = *(int*)Data;
                Reel::HdCommChannel::hda->hd_shutdown = !(*(int*)Data);
            }
            return true;
        }
#if APIVERSNUM >= 10716 && REELVDR
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

                            Reel::HdCommChannel::hda->hd_shutdown = 0;
                            Reel::HdCommChannel::SetVideomode();
                            Reel::HdCommChannel::hda->hdp_enable = 1;

                            //Reel::HdCommChannel::SetVideomode();
                        }
                        break;

                    case standby:
                        {
                            Reel::HdCommChannel::hda->hdp_enable = 0;
                            Reel::HdCommChannel::hda->hd_shutdown = 1;
                            SystemExec("quickshutdown.sh start"); // sets Front panel LEDs
                            break;
                        }
                    case poweroff:
                        {
                            Reel::HdCommChannel::hda->hdp_enable = 0;
                            Reel::HdCommChannel::hda->hd_shutdown = 1;
                            //do not (!) call quickshutdown.sh as it will cause wrong LED setting on poweroff
                        }
                        break;

                    default:
                        return false;
                        break;
                } // switch

           } //if
           else
               esyslog("ERROR reelbox pl: Service(\"ActiveModeChange\") Data == NULL");

           return true;
        }
#endif
        else if (strcmp(Id, "ToggleAudioPicPlayer") == 0)
        {
            if (ReelBoxDevice::Instance())
                ReelBoxDevice::Instance()->SetAudioBackgroundPics((bool)Data);
            return true;
        }
	else if (strcmp(Id, "GetUnbufferedPicSize") == 0)
	{
		if (RBSetup.usehdext && Data) {
			struct imgSize *thumbSize = (struct imgSize*)Data;
			thumbSize->width = lastThumbWidth[thumbSize->slot];
			thumbSize->height = lastThumbHeight[thumbSize->slot];
		}
                return true;
	}
        else if (strcmp(Id, "Get AudioSetting Menu") == 0)
        {
            if (Data)
                ((service_menu*)Data)->menu = new cMenuVideoMode(this, false); // video mode false, therefore audio mode

            return true;
        }
        else if (strcmp(Id, "Get VideoSetting Menu") == 0)
        {
            if (Data)
                ((service_menu*)Data)->menu = new cMenuVideoMode(this, true); // video mode true

            return true;
        }
        return false;
    }


    bool Plugin::SetupParse(const char *Name, const char *Value) NO_THROW
    {
        bool res = true;

        if      (!strcasecmp(Name, "Brightness")) RBSetup.brightness = atoi(Value);
        else if (!strcasecmp(Name, "Contrast"))   RBSetup.contrast   = atoi(Value);
        else if (!strcasecmp(Name, "Colour"))     RBSetup.colour     = atoi(Value);
        else if (!strcasecmp(Name, "Sharpness"))  RBSetup.sharpness  = atoi(Value);
        else if (!strcasecmp(Name, "Gamma"))      RBSetup.gamma      = atoi(Value);
        else if (!strcasecmp(Name, "Flicker"))    RBSetup.flicker    = atoi(Value);
        else if (!strcasecmp(Name, "VMM"))        RBSetup.main_mode  = atoi(Value);
        else if (!strcasecmp(Name, "VSM"))        RBSetup.sub_mode   = atoi(Value);
        else if (!strcasecmp(Name, "DisplayType"))RBSetup.display_type = atoi(Value);
        else if (!strcasecmp(Name, "Norm"))       RBSetup.norm       = atoi(Value);
        else if (!strcasecmp(Name, "Aspect"))     RBSetup.aspect     = atoi(Value);
        else if (!strcasecmp(Name, "Framerate"))  RBSetup.framerate  = atoi(Value);
        else if (!strcasecmp(Name, "Resolution")) RBSetup.resolution = atoi(Value);
        else if (!strcasecmp(Name, "Scartmode"))  RBSetup.scart_mode = atoi(Value);
        else if (!strcasecmp(Name, "Deint"))      RBSetup.deint = atoi(Value);
        else if (!strcasecmp(Name, "AudioOverHDMI")) RBSetup.audio_over_hdmi = atoi(Value);
        else if (!strcasecmp(Name, "AudioOverHD")) RBSetup.audio_over_hd = atoi(Value);
        else if (!strcasecmp(Name, "Ac3"))        RBSetup.ac3     = atoi(Value);
        else if (!strcasecmp(Name, "UseHdExt"))   RBSetup.usehdext   = atoi(Value);

        else if (!strcasecmp(Name, "HDDM"))         RBSetup.HDdmode   = atoi(Value);
        else if (!strcasecmp(Name, "HDAM"))         RBSetup.HDamode   = atoi(Value);
        else if (!strcasecmp(Name, "HDAPort"))      RBSetup.HDaport   = atoi(Value);
        else if (!strcasecmp(Name, "HDDisplayType"))RBSetup.HDdisplay_type = atoi(Value);
        else if (!strcasecmp(Name, "HDAspect"))     RBSetup.HDaspect     = atoi(Value);
        else if (!strcasecmp(Name, "HDResolution")) RBSetup.HDresolution = atoi(Value);
        else if (!strcasecmp(Name, "HDintProg"))    RBSetup.HDintProg    = atoi(Value);
        else if (!strcasecmp(Name, "HDnorm"))       RBSetup.HDnorm       = atoi(Value);
        else if (!strcasecmp(Name, "HDfb"))         RBSetup.HDfb_dbd     = atoi(Value);
        else if (!strcasecmp(Name, "HDdeint"))      RBSetup.HDdeint     = atoi(Value);
        else if (!strcasecmp(Name, "HDauto_format"))RBSetup.HDauto_format = atoi(Value);
        else if (!strcasecmp(Name, "HDswitch_voltage")) RBSetup.HDswitch_voltage = atoi(Value);
        else if (!strcasecmp(Name, "HDoutput"))     RBSetup.HDoutput     = atoi(Value);
        else if (!strcasecmp(Name, "HDoverscan"))   RBSetup.HDoverscan   = atoi(Value);

        else if (!strcasecmp(Name, "DelayAc3"))     RBSetup.delay_ac3    = atoi(Value);
        else if (!strcasecmp(Name, "DelayStereo"))  RBSetup.delay_stereo = atoi(Value);

        else if (!strcasecmp(Name, "AudioMix"))  RBSetup.audiomix = atoi(Value);
        else
           res = false;

        return res;
    }

    cString Plugin::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) NO_THROW
    {
        if (strcasecmp(Command, "REINIT") == 0)
        {
            if (RBSetup.usehdext == 1)
            {
                Reel::HdCommChannel::SetVideomode();
            }
            else
            {
                Reel::Bsp::BspCommChannel::SetVideomode();
            }

            return cString("video settings updated");
        }
        return NULL;
    }
}

VDRPLUGINCREATOR(Reel::Plugin); // Don't touch this!
