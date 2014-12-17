/*
 * Frontpanel communication for Reelbox PVR1100
 *
  * (c) Georg Acher, acher (at) baycom dot de
 *     BayCom GmbH, http://www.baycom.de
 *
 * #include <gpl-header.h>
 *
 * $Id: vdr_control.c,v 1.1 2004/09/28 00:19:09 acher Exp $
*/

#include <stdlib.h>
#include <linux/cdrom.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h> 

#include "frontpanel.h"
#define STD_FLG (CS8|CREAD|HUPCL)

static int fd;
int rs232_fd;
/*-------------------------------------------------------------------------*/
void fp_noop(void)
{
	char buf[]={0xa6};
	write(fd,buf,1);
}
/*-------------------------------------------------------------------------*/
void fp_get_version(void)
{
	char buf[]={0xa5,0x00,0x00};
	write(fd,buf,3);
}
/*-------------------------------------------------------------------------*/
void fp_enable_messages(int n)
{
	char buf[]={0xa5,0x01,n};
	write(fd,buf,3);
}
/*-------------------------------------------------------------------------*/
void fp_display_brightness(int n)
{
	char buf[]={0xa5,0x02,n,00};
	write(fd,buf,4);
}
/*-------------------------------------------------------------------------*/
void fp_display_contrast(int n)
{
	char buf[]={0xa5,0x03,n,00};
	write(fd,buf,4);
}
/*-------------------------------------------------------------------------*/
void fp_clear_display(void)
{
	char buf[]={0xa5,0x04};
	write(fd,buf,2);
}
/*-------------------------------------------------------------------------*/
// Ignore, use display_cmd or display_data instead
void fp_write_display(unsigned char *data, int datalen)
{
	char buf[]={0xa5,0x05};
	write(fd,buf,2);
	write(fd,data,datalen);
}
/*-------------------------------------------------------------------------*/
void fp_display_cmd(char cmd)
{
	char xbuf[]={0xa5, 0x05, 3, 0, cmd};
	write(fd,xbuf,5);
}
/*-------------------------------------------------------------------------*/
void fp_display_data(char *data, int l)
{
	char buf[64]={0xa5,0x05,l+2,+1};
	memcpy(buf+4,data,l);
	write(fd,buf,l+4);
	u_sleep(3000);
}
/*-------------------------------------------------------------------------*/
void fp_set_leds(int blink, int state)
{
	char buf[]={0xa5,0x06,blink,state};
	write(fd,buf,4);
}
/*-------------------------------------------------------------------------*/
void fp_set_clock(void)
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
/*-------------------------------------------------------------------------*/
void fp_set_wakeup(time_t t)
{
	char buf[]={0xa5,0x08,t>>24,t>>16,t>>8,t};
	write(fd,buf,6);
}
/*-------------------------------------------------------------------------*/
void fp_set_displaymode(int m)
{
	char buf[]={0xa5,0x09,m};
	write(fd,buf,3);
}
/*-------------------------------------------------------------------------*/
void fp_set_switchcpu(int timeout)
{
	char buf[]={0xa5,0x0a,0x42,0x71,timeout};
	write(fd,buf,5);
}
/*-------------------------------------------------------------------------*/
void fp_get_clock(void)
{
	char buf[]={0xa5,0x0b};
	write(fd,buf,2);
}
/*-------------------------------------------------------------------------*/
void fp_set_clock_adjust(int v1, int v2)
{
	char buf[]={0xa5,0x0a,v1,v2};
	write(fd,buf,4);
}
/*-------------------------------------------------------------------------*/
int fp_open_serial(void)
{
	int bdflag=B115200;//B57600;
	struct termios tty;

	fd=open(FP_DEVICE, O_RDWR|O_NONBLOCK|O_NOCTTY);
	if (fd<0)
		return -1;

	tcgetattr(fd, &tty);
        tty.c_iflag = IGNBRK | IGNPAR;
        tty.c_oflag = 0;
        tty.c_lflag = 0;
        tty.c_line = 0;
        tty.c_cc[VTIME] = 0;
        tty.c_cc[VMIN] = 1;
        tty.c_cflag =STD_FLG;
        cfsetispeed(&tty,bdflag);      
        cfsetospeed(&tty,bdflag);      
        tcsetattr(fd, TCSAFLUSH, &tty); 
        rs232_fd=fd;
	return 0;
}
/*-------------------------------------------------------------------------*/
void fp_close_serial(void)
{
	close(fd);
}
/*-------------------------------------------------------------------------*/
size_t fp_read(unsigned char *b, size_t len)
{
	return read(fd,b,len);
}
/*-------------------------------------------------------------------------*/
int get_answer_length(int cmd)
{
	if (cmd==0xf2 || cmd==0xf1)
		return 4;
	else if (cmd==0x00)
		return 2;
	else if (cmd==0xb)
		return 7;
	else 
		return 0;
}
/*-------------------------------------------------------------------------*/
int fp_read_msg(unsigned char *msg, int ms)
{
	static int state=0;
	static int cmd_length=0;
	unsigned char buf[10];
	int l;

	while(1) {

		l=fp_read(msg+state,1);
		if (l!=1) {
			ms--;
			if (ms<0)
				return 0;
			u_sleep(1000);
			continue;
		}
		
		if (state==0) {
			if (msg[0]==0x5a)
				state=1;
		}
		else if (state==1) {
			cmd_length=get_answer_length(msg[1]);
			if (cmd_length==0) {
				state=0;
				return 2;
			}
			state++;
		}
		else {
			if (state==1+cmd_length) {
				state=0;
				return 2+cmd_length;
			}
			state++;
		}
	}

	return 0;
}

/*-------------------------------------------------------------------------*/

int u_sleep(long long usec)
{
	struct timespec st_time;
	st_time.tv_sec = 0; 
	st_time.tv_nsec = (usec * 1000);
	return nanosleep(&st_time,NULL);
}

