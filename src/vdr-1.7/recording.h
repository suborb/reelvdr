/*
 * recording.h: Recording file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recording.h 2.25 2011/08/21 13:10:39 kls Exp $
 */

#ifndef __RECORDING_H
#define __RECORDING_H

#include <time.h>
#include "channels.h"
#include "config.h"
#include "epg.h"
#include "thread.h"
#include "timers.h"
#include "tools.h"

#define FOLDERDELIMCHAR '~'
#define TIMERMACRO_TITLE    "TITLE"
#define TIMERMACRO_EPISODE  "EPISODE"

//#define __RECORDING_H_DEPRECATED_DIRECT_MEMBER_ACCESS // Code enclosed with this macro is deprecated and may be removed in a future version

extern bool VfatFileSystem;
#ifdef USE_LIEMIEXT
extern bool DirOrderState;
#endif /* LIEMIEXT */
extern int InstanceId;

void RemoveDeletedRecordings(void);
void AssertFreeDiskSpace(int Priority = 0, bool Force = false);
     ///< The special Priority value -1 means that we shall get rid of any
     ///< deleted recordings faster than normal (because we're cutting).
     ///< If Force is true, the check will be done even if the timeout
     ///< hasn't expired yet.

class cResumeFile {
private:
  char *fileName;
  bool isPesRecording;
public:
  cResumeFile(const char *FileName, bool IsPesRecording);
  ~cResumeFile();
  int Read(void);
  bool Save(int Index);
  void Delete(void);
  };

class cRecordingInfo {
  friend class cRecording;
private:
  tChannelID channelID;
  char *channelName;
  const cEvent *event;
  cEvent *ownEvent;
  char *aux;
  double framesPerSecond;
  int priority;
  int lifetime;
  char *fileName;
  cRecordingInfo(const cChannel *Channel = NULL, const cEvent *Event = NULL);
  bool Read(FILE *f);
  void SetData(const char *Title, const char *ShortText, const char *Description);
  void SetAux(const char *Aux);
#ifdef REELVDR
  bool isHD; //TB
  bool isCompressed;
#endif
public:
  cRecordingInfo(const char *FileName);
  ~cRecordingInfo();
  tChannelID ChannelID(void) const { return channelID; }
  const char *ChannelName(void) const { return channelName; }
#ifdef REELVDR
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
#endif
  const cEvent *GetEvent(void) const { return event; }
  const char *Title(void) const { return event->Title(); }
  const char *ShortText(void) const { return event->ShortText(); }
#ifdef USE_GRAPHTFT
  tEventID EventID(void) const { return event->EventID(); }
#endif /* GRAPHTFT */
  const char *Description(void) const { return event->Description(); }
  const cComponents *Components(void) const { return event->Components(); }
  const char *Aux(void) const { return aux; }
#ifdef REELVDR
  void SetIsHD(bool IsHD) { isHD = IsHD; }; //TB
  bool IsHD(void) const { return isHD; } //TB
  void SetIsCompressed(bool IsCompressed) { isCompressed = IsCompressed; };
  bool IsCompressed(void) const { return isCompressed; } 
#endif
  double FramesPerSecond(void) const { return framesPerSecond; }
  void SetFramesPerSecond(double FramesPerSecond);
  bool Write(FILE *f, const char *Prefix = "") const;
  bool Read(void);
  bool Write(void) const;
  };

class cRecording : public cListObject {
  friend class cRecordings;
private:
  mutable int resume;
  mutable char *titleBuffer;
  mutable char *sortBuffer;
  mutable char *fileName;
  mutable char *baseDir;
  mutable char *name;
  mutable int fileSizeMB;
  mutable int numFrames;
  int channel;
  int instanceId;
  bool isPesRecording;
  double framesPerSecond;
  cRecordingInfo *info;
  cRecording(const cRecording&); // can't copy cRecording
  cRecording &operator=(const cRecording &); // can't assign cRecording
  static char *StripEpisodeName(char *s);
  char *SortName(void) const;
  int GetResume(void) const;

#ifdef __RECORDING_H_DEPRECATED_DIRECT_MEMBER_ACCESS
public:
#endif
  time_t start;
  int priority;
  int lifetime;
  time_t deleted;
public:
  cRecording(cTimer *Timer, const cEvent *Event);

#ifdef REELVDR
  cRecording(const char *FileName, const char*BaseDir=NULL);
#else
  cRecording(const char *FileName);
#endif

