/*
 * fbshot.c -- FrameBuffer Screen Capture Utility
 * (C)opyright 2002 sfires@sfires.net
 *
 * Originally Written by: Stephan Beyer <PH-Linex@gmx.net>
 * Further changes by: Paul Mundt <lethal@chaoticdreams.org>
 * Rewriten and maintained by: Dariusz Swiderski <sfires@sfires.net>
 * 
 * 	This is a simple program that generates a
 * screenshot of the specified framebuffer device and
 * terminal and writes it to a specified file using
 * the PNG format.
 *
 * See ChangeLog for modifications, CREDITS for credits.
 * 
 * Small changes for Reel Multimedia's HDE 
 *   by Georg Acher, BayCom GmbH (acher(at)baycom.de)
 *
 *
 * fbshot is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either Version 2
 * of the License, or (at your option) any later version.
 *
 * fbshot is distributed in the hope that it will be useful, but
 * WITHOUT ANY  WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License with fbshot; if not, please write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <byteswap.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mntent.h>
#include <errno.h>
#include <sys/utsname.h>

#include <sys/vt.h>
#include <png.h>
#include <linux/fb.h> 

#define DEFAULT_FB      "/dev/fb0"
#define PACKAGE 	"fbshot"
#define VERSION 	"0.3"
#define MAINTAINER_NAME "Dariusz Swiderski"
#define MAINTAINER_ADDR "sfires@sfires.net"

static int waitbfg=0; /* wait before grabbing (for -C )... */


/* some conversion macros */
#define RED565(x)    ((((x) >> (11 )) & 0x1f) << 3)
#define GREEN565(x)  ((((x) >> (5 )) & 0x3f) << 2)
#define BLUE565(x)   ((((x) >> (0)) & 0x1f) << 3)

#define ALPHA1555(x) ((((x) >> (15)) & 0x1 ) << 0)
#define RED1555(x)   ((((x) >> (10)) & 0x1f) << 3)
#define GREEN1555(x) ((((x) >> (5 )) & 0x1f) << 3)
#define BLUE1555(x)  ((((x) >> (0 )) & 0x1f) << 3)

struct picture{
  int xres,yres;
  char *buffer;
  struct fb_cmap *colormap;
  char bps,gray;
};

void FatalError(char* err){
  fprintf(stderr,"An error occured: %s %s\nExiting now...\n",err,strerror(errno));
  fflush(stderr);
  exit (1);
}

void Usage(char *binary){
  printf("Usage: %s [-ghi] [-{C|c} vt] [-d dev] [-s n] filename.png\n", binary);
}

void Help(char *binary){
    printf("FBShot - makes screenshots from framebuffer, v%s\n", VERSION);
    printf("\t\tby Dariusz Swiderski <sfires@sfires.net>\n\n");

    Usage(binary);

    printf("\nPossible options:\n");
    printf("\t-C n  \tgrab from console n, for slower framebuffers\n");
    printf("\t-c n  \tgrab from console n\n");
    printf("\t-d dev\tuse framebuffer device dev instead of default\n");
/* not supported as for now
    printf("\t-g    \tsave a grayscaled PNG\n");
 */
    printf("\t-h    \tprint this usage information\n");
    printf("\t-i    \tturns OFF interlacing\n");
    printf("\t-s n  \tsleep n seconds before making screenshot\n");

    printf("\nSend feedback !!!\n");
}

