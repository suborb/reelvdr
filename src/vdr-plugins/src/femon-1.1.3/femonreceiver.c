/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <unistd.h>
#include "femontools.h"
#include "femoncfg.h"
#include "femonreceiver.h"
#include "vdr/config.h"

#define TS_SIZE       188
#define PAY_START     0x40
#define ADAPT_FIELD   0x20
#define PAYLOAD       0x10
#define PTS_DTS_FLAGS 0xC0

#if VDRVERSNUM < 10716
#if defined(APIVERSNUM) && APIVERSNUM < 10500
cFemonReceiver::cFemonReceiver(tChannelID ChannelID, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[])
:cReceiver(Ca, -1, Vpid, Ppid, Apid, Dpid, NULL), cThread("femon receiver")
{
#else
cFemonReceiver::cFemonReceiver(tChannelID ChannelID, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[])
:cReceiver(ChannelID, -1, Vpid, Ppid, Apid, Dpid, NULL), cThread("femon receiver")
{
#endif
#elif VDRVERSNUM < 10727
cFemonReceiver::cFemonReceiver(tChannelID ChannelID, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[])
:cReceiver(ChannelID, -1, Vpid, Apid, Dpid), cThread("femon receiver")
{
#else
cFemonReceiver::cFemonReceiver(const cChannel* chan, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[])
:cReceiver(chan, -1), cThread("femon receiver") { AddPids(Vpid, *Apid, *Dpid);
#endif

  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  m_VideoPid = Vpid;
  m_AudioPid = Apid[0];
  m_AC3Pid = Dpid[0];
  m_VideoValid = false;
  m_VideoPacketCount = 0;
  m_VideoHorizontalSize = 0;
  m_VideoVerticalSize = 0;
  m_VideoAspectRatio = AR_RESERVED;
  m_VideoFormat = VF_UNKNOWN;
  m_VideoFrameRate = 0.0;
  m_VideoStreamBitrate = 0.0;
  m_VideoBitrate = 0.0;
  m_AudioValid = false;
  m_AudioPacketCount = 0;
  m_AudioStreamBitrate = -2.0;
  m_AudioBitrate = 0.0;
  m_AudioSamplingFreq = -1;
  m_AudioMPEGLayer = 0;
  m_AudioBitrate = 0.0;
  m_AC3Valid = false;
  m_AC3PacketCount = 0; 
  m_AC3StreamBitrate = 0;
  m_AC3SamplingFreq = 0;
  m_AC3Bitrate = 0;
  m_AC3FrameSize = 0;
  m_AC3BitStreamMode = FR_NOTVALID;
  m_AC3AudioCodingMode = FR_NOTVALID;
  m_AC3CenterMixLevel = FR_NOTVALID;
  m_AC3SurroundMixLevel = FR_NOTVALID;
  m_AC3DolbySurroundMode = FR_NOTVALID;
  m_AC3LfeOn = false;
  m_AC3DialogLevel = FR_NOTVALID;
}
 
cFemonReceiver::~cFemonReceiver(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  if (Running())
     Cancel(3);
  Detach();
}

/* The following function originates from libdvbmpeg: */
void cFemonReceiver::GetVideoInfo(uint8_t *mbuf, int count)
{
  uint8_t *headr;
  int found = 0;
  int c = 0;
  //m_VideoValid = false;
  while ((found < 4) && ((c + 4) < count)) {
    uint8_t *b;
    b = mbuf + c;
    if ((b[0] == 0x00) && (b[1] == 0x00) && (b[2] == 0x01) && (b[3] == 0xb3))
       found = 4;
    else
       c++;
    }
  if ((!found) || ((c + 16) >= count)) return;
  m_VideoValid = true;
  headr = mbuf + c + 4;
  m_VideoHorizontalSize = ((headr[1] & 0xF0) >> 4) | (headr[0] << 4);
  m_VideoVerticalSize = ((headr[1] & 0x0F) << 8) | (headr[2]);
  int sw = (int)((headr[3] & 0xF0) >> 4);
  switch ( sw ){
    case 1:
      m_VideoAspectRatio = AR_1_1;
      break;
    case 2:
      m_VideoAspectRatio = AR_4_3;
      break;
    case 3:
      m_VideoAspectRatio = AR_16_9;
      break;
    case 4:
      m_VideoAspectRatio = AR_2_21_1;
      break;
    case 5 ... 15:
    default:
      m_VideoAspectRatio = AR_RESERVED;
      break;
    }
  sw = (int)(headr[3] & 0x0F);
  switch ( sw ) {
    case 1:
      m_VideoFrameRate = 24000 / 1001.0;
      m_VideoFormat = VF_UNKNOWN;
      break;
    case 2:
      m_VideoFrameRate = 24.0;
      m_VideoFormat = VF_UNKNOWN;
      break;
    case 3:
      m_VideoFrameRate = 25.0;
      m_VideoFormat = VF_PAL;
      break;
    case 4:
      m_VideoFrameRate = 30000 / 1001.0;
      m_VideoFormat = VF_NTSC;
      break;
    case 5:
      m_VideoFrameRate = 30.0;
      m_VideoFormat = VF_NTSC;
      break;
    case 6:
      m_VideoFrameRate = 50.0;
      m_VideoFormat = VF_PAL;
      break;
    case 7:
      m_VideoFrameRate = 60.0;
      m_VideoFormat = VF_NTSC;
      break;
    case 8:
      m_VideoFrameRate = 60000 / 1001.0;
      m_VideoFormat = VF_NTSC;
      break;
    case 9 ... 15:
    default:
      m_VideoFrameRate = 0;
      m_VideoFormat = VF_UNKNOWN;
      break;
    }
  m_VideoStreamBitrate = 400.0 * (((headr[4] << 10) & 0x0003FC00UL) | ((headr[5] << 2) & 0x000003FCUL) | (((headr[6] & 0xC0) >> 6) & 0x00000003UL));
}

static unsigned int bitrates[3][16] =
{
  {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
  {0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0},
  {0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0}
};

static unsigned int samplerates[4] =
{441, 480, 320, 0};

/* The following function originates from libdvbmpeg: */
void cFemonReceiver::GetAudioInfo(uint8_t *mbuf, int count)
{
  uint8_t *headr;
  int found = 0;
  int c = 0;
  int tmp = 0;
  //m_AudioValid = false;
  while (!found && (c < count)) {
    uint8_t *b = mbuf + c;
    if ((b[0] == 0xff) && ((b[1] & 0xf8) == 0xf8))
       found = 1;
    else
       c++;
    }	
  if ((!found) || ((c + 3) >= count)) return;
  m_AudioValid = true;
  headr = mbuf + c;
  m_AudioMPEGLayer = 4 - ((headr[1] & 0x06) >> 1);
  tmp = bitrates[(3 - ((headr[1] & 0x06) >> 1))][(headr[2] >> 4)] * 1000;
  if (tmp == 0)
     m_AudioStreamBitrate = (double)FR_FREE;
  else if (tmp == 0xf)
     m_AudioStreamBitrate = (double)FR_RESERVED;
  else
     m_AudioStreamBitrate = tmp;
  tmp = samplerates[((headr[2] & 0x0c) >> 2)] * 100;
  if (tmp == 3)
     m_AudioSamplingFreq = FR_RESERVED;
  else
     m_AudioSamplingFreq = tmp;
}

static unsigned int ac3_bitrates[32] =
{32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static unsigned int ac3_freq[4] =
{480, 441, 320, 0};

static unsigned int ac3_frames[3][32] =
{
  {64, 80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640, 768,  896,  1024, 1152, 1280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {69, 87,  104, 121, 139, 174, 208, 243, 278, 348, 417, 487, 557, 696, 835,  975,  1114, 1253, 1393, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672, 768, 960, 1152, 1344, 1536, 1728, 1920, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/*
** AC3 Audio Header: http://www.atsc.org/standards/a_52a.pdf
** The following function originates from libdvbmpeg:
*/
void cFemonReceiver::GetAC3Info(uint8_t *mbuf, int count)
{
  uint8_t *headr;
  int found = 0;
  int c = 0;
  uint8_t frame;
  //m_AC3Valid = false;
  while (!found && (c < count)) {
    uint8_t *b = mbuf + c;
    if ((b[0] == 0x0b) && (b[1] == 0x77))
       found = 1;
    else
       c++;
    }
  if ((!found) || ((c + 5) >= count)) return;
  m_AC3Valid = true;
  headr = mbuf + c + 2;
  frame = (headr[2] & 0x3f);
  m_AC3StreamBitrate = ac3_bitrates[frame >> 1] * 1000;
  int fr = (headr[2] & 0xc0 ) >> 6;
  m_AC3SamplingFreq = ac3_freq[fr] * 100;
  m_AC3FrameSize = ac3_frames[fr][frame >> 1];
  if ((frame & 1) && (fr == 1)) m_AC3FrameSize++;
     m_AC3FrameSize <<= 1;
  m_AC3BitStreamMode = (headr[3] & 7);
  m_AC3AudioCodingMode = (headr[4] & 0xE0) >> 5;
  if ((m_AC3AudioCodingMode & 0x01) && (m_AC3AudioCodingMode != 0x01)) {
     // 3 front channels
     m_AC3CenterMixLevel = (headr[4] & 0x18) >> 3;
     if (m_AC3AudioCodingMode & 0x04) {
        // a surround channel exists
        m_AC3SurroundMixLevel = (headr[4] & 0x06) >> 1;
        if (m_AC3AudioCodingMode == 0x02) {
           // if in 2/0 mode
           m_AC3DolbySurroundMode = ((headr[4] & 0x01) << 1) | ((headr[5] & 0x80) >> 7);
           m_AC3LfeOn = (headr[5] & 0x40) >> 6;
           m_AC3DialogLevel = (headr[5] & 0x3e) >> 1;
           }
        else {
           m_AC3DolbySurroundMode = FR_NOTVALID;
           m_AC3LfeOn = (headr[4] & 0x01);
           m_AC3DialogLevel = (headr[5] & 0xF8) >> 3;
           }
        }
     else {
        m_AC3SurroundMixLevel = FR_NOTVALID;
        if (m_AC3AudioCodingMode == 0x02) {
           // if in 2/0 mode
            m_AC3DolbySurroundMode = (headr[4] & 0x06) >> 1;
            m_AC3LfeOn = (headr[4] & 0x01);
            m_AC3DialogLevel = (headr[5] & 0xF8) >> 3;
           }
        else {
           m_AC3DolbySurroundMode = FR_NOTVALID;
           m_AC3LfeOn = (headr[4] & 0x04) >> 2;
           m_AC3DialogLevel = (headr[4] & 0x03) << 3 | ((headr[5] & 0xE0) >> 5);
           }
        }
     }
  else {
     m_AC3CenterMixLevel = FR_NOTVALID;
     if (m_AC3AudioCodingMode & 0x04) {
        // a surround channel exists
        m_AC3SurroundMixLevel = (headr[4] & 0x18) >> 3;
        if (m_AC3AudioCodingMode == 0x02) {
           // if in 2/0 mode
           m_AC3DolbySurroundMode = (headr[4] & 0x06) >> 1;
           m_AC3LfeOn = (headr[4] & 0x01);
           m_AC3DialogLevel = (headr[5] & 0xF8) >> 3;
           }
        else {
           m_AC3DolbySurroundMode = FR_NOTVALID;
           m_AC3LfeOn = (headr[4] & 0x04) >> 2;
           m_AC3DialogLevel = (headr[4] & 0x03) << 3 | ((headr[5] & 0xE0) >> 5);
           }
        }
     else {
        m_AC3SurroundMixLevel = FR_NOTVALID;
        if (m_AC3AudioCodingMode == 0x02) {
           // if in 2/0 mode
           m_AC3DolbySurroundMode = (headr[4] & 0x18) >> 3;
           m_AC3LfeOn = (headr[4] & 0x04) >> 2;
           m_AC3DialogLevel = (headr[4] & 0x03) << 3 | ((headr[5] & 0xE0) >> 5);
           }
        else {
           m_AC3DolbySurroundMode = FR_NOTVALID;
           m_AC3LfeOn = (headr[4] & 0x10) >> 4;
           m_AC3DialogLevel = ((headr[4] & 0x0F) << 1) | ((headr[5] & 0x80) >> 7);
           }
        }
     }
}

void cFemonReceiver::Activate(bool On)
{
  Dprintf("%s(%d)\n", __PRETTY_FUNCTION__, On);
  if (On)
     Start();
  else
     Cancel(10);
}

void cFemonReceiver::Receive(uchar *Data, int Length)
{
  // TS packet length: TS_SIZE
  //if (Length == TS_SIZE) {
    int n;
   for(n=0;n<Length;n+=188) {
     int pid = ((Data[1] & 0x1f) << 8) | (Data[2]);
     if (pid == m_VideoPid) {
        m_VideoPacketCount++;
        }
     else if (pid == m_AudioPid) {
        m_AudioPacketCount++;
        }
     else if (pid == m_AC3Pid) {
        m_AC3PacketCount++;
        }
     /* the following originates from libdvbmpeg: */
     if (!(Data[3] & PAYLOAD)) {
        return;
        }
     uint8_t off = 0;
     if (Data[3] & ADAPT_FIELD) {
        off = Data[4] + 1;
        }
     if (Data[1] & PAY_START) {
        uint8_t *sb = Data + 4 + off;
        if (sb[7] & PTS_DTS_FLAGS) {
           uint8_t *pay = sb + sb[8] + 9; 
           int l = TS_SIZE - 13 - off - sb[8];
           if (pid == m_VideoPid) {
              GetVideoInfo(pay, l);
              }
           else if (pid == m_AudioPid) {
              GetAudioInfo(pay, l);
              }
           else if (pid == m_AC3Pid) {
              GetAC3Info(pay, l);
              }
           }
        }
     /* end */
     Data+=188;
     }
}

void cFemonReceiver::Action(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  cTimeMs t;
  while (Running()) {
        t.Set(0);
        // TS packet 188 bytes - 4 byte header; MPEG standard defines 1Mbit = 1000000bit
        m_VideoBitrate = (10.0 * 8.0 * 184.0 * m_VideoPacketCount) / femonConfig.calcinterval;
        m_VideoPacketCount = 0;
        m_AudioBitrate = (10.0 * 8.0 * 184.0 * m_AudioPacketCount) / femonConfig.calcinterval;
        m_AudioPacketCount = 0;
        m_AC3Bitrate   = (10.0 * 8.0 * 184.0 * m_AC3PacketCount)   / femonConfig.calcinterval;
        m_AC3PacketCount = 0;
        cCondWait::SleepMs(100 * femonConfig.calcinterval - t.Elapsed());
    }
}
