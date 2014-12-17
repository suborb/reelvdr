// osd.c

#include "osd.h"

#include "channel.h"
#include "hdplayer.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/go8xxx-video.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

#include <hdchannel.h>
#include <hdshm_user_structs.h>

#define MAX_FONTS        2
#define MAX_FONT_HEIGHT 32
#define MAX_FONT_WIDTH  32

extern hd_data_t volatile *hda;
static int osd_enabled=0;

static inline void ClearOsd();
static inline void FlushOsd();
static bool ClipArea(unsigned int *l, unsigned int *t,unsigned int *r,unsigned int *b);
static void DrawRect(unsigned int l, unsigned int t,unsigned int r,unsigned int b,unsigned int color);
static void DrawRect2(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV);

typedef struct 
{
    unsigned int  width;
    unsigned char  pixels[MAX_FONT_HEIGHT][MAX_FONT_WIDTH];
} FontChar;

typedef struct 
{
    unsigned int     height;
    FontChar chars[256];
} Font;

static Font          fonts_[MAX_FONTS];

typedef struct
{
    int width, height;
} CachedImage;

static unsigned int palette[256];

static inline void DrawRectIncl(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color);
static void DrawEllipse(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int quadrants);
static void DrawText(int x, int y, const char *s, unsigned int colorFg, unsigned int colorBg, Font const *font,
                     int width, int height, int alignment);
void DrawImage(unsigned int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat);

#define OSD_WIDTH  osd->width 
#define OSD_HEIGHT osd->height

#define MAX_CACHED_IMAGES_SIZE 2048 * 1024

osd_t *osd;

enum 
{
    taCenter  = 0x00,
    taLeft    = 0x01,
    taRight   = 0x02,
    taTop     = 0x04,
    taBottom  = 0x08,
    taDefault = taTop | taLeft
};

typedef void plError_t;

#define PL_OK
#define PL_ERR

typedef   void     *Pointer;

typedef struct plQueue_
{
    Pointer  *p1;
    Pointer  *p2;
    Pointer  *pBegin;
    Pointer  *pEnd;
    unsigned int      size;
    unsigned int      maxSize;
} plQueue;

plError_t plQueuePut(plQueue *queue, Pointer element)
{
    *queue->p2++ = element;
    ++ queue->size;
    if (queue->p2 == queue->pEnd)
    {
        queue->p2 = queue->pBegin;
    }
    return PL_OK;
}

plError_t plQueueGet(plQueue *queue, Pointer *element)
{
    *element = *queue->p1++;
    -- queue->size;
    if (queue->p1 == queue->pEnd)
    {
        queue->p1 = queue->pBegin;
    }
    return PL_OK;
}

plError_t plQueueInit(plQueue *queue, unsigned int maxSize)
{
    Pointer *p;
     
    p = (Pointer)malloc( maxSize * sizeof(Pointer));
   
    if(!p) {
       printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
       abort();
    } 
    queue->p1      = p;
    queue->p2      = p;
    queue->pBegin  = p;
    queue->pEnd    = p + maxSize;
    queue->size    = 0;
    queue->maxSize = maxSize;
    
    return PL_OK;
}

plError_t plQueueDestroy(plQueue *queue)
{
    free(queue->pBegin);
    return PL_OK;
}

static plQueue       cachedImagesQueue_;
static int           cachedImagesSize_ = 0;

