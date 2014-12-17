/*
 * status.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <typeinfo>
#include <malloc.h>
#include <stdlib.h>
#include <sys/time.h>

#include <vdr/plugin.h>
#include <vdr/menuitems.h>
#include <vdr/status.h>
#include <vdr/dvbdevice.h>
#include <vdr/channels.h>
#include <vdr/ci.h>
#include <vdr/eitscan.h>
#include <vdr/timers.h>

#ifndef RBLITE
#include <linux/dvb/ca.h>
#endif

#include "reelcam.h"
#include "common.h"
#include "reel.h"
#include "observerthread.h"
#include "status.h"

#define MAX_CI_SLOTS 3

using std::vector;

extern int multica;
extern int nocamlimit;

#define ALPHA_MAX 3
#define ALPHATC_MAX 2
#define VIACCESS_MAX 1
#define DIABLO_MAX 2
#define NAGRA_MAX 4
#define ASTON_DUAL_MAX 2
#define CANALDIGITAAL_MAX 4
#define POWERCAM_MAX 4

// -- cRcStatus ----------------------------------------------------------------

cRcStatus::cRcStatus(int Fd){
  fd = Fd;
  for (int i=0; i<MAX_CI_SLOTS; i++){
    slotInUse[i] = -1;
  }      
}

cRcStatus::~cRcStatus(void){
  active = false;
}

void cRcStatus::ChannelSwitch(const cDevice *device, int chNumber){
  
  int cardIndex = device->CardIndex();
  if( (chNumber>0) && (!device->IsPrimaryDevice()) && (!EITScanner.UsesDevice(device)) ) {  /* Not if output-device is switched and not if initiated by the epg-scan */
    cChannel *channel=Channels.GetByNumber(chNumber);
#if 0
    char *t1 = (char*)malloc(256);
    sprintf(t1, "reelcam switch: card=%d primary=%d dev=%p actDev=%p to channel %d with caIDs ", cardIndex,device->IsPrimaryDevice(),device,cDevice::ActualDevice(), chNumber);
    for(int i=0; i<MAXCAIDS; i++){
      int caid = channel->Ca(i);
      if (caid){
	char *t2 = (char*)malloc(8);
        sprintf(t2, "%#04x ", caid);
	strcat(t1, t2);
	free(t2);
      }
    }
    d(esyslog(t1))
    free(t1);
