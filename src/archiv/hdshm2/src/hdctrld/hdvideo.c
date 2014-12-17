/*****************************************************************************
 *
 * HD Player management daemon for DeCypher
 *
 * #include <GPL-header>
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#ifdef CONFIG_MIPS
#include <linux/types.h>
#include <linux/go8xxx-video.h>
#endif

#include "hdshmlib.h"
#include "hdshm_user_structs.h"

#define DSPC_SHOW 0x8101
#define DSPC_HIDE 0x8100

#define GPIO_AUDIO_SEL  10

extern volatile hd_data_t *hdd;

#ifdef CONFIG_MIPS
extern void hdp_kill(void);
static hd_video_mode_t  last_video_mode={0};
static hd_hw_control_t   last_hw_control={0};
static hd_audio_control_t last_audio_control={0};

/*------------------------------------------------------------------------*/
// FIXME: Change to ioctl
void set_gpio(int n,int val)
{
        char path[256];
        int fdx;
        sprintf(path,"/sys/bus/platform/drivers/go7x8x-gpio/pin%i",n);
        fdx=open(path,O_RDWR);
        sprintf(path,"%i\n",val);
        write(fdx,path,sizeof(path));
        close(fdx);
}
/* --------------------------------------------------------------------- */
void set_videomode(char *vop_mode)
{
        dspc_vop_t vmode; 
        int fd;
        
        vmode.oformat=DSPC_OFORMAT_YUV444;
        vmode.oport=DSPC_TV_BOTH;
        vmode.tv_out = 0;

	if (!strcasecmp(vop_mode,"1080p")) { 
                vmode.vop_mode = VOP_MODE_1080p;
        } else if (!strcasecmp(vop_mode,"1080i")) {
                vmode.vop_mode = VOP_MODE_1080i;
        } else if (!strcasecmp(vop_mode, "1080i50")) {
                vmode.vop_mode = VOP_MODE_1080i50;              
        } else if (!strcasecmp(vop_mode, "720p")) {
                vmode.vop_mode = VOP_MODE_720p;
        } else if (!strcasecmp(vop_mode, "720p50")) {
                vmode.vop_mode = VOP_MODE_720p50;               
        } else if (!strcasecmp(vop_mode, "576p")) {
                vmode.vop_mode = VOP_MODE_576p;
        } else if (!strcasecmp(vop_mode, "576i")) {
                vmode.vop_mode = VOP_MODE_576i;
        } else if (!strcasecmp(vop_mode, "480p")) {
                vmode.vop_mode = VOP_MODE_480p;
        } else if (!strcasecmp(vop_mode, "480i")) {
                vmode.vop_mode = VOP_MODE_480i;
        } else if (!strcasecmp(vop_mode, "vga")) {
                vmode.vop_mode = VOP_MODE_VGA;
        } else if (!strcasecmp(vop_mode, "xga")) {
                vmode.vop_mode = VOP_MODE_XGA;
        } else if (!strcasecmp(vop_mode, "1080i48")) {
                vmode.vop_mode = VOP_MODE_1080i48;
        } else { 
                vmode.vop_mode = VOP_MODE_720p;
        }
	fd=open("/dev/fb0",O_RDONLY);
	if (fd<0)
		return;
//	printf("VMODE %i\n",vmode.vop_mode);
        if (ioctl(fd, DSPC_SET_VOP, &vmode))
                printf("set_videomode: %s\n",strerror(errno));
	close(fd);
}
/* --------------------------------------------------------------------- */
void set_planeconfig(int plane, int xpos, int ypos, int w, int h, int ow, int oh, int bpp, int cmode)
{
	dspc_cfg_t cfg;
	int fd;
	cfg.id=plane;
	cfg.xpos=xpos;
	cfg.ypos=ypos;
	cfg.width=w;
	cfg.height=h;
	cfg.owidth=ow;
	cfg.oheight=oh;
	cfg.bpp=bpp;
	cfg.cmode=cmode;
	fd=open("/dev/fb0",O_RDONLY);
	if (fd<0)
		return;
//	printf("PLANECONFIG id %i, %i.%i, %i*%i, %i*%i, bpp %i, cmode %i\n",plane,xpos,ypos,w,h,ow,oh,bpp,cmode);
        if (ioctl(fd, DSPC_P_CFG, &cfg))
                printf("set_planeconfig: %s\n",strerror(errno));
	close(fd);
	
}
/* --------------------------------------------------------------------- */
void set_deinterlacer(int mode)
{
	int fd;
	fd=open("/dev/fb0",O_RDONLY);
	if (fd<0)
		return;
        if (ioctl(fd,DSPC_SET_SCALER ,&mode))
                printf("set_deinterlacer: %s\n",strerror(errno));
	close(fd);
}
/* --------------------------------------------------------------------- */
void set_aspect(int force, int automatic, int aw, int ah, int overscan, int amode)
{
        int fd;
        dspc_aspect_ratio_t aspect_ratio;
        int aspect_mode;
#if 0       
	char cmdstr[256]="";
	char *scalemode[4]={"F2S","F2A","C2F","DBD"};
	sprintf(cmdstr,"aspect -a %i -w %i -h %i -o %i -m %s >/dev/null",
		automatic, aw, ah, 
		overscan, scalemode[amode]);
	printf("#### %s\n",cmdstr);
	system(cmdstr);
#else
	fd=open("/dev/fb0",O_RDONLY);
	if (fd<0)
		return;
        if (ioctl(fd, DSPC_GET_ASPECT_RATIO, &aspect_ratio) < 0)
                goto out;

        if (amode==0)
                aspect_mode=ASPECT_MODE_FILL2SCREEN;
        else if (amode==1)
                aspect_mode=ASPECT_MODE_FILL2ASPECT;
        else if (amode==2)
                aspect_mode=ASPECT_MODE_CROP2FILL;
        else 
                aspect_mode=ASPECT_MODE_DOTBYDOT;
        
        aspect_ratio.automatic_flag=automatic;     
        aspect_ratio.width=aw;
        aspect_ratio.height=ah;
        aspect_ratio.overscan=overscan;

        if (ioctl(fd, DSPC_SET_ASPECT_RATIO, &aspect_ratio) < 0)
                goto out;
        if (ioctl(fd, DSPC_SET_ASPECT_MODE, &aspect_mode) < 0)
                goto out;
        
        dspc_init_t idata;
                
        memset(&idata, 0, sizeof(dspc_init_t));

	// Force aspect update
        idata.cfg.id = DSPC_MP;
        if (ioctl(fd, DSPC_GET_INFO, &idata) < 0)
            	goto out;

        idata.cfg.flags |= DSPC_P_CFG_FORCE_UPD;
        ioctl(fd, DSPC_P_CFG, &idata.cfg);

out:
        close(fd);        
#endif
}
/* --------------------------------------------------------------------- */
void set_vmode(volatile hd_video_mode_t *vm)
{
	char vmodestr[256]="";
	char hdmistr[256]="";
	char analogstr[256]="";
	char fmtstr[256]="";
	char cmdstr[256]="";
	int w=vm->width;
	int h=vm->height;
	int fw=720;
	int fh=576;
	int fvw=w;
	int fvh=h;
	int fx=0;
	int fy=0;
	int fb=4;
	int fm=1;
	int i=vm->interlace;
	int fps=vm->framerate;
	int xfd;
	int vs=vm->aspect.scale & HD_VM_SCALE_VID;
	int fs=vm->aspect.scale & HD_VM_SCALE_FB;
	int fp=vm->aspect.scale & HD_VM_SCALE_PIC;
	int dvihdmi=0;

	xfd=open("/sys/class/wis/adec0/passthrough",O_WRONLY);
	
	if (xfd) {
		if (vm->outputda == HD_VM_OUTDA_AC3) {
			printf("Switching on AC3-loopthrough\n");
			write(xfd,"1\n",2);
		}
		else {
			write(xfd,"0\n",2);
			printf("Switching off AC3-loopthrough\n");
		}
		close(xfd);
	}


	if (vm->outputd == HD_VM_OUTD_HDMI) {		
		strcpy(hdmistr,"hdmi");
		dvihdmi=1;
	}
	else {
		strcpy(hdmistr,"dvi");
		dvihdmi=0;
	}
	switch(h) {
	case 1080:
#if 0	
		if (i==0) { // 1080p
			strcpy(fmtstr,"1080p");
		}
		else 
#endif
		{
			if (fps==60) { // 1080i60
				strcpy(fmtstr,"1080i");
			}
			else if (fps==48){ // 1080i48
				strcpy(fmtstr,"1080i48");
			}
			else {
				strcpy(fmtstr,"1080i50");
			}
			
		}
		break;

	case 720:
		if (fps==60) { // 720p60
			strcpy(fmtstr,"720p");

		}
		else { // 720p50
			strcpy(fmtstr,"720p50");
		}
		break;

	case 480:
		if (i==0) { // 480p
			strcpy(fmtstr,"480p");
		}
		else {
#define DISABLE_SDTV_INTERLACED		
#ifdef DISABLE_SDTV_INTERLACED		
			strcpy(fmtstr,"480p");
#else
        		strcpy(fmtstr,"480i");
#endif		
			strcpy(analogstr,"480i");	
		}

		break;

	case 576:
	default: 
		if (i==0) { // 576p
			strcpy(fmtstr,"576p");
		}
		else {
#ifdef DISABLE_SDTV_INTERLACED
			// HDMI progressive, Focus making interlace out of it
			strcpy(fmtstr,"576p");
#else
			strcpy(fmtstr,"576i");
#endif			
			strcpy(analogstr,"576i"); 			
		}
		
		break;
	}
	if(fp) {
	    switch(fp) {
		case HD_VM_SCALE_PIC_576:
		    fw=720;  fh=576; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=3; fm=0;
		    break;
		case HD_VM_SCALE_PIC_576A:
		    fw=720;  fh=576; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=4; fm=1;
		    break;
		case HD_VM_SCALE_PIC_600:
		    fw=800;  fh=600; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=3; fm=0;
		    break;
		case HD_VM_SCALE_PIC_600A:
		    fw=800;  fh=600; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=4; fm=1;
		    break;
		case HD_VM_SCALE_PIC_720:
		    fw=1280; fh=720; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=3; fm=0;
		    break;
	    }
	} else {
	    switch(fs) {
		case HD_VM_SCALE_FB_FIT:
		    fw=720;  fh=576; fvw=w;  fvh=h;  fx=0;        fy=0;        fb=4; fm=1;
		    break;
		case HD_VM_SCALE_FB_DBD:
		    fw=720;  fh=576; fvw=fw; fvh=fh; fx=(w-fw)/2; fy=(h-fh)/2; fb=4; fm=1;
		    break;
		default: // old 800x600 mode - already set
		    break;
	    }
	} // if
    
	strcpy(vmodestr,fmtstr);
	
	if (vm->outputd==HD_VM_OUTD_OFF) 
		strcpy(hdmistr,"off");
	else
		strcat(hdmistr,fmtstr);

	if (!analogstr[0])
		strcpy(analogstr,fmtstr);


	if ((vm->outputa&0xff)==HD_VM_OUTA_OFF) {  // powerdown
		strcpy(analogstr,"off");
	}
	else if (!strcmp(analogstr,"576i") || !strcmp(analogstr,"480i") ) {
		if ((vm->outputa&0xff00)==HD_VM_OUTA_PORT_SCART)
			strcat (analogstr, " out_scart");
		else if ((vm->outputa&0xff00)==HD_VM_OUTA_PORT_MDIN)
			strcat (analogstr, " out_mdin");

		if ((vm->outputa&0xff)==HD_VM_OUTA_YUV)
			strcat (analogstr, " yuv");
		else if ((vm->outputa&0xff)==HD_VM_OUTA_YC)
			strcat (analogstr, " yc");
		else if ((vm->outputa&0xff)==HD_VM_OUTA_RGB)
			strcat (analogstr, " rgb");
	}
	else {
		strcat(analogstr,"Y");
	}

	{
#define SI9030_DVIHDMI	_IOR ('d', 351, int)  // DVI=0, HDMI=1
		int fd;
		fd=open("/dev/si9030",O_RDWR);
		if (fd>=0) {
			ioctl(fd,SI9030_DVIHDMI,dvihdmi);
			close(fd);
		}	
	}

	set_videomode(vmodestr);
#if 1
	if (!strcmp(hdmistr,"off")) {
		    sprintf(cmdstr,"setsi9030 %s >/dev/null",hdmistr);
		    printf("#### %s\n",cmdstr);
		    system(cmdstr);
	}
#endif

	// Avoid blue flicker, always re-set OSD
//	sprintf(cmdstr,"fbset -r 800 600 %i %i >/dev/null",w,h);

#if 1
	sprintf(cmdstr,"fbset -pi 2 %i %i %i %i %i %i %i %i >/dev/null",fx,fy,fw,fh,fvw,fvh,fb,fm);
	printf("#### %s\n",cmdstr);
	system(cmdstr);
#else
        set_planeconfig(2,fx,fy,fw,fh,fvw,fvh,fb,fm);		
#endif	

	// Disable Bob deinterlacer
	if (!strncmp(analogstr,"576i",4) || !strncmp(analogstr,"480i",4))  {
	        vm->aspect.overscan=1; // disable overscan
		set_deinterlacer(1);
        }      
	else {
	        vm->aspect.overscan=1; // disable overscan
		set_deinterlacer(0);
        }

	// Now aspect
	if (vs>=0 && vs<=HD_VM_SCALE_DBD) {
                set_aspect(0,vm->aspect.automatic, vm->aspect.w, vm->aspect.h,
                                vm->aspect.overscan, vs);
	}

	sprintf(cmdstr,"setfs453 %s >/dev/null",analogstr);
	printf("#### %s\n",cmdstr);
	system(cmdstr);	
}
/* --------------------------------------------------------------------- */
void enable_fb_layer(int enable) {
    int pid = 2; // FB
    int fd = open("/dev/fb0", O_RDWR|O_NDELAY);
    if (fd == -1) 
	return;
    ioctl(fd, enable ? DSPC_SHOW : DSPC_HIDE,&pid);
// tiqq: save bandwidth?
//    pid = 0; // MP
//    ioctl(fd, enable ? (enable & HD_VM_SCALE_PIC_A ? DSPC_SHOW : DSPC_HIDE) : DSPC_SHOW,&pid);
    close(fd);
}
/* --------------------------------------------------------------------- */
void set_hw_control(hd_hw_control_t *hwc)
{
	if (hwc->audiomix)
		set_gpio(GPIO_AUDIO_SEL,0);
	else
		set_gpio(GPIO_AUDIO_SEL,1);
}
/* --------------------------------------------------------------------- */
void set_audio_control(hd_audio_control_t *hac)
{
	int fd;
	char volstr[32];
	fd=open("/sys/class/wis/adec0/volume",O_RDWR);
	if (fd>=0) {
		sprintf(volstr,"0x%x\n", hac->volume);
		write(fd,volstr,strlen(volstr));
		close(fd);
	}
}
/* --------------------------------------------------------------------- */
// aspect can be done on-the-fly, detect aspect-only change
int find_video_changes(void)
{
	hd_video_mode_t a,b;
	a=hdd->video_mode;
	b=last_video_mode;
	a.changed=b.changed=0;  // Mask out
	a.aspect.scale&=~HD_VM_SCALE_VID;
	b.aspect.scale&=~HD_VM_SCALE_VID;
	a.aspect.w=0;
	b.aspect.w=0;
	a.aspect.h=0;
	b.aspect.h=0;

	if (!memcmp(&a,&b,sizeof(hd_video_mode_t)))
		return 1;

	return 0;
}
/* --------------------------------------------------------------------- */
// misnomer: checks all settings for changes
void video_check_settings(void)
{
	int tmp;
	if (hdd->video_mode.changed!=last_video_mode.changed) {
//		printf("############# %i %i\n",hdd->video_mode.changed, last_video_mode.changed);
		int last_fp = last_video_mode.aspect.scale & HD_VM_SCALE_PIC;
		int curr_fp = hdd->video_mode.aspect.scale & HD_VM_SCALE_PIC;
		int aspect_only=find_video_changes();

		// Mode switching is tricky...
		if (find_video_changes()) {
			set_aspect(0,hdd->video_mode.aspect.automatic, hdd->video_mode.aspect.w, hdd->video_mode.aspect.h,
                                hdd->video_mode.aspect.overscan, hdd->video_mode.aspect.scale & HD_VM_SCALE_VID);	
		}
		else
		{
			last_video_mode=hdd->video_mode;

			tmp=hdd->hdp_enable;
			hdd->hdp_enable=0;
			hdp_kill();
			usleep(1000*1000);

			set_vmode(&hdd->video_mode);
			if(last_fp != curr_fp) 
				enable_fb_layer(curr_fp);
			hdd->hdp_enable=tmp;
		}			
	}

	if (hdd->hw_control.changed!=last_hw_control.changed) {
		last_hw_control=hdd->hw_control;
		set_hw_control(&last_hw_control);
	}

	if (hdd->audio_control.changed!=last_audio_control.changed) {
		last_audio_control=hdd->audio_control;
		set_audio_control(&last_audio_control);
	}
	
}
#endif // CONFIG_MIPS
/* --------------------------------------------------------------------- */
void set_dont_touch_osd(int dont_touch_osd)
{
        hdd->osd_dont_touch = dont_touch_osd;
#ifdef CONFIG_MIPS
        enable_fb_layer(dont_touch_osd);
#endif
}

