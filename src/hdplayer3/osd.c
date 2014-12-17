// osd.c
#include <sys/time.h>
#include <sys/resource.h>
#include "osd.h"

#include "channel.h"
#include "hdplayer.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
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
#include <syslog.h>
extern hd_data_t volatile *hda;
int osd_enabled=1; // on after startup

static inline void ClearOsd();
static inline void FlushOsd();
static void UpdateDirty(int x0, int y0, int x1, int y1);
static bool ClipArea(unsigned int *l, unsigned int *t,unsigned int *r,unsigned int *b);
static void DrawRect(unsigned int l, unsigned int t,unsigned int r,unsigned int b,unsigned int color);
static void DrawRect2(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV);
static void XineStart();
static void XineTile(hdcmd_osd_xine_tile const *tile);
static void XineEnd();
static void XineTitleDraw();
static inline uint32_t AlphaBlend(uint32_t p2, uint32_t p1);

typedef struct xine_tiles_a {
    struct xine_tiles_a *next;
    hdcmd_osd_tile tile;
} xine_tiles_t;
static xine_tiles_t *xine_titles = 0;

typedef struct {
    unsigned int  width;
    unsigned char  pixels[MAX_FONT_HEIGHT][MAX_FONT_WIDTH];
} FontChar;

typedef struct {
    unsigned int     height;
    FontChar chars[256];
} Font;

static Font          fonts_[MAX_FONTS];

typedef struct {
    int width, height;
} CachedImage;

typedef struct {
   int x0, y0;
   int x1, y1;
} dirtyArea;

static dirtyArea dirtyArea_;
static int savedRegion_x0, savedRegion_y0, savedRegion_x1, savedRegion_y1;

static unsigned int palette[256];

static inline void DrawRectIncl(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color);
static void DrawEllipse(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int quadrants);
static void DrawText(int x, int y, const char *s, unsigned int colorFg, unsigned int colorBg, Font const *font,
                     int width, int height, int alignment);
void DrawImage(unsigned int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat);
void DrawCropImage(unsigned int imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend);

#define DOUBLE_BUFFER 1

#define OSD_WIDTH  osd->width 
#define OSD_HEIGHT osd->height
#ifdef DOUBLE_BUFFER
#define OSD_MAXADDR (osd->buffer + osd->width*osd->height*osd->bpp)
#define WITHIN_OSD_BORDERS(arg) (/* (unsigned int*)arg >= (unsigned int*)osd->buffer &&*/ (unsigned int*)arg <= (unsigned int*)(osd->buffer + osd->width*osd->bpp*osd->height))
#else
#define OSD_MAXADDR (osd->data + osd->width*osd->height*osd->bpp)
#define WITHIN_OSD_BORDERS(arg) (/* (unsigned int*)arg >= (unsigned int*)osd->data &&*/ (unsigned int*)arg <= (unsigned int*)(osd->data + osd->width*osd->bpp*osd->height))
#endif
#define OSD_EXTRA_CHECK 1

#define MAX_CACHED_IMAGES_SIZE 2048 * 1024

osd_t *osd;

