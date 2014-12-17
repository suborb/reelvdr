/*
 * mediapoller.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_POLLER_H
#define _MEDIA_POLLER_H

#include "mediatypes.h"

class cMediaPoller {
private:
	bool first;
	bool ismounted;
	bool trayisopen;
	int support_media_changed;
	int mediastatus;
	eMediaType mediatype;
protected:
	void check_media(const char *DeviceFile, const char *RealDevice);
	eMediaType advanced_disc_detect(int fd, const char *DeviceFile);
public:
	cMediaPoller();
	~cMediaPoller();
	bool isMounted(const char *RealDevice);
	eMediaType poll(const char *DeviceFile, const char *RealDevice);
	bool EjectDisc(const char *DeviceFile);
	bool OpenCloseTray(const char *DeviceFile);
};

#endif
