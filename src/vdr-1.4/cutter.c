/*
 * cutter.c: The video cutting facilities
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: cutter.c 1.16 2006/07/30 10:22:08 kls Exp $
 */

#include "cutter.h"
#include "debug.h"
#include "recording.h"
#include "remux.h"
#include "thread.h"
#include "videodir.h"


// --- cCuttingThread --------------------------------------------------------

#ifndef CUTTER_MAX_BANDWIDTH
#ifdef RBLITE
#  define CUTTER_MAX_BANDWIDTH MEGABYTE(3) // 10 MB/s
#else
#  define CUTTER_MAX_BANDWIDTH MEGABYTE(20) // 10 MB/s
#endif
#endif
#ifndef CUTTER_REL_BANDWIDTH
#  define CUTTER_REL_BANDWIDTH 75 // %
#endif
#ifndef CUTTER_PRIORITY
#  define CUTTER_PRIORITY sched_get_priority_min(SCHED_OTHER)
#endif
#define CUTTER_TIMESLICE   100   // ms

class cCuttingThread : public cThread {
private:
  const char *error;
  cUnbufferedFile *fromFile, *toFile;
  cFileName *fromFileName, *toFileName;
  cIndexFile *fromIndex, *toIndex;
  cMarks fromMarks, toMarks;
  uchar *PATPMT;
  void CheckTS(cUnbufferedFile *replayFile);
protected:
  virtual void Action(void);
public:
  cCuttingThread(const char *FromFileName, const char *ToFileName);
  virtual ~cCuttingThread();
  const char *Error(void) { return error; }
  };

cCuttingThread::cCuttingThread(const char *FromFileName, const char *ToFileName)
:cThread("video cutting")
{
  error = NULL;
  fromFile = toFile = NULL;
  fromFileName = toFileName = NULL;
  fromIndex = toIndex = NULL;
  if (fromMarks.Load(FromFileName) && fromMarks.Count()) {
     fromFileName = new cFileName(FromFileName, false, true);
     toFileName = new cFileName(ToFileName, true, true);
     fromIndex = new cIndexFile(FromFileName, false);
     toIndex = new cIndexFile(ToFileName, true);
     toMarks.Load(ToFileName); // doesn't actually load marks, just sets the file name
     Start();
     }
  else
     esyslog("no editing marks found for %s", FromFileName);
}

cCuttingThread::~cCuttingThread()
{
  Cancel(3);
  if( PATPMT != NULL )
     free(PATPMT);
  delete fromFileName;
  delete toFileName;
  delete fromIndex;
  delete toIndex;
}

void cCuttingThread::CheckTS(cUnbufferedFile *replayFile)
{
   uchar * pp = NULL;
   pp = MALLOC(uchar, 2*TS_SIZE);
   if ( pp != NULL) {
      if ( replayFile->Read( pp, 2*TS_SIZE ) == 2*TS_SIZE ) {
         if ( (*pp == 0x47 && *(pp+1) == 0x40) && (*(pp+TS_SIZE) == 0x47 && *(pp+TS_SIZE+1) == 0x40)) {
            PRINTF("cCuttingThread::CheckTS(): TS recognized\n");
            PATPMT = pp;
         } else {
            PRINTF("cCuttingThread::CheckTS(): PES recognized\n");
            PATPMT = NULL;
            free(pp);
         }
      }
      if (PATPMT == NULL)
         replayFile->Seek(0,SEEK_SET);
   }
}

