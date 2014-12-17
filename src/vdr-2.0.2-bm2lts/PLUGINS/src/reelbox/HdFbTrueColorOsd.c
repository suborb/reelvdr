/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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

// HdFbTrueColorOsd.c


#include "HdFbTrueColorOsd.h"

#include <vdr/tools.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <cstring>
#include <iostream>
#include <algorithm>

#include <png.h>

#include "font_helper.h"

namespace Reel
{
#define MAX_CACHED_IMAGES 256

    static CachedImage *cachedImages_[MAX_CACHED_IMAGES];
    std::string    imagePaths_[MAX_CACHED_IMAGES];
    bool           imageDirty_[MAX_CACHED_IMAGES];
    static int savedRegion_x0, savedRegion_y0, savedRegion_x1, savedRegion_y1;

    typedef struct
    {
       int x0, y0;
       int x1, y1;
    } dirtyArea;

    static dirtyArea dirtyArea_; /** to be able remember where the OSD has changed, to only need to flush this area */

    static uint32_t last_p1, last_p2, last_res;

    static bool Initialize() {
	for(int i=0; i<MAX_CACHED_IMAGES; i++) {
	    cachedImages_[i]=0;
	    imageDirty_[i]=true;
	}
	savedRegion_x0 = savedRegion_y0 = savedRegion_x1 = savedRegion_y1 = 0;
	memset(&dirtyArea_, 0, sizeof(dirtyArea_));
	last_p1 = last_p2 = last_res = 0;
	return true;
    }
    static bool Inited = Initialize(); 

    static inline uint32_t AlphaBlend(uint32_t p2, uint32_t p1) {

        if(p1 == last_p1 && p2 == last_p2)
            return last_res;
        else {
            last_p1 = p1;
            last_p2 = p2;
        }

        uint8_t const xa1 = (p1 >> 24);
        uint8_t const xa2 = (p2 >> 24);

        //printf("blend: fg: %#x bg: %#x\n", p2, p1);

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
        }
    }

    static inline void FlushOsd(osd_t *osd) {
        if (HdCommChannel::hda->osd_dont_touch&~4) {
            //DDD("FlushOSD BLOCKED!\n");
            return;
	}
        //HdCommChannel::hda->plane[2].alpha = 255;
        //HdCommChannel::hda->plane[2].changed++;                

        int lines = dirtyArea_.y1 - dirtyArea_.y0;    
        int pixels = dirtyArea_.x1 - dirtyArea_.x0;
        int rest = pixels%4;
        //printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>FlushOSD %d %d/%d-%d/%d w %d h %d\n", HdCommChannel::hda->osd_hidden, dirtyArea_.x0, dirtyArea_.y0, dirtyArea_.x1, dirtyArea_.y1, pixels, lines);
        if (pixels>0)
            pixels+=4-rest;
        if(pixels+dirtyArea_.x0 > (int)osd->width)
            pixels = osd->width - dirtyArea_.x0;
        int i, ii;
        //printf("dirty: %i %i %i %i lines: %i pixels: %i\n", dirtyArea_.x0, dirtyArea_.y0, dirtyArea_.x1, dirtyArea_.y1, lines, pixels);
        if(pixels > 0)
            for(ii = 0; ii < lines; ii++) {
                if(ii&1)
                    i = lines/2-ii/2;
                else
                    i = lines/2+ii/2;
#ifdef DECYPHER_BUG
                memcpy(osd->data + (i+dirtyArea_.y0)*osd->width*osd->bpp + dirtyArea_.x0*osd->bpp, osd->buffer + (i+dirtyArea_.y0)*osd->width*osd->bpp + dirtyArea_.x0*osd->bpp, pixels*osd->bpp);
#else
                int j;
                uint32_t *dst = ((uint32_t*)osd->data + (i+dirtyArea_.y0)*osd->width + dirtyArea_.x0);
                uint32_t *src = ((uint32_t*)osd->buffer + (i+dirtyArea_.y0)*osd->width + dirtyArea_.x0);
                for(j=0; j<pixels; j++)
#if 1
                  if(dst >= (uint32_t*)osd->data && dst < (uint32_t*)(osd->data+osd->height*osd->width*osd->bpp) &&
                    src >= (uint32_t*)osd->buffer  && src  < (uint32_t*)(osd->buffer+osd->height*osd->width*osd->bpp))
#endif
                    *(dst+j) = *(src+j);
#endif
            }
        dirtyArea_.x0 = osd->width-1;
        dirtyArea_.y0 = osd->height-1;
        dirtyArea_.x1 = 0;
        dirtyArea_.y1 = 0;
}