#endif
    int ca;
    if(channel){
      if ((ca=channel->Ca())>=CA_ENCRYPTED_MIN) {
	unsigned short channel_ca_ids[MAXCAIDS + 1];
        unsigned short ca_ids[MAX_CI_SLOTS][MAXCAIDS + 1];     /* the ci_ids in the 3 slots */
	for (int i = 0; i <= MAXCAIDS; i++)                    /* '<=' copies the terminating 0! */
          channel_ca_ids[i] = channel->Ca(i);
	cCiHandler *cihandler = (cDevice::GetDevice(0))->CiHandler(); /* always get ciHandler from adapter0/ca0 */
	if (cihandler) {
	  for (int i = 0; i < MAX_CI_SLOTS; i++){
            const short unsigned int *ids = cihandler->GetCaSystemIds(i);
	    if(ids)
	      for (int j = 0; j <= MAXCAIDS; j++)
	        ca_ids[i][j] = ids[j];
	  } // for
        } // if
        int foundOnSlot = -1;
	for (int i = 0; i < MAX_CI_SLOTS; i++) /* For all 3 slots */
	  for (int j = 0; j <= MAXCAIDS; j++)
	    for (int k = 0; k <= MAXCAIDS; k++)
	      if( (channel_ca_ids[j] != 0) && (ca_ids[i][0] != 0) && (foundOnSlot == -1) && (ca_ids[i][k] == channel_ca_ids[j]) && ((slotInUse[i] == device->CardIndex()) || (slotInUse[i] == -1))){  /* if "encrypted", "not found yet" and "suitable CAM" */
		foundOnSlot = i;
		d(esyslog("reelcam: Slot %i provides the needed CAM\n", i))
		break;
	      }
		
	if ( (foundOnSlot >= 0) && (slotInUse[foundOnSlot] == cardIndex) && CanDo(foundOnSlot, chNumber) ) {   /* if found and already on the right tuner and is not used for recording */
	  d(esyslog("reelcam: Slot %i already on tuner nr. %i, no need to switch\n", foundOnSlot, cardIndex))
	} else if ( (foundOnSlot >= 0) && (slotInUse[foundOnSlot] < 0) ) {          /* if found and not used yet */	  
	  for(int i = 0; i < MAX_CI_SLOTS; i++)
            if((foundOnSlot != i) && (slotInUse[i] == cardIndex)){                 /* if other CAMs than the requested are used by the tuner */
            //if((foundOnSlot != i) && (slotInUse[i] == cardIndex) && channelsRecording[i].size()==0){                 /* if other CAMs than the requested are used by the tuner */
	      d(esyslog("reelcam: releasing slot %i from tuner nr. %i, slotInUse[%i]: %i\n", i, cardIndex, i, slotInUse[i]))
	      slotInUse[i]=-1;
	    }
#ifdef RBLITE
	  ioctl(fd,IOCTL_REEL_CI_ASSOC,(foundOnSlot)|(cardIndex<<16));
#endif
	  slotInUse[foundOnSlot] = cardIndex;
	  d(esyslog("reelcam: Slot %i switched to tuner nr. %i: slotInUse[%i]: %i\n", foundOnSlot, cardIndex, foundOnSlot, slotInUse[foundOnSlot]))
        } else if ( (foundOnSlot >= 0) && slotInUse[foundOnSlot] >= 0) {
	  for(int i = 0; i < MAX_CI_SLOTS; i++)
            if(slotInUse[i] == cardIndex){
	      if(channelsRecording[foundOnSlot].size() == 0){
	        d(esyslog("reelcam: Slot %i brutaly switched to tuner nr. %i: slotInUse[%i]: %i\n", foundOnSlot, cardIndex, foundOnSlot, slotInUse[foundOnSlot]))
#ifdef RBLITE
	        ioctl(fd,IOCTL_REEL_CI_ASSOC,(foundOnSlot)|(cardIndex<<16));
#endif
		slotInUse[i] = -1;
	        slotInUse[foundOnSlot] = cardIndex;
	      } else
	        d(esyslog("reelcam: needed CAM already used!\n"))
	    }
        } else 
	  d(esyslog("reelcam: not suitable CAM found!\n"))
	    
      } else if (ca<=3 && ca>0){ /* a certain CAM is requested */
	ca--;
	d(esyslog("reelcam: certain CAM nr %i requested\n", ca)) 
	  if ( (slotInUse[ca] == cardIndex) && CanDo(ca, chNumber) ) {   /* if found and already on the right tuner and is not used for recording */
	    d(esyslog("reelcam: Slot %i already on tuner nr. %i, no need to switch\n", ca, cardIndex))
	    ca++;
	  } else if ( slotInUse[ca] < 0) {          /* if not used yet */
	    for(int i = 0; i < MAX_CI_SLOTS; i++)
              if(slotInUse[i] == cardIndex){                  /* if other CAMs than the requested are used by the tuner */
	      //if(slotInUse[i] == cardIndex && channelsRecording[i].size()==0){                  /* if other CAMs than the requested are used by the tuner */
		d(esyslog("reelcam: releasing slot %i from tuner nr. %i, slotInUse[%i]: %i\n", i, cardIndex, i, slotInUse[i]))
		slotInUse[i] = -1;
#ifdef RBLITE
                ioctl(fd,IOCTL_REEL_CI_ASSOC,(0xff)|(cardIndex<<16)); 
#endif
	      }
#ifdef RBLITE
	      ioctl(fd,IOCTL_REEL_CI_ASSOC,(ca)|(cardIndex<<16));
#endif
	      slotInUse[ca] = cardIndex;
	      d(esyslog("reelcam: Slot %i switched to tuner nr. %i, slotInUse[%i]: %i\n", ca, cardIndex, ca, slotInUse[ca]))
	  } else {
            for(int i = 0; i < MAX_CI_SLOTS; i++)
              if(slotInUse[i] == cardIndex){
	        if(channelsRecording[i].size() == 0){
                  d(esyslog("reelcam: Slot %i brutaly switched to tuner nr. %i: slotInUse[%i]: %i\n", ca, cardIndex, ca, slotInUse[ca]))
#ifdef RBLITE
                  ioctl(fd,IOCTL_REEL_CI_ASSOC,(ca)|(cardIndex<<16));
#endif
		  slotInUse[i] = -1;
                  slotInUse[ca] = cardIndex;
		} else
		  d(esyslog("reelcam: needed CAM already used!\n"))
	      }
	  } 
      } else {                                         /* in case of a switch to a channel without encryption */
	for (int i = 0; i < MAX_CI_SLOTS; i++)
	  if(slotInUse[i] == cardIndex && channelsRecording[i].size() == 0){               /* release the slot if now not longer needed */
	    d(esyslog("reelcam: releasing slot %i from tuner nr. %i, slotInUse[%i]: %i\n", i, cardIndex, i, slotInUse[i]))
#ifdef RBLITE
	    ioctl(fd,IOCTL_REEL_CI_ASSOC,(0xff)|(cardIndex<<16));
#endif
	    slotInUse[i] = -1;
	  }
      }
    }
  } 
}

