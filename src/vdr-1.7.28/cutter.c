/*
 * cutter.c: The video cutting facilities
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: cutter.c 2.12 2012/06/02 13:46:55 kls Exp $
 */

#include "cutter.h"
#include "menu.h"
#include "recording.h"
#include "remux.h"
#include "videodir.h"

// --- cCuttingThread --------------------------------------------------------

#ifdef USE_CUTTERLIMIT
#ifndef CUTTER_MAX_BANDWIDTH
#define CUTTER_MAX_BANDWIDTH MEGABYTE(10) // 10 MB/s
#endif
#ifndef CUTTER_REL_BANDWIDTH
#define CUTTER_REL_BANDWIDTH 75 // %
#endif
#ifndef CUTTER_PRIORITY
#define CUTTER_PRIORITY sched_get_priority_min(SCHED_OTHER)
#endif
#define CUTTER_TIMESLICE   100   // ms
#endif /* CUTTERLIMIT */

class cCuttingThread : public cThread {
private:
  const char *error;
  bool isPesRecording;
  cUnbufferedFile *fromFile, *toFile;
  cFileName *fromFileName, *toFileName;
  cIndexFile *fromIndex, *toIndex;
  cMarks fromMarks, toMarks;
  off_t maxVideoFileSize;
#ifdef REELVDR
  uchar *GetPATPMT(cUnbufferedFile *file); // Check if we have an old ts recording
  uchar *PATPMT;
  #define PATPMT_SIZE (2*TS_SIZE)
#endif /*REELVDR*/
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
  cRecording Recording(FromFileName);
  isPesRecording = Recording.IsPesRecording();
#ifdef REELVDR
  PATPMT = NULL;
#endif /*REELVDR*/
  if (fromMarks.Load(FromFileName, Recording.FramesPerSecond(), isPesRecording) && fromMarks.Count()) {
     fromFileName = new cFileName(FromFileName, false, true, isPesRecording);
     toFileName = new cFileName(ToFileName, true, true, isPesRecording);
     fromIndex = new cIndexFile(FromFileName, false, isPesRecording);
     toIndex = new cIndexFile(ToFileName, true, isPesRecording);
     toMarks.Load(ToFileName, Recording.FramesPerSecond(), isPesRecording); // doesn't actually load marks, just sets the file name
     maxVideoFileSize = MEGABYTE(Setup.MaxVideoFileSize);
     if (isPesRecording && maxVideoFileSize > MEGABYTE(MAXVIDEOFILESIZEPES))
        maxVideoFileSize = MEGABYTE(MAXVIDEOFILESIZEPES);
     Start();
     }
  else
     esyslog("no editing marks found for %s", FromFileName);
}

cCuttingThread::~cCuttingThread()
{
  Cancel(3);
#ifdef REELVDR
  if(PATPMT) free(PATPMT);
#endif /*REELVDR*/
  delete fromFileName;
  delete toFileName;
  delete fromIndex;
  delete toIndex;
}

#ifdef REELVDR
uchar *cCuttingThread::GetPATPMT(cUnbufferedFile *file) {
  if(!isPesRecording) return NULL;
  uchar *ret = (uchar *)malloc(PATPMT_SIZE);
  if(!ret) goto failed;
  if(file->Read(ret, PATPMT_SIZE) != PATPMT_SIZE) goto failed;
  if(ret[0] != 0x47 || ret[1] != 0x40 || ret[TS_SIZE] != 0x47 || ret[TS_SIZE+1] != 0x40) goto failed;
  isyslog("-> detected old ts file");
  return ret;
failed:
  if(ret) free(ret);
  file->Seek(0, SEEK_SET);
  return NULL;
} // cCuttingThread::GetPATPMT
#endif /*REELVDR*/

