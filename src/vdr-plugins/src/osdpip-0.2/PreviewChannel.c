#include <vdr/tools.h> // for uchar
#include "PreviewChannel.h"
#include <vdr/reelboxbase.h>

cPreviewChannel::cPreviewChannel(int chNumber)
    :pipPlayer(0), pipReceiver(0), device(0)
{
    SetChannel(chNumber);
    StartPreview();
}


cPreviewChannel::~cPreviewChannel()
{
    StopPreview();
}


bool cPreviewChannel::SetChannel(int chNumber)
{
    cChannel *chan = Channels.GetByNumber(chNumber);
    if (!chan) return false;

    channelNumber = chNumber;
    cDevice *dev = cDevice::GetDevice(chan, 0, true); // least priorty
    if (!dev) return false;

    device = dev;
    device->SwitchChannel(chan, false);

    // stop any preview that maybe running
    StopPreview();

    pipPlayer = new cPipPlayer();
    pipReceiver = new cOsdPipReceiver(chan, pipPlayer);

    device->AttachReceiver(pipReceiver);

    return true;
}



bool cPreviewChannel::StartPreview()
{
    if(cReelBoxBase::Instance()) {
        //SetDimensions()
        cReelBoxBase::Instance()->StartPip(true);
        return true;
    }

    return false;
}


bool cPreviewChannel::StopPreview()
{
    isyslog("--- stop preview ----\n");
    if(cReelBoxBase::Instance()) {
        cReelBoxBase::Instance()->StartPip(false);
        printf("\033[0;92mStop pip\033[0m\n");
    }
    else
        esyslog("cReelBoxBase::Instance not set!");

    delete pipPlayer;
    delete pipReceiver;

    pipPlayer = 0;
    pipReceiver = 0;

    return true;
}


bool cPreviewChannel::SetDimensions(int x1, int y1, int width, int height)
{
    if(cReelBoxBase::Instance()) {
        //if(pipPlayer) pipPlayer->SetPipDimensions(pipPositions[OsdPipSetup.PipPosition].xpos, pipPositions[OsdPipSetup.PipPosition].ypos, 0, 0);
        if(pipPlayer) {
            pipPlayer->SetPipDimensions(x1, y1, width, height);
            return true;
        }
    }
    return false;
}
