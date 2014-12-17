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

// BspTrueColorOsd.c

#include "BspTrueColorOsd.h"

#include <vdr/tools.h>

#include <algorithm>
#include <cstring>

#include <png.h>

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    int          BspTrueColorOsd::fontGeneration_ = 0;
    cFont const *BspTrueColorOsd::cachedFonts_[BSPOSD_MAX_CACHED_FONTS];

    int volatile *BspTrueColorOsd::bspCachedFonts_;

    int volatile  *BspTrueColorOsd::cachedImages_;
    std::string    BspTrueColorOsd::imagePaths_[BspTrueColorOsd::MaxImageId + 1];
    bool           BspTrueColorOsd::imageDirty_[BspTrueColorOsd::MaxImageId + 1];

    static png_structp png_ptr;
    static png_infop   info_ptr;

    //--------------------------------------------------------------------------------------------------------------

#if APIVERSNUM >= 10509 || defined(REELVDR)
    BspTrueColorOsd::BspTrueColorOsd(int left, int top, uint level)
    :   cOsd(left, top, level),
#else
    BspTrueColorOsd::BspTrueColorOsd(int left, int top)
    :   cOsd(left, top),
#endif
        osdChannel_(Bsp::BspCommChannel::Instance().bsc_osd),
        dirty_(false)
    {
        bspCachedFonts_ = Bsp::BspCommChannel::Instance().bspd->osd_cached_fonts;
        cachedImages_ = Bsp::BspCommChannel::Instance().bspd->osd_cached_images;
    }

    //--------------------------------------------------------------------------------------------------------------

    BspTrueColorOsd::~BspTrueColorOsd()
    {
        bspcmd_osd_clear_t const bco = {BSPCMD_OSD_CLEAR,
                                        0, 0, 720, 576};
        SendOsdCmd(bco);
    }
    
    //--------------------------------------------------------------------------------------------------------------

    int BspTrueColorOsd::CacheFont(cFont const &font)
    {
#if 0
        int const fontGen = cFont::GetGeneration();
        if (fontGeneration_ != fontGen)
        {
            ClearFontCache();
            fontGeneration_ = fontGen;
        }

        // Search cache for font.
        for (int n = 0; n < BSPOSD_MAX_CACHED_FONTS; ++n)
        {
            if (!bspCachedFonts_[n])
            {
                cachedFonts_[n] = 0;
            }
            if (&font == cachedFonts_[n])
            {
                // Cache hit, return index.
                return n;
            }
        }

        int const fontHeight = font.Height();
        if (fontHeight == 0)
        {
            return -1;
        }
        if (fontHeight > 32)
        {
            esyslog("ERROR: BspTrueColorOsd font height > 32\n");
            return -1;
        }
        // Font not in cache.
        // Search for empty slot.
        int ind = -1;
        for (int n = 0; n < BSPOSD_MAX_CACHED_FONTS; ++n)
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
        
        bspcmd_osd_cache_font const bco = {BSPCMD_OSD_CACHE_FONT,
                                           ind};

        UInt const charDataSize = (fontHeight + 2) * sizeof(UInt);
        UInt const payloadSize = charDataSize * cFont::NUMCHARS;

        void *buffer;

        int bufferSize = 0;

        while (!bufferSize)
        {
            bufferSize = ::bsp_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   sizeof(bspcmd_osd_cache_font) + payloadSize,
                                                   1000);
        }

        std::memcpy(buffer, &bco, sizeof(bspcmd_osd_cache_font));

        Byte *p = static_cast<Byte *>(buffer) + sizeof(bspcmd_osd_cache_font);

        for (int c = 0; c < cFont::NUMCHARS; ++c)
        {
            cFont::tCharData const *charData = font.CharData(implicit_cast<unsigned char>(c));
            std::memcpy(p, charData, charDataSize);
            p += charDataSize;
        }

        ::bsp_channel_write_finish(osdChannel_, bufferSize);
        return ind;
#else
        return 0;