enum {
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

typedef struct plQueue_ {
    Pointer  *p1;
    Pointer  *p2;
    Pointer  *pBegin;
    Pointer  *pEnd;
    unsigned int      size;
    unsigned int      maxSize;
} plQueue;

plError_t plQueuePut(plQueue *queue, Pointer element) {
    *queue->p2++ = element;
    ++ queue->size;
    if (queue->p2 == queue->pEnd)
    {
        queue->p2 = queue->pBegin;
    }
    return PL_OK;
} // plQueuePut

plError_t plQueueGet(plQueue *queue, Pointer *element) {
    *element = *queue->p1++;
    -- queue->size;
    if (queue->p1 == queue->pEnd)
        queue->p1 = queue->pBegin;
    return PL_OK;
} // plQueueGet

plError_t plQueueInit(plQueue *queue, unsigned int maxSize) {
    Pointer *p = (Pointer)malloc( maxSize * sizeof(Pointer));
    if(!p) {
       printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
       abort();
    }  // if
    queue->p1      = p;
    queue->p2      = p;
    queue->pBegin  = p;
    queue->pEnd    = p + maxSize;
    queue->size    = 0;
    queue->maxSize = maxSize;
    return PL_OK;
} // plQueueInit

plError_t plQueueDestroy(plQueue *queue) {
    free(queue->pBegin);
    return PL_OK;
} // plQueueDestroy

static plQueue       cachedImagesQueue_;
static int           cachedImagesSize_ = 0;

/*-------------------------------------------------------------------------*/
osd_t *new_osd(hd_channel_t *channel) {
    osd = (osd_t*)malloc(sizeof(osd_t));
    if(!osd) {
        printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
        abort();
    } // if
    osd->fd = open("/dev/fb0", O_RDWR|O_NDELAY);
    if(osd->fd==-1)
        printf("couldn't open /dev/fb0, error: %s\n", strerror(errno));
    assert(osd->fd!=-1);
    struct fb_var_screeninfo screeninfo;
    ioctl(osd->fd, FBIOGET_VSCREENINFO, &screeninfo);

    osd->bpp = screeninfo.bits_per_pixel/8; 
    osd->width = screeninfo.xres;
    osd->height = screeninfo.yres;
    osd->xine_osd = 0;
    osd->vdr_osd  = 0;
    osd->data = (unsigned char*) mmap(0, osd->bpp* osd->width * osd->height, PROT_READ|PROT_WRITE, MAP_SHARED, osd->fd, 0);

    assert(osd->data!=(void*)-1);
    /* clear osd */
    /* RC: don't clear osd because of start logo
    int i;
    for(i=0; i<osd->bpp*osd->width*osd->height; i++)
        osd->data[i] = 0;
    */
#ifdef DOUBLE_BUFFER
    osd->buffer = (unsigned char*) malloc(osd->bpp * osd->width * osd->height);
    osd->savedRegion = (unsigned char*) malloc(osd->bpp * osd->width * osd->height);
    if(!osd->buffer) {
        printf("ERROR: couldn't malloc in %s: %i\n", __FILE__, __LINE__);
        abort();
    } // if
#else
    osd->buffer = osd->data;
#endif
    printf("FB: xres: %i, yres: %i, bpp: %i, osd->data: %p\n", osd->width, osd->height, osd->bpp, osd->data);
    plQueueInit(&cachedImagesQueue_, HDOSD_MAX_IMAGES);
    return osd;
} // new_osd

/*-------------------------------------------------------------------------*/
void delete_osd(osd_t *osd) {
    close(osd->fd);
#ifdef DOUBLE_BUFFER
    free(osd->buffer);
#endif
    free(osd->savedRegion);
    free(osd);
} // delete_osd

void* thread_loop(void *para) {
    printf("osd thread running\n");
    int result;
    setpriority(PRIO_PROCESS, 0, 5);
    while(1) {
        result = run_osd();
        if(result < 0) result = run_xine_osd();
        if(result < 0) usleep(1*1000);
    } // while
    printf("osd thread ended!\n");
    return NULL;
} // thread_loop

int start_thread() {
    pthread_t thread;
    if (!pthread_create(&thread, NULL,thread_loop,(void*)osd)){
        fprintf(stderr,"pthread_detach!\n");
        pthread_detach(thread);
    } // if
    return 1;
} // start_thread

int put_packet_osd(void const *rcvBuffer) {
    switch(*(char*)(rcvBuffer + sizeof(hd_packet_es_data_t) )){
        case HDCMD_OSD_PALETTE: {
            //printf("HDCMD_OSD_PALETTE\n");
            hdcmd_osd_palette_t *bco = (hdcmd_osd_palette_t*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            int n = bco->count;
            if (n > 256)
                break;
            memcpy(palette, &bco->palette, n * sizeof(int));
            break;
        } // HDCMD_OSD_PALETTE
        case HDCMD_OSD_DRAW8_OVERLAY: {
            //printf("HDCMD_OSD_DRAW8_OVERLAY\n");
            hdcmd_osd_draw8_t *bco=(hdcmd_osd_draw8_t*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            unsigned char const *srcData = (unsigned char const *)(bco->data);
            unsigned char const *xs;
            static int qx, qy, xt1, yt1, x, y, w, h, *px, line, row;
            static unsigned int pxs;

            x=bco->x;
            y=bco->y;
            w=bco->w;
            h=bco->h;
            UpdateDirty(x, y, x+w, y+h);

            xt1 = x * OSD_WIDTH;
            yt1 = y * OSD_HEIGHT;
            qy = 0;
            for(line=0;line<h; y++) {
                px=(int*)(osd->buffer + OSD_WIDTH * y * osd->bpp  + x * osd->bpp );
                xs = srcData + line * w;
                qx = xt1 % OSD_WIDTH;
#if 0 //def OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(px+w-1))
#endif
                for(row=0;row<w; px++) {
                    pxs = palette[*xs];
                    if(pxs&0x00FFFFFF && pxs!=0x01ffffff){
                        *px = AlphaBlend(pxs, *px);
                    } else {
                        *px = AlphaBlend(*px, pxs);
                    } // if
                    if (qx >= 0) {
                        xs++;
                        row++;
                        qx = 0;
                    } else
                        qx += OSD_WIDTH;
                } // for
                if (qy >= 0) {
                    ++line;
                    qy = 0;
                } else
                    qy += OSD_HEIGHT;
            } // for
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            break;
        } // HDCMD_OSD_DRAW8_OVERLAY
        case HDCMD_OSD_DRAW8: {
            //printf("HDCMD_OSD_DRAW8\n");
            hdcmd_osd_draw8_t *bco=(hdcmd_osd_draw8_t*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            unsigned char const *srcData = (unsigned char const *)(bco->data);
            unsigned char const *xs;
            int qx, qy, xt1, yt1, xt, yt, vfx, vfy, vfw, vfh, x, y, w, h, m, *px, n;

            x=bco->x;
            y=bco->y;
            w=bco->w;
            h=bco->h;
            UpdateDirty(x, y, x+w, y+h);

            if (bco->scale) {
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
                for(m=0;m<h;) {
                    px=(int*)(osd->buffer + OSD_WIDTH * yt * osd->bpp  + xt * osd->bpp );
                    xs = srcData + m * w;
                    qx = xt1 % OSD_WIDTH;
#ifdef OSD_EXTRA_CHECK
                    if(WITHIN_OSD_BORDERS(px+w-1))
#endif
                    for(n=0;n<w;) {
                        // if ((o&3)==0)
                        //         val=*bm++;
                        *px++=palette[*xs];
                        qx += OSD_WIDTH;
                        while (qx >= OSD_WIDTH) {
                            ++xs;
                            ++n;
                            qx -= vfw;
                        } // while
                        // __SYSLOG("px = %08X\n", px);
                        // val>>=8;
                        // o++;
                    } // for
                    ++yt;
                    qy += OSD_HEIGHT;
                    while (qy >= OSD_HEIGHT) {
                        ++m;
                        qy -= vfh;
                    } // while
                } // for
            } else {
                unsigned int *bm = (unsigned int*)bco->data;
                int o, val = 0;

                if (w>OSD_WIDTH || x>OSD_WIDTH || y>OSD_HEIGHT || h>OSD_HEIGHT)
                    break;
                o=0;
                for (m=0;m<h;m++) {
                    px=(int*)(osd->buffer +OSD_WIDTH*(y+m)*osd->bpp+x*osd->bpp);
#if OSD_EXTRA_CHECK
                    if(WITHIN_OSD_BORDERS(px+w-1))
#endif
                    for(n=0;n<w;n++) {
                        if ((o&3)==0)
                            val=*bm++;
                        *px++=palette[val&255];
                        val>>=8;
                        o++;
                    } // for
                } // for
            } // if
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            break;
        } // HDCMD_OSD_DRAW8
        case HDCMD_OSD_CLEAR: {
            //printf("HDCMD_OSD_CLEAR\n");
            osd->vdr_osd = 0;
            if(osd->xine_osd)
                XineEnd();
            else
                ClearOsd();
            break;
        } // HDCMD_OSD_CLEAR
        case HDCMD_OSD_DRAW_RECT: {
            hdcmd_osd_draw_rect *bco = (hdcmd_osd_draw_rect*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));

            UpdateDirty(bco->l, bco->t, bco->r, bco->b);
            DrawRect(bco->l, bco->t, bco->r, bco->b, bco->color);
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            //  printf("HDCMD_OSD_DRAW_RECT - l: %i, t: %i, r: %i, b: %i, color: %x\n", bco->l, bco->t, bco->r, bco->b, bco->color);
            break; 
        } // HDCMD_OSD_DRAW_RECT
        case HDCMD_OSD_DRAW_RECT2: {
            hdcmd_osd_draw_rect2 *bco = (hdcmd_osd_draw_rect2*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));

            UpdateDirty(bco->l, bco->t, bco->r, bco->b);
            DrawRect2(bco->l, bco->t, bco->r, bco->b, bco->color, bco->alphaGradH, bco->alphaGradV, bco->alphaGradStepH, bco->alphaGradStepV);
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            //  printf("HDCMD_OSD_DRAW_RECT - l: %i, t: %i, r: %i, b: %i, color: %x\n", bco->l, bco->t, bco->r, bco->b, bco->color);
            break; 
        } // HDCMD_OSD_DRAW_RECT2
        case HDCMD_OSD_DRAW_ELLIPSE: {
            //printf("HDCMD_OSD_DRAW_ELLIPSE\n");
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            hdcmd_osd_draw_ellipse *bco = (hdcmd_osd_draw_ellipse*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            UpdateDirty(bco->l, bco->t, bco->r, bco->b);
            DrawEllipse(bco->l, bco->t, bco->r, bco->b, bco->color, bco->quadrants);
            break;
        } // HDCMD_OSD_DRAW_ELLIPSE
        case HDCMD_OSD_CACHE_FONT: {
            //printf("HDCMD_OSD_CACHE_FONT\n");

            int c, x, y;
            hdcmd_osd_cache_font *bco = (hdcmd_osd_cache_font*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            unsigned int *fontData = (unsigned int *)(bco + 1);
            unsigned int const fontIndex = bco->fontIndex;

            unsigned int const fontHeight = fontData[1];
            Font *font = &fonts_[fontIndex];

            font->height = fontHeight;
            for (c = 0; c < 256; ++c) {
                FontChar *fontChar = &font->chars[c];
                unsigned char *pix = &fontChar->pixels[0][0];

                fontChar->width = *fontData;
                fontData += 2;
                for (y = fontHeight; y; --y) {
                    unsigned int line = *fontData++;
                    line <<= (32 - fontChar->width);
                    for (x = MAX_FONT_WIDTH; x; --x) {
                        *pix++ = ((int)line) < 0 ? 0x00 : 0xFF;
                        line <<= 1;
                    } // for
                } // for
            } // for
            hda->osd_cached_fonts[fontIndex] = 1;
            break;
        } // HDCMD_OSD_CACHE_FONT
        case HDCMD_OSD_DRAW_TEXT: {
            hdcmd_osd_draw_text *bco = (hdcmd_osd_draw_text*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            char const *s = (char const *)(bco + 1);
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            //printf("HDCMD_OSD_DRAW_TEXT %d, %s - fgcolor: %x, bgcolor: %x\n", bco->fontIndex, s, bco->colorFg, bco->colorBg);
            UpdateDirty(bco->x, bco->y, bco->x+bco->width, bco->y+bco->height);
            DrawText(bco->x, bco->y, s, bco->colorFg, bco->colorBg, &fonts_[bco->fontIndex], bco->width, bco->height, bco->alignment);
            break;
        } // HDCMD_OSD_DRAW_TEXT
        case HDCMD_OSD_CACHE_IMAGE: {
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

            if (hda->osd_cached_images[imageId]) {
                while (true) {
                    unsigned int         cachedImageId;
                    CachedImage *cachedImage;

                    plQueueGet(&cachedImagesQueue_, (void *)&cachedImageId);
                    if (cachedImageId == imageId) {
                        cachedImage = (CachedImage *)hda->osd_cached_images[cachedImageId];
                        cachedImagesSize_ -= cachedImage->width * cachedImage->height * sizeof(unsigned int);

                        free(cachedImage);
                        hda->osd_cached_images[cachedImageId] = 0;
                        break;
                    } else {
                        plQueuePut(&cachedImagesQueue_, (void *)cachedImageId);
                    } // if
                } // while
            } // if

            while (cachedImagesSize_ + size > MAX_CACHED_IMAGES_SIZE) {
                unsigned int         cachedImageId;
                CachedImage *cachedImage;

                // __SYSLOG("Uncacheing Image\n");
                /*if (plQueueIsEmpty(&cachedImagesQueue_)) {
                    //__SYSLOG("Image %d too big.\n", imageId);
                    return 0;
                }*/

                plQueueGet(&cachedImagesQueue_, (void *)&cachedImageId);
                cachedImage = (CachedImage *)hda->osd_cached_images[cachedImageId];
                cachedImagesSize_ -= cachedImage->width * cachedImage->height * sizeof(unsigned int);

                free(cachedImage);
                hda->osd_cached_images[cachedImageId] = 0;
            } // while
            cachedImage = (CachedImage *)malloc(sizeof(CachedImage) + size);
            if (!cachedImage)
                printf("Error caching image: out of memory!\n");
            cachedImage->width = width;
            cachedImage->height = height;
            tgtPixels = (unsigned int *)(cachedImage + 1);

            for (n = 0; n < pixelCount; ++n) {
                unsigned int pix = (*imagePixels++);
                *tgtPixels++ = pix;
            } // for

            hda->osd_cached_images[imageId] = (int)cachedImage;
            cachedImagesSize_ += size;
            plQueuePut(&cachedImagesQueue_, (void*)imageId);
            break;
        } // HDCMD_OSD_CACHE_IMAGE
        case HDCMD_OSD_DRAW_IMAGE: {
            hdcmd_osd_draw_image *bco = (hdcmd_osd_draw_image *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t) );
            //printf("HDCMD_OSD_DRAW_IMAGE %x\n",bco->image_id);
            UpdateDirty(bco->x, bco->y, bco->x + ((CachedImage *)hda->osd_cached_images[bco->image_id])->width, bco->y + ((CachedImage *)hda->osd_cached_images[bco->image_id])->height);
            DrawImage(bco->image_id, bco->x, bco->y, bco->blend, bco->hor_repeat, bco->vert_repeat);
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            break;
        } // HDCMD_OSD_DRAW_IMAGE
        case HDCMD_OSD_DRAW_CROP_IMAGE: {
            hdcmd_osd_draw_crop_image *bco = (hdcmd_osd_draw_crop_image *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t) );
            //printf("HDCMD_OSD_DRAW_IMAGE %x\n",bco->image_id);
            UpdateDirty(bco->x0, bco->y0, bco->x1, bco->y1);
            DrawCropImage(bco->image_id, bco->x, bco->y, bco->x0, bco->y0, bco->x1, bco->y1, bco->blend);
            hda->osd_hidden = 0;
            osd->vdr_osd = 1;
            break;
        } // HDCMD_OSD_DRAW_CROP_IMAGE
        case HDCMD_OSD_FLUSH: {
            hdcmd_osd_flush const *bco = (hdcmd_osd_flush const *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
           //printf("HDCMD_OSD_FLUSH\n");
            hda->osd_flush_count = bco->flush_count;
            osd->vdr_osd = 1;
            FlushOsd();
            break;
        } // HDCMD_OSD_FLUSH
        case HDCMD_OSD_SAVE_REGION: {
            hdcmd_osd_clear_t const *bco = (hdcmd_osd_clear_t const *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t));
            savedRegion_x0 = bco->x; savedRegion_y0 = bco->y; savedRegion_x1 = bco->x1; savedRegion_y1 = bco->y1;
            int lines = bco->y1 - bco->y;    
            int pixels = bco->x1 - bco->x;
            int i;
            for(i = 0; i < lines; i++)
                memcpy(osd->savedRegion + (i+bco->y)*osd->width*osd->bpp + bco->x*osd->bpp, osd->buffer + (i+bco->y)*osd->width*osd->bpp + bco->x*osd->bpp, pixels*osd->bpp);
            break;
        } // HDCMD_OSD_SAVE_REGION
        case HDCMD_OSD_RESTORE_REGION: {
            int x1 = savedRegion_x0; int y1 = savedRegion_y0; int x2 = savedRegion_x1; int y2 = savedRegion_y1;
            int lines = y2 - y1;    
            int pixels = x2 - x1;
            int i;
            for(i = 0; i < lines; i++)
                memcpy(osd->buffer + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, osd->savedRegion + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, pixels*osd->bpp);
            break;
        } // HDCMD_OSD_RESTORE_REGION
        case HDCMD_OSD_XINE_START:
            XineStart();
            break;
        case HDCMD_OSD_XINE_TILE:
            XineTile((hdcmd_osd_xine_tile const *)((void*)rcvBuffer + sizeof(hd_packet_es_data_t)));
            break;
        case HDCMD_OSD_XINE_END:
            XineEnd();
            break;
        default:
            printf("ERROR: unkown OSD Command!: %i\n", *(char*)((void*)rcvBuffer + sizeof(hd_packet_es_data_t) ));
    } // switch
    return 1;
} // put_packet_osd

/*-------------------------------------------------------------------------*/
int run_osd() {
    hd_packet_header_t const *packet_header;
    void *rcvBuffer;
    int packet_size;

    if (!hd_channel_read_start(ch_osd, &rcvBuffer, &packet_size, 0))
        return -1;
    packet_header = (hd_packet_header_t *)rcvBuffer;
    //printf("got_osd_packet(%d) [%d]\n", packet_header->packet_size, packet_header->seq_nr);
    switch (packet_header->type) {
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
    } // switch
    hd_channel_read_finish(ch_osd);
    return 1;
} // run_osd

int run_xine_osd() {
    hd_packet_header_t const *packet_header;
    void *rcvBuffer;
    int packet_size;

    if (!hd_channel_read_start(ch_xine_osd, &rcvBuffer, &packet_size, 0))
        return -1;
    packet_header = (hd_packet_header_t *)rcvBuffer;
    //printf("got_xine_osd_packet(%d) [%d]\n", packet_header->packet_size, packet_header->seq_nr);
    switch (packet_header->type) {
        case HD_PACKET_OSD:
            //printf("INFO: xine_osd HD_PACKET_OSD\n");
            put_packet_osd((hd_packet_es_data_t const *)packet_header);
            break;
        case HD_PACKET_ES_DATA:
            printf("INFO: xine_osd HD_PACKET_ES_DATA\n");
            //put_packet_es_data(pl, (hd_packet_es_data_t const *)packet_header);
            break;
        default:
            printf("ERROR: Unknown packet type: %d\n", packet_header->type);
    } // switch
    hd_channel_read_finish(ch_xine_osd);
    return 1;
} // run_xine_osd

static uint32_t last_p1, last_p2, last_res;

static inline uint32_t AlphaBlend(uint32_t p2, uint32_t p1) {
    if(p1 == last_p1 && p2 == last_p2) {
        return last_res;
    } else {
        last_p1 = p1;
        last_p2 = p2;
    } // if

    uint8_t const xa1 =  (p1 >> 24);
    uint8_t const xa2 = (p2 >> 24);
//    printf("blend: fg: %#x bg: %#x\n", p2, p1);
    if (xa2==255 || !xa1) {
        last_res = p2;
        return p2;
    } else if (!xa2) {
        last_res = p1;
        return p1;
    } else {
        uint8_t const a1 = 255 - xa1;
        uint8_t const a2 = 255 - xa2;

        uint8_t const r2 = (p2 >> 16);
        uint8_t const g2 = (p2 >> 8);
        uint8_t const b2 = p2;

        uint8_t const r1 = (p1 >> 16);
        uint8_t const g1 = (p1 >> 8);
        uint8_t const b1 = p1;

        uint32_t const al1_al2 = a1 * a2;
        uint32_t const al2     = a2 * 255;

        uint32_t const al1_al2_inv = 255*255 - al1_al2; // > 0

        uint32_t const c1 = al2 - al1_al2;
        uint32_t const c2 = 255*255 - al2;

        uint8_t const a = al1_al2 / 255;
        uint8_t const r = (c1 * r1 + c2 * r2) / al1_al2_inv;
        uint8_t const g = (c1 * g1 + c2 * g2) / al1_al2_inv;
        uint8_t const b = (c1 * b1 + c2 * b2) / al1_al2_inv;
        uint32_t res = last_res = ((255 - a) << 24) | (r << 16) | (g << 8) | b;
        return res;
    } // if
} // AlphaBlend

static bool inline ClipArea(unsigned int *l,unsigned  int *t,unsigned  int *r,unsigned  int *b) {
    if (*r >= OSD_WIDTH)
        *r = OSD_WIDTH;
    if (*b >= OSD_HEIGHT)
        *b = OSD_HEIGHT;
    return *l < *r && *t < *b;
} // ClipArea

static inline void ClearOsd() {
    if(!hda->osd_hidden && !(hda->osd_dont_touch&1)){ /* only reset buffer & osd if osd wasn't hidden yet */
        UpdateDirty(0,0,OSD_WIDTH-1, OSD_HEIGHT-1);
        memset(osd->buffer, 0x00, osd->width*osd->height*osd->bpp);
        memset(osd->data, 0x00, osd->width*osd->height*osd->bpp);
    } // if
    hda->osd_hidden = 1;
    FlushOsd();
} // ClearOsd

static inline void FlushOsd() {
    int pid = 2;

    if (hda->osd_hidden && !(hda->osd_dont_touch&1)) {
//DL Don´t hide fb or subtitle will produce hickups in video layer if osd gets enabled
//        ioctl(osd->fd, DSPC_HIDE, &pid);
        osd_enabled=0;
    } else {
#ifdef DOUBLE_BUFFER
        if(!(hda->osd_dont_touch&1)) {
            //memcpy(osd->data, osd->buffer, osd->width*osd->height*osd->bpp);
            int lines = dirtyArea_.y1 - dirtyArea_.y0;
            int pixels = dirtyArea_.x1 - dirtyArea_.x0;
            int i;
            //printf("dirty: %i %i %i %i lines: %i pixels: %i\n", dirtyArea_.x0, dirtyArea_.y0, dirtyArea_.x1, dirtyArea_.y1, lines, pixels);
            for(i = 0; i < lines; i++)
                memcpy(osd->data + (i+dirtyArea_.y0)*OSD_WIDTH*osd->bpp + dirtyArea_.x0*osd->bpp, osd->buffer + (i+dirtyArea_.y0)*OSD_WIDTH*osd->bpp + dirtyArea_.x0*osd->bpp, pixels*osd->bpp);
            dirtyArea_.x0 = OSD_WIDTH;
            dirtyArea_.y0 = OSD_HEIGHT;
            dirtyArea_.x1 = -1;
            dirtyArea_.y1 = -1;
        } // if
#endif
        if (!osd_enabled) {
            ioctl(osd->fd, DSPC_SHOW, &pid);
            osd_enabled=1;
        } // if
    } // if
} // FlushOsd


static unsigned int draw_linebuf[1024]={0};
static unsigned int draw_color=0;

static void DrawRect(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color) {
    if (ClipArea(&l, &t, &r, &b)) {
        unsigned int m, n;
        unsigned int width = r - l;
        unsigned int height = b - t;
        unsigned int *pixels = (int*)(osd->buffer + OSD_WIDTH * t *osd->bpp  + l*osd->bpp);
        if (draw_color!=color) {
            for(n=0;n<1024;n++)
                draw_linebuf[n]=color;
            draw_color=color;
        } // if
        for (m = height; m; --m) {
#if OSD_EXTRA_CHECK
            if(WITHIN_OSD_BORDERS(pixels+width*sizeof(int)))
#endif
            memcpy(pixels,draw_linebuf,width*sizeof(int));
            pixels+=OSD_WIDTH;
        } // for
    } // if
} // DrawRect

static void DrawRect2(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV) {
    if (ClipArea(&l, &t, &r, &b)) {
        unsigned int m, n;
        unsigned int width = r - l;
        unsigned int height = b - t;
        unsigned int mod = OSD_WIDTH - width;
        unsigned int *pixels = (int*)(osd->buffer + OSD_WIDTH * t *osd->bpp  + l*osd->bpp);
        unsigned int curAlphaH = 0, curAlphaV = 0;

        for(n=0; n<height; n++){
            if(alphaGradStepV && n%alphaGradStepV==0)
                curAlphaV++;
#if OSD_EXTRA_CHECK
            if(WITHIN_OSD_BORDERS(pixels+width-1))
#endif
            for (m=0; m<width; m++){
                if(alphaGradStepH && m%alphaGradStepH==0)
                    curAlphaH++;
                *pixels++ = ( (color&0x00FFFFFF) | (((color>>24) + ((curAlphaH*alphaGradH + curAlphaV*alphaGradV)))<<24) );
            } // for
            pixels += mod;
            curAlphaH = 0;
        } // for
    } // if
} // DrawRect2

static int StringWidth(const char *s, Font const *font) {
    unsigned int c;
    int width = 0;
    const unsigned char *p = (const unsigned char *)s;
    while ((c = *p++)) {
        if(c<0 || c>= 255) {
          printf("ERROR: crappy char: %x\n", c);
        } else {
          FontChar const *fontChar = &font->chars[c];
          width += fontChar->width;
        } // if
    } // while
    return width;
} // StringWidth

static void DrawText(int x, int y, const char *s, unsigned int colorFg, unsigned int colorBg, Font const *font, int width, int height, int alignment) {
    if(!font){
        printf("NO font!\n");
        return;
    } // if
    int w = StringWidth(s, font);
    int h = font->height;
    int limit = 0;
    int x0 = x;
    if (width || height) {
        int cw = width ? width : w;
        int ch = height ? height : h;
        /*if (!intersects(x, y, x + cw - 1, y + ch - 1))
           return;*/
        if ((colorBg >> 24) != 0x00)
            DrawRect(x, y, x + cw, y + ch, colorBg);
        limit = x + cw;
        if (width) {
            if ((alignment & taLeft) != 0) {
            } else if ((alignment & taRight) != 0) {
                if (w < width)
                    x += width - w;
            } else { // taCentered
                if (w < width)
                    x += (width - w) / 2;
            } // if
        } // if
        if (height) {
            if ((alignment & taTop) != 0) {
            } else if ((alignment & taBottom) != 0) {
                if (h < height)
                    y += height - h;
            } else { // taCentered
                if (h < height)
                    y += (height - h) / 2;
            } // if
        } // if
    } else if (/*!intersects(x, y, x + w - 1, y + h - 1)*/ false)
        return;

    while (*s) {
        unsigned char c = *s++; // Caution, char is signed.
        //printf("char: %c - %x\n", c, c);
        FontChar const *fontChar = &font->chars[c];
        int charWidth = fontChar->width;
        if (limit && (int)(x + charWidth) > limit)
            break; // we don't draw partial characters
        if (x - x0 >= OSD_WIDTH)
            break;
        // TRACE1("x = %d\n", x);
        if ((int)(x + charWidth) > 0) {// char is not completely outside the left screen border
            int row;
            for (row = 0; row < h; ++row) {
                unsigned char const *fontPixels = fontChar->pixels[row];
                int *targetPixels = (int*)(osd->buffer + (y + row) * osd->bpp * OSD_WIDTH  + x*osd->bpp);
                int col;
                if (colorBg) {
            // GA: Simplification without AlphaBlend()
#if OSD_EXTRA_CHECK
                     if(WITHIN_OSD_BORDERS(targetPixels+charWidth))
#endif
                     for (col = charWidth; col-- > 0; ) {
                         unsigned char pix=*fontPixels++;
                         if (!pix)
                             *targetPixels=colorFg;
                         targetPixels++;
                     } // for
                } else {
#if OSD_EXTRA_CHECK
                    if(WITHIN_OSD_BORDERS(targetPixels+charWidth))
#endif
                    for (col = charWidth; col-- > 0; ) {
                        unsigned int const bgColor = *targetPixels;
                        unsigned int const fontColor = (colorFg & 0x00FFFFFF) | ((255 - *fontPixels++) << 24);
#ifdef SEC_CHECK
                        if(targetPixels > osd->buffer + osd->width*osd->height*osd->bpp)
                            printf("OVERFLOW!\n");
                        else
#endif
                        *targetPixels++ = AlphaBlend(fontColor, bgColor);
                    } // for
                } // if
            } // for
        } // if
        x += charWidth;
    } // while
} // DrawText

static inline void DrawRectIncl(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color) {
    DrawRect(l, t, r + 1, b + 1, color);
} // DrawRectIncl

static void DrawEllipse(unsigned int l, unsigned int t, unsigned int r, unsigned int b, unsigned int color, int quadrants) {
    int x1 = l;
    int y1 = t;
    int x2 = r - 1;
    int y2 = b - 1;
    int rx, ry, cx, cy;
    int TwoASquare, TwoBSquare, x, y, XChange, YChange, EllipseError, StoppingX, StoppingY;

    if (!ClipArea(&l, &t, &r, &b))
        return;
    // Algorithm based on http://homepage.smc.edu/kennedy_john/BELIPSE.PDF
    rx = x2 - x1;
    ry = y2 - y1;
    cx = (x1 + x2) / 2;
    cy = (y1 + y2) / 2;
    switch (abs(quadrants)) {
        case 0: rx /= 2; ry /= 2; break;
        case 1: cx = x1; cy = y2; break;
        case 2: cx = x2; cy = y2; break;
        case 3: cx = x2; cy = y1; break;
        case 4: cx = x1; cy = y1; break;
        case 5: cx = x1;          ry /= 2; break;
        case 6:          cy = y2; rx /= 2; break;
        case 7: cx = x2;          ry /= 2; break;
        case 8:          cy = y1; rx /= 2; break;
    } // switch
    TwoASquare = 2 * rx * rx;
    TwoBSquare = 2 * ry * ry;
    x = rx;
    y = 0;
    XChange = ry * ry * (1 - 2 * rx);
    YChange = rx * rx;
    EllipseError = 0;
    StoppingX = TwoBSquare * rx;
    StoppingY = 0;
    while (StoppingX >= StoppingY) {
        switch (quadrants) {
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
        } // switch
        ++ y;
        StoppingY += TwoASquare;
        EllipseError += YChange;
        YChange += TwoASquare;
        if (2 * EllipseError + XChange > 0) {
            x--;
            StoppingX -= TwoBSquare;
            EllipseError += XChange;
            XChange += TwoBSquare;
        } // if
    } // while
    x = 0;
    y = ry;
    XChange = ry * ry;
    YChange = rx * rx * (1 - 2 * ry);
    EllipseError = 0;
    StoppingX = 0;
    StoppingY = TwoASquare * ry;
    while (StoppingX <= StoppingY) {
        switch (quadrants) {
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
        } // switch
        x++;
        StoppingX += TwoBSquare;
        EllipseError += XChange;
        XChange += TwoBSquare;
        if (2 * EllipseError + YChange > 0) {
            y--;
            StoppingY -= TwoASquare;
            EllipseError += YChange;
            YChange += TwoASquare;
        } // if
    } // while
} // DrawEllipse

void UpdateDirty(int x0, int y0, int x1, int y1) {
#ifdef DOUBLE_BUFFER
    if(x0 > OSD_WIDTH)  x0 = OSD_WIDTH;
    if(x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
    if(y0 > OSD_HEIGHT) y0 = OSD_HEIGHT;
    if(y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;

    if (dirtyArea_.x0 > x0)  dirtyArea_.x0 = x0;
    if (dirtyArea_.y0 > y0)  dirtyArea_.y0 = y0;
    if (dirtyArea_.x1 < x1)  dirtyArea_.x1 = x1;
    if (dirtyArea_.y1 < y1)  dirtyArea_.y1 = y1;
#endif
} // UpdateDirty

void DrawImage(unsigned int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat) {
    CachedImage const *img = (CachedImage const *)hda->osd_cached_images[imageId];
    int m, n, h, v;
    int width;    
    int height;    
    unsigned int const *srcPixels;

    if (img) {
        width = img->width;    
        height = img->height;    

        if (blend) {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = (unsigned int const *)(img + 1);
                for (n = height; n > 0; --n) {
                    unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
                    for (h = horRepeat; h > 0; --h) {
                        unsigned int const *src = srcPixels;
#if OSD_EXTRA_CHECK
                        if(WITHIN_OSD_BORDERS(tgtPixels+width))
#endif
                        for (m = width; m > 0; --m) {
                            *tgtPixels = AlphaBlend((*src++), (*tgtPixels) );
                            ++ tgtPixels;
                        } // for
                    } // for
                    srcPixels += width;
                } // for
            } // for
        } else {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = (unsigned int const *)(img + 1);
                for (n = height; n > 0; --n) {
                    unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
                    for (h = horRepeat; h > 0; --h) {
                        unsigned int const *src = srcPixels;
#if OSD_EXTRA_CHECK
                        if(WITHIN_OSD_BORDERS(tgtPixels+width*(sizeof(int))))
#endif
                        memcpy(tgtPixels, src, width*sizeof(int));
                        tgtPixels+=width;
                        src+=width;
                    } // for
                    srcPixels += width;
                } // for
            } // for
        } // if
    } else {
        printf("Image %d not cached.\n", imageId);
    } // if
} // DrawImage

void DrawCropImage(unsigned int imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend) {
    CachedImage const *img = (CachedImage const *)hda->osd_cached_images[imageId];
    int m, n, h, v;
    int width;
    int height;
    unsigned int const *srcPixels;
    int vertRepeat = 1;
    int horRepeat = 1;

    if (img) {
        width = x1-x0;
        height = y1-y0;
        if(img->height)
            vertRepeat = (y1-y0)/img->height;
        if(vertRepeat < 1) vertRepeat = 1;
        if(img->width)
            horRepeat = (x1-x0)/img->width;
        if(horRepeat < 1) horRepeat = 1;
        if (blend) {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = (unsigned int const *)(img + 1) + img->width*(y0-y) + (x0-x);
                for (n = height; n > 0; --n) {
                    unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y0++ + x0*osd->bpp);
                    for (h = horRepeat; h > 0; --h) {
                        unsigned int const *src = srcPixels;
                        for (m = width; m > 0; --m) {
                            *tgtPixels = AlphaBlend((*src++), (*tgtPixels) );
                            ++tgtPixels;
                        } // for
                    } // for
                    srcPixels += img->width;
                } // for
            } // for
        } else {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = (unsigned int const *)(img + 1) + img->width*(y0-y) + (x0-x);
                for (n = height; n > 0; --n) {
                    unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y0++ + x0*osd->bpp);
                    for (h = horRepeat; h > 0; --h) {
                        unsigned int const *src = srcPixels;
                        memcpy(tgtPixels, src, img->width*sizeof(int));
                        tgtPixels += img->width;
                    } // for
                    srcPixels += img->width;
                } // if
            } // for
        } // if
    } else {
        printf("Image %d not cached.\n", imageId);
    }
} // DrawCropImage

