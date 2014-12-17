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

#include <hdchannel.h> // for hd_data
#include "types.h"
#include "demux.h"

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
static int fd=-1;  // for i2c

#define GPIO_SCART_ASP  11

extern hd_data_t volatile *hda;

//---------------------------------------------------------------------------
// FIXME: Change to ioctl
void set_gpio(int n,int val)
{
        char path[256];
        int fdx;
        sprintf(path,"/sys/bus/platform/drivers/go7x8x-gpio/pin%i",n);
        fdx=open(path,O_RDWR);
        sprintf(path,"%i\n",val);
        write(fdx,path,sizeof(path));
        close(fdx);
}

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
	if (fd<0) {
		fd=open("/dev/i2c-0",O_RDWR);
		if (fd<0) 
		{
			printf("Can't open /dev/i2c-0\n");
			return 1;
		}
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
//		usleep(500);
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
		return ret;
	}
	return 1;
}
//---------------------------------------------------------------------------
void focus_set_WSS(int norm, int value)
{
	if (i2c_init())
		return; 
	if (norm==-1) { // disable WSS-line
		i2c_write(FS453_ADDR, 0x80, 0, 1);
	}
        else if (norm==0) { // PAL
                // FIXME PAL-60
                i2c_write(FS453_ADDR, 0x89, 0x16,1);  // x10 for PAL-60
                i2c_write(FS453_ADDR, 0x8a, 0x16,1);
                i2c_write(FS453_ADDR, 0x81, 0x62f,2);
                i2c_write(FS453_ADDR, 0x8b, 0xc0,2);
                i2c_write(FS453_ADDR, 0x83, value,3);
                i2c_write(FS453_ADDR, 0x86, value,3);
                i2c_write(FS453_ADDR, 0x80, 0x76,1); 
        }
        else {
                // FIXME Check
                i2c_write(FS453_ADDR, 0x89, 0x13, 1);
                i2c_write(FS453_ADDR, 0x8a, 0x13, 1);
                i2c_write(FS453_ADDR, 0x81, 0xc21, 2);
                i2c_write(FS453_ADDR, 0x83, value, 3);
                i2c_write(FS453_ADDR, 0x86, value, 3);
                i2c_write(FS453_ADDR, 0x80, 0x66,1); 
        }
}
/*-------------------------------------------------------------------------*/
// palntsc=0 for PAL
// aspect=1 for 16:9
#if 1
void set_sdtv_aspect(int palntsc, int aspect)
{
	// WSS and voltage only in anamorphic mode
	if ((hda->aspect.scale&HD_VM_SCALE_VID)==HD_VM_SCALE_F2S) {

		if (!aspect) {
			if (palntsc==0)
				focus_set_WSS(0, 0x80000);
			else
				focus_set_WSS(1, 0x180000);
			
			set_gpio(GPIO_SCART_ASP,0); // 12V
		}
		else {
			if (palntsc==0)
				focus_set_WSS(0, 0x70000);
			else
				focus_set_WSS(1, 0x600001);
			
			if ((hda->video_mode.outputa&(0xff00))!=HD_VM_OUTA_PORT_SCART_V1)
				set_gpio(GPIO_SCART_ASP,1); // 6V
			else
				set_gpio(GPIO_SCART_ASP,0); // 12V
		}
	}
	else {
		set_gpio(GPIO_SCART_ASP,0); // 12V
		focus_set_WSS(-1,0); // off
	}
}
#endif
/*-------------------------------------------------------------------------*/
// There's no VCXO-ioctl, so access the HW directly

#define PERIPHERAL_BASE 0x18000000
#define SE_REG_BASE (0x00500000+PERIPHERAL_BASE)
#define DSPC_REG_BASE (0x00600000+PERIPHERAL_BASE)
#define VOP_REG_BASE (0x00680000+PERIPHERAL_BASE)

#define PWM0  0x0048
#define TSI0ADDR 0x0b00
#define TSI0_PWM0_CTRL (TSI0ADDR+PWM0)
#define TSI0_STCBASECURRL_OFF   (0xb68)
#define TSI0_STCBASECURRH_OFF   (0xb6c)
#define TSI1_STCBASECURRL_OFF  (0xc68)
#define TSI1_STCBASECURRH_OFF  (0xc6c)
#define TSI0_STCBASECURRL      (SE_REG_BASE + TSI0_STCBASECURRL_OFF)
#define TSI0_STCBASECURRH      (SE_REG_BASE + TSI0_STCBASECURRH_OFF)
#define TSI1_STCBASECURRL      (SE_REG_BASE + TSI1_STCBASECURRL_OFF)
#define TSI1_STCBASECURRH      (SE_REG_BASE + TSI1_STCBASECURRH_OFF)

char* reg_base=NULL;
char* reg_base1=NULL;
char* reg_base2=NULL;
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

	reg_base1=(char*)mmap(0,4*1024, 
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, fdx,
			     (off_t)(DSPC_REG_BASE));

	reg_base2=(char*)mmap(0,4*1024, 
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, fdx,
			     (off_t)(VOP_REG_BASE));
	
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
// Less overhead than kernel call
u64 hw_get_stc(int id)
{
	void *high_reg, *low_reg;
        u32 high, low;
	if (id==0) {
		high_reg = reg_base+TSI0_STCBASECURRH_OFF;
		low_reg = reg_base+TSI0_STCBASECURRL_OFF;
	}
	else {
		high_reg = reg_base+TSI1_STCBASECURRH_OFF;
		low_reg = reg_base+TSI1_STCBASECURRL_OFF;
	}

        do {
                high = *(u32*)(high_reg) & 0x1;
                low = *(u32*)low_reg;
        } while (high != (*(u32*)(high_reg) & 0x1));

        return (u64)low | ((u64)high << 32);

}
/*-------------------------------------------------------------------------*/