void cCuttingThread::Action(void)
{
  {
    sched_param tmp;
    tmp.sched_priority = CUTTER_PRIORITY;
    if(!pthread_setschedparam(pthread_self(), SCHED_OTHER, &tmp))
      PRINTF("cCuttingThread::Action: cant set priority\n");
  }

  int bytes = 0;
  int __attribute__((unused)) burst_size = CUTTER_MAX_BANDWIDTH * CUTTER_TIMESLICE / 1000; // max bytes/timeslice 
  cTimeMs __attribute__((unused)) t;

  cMark *Mark = fromMarks.First();
  if (Mark) {
     fromFile = fromFileName->Open();
     toFile = toFileName->Open();
     if (!fromFile || !toFile)
        return;
     CheckTS(fromFile);
     fromFile->SetReadAhead(MEGABYTE(20));
     int Index = Mark->position;
     Mark = fromMarks.Next(Mark);
     int FileSize = 0;
     int CurrentFileNumber = 0;
     int LastIFrame = 0;
     toMarks.Add(0);
     toMarks.Save();
     uchar buffer[MAXFRAMESIZE];
     bool LastMark = false;
     bool cutIn = true;
     while (Running()) {
           uchar FileNumber;
           int FileOffset, Length;
           uchar PictureType;

           // Make sure there is enough disk space:

           AssertFreeDiskSpace(-1);

           // Read one frame:

           if (fromIndex->Get(Index++, &FileNumber, &FileOffset, &PictureType, &Length)) {
              if (FileNumber != CurrentFileNumber) {
                 fromFile = fromFileName->SetOffset(FileNumber, FileOffset);
                 fromFile->SetReadAhead(MEGABYTE(20));
                 CurrentFileNumber = FileNumber;
                 }
              if (fromFile) {
                 int len = ReadFrame(fromFile, buffer,  Length, sizeof(buffer));
                 if (len < 0) {
                    error = "ReadFrame";
                    break;
                    }
                 if (len != Length) {
                    CurrentFileNumber = 0; // this re-syncs in case the frame was larger than the buffer
                    Length = len;
                    }
                 }
              else {
                 error = "fromFile";
                 break;
                 }
              }
           else
              break;

           // Write one frame:

           if (PictureType == I_FRAME || PATPMT != NULL) { // every file shall start with an I_FRAME
              if (LastMark) // edited version shall end before next I-frame
                 break;
              if (FileSize == 0) {
                 if (PATPMT != NULL) {
                    if (toFile->Write(PATPMT, 2*TS_SIZE) < 0) // Add PATPMT to start of every file
                       break;
                    else
                       FileSize+=TS_SIZE*2;
                    }   
                 }
              else if (FileSize > MEGABYTE(Setup.MaxVideoFileSize)) {
                 toFile = toFileName->NextFile();
                 if (!toFile) {
                    error = "toFile 1";
                    break;
                    }
                 FileSize = 0;
                 if (PATPMT != NULL) {
                    if (toFile->Write(PATPMT, 2*TS_SIZE) < 0) // Add PATPMT to start of every file
                       break;
                    else
                       FileSize+=TS_SIZE*2;
                    }
                 }
              LastIFrame = 0;

              if (cutIn) {
                 cRemux::SetBrokenLink(buffer, Length);
                 cutIn = false;
                 }
              }
           if (toFile->Write(buffer, Length) < 0) {
              error = "safe_write";
              break;
              }
           if (!toIndex->Write(PictureType, toFileName->Number(), FileSize)) {
              error = "toIndex";
              break;
              }
           FileSize += Length;
           if (!LastIFrame)
              LastIFrame = toIndex->Last();

           // Check editing marks:

           if (Mark && Index >= Mark->position) {
              Mark = fromMarks.Next(Mark);
              toMarks.Add(LastIFrame);
              if (Mark)
                 toMarks.Add(toIndex->Last() + 1);
              toMarks.Save();
              if (Mark) {
                 Index = Mark->position;
                 Mark = fromMarks.Next(Mark);
                 CurrentFileNumber = 0; // triggers SetOffset before reading next frame
                 cutIn = true;
                 if (Setup.SplitEditedFiles) {
                    toFile = toFileName->NextFile();
                    if (!toFile) {
                       error = "toFile 2";
                       break;
                       }
                    FileSize = 0;
                    }
                 }
              else
                 LastMark = true;
              }

	   bytes += Length;
	   if(bytes >= burst_size) {
	     int elapsed = t.Elapsed();
	     int sleep = 0;
	     
#if CUTTER_REL_BANDWIDTH > 0 &&  CUTTER_REL_BANDWIDTH < 100
	     // stay under max. relative bandwidth

	     sleep = (elapsed * 100 / CUTTER_REL_BANDWIDTH) - elapsed;
	     //if(sleep<=0 && elapsed<=2) sleep = 1; 
	     //if(sleep) esyslog("cutter: relative bandwidth limit, sleep %d ms (chunk %dk / %dms)", sleep, burst_size/1024, CUTTER_TIMESLICE);
#endif
	     // stay under max. absolute bandwidth
	     if(elapsed < CUTTER_TIMESLICE) {
	       sleep = max(CUTTER_TIMESLICE - elapsed, sleep);
	       //if(sleep) esyslog("cutter: absolute bandwidth limit, sleep %d ms (chunk %dk / %dms)", sleep, burst_size/1024, CUTTER_TIMESLICE);
	     }

	     if(sleep>0)
	       cCondWait::SleepMs(sleep);
	     t.Set();
	     bytes = 0;
	   }

           }
     Recordings.TouchUpdate();
     }
  else
     esyslog("no editing marks found!");
}