#if VDRVERSNUM && VDRVERSNUM >= 10401
void cRcStatus::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On, int ChannelNumber) { 
#else 
void cRcStatus::Recording(const cDevice *Device, const char *Name, bool On, int ChannelNumber) {
#endif 
  int cardIndex = Device->CardIndex();
  for (unsigned int i=0; i<MAX_CI_SLOTS; i++)
    if(slotInUse[i] == cardIndex){
      if(On){  /* Recording started */
	channelsRecording[i].push_back(ChannelNumber);
        d(esyslog("reelcam: started recording \"%s\" channel %i with slot %i on device %i - now %i recording(s) on that slot: slotInUse[%i]: %i\n", Name, ChannelNumber, i, cardIndex, channelsRecording[i].size(), i, slotInUse[i]))
      } else { /* Recording ended */
	if(channelsRecording[i].size()==1){
          channelsRecording[i].pop_back();  
	  d(esyslog("reelcam: 'cause recording ended: releasing slot %i from tuner nr. %i, slotInUse[%i]: %i\n", i, cardIndex, i, slotInUse[i]))
          slotInUse[i] = -1; /* this is telling that the slot is again free for use */
	  /* switch device?! */
        } else {
	  unsigned int k = 0;
	  for(vector<unsigned int>::iterator j = channelsRecording[i].begin(); j != channelsRecording[i].end(); j++, k++)
	    if(channelsRecording[i][k] == (unsigned int)ChannelNumber){
	      channelsRecording[i].erase(j);
	      break;
	    }
	}

	d(esyslog("reelcam: recording of \"%s\" on channel %i with slot %i ended on device %i - now %i recording(s) on that slot\n", Name, ChannelNumber, i, cardIndex, channelsRecording[i].size()))
      }
    }
}

#if 0
void cRcStatus::Action(){
  struct timeval t;
  active = true;
  while(active){
  if (gettimeofday(&t, NULL) == 0){
    if(t.tv_sec-now >= 5*60) /* all 5 minutes */
      for (int i=0; i<MAX_CI_SLOTS; i++){ /* For all 3 slots */
	if(false && slotInUse[i]>=0){ /* if slot is used and used by device-nr>0, vdr itself cares about device 0 - but not properly?! */
	  /* cCiHandler *ciHandler = (cDevice::GetDevice(0))->CiHandler();
          ciHandler->StartDecrypting();
            if (ciHandler->Process(i))
              d(esyslog("reelcam: manually set CaPmt on slot %i cause of the 5-min timer\n", i)) */ 
        }
    now = t.tv_sec;
    }
  }
  cCondWait::SleepMs(2000);
  }
}
#endif

int cRcStatus::SlotOnDev(const cDevice *device){
  for (int i=0; i<MAX_CI_SLOTS; i++){
    if(slotInUse[i] == device->CardIndex()){
      //esyslog("reelcam: SlotOnDev: devnr: %i, slotInUse[%i]: %i\n", device->CardIndex(), i, slotInUse[i]);
      return i;
    }
  }
  return -1;
}
    
int cRcStatus::SlotOnDev(int deviceNumber){
  for (int i=0; i<MAX_CI_SLOTS; i++){
     if(slotInUse[i] == deviceNumber){
       //esyslog("reelcam: SlotOnDev: devnr: %i, slotInUse[%i]: %i\n", deviceNumber, i, slotInUse[i]);
       return i;
     }
  }
  return -1;
}

#define MAXCASYSTEMIDS 64

class cCiSession {
private:
  uint16_t sessionId;
  uint32_t resourceId;
  cCiTransportConnection *tc;
protected:
  int GetTag(int &Length, const uint8_t **Data);
  const uint8_t *GetData(const uint8_t *Data, int &Length);
  int SendData(int Tag, int Length = 0, const uint8_t *Data = NULL);
public:
  cCiSession(uint16_t SessionId, uint32_t ResourceId, cCiTransportConnection *Tc);
  virtual ~cCiSession();
  const cCiTransportConnection *Tc(void) { return tc; }
  uint16_t SessionId(void) { return sessionId; }
  uint32_t ResourceId(void) { return resourceId; }
  virtual bool HasUserIO(void) { return false; }
  virtual bool Process(int Length = 0, const uint8_t *Data = NULL);
  };

class cCiCaPmt : public cListObject {
  friend class cCiConditionalAccessSupport;
private:
  uint8_t cmdId;
  int length;
  int esInfoLengthPos;
  uint8_t capmt[2048]; ///< XXX is there a specified maximum?
  int caDescriptorsLength;
  uint8_t caDescriptors[2048];
  bool streamFlag;
  unsigned short magicCaId;
  //void AddCaDescriptors(int Length, const uint8_t *Data);
public:
  cCiCaPmt(uint8_t CmdId, int Source, int Transponder, int ProgramNumber, const unsigned short *CaSystemIds, unsigned short MagicCaId=0);
  void SetListManagement(uint8_t ListManagement);
  bool Valid(void);
  void AddPid(int Pid, uint8_t StreamType);
  void AddCaDescriptors(int Length, const uint8_t *Data);
  };

class cCiConditionalAccessSupport : public cCiSession {
private:
  int state;
  int numCaSystemIds;
  unsigned short caSystemIds[MAXCASYSTEMIDS + 1]; // list is zero terminated!
public:
  int GetState() { return state; }
  cCiConditionalAccessSupport(uint16_t SessionId, cCiTransportConnection *Tc);
  virtual bool Process(int Length = 0, const uint8_t *Data = NULL);
  const unsigned short *GetCaSystemIds(void) { return caSystemIds; }
  bool SendPMT(cCiCaPmt *CaPmt);
  bool ReceivedReply(bool CanDescramble = false);
  };

#define RI_CONDITIONAL_ACCESS_SUPPORT  0x00030041

int cRcStatus::ProvidesCa(const cDevice *device, const unsigned short *ch_caids, int channelNumber){
  cCiHandler *handler = (cDevice::GetDevice(0))->CiHandler();
  int cardIndex = device->CardIndex();
  unsigned short all_ca_ids[MAX_CI_SLOTS][MAXCAIDS + 1];     /* the ci_ids in the 3 slots */
  /* TB: wait until all CAMs are ready */
  if (!handler) {
	esyslog("ERROR: cCiHandler is NULL!\n");
	return false;
  }
  if (handler->NumCams() == 0)
	return false;
  int count = 0;
  for (int Slot = 0; Slot < handler->NumSlots(); Slot++) {
        if (handler->moduleReady[Slot]) {
         count++;
         cCiConditionalAccessSupport *cas = (cCiConditionalAccessSupport *)handler->GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT, Slot);
         if (!cas || (cas && !*cas->GetCaSystemIds()) || (cas && cas->GetState() < 2 && cas->GetState() >= 0))
            return false;
       }
  }
  if (count != handler->NumCams()) {
	printf("DEBUG: not yet all CAMs ready\n");
	return false;
  }
//#define DEBUG_CAIDS
#ifdef DEBUG_CAIDS
  char *t1 = (char*)malloc(1024);
  strcpy(t1, "reelcam: CAIDs: ");
#endif
  for (int i=0; i<MAX_CI_SLOTS; i++){
    const short unsigned int *ids = handler->GetCaSystemIds(i);
#ifdef DEBUG_CAIDS
      char *t9 = (char*)malloc(8);
      sprintf(t9, "slot %i: ", i);
      strcat(t1, t9);
      free(t9);
#endif
    if(ids){
      for (int j = 0; j <= 5; j++){
#ifdef DEBUG_CAIDS
        char *t2 = (char*)malloc(8);
        sprintf(t2, "%#0x ", ids[j]);
        strcat(t1, t2);
        free(t2);
#endif
        all_ca_ids[i][j] = ids[j];
      }
    } else
	all_ca_ids[i][0] = 0;
  }
#ifdef DEBUG_CAIDS
  esyslog(t1);
  free(t1);
  char *t3  = (char*)malloc(1024);
  strcpy(t3, "reelcam: channel-CAIDs: ");
  for(int i=0; i<=5 ; i++){
    char *t4 = (char*)malloc(8);
    sprintf(t4, "%#0x ", ch_caids[i]);
    strcat(t3, t4);
    free(t4);
  }
  esyslog(t3);
  free(t3);
#endif
    if(ch_caids[0] > 0 && ch_caids[0] <= 3){ /* if certain CAM requested */
      int cam_nr = ch_caids[0] - 1;
	d(esyslog("reelcam ProvidesCa(): requesting cam nr. %i : device %i slotInUse[cam_nr]: %i SlotOnDev(cardIndex): %i\n", cam_nr, cardIndex, slotInUse[cam_nr], SlotOnDev(cardIndex)))

        if((slotInUse[cam_nr] >= 0) && (slotInUse[cam_nr] != cardIndex)){ /* if other tuner uses the requested cam */
        //if(((slotInUse[cam_nr] >= 0) && (slotInUse[cam_nr] != cardIndex)) || (SlotOnDev(cardIndex)>=0 && SlotOnDev(cardIndex) != cam_nr)){ /* if other tuner uses the requested cam */
	  d(esyslog("reelcam ProvidesCa(): requesting cam nr. %i : false for device %i - it is used by slot nr: %i\n", cam_nr, cardIndex, slotInUse[cam_nr]))
	  return false;
	}
      for (int i = 0; i < MAX_CI_SLOTS; i++){
        if(CanDo(cam_nr, channelNumber)){      /* if unused or just by the right device and not used for recording */
          d(esyslog("reelcam ProvidesCa(): requesting cam nr. %i : true 'cause can do more for device %i, slotInUse[%i]: %i\n", cam_nr, cardIndex, cam_nr, slotInUse[cam_nr]))
          return true;
        }
      }
      return false;
    }
    if(ch_caids[0] == 0){
      d(esyslog("reelcam ProvidesCa(): true for dev %i - unencrypted channel\n", cardIndex))
      return true;
    }
    for (int i = 0; i < MAX_CI_SLOTS; i++){ /* For all 3 slots */
       if (all_ca_ids[i][0] == 0){
         d(esyslog("reelcam ProvidesCa(): false for cam nr. %i - cam not existent...\n", i))
       } else 
       if( (slotInUse[i] != cardIndex) && (slotInUse[i] >= 0) ){
       //if( ((slotInUse[i] != cardIndex) && (slotInUse[i] >= 0) ) || (SlotOnDev(cardIndex)>=0 && SlotOnDev(cardIndex) != i)){
         d(esyslog("reelcam ProvidesCa(): false for cam nr. %i and device %i - slot uses device nr: %i\n", i, cardIndex, slotInUse[i]))
       } else
       for (int j = 0; j <= MAXCAIDS; j++){
           for (int k = 0; k <= MAXCAIDS; k++){
               if((all_ca_ids[i][k] == ch_caids[j]) && CanDo(i, channelNumber)){  /* if "encrypted", "CAM not used yet or used by the right device and not used for recording" and "suitable CAM" */
                 d(esyslog("reelcam ProvidesCa(): true 'cause not yet used for recording for cam nr %i and device %i, slotInUse[%i]: %i\n", i, cardIndex, i, slotInUse[i]))
	         return true;
               }
	   }
        }
    }
#if 0
  char* t1 = (char*)malloc(256);
  sprintf(t1, "reelcam ProvidesCa(): no suitable cam / tuner found (device %i switched to channel with caids ", device->CardIndex());
  for(int i=0; i<=MAXCAIDS ; i++){
    char* t2 = (char*)malloc(8);
    sprintf(t2, "%#0x ", ch_caids[i]);
    strcat(t1, t2);
    free(t2);
  }
  char* t3 = (char*)malloc(128);
  sprintf(t3, ") slotInUse[0]: %i, slotInUse[1]: %i, slotInUse[2]: %i\n", slotInUse[0], slotInUse[1], slotInUse[2]);
  strcat(t1, t3);
  free(t3);
  esyslog(t1);
  free(t1);
#endif
  return false;
}