static bool inline ClipArea(osd_t *osd, unsigned int *l,unsigned  int *t,unsigned  int *r,unsigned  int *b) {
    if (*r >= osd->width) {
        *r = osd->width-1;
    }
    if (*b >= osd->height) {
        *b = osd->height-1;
    }
    return *l < *r && *t < *b;
}

void HdFbTrueColorOsd::ClearOsd(osd_t *osd) {
    if(osd && osd->buffer && osd->data) {
        memset(osd->buffer, 0x00, osd->width*osd->height*osd->bpp);
        memset(osd->data, 0x00, osd->width*osd->height*osd->bpp);
    }
}

    void HdFbTrueColorOsd::SendOsdCmd(void const *bco, UInt bcoSize, void const *payload, UInt payloadSize)
    {
        static char buffer[HD_MAX_DGRAM_SIZE];

        if(bcoSize + payloadSize > HD_MAX_DGRAM_SIZE)
		return;
#if 0
        int bufferSize = 0;
        while (!bufferSize)
        {
            bufferSize = ::hd_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   bcoSize + payloadSize,
                                                   1000);
        }
#endif
        std::memcpy(buffer, bco, bcoSize);
        if (payloadSize)
        {
            std::memcpy(static_cast<Byte *>((void*)buffer) + bcoSize, payload, payloadSize);
        }
        //::hd_channel_write_finish(osdChannel_, bufferSize);

        hd_packet_es_data_t packet;
        //printf("sendpacket!\n");
        HdCommChannel::chOsd.SendPacket(HD_PACKET_OSD, packet, (Reel::Byte*)(void*)buffer, bcoSize + payloadSize);
    }

    //--------------------------------------------------------------------------------------------------------------

    int          HdFbTrueColorOsd::fontGeneration_ = 0;
    cFont const *HdFbTrueColorOsd::cachedFonts_[HDOSD_MAX_CACHED_FONTS];

    int volatile *HdFbTrueColorOsd::hdCachedFonts_;

    static png_structp png_ptr;
    static png_infop   info_ptr;
    extern int lastThumbWidth[2];
    extern int lastThumbHeight[2];
    extern const char *fbdev;

    //--------------------------------------------------------------------------------------------------------------

#if APIVERSNUM >= 10509 || defined(REELVDR)
    HdFbTrueColorOsd::HdFbTrueColorOsd(int left, int top, uint level)
    :   cOsd(left, top, level),
#else
    HdFbTrueColorOsd::HdFbTrueColorOsd(int left, int top)
    :   cOsd(left, top),
#endif
        /*osdChannel_(Hd::HdCommChannel::Instance().bsc_osd),*/
        dirty_(false)
    {
        hdCachedFonts_ = HdCommChannel::hda->osd_cached_fonts;
        //cachedImages_ = HdCommChannel::hda->osd_cached_images;
        
#if 0
        //TB: mark all images as dirty as they are not cached at startup
        for(int i=0; i<=HdFbTrueColorOsd::MaxImageId; i++)
            imageDirty_[i] = true; 
#endif
        numBitmaps = 0;

        //std::cout << "OSD: " << osd << std::endl;
        if(osd == NULL || (osd->data == NULL && osd->buffer == NULL))
          new_osd();

        if(255 != HdCommChannel::hda->plane[2].alpha) {
            HdCommChannel::hda->plane[2].alpha = 255;
            HdCommChannel::hda->plane[2].changed++;                
        }

        dirtyArea_.x0 = osd->width-1;
        dirtyArea_.y0 = osd->height-1;
        dirtyArea_.x1 = 0;
        dirtyArea_.y1 = 0;

       }


