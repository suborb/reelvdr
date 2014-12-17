#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

void main(int argc, char** argv)
{
	off_t base= 0x40000000;
        volatile unsigned char * prba;
        unsigned int* pp;
        int fd,a,n,m;

        if ((fd = open("/dev/mem", O_RDWR)) < 0) {
                printf(" Failed to open /dev/mem (%s)\n",
                           strerror(errno));
                exit(-1);
        }

        prba = (volatile char*)mmap(0, 128*1024*1024,
                                    PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd,
                                    (off_t)base);

	printf("%x\n",*(int*)(prba+0x8400));
	printf("%x\n",*(int*)(prba+0x8404));
	printf("%x\n",*(int*)(prba+0x8408));
	printf("%x\n",*(int*)(prba+0x840c));

	printf("%x\n",*(int*)(prba+0x8414));
	printf("%x\n",*(int*)(prba+0x8418));
	printf("%x\n",*(int*)(prba+0x841c));
}

/* 
pc133-RAM:
00 b68e4704     # 8e= 1000.1110, clk/3
04 18           # 1.5 clock
08 00701520
0c 28632125     # 2 clk CAS Lat, tRC 9, tRAS 7
14 f8
18 180
1c 00

pc100-RAM:
00 b6964704     # 96 = 1001.0110, clk/4
04 20           # 2 clock
08 00701420
0c 27532125     # 2clk CAS Lat, tRC 8, tRAS 6
14 7818 
18 0
1c 0

*/
