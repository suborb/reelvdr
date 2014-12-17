#ifndef LIVEBUFFER_H
#define LIVEBUFFER_H

#ifdef USE_LIVEBUFFER
#include "config.h"
#if VDRVERSNUM >= 10716

#include "recorder.h"

class cLiveRecorder : public cRecorder {
public:
	cLiveRecorder(const cChannel *Channel);
	virtual bool RunningLowOnDiskSpace(void);
	virtual bool NextFile(void);
	virtual ~cLiveRecorder();
	virtual bool IsWritingBuffer();
	virtual void CancelWritingBuffer();
	virtual int LastIFrame();
	virtual int LastFrame();
	virtual void SetResume(int Index);
	virtual bool SetBufferStart(time_t Start);
	virtual cIndex *GetIndex();
	static bool Cleanup();
	static bool Prepare();
	static const char *FileName();
protected:
	virtual void Activate(bool On);
	virtual void Receive(uchar *Data, int Length);
	bool broken;
	int MaxFileSize;
	static cString liveFileName;
}; // cLiveRecorder

class cBufferRecorder : public cRecorder {
public:
	cBufferRecorder(const char *FileName, const cChannel *Channel, int Priority, cIndex *LiveBufferIndex);
	virtual ~cBufferRecorder();
	virtual void FillInitialData(uchar *Data, int Size);
protected:
	virtual void Action(void);
	virtual void Activate(bool On);
	virtual void Receive(uchar *Data, int Length);
	cIndex *liveBufferIndex;
	bool dropData;
}; // cBufferRecorder

#endif /*VDRVERSNUM*/
#endif /*USE_LIVEBUFFER*/
#endif /*LIVEBUFFER_H*/
