/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
*                                                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
/*
>Are we the only 2 people with this problem (Morfsta and myself)? 
Don't think so...
>Schorsch reports that BBC HD appears ok on his system.
I can read ;)
>Would that narrow down the problem?
No, because he has not responded to my post.
>For me this is a serious problem which has significantly dented my appreciation of the Avantgarde
Well, thanks to Morfsta, who gave me a whole transponder recording, I can say that BBC HD is ignoring the specification.
They are sending AC3 data with a PES id reserved for MPA.
*/

#include "rbmini.h"

#include <sys/ioctl.h>
#include <math.h>

#define DBG_FN   DBG_NONE
//#define DBG_FN   DBG_STDOUT
//#define DBG_FN   DBG_SYSLOG

// Disabled due to problems displaying jpegs after switching to radio channels
//#define SHOW_BGPIC

cRBMBgPic::cRBMBgPic(cRBMDevice *fDevice) : cThread("RBMBgPic") {
	device  = fDevice;
	nextPic = 0;
} // cRBMBgPic::cRBMBgPic

void cRBMBgPic::Action() {
    char filename[512];
    nextPic++;
    ::sprintf(filename, PIC_IMAGE_DIR, nextPic);
    FILE *file = ::fopen(filename, "rb");
    if(file) {
        struct stat filestat;
        ::stat(filename, &filestat);
        if(filestat.st_size > 0) {
            Buffer.resize(filestat.st_size);
            if(filestat.st_size == (off_t)::fread(&Buffer[0], 1, filestat.st_size, file)) {
//                Length = filestat.st_size;
                if(device && !device->HasVideo()) device->StillPicture(&Buffer[0], filestat.st_size);
printf("cRBMBgPic::Should show file %s %lld\n", filename, filestat.st_size);
            } // if
        } // if
        ::fclose(file);
    } else if (nextPic != 1) {
        nextPic = 1;
        Action();
    } // if
} // cRBMBgPic::Action

/**************************************************************************/

bool cRBMDevice::Init() {
    rbm_dmx_mode dmx = RBM_DMX_MODE_NONE;
#ifdef ALWAYS_TS
    dmx_mode = RBM_DMX_MODE_TS;
#endif
    if(main_fd != -1) return true;
    DBG(DBG_FN,"cRBMDevice::Init");
    main_fd = open(MAIN_DEVICE_NAME, O_RDWR|O_NDELAY);
    if(main_fd == -1) {
        printf("[RBM-DEV] cant open main %s\n", MAIN_DEVICE_NAME);
        goto error;
    } // if
    if((ioctl(main_fd, RBMIO_GET_DMX_MODE, &dmx) != 0) || memcmp(&dmx_mode, &dmx, sizeof(dmx))) {
        printf("Demux type changed: %ld->%ld\n", dmx, dmx_mode);
        if(ioctl(main_fd, RBMIO_SET_DMX_MODE, &dmx_mode) != 0)
            goto error;
    } else {
        ioctl(main_fd, RBMIO_STOP, NULL);
    } // if
    Buffer = new uchar[MAX_BUFFER];
    BufferPos=0;
    IFrameTrick=false;
    last_audio_pts = last_video_pts = -1;
    return true;
error:
    Done();
    return false;
} // cRBMDevice::Init

void cRBMDevice::Done() {
    if(Buffer) delete[] Buffer;
    Buffer=NULL;
    if(main_fd != -1) close (main_fd);
    main_fd = -1;
} // cRBMDevice::Done

bool cRBMDevice::CheckMode() {
    if(!Init()) return false;
    rbm_state state = RBM_STATE_NONE;
    if(ioctl(main_fd, RBMIO_GET_STATE, &state) != 0) return false;
    rbm_dmx_mode dmx = RBM_DMX_MODE_NONE;
    if(ioctl(main_fd, RBMIO_GET_DMX_MODE, &dmx) != 0) return false;
    if((state < RBM_STATE_INIT) || memcmp(&dmx_mode, &dmx, sizeof(dmx))) {
        printf("Demux type changed: %ld->%ld\n", dmx, dmx_mode);
        if(ioctl(main_fd, RBMIO_SET_DMX_MODE, &dmx_mode) != 0) {
            return false;
        }
        cDevice::Clear();
        state = RBM_STATE_INIT;
    } // if
    eTrackType Type = GetCurrentAudioTrack();
    u_int32 format = IS_AUDIO_TRACK(Type) ? RBM_AUDIO_FORMAT_MPA : IS_DOLBY_TRACK(Type) ? RBM_AUDIO_FORMAT_AC3 : RBM_AUDIO_FORMAT_NONE;
    if((RBM_AUDIO_FORMAT_MPA == format) && MP3Detected)
        format = RBM_AUDIO_FORMAT_MP3;

    u_int32 pid    = 0;
    const tTrackId *lTrack = GetTrack(Type);
    if(lTrack) pid = lTrack->id;
    if(!pid) format = RBM_AUDIO_FORMAT_NONE;
    if((av_mode.uAudioFormat != format) || (av_mode.uAudioPid != pid))
        printf("Audio changed: pid %03lx->%03lx format %s->%s\n", av_mode.uAudioPid, pid, RBM_AUDIO_FORMAT(av_mode.uAudioFormat), RBM_AUDIO_FORMAT(format));
    av_mode.uAudioFormat = pid ? format : RBM_AUDIO_FORMAT_NONE;
    av_mode.uAudioPid    = pid;
    rbm_av_mode new_av = av_mode;
    if(new_av.uVideoPid == 0 || new_av.uVideoPid == 1 || new_av.uVideoPid >= 0x1FFF)  {// Radio mode (assume pcr = audio pid and stillframes to be mpeg2)
        new_av.uVideoPid    = 0;
        new_av.uPCRPid      = 0;//pid;
        new_av.uStillFormat = new_av.uVideoFormat = RBM_VIDEO_FORMAT_MPEG2;
    } // if
    if(!new_av.uPCRPid)
        new_av.uPCRPid = av_mode.uAudioPid;
    if(new_av.uPCRPid) {
        rbm_av_mode av;
        if(ioctl(main_fd, RBMIO_GET_AV_MODE, &av) != 0) return false;
        if((state < RBM_STATE_RUN) || memcmp(&new_av, &av, sizeof(av))) {
            printf("Audio/Video mode changed p 0x%lx a 0x%lx %s v 0x%lx %s\n", new_av.uPCRPid, new_av.uAudioPid, RBM_AUDIO_FORMAT(new_av.uAudioFormat), new_av.uVideoPid, RBM_VIDEO_FORMAT(new_av.uVideoFormat));
            if(ioctl(main_fd, RBMIO_SET_AV_MODE, &new_av) != 0) {
                return false;
            }
            state = RBM_STATE_RUN;
        } // if
        if(!new_av.uVideoPid) {
            int now = ::time(0);
            if(now >= lastPicTime + PIC_INTERVAL) {
                lastPicTime = now;
#ifdef SHOW_BGPIC
                bgPic.Start();
#endif
            } // if
        } // if
    } // if
    return true;
} // cRBMDevice::CheckMode