  virtual ~cRecording();
  time_t Start(void) const { return start; }
  int Priority(void) const { return priority; }
  int Lifetime(void) const { return lifetime; }
  time_t Deleted(void) const { return deleted; }
  virtual int Compare(const cListObject &ListObject) const;
  const char *Name(void) const { return name; }

#ifdef REELVDR
  const char *BaseDir(void) const { return baseDir; }
  void SetTitle(const char *Title, const char *ShortText);
#endif

  const char *FileName(void) const;
#ifdef USE_LIEMIEXT
  const char *Title(char Delimiter = ' ', bool NewIndicator = false, int Level = -1, bool Original = true) const;
#else
  const char *Title(char Delimiter = ' ', bool NewIndicator = false, int Level = -1) const;
#endif /* LIEMIEXT */
  const cRecordingInfo *Info(void) const { return info; }
  const char *PrefixFileName(char Prefix);
  int HierarchyLevels(void) const;
  void ResetResume(void) const;
  double FramesPerSecond(void) const { return framesPerSecond; }
  int NumFrames(void) const;
       ///< Returns the number of frames in this recording.
       ///< If the number of frames is unknown, -1 will be returned.
  int LengthInSeconds(void) const;
       ///< Returns the length (in seconds) of this recording, or -1 in case of error.
#ifdef REELVDR
  bool IsNew(void) const { return GetResume() == -1; } // New only if no 'resume' file, as in reelvdr 1.4
  bool IsCompressed(void) const { return info->IsCompressed(); }
#else
  bool IsNew(void) const { return GetResume() <= 0; }
#endif
  bool IsEdited(void) const;
  bool IsPesRecording(void) const { return isPesRecording; }
  void ReadInfo(void);
#ifdef REELVDR
  void ResetResumeToNew(void) const;
  void SetIsHD(bool Hd) { if (info) info->SetIsHD(Hd); }
  bool IsHD(void) const { return info->IsHD(); }
#endif
  bool WriteInfo(void);
  void SetStartTime(time_t Start);
       ///< Sets the start time of this recording to the given value.
       ///< If a filename has already been set for this recording, it will be
       ///< deleted and a new one will be generated (using the new start time)
       ///< at the next call to FileName().
       ///< Use this function with care - it does not check whether a recording with
       ///< this new name already exists, and if there is one, results may be
       ///< unexpected!
  bool Delete(void);
       ///< Changes the file name so that it will no longer be visible in the "Recordings" menu
       ///< Returns false in case of error
  bool Remove(void);
       ///< Actually removes the file from the disk
       ///< Returns false in case of error
  bool Undelete(void);
       ///< Changes the file name so that it will be visible in the "Recordings" menu again and
       ///< not processed by cRemoveDeletedRecordingsThread.
       ///< Returns false in case of error
#ifdef USE_LIEMIEXT
  bool Rename(const char *newName);
       ///< Changes the file name
       ///< Returns false in case of error
#endif /* LIEMIEXT */
  };

class cRecordings : public cList<cRecording>, public cThread {
private:
  static char *updateFileName;
  bool deleted;
  time_t lastUpdate;
  int state;
  const char *UpdateFileName(void);
  void Refresh(bool Foreground = false);
  void ScanVideoDir(const char *DirName, bool Foreground = false, int LinkLevel = 0);
protected:
  void Action(void);
public:
  cRecordings(bool Deleted = false);
  virtual ~cRecordings();
  bool Load(void) { return Update(true); }
       ///< Loads the current list of recordings and returns true if there
       ///< is anything in it (for compatibility with older plugins - use
       ///< Update(true) instead).
  bool Update(bool Wait = false);
       ///< Triggers an update of the list of recordings, which will run
       ///< as a separate thread if Wait is false. If Wait is true, the
       ///< function returns only after the update has completed.
       ///< Returns true if Wait is true and there is anyting in the list
       ///< of recordings, false otherwise.
  void TouchUpdate(void);
       ///< Touches the '.update' file in the video directory, so that other
       ///< instances of VDR that access the same video directory can be triggered
       ///< to update their recordings list.
  bool NeedsUpdate(void);
  void ChangeState(void) { state++; }
  bool StateChanged(int &State);
  void ResetResume(const char *ResumeFileName = NULL);
  cRecording *GetByName(const char *FileName);
  void AddByName(const char *FileName, bool TriggerUpdate = true);
  void DelByName(const char *FileName);
  void UpdateByName(const char *FileName);
  int TotalFileSizeMB(void); ///< Only for deleted recordings!
  };