// --- cCutter ---------------------------------------------------------------

char *cCutter::editedVersionName = NULL;
cCuttingThread *cCutter::cuttingThread = NULL;
bool cCutter::error = false;
bool cCutter::ended = false;

bool cCutter::Start(const char *FileName)
{
  if (!cuttingThread) {
     error = false;
     ended = false;
     cRecording Recording(FileName);
     
     cMarks FromMarks;
     FromMarks.Load(FileName);
     cMark *First=FromMarks.First();
     if (First) Recording.SetStartTime(Recording.start+((First->position/FRAMESPERSEC+30)/60)*60);
     
     const char *evn = Recording.PrefixFileName('%');
     if (evn && RemoveVideoFile(evn) && MakeDirs(evn, true)) {
        // XXX this can be removed once RenameVideoFile() follows symlinks (see videodir.c)
        // remove a possible deleted recording with the same name to avoid symlink mixups:
        char *s = strdup(evn);
        char *e = strrchr(s, '.');
        if (e) {
           if (strcmp(e, ".rec") == 0) {
              strcpy(e, ".del");
              RemoveVideoFile(s);
              }
           }
        free(s);
        // XXX
        editedVersionName = strdup(evn);
        Recording.WriteInfo();
        Recordings.AddByName(editedVersionName, false);
        cuttingThread = new cCuttingThread(FileName, editedVersionName);
        return true;
        }
     }
  return false;
}

void cCutter::Stop(void)
{
  bool Interrupted = cuttingThread && cuttingThread->Active();
  const char *Error = cuttingThread ? cuttingThread->Error() : NULL;
  delete cuttingThread;
  cuttingThread = NULL;
  if ((Interrupted || Error) && editedVersionName) {
     if (Interrupted)
        isyslog("editing process has been interrupted");
     if (Error)
        esyslog("ERROR: '%s' during editing process", Error);
     RemoveVideoFile(editedVersionName); //XXX what if this file is currently being replayed?
     Recordings.DelByName(editedVersionName);
     }
}

bool cCutter::Active(void)
{
  if (cuttingThread) {
     if (cuttingThread->Active())
        return true;
     error = cuttingThread->Error();
     Stop();
     if (!error)
        cRecordingUserCommand::InvokeCommand(RUC_EDITEDRECORDING, editedVersionName);
     free(editedVersionName);
     editedVersionName = NULL;
     ended = true;
     }
  return false;
}

bool cCutter::Error(void)
{
  bool result = error;
  error = false;
  return result;
}

bool cCutter::Ended(void)
{
  bool result = ended;
  ended = false;
  return result;
}
