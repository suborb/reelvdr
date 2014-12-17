#include "diseqc.h"
#include "menu.h"
#include <string.h>
#include <vdr/plugin.h>
#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>
#include <math.h>
#include <vdr/dvbdevice.h>
#include <vdr/diseqc.h>

// --- DiseqcCommand ------------------------------------------------------

void DiseqcCommand(int Tuner, int KNr, int p1, int p2)
{
  struct dvb_diseqc_master_cmd switch_cmds[] = {
        { { 0xe0, 0x31, 0x60, 0x00, 0x00, 0x00 }, 3 },  //0 Stop Positioner movement
        { { 0xe0, 0x31, 0x63, 0x00, 0x00, 0x00 }, 3 },  //1 Disable Limits
        { { 0xe0, 0x31, 0x66, 0x00, 0x00, 0x00 }, 3 },  //2 Set East Limit
        { { 0xe0, 0x31, 0x67, 0x00, 0x00, 0x00 }, 3 },  //3 Set West Limit
        { { 0xe0, 0x31, 0x68, 0x00, 0x00, 0x00 }, 4 },  //4 Drive Motor East continously
        { { 0xe0, 0x31, 0x68,256-p1,0x00, 0x00 }, 4 },  //5 Drive Motor East nn steps
        { { 0xe0, 0x31, 0x69,256-p1,0x00, 0x00 }, 4 },  //6 Drive Motor West nn steps
        { { 0xe0, 0x31, 0x69, 0x00, 0x00, 0x00 }, 4 },  //7 Drive Motor West continously
        { { 0xe0, 0x31, 0x6a, p1,   0x00, 0x00 }, 4 },  //8 Store nn
        { { 0xe0, 0x31, 0x6b, p1,   0x00, 0x00 }, 4 },  //9 Goto nn
        { { 0xe0, 0x31, 0x6f, p1,   0x00, 0x00 }, 4 },  //10 Recalculate Position
        { { 0xe0, 0x31, 0x6a, 0x00, 0x00, 0x00 }, 4 },  //11 Enable Limits
        { { 0xe0, 0x31, 0x6e, p1,   p2,   0x00 }, 5 },  //12 Gotoxx
        { { 0xe0, 0x10, 0x38, 0xF4, 0x00, 0x00 }, 4 }   //13 User
  };
  int n=0;
  while (n<10 && !cDevice::GetDevice(Tuner-1)->SendDiseqcCmd(switch_cmds[KNr])) 
  {
    usleep(15000);
    n++;
  }
  if (n==10)
    dsyslog("Failed to send diseqc command!\n");
    switch_cmds[KNr].msg[0]=0xe1;
    for (int i=0; i<2;)
    {
      usleep(15000);
      if (cDevice::GetDevice(Tuner-1)->SendDiseqcCmd(switch_cmds[KNr])) 
        i++;
    }
  }

// --- GotoX -------------------------------------------------------------

void GotoX(int Tuner, int Source)
{
  int gotoXTable[10] = { 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };
  int satlong = (Source & ~0xC800);
  if ((Source & 0xC000) != 0x8000)
    return;
  if (Source & 0x0800)
    satlong = satlong * (-1);
  int Long=data.Long;
  int Lat=data.Lat;
  double azimuth=M_PI+atan(tan((satlong-Long)*M_PI/1800)/sin(Lat*M_PI/1800));
  double x=acos(cos((satlong-Long)*M_PI/1800)*cos(Lat*M_PI/1800));
  double elevation=atan((cos(x)-0.1513)/sin(x));
  double SatHourangle=180+atan((-cos(elevation)*sin(azimuth))/(sin(elevation)*cos(Lat*M_PI/1800)-cos(elevation)*sin(Lat*M_PI/1800)*cos(azimuth)))*180/M_PI;
  int tmp=(int)(fabs(180-SatHourangle)*10);
  tmp=(tmp/10)*0x10 + gotoXTable[ tmp % 10 ];
  int p2=(tmp%0x0100);
  int p1=(tmp/0x0100);
  if (SatHourangle < 180)
    p1 |= 0xe0;
  else
    p1 |= 0xd0;
  DiseqcCommand(Tuner,Gotox,p1,p2);
}


