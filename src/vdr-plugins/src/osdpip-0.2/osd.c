/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#include "osd.h"
#include "receiver.h"
#include "setup.h"

#include "../filebrowser/servicestructs.h"
#include "../filebrowser/stillpictureplayer.h"

#include <iostream>

#include <vdr/debug.h>s
#include <vdr/ringbuffer.h>
#include <vdr/remux.h>
#include <vdr/thread.h>
#include <vdr/device.h>
#include <vdr/player.h>
#include <vdr/device.h>
#include <vdr/config.h>
#include <vdr/reelboxbase.h>

#if VDRVERSNUM >= 10716
int cOsdPipObject::CurrentPipChannel = -1;
#endif
int cOsdPipObject::NextPipChannel = -1;

//#if APIVERSNUM < 10716
//#define IS_HD_CHANNEL(c)              (c->IsSat() && (c->Modulation() == 10))
//#else
//#define IS_HD_CHANNEL(c)              ( (c->Vpid() != 0) && c->IsSat() && (cDvbTransponderParameters(c->Parameters()).System() == SYS_DVBS2 || strstr(c->Name(), " HD")))
//#endif
//
// PiP available only for SD video channels
// skip audio and HD channels
//let player decide: #define CAN_DISPLAY_CHANNEL_IN_PIP(c) ((c->Vpid() != 0) &&  !IS_HD_CHANNEL(c))
#define CAN_DISPLAY_CHANNEL_IN_PIP(c) (player && player->IsPipChannel(c))

cMutex Mutex;

cOsdPipObject::cOsdPipObject(cDevice * Device, const cChannel * Channel): cOsdObject(true)
{
    m_Channel = Channel;
    m_Osd = NULL;

#if VDRVERSNUM < 10716
    Setup.PipIsActive = true;
    Setup.PipIsRunning = true;
#endif

    m_Ready = false;
    m_Width = 720 / 4;
    m_Height = 576 / 4;
    m_Bitmap = NULL;
    m_InfoWindow = NULL;

    m_AlphaBase = 0xFF000000;
    for (int i = 0; i < 256; i++)
        m_Palette[i] = 0xFD000000 | i;
    m_PaletteStart = 1;

    m_Device = Device;
    Device->SwitchChannel(m_Channel, false);
    player = new cPipPlayer();
    m_Receiver = new cOsdPipReceiver(m_Channel, player);
    Device->AttachReceiver(m_Receiver);

    //printf("------------------StartPip------------------------\n");
    if(cReelBoxBase::Instance()) {
        if(player) player->SetPipDimensions(pipPositions[OsdPipSetup.PipPosition].xpos, pipPositions[OsdPipSetup.PipPosition].ypos, 0, 0);
        cReelBoxBase::Instance()->StartPip(true);
    } else esyslog("cReelBoxBase::Instance not set!");
}

cOsdPipObject::~cOsdPipObject()
{
    //printf("------------------EndPip------------------------\n");
    if(cReelBoxBase::Instance()) cReelBoxBase::Instance()->StartPip(false);
    else esyslog("cReelBoxBase::Instance not set!");

    delete m_Receiver;
    delete player;

#if VDRVERSNUM < 10716
    Setup.PipIsRunning = false;
    Setup.CurrentPipChannel = m_Channel->Number();
#else
    CurrentPipChannel = m_Channel->Number();
#endif

    if (m_Bitmap != NULL) {
        delete m_Bitmap;
    }

    delete m_InfoWindow;
    if (m_Osd != NULL) {
        delete m_Osd;
    }
}

/* swap live TV and PiP channels
   only if LiveTV can be shown inside PiP Window */
void cOsdPipObject::SwapChannels(void)
{
    const cChannel *chan = cDevice::CurrentChannel() != 0 ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
    if (chan && CAN_DISPLAY_CHANNEL_IN_PIP(chan)) {
        Channels.SwitchTo(m_Channel->Number());
#if VDRVERSNUM < 10716
        cDevice *dev = cDevice::GetDevice(chan, 1);
#else
        cDevice *dev = cDevice::GetDevice(chan, 0, true);
#endif
        if (dev) {
            DELETENULL(m_Receiver);
            m_Channel = chan;
            dev->SwitchChannel(m_Channel, false);
            m_Receiver = new cOsdPipReceiver(m_Channel, player);
            dev->AttachReceiver(m_Receiver);
        }
        StartDisplay();
    } else if (chan) // ie cannot display channel in PiP
    {
//        Skins.Message(mtError, tr("Cannot swap to HD/radio channels"));
        Skins.Message(mtError, tr("Cannot swap to this channel"));
    }
}

void cOsdPipObject::AdoptPipChannel()
{
    Channels.SwitchTo(m_Channel->Number());
    StartDisplay();
}

/* Switches to next available channel in "Direction"
   jumps pin protected channels and channels that cannot be shown
   inside PiP window */