int cRcStatus::CamReset(int slot){
  d(esyslog("reelcam: cam reset on slot %i\n", slot))
  slotInUse[slot] = -1;
  return 0;
}

inline bool cRcStatus::IsViaccess(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strcmp(camName, "Viaccess"))
    return true;
  return false;
}

inline bool cRcStatus::IsAlpha(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strcmp(camName, "AlphaCrypt") || !strcmp(camName, "AlphaCrypt Light"))
    return true;
  return false;
}

inline bool cRcStatus::IsAlphaTC(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
   if(!strcmp(camName, "AlphaCrypt TC") || !strcmp(camName, "easy.TV"))
     return true;
  return false;
}

inline bool cRcStatus::IsDiablo(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strncmp(camName, "STEALTH", 7)) 
    return true;
  return false; 
}

inline bool cRcStatus::IsNagra(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strcmp(camName, "NagraVision")) 
    return true;
  return false; 
}

inline bool cRcStatus::IsAstonDual(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strncmp(camName, "Aston Module 2", 14))
    return true;
  return false;
}

inline bool cRcStatus::IsCanalDigitaal(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strcmp(camName, "CANALDIGITAAL"))
    return true;
  return false;
}

inline bool cRcStatus::IsPowerCam(int slot){
  const char *camName = (cDevice::GetDevice(0))->CiHandler()->GetCamName(slot);
  if(!strncmp(camName, "PowerCam", 8))
    return true;
  return false;
}

