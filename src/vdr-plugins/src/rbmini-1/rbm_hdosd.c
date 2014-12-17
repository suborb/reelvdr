/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
*                                                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "rbmini.h"

#define DBG_FN   DBG_STDOUT

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string>
#include <png.h>
#include <math.h>

#include "rbm_hdosd.h"
#include "rbm_osd.h"
#include <vdr/font_ft.h>

static bool cRBMHdOsd_FirstRun=true;

#define OSD_MAXADDR (data + osd_width*osd_height*bpp)
#define WITHIN_OSD_BORDERS(arg) ( (unsigned int*)arg >= (unsigned int*)data && (unsigned int*)arg <= (unsigned int*)(data + osd_width*bpp*osd_height))
//#define OSD_EXTRA_CHECK 1

#define MAX_CACHED_IMAGES 256
static bool ImageIdInRange(u_int imageId) {
    if(imageId<MAX_CACHED_IMAGES) return true;
    esyslog("ERROR: ImageId %d not in range (max %d)", imageId, MAX_CACHED_IMAGES);
    return false;
}

static png_structp png_ptr;
static png_infop   info_ptr;
static struct CachedImage *cachedImages_[MAX_CACHED_IMAGES];
static std::string    imagePaths_[MAX_CACHED_IMAGES];
static bool           imageDirty_[MAX_CACHED_IMAGES];

int lastThumbWidth[2];
int lastThumbHeight[2];

static dirtyArea dirtyArea_; /** to be able remember where the OSD has changed, to only need to flush this area */

/* a buffer for the last result and last arguments of DoAlphaBlend() */
static uint32_t last_p1, last_p2, last_res;

static inline uint32_t DoAlphaBlend(uint32_t p2, uint32_t p1) {

    /* Don't do expensive calculations if the last calculation was
     * done with the same arguments. Return the last result instead */
    if(p1 == last_p1 && p2 == last_p2)
        return last_res;
    else {
        last_p1 = p1;
        last_p2 = p2;
    }

    uint8_t const xa1 = (p1 >> 24);
    uint8_t const xa2 = (p2 >> 24);

    /* one color completely opaque, one completely transparent.
     * Return the opaque one */
    if (xa2==255 || !xa1) {
        last_res = p2;
        return p2;
    } else if (!xa2) {
        last_res = p1;
        return p1;
    } else { /* now we really have to calculate */
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
        uint32_t const one     = 255 * 255;

        uint32_t const al1_al2_inv = one - al1_al2; // > 0

        uint32_t const c1 = al2 - al1_al2;
        uint32_t const c2 = one - al2;

        uint8_t const a = al1_al2 / 255;
        uint8_t const r = (c1 * r1 + c2 * r2) / al1_al2_inv;
        uint8_t const g = (c1 * g1 + c2 * g2) / al1_al2_inv;
        uint8_t const b = (c1 * b1 + c2 * b2) / al1_al2_inv;
        uint32_t res = last_res = ((255 - a) << 24) | (r << 16) | (g << 8) | b;
        return res;
    }
}

cRBMHdOsd::cRBMHdOsd(int Left, int Top, uint Level, int fb_fd):cOsd(Left, Top, Level), scaleToVideoPlane(0), osd_fd(fb_fd) {
    /* DBG(DBG_FN,"cRBMHdOsd::cRBMHdOsd"); */
    left = Left;
    top = Top;
    numBitmaps = 0;
    //SetAntiAliasGranularity(1,1);
    if (!Init())
        Done();
    if(cRBMHdOsd_FirstRun) {
        //Clear(); //TB: not needed here?! is called by SetActive() anyway... - takes 100ms....
        cRBMHdOsd_FirstRun = false;
    } // if
    /* DBG(DBG_FN,"cRBMHdOsd::cRBMHdOsd DONE"); */
} // cRBMHdOsd

void cRBMHdOsd::SetActive(bool On) {
    //DBG(DBG_FN,"cRBMHdOsd::SetActive(%b)", On);
    if( On != Active()) {
        cOsd::SetActive(On);
        Clear();
        if(On)
            Flush();
    } // if
} // SetActive

bool cRBMHdOsd::Init() {
    if (!gotFBInfo) {
      struct fb_var_screeninfo screeninfo;
      if(ioctl(osd_fd, FBIOGET_VSCREENINFO, &screeninfo)) {
        printf("[RBM-FB] Can't get VSCREENINFO\n");
        return false;
      } // if
      printf("bits_per_pixel %d xres %d yres %d\n", screeninfo.bits_per_pixel, screeninfo.xres, screeninfo.yres);
      if (screeninfo.bits_per_pixel != 32) {
        screeninfo.bits_per_pixel = 32;
        if(ioctl(osd_fd, FBIOPUT_VSCREENINFO, &screeninfo)) {
          printf("[RBM-FB] Can't set VSCREENINFO\n");
          return false;
        } // if
      }

      bpp = osd_bpp = screeninfo.bits_per_pixel/8;
      width = osd_width = screeninfo.xres;
      height = osd_height = screeninfo.yres;
      gotFBInfo = true;
    } else {
      bpp = osd_bpp;
      width = osd_width;
      height = osd_height;
    }

#ifdef DOUBLE_BUFFER
    /** check if pointer has already been set */
    if (fb_ptr == (unsigned char*)-1 || fb_ptr == NULL)
        fb_ptr = (unsigned char*) mmap(0, osd_bpp * osd_width * osd_height, PROT_READ|PROT_WRITE, MAP_SHARED, osd_fd, 0);
    /** check if mmap() failed */
    if(fb_ptr == (unsigned char*)-1 || !fb_ptr) {
        printf("[RBM-FB] Can't mmap\n");
        return false;
    } // if
    /** check if pointer has already been set */
    if (data == NULL) {
        data = (unsigned char*) malloc(osd_bpp*osd_width*osd_height);
        memset(data, 0x00, osd_bpp*osd_width*osd_height);
    }
    /* check if malloc() failed */
    if(data == NULL) {
        printf("[RBM-FB] Can't malloc\n");
        return false;
    }
    if (mySavedRegion == NULL) {
        mySavedRegion = (unsigned char*) malloc(osd_bpp*osd_width*osd_height);
        memset(mySavedRegion, 0x00, osd_bpp*osd_width*osd_height);
    }
    if (mySavedRegion == NULL) {
        printf("[RBM-FB] Can't malloc\n");
        return false;
    }
#else
    data = (unsigned char*) mmap(0, osd_bpp * osd_width * osd_height, PROT_READ|PROT_WRITE, MAP_SHARED, osd_fd, 0);
    /** check if mmap() failed */
    if(data == (unsigned char*)-1 || !data) {
        printf("[RBM-FB] Can't mmap\n");
        return false;
    } // if
#endif
    return true;
} // Init

