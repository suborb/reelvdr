/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  st7565_reel.c - Plugin for ST7565 display on Reelbox
 *                  driven by an AVR on the front panel
 *
 *  (c) 2004 Georg Acher, BayCom GmbH, http://www.baycom.de
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "common.h"
#include "config.h"

#include "st7565_reel.h"
#include "port.h"


namespace GLCD
{

cDriverST7565R::cDriverST7565R(cDriverConfig * config)
	: config(config)
{
	//printf("sdfsdfs\n");
	oldConfig = new cDriverConfig(*config);
	LCD=NULL;
}

cDriverST7565R::~cDriverST7565R()
{
	delete oldConfig;
}

int cDriverST7565R::Init()
{  
	int x;
	width = config->width;

	if (width < 0)
                width = 240;
        height = config->height;
        if (height < 0)
                height = 128;

	// setup lcd array
	LCD = (unsigned char**)malloc(((width + 7) / 8)*sizeof(unsigned char*));
	if (LCD)
	{
		for (x = 0; x < (width + 7) / 8; x++)
		{
			LCD[x] = (unsigned char*)malloc(height);
			memset(LCD[x], 0, height);
		}
	}	

	int bdflag=B115200;
	struct termios tty;
	fd = open(FP_DEVICE, O_RDWR|O_NONBLOCK|O_NOCTTY);
	fcntl(fd,F_SETFL,0);
  
	tcgetattr(fd, &tty);
	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_line = 0;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cflag = CS8|CREAD|HUPCL;
	cfsetispeed(&tty,bdflag);      
	cfsetospeed(&tty,bdflag);      
	tcsetattr(fd, TCSAFLUSH, &tty); 

	*oldConfig = *config;
	set_displaymode(0);
	set_clock();
	clear_display();	
	return 0;
}

int cDriverST7565R::DeInit()
{ 
	int x;

	if (fd>0) 
	{
		clear_display();
		set_displaymode(1); // Clock
		close(fd);
	}

	// free lcd array
	if (LCD)
	{
		for (x = 0; x < (width + 7) / 8; x++)
		{
			if (LCD[x]) free(LCD[x]);
		}
		free(LCD);
		LCD=NULL; // avoid double free
	}
	return 0;
}


int cDriverST7565R::CheckSetup()
{
/*
	if (config->DiffersImportantlyWith(oldSetup))
	{
		DeInit();
		Init();
	}
*/
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


void cDriverST7565R::Clear()
{
	if (LCD)
		for (int x = 0; x < (width + 7) / 8; x++)
			memset(LCD[x], 0, height);
}


void cDriverST7565R::SetPixel(int x, int y)
{
	unsigned char c;
	if (!LCD)
		return;
	clip(x, 0, width - 1);
	clip(y, 0, height - 1);

	c = 0x80;
	if (0) //config->upsideDown)
	{
		x = width - 1 - x;
		y = height - 1 - y;
	}

	c = c >> (x % 8);
	LCD[x / 8][y] = LCD[x / 8][y] | c;
}


void cDriverST7565R::Set8Pixels(int x, int y, unsigned char data)
{
	if (!LCD)
		return;

	clip(x, 0, width - 1);
	clip(y, 0, height - 1);

	if (1) // !config->upsideDown)
	{
		// normal orientation
		LCD[x / 8][y] = LCD[x / 8][y] | data;
	}
	else
	{
		// upside down orientation
		x = width - 1 - x;
		y = height - 1 - y;
		LCD[x / 8][y] = LCD[x / 8][y] | ReverseBits(data);
	}
}

void cDriverST7565R::display_cmd( unsigned char cmd)
{
	unsigned char buf[]={0xa5, 0x05, 3, 0, cmd};
        write(fd,buf,5);
}

void cDriverST7565R::display_data( unsigned char *data, int l)
{
        unsigned char buf[64]={0xa5,0x05,l+2,+1};
        memcpy(buf+4,data,l);
        write(fd,buf,l+4);
}
void cDriverST7565R::set_displaymode(int m)
{
        char buf[]={0xa5,0x09,m};
        write(fd,buf,3);
}

void cDriverST7565R::set_clock(void)
{
        time_t t;
        struct tm tm;
        t=time(0);
        localtime_r(&t,&tm); 
	{
		char buf[]={0xa5,0x7,tm.tm_hour,tm.tm_min,tm.tm_sec,
			    t>>24,t>>16,t>>8,t};
		write(fd,buf,9);
	} 
}

void cDriverST7565R::clear_display(void)
{
        char buf[]={0xa5,0x04};
        write(fd,buf,2);
}

void cDriverST7565R::SetBrightness(unsigned int percent)
{
        char buf[]={0xa5,0x02, 0x00, 0x00};
	int n=static_cast<int>(percent*2.5);
	if (n>255)
		n=255;
	buf[2]=(char)(n);
        write(fd,buf,4);	
}

void cDriverST7565R::SetContrast(unsigned int val)
{
        char buf[]={0xa5,0x03, 0x00, 0x00};
	buf[2]=(char)(val*25);
        write(fd,buf,4);	
}

void cDriverST7565R::Refresh(bool refreshAll)
{
	int x,y,xx,yy;
	int i;
	unsigned char c;
	int rx;
	int invert;

	CheckSetup();
	if (!LCD)
		return;
	invert=(config->invert != 0) ? 0xff : 0x00;
  	if (fd>=0)
	{
		for (y = 0; y < (height); y+=8)
		{
			display_cmd( 0xb0+((y/8)&15));
			for (x = 0; x < width / 8; x+=4)
			{
				unsigned char d[32]={0};

				for(yy=0;yy<8;yy++)
				{
					for (xx=0;xx<4;xx++)
					{
						c = (LCD[x+xx][y+yy])^invert;
										
						for (i = 0; i < 8; i++)
						{
							d[i+xx*8]>>=1;
							if (c & 0x80)
								d[i+xx*8]|=0x80;
							c<<=1;
						}
					}
				}
				rx=4+x*8; 
//				printf("X %i, y%i\n",rx,y);

				display_cmd( 0x10+((rx>>4)&15));
				display_cmd( 0x00+(rx&15));
    				display_data( d,32);
			}
		}
        }
}

}