void cRBMDevice::CheckPATPMT(uchar *PATPMT) {
    if(!PATPMT)
        return;
    printf("cRBMDevice::CheckPATPMT\n");
    u_int32 vpid    = 0;
    u_int32 vformat = RBM_VIDEO_FORMAT_NONE;
    int mpapos = 0;
    int ac3pos = 0;
    int length = ((PATPMT[TS_SIZE+6]&0xF)<<8)|(PATPMT[TS_SIZE+7]&0xFF);
    unsigned char *pos = &PATPMT[TS_SIZE+8];
    length-=5; pos +=5; // skip to pcr pid
    u_int32 ppid = ((pos[0]&0x1F)<<8)|(pos[1]&0xFF);
    length-=2; pos +=2; 
    int size = ((pos[0]&0xF)<<8)|(pos[1]&0xFF);
    length-=2+size; pos+=2+size; // skip program info 
    ClrAvailableTracks();
    while(length > 4) {
        char *descr = NULL;
        int pid = ((pos[1]&0x1F)<<8)|(pos[2]&0xFF);
        size = ((pos[3]&0xF)<<8)|(pos[4]&0xFF);
        if(size > 1)
            printf("Descriptor: %x for pid %03x len %d %02x %02x %02x %02x\n", pos[5], pid, pos[6], pos[7], pos[8], pos[9], pos[10]);
        switch(*pos) {
            case 0x02: 
                vpid    = pid;
                if(vpid == 0 || vpid == 1 || vpid == 0x1FFF) // Radio
                    vpid = 0;
                else
                    vformat = RBM_VIDEO_FORMAT_MPEG2;
                printf("vid %03lx (%03x)\n", vpid, pid);
                break;
            case 0x04: 
                printf("mpa %d %03x\n", mpapos, pid);
                SetAvailableTrack(ttAudio, mpapos++, pid, descr);
                break;
            case 0x06: 
                printf("ac3 %d %03x\n", mpapos, pid);
                SetAvailableTrack(ttDolby, ac3pos++, pid, descr);
                break;
            case 0x1b: 
                vpid    = pid;
                vformat = RBM_VIDEO_FORMAT_H264;
                break;
        } // switch
        length-=5+size; pos+=5+size;
    } // while
    if(GetCurrentAudioTrack() == ttNone)
        EnsureAudioTrack();
    EnsureSubtitleTrack();
    dmx_mode             = RBM_DMX_MODE_TS;
    if((av_mode.uPCRPid != ppid) || (av_mode.uVideoPid != vpid) || ( av_mode.uVideoFormat != vformat))
        printf("Video changed: pcr %03lx->%03lx video: %03lx->%03lx format %s->%s\n", av_mode.uPCRPid, ppid, av_mode.uVideoPid, vpid, RBM_VIDEO_FORMAT(av_mode.uVideoFormat), RBM_VIDEO_FORMAT(vformat));
    av_mode.uPCRPid      = ppid;
    av_mode.uVideoPid    = vpid;
    av_mode.uStillFormat = av_mode.uVideoFormat = vformat;
    return;
} // cRBMDevice::CheckPATPMT

void cRBMDevice::CheckPatPmt() {
    const cPatPmtParser *p = PatPmtParser();
    if(!p) return;
    u_int32 vformat = RBM_VIDEO_FORMAT_NONE;
    switch (p->Vtype()) {
        case 0x02: vformat = RBM_VIDEO_FORMAT_MPEG2; break;
        case 0x1b: vformat = RBM_VIDEO_FORMAT_H264; break;
    } // switch
    dmx_mode             = RBM_DMX_MODE_TS;
    if((av_mode.uPCRPid != (u_int32)p->Ppid()) || (av_mode.uVideoPid != (u_int32)p->Vpid()) || ( av_mode.uVideoFormat != vformat))
        printf("Video changed: pcr %03lx->%03x video: %03lx->%03x format %s->%s\n", av_mode.uPCRPid, p->Ppid(), av_mode.uVideoPid, p->Vpid(), RBM_VIDEO_FORMAT(av_mode.uVideoFormat), RBM_VIDEO_FORMAT(vformat));
    av_mode.uPCRPid      = p->Ppid();
    av_mode.uVideoPid    = p->Vpid();
    av_mode.uStillFormat = av_mode.uVideoFormat = vformat;
} // cRBMDevice::CheckPatPmt

/**************************************************************************/

