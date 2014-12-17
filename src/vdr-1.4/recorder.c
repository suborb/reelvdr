/*
 * recorder.c: The actual DVB recorder
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recorder.c 1.17 2006/01/08 11:01:25 kls Exp $
 */

#include "recorder.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#define RECORDERBUFSIZE  MEGABYTE(5)

// The maximum time we wait before assuming that a recorded video data stream
// is broken:
#define MAXBROKENTIMEOUT 30 // seconds

#define MINFREEDISKSPACE    (512) // MB
#define DISKCHECKINTERVAL   100 // seconds

#define PATPMT_DISTANCE (1*1024*1024)

class cFileWriter : public cThread {
private:
  cRemux *remux;
  cFileName *fileName;
  cIndexFile *index;
  uchar pictureType;
  int fileSize;
  int diffSize;
  cUnbufferedFile *recordFile;
  time_t lastDiskSpaceCheck;
  bool RunningLowOnDiskSpace(void);
  bool NextFile(void);
protected:
  virtual void Action(void);
public:
  cFileWriter(const char *FileName, cRemux *Remux);
  virtual ~cFileWriter();
  };

cFileWriter::cFileWriter(const char *FileName, cRemux *Remux)
:cThread("file writer")
{
  fileName = NULL;
  remux = Remux;
  index = NULL;
  pictureType = NO_PICTURE;
  fileSize = 0;
  diffSize = 0;
  lastDiskSpaceCheck = time(NULL);
  fileName = new cFileName(FileName, true);
  recordFile = fileName->Open();
  if (!recordFile)
     return;
  // Create the index file:
  index = new cIndexFile(FileName, true);
  if (!index)
     esyslog("ERROR: can't allocate index");
     // let's continue without index, so we'll at least have the recording
}

cFileWriter::~cFileWriter()
{
  Cancel(3);
  delete index;
  delete fileName;
}

bool cFileWriter::RunningLowOnDiskSpace(void)
{
  if (time(NULL) > lastDiskSpaceCheck + DISKCHECKINTERVAL) {
     int Free = FreeDiskSpaceMB(fileName->Name());
     lastDiskSpaceCheck = time(NULL);
     if (Free < MINFREEDISKSPACE) {
        dsyslog("low disk space (%d MB, limit is %d MB)", Free, MINFREEDISKSPACE);
        return true;
        }
     }
  return false;
}

bool cFileWriter::NextFile(void)
{
  if (recordFile && pictureType == I_FRAME) { // every file shall start with an I_FRAME
     if (fileSize > MEGABYTE(Setup.MaxVideoFileSize) || RunningLowOnDiskSpace()) {
        recordFile = fileName->NextFile();
        fileSize = 0;
        }
     }
  return recordFile != NULL;
}

void cFileWriter::Action(void)
{
  time_t t = time(NULL);
  unsigned int skipped = 0;

  while (Running()) {
        int Count;
        uchar *p = remux->Get(Count, &pictureType, 1);

        while(skipped < 10 && (remux->SFmode() == SF_UNKNOWN || remux->TSmode() == rAuto)){ // TB: give remuxer a chance to detect the stream type
           skipped++;
           Count = 0;
           continue;
        }
	
        if (p && Count) {
//		esyslog("COUNT %i\n", Count);
           if (!Running() && pictureType == I_FRAME) // finish the recording before the next 'I' frame
              break;
           if (NextFile()) {
#if 1
              // Add PAT+PMT at every filestart and every MB
              if ((!fileSize || diffSize > PATPMT_DISTANCE) && remux->TSmode()==SF_H264) {
		      uchar patpmt[2*TS_SIZE];
		      int plen;
		      plen=remux->GetPATPMT(patpmt, 2*TS_SIZE);
		      if (plen) {
			      if (recordFile->Write(patpmt, plen) < 0) {
				      LOG_ERROR_STR(fileName->Name());
				      break;
			      }
			      fileSize+=plen;
		      }
		      diffSize=0;
	      }
#endif
              if (index && pictureType != NO_PICTURE)
                 index->Write(pictureType, fileName->Number(), fileSize);
              if (recordFile->Write(p, Count) < 0) {
                 LOG_ERROR_STR(fileName->Name());
                 break;
                 }
              fileSize += Count;
              diffSize += Count;
              remux->Del(Count);
              }
           else
              break;
           t = time(NULL);
           }
        else if (time(NULL) - t > MAXBROKENTIMEOUT) {
           esyslog("ERROR: video data stream broken");
           //cThread::EmergencyExit(true);
	   Skins.Message(mtError, tr("can't record - check your configuration"));
	   // TB
#if 1
	   if(cDevice::GetDevice(0) && cDevice::GetDevice(0)->CiHandler() && cDevice::GetDevice(0)->CiHandler()->NumCams())
		cDevice::GetDevice(0)->CiHandler()->StartDecrypting();
#endif
           t = time(NULL);
           }
        }
}