void chvt(int num){
  int fd;
  if(!(fd = open("/dev/console", O_RDWR)))
    FatalError("cannot open /dev/console");
  if (ioctl(fd, VT_ACTIVATE, num))
    FatalError("ioctl VT_ACTIVATE ");
  if (ioctl(fd, VT_WAITACTIVE, num))
    FatalError("ioctl VT_WAITACTIVE");
  close(fd);
  if (waitbfg)
    sleep (3);
}
int sleepval;
int read_fb(char *device, int vt_num, struct picture *pict){
  int fd, vt_old, i,j;
  struct fb_fix_screeninfo fb_fixinfo;
  struct fb_var_screeninfo fb_varinfo;
  struct vt_stat vt_info;

  if (vt_num!=-1){
    if ((fd = open("/dev/console", O_RDONLY)) == -1)
      FatalError("could not open /dev/console");
    if (ioctl(fd, VT_GETSTATE, &vt_info))
      FatalError("ioctl VT_GETSTATE");
    close (fd);
    vt_old=vt_info.v_active;
  }

  if(!(fd=open(device, O_RDONLY)))
    FatalError("Couldn't open framebuffer device");

  if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_fixinfo))
    FatalError("ioctl FBIOGET_FSCREENINFO");

  if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_varinfo))
    FatalError("ioctl FBIOGET_VSCREENINFO");

  pict->xres=fb_varinfo.xres;
  pict->yres=fb_varinfo.yres;
  pict->bps=fb_varinfo.bits_per_pixel;
  pict->gray=fb_varinfo.grayscale;

  if(fb_fixinfo.visual==FB_VISUAL_PSEUDOCOLOR){
    pict->colormap=(struct fb_cmap*)malloc(sizeof(struct fb_cmap));
    pict->colormap->red=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
    pict->colormap->green=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
    pict->colormap->blue=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
    pict->colormap->transp=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
    pict->colormap->start=0;
    pict->colormap->len=1<<pict->bps;
    if (ioctl(fd, FBIOGETCMAP, pict->colormap))
      FatalError("ioctl FBIOGETCMAP");
  }
  if (vt_num!=-1)
    chvt(vt_old);
   
  switch(pict->bps){
  case 15:
    i=2;
    break;
  default:
    i=pict->bps>>3;
  }
   
  if(!(pict->buffer=malloc(pict->xres*pict->yres*i)))
    FatalError("couldnt malloc");

  fprintf(stdout, "Framebuffer %s is %i bytes.\n", device,
                    (fb_varinfo.xres * fb_varinfo.yres * i));
  fprintf(stdout, "Grabbing %ix%i ... \n", fb_varinfo.xres, fb_varinfo.yres);

#ifdef DEBUG
/* Output some more information bout actual graphics mode
 */
  fprintf(stdout, "%ix%i [%i,%i] %ibps %igr\n",
  	fb_varinfo.xres_virtual, fb_varinfo.yres_virtual,
  	fb_varinfo.xoffset, fb_varinfo.yoffset,
  	fb_varinfo.bits_per_pixel, fb_varinfo.grayscale); 
  fprintf(stdout, "FIX: card:%s mem:0x%.8X mem_len:%d visual:%i type:%i type_aux:%i line_len:%i accel:%i\n",
  fb_fixinfo.id,fb_fixinfo.smem_start,fb_fixinfo.smem_len,fb_fixinfo.visual,
  fb_fixinfo.type,fb_fixinfo.type_aux,fb_fixinfo.line_length,fb_fixinfo.accel);
#endif

  fflush(stdout);
  if (vt_num!=-1)
    chvt(vt_num);

  j= (read(fd, pict->buffer, ((pict->xres * pict->yres) * i) )!=
  	(pict->xres * pict->yres *i ));
#ifdef DEBUG
  printf("to read:%i read:%i\n",(pict->xres* pict->yres * i), j);	
#endif
  if (vt_num!=-1)
    chvt(vt_old); 

  if(j)
    FatalError("couldn't read the framebuffer");
  else
    fprintf(stdout,"done.\n");
  close (fd);
  return 0;
}
  
void convert8to32(struct picture *pict){
  int i;
  int j=0;
  __u8 c;
  char *out=(char*)malloc(pict->xres*pict->yres*4);
  for (i=0; i<pict->xres*pict->yres; i++)
  {
    c = ((__u8*)(pict->buffer))[i];
    out[j++]=(char)(pict->colormap->red[c]);
    out[j++]=(char)(pict->colormap->green[c]);
    out[j++]=(char)(pict->colormap->blue[c]);
    out[j++]=(char)(pict->colormap->transp[c]);
  }
  free(pict->buffer);
  pict->buffer=out;
}

