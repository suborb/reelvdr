/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef VDR_OSDPIP_RECEIVER_H
#define VDR_OSDPIP_RECEIVER_H

#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <vdr/config.h>
#include "player.h"

class cRingBufferLinear;
class cPlayer;
class cRemux;
class cPipPlayer;

class cOsdPipReceiver:public cReceiver, public cThread
{
  private:
#if VDRVERSNUM < 10716
    cRingBufferLinear * m_TSBuffer;
    cRemux *m_Remux;
#else
    cTsToPes tsToPesVideo;
#endif
    cPipPlayer *player;
    const cChannel *channel;

    bool m_Active;
    int PlayPipPesPacket(const uchar * Data, int Length);
    int PlayPipPes(const uchar * Data, int Length);

  protected:
      virtual void Activate(bool On);
    virtual void Receive(uchar * Data, int Length);
    virtual void Action(void);

  public:
    //cOsdPipReceiver::cOsdPipReceiver(const cChannel *Channel,int VPid, const int *APids, const int *DPids, const int *SPids);
      cOsdPipReceiver(const cChannel * Channel, cPipPlayer * player);
      virtual ~ cOsdPipReceiver();
};

#endif // VDR_OSDPIP_RECEIVER_H
