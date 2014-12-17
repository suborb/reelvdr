#ifndef ROTOR_STATUS_H
#define ROTOR_STATUS_H

#include <vdr/status.h>

class cStatusMonitor:public cStatus {
private:
  int oldupdate;
  static bool transfer;
  cChannel* Channel;
protected:
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
public:
  static void ChannelSwitchAction(const cDevice *Device, cChannel *Channel);
  cStatusMonitor();
};

#endif
