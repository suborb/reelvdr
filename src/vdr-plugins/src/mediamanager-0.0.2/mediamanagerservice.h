/*
 * mediamanagerservice.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_MANAGER_SERVICE_H
#define _MEDIA_MANAGER_SERVICE_H

#include "mediatypes.h"

/* flags for MediaManager_Status_v1_0 */
#define MEDIA_STATUS_REPLAY 	0x01
#define MEDIA_STATUS_EJECTDISC	0x02
#define MEDIA_STATUS_ACTIVE 	0x04
#define MEDIA_STATUS_CRASHED	0x08

#define MEDIA_MANAGER_REGISTER_ID	"MediaManager-Register-v1.0"
#define MEDIA_MANAGER_ACTIVATE_ID	"MediaManager-Activate-v1.0"
#define MEDIA_MANAGER_MAINMENU_ID	"MediaManager-Mainmenu-v1.0"
#define MEDIA_MANAGER_STATUS_ID 	"MediaManager-Status-v1.0"
#define MEDIA_MANAGER_SUSPEND_ID	"MediaManager-Suspend-v1.0"

struct MediaManager_Register_v1_0 {
	const char *PluginName;
	const char *MainMenuEntry;
	const char *Description;
	eMediaType mediatype;
	bool shouldmount;
};

struct MediaManager_Activate_v1_0 {
	bool on;
	const char *device_file;
	const char *mount_point;
};

struct MediaManager_Mainmenu_v1_0 {
	cOsdObject *mainmenu;
};

struct MediaManager_Status_v1_0 {
	bool set;
	int flags;
	bool isreplaying;
	bool ejectdisc;
	bool active;
	bool crashed;
};

struct MediaManager_Suspend_v1_0 {
	const char *PluginName;
	bool suspend;
};

#endif
