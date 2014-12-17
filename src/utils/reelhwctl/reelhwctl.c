/************************************************************************
 * reelhwctl.c
 * Control of video-options for Reelbox PVR1100
 * (c) 2005, Georg Acher, acher (at) baycom dot de
 *           BayCom GmbH, http://www.baycom.de
 *
 * $Id:$
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
Demux-Ports:
0: SCART_VCR_RGB_BLANK
1: RGB_COMP_RCA_BLANK
2: VGA_BLANK
3: SCART_TV_RGB_BLANK
4: SCREENM0
5: SCREENS1  (0: CVBS, 1: RGB) -> TV
6: SCREENM1
7: SCREENM2
8: SCREEN2   (0: CVBS, 1: RGB) -> VCR
9: SCREENM3
10: SPDIF3SEL#
11: SPDIF2SEL#
12: SPDIF1SEL#
13: AUD_MUTE
14: SCART_VCR_CTRL_TRI
15: LOOP_THROUGH?

*/

// FIXME: Get from reeldvb-include
#define IOCTL_REEL_SET_DEMUX _IOWR('d', 0x4, int)  


#define STATE_FILE "/tmp/demux_state"

#define F1BAR1 0x9c00
/*-----------------------------------------------------------------------*/
void usage(void)
{
        fprintf(stderr,
                "Usage:\n"
                "reelhwctl <options>\n"
                "Options:   -reset\n"
                "           -video-mute [0...15|q]       (q=query)\n"
                "           -audio-mute [0|1|q]\n"
                "           -scart-enable [0|1|q]\n"
                "           -scart-tv  [{iwo}{cr}|q]     (In/Wide-out/Out, CVBS/RBG)\n"
                "           -scart-vcr [t|{iwo}{cr}|q]   (Tristate, In/Wide-out/Out, CVBS/RBG)\n"
                "           -spdif-in  [bsp|coax|opt|q]\n"
		"           -line-in   [front|vcr|tuner|q]\n"
                "           -scart-vcr-query\n"                
                );
        exit(0);
}
/*-----------------------------------------------------------------------*/
unsigned int demux_read(void)
{
        int fd;
        unsigned char buf[2]={0,0};
#if 1        
        if (getuid()!=0) {
                fprintf(stderr,"Need root privileges!\n");
                exit(-1);
        }
        iopl(3);
#endif        
        fd=open(STATE_FILE,O_RDONLY);
        if (fd<0) {
                fd=open(STATE_FILE,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
                if (fd<0) {
                        perror("Failed to create statefile");
                        exit(-1);
                }
                write(fd,buf,2);
                close(fd);
                return 0;
        }
        read(fd,buf,2);
        close(fd);
        return ((buf[0]&255)<<8)|buf[1];     
}
/*-----------------------------------------------------------------------*/
void demux_write(unsigned int val)
{
        int fd,fd1;
        char buf[2];
//        printf("WRITE %x\n",val);
	fd1=open("/dev/reelfpga0",O_RDWR);
	if (fd1<0) {
                perror("Failed to open /dev/reelfpga");
                exit(-1);
        }
	ioctl(fd1,IOCTL_REEL_SET_DEMUX,val);
	close(fd1);

        fd=open(STATE_FILE,O_WRONLY);
        if (fd<0) {
                perror("Failed to create statefile");
                exit(-1);
        }
        buf[0]=(val>>8);
        buf[1]=val&255;
        write(fd,buf,2);
        close(fd);
}
/*-----------------------------------------------------------------------*/
void reset(void)
{
	int a;
        demux_read();	
        demux_write(0);      
}
/*-----------------------------------------------------------------------*/
// 8 SCART
void video_mute(char *arg)
{
        unsigned int val,old;
        old=demux_read();
        if (!strcmp(arg,"q")) {
                printf("%i\n",old&15);
                return;
        }
        val=atoi(arg)&15;
        demux_write((old&~15)|val);
}
/*-----------------------------------------------------------------------*/
void audio_mute(char *arg)
{
        unsigned int val,old;
        old=demux_read();
        if (!strcmp(arg,"q")) {
                printf("%i\n",(old>>13)&1);
                return;
        }
        val=atoi(arg)&1;
        demux_write((old&~0x2000)|(val<<13));
}
/*-----------------------------------------------------------------------*/
void scart_enable(char *arg)
{
        unsigned int val=0,old;
        old=demux_read();
        if (!strcmp(arg,"q")) {
                printf("%i\n",((old>>15)&1));
                return;
        }
        val=(atoi(arg)&1);
        demux_write((old&~0x8000)|(val<<15));
}
/*-----------------------------------------------------------------------*/
char *smodes[]={"or","ir","oc","ic","wr","--","wc","--"};
//   0+2         
//   1            R    R    C    C    R    R   C    C
// Bit 0+2: Mode
// Bit 1: CVBS/RGB

int get_mode(char *arg)
{
        int n;
        for(n=7;n>=0;n--)
                if (!strcmp(arg,smodes[n]))
                        return n;
        usage();
        return 0;
}
/*-----------------------------------------------------------------------*/
void scart_tv(char *arg)
{
        unsigned int val=0,old;      
        old=demux_read();
        if (!strcmp(arg,"q")) {
                puts(smodes[(old>>4)&7]);
                return;
        }
        val=get_mode(arg)&7;
        demux_write((old&(~0x70))|(val<<4));        
}
/*-----------------------------------------------------------------------*/
void scart_vcr(char *arg)
{
        unsigned int val=0,old;      
        old=demux_read();
        if (!strcmp(arg,"q")) {
                if (old&0x4000)
                        puts("t");
                else
                        puts(smodes[(old>>7)&7]);
                return;
        }
        if (!strcmp(arg,"t")) {
                demux_write( (old|0x4000));
                return;
        }
        val=get_mode(arg)&7;
        demux_write((old&(~0x4380))|(val<<7));
}
/*-----------------------------------------------------------------------*/
void line_in(char *arg)
{
	char *valtab[4]={"vcr","front","tuner","nc"};
	int val;
	iopl(3);
	if (!strcmp(arg,"q")) {
		val=inb(F1BAR1+0x16);
		val&=3;
		puts(valtab[val]);
		return;
	}
	if (!strcmp(arg,"vcr"))
		val=0;
	else if (!strcmp(arg,"front"))
		val=1;
	else if (!strcmp(arg,"tuner"))
		val=2;
	else
		usage();
	outb(3, F1BAR1+0x15);
	outb(val, F1BAR1+0x16);
}
/*-----------------------------------------------------------------------*/
void scart_vcr_query(void)
{
//        FIXME
        puts("0");
}
/*-----------------------------------------------------------------------*/
void spdif_in(char *arg)
{
        unsigned int val=0,old;
        old=demux_read();
        if (!strcmp(arg,"q")) {
                if (! (old&0x1000))
                        printf("bsp\n");
                else if (! (old&0x800))
                        printf("coax\n");
                else if (! (old&0x400))
                        printf("opt\n");
                else 
                        printf("off\n");
                return;
        }
        if (!strcmp(arg,"bsp"))
                val=0xc00;
        else if (!strcmp(arg,"coax"))
                val=0x1400;
        else if (!strcmp(arg,"opt"))
                val=0x1800;
        else if (!strcmp(arg,"off"))
                val=0x1c00;
        else
                usage();
        demux_write((old&~0x1c00)|val);        
}
/*-----------------------------------------------------------------------*/
int main (int argc, char** argv)
{
        int idx=1,l;
        char *opt;

        while(idx<argc) 
        {
                opt=argv[idx];
		l=0;
                if ((idx+1<argc)) {
                        l=1;
                        if (!strcmp(opt,"-video-mute")) 
                                video_mute(argv[++idx]);
                        else if (!strcmp(opt,"-audio-mute")) 
                                audio_mute(argv[++idx]);
                        else if (!strcmp(opt,"-scart-enable"))
                                scart_enable(argv[++idx]);
                        else if (!strcmp(opt,"-scart-tv")) 
                                scart_tv(argv[++idx]);
                        else if (!strcmp(opt,"-scart-vcr"))
                                scart_vcr(argv[++idx]);
                        else if (!strcmp(opt,"-spdif-in")) 
                                spdif_in(argv[++idx]);
                        else if (!strcmp(opt,"-line-in")) 
                                line_in(argv[++idx]);
			else 
				l=0;
                }                        
		if (l==0) {
			if (!strcmp(opt,"-reset")) 
				reset();
			else if (!strcmp(opt,"-scart-vcr-query")) 
				scart_vcr_query();
			else
				usage();
		}
                idx++;
        }
}
