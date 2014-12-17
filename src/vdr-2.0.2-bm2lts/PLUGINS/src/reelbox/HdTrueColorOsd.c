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

// HdTrueColorOsd.c


#include "HdTrueColorOsd.h"

#include <vdr/tools.h>

#include <cstring>
#include <algorithm>

#include <png.h>


namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------
    static cBitmap *cacheBitmap;

    int          HdTrueColorOsd::fontGeneration_ = 0;
    cFont const *HdTrueColorOsd::cachedFonts_[HDOSD_MAX_CACHED_FONTS];

    int volatile *HdTrueColorOsd::hdCachedFonts_;

    int volatile  *HdTrueColorOsd::cachedImages_;
    std::string    HdTrueColorOsd::imagePaths_[HdTrueColorOsd::MaxImageId + 1];
    bool           HdTrueColorOsd::imageDirty_[HdTrueColorOsd::MaxImageId + 1];

    static png_structp png_ptr;
    static png_infop   info_ptr;
    int lastThumbWidth[2];
    int lastThumbHeight[2];

    //--------------------------------------------------------------------------------------------------------------

#if APIVERSNUM >= 10509 || defined(REELVDR)
    HdTrueColorOsd::HdTrueColorOsd(int left, int top, uint level)
    :   cOsd(left, top, level),
#else
    HdTrueColorOsd::HdTrueColorOsd(int left, int top)
    :   cOsd(left, top),
#endif
        /*osdChannel_(Hd::HdCommChannel::Instance().bsc_osd),*/
        dirty_(false)
    {
        hdCachedFonts_ = HdCommChannel::hda->osd_cached_fonts;
        cachedImages_ = HdCommChannel::hda->osd_cached_images;
        
#if 0
        //TB: mark all images as dirty as they are not cached at startup
        for(int i=0; i<=HdTrueColorOsd::MaxImageId; i++)
            imageDirty_[i] = true; 
#endif
        numBitmaps = 0;

	cacheBitmap = new cBitmap(720, 576, 8, 0, 0);

    }

    //--------------------------------------------------------------------------------------------------------------

    HdTrueColorOsd::~HdTrueColorOsd()
    {

#if APIVERSNUM >= 10509 || defined(REELVDR)
            SetActive(false);
        
#else
        hdcmd_osd_clear_t const bco = {HDCMD_OSD_CLEAR,
                                        0, 0, 720, 576};
        SendOsdCmd(bco);
#endif

        delete cacheBitmap;

    }
    
    //--------------------------------------------------------------------------------------------------------------
    

#if APIVERSNUM >= 10509 || defined(REELVDR)
    void HdTrueColorOsd::SetActive(bool On)
    {
        //printf("%s On=%i\n", __PRETTY_FUNCTION__, On);
        if (On != Active())
        {
            cOsd::SetActive(On);

            // clear 
            hdcmd_osd_clear_t const bco = {HDCMD_OSD_CLEAR, 0, 0, 720, 576};
            SendOsdCmd(bco);

            // if Active draw
            if (On)
            {
                // should flush only if something is already drawn
                dirty_ =  true;
                Flush();
            }
        }// if

    }
