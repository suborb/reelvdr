/*
 *  $Id: suspend.c,v 1.3 2008/10/22 11:59:32 schmirl Exp $
 */
 
#include "server/suspend.h"
#include "server/suspend.dat"
#include "common.h"

cSuspendLive::cSuspendLive(void)
		: cThread("Streamdev: server suspend")
{
}

cSuspendLive::~cSuspendLive() {
	Stop();
	Detach();
}

void cSuspendLive::Activate(bool On) {
	Dprintf("Activate cSuspendLive %d\n", On);
	if (On)
		Start();
	else
		Stop();
}

void cSuspendLive::Stop(void) {
	if (Running())
		Cancel(3);
}

void cSuspendLive::Action(void) {
#if VDRVERSNUM < 10300
	isyslog("Streamdev: Suspend Live thread started (pid = %d)", getpid());
#endif

	m_Active = true;
	while (m_Active) {
		//DeviceStillPicture(suspend_mpg, sizeof(suspend_mpg));
		usleep(100000);
	}
}

bool cSuspendCtl::m_Active = false;

cSuspendCtl::cSuspendCtl(void):
		cControl(m_Suspend = new cSuspendLive) {
	m_Active = true;
}

cSuspendCtl::~cSuspendCtl() {
	m_Active = false;
	DELETENULL(m_Suspend);
}

eOSState cSuspendCtl::ProcessKey(eKeys Key) {
	if (!m_Suspend->IsActive() || Key == kBack || Key == kOk || Key == kUp 
            || Key == kDown || Key == kChanUp || Key == kChanDn || Key == kStop) {
		DELETENULL(m_Suspend);
		return osEnd;
	}
	return osContinue;
}
