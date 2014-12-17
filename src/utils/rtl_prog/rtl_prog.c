/*
	rtl_prog.c
	Utility to dump, read and write RTL8139-EEPROM on Reelbox
	(c) 2005, Georg Acher

	WARNING: This is a really ugly hack!

	#include <gpl-header.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include  <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>

typedef unsigned int __u32;
typedef unsigned short __u16;
typedef unsigned char __u8;
/*-----------------------------------------------------*/
void EEPROM_cs(int prba,int bit)
{
	int a=inb_p(prba);
	if (bit!=0) bit=1;
	a=a&(~8);
	a=a| (8*bit);
	outb_p(a,prba);
}
/*-----------------------------------------------------*/
void EEPROM_out_bit(int prba,int bit,int cs)
{
	int a,n;
	if (bit!=0) bit=1;
	if (cs!=0) cs=1;
	a=inb_p(prba);
	a= a& (~(8+4+2));
	a= a| (8*cs + 2*bit);
	outb_p(a,prba);
	for(n=0;n<50;n++);
	a|=4;
	outb_p(a,prba);
	for(n=0;n<50;n++);
	a&=~4;
	outb_p(a,prba);
	for(n=0;n<50;n++);
}

/*-----------------------------------------------------*/
void EEPROM_WR_ENABLE(int prba)
{
        EEPROM_cs(prba,1);
        EEPROM_out_bit(prba,1,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,1,1);
        EEPROM_out_bit(prba,1,1);
        
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,0,1);
        EEPROM_cs(prba,0);
}
/*-----------------------------------------------------*/
void EEPROM_WRITE(int prba,int adr, unsigned int value)
{
	int n;
	char x;

        EEPROM_out_bit(prba,1,1);       /* Startbit */
        EEPROM_out_bit(prba,0,1);       /* Write Opcode 01 */
        EEPROM_out_bit(prba,1,1);
        for(n=5;n>=0;n--)
                EEPROM_out_bit(prba,adr&(1<<n),1);
        for(n=15;n>=0;n--)
                EEPROM_out_bit(prba,value&(1<<n),1);
        
        EEPROM_cs(prba,0);      /* deassert CS/D, start write */
         EEPROM_cs(prba,1);     
        usleep(10*1000);        /* 10ms programming time */
}
/*-----------------------------------------------------*/
int EEPROM_READ( int prba,int adr)
{
	int ee;
	int ret,n,m;

        EEPROM_out_bit(prba,0,0);
        EEPROM_out_bit(prba,0,0);
        EEPROM_out_bit(prba,0,1);
        EEPROM_out_bit(prba,1,1);       /* Startbit */
        EEPROM_out_bit(prba,1,1);       
        EEPROM_out_bit(prba,0,1);
        for(n=5;n>=0;n--)
                EEPROM_out_bit(prba,adr&(1<<n),1);
                        
        ret=0;
        for(n=0;n<16;n++)
        {
                ret=ret<<1;
                EEPROM_out_bit(prba,0,1);
                for(m=0;m<10000;m++);
		ee=inb_p(prba);
                if (ee&1) ret|=1;
        }
        EEPROM_cs(prba,0);
        return ret;
}
/*-----------------------------------------------------*/
void dump_hex(int prba)
{
	int n;
	outb(0x80,prba);
	for(n=0;n<32;n++)
        {
                printf("%02x:%02x %04x\n",(n*2)+1,n*2,EEPROM_READ(prba,n));
        }	
}
/*-----------------------------------------------------*/
void dump_file(int prba, char *fname)
{
	short data[32];
	FILE *file;
	int n;
	outb(0x80,prba);
	for(n=0;n<32;n++)
		data[n]=EEPROM_READ(prba,n);

	file=fopen(fname,"w");
	if (!file) 
	{
		perror(fname);
		exit(0);
	}
	fwrite(&data,2,32,file);
	fclose(file);
}
/*-----------------------------------------------------*/
void get_file(char *fname, unsigned short *data)
{
	FILE *file;
	int l;
	file=fopen(fname,"r");
	if (!file) 
	{
		perror(fname);
		exit(0);
	}
	l=fread(data,2,32,file);
	fclose(file);
	if (l!=32) 
	{
		fprintf(stderr,"Data length != 64bytes\n");
		exit(0);
	}	
}
/*-----------------------------------------------------*/
void eeprom_write_data(int prba, unsigned short *data)
{
	int n,m;
	
	outb(0x80,prba);	
	EEPROM_WR_ENABLE(prba);
	for(n=0;n<32;n++)
	{
		printf("%i: %x\n",n/2,data[n]);
		for(m=0;m<10;m++) {
			unsigned int x;
			EEPROM_WRITE(prba,n,data[n]);
			x=EEPROM_READ(prba,n);
			if (x==data[n])
				break;
			if (m==9) {
				printf("Max retries reached\n");
				exit(0);
			}
			printf("Retry %i\n",m);
		}
	}
}

/*-----------------------------------------------------*/
void write_file(int prba, char *fname)
{
	unsigned short data[32];

	get_file(fname,data);
	eeprom_write_data(prba, data);
}

/*-----------------------------------------------------*/
void merge_file(int prba, char *fname)
{
	unsigned short data[32];
	int n;
	
	get_file(fname,data);

 	for(n=0;n<3;n++)
		data[0xe/2+n]=EEPROM_READ(prba,0xe/2+n);

	for(n=0;n<3;n++)
		data[0xe/2+n]=EEPROM_READ(prba,0xe/2+n);
	
	if (data[0xe/2]!=0xe000) {
		data[0xe/2]=0xe000;
		data[0xe/2+1]=0x394c;
		data[0xe/2+2]=0x2+((time(0)&255)<<8);
		printf("Generating MAC\n");
	}

	eeprom_write_data(prba, data);	
}
/*-----------------------------------------------------*/
int check_is_cplus(void)
{
	int fd,pr;
	unsigned char x;
	unsigned char y[4];
	
	fd=open("/proc/bus/pci/00/0b.0",O_RDWR);
	if (fd<0) {
		fprintf(stderr,"pci open failed\n");
		exit(0);
	}
	lseek(fd,0x08, SEEK_SET);
	read(fd,&x,1);
	if (x!=0x20) {
		fprintf(stderr,"No 8139C+, revision 0x%x\n",x);
		exit(0);
	}
	lseek(fd,0x10, SEEK_SET);
	read(fd,&y,4);
	pr=(y[0]&0xf0)|(y[1]<<8)|(y[2]<<16)|(y[3]<<24);
	printf("8139C+ IO-Base address %x\n",pr);
	close(fd);
	return pr;
}
/*-----------------------------------------------------*/
int main(int argc, char ** argv)
{
	int prba=0xee00;
	int n;
	iopl(3);

	while(1)
        {
		int c;
                c=getopt(argc, argv, "dr:w:M:c");
                if(c == -1)
                        break;
		switch(c)
		{
		case 'c':
			prba=check_is_cplus();
			break;
		case 'd':
			dump_hex(prba+0x50);
			exit(0);
		case 'r':
			dump_file(prba+0x50,optarg);
			exit(0);
		case 'w':
			write_file(prba+0x50,optarg);
			exit(0);
		case 'M':
			merge_file(prba+0x50,optarg);
			exit(0);
		default:
			fprintf(stderr,"Usage: rtl_eeprom -d | -r <file> | -w <file> | -M <file> | -c\n"
				"-d dump as hex, -r read to file, -w write file, -M write but keep MAC, -c check for C+\n");
			exit(0);
		}
	}
}
