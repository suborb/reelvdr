/*
 * VDR control for Reelbox
 * V2: LIRC
 *
 * (c) Georg Acher, BayCom GmbH, http://www.baycom.de
 *
 * #include <gpl-header.h>
 *
*/
#include <stdlib.h>
#include <linux/cdrom.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <pthread.h>

#include "frontpanel.h"

#define KEY_REPEAT 1
#define KEY_REPEAT_FAST 2
#define KEY_SHIFT 4
#define SHIFT_THRESHOLD 8
#define REPEAT_DELAY 250000
#define REPEAT_TIMEOUT (300*1000)  // 300ms, IR repeat is about 120ms

// beware of VDR`s #define KEYPRESSDELAY 150 (lirc.c) //

//#define DEBUG_KEYS

typedef struct
{
	int code;
	char *fn;
	int flags;
} kbdx_t;

typedef struct {
	int cmd;
	int device;
	unsigned long long timestamp;
	char name[32];
	//int flags;
} ir_cmd_t;


kbdx_t ir_kbdx[]={
	{REEL_IR_NONE,"None",0},
	{REEL_IR_0,"0"},
	{REEL_IR_1,"1"},
	{REEL_IR_2,"2"},
	{REEL_IR_3,"3"},
	{REEL_IR_4,"4"},
	{REEL_IR_5,"5"},
	{REEL_IR_6,"6"},
	{REEL_IR_7,"7"},
	{REEL_IR_8,"8"},
	{REEL_IR_9,"9"},
	{REEL_IR_MENU,"Menu"},
	{REEL_IR_UP,"Up",KEY_REPEAT},
	{REEL_IR_DOWN,"Down",KEY_REPEAT},
	{REEL_IR_LEFT,"Left",KEY_REPEAT},
	{REEL_IR_RIGHT,"Right",KEY_REPEAT},
	{REEL_IR_OK,"Ok"},
	{REEL_IR_EXIT,"Back"},
	{REEL_IR_RED,"Red",KEY_REPEAT},
	{REEL_IR_GREEN,"Green",KEY_REPEAT},
	{REEL_IR_YELLOW,"Yellow",KEY_REPEAT},
	{REEL_IR_BLUE,"Blue",KEY_REPEAT},
	{REEL_IR_SETUP,"Setup"},
	{REEL_IR_PROGPLUS,"Channel+",KEY_REPEAT},
	{REEL_IR_PROGMINUS,"Channel-",KEY_REPEAT},
	{REEL_IR_MUTE,"Mute"},
	{REEL_IR_VOLPLUS,"Volume+",KEY_REPEAT},
	{REEL_IR_VOLMINUS,"Volume-",KEY_REPEAT},
	{REEL_IR_STANDBY,"Power",0},
	{REEL_IR_HELP,"Info",KEY_SHIFT}, //shift: Help
	{REEL_IR_TEXT,"TT",KEY_SHIFT},	//shift: User5
	{REEL_IR_PIP,"PiP",KEY_SHIFT},	//shift: User6
	{REEL_IR_GT,"Greater",KEY_SHIFT}, //shift: 2digit
	{REEL_IR_LT,"Audio",KEY_SHIFT}, //shift: Aspect
	//{REEL_IR_AV,"Audio",0},  shift Less
	{REEL_IR_DVB,"DVB",KEY_SHIFT},     //shift: User1
	{REEL_IR_DVD,"DVD",KEY_SHIFT},     //shift: User2
	{REEL_IR_PVR,"PVR",KEY_SHIFT},     //shift: User3
	{REEL_IR_REEL,"Reel",KEY_SHIFT},   //shift: User4
#if 1
	{REEL_IR_PLAY,"Play",0},
	{REEL_IR_PAUSE,"Pause",0},
	{REEL_IR_STOP,"Stop",KEY_SHIFT}, //shift: Eject
	{REEL_IR_REC,"Record",0},
	{REEL_IR_REV,"FastRew",KEY_SHIFT}, //shift: Prev
	{REEL_IR_FWD,"FastFwd",KEY_SHIFT}, //shift: Next
	{REEL_IR_TIMER,"Timers",KEY_SHIFT}, //shift: Search
#endif

	{0xffff,""}
};