//*****************************************************************************/
//*** Xine part
//*****************************************************************************/

#define SCALE_SHIFT   16
#define SCALE_FACTOR      (1<<SCALE_SHIFT)
#define INT_TO_SCALED(i)  ((i)  << SCALE_SHIFT)
#define SCALED_TO_INT(sc) ((sc) >> SCALE_SHIFT)

typedef struct {         /* CLUT == Color LookUp Table */
    uint8_t cb;
    uint8_t cr;
    uint8_t y;
    uint8_t foo;
} /*__attribute__ ((packed))*/ clut_t;

static hdcmd_osd_rle_elem * rle_img_advance_line(hdcmd_osd_rle_elem *rle, hdcmd_osd_rle_elem *rle_limit, int w) {
    int x;
    for (x = 0; x < w && rle < rle_limit; ) {
        x += rle->len;
        rle++;
    } // for
    return rle;
} // rle_img_advance_line

#define BLEND_BYTE(dst, src, o) (((((src)-(dst))*(o*0x1111+1))>>16)+(dst))

static void mem_set32(uint8_t *mem, const uint8_t *src, uint8_t o, int len) {
    uint8_t *limit = mem + len*4;
    while (mem < limit) {
        *mem = src[0];
        mem++;
        *mem = src[1];
        mem++;
        *mem = src[2];
        mem++;
        *mem = 0xFF-BLEND_BYTE(0xFF, src[3], o);
        mem++;
    } // while
} // mem_set32

