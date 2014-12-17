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

// HdTrueColorOsd.h
 
#ifndef HD_TRUE_COLOR_OSD_H_INCLUDED
#define HD_TRUE_COLOR_OSD_H_INCLUDED

#include "Reel.h"

#include "HdCommChannel.h"

#include <string>
#include <vdr/osd.h>

namespace Reel
{
    class HdTrueColorOsd : public cOsd
    {
    public:
        enum
        {
            MaxImageId = HDOSD_MAX_IMAGES - 1
        };

#if APIVERSNUM >= 10509 || defined(REELVDR)
        HdTrueColorOsd(int left, int top, uint level);
        void SetActive(bool On);
#else
        HdTrueColorOsd(int left, int top);
#endif

        ~HdTrueColorOsd();
        
        /* override */ cBitmap *GetBitmap(int area);
        
        /* override */ eOsdError CanHandleAreas(const tArea *areas, int numAreas);
        
        /* override */ eOsdError SetAreas(const tArea *areas, int numAreas);
        
        /* override */ void SaveRegion(int x1, int y1, int x2, int y2);
        
        /* override */ void RestoreRegion();
        
        /* override */ eOsdError SetPalette(const cPalette &palette, int area);
        
        /* override */ void DrawPixel(int x, int y, tColor color);
        
        /* override */ void DrawBitmap(int x,
                                       int y,
                                       const cBitmap &bitmap,
                                       tColor colorFg,
                                       tColor colorBg,
                                       bool replacePalette,
                                       bool overlay);
        
        /* override */ void DrawBitmap32(int x,
                                       int y,
                                       const cBitmap &bitmap,
                                       tColor colorFg,
                                       tColor colorBg,
                                       bool replacePalette,
                                       bool overlay, int width, int height);

        /* override */ void DrawBitmapHor(int x, int y, int w, const cBitmap &bitmap);
        
        /* override */ void DrawBitmapVert(int x, int y, int h, const cBitmap &bitmap);
        
        /* override */ void DrawImage(UInt imageId, int x, int y, bool blend,
                                      int horRepeat, int vertRepeat);

        void DrawCropImage(UInt imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend);

        /* override */ void DrawText(int x,
                                     int y,
                                     const char *s,
                                     tColor colorFg,
                                     tColor colorBg,
                                     const cFont *font,
                                     int width,
                                     int height,
                                     int alignment);
        
        /* override */ void DrawRectangle(int x1, int y1, int x2, int y2, tColor color);
       
        /* override */ void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV);
 
        /* override */ void DrawEllipse(int x1, int y1, int x2, int y2, tColor color, int quadrants);
        
        /* override */ void DrawSlope(int x1, int y1, int x2, int y2, tColor color, int type);

        /* override */ void Flush(void);

        /* override */ void SetImagePath(UInt imageId, char const *path);

        
    private:
        // No assigning or copying
        HdTrueColorOsd(const HdTrueColorOsd &);
        HdTrueColorOsd &operator=(const HdTrueColorOsd &);

        int CacheFont(cFont const &font);
        void CacheImage(UInt imageId);
        void ClosePngFile();
        static bool ImageIdInRange(UInt imageId);
        bool OpenPngFile(char const         *path,
                         Byte **&rows,
                         int                &width,
                         int                &height);
        bool SendImage(UInt imageId, char const *path);
        void SendOsdCmd(void const *bco, UInt bcoSize, void const *payload = 0, UInt payloadSize = 0);
        template<typename T>
        void SendOsdCmd(T const &bco, void const *payload = 0, UInt payloadSize = 0);

#define MAXOSDAREAS 16

  cBitmap *bitmaps[MAXOSDAREAS];
  int numBitmaps;


        hd_channel_t *osdChannel_;

        bool dirty_;

        static void ClearFontCache();

        static int            fontGeneration_;
        static cFont const   *cachedFonts_[HDOSD_MAX_CACHED_FONTS];
        static int volatile  *hdCachedFonts_;

        static int volatile  *cachedImages_;
        static bool        imageDirty_[MaxImageId + 1];
        static std::string imagePaths_[MaxImageId + 1];
    };

    template<typename T>
    inline void HdTrueColorOsd::SendOsdCmd(T const &bco, void const *payload, UInt payloadSize)
    {
        return SendOsdCmd(&bco, sizeof(T), payload, payloadSize);
    }
}

#endif // HD_TRUE_COLOR_OSD_H_INCLUDED
