/*
 * SAA7114 control for Reelbox PVR1100
 *
 * (c) Georg Acher, acher at baycom dot de
 *     BayCom GmbH, http://www.baycom.de
 *
 * #include <gpl-header.h>
 *
 * $Id: vdr_control.c,v 1.1 2004/09/28 00:19:09 acher Exp $
*/

#include <stdio.h>
#include "libsaa7114.h"

/*-------------------------------------------------------------------------*/
void usage(void)
{
	fprintf(stderr, 
		"reelsaactl  -- SAA7114 video decoder control for Reelbox PVR1100\n"
		"Usage: reelsaactl <Options>\n"
		"Options:          -input [cvbs_scart|cvbs_front|yc_scart|yc_front|tuner]\n"
		"                  -standard [pal|ntsc]    (default pal)\n"
		"                  -standby\n"
		"                  -status                 (exklusive)\n");
	exit(0);
}
/*-------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
	struct saa7114_handle *handle;
	int input=REEL_CVBS_IN_SCART;
	int standard=VIDEO_MODE_PAL;
	int read_status=0,standby=0;
	int l,i;
	char *opt,*para;
	int idx=1;

	while(idx<argc) 
        {
                opt=argv[idx];
                l=0;
                if ((idx+1<argc)) {
                        l=1;
			para=argv[++idx];
			if (!strcmp(opt,"-input")) {
				if (!strcmp(para,"cvbs_scart"))
					input=REEL_CVBS_IN_SCART;
				else if (!strcmp(para,"cvbs_front"))
					input=REEL_CVBS_IN_FRONT;
				else if (!strcmp(para,"yc_front"))
					input=REEL_YC_IN_FRONT;
				else if (!strcmp(para,"yc_scart"))
					input=REEL_YC_IN_SCART;
				else if (!strcmp(para,"tuner"))
					input=REEL_CVBS_IN_TUNER;
				else 
					usage();
			}
			else if (!strcmp(opt,"-standard")) {
				if (!strcmp(para,"pal"))
					standard=VIDEO_MODE_PAL;
				else if (!strcmp(para,"ntsc"))
					standard=VIDEO_MODE_NTSC;
				else
					usage();
			}
			else
				l=0;
		}
		if (l==0) {
			if (!strcmp(opt,"-status")) {
				read_status=1;
			}
			else if (!strcmp(opt,"-standby"))
				standby=1;
			else 
				usage();
		}
		idx++;
	}
	if (read_status) {
		i=0;
		handle=SAA7114_init("/dev/i2c-0", 1); // don't reset
		if (handle==NULL) {
			fprintf(stderr,"SAA7114_init failed\n");
			exit(-1);
		}
		SAA7114_command(handle, DECODER_GET_STATUS, &i);
		printf("%x\n",i); 
		SAA7114_close(handle);
		exit(0);
	}

	handle=SAA7114_init("/dev/i2c-0", 0);

	if (handle==NULL) {
		fprintf(stderr,"SAA7114_init failed\n");
		exit(-1);
	}

	if (standby) {
		SAA7114_write(handle, 0x88, 0x0b);
		exit(-0);
	}

	SAA7114_command(handle, DECODER_SET_INPUT, &input);
	SAA7114_command(handle, DECODER_SET_NORM, &standard);
	
	i=1;
	SAA7114_command(handle, DECODER_ENABLE_OUTPUT, &i);
	
	// Enable data output mode on I-Port with gated clock for Reelbox
	
	SAA7114_write(handle, 0x80,0x14);
	SAA7114_write(handle, 0x85,0x00);
	SAA7114_write(handle, 0x87,0xc1); 
	
	SAA7114_write(handle, 0x90,0x3);
	SAA7114_write(handle, 0x91,0x00);
	SAA7114_write(handle, 0x93,0x80);

	SAA7114_write(handle, 0xc0,0x2);
	SAA7114_write(handle, 0xc1,0x00);
	SAA7114_write(handle, 0xc3,0x80);

	SAA7114_close(handle);
}