cRBMDevice::cRBMDevice(cPluginRBM *Plugin) : cDevice(), bgPic(this),
                           plugin(Plugin), main_fd(-1) {
    DBG(DBG_FN,"cRBMDevice::cRBMDevice");
    memset(&dmx_mode, 0, sizeof(dmx_mode));
    memset(&av_mode,  0, sizeof(av_mode));
    memset(&tv_mode,  0, sizeof(tv_mode));
    Init();
    if(GetTvMode(&tv_mode))
        OutputEnable = ((tv_mode.uHDOutputMode != RBM_OUTPUT_MODE_NONE) || (tv_mode.uSDOutputMode != RBM_OUTPUT_MODE_NONE));
    else
        OutputEnable = true;
#ifdef TRIGGER_AR_ON_SET_VIDEO_FORMAT
    SkipSetVideoFormat=1; // We expect one initial SetVideoFormat call
#endif
#if VDRVERSNUM >= 10716
    lastWrite=0;
#endif
} // cRBMDevice::cRBMDevice

cRBMDevice::~cRBMDevice() {
    DBG(DBG_FN,"cRBMDevice::~cRBMDevice");
    while(bgPic.Active())
        cCondWait::SleepMs(10);
    SetOutput(false);
    Done();
    DBG(DBG_FN,"cRBMDevice::~cRBMDevice DONE");
} // cRBMDevice::~cRBMDevice

///< Tells whether this device has an MPEG decoder.
bool cRBMDevice::HasDecoder(void) const { 
    return true;
} // cRBMDevice::HasDecoder

///< Gets the current System Time Counter, which can be used to
///< synchronize audio and video. If this device is unable to
///< provide the STC, -1 will be returned.
int64_t cRBMDevice::GetSTC(void) { 
//    DBG(DBG_FN,"cRBMDevice::GetSTC");
    int64_t stc = -1;
    rbm_stc rbm_stc;
    memset(&stc, 0, sizeof(stc));
    int ret = ioctl(main_fd, RBMIO_GET_STC, &rbm_stc);
    if(!ret) stc = (((int64_t)rbm_stc.high)<<32)|rbm_stc.low;
//    DBG(DBG_FN,"cRBMDevice::GetSTC %d %llx (%llx %llx) -> %llx", ret, stc, last_audio_pts, last_video_pts, (-1==stc)?(-1==last_audio_pts)?last_video_pts:last_audio_pts:stc);
    if(-1 == stc) stc = last_audio_pts;
    if(-1 == stc) stc = last_video_pts;
    return stc;
} // cRBMDevice::GetSTC

///< Returns true if this device can currently start a replay session.
bool cRBMDevice::CanReplay(void) const {
//  DBG(DBG_FN,"cRBMDevice::CanReplay");
    return cDevice::CanReplay();
} // cRBMDevice::CanReplay

///< Sets the device into a mode where replay is done slower.
///< Every single frame shall then be displayed the given number of
///< times.
void cRBMDevice::TrickSpeed(int Speed) {
//    DBG(DBG_FN,"cRBMDevice::TrickSpeed %d", Speed);
    if(!Init()) return;
    cMutexLock lock(&dataMutex);
    rbm_av_trick trick = Speed;
    av_mode.uStillFormat = av_mode.uVideoFormat; // make sure still format for i-frame trick has the same format
    IFrameTrick = Speed <= 0;
    if(IFrameTrick) {
//Not needed        Clear();
        trick = 100|RBMIO_TRICK_I_FRAME;
    } // if
    int ret = ioctl(main_fd, RBMIO_TRICK, &trick);
    DBG(DBG_FN,"cRBMDevice::TrickSpeed %d -> %s %lx %d", Speed, IFrameTrick ? "i-Frame" : "Smooth", trick, ret);
    cDevice::PlayPes(NULL, 0, false); // Clear pes buffer
    cDevice::TrickSpeed(Speed);
} // cRBMDevice::TrickSpeed

///< Clears all video and audio data from the device.
///< A derived class must call the base class function to make sure
///< all registered cAudio objects are notified.
void cRBMDevice::Clear(void) {
    DBG(DBG_FN,"cRBMDevice::Clear");
    if(!Init()) return;
    cMutexLock lock(&dataMutex);
    rbm_av_flush flush;
    memset(&flush, 0, sizeof(flush));
    flush.mode = RBMIO_FLUSH_CLEAR;
    int ret = ioctl(main_fd, RBMIO_FLUSH, &flush);
    DBG(DBG_FN,"cRBMDevice::Clear %d", ret);
    BufferPos = 0;
    cDevice::Clear();
} // cRBMDevice::Clear

///< Sets the device into play mode (after a previous trick mode).
void cRBMDevice::Play(void) {
    DBG(DBG_FN,"cRBMDevice::Play");
    if(!Init()) return;
    cMutexLock lock(&dataMutex);
    MP3Detected = IFrameTrick = false;
    BufferPos = 0;
    int ret = ioctl(main_fd, RBMIO_RESUME, NULL);
    DBG(DBG_FN,"cRBMDevice::Play %d", ret);
    cDevice::Play();
} // cRBMDevice::Play

void cRBMDevice::SetVolumeDevice(int Volume) {
    DBG(DBG_FN,"cRBMDevice::SetVolumeDevice %d", Volume);
    if(!Init()) return;
    if(Volume > 255) Volume = 255;
    if(Volume < 0  ) Volume = 0;
    rbm_audio_volume volume = Volume ? (rbm_audio_volume)(log10(((double)Volume)/255)*30000) : RBM_AUDIO_VOLUME_MIN;
    int ret = ioctl(main_fd, RBMIO_SET_AUDIO_VOLUME, &volume);
    DBG(DBG_FN,"cRBMDevice::SetVolumeDevice %d (%ld)", ret, volume);
    cDevice::SetVolumeDevice(Volume);
} // SetVolumeDevice

