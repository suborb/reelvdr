#ifdef USE_LIVEBUFFER
#include "livebuffer.h"
#if VDRVERSNUM >= 10716

#include "videodir.h"
#include "recording.h"
#include "skins.h"
#include "player.h"

#include <sys/statfs.h>

#define WAIT_WRITING_COUNT 1000
#define WAIT_WRITING_SLEEP 10000

#define WAIT_TERMINATE_COUNT 300
#define WAIT_TERMINATE_SLEEP 10000

#define MINFREEDISKSPACE    (512) // MB (same as in recorder.c)
#define MINLIVEBUFFERSIZE   (512) // MB

#define DISKCHECKINTERVAL 1

struct tLiveIndex {
  int index;
  uint64_t offset:40; // up to 1TB per file (not using off_t here - must definitely be exactly 64 bit!)
  int reserved:7;     // reserved for future use
  int independent:1;  // marks frames that can be displayed by themselves (for trick modes)
  uint16_t number:16; // up to 64K files per recording
  tLiveIndex(int Index, bool Independent, uint16_t Number, off_t Offset)
  {
    index = Index;
    offset = Offset;
    reserved = 0;
    independent = Independent;
    number = Number;
  }
}; // tLiveIndex

class cLiveIndex : public cIndex {
public:
	cLiveIndex(const char *FileName): bufferFileName(FileName, false), bufferBaseName(FileName) {
		resumePos = -1;
		lastPos = lastGet = lastBuf = 0;
		lastFileNumber=1;
		dropFile = false;
		maxSize = Setup.LiveBufferSize * 60 * DEFAULTFRAMESPERSECOND;
		idx.reserve(maxSize+1);
	}; // cLiveIndex
	virtual ~cLiveIndex() {
	}; // ~cLiveIndex
	virtual void SetFramesPerSecond(double fps) {
		maxSize = Setup.LiveBufferSize * 60 * fps;
		idx.reserve(maxSize+1);
	} // SetFramesPerSecond
	virtual bool Write(bool Independent, uint16_t FileNumber, off_t FileOffset) {
		cMutexLock lock(&idx_lock);
		idx.push_back(tLiveIndex(++lastPos, Independent, FileNumber, FileOffset));
		while(((idx.size() > maxSize) && (lastGet ? (lastGet > First()) : true) && (lastBuf ? (lastBuf > First()) : true)) || dropFile) {
			if(idx.front().number != lastFileNumber) {
				isyslog("Deleting old livebuffer file #%d (%d)", lastFileNumber, dropFile);
				unlink(cString::sprintf("%s/%05d.ts", (const char *)bufferBaseName, lastFileNumber));
				lastFileNumber = idx.front().number;
				dropFile=false;
			} // if
			if(!idx.size()) break;
			idx.erase(idx.begin());
		} // if
		return true;
	}; // Write
	virtual bool Get(int Index, uint16_t *FileNumber, off_t *FileOffset, bool *Independent = NULL, int *Length = NULL) {
		cMutexLock lock(&idx_lock);
		std::vector<tLiveIndex>::iterator item = GetIndex(Index);
		if(item == idx.end()) return false;
		*FileNumber = item->number;
		*FileOffset = item->offset;
		if (Independent)
			*Independent = item->independent;
		item++;
		if(item == idx.end()) return false;
		if (Length) {
			uint16_t fn = item->number;
			off_t fo = item->offset;
			if (fn == *FileNumber)
				*Length = int(fo - *FileOffset);
			else
				*Length = -1; // this means "everything up to EOF" (the buffer's Read function will act accordingly)
		} // if
		lastGet = Index;
		return true;
	}; // Get
	virtual int GetNextIFrame(int Index, bool Forward, uint16_t *FileNumber = NULL, off_t *FileOffset = NULL, int *Length = NULL, bool StayOffEnd = false) {
		cMutexLock lock(&idx_lock);
		std::vector<tLiveIndex>::iterator item = GetIndex(Index);
		if(item == idx.end()) {
			if(Index < First() && Forward)
				item = idx.begin();
			else
				return -1;
		}
		if(Forward) {
			do {
				item++;
				if(item == idx.end()) return -1;
			} while(!item->independent);
		} else {
			do {
				if(item == idx.begin()) return -1;
				item--;
			} while(!item->independent);
		} // if
		uint16_t fn;
		if (!FileNumber)
			FileNumber = &fn;
		off_t fo;
		if (!FileOffset)
			FileOffset = &fo;
		*FileNumber = item->number;
		*FileOffset = item->offset;
		item++;
		if(item == idx.end()) return -1;
		if (Length) {
			// all recordings end with a non-independent frame, so the following should be safe:
			uint16_t fn = item->number;
			off_t fo = item->offset;
			if (fn == *FileNumber) {
				*Length = int(fo - *FileOffset);
			} else {
				esyslog("ERROR: 'I' frame at end of file #%d", *FileNumber);
				*Length = -1;
			} // if
		} // if
		return Index;
	}; // GetNextIFrame
	virtual bool SetBufferStart(int Frames) {
		cMutexLock lock(&idx_lock);
		abortBuf = false;
		if(Frames <= 0) {
			lastBuf = 0;
			return false;
		} // if
		lastBuf = Last()-Frames;
		if(lastBuf < First())
			lastBuf = First();
		lastBuf = GetNextIFrame(lastBuf, true);
		return true;
	} // SetBufferStart
	virtual cUnbufferedFile *GetNextBuffer(int &Length, bool &Independent) {
		if(abortBuf || !lastBuf) return NULL;
		cMutexLock lock(&idx_lock);
		std::vector<tLiveIndex>::iterator buff = GetIndex(lastBuf);
		if((buff == idx.end()) || ((buff+1) == idx.end())) return NULL;
		off_t offset = buff->offset;
		int number   = buff->number;
		cUnbufferedFile *ret = bufferFileName.SetOffset(number, offset);
		Independent = buff->independent;
		buff++;
		lastBuf = buff->index;
		if(number != buff->number)
			Length = -1;
		else
			Length = buff->offset-offset;
		return ret;
	} // GetNextBuffer
	virtual int Get(uint16_t FileNumber, off_t FileOffset) {
		for ( std::vector<tLiveIndex>::iterator item = idx.begin(); item != idx.end(); item++)
			if (item->number > FileNumber || ((item->number == FileNumber) && off_t(item->offset) >= FileOffset))
				return item->index;
		return lastPos;
	}; // Get
	virtual bool Ok(void)                    {return true;};
	virtual int  First(void)                 {return idx.size() ? idx.front().index : -1;};
	virtual int  Last(void)                  {return idx.size() ? idx.back().index  : -1;};
	virtual void SetResume(int Index)        {resumePos = lastGet = Index;};
	virtual int  GetResume(void)             {return resumePos;};
	virtual bool StoreResume(int Index)      {resumePos=Index; lastGet=0; return true;};
	virtual bool IsStillRecording(void)      {return true;};
	virtual void Delete(void)                {};
	virtual void DropFile(void)              {dropFile=true;};
	virtual bool IsWritingBuffer(void)       {return lastBuf != 0;};
	virtual void CancelWritingBuffer(void)   {abortBuf = true;};
	virtual bool WritingBufferCanceled(void) {return abortBuf;};
protected:
	int firstPos;
	int lastPos;
	int resumePos;
	int lastFileNumber;
	int lastGet;
	int lastBuf;
	bool abortBuf;
	bool dropFile;
	unsigned int maxSize;
	cFileName bufferFileName;
	cString bufferBaseName;
	cMutex idx_lock;
	std::vector<tLiveIndex> idx;
	virtual std::vector<tLiveIndex>::iterator GetIndex(int Index) {
		if(!idx.size()) return idx.end();
		std::vector<tLiveIndex>::iterator item = idx.begin();

		unsigned int guess = Index-First(); // Try to guess the position
		if(guess > 0) {
			if(guess < idx.size())
				item += guess;
			else
				item = idx.end()-1;
		} // if
		while(item->index < Index) {
			item++;
			if(item == idx.end())
				return idx.end();
		} // while
		while(item->index > Index) {
			if(item == idx.begin())
				return idx.end();
			item--;
		} // while
		if(item->index != Index)
			return idx.end();
		return item;
	}; // GetIndex
}; // cLiveIndex