void cRBMHdOsd::Done() {
    for (int i = 0; i < numBitmaps; i++)
        delete bitmaps[i];
    //delete savedRegion;

#ifdef DOUBLE_BUFFER
    //munmap(fb_ptr, bpp * osd_width * osd_height);
#else
    //munmap(data, bpp * osd_width * ods_height);
#endif
} // Done

void cRBMHdOsd::Clear() {
    memset(data, 0x00, osd_width*osd_height*osd_bpp);
#ifdef DOUBLE_BUFFER
    //ioctl(osd_fd, FBIOBLANK);
    memset(fb_ptr, 0x00, osd_width*osd_height*osd_bpp);
#endif
} // Clear

void cRBMHdOsd::Draw8RGBLine(int x, int y, int w, const tIndex *srcData, const tColor *palette, int num_colors) {
    //DBG(DBG_FN,"cRBMHdOsd::Draw8RGBLine x %d y %d w %d", x, y, w);
    unsigned char *bm = (unsigned char *)srcData;
    int n = 0;
    if (x+w>=osd_width || x>=osd_width || y>=osd_height)
        return;
    uint32_t *px=(uint32_t*)(data +osd_width*y*bpp+x*bpp);
#if OSD_EXTRA_CHECK
    if(WITHIN_OSD_BORDERS(px+w-1))
#endif
    for(n=0;n<w;n++) {
        if(palette)
            //TB *px++=palette[(*bm++)%(num_colors/*TB:?? -1*/)];
            *px++=palette[(*bm++)%num_colors];
    }
} // Draw8RGB

void cRBMHdOsd::FlushBitmap(cBitmap &bitmap, bool full) {
    //DBG(DBG_FN,"cRBMHdOsd::FlushBitmap full %d", full);
    int x1 = 0;
    int y1 = 0;
    int x2 = bitmap.Width() - 1;
    int y2 = bitmap.Height() - 1;
    if (full || bitmap.Dirty(x1, y1, x2, y2)) {
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 < 0) x2 = 0;
        if (y2 < 0) y2 = 0;
        int x = Left() + bitmap.X0() + x1;
        int y = Top()  + bitmap.Y0() + y1;
        if (x >= osd_width || y >= osd_height) {
            bitmap.Clean();
            return;
        } // if
        int w = x2 - x1 + 1;
        int h = y2 - y1 + 1;
        if (x + w > osd_width)
            w = osd_width - x;
        if (y + h > osd_height)
            h = osd_height - y;
        int numColors;
        const tColor *palette = bitmap.Colors(numColors);
        for (int yy = y1; yy < y1+h; yy++)
            Draw8RGBLine(x, y++, w, bitmap.Data(x1,yy), palette, numColors);
        bitmap.Clean();
    } // if
} // FlushBitmap

cRBMHdOsd::~cRBMHdOsd(){
    DBG(DBG_FN,"cRBMHdOsd::~cRBMHdOsd");
    SetActive(false);
    Done();
} // ~cRBMHdOsd


cBitmap *cRBMHdOsd::GetBitmap(int Area){
    //DBG(DBG_FN,"cRBMHdOsd::GetBitmap");
    return cOsd::GetBitmap(Area); //TODO
#if 0
    return Area < numBitmaps ? bitmaps[Area] : NULL;
#endif
} // GetBitmap


eOsdError cRBMHdOsd::CanHandleAreas(const tArea *Areas, int NumAreas){
    //DBG(DBG_FN,"cRBMHdOsd::CanHandleAreas");
  if (NumAreas > MAXOSDAREAS)
     return oeTooManyAreas;
  eOsdError Result = oeOk;
  for (int i = 0; i < NumAreas; i++) {
      if (Areas[i].x1 > Areas[i].x2 || Areas[i].y1 > Areas[i].y2 || Areas[i].x1 < 0 || Areas[i].y1 < 0)
         return oeWrongAlignment;
      for (int j = i + 1; j < NumAreas; j++) {
          if (Areas[i].Intersects(Areas[j])) {
             Result = oeAreasOverlap;
             break;
             }
          }
      }
  return Result;
} // CanHandleAreas


eOsdError cRBMHdOsd::SetAreas(const tArea *Areas, int NumAreas){
    //DBG(DBG_FN,"cRBMHdOsd::SetAreas");
    //if (Active()) //TB: not needed here?! it cost's 100ms....
    //    Clear();

    eOsdError Result = CanHandleAreas(Areas, NumAreas); // 1.6
    if (Result == oeOk) {
        width = height = 0;
        for (int i = 0; i < NumAreas; i++) {
            width = max(width, Areas[i].x2 + 1);
            height = max(height, Areas[i].y2 + 1);
        }
    } else // 1.6
        esyslog("ERROR: cOsd::SetAreas returned %d", Result);
  return Result;
} // SetAreas

