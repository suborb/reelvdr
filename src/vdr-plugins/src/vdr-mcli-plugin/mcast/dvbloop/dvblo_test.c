/* dvblo_test - A DVB Loopback Device Demo
 * Copyright (C) 2007 Deti Fliegl
 -----------------------------------------
 * File: dvblo_test.c
 * Desc: Demo for virtual DVB adapters
 * Date: March 2007
 * Author: Deti Fliegl <deti@fliegl.de>
 *
 * This file is released under the GPLv2.
 */

/* avoid definition of __module_kernel_version in the resulting object file */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/ca.h>
#include <errno.h>

#include "dvblo_ioctl.h"

void *read_ca(void *arg)
{
	char buf[80];
	int i=0;
	int fd = open ("/dev/dvb/adapter0/ca0", O_RDWR);
	if(fd<0) {
		return NULL;
	}
	ioctl(fd, CA_RESET, 7);
	while(1) {
		ssize_t p = read(fd, buf, sizeof(buf));
		if (p < 0 && errno == EINTR) {
	       		continue;
	       	}
		printf("DVB CA Read: %s\n",buf);
		sprintf(buf,"<TPDU Test %d",i++);
		write(fd, buf, strlen(buf)+1);
	}
}

void *write_ca(void *arg)
{
	int i=0;
	int fd=(int)arg;
	dvblo_tpdu_t tpdu;
	while(1) {
		sprintf(tpdu.data, ">TPDU Test %d",i++);
		tpdu.len=strlen(tpdu.data)+1;
		ioctl(fd, DVBLO_SET_TPDU, &tpdu);
		sleep(1);
	}
}

int main (int argc, char **argv)
{
	struct dvb_frontend_info info;
	struct pollfd fds[1];
	int timeout_msecs = -1;
	int ret;
	int i;
	char *dev = "/dev/dvblo0";
	dvblo_festatus_t fe_status;

	if (argc > 1) {
		dev = argv[1];
	}

	int fd = open (dev, O_RDWR);

	fds[0].fd = fd;

	ret = ioctl (fd, DVBLO_IOCADDDEV);
	printf ("created dvb adapter: %d\n", ret);

	pthread_t p;
	pthread_create(&p, NULL, read_ca, NULL);

	pthread_t r;
	pthread_create(&r, NULL, write_ca, fd);
	
	ioctl (fd, DVBLO_GET_FRONTEND_INFO, &info);
	printf ("original dvb frontend name: %s\n", info.name);

	strcpy (info.name, "wuff");
	ioctl (fd, DVBLO_SET_FRONTEND_INFO, &info);

	ioctl (fd, DVBLO_GET_FRONTEND_INFO, &info);
	printf ("new dvb frontend name: %s\n", info.name);

	fe_status.st = FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
	fe_status.ber = 0;
	fe_status.strength = 100;
	fe_status.snr = 100;
	fe_status.ucblocks = 0;

	ioctl (fd, DVBLO_SET_FRONTEND_STATUS, &fe_status);

	dvblo_private_t test;
	memset (&test, 0, sizeof (test));
	test[0] = 1;
	ioctl (fd, DVBLO_SET_PRIVATE, &test);
	memset (&test, 0, sizeof (test));
	ioctl (fd, DVBLO_GET_PRIVATE, &test);
	if (test[0] != 1) {
		printf ("Private call failed\n");
	}
	dvblo_cacaps_t c;
	c.cap.slot_num=1;
	c.cap.slot_type=CA_CI_LINK;
	c.cap.descr_num=0;
	c.cap.descr_type =0;
	c.info[0].num=0;
	c.info[0].type=CA_CI_LINK;
	c.info[0].flags=0;
	
	ioctl(fd, DVBLO_SET_CA_CAPS, &c);
	
	dvblo_tpdu_t tpdu;
	tpdu.len=64;
	memset(tpdu.data, 0x55, tpdu.len);
	ioctl(fd, DVBLO_SET_TPDU, &tpdu);

	while (1) {
		fds[0].events = POLLIN;
		ret = poll (fds, 1, timeout_msecs);
		if (ret > 0) {
			unsigned int event;
			ioctl (fd, DVBLO_GET_EVENT_MASK, &event);
			if (event & EV_MASK_FE) {
				struct dvb_frontend_parameters fe_parms;

				printf ("Event Received: FE+Tuner/");
				if (event & EV_FRONTEND) {
					printf ("Frontend ");
				}
				if (event & EV_TUNER) {
					printf ("Tuner ");
				}
				if (event & EV_FREQUENCY) {
					printf ("Frequency ");
				}
				if (event & EV_BANDWIDTH) {
					printf ("Bandwidth ");
				}
				printf ("\n");
				ioctl (fd, DVBLO_GET_FRONTEND_PARAMETERS, &fe_parms);
				printf ("frequency: %d\n", fe_parms.frequency);
			}
			if (event & EV_MASK_PID) {
				dvblo_pids_t pids;

				printf ("Event Received: Demux/");
				if (event & EV_PIDFILTER) {
					printf ("PID filter ");
				}
				printf ("\n");
				ioctl (fd, DVBLO_GET_PIDLIST, &pids);
				if (pids.num) {
					printf ("PIDs: ");
					for (i = 0; i < pids.num; i++) {
						printf ("%d ", pids.pid[i]);
					}
					printf ("\n");
				}
			}
			if (event & EV_MASK_SEC) {
				dvblo_sec_t sec;

				printf ("Event Received: SEC/");
				if (event & EV_TONE) {
					printf ("Tone ");
				}
				if (event & EV_VOLTAGE) {
					printf ("Voltage ");
				}
				if (event & EV_DISEC_MSG) {
					printf ("DISEC-Message ");
				}
				if (event & EV_DISEC_BURST) {
					printf ("DISEC-Burst ");
				}
				printf ("\n");
				ioctl (fd, DVBLO_GET_SEC_PARAMETERS, &sec);
				printf ("voltage: %d\n", sec.voltage);
			}
			if (event & EV_MASK_PRIV) {
				printf ("Event Received: PRIV/");
				if (event & EV_PRIV_READ) {
					printf ("READ ");
				}
				if (event & EV_PRIV_WRITE) {
					printf ("WRITE ");
				}
				printf ("\n");
			}
			if (event & EV_MASK_CA) {
				printf ("Event Received: CA/");
				if (event & EV_CA_WRITE) {
					printf ("WRITE ");
					while(1) {
						ioctl(fd, DVBLO_GET_TPDU, &tpdu);
						if(tpdu.len) {
							printf("len:%d data:%s",tpdu.len, tpdu.data);
						} else {
							break;
						}
					} 
				}
				if (event & EV_CA_RESET) {
					printf ("RESET ");
				}
				if (event & EV_CA_PID) {
					printf ("PID ");
				}
				if (event & EV_CA_DESCR) {
					printf ("DESCR ");
				}
				printf ("\n");
			}

		}
	}

	ret = ioctl (fd, DVBLO_IOCDELDEV);
	printf ("delete dvb adapter: %d\n", ret);

	close (fd);
	return 0;
}