#endif
    }

    //--------------------------------------------------------------------------------------------------------------

    void BspTrueColorOsd::CacheImage(UInt imageId)
    {
        if (cachedImages_[imageId] && !imageDirty_[imageId])
        {
            // Already cached.
            return;
        }

        // Not cached, load it.
        std::string const &path = imagePaths_[imageId];

        if (path.empty())
        {
            esyslog("ERROR: BspTrueColorOsd Image id %u: No path set.\n", imageId);
            return;
        }

        if (SendImage(imageId, path.c_str()))
        {
            // Wait a limited time for the BSP to receive the image.
            // (The BSP may uncache other cached images in order to free memory,
            //  If we don't wait we may subsequently draw one of those, without uploading it before)
            for (int n = 0; n < 400 && !cachedImages_[imageId]; ++n)
            {
                ::usleep(5000);
            }
        }
        else
        {
            esyslog("ERROR: BspTrueColorOsd Image id %u: Unable to load image '%s'.\n", imageId, path.c_str());
        }
        imageDirty_[imageId] = false;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError BspTrueColorOsd::CanHandleAreas(tArea const *areas, int numAreas)
    {
        if (numAreas != 1)
        {
            esyslog("ERROR: BspTrueColorOsd::CanHandleAreas numAreas = %d\n", numAreas);
            return oeTooManyAreas;
        }

        if (areas[0].bpp != 32)
        {
            esyslog("ERROR: BspTrueColorOsd::CanHandleAreas bpp = %d\n", areas[0].bpp);
            return oeBppNotSupported;
        }

        return oeOk;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    void BspTrueColorOsd::ClearFontCache()
    {
        for (int n = 0; n < BSPOSD_MAX_CACHED_FONTS; ++n)
        {
            cachedFonts_[n] = 0;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    void BspTrueColorOsd::ClosePngFile()
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawBitmap(int x,
                                                    int y,
                                                    const cBitmap &bitmap,
                                                    tColor colorFg,
                                                    tColor colorBg,
                                                    bool replacePalette,
                                                    bool overlay)
    {
        REEL_ASSERT(!overlay);

        // Send the palette.
        int numColors;
        tColor const *colors = bitmap.Colors(numColors);

        UInt const payloadSize = numColors * sizeof(UInt);

        int bufferSize = 0;
        void *buffer;
        while (!bufferSize)
        {
            bufferSize = ::bsp_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   sizeof(bspcmd_osd_palette_t) + payloadSize,
                                                   1000);
        }

        bspcmd_osd_palette_t &bco = *static_cast<bspcmd_osd_palette_t *>(buffer);
        bco.cmd   = BSPCMD_OSD_PALETTE;
        bco.count = numColors;

        for (int n = 0; n < numColors; ++n)
        {
            bco.palette[n] = colors[n] ^ 0xFF000000; // Invert alpha.
        }

        ::bsp_channel_write_finish(osdChannel_, bufferSize);

        // Send the palette indexes.

        bspcmd_osd_draw8_t bco2 = {BSPCMD_OSD_DRAW8, left + x, top + y, bitmap.Width(), bitmap.Height(), 0};

        SendOsdCmd(&bco2, sizeof(bspcmd_osd_draw8_t), bitmap.Data(0, 0), bitmap.Width() * bitmap.Height());
        dirty_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::BspTrueColorOsd::DrawBitmapHor(int x, int y, int w, const cBitmap &bitmap)
    {
        esyslog("ERROR: BspTrueColorOsd::DrawBitmapHor not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawBitmapVert(int x, int y, int h, const cBitmap &bitmap)
    {
        esyslog("ERROR: BspTrueColorOsd::DrawBitmapVert not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor color, int quadrants)
    {
        bspcmd_osd_draw_ellipse const bco = {BSPCMD_OSD_DRAW_ELLIPSE,
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

    /* override */ void BspTrueColorOsd::DrawImage(UInt imageId, int x, int y, bool blend,
                                                   int horRepeat, int vertRepeat)
    {
        if (ImageIdInRange(imageId))
        {
            CacheImage(imageId);
            bspcmd_osd_draw_image const bco = {BSPCMD_OSD_DRAW_IMAGE,
                                               imageId,
                                               left + x, top + y,
                                               blend,
                                               horRepeat, vertRepeat};
    
            SendOsdCmd(bco);
        }
        dirty_ = true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawPixel(int x, int y, tColor color)
    {
        esyslog("ERROR: BspTrueColorOsd::DrawPixel not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color)
    {
        bspcmd_osd_draw_rect const bco = {BSPCMD_OSD_DRAW_RECT,
                                          left + x1,
                                          top + y1,
                                          left + x2 + 1,
                                          top + y2 + 1,
                                          color};

        SendOsdCmd(bco);
        dirty_ = true;
    }
   
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV)
    {
	//esyslog("HdTrueColorOsd: DrawRectangle\n");

    bspcmd_osd_draw_rect2 const bco = {BSPCMD_OSD_DRAW_RECT2,
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

    /* override */ void BspTrueColorOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor color, int type)
    {
        esyslog("ERROR: BspTrueColorOsd::DrawSlope not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::DrawText(int x,
                                                  int y,
                                                  const char *s,
                                                  tColor colorFg,
                                                  tColor colorBg,
                                                  const cFont *font,
                                                  int width,
                                                  int height,
                                                  int alignment)
    {
        if (s)
        {
            int fontInd = CacheFont(*font);
            if (fontInd >= 0)
            {
                bspcmd_osd_draw_text const bco = {BSPCMD_OSD_DRAW_TEXT,
                                                  x + left, y + top,
                                                  colorFg, colorBg,
                                                  fontInd,
                                                  width, height,
                                                  alignment};
        
                SendOsdCmd(bco, s, std::strlen(s) + 1);
                dirty_ = true;
            }
        }
    }
   
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::Flush()
    {
        if (dirty_)
        {
            static int flushCount = 1;

            // ::printf("BspTrueColorOsd::Flush()\n");

            bspcmd_osd_flush const bco = {BSPCMD_OSD_FLUSH, flushCount};
    
            SendOsdCmd(bco);
            dirty_ = false;

            int loop = 200;

            int osdFlushCount;
            do
            {
                ::usleep(5000);
                osdFlushCount = Bsp::BspCommChannel::Instance().bspd->osd_flush_count;
            }
            while (--loop && flushCount != osdFlushCount /*&& flushCount != osdFlushCount + 1*/);

            ++ flushCount;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ cBitmap *BspTrueColorOsd::GetBitmap(int area)
    {
        esyslog("ERROR: BspTrueColorOsd::GetBitmap not supported\n");
        return 0;
    }
    
    //--------------------------------------------------------------------------------------------------------------

    bool BspTrueColorOsd::ImageIdInRange(UInt imageId)
    {
        if (imageId > MaxImageId)
        {
            esyslog("ERROR: BspTrueColorOsd Image id %u: Out of range.\n", imageId);
            return false;
        }
        else
        {
            return true;
        }
    }

    //--------------------------------------------------------------------------------------------------------------

    bool BspTrueColorOsd::OpenPngFile(char const         *path,
                                      Byte const *const *&rows,
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
    
        /*png_infop end_info = png_create_info_struct(png_ptr);
        if (!end_info)
        {
            png_destroy_read_struct(&png_ptr, &info_ptr,
                                    (png_infopp)NULL);
            fclose(fp);
            return false;
        }*/

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            ::printf("setjmp err\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return false;
        }

        png_init_io(png_ptr, fp);

        png_set_sig_bytes(png_ptr, 8);

        png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

        rows = png_get_rows(png_ptr, info_ptr);
        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);

        fclose(fp);
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::RestoreRegion()
    {
        esyslog("ERROR: BspTrueColorOsd::RestoreRegion not supported\n");
    }
    
    //--------------------------------------------------------------------------------------------------------------

    /* override */ void BspTrueColorOsd::SaveRegion(int x1, int y1, int x2, int y2)
    {
        esyslog("ERROR: BspTrueColorOsd::SaveRegion not supported\n");
    }

    //--------------------------------------------------------------------------------------------------------------

    bool BspTrueColorOsd::SendImage(UInt imageId, char const *path)
    {
        // Only 32-Bit ARGB-PNGs are supported.

        Byte const *const *rows;
        Int  width, height;

        if (OpenPngFile(path, rows, width, height))
        {
            // Send the image to the BSP.
            int bufferSize = 0;
    
            bspcmd_osd_cache_image bco = {BSPCMD_OSD_CACHE_IMAGE,
                                          imageId,
                                          width, height};

            UInt const payloadSize = width * height * sizeof(UInt);

            void *buffer;
            while (!bufferSize)
            {
                bufferSize = ::bsp_channel_write_start(osdChannel_,
                                                       &buffer,
                                                       sizeof(bspcmd_osd_cache_image) + payloadSize,
                                                       1000);
            }
    
            std::memcpy(buffer, &bco, sizeof(bspcmd_osd_cache_image));
    
            Byte *p = static_cast<Byte *>(buffer) + sizeof(bspcmd_osd_cache_image);
    
            for (int y = 0; y < height; ++y)
            {
                Byte const *r = rows[y];
                for (int x = 0; x < width; ++x)
                {
                    p[0] = r[2];
                    p[1] = r[1];
                    p[2] = r[0];
                    p[3] = r[3];
                    r += 4;
                    p += 4;
                }
            }
    
            ::bsp_channel_write_finish(osdChannel_, bufferSize);

            ClosePngFile();
            return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------------------------------------------

    void BspTrueColorOsd::SendOsdCmd(void const *bco, UInt bcoSize, void const *payload, UInt payloadSize)
    {
        void *buffer;
        int bufferSize = 0;

        while (!bufferSize)
        {
            bufferSize = ::bsp_channel_write_start(osdChannel_,
                                                   &buffer,
                                                   bcoSize + payloadSize,
                                                   1000);
        }

        std::memcpy(buffer, bco, bcoSize);
        if (payloadSize)
        {
            std::memcpy(static_cast<Byte *>(buffer) + bcoSize, payload, payloadSize);
        }
        ::bsp_channel_write_finish(osdChannel_, bufferSize);
    }

    //--------------------------------------------------------------------------------------------------------------

    /* override */ eOsdError BspTrueColorOsd::SetAreas(tArea const *areas, int numAreas)
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

    /* override */ eOsdError BspTrueColorOsd::SetPalette(const cPalette &palette, int area)
    {
        esyslog("ERROR: BspTrueColorOsd::SetPalette not supported\n");
        return oeOk;
    }

    //--------------------------------------------------------------------------------------------------------------

    void BspTrueColorOsd::SetImagePath(UInt imageId, char const *path)
    {
        if (ImageIdInRange(imageId))
        {
            imagePaths_[imageId] = path;
            imageDirty_[imageId] = true;
        }
    }

    //--------------------------------------------------------------------------------------------------------------
}
