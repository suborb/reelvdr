/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef VDR_OSDPIP_OSD_H
#define VDR_OSDPIP_OSD_H

#include <sys/time.h>

#include <vdr/osd.h>
#include <vdr/thread.h>
#include <vdr/status.h>
#include <vdr/receiver.h>

#include "osd_info.h"
#include "player.h"

class cOsdPipReceiver;
class cReelBoxDevice;
class cPesAssembler;

class cOsdPipObject:public cOsdObject, public cThread, public cStatus
{
  private:
    cOsd * m_Osd;
    cOsdPipReceiver *m_Receiver;
    cPipPlayer *player;
    const cChannel *m_Channel;
    cDevice *m_Device;

    cBitmap *m_Bitmap;
    cOsdInfoWindow *m_InfoWindow;

    //bool m_Active;
    bool m_Ready;
    //bool m_Reset;
    bool m_MoveMode;
    int m_Width, m_Height;
    int m_FrameDrop;

    unsigned int m_AlphaBase;
    unsigned int m_Palette[256];
    int m_PaletteStart;

    //void Stop(void);
    //void StopDisplay(void);
    void StartDisplay();
    void SwapChannels();
    void AdoptPipChannel();
    void SetPipChannelCurrent();
    bool SwitchPipChannel(int Direction);
    bool PicturePlayerIsActive();
  protected:
      virtual void Action()
    {
    };
    virtual void ChannelSwitch(const cDevice * device, int channelNumber);
    virtual void OsdClear();
    virtual void OsdStatusMessage(const char *message);

  public:
    cOsdPipObject(cDevice * Device, const cChannel * Channel);
    virtual ~ cOsdPipObject();
    virtual void Show();
    void ProcessImage(unsigned char *data, int length);
    eOSState ProcessKey(eKeys k);
    virtual bool NoAutoClose(void) { return true; }
#if VDRVERSNUM >= 10716
    static int CurrentPipChannel;
#endif
    static int NextPipChannel;
};

#endif // VDR_OSDPIP_OSD_H