// Frontpanel Keys
kbdx_t kb_kbdx[]={
	{REEL_KBD_BT0,"Power"},
	{REEL_KBD_BT1,"Ok"},
	{REEL_KBD_BT2,"Up",KEY_REPEAT},
	{REEL_KBD_BT3,"Down",KEY_REPEAT},
	{REEL_KBD_BT4,"Back"},

	{REEL_KBD_BT5,"Eject"},
	// BT6 is the "Play" key but sends "DVD" to start Disc player
	{REEL_KBD_BT6,"DVD"},

	{REEL_KBD_BT7,"Pause"},
	{REEL_KBD_BT8,"Stop"},
	{REEL_KBD_BT9,"FastRwd",KEY_REPEAT_FAST},
	{REEL_KBD_BT10,"FastFwd",KEY_REPEAT_FAST},
	{REEL_KBD_BT11,"Record"},
	{0xffff,""}
};

#define QUEUELEN 256
ir_cmd_t code_queue[QUEUELEN];
volatile int code_w;
extern int rs232_fd;
volatile int vdr_is_up = 0;
/*-------------------------------------------------------------------------*/
void put_code(ir_cmd_t cmd)
{
	code_queue[code_w]=cmd;
	code_w=(code_w+1)%QUEUELEN;
}
/*-------------------------------------------------------------------------*/
int get_code(int* rp, ir_cmd_t *cmd)
{
	*rp=*rp%QUEUELEN;
	if (code_w!=*rp)
	{
		*cmd=code_queue[(*rp)++];
		return 0;
	}
	return -1;
}
/*-------------------------------------------------------------------------*/
void setnolinger(int sock)
{
	static struct linger  linger = {0, 0};
	int l  = sizeof(struct linger);
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&linger, l);
}

/*-------------------------------------------------------------------------*/
int lirc_open(void)
{
	int f;
	struct sockaddr_un addr;

	unlink(LIRC_DEVICE);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, LIRC_DEVICE);
	if ((f = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0) {
		if(bind(f,(struct sockaddr *)&addr,sizeof(addr))) {
			perror("bind error");
			close(f);
			return -1;
		}

		setnolinger(f);
//		flags=fcntl(fd,F_GETFL,0);

	}
	return f;
}
/*-------------------------------------------------------------------------*/
int write_socket(int fd, char *buf, int l)
{
        int written,todo=l;

        while(todo)
        {
                written=write(fd,buf,todo);
                if(written<=0)
			return(written);
                buf+=written;
                todo-=written;
        }
        return(l);
}
/*-------------------------------------------------------------------------*/
unsigned long long get_timestamp(void)
{
        struct  timeval ti;
        struct timezone tzp;
        gettimeofday(&ti,&tzp);
        return ti.tv_usec+1000000LL*ti.tv_sec;
}
/*-------------------------------------------------------------------------*/
char * find_ir_key(int ir_code,int *flags)
{
	int n=0;

	while(ir_kbdx[n].code!=0xffff) {
		if (ir_code==ir_kbdx[n].code) {
			if (flags)
				*flags=ir_kbdx[n].flags;
			return ir_kbdx[n].fn;
		}
		n++;
	}
	return "";
}
/*-------------------------------------------------------------------------*/
char * find_kbd_key(int ir_code,int *flags)
{
	int n=0;

	while(kb_kbdx[n].code!=0xffff) {
		if (ir_code==kb_kbdx[n].code) {
			if (flags)
				*flags=kb_kbdx[n].flags;
		    return kb_kbdx[n].fn;
		}
		n++;
	}
	return "";
}
/*-------------------------------------------------------------------------*/
char *find_shift_name(ir_cmd_t cmd)
{
	if(strcmp(cmd.name,"Audio")==0)
		return "Aspect";
	if(strcmp(cmd.name,"Stop")==0)
		return "Eject";
	if(strcmp(cmd.name,"Info")==0)
		return "Help";
	if(strcmp(cmd.name,"Greater")==0)
		return "2digit";
	if(strcmp(cmd.name,"FastRew")==0)
		return "Prev";
	if(strcmp(cmd.name,"FastFwd")==0)
		return "Next";
	if(strcmp(cmd.name,"Timers")==0)
		return "Search";
	if(strcmp(cmd.name,"DVB")==0)
		return "User1";
	if(strcmp(cmd.name,"DVD")==0)
		return "User2";
	if(strcmp(cmd.name,"PVR")==0)
		return "User3";
	if(strcmp(cmd.name,"Reel")==0)
		return "User4";
	if(strcmp(cmd.name,"TT")==0)
		return "User5";
	if(strcmp(cmd.name,"PiP")==0)
		return "User6";

	return "None";
}
/*-------------------------------------------------------------------------*/
int send_command(int sock, char *string)
{
	int l;
	fp_set_leds(0x10,2);
	l=write_socket(sock,string,strlen(string));
	u_sleep(20000);
	fp_set_leds(0x20,2);
	return l;
}
/*-------------------------------------------------------------------------*/
void cdeject(void)
{
	system("/sbin/mount.sh eject");
}
/*-------------------------------------------------------------------------*/

