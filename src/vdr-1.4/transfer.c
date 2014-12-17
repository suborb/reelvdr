/*
 * transfer.c: Transfer mode
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: transfer.c 1.33 2006/01/29 17:24:39 kls Exp $
 */

#include "transfer.h"

#define TRANSFERBUFSIZE  MEGABYTE(2)
#define POLLTIMEOUTS_BEFORE_DEVICECLEAR 6

// --- cTransfer -------------------------------------------------------------

cTransfer::cTransfer(int VPid, int PPid, const int *APids, const int *DPids, const int *SPids)
:cReceiver(0, -1, VPid, PPid, APids, DPids, SPids)
,cThread("transfer")
{
//  ringBuffer = new cRingBufferLinear(TRANSFERBUFSIZE, TS_SIZE * 2, true, "Transfer");
  remux = new cRemux(VPid, PPid, APids, DPids, SPids, false, rAuto, true);
  remux->SetTimeouts(50, 20);
}

cTransfer::~cTransfer()
{
  cReceiver::Detach();
  cPlayer::Detach();
  delete remux;
//  delete ringBuffer;
}

void cTransfer::Activate(bool On)
{
  if (On)
     Start();
  else
     Cancel(3);
}

void cTransfer::Receive(uchar *Data, int Length)
{
  if (IsAttached() && Running()) {
/*
    static FILE *tsOut = NULL;
    if (!tsOut)
    {
    tsOut = ::fopen("/mnt/hd/video/aufnahme.ts", "wb");
    }
    if (tsOut)
    {
    ::fwrite(Data, 1, Length, tsOut);
    }
*/					      

/*
     int p = ringBuffer->Put(Data, Length);
     if (p != Length && Running())
        ringBuffer->ReportOverflow(Length - p);
*/
	   remux->Put(Data,Length);
     return;
     }
}

void cTransfer::Action(void)
{
  int PollTimeouts = 0;
  uchar *p = NULL;
  int firstTime = 0;
  int Result = 0;
  uchar patpmt[TS_SIZE*2];
  patpmt[0]=0x47;
  while (Running()) {
        if (!p || !Result)
           //p = remux->Get(Result);
	   p = remux->Get(Result,NULL, 1);
        if (p) {
           cPoller Poller;
           if (DevicePoll(Poller, 100)) {
              int w;
              PollTimeouts = 0;
              if(remux->TSmode()==rTS){
                 if(firstTime++==0){
		            remux->GetPATPMT(patpmt, 2*TS_SIZE);
                    //printf("cTransfer::Action() calls PlayTS\n");
                    w = PlayTS(p, Result, false, (unsigned char*)&patpmt);
		         } else
	                w = PlayTS(p, Result, false, NULL);
	          } else {
                 //printf("cTransfer::Action() calls PlayPS\n");
                 w = PlayPes(p, Result);
              }

              if (w > 0) {
                 p += w;
                 Result -= w;
                 remux->Del(w);
                 if (Result <= 0)
                    p = NULL;
                 }
              else if (w < 0 && FATALERRNO)
                 LOG_ERROR;
              }
           else {
              PollTimeouts++;
              if (PollTimeouts == POLLTIMEOUTS_BEFORE_DEVICECLEAR) {
                 dsyslog("clearing device because of consecutive poll timeouts");
                 DeviceClear();
//                 ringBuffer->Clear();
                 remux->Clear();
                 if(remux->TSmode()==rTS)
                    PlayTS(NULL, 0);
                 else
                    PlayPes(NULL, 0);
                 p = NULL;
                 }
              }
           }
        }
}

// --- cTransferControl ------------------------------------------------------

cDevice *cTransferControl::receiverDevice = NULL;

cTransferControl::cTransferControl(cDevice *ReceiverDevice, int VPid, int PPid, const int *APids, const int *DPids, const int *SPids)
:cControl(transfer = new cTransfer(VPid, PPid, APids, DPids, SPids), true)
{
  ReceiverDevice->AttachReceiver(transfer, true);
  receiverDevice = ReceiverDevice;
}

cTransferControl::~cTransferControl()
{
  receiverDevice = NULL;
  delete transfer;
}