/*-------------------------------------------------------------------------*/
osd_t *new_osd(hd_channel_t *channel)
{
    osd = (osd_t*)malloc(sizeof(osd_t));

    if(!osd) {
       printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
       abort();
    }

    osd->fd = open("/dev/fb0", O_RDWR|O_NDELAY);

    if(osd->fd==-1)
       printf("couldn't open /dev/fb0, error: %s\n", strerror(errno));
    assert(osd->fd!=-1);
    struct fb_var_screeninfo screeninfo;
    ioctl(osd->fd, FBIOGET_VSCREENINFO, &screeninfo);

    osd->bpp = screeninfo.bits_per_pixel/8; 
    osd->width = screeninfo.xres;
    osd->height = screeninfo.yres;

    osd->data = (unsigned char*) mmap(0, osd->bpp* osd->width * osd->height, PROT_READ|PROT_WRITE, MAP_SHARED, osd->fd, 0);

    assert(osd->data!=(void*)-1);
    /* clear osd */
    /* RC: don't clear osd because of start logo
    int i;
    for(i=0; i<osd->bpp*osd->width*osd->height; i++)
	    osd->data[i] = 0;
    */

    osd->buffer = (unsigned char*) malloc(osd->bpp * osd->width * osd->height);
    if(!osd->buffer) {
       printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
       abort();
    }

    printf("FB: xres: %i, yres: %i, bpp: %i, osd->data: %p\n", osd->width, osd->height, osd->bpp, osd->data);

    plQueueInit(&cachedImagesQueue_, HDOSD_MAX_IMAGES);

    return osd;
}
/*-------------------------------------------------------------------------*/
void delete_osd(osd_t *osd)
{
    close(osd->fd);
    free(osd->buffer);
    free(osd);
}

void* thread_loop(void *para){
   printf("osd thread running\n");
   int result;
   while(1){
     // printf(".");
     result = run_osd();
     if(result < 0) usleep(1*1000);
   }
   printf("osd thread ended!\n");
   return NULL;
}

