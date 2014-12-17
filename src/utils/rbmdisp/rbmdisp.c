/*****************************************************************************
 * 
 * Tool to display PPM-pics on the framebuffer
 *
 * (c) Tobias Bratfisch, tobias (at) reel-multimedia (dot) com
 *
 * #include <GPL-header>
 *
 *****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <string.h>

typedef unsigned long  long u_int64;
typedef unsigned long       u_int32;
typedef long                int32;
#include "../../vdr-plugins/src/rbmini/include/cnxt_rbm.h"

#define RBM_DEVICE_NAME "/dev/rbm0"
#define PATH_TO_CONFIG "/etc/vdr/setup.conf"
#define RBM_TOKEN "rbmini."

void print_usage(void) {
	puts("rbmdisp: Sets HDMI/Scart output mode");
	puts("Usage: rbmdisp <output> [<playback>] [<path to config>]");
	puts("output  : 0 = Off;  1 = On (576p/PAL); 2 = Ignore          ;3 = From config");
	puts("playback: 0 = Stop; 1 = Prepare      ; 2 = Ignore (default)");
	puts("path to config (default): /etc/vdr/setup.conf (only used if output = 3)");
} // print_usage

int read_config(const char *file, rbm_tv_mode *mode) {
	FILE * fp = fopen(file, "r");
	if(fp == NULL) {
		printf("Unable to load config file %s\n", file);
		return -3;
	} // if
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	while ((read = getline(&line, &len, fp)) != -1) {
		char *Name  = line;
		int pos=0;
		while((pos<len) && (line[pos] != ' '))
			pos++;
		if(pos>=len)
			continue;
		line[pos] = 0;
		while((pos<len) && ((line[pos] < '0') || (line[pos] > '9')))
			pos++;
		if(pos>=len)
			continue;
		char *Value = &line[pos];
		if     (!strcasecmp(Name, RBM_TOKEN"HDMIOutput" )) mode->uHDOutputMode    = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"HDMIAspect" )) mode->uHDAspect        = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"HDMIARMode" )) mode->uHDARMode        = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"SCARTOutput")) mode->uSDOutputMode    = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"SCARTAspect")) mode->uSDAspect        = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"SCARTARMode")) mode->uSDARMode        = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"HDMIMode"   )) mode->uHDMIMode        = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"SCARTMode"  )) mode->uSCARTMode       = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"MPEG2Mode"  )) mode->uMPEG2Mode       = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"MPADelay"   )) mode->iMPAOffset       = atoi(Value);
		else if(!strcasecmp(Name, RBM_TOKEN"AC3Delay"   )) mode->iAC3Offset       = atoi(Value);
	} // while
	if (line)
		free(line);
	fclose(fp);
	return 0;
} // read_config

int main(int argc, char** argv){
	if((argc < 2) || (argc > 4)) {
		print_usage(); 
		return -1;
	} // if
	int fd = open(RBM_DEVICE_NAME, O_RDWR|O_NDELAY);
	if(fd == -1) {
		printf("Can't open rbm device %s\n", RBM_DEVICE_NAME);
		return -2;
	} // if
	int ret = 0;
	int display = 0;
	if(argc >= 2)
		display = atoi(argv[1]);
	int playback = 2;
	if(argc >= 3)
		playback = atoi(argv[2]);
		
	rbm_dmx_mode dmx      = RBM_DMX_MODE_NONE;
	rbm_dmx_mode dmx_mode = RBM_DMX_MODE_TS;
	switch(playback == 1) {
		case 0: // Stop playback
			ioctl(fd, RBMIO_STOP, NULL);
			break;
		case 1: // Initialize demux or stop current playback
			if((ioctl(fd, RBMIO_GET_DMX_MODE, &dmx) != 0) || memcmp(&dmx_mode, &dmx, sizeof(dmx))) {
				ioctl(fd, RBMIO_SET_DMX_MODE, &dmx_mode);
			} else {
				ioctl(fd, RBMIO_STOP, NULL);
			} // if
			break;
		case 2: // Ignore
			break;
	} // switch

	// Set output mode
	rbm_tv_mode  tv_mode;
	if(ioctl(fd, RBMIO_GET_TV_MODE, &tv_mode) == 0) {
		switch(display) {
			case 0: // Off
				tv_mode.uHDOutputMode = RBM_OUTPUT_MODE_NONE;
				tv_mode.uSDOutputMode = RBM_OUTPUT_MODE_NONE;
				break;
			case 1: // Default on
				tv_mode.uHDOutputMode = RBM_OUTPUT_MODE_576P;
				tv_mode.uSDOutputMode = RBM_OUTPUT_MODE_PAL_B_WEUR;
				break;
			case 2: // Ignore
				break;
			case 3: // From config
				if(argc >= 4)
					ret = read_config(argv[3], &tv_mode);
				else
					ret = read_config(PATH_TO_CONFIG, &tv_mode);
				break;
		} // switch
		if(!ret) {
			printf("Setting TVMode HDMI: %s %s %s %s SCART: %s %s %s %s MPEG2: %s AUDIO: MPA offset %ldms AC3 offset %ldms\n",
			RBM_HDMIMODE(tv_mode.uHDMIMode), RBM_OUTPUT_MODE(tv_mode.uHDOutputMode), RBM_ASPECT(tv_mode.uHDAspect), RBM_ARMODE(tv_mode.uHDARMode),
			RBM_SCARTMODE(tv_mode.uSCARTMode), RBM_OUTPUT_MODE(tv_mode.uSDOutputMode), RBM_ASPECT(tv_mode.uSDAspect), RBM_ARMODE(tv_mode.uSDARMode),
			RBM_MPEG2_MODE(tv_mode.uMPEG2Mode), tv_mode.iMPAOffset, tv_mode.iAC3Offset);
    			if(ioctl(fd, RBMIO_SET_TV_MODE, &tv_mode) != 0) {
				printf("FAILED!\n");
				ret = 2;
			} // if
		} // if
	} else {
		printf("Failed to get tv mode\n");
		ret = 3;
	} // if
	close(fd);
	return ret;
} // main