/*****************************************************************************/

int DiskSizeMB(const char *Directory) {
	int Ret = 0;
	struct statfs statFs;
	if (statfs(Directory, &statFs) == 0) {
		double blocksPerMeg = 1024.0 * 1024.0 / statFs.f_bsize;
		Ret = int(statFs.f_blocks / blocksPerMeg);
	} else LOG_ERROR_STR(Directory);
	return Ret;
} // DiskSizeMB

cString cLiveRecorder::liveFileName;

cLiveRecorder::cLiveRecorder(const cChannel *Channel):cRecorder(FileName(), Channel, -1)
              ,broken(false) {
	handleError = false;
	if(index) delete index;
	index = new cLiveIndex(FileName());
	Activate(true);
	int DiskSize = DiskSizeMB(FileName());
	MaxFileSize = DiskSize / 10;
	if(!MaxFileSize || (MaxFileSize > Setup.LiveBufferMaxFileSize)) MaxFileSize = Setup.LiveBufferMaxFileSize;
	else isyslog("Reducing LiveBufferMaxFileSize from %d MB to %d MB due to small disk size (%d MB)", Setup.LiveBufferMaxFileSize, MaxFileSize, DiskSize);
}; // cLiveRecorder::cLiveRecorder

cLiveRecorder::~cLiveRecorder() {
	int maxWait = WAIT_TERMINATE_COUNT;
	CancelWritingBuffer();
	while(IsWritingBuffer() && maxWait--)
		usleep(WAIT_TERMINATE_SLEEP);
	Activate(false);
	Cleanup();
}; // cLiveRecorder::~cLiveRecorder

