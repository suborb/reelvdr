#include "tools.h"

#include <stdio.h>
#include <string>

#include <vdr/i18n.h>
#include "ripitstatus.h"

cRipitStatus *ripStatus;

/* converts the menu's int-index-value to bitrates */
void GenMinMaxBitrateFromSetup(int *lowbit, int *maxbit)
{
    switch (RipitSetup.Ripit_lowbitrate)
    {
    case 0:
        *lowbit = 32;
        break;
    case 1:
        *lowbit = 64;
        break;
    case 2:
        *lowbit = 96;
        break;
    case 3:
        *lowbit = 112;
        break;
    case 4:
        *lowbit = 128;
        break;
    case 5:
        *lowbit = 160;
        break;
    case 6:
        *lowbit = 192;
        break;
    case 7:
        *lowbit = 224;
        break;
    case 8:
        *lowbit = 256;
        break;
    case 9:
        *lowbit = 320;
        break;
    default:
        break;
    }

    switch (RipitSetup.Ripit_maxbitrate)
    {
    case 0:
        *maxbit = 32;
        break;
    case 1:
        *maxbit = 64;
        break;
    case 2:
        *maxbit = 96;
        break;
    case 3:
        *maxbit = 112;
        break;
    case 4:
        *maxbit = 128;
        break;
    case 5:
        *maxbit = 160;
        break;
    case 6:
        *maxbit = 192;
        break;
    case 7:
        *maxbit = 224;
        break;
    case 8:
        *maxbit = 256;
        break;
    case 9:
        *maxbit = 320;
        break;
    default:
        break;
    }
}

/* Build the beginning of the command string */
void BuildCommonOptsStart(std::stringstream * buffer, const char *Device)
{
    std::string device;
    if (Device)
        device = Device;
    else
        device = RipitSetup.Ripit_dev;

    *buffer << "`echo 'Ripping process started....' > " << RIPIT_LOG << " ; ";
    *buffer << " touch /tmp/ripit.process; ";
    *buffer << " nice -n " << RipitSetup.Ripit_nice;
    *buffer << " ripit.pl ";
    *buffer << " --device " << device;
    *buffer << " --nointeraction ";
    *buffer << " --outputdir '" << RipitSetup.Ripit_dir << "'";
    if (!RipitSetup.Ripit_noquiet)
        *buffer << " --quiet ";
    if (RipitSetup.Ripit_eject)
        *buffer << " --eject ";
    if (RipitSetup.Ripit_halt)
        *buffer << " --halt ";
    if (RipitSetup.Ripit_fastrip)
        *buffer << " '--ripopt -Z' ";
    if (RipitSetup.Ripit_remote)
        *buffer << RipitSetup.Ripit_remotevalue;
}

/* Builds the end of the command string */
void BuildCommonOptsEnd(std::stringstream * buffer)
{
    *buffer << " `& ";
}

/* Abort the Encoding process */
void AbortEncoding(void)
{
    std::stringstream buffer;
    buffer << "killall ripit.pl ; killall oggenc ; killall lame; killall flac ; killall cdparanoia ; echo '" << RIPIT_LOG;
    buffer << "' > " << tr("PROCESS MANUALLY ABORTED") << "; rm -f /tmp/ripit.process";
    esyslog("Ripit: Executing '%s'\nProcess manually aborted ...!!!\n", buffer.str().c_str());
    SystemExec(buffer.str().c_str());
    if (ripStatus)
        ripStatus->Abort();
}

void StartEncodingWav(const char *Device)
{
    std::stringstream buffer;

    BuildCommonOptsStart(&buffer, Device);
    /* "-w" is "keep WAVs" */
    buffer << " -w --noencode ";
    BuildCommonOptsEnd(&buffer);

    dsyslog("Ripit: Executing wav-ripping: '%s'", buffer.str().c_str());
    ripStatus = new cRipitStatus(RIPIT_LOG, "ripping CD");
    ripStatus->Start();
    SystemExec(buffer.str().c_str());
}

