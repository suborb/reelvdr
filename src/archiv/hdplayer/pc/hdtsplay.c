/* Simple TS player for DeCypher */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/dvb/dmx.h>
#include <hdchannel.h>
#include <hdshm_user_structs.h>

#define TS_SIZE 188

void stream(int file, int vpid, int apid)
{
	hd_channel_t *tsch;
	hd_packet_ts_data_t header;
	int len,len1,sum;
	void* buffer;
	int n=0;
	int buffer_size;
	hdshm_area_t *area;
	hd_data_t volatile *hda;
	char *xbuf;
	hd_packet_trickmode_t trick;
	int retries;

	hd_init(0);
	
	area=hd_get_area(HDID_HDA);
	if (!area || !area->mapped) {
		printf("Player area not found\n");
		exit(-1);
	}

	tsch=hd_channel_open(HDCH_STREAM1);

	if (!tsch) {
		printf("TS-channel not found\n");
		exit(-1);
	}
	
	hda=(hd_data_t*)area->mapped;
	len=174*TS_SIZE;
	hd_channel_invalidate(tsch,100);

	hda->player[0].data_generation++;
	sum=0;
	xbuf=malloc(len);

	trick.header.magic = HD_PACKET_MAGIC;
	trick.header.type=HD_PACKET_TRICKMODE;
	trick.trickspeed=0;
	trick.i_frames_only=0;

	buffer_size = hd_channel_write_start(tsch, &buffer,
							    sizeof(trick), 0);
	memcpy(buffer,&trick,sizeof(trick));
	hd_channel_write_finish(tsch, sizeof(trick));

	while(1) {
		header.header.magic = HD_PACKET_MAGIC;
		header.header.seq_nr = n++;
		header.header.type = HD_PACKET_TS_DATA;
//		header.header.packet_size = sizeof(header) + len;
		header.generation=hda->player[0].data_generation;
		header.vpid=vpid;
		header.apid=apid;

		buffer_size=0;
		
		do {
			buffer_size = hd_channel_write_start(tsch, &buffer,
							    sizeof(header)+len, 0);
			if (buffer_size) 
				break;
			usleep(10000);
		} while(1);

		retries=0;
		while(1) {
			len1=read(file,buffer+sizeof(header),len);
			if (len1>0) {
				sum+=len1;
				printf("Write %i %i       \r",len1,sum);
				fflush(stdout);
				header.header.packet_size = sizeof(header) + len1;
			
				memcpy(buffer, &header,sizeof(header));
			
				hd_channel_write_finish(tsch, sizeof(header)+len1);
				break;
			}
			if (len1==0)
				retries++;
			if (retries>100)
				exit(0);
				
			usleep(10000);
		}
	}
}


int main(int argc, char **argv)
{
	int file;
	int apid=0,vpid=0;

	if (argc!=4) {
		fprintf(stderr,"Usage: hdtsplay <file> VPID(hex) APID(hex) \n");
		exit(-1);
	}

	vpid=strtol(argv[2],NULL,16);
	apid=strtol(argv[3],NULL,16);
	
	printf("VPID: 0x%x, APID: 0x%x\n",vpid,apid);
	file=open(argv[1],O_RDONLY| 0*O_NONBLOCK);	
	if (file<0) {
		perror(argv[1]);
		exit(-1);
	}
	ioctl(file, DMX_SET_BUFFER_SIZE, 4*262144); // for DVB

	stream(file,vpid,apid);	
	exit(0);
}

