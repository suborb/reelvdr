#include "status.h"
#include "menu.h"

cStatusMonitor::cStatusMonitor()
{
  for (int i=0; i<4; i++)
    data.ActualSource[i]=0;
}

bool cStatusMonitor::transfer = false;

void cStatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  if (Channels.GetByNumber(ChannelNumber) && !data.sw)
    ChannelSwitchAction(Device,Channels.GetByNumber(ChannelNumber)); 
}

void cStatusMonitor::ChannelSwitchAction(const cDevice *Device, cChannel *Channel)
{
  if (Device!=cDevice::PrimaryDevice() || cDevice::ActualDevice()==cDevice::PrimaryDevice() || cDevice::PrimaryDevice()->cDevice::HasProgramme() && transfer)
  {
    int Tuner = Device->CardIndex()+1;
    int Type = ((::Setup.DiSEqC & (TUNERMASK << (TUNERBITS * Device->CardIndex()))) >> (TUNERBITS * Device->CardIndex())) & ROTORMASK;
    if (!(::Setup.DiSEqC & DIFFSETUPS)) 
        Type = ::Setup.DiSEqC;
    else if ((Type & ROTORLNB) == ROTORLNB) {
        Tuner = (Type & 0x30) >> 4; 
        Type = ((::Setup.DiSEqC & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1))) & ROTORMASK; 
        }
    bool UseGotoX = (Type & ROTORMASK) == GOTOX;  
    bool UseGotoPos = (Type & ROTORMASK) == DISEQC12; 
    int channelsource = UseGotoX ? SatAngles.GetfromSource(Channel->Source())->Code(Tuner) : Channel->Source();
    if ((UseGotoX || UseGotoPos) && channelsource && !Diseqcs.ProvidesSource(Channel->Source(),Device->CardIndex()+1))
      if (channelsource!=data.ActualSource[Tuner-1])  
      {
        if (UseGotoX || UseGotoPos)
        {
          data.ActualSource[Tuner-1]=channelsource;
          if (UseGotoX)
            GotoX(Tuner,data.ActualSource[Tuner-1]);
          if (UseGotoPos && RotorPositions.GetfromSource(data.ActualSource[Tuner-1])->Pos(Tuner))
            DiseqcCommand(Tuner,Goto,RotorPositions.GetfromSource(data.ActualSource[Tuner-1])->Pos(Tuner));
        }
      }
  }
  if ((cDevice::ActualDevice()!=cDevice::PrimaryDevice()) && (Device==cDevice::PrimaryDevice()) )
    transfer=true;
  if ((cDevice::ActualDevice()==cDevice::PrimaryDevice()) && (Device==cDevice::PrimaryDevice())) transfer=false;
}