bool cLiveRecorder::IsWritingBuffer() {
	return index && ((cLiveIndex *)index)->IsWritingBuffer();
} // cLiveRecorder::IsWritingBuffer

void cLiveRecorder::CancelWritingBuffer() {
	if(index) ((cLiveIndex *)index)->CancelWritingBuffer();
} // cLiveRecorder::CancelWritingBuffer

bool cLiveRecorder::RunningLowOnDiskSpace(void) {
	if(!BufferDirectoryMinFree) return cRecorder::RunningLowOnDiskSpace();
	if (time(NULL) > lastDiskSpaceCheck + DISKCHECKINTERVAL) {
		int Free = FreeDiskSpaceMB(fileName->Name());
		lastDiskSpaceCheck = time(NULL);
		if (Free < BufferDirectoryMinFree) 
			return true;
	} // if
	return false;
} // cLiveRecorder::RunningLowOnDiskSpace

bool cLiveRecorder::NextFile(void) {
	if (recordFile && frameDetector->IndependentFrame()) { // every file shall start with an independent frame
		if(RunningLowOnDiskSpace() && index)
			((cLiveIndex *)index)->DropFile();
		if (fileSize > MEGABYTE(off_t(MaxFileSize)) || RunningLowOnDiskSpace()) {
			recordFile = fileName->NextFile();
			fileSize = 0;
		} // if
	} // if
	return recordFile != NULL;
} // cLiveRecorder::NextFile

int cLiveRecorder::LastIFrame() {
	if(!index) return 0;
	int ret = index->GetNextIFrame(index->Last()-1, false);
	return (ret > 0) ? ret : 0;
}; // cLiveRecorder::LastIFrame

int cLiveRecorder::LastFrame() { 
	return index ? index->Last() : 0;
}; // cLiveRecorder::LastFrame

void cLiveRecorder::SetResume(int Index) { 
	if(index) ((cLiveIndex *)index)->SetResume(Index);
}; // cLiveRecorder::SetResume

bool cLiveRecorder::SetBufferStart(time_t Start) {
	if(!index) return false;
	if(time(NULL) <= Start) return false;
	int Frames = SecondsToFrames(time(NULL)-Start+60, frameDetector ? frameDetector->FramesPerSecond() : DEFAULTFRAMESPERSECOND);
	return ((cLiveIndex *)index)->SetBufferStart(Frames);
} // cLiveRecorder::SetBufferStart

cIndex *cLiveRecorder::GetIndex() { 
	return index;
}; // cLiveRecorder::GetIndex

bool cLiveRecorder::Cleanup() {
	if(FileName()) 
		if(-1 == system(cString::sprintf("rm %s/*", FileName())))
			return false;
	return true;
}; // cLiveRecorder::Cleanup

bool cLiveRecorder::Prepare() {
	if (!MakeDirs(FileName(), true)) return false;
	if (!Cleanup()) return false;
	int DiskSize = DiskSizeMB(FileName());
	if (DiskSize < ((BufferDirectoryMinFree?BufferDirectoryMinFree:MINFREEDISKSPACE)+MINLIVEBUFFERSIZE)) return false;  
	return true;
}; // cLiveRecorder::Prepare

