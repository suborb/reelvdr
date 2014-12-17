/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __VDR_DVDDEV_H
#define __VDR_DVDDEV_H

class cDVD {
private:
  static cDVD *dvdInstance;
  static const char *deviceName;

  static int Command(int Cmd);
public:
  cDVD(void);
  ~cDVD();
  static void SetDeviceName(const char *DeviceName);
  static const char *DeviceName(void);
  static bool DriveExists(void);
  static bool DiscOk(void);
  static void Eject(void);

  static cDVD *getDVD(void);
  };

#endif //__VDR_DVDDEV_H