void cCuttingThread::Action(void)
{
#ifdef USE_CUTTERLIMIT
#ifdef USE_HARDLINKCUTTER
  if (!Setup.HardLinkCutter)
#endif /* HARDLINKCUTTER */
  {
    sched_param tmp;
    tmp.sched_priority = CUTTER_PRIORITY;
    if(!pthread_setschedparam(pthread_self(), SCHED_OTHER, &tmp))
      printf("cCuttingThread::Action: cant set priority\n");
  }

  int bytes = 0;
  int __attribute__((unused)) burst_size = CUTTER_MAX_BANDWIDTH * CUTTER_TIMESLICE / 1000; // max bytes/timeslice
  cTimeMs __attribute__((unused)) t;
#endif /* CUTTERLIMIT */

  cMark *Mark = fromMarks.First();
  if (Mark) {
     SetPriority(19);
     SetIOPriority(7);
     fromFile = fromFileName->Open();
     toFile = toFileName->Open();
     if (!fromFile || !toFile)
        return;
#if REELVDR
     PATPMT = GetPATPMT(fromFile);
#endif /*REELVDR*/
     fromFile->SetReadAhead(MEGABYTE(20));
     int Index = Mark->Position();
     Mark = fromMarks.Next(Mark);
     off_t FileSize = 0;
     int CurrentFileNumber = 0;
#ifdef USE_HARDLINKCUTTER
     bool SkipThisSourceFile = false;
#endif /* HARDLINKCUTTER */
     int LastIFrame = 0;
     toMarks.Add(0);
     toMarks.Save();
     uchar buffer[MAXFRAMESIZE];
     bool LastMark = false;
     bool cutIn = true;
     while (Running()) {
           uint16_t FileNumber;
           off_t FileOffset;
           int Length;
           bool Independent;

           // Make sure there is enough disk space:

           AssertFreeDiskSpace(-1);

           // Read one frame:

#ifdef USE_HARDLINKCUTTER
           if (!fromIndex->Get(Index++, &FileNumber, &FileOffset, &Independent, &Length)) {
              // Error, unless we're past last cut-in and there's no cut-out
              if (Mark || LastMark)
                 error = "index";
              break;
              }

           if (FileNumber != CurrentFileNumber) {
              fromFile = fromFileName->SetOffset(FileNumber, FileOffset);
              fromFile->SetReadAhead(MEGABYTE(20));
              CurrentFileNumber = FileNumber;
              if (SkipThisSourceFile) {
                 // At end of fast forward: Always skip to next file
                 toFile = toFileName->NextFile();
                 if (!toFile) {
                    error = "toFile 4";
                    break;
                    }
                 FileSize = 0;
                 SkipThisSourceFile = false;
                 }


              if (Setup.HardLinkCutter && FileOffset == 0) {
                 // We are at the beginning of a new source file.
                 // Do we need to copy the whole file?

                 // if !Mark && LastMark, then we're past the last cut-out and continue to next I-frame
                 // if !Mark && !LastMark, then there's just a cut-in, but no cut-out
                 // if Mark, then we're between a cut-in and a cut-out

                 uint16_t MarkFileNumber;
                 off_t MarkFileOffset;
                 // Get file number of next cut mark
                 if (!Mark && !LastMark
                     || Mark
                        && fromIndex->Get(Mark->position, &MarkFileNumber, &MarkFileOffset)
                        && (MarkFileNumber != CurrentFileNumber)) {
                    // The current source file will be copied completely.
                    // Start new output file unless we did that already
                    if (FileSize != 0) {
                       toFile = toFileName->NextFile();
                       if (!toFile) {
                          error = "toFile 3";
                          break;
                          }
                       FileSize = 0;
                       }

                    // Safety check that file has zero size
                    struct stat buf;
                    if (stat(toFileName->Name(), &buf) == 0) {
                       if (buf.st_size != 0) {
                          esyslog("cCuttingThread: File %s exists and has nonzero size", toFileName->Name());
                          error = "nonzero file exist";
                          break;
                          }
                       }
                    else if (errno != ENOENT) {
                       esyslog("cCuttingThread: stat failed on %s", toFileName->Name());
                       error = "stat";
                       break;
                       }

                    // Clean the existing 0-byte file
                    toFileName->Close();
                    cString ActualToFileName(ReadLink(toFileName->Name()), true);
                    unlink(ActualToFileName);
                    unlink(toFileName->Name());

                    // Try to create a hard link
                    if (HardLinkVideoFile(fromFileName->Name(), toFileName->Name())) {
                       // Success. Skip all data transfer for this file
                       SkipThisSourceFile = true;
                       cutIn = false;
                       toFile = NULL; // was deleted by toFileName->Close()
                       }
                    else {
                       // Fallback: Re-open the file if necessary
                       toFile = toFileName->Open();
                       }
                    }
                 }
              }

           if (!SkipThisSourceFile) {
#else
           if (fromIndex->Get(Index++, &FileNumber, &FileOffset, &Independent, &Length)) {
              if (FileNumber != CurrentFileNumber) {
                 fromFile = fromFileName->SetOffset(FileNumber, FileOffset);
                 if (fromFile)
                    fromFile->SetReadAhead(MEGABYTE(20));
                 CurrentFileNumber = FileNumber;
                 }
#endif /* HARDLINKCUTTER */
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
#ifndef USE_HARDLINKCUTTER
           else {
              // Error, unless we're past the last cut-in and there's no cut-out
              if (Mark || LastMark)
                 error = "index";
              break;
              }
#endif /* HARDLINKCUTTER */

           // Write one frame:

           if (Independent) { // every file shall start with an independent frame
              if (LastMark) // edited version shall end before next I-frame
                 break;
#ifdef USE_HARDLINKCUTTER
              if (!SkipThisSourceFile && FileSize > toFileName->MaxFileSize()) {
#else
              if (FileSize > maxVideoFileSize) {
#endif /* HARDLINKCUTTER */
                 toFile = toFileName->NextFile();
                 if (!toFile) {
                    error = "toFile 1";
                    break;
                    }
                 FileSize = 0;
                 }
              LastIFrame = 0;

#ifdef USE_HARDLINKCUTTER
              if (!SkipThisSourceFile && cutIn) {
#else
              if (cutIn) {
#endif /* HARDLINKCUTTER */
#ifdef REELVDR
                 if (isPesRecording && !PATPMT)
                    cRemux::SetBrokenLink(buffer, Length);
                 else
                    TsSetTeiOnBrokenPackets(buffer, Length);
#else
                 if (isPesRecording)
                    cRemux::SetBrokenLink(buffer, Length);
                 else
                    TsSetTeiOnBrokenPackets(buffer, Length);
#endif
                 cutIn = false;
                 }
              }
#ifdef REELVDR
           if(PATPMT && !FileSize) {
              if(toFile->Write(PATPMT,PATPMT_SIZE) < 0) {
                 error = "patpmt_write";
                 break;
              } // if
              FileSize += PATPMT_SIZE;
           } // if
#endif
#ifdef USE_HARDLINKCUTTER
           if (!SkipThisSourceFile && toFile->Write(buffer, Length) < 0) {
#else
           if (toFile->Write(buffer, Length) < 0) {
#endif /* HARDLINKCUTTER */
              error = "safe_write";
              break;
              }
           if (!toIndex->Write(Independent, toFileName->Number(), FileSize)) {
              error = "toIndex";
              break;
              }
           FileSize += Length;
           if (!LastIFrame)
              LastIFrame = toIndex->Last();

           // Check editing marks:

           if (Mark && Index >= Mark->Position()) {
              Mark = fromMarks.Next(Mark);
              toMarks.Add(LastIFrame);
              if (Mark)
                 toMarks.Add(toIndex->Last() + 1);
              toMarks.Save();
              if (Mark) {
                 Index = Mark->Position();
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
#ifdef USE_HARDLINKCUTTER
                 LastMark = true; // After last cut-out: Write on until next I-frame, then exit
#else
                 LastMark = true;
#endif /* HARDLINKCUTTER */
              }

#ifdef USE_CUTTERLIMIT
#ifdef USE_HARDLINKCUTTER
           if (!Setup.HardLinkCutter) {
#endif /* HARDLINKCUTTER */
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
#ifdef USE_HARDLINKCUTTER
              }
#endif /* HARDLINKCUTTER */
#endif /* CUTTERLIMIT */

           }
     Recordings.TouchUpdate();
     }
  else
     esyslog("no editing marks found!");
}

// --- cCutter ---------------------------------------------------------------

cMutex cCutter::mutex;
cString cCutter::originalVersionName;
cString cCutter::editedVersionName;
cCuttingThread *cCutter::cuttingThread = NULL;
bool cCutter::error = false;
bool cCutter::ended = false;

bool cCutter::Start(const char *FileName)
{
  cMutexLock MutexLock(&mutex);
  if (!cuttingThread) {
     error = false;
     ended = false;
     originalVersionName = FileName;
     cRecording Recording(FileName);
     cMarks FromMarks;
     FromMarks.Load(FileName, Recording.FramesPerSecond(), Recording.IsPesRecording());
     if (cMark *First = FromMarks.First())
        Recording.SetStartTime(Recording.Start() + (int(First->Position() / Recording.FramesPerSecond() + 30) / 60) * 60);
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
        editedVersionName = evn;
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
  cMutexLock MutexLock(&mutex);
  bool Interrupted = cuttingThread && cuttingThread->Active();
  const char *Error = cuttingThread ? cuttingThread->Error() : NULL;
  delete cuttingThread;
  cuttingThread = NULL;
  if ((Interrupted || Error) && *editedVersionName) {
     if (Interrupted)
        isyslog("editing process has been interrupted");
     if (Error)
        esyslog("ERROR: '%s' during editing process", Error);
     if (cReplayControl::NowReplaying() && strcmp(cReplayControl::NowReplaying(), editedVersionName) == 0)
        cControl::Shutdown();
     RemoveVideoFile(editedVersionName);
     Recordings.DelByName(editedVersionName);
     }
}

bool cCutter::Active(const char *FileName)
{
  cMutexLock MutexLock(&mutex);
  if (cuttingThread) {
     if (cuttingThread->Active())
        return !FileName || strcmp(FileName, originalVersionName) == 0 || strcmp(FileName, editedVersionName) == 0;
     error = cuttingThread->Error();
     Stop();
     if (!error)
        cRecordingUserCommand::InvokeCommand(RUC_EDITEDRECORDING, editedVersionName, originalVersionName);
     originalVersionName = NULL;
     editedVersionName = NULL;
     ended = true;
     }

  return false;
}

bool cCutter::Error(void)
{
  cMutexLock MutexLock(&mutex);
  bool result = error;
  error = false;
  return result;
}

bool cCutter::Ended(void)
{
  cMutexLock MutexLock(&mutex);
  bool result = ended;
  ended = false;
  return result;
}

#define CUTTINGCHECKINTERVAL 500 // ms between checks for the active cutting process

bool CutRecording(const char *FileName)
{
  if (DirectoryOk(FileName)) {
     cRecording Recording(FileName);
     if (Recording.Name()) {
        cMarks Marks;
        if (Marks.Load(FileName, Recording.FramesPerSecond(), Recording.IsPesRecording()) && Marks.Count()) {
           if (cCutter::Start(FileName)) {
              while (cCutter::Active())
                    cCondWait::SleepMs(CUTTINGCHECKINTERVAL);
              return true;
              }
           else
              fprintf(stderr, "can't start editing process\n");
           }
        else
           fprintf(stderr, "'%s' has no editing marks\n", FileName);
        }
     else
        fprintf(stderr, "'%s' is not a recording\n", FileName);
     }
  else
     fprintf(stderr, "'%s' is not a directory\n", FileName);
  return false;
}