bool cOsdPipObject::SwitchPipChannel(int Direction)
{
    bool result = false;
    Direction = sgn(Direction);
    //DELETENULL(m_Receiver);

    if(m_Device)
    {
        m_Device->Detach(m_Receiver);
        DELETENULL(m_Receiver);
    }

    if (Direction) {
        int n = m_Channel->Number() + Direction;
        int first = n;
        cChannel *channel = NULL;
        cDevice *dev = NULL;
        while ((channel = Channels.GetByNumber(n, Direction)) != NULL) {
#if VDRVERSNUM < 10716
            // try only channels which are currently available
            bool ndr = false;
            dev = cDevice::GetDevice(channel, 0, &ndr);
            if (dev && !ndr && !cStatus::MsgChannelProtected(0, channel)// PIN PATCH
                && CAN_DISPLAY_CHANNEL_IN_PIP(channel))
            {
                break;
            }
            n = channel->Number() + Direction;
#else
            dev = cDevice::GetDevice(channel, 0, 1);
            if (dev && !cStatus::MsgChannelProtected(0, channel)     // PIN PATCH
                && CAN_DISPLAY_CHANNEL_IN_PIP(channel))
            {
                break;
            }
            n = channel->Number() + Direction;
#endif
        }
        if (channel) {
            DELETENULL(m_Receiver);
            int d = n - first;
            if (abs(d) == 1)
                dsyslog("skipped pip channel %d", first);
            else if (d)
                dsyslog("skipped pip channels %d..%d", first, n - sgn(d));
            if (dev) {
                m_Channel = channel;
                dev->SwitchChannel(channel, false);
                m_Receiver = new cOsdPipReceiver(m_Channel, player);
                dev->AttachReceiver(m_Receiver);
                m_Device = dev;
                result = true;
            }
        } else {
            Skins.Message(mtError, tr("Pip channel not available!"));
        }
        if (result && OsdPipSetup.ShowInfo)
        {
            m_InfoWindow->SetChannel(channel, true);
        }
        m_InfoWindow->Show(true, (OsdPipSetup.ShowInfo == 0));
    }
    return result;
}


void cOsdPipObject::SetPipChannelCurrent()
{
    const cChannel *chan = cDevice::CurrentChannel() != 0 ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
    if (chan && CAN_DISPLAY_CHANNEL_IN_PIP(chan)) {
#if VDRVERSNUM < 10716
        cDevice *dev = cDevice::GetDevice(chan, 1);
#else
        cDevice *dev = cDevice::GetDevice(chan, 0, true);
#endif
        if (dev) {
            DELETENULL(m_Receiver);
            m_Channel = chan;
            dev->SwitchChannel(m_Channel, false);
            m_Receiver = new cOsdPipReceiver(m_Channel, player);
            dev->AttachReceiver(m_Receiver);
        }
        StartDisplay();
    } else if (chan) { // ie. cannot display channel in Pip
//        Skins.Message(mtError, tr("Cannot show HD/radio channels in PiP"));
        Skins.Message(mtError, tr("Cannot show this channel in PiP"));
    }
}

void cOsdPipObject::StartDisplay(void)
{
    if (m_Osd)
        delete m_Osd;
#ifdef TRUECOLOR
    m_Osd = cOsdProvider::NewTrueColorOsd(0, 0, 0, 0);
#else
    m_Osd = cOsdProvider::NewOsd(0, 0);
#endif
    if (!m_Osd)
        return;
    m_Ready = false;
    ProcessImage(NULL, 0);
}

void cOsdPipObject::Show(void)
{
    StartDisplay();
}

bool cOsdPipObject::PicturePlayerIsActive()
{
    FileBrowserPicPlayerInfo info;
    //cPluginManager::CallAllServices("Filebrowser get pictureplayer info", &info);
    return info.active;
}

eOSState cOsdPipObject::ProcessKey(eKeys Key)
{
    if(PicturePlayerIsActive())
    {
        return osUser10; //very dirty hack to make pip compatible with pic player
    }

    //if (Key == kUser6)
        //std::cout << "-----------cOsdPipObject::ProcessKey, Key = kUser6-----------" << std::endl;

    eOSState state = cOsdObject::ProcessKey(Key);
    if (state == osUnknown) {
        if (m_Ready && m_InfoWindow) {
            state = m_InfoWindow->ProcessKey(Key);
        }
    }
    if (state == osUnknown) {
        switch (Key & ~k_Repeat)
        {
        case kGreater:
	case kFastFwd:
	    if (OsdPipSetup.PipPosition == 3)
		OsdPipSetup.PipPosition = kInfoTopLeft;
	    else
		OsdPipSetup.PipPosition += 1;
            StartDisplay();
            break;
	case kLess:
	case kFastRew:
	    if (OsdPipSetup.PipPosition == 0)
		OsdPipSetup.PipPosition = kInfoBottomLeft;
	    else
		OsdPipSetup.PipPosition -= 1;
	    StartDisplay();
	    break;
        case kPiP:
            // swap channels only when showing LiveTV
            // not when replaying

            // Live TV = no Control() OR (Control() present and IS Livebuffer control)
            if ( !cControl::Control()
#ifdef USE_LIVEBUFFER
                    || cRecordControls::GetLiveIndex(cReplayControl::NowReplaying()) != NULL /*if control present it IS live buffer*/
#endif
               )
            {
                SwapChannels();
                break;
            }
        case kBack:
#if VDRVERSNUM < 10716
            Setup.PipIsActive = false;
#endif
            return osEnd;
        case kRed:
            SetPipChannelCurrent();
            break;
        case kGreen:
            AdoptPipChannel();
            break;
        case kBlue:
            //printf("#################case kBlue####################\n");
            Channels.SwitchTo(m_Channel->Number());
#if VDRVERSNUM < 10716
            Setup.PipIsActive = false;
            Setup.PipSwitchedToTV = true;
#endif
            return osEnd;
        case kUser6:
        case kYellow:
            SwapChannels();
            break;
        case kChanUp:
            SwitchPipChannel(1);
            break;
        case kChanDn:
            SwitchPipChannel(-1);
            break;
        case kUp:
        case kDown:
            cDevice::SwitchChannel(NORMALKEY(Key) == kUp ? 1 : -1);
            break;
        case kOk:
            m_InfoWindow->Show(false, (OsdPipSetup.ShowInfo == 0));
            break;
        default:
            return state;
        }
        state = osContinue;
    }
    return state;
}

