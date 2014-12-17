#include <stdio.h>
#include <vdr/ringbuffer.h>
#include "control.h"
#include "InputThread.h"
#include <vdr/thread.h>

#define PTS 1
#define WRITE_MPA_FRAMES 0
#define LOG_PTS 0
#define LOG_BAD_CHUNKS 0

#define BUF_SIZE 4096
#define HEADER_LEN 128
#define RING_BUF_SIZE 102400

cString mrl;
#include "mpa.c"


cMyPlayer::cMyPlayer()
{
    closePlayer = false;
}
cMyPlayer::~cMyPlayer()
{
    closePlayer = true;
    StopPlayback();
    printf("~cMyPlayer()\n");
}

void cMyPlayer::Activate(bool On)
{
    printf("Called Activate(%i)\n", On);
    if(On)
    {
        if(!Running())
            Start();
    }
    else
    {
        Cancel(-1);
    }
}

void cMyPlayer::StopPlayback()
{
    printf("StopPlayback: waiting for 9 secs before cancelling\n");
    Cancel(9);
    Detach();
}

int AddPesAudioHeader(uchar*, size_t, uint64_t);

int fw;

#if WRITE_MPA_FRAMES
int f_mpa;
#endif

#if LOG_BAD_CHUNKS
int f_bad;
#endif

#if PTS && LOG_PTS
FILE* f_pts;
#endif

#define PES_LEN 4096
void cMyPlayer::Action()
{
    printf("Action() Started\n");
    int len = 0;
    uchar *Data = NULL; //new uchar[BUF_SIZE];
    cPoller Poll;

    fw = open("/tmp/pespackets", O_CREAT|O_WRONLY);

#if WRITE_MPA_FRAMES
    f_mpa = open("/tmp/mpa_frames", O_CREAT|O_WRONLY);
#endif

#if LOG_BAD_CHUNKS
    f_bad = open("/tmp/bad_chunks", O_CREAT|O_WRONLY);
#endif

#if PTS && LOG_PTS
    f_pts = fopen("/tmp/pts", "w");
#endif

    cRingBufferLinear ringbuffer(RING_BUF_SIZE,5000);
    cMutex bufferMutex;

    cInputThread ReadThread(*mrl, &ringbuffer, &bufferMutex);
    ReadThread.Start();

    float duration = 0.0;
    uint64_t pts;

    if(!DeviceSetAvailableTrack(ttAudio, 0 ,0xC0))
    {
        printf("(%s:%i) Setting Audio tracks failed.\n", __FILE__, __LINE__);
    }
    DeviceSetCurrentAudioTrack(ttAudio);


    while(Running())
    {
        //check if device is ready
        if(DevicePoll(Poll,100))
        {
            bufferMutex.Lock();
            Data = ringbuffer.Get(len);
            bufferMutex.Unlock();

            if(!Data || len <= 0) 
            {
                len=0; 

                if(!ReadThread.IsActiveThread()) 
                {
                    printf("Read-Thread not active and 0 bytes got from ringbuffer. Exiting.\n");
                    break;
                }

                /* no data got from buffer
                   Nothing to do in the rest of the loop 
                   take a quick nap and try again
                   */
                cCondWait::SleepMs(5);
                continue;
            }

#if PTS
            pts = (uint64_t) (duration*90000UL) ; // 90KHz clock
#else
            pts  = 0;
#endif

            //Play as pes packets
            int last_valid_frame_ending = PlayAsPesPackets(Data, len, pts, duration);

            if(last_valid_frame_ending)
            {
                // delete the used bytes from the buffer
                bufferMutex.Lock();
                ringbuffer.Del(last_valid_frame_ending);
                bufferMutex.Unlock();
            }
            else
            {
                printf("No frame found in %i bytes : %i available\n",len, ringbuffer.Available());
                if(len>2000)
                {

#if LOG_BAD_CHUNKS
                    if(f_bad>=0)
                    {
                        write(f_bad, Data, len);
                    }

#endif
                    bufferMutex.Lock();
                    ringbuffer.Del(len);
                    bufferMutex.Unlock();
                }
                //cCondWait::SleepMs(10);
            }
            last_valid_frame_ending = 0;
        }//DevicePoll()
    }//while Running()

    printf("Calling ReadThread.StopThread()\n");
    ReadThread.StopThread();

    if(fw>=0)
        close(fw);

#if WRITE_MPA_FRAMS
        if(f_mpa>=0)
            close(f_mpa);
#endif

#if LOG_BAD_CHUNKS
    if(f_bad>=0)
        close(f_bad);
#endif

#if PTS && LOG_PTS
            if(f_pts)
                fclose(f_pts);
#endif
            

            // wait for the device to finish playing
            if (Running())
            {
                printf("Waiting for device to finish playing\n");
                while(!DeviceFlush(100)); 
            }

            printf("Action() complete\n");
            closePlayer = true;
}



int cMyPlayer::PlayAsPesPackets(uchar* Data, int len, uint64_t pts, float& duration)
{
    mpa_frame_t frame;
    uchar pesPacket[PES_LEN];

    int payload_i = AddPesAudioHeader(pesPacket, PES_LEN, pts);

    int i = 0,j = 0;
    int last_valid_frame_ending = 0;
    int payload_len = 0;

    // find all valid mpa frames and copy into the pes packet
    while(i<len && Data&& Running())
    {
        j = find_valid_mpa_frame(Data+i, len-i, &frame);

        if(j>=0) //check for min. packet length?
        {
            //copy to pes packet
            if(frame.len>0)
            {
                if(payload_len + frame.len > PES_LEN) // more than what pesPacket can hold
                    break;

                i += j;

                if(i+(int)frame.len>len) // not enough data in buffer to complete this frame
                    break;

#if WRITE_MPA_FRAMES
                if(f_mpa>=0)
                    write(f_mpa, Data+i, frame.len);
#endif

                //copy one mpa frame
                memcpy(pesPacket+payload_i, Data+i, frame.len);

                payload_i += frame.len;

                i += frame.len;
                last_valid_frame_ending = i;
                payload_len += frame.len;

                duration += frame.duration;
            }
            else
                i++;
        }
        else
            // no more valid frames
            break;

    } // while

    if(last_valid_frame_ending)
    {
        int pes_len = payload_i - 6;
        pesPacket[4] = (pes_len >> 8) & 0xff;
        pesPacket[5] = pes_len & 0xff;

        if (fw>=0)
            write(fw, pesPacket, payload_i);
#if LOG_PTS
        fprintf(f_pts,"%09llx\n", pts);
        printf("pts:%09llx\n",pts);
#endif
        //int ret = PlayPes(pesPacket, payload_i, false);
        PlayPes(pesPacket, payload_i, false);
        cCondWait::SleepMs(3);
    }

    return last_valid_frame_ending;
}

/// cMyControl class -----------------------------
cMyControl::cMyControl(): cControl(player=new cMyPlayer)
{
}

eOSState cMyControl::ProcessKey(eKeys key)
{
    if(!player || player && player->ClosePlayer())
    {
        printf("closing player !\n");
        return osEnd;
    }

    switch (key)
    {
        case kBack:
        case kStop:
            if(player) player->StopPlayback();
            printf("exiting cMyControl\n");
            return osEnd;

        default:
            return cControl::ProcessKey(key);
    }
}

void cMyControl::Hide()
{
}

cMyControl::~cMyControl()
{
    delete player;
    printf("~cMyControl()\n");
}
