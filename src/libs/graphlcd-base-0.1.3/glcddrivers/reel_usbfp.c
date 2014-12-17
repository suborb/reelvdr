/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  reel_usbfp.c - Plugin for Reel USB OLED and AVG serial frontpanel
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

#include "common.h"
#include "config.h"
#include "port.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <termios.h>
#include <usb.h>

#include <sys/time.h>
#include <sys/ioctl.h>

#include "reel_usbfp.h"
#include "st7565_reel.h"

#define VENDOR_ID         0x16d0 
#define PRODUCT_ID        0x054b

#define VENDOR_ID1         0x16d0 
#define PRODUCT_ID1        0x054b

#define DISP_RESET_ON     0xb9
#define DISP_RESET_OFF    0xb8
#define DISP_NORMAL       0xc2

#define INIT_DATA_SIZE    90

namespace GLCD
{

unsigned char ReelUSBFP_InitData[] =
{ 
0x15, 0x00, /* Set Column Address */
0x00, 0x00, /* Start = 0 */
0x3F, 0x00, /* End = 127 */

0x75, 0x00, /* Set Row Address */
0x00, 0x00, /* Start = 0 */
0x3F, 0x00, /* End = 63 */

0x81, 0x00, /* Set Contrast Control (1) */
0x7F, 0x00, /* 0 ~ 127 */

0x86, 0x00, /* Set Current Range 84h:Quarter, 85h:Half, 86h:Full*/

0xA0, 0x00, /* Set Re-map */
0x41, 0x00, /* [0]:MX, [1]:Nibble, [2]:H/V address [4]:MY, [6]:Com Split Odd/Even "1000010"*/

0xA1, 0x00, /* Set Display Start Line */
0x00, 0x00, /* Top */

0xA2, 0x00, /* Set Display Offset */
0x44, 0x00, /*44 Offset 76 rows */

0xA4, 0x00, /* Set DisplaMode,A4:Normal, A5:All ON, A6: All OFF, A7:Inverse */

0xA8, 0x00, /* Set Multiplex Ratio */
0x3F, 0x00, /* 64 mux*/

0xB1, 0x00, /* Set Phase Length */
0x53, 0x00, /*53 [3:0]:Phase 1 period of 1~16 clocks */

/* [7:4]:Phase 2 period of 1~16 clocks POR = 0111 0100 */

0xB2, 0x00, /* Set Row Period */
0x46, 0x00, /* 46 [7:0]:18~255, K=P1+P2+GS15 (POR:4+7+29)*/

0xB3, 0x00, /* Set Clock Divide (2) */
0x91, 0x00, /* [3:0]:1~16, [7:4]:0~16, 70Hz */

0xBF, 0x00, /* Set VSL */
0x0D, 0x00, /* [3:0]:VSL */

0xBE, 0x00, /* Set VCOMH (3) */
0x02, 0x00, /* [7:0]:VCOMH, (0.51 X Vref = 0.51 X 12.5 V = 6.375V)*/

0xBC, 0x00, /* Set VP (4) */
0x0F, 0x00, /* [7:0]:VP, (0.67 X Vref = 0.67 X 12.5 V = 8.375V) */

0xB8, 0x00, /* Set Gamma with next 8 bytes */
0x01, 0x00, /* L1[2:1] */
0x11, 0x00, /* L3[6:4], L2[2:0] 0001 0001 */
0x22, 0x00, /* L5[6:4], L4[2:0] 0010 0010 */
0x32, 0x00, /* L7[6:4], L6[2:0] 0011 1011 */
0x43, 0x00, /* L9[6:4], L8[2:0] 0100 0100 */
0x54, 0x00, /* LB[6:4], LA[2:0] 0101 0101 */
0x65, 0x00, /* LD[6:4], LC[2:0] 0110 0110 */
0x76, 0x00, /* LF[6:4], LE[2:0] 1000 0111 */

0xAD, 0x00, /* Set DC-DC */
0x03, 0x00, /* 03=ON, 02=Off */

0xAF, 0x00, /* AF=ON, AE=Sleep Mode */
0xA4, 0x00, /* Set DisplaMode,A4:Normal, A5:All ON, A6: All OFF, A7:Inverse */
0xA1, 0x00, /* Set Display Start Line */
0x00, 0x00 /* Top */
};

#define USB_REFRESH_TIMEOUT 15

cDriverReelUSBFP::cDriverReelUSBFP(cDriverConfig * config)
	: config(config)
{
	oldConfig = new cDriverConfig(*config);
	last_refresh=0;
	secondary=NULL;
	LCD=NULL;
	for(int i=0;i<MAX_USB_HANDLES;i++) {
		displays[i].state=0;
		displays[i].handle=NULL;
		displays[i].device=NULL;
	}
	usb_init();
        usb_set_debug(0);

	struct stat statbuf;
	int ret=stat("/dev/frontpanel",&statbuf);
	if (!ret) {
		printf("Use AVG frontpanel as secondary display\n");
		secondary=new cDriverST7565R(config);
	}
}

cDriverReelUSBFP::~cDriverReelUSBFP()
{
	delete oldConfig;
	if (secondary)
		delete secondary;
	secondary=NULL;
}

int cDriverReelUSBFP::Init()
{  
	width = config->width;

	if (width < 0)
                width = 128;
        height = config->height;
        if (height < 0)
                height = 64;

	// setup lcd array, 2 pixels per byte
	LCD = (unsigned char*)malloc(width*height/2);
	if (LCD)
		memset(LCD, 0, width*height/2);

	*oldConfig = *config;
	brightness=15;
	
	if (secondary)
		secondary->Init();
	return 0;
}

int cDriverReelUSBFP::DeInit()
{ 
	Clear();
	Refresh(1);
	if (LCD)
	{
		free(LCD);
		LCD=NULL;
	}
	if (secondary)
		secondary->DeInit();

	return 0;
}

int cDriverReelUSBFP::CheckSetup()
{
	if (config->brightness != oldConfig->brightness)
	{
		oldConfig->brightness = config->brightness;
                SetBrightness(config->brightness);
		usleep(500000);
	}
	if (config->contrast != oldConfig->contrast)
	{
		oldConfig->contrast = config->contrast;
                SetContrast(config->contrast);	
	}
	if (config->upsideDown != oldConfig->upsideDown ||
            config->invert != oldConfig->invert)
        {
                oldConfig->upsideDown = config->upsideDown;
                oldConfig->invert = config->invert;
                return 1;
        }
	return 0;
}

void cDriverReelUSBFP::Clear()
{
	if (LCD)
		memset(LCD, 0, width*height/2);
	if (secondary)
		secondary->Clear();
}

void cDriverReelUSBFP::SetPixel(int x, int y)
{
	unsigned char c;
	if (!LCD)
		return;
	clip(x, 0, width - 1);
	clip(y, 0, height - 1);

	c = 0x80;
	if (!config->upsideDown)
	{
		x = width - 1 - x;
		y = height - 1 - y;
	}
	
	// Default: Full brightness (0xf)
	int idx=y*width/2+(x>>1);
	if (x&1)
		LCD[idx] = (LCD[idx]&0x0f) | 0xf0 ;
	else
		LCD[idx] = (LCD[idx]&0xf0) | 0x0f ;
//	if (secondary)
//		secondary->SetPixel(x,y);
}


void cDriverReelUSBFP::Set8Pixels(int x, int y, unsigned char data)
{
	unsigned char bdata=data;
	int bx=x,by=y;
	if (!LCD)
		return;

	clip(y, 0, height - 1);
	if ( config->upsideDown)
	{
		// normal orientation
		for(int i=0;i<8;i++) {
			clip(x, 0, width - 1);
			int idx=y*width/2+(x>>1);
			int mask=0;
			if (data&0x80)
				mask=0xf0;
			data<<=1;
			if (!(x&1))
				LCD[idx] = (LCD[idx]&0x0f) | (mask) ;
			else
				LCD[idx] = (LCD[idx]&0xf0) | (mask>>4) ;
			x++;
		}
	}
	else
	{
		// upside down orientation
		x = width - 1 - x;
		y = height - 1 - y;
		for(int i=0;i<8;i++) {
			clip(x, 0, width - 1);
			int idx=y*width/2+(x>>1);
			int mask=0;
			if (data&0x80)
				mask=0xf0;
			data<<=1;
			if (!(x&1))
				LCD[idx] = (LCD[idx]&0x0f) | (mask) ;
			else
				LCD[idx] = (LCD[idx]&0xf0) | (mask>>4) ;
			x--;
		}
	}
	if (secondary)
		secondary->Set8Pixels(bx,by,bdata);
}

void cDriverReelUSBFP::RefreshUsb(void)
{
	struct usb_bus *bus;
        struct usb_device *dev;
        usb_dev_handle *disp_handle = 0;
	int ret1,ret2;
        ret1=usb_find_busses();
        ret2=usb_find_devices();
        for (bus = usb_busses; bus; bus = bus->next)
        {
                for (dev = bus->devices; dev; dev = dev->next)
                {
                        if ( ((dev->descriptor.idVendor == VENDOR_ID) && (dev->descriptor.idProduct == PRODUCT_ID)) ||
			     ((dev->descriptor.idVendor == VENDOR_ID1) && (dev->descriptor.idProduct == PRODUCT_ID1)))
                        {
				int found=0;
				for(int i=0;i<MAX_USB_HANDLES;i++) {
					if (displays[i].state && displays[i].device==dev) { // display already known
						displays[i].state=2;
						found=1;
						break;
					}
				}
				if (found)
					break;
                                printf("New GrauTec display device found at bus %s, device %s \n", dev->bus->dirname, dev->filename);
				for(int i=0;i<MAX_USB_HANDLES;i++) {
					if (!displays[i].state) {
						disp_handle = usb_open(dev);
						if (disp_handle) {
							int ret=SetupDisplay(disp_handle);
							displays[i].state=2;
							displays[i].handle=disp_handle;
							displays[i].device=dev;
							printf("Allocated display %i, ret %i\n",i,ret);
						}
						break;
					}
				}
                        }
                }
        }

	// Cleanup dead displays
	for(int i=0;i<MAX_USB_HANDLES;i++) {
		if (displays[i].state==1) {
			printf("Deallocated display %i\n",i);
			if (displays[i].handle)
				usb_close(displays[i].handle);
			displays[i].handle=NULL;
			displays[i].state=0;
			displays[i].device=NULL;
		}
		else if (displays[i].state==2) {
			displays[i].state=1;
		}
	}
	last_refresh=time(0);
}

void cDriverReelUSBFP::BulkData(struct usb_dev_handle *disp_handle, unsigned char *data, int size)
{
        int send_status;

        if (!disp_handle)
                return;
	usb_claim_interface(disp_handle, 0);
        // int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
        send_status = usb_bulk_write(disp_handle, 2, (char*) (data), size, 100);
	usb_release_interface(disp_handle, 0);
        return;
}

int cDriverReelUSBFP::SetupDisplay(struct usb_dev_handle *disp_handle)
{
	char receive_data[64];
        int send_status;

        if (!disp_handle)
                return -1;

        // int usb_control_msg(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);
        send_status = usb_control_msg(disp_handle, 0xA0, DISP_RESET_ON, 0, 1, receive_data, 4, 200);
	usleep(10*1000);
        send_status = usb_control_msg(disp_handle, 0xA0, DISP_RESET_OFF, 0, 1, receive_data, 4, 200);
        usleep(50*1000);
        send_status = usb_control_msg(disp_handle, 0xA0, DISP_NORMAL, 0, 1, receive_data, 64, 200);
        usleep(100*1000);

	usb_claim_interface(disp_handle, 0);
        // Fix init problem on client
        char dummy[2]={0,0};
        // int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
        send_status = usb_bulk_write(disp_handle, 2, dummy, 2, 500);                

        send_status = usb_bulk_write(disp_handle, 2, (char*) (ReelUSBFP_InitData), INIT_DATA_SIZE, 200);
	usb_release_interface(disp_handle, 0);

        if (send_status != INIT_DATA_SIZE)
                return -1;
	return 0;
}

void cDriverReelUSBFP::SendData( unsigned char *data, int l)
{
	for(int i=0;i<MAX_USB_HANDLES;i++) {
		if (displays[i].state)
			BulkData(displays[i].handle,data,l);
	}
}

#define min(a,b) ((a)<(b)?(a):(b))

void cDriverReelUSBFP::SetBrightness(unsigned int percent)
{
	if (secondary)
		secondary->SetBrightness(percent);

	int n=static_cast<int>(percent/6);
	if (n>15)
		n=15;
	int b=brightness;
	while(b!=n) {
		if (b<n)
			b+=min(4,n-b);
		if (b>n)
			b-=min(4,b-n);
		brightness=b;
		RefreshInternal();
	}
}

void cDriverReelUSBFP::RefreshInternal(void)
{
	int n;
	int invert;
	unsigned char lcd_data[8192];
	int br;

	CheckSetup();
	if (!LCD)
		return;

	if (last_refresh+USB_REFRESH_TIMEOUT<time(0))
		RefreshUsb();

	invert=(config->invert != 0) ? 0xff : 0x00;
	br=(brightness<<4)|brightness;

	// Merge with brightness, add dummy byte
	for(n=0;n<4096;n++) {
		lcd_data[2*n]=LCD[n] & br;
		lcd_data[2*n+1]=0xff;
	}
	
	SendData(lcd_data, 8192);
}

void cDriverReelUSBFP::Refresh(bool refreshAll)
{
	if (secondary)
		secondary->Refresh(refreshAll);

	RefreshInternal();
}

}