void cRBMHdOsd::SaveRegion(int x1, int y1, int x2, int y2){
    //DBG(DBG_FN,"cRBMHdOsd::SaveRegion");
    x1 += left; y1 += top; x2 += left; y2 += top;
    savedRegion_x0 = x1; savedRegion_y0 = y1; savedRegion_x1 = x2; savedRegion_y1 = y2+1;
    int lines = y2 - y1+1;
    int pixels = x2 - x1;
    int i;
    if (pixels > 0)
        for(i = 0; i < lines; i++)
            memcpy(mySavedRegion + (i+y1)*osd_width*bpp + x1*bpp, data + (i+y1)*osd_width*bpp + x1*bpp, pixels*osd_bpp);
} // SaveRegion


void cRBMHdOsd::RestoreRegion(void){
    //DBG(DBG_FN,"cRBMHdOsd::RestoreRegion");
    int x1 = savedRegion_x0; int y1 = savedRegion_y0; int x2 = savedRegion_x1; int y2 = savedRegion_y1;
    int lines = y2 - y1;
    int pixels = x2 - x1;
    int i;
    if (pixels > 0)
        for(i = 0; i < lines; i++)
            memcpy(data + (i+y1)*osd_width*osd_bpp + x1*osd_bpp, mySavedRegion + (i+y1)*osd_width*osd_bpp + x1*osd_bpp, pixels*osd_bpp);
} // RestoreRegion

eOsdError cRBMHdOsd::SetPalette(const cPalette &Palette, int Area){
    //DBG(DBG_FN,"cRBMHdOsd::SetPalette");
    return cOsd::SetPalette(Palette, Area);
} // SetPalette

void cRBMHdOsd::DrawPixel(int x, int y, tColor Color){
    //DBG(DBG_FN,"cRBMHdOsd::DrawPixel");
    x += left;
    y += top;
    UpdateDirty(x, y, x+1, y+1);
    if (0 <= x && x < osd_width && 0 <= y && y < osd_height)
        *((uint32_t*)(data + osd_width * y * bpp  + x * bpp )) = Color;
} // DrawPixel


void cRBMHdOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg, tColor ColorBg, bool ReplacePalette, bool blend){
    //DBG(DBG_FN,"cRBMHdOsd::DrawBitmap");
    //cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);

    unsigned char const *srcData = (unsigned char const *)Bitmap.Data(0,0);
    unsigned char const *xs;
    int qx, qy, xt1, yt1, xt, yt, vfx, vfy, vfw, vfh, w, h, m, *px, n;

    x += left;
    y += top;
    w=Bitmap.Width();
    h=Bitmap.Height();

    if (w+x >= osd_width || y+h >= osd_height)
        return;

    UpdateDirty(x, y, x+w, y+h);

    if (blend) {

        vfx = 0;
        vfy = 0;
        vfw = osd_width-1;
        vfh = osd_height-1;

        xt1 = vfx * osd_width + x * vfw;
        xt = xt1 / osd_width;
        yt1 = vfy * osd_height + y * vfh;
        yt = yt1 / osd_height;
        qy = yt1 % osd_height;
        // bm = (unsigned int*)bco->data;

        for(m=0;m<h;) {
            px=(int*)(data + osd_width * yt * bpp  + xt * bpp );
            xs = srcData + m * w;
            qx = xt1 % osd_width;
#ifdef OSD_EXTRA_CHECK
            if(WITHIN_OSD_BORDERS(px+w-1))
#endif
            for(n=0;n<w;) {
                *px++=Bitmap.Color(*xs);
                qx += osd_width;
                while (qx >= osd_width) {
                    ++xs;
                    ++n;
                    qx -= vfw;
                }
            }
            ++yt;
            qy += osd_height;
            while (qy >= osd_height) {
                ++m;
                qy -= vfh;
            }
        }
    } else {
        unsigned int *bm = (unsigned int*)Bitmap.Data(0,0);
        int o=0, val = 0;

        for (m=0;m<h;m++) {
            px=(int*)(data +osd_width*(y+m)*bpp+x*bpp);
#if OSD_EXTRA_CHECK
            if(WITHIN_OSD_BORDERS(px+w-1))
#endif
            for(n=0;n<w;n++) {
                    if ((o&3)==0)
                            val=*bm++;
                    *px++=Bitmap.Color(val&255);
                    val>>=8;
                    o++;
            }
        }
    }
} // DrawBitmap

void cRBMHdOsd::DrawBitmap32(int x, int y, const cBitmap &Bitmap, tColor ColorFg, tColor ColorBg, bool ReplacePalette, bool Overlay, int width_, int height_){
    //DBG(DBG_FN,"cRBMHdOsd::DrawBitmap");
    //cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);
    //if(width_ == 0 || height_ == 0)
    //      return;
    //printf("DrawBitmap32: x0: %i y0: %i width: %i height: %i w: %i h: %i\n", x0, y0, width_, height_, Bitmap.Width(), Bitmap.Height());

    unsigned char const *srcData = Bitmap.Data(0,0);
    unsigned char const *xs;
    static int qx, qy, xt1, yt1, line, row;
    static uint32_t pxs, *px;

    UpdateDirty(x, y, x+width_, y+height_);

    xt1 = x * osd_width;
    yt1 = y * osd_height;
    qy = 0;

    int numColors;
    tColor const *colors = Bitmap.Colors(numColors);
    for (int n = 0; n < numColors; ++n) {
        palette[n] = colors[n];
    }

    for(line=0;line<height_; y++) {
        px=(uint32_t*)(data + osd_width * y * bpp  + x * bpp );
        xs = srcData + line * width_;
        qx = xt1 % osd_width;
#if 0 //def OSD_EXTRA_CHECK
        if(WITHIN_OSD_BORDERS(px+w-1))
#endif

        for(row=0;row<width_ && ((int*)(px+width_) < (int*)(data+osd_width*osd_height*bpp)); px++) {
            pxs = palette[*xs];
            if(pxs&0x00FFFFFF && pxs!=0x01ffffff)
                *px = DoAlphaBlend(pxs, *px);
            else
                *px = DoAlphaBlend(*px, pxs);
            if (qx >= 0) {
                xs++;
                row++;
                qx = 0;
            } else
                qx += osd_width;
        }
        if (qy >= 0) {
            ++line;
            qy = 0;
        } else
            qy += osd_height;
    }
} // DrawBitmap32

