/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/reelboxbase.h>
#include <vdr/dvbdevice.h>
#include <vdr/tools.h>
#include "player.h"

#if APIVERSNUM < 10716
#define IS_HD_CHANNEL(c)              (c->IsSat() && (c->Modulation() == 10))
#else
#define IS_HD_CHANNEL(c)              ( (c->Vpid() != 0) && c->IsSat() && (cDvbTransponderParameters(c->Parameters()).System() == SYS_DVBS2 || strstr(c->Name(), " HD")))
#endif

// PiP available only for SD video channels
// skip audio and HD channels
#define CAN_DISPLAY_CHANNEL_IN_PIP(c) ((c->Vpid() != 0) &&  !IS_HD_CHANNEL(c))

cPipPlayer::cPipPlayer()
{
    device = cReelBoxBase::Instance();
    pesAssembler = new cPesAssembler;
}

int
cPipPlayer::PlayPipPesPacket(int type, const uchar * Data, int Length)
{
    uchar c = Data[3];
    switch (c)
    {
    case 0xBE:                 // padding stream, needed for MPEG1
    case 0xE0 ... 0xEF:          // video 
        //printf("vor  device->PlayPipVideo, device = %u\n", (uint)device);
#if APIVERSNUM >= 10700
        if(device->SetPipVideoType(type));
#endif
            device->PlayPipVideo(Data, Length);
        //printf("nach  device->PlayPipVideo\n");
        break;
    default:
        ;                       //esyslog("ERROR: unexpected packet id %02X", c);
    }
    return Length;
}


int
cPipPlayer::PlayPipPes(int type, const uchar * Data, int Length)
{
    //printf("----------cPipPlayer::PlayPipPes---------\n");

    if (!Data)
    {
        pesAssembler->Reset();
        return 0;
    }
    int Result = 0;
    if (pesAssembler->Length())
    {
        // Make sure we have a complete PES header:
        while (pesAssembler->Length() < 6 && Length > 0)
        {
            pesAssembler->Put(*Data++);
            Length--;
            Result++;
        }
        if (pesAssembler->Length() < 6)
            return Result;      // Still no complete PES header - wait for more
        int l = pesAssembler->ExpectedLength();
        int Rest = min(l - pesAssembler->Length(), Length);
        pesAssembler->Put(Data, Rest);
        Data += Rest;
        Length -= Rest;
        Result += Rest;
        if (pesAssembler->Length() < l)
            return Result;      // Still no complete PES packet - wait for more
        // Now pesAssembler contains one complete PES packet. 
        //printf("-----cPipPlayer::PlayPipPes, vor PlayPipPesPacket-----\n");
        int w =
            PlayPipPesPacket(type, pesAssembler->Data(), pesAssembler->Length());
        if (w > 0)
            pesAssembler->Reset();
        return Result > 0 ? Result : w < 0 ? w : 0;
    }
    int i = 0;
    while (i <= Length - 6)
    {
        if (Data[i] == 0x00 && Data[i + 1] == 0x00 && Data[i + 2] == 0x01)
        {
            int l = cPesAssembler::PacketSize(&Data[i]);
            if (i + l > Length)
            {
                // Store incomplete PES packet for later completion:
                pesAssembler->Put(Data + i, Length - i);
                return Length;
            }
            int w = PlayPipPesPacket(type, Data + i, l);
            if (w > 0)
                i += l;
            else
                return i == 0 ? w : i;
        }
        else
            i++;
    }
    if (i < Length)
        pesAssembler->Put(Data + i, Length - i);
    return Length;
}

void
cPipPlayer::SetPipDimensions(const uint x, const uint y, const uint width,
                             const uint height)
{
    device->SetPipDimensions(x, y, width, height);
}

bool
cPipPlayer::IsPipChannel(const cChannel *channel)
{
    bool ret=false;
#if APIVERSNUM >= 10700
    if(device->IsPipChannel(channel, &ret)) return ret;
#endif
    return CAN_DISPLAY_CHANNEL_IN_PIP(channel);
}

