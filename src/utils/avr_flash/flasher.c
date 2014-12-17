/*------------------------------------------------------------------------*/
// flasher.c
// AVR in-system flasher
//
// (c) 2004-2005 Georg Acher, BayCom GmbH, http://www.baycom.de
//
// $Id: flasher.c,v 1.6 2005/05/21 19:06:09 acher Exp $
//
/*------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define STD_FLG (CS8|CREAD|HUPCL)

int fd;
/*-------------------------------------------------------------------------*/
void open_serial(void)
{
        int bdflag=B115200;//B57600;
        struct termios tty;

        fd=open(FP_DEVICE, O_RDWR|O_NONBLOCK|O_NOCTTY);

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
}
/*-------------------------------------------------------------------------*/
void wait_for(char *x, int n)
{
	int l;
	char buf[10];

	while(1) {
		l=read(fd,buf,1);
		if (l==1 && buf[0]==*x) {
			x++;
			n--;
			if (n==0)
				return;
		}
		else
			usleep(10000);
	}
}
/*-------------------------------------------------------------------------*/
void read_n(char *x, int n)
{
	int l,m=0;
	char buf[10];

	while(m<400) {
		if (n==0)
			return;

		l=read(fd,buf,1);
		if (l==1) {
			*x=buf[0];
			n--;
			x++;
		}
		else
			usleep(10000);
		m++;
	}
}
/*-------------------------------------------------------------------------*/
void dump_hex(unsigned char *c, int l)
{
        int n;

        for(n=0;n<l;n++) {
                if ((n&15)==0)
                        printf( "%04x ",n);
                printf("%02x ",*c++);
                if (n%16==15)
                           printf("\n");
        }
        printf("\n");
}
/*-------------------------------------------------------------------------*/
void fp_get_version(void)
{
        char buf[]={0xa5,0x00,0x00};
        write(fd,buf,3);
}
/*-------------------------------------------------------------------------*/
size_t fp_read(unsigned char *b, size_t l)
{
        return read(fd,b,l);
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
                        usleep(1000);
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
// Return 1 version older than file, 2 when safe bootloader active
int check_version(unsigned int compare)
{
	unsigned char buf[32]={0};
	unsigned int v;
	int l;
	printf("Get firmware version...\n");
	fp_get_version();
	l=fp_read_msg(buf,1000);
	if (l==4) {
		if (buf[0]==0x5a && buf[1]==0) {
			v=(buf[2]<<8)|buf[3];
			printf("Frontpanel version %04x\n",v);
			if (v<compare)
				return 1;
		}
		return 0;
	}
	printf("Frontpanel seems to be in safe bootloader mode\n");
	return 2;
}
/*-------------------------------------------------------------------------*/
void enter_flash(void)
{
	char enter[]={0xa5,0x0d,0x19,0x71};
	unsigned char buf[2]={0};
	printf("Enter Flashmode...\n");
	write(fd,enter,sizeof(enter));
	usleep(200000); // Wait before next cmds
	read_n(buf,2);
	usleep(200000); // Wait before next cmds
	if (buf[0]==0x5a && buf[1]==0xfd)
		printf("Accepted (1)\n");
	else if (buf[0]==0xff && buf[1]==0xff)
		printf("Accepted (2)\n");
	else {
		printf("Unknown response %x %x, is FP really connected?\n",buf[0],buf[1]);
		exit(0);
	}
}
/*-------------------------------------------------------------------------*/
void write_page(int addr, char* d)
{
	char write_page[69]={0x3,0x42,addr>>8,addr&255};
	char cmd_ok[]={0x00};
	int n,m;
	m=0;
	for(n=0;n<64;n++) {
		write_page[4+n]=d[n];
		m+=d[n];
	}
	write_page[68]=(m&255);
	write(fd, write_page, sizeof(write_page));
	wait_for(cmd_ok,1);
}
/*-------------------------------------------------------------------------*/
void erase_page(int addr)
{
	char erase[]={2,0x42,addr>>8,addr&255};
	char cmd_ok[]={0x00};
	write(fd, erase, sizeof(erase));
	wait_for(cmd_ok,1);
}
/*-------------------------------------------------------------------------*/
void read_page(int addr, char* d)
{
	char read_cmd[]={0x04,0x42,addr>>8,addr&255};
	char cmd_ok[]={0x00};
	write(fd, read_cmd, sizeof(read_cmd));
	wait_for(cmd_ok,1);
	read_n(d,64);
}
/*-------------------------------------------------------------------------*/
void exit_flash(void)
{
	char exit_cmd[]={0x01,0x42};
	char cmd_ok[]={0x00};
	printf("Exit Flashmode\n");
	write(fd, exit_cmd, sizeof(exit_cmd));
	wait_for(cmd_ok,1);
}
/*-------------------------------------------------------------------------*/
char flash_data[32768]={0};
int flash_len;

void load_flash_data(char *filename)
{
	FILE *fd;
	printf("Open data file <%s>\n",filename);
	fd=fopen(filename,"rb");
	if (!fd) {
		perror("Loading failed.");
		exit(-1);
	}
	flash_len=fread(flash_data,1,32768,fd);
	printf("Read Flash data length %i\n",flash_len);
	fclose(fd);
}
/*-------------------------------------------------------------------------*/
void do_flash(char* data, int len, int no_verify)
{
	int n;
	char vbuf[64]={0};
	int retry;
	if (len==0) {
		printf("Empty file, exiting\n");
		exit(0);
	}

	if (len>0x1e00) {
		len=0x1e00-64;
		printf("Omitting bootloader, stripped to %i\n",len+64);
	}
	if (no_verify)
		printf("VERIFY SWITCHED OFF!!!\n");

	for(n=0; n<1+(len/64);n++)
	{
		printf("Program at %x   \r",n*64);
		fflush(stdout);
		retry=0;

	do_retry:
		erase_page(n*64);
		write_page(n*64,data+n*64);
		write_page(n*64,data+n*64);
		read_page(n*64,vbuf);
		if (memcmp(vbuf,data+n*64,64)!=0 && no_verify==0) {
			if (retry!=20) {
				printf("\nVerify error, retry #%i\n",retry);
				retry++;
				goto do_retry;
			}
			else {
				printf("Too many retries, aborting\n");
				printf("Data should be:\n");
				dump_hex(data+n*64,64);
				printf("Data is:\n");
				dump_hex(vbuf,64);
				exit(-1);
			}
		}
	}
}
/*-------------------------------------------------------------------------*/
void avr_init(unsigned int version)
{
	char dummy_cmd[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	char dummy_cmd1[]={0xaa,0xaa};
	int c,n,l;
	char buf[64]={0};

	open_serial();
	if (version) {
		write(fd,dummy_cmd,16);
		c=check_version(version);
		if (!c) {
			printf("Frontpanel version OK, no programming done.\n");
			exit(1);
		}
	}
	write(fd,dummy_cmd,16);
	write(fd,dummy_cmd,16);
	write(fd,dummy_cmd,16);
	read(fd,buf,64);

	enter_flash();

	printf("Sync\n");
	l=0;
	n=0;
	while(n<1000) {
		write(fd,dummy_cmd1,1);
		usleep(10000);
		l=read(fd,buf,64);
		if (l==1) break;
		n++;
	}
	if (l!=1) {
		printf("Can't sync with Frontpanel, powercycle and try without -c option\n");
		exit(2);
	}
	printf("Synced\n");
}
/*-------------------------------------------------------------------------*/
void write_flash(char* fname, unsigned int version, int no_verify)
{
	avr_init(version);

	load_flash_data(fname);
	do_flash(flash_data,flash_len,no_verify);

	exit_flash();
}

/*-------------------------------------------------------------------------*/
#ifndef NOFWREAD
void read_flash(char* fname)
{

	FILE *fd;
	int n;
	char vbuf[64]={0};

	fd=fopen(fname,"wb");
	if (!fd) {
		perror("Can't create file");
		exit(-1);
	}

	avr_init(0);

	for(n=0;n<8192/64;n++) {
		printf("Read %x   \r",n*64);
		fflush(stdout);
		read_page(n*64,vbuf);
		fwrite(vbuf,1,64,fd);
	}
	fclose(fd);
	exit_flash();
}
#endif
/*-------------------------------------------------------------------------*/
void usage(char* name)
{
	fprintf(stderr,"Usage: %s [-v ] -w <filename>       : Program flash from file (-v: ommit verify)\n",name);
#ifndef NOFWREAD
	fprintf(stderr,"       %s -r <filename>             : Read out flash to file\n",name);
#endif
	fprintf(stderr,"       %s -c version -w <filename>  : Check version and write if older\n",name);
	exit(0);
}
/*-------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
	char dummy_cmd[]={0xaa,0xaa};
	char nack[]={0xff};
	char buf[64]={0};
	char data[64];
	int n;
	int c;
	unsigned int version=0;
	unsigned no_verify=0;

	while(1)
        {
#ifndef NOFWREAD
		c=getopt(argc, argv, "vc:r:w:");
#else
		c=getopt(argc, argv, "c:w:");
#endif
		if (c == -1)
			usage(argv[0]);
		switch(c)
                {
                case 'v':
                	no_verify=1;
                	break;
		case 'c':
			version=strtol(optarg,NULL,16);
			break;
#ifndef NOFWREAD
		case 'r':
			read_flash(optarg);
			exit(0);
#endif
		case 'w':
			write_flash(optarg,version,no_verify);
			exit(0);
		default:
			usage(argv[0]);
		}
	}

}
