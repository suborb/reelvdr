/*
  avr_flash - Flashing the AVR microcontroller over SPI on Reel NetClient
  (c) 2009 Georg Acher (acher (at) baycom (dot) de) for Reel Multimedia
  
  #include <gpl_v2.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


int fd;

#define IOCTL_REEL_SPI_READ    _IOWR('d', 0x5, int)
#define IOCTL_REEL_SPI_WRITE   _IOWR('d', 0x6, int)
#define IOCTL_REEL_AVR_CTRL    _IOWR('d', 0x7, int)

#define FUSE_LOCK 0
#define FUSE_LOW  1
#define FUSE_HIGH 2
#define FUSE_EXT  3

/*--------------------------------------------------------------------------*/
void do_reset(int reset)
{
	if (reset) {
		printf("Force AVR Reset\n");
		ioctl(fd,IOCTL_REEL_AVR_CTRL,0x12345678);
	}
	else {
		printf("Release AVR Reset\n");
		ioctl(fd,IOCTL_REEL_AVR_CTRL,0);
	}
}
/*--------------------------------------------------------------------------*/
void clear_fifo(void)
{
	ioctl(fd,IOCTL_REEL_SPI_WRITE, -1);
	usleep(10*1000);
}
/*--------------------------------------------------------------------------*/
void send_cmd(char *din, char *dout)
{
	int n,v,m;	
	for(n=0;n<4;n++) 
		ioctl(fd,IOCTL_REEL_SPI_WRITE,din[n]&255);

	for(n=0;n<4;n++) {
		for(m=0;m<100;m++) {
			v=ioctl(fd,IOCTL_REEL_SPI_READ);
			if (v!=-1) {
				dout[n]=v;
				break;
			}		       
			if (m>80)
				usleep(1000);
		}
	}
}
/*--------------------------------------------------------------------------*/
int fp_get_version(void)
{
	char d[8]={0xaa,0x9,0xff, 0x00,  0x00,0x00,0x00,0x00};
	char r[8];
	int n;
	int ret=-1;
	send_cmd(d,r);
	r[4]=r[5]=0xff;
	send_cmd(d+4,r+4);
	ret=((r[4]&255)<<8)|(r[5]&255);
	if (r[4]==0xff && r[5]==0xff)
		ret=0;
	return ret;
}
/*--------------------------------------------------------------------------*/
int programming_enable(void)
{
	char d[4]={0xac,0x53,0x00,0x00};
	char r[4]={0};

	send_cmd(d,r);
//	printf("PEN %x %x %x %x\n",r[0],r[1],r[2],r[3]);
	usleep(10*1000);
	return r[2];
}
/*--------------------------------------------------------------------------*/
int try_sync(void)
{
	int n;
	int val;
	printf("Enable Programming\n");
	clear_fifo();
	for(n=0;n<100;n++) {
		val=programming_enable();
		if (val==0x53) {
			printf("Synced...\n");
			return 0;
		}

	}
	fprintf(stderr,"Sync failed\n");
	return -1;
}
/*--------------------------------------------------------------------------*/
// timeout in ms
// ret 1 if operation pending (or timeout)
int poll_rdy(int timeout)
{
	char d[4]={0xf0,0x00,0x00,0x00};
	char r[4]={0};
	int n;
	for(n=0;n<timeout;n++) {
		send_cmd(d,r);
//		printf("%x\n",r[3]);
		if (!(r[3]&1))
			break;
//		printf("Poll wait %i\n",n);
		usleep(1000);
	}
	return r[3]&1;
}
/*--------------------------------------------------------------------------*/
int read_fuse(int type) 
{
	char d[4][4]={
		{0x58, 0x00, 0x00, 0x00}, // Lock
		{0x50, 0x00, 0x00, 0x00}, // Fuse Low
		{0x58, 0x08, 0x00, 0x00}, // Fuse High
		{0x50, 0x08, 0x00, 0x00}, // Fuse Ext
	};
	char *tstring[4]={"Lock","Low ","High","Ext "};
	char r[4]={0};
	send_cmd(d[type],r);
	printf("Read Fuse %s: %x\n",tstring[type],r[3]);
}
/*--------------------------------------------------------------------------*/
void write_fuse(int type, int val)
{
	char d[4][4]={
		{0xac, 0xe0, 0x00, val}, // Lock
		{0xac, 0xa0, 0x00, val}, // Low
		{0xac, 0xa8, 0x00, val}, // High
		{0xac, 0xa4, 0x00, val}, // Ext
	};
	char *tstring[4]={"Lock","Low ","High","Ext "};
	char r[4]={0};
	printf("Write Fuse %s: %x\n",tstring[type],val);
	send_cmd(d[type],r);
}
/*--------------------------------------------------------------------------*/
void chip_erase(void)
{
	char d[4]={0xac,0x80,0x00,0x00};
	char r[4]={0};
	printf("Chip erase...\n");
	send_cmd(d,r);
}
/*--------------------------------------------------------------------------*/
void program_page(char *data, int addr)
{
	int n;
	char d[4],r[4];
	printf("Flash Addr %x \r",addr);
	fflush(stdout);

	d[0]=0x4d;
	d[1]=0x00;
	d[2]=0;
	d[3]=0;
	send_cmd(d,r); // ext=0

	for(n=0;n<32;n++) {

		d[0]=0x40;
		d[1]=0;
		d[2]=(addr+n)&31;
		d[3]=*data++;		
//		printf("%02x ",d[3]&255);
		send_cmd(d,r); // L
		d[0]=0x48;
		d[3]=*data++;
		send_cmd(d,r); // H
/*		printf("%02x ",d[3]&255);
		if (n==15 || n==31)
			puts("");
*/
	}
	
	d[0]=0x4c;
	d[1]=addr>>8;
	d[2]=addr&255;
	d[3]=0;
	send_cmd(d,r); // Write flash page		
}
/*--------------------------------------------------------------------------*/
void read_flashmem(char *data, int addr)
{
		char r[4];
	if (addr&1) {
		char d[4]={0x28,addr>>9,addr>>1,0};

		send_cmd(d,r); // high
	} else {
		char d[4]={0x20,addr>>9,addr>>1,0};

		send_cmd(d,r); //low
	}

	*data=r[3];
}
/*--------------------------------------------------------------------------*/
int read_signature(void)
{
	char d[4],r[4];
	int n;
	int sig[3];
	d[0]=0x30;
	d[1]=0;
	d[2]=0;
	d[3]=0;

	for(n=0;n<3;n++) {
		d[2]=n;
		send_cmd(d,r);
		sig[n]=r[3];
//		printf("%02x %02x %02x %02x\n",r[0],r[1],r[2],r[3]);
	}

	printf("Chip Signature: %02x %02x %02x\n",sig[0],sig[1],sig[2]);
	if (sig[0]!=0x1e || sig[1]!=0x92 || sig[2]!=0x07) {
		fprintf(stderr,"Signature mismatch\n");
		return -1;
	}
	return 0;
}
/*--------------------------------------------------------------------------*/
void read_mem(void)
{
	int len,n;
	char r;
	len=256;
	for(n=0;n<len;n++) {
		read_flashmem(&r,n);
		printf("%02x ",r&255);
		if ((n&31)==31)
			puts("");
	}
}
/*--------------------------------------------------------------------------*/
int flash_chip(char *fname)
{
	FILE *file;
	char data[8192]; // Enough for tiny44
	int len;
	int n;
	char r;
	int rounded;
	int err;

	file=fopen(fname,"r");
	if (!file) {
		perror("Can't open file");
		return -1;
	}

	len=fread(data,1,8192,file);
	fclose(file);

	rounded=(len+63)&(~63);
	printf("Read length: %i Bytes, rounded: %i\n",len,rounded);

	if (len<200) {
		fprintf(stderr,"Flash file too small\n");
		return -1;
	}

	chip_erase();
	poll_rdy(100);

	write_fuse(FUSE_LOCK,0xff);
	poll_rdy(100);
	write_fuse(FUSE_LOW,0xfd);
	poll_rdy(100);
	write_fuse(FUSE_HIGH,0xd4); // enable SPI, EESAVE, BOD=4.3V
	poll_rdy(100);

	for(n=0;n<rounded;n+=64) {

		program_page(data+n,n/2);
		poll_rdy(100);
	}
	puts("\nVerify...");
	err=0;
#if 1	
	for(n=0;n<len;n++) {
		read_flashmem(&r,n);
		if (r!=data[n]) {
			printf("Verify Error at %x (read %x, should be %x)\n",n,r,data[n]);
			err++;
		}	
	}
	printf("Errors: %i\n",err);
#endif
	return err;
}
/*--------------------------------------------------------------------------*/
void read_fuses(void)
{
	read_fuse(FUSE_LOCK);
	read_fuse(FUSE_LOW);
	read_fuse(FUSE_HIGH);
	read_fuse(FUSE_EXT);
}
/*--------------------------------------------------------------------------*/
void usage(char *x)
{
	fprintf(stderr,"%s: AVR flasher for NetClient\n"
		"Options:         -w <file>     Flash AVR\n"
		"         -c version -w <file> Check version and write if older\n",
		x);
	exit(0);
}
/*--------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
	int exitval=0;
	int n;	
	int c;
	char fname[256]={0};
	int version=0xffff; // Force flash
	int read_version;

	while(1) {
		c=getopt(argc,argv,"c:w:");
		if (c==-1)
			break;
		switch(c)
		{
		case 'w':
			strncpy(fname,optarg,256);
			fname[255]=0;
			break;
		case 'c':
                        version=strtol(optarg,NULL,16);
                        break;
		default:
			usage(argv[0]);
			
		}
	}
	
	fd=open("/dev/reeldrv",O_RDWR);
	if (fd<0) {
		perror("Can't open /dev/reeldrv:");
		exit(-1);
	}
	
	read_version=fp_get_version();

	if (read_version>=version) {
		printf("AVR FW version %04x up-to-date, exiting\n", read_version);
		exit(0);
	}
	printf("AVR FW version %04x needs update\n", read_version);
//	exit(0);
	do_reset(1);
	usleep(20*1000);
	if (try_sync()) {
		exitval=-1;
		goto do_exit_error;
	}

	if (read_signature()) {
		exitval=-1;
		goto do_exit_error;
	}

	read_fuses();

	if (strlen(fname))
		flash_chip(fname);
		
//	read_mem();

	do_reset(0);
	usleep(100*1000);
	read_version=fp_get_version();
	printf("AVR FW version: %04x\n", read_version);
	exit(0);

do_exit_error:
	fprintf(stderr,"Exiting (failed)...\n");
	do_reset(0);
	exit(exitval);
}