#define MAX_OSD_WITDH  720
#define MAX_OSD_HEIGHT 576
void _x_blend_argb32 (uint8_t * img, hdcmd_osd_overlay * img_overl,
                      int img_width, int img_height,
                      int offset_x, int offset_y,
                      int dst_width, int dst_height) {
    int src_width = img_overl->width;
    int src_height = img_overl->height;
    hdcmd_osd_rle_elem *rle = img_overl->rle;
    hdcmd_osd_rle_elem *rle_limit = rle + img_overl->num_rle;
    int x_off = img_overl->x + offset_x;
    int y_off = img_overl->y + offset_y;
    int x, y, x1_scaled, x2_scaled;
    int dy, dy_step, x_scale;   /* scaled 2**SCALE_SHIFT */
    int hili_right, hili_left;
    int clip_right, clip_left, clip_top;
    uint8_t *img_pix;

    dy_step = INT_TO_SCALED(dst_height) / img_height;
    x_scale = INT_TO_SCALED(img_width)  / dst_width;

    img_pix = img + 4 * (  (y_off * img_height / dst_height) * img_width
            + (x_off * img_width / dst_width));

    /* checks to avoid drawing overlay outside the destination buffer */
    if( (x_off + src_width) <= MAX_OSD_WITDH )
        clip_right = src_width;
    else
        clip_right = MAX_OSD_WITDH - x_off;

    if( x_off >= 0 )
        clip_left = 0;
    else
        clip_left = -x_off;

    if( y_off >= 0 )
        clip_top = 0;
    else
        clip_top = -y_off;

    if( (src_height + y_off) > MAX_OSD_HEIGHT )
      src_height = MAX_OSD_HEIGHT - y_off;

    /* make highlight area fit into clip area */
    if( img_overl->hili_right <= clip_right )
        hili_right = img_overl->hili_right;
    else
        hili_right = clip_right;

    if( img_overl->hili_left >= clip_left )
        hili_left = img_overl->hili_left;
    else
        hili_left = clip_left;

    for (y = dy = 0; y < src_height && rle < rle_limit; ) {
        int mask = !(y < img_overl->hili_top || y >= img_overl->hili_bottom);
        hdcmd_osd_rle_elem *rle_start = rle;

        int rlelen = 0;
        uint8_t clr = 0;

        for (x = x1_scaled = 0; x < src_width;) {
            int rle_bite;
            clut_t *colors;
            uint8_t *trans;
            uint16_t o;
            int clipped = (y < clip_top);

            /* take next element from rle list everytime an element is finished */
            if (rlelen <= 0) {
                if (rle >= rle_limit)
                    break;
                rlelen = rle->len;
                clr = rle->color;
                rle++;
            } // if
            if (!mask) {
                /* above or below highlight area */
                rle_bite = rlelen;
                /* choose palette for surrounding area */
                colors = (clut_t*)img_overl->color;
                trans = img_overl->trans;
            } else {
                /* treat cases where highlight border is inside rle->len pixels */
                if ( x < hili_left ) {
                    /* starts left */
                    if( x + rlelen > hili_left ) {
                        /* ends not left */
                        /* choose the largest "bite" up to palette change */
                        rle_bite = hili_left - x;
                        /* choose palette for surrounding area */
                        colors = (clut_t*)img_overl->color;
                        trans = img_overl->trans;
                    } else {
                        /* ends left */
                        rle_bite = rlelen;
                        /* choose palette for surrounding area */
                        colors = (clut_t*)img_overl->color;
                        trans = img_overl->trans;
                    } // if
                    if( x < clip_left )
                        clipped = 1;
                } else if( x + rlelen > hili_right ) {
                    /* ends right */
                    if( x < hili_right ) {
                        /* starts not right */
                        /* choose the largest "bite" up to palette change */
                        rle_bite = hili_right - x;
                        /* we're in the center area so choose highlight palette */
                        colors = (clut_t*)img_overl->hili_color;
                        trans = img_overl->hili_trans;
                    } else {
                        /* starts right */
                        rle_bite = rlelen;
                        /* choose palette for surrounding area */
                        colors = (clut_t*)img_overl->color;
                        trans = img_overl->trans;
                        if( x + rle_bite >= clip_right )
                            clipped = 1;
                    } // if
                } else {
                    /* starts not left and ends not right */
                    rle_bite = rlelen;
                    /* we're in the center area so choose highlight palette */
                    colors = (clut_t*)img_overl->hili_color;
                    trans = img_overl->hili_trans;
                } // if
            } // if
            x2_scaled = SCALED_TO_INT((x + rle_bite) * x_scale);
            o = trans[clr];
            if (o && !clipped)
                mem_set32(img_pix + x1_scaled*4, (uint8_t *)&colors[clr], o, x2_scaled-x1_scaled);
            x1_scaled = x2_scaled;
            x += rle_bite;
            rlelen -= rle_bite;
        } // for
        img_pix += img_width * 4;
        dy += dy_step;
        if (dy >= INT_TO_SCALED(1)) {
            dy -= INT_TO_SCALED(1);
            ++y;
            while (dy >= INT_TO_SCALED(1)) {
                rle = rle_img_advance_line(rle, rle_limit, src_width);
                dy -= INT_TO_SCALED(1);
                ++y;
            } // while
        } else {
            rle = rle_start;        /* y-scaling, reuse the last rle encoded line */
        } // if
    } // for
} // _x_blend_argb32



