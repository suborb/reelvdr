#include "status.h"
#include "mediad.h"

cMediadStatus::cMediadStatus(void) {
	_prevTitle = NULL;
	_title = NULL;
	_current = NULL;
	_linkedRecord = false;
	_doUnmount = false;
	_replay = false;
	_currentDevice = NULL;
	_videoDir = NULL;
}

cMediadStatus::~cMediadStatus() {
	if (_videoDir)
		free (_videoDir);
	if(_prevTitle)
		free (_prevTitle);
	if(_title)
		free (_title);
}

void cMediadStatus::OsdTitle(const char *title) {
	if(_title) {
		if(_prevTitle)
			free(_prevTitle);
		asprintf(&_prevTitle, "%s", _title);
		free (_title);
	}
	asprintf(&_title, "%s", title);
}

void cMediadStatus::OsdCurrentItem(const char *text) {
	_current = text;
}

void cMediadStatus::OsdClear(void) {
	//DDD("OSDCLEAR %s %s", _title, _replay ? "true":"false");
	if(!_replay && 
	    _title && 
	    _prevTitle && 
	    _linkedRecord &&
	    strncmp(tr("Recordings"), _prevTitle, strlen(tr("Recordings"))) == 0 &&
     	    strncmp(trVDR("VDR"), _title, strlen(trVDR("VDR"))) == 0
	    ) {
		DDD("Reload recordings");
		Recordings.Load();
		if( (_doUnmount && _currentDevice) || _linkedRecord) {
			_currentDevice->umount();
			_doUnmount = false;
		}
		if(_videoDir)
			free(_videoDir);
		_videoDir = NULL;
		_linkedRecord = false;
		_doUnmount = false;
	} else
		if (_doUnmount && _currentDevice) {
			_currentDevice->umount();
			_doUnmount = false;
			if(_videoDir)
				free(_videoDir);
		}
}

void cMediadStatus::Replaying(const cControl *control, const char *name, const char *fileName, bool on)
{
	_replay = on;
	// set to avoid decision like (based on a timing problem where reply stoppped before recording display
	// !_replay: 0 && _Title VDR  -  Disk 95%  -   4:11 frei && _prevTitle Aufzeichnungen -  4:11 frei && _LinkededRecord 1
	// but only if this is a file replayed within our directory
	if (_videoDir && strncmp(fileName, _videoDir, strlen(_videoDir)) == 0)
		DDD("setting prev title null");
		_prevTitle = NULL;
	if (_currentDevice && _videoDir && !on && !_linkedRecord) {
		DDD("umount decision filename %s videodir %s", fileName, _videoDir);
		if (strncmp(fileName, _videoDir, strlen(_videoDir)) == 0)
			_doUnmount = true;
		else
			_doUnmount = false;
	}
}