unsigned int cRcStatus::MaxChannels(int slot){
  if(nocamlimit)
    return 999;
  if(IsAlpha(slot))
    return ALPHA_MAX;
  else if(IsAlphaTC(slot))
    return ALPHATC_MAX;
  else if(IsViaccess(slot))
    return VIACCESS_MAX;
  else if(IsAstonDual(slot))
    return ASTON_DUAL_MAX;
  else if(IsDiablo(slot))
    return DIABLO_MAX;
  else if(IsNagra(slot))
    return NAGRA_MAX;
  else if(IsCanalDigitaal(slot))
    return CANALDIGITAAL_MAX;
  else if(IsPowerCam(slot))
    return POWERCAM_MAX;
  return 1;
}

bool cRcStatus::CanDo(int slot, int channelNumber){
  if(multica) {
    if (channelsRecording[slot].size() == 0){ /* if not yet recording */
      d(esyslog("reelcam: true 'cause not yet recording for cam nr %i, channelnr: %i, slotInUse[%i]: %i\n", slot, channelNumber, slot, slotInUse[slot]))
      return true;
    }
    for (unsigned int i = 0; i < channelsRecording[slot].size(); i++){
      if (channelsRecording[slot][i] == (unsigned int)channelNumber){ /* if already recording that channel */
	d(esyslog("reelcam: true 'cause already decrypting that channel for cam nr %i, channelnr: %i, slotInUse[%i]: %i\n", slot, channelNumber, slot, slotInUse[slot]))
        return true;
      }
    }
    if (channelsRecording[slot].size() < MaxChannels(slot)){ /* if recording, but still can do more */
      d(esyslog("reelcam: true 'cause can do more for cam nr %i, channelnr: %i, slotInUse[%i]: %i\n", slot, channelNumber, slot, slotInUse[slot]))
      return true;
    }
    return false;
  } else {
    if (channelsRecording[slot].size() == 0 || channelsRecording[slot][0] == (unsigned int)channelNumber)
      return true;
    return false;
  }
}