// ---

cRecorder::cRecorder(const char *FileName, int Ca, int Priority, int VPid, int PPid, const int *APids, const int *DPids, const int *SPids, cLiveBuffer *LiveBuffer)
:cReceiver(Ca, Priority, VPid, PPid, APids, Setup.UseDolbyInRecordings ? DPids : NULL, SPids)
,cThread("recording")
{
  // Make sure the disk is up and running:

  SpinUpDisk(FileName);
#if 0
  ringBuffer = new cRingBufferLinear(RECORDERBUFSIZE, TS_SIZE * 2, true, "Recorder");
  ringBuffer->SetTimeouts(0, 100);
#endif
  remux = new cRemux(VPid, PPid, APids, Setup.UseDolbyInRecordings ? DPids : NULL, SPids, true);
  fileName = strdup(FileName);
  writer = NULL;
  liveBuffer = LiveBuffer;
  if (!LiveBuffer)
    writer = new cFileWriter(FileName, remux);
}

cRecorder::~cRecorder()
{
  Detach();
  delete writer;
  delete remux;
//  delete ringBuffer;
  free(fileName);
}

void cRecorder::Activate(bool On)
{
  if (On) {
     if (writer)
       writer->Start();
     Start();
     }
  else
     Cancel(3);
}

void cRecorder::Receive(uchar *Data, int Length)
{
  if (Running()) {
#if 0
     int p = ringBuffer->Put(Data, Length);
     if (p != Length && Running())
        ringBuffer->ReportOverflow(Length - p);
#else
     remux->Put(Data, Length);
#endif
     }
}

void cRecorder::Action(void)
{
  while (Running()) {
           if (!writer && liveBuffer) {
              int c;
              uchar pictureType;
              uchar *p = remux->Get(c, &pictureType, 1);        
        if (remux->TSmode() == rTS)
        {
             writer = new cFileWriter(fileName, remux);
             writer->Start();
        }
        else
        {
            if (p) {
               if (pictureType == I_FRAME && p[0]==0x00 && p[1]==0x00 && p[2]==0x01 && (p[7] & 0x80) && p[3]>=0xC0 && p[3]<=0xEF) {
                     int64_t pts  = (int64_t) (p[ 9] & 0x0E) << 29 ;
                     pts |= (int64_t)  p[ 10]         << 22 ;
                     pts |= (int64_t) (p[ 11] & 0xFE) << 14 ;
                     pts |= (int64_t)  p[ 12]         <<  7 ;
                     pts |= (int64_t) (p[ 13] & 0xFE) >>  1 ;
                     liveBuffer->CreateIndexFile(fileName,pts);
                     writer = new cFileWriter(fileName, remux);
                     writer->Start();
                     }
                  else
                     remux->Del(c);              
                  }
        }
        continue;
        }
#if 0
        int r;
        uchar *b = ringBuffer->Get(r);
        if (b) {
           int Count = remux->Put(b, r);
           if (Count)
              ringBuffer->Del(Count);
           else
              cCondWait::SleepMs(100); // avoid busy loop when resultBuffer is full in cRemux::Put()
           }
#endif
        usleep(100*1000); // FIXME
        }
}

