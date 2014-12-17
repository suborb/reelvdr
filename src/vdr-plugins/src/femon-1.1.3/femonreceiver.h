/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __FEMONRECEIVER_H
#define __FEMONRECEIVER_H

#include <vdr/thread.h>
#include <vdr/receiver.h>

enum eVideoFormat {
  VF_UNKNOWN = 0,
  VF_PAL     = 1,
  VF_NTSC    = 2,
  };

enum eAspectRatio {
  AR_RESERVED = 0,
  AR_1_1      = 100,
  AR_4_3      = 133,
  AR_16_9     = 177,
  AR_2_21_1   = 233,
  };

enum eCenterMixLevel {
  CML_MINUS_3dB   = 0,
  CML_MINUS_4_5dB = 1,
  CML_MINUS_6dB   = 2,
  CML_RESERVED    = 3,
  };

enum eSurroundMixLevel {
  SML_MINUS_3dB = 0,
  SML_MINUS_6dB = 1,
  SML_0_dB      = 2,
  SML_RESERVED  = 3,
  };

enum eDolbySurroundMode {
  DSM_NOT_INDICATED     = 0,
  DSM_NOT_DOLBYSURROUND = 1,
  DSM_DOLBYSURROUND     = 2,
  DSM_RESERVED          = 3,
  };

enum eReveiverCodes {
  FR_RESERVED = -1,
  FR_FREE     = -2,
  FR_NOTVALID = -3
  };

class cFemonReceiver : public cReceiver, public cThread {
private:
  int    m_VideoPid;
  int    m_AudioPid;
  int    m_AC3Pid;
  bool   m_VideoValid;
  int    m_VideoPacketCount;
  int    m_VideoHorizontalSize;
  int    m_VideoVerticalSize;
  int    m_VideoAspectRatio;
  int    m_VideoFormat;
  double m_VideoFrameRate;
  double m_VideoStreamBitrate;
  double m_VideoBitrate;
  bool   m_AudioValid;
  int    m_AudioPacketCount;
  double m_AudioStreamBitrate;
  double m_AudioBitrate;
  int    m_AudioSamplingFreq;
  int    m_AudioMPEGLayer;
  bool   m_AC3Valid;
  int    m_AC3PacketCount;
  double m_AC3Bitrate;
  int    m_AC3FrameSize;
  int    m_AC3SamplingFreq;
  int    m_AC3StreamBitrate;
  int    m_AC3BitStreamMode;
  int    m_AC3AudioCodingMode;
  int    m_AC3CenterMixLevel;
  int    m_AC3SurroundMixLevel;
  int    m_AC3DolbySurroundMode;
  bool   m_AC3LfeOn;
  int    m_AC3DialogLevel;  
  void   GetVideoInfo(uint8_t *mbuf, int count);
  void   GetAudioInfo(uint8_t *mbuf, int count);
  void   GetAC3Info(uint8_t *mbuf, int count);

protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);
  virtual void Action(void);

public:
#if VDRVERSNUM < 10727
  cFemonReceiver(tChannelID ChannelID, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[]);
#else
  cFemonReceiver(const cChannel* chan, int Ca, int Vpid, int Ppid, int Apid[], int Dpid[]);
#endif
  virtual ~cFemonReceiver();

  bool VideoValid(void)           { return m_VideoValid; };          // boolean
  int VideoHorizontalSize(void)   { return m_VideoHorizontalSize; }; // pixels
  int VideoVerticalSize(void)     { return m_VideoVerticalSize; };   // pixels
  int VideoAspectRatio(void)      { return m_VideoAspectRatio; };    // eAspectRatio
  int VideoFormat(void)           { return m_VideoFormat; };         // eVideoFormat
  double VideoFrameRate(void)     { return m_VideoFrameRate; };      // Hz
  double VideoStreamBitrate(void) { return m_VideoStreamBitrate; };  // bit/s
  double VideoBitrate(void)       { return m_VideoBitrate; };        // bit/s

  bool AudioValid(void)           { return m_AudioValid; };          // boolean
  int AudioMPEGLayer(void)        { return m_AudioMPEGLayer; };      // layer number
  int AudioSamplingFreq(void)     { return m_AudioSamplingFreq; };   // Hz
  double AudioStreamBitrate(void) { return m_AudioStreamBitrate; };  // bit/s
  double AudioBitrate(void)       { return m_AudioBitrate; };        // bit/s

  bool AC3Valid(void)             { return m_AC3Valid; };                // boolean
  int AC3SamplingFreq(void)       { return m_AC3SamplingFreq; };         // Hz
  double AC3StreamBitrate(void)   { return m_AC3StreamBitrate; };        // bit/s
  double AC3Bitrate(void)         { return m_AC3Bitrate; };              // bit/s
  int AC3FrameSize(void)          { return m_AC3FrameSize; };            // Bytes
  int AC3BitStreamMode(void)      { return m_AC3BitStreamMode; };        // 0..7
  int AC3AudioCodingMode(void)    { return m_AC3AudioCodingMode; };      // 0..7
  bool AC3_2_0(void)		  { return m_AC3AudioCodingMode == 2; }; // DD 2.0
  bool AC3_5_1(void)	          { return m_AC3AudioCodingMode == 7; }; // DD 5.1
  int AC3CenterMixLevel(void)     { return m_AC3CenterMixLevel; };       // eCenterMixLevel
  int AC3SurroundMixLevel(void)   { return m_AC3SurroundMixLevel; };     // eSurroundMixLevel
  int AC3DolbySurroundMode(void)  { return m_AC3DolbySurroundMode; };    // eDolbySurroundMode
  bool AC3LfeOn(void)             { return m_AC3LfeOn; };                // boolean
  int AC3DialogLevel(void)        { return m_AC3DialogLevel; };          // -dB
  };

#endif //__FEMONRECEIVER_H

