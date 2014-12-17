/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

//#define DVDDEBUG

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif

#include "dvddev.h"

#ifndef __QNXNTO__
#include <linux/cdrom.h>
#endif

#ifdef DEBUG
#define DVDDEBUG
#undef DEBUG
#else
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

#ifdef DVDDEBUG
#define DEBUG(format, args...) fprintf (stderr, format, ## args)
#else
#define DEBUG(format, args...)
#endif

// --- cDVD ------------------------------------------------------------------

const char *cDVD::deviceName = "/dev/dvd";

cDVD *cDVD::dvdInstance = NULL;

cDVD *cDVD::getDVD(void)
{
  if (!dvdInstance)
     new cDVD;
  return dvdInstance;
}

cDVD::cDVD(void)
{
  dvdInstance = this;
}

int cDVD::Command(int Cmd)
{
  int result = -1;
  int f;
  if ((f = open(deviceName, O_RDONLY | O_NONBLOCK)) > 0) {
     result = ioctl(f, Cmd, 0);
     close(f);
     }
  return result;
}

void cDVD::SetDeviceName(const char *DeviceName)
{
  deviceName = strdup(DeviceName);
}

const char *cDVD::DeviceName(void)
{
  return deviceName;
}

bool cDVD::DriveExists(void)
{
  return access(deviceName, F_OK) == 0;
}

bool cDVD::DiscOk(void)
{
#ifndef __QNXNTO__
  return Command(CDROM_DRIVE_STATUS) == CDS_DISC_OK;
#else
  return 1; // fake ok for QNX
#endif
}

void cDVD::Eject(void)
{
  //  vm_destroy();
#ifndef __QNXNTO__
  Command(CDROMEJECT);
#endif
}

#if 0
#define _lang2int( x )   ((x[0] << 8) | x[1]);

int cDVD::vm_reset(void)
{
  int ret = ::vm_init(const_cast<char *>(deviceName));
  if (ret ==0) {
     ::vm_reset();
     state.registers.SPRM[14] &= ~(0x0f <<  8); 
     state.registers.SPRM[14] |= (Setup.VideoFormat ? 0x03 : 0) << 10; 
     state.registers.SPRM[14] |= 0x02 << 8;   // letterbox is default for dvb/vdr
     state.registers.SPRM[0]  = _lang2int( LangCodes()[Setup.DVDMenuLanguage] );  // Player Menu Languange code
     state.registers.SPRM[16] = _lang2int( LangCodes()[Setup.DVDAudioLanguage] ); // Initial Language Code for Audio
     state.registers.SPRM[18] = _lang2int( LangCodes()[Setup.DVDSpuLanguage] );   // Initial Language Code for Spu
     state.registers.SPRM[20] = _lang2int( LangCodes()[Setup.DVDPlayerRCE] );     // Player Regional Code (bit mask?)
  }
  return ret;
}
#endif
