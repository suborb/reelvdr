/*
 * CAM initialization helper for Reelbox
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <reeldvb.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Destinations
#define REEL_DST_DMA0     0
#define REEL_DST_PIDF0    1
#define REEL_DST_DMA1     2
#define REEL_DST_PIDF1    3
#define REEL_DST_DMA2     4
#define REEL_DST_PIDF2    5
#define REEL_DST_DMA3     6
#define REEL_DST_PIDF3    7
#define REEL_DST_CI0      8
#define REEL_DST_X0       9
#define REEL_DST_CI1      10
#define REEL_DST_X1       11
#define REEL_DST_CI2      12
#define REEL_DST_X2       13

#define REEL_DST_DMA(n)   ((n)*2)
#define REEL_DST_PIDF(n)  (1+(n)*2)
#define REEL_DST_CI(n)    (8+(n)*2)
#define REEL_DST_X(n)     (9+(n)*2)

// Source definitions for Matrix

#define REEL_SRC_TS0   0
#define REEL_SRC_PIDF0 1
#define REEL_SRC_TS1   2
#define REEL_SRC_PIDF1 3
#define REEL_SRC_TS2   4
#define REEL_SRC_PIDF2 5
#define REEL_SRC_TS3   6
#define REEL_SRC_PIDF3 7
#define REEL_SRC_CI0   8
#define REEL_SRC_X0    9
#define REEL_SRC_CI1   10
#define REEL_SRC_X1    11
#define REEL_SRC_CI2   12
#define REEL_SRC_X2    13

#define REEL_SRC_TS(n)   ((n)*2)
#define REEL_SRC_PIDF(n) (1+(n)*2)
#define REEL_SRC_CI(n)   (8+(n)*2)
#define REEL_SRC_X(n)    (9+(n)*2)

/*--------------------------------------------------------------------------*/
int getSetup(void)
{
	FILE *fd;
	char name[1024];
	int val;

	fd=fopen("/etc/vdr/setup.conf","r");
	if (!fd)
		return 7; // all enabled
	while(!feof(fd)) {
		val=-1;
		fscanf(fd,"%1023s = %i\n",name,&val);
		if (!strcmp(name,"CAMEnabled") && val>=0 && val<8) {
			fclose(fd);
			return val;
		}
	}
	fclose(fd);
	return 7;
}

/*--------------------------------------------------------------------------*/

int main (int argc, char **argv){
	int fd = 0, result = 0;
	int i,enabled;

	if ((fd = open("/dev/reelfpga0", O_RDWR)) < 0){
		perror("error opening /dev/reelfpga0");
		return 1;
	}
   
	// Remove all Slots
	for(i=0;i<3;i++) {
		result |= ioctl(fd,IOCTL_REEL_CI_SWITCH,(i)|(0<<8));
	}

	if (argc==2) {
		if (!strcmp(argv[1],"-down"))
			return result;	       
		else {
			fprintf(stderr,
				"%s:    Early CI initialisation\n"
				"Options:     <none> : Init CAMs\n"
				"             -down  : Power down CAMs\n", argv[0]);
			return 0;
		}
	}

	usleep(50000);
	enabled=getSetup();

	for(i=0;i<3;i++) {
		if (enabled&(1<<i)) {
			// enable power and DVB-core detection
			result |= ioctl(fd,IOCTL_REEL_CI_SWITCH,(i)|(2<<8));
			sleep(1);
		}
	}
	return result;
}