void XineStart() {
    //printf("XineStart START\n");
    hda->osd_dont_touch|=2; // Tell fb not to draw due to xine

    xine_tiles_t *pos = xine_titles;
    xine_titles = 0;
    while(pos) {
        xine_tiles_t *next = pos->next;
        if(pos->tile.overlay.rle)
            free(pos->tile.overlay.rle);
        free(pos);
        pos = next;
    } // while
    //printf("XineStart DONE\n");
} // XineStart

void XineTile(hdcmd_osd_xine_tile const *tile) {
    //printf("XineTile START\n");
    xine_tiles_t *pos = xine_titles;
    if(pos) {
        while(pos->next)
            pos = pos->next;
        pos->next = (xine_tiles_t *)malloc(sizeof(xine_tiles_t));
        pos = pos->next;
    } else {
        xine_titles = pos = (xine_tiles_t *)malloc(sizeof(xine_tiles_t));
    } // if
    if(!pos)
        return;
    pos->next = 0;
    memcpy(&pos->tile, &tile->tile, sizeof(hdcmd_osd_tile));
    pos->tile.overlay.rle = (hdcmd_osd_rle_elem *)malloc(pos->tile.overlay.data_size);
    if(!pos->tile.overlay.rle)
        return;
    memcpy(pos->tile.overlay.rle, ((unsigned char *)tile)+sizeof(hdcmd_osd_xine_tile), pos->tile.overlay.data_size);
    //printf("XineTile DONE\n");
} // XineTile

void XineEnd() {
    osd->xine_osd = (xine_titles != 0);
    if(!osd->vdr_osd && !(hda->osd_dont_touch&~2)) {
        hda->osd_dont_touch|=2; // Tell fb block draw for xine ended
        //printf("XineEnd START\n");
        ClearOsd();
        hda->osd_hidden = !osd->xine_osd;
        //printf("hda->osd_hidden %d\n", hda->osd_hidden);
        if(!hda->osd_hidden) {
            XineTitleDraw();
            FlushOsd();
        } // if
    } // if
    hda->osd_dont_touch&=~2; // Tell fb block draw for xine ended
    //printf("XineEnd DONE\n");
} // XineEnd

void XineTitleDraw() {
    //printf("XineTitleDraw\n");
    xine_tiles_t *pos = xine_titles;
    while(pos) {
        _x_blend_argb32(osd->buffer, &pos->tile.overlay, 
                        pos->tile.img_width, pos->tile.img_height, 
                        pos->tile.offset_x, pos->tile.offset_y, 
                        pos->tile.dst_width, pos->tile.dst_height);
        pos = pos->next;
    } // while
    //printf("XineTitleDraw DONE\n");
} // XineTitleDraw
