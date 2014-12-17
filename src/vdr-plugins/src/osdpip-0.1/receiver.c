/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#include "receiver.h"
#include "setup.h"
#include "player.h"

#include <vdr/channels.h>
#include <vdr/remux.h>
#include <vdr/ringbuffer.h>
#include <vdr/reelboxbase.h>
#include <vdr/config.h>

cOsdPipReceiver::cOsdPipReceiver(const cChannel * Channel,
                                 cPipPlayer * pipplayer):
#if VDRVERSNUM < 10716
cReceiver(Channel->Ca(), 0, Channel->Vpid()),
#else
cReceiver(Channel->GetChannelID(), 0, Channel->Vpid()),
#endif
player(pipplayer),
channel(Channel)
{
#if VDRVERSNUM < 10716
    m_Remux = new cRemux(Channel->Vpid(), 0, NULL, NULL, NULL, true);
    m_Remux->SetTimeouts(0, 0);
#endif
}

cOsdPipReceiver::~cOsdPipReceiver()
{
    cReceiver::Detach();
#if VDRVERSNUM < 10716
    delete m_Remux;
#endif
}

void
cOsdPipReceiver::Activate(bool On)
{
#if 0 /* starting and stopping PiP() destroys SWDecoder in reelbox plugin
      and so it loose the height of prev pip window ==> bug #470*/
if (On)
    {
        //printf("------------------StartPip------------------------\n");
        if(cReelBoxBase::Instance()) cReelBoxBase::Instance()->StartPip(true);
        else esyslog("cReelBoxBase::Instance not set!");
    }
    else
    {
        //printf("------------------EndPip------------------------\n");
        if(cReelBoxBase::Instance()) cReelBoxBase::Instance()->StartPip(false);
        else esyslog("cReelBoxBase::Instance not set!");
    }
#endif
}

void
cOsdPipReceiver::Receive(uchar * Data, int Length)
{
#if VDRVERSNUM < 10716
    //printf("-------------------cOsdPipReceiver::Receive---------------------\n");
    uchar *p = NULL;
    int Result = 0;
    /*int Count = */ m_Remux->Put(Data, Length);

    while (true)
    {
        p = m_Remux->Get(Result);
        if (Result <= 0)
            break;
        while (p != 0)
        {
            int w = player->PlayPipPes((SF_H264 == m_Remux->SFmode()) ? 0x1B : 0x02, p, Result);
            if (w > 0)
            {
                p += w;
                Result -= w;
                m_Remux->Del(w);
                if (Result <= 0)
                    p = NULL;
            }
        }
    }
#else
    if (TsPayloadStart(Data)) {
        int l;
        while (const uchar *p = tsToPesVideo.GetPes(l)) {
            int w = player->PlayPipPes(channel->Vtype(), p, l);
            if (w <= 0) {
                tsToPesVideo.SetRepeatLast();
                return;
            } // if
        } // while
        tsToPesVideo.Reset();
    } // if
    tsToPesVideo.PutTs(Data, Length);
#endif
}

void
cOsdPipReceiver::Action(void)
{
}
