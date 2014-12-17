/***************************************************************************
 * ReelBoxMenu.c  -- Videomode menu for Reelbox
 ***************************************************************************/

#include <fstream>
#include <sstream>

#include <vdr/interface.h>

#include "reelbox.h"
#include "ReelBoxDevice.h"
#include "ReelBoxMenu.h"
#include "BspCommChannel.h"
#include "HdCommChannel.h"
#include "hdshmlib.h"
#include "hdshm_user_structs.h"
#include "fs453settings.h"

using std::string;
using std::ifstream;


namespace Reel
{

ReelBoxSetup RBSetup;

cMenuVideoMode::cMenuVideoMode(cPlugin *Plugin, bool isVideoMode)
:liteHasHdext_(false)
{
    SetTitle(tr("Setup - Video"));
#if APIVERSNUM < 10700
    SetCols(20);
#else
    SetCols(27);
#endif

    if ( HaveDevice("12d51000")  //  BSP
      && HaveDevice("19058100")) //  HD Extension
    {
       liteHasHdext_ = true;
    }

    rbSetup_ = RBSetup;
    data = ::Setup;
    plugin = Plugin;
    oldUseHDext_ = RBSetup.usehdext;
    oldAudioOverHdmi_ = RBSetup.audio_over_hdmi;
    oldAudioOverHd_ = RBSetup.audio_over_hd;
    if (RBSetup.HDaport != HD_APORT_AUTO || RBSetup.HDamode != HD_AMODE_AUTO)
	expert_mode = true;
    else
	expert_mode = false;
    old_output = RBSetup.HDoutput;
    output_changed = false;
    last_normal_output = -1;
    expert_mode_changed = false;


    if (RBSetup.HDaport == HD_APORT_SCART
	|| RBSetup.HDaport == HD_APORT_SCART_V0
	|| RBSetup.HDaport == HD_APORT_SCART_V1)
    {
	rbSetup_.HDaport = HD_APORT_SCART;
	switch (RBSetup.HDaport)
	{
	    case HD_APORT_SCART:    rbSetup_.HDswitch_voltage = 0; break;
	    case HD_APORT_SCART_V1: rbSetup_.HDswitch_voltage = 1; break;
	    case HD_APORT_SCART_V0: rbSetup_.HDswitch_voltage = 2; break;
	    default: break;
	}
    }

    showDisplayType[0] = "4:3";
    showDisplayType[1] = "16:9";

    showMainMode[0] = tr("SDTV only");
    showMainMode[1] = "SDTV+HDTV";
    showMainMode[2] = "PC/VGA";

    showSubMode[0] = "YUV";
    showSubMode[1] = "RGB";
    showSubMode[2] = "CVBS";
    showSubMode[3] = "YC";

    showHDDMode[0] = "DVI";
    showHDDMode[1] = "HDMI";

    showHDAMode[0] = tr("Auto");
    showHDAMode[1] = "YUV";
    showHDAMode[2] = "RGB";
    showHDAMode[3] = "YC";
    showHDAMode[4] = tr("Off");

    showHDAPort[0] = tr("Auto");
    showHDAPort[1] = "SCART";
    showHDAPort[2] = "Mini-DIN";
    //showHDAPort[3] = "SCART (12V)";
    //showHDAPort[4] = "SCART (0V)";

    showIntProg[0] = tr("Progressive");
    showIntProg[1] = tr("Interlaced");

    showAutoFormat[0] = tr("Maximum");
    showAutoFormat[1] = tr("Media resolution");

    switchVoltage[0] = tr("Normal");
    switchVoltage[1] = tr("Always on");
    switchVoltage[2] = tr("Always off");

    deint[0] = tr("Off");
    deint[1] = tr("Auto");
    deint[2] = tr("On");

    output[0] = tr("HDMI");
    output[1] = tr("DVI");
    output[2] = tr("SCART");
    output[3] = tr("Mini-DIN");
    output[4] = tr("Analog+digital");

    menu_mode= isVideoMode?video_mode:audio_mode;


    if (rbSetup_.main_mode!=BSP_VMM_PC)
    {
       useHdAtStart=true;
    }
    else
    {
       useHdAtStart=false;
    }
    Setup();
}

cMenuVideoMode::~cMenuVideoMode()
{
}

void cMenuVideoMode::Setup()
{
    //printf("---------cMenuVideoMode::Setup()-------------\n");
    int current = Current();

    Clear();

    if (menu_mode == video_mode) {
		if (rbSetup_.usehdext) {
		   if (expert_mode)
        		SetHelp(NULL/*tr("Audio")*/, NULL, NULL, tr("Normal"));
		   else
			SetHelp(NULL/*tr("Audio")*/, NULL, NULL, data.ExpertOptions ? tr("Experts") : NULL);
		} else {
        		SetHelp(NULL/*tr("Audio")*/, NULL, NULL, NULL);
                }
    } else
			SetHelp(NULL/*tr("Video")*/, NULL, NULL, NULL);

    if (menu_mode == video_mode)
    {

        if (rbSetup_.main_mode==BSP_VMM_PC && useHdAtStart)
        {
            rbSetup_.sub_mode=BSP_VSM_RGB;

            if (rbSetup_.resolution==0)
            {
                rbSetup_.resolution=1; // 800x600
            }

            useHdAtStart=false;
         }

         Add(new cOsdItem(tr("Video Settings"), osUnknown, false));
         Add(new cOsdItem(" ", osUnknown, false), false, NULL);

         if (liteHasHdext_)
         {
             Add (new cMenuEditBoolItem(tr("Use extension HD"), &rbSetup_.usehdext));
         }

         if (rbSetup_.usehdext)
         {
	    //int numberOutputs = expert_mode ? 5 : 4;
	    int numberOutputs = 4;

	    /* TB: check it output has been changed */
	    if ( rbSetup_.HDoutput != old_output ) {
		if(!expert_mode_changed)
		   output_changed = true;
		else
		   expert_mode_changed = false;
		/* remember the last output port in "normal mode" */
		if(expert_mode && rbSetup_.HDoutput == HD_OUTPUT_BOTH)
			last_normal_output = old_output;
		/* if switching back from expert mode */
		if(!expert_mode && old_output == HD_OUTPUT_BOTH && rbSetup_.HDoutput != HD_OUTPUT_BOTH && last_normal_output != -1)
			rbSetup_.HDoutput = last_normal_output;
		old_output = rbSetup_.HDoutput;
	    } else
		output_changed = false;

            Add(new cMenuEditStraItem(tr("Output"), &rbSetup_.HDoutput, numberOutputs, output));
	    /*
	    if (rbSetup_.HDoutput == HD_OUTPUT_BOTH) {
	        if (output_changed)
			rbSetup_.HDresolution = HD_VM_RESOLUTION_576;

		Add(new cMenuEditStraItem(tr("Digital Output"), &rbSetup_.HDdmode, 2, showHDDMode));
		Add(new cMenuEditStraItem(tr("Analog Output"), &rbSetup_.HDamode, 5, showHDAMode));
		if (rbSetup_.HDamode!=HD_AMODE_OFF) {
#ifdef RBLITE
			rbSetup_.HDaport = HD_APORT_MDIN;
#else
			Add(new cMenuEditStraItem(tr("Analog Port"), &rbSetup_.HDaport, 3, showHDAPort));
			if (expert_mode && (rbSetup_.HDaport == HD_APORT_SCART || rbSetup_.HDaport == HD_APORT_SCART_V1 || rbSetup_.HDaport == HD_APORT_SCART_V0))
				Add(new cMenuEditStraItem(tr("Switching Voltage"), &rbSetup_.HDswitch_voltage, 3, switchVoltage));
#endif
		}
            } else { */
		switch(rbSetup_.HDoutput) {
			case HD_OUTPUT_HDMI:
			{
				rbSetup_.HDdmode = HD_DMODE_HDMI;
			        if (output_changed) {
					rbSetup_.HDamode = HD_AMODE_AUTO;
					rbSetup_.HDresolution = HD_VM_RESOLUTION_1080;
				}
				break;
			}
			case HD_OUTPUT_DVI:
			{
				rbSetup_.HDdmode = HD_DMODE_DVI;
			        if (output_changed)  {
					rbSetup_.HDamode = HD_AMODE_AUTO;
					rbSetup_.HDresolution = HD_VM_RESOLUTION_1080;
				}
				break;
			}
			case HD_OUTPUT_SCART:
			{
			        if (output_changed ) {
					rbSetup_.HDresolution = HD_VM_RESOLUTION_576;
					rbSetup_.HDamode = HD_AMODE_RGB;
					rbSetup_.HDaport = HD_APORT_SCART;
				}
				break;
			}
			case HD_OUTPUT_MINIDIN:
			{
			        if (output_changed) {
					rbSetup_.HDresolution = HD_VM_RESOLUTION_1080;
					rbSetup_.HDamode = HD_AMODE_YUV;
					rbSetup_.HDaport = HD_APORT_MDIN;
                                }
		                //Add(new cMenuEditStraItem(tr("Analog Output"), &rbSetup_.HDamode, 4, showHDAMode));
				break;
			}
			default: break;
		}
	    //}

	//FIX THIS if (rbSetup_.HDaport != HD_APORT_SCART && rbSetup_.HDaport != HD_APORT_SCART_V1 && rbSetup_.HDaport != HD_APORT_SCART_V0)
	//FIX THIS	rbSetup_.HDamode = HD_APORT_SCART;
	    if (expert_mode )
	    {
		if (rbSetup_.HDoutput == HD_OUTPUT_HDMI || rbSetup_.HDoutput == HD_OUTPUT_DVI)
		{
			Add(new cMenuEditStraItem(tr("Analog Output"), &rbSetup_.HDamode, 5, showHDAMode));
			if (rbSetup_.HDamode != HD_AMODE_OFF)
#ifdef RBLITE
				rbSetup_.HDaport = HD_APORT_MDIN;
#else
				Add(new cMenuEditStraItem(tr("Analog Port"), &rbSetup_.HDaport, 3, showHDAPort));
#endif
		}
		else
		{
			Add(new cMenuEditStraItem(tr("Secondary Output"), &rbSetup_.HDdmode, 2, showHDDMode));
			Add(new cMenuEditStraItem(tr("Analog Output"), &rbSetup_.HDamode, 4, showHDAMode));
		}
#ifndef RBLITE
		if (rbSetup_.HDamode != HD_AMODE_OFF)
		{
			if (rbSetup_.HDaport == HD_APORT_SCART || rbSetup_.HDaport == HD_APORT_SCART_V1 || rbSetup_.HDaport == HD_APORT_SCART_V0)
				Add(new cMenuEditStraItem(tr("Switching Voltage"), &rbSetup_.HDswitch_voltage, 3, switchVoltage));
		}
#endif
	    }

            Add(new cMenuEditStraItem(tr("Display Type"), &rbSetup_.HDdisplay_type, 2, showDisplayType));

            showAspect[0] = tr("Fill to Screen");
            showAspect[1] = tr("Fill to Aspect");
            showAspect[2] = tr("Crop to Fill");
            Add(new cMenuEditStraItem(tr("Aspect Ratio"), &rbSetup_.HDaspect, 3, showAspect));

            showResolution[0] = "1080";
            showResolution[1] = "720";
            showResolution[2] = "576";
            showResolution[3] = "480";

            if (rbSetup_.HDaport != HD_APORT_SCART)
                Add(new cMenuEditStraItem(tr("Resolution"), &rbSetup_.HDresolution, 4, showResolution));

            if (rbSetup_.HDresolution != HD_VM_RESOLUTION_576 && rbSetup_.HDresolution != HD_VM_RESOLUTION_480 && expert_mode)
            {
		static const char * showNorm[2];
		showNorm[0] = "50 Hz";
		showNorm[1] = "60 Hz";
		Add(new cMenuEditStraItem(tr("Refresh Rate"), &rbSetup_.HDnorm, 2, showNorm));
    	    }

            if (rbSetup_.HDresolution != HD_VM_RESOLUTION_1080)  // not 1080
            {
		if (rbSetup_.HDresolution != HD_VM_RESOLUTION_720)
			Add(new cMenuEditStraItem(tr("Progressive/Interlaced"), &rbSetup_.HDintProg, 2, showIntProg));
            }
            else
            {
                rbSetup_.HDintProg = 1; // no 1080p yet
            }

            if (!((rbSetup_.HDresolution == HD_VM_RESOLUTION_576 || rbSetup_.HDresolution==HD_VM_RESOLUTION_480)
                 && rbSetup_.HDintProg))
            {
                if (expert_mode)
                   Add(new cMenuEditStraItem(tr("Deinterlacer for SDTV"), &rbSetup_.HDdeint, 2, deint)); // use only off/Auto
            }

            showOsd[0] = tr("Fill to Screen");
            showOsd[1] = tr("Dot by Dot");

            if (rbSetup_.HDresolution != HD_VM_RESOLUTION_576 && rbSetup_.HDresolution != HD_VM_RESOLUTION_480)
            {
                if (expert_mode)
                {
                    Add(new cMenuEditStraItem(tr("Resolution change"), &rbSetup_.HDauto_format, 2, showAutoFormat));
                    Add (new cMenuEditStraItem(tr("OSD scaling"), &rbSetup_.HDfb_dbd, 2, showOsd));
                }
            }
#if 0
            if (expert_mode)
		Add(new cMenuEditBoolItem(tr("Overscan"), &rbSetup_.HDoverscan, tr("On"), tr("Off")));
#endif
	}
        else  //BSP mode
        {
            Add(new cMenuEditStraItem(tr("Mode"), &rbSetup_.main_mode, 3, showMainMode));

            if (rbSetup_.main_mode==BSP_VMM_SD)
            {
                rbSetup_.deint=0;
                Add(new cMenuEditStraItem(tr("Output mode"), &rbSetup_.sub_mode, 4, showSubMode));
            }
            else //if (rbSetup_.main_mode!=BSP_VMM_SD)
            {
                if (rbSetup_.sub_mode >= BSP_VSM_CVBS)
                    rbSetup_.sub_mode=BSP_VSM_YUV;
                Add(new cMenuEditStraItem(tr("Output mode"), &rbSetup_.sub_mode, 2, showSubMode));
            }
            Add(new cMenuEditStraItem(tr("Display Type"), &rbSetup_.display_type, 2, showDisplayType));

            if (rbSetup_.main_mode==BSP_VMM_SD)
                showAspect[0] = tr("Anamorphic+WSS");
            else
                showAspect[0] = tr("Anamorphic");

            showAspect[1] = tr("Auto");
            showAspect[2] = tr("Manual");
            Add(new cMenuEditStraItem(tr("Aspect Ratio Ctrl."), &rbSetup_.aspect, 3, showAspect));

            if (rbSetup_.main_mode==BSP_VMM_SD )
            {
                static const char * showNorm[3];
                showNorm[0] = tr("Auto PAL50/PAL60");
                showNorm[1] = tr("PAL50 only");
                showNorm[2] = tr("NTSC only");
                Add(new cMenuEditStraItem(tr("Norm/Framerate"), &rbSetup_.norm, 3, showNorm));
                static const char * scartMode[3];
                scartMode[0] = tr ("On");
                scartMode[1] = tr ("At Startup");
                scartMode[2] = tr ("Off");
                Add(new cMenuEditStraItem(tr("Scart Voltage"), &rbSetup_.scart_mode, 3, scartMode));
            }
            if (rbSetup_.main_mode==BSP_VMM_PC)
            {
                static const char * showFramerate[7];
                showFramerate[0] = tr("Auto 75Hz+60Hz (VESA)");
                showFramerate[1] = tr("Auto 50Hz+60Hz (NON-VESA!)");
                showFramerate[2] = tr("Auto 67Hz+60Hz (NON-VESA!)");
                showFramerate[3] = tr("50Hz Only (NON-VESA!)");
                showFramerate[4] = tr("60Hz Only (VESA)");
                showFramerate[5] = tr("67Hz Only (NON-VESA!)");
                showFramerate[6] = tr("75Hz Only (VESA)");

                Add(new cMenuEditStraItem(tr("Framerate"), &rbSetup_.framerate, 7, showFramerate));

                static const char * showResolution[4];
                showResolution[0] = "640x480 (VESA)";
                showResolution[1] = "800x600 (VESA)";
                showResolution[2] = "1024x768 (VESA)";
                showResolution[3] = "1280x720 (NON-VESA!)";
                Add(new cMenuEditStraItem(tr("Resolution"), &rbSetup_.resolution, 4, showResolution));

                if (rbSetup_.resolution)
                {
                    Add(new cMenuEditStraItem(tr("Deinterlacer for SDTV"), &rbSetup_.deint, 3, deint));
                }
                else
                {
                    rbSetup_.deint=0;
                }
            }

            if (rbSetup_.main_mode==BSP_VMM_SDHD)
            {
                static const char * showResolution[3];
                showResolution[0] = tr("no");
                showResolution[1] = tr("1080i");
                showResolution[2] = tr("720p");
                if (rbSetup_.resolution>BSP_VM_RESOLUTION_720 || (RBSetup.main_mode!=BSP_VMM_SDHD && useHdAtStart))
                {
                    rbSetup_.resolution=BSP_VM_RESOLUTION_AUTO;
                    useHdAtStart=0;
                }
                Add(new cMenuEditStraItem(tr("Upscaling for SDTV"), &rbSetup_.resolution, 3, showResolution));

                if (rbSetup_.resolution)
                {
                    Add(new cMenuEditStraItem(tr("Deinterlacer for SDTV"), &rbSetup_.deint, 3, deint));
                }
                else
                {
                    rbSetup_.deint=0;
                }
            }

        }

    }
    else // menu_mode == audio_mode
    {
    SetTitle(tr("Setup - Audio"));
#if APIVERSNUM < 10700
    SetCols(20);
#else
    SetCols(27);
#endif
        //Audio Settings
        Add(new cOsdItem(tr("Audio Settings"), osUnknown, false));
        Add(new cOsdItem(" ", osUnknown, false), false, NULL);


        Add(new cMenuEditBoolItem(tr("Setup.DVB$Use Dolby Digital"), &data.UseDolbyDigital));
        if (data.UseDolbyDigital)
        {
#if defined RBLITE // RB Lite
            if (rbSetup_.usehdext && rbSetup_.HDdmode == HD_DMODE_HDMI)
            {
                Add (new cMenuEditBoolItem(tr(" Output Dolby D/dts on"),      &rbSetup_.audio_over_hdmi,
                                           tr("S/P-DIF"), tr("HDMI")));
                if (rbSetup_.audio_over_hdmi)
                {
                    Add (new cMenuEditBoolItem(tr(" Output as"), &rbSetup_.ac3,
                                               tr("Stereo (PCM)"), tr("bitstream (Dolby D)")));
                }
                else
                {
                    rbSetup_.ac3 = 0;
                }
            }
#elif defined REELVDR // RB AVG
                Add (new cMenuEditBoolItem(tr(" Output as"), &rbSetup_.ac3,
                                           tr("Stereo (PCM)"), tr("bitstream (Dolby D)")));
#else // non RB
            if (rbSetup_.usehdext && rbSetup_.HDdmode == HD_DMODE_HDMI)
            {
                Add (new cMenuEditBoolItem(tr(" Output Dolby D/dts on"),      &rbSetup_.audio_over_hdmi,
                                           tr("Soundcard"), tr("HDMI")));
                if (rbSetup_.audio_over_hdmi)
                {
                    Add (new cMenuEditBoolItem(tr(" Output as"), &rbSetup_.ac3,
                                               tr("Stereo (PCM)"), tr("bitstream (Dolby D)")));
                }
                else
                {
                    rbSetup_.ac3 = 0;
                }
            }

#endif

            Add(new cMenuEditIntItem(tr(" Delay (1/100s)"), &rbSetup_.delay_ac3, -25, 25));
        }
        else
        {
            rbSetup_.audio_over_hdmi = 0;
            rbSetup_.ac3 = 0;
        }

        Add(new cOsdItem(" ", osUnknown, false), false, NULL);
#ifndef RBLITE
#ifdef REELVDR
        Add(new cMenuEditBoolItem(tr("Analog Audio on"), &rbSetup_.audio_over_hd, tr("TRS (3.5mm phone jack)"), tr("HDMI/SCART/RCA jack")));
        Add(new cMenuEditBoolItem(tr("Analog Mix"), &rbSetup_.audiomix, tr("Off"), tr("On")));
#else
        Add(new cMenuEditBoolItem(tr("Analog Audio on"), &rbSetup_.audio_over_hd, tr("Soundcard"), tr("HDMI")));
#endif
#endif

        Add(new cMenuEditIntItem(tr(" Delay (1/100s)"), &rbSetup_.delay_stereo, -25, 25));
    } // if(menu_mode==...)

    SetCurrent(Get(current));
    Display();
}

eOSState cMenuVideoMode::ProcessKey(eKeys Key)
{
    //printf("------------cMenuVideoMode::ProcessKey---------------\n");
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    switch (Key)
    {
        case kRed:
#if REELVDR
            state = osContinue;
            break;
#else
            if(menu_mode == video_mode)
                menu_mode = audio_mode;
            else
                menu_mode = video_mode;
            Setup();
            break;
#endif
        case kGreen:
        case kYellow:
            state = osContinue;
            break;
        case kBlue:
            if (menu_mode == video_mode && data.ExpertOptions)
            {
                expert_mode_changed = true;
                expert_mode = !expert_mode;
                Setup();
            }
            state = osContinue;
            break;
        case kRight:
        case kLeft:
            Setup();
            break;
        case kBack:
        case kOk:
            //TODO: ask the user to keep the new settings, after 10secs restore old ones
            state = osBack;
            break;
        default: state = osUnknown;
    }

#ifndef RBLITE
    if (RBSetup.usehdext == 1)
        Reel::HdCommChannel::SetHWControl(&rbSetup_);
#endif
    return state;
}

void cMenuVideoMode::PostStore()
{

    struct XineAudioMode
    {
        // in
        bool UseDolbyDigital;
    } Data;

    Data.UseDolbyDigital = data.UseDolbyDigital;

    cPluginManager::CallAllServices("Xine AudioMode", &Data);

#ifdef RBLITE
    if(oldAudioOverHdmi_ != RBSetup.audio_over_hdmi)
#else
    if(oldAudioOverHd_ != RBSetup.audio_over_hd)
#endif
    //printf("--------------oldAudioOverHdmi_ != RBSetup.audio_over_hdmi------------\n");
    {
        if(Reel::ReelBoxDevice::Instance())
        {
            Reel::ReelBoxDevice::Instance()->RestartAudio();
        }
    }

    if (oldUseHDext_== RBSetup.usehdext) // only video modes
    {
        //printf (" \033[0;44m  HDext Stat: unchanged  \033[0m\n");
        if (RBSetup.usehdext == 1)
        {
            Reel::HdCommChannel::hda->player[0].pts_shift     = 10 * RBSetup.delay_stereo;
            Reel::HdCommChannel::hda->player[0].ac3_pts_shift = 10 * RBSetup.delay_ac3;
            Reel::HdCommChannel::SetPicture(&RBSetup);
            Reel::HdCommChannel::SetVideomode();
        }
        else
        {
            Reel::Bsp::BspCommChannel::SetVideomode();
        }
        sleep(4);
        //printf("==> REFRESH SCREEN\n");
        Display(); // Refresh screen
    }
    else  // restart BSP resp. HD ext.
    {
        //printf (" \033[0;44m  HDext Stat: chagend   \033[0m\n");

        if (Reel::ReelBoxDevice::Instance()) // ASSERT ?
        {
            // TOD0
            // Reel::ReelBoxDevice::Stop())
        }
        if (Interface->Confirm(tr("Restart ReelBox now? Please wait 2 minutes!"), 30, false))
            SystemExec("/etc/bin/hd_lite_restart.sh");
    }
}


void cMenuVideoMode::Store(void)
{
    //printf (" \033[0;41m %s \033[0m\n", __PRETTY_FUNCTION__);

    RBSetup = rbSetup_;
    //printf("HDdmode %d HDamode %d HDdisplay_type %d HDAspect %d\n", rbSetup_.HDdmode, rbSetup_.HDamode, rbSetup_.HDdisplay_type, rbSetup_.HDaspect);

    plugin->SetupStore("VMM",          RBSetup.main_mode  );
    plugin->SetupStore("VSM",          RBSetup.sub_mode   );
    plugin->SetupStore("DisplayType",  RBSetup.display_type);
    plugin->SetupStore("AudioOverHDMI",RBSetup.audio_over_hdmi);
    plugin->SetupStore("Ac3",          RBSetup.ac3);
    plugin->SetupStore("Norm",         RBSetup.norm       );
    plugin->SetupStore("Aspect",       RBSetup.aspect     );
    plugin->SetupStore("Framerate",    RBSetup.framerate  );
    plugin->SetupStore("Resolution",   RBSetup.resolution );
    plugin->SetupStore("Scartmode",    RBSetup.scart_mode );
    plugin->SetupStore("Deint",        RBSetup.deint );

    plugin->SetupStore("HDoutput",     RBSetup.HDoutput );
    plugin->SetupStore("HDDM",         RBSetup.HDdmode  );
    plugin->SetupStore("HDAM",         RBSetup.HDamode  );
    //plugin->SetupStore("HDoutput",     RBSetup.HDoutput );
    if(RBSetup.HDaport == HD_APORT_SCART) {
        switch(RBSetup.HDswitch_voltage) {
            case 0: RBSetup.HDaport = HD_APORT_SCART; break;
            case 1: RBSetup.HDaport = HD_APORT_SCART_V1; break;
            case 2: RBSetup.HDaport = HD_APORT_SCART_V0; break;
	    default: break;
        }
    }
    plugin->SetupStore("HDAPort",      RBSetup.HDaport  );
    plugin->SetupStore("HDDisplayType", RBSetup.HDdisplay_type );
    plugin->SetupStore("HDAspect",     RBSetup.HDaspect    );
    plugin->SetupStore("HDResolution", RBSetup.HDresolution);
    plugin->SetupStore("HDIntProg",    RBSetup.HDintProg);
    plugin->SetupStore("HDnorm",       RBSetup.HDnorm);
    plugin->SetupStore("HDdeint",      RBSetup.HDdeint);
    plugin->SetupStore("HDfb",         RBSetup.HDfb_dbd);
    plugin->SetupStore("HDauto_format",RBSetup.HDauto_format);
    plugin->SetupStore("Overscan",     RBSetup.HDoverscan);

    plugin->SetupStore("DelayAc3",     RBSetup.delay_ac3);
    plugin->SetupStore("DelayStereo",  RBSetup.delay_stereo);

    plugin->SetupStore("UseHdExt",     RBSetup.usehdext );
#ifndef RBLITE
    plugin->SetupStore("AudioMix",     RBSetup.audiomix);
    plugin->SetupStore("AudioOverHD",  RBSetup.audio_over_hd);
#endif
    //printf (" \033[0;41m %s END \033[0m\n", __PRETTY_FUNCTION__);

    //save the vdr config
    ::Setup = data;
    ::Setup.Save();

    PostStore();
}

bool cMenuVideoMode::HaveDevice(const char *deviceID)
{
    ifstream inFile("/proc/bus/pci/devices");

    if (!inFile.is_open() || !deviceID)
        return  false;

    string line, word;

    bool found = false;
    while (getline(inFile, line))
    {
       std::istringstream splitStr(line);
       string::size_type pos = line.find(deviceID);
       if (pos != string::npos)
       {
          if (string(deviceID) ==  "19058100") // HD EXT
          {
             ; // test -f  /media/hd/opt/linux.bin  && != size
          }
          //printf (" Found Device %s \n", deviceID);
          found = true;
          break;
       }
    }
    return found;
}
} //  namespace REEL