int start_thread()
{
   pthread_t thread;

   //while(1){
   if (!pthread_create(&thread, NULL,thread_loop,(void*)osd)){
       fprintf(stderr,"pthread_detach!\n");
       pthread_detach(thread);
 // }
   }
  return 1;
}
int put_packet_osd(void const *rcvBuffer)
{

    switch(*(char*)(rcvBuffer + sizeof(hd_packet_es_data_t) )){
       case HDCMD_OSD_PALETTE:
           {
    //        printf("HDCMD_OSD_PALETTE\n");
    //    int m;

            hdcmd_osd_palette_t *bco = (hdcmd_osd_palette_t*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));

            int n = bco->count;

            if (n > 256)
            {
                break;
            }
            memcpy(palette, &bco->palette, n * sizeof(int));
            break;
           }

       case HDCMD_OSD_DRAW8:
           {
   //      printf("HDCMD_OSD_DRAW8\n");
            hdcmd_osd_draw8_t *bco=(hdcmd_osd_draw8_t*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            unsigned char const *srcData = (unsigned char const *)(bco->data);
            unsigned char const *xs;
            int qx, qy, xt1, yt1, xt, yt, vfx, vfy, vfw, vfh, x, y, w, h, m, *px, n;

            x=bco->x;
            y=bco->y;
            w=bco->w;
            h=bco->h;

            if (bco->scale)
            {
                if (w>OSD_WIDTH || x>OSD_WIDTH || y>OSD_HEIGHT || h>OSD_HEIGHT)
                        break;
    
                vfx = 0; //vfLayerXPos;
                vfy = 0; //vfLayerYPos;
                vfw = OSD_WIDTH; //vfLayerWidth;
                vfh = OSD_HEIGHT; //vfLayerHeight;
    
                xt1 = vfx * OSD_WIDTH + x * vfw;
                xt = xt1 / OSD_WIDTH;
                yt1 = vfy * OSD_HEIGHT + y * vfh;
                yt = yt1 / OSD_HEIGHT;
                qy = yt1 % OSD_HEIGHT;
                // bm = (unsigned int*)bco->data;
        
                // o=0;
                for(m=0;m<h;)
                {
                    px=(int*)(osd->buffer + OSD_WIDTH * yt * osd->bpp  + xt * osd->bpp );
                    xs = srcData + m * w;
                    qx = xt1 % OSD_WIDTH;
                    for(n=0;n<w;)
                    {
                        // if ((o&3)==0)
                        //         val=*bm++;
                        *px++=palette[*xs];

                        qx += OSD_WIDTH;
                        while (qx >= OSD_WIDTH)
                        {
                            ++xs;
                            ++n;
                            qx -= vfw;
                        }

                        // __SYSLOG("px = %08X\n", px);
                        // val>>=8;
                        // o++;
                    }
                    ++yt;
                    qy += OSD_HEIGHT;
                    while (qy >= OSD_HEIGHT)
                    {
                        ++m;
                        qy -= vfh;
                    }
                }
            }
            else
            {
                unsigned int *bm = (unsigned int*)bco->data;
                int o, val = 0;

                if (w>OSD_WIDTH || x>OSD_WIDTH || y>OSD_HEIGHT || h>OSD_HEIGHT)
                {
                    break;
                }
        
                o=0;
                for (m=0;m<h;m++)
                {
                    px=(int*)(osd->buffer +OSD_WIDTH*(y+m)*osd->bpp+x*osd->bpp);
                    for(n=0;n<w;n++) {
                            if ((o&3)==0)
                                    val=*bm++;
                            *px++=palette[val&255];
                            val>>=8;
                            o++;
                    }
                }
            }
            hda->osd_hidden = 0;
            break;
            }
       case HDCMD_OSD_CLEAR:
         {
           //printf("HDCMD_OSD_CLEAR\n");
           ClearOsd();
	   break;
         }
       case HDCMD_OSD_DRAW_RECT:
         {
           hdcmd_osd_draw_rect *bco = (hdcmd_osd_draw_rect*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));

           DrawRect(bco->l, bco->t, bco->r, bco->b, bco->color);
           hda->osd_hidden = 0;    
         //  printf("HDCMD_OSD_DRAW_RECT - l: %i, t: %i, r: %i, b: %i, color: %x\n", bco->l, bco->t, bco->r, bco->b, bco->color);
	   break; 
         }
       case HDCMD_OSD_DRAW_RECT2:
         {
           hdcmd_osd_draw_rect2 *bco = (hdcmd_osd_draw_rect2*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));

           DrawRect2(bco->l, bco->t, bco->r, bco->b, bco->color, bco->alphaGradH, bco->alphaGradV, bco->alphaGradStepH, bco->alphaGradStepV);
           hda->osd_hidden = 0;    
         //  printf("HDCMD_OSD_DRAW_RECT - l: %i, t: %i, r: %i, b: %i, color: %x\n", bco->l, bco->t, bco->r, bco->b, bco->color);
	   break; 
         }
       case HDCMD_OSD_DRAW_ELLIPSE:
         {
         //printf("HDCMD_OSD_DRAW_ELLIPSE\n");
         hda->osd_hidden = 0;
	 hdcmd_osd_draw_ellipse *bco = (hdcmd_osd_draw_ellipse*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
         DrawEllipse(bco->l, bco->t, bco->r, bco->b, bco->color, bco->quadrants);
	 break;
         }
       case HDCMD_OSD_CACHE_FONT:
         {
         //printf("HDCMD_OSD_CACHE_FONT\n");

            int c, x, y;
            hdcmd_osd_cache_font *bco = (hdcmd_osd_cache_font*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            unsigned int *fontData = (unsigned int *)(bco + 1);
            unsigned int const fontIndex = bco->fontIndex;

            unsigned int const fontHeight = fontData[1];
            Font *font = &fonts_[fontIndex];

            font->height = fontHeight;
            for (c = 0; c < 256; ++c)
            {
                FontChar *fontChar = &font->chars[c];
                unsigned char *pix = &fontChar->pixels[0][0];

                fontChar->width = *fontData;
                fontData += 2;

                for (y = fontHeight; y; --y)
                {
                    unsigned int line = *fontData++;
                    line <<= (32 - fontChar->width);
                    for (x = MAX_FONT_WIDTH; x; --x)
                    {
                        *pix++ = ((int)line) < 0 ? 0x00 : 0xFF;
                        line <<= 1;
                    }
                }
            }

            hda->osd_cached_fonts[fontIndex] = 1;

	 break;
       }
       case HDCMD_OSD_DRAW_TEXT:
        {
            hdcmd_osd_draw_text *bco = (hdcmd_osd_draw_text*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            char const *s = (char const *)(bco + 1);
	    hda->osd_hidden = 0;
            //printf("HDCMD_OSD_DRAW_TEXT %d, %s - fgcolor: %x, bgcolor: %x\n", bco->fontIndex, s, bco->colorFg, bco->colorBg);

           DrawText(bco->x, bco->y, s, bco->colorFg, bco->colorBg,
                    &fonts_[bco->fontIndex], bco->width, bco->height, bco->alignment);

            break;
        }
       case HDCMD_OSD_CACHE_IMAGE:
         {
            hdcmd_osd_cache_image const *bco = (hdcmd_osd_cache_image const *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
	    //printf("HDCMD_OSD_CACHE_IMAGE\n",bco->image_id);
	    int n;
            unsigned int const imageId = bco->image_id;
            int const width = bco->width;
            int const height = bco->height;
            int const pixelCount = width * height;
            int const size = pixelCount * sizeof(unsigned int);
            unsigned int const *imagePixels = (unsigned int const *)(bco + 1);
            unsigned int *tgtPixels;
            CachedImage *cachedImage;
            
            if (hda->osd_cached_images[imageId])
            {
                while (true)
                {
                    unsigned int         cachedImageId;
                    CachedImage *cachedImage;
    
                    plQueueGet(&cachedImagesQueue_, (void *)&cachedImageId);
                    if (cachedImageId == imageId)
                    {
                        cachedImage = (CachedImage *)hda->osd_cached_images[cachedImageId];
                        cachedImagesSize_ -= cachedImage->width * cachedImage->height * sizeof(unsigned int);
            
                        free(cachedImage);
                        hda->osd_cached_images[cachedImageId] = 0;
                        break;
                    }
                    else
                    {
                        plQueuePut(&cachedImagesQueue_, (void *)cachedImageId);
                    }
                }
            }

            while (cachedImagesSize_ + size > MAX_CACHED_IMAGES_SIZE)
            {
                unsigned int         cachedImageId;
                CachedImage *cachedImage;

                // __SYSLOG("Uncacheing Image\n");

                /*if (plQueueIsEmpty(&cachedImagesQueue_))
                {
                    //__SYSLOG("Image %d too big.\n", imageId);
                    return 0;
                }*/

                plQueueGet(&cachedImagesQueue_, (void *)&cachedImageId);
                cachedImage = (CachedImage *)hda->osd_cached_images[cachedImageId];
                cachedImagesSize_ -= cachedImage->width * cachedImage->height * sizeof(unsigned int);
    
                free(cachedImage);
                hda->osd_cached_images[cachedImageId] = 0;
            }
            cachedImage = (CachedImage *)malloc(sizeof(CachedImage) + size);
            if (!cachedImage)
            {
               printf("Error caching image: out of memory!\n");
            }
            cachedImage->width = width;
            cachedImage->height = height;
            tgtPixels = (unsigned int *)(cachedImage + 1);

            for (n = 0; n < pixelCount; ++n)
            {
                unsigned int pix = (*imagePixels++);
                *tgtPixels++ = pix;
            }

            hda->osd_cached_images[imageId] = (int)cachedImage;
            cachedImagesSize_ += size;

            plQueuePut(&cachedImagesQueue_, (void*)imageId);


            break;
         }
       case HDCMD_OSD_DRAW_IMAGE:
        {
            hdcmd_osd_draw_image *bco = (hdcmd_osd_draw_image *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t) );
	    //printf("HDCMD_OSD_DRAW_IMAGE %x\n",bco->image_id);
            DrawImage(bco->image_id, bco->x, bco->y, bco->blend, bco->hor_repeat, bco->vert_repeat);
            hda->osd_hidden = 0;
            break;
        }

       case HDCMD_OSD_FLUSH:
         {
            hdcmd_osd_flush const *bco = (hdcmd_osd_flush const *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
           //printf("HDCMD_OSD_FLUSH\n");
           hda->osd_flush_count = bco->flush_count;
           FlushOsd();
	       break;
         }
       default:
         printf("ERROR: unkown OSD Command!: %i\n", *(char*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t) ));
    }

   return 1;
}

/*-------------------------------------------------------------------------*/
int run_osd()
{
    hd_packet_header_t const *packet_header;
    void *rcvBuffer;
    int packet_size;

    if (!hd_channel_read_start(ch_osd, &rcvBuffer, &packet_size, 0))
    {
        return -1;
    }

    packet_header = (hd_packet_header_t *)rcvBuffer;

    //printf("got_osd_packet(%d) [%d]\n", packet_header->packet_size, packet_header->seq_nr);

    switch (packet_header->type)
    {
    case HD_PACKET_OSD:
        //printf("INFO: HD_PACKET_OSD\n");
        put_packet_osd((hd_packet_es_data_t const *)packet_header);
        break;
    case HD_PACKET_ES_DATA:
        printf("INFO: HD_PACKET_ES_DATA\n");
        //put_packet_es_data(pl, (hd_packet_es_data_t const *)packet_header);
        break;
    default:
        printf("ERROR: Unknown packet type: %d\n", packet_header->type);
    }
    hd_channel_read_finish(ch_osd);
    return 1;
}

static inline unsigned int AlphaBlend(unsigned int p2, unsigned int p1)
{
    unsigned int const xa1 =  (p1 >> 24);
    unsigned int const xa2 = (p2 >> 24);

//    printf("blend: fg: %#x bg: %#x\n", p2, p1);

    if (xa2==255 || !xa1)
    {
        return p2;
    }
    else if (!xa2)
    {
        return p1;
    }
    else
    {
        unsigned int const a1 = 255 - xa1;
	unsigned int const a2 = 255 - xa2;

        unsigned int const r2 = (p2 >> 16) & 255;
        unsigned int const g2 = (p2 >> 8) & 255;
        unsigned int const b2 = p2  & 255;

        unsigned int const r1 = (p1 >> 16) & 255;
        unsigned int const g1 = (p1 >> 8) & 255;
        unsigned int const b1 = p1 & 255;

        unsigned int const al1_al2 = a1 * a2;
        unsigned int const al2     = a2 * 255;
        unsigned int const one     = 255 * 255;

        unsigned int const al1_al2_inv = one - al1_al2; // > 0

        unsigned int const c1 = al2 - al1_al2;
        unsigned int const c2 = one - al2;

        unsigned int const a = al1_al2 / 255;
        unsigned int const r = (c1 * r1 + c2 * r2) / al1_al2_inv;
        unsigned int const g = (c1 * g1 + c2 * g2) / al1_al2_inv;
        unsigned int const b = (c1 * b1 + c2 * b2) / al1_al2_inv;
        return ((255 - a) << 24) | (r << 16) | (g << 8) | b;
    }
}

static bool inline ClipArea(unsigned int *l,unsigned  int *t,unsigned  int *r,unsigned  int *b)
{
    if (*r >= OSD_WIDTH)
    {
        *r = OSD_WIDTH;
    }
    if (*b >= OSD_HEIGHT)
    {
        *b = OSD_HEIGHT;
    }
    return *l < *r && *t < *b;
}

static inline void ClearOsd()
{
    if(!hda->osd_hidden && !hda->osd_dont_touch){ /* only reset buffer & osd if osd wasn't hidden yes */
    	memset(osd->buffer, 0x00, osd->width*osd->height*osd->bpp);
    	memset(osd->data, 0x00, osd->width*osd->height*osd->bpp);
    }
    hda->osd_hidden = 1;
    FlushOsd();
}

static inline void FlushOsd()
{
	int pid = 2;

	if (hda->osd_hidden && !hda->osd_dont_touch) {
		ioctl(osd->fd, DSPC_HIDE, &pid);
		osd_enabled=0;
	}
	else {
		if(!hda->osd_dont_touch)
		        memcpy(osd->data, osd->buffer, osd->width*osd->height*osd->bpp);
		if (!osd_enabled) {
			ioctl(osd->fd, DSPC_SHOW, &pid);
			osd_enabled=1;
		}
	}
}

static void DrawRect(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color)
{
    if (ClipArea(&l, &t, &r, &b))
    {
        unsigned int m, n;
        unsigned int width = r - l;
        unsigned int height = b - t;
        unsigned int *pixels = (int*)(osd->buffer + OSD_WIDTH * t *osd->bpp  + l*osd->bpp);
	unsigned int linebuf[1024];

	for(n=0;n<width;n++)
		linebuf[n]=color;
        for (m = height; m; --m)
        {
		memcpy(pixels,linebuf,width*sizeof(int));
		pixels+=OSD_WIDTH;
        }
    }
}

static void DrawRect2(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV)
{
    if (ClipArea(&l, &t, &r, &b))
    {
        unsigned int m, n;
        unsigned int width = r - l;
        unsigned int height = b - t;
        unsigned int mod = OSD_WIDTH - width;
        unsigned int *pixels = (int*)(osd->buffer + OSD_WIDTH * t *osd->bpp  + l*osd->bpp);
        unsigned int curAlphaH = 0, curAlphaV = 0;

        for(n=0; n<height; n++){
                if(alphaGradStepV && n%alphaGradStepV==0)
                        curAlphaV++;
                for (m=0; m<width; m++){
                        if(alphaGradStepH && m%alphaGradStepH==0)
                                 curAlphaH++;
                        *pixels++ = ( (color&0x00FFFFFF) | (((color>>24) + ((curAlphaH*alphaGradH + curAlphaV*alphaGradV)))<<24) );
                }
                pixels += mod;
                curAlphaH = 0;
        }
    }
}

static int StringWidth(const char *s, Font const *font)
{
    unsigned int c;
    int width = 0;
    const unsigned char *p = (const unsigned char *)s;
    while ((c = *p++))
    {
        if(c<0 || c>= 255){
          printf("ERROR: crappy char: %x\n", c);
        } else {
          FontChar const *fontChar = &font->chars[c];
          width += fontChar->width;
        }
    }
    return width;
}

static void DrawText(int x,
                     int y,
                     const char *s,
                     unsigned int colorFg,
                     unsigned int colorBg,
                     Font const *font,
                     int width,
                     int height,
                     int alignment)
{
    if(!font){
      printf("NO font!\n");
      return;
    }
    int w = StringWidth(s, font);
    int h = font->height;
    int limit = 0;
    int x0 = x;
    if (width || height)
    {
        int cw = width ? width : w;
        int ch = height ? height : h;
        /*if (!intersects(x, y, x + cw - 1, y + ch - 1))
           return;*/
        if ((colorBg >> 24) != 0x00)
        {
            DrawRect(x, y, x + cw, y + ch, colorBg);
        }
        limit = x + cw;
        if (width)
        {
            if ((alignment & taLeft) != 0)
            {
            }
            else if ((alignment & taRight) != 0)
            {
                if (w < width)
                {
                    x += width - w;
                }
            }
            else
            { // taCentered
                if (w < width)
                {
                    x += (width - w) / 2;
                }
            }
        }
        if (height)
        {
            if ((alignment & taTop) != 0)
            {
            }
            else if ((alignment & taBottom) != 0)
            {
                if (h < height)
                {
                    y += height - h;
                }
            }
            else
            { // taCentered
                if (h < height)
                {
                    y += (height - h) / 2;
                }
            }
        }
    }
    else if (/*!intersects(x, y, x + w - 1, y + h - 1)*/ false)
        return;
    // x -= x0;
    // y -= y0;

    while (*s)
    {
        unsigned char c = *s++; // Caution, char is signed.
        //printf("char: %c - %x\n", c, c);
        FontChar const *fontChar = &font->chars[c];
        int charWidth = fontChar->width;
        if (limit && (int)(x + charWidth) > limit)
        {
            break; // we don't draw partial characters
        }
        if (x - x0 >= OSD_WIDTH)
        {
            break;
        }
        // TRACE1("x = %d\n", x);
        if ((int)(x + charWidth) > 0) // char is not completely outside the left screen border
        {
            int row;
            for (row = 0; row < h; ++row)
            {
                unsigned char const *fontPixels = fontChar->pixels[row];
                int *targetPixels = (int*)(osd->buffer + (y + row) * osd->bpp * OSD_WIDTH  + x*osd->bpp);
                int col;
		if (colorBg) {
			// GA: Simplification without AlphaBlend()
			for (col = charWidth; col-- > 0; )
			{
				unsigned char pix=*fontPixels++;
				if (!pix)
					*targetPixels=colorFg;
				targetPixels++;
			}
		}
		else {
			for (col = charWidth; col-- > 0; )
			{
				unsigned int const bgColor = *targetPixels;
				unsigned int const fontColor = (colorFg & 0x00FFFFFF) | ((255 - *fontPixels++) << 24);
#ifdef SEC_CHECK
				if(targetPixels > osd->buffer + osd->width*osd->height*osd->bpp)
					printf("OVERFLOW!\n");
				else
#endif
					*targetPixels++ = AlphaBlend(fontColor, bgColor);
				
				//        printf("fg: %x, bg: %x\n", fontColor, bgColor);
			}
                }
            }
        }
        x += charWidth;
    }
}

static inline void DrawRectIncl(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color)
{
    DrawRect(l, t, r + 1, b + 1, color);
}

static void DrawEllipse(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int quadrants)
{
    int x1 = l;
    int y1 = t;
    int x2 = r - 1;
    int y2 = b - 1;
    int rx, ry, cx, cy;
    int TwoASquare, TwoBSquare, x, y, XChange, YChange, EllipseError, StoppingX, StoppingY;

    if (!ClipArea(&l, &t, &r, &b))
    {
        return;
    }
    // Algorithm based on http://homepage.smc.edu/kennedy_john/BELIPSE.PDF
    rx = x2 - x1;
    ry = y2 - y1;
    cx = (x1 + x2) / 2;
    cy = (y1 + y2) / 2;
    switch (abs(quadrants))
    {
    case 0: rx /= 2; ry /= 2; break;
    case 1: cx = x1; cy = y2; break;
    case 2: cx = x2; cy = y2; break;
    case 3: cx = x2; cy = y1; break;
    case 4: cx = x1; cy = y1; break;
    case 5: cx = x1;          ry /= 2; break;
    case 6:          cy = y2; rx /= 2; break;
    case 7: cx = x2;          ry /= 2; break;
    case 8:          cy = y1; rx /= 2; break;
    }
    TwoASquare = 2 * rx * rx;
    TwoBSquare = 2 * ry * ry;
    x = rx;
    y = 0;
    XChange = ry * ry * (1 - 2 * rx);
    YChange = rx * rx;
    EllipseError = 0;
    StoppingX = TwoBSquare * rx;
    StoppingY = 0;
    while (StoppingX >= StoppingY)
    {
        switch (quadrants)
        {
            case  5: DrawRectIncl(cx,     cy + y, cx + x, cy + y, color); // no break
            case  1: DrawRectIncl(cx,     cy - y, cx + x, cy - y, color); break;
            case  7: DrawRectIncl(cx - x, cy + y, cx,     cy + y, color); // no break
            case  2: DrawRectIncl(cx - x, cy - y, cx,     cy - y, color); break;
            case  3: DrawRectIncl(cx - x, cy + y, cx,     cy + y, color); break;
            case  4: DrawRectIncl(cx,     cy + y, cx + x, cy + y, color); break;
            case  0:
            case  6: DrawRectIncl(cx - x, cy - y, cx + x, cy - y, color); if (quadrants == 6) break;
            case  8: DrawRectIncl(cx - x, cy + y, cx + x, cy + y, color); break;
            case -1: DrawRectIncl(cx + x, cy - y, x2,     cy - y, color); break;
            case -2: DrawRectIncl(x1,     cy - y, cx - x, cy - y, color); break;
            case -3: DrawRectIncl(x1,     cy + y, cx - x, cy + y, color); break;
            case -4: DrawRectIncl(cx + x, cy + y, x2,     cy + y, color); break;
        }
        ++ y;
        StoppingY += TwoASquare;
        EllipseError += YChange;
        YChange += TwoASquare;
        if (2 * EllipseError + XChange > 0)
        {
            x--;
            StoppingX -= TwoBSquare;
            EllipseError += XChange;
            XChange += TwoBSquare;
        }
    }
    x = 0;
    y = ry;
    XChange = ry * ry;
    YChange = rx * rx * (1 - 2 * ry);
    EllipseError = 0;
    StoppingX = 0;
    StoppingY = TwoASquare * ry;
    while (StoppingX <= StoppingY)
    {
        switch (quadrants)
        {
            case  5: DrawRectIncl(cx,     cy + y, cx + x, cy + y, color); // no break
            case  1: DrawRectIncl(cx,     cy - y, cx + x, cy - y, color); break;
            case  7: DrawRectIncl(cx - x, cy + y, cx,     cy + y, color); // no break
            case  2: DrawRectIncl(cx - x, cy - y, cx,     cy - y, color); break;
            case  3: DrawRectIncl(cx - x, cy + y, cx,     cy + y, color); break;
            case  4: DrawRectIncl(cx,     cy + y, cx + x, cy + y, color); break;
            case  0:
            case  6: DrawRectIncl(cx - x, cy - y, cx + x, cy - y, color); if (quadrants == 6) break;
            case  8: DrawRectIncl(cx - x, cy + y, cx + x, cy + y, color); break;
            case -1: DrawRectIncl(cx + x, cy - y, x2,     cy - y, color); break;
            case -2: DrawRectIncl(x1,     cy - y, cx - x, cy - y, color); break;
            case -3: DrawRectIncl(x1,     cy + y, cx - x, cy + y, color); break;
            case -4: DrawRectIncl(cx + x, cy + y, x2,     cy + y, color); break;
        }
        x++;
        StoppingX += TwoBSquare;
        EllipseError += XChange;
        XChange += TwoBSquare;
        if (2 * EllipseError + YChange > 0)
        {
            y--;
            StoppingY -= TwoASquare;
            EllipseError += YChange;
            YChange += TwoASquare;
        }
    }
}

void DrawImage(unsigned int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat)
{
    CachedImage const *img = (CachedImage const *)hda->osd_cached_images[imageId];
    int m, n, h, v;
    int width;    
    int height;    
    unsigned int const *srcPixels;

    if (img)
    {
        width = img->width;    
        height = img->height;    

	if (blend) {
		for (v = vertRepeat; v > 0; --v)
		{
			srcPixels = (unsigned int const *)(img + 1);
			for (n = height; n > 0; --n)
			{
				unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
				for (h = horRepeat; h > 0; --h)
				{
					unsigned int const *src = srcPixels;
					for (m = width; m > 0; --m)
					{
						*tgtPixels = AlphaBlend((*src++), (*tgtPixels) );
						++ tgtPixels;
					}
				}
				srcPixels += width;
			}
		}
	}
	else {
		for (v = vertRepeat; v > 0; --v)
		{
			srcPixels = (unsigned int const *)(img + 1);
			for (n = height; n > 0; --n)
			{
				unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
				for (h = horRepeat; h > 0; --h)
				{
					unsigned int const *src = srcPixels;
					memcpy(tgtPixels, src, width*sizeof(int));
					tgtPixels+=width;
					src+=width;
				}
				srcPixels += width;
			}
		}
	}

    }
    else
    {
        printf("Image %d not cached.\n", imageId);
    }
}