void cOsdPipObject::ChannelSwitch(const cDevice * device, int channelNumber)
{
    if (device != cDevice::ActualDevice())
        return;
    if (channelNumber == 0)
        return;
    if (channelNumber != cDevice::CurrentChannel())
        return;
    if (!m_Ready)
        return;
    if (OsdPipSetup.ShowInfo)
    {
        m_InfoWindow->SetChannel(Channels.GetByNumber(channelNumber), false);
    }
    m_InfoWindow->Show(false, (OsdPipSetup.ShowInfo == 0));
}

void cOsdPipObject::OsdClear(void)
{
}

void cOsdPipObject::OsdStatusMessage(const char *message)
{
    if (!m_Ready)
        return;
    if (OsdPipSetup.ShowInfo)
        m_InfoWindow->SetMessage(message);
    m_InfoWindow->Show(false, (OsdPipSetup.ShowInfo == 0));
}

void cOsdPipObject::ProcessImage(unsigned char *data, int length)
{
    if (!m_Ready) {
        if (m_Bitmap != NULL)
            delete m_Bitmap;
        m_Bitmap = NULL;
        if (m_InfoWindow != NULL)
            delete m_InfoWindow;
        m_InfoWindow = NULL;

//        if (OsdPipSetup.ShowInfo > 0) {
            int infoX = 0;
            int infoY = 0;
            int infoH = OsdPipSetup.ShowInfo * 30;

            switch (OsdPipSetup.InfoPosition) {
            case kInfoTopLeft:
                infoX = Setup.OSDLeft;
                infoY = Setup.OSDTop;
                break;
            case kInfoTopRight:
                infoX = Setup.OSDLeft + Setup.OSDWidth - OsdPipSetup.InfoWidth;
                infoY = Setup.OSDTop;
                break;
            case kInfoBottomLeft:
                infoX = Setup.OSDLeft;
                infoY = Setup.OSDTop + Setup.OSDHeight - infoH;
                break;
            case kInfoBottomRight:
                infoX = Setup.OSDLeft + Setup.OSDWidth - OsdPipSetup.InfoWidth;
                infoY = Setup.OSDTop + Setup.OSDHeight - infoH;
                break;
            }
            tArea areas[] = { {0, 0, 720, 576, 4} };
#if 0
#ifdef TRUECOLOR
            tArea areas[] = { {infoX, infoY, infoX + OsdPipSetup.InfoWidth - 1, infoY + infoH - 1, 32} };
#else
            tArea areas[] = { {infoX, infoY, infoX + OsdPipSetup.InfoWidth - 1, infoY + infoH - 1, 4} };
#endif
#endif

#if 0
            std::cout << "x: " << infoX << " y: " << infoY << std::endl;
            std::cout << "x: " << pipPositions[OsdPipSetup.PipPosition].xpos << " y: " << pipPositions[OsdPipSetup.PipPosition].ypos << std::endl;
            std::cout << "i: " << OsdPipSetup.PipPosition << std::endl;
#endif

            m_Osd->SetAreas(areas, 1);
            m_InfoWindow = new cOsdInfoWindow(m_Osd, m_Palette, infoX, infoY, pipPositions[OsdPipSetup.PipPosition].xpos, pipPositions[OsdPipSetup.PipPosition].ypos);
        //}
    }
    if (!m_Ready) {
        if (OsdPipSetup.ShowInfo)
        {
            m_InfoWindow->SetChannel(Channels.GetByNumber(cDevice::ActualDevice()-> CurrentChannel()), false);
            m_InfoWindow->SetChannel(m_Channel, true); //display actual pip channel when starting pip
        }
        m_InfoWindow->Show(true, (OsdPipSetup.ShowInfo == 0));
        m_Ready = true;
    }
    player->SetPipDimensions(pipPositions[OsdPipSetup.PipPosition].xpos, pipPositions[OsdPipSetup.PipPosition].ypos, 0, 0);

}

