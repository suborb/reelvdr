/*
 * mcrip.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/interface.h>
#include "setup.h"
#include "ripit.h"
#include "tools.h"

cPluginRipit::cPluginRipit(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginRipit::~cPluginRipit()
{
    // Clean up after yourself!
}


const char *cPluginRipit::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginRipit::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginRipit::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}


bool cPluginRipit::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}


void cPluginRipit::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}


const char *cPluginRipit::MainMenuEntry(void)
{
    if (access(RipitSetup.Ripit_dev, R_OK)==0 || RipitSetup.Ripit_hidden) // file exists
        return tr(MAINMENUENTRY);
    else
    {
        isyslog("[" PLUGIN_NAME "]" " - no CD/DVD drive available - hiding main menu entry");
        return NULL;
    }
}

cOsdObject *cPluginRipit::MainMenuAction(void)
{
    return new cRipitOsd;
}


#if APIVERSNUM >= 10331
const char **cPluginRipit::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {

        "START\n" "   'START [device]'  Start a new rip process. If 'device' is given , use this one else from setup",
        "ABORT\n" "   'ABORT'  Abort a running rip process",
        "STATUS\n" "   'STATUS' Shows status of ripit",
        NULL
    };
    return HelpPages;
}

cString cPluginRipit::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (!strcasecmp(Command, "START"))
    {
        if (IsRipRunning())
        {
            ReplyCode = 550;
            return "Another rip process is running, Can't start new one";
        }
        else
        {
            if (*Option)
            {
                if (access(Option, F_OK) == 0)
                {
                    StartEncoding(Option);
                }
                else
                {
                    ReplyCode = 550;
                    return "Device don't exist, Stopped";
                }
            }
            else
                StartEncoding(NULL);

            return "New rip process started";
        }
    }
    else if (!strcasecmp(Command, "ABORT"))
    {
        if (!IsRipRunning())
        {
            ReplyCode = 550;
            return "Can't abort process because no one is running";
        }
        else
        {
            AbortEncoding();
            return "Rip process aborted";
        }
    }
    else if (!strcasecmp(Command, "STATUS"))
    {
        if (IsRipRunning())
            return "A rip process is running";
        else
            return "No rip process is running";
    }
    else
    {
        ReplyCode = 502;
        return "Wrong command";
    }
    return NULL;
}
#endif


cMenuSetupPage *cPluginRipit::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return new cMenuRipitSetup;
}

bool cPluginRipit::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    if (!strcasecmp(Name, "Ripit_hidden"))
        RipitSetup.Ripit_hidden = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_halt"))
        RipitSetup.Ripit_halt = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_noquiet"))
        RipitSetup.Ripit_noquiet = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_eject"))
        RipitSetup.Ripit_eject = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_fastrip"))
        RipitSetup.Ripit_fastrip = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_lowbitrate"))
        RipitSetup.Ripit_lowbitrate = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_maxbitrate"))
        RipitSetup.Ripit_maxbitrate = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_preset"))
        RipitSetup.Ripit_preset = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_encopts"))
        strn0cpy(RipitSetup.Ripit_encopts, Value, sizeof(RipitSetup.Ripit_encopts));
    else if (!strcasecmp(Name, "Ripit_dev"))
        strn0cpy(RipitSetup.Ripit_dev, Value, sizeof(RipitSetup.Ripit_dev));
    else if (!strcasecmp(Name, "Ripit_dir"))
        strn0cpy(RipitSetup.Ripit_dir, Value, sizeof(RipitSetup.Ripit_dir));
    else if (!strcasecmp(Name, "Ripit_nice"))
        RipitSetup.Ripit_nice = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_remote"))
        RipitSetup.Ripit_remote = atoi(Value);
    else if (!strcasecmp(Name, "Ripit_remotevalue"))
        strn0cpy(RipitSetup.Ripit_remotevalue, Value, sizeof(RipitSetup.Ripit_remotevalue));
    else if (!strcasecmp(Name, "Ripit_encoder"))
        RipitSetup.Ripit_encoder = (ripitEncoder) atoi(Value);

    else
        return false;
    return true;
}

cMenuRipitSetup::cMenuRipitSetup(void)
{
    bitrateStrings[0] = "32";
    bitrateStrings[1] = "64";
    bitrateStrings[2] = "96";
    bitrateStrings[3] = "112";
    bitrateStrings[4] = "128";
    bitrateStrings[5] = "160";
    bitrateStrings[6] = "192";
    bitrateStrings[7] = "224";
    bitrateStrings[8] = "256";
    bitrateStrings[9] = "320";

    encoders[0] = "OGG";
    encoders[1] = "FLAC";
    encoders[2] = "WAV";
    encoders[3] = "MP3";

    preset[0] = tr("user defined");
    preset[1] = tr("low");
    preset[2] = tr("standard");
    preset[3] = tr("great");
    preset[4] = tr("best");

    Setup();
}

void cMenuRipitSetup::Setup(void)
{
    static const char allowed[] = { "abcdefghijklmnopqrstuvwxyz0123456789-_/" };
    int current = Current();

    Clear();

    SetCols(22);

    int nrEncoders = 3;
    if (IsLameInstalled())
        nrEncoders = 4;

    Add(new cMenuEditStraItem(tr("Destination format"), &RipitSetup.Ripit_encoder, nrEncoders, encoders));

    if (RipitSetup.Ripit_allSecretOptions)
    {
        Add(new cMenuEditBoolItem(tr("Hide mainmenu entry"), (int *)&RipitSetup.Ripit_hidden));
        Add(new cMenuEditBoolItem(tr("Shutdown when finished"), (int *)&RipitSetup.Ripit_halt));
        Add(new cMenuEditBoolItem(tr("Verbose output"), (int *)&RipitSetup.Ripit_noquiet));
        Add(new cMenuEditBoolItem(tr("Fast mode"), (int *)&RipitSetup.Ripit_fastrip));
    }

    Add(new cMenuEditBoolItem(tr("Eject CD when finished"), (int *)&RipitSetup.Ripit_eject));

    if (RipitSetup.Ripit_encoder == RIPIT_LAME || RipitSetup.Ripit_encoder == RIPIT_OGG)
    {
        Add(new cMenuEditStraItem(tr("Presets"), &RipitSetup.Ripit_preset, 5, preset));

        if (RipitSetup.Ripit_preset == 0 /*&& RipitSetup.Ripit_encoder == RIPIT_LAME */ )
        {
            Add(new cMenuEditStraItem(tr("Min. bitrate"), &RipitSetup.Ripit_lowbitrate, 10, bitrateStrings));
            Add(new cMenuEditStraItem(tr("Max. bitrate"), &RipitSetup.Ripit_maxbitrate, 10, bitrateStrings));
        }
    }

    if (RipitSetup.Ripit_allSecretOptions)
    {
        Add(new cMenuEditStrItem(tr("More encoder settings"), RipitSetup.Ripit_encopts, 255, allowed));
        Add(new cMenuEditStrItem(tr("Device"), RipitSetup.Ripit_dev, 255, allowed));
        Add(new cMenuEditStrItem(tr("Directory for ripped tracks"), RipitSetup.Ripit_dir, 255, allowed));
        Add(new cMenuEditIntItem(tr("Priority of task (nice)"), &RipitSetup.Ripit_nice, -20, 19));

        Add(new cMenuEditBoolItem(tr("Encode remotely"), (int *)&RipitSetup.Ripit_remote));
        if (RipitSetup.Ripit_remote)
            Add(new cMenuEditStrItem(tr("Options for remote encoding"), RipitSetup.Ripit_remotevalue, 255, allowed));
    }

    SetCurrent(Get(current));
    Display();

}