void HdFbTrueColorOsd::new_osd() {
        osd = (osd_t*)malloc(sizeof(osd_t));
 
        if(!osd) {
            std::cout << "ERROR: couldn't malloc in " << __FILE__ << ": " << __LINE__ << std::endl;
            abort();
        }
 
        osd->fd = open(fbdev, O_RDWR|O_NDELAY);
        if(osd->fd==-1)
            osd->fd = open(FB_DEFAULT_DEVICE, O_RDWR|O_NDELAY);
        if(osd->fd==-1)
            std::cout << "couldn't open " << fbdev << ", error: " << strerror(errno) << std::endl;
 
        //assert(fb_fd!=-1);
        struct fb_var_screeninfo screeninfo;
        ioctl(osd->fd, FBIOGET_VSCREENINFO, &screeninfo);
        //printf("bits_per_pixel %d xres %d yres %d\n", screeninfo.bits_per_pixel, screeninfo.xres, screeninfo.yres);
 
        if (screeninfo.bits_per_pixel != 32) {
            screeninfo.bits_per_pixel = 32;
            if(ioctl(osd->fd, FBIOPUT_VSCREENINFO, &screeninfo)) {
                printf("Can't set VSCREENINFO\n");
            } // if
        }

        osd->bpp = screeninfo.bits_per_pixel/8; 
        osd->width = screeninfo.xres;
        osd->height = screeninfo.yres;
 
        osd->data = (unsigned char*) mmap(0, osd->bpp * osd->width * osd->height, PROT_READ|PROT_WRITE, MAP_SHARED, osd->fd, 0);
        if(osd->data == MAP_FAILED ) {
          esyslog("fb mmap failed, allocating dummy storage via malloc\n");
          osd->bpp = 4;
          osd->width = 800;
          osd->height = 576;
          osd->data = (unsigned char*)malloc(osd->bpp * osd->width * osd->height);
        }
 
        //assert(data!=(void*)-1);
 
        osd->buffer = (unsigned char*) malloc(osd->bpp * osd->width * osd->height);
        if(!osd->buffer) {
            std::cout << "ERROR: couldn't malloc in " << __FILE__ << ": " << __LINE__ << std::endl;
            abort();
        }
        if (mySavedRegion == NULL)
            mySavedRegion = (unsigned char*) malloc(osd->bpp*osd->width*osd->height);
        if (mySavedRegion == NULL) {
            printf("[RBM-FB] Can't malloc\n");
            abort();
        }
 
        //printf("FB: xres: %i, yres: %i, bpp: %i, data: %p\n", osd->width, osd->height, osd->bpp, osd->data);
    }

    //--------------------------------------------------------------------------------------------------------------

    HdFbTrueColorOsd::~HdFbTrueColorOsd()
    {

#if APIVERSNUM >= 10509 || defined(REELVDR)
            SetActive(false);
        
#else
        ClearOsd();
#endif

        //close(osd->fd);
        //osd->fd = -1;
        //free(osd->buffer);
        //free(osd);
//    if(mySavedRegion)
//        free(mySavedRegion);
//    mySavedRegion = NULL;
    }
    
    //--------------------------------------------------------------------------------------------------------------
    

#if APIVERSNUM >= 10509 || defined(REELVDR)
    void HdFbTrueColorOsd::SetActive(bool On)
    {
        //printf("%s On=%i\n", __PRETTY_FUNCTION__, On);
        if (On != Active())
        {
            cOsd::SetActive(On);

            ClearOsd(osd);

            // if Active draw
            if (On)
            {
                HdCommChannel::hda->osd_dont_touch|=4;
                // should flush only if something is already drawn
                dirty_ =  true;
                Flush();
            } else {
                HdCommChannel::hda->osd_dont_touch&=~4;
                hdcmd_osd_clear_t const bco = {HDCMD_OSD_CLEAR, 0, 0, 720, 576};
                SendOsdCmd(bco); // Allow Xine to draw its content if available
            }
        }// if

    }