/* --------------------------------------------------------------------- */
// asp: WF WS NF NS Wide/Normal Fill/Scale

void set_video_from_string(char *vm, char* dv, char* asp, char * analog_mode, char * port_mode, char* digital_audio_mode)
{
	int w,h,i,fps;
	int dvimode=HD_VM_OUTD_HDMI;
	hd_aspect_t hda={0};
	int default_asp=1; // 16:9
	w=h=i=fps=0;

	if(!strcasecmp(digital_audio_mode, "AC3")) {
		hdd->video_mode.outputda=HD_VM_OUTDA_AC3;
	}
	else if (!strcasecmp(digital_audio_mode, "PCM")) {
		hdd->video_mode.outputda=HD_VM_OUTDA_PCM;
	}

	if (!strcasecmp(vm,"1080p")) {
		w=1920;
		h=1080;
		i=0;
		fps=60;
	}
	else if (!strcasecmp(vm,"1080i")) {
		w=1920;
		h=1080;
		i=1;
		fps=60;
	}
	else if (!strcasecmp(vm,"1080i50")) {
		w=1920;
		h=1080;
		i=1;
		fps=50;
	}
	else if (!strcasecmp(vm,"720p")) {
		w=1280;
		h=720;
		i=0;
		fps=60;
	}
	else if (!strcasecmp(vm,"720p50")) {
		w=1280;
		h=720;
		i=0;
		fps=50;
	}
	else if (!strcasecmp(vm,"480p")) {
		w=720;
		h=480;
		i=0;
		fps=60;
		default_asp=0;
	}
	else if (!strcasecmp(vm,"480i")) {
		w=720;
		h=480;
		i=1;
		fps=60;
		default_asp=0;
	}
	else if (!strcasecmp(vm,"576p")) {
		w=720;
		h=576;
		i=0;
		fps=50;
		default_asp=0;
	}
	else if (!strcasecmp(vm,"576i")) {
		w=720;
		h=576;
		i=1;
		fps=50;
		default_asp=0;
	}

	if (!strcasecmp(dv,"DVI")) {
		dvimode=HD_VM_OUTD_DVI;
	}

	if (!strcasecmp(asp,"WF") ) {
		hda.w=16;
		hda.h=9;
		hda.scale=HD_VM_SCALE_F2S;
		hda.overscan=0;
		hda.automatic=0;
	}
	if (!strcasecmp(asp,"WS") || (strlen(asp)==0 && default_asp)) {
		hda.w=16;
		hda.h=9;
		hda.scale=HD_VM_SCALE_F2A;
		hda.overscan=0;
		hda.automatic=0;
	}
	if (!strcasecmp(asp,"WC")) {
		hda.w=16;
		hda.h=9;
		hda.scale=HD_VM_SCALE_C2F;
		hda.overscan=0;
		hda.automatic=0;
	}
	if (!strcasecmp(asp,"NF")) {
		hda.w=4;
		hda.h=3;
		hda.scale=HD_VM_SCALE_F2S;
		hda.overscan=0;
		hda.automatic=0;
	}
	if (!strcasecmp(asp,"NS")) {
		hda.w=4;
		hda.h=3;
		hda.scale=HD_VM_SCALE_F2A;
		hda.overscan=0;
		hda.automatic=0;
	}
	if (!strcasecmp(asp,"NC")) {
		hda.w=4;
		hda.h=3;
		hda.scale=HD_VM_SCALE_C2F;
		hda.overscan=0;
		hda.automatic=0;
	}

	if (w) {
		hdd->video_mode.width=w;
		hdd->video_mode.height=h;
		hdd->video_mode.interlace=i;
		hdd->video_mode.framerate=fps;
		hdd->video_mode.outputd=HD_VM_OUTD_HDMI;
		hdd->video_mode.outputa=HD_VM_OUTA_AUTO;
	}

	if (hda.w)
		hdd->video_mode.aspect=hda;

	hdd->video_mode.outputd=dvimode;
	
	if (!strcasecmp(vm,"off")) {
		hdd->video_mode.outputd=HD_VM_OUTD_OFF;
		hdd->video_mode.outputa=HD_VM_OUTA_OFF;
	}
	else {
		if (!strcasecmp(analog_mode,"yuv"))
			hdd->video_mode.outputa=HD_VM_OUTA_YUV;
		else if (!strcasecmp(analog_mode,"rgb"))
			hdd->video_mode.outputa=HD_VM_OUTA_RGB;
		else if (!strcasecmp(analog_mode,"yc"))
			hdd->video_mode.outputa=HD_VM_OUTA_YC;

		if (!strcasecmp(port_mode,"scart"))
			hdd->video_mode.outputa|=HD_VM_OUTA_PORT_SCART;
		else if (!strcasecmp(port_mode,"MDIN"))
			hdd->video_mode.outputa|=HD_VM_OUTA_PORT_MDIN;
	}

	hdd->video_mode.changed++;
}
/* --------------------------------------------------------------------- */
void set_audio_mix(int audiomix)
{
	hdd->hw_control.audiomix=audiomix;
	hdd->hw_control.changed++;
}
/* --------------------------------------------------------------------- */
void set_audio_volume(int volume)
{
	hdd->audio_control.volume=volume;
	hdd->audio_control.changed++;
}
/* --------------------------------------------------------------------- */