void cMenuRipitSetup::Store(void)
{
    SetupStore("Ripit_hidden", RipitSetup.Ripit_hidden);
    SetupStore("Ripit_halt", RipitSetup.Ripit_halt);
    SetupStore("Ripit_noquiet", RipitSetup.Ripit_noquiet);
    SetupStore("Ripit_eject", RipitSetup.Ripit_eject);
    SetupStore("Ripit_fastrip", RipitSetup.Ripit_fastrip);
    SetupStore("Ripit_lowbitrate", RipitSetup.Ripit_lowbitrate);
    SetupStore("Ripit_maxbitrate", RipitSetup.Ripit_maxbitrate);
    SetupStore("Ripit_preset", RipitSetup.Ripit_preset);
    SetupStore("Ripit_encopts", RipitSetup.Ripit_encopts);
    SetupStore("Ripit_dev", RipitSetup.Ripit_dev);
    SetupStore("Ripit_dir", RipitSetup.Ripit_dir);
    SetupStore("Ripit_nice", RipitSetup.Ripit_nice);
    SetupStore("Ripit_remote", RipitSetup.Ripit_remote);
    SetupStore("Ripit_remotevalue", RipitSetup.Ripit_remotevalue);
    SetupStore("Ripit_encoder", RipitSetup.Ripit_encoder);
}


eOSState cMenuRipitSetup::ProcessKey(eKeys Key)
{
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    Setup();
    return state;
}



VDRPLUGINCREATOR(cPluginRipit); // Don't touch this!
