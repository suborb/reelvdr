/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __FEMONOSD_H
#define __FEMONOSD_H

#include <linux/dvb/frontend.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <vdr/osd.h>
#include <vdr/thread.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/channels.h>
#include <vdr/transfer.h>
#include <vdr/tools.h>
#include "svdrpservice.h"

#define MAX_BM_NUMBER 5

class cFemonOsd : public cOsdObject, public cThread, public cStatus {
private:
  enum { MAX_BMNUMBERS = 6 };
  static cFemonOsd *pInstance;
  cOsd *m_Osd;
  cFemonReceiver *m_Receiver;
  int m_Frontend;
  int m_SvdrpFrontend;
  double m_SvdrpVideoBitrate;
  double m_SvdrpAudioBitrate;
  SvdrpConnection_v1_0 m_SvdrpConnection;
  cPlugin *m_SvdrpPlugin;
  struct dvb_frontend_info m_FrontendInfo;
  int m_Number;
  int m_OldNumber;
  uint16_t m_SNR;
  uint16_t m_Signal;
  uint32_t m_BER;
  uint32_t m_UNC;
  fe_status_t m_FrontendStatus;
  int m_DisplayMode;
  const cFont *m_Font;
  cTimeMs m_InputTime;
  cMutex* m_Mutex;
  static cBitmap bmStereo, bmMonoLeft, bmMonoRight, bmDD, bmDD20, bmDD51;
  static cBitmap bmNumbers[MAX_BMNUMBERS];
  static cBitmap bmDevice, bmPAL, bmNTSC, bmSVDRP;
  static cBitmap bmAspectRatio_1_1, bmAspectRatio_16_9, bmAspectRatio_2_21_1, bmAspectRatio_4_3;
  static cBitmap bmLock, bmSignal, bmCarrier, bmViterbi, bmSync;
  static cBitmap bmLockRed, bmSignalRed, bmCarrierRed, bmViterbiRed, bmSyncRed;
  void DrawStatusWindow(void);
  void DrawInfoWindow(void);
  bool SvdrpConnect(void);
  bool SvdrpTune(void);

protected:
  cFemonOsd();
  cFemonOsd(const cFemonOsd&);
  cFemonOsd& operator= (const cFemonOsd&);
  virtual void Action(void);
  virtual void ChannelSwitch(const cDevice * device, int channelNumber);
  virtual void SetAudioTrack(int Index, const char * const *Tracks);

public:
  static cFemonOsd *Instance(bool create = false);
  ~cFemonOsd();

  virtual void Show(void);
  virtual eOSState ProcessKey(eKeys Key);

  bool DeviceSwitch(int direction);
  double GetVideoBitrate(void);
  double GetAudioBitrate(void);
  double GetDolbyBitrate(void);
};

#endif //__FEMONOSD_H

