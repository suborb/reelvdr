#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include <hdchannel.h>

int main(int argc, char ** argv)
{
	hd_channel_t *bsc;
	unsigned char buf[131072];
	int ret,n=0;
	int x=0,lx=0;

	hd_init(0);
//	hd_channel_init();

#if CONFIG_MIPS
#warning "MIPS"

	bsc=hd_channel_create(10, 131072, HD_CHANNEL_WRITE);
	sleep(1);
	printf("bsc %p\n",bsc);
	while(1) {
		buf[0]=n++;
		ret=hd_channel_write(bsc, buf, 8*1024, 1000*100);
//		printf("write %i\n",ret);
		usleep(20*1000);

	}
#else
#warning "x86"

//		bsc=hd_channel_open(10);

	do {
		bsc=hd_channel_open(10);
		printf("bsc %p\n",bsc);
	} while(!bsc);

	printf("##############################################\n");
	for(n=0;n<1024*16;){
		char *bf;
		int size;

		ret=hd_channel_read(bsc,buf,16374,1000*100);

		if (ret) {
			x=buf[0];
			printf("xxxx %x %i  %i %i %i\n",bsc->read_data,bsc->read_size,x,lx,ret);
			if (((lx+1)&255)!=x && lx!=0) {
				printf("ERROR\n");
				exit(0);
			}
			lx=x;
			n++;
			if ((n&1023)==0)
				printf("%i\n",n);				
		}
		
	}
#endif
}