void do_focus_sync(vdec_t *vdec, int64_t pts, int *x_sdtv_sync_retries, int *x_skip_next)
{

	// Sync to Focus field for analog 576i/480i output
	// Bit of a hack, but works most of the time
	// FIXME 60Hz field swap stuff?
	int sdtv_sync_retries, skip_next;
	u64 now;
	s64 z,n2;
	s64 framediv=3600; // default 25Hz

	sdtv_sync_retries=*x_sdtv_sync_retries;
	skip_next=*x_skip_next;

	if ( !IS_SLOMO ( vdec ) &&!IS_FAST ( vdec ) ) {

		if ( vdec->vattr.framerate_div )
			framediv= ( 90000*vdec->vattr.framerate_nom ) /vdec->vattr.framerate_div;

//		printf ( "SYNC %i %i\n", vdec->frameno,sdtv_sync_retries );
		RealtimePrioOn();
		correct_focus_field();
		n2 = hw_get_stc(0);

		if (!correct_focus_field()) {
			now = hw_get_stc(0);
			z=(pts-now );  // magic= approx. measurement delay
#if 1
			printf("SDTV Field Sync diff=%09lli pts=%09lli pts_off=%09lli new=%09lli d1=%i d2=%i\n",
			       z,pts,vdec->pts_off,now,
			       (int)(pts-now),(int)(now-n2));
#endif
			z+=framediv;

			if (z<0 || (int)(now-n2)>framediv+100 || (int)(pts-now)<0 || z>(8*framediv) || (int)(pts-now)>3*3600) {
				// something wrong...
				vdec->frameno=2;
				sdtv_sync_retries++;
				if (z<0)
					vdec->pts_off-=framediv/8;
			}
			else {
				int mo=(int)(z);  // Avoid gcc bug with u64
				mo=mo/ ( framediv/4 );

				if (z<(framediv/2+framediv+100) && z>(framediv/2+framediv-100)) {
					vdec->pts_off-=framediv/8; // Move away from detection border
				}
				if ( ( mo&3 ) <2 ) {
					now+=framediv/2;
					vdec->pts_off+=framediv/2;
					printf ( "ADJUST FIELD %i\n",mo );
				}
				sdtv_sync_retries=0;
			}
		}
		else
			vdec->frameno=0;
		RealtimePrioOff();
	}
	*x_sdtv_sync_retries=sdtv_sync_retries;
	*x_skip_next=skip_next;
	
}
/*-------------------------------------------------------------------------*/
void wait_sync(void)
{
	int n;
	for(n=0;n<1000000;n++) {
		int v=*(volatile int*)(reg_base2+0x80);
		v=(v>>7)&2047;
		if (v<5)
			break;
	}
}
/*-------------------------------------------------------------------------*/
//  200               208              210           218             220
//  05000095          000950e6         0084581b      00110095        00000001
//  27:24 23:12 11:0     23:12 11:0    23:12  11:0    23:12 11:0         11:0
//         C01   C00      C10  C02      C12   C11      C21  C20           C22

int clr_vals[3][3]={
	{0x95, 0,     0xe6},   // R
	{0x95, -0x1b, -0x45},  // G
	{0x95, 0x110, 0x1}     // B
};

/*-------------------------------------------------------------------------*/
void fade(int v1)
{
	int new_vals[3][3];
	int new_regs[3][3];
	int n,m,v2;
	double fak1=v1/256.0;
	double fak2;
	v2=256-(256-v1)*2;
	if (v2<0)
		v2=0;
	fak2=v2/256.0;
	
	for(n=0;n<3;n++) {
		for(m=0;m<3;m++) {
			if (m==0)
				new_vals[n][m]=clr_vals[n][m]*fak1;
			else
				new_vals[n][m]=clr_vals[n][m]*fak2;
			if (new_vals[n][m]>=0)
				new_regs[n][m]=new_vals[n][m];
			else
				new_regs[n][m]=0x800|((-new_vals[n][m])&0x7ff);
		}
	}
	wait_sync();
	*(volatile int*)(reg_base1+0x200)=(5<<24)|new_regs[0][0]|(new_regs[0][1]<<12);
	*(volatile int*)(reg_base1+0x208)=new_regs[0][2]|(new_regs[1][0]<<12);
	*(volatile int*)(reg_base1+0x210)=new_regs[1][1]|(new_regs[1][2]<<12);
	*(volatile int*)(reg_base1+0x218)=new_regs[2][0]|(new_regs[2][1]<<12);
	*(volatile int*)(reg_base1+0x220)=new_regs[2][2];

}
/*-------------------------------------------------------------------------*/
#if 0
int main(int argc, char ** argv)
{
	int v1,n,m;
	int v2;
	init_vcxo();
	int vals[1000];
#if 1
	for(v1=0;v1<=256;v1+=8) {
		v2=256-(256-v1)*2;
		if (v2<0)
			v2=0;

		fade(v1,v2);	
	}
#endif
#if 0
	for(n=0;n<1000;n++) {
		vals[n]=*(volatile int*)(reg_base2+0x80);
		for(m=0;m<1000;m++);
	}
	for(n=0;n<1000;n++)
		printf("%08x  ",(vals[n]>>7)&(2048+4096+8192+16384));
#endif
}
#endif
