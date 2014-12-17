/*****************************************************************************
*
* hw_specials.c - obscure HW tricks for DeCypher...
*
* Copyright (C) 2008 Georg Acher (acher (at) baycom dot de)
*
* #include <gpl_v2.h>
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include <linux/i2c-dev.h>

#ifndef I2C_RDWR

#define I2C_RDWR        0x0707  /* Combined R/W transfer (one stop only)*/
struct i2c_msg {
        __u16 addr;     /* slave address                        */
        __u16 flags;
#define I2C_M_TEN       0x10    /* we have a ten bit chip address       */
#define I2C_M_RD        0x01
#define I2C_M_NOSTART   0x4000
#define I2C_M_REV_DIR_ADDR      0x2000
#define I2C_M_IGNORE_NAK        0x1000
#define I2C_M_NO_RD_ACK         0x0800
        __u16 len;              /* msg length                           */
        __u8 *buf;              /* pointer to msg data                  */
};
#endif

#define FS453_ADDR 0x4a
static int fd;

//---------------------------------------------------------------------------
void RealtimePrioOff()
{
    pthread_t threadId = pthread_self();
    int p;
    struct sched_param param;
    pthread_getschedparam(threadId, &p, &param);
    param.sched_priority = 0;
    pthread_setschedparam(threadId, SCHED_OTHER, &param);
}
//---------------------------------------------------------------------------
void RealtimePrioOn()
{
    pthread_t threadId = pthread_self();
    int p;
    struct sched_param param;
    pthread_getschedparam(threadId, &p, &param);
    param.sched_priority = 4;
    pthread_setschedparam(threadId, SCHED_RR, &param);
}
//---------------------------------------------------------------------------
unsigned long long GetTime()
{
        struct  timeval  ti;
        struct  timezone tzp;
        gettimeofday(&ti,&tzp);
        return ti.tv_usec + 1000000LL * ti.tv_sec;
}
/*-------------------------------------------------------------------------*/
int i2c_init(void)
{
        fd=open("/dev/i2c-0",O_RDWR);
        if (fd<0) 
        {
                printf("Can't open /dev/i2c-0\n");
		return 1;
        }
        return 0;
}
/*-------------------------------------------------------------------------*/
int i2c_write(int addr, unsigned int reg, unsigned long value, unsigned int bytes)
{
        struct i2c_rdwr_ioctl_data data;
        struct i2c_msg msg[2];
        unsigned char buf[20];
	int ret;

        data.msgs=msg;
        data.nmsgs=1;
//        printf("write %x, %x.%i = %x\n",addr,reg,bytes,value);
        msg[0].addr=addr;
        msg[0].flags=0;
        msg[0].len=1+bytes;
        msg[0].buf=buf;
        buf[0]=reg;
        buf[1]=value&255;
        buf[2]=(value>>8)&255;
        buf[3]=(value>>16)&255;
        buf[4]=(value>>24)&255;

        ret=ioctl(fd, I2C_RDWR,&data);
	if (ret!=1)
		printf("fs453-write failed %i\n",ret);
        return 0;
}
/*-------------------------------------------------------------------------*/
int i2c_read(int addr, unsigned int reg, unsigned int *p_value, unsigned int bytes)
{
        struct i2c_rdwr_ioctl_data data;
        struct i2c_msg msg[2];
        unsigned char buf[20];
        unsigned char buf1[20];
        data.msgs=msg;
        data.nmsgs=2;

        msg[0].addr=addr;
        msg[0].flags=0;
        msg[0].len=1;
        msg[0].buf=buf;
        buf[0]=reg;

        msg[1].addr=addr;
        msg[1].flags=I2C_M_RD;
        msg[1].len=bytes;
        msg[1].buf=buf1;
        buf1[0]=buf1[1]=buf1[2]=buf1[3]=0;

        ioctl(fd, I2C_RDWR,&data);
        *p_value=buf1[0]+(buf1[1]<<8)+(buf1[2]<<16)+(buf1[3]<<24);
//      printf("read %x, %x = %x\n",addr,reg, *p_value);
        return 0;
}
/*-------------------------------------------------------------------------*/
int wait_focus_field(int fd)
{
        int v;
	int n;
	i2c_write(FS453_ADDR, 0xb2, 0x6000,2);
	i2c_write(FS453_ADDR, 0xb0, 0x1,2);
        for (n=0;n<100;n++)
        {
		i2c_read(FS453_ADDR, 0xb0, &v, 2);
                if (v == 0) {
                        return 0;
		}
		usleep(2000);
        }
	printf("WaitFocusFieldSync timeout#2!\n");
	return 1;
}
/*-------------------------------------------------------------------------*/
int correct_focus_field(void)
{
	int ret;
	if (!i2c_init()) {
		ret=wait_focus_field(fd);
		close(fd);
		return ret;
	}
	return 1;
}
/*-------------------------------------------------------------------------*/
// There's no VCXO-ioctl, so access the HW directly

#define PERIPHERAL_BASE 0x18000000
#define SE_REG_BASE (0x00500000+PERIPHERAL_BASE)
#define PWM0  0x0048
#define TSI0ADDR 0x0b00
#define TSI0_PWM0_CTRL (TSI0ADDR+PWM0)
char* reg_base=NULL;
int vcxo_inited=0;

int init_vcxo(void)
{
	int fdx;
	if (vcxo_inited)
		return 0;

	fdx=open("/dev/mem",O_RDWR);
	if (fdx<0)
		return -1;
	reg_base=(char*)mmap(0,4*1024, 
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, fdx,
			     (off_t)(SE_REG_BASE));
	if ((int)reg_base==-1) {
		reg_base=NULL;
		return -1;
	}
	close(fdx);
	vcxo_inited=1;
	return 0;
}
/*-------------------------------------------------------------------------*/
// 0: max frequency, 65535: min frequency
void set_vcxo(int pwm)
{
	int tmp = (1 << 16) | pwm;
	if (reg_base)
		*(int*)(reg_base+TSI0_PWM0_CTRL)=tmp;
}
/*-------------------------------------------------------------------------*/
#if 0
int main(int argc, char ** argv)
{
	int x=0;
	int v,n;
	i2c_init();

	while(1) {
		printf("X %i\n",x++);
		correct_focus_field();
//		usleep(20*1000);
	}

	i2c_write(FS453_ADDR, 0xb2, 0x6000,2);
	while(1) {
		printf("X %i\n",x++);
		for(n=0;n<10;n++) {
			i2c_write(FS453_ADDR, 0xb0, 0x1,2);
//			i2c_write(FS453_ADDR, 0x89, 0x00,1);
			i2c_read(FS453_ADDR, 0xb0, &v, 2);
		}
	}
}
#endif