#endif
    //--------------------------------------------------------------------------------------------------------------

    int HdTrueColorOsd::CacheFont(cFont const &font)
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
            esyslog("ERROR: HdTrueColorOsd font height > 32\n");
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
        hd_packet_es_data_t packet;
        //printf("DDDD: size: %i\n",  sizeof(hdcmd_osd_cache_font) + payloadSize);
	HdCommChannel::chOsd.SendPacket(HD_PACKET_OSD, packet, (Reel::Byte*)buffer, sizeof(hdcmd_osd_cache_font) + payloadSize);
        //::hd_channel_write_finish(osdChannel_, bufferSize);
        free(buffer);
        return ind;
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdTrueColorOsd::CacheImage(UInt imageId)
    {
	//esyslog("HdTrueColorOsd: CacheImage()\n");

        if (cachedImages_[imageId] && !imageDirty_[imageId])
        {
            // Already cached.
            return;
        }

        // Not cached, load it.
        std::string const &path = imagePaths_[imageId];

        if (path.empty())
        {
            esyslog("ERROR: HdTrueColorOsd Image id %u: No path set.\n", imageId);
            return;
        }

        if (SendImage(imageId, path.c_str()))
        {
            // Wait a limited time for the HD to receive the image.
            // (The HD may uncache other cached images in order to free memory,
            //  If we don't wait we may subsequently draw one of those, without uploading it before)
#if 0 
            for (int n = 0; n < 400 && !cachedImages_[imageId]; ++n)
            {
                ::usleep(5000);
            }
#endif
        }
        else
        {
            esyslog("ERROR: HdTrueColorOsd Image id %u: Unable to load image '%s'.\n", imageId, path.c_str());
        }
        imageDirty_[imageId] = false;


    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError HdTrueColorOsd::CanHandleAreas(tArea const *areas, int numAreas)
    {
//esyslog("HdTrueColorOsd: CanHandleAreas\n");

        if (numAreas != 1)
        {
            esyslog("ERROR: HdTrueColorOsd::CanHandleAreas numAreas = %d\n", numAreas);
            return oeTooManyAreas;
        }

        if (areas[0].bpp != 32)
        {
            esyslog("ERROR: HdTrueColorOsd::CanHandleAreas bpp = %d\n", areas[0].bpp);
            return oeBppNotSupported;
        }

        return oeOk;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void HdTrueColorOsd::ClearFontCache()
    {
        for (int n = 0; n < HDOSD_MAX_CACHED_FONTS; ++n)
        {
            cachedFonts_[n] = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdTrueColorOsd::ClosePngFile()
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawBitmap(int x,
                                                    int y,
                                                    const cBitmap &bitmap,
                                                    tColor colorFg,
                                                    tColor colorBg,
                                                    bool replacePalette,
                                                    bool blend)
    {
	//esyslog("HdTrueColorOsd: DrawBitmap\n");

       REEL_ASSERT(!overlay);

        // Send the palette.
        int numColors;
        tColor const *colors = bitmap.Colors(numColors);

        UInt const payloadSize = numColors * sizeof(UInt);

        // int bufferSize = 0;
        void *buffer = malloc(sizeof(hdcmd_osd_palette_t) + payloadSize);
/*        while (!bufferSize)
        {
            bufferSize = ::hd_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   sizeof(hdcmd_osd_palette_t) + payloadSize,
                                                   1000);
        }*/

        hdcmd_osd_palette_t *bco = (hdcmd_osd_palette_t *)(buffer);
        bco->cmd   = HDCMD_OSD_PALETTE;
        bco->count = numColors;

        for (int n = 0; n < numColors; ++n)
        {
            bco->palette[n] = colors[n];
        }

//        ::hd_channel_write_finish(osdChannel_, bufferSize);

        // Send the palette indexes.
        SendOsdCmd(bco, sizeof(hdcmd_osd_palette_t) + payloadSize);

        hdcmd_osd_draw8_t bco2 = {HDCMD_OSD_DRAW8, left + x, top + y, bitmap.Width(), bitmap.Height(), blend};

        SendOsdCmd(&bco2, sizeof(hdcmd_osd_draw8_t), bitmap.Data(0, 0), bitmap.Width() * bitmap.Height());
        free(buffer);
        dirty_ = true;

    }

    /* override */ void HdTrueColorOsd::DrawBitmap32(int x,
                                                    int y,
                                                    const cBitmap &bitmap,
                                                    tColor colorFg,
                                                    tColor colorBg,
                                                    bool replacePalette,
                                                    bool blend, int width, int height)
    {
	//esyslog("HdTrueColorOsd: DrawBitmap\n");

        // Send the palette.
        int numColors;
        tColor const *colors = bitmap.Colors(numColors);

        UInt const payloadSize = numColors * sizeof(UInt);

        // int bufferSize = 0;
        void *buffer = malloc(sizeof(hdcmd_osd_palette_t) + payloadSize);
/*        while (!bufferSize)
        {
            bufferSize = ::hd_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   sizeof(hdcmd_osd_palette_t) + payloadSize,
                                                   1000);
        }*/

        hdcmd_osd_palette_t *bco = (hdcmd_osd_palette_t *)(buffer);
        bco->cmd   = HDCMD_OSD_PALETTE;
        bco->count = numColors;

        for (int n = 0; n < numColors; ++n)
        {
            bco->palette[n] = colors[n];
        }

//        ::hd_channel_write_finish(osdChannel_, bufferSize);

        // Send the palette indexes.
        SendOsdCmd(bco, sizeof(hdcmd_osd_palette_t) + payloadSize);

        hdcmd_osd_draw8_t bco2 = {HDCMD_OSD_DRAW8_OVERLAY, left + x, top + y, width, height, blend};

        SendOsdCmd(&bco2, sizeof(hdcmd_osd_draw8_t), bitmap.Data(0, 0), width * height);
        free(buffer);
        dirty_ = true;

    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::HdTrueColorOsd::DrawBitmapHor(int x, int y, int w, const cBitmap &bitmap)
    {
        esyslog("ERROR: HdTrueColorOsd::DrawBitmapHor not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawBitmapVert(int x, int y, int h, const cBitmap &bitmap)
    {
        esyslog("ERROR: HdTrueColorOsd::DrawBitmapVert not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor color, int quadrants)
    {
	//esyslog("HdTrueColorOsd: DrawEllipse\n");

        hdcmd_osd_draw_ellipse const bco = {HDCMD_OSD_DRAW_ELLIPSE,
                                             left + x1,
                                             top + y1,
                                             left + x2 + 1,
                                             top + y2 + 1,
                                             color,
                                             quadrants};

        SendOsdCmd(bco);

        dirty_ = true;

    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawImage(UInt imageId, int x, int y, bool blend,
                                                   int horRepeat, int vertRepeat)
    {
	//esyslog("HdTrueColorOsd: DrawImage\n");

        if (ImageIdInRange(imageId))
        {
            CacheImage(imageId);
            hdcmd_osd_draw_image const bco = {HDCMD_OSD_DRAW_IMAGE,
                                               imageId,
                                               left + x, top + y,
                                               blend,
                                               horRepeat, vertRepeat};
    
            SendOsdCmd(bco);
        }
        dirty_ = true;

    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawCropImage(UInt imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend)
    {
	//esyslog("HdTrueColorOsd: DrawImage\n");

        if (ImageIdInRange(imageId))
        {
            CacheImage(imageId);
            hdcmd_osd_draw_crop_image const bco = {HDCMD_OSD_DRAW_CROP_IMAGE,
                                               imageId,
                                               left + x, top + y,
                                               left + x0, top + y0,
                                               left + x1, top + y1,
                                               blend};
            SendOsdCmd(bco);
        }
        dirty_ = true;

    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawPixel(int x, int y, tColor color)
    {
	//esyslog("HdTrueColorOsd: DrawPixel\n");
        esyslog("ERROR: HdTrueColorOsd::DrawPixel not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color)
    {
	//esyslog("HdTrueColorOsd: DrawRectangle\n");

  hdcmd_osd_draw_rect const bco = {HDCMD_OSD_DRAW_RECT,
                                          left + x1,
                                          top + y1,
                                          left + x2 + 1,
                                          top + y2 + 1,
                                          color};

        SendOsdCmd(bco);
        dirty_ = true;

    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV)
    {
	//esyslog("HdTrueColorOsd: DrawRectangle\n");

  hdcmd_osd_draw_rect2 const bco = {HDCMD_OSD_DRAW_RECT2,
                                          left + x1,
                                          top + y1,
                                          left + x2 + 1,
                                          top + y2 + 1,
                                          color,
                                          alphaGradH,
                                          alphaGradV,
                                          alphaGradStepH,
                                          alphaGradStepV};

        SendOsdCmd(bco);
        dirty_ = true;

    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor color, int type)
    {
        esyslog("ERROR: HdTrueColorOsd::DrawSlope not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::DrawText(int x,
                                                  int y,
                                                  const char *s_in,
                                                  tColor colorFg,
                                                  tColor colorBg,
                                                  const cFont *font,
                                                  int width,
                                                  int height,
                                                  int alignment)
    {

      if (s_in)
      {
        /* check for empty string */
        unsigned int i;
	for(i=0; i<strlen(s_in); i++){
		if(s_in[i] == ' ')
			continue;
		else
			break;
	}
	if(i == strlen(s_in))
		return;
        if(width==0) {
		width = font->Width(s_in);
	}
        if(height == 0)
	     height=font->Height();

        cacheBitmap->SetSizeWithoutRealloc(width, height); 

        if((colorBg >> 24 == 0) || ((colorBg&0x00ffffff) == 0x00000000)){ /* TB: transparent as bgcolor is evil */
		colorBg = colorFg&0x01ffffff; 
        }

        cacheBitmap->DrawText(0, 0, s_in, colorFg, colorBg /*clrTransparent*/, font, width, height, alignment);
        DrawBitmap32(x, y, *cacheBitmap, colorFg, /*colorBg*/ clrTransparent, false, false, width, height);
        //printf("DrawText: %s colorFg: %#08x colorBg: %#08x x: %i y: %i w: %i h: %i\n", s, colorFg, colorBg, x, y, width, height);
      }
    }
   
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::Flush()
    {
#if APIVERSNUM >= 10509 || defined(REELVDR)
        if (! Active()) return ;
#endif
        if (dirty_)
        {
            static int flushCount = 1;

            // ::printf("HdTrueColorOsd::Flush()\n");

        //DrawBitmap32(/*old_x, old_y*/ 0,0 /*bitmaps[0]->X0(), bitmaps[0]->Y0()*/, *bitmaps[0], old_colorFg, old_colorBg, false, false);

            hdcmd_osd_flush const bco = {HDCMD_OSD_FLUSH, flushCount};

            SendOsdCmd(bco);
            dirty_ = false;

#if 1
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

        }
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ cBitmap *HdTrueColorOsd::GetBitmap(int area)
    {
#if 1
        return bitmaps[area];
#endif
        esyslog("ERROR: HdTrueColorOsd::GetBitmap not supported\n");
        return 0;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    bool HdTrueColorOsd::ImageIdInRange(UInt imageId)
    {

        if (imageId > MaxImageId)
        {
            esyslog("ERROR: HdTrueColorOsd Image id %u: Out of range.\n", imageId);
            return false;
        }
        else

        {
            return true;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    bool HdTrueColorOsd::OpenPngFile(char const         *path,
                                      Byte **&rows,
                                      int                &width,
                                      int                &height)
    {

        Byte header[8];

        FILE *fp = fopen(path, "rb");
        if (!fp || fread(header, 1, 8, fp) != 8)
        {
            if(fp)
                fclose(fp);
            return false;
        }

        if (png_sig_cmp(header, 0, 8))
        {
            fclose(fp);
            return false;
        }

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                     NULL, NULL, NULL);
        if (!png_ptr)
        {
            fclose(fp);
            return false;
        }
    
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr,
                                    NULL, NULL);
            fclose(fp);
            return false;
        }
    
        png_infop end_info = png_create_info_struct(png_ptr);
        if (!end_info)
        {
            png_destroy_read_struct(&png_ptr, &info_ptr,
                                    (png_infopp)NULL);
            fclose(fp);
            return false;
        }

        png_init_io(png_ptr, fp);

        png_set_sig_bytes(png_ptr, 8);

        if (setjmp(png_jmpbuf(png_ptr)))
        {
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

    /* override */ void HdTrueColorOsd::RestoreRegion()
    {
        hdcmd_osd_clear_t const bco = {HDCMD_OSD_RESTORE_REGION, 0, 0, 0, 0};
        SendOsdCmd(bco);
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void HdTrueColorOsd::SaveRegion(int x1, int y1, int x2, int y2)
    {
        hdcmd_osd_clear_t const bco = {HDCMD_OSD_SAVE_REGION, x1+left, y1+top, x2+left, y2+top};
        SendOsdCmd(bco);
    }

    //--------------------------------------------------------------------------------------------------------------

    bool HdTrueColorOsd::SendImage(UInt imageId, char const *path)
    {

        // Only 32-Bit ARGB-PNGs are supported.
        Byte **rows;
        Int  width, height;

        if (OpenPngFile(path, rows, width, height))
        {
            // Send the image to the HD.
            // int bufferSize = 0;
    
            hdcmd_osd_cache_image bco = {HDCMD_OSD_CACHE_IMAGE,
                                          imageId,
                                          width, height};

            UInt const payloadSize = width * height * sizeof(UInt);

            void *buffer = malloc(sizeof(hdcmd_osd_cache_image) + payloadSize);
	    /*
            while (!bufferSize)
            {
                bufferSize = ::hd_channel_write_start(osdChannel_,
                                                       &buffer,
                                                       sizeof(hdcmd_osd_cache_image) + payloadSize,
                                                       1000);
            }*/
    
            std::memcpy(buffer, &bco, sizeof(hdcmd_osd_cache_image));
    
            Byte *p = static_cast<Byte *>(buffer) + sizeof(hdcmd_osd_cache_image);
    
            for (int y = 0; y < height; ++y)
            {
                Byte *r = rows[y];
                for (int x = 0; x < width; ++x)
                {
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
    
            //::hd_channel_write_finish(osdChannel_, bufferSize);

            hd_packet_es_data_t packet;
            HdCommChannel::chOsd.SendPacket(HD_PACKET_OSD, packet, (Reel::Byte*)buffer, sizeof(hdcmd_osd_cache_image) + payloadSize);

            ClosePngFile();
            free(buffer);

	    if (imageId == 119) {  /* thumbnails */
		lastThumbWidth[0] = width;
		lastThumbHeight[0] = height;
	    } else if (imageId == 120) {
                lastThumbWidth[1] = width;
                lastThumbHeight[1] = height;
            }

            return true;
        }


        return false;
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdTrueColorOsd::SendOsdCmd(void const *bco, UInt bcoSize, void const *payload, UInt payloadSize)
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

    /* override */ eOsdError HdTrueColorOsd::SetAreas(tArea const *areas, int numAreas)
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

    /* override */ eOsdError HdTrueColorOsd::SetPalette(const cPalette &palette, int area)
    {
        esyslog("ERROR: HdTrueColorOsd::SetPalette not supported\n");
        return oeOk;
    }

    //--------------------------------------------------------------------------------------------------------------

    void HdTrueColorOsd::SetImagePath(UInt imageId, char const *path)
    {

        if (ImageIdInRange(imageId))
        {
            imagePaths_[imageId] = path;
            imageDirty_[imageId] = true;
        }

    }

    //--------------------------------------------------------------------------------------------------------------
}
