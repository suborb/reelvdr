/*****************************************************************************/ 
/*
 *     fpga_load.c -- FPGA bitstream downloading (adapted to reelfpga-IO)
 *
 *      Copyright (C) 2000-2004  Georg Acher (acher at baycom dot de)
 *
 * $Id: fpga_load.cpp,v 1.1 2004/09/05 22:16:21 acher Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/ioctl.h>

// FPGA-download
#define IOCTL_REEL_FPGA_CLEAR  _IO('d', 0x1 ) 
#define IOCTL_REEL_FPGA_WRITE  _IOWR('d', 0x2, char[512] )
#define IOCTL_REEL_FPGA_HOTPLUG _IO('d', 0x3 )

/*------------------------------------------------------------------*/
void download_fpga(int num, char *fname)
{
        FILE *file;
        unsigned char buf[262144];
        unsigned int blen, n;
        unsigned char data[128];
        int start;
	int a;
	int fd;
	
	fd=open("/dev/reelfpga0",O_RDWR);
	
	if (fd<0)
	{
		fprintf(stderr,"Can't open /dev/reelfpga0\n");
		exit(-1);
	}

        file=fopen(fname,"rb");
        if (!file)
        {
                perror(fname);
                exit(0);
        }
        fread(buf,1,262144,file);
        
	ioctl(fd,IOCTL_REEL_FPGA_CLEAR);

        usleep(10000);
        for(start=0x40;start<0x60;start++)
        {
                if (buf[start]==0 && buf[start+1]==0x65 && buf[start+2]==0)
                        goto found;
        }
        fprintf(stderr,"Can't find Bitstream start!\n");
        exit(0);
found:
        start+=3;

        blen = buf[start+2] + (buf[start+1] << 8)+(buf[start] << 16);
        
        printf("Bitstream len: %i, start %i\n", blen,start);
        if (blen>181000) // FIXME
	{
		printf("Bitstream too long\n");
                exit(0);
	}

	 printf("Download FPGA %i with %s\n",num, fname);
        for (n = 0; n <= blen + 512; n+=512) 
        {
		ioctl(fd, IOCTL_REEL_FPGA_WRITE, buf+start+3+n);
                if ((n%8192)==0) 
                {                        
                        printf("%i  \r",n);
                        fflush(stdout);
                }
        }
	ioctl(fd,IOCTL_REEL_FPGA_HOTPLUG);
        printf("%i  \n",n);
}
/*------------------------------------------------------------------*/
int main(int argc, char** argv)
{
        char s[20];
        int prog_fpga=0;
        int n=1,l;
        char c;

        while(1)
        {
                c=getopt(argc,argv,"1:?");
                if (c==-1)
                        break;
                switch(c)
                {
                case '1':
                        download_fpga(1,optarg);
                        break;
                default:
                        fprintf(stderr,"Rülps.\n");
			exit(0);
                }
        }
}