///< Displays the given I-frame as a still picture.
void cRBMDevice::StillPicture(const uchar *Data, int Length) {
    if (Length > (188*2) && Data[0] == 0x47 && Data[188] == 0x47 && Data[188*2] == 0x47) {
        // TS data
        uchar *buf = MALLOC(uchar, Length);
        if(!buf) return;
        int i = 0;
        int blen = 0;
        bool first = true;
        for(i=0; i <= (Length-188); i+=188) {
            if(((((u_int32)Data[i+1]&0x1F)<<8)|Data[i+2]) != av_mode.uVideoPid) // No interest
                continue;
            if(!(Data[i+3] & 0x10)) // No payload
                continue;
            if(!(Data[i+1] & 0x40) && first) // start with pes header
                continue;
            first=false;
            int data_pos = 4;
            if(Data[i+3] & 0x20)
                data_pos += Data[i+4]+1;
            if((Data[i+1] & 0x40) && !Data[i+data_pos] && !Data[i+data_pos+1] && (Data[i+data_pos+2]==1)) // pes header
                data_pos += 9+Data[i+data_pos+8];
            if(188 > data_pos) {
                memcpy(&buf[blen], &Data[i+data_pos], 188-data_pos);
                blen += 188-data_pos;
            } // if
        } // for
        DBG(DBG_FN,"cRBMDevice::StillPicture prepare TS (%d %d)", Length, blen);
        StillPicture(buf, blen);
        free(buf);
    } else if (Length >= 4 && Data[0] == 0x00 && Data[1] == 0x00 && Data[2] == 0x01 && (Data[3] & 0xF0) == 0xE0) {
        // PES data
        uchar *buf = MALLOC(uchar, Length);
        if (!buf) return;
        int i = 0;
        int blen = 0;
        while (i < Length - 6) {
            if (Data[i] == 0x00 && Data[i + 1] == 0x00 && Data[i + 2] == 0x01) {
                int len = Data[i + 4] * 256 + Data[i + 5];
                if ((Data[i + 3] & 0xF0) == 0xE0) { // video packet
                    // skip PES header
                    int offs = i + 6;
                    // skip header extension
                    if ((Data[i + 6] & 0xC0) == 0x80) {
                        // MPEG-2 PES header
                        if (Data[i + 8] >= Length) break;
                        offs += 3;
                        offs += Data[i + 8];
                        len -= 3;
                        len -= Data[i + 8];
                        if (len < 0 || offs + len > Length) break;
                    } else {
                        // MPEG-1 PES header
                        while (offs < Length && len > 0 && Data[offs] == 0xFF) {
                                offs++;
                                len--;
                        } // while
                        if (offs <= Length - 2 && len >= 2 && (Data[offs] & 0xC0) == 0x40) {
                            offs += 2;
                            len -= 2;
                        } // if
                        if (offs <= Length - 5 && len >= 5 && (Data[offs] & 0xF0) == 0x20) {
                            offs += 5;
                            len -= 5;
                        } else if (offs <= Length - 10 && len >= 10 && (Data[offs] & 0xF0) == 0x30) {
                            offs += 10;
                            len -= 10;
                        } else if (offs < Length && len > 0) {
                            offs++;
                            len--;
                        } // if
                    } // if
                    if (blen + len > Length) break;// invalid PES length field
                    memcpy(&buf[blen], &Data[offs], len);
                    i = offs + len;
                    blen += len;
                } else if (Data[i + 3] >= 0xBD && Data[i + 3] <= 0xDF) // other PES packets
                    i += len + 6;
                else
                    i++;
            } else
                i++;
        } // while
        if((blen+4 <= Length) && (RBM_VIDEO_FORMAT_MPEG2 == av_mode.uStillFormat)) {
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x01;
            buf[blen++] = 0xB7;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            buf[blen++] = 0x00;
            while((blen%4) && (blen < Length)){
                buf[blen++] = 0x00;
            } // while
        } // if
        DBG(DBG_FN,"cRBMDevice::StillPicture prepare PES (%d %d)", Length, blen);
        StillPicture(buf, blen);
        free(buf);
    } else {
        if(Length >= 2 && Data[0] == 0xFF && Data[1] == 0xD8) {
           av_mode.uStillFormat = RBM_VIDEO_FORMAT_JPEG;
           if(ioctl(main_fd, RBMIO_SET_AV_MODE, &av_mode) != 0) return;
        } // if
        rbm_av_still still;
        still.uSize = Length;
        still.pData = (char *)Data;
        int ret = ioctl(main_fd, RBMIO_STILL, &still);
        DBG(DBG_FN,"cRBMDevice::StillPicture ES %d (%d %ld) Format still %s motion %s", ret, Length, still.uSize, RBM_VIDEO_FORMAT(av_mode.uStillFormat), RBM_VIDEO_FORMAT(av_mode.uVideoFormat));
    } // if
} // cRBMDevice::StillPicture

///< Puts the device into "freeze frame" mode.
void cRBMDevice::Freeze(void) {
    DBG(DBG_FN,"cRBMDevice::Freeze");
    if(!Init()) return;
    int ret = ioctl(main_fd, RBMIO_PAUSE, NULL);
    DBG(DBG_FN,"cRBMDevice::Freeze %d", ret);
    cDevice::Freeze();
} // cRBMDevice::Freeze

