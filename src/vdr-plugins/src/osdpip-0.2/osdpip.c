/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

/*extern "C"
{
#ifdef HAVE_FFMPEG_STATIC
# include <avcodec.h>
#else
# include <ffmpeg/avcodec.h>
#endif
}*/

#include "osd.h"
#include "setup.h"
#include "PreviewChannel.h"

#include <vdr/config.h>
#include <vdr/plugin.h>
#include <vdr/remote.h>

/* remember cPreviewChannel object so that it maybe changed/deleted as and when required */
static cPreviewChannel *previewChannel=NULL;

static const char *VERSION = "0.0.8";
static const char *DESCRIPTION = "OSD Picture-in-Picture";
static const char *MAINMENUENTRY = trNOOP("Picture-in-Picture");

class cPluginOsdpip:public cPlugin
{
  private:

  public:
    cPluginOsdpip(void);
      virtual ~ cPluginOsdpip();
    virtual const char *Version(void)
    {
        return VERSION;
    }
    virtual const char *Description(void)
    {
        return tr(DESCRIPTION);
    }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Housekeeping(void);
    virtual const char *MainMenuEntry(void)
    {
        return tr(MAINMENUENTRY);
    }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    bool Service(char const *id, void *data);
};

cPluginOsdpip::cPluginOsdpip(void)
{
}

cPluginOsdpip::~cPluginOsdpip()
{
}

const char *
cPluginOsdpip::CommandLineHelp(void)
{
    return NULL;
}

bool
cPluginOsdpip::ProcessArgs(int argc, char *argv[])
{
    return true;
}

bool
cPluginOsdpip::Initialize(void)
{
    return true;
}

bool
cPluginOsdpip::Start(void)
{
    return true;
}

void
cPluginOsdpip::Housekeeping(void)
{
}

cOsdObject *
cPluginOsdpip::MainMenuAction(void)
{
    /*if (Channel->Number() == cDevice::CurrentChannel()) {
       if (!cDevice::PrimaryDevice()->Replaying() || cTransferControl::ReceiverDevice()) {
       if (cDevice::ActualDevice()->ProvidesTransponder(Channel)) */

    //ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL)

    const cChannel *chan;
    cDevice *dev;
    if(cOsdPipObject::NextPipChannel > 0) {
#if VDRVERSNUM < 10716
        Setup.CurrentPipChannel = cOsdPipObject::NextPipChannel;
#else
        cOsdPipObject::CurrentPipChannel = cOsdPipObject::NextPipChannel;
#endif
        cOsdPipObject::NextPipChannel = -1;
    }
#if VDRVERSNUM < 10716
    if (Setup.CurrentPipChannel <= 0)
        Setup.CurrentPipChannel = cDevice::CurrentChannel();
    chan = Channels.GetByNumber(Setup.CurrentPipChannel);
#else
    if (cOsdPipObject::CurrentPipChannel <= 0)
        cOsdPipObject::CurrentPipChannel = cDevice::CurrentChannel();
    chan = Channels.GetByNumber(cOsdPipObject::CurrentPipChannel);
#endif
#if VDRVERSNUM < 10716
    bool needsDetachReceiver = false;

    cDevice::GetDevice(chan, 0, &needsDetachReceiver);

    //cDevice::GetDevice(0)->ProvidesChannel(chan, 0, &needsDetachReceiver);

    if (chan == NULL || needsDetachReceiver)
        chan = cDevice::CurrentChannel() != 0
            ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
    if (chan != NULL)
    {
        dev = cDevice::GetDevice(chan, 1);
        if (dev)
        {
            return new cOsdPipObject(dev, chan);
        }
    }
#else
    dev = cDevice::GetDevice(chan, 0, true);
    if (!dev || !chan) {
        chan = cDevice::CurrentChannel() != 0
            ? Channels.GetByNumber(cDevice::CurrentChannel()) : NULL;
        dev = cDevice::GetDevice(chan, 0, true);
    }
    if(dev && chan) return new cOsdPipObject(dev, chan);
#endif
    return NULL;
}

cMenuSetupPage *
cPluginOsdpip::SetupMenu(void)
{
    return new cOsdPipSetupPage;
}

bool
cPluginOsdpip::SetupParse(const char *Name, const char *Value)
{
    return OsdPipSetup.SetupParse(Name, Value);
}


bool
cPluginOsdpip::Service(char const *id, void *data)
{
    if (!id) return false;

    if (strcmp(id, "start preview channel")==0) {
        if (!data) return false;
        int number = *((int*) data);

        if (previewChannel)  {
            previewChannel->SetChannel(number);
            previewChannel->StartPreview();
        } else {
            //delete previewChannel;
            previewChannel = new cPreviewChannel(number);
        }
        previewChannel->SetDimensions(55, 75, 140, 100);
    } else if (strcmp(id, "stop preview channel")==0) {
        delete previewChannel;
        previewChannel = 0;
    } else if (strcmp(id, "show pip channel")==0) {
        int number = *((int*) data);
        cOsdPipObject::NextPipChannel = number;
        cRemote::CallPlugin("osdpip");
        return true;
    } else if (strcmp(id, "is pip channel")==0) {
        int number = *((int*) data);
        const cChannel *chan = Channels.GetByNumber(number);
        if(!chan) return false;
        cPipPlayer player;
        if(!player.IsPipChannel(chan)) return false;
        return true;
    }

    return false;
}

VDRPLUGINCREATOR(cPluginOsdpip);        // Don't touch this!