#endif
    //--------------------------------------------------------------------------------------------------------------
    /** Mark the rectangle between (x0, y0) and (x1, y1) as an area that has changed */
    void HdFbTrueColorOsd::UpdateDirty(int x0, int y0, int x1, int y1) {
        if(x0 >= (int)osd->width)  x0 = osd->width-1;
        if(x1 >= (int)osd->width)  x1 = osd->width-1;
        if(y0 >= (int)osd->height) y0 = osd->height-1;
        if(y1 >= (int)osd->height) y1 = osd->height-1;

        if (dirtyArea_.x0 > x0)  dirtyArea_.x0 = x0;
        if (dirtyArea_.y0 > y0)  dirtyArea_.y0 = y0-1; //TB: -1 is needed, 'cause else the top line of the progress bar is missing in the replay display
        if (dirtyArea_.x1 < x1)  dirtyArea_.x1 = x1;
        if (dirtyArea_.y1 < y1)  dirtyArea_.y1 = y1;
    }

    /** Mark the rectangle between (x0, y0) and (x1, y1) as an area that has changed */
    int HdFbTrueColorOsd::CacheFont(cFont const &font)
    {
#if 0 //def REELVDR
        int const fontGen = cFont::GetGeneration();
        if (fontGeneration_ != fontGen)
        {
            ClearFontCache();
            fontGeneration_ = fontGen;
        }
#endif

        // Search cache for font.
        for (int n = 0; n < HDOSD_MAX_CACHED_FONTS; ++n)
        {
            if (!hdCachedFonts_[n])
            {
                cachedFonts_[n] = 0;
            }
            if (&font == cachedFonts_[n])
            {
                // Cache hit, return index.
                return n;
            }
        }

        int const fontHeight = 22; //font.Height();

        if (fontHeight == 0)
        {
            return -1;
        }
        if (fontHeight > 32)
        {
            esyslog("ERROR: HdFbTrueColorOsd font height > 32\n");
            return -1;
        }
        // Font not in cache.
        // Search for empty slot.
        int ind = -1;
        for (int n = 0; n < HDOSD_MAX_CACHED_FONTS; ++n)
        {
            if (!cachedFonts_[n])
            {
                // Empty slot found.
                ind = n;
                cachedFonts_[n] = &font;
                break;
            }
        }
        if (ind == -1)
        {
            // Cache full.
            ClearFontCache();
            ind = 0;
        }
        
        hdcmd_osd_cache_font const bco = {HDCMD_OSD_CACHE_FONT,
                                           ind};

        UInt const charDataSize = (fontHeight + 2) * sizeof(UInt);
        UInt const payloadSize = charDataSize * 255; //cFont::NUMCHARS;

        void *buffer = (void*)malloc(sizeof(hdcmd_osd_cache_font) + payloadSize);

/*        int bufferSize = 0;

        while (!bufferSize)
        {
            bufferSize = ::hd_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   sizeof(hdcmd_osd_cache_font) + payloadSize,
                                                   1000);
        }
*/
        std::memcpy(buffer, &bco, sizeof(hdcmd_osd_cache_font));

#if 0 // def REELVDR
        Byte *p = static_cast<Byte *>(buffer) + sizeof(hdcmd_osd_cache_font);

        for (int c = 0; c < cFont::NUMCHARS; ++c)
        {
            cFont::tCharData const *charData = font.CharData(implicit_cast<unsigned char>(c));
            std::memcpy(p, charData, charDataSize);
            p += charDataSize;
        }
#else
#if 0
        cFont::tCharData *charData;
        for (int c = 0; c < cFont::NUMCHARS; ++c)
        {
          if (c < 32)
          {
            charData->width = 1;
            charData->height = fontHeight;
            for (int i = 0 ; i < 22; i++)
            {
              charData->lines[i] = 0x00000000;
            }
          }
          
          else
          {
            charData->width = FontSml_iso8859_15[c-32][0];
            charData->height = FontSml_iso8859_15[c-32][1];
            for (int i = 0 ; i < 22; i++)
            {
              charData->lines[i] = FontSml_iso8859_15[c-32][i+2];
            }
          }
            
            //cFont::tCharData const *charData = font.CharData(implicit_cast<unsigned char>(c));
            std::memcpy(p, charData, charDataSize);
            p += charDataSize;
        }
#endif
#endif
        //hd_packet_es_data_t packet;
        //printf("DDDD: size: %i\n",  sizeof(hdcmd_osd_cache_font) + payloadSize);
    //HdCommChannel::chOsd.SendPacket(HD_PACKET_OSD, packet, (Reel::Byte*)buffer, sizeof(hdcmd_osd_cache_font) + payloadSize);
        //::hd_channel_write_finish(osdChannel_, bufferSize);
        free(buffer);
        return ind;
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError HdFbTrueColorOsd::CanHandleAreas(tArea const *areas, int numAreas)
    {
//esyslog("HdFbTrueColorOsd: CanHandleAreas\n");

        if (numAreas != 1) {
            esyslog("ERROR: HdFbTrueColorOsd::CanHandleAreas numAreas = %d\n", numAreas);
            return oeTooManyAreas;
        }

        if (areas[0].bpp != 32) {
            esyslog("ERROR: HdFbTrueColorOsd::CanHandleAreas bpp = %d\n", areas[0].bpp);
            return oeBppNotSupported;
        }

        return oeOk;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void HdFbTrueColorOsd::ClearFontCache()
    {
        for (int n = 0; n < HDOSD_MAX_CACHED_FONTS; ++n) {
            cachedFonts_[n] = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdFbTrueColorOsd::ClosePngFile()
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawBitmap(int X,
                                                    int Y,
                                                    const cBitmap &bitmap,
                                                    tColor colorFg,
                                                    tColor colorBg,
                                                    bool replacePalette,
                                                    bool blend)
    {
    //esyslog("HdFbTrueColorOsd: DrawBitmap\n");

        unsigned char const *srcData = bitmap.Data(0,0); //(unsigned char const *)(bco->data);
        unsigned char const *xs;
        unsigned int qx, qy, xt1, yt1, xt, yt, vfx, vfy, vfw, vfh, x, y, w, h, m, *px, n;

        x = X + left;
        y = Y + top;
        w = bitmap.Width();
        h = bitmap.Height();

        if (w+x>=osd->width || y+h>=osd->height)
            return;

        UpdateDirty(x, y, x+w, y+h);

        if (blend) {
            vfx = 0;
            vfy = 0;
            vfw = osd->width;
            vfh = osd->height;
    
            xt1 = vfx * osd->width + x * vfw;
            xt = xt1 / osd->width;
            yt1 = vfy * osd->height + y * vfh;
            yt = yt1 / osd->height;
            qy = yt1 % osd->height;
        
            for(m=0;m<h;) {
                px=(unsigned int*)(osd->buffer + osd->width * yt * osd->bpp  + xt * osd->bpp );
                xs = srcData + m * w;
                qx = xt1 % osd->width;
#ifdef OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(px+w-1))
#endif
                for(n=0;n<w;) {
                    *px++=bitmap.Color(*xs);
                    qx += osd->width;
                    while (qx >= osd->width) {
                        ++xs;
                        ++n;
                        qx -= vfw;
                    }
                }
                ++yt;
                qy += osd->height;
                while (qy >= osd->height) {
                    ++m;
                    qy -= vfh;
                }
            }
        } else {
            unsigned int *bm = (unsigned int*)bitmap.Data(0,0); //bco->data;
            int o=0, val = 0;

            for (m=0;m<h;m++) {
                px=(unsigned int*)(osd->buffer +osd->width*(y+m)*osd->bpp+x*osd->bpp);
#if OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(px+w-1))
#endif
                for(n=0;n<w;n++) {
                    if ((o&3)==0)
                            val=*bm++;
                        *px++=bitmap.Color(val&255);
                        val>>=8;
                        o++;
                }
            }
        }
        dirty_ = true;
    }

    /* override */ void HdFbTrueColorOsd::DrawBitmap32(int X,
                                                    int Y,
                                                    const cBitmap &bitmap,
                                                    tColor colorFg,
                                                    tColor colorBg,
                                                    bool replacePalette,
                                                    bool blend, int width, int height)
    {
        //esyslog("HdFbTrueColorOsd: DrawBitmap\n");

        //printf("HDCMD_OSD_DRAW8_OVERLAY\n");
        unsigned char const *srcData = bitmap.Data(0,0); //(unsigned char const *)(bco->data);
        unsigned char const *xs;
        static unsigned int qx, qy, xt1, yt1, x, y, w, h, *px, line, row;
        static unsigned int pxs;

        x = X + left;
        y = Y + top;
        w = bitmap.Width();
        h = bitmap.Height();
        UpdateDirty(x, y, x+w, y+h);

        xt1 = x * osd->width;
        yt1 = y * osd->height;
        qy = 0;
        
        for(line=0;line<h; y++) {
            px=(unsigned int*)(osd->buffer + osd->width * y * osd->bpp  + x * osd->bpp);
            xs = srcData + line * w;
            qx = xt1 % osd->width;
#ifdef OSD_EXTRA_CHECK
            if(WITHIN_OSD_BORDERS(px+w-1))
#endif
            for(row=0;row<w; px++) {
                pxs = bitmap.Color(*xs);
                if(pxs&0x00FFFFFF && pxs!=0x01ffffff){
                    *px = AlphaBlend(pxs, *px);
                } else {
                    *px = AlphaBlend(*px, pxs);
                }

                if (qx >= 0) {
                    xs++;
                    row++;
                    qx = 0;
                } else
                    qx += osd->width;
            }

            if (qy >= 0) {
                ++line;
                qy = 0;
            } else
                qy += osd->height;
        }
        dirty_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::HdFbTrueColorOsd::DrawBitmapHor(int x, int y, int w, const cBitmap &bitmap)
    {
        esyslog("ERROR: HdFbTrueColorOsd::DrawBitmapHor not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawBitmapVert(int x, int y, int h, const cBitmap &bitmap)
    {
        esyslog("ERROR: HdFbTrueColorOsd::DrawBitmapVert not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawEllipse(int X1, int Y1, int X2, int Y2, tColor color, int quadrants)
    {
    //esyslog("HdFbTrueColorOsd: DrawEllipse\n");

        unsigned int l, t, r, b;
        l = left + X1;
        t = top + Y1;
        r = left + X2 + 1;
        b = top + Y2 + 1;

        int x1 = l;
        int y1 = t;
        int x2 = r - 1;
        int y2 = b - 1;
        UpdateDirty(x1, y1, x2, y2);

        int rx, ry, cx, cy;
        int TwoASquare, TwoBSquare, x, y, XChange, YChange, EllipseError, StoppingX, StoppingY;

        if (!ClipArea(osd, &l, &t, &r, &b))
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
        while (StoppingX >= StoppingY) {
            switch (quadrants) {
                case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, color); // no break
                case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, color); break;
                case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, color); // no break
                case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, color); break;
                case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, color); break;
                case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, color); break;
                case  0:
                case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, color); if (quadrants == 6) break;
                case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, color); break;
                case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, color); break;
                case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, color); break;
                case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, color); break;
                case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, color); break;
            }
            ++ y;
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
            switch (quadrants) {
                case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, color); // no break
                case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, color); break;
                case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, color); // no break
                case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, color); break;
                case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, color); break;
                case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, color); break;
                case  0:
                case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, color); if (quadrants == 6) break;
                case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, color); break;
                case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, color); break;
                case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, color); break;
                case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, color); break;
                case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, color); break;
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
        dirty_ = true;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawImage(UInt imageId, int x, int y, bool blend,
                                                   int horRepeat, int vertRepeat)
    {
    //esyslog("HdFbTrueColorOsd: DrawImage\n");

        if (ImageIdInRange(imageId))
            LoadImage(imageId);

        x += left;
        y += top;

        CachedImage const *img = cachedImages_[imageId]; //hda->osd_cached_images[imageId];
        int m, n, h_, v;
        int w, h;
        uint32_t const *srcPixels;

        if(!img) return;

        if (img) {
            w = img->width;    
            h = img->height;
            unsigned int w_all = horRepeat ? horRepeat * w : w;
            unsigned int h_all = vertRepeat ? vertRepeat * h : h;
            if(horRepeat * w > width) horRepeat = width / w;
            if(vertRepeat * h > height) vertRepeat = height / h;
            UpdateDirty(x, y, x+w_all, y+h_all);

            w_all = horRepeat ? horRepeat * w : w;
            h_all = vertRepeat ? vertRepeat * h : h;

            //printf("X: %i y: %i x2: %i y2: %i vr: %i hr: %i\n", x, y, x+w_all, y+h_all, horRepeat, vertRepeat);

            if (blend) {
                for (v = vertRepeat; v > 0; --v) {
                    srcPixels = img->data;
                    for (n = img->height; n > 0; --n) {
                        uint32_t *tgtPixels = (uint32_t*)(osd->buffer + osd->bpp * osd->width * y++ + x*osd->bpp);
                        for (h_ = horRepeat; h_ > 0; --h_) {
                            uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
                            if(WITHIN_OSD_BORDERS(tgtPixels+w))
#endif
                            for (m = w; m > 0; --m) {
                                int res = AlphaBlend((*src++), (*tgtPixels) );          // where they come from (libpng?) - so let's filter them out

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
                        uint32_t *tgtPixels = (uint32_t*)(osd->buffer + osd->bpp * osd->width * y++ + x*osd->bpp);
                        for (h_ = horRepeat; h_ > 0; --h_) {
                            uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
                            if(WITHIN_OSD_BORDERS(tgtPixels+img->width*(sizeof(int))))
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
        dirty_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawCropImage(UInt imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend)
    {
    //esyslog("HdFbTrueColorOsd: DrawImage\n");

        if (ImageIdInRange(imageId)) {
            LoadImage(imageId);
            

            x+=left;
            y+=top;
            x0+=left;
            y0+=top;
            x1+=left;
            y1+=top;
            UpdateDirty(x0, y0, x1, y1);

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
                if(vertRepeat < 1) vertRepeat = 1;
                if(img->width)
                      horRepeat = (x1-x0)/img->width;
                if(horRepeat < 1) horRepeat = 1;
                if (blend) {
                    int dy=y0;
                    for (v = vertRepeat; v > 0; --v) {
                        srcPixels = img->data + img->width*(y0-y) + (x0-x);
                        for (n = min(height_, img->height); n > 0; --n) {
                            unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * osd->width * dy++ + x0*osd->bpp);
                            for (h = horRepeat; h > 0; --h) {
                                unsigned int const *src = srcPixels;
                                for (m = min(width_, img->width); m > 0; --m) {
                                    *tgtPixels = AlphaBlend((*src++), (*tgtPixels) );
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
                            unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * osd->width * dy++ + x0*osd->bpp);
                            for (h = horRepeat; h > 0; --h) {
                                unsigned int const *src = srcPixels;
                                memcpy(tgtPixels, src, min(width_,img->width)*sizeof(int));
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
        dirty_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawPixel(int x, int y, tColor color)
    {
    //esyslog("HdFbTrueColorOsd: DrawPixel\n");
        esyslog("ERROR: HdFbTrueColorOsd::DrawPixel not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------
    static unsigned int draw_linebuf[1024]={0};
    static unsigned int draw_color=0;

    /* override */ void HdFbTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color)
    {
    //esyslog("HdFbTrueColorOsd: DrawRectangle\n");

        unsigned int l, t, r, b;
        l = left + x1;
        t = top + y1;
        r = left + x2 + 1;
        b = top + y2 + 1;

        if (ClipArea(osd, &l, &t, &r, &b)) {
            UpdateDirty(l, t, r, b);

            unsigned int m, n;
            unsigned int width = r - l;
            unsigned int height = b - t;
            unsigned int *pixels = (unsigned int*)(osd->buffer + osd->width * t *osd->bpp  + l*osd->bpp);

            if (draw_color!=color) {
                for(n=0;n<1024;n++)
                    draw_linebuf[n]=color;
                draw_color=color;
            }
            for (m = height; m; --m) {
#if OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(pixels+width*sizeof(int)))
#endif
                memcpy(pixels,draw_linebuf,width*sizeof(int));
                pixels+=osd->width;
            }
        }
        dirty_ = true;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV)
    {
    //esyslog("HdFbTrueColorOsd: DrawRectangle\n");

        dirty_ = true;

        unsigned int l, t, r, b;
        l = left + x1;
        t = top + y1;
        r = left + x2 + 1;
        b = top + y2 + 1;

        if (ClipArea(osd, &l, &t, &r, &b)) {
            UpdateDirty(l, t, r, b);

            unsigned int m, n;
            unsigned int width = r - l;
            unsigned int height = b - t;
            unsigned int *pixels = (unsigned int*)(osd->buffer + osd->width * t *osd->bpp  + l*osd->bpp);

            if (draw_color!=color) {
                for(n=0;n<1024;n++)
                    draw_linebuf[n]=color;
                draw_color=color;
            }
            for (m = height; m; --m) {
#if OSD_EXTRA_CHECK
                if(WITHIN_OSD_BORDERS(pixels+width*sizeof(int)))
#endif
                memcpy(pixels,draw_linebuf,width*sizeof(int));
                pixels+=osd->width;
            }
        }
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor color, int type)
    {
        esyslog("ERROR: HdFbTrueColorOsd::DrawSlope not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::DrawText(int x, int y, const char *s_in, tColor colorFg, tColor colorBg,
                                                  const cFont *font, int w, int h, int alignment)
    {

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

        UpdateDirty(x, y, x+w, y+h);

        if((colorBg >> 24 == 0) || ((colorBg&0x00ffffff) == 0x00000000)){ /* TB: transparent as bgcolor is evil */
            colorBg = colorFg&0x01ffffff; 
        }

        //printf("DrawText: %s colorFg: %#08x colorBg: %#08x x: %i y: %i w: %i h: %i\n", s_in, colorFg, colorBg, x, y, w, h);

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
                  }
               else { // taCentered
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
                  }
               else { // taCentered
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
                            uint32_t *dstPx = (uint32_t*)(osd->buffer + osd->width * (py+old_y) * osd->bpp  + (px+old_x) * osd->bpp );

                            if (bt == 0xFF)
                               *dstPx = colorFg;
                            else if (TransparentBackground)
                               *dstPx = palette.Blend(colorFg, *dstPx, bt);
                            else if (BlendLevelIndex[bt] >= 0)
                               *dstPx = palette.Blend(palette.Color(BlendLevelIndex[bt]), *dstPx, bt);
                            else
                               *dstPx = palette.Blend(colorFg, *dstPx, bt);

                            }
                         }
                      else { //monochrome rendering
                         for (int col = 0; col < 8 && col + pitch * 8 <= symWidth; col++) {
                             if (bt & 0x80) {
                                uint32_t *dstPx = (uint32_t*)(osd->buffer + osd->width * (old_y + y + row + (font->Height() - ((cFreetypeFont*)font)->Bottom() - symTop)) * osd->bpp  + (old_x + x + col + pitch * 8 + symLeft + kerning) * osd->bpp );
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
    }
   
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::Flush()
    {
#if APIVERSNUM >= 10509 || defined(REELVDR)
        if (! Active()) return ;
#endif
        //if (dirty_)
       // {
        //    static int flushCount = 1;

            // ::printf("HdFbTrueColorOsd::Flush()\n");

        //DrawBitmap32(/*old_x, old_y*/ 0,0 /*bitmaps[0]->X0(), bitmaps[0]->Y0()*/, *bitmaps[0], old_colorFg, old_colorBg, false, false);

            //hdcmd_osd_flush const bco = {HDCMD_OSD_FLUSH, flushCount};

            //SendOsdCmd(bco);
            FlushOsd(osd);
            dirty_ = false;

#if 0
            int loop = 100;
            int osdFlushCount;
            do
            {
                ::usleep(10*1000);
                osdFlushCount = HdCommChannel::hda->osd_flush_count;
            }
            while (--loop && flushCount != osdFlushCount /*&& flushCount != osdFlushCount + 1*/);

            ++ flushCount;
#endif

        //}
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ cBitmap *HdFbTrueColorOsd::GetBitmap(int area)
    {
#if 1
        return bitmaps[area];
#endif
        esyslog("ERROR: HdFbTrueColorOsd::GetBitmap not supported\n");
        return 0;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    bool HdFbTrueColorOsd::ImageIdInRange(UInt imageId)
    {

        if (imageId > MaxImageId) {
            esyslog("ERROR: HdFbTrueColorOsd Image id %u: Out of range.\n", imageId);
            return false;
        } else {
            return true;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    bool HdFbTrueColorOsd::OpenPngFile(char const         *path,
                                      Byte **&rows,
                                      int                &width,
                                      int                &height)
    {

        Byte header[8];

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
            png_destroy_read_struct(&png_ptr,
                                    NULL, NULL);
            fclose(fp);
            return false;
        }
    
        png_infop end_info = png_create_info_struct(png_ptr);
        if (!end_info) {
            png_destroy_read_struct(&png_ptr, &info_ptr,
                                    (png_infopp)NULL);
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

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::RestoreRegion()
    {
        int x1 = savedRegion_x0; int y1 = savedRegion_y0; int x2 = savedRegion_x1; int y2 = savedRegion_y1+1;
        int lines = y2 - y1+1;    
        int pixels = x2 - x1;
        int i;
        if (pixels > 0)
            for(i = 0; i < lines; i++)
                memcpy(osd->buffer + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, mySavedRegion + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, pixels*osd->bpp);
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdFbTrueColorOsd::SaveRegion(int x1, int y1, int x2, int y2)
    {
        x1 += left; y1 += top; x2 += left; y2 += top;
        savedRegion_x0 = x1; savedRegion_y0 = y1; savedRegion_x1 = x2; savedRegion_y1 = y2;
        int lines = y2 - y1;    
        int pixels = x2 - x1;
        int i;
        if (pixels > 0)
            for(i = 0; i < lines; i++)
                memcpy(mySavedRegion + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, osd->buffer + (i+y1)*osd->width*osd->bpp + x1*osd->bpp, pixels*osd->bpp);
    }

    //--------------------------------------------------------------------------------------------------------------

    bool HdFbTrueColorOsd::LoadImage(UInt imageId)
    {
        //printf("LoadImage() ID: %i\n", imageId);
        if (cachedImages_[imageId]) {
	    if(cachedImages_[imageId]->data && !imageDirty_[imageId])
                return false; /* Already cached. */
	    if(cachedImages_[imageId]->data) free(cachedImages_[imageId]->data);
	    free(cachedImages_[imageId]);
	    cachedImages_[imageId]=0;
        }
        /* Not cached, load it. */
        std::string const &path_ = imagePaths_[imageId];
        if (path_.empty()) {
            printf("ERROR: Image id %u: No path set.\n", imageId);
            return false;
        }
        const char *path = path_.c_str();

        unsigned char **rows;
        int  width, height;

        if (OpenPngFile(path, rows, width, height)) {
            void *buffer = malloc(width * height * sizeof(u_int));
            CachedImage *img = (CachedImage*)malloc(sizeof(CachedImage));
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
            printf("ERROR: failed opening %s\n", path);

        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError HdFbTrueColorOsd::SetAreas(tArea const *areas, int numAreas)
    {

        eOsdError ret = CanHandleAreas(areas, numAreas);
        if (ret == oeOk)
        {
            int l = areas->x1;
            int t = areas->y1;
            int r = areas->x2 + 1;
            int b = areas->y2 + 1;

            l = std::max(0, l);
            t = std::max(0, l);
            width = r - l;
            height = b - t;
            width = std::max(1, width);
            height = std::max(1, height);
        }
        return ret;

    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError HdFbTrueColorOsd::SetPalette(const cPalette &palette, int area)
    {
        esyslog("ERROR: HdFbTrueColorOsd::SetPalette not supported\n");
        return oeOk;
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdFbTrueColorOsd::SetImagePath(UInt imageId, char const *path)
    {

        if (ImageIdInRange(imageId) && strcmp(imagePaths_[imageId].c_str(), path)) {
            imagePaths_[imageId] = path;
            imageDirty_[imageId] = true;
        }

    }

    //--------------------------------------------------------------------------------------------------------------
}
