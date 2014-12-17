/*
 * remotetimers.h: Public interface of the plugin's services
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef _SERVICE__H
#define _SERVICE__H

#ifndef __TIMERS_H
#include <vdr/timer.h>
#include <vdr/epg.h>
#endif

struct RemoteTimers_InstantRecording_v1_0 {
//in
	const cTimer	*timer;
	bool		pause;
	const cEvent	*event;
//out
	cString		name;
	cString		fileName;
};

#endif //_SERVICE__H