void cRBMHdOsd::DrawBitmapHor(int x, int y, int w, const cBitmap &Bitmap){
    //DBG(DBG_FN,"cRBMHdOsd::DrawBitmapHor");
    //cOsd::DrawBitmapHor(x, y, w, Bitmap);
} // DrawBitmapHor

void cRBMHdOsd::DrawBitmapVert(int x, int y, int h, const cBitmap &Bitmap){
    //DBG(DBG_FN,"cRBMHdOsd::DrawBitmapVert");
    //cOsd::DrawBitmapVert(x, y, h, Bitmap);
} // DrawBitmapVert

void cRBMHdOsd::DrawCropImage(u_int imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend) {
    //printf("DrawCropImage\n");
    if (!ImageIdInRange(imageId)) return;

    /* adjust coordinates with global OSD-margins */
    x+=left;
    y+=top;
    x0+=left;
    y0+=top;
    x1+=left;
    y1+=top;

    UpdateDirty(x0, y0, x1, y1);

    LoadImage(imageId);

    CachedImage const *img = cachedImages_[imageId];
    int m, n, h, v;
    int width_;
    int height_;
    uint32_t const *srcPixels;
    int vertRepeat = 1;
    int horRepeat = 1;

    if (img) {
        if(x0<x || y0<y) return; // invalid param
        if(x1<=x0 || y1<=y0) return; // inavlid param
        if(img->height<=0 || img->width<=0) return; // invalid image
        if(x0-x >= img->width) return; // invalid param for this image
        if(y0-y >= img->height) return; // invalid param for this image

        width_ = x1-x0;
        height_ = y1-y0;
        if(img->height)
            vertRepeat = (y1-y0)/img->height;
        if(vertRepeat < 1)
            vertRepeat = 1;
        if(img->width)
            horRepeat = (x1-x0)/img->width;
        if(horRepeat < 1)
            horRepeat = 1;

        if (blend) {
            int dy=y0;
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = img->data + img->width*(y0-y) + (x0-x);
                for (n = min(height_, img->height); n > 0; --n) {
                    uint32_t *tgtPixels = (uint32_t*)(data + bpp * osd_width * dy++ + x0*bpp);
                    for (h = horRepeat; h > 0; --h) {
                        uint32_t const *src = srcPixels;
                        for (m = min(width_, img->width); m > 0; --m) {
                            *tgtPixels = DoAlphaBlend((*src++), (*tgtPixels) );
                            ++tgtPixels;
                        }
                    }
                    srcPixels += img->width;
                }
            }
        } else {
            int dy=y0;
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = img->data + img->width*(y0-y) + (x0-x);
                for (n = min(height_, img->height); n > 0; --n) {
                    uint32_t *tgtPixels = (uint32_t*)(data + bpp * osd_width * dy++ + x0*bpp);
                    for (h = horRepeat; h > 0; --h) {
                        uint32_t const *src = srcPixels;
                        memcpy(tgtPixels, src, min(width_, img->width)*sizeof(int));
                        tgtPixels += img->width;
                    }
                    srcPixels += img->width;
                }
            }
        }
    } else {
        printf("Image %d not cached.\n", imageId);
    }
}