void StartEncodingLame(const char *Device)
{
    std::stringstream buffer;
    int lowbit = 0;
    int maxbit = 0;

    GenMinMaxBitrateFromSetup(&lowbit, &maxbit);

    BuildCommonOptsStart(&buffer, Device);
    buffer << " --coder 0 ";

    if (RipitSetup.Ripit_preset)
    {
        switch (RipitSetup.Ripit_preset)
        {
        case 1:
            buffer << " --lameopt '--preset medium' ";
            break;
        case 2:
            buffer << " --lameopt '--alt-preset 192' ";
            break;
        case 3:
            buffer << " --lameopt '--preset extreme -V 2 --vbr-new -h' ";
            break;
        case 4:
            buffer << " --lameopt '--preset 320 -h' ";
            break;
        default:
            break;
        }
    }
    else
        buffer << " --bitrate " << lowbit << " --maxrate " << maxbit;
    if (strcmp(RipitSetup.Ripit_encopts, ""))
        buffer << RipitSetup.Ripit_encopts;
    BuildCommonOptsEnd(&buffer);

    dsyslog("Ripit: Executing mp3-ripping: '%s'", buffer.str().c_str());
    ripStatus = new cRipitStatus(RIPIT_LOG, "ripping CD");
    ripStatus->Start();
    SystemExec(buffer.str().c_str());
}

void StartEncodingFlac(const char *Device)
{
    std::stringstream buffer;

    BuildCommonOptsStart(&buffer, Device);
    buffer << " --coder 2 ";

    //buffer << " --flacopt '" << presetarg;
    if (strcmp(RipitSetup.Ripit_encopts, ""))
        buffer << RipitSetup.Ripit_encopts;
    BuildCommonOptsEnd(&buffer);

    dsyslog("Ripit: Executing Flac-ripping: '%s'", buffer.str().c_str());
    ripStatus = new cRipitStatus(RIPIT_LOG, "ripping CD");
    ripStatus->Start();
    SystemExec(buffer.str().c_str());
}

void StartEncodingOgg(const char *Device)
{
    std::stringstream buffer;
    int lowbit = 0, maxbit = 0;

    GenMinMaxBitrateFromSetup(&lowbit, &maxbit);

    BuildCommonOptsStart(&buffer, Device);
    buffer << " --coder 1 ";

    buffer << " --bitrate " << lowbit << " --maxrate " << maxbit;
    //buffer << " --oggopt '" << presetarg; 
    if (RipitSetup.Ripit_preset)
        buffer << " -q " << 1 + RipitSetup.Ripit_preset * 2;    //TB: quality 1-10 for OGG
    if (strcmp(RipitSetup.Ripit_encopts, ""))
        buffer << " " << RipitSetup.Ripit_encopts;
    BuildCommonOptsEnd(&buffer);

    dsyslog("Ripit: Executing Ogg-Ripping: '%s'", buffer.str().c_str());
    ripStatus = new cRipitStatus(RIPIT_LOG, "ripping CD");
    ripStatus->Start();
    SystemExec(buffer.str().c_str());
}

void StartEncoding(const char *Device)
{
    remove(RIPIT_LOG);          // delete the old logfile

    switch (RipitSetup.Ripit_encoder)
    {
    case RIPIT_WAV:
        StartEncodingWav(Device);
        break;
    case RIPIT_OGG:
        StartEncodingOgg(Device);
        break;
    case RIPIT_LAME:
        StartEncodingLame(Device);
        break;
    case RIPIT_FLAC:
        StartEncodingFlac(Device);
        break;
    }
}

bool IsRipRunning(void)
{
    return access("/tmp/ripit.process", F_OK) == 0;
}

bool IsLameInstalled(void)
{
    return access("/usr/bin/lame", F_OK) == 0;
}