extern cRecordings Recordings;
extern cRecordings DeletedRecordings;

#define DEFAULTFRAMESPERSECOND 25.0

class cMark : public cListObject {
  friend class cMarks; // for sorting
private:
  double framesPerSecond;
#ifdef __RECORDING_H_DEPRECATED_DIRECT_MEMBER_ACCESS
public:
#endif
  int position;
  cString comment;
public:
  cMark(int Position = 0, const char *Comment = NULL, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
  virtual ~cMark();
  int Position(void) const { return position; }
  const char *Comment(void) const { return comment; }
  void SetPosition(int Position) { position = Position; }
  void SetComment(const char *Comment) { comment = Comment; }
  cString ToText(void);
  bool Parse(const char *s);
  bool Save(FILE *f);
  };

class cMarks : public cConfig<cMark> {
private:
  cString fileName;
  double framesPerSecond;
  time_t nextUpdate;
  time_t lastFileTime;
  time_t lastChange;
public:
  bool Load(const char *RecordingFileName, double FramesPerSecond = DEFAULTFRAMESPERSECOND, bool IsPesRecording = false);
  bool Update(void);
  void Sort(void);
  cMark *Add(int Position);
  cMark *Get(int Position);
  cMark *GetPrev(int Position);
  cMark *GetNext(int Position);
  };

#ifdef USE_JUMPPLAY
class cMarksReload : public cMarks {
private:
  cString recDir;
  cTimeMs nextreload;
  time_t lastmodtime;
  static time_t lastsavetime;
public:
  cMarksReload(const char *RecordingFileName);
  bool Reload(void);
  bool Save(void);
  };

#endif /* JUMPPLAY */

#define RUC_BEFORERECORDING "before"
#define RUC_AFTERRECORDING  "after"
#define RUC_EDITEDRECORDING "edited"

class cRecordingUserCommand {
private:
  static const char *command;
public:
  static void SetCommand(const char *Command) { command = Command; }
#ifdef USE_DVLRECSCRIPTADDON
  static void InvokeCommand(const char *State, const char *RecordingFileName, char *chanName = NULL);
#else
  static void InvokeCommand(const char *State, const char *RecordingFileName);
#endif /* DVLRECSCRIPTADDON */
  };

// The maximum size of a single frame (up to HDTV 1920x1080):
#define MAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets

// The maximum file size is limited by the range that can be covered
// with a 40 bit 'unsigned int', which is 1TB. The actual maximum value
// used is 6MB below the theoretical maximum, to have some safety (the
// actual file size may be slightly higher because we stop recording only
// before the next independent frame, to have a complete Group Of Pictures):
#define MAXVIDEOFILESIZETS  1048570 // MB
#define MAXVIDEOFILESIZEPES    2000 // MB
#ifdef USE_HARDLINKCUTTER
#define MINVIDEOFILESIZE          1 // MB
#else
#define MINVIDEOFILESIZE        100 // MB
#endif /* HARDLINKCUTTER */
#define MAXVIDEOFILESIZEDEFAULT MAXVIDEOFILESIZEPES

#ifdef USE_HARDLINKCUTTER
#define MINRECORDINGSIZE      25 // GB
#define MAXRECORDINGSIZE     500 // GB
#define DEFAULTRECORDINGSIZE 100 // GB
// Dynamic recording size:
// Keep recording file size at Setup.MaxVideoFileSize for as long as possible,
// but switch to MAXVIDEOFILESIZE early enough, so that Setup.MaxRecordingSize
// will be reached, before recording to file 65535.vdr
#endif /* HARDLINKCUTTER */

struct tIndexTs;
class cIndexFileGenerator;

#ifdef USE_LIVEBUFFER
class cIndex {
public:
  virtual bool Ok(void) =0;
  virtual bool Write(bool Independent, uint16_t FileNumber, off_t FileOffset) =0;
  virtual bool Get(int Index, uint16_t *FileNumber, off_t *FileOffset, bool *Independent = NULL, int *Length = NULL) =0;
  virtual int GetNextIFrame(int Index, bool Forward, uint16_t *FileNumber = NULL, off_t *FileOffset = NULL, int *Length = NULL, bool StayOffEnd = false) =0;
  virtual int Get(uint16_t FileNumber, off_t FileOffset) =0;
  virtual int First(void) {return 0;};
  virtual int Last(void) =0;
  virtual int GetResume(void) =0;
  virtual bool StoreResume(int Index) =0;
  virtual bool IsStillRecording(void) =0;
  virtual void Delete(void) =0;
  virtual void SetFramesPerSecond(double fps) {};
  virtual ~cIndex(){}
  };

class cIndexFile : public cIndex {
#else
class cIndexFile {
#endif /*USE_LIVEBUFFER*/
private:
  int f;
  cString fileName;
  int size, last;
  tIndexTs *index;
  bool isPesRecording;
  cResumeFile resumeFile;
  cIndexFileGenerator *indexFileGenerator;
  cMutex mutex;
  static cString IndexFileName(const char *FileName, bool IsPesRecording);
  void ConvertFromPes(tIndexTs *IndexTs, int Count);
  void ConvertToPes(tIndexTs *IndexTs, int Count);
  bool CatchUp(int Index = -1);
public:
  cIndexFile(const char *FileName, bool Record, bool IsPesRecording = false);
  ~cIndexFile();
  bool Ok(void) { return index != NULL; }
  bool Write(bool Independent, uint16_t FileNumber, off_t FileOffset);
  bool Get(int Index, uint16_t *FileNumber, off_t *FileOffset, bool *Independent = NULL, int *Length = NULL);
  int GetNextIFrame(int Index, bool Forward, uint16_t *FileNumber = NULL, off_t *FileOffset = NULL, int *Length = NULL, bool StayOffEnd = false);
  int Get(uint16_t FileNumber, off_t FileOffset);
  int Last(void) { CatchUp(); return last; }
  int GetResume(void) { return resumeFile.Read(); }
  bool StoreResume(int Index) { return resumeFile.Save(Index); }
  bool IsStillRecording(void);
  void Delete(void);
  static int GetLength(const char *FileName, bool IsPesRecording = false);
       ///< Calculates the recording length (numer of frames) without actually reading the index file.
       ///< Returns -1 in case of error.
#if 0 //def USE_LIEMIEXT //TODO: change Length->GetLength and eliminate this ifdef
  static int Length(const char *FileName, bool IsPesRecording = false);
       ///< Calculates the recording length without reading the index.
#endif /* LIEMIEXT */
  };

class cFileName {
private:
  cUnbufferedFile *file;
  uint16_t fileNumber;
  char *fileName, *pFileNumber;
  bool record;
  bool blocking;
  bool isPesRecording;
public:
  cFileName(const char *FileName, bool Record, bool Blocking = false, bool IsPesRecording = false);
  ~cFileName();
  const char *Name(void) { return fileName; }
  uint16_t Number(void) { return fileNumber; }
  bool GetLastPatPmtVersions(int &PatVersion, int &PmtVersion);
  cUnbufferedFile *Open(void);
  void Close(void);
  cUnbufferedFile *SetOffset(int Number, off_t Offset = 0); // yes, Number is int for easier internal calculating
#ifdef USE_HARDLINKCUTTER
  off_t MaxFileSize();
      // Dynamic file size for this file
#endif /* HARDLINKCUTTER */
  cUnbufferedFile *NextFile(void);
  };

cString IndexToHMSF(int Index, bool WithFrame = false, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
      // Converts the given index to a string, optionally containing the frame number.
int HMSFToIndex(const char *HMSF, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
      // Converts the given string (format: "hh:mm:ss.ff") to an index.
int SecondsToFrames(int Seconds, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
      // Returns the number of frames corresponding to the given number of seconds.

int ReadFrame(cUnbufferedFile *f, uchar *b, int Length, int Max);

char *ExchangeChars(char *s, bool ToFileSystem);
      // Exchanges the characters in the given string to or from a file system
      // specific representation (depending on ToFileSystem). The given string will
      // be modified and may be reallocated if more space is needed. The return
      // value points to the resulting string, which may be different from s.

bool GenerateIndex(const char *FileName);

#endif //__RECORDING_H
