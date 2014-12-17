#ifndef ___STATUS_H
#define ___STATUS_H

#include <vdr/thread.h>
#include "common.h"
#include <vector>


#define MAXDVBAPI 4
#define MAXSCCAPS 10

class cRcStatus : public cStatus { //, public cThread {
  private:
    int fd;
    signed char slotInUse[MAX_CI_SLOTS]; /* slotInUse[i]==-1 means "slot i unused", slotInUse[i]==j means "slot i used by tuner j */
    std::vector <unsigned int> channelsRecording[MAX_CI_SLOTS]; /* list of currently recording channels on each slot */
    time_t now;
    bool active;
    bool IsViaccess(int slot);
    bool IsAlpha(int slot);
    bool IsAlphaTC(int slot);
    bool IsDiablo(int slot);
    bool IsAstonDual(int slot);
    bool IsNagra(int slot);
    bool IsCanalDigitaal(int slot);
    bool IsPowerCam(int slot);
    unsigned int MaxChannels(int slot);
    bool CanDo(int slot, int channelNumber);
  protected:
    virtual void ChannelSwitch(const cDevice *device, int chNumber);
#if VDRVERSNUM && VDRVERSNUM >= 10401 
    virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On, int ChannelNumber);
#else 
    virtual void Recording(const cDevice *Device, const char *Name, bool On, int chNumber);
#endif 
#if 0
    virtual void Action(void);
#endif
  public:
    cRcStatus(int Fd);
   ~cRcStatus(void);
   int ProvidesCa(const cDevice *device, const unsigned short *caids, int channelNumber);
   int SlotOnDev(const cDevice *device);
   int SlotOnDev(int DeviceNumber);
   int CamReset(int Slot);
};


#endif // ___STATUS_H