void convert1555to32(struct picture *pict){
  int i;
  int j=0;
  __u16 t,c;
  char *out=(char*)malloc(pict->xres*pict->yres*4);
  for (i=0; i<pict->xres*pict->yres; i++)
  {
    c = ( (__u16*)(pict->buffer))[i];
    out[j++]=(char)RED1555(c);
    out[j++]=(char)GREEN1555(c);
    out[j++]=(char)BLUE1555(c);
    out[j++]=(char)ALPHA1555(c);
  }
  free(pict->buffer);
  pict->buffer=out;
}

void convert565to24(struct picture *pict){
  int i;
  int j=0;
  __u16 t,c;
  char *out=(char*)malloc(pict->xres*pict->yres*3);
  for (i=0; i<pict->xres*pict->yres; i++)
  {
    c = ( (__u16*)(pict->buffer))[i];
    out[j++]=(char)RED565(c);
    out[j++]=(char)GREEN565(c);
    out[j++]=(char)BLUE565(c);
  }
  free(pict->buffer);
  pict->buffer=out;
}

void convert32(struct picture *pict){
  int i;
  int j=0;
  __u32 c;
  for (i=0; i<pict->xres*pict->yres; i++)
  {
    c = ( (__u32*)(pict->buffer))[i];
    c = (c&0x0000ff00) | ((c&255)<<16)|((c>>16)&255); // remove alpha, swap R/B
    ( (__u32*)(pict->buffer))[i]=c;
  }
}

static int Write_PNG(struct picture * pict, char *filename, int interlace, int gray){
  png_bytep *row_pointers;
  png_structp png_ptr;
  png_infop info_ptr;
  png_text txt_ptr[4];
  struct utsname *host_info;
  char *out;
  int i;
  int bit_depth=0, color_type;   
  FILE *OUTfd = fopen(filename, "wb");
#ifdef DEBUG
    i=open("pict.bin",O_CREAT|O_WRONLY|O_TRUNC);
    fprintf(stdout, "%i\n",write(i,(void*)pict->buffer,(pict->xres)*(pict->yres)));
    perror("dupa");
    fprintf(stdout, "Writing to %s %ix%i %i\n", filename,(pict->xres),(pict->yres),(pict->xres)*(pict->yres));
    fprintf(stdout, "start: %i, size: %i\n", pict->colormap->start,pict->colormap->len);
    fflush(stdout);
    close(i);
#endif

  if (!OUTfd)
    FatalError("couldn't open output file");

  png_ptr = png_create_write_struct(
        	PNG_LIBPNG_VER_STRING, 
        	(png_voidp)NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);
        
  if (!png_ptr)
    FatalError("couldn't create PNG write struct.");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    FatalError("couldn't create PNG info struct.");
  }