///< Sets the device into the given play mode.
///< \return true if the operation was successful.
bool cRBMDevice::SetPlayMode(ePlayMode PlayMode) {
    DBG(DBG_FN,"cRBMDevice::SetPlayMode %d", PlayMode);
    if(pmExtern_THIS_SHOULD_BE_AVOIDED==PlayMode) {
        Done();
        return true;
    } // if
    if(!Init()) return false;
    MP3Detected=IFrameTrick=false;
    int ret = ioctl(main_fd, RBMIO_STOP, NULL);
    DBG(DBG_FN,"cRBMDevice::SetPlayMode ioctl RBMIO_STOP %d", ret);
    last_audio_pts = last_video_pts = -1;
    memset(&av_mode, 0, sizeof(av_mode));
    av_mode.uStillFormat = RBM_VIDEO_FORMAT_MPEG2;
#ifdef ALWAYS_TS
    dmx_mode             = RBM_DMX_MODE_TS;
    av_mode.uVideoFormat = RBM_VIDEO_FORMAT_NONE;
    av_mode.uVideoPid    = 0x0;
    av_mode.uPCRPid      = 0x0;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>ALWAYS_TS\n");
#else
    dmx_mode             = RBM_DMX_MODE_PES;
    av_mode.uVideoFormat = RBM_VIDEO_FORMAT_MPEG2;
    av_mode.uVideoPid    = 0xE0;
    av_mode.uPCRPid      = 0xE0;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>NOT ALWAYS_TS\n");
#endif
    switch(PlayMode) {
        case pmNone          : printf("pmNone\n"); break;
        case pmAudioVideo    : printf("pmAudioVideo\n"); break;
        case pmAudioOnlyBlack: printf("pmAudioOnlyBlack\n"); break;
        case pmAudioOnly     : printf("pmAudioOnly\n"); break;
        case pmVideoOnly     : printf("pmVideoOnly\n"); break;
        default: return false;
    } // switch
    lastPicTime = 0;//::time(0) + PIC_INTERVAL - PIC_DELAY;
    BufferPos=0;
    return true;
} // cRBMDevice::SetPlayMode

///< Sets the current audio track to the given value.
void cRBMDevice::SetAudioTrackDevice(eTrackType Type) {
    DBG(DBG_FN,"cRBMDevice::SetAudioTrackDevice %d", Type);
} // cRBMDevice::SetAudioTrackDevice

///< Sets the current subtitle track to the given value.
void cRBMDevice::SetSubtitleTrackDevice(eTrackType Type) {
    DBG(DBG_FN,"cRBMDevice::SetSubtitleTrackDevice %d", Type);
    cDevice::SetSubtitleTrackDevice(Type);
} // cRBMDevice::SetSubtitleTrackDevice

///< Sets the audio channel to stereo (0), mono left (1) or mono right (2).
void cRBMDevice::SetAudioChannelDevice(int audioChannel) {
    if(!Init()) return;
    u_int32 channel;
    switch(audioChannel) {
        case 0 : channel = RBM_AUDIO_CHANNEL_STEREO; break;
        case 1 : channel = RBM_AUDIO_CHANNEL_LEFT  ; break;
        case 2 : channel = RBM_AUDIO_CHANNEL_RIGHT ; break;
        default: return;
    } // switch
    int ret = ioctl(main_fd, RBMIO_SET_AUDIO_CHANNEL, &channel);
    DBG(DBG_FN,"cRBMDevice::SetAudioChannelDevice %d", ret);
} // cRBMDevice::SetAudioChannelDevice

///< Gets the current audio channel, which is stereo (0), mono left (1) or mono right (2).
int cRBMDevice::GetAudioChannelDevice() {
    if(!Init()) return 0;
    u_int32 lChannel = RBM_AUDIO_CHANNEL_STEREO;
    int ret = ioctl(main_fd, RBMIO_GET_AUDIO_CHANNEL, &lChannel);
    DBG(DBG_FN,"cRBMDevice::GetAudioChannelDevice %d", ret);
    switch(lChannel) {
        case RBM_AUDIO_CHANNEL_STEREO: return 0;
        case RBM_AUDIO_CHANNEL_LEFT  : return 1;
        case RBM_AUDIO_CHANNEL_RIGHT : return 2;
    } // switch
    return 0;
} // cRBMDevice::SetAudioChannelDevice

///< Tells the actual device that digital audio output shall be switched on or off.
void cRBMDevice::SetDigitalAudioDevice(bool On) {
    DBG(DBG_FN,"cRBMDevice::SetDigitalAudioDevice %d", On);
    cDevice::SetDigitalAudioDevice(On);
} // cRBMDevice::SetDigitalAudioDevice

///< Sets the video display format to the given one (only useful
///< if this device has an MPEG decoder).
///< A derived class must first call the base class function!
void cRBMDevice::SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat) {
    DBG(DBG_FN,"cRBMDevice::SetVideoDisplayFormat %d", VideoDisplayFormat);
    cDevice::SetVideoDisplayFormat(VideoDisplayFormat);
#ifdef TRIGGER_AR_ON_SET_VIDEO_FORMAT
    if((tv_mode.uHDARMode != setup_tv_mode.uHDARMode) || (tv_mode.uSDARMode != setup_tv_mode.uSDARMode)) {
        tv_mode.uHDARMode = setup_tv_mode.uHDARMode;
        tv_mode.uSDARMode = setup_tv_mode.uSDARMode;
        SetTvMode(&tv_mode);
    } // if
#else
/*
    u_int32 mode = RBM_ARMODE_NONE;
    switch(VideoDisplayFormat) {
        case vdfPanAndScan  : mode = RBM_ARMODE_PANSCAN; break;
        case vdfLetterBox   : mode = RBM_ARMODE_LETTERBOX; break;
        case vdfCenterCutOut: mode = RBM_ARMODE_NONE; break;
        default: return;
    } // switch
    tv_mode.uHDARMode = mode;
    tv_mode.uSDARMode = mode;
    SetTvMode(&tv_mode);
*/
#endif
} // cRBMDevice::SetVideoDisplayFormat