const char *cLiveRecorder::FileName() {
	if(!(const char *)liveFileName && BufferDirectory)
		liveFileName = cString::sprintf("%s/LiveBuffer", BufferDirectory);
	return liveFileName;
}; // cLiveRecorder::FileName

void cLiveRecorder::Activate(bool On) {
	cRecorder::Activate(On);
	if(!On) broken=true;
} // cLiveRecorder::Activate

void cLiveRecorder::Receive(uchar *Data, int Length) {
	if(broken) {
		isyslog("Continue live recorder on broken stream (maybe due to switching to same channel on other device)");
		TsSetTeiOnBrokenPackets(Data, Length);
		broken = false;
	} // if
	cRecorder::Receive(Data, Length);
} // cLiveRecorder::Receive

/*****************************************************************************/

cBufferRecorder::cBufferRecorder(const char *FileName, const cChannel *Channel, int Priority, cIndex *LiveBufferIndex)
                :cRecorder(FileName, Channel, Priority)
                ,liveBufferIndex(LiveBufferIndex)
                ,dropData(false) {
	if(liveBufferIndex) dropData=true; // Drop new data till we have written most of the live buffer data
} // cBufferRecorder::cBufferRecorder

cBufferRecorder::~cBufferRecorder() {
	if(liveBufferIndex) ((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
} // cBufferRecorder::~cBufferRecorder

void cBufferRecorder::Action(void) {
	if(liveBufferIndex)
		FillInitialData(NULL, 0);
	dropData=false;
	cRecorder::Action();
	if(liveBufferIndex) ((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
	liveBufferIndex = NULL;
} // cBufferRecorder::Action

void cBufferRecorder::Activate(bool On) {
	if(!On && liveBufferIndex) ((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
	cRecorder::Activate(On);
} // cBufferRecorder::Activate

void cBufferRecorder::Receive(uchar *Data, int Length) {
	if(!dropData) cRecorder::Receive(Data, Length);
} // cBufferRecorder::Receive

void cBufferRecorder::FillInitialData(uchar *Data, int Size) {
	if(liveBufferIndex) {
		int64_t search_pts = Data ? TsGetPts(Data, Size) : -1;
		int maxWait = WAIT_WRITING_COUNT;
		uchar buffer[MAXFRAMESIZE];
		int Length;
		bool Independent;
		bool found = false;
		while(Running() && (!Data || (Size >= TS_SIZE))) {
			cUnbufferedFile *file = ((cLiveIndex *)liveBufferIndex)->GetNextBuffer(Length, Independent);
			if(!file) {
				if(((cLiveIndex *)liveBufferIndex)->WritingBufferCanceled()) {
					isyslog("Writing buffer canceled by user");
					if(fileSize) TsSetTeiOnBrokenPackets(Data, Size);
					((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
					liveBufferIndex = NULL;
					return;
				} // if
				if(!Data || !Size) return;
				if(!maxWait--)
					break;
				usleep(WAIT_WRITING_SLEEP);
				continue;
			} // if
			if (!NextFile())
				break;
			int len = ReadFrame(file, buffer, Length, sizeof(buffer));
			if(len < TS_SIZE) {
				isyslog("Failed to read live buffer data");
				break;
			} // if
			if(Data && Independent && (search_pts == TsGetPts(buffer, len))) {
				found = true;
				break;
			} // if
			if (index)
				index->Write(Independent, fileName->Number(), fileSize);
			if (recordFile->Write(buffer, len) < 0) {
				isyslog("Failed to write live buffer data");
				break;
			} // if
			fileSize += len;
		} // while
		if(Data) {
			isyslog("%lld bytes from live buffer %swritten to recording", fileSize, found ? "seamless ": "");
			if(!found && fileSize) TsSetTeiOnBrokenPackets(Data, Size);
			((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
			liveBufferIndex = NULL;
		} else if(((cLiveIndex *)liveBufferIndex)->WritingBufferCanceled()) {
			isyslog("%lld bytes from live buffer written to recording (aborted)", fileSize);
			((cLiveIndex *)liveBufferIndex)->SetBufferStart(0);
			liveBufferIndex = NULL;
		} // if
	} else if (Data && fileSize)
		TsSetTeiOnBrokenPackets(Data, Size);
} // cBufferRecorder::FillInitialData

#endif /*VDRVERSNUM*/
#endif /*USE_LIVEBUFFER*/