/*  host_info=(struct utsname*)malloc(sizeof(struct utsname));
  uname(host_info);*/
  txt_ptr[0].key="Name";
  txt_ptr[0].text="FrameBuffer screenshot";
  txt_ptr[0].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[1].key="Date";
  txt_ptr[1].text="Current Date";
  txt_ptr[1].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[2].key="Hostname";
  txt_ptr[2].text="snoopy";
  txt_ptr[2].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[3].key="Program";
  txt_ptr[3].text=PACKAGE" v."VERSION;
  txt_ptr[3].compression=PNG_TEXT_COMPRESSION_NONE;

  png_set_text(png_ptr, info_ptr, txt_ptr, 4);

  png_init_io(png_ptr, OUTfd);
    
  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

  row_pointers=(png_bytep*)malloc(sizeof(png_bytep)*pict->yres);

  switch(pict->bps) {
    case 8:
      convert8to32(pict);
      bit_depth=8;
      color_type=PNG_COLOR_TYPE_RGB_ALPHA;
      for (i=0; i<(pict->yres); i++)
        row_pointers[i]=pict->buffer+i*4*(pict->xres);
      png_set_invert_alpha(png_ptr);
      break;
  
    case 15:
      convert1555to32(pict);
      bit_depth=8;
      color_type=PNG_COLOR_TYPE_RGB_ALPHA;
      for (i=0; i<(pict->yres); i++)
        row_pointers[i]=pict->buffer+i*4*(pict->xres);
      png_set_invert_alpha(png_ptr);
      break;

    case 16:
      convert565to24(pict);
      bit_depth=8;
      color_type=PNG_COLOR_TYPE_RGB;
      for (i=0; i<(pict->yres); i++)
        row_pointers[i]=pict->buffer+i*3*(pict->xres);
      png_set_invert_alpha(png_ptr);
      break;

    case 24:
      bit_depth=8;
      color_type=PNG_COLOR_TYPE_RGB;
      for (i=0; i<(pict->yres); i++)
        row_pointers[i]=pict->buffer+i*3*(pict->xres);
      png_set_invert_alpha(png_ptr);
      break;

    case 32:
      convert32(pict);
      bit_depth=8;
      color_type=PNG_COLOR_TYPE_RGB_ALPHA;
      for (i=0; i<(pict->yres); i++)
        row_pointers[i]=pict->buffer+i*4*(pict->xres);
      png_set_invert_alpha(png_ptr);
      break;
    }
    
    if (bit_depth==0){
        fprintf (stderr, "%d bits per pixel are not yet supported! ", pict->bps);
        fprintf (stderr, "But you may write it...\n");
        exit(1);
    }

    png_set_IHDR(png_ptr, info_ptr, pict->xres, pict->yres, 
        bit_depth, color_type, interlace, 
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    png_write_info(png_ptr, info_ptr);

    png_write_image(png_ptr, row_pointers);

    png_write_end(png_ptr, info_ptr);
    /* puh, done, now freeing memory... */
    png_destroy_write_struct(&png_ptr, &info_ptr);
   

    free(row_pointers);
    fclose(OUTfd);
    return 0;
 }



static char optstring[] = "hiC:c:d:s:";
static struct option long_options[] = {
        {"slowcon", 1, 0, 'C'},
        {"console", 1, 0, 'c'},
        {"device", 1, 0, 'd'},
        {"help", 0, 0, 'h'},
        {"noint", 0, 0, 'i'},
        {"sleep", 1, 0, 's'},
        {0, 0, 0, 0}
        };
                                                                
int main(int argc, char **argv){
  char *buffer, *device = NULL, *outfile = argv[argc-1];
  int optc, vt_num=-1;
  struct picture pict;
  int interlace=PNG_INTERLACE_ADAM7;
  int gray=0; /* -1 on ; 0 off ; */
  pict.colormap=NULL;
  sleepval=0;
  for(;;){
    int optind = 0;

    if ((optc = getopt_long(argc, argv, optstring, long_options, &optind)) == -1)
      break;

    switch (optc){
    case 'C':
      waitbfg=1;
    case 'c':
      vt_num=atoi(optarg);
      break;
    case 'd':
      device=optarg;
      break;
/* not supported as for now
    case 'g':
      gray=-1;
      break;
 */
    case 'h':
      Help(argv[0]);
      return 1;
      break;
    case 'i':
      interlace=PNG_INTERLACE_NONE;
      break;
    case 's':
      sleepval=atoi(optarg);

      break;
    default:
      break;
    }
  }

  if ((optind==argc) || (1!=argc-optind)){
    Usage(argv[0]);
    return 1;
  }

  if (NULL==device){
    device=getenv("FRAMEBUFFER");
    if (NULL==device){
      device=DEFAULT_FB;
    }
  }

  read_fb(device, vt_num, &pict);

  printf("Writing %s ...",outfile);fflush(stdout);
      
  Write_PNG(&pict, outfile, interlace, gray );

  if(pict.colormap){
    free(pict.colormap->red);
    free(pict.colormap->green);
    free(pict.colormap->blue);
    free(pict.colormap->transp);
    free(pict.colormap);
  }
  free(pict.buffer);
    
  printf ("done.\n");
    
  return 0;
}        