///< Sets the output video format to either 16:9 or 4:3 (only useful
///< if this device has an MPEG decoder).
void cRBMDevice::SetVideoFormat(bool VideoFormat16_9) {
    DBG(DBG_FN,"cRBMDevice::SetVideoFormat %d", VideoFormat16_9);
#ifdef TRIGGER_AR_ON_SET_VIDEO_FORMAT
    // Ignore function description!!!
    if(SkipSetVideoFormat > 0) {
        SkipSetVideoFormat--;
        return;
    } // if
    switch(tv_mode.uHDARMode) {
        case RBM_ARMODE_LETTERBOX: tv_mode.uHDARMode = RBM_ARMODE_PANSCAN;   break;
        case RBM_ARMODE_PANSCAN  : tv_mode.uHDARMode = RBM_ARMODE_NONE;      break;
        default                  : tv_mode.uHDARMode = RBM_ARMODE_LETTERBOX; break;
    } // switch
    switch(tv_mode.uSDARMode) {
        case RBM_ARMODE_LETTERBOX: tv_mode.uSDARMode = RBM_ARMODE_PANSCAN;   break;
        case RBM_ARMODE_PANSCAN  : tv_mode.uSDARMode = RBM_ARMODE_NONE;      break;
        default                  : tv_mode.uSDARMode = RBM_ARMODE_LETTERBOX; break;
    } // switch
    SetTvMode(&tv_mode);
    if(tv_mode.uHDARMode == tv_mode.uHDARMode)
        Skins.Message(mtInfo, tr(RBM_ARMODE(tv_mode.uHDARMode)));
    else
        Skins.Message(mtInfo, cString::sprintf(tr("HDMI: %s      SCART: %s"), RBM_ARMODE(tv_mode.uHDARMode), RBM_ARMODE(tv_mode.uHDARMode)));
#else
    tv_mode.uHDAspect = VideoFormat16_9 ? RBM_ASPECT_16_9 : RBM_ASPECT_4_3;
    tv_mode.uSDAspect = VideoFormat16_9 ? RBM_ASPECT_16_9 : RBM_ASPECT_4_3;
    SetTvMode(&tv_mode);
#endif
} // cRBMDevice::SetVideoFormat

int cRBMDevice::PlayVideoTs(const uchar *Data, int Length, bool VideoOnly, uchar *PATPMT) {
//    DBG(DBG_FN,"cRBMDevice::PlayVideoTs");
    CheckPATPMT(PATPMT);
    if(!CheckMode()) return 0;
    if(!Length) return 0;
    cMutexLock lock(&dataMutex);
    if(IFrameTrick) {
        StillPicture(Data, Length);
        return Length;
    } else
        return WriteAllOrNothing(main_fd, Data, Length, 0/*1000*/, 10);
} // cRBMDevice::PlayVideoTs

///< Plays all valid PES packets in Data with the given Length.
///< If Data is NULL any leftover data from a previous call will be
///< discarded. If VideoOnly is true, only the video will be displayed,
///< which is necessary for trick modes like 'fast forward'.
///< Data should point to a sequence of complete PES packets. If the
///< last packet in Data is not complete, it will be copied and combined
///< to a complete packet with data from the next call to PlayPes().
///< That way any functions called from within PlayPes() will be
///< guaranteed to always receive complete PES packets.
int cRBMDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly) {
//	DBG(DBG_FN,"cRBMDevice::PlayPes %d videoonly %d IFrameTrick %d", Length, VideoOnly, IFrameTrick);
	cMutexLock lock(&dataMutex);
	if(!Length)
		Clear();
	if(IFrameTrick) {
		last_audio_pts = -1;
		if((Length >= 9+5) && PesHasPts(Data)) last_video_pts = PesGetPts(Data);
		if(Length) StillPicture(Data, Length);
		return Length;
	} // if	
	BufferPos=0;
	int ret = cDevice::PlayPes(Data, Length, VideoOnly);
	if(!CheckMode()) return 0;
	if((ret > 0) && BufferPos) {
		if(((int)BufferPos)==WriteAllOrNothing(main_fd, Buffer, BufferPos, 0, 10))
			return ret;
		return 0;
	} // if
	return ret;
} // cRBMDevice::PlayPes

int cRBMDevice::PlayPesPacket(const uchar *Data, int Length, bool videoOnly) {
//    DBG(DBG_FN,"cRBMDevice::PlayPesPacket %d videoonly %d (%02x)", Length, videoOnly, (Length>3)?Data[3]:0);
    return cDevice::PlayPesPacket(Data, Length, videoOnly);
} // cRBMDevice::PlayPesPacket

///< Plays the given data block as video.
///< Data points to exactly one complete PES packet of the given Length.
///< PlayVideo() shall process the packet either as a whole (returning
///< Length) or not at all (returning 0 or -1 and setting 'errno' to EAGAIN).
///< \return Returns the number of bytes actually taken from Data, or -1
///< in case of an error.

bool cRBMDevice::AddPacket(const uchar *Data, int Length) {
	if(BufferPos+Length > MAX_BUFFER) {
		printf("AddPacket: Buffer overflow!! %ld %d %d\n", BufferPos, Length, MAX_BUFFER);
		memcpy(&Buffer[BufferPos], Data, MAX_BUFFER-BufferPos);
		BufferPos = MAX_BUFFER;
	} else {
		memcpy(&Buffer[BufferPos], Data, Length);
		BufferPos += Length;
	} // if
	return true;
} // cRBMDevice::AddPacket

