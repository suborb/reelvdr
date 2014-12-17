#ifndef VDR_VDRCD_STATUS_H
#define VDR_VDRCD_STATUS_H

#include <vdr/status.h>
#include "devicesetup.h"

class cMediadStatus: public cStatus {
private:
	char *_title;
	const char *_current;
	bool    _doUnmount;
	char   *_videoDir;
	bool   _replay;
	char   *_prevTitle;
	bool   _linkedRecord;
	device *_currentDevice;
protected:
	virtual void OsdTitle(const char *Title);
        virtual void OsdCurrentItem(const char *Text);
	virtual void OsdClear(void);
	virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);

public:
	cMediadStatus(void);
	~cMediadStatus();

	const char *Title(void) const { return _title; }
	const char *Current(void) const { return _current; }
	void        LinkedRecording(bool linked) { _linkedRecord = linked; }
	void        setCurrentDevice(device* curDevice) { _currentDevice = curDevice; }
};

#endif // VDR_VDRCD_STATUS_H