void cRBMHdOsd::DrawText(int x, int y, const char *s_in, tColor colorFg, tColor colorBg, const cFont *font, int w, int h, int alignment){
    //DBG(DBG_FN,"cRBMHdOsd::DrawText");
    //cOsd::DrawText(x, y, s_in, colorFg, colorBg, font, w, h, alignment);

    if (s_in) {
        /* adjust coordinates with global OSD-margins */
        x+=left;
        y+=top;

        /* check for empty string */
        unsigned int i;
        unsigned int len = strlen(s_in);
        for(i=0; i<len; i++){ /* count the spaces */
            if(s_in[i] == ' ')
                continue;
            else
                break;
        }

        if(w == 0) w = font->Width(s_in);
        if(h == 0) h = font->Height();

        if(i == len) { /* every char is a space */
            if((colorBg >> 24) != 0) /* not transparent */
                DrawRectangle(x-left, y-top, x + w - left, y + h - top, colorBg); /* clear the background */
            return;
        }

        UpdateDirty(x, y, x+w, y+w);

        if((colorBg >> 24 == 0) || ((colorBg&0x00ffffff) == 0x00000000)){ /* TB: transparent as bgcolor is evil */
            colorBg = colorFg&0x01ffffff;
        }


        int old_x = x; int old_y = y; y=0; x=0;

        int w_ = font->Width(s_in);
        int h_ = font->Height();
        int limit = 0;
        if (w || h) {
            int cw = w ? w : w_;
            //int ch = h ? h : h_;
            limit = x + cw;
            if (w) {
               if ((alignment & taLeft) != 0)
                  ;
               else if ((alignment & taRight) != 0) {
                  if (w_ < w)
                     x += w - w_;
               } else { // taCentered
                  if (w_ < w)
                     x += (w - w_) / 2;
               }
            }
            if (h) {
               if ((alignment & taTop) != 0)
                  ;
               else if ((alignment & taBottom) != 0) {
                  if (h_ < h)
                     y += h - h_;
               } else { // taCentered
                  if (h_ < h)
                     y += (h - h_) / 2;
               }
            }
         }
         bool AntiAliased = Setup.AntiAlias;
         bool TransparentBackground = (colorBg == clrTransparent);
         static int16_t BlendLevelIndex[MAX_BLEND_LEVELS]; // tIndex is 8 bit unsigned, so a negative value can be used to mark unused entries
         if (AntiAliased && !TransparentBackground)
            memset(BlendLevelIndex, 0xFF, sizeof(BlendLevelIndex)); // initializes the array with negative values
         cPalette palette;
         palette.Index(colorFg);
         uint prevSym = 0;
         while (*s_in) {
           int sl = Utf8CharLen(s_in);
           uint sym = Utf8CharGet(s_in, sl);
           s_in += sl;
           cGlyph *g = ((cFreetypeFont*)font)->Glyph(sym, AntiAliased);
           if (!g)
              continue;
           int kerning = ((cFreetypeFont*)font)->Kerning(g, prevSym);
           prevSym = sym;
           uchar *buffer = g->Bitmap();
           int symWidth = g->Width();
           int symLeft = g->Left();
           int symTop = g->Top();
           int symPitch = g->Pitch();
           if (limit && x + symWidth + symLeft + kerning - 1 > limit)
              break; // we don't draw partial characters
           int px_tmp_sum = symLeft + kerning + x;
           int py_tmp_sum = y + (font->Height() - ((cFreetypeFont*)font)->Bottom() - symTop);
           if (x + symWidth + symLeft + kerning > 0) {
              for (int row = 0; row < g->Rows(); row++) {
                  for (int pitch = 0; pitch < symPitch; pitch++) {
                      uchar bt = *(buffer + (row * symPitch + pitch));
                      if (AntiAliased) {
                         if (bt > 0x00) {
                            int px = pitch + px_tmp_sum;
                            int py = row + py_tmp_sum;
                            uint32_t *dstPx = (uint32_t*)(data + osd_width * (py+old_y) * bpp  + (px+old_x) * bpp );
                            if (bt == 0xFF)
                               *dstPx = colorFg;
                            else if (TransparentBackground)
                               *dstPx = palette.Blend(colorFg, *dstPx, bt);
                            else if (BlendLevelIndex[bt] >= 0)
                               *dstPx = palette.Blend(palette.Color(BlendLevelIndex[bt]), *dstPx, bt);
                            else
                               *dstPx = palette.Blend(colorFg, *dstPx, bt);

                         }
                      } else { //monochrome rendering
                         for (int col = 0; col < 8 && col + pitch * 8 <= symWidth; col++) {
                             if (bt & 0x80) {
                                uint32_t *dstPx = (uint32_t*)(data + osd_width * (old_y + y + row + (font->Height() - ((cFreetypeFont*)font)->Bottom() - symTop)) * bpp  + (old_x + x + col + pitch * 8 + symLeft + kerning) * bpp );
                                *dstPx = colorFg;
                             }
                             bt <<= 1;
                          }
                      }
                   }
               }
           }
           x += g->AdvanceX() + kerning;
           if (x > w - 1)
              break;
        }
   }
} // DrawText

void cRBMHdOsd::ClosePngFile() {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

void cRBMHdOsd::SetImagePath(u_int imageId, char const *path) {
    //printf("SetImagePath: ID: %i path: %s\n", imageId, path);
    if (!ImageIdInRange(imageId)) return;
    if (strcmp(path, imagePaths_[imageId].c_str())) { /* TB: no need to reload if path hasn't changed */
        //printf("SetImagePath: ID: %i path: %s oldpath: %s\n", imageId, path, imagePaths_[imageId].c_str());
        imagePaths_[imageId] = path;
        imageDirty_[imageId] = true;
    }
}

bool cRBMHdOsd::OpenPngFile(char const *path, unsigned char **&rows, int &width, int &height) {
    //printf("OpenPngFile\n");

    unsigned char header[8];

    FILE *fp = fopen(path, "rb");
    if (!fp || fread(header, 1, 8, fp) != 8) {
        if(fp)
            fclose(fp);
        return false;
    }
    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        return false;
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return false;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return false;
    }
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        fclose(fp);
        return false;
    }
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    if (setjmp(png_jmpbuf(png_ptr))) {
        ::printf("setjmp err\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return false;
    }
    png_read_info(png_ptr, info_ptr);
    png_byte h = info_ptr->height;
    png_byte color_type = info_ptr->color_type;
//#define PRINT_COLOR_TYPE 1
#ifdef PRINT_COLOR_TYPE
    png_byte w = info_ptr->width;
    png_byte bit_depth = info_ptr->bit_depth;
    printf("bit depth: %i - ", bit_depth);
    switch (color_type) {
        case PNG_COLOR_TYPE_GRAY: puts("color_type: PNG_COLOR_TYPE_GRAY"); break;
        case PNG_COLOR_TYPE_PALETTE: puts("color_type: PNG_COLOR_TYPE_PALETTE"); break;
        case PNG_COLOR_TYPE_RGB: puts("color_type: PNG_COLOR_TYPE_RGB"); break;
        case PNG_COLOR_TYPE_RGB_ALPHA: puts("color_type: PNG_COLOR_TYPE_RGB_ALPHA"); break;
        case PNG_COLOR_TYPE_GRAY_ALPHA: puts("color_type: PNG_COLOR_TYPE_GRAY_ALPHA"); break;
        default: puts("ERROR: unknown color_type"); break;
    }
#endif

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);
    rows = (png_bytep*) malloc(sizeof(png_bytep) * h);
    int y;
    for (y=0; y<h; y++) {
        rows[y] = (png_byte*) malloc(info_ptr->rowbytes);
    }
    png_read_image(png_ptr, rows);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    fclose(fp);
    return true;
}