void* ir_send_thread(void* para)
{
	int sock=(int)para;
	ir_cmd_t cmd;
	int rp=code_w;
	char string[256];
	int died=0,l;
	int rep_count=0;
	unsigned long long last_timestamp=0;
	struct pollfd pfd[]=  {{sock, POLLERR|POLLHUP,0},
				{rs232_fd, POLLIN|POLLERR|POLLHUP,0}};

	puts("send thread started");
	vdr_is_up = 1;

	while(!died) {
		if (get_code(&rp,&cmd)==-1) {
			u_sleep(10*1000);
			poll(pfd,2,50);

			if (pfd[0].revents&(POLLERR|POLLHUP))
				break;
			// Special treatment for STOP/EJECT
			if (!pfd[1].revents&POLLIN && rep_count<15 && ((cmd.cmd&0xffff)==REEL_IR_STOP))   {
				sprintf(string,"%x %i LIRC.%s reel",cmd.cmd,rep_count,"Stop");
				l=send_command(sock,string);

				if (l<0)
					died=1;
				cmd.cmd=0;
			}
			continue;
		}

		// Reset repeat count
		if ((cmd.device!=1 && !(cmd.cmd&0x00800000)) || (cmd.device==1) || cmd.timestamp-last_timestamp>REPEAT_TIMEOUT)
			rep_count=0;

		// XXX dirty hack XX
		if (cmd.device == 1 || strcmp(cmd.name,"Red") == 0)
			cmd.cmd = 0x800000;

#ifdef DEBUG_KEYS
		printf("cmd: %x repeat: %i LIRC.%s reel\n",cmd.cmd,rep_count,cmd.name);
#endif
		sprintf(string,"%x %i LIRC.%s reel ",cmd.cmd,rep_count,cmd.name); // workaround for 2 digits (_blank after reel)

		l=send_command(sock,string);
		if (l<0)
			died=1;

		if (rep_count == 0)
			usleep(REPEAT_DELAY);
		rep_count++;
		last_timestamp=cmd.timestamp;
	}

	close(sock);
	vdr_is_up = 0;
	puts("send thread exit");
	return 0;
}
/*-------------------------------------------------------------------------*/
void* server_thread(void* p)
{
	int sock,newsock;
	socklen_t addrlen;
	struct sockaddr_un peer_addr;
	pthread_t clientthread;

	do {
		u_sleep(100*1000);
		sock=lirc_open();
	}  while(sock==-1);

	printf("Server Thread running!\n");

	while(1) {
		listen(sock,5);
		newsock=accept(sock,(struct sockaddr *)&peer_addr,&addrlen);
		setnolinger(newsock);
		if (!pthread_create(&clientthread, NULL,ir_send_thread,(void*)newsock)){
				fprintf(stderr,"pthread_detach!\n");
			pthread_detach(clientthread);
	}
	}
	return 0;
}
/*-------------------------------------------------------------------------*/
//int fp_scan ( ir_cmd_t *cmd, unsigned long long now)
int fp_scan(ir_cmd_t *cmd)
{
	static int last_none=0;
	static int vendor_reel=0;
	static int device_id=-1;
	static unsigned long long last_device=0;
	unsigned char buf[16];
	int l;
	unsigned int xcmd;

	buf[0]=buf[1]=buf[2]=buf[3]=0;

	l=fp_read_msg(buf,50);

	//key_none special
	if(l==0)  {
		buf[1]=0xf2;
		buf[0]=0x5a;
		last_none=1;
	}

	switch (buf[1]) {
	case 0xf1:
	case 0xf2:

		xcmd=(buf[2]<<24)| (buf[3]<<16)| (buf[4]<<8)|(buf[5]);

		if ((xcmd&0xffff)==REEL_IR_VENDOR) {
			vendor_reel=1;
			device_id=xcmd>>16;
			last_device=1; //???
		}
		else if (vendor_reel || (buf[1]==0xf1) || l==0) {

			if (buf[1]==0xf1)
				cmd->device=1; // Kbd
			else {
				cmd->device=device_id; // IR
			}

			if (l>0) {
				//printf("xcmd: %x \n",xcmd);
				cmd->cmd=xcmd;
				last_none=0;
			}
			else {
				cmd->cmd=REEL_IR_NONE;
			   vendor_reel=1;
			}

			device_id=0;
			cmd->timestamp=get_timestamp();
			last_device=0;
			vendor_reel=0;

			return 1;
		}
		break;
	}

	return 0;
}
/*-------------------------------------------------------------------------*/
void signal_nop(int sig)
{
}
/*-------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	int n;
	ir_cmd_t cmd,last_cmd;
	int flags=0;
	int last_flags=0;
	int ir_avail=0;
	pthread_t servermain;
	int power_state=1;
	int send=0;
	int rep_count=0;


	while(fp_open_serial())
		sleep(1);

	// Sync (if previous commands weren't finished)

	for(n=0;n<40;n++)
		fp_noop();

//	fp_get_version();

	signal(SIGPIPE,signal_nop);

	n=0;
	fp_enable_messages(0xff);

	last_cmd.cmd=0;
	unlink("/tmp/vdr.standby");

	pthread_create(&servermain, NULL, server_thread , 0);

	while(1) {
		char *found_key="";
		//char *shift_key="";

		//ir_avail=fp_scan(&cmd,now);
		ir_avail=fp_scan(&cmd);
		if (ir_avail) {

			if (cmd.device==1) {
				found_key=find_kbd_key(cmd.cmd>>16,&flags);
			}
			else {
				found_key=find_ir_key(cmd.cmd&0xffff,&flags);
			}
			if (strlen(found_key)) {
				strcpy(cmd.name,found_key);


				// just key none, keep on going
				if(cmd.cmd==REEL_IR_NONE && last_flags!=KEY_SHIFT){
					send = last_flags= rep_count=0;
					continue;
				}
				//all normal keys (not none !& shift) send them immediately
				if(cmd.cmd!=REEL_IR_NONE && flags!=KEY_SHIFT) {
					put_code(cmd);
				}
				// hey, we catched a Shift key;) send them if rep_count > SHIFT_THRESHOLD
				else if(cmd.cmd!=REEL_IR_NONE && flags==KEY_SHIFT ) {
					if(rep_count>=SHIFT_THRESHOLD) {
						strcpy(cmd.name,find_shift_name(cmd));
						if(!send)
							cmd.cmd&=0xff0fffff;
						put_code(cmd);
						last_flags=0;
						send=1;
					}
					// hey, we catched a Shift key;) we have to wait whats going on
					else {
						last_cmd=cmd;
						last_flags=flags;
						rep_count++;
					}
				}
				// we send Shift_Key with first Value if Key_None
				if(cmd.cmd==REEL_IR_NONE && last_flags==KEY_SHIFT) {
					if(!send)
						last_cmd.cmd&=0xff0fffff;
					put_code(last_cmd);
					last_flags=0;
					rep_count=0;
					send=1;
				}

				//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

				if (vdr_is_up == 0) {
					if (strcmp(cmd.name,"Eject")==0 && (cmd.device ==1 || !(cmd.cmd&0x00800000))) {
						cdeject();
					}

					else if (!strcmp(cmd.name,"Back")) {
						system("touch /tmp/vdr.deepstandby");
						unlink("/tmp/vdr.standby");
					}
				}

				if (!strcmp(cmd.name,"Power")) {
					fp_set_leds(0x10,2);
					if (!unlink("/tmp/vdr.standby"))
						power_state=1; // unlink successfull -> was in standby state
					else
						power_state=0; // unlink unsuccessfull -> going into standby
					u_sleep(20000);
					fp_set_leds(0x20,2);
				}
			}
		}
	}

}
