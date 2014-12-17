/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  reel_usbfp.h - Plugin for Reel USB OLED and AVG serial frontpanel
 *
 *  (c) 2010 Georg Acher, BayCom GmbH, http://www.baycom.de
 *      based on simlcd.c by Carsten Siebholz
 **/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#ifndef _LIBGRAPHLCD_REELUSBFP_H_
#define _LIBGRAPHLCD_REELUSBFP_H_

#include "driver.h"
#include <sys/stat.h>

#include <usb.h>

#define MAX_USB_HANDLES 8

namespace GLCD
{
class cDriverConfig;

typedef struct {
	int state;
	struct usb_dev_handle *handle;
	struct usb_device *device;
} usb_display_t;

class cDriverReelUSBFP : public cDriver
{
private:

	int CheckSetup(void);
	void RefreshUsb(void);
	void RefreshInternal(void);
	void BulkData(struct usb_dev_handle *disp_handle, unsigned char *data, int size);
	int SetupDisplay(struct usb_dev_handle *disp_handle);
	void SendData( unsigned char *data, int l);

	cDriverConfig * config;
        cDriverConfig * oldConfig;
	usb_display_t displays[MAX_USB_HANDLES];
	time_t last_refresh;
	unsigned char * LCD;
	int brightness;

	cDriver *secondary; // secondary display
public:
	cDriverReelUSBFP(cDriverConfig * setup);
	virtual ~cDriverReelUSBFP();
	
	virtual int Init();
	virtual int DeInit();
	
	virtual void Clear();
	virtual void SetPixel(int x, int y);
	virtual void Set8Pixels(int x, int y, unsigned char data);
	virtual void Refresh(bool refreshAll = false);
	virtual void SetBrightness(unsigned int percent);
	virtual void SetContrast(unsigned int percent) {};
};

}

#endif