int cRBMDevice::ConvertPEStoTS(const uchar *Data, int Length, uchar *Counter, uchar Id) {
	unsigned char ts_buffer[188];
	unsigned char stream_id = Id ? Id : Data[3];
	bool first=true;
	int i = 0;
	while (i <= (Length-184)) {
		unsigned long long pts = 0;
		ts_buffer[0] = 0x47;
		ts_buffer[1] = first ? 0x40 : 0x00;
		ts_buffer[1] |= (stream_id >> 8) & 0x1F;
		ts_buffer[2] = stream_id & 0xFF;
//		if(!i && ((Data[i+7] & 0xC0) == 0xC0)) {
		if(!i && (Data[i+7] & 0x80)) {
			int pts_pos= i+9;
			if(Data[i+7] & 0x40)
				pts_pos += 5;
			pts = (((unsigned long long)Data[pts_pos++]) & 0x0E)<<(30-1);
			pts += (((unsigned long long)Data[pts_pos++]) & 0xFF)<<22;
			pts += (((unsigned long long)Data[pts_pos++]) & 0xFE)<<(15-1);
			pts += (((unsigned long long)Data[pts_pos++]) & 0xFF)<<7;
			pts += (((unsigned long long)Data[pts_pos++]) & 0xFE)>>1;
			if(pts>21600)
				pts-=21600;
			else if (pts)
				pts = 1;
		} // if
		if(pts) {
			ts_buffer[3] = 0x30 | (*Counter & 0xF);
			ts_buffer[4] = 7;
			ts_buffer[5] = 0x10;
			ts_buffer[6] = (pts>>25)&0xFF;
			ts_buffer[7] = (pts>>17)&0xFF;
			ts_buffer[8] = (pts>>9 )&0xFF;
			ts_buffer[9] = (pts>>1 )&0xFF;
			ts_buffer[10] = (pts<<7)&0x80;
			ts_buffer[11] = 0xFF;
			memcpy(&ts_buffer[12], &Data[i], 188-12);
			i += 188-12;
		} else {
			ts_buffer[3] = 0x10 | (*Counter & 0xF);
			memcpy(&ts_buffer[4], &Data[i], 184);
			i += 184;
		} // if
		AddPacket(ts_buffer, 188);
		*Counter = (*Counter + 1) & 15;
		first = false;
	} // for
	int rest = Length -i;
	if (rest > 0) {
		ts_buffer[0] = 0x47;
		ts_buffer[1] = first ? 0x40 : 0x00;
		ts_buffer[1] |= (stream_id >> 8) & 0x1F;
		ts_buffer[2] = stream_id & 0xFF;
		ts_buffer[4] = 183 - rest;
		if (ts_buffer[4] > 0) {
			ts_buffer[3] = 0x30 | (*Counter & 0xF);
			ts_buffer[5] = 0x00;
			memset(&ts_buffer[6], 0xFF, ts_buffer[4] - 1);
			memcpy(&ts_buffer[5 + ts_buffer[4]], &Data[i], rest);
		} else {
			ts_buffer[3] = 0x30 | (*Counter & 0xF);
			memcpy(&ts_buffer[5], &Data[i], rest);
		} // if
		AddPacket(ts_buffer, 188);
		*Counter = (*Counter + 1) & 15;
	} // if
	return Length;
} // cRBMDevice::ConvertPEStoTS

int cRBMDevice::PlayVideo(const uchar *Data, int Length) {
//    DBG(DBG_FN,"cRBMDevice::PlayVideo %d (%02x)", Length, (Length>3)?Data[3]:0);
    if((Length < 6) || Data[0] || Data[1] || (Data[2]!=1)) {
        printf("Not a valid video pes packet\n");
        return Length;
    } // if
    if((Data[3] >= 0xe0) && (Data[3] <= 0xef))
    	av_mode.uVideoPid  = Data[3];
//    av_mode.uPCRPid    = Data[3];
    if((Length >= 9+5) && PesHasPts(Data)) last_video_pts = PesGetPts(Data);
    av_mode.uStillFormat = av_mode.uVideoFormat = RBM_VIDEO_FORMAT_MPEG2;
    if(IFrameTrick)
        return Length;//ConvertPEStoES(Data, Length);
    return ConvertPEStoTS(Data, Length, &VideoCounter, av_mode.uVideoPid);
} // cRBMDevice::PlayVideo

///< Plays the given data block as audio.
///< Data points to exactly one complete PES packet of the given Length.
///< Id indicates the type of audio data this packet holds.
///< PlayAudio() shall process the packet either as a whole (returning
///< Length) or not at all (returning 0 or -1 and setting 'errno' to EAGAIN).
///< \return Returns the number of bytes actually taken from Data, or -1
///< in case of an error.
int cRBMDevice::PlayAudio(const uchar *Data, int Length, uchar Id) {
//    DBG(DBG_FN,"cRBMDevice::PlayAudio %d %d", Length, Id);
    if((Length < 6) || Data[0] || Data[1] || (Data[2]!=1)) {
        printf("Not a valid audio pes packet\n");
        return Length;
    } // if
    if((Length >= 9) && (Data[6] & 0x80) && (Data[6] & 0x4)) {
        int off = 9+Data[8]+1;
        if(Length >= off) {
            int layer = 4-((Data[off]>>1)&0x03);
            MP3Detected = (layer == 3);
        } // if
    } // if
    if((Length >= 9+5) && PesHasPts(Data)) last_audio_pts = PesGetPts(Data);
    if(IFrameTrick)
        return Length;
    return ConvertPEStoTS(Data, Length, &AudioCounter[Id], Id);
} // cRBMDevice::PlayAudio

///< Returns true if the device itself or any of the file handles in
///< Poller is ready for further action.
///< If TimeoutMs is not zero, the device will wait up to the given number
///< of milleseconds before returning in case it can't accept any data.
bool cRBMDevice::Poll(cPoller &Poller, int TimeoutMs /*= 0*/) {
//  DBG(DBG_FN,"cRBMDevice::Poll");
    if(!Init()) return false;
    Poller.Add(main_fd, true);
    bool ret = Poller.Poll(TimeoutMs);
//    if(!ret) {printf("Poll blocked\n"); esyslog("Poll blocked\n");}
    return ret;
} // cRBMDevice::Poll