void cRBMHdOsd::LoadImage(u_int imageId) {
    //printf("LoadImage() ID: %i\n", imageId);
    if (!ImageIdInRange(imageId)) return;
    if (cachedImages_[imageId]) {
        if(cachedImages_[imageId]->data && !imageDirty_[imageId])
            return; /* Already cached. */
        if(cachedImages_[imageId]->data) free(cachedImages_[imageId]->data);
        free(cachedImages_[imageId]);
        cachedImages_[imageId]=0;
    }
    /* Not cached, load it. */
    std::string const &path_ = imagePaths_[imageId];
    if (path_.empty()) {
        printf("ERROR: Image id %u: No path set.\n", imageId);
        return;
    }
    const char *path = path_.c_str();

    unsigned char **rows;
    int  width, height;

    if (OpenPngFile(path, rows, width, height)) {
        void *buffer = malloc(width * height * sizeof(u_int));
        CachedImage *img = (struct CachedImage*)malloc(sizeof(struct CachedImage));
        img->data = (unsigned int*)buffer;
        img->height = height;
        img->width = width;

        cachedImages_[imageId] = img;
        unsigned char *p = static_cast<unsigned char *>(buffer);

        for (int y = 0; y < height; ++y) {
            unsigned char *r = rows[y];
            for (int x = 0; x < width; ++x) {
                p[0] = r[2];
                p[1] = r[1];
                p[2] = r[0];
                p[3] = r[3];
                r += 4;
                p += 4;
            }
            free(rows[y]);
        }
        free(rows);

        ClosePngFile();
        //free(buffer);

        if (imageId == 119) {  /* thumbnails */
            lastThumbWidth[0] = width;
            lastThumbHeight[0] = height;
        } else if (imageId == 120) {
            lastThumbWidth[1] = width;
            lastThumbHeight[1] = height;
        }
        imageDirty_[imageId] = false;
    } else
        printf("ERROR: failed opening %s ID %u\n", path, imageId);
}

void cRBMHdOsd::DrawImage(u_int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat){
    //DBG(DBG_FN,"cRBMHdOsd::DrawImage");
    //cOsd::DrawImage(imageId, x, y, blend, horRepeat, vertRepeat);
    //printf("cRBMHdOsd::DrawImage: id: %i x: %i y: %i blend: %i hR: %i vR: %i path: %s\n", imageId, x, y, blend, horRepeat, vertRepeat, imagePaths_[imageId].c_str());
    if (!ImageIdInRange(imageId)) return;

    /* adjust coordinates with global OSD-margins */
    x+=left;
    y+=top;

    LoadImage(imageId);

    CachedImage const *img = cachedImages_[imageId];

    int m, n, h_, v;
    int w, h;
    uint32_t const *srcPixels;

    if (img) {
        w = img->width;
        h = img->height;
        int w_all = vertRepeat ? vertRepeat * w : w;
        int h_all = horRepeat ? horRepeat * h : h;

        UpdateDirty(x, y, x+w_all, y+h_all);

        if (blend) {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = img->data;
                for (n = h; n > 0; --n) {
                    uint32_t *tgtPixels = (uint32_t*)(data + bpp * osd_width * y++ + x*bpp);
                    for (h_ = horRepeat; h_ > 0; --h_) {
                        uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
                        if(WITHIN_OSD_BORDERS(tgtPixels+w))
#endif
                        for (m = w; m > 0; --m) {
                            int res = DoAlphaBlend((*src++), (*tgtPixels) );          // where they come from (libpng?) - so let's filter them out

                            if( (res>>24)!=0 || (res&0xffffff) != 0xffffff){ // TB: there are white pixels on the left corner of the OSD - I don't know
                                *tgtPixels = res;          // where they come from (libpng?) - so let's filter them out
                            } else                            // explanation: those pixels are completely transparent, but there
                                *tgtPixels = 0x00000000;    // is no alpha-blending done as long as there isn't any video data
                            ++tgtPixels;
                        }
                    }
                    srcPixels += w;
                }
            }
        } else {
            for (v = vertRepeat; v > 0; --v) {
                srcPixels = img->data;
                for (n = h; n > 0; --n) {
                    uint32_t *tgtPixels = (uint32_t*)(data + bpp * osd_width * y++ + x*bpp);
                    for (h_ = horRepeat; h_ > 0; --h_) {
                        uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
                        if(WITHIN_OSD_BORDERS(tgtPixels+w*(sizeof(int))))
#endif
#if 0
                        memcpy(tgtPixels, src, w*sizeof(int));
                        tgtPixels+=w;
                        src+=w;
#else
                        for (m = w; m > 0; --m) {
                            if((*src>>24)!=0 || (*src&0xffffff) != 0xffffff){ // TB: there are white pixels on the left corner of the OSD - I don't know
                                *tgtPixels = *src;          // where they come from (libpng?) - so let's filter them out
                            } else                            // explanation: those pixels are completely transparent, but there
                                *tgtPixels = 0x00000000;    // is no alpha-blending done as long as there isn't any video data
                            ++src;
                            ++tgtPixels;
                        }
#endif

                    }
                    srcPixels += w;
                }
            }
         }
     } else {
        printf("Image %d not cached.\n", imageId);
    }
} // DrawImage

