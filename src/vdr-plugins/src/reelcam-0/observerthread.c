
#include "observerthread.h"
#include "reel.h"
#include "common.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/dvbdevice.h>
#include <vdr/ci.h>
#include <vdr/config.h>

#if 1
#undef d
#define d(x) x;
#endif

void ObserverThread::Action(void){
	int last_enabled=Setup.CAMEnabled;
	int enabled;
	int i;
	int newCamState = 0;
#if 0
	// Check if init_ci was forgotten...
	
	newCamState = ioctl(fd,IOCTL_REEL_CI_GET_STATE,1);
	for(i=0;i<3;i++) {
		if (last_enabled&(1<<i) && !((newCamState>>(16+2*i))&3) && newCamState&(1<<i)) {
			powerOn(fd, i);
		}
	}
	cCondWait::SleepMs(200);	
#endif	
	
	active = true;
	while(active){
#if 1
#ifdef RBLITE
        newCamState = ioctl(fd,IOCTL_REEL_CI_GET_STATE,1);
#endif
	int cam_change = (newCamState & 0x00700) >> 8;      /* Bit 8-10 is 1 if slot changed since last ioctl */

	enabled=Setup.CAMEnabled;
//	esyslog("reelcam: %x %x\n",enabled,newCamState);
	for(i=0;i<3;i++) {
	  if (cam_change&(1<<i)|| ((enabled^last_enabled)&(1<<i))) {
	    if (newCamState&(1<<i) && (enabled&(1<<i)))
	      powerOn(fd, i);
	    else
//              if (newCamState&(3<<(16+2*i)))
		powerOff(fd, i); 
	  }	    
	}
		
	camState = newCamState;
	/* d(printState(camState)) */
	last_enabled=enabled;
#endif
	cCondWait::SleepMs(500);
	}
	finished = true;
}

ObserverThread::ObserverThread(int Fd){
  fd = Fd;
  finished = false;
  SetDescription("ReelCAM-Observer-Thread\n");
#ifdef RBLITE
  camState = ioctl(fd,IOCTL_REEL_CI_GET_STATE);  /* get state */
#endif
}

ObserverThread::~ObserverThread(void){
  active = false;
  while(!finished)
    cCondWait::SleepMs(250);  /* let Action() finish before destroying the object... */
}

/* for debugging */
void ObserverThread::printState(int state){
  int state_cp = state & 0x00007;         /* Bit 0-2 is 1 if something is in slot 0-2 */
  printf("reelcam: s0: %i, s1: %i, s2: %i ", state_cp&0x1, (state_cp&0x2)>>1, (state_cp&0x4)>>2);
  state_cp = (state & 0x00700) >> 8;      /* Bit 8-10 is 1 if slot changed since last ioctl */
  printf("c0: %i, c1: %i, c2: %i ", state_cp&0x1, (state_cp&0x2)>>1, (state_cp&0x4)>>2);
  state_cp = (state & 0x2f0000) >> 16;     /* Bit 17/16... is 2 if slot is powered on and DVB-Enabled */
  printf("p0: %i, p1: %i, p2: %i \n", state_cp&0x1, (state_cp&0x2)>>1, (state_cp&0x4)>>2);

}

int ObserverThread::powerOff(int fd, int slot){
  d(esyslog("reelcam: CAM in slot%i removed, powering off slot%i\n", slot, slot))
  cDevice::GetDevice(0)->CiHandler()->CloseAllSessions(slot);
  return ioctl(fd, IOCTL_REEL_CI_SWITCH, (slot)|(0<<8));  /* power off slot */
}

int ObserverThread::powerOn(int fd, int slot){
  int result = 0;
  d(esyslog("reelcam: CAM in slot%i inserted, powering on slot%i\n", slot, slot))
#ifdef RBLITE
  result = ioctl(fd, IOCTL_REEL_CI_SWITCH, (slot)|(2<<8));  /* power on + make visible for DVB driver */
#endif
  return result;	
}