///< Returns true if the device's output buffers are empty, i. e. any
///< data which was bufferd so far has been processed.
///< If TimeoutMs is not zero, the device will wait up to the given
///< number of milliseconds before returning in case there is still
///< data in the buffers..
bool cRBMDevice::Flush(int TimeoutMs /*= 0*/) {
//    DBG(DBG_FN,"cRBMDevice::Flush %d", TimeoutMs);
    if(!Init()) return false;
    rbm_av_flush flush;
    memset(&flush, 0, sizeof(flush));
    flush.mode    = RBMIO_FLUSH_WAIT;
    flush.timeout = TimeoutMs;
    int ret = ioctl(main_fd, RBMIO_FLUSH, &flush);
    DBG(DBG_FN,"cRBMDevice::Flush %d", ret);
    return ret ? false : true;
} // cRBMDevice::Flush

int cRBMDevice::AproxFramesInQueue() {
//  DBG(DBG_FN,"cRBMDevice::AproxFramesInQueue");
    if(!Init()) return false;
    rbm_afc av_afc = 0;
    int ret = ioctl(main_fd, RBMIO_GET_AFC, &av_afc);
    if(ret) {
        DBG(DBG_FN,"cRBMDevice::AproxFramesInQueue %d", ret);
        return 0;
    } // if
    return av_afc;
} // cRBMDevice::AproxFramesInQueue

#if VDRVERSNUM >= 10716

void cRBMDevice::GetOsdSize(int &Width, int &Height, double &Aspect) {
    Width=720;
    Height=576;
    Aspect=1.0;
} // cRBMDevice::GetOsdSize

int cRBMDevice::PlayTsVideo(const uchar *Data, int Length) {
    int64_t pts = TsGetPts(Data, Length);
    if(pts != -1) last_video_pts=pts;
    return WriteTS(Data, Length);
} // cRBMDevice::PlayTsVideo

int cRBMDevice::PlayTsAudio(const uchar *Data, int Length) {
    int64_t pts = TsGetPts(Data, Length);
    if(pts != -1) last_audio_pts=pts;
    return WriteTS(Data, Length);
} // cRBMDevice::PlayTsAudio

int cRBMDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly) {
    if(IFrameTrick) {
        last_audio_pts=-1;
        int64_t pts = TsGetPts(Data, Length);
        if(pts != -1) last_video_pts=pts;
        StillPicture(Data, Length);
        return Length;
    } else
        return cDevice::PlayTs(Data, Length);
} // cRBMDevice::PlayTs

int cRBMDevice::WriteTS(const uchar *Data, int Length) {
    cMutexLock lock(&dataMutex);
    int ret = Length;
    if(BufferPos+ret > MAX_BUFFER)
        ret = ((MAX_BUFFER-BufferPos)/TS_SIZE)*TS_SIZE;
    if(ret) AddPacket(Data, ret);
//isyslog("added %d/%d", ret, BufferPos);
    if((BufferPos>(MAX_BUFFER>>1)) || ((cTimeMs::Now() - lastWrite) > 40)) {
        CheckPatPmt();
        if(!CheckMode()) return 0;
        int written = WriteAllOrNothing(main_fd, Buffer, BufferPos, 0, 10);
        if(((int)BufferPos)==written)
            BufferPos=0;
//else isyslog("failed write %d", BufferPos);
        lastWrite = cTimeMs::Now();
    } // if
    return ret;
} // cRBMDevice::WriteTS

#endif 

///< Actually starts the thread.
bool cRBMDevice::Start(void) {return true;}

bool cRBMDevice::SetTvMode(rbm_tv_mode *mode, bool setup) {
    if(setup) setup_tv_mode = *mode;
    if(!Init() || !mode) return false;
    rbm_tv_mode new_mode = tv_mode = *mode;
    if(!OutputEnable) {
        new_mode.uHDOutputMode = RBM_OUTPUT_MODE_NONE;
        new_mode.uSDOutputMode = RBM_OUTPUT_MODE_NONE;
    } // if
    if(ioctl(main_fd, RBMIO_SET_TV_MODE, &new_mode) != 0) return false;
    isyslog("Setting TVMode HDMI: (%s) %s %s %s %s SCART: %s %s %s %s MPEG2: %s AUDIO: MPA offset %ldms AC3 offset %ldms",
    OutputEnable ? "enabled" : "disabled",
    RBM_HDMIMODE(tv_mode.uHDMIMode), RBM_OUTPUT_MODE(tv_mode.uHDOutputMode), RBM_ASPECT(tv_mode.uHDAspect), RBM_ARMODE(tv_mode.uHDARMode),
    RBM_SCARTMODE(tv_mode.uSCARTMode), RBM_OUTPUT_MODE(tv_mode.uSDOutputMode), RBM_ASPECT(tv_mode.uSDAspect), RBM_ARMODE(tv_mode.uSDARMode),
    RBM_MPEG2_MODE(tv_mode.uMPEG2Mode), tv_mode.iMPAOffset, tv_mode.iAC3Offset);
    return true;
} // cRBMDevice::SetTvMode

bool cRBMDevice::GetTvMode(rbm_tv_mode *mode) {
    if(!Init()) return false;
    if(ioctl(main_fd, RBMIO_GET_TV_MODE, mode) != 0) return false;
    return true;
} // cRBMDevice::GetTvMode

bool cRBMDevice::SetOutput(bool enable) {
    OutputEnable = enable;
    return SetTvMode(&tv_mode);
} // cRBMDevice::SetOutput