bool inline cRBMHdOsd::ClipArea(int *l, int *t, int *r, int *b) {
    if (*r >= osd_width)
        *r = osd_width-1;
    if (*b >= osd_height)
        *b = osd_height-1;
    return *l < *r && *t < *b;
}

/* buffer for DrawRectangle() */
static uint32_t draw_linebuf[1024];
static uint32_t draw_color;

void cRBMHdOsd::DrawRectangle(int l, int t, int r, int b, tColor Color){
    //DBG(DBG_FN,"cRBMHdOsd::DrawRectangle1");
    //printf("DrawRectangle: l: %i t: %i r: %i b: %i clr: %#08x\n", l, t, r, b, Color);
    /* adjust coordinates with global OSD-margins */
    l+=left;
    t+=top;
    r+=left+1;
    b+=top+1;

    if (ClipArea(&l, &t, &r, &b)) {
        UpdateDirty(l, t, r, b);

        unsigned int m, n;
        unsigned int w = r - l;
        unsigned int h = b - t;
        uint32_t *pixels = (uint32_t*)(data + osd_width * t * bpp  + l*bpp);

        if (draw_color!=Color) {
            for(n=0;n<1024;n++)
                draw_linebuf[n]=Color;
                draw_color=Color;
            }
            for (m = h; m; --m) {
#if OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(pixels+w*sizeof(int)))
#endif
                memcpy(pixels,draw_linebuf,w*sizeof(int));
                pixels+=osd_width;
        }
    }
} // DrawRectangle


void cRBMHdOsd::DrawRectangle(int l, int t, int r, int b, tColor color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV){
    //DBG(DBG_FN,"cRBMHdOsd::DrawRectangle2")

    /* adjust coordinates with global OSD-margins */
    l+=left;
    t+=top;
    r+=left+1;
    b+=top+1;

    if (ClipArea(&l, &t, &r, &b)) {
        UpdateDirty(l, t, r, b);

        unsigned int m, n;
        unsigned int w = r - l;
        unsigned int h = b - t;
        unsigned int mod = osd_width - w;
        uint32_t *pixels = (uint32_t*)(data + osd_width * t * bpp  + l*bpp);
        unsigned int curAlphaH = 0, curAlphaV = 0;

        for(n=0; n<h; n++){
            if(alphaGradStepV && n%alphaGradStepV==0)
                curAlphaV++;
#if OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(pixels+w-1))
#endif
                for (m=0; m<w; m++){
                    if(alphaGradStepH && m%alphaGradStepH==0)
                        curAlphaH++;
                    *pixels++ = ( (color&0x00FFFFFF) | (((color>>24) + ((curAlphaH*alphaGradH + curAlphaV*alphaGradV)))<<24) );
                }
            pixels += mod;
            curAlphaH = 0;
        }
    }
    //cOsd::DrawRectangle(x1, y1, x2, y2, Color, alphaGradH, alphaGradV, alphaGradStepH, alphaGradStepV);
} // DrawRectangle

void cRBMHdOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants){
    //DBG(DBG_FN,"cRBMHdOsd::DrawEllipse");
    x1 += left;
    y1 += top;
    x2 += left;
    y2 += top;
    UpdateDirty(x1, y1, x2, y2);
    // Algorithm based on http://homepage.smc.edu/kennedy_john/BELIPSE.PDF
    int rx = x2 - x1;
    int ry = y2 - y1;
    int cx = (x1 + x2) / 2;
    int cy = (y1 + y2) / 2;
    switch (abs(Quadrants)) {
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
    int TwoASquare = 2 * rx * rx;
    int TwoBSquare = 2 * ry * ry;
    int x = rx;
    int y = 0;
    int XChange = ry * ry * (1 - 2 * rx);
    int YChange = rx * rx;
    int EllipseError = 0;
    int StoppingX = TwoBSquare * rx;
    int StoppingY = 0;
    while (StoppingX >= StoppingY) {
        switch (Quadrants) {
          case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); // no break
          case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, Color); break;
          case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); // no break
          case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, Color); break;
          case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); break;
          case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); break;
          case  0:
          case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, Color); if (Quadrants == 6) break;
          case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, Color); break;
          case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, Color); break;
          case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, Color); break;
          case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, Color); break;
          case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, Color); break;
        }
        y++;
        StoppingY += TwoASquare;
        EllipseError += YChange;
        YChange += TwoASquare;
        if (2 * EllipseError + XChange > 0) {
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
    while (StoppingX <= StoppingY) {
        switch (Quadrants) {
          case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); // no break
          case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, Color); break;
          case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); // no break
          case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, Color); break;
          case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); break;
          case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); break;
          case  0:
          case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, Color); if (Quadrants == 6) break;
          case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, Color); break;
          case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, Color); break;
          case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, Color); break;
          case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, Color); break;
          case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, Color); break;
        }
        x++;
        StoppingX += TwoBSquare;
        EllipseError += XChange;
        XChange += TwoBSquare;
        if (2 * EllipseError + YChange > 0) {
           y--;
           StoppingY -= TwoASquare;
           EllipseError += YChange;
           YChange += TwoASquare;
        }
    }
} // DrawEllipse

void cRBMHdOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type){
    //DBG(DBG_FN,"cRBMHdOsd::DrawSlope");
    x1 += left;
    y1 += top;
    x2 += left;
    y2 += top;
    UpdateDirty(x1, y1, x2, y2);
    /* TODO This is just a quick and dirty implementation of a slope drawing
     * machanism. If somebody can come up with a better solution, let's have it! */
    bool upper    = Type & 0x01;
    bool falling  = Type & 0x02;
    bool vertical = Type & 0x04;
    if (vertical) {
       for (int y = y1; y <= y2; y++) {
           double c = cos((y - y1) * M_PI / (y2 - y1 + 1));
           if (falling)
              c = -c;
           int x = int((x2 - x1 + 1) * c / 2);
           if ((upper && !falling) || (!upper && falling))
              DrawRectangle(x1, y, (x1 + x2) / 2 + x, y, Color);
           else
              DrawRectangle((x1 + x2) / 2 + x, y, x2, y, Color);
       }
   } else {
       for (int x = x1; x <= x2; x++) {
           double c = cos((x - x1) * M_PI / (x2 - x1 + 1));
           if (falling)
              c = -c;
           int y = int((y2 - y1 + 1) * c / 2);
           if (upper)
              DrawRectangle(x, y1, x, (y1 + y2) / 2 + y, Color);
           else
              DrawRectangle(x, (y1 + y2) / 2 + y, x, y2, Color);
       }
   }
} // DrawSlope

void cRBMHdOsd::Flush(void){
    //DBG(DBG_FN,"cRBMHdOsd::Flush");
#ifdef DOUBLE_BUFFER
#if 0
    memcpy(fb_ptr, data, osd_width*osd_height*bpp);
#else
    int lines = dirtyArea_.y1 - dirtyArea_.y0;
    int pixels = dirtyArea_.x1 - dirtyArea_.x0;
    int i, ii;
    //printf("dirty: %i %i %i %i lines: %i pixels: %i\n", dirtyArea_.x0, dirtyArea_.y0, dirtyArea_.x1, dirtyArea_.y1, lines, pixels);
    if(pixels > 0 && pixels <= (int)(osd_width - dirtyArea_.x0))
        for(ii = 0; ii < lines; ii++) {
            if(ii&1)
                i = lines/2-ii/2;
            else
                i = lines/2+ii/2;
            memcpy(fb_ptr + (i+dirtyArea_.y0)*osd_width*bpp + dirtyArea_.x0*bpp, data + (i+dirtyArea_.y0)*osd_width*bpp + dirtyArea_.x0*bpp, pixels*bpp);
        }
    dirtyArea_.x0 = osd_width-1;
    dirtyArea_.y0 = osd_height-1;
    dirtyArea_.x1 = 1;
    dirtyArea_.y1 = 1;
#endif
#endif
} // Flush

/** Mark the rectangle between (x0, y0) and (x1, y1) as an area that has changed */
void cRBMHdOsd::UpdateDirty(int x0, int y0, int x1, int y1) {
#ifdef DOUBLE_BUFFER
    if(x0 >= osd_width)  x0 = osd_width-1;
    if(x1 >= osd_width)  x1 = osd_width-1;
    if(y0 >= osd_height) y0 = osd_height-1;
    if(y1 >= osd_height) y1 = osd_height-1;

    if (dirtyArea_.x0 > x0)  dirtyArea_.x0 = x0;
    if (dirtyArea_.y0 > y0)  dirtyArea_.y0 = y0-1; //TB: -1 is needed, 'cause else the top line of the progress bar is missing in the replay display
    if (dirtyArea_.x1 < x1)  dirtyArea_.x1 = x1;
    if (dirtyArea_.y1 < y1)  dirtyArea_.y1 = y1;
#endif
}

cRBMOsdProvider::cRBMOsdProvider(void) {
    memset(&png_ptr, 0, sizeof(png_ptr));
    memset(&info_ptr, 0, sizeof(info_ptr));
    for(int i=0; i<MAX_CACHED_IMAGES;i++) {
	cachedImages_[i]=0;
	imagePaths_[i]="";
	imageDirty_[i]=true;
    } // for
    memset(&lastThumbWidth, 0, sizeof(lastThumbWidth));
    memset(&lastThumbHeight, 0, sizeof(lastThumbHeight));
    memset(&dirtyArea_, 0, sizeof(dirtyArea_));
    last_p1 = last_p2 = last_res = 0;
    fb_fd = open(FB_DEVICE_NAME, O_RDWR|O_NDELAY);
    //printf("new cRBMOsdProvider\n");
    if(fb_fd == -1) {
        printf("[RBM-FB] cant open framebuffer %s\n", FB_DEVICE_NAME);
    } // if
}

cRBMOsdProvider::~cRBMOsdProvider(){
    if(fb_fd != -1) close (fb_fd);
    fb_fd = -1;
    int i;
    for (i = 0; i<MAX_CACHED_IMAGES; i++) {
        if(cachedImages_[i]) {
            if(cachedImages_[i]->data) {
                free(cachedImages_[i]->data);
                cachedImages_[i]->data = NULL;
            }
            free(cachedImages_[i]);
            cachedImages_[i] = NULL;
        }
    }
    free(data);
    data = NULL;
    if(mySavedRegion)
        free(mySavedRegion);
    mySavedRegion = NULL;
}
///< Returns a pointer to a newly created cOsd object, which will be located
///< at the given coordinates.
cOsd *cRBMOsdProvider::CreateOsd(int Left, int Top, uint Level) {
    //DBG(DBG_FN,"cRBMHdOsdProvider::CreateOsd");
    if(fb_fd != -1)
        return new cRBMOsd(Left, Top, Level, fb_fd);
    return NULL;
} // cRBMHdOsdProvider::CreateOsd

cOsd *cRBMOsdProvider::CreateTrueColorOsd(int Left, int Top, uint Level) {
    //DBG(DBG_FN,"cRBMHdOsdProvider::CreateTrueColorOsd");
    if(fb_fd != -1)
        return new cRBMHdOsd(Left, Top, Level, fb_fd);
    return NULL;
} // cRBMHdOsdProvider::CreateTrueColorOsd

