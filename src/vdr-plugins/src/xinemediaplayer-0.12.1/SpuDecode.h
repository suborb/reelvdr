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

// SpuDecode.h

#include <deque>
#include <memory>

#include "vdr/dvbspu.h"

#include "Mutex.h"
#ifndef USE_PLAYPES
#include "XineLib.h"
#endif
#include "VdrXineMpIf.h"


#ifndef SPUDECODE_H
#define SPUDECODE_H

//#define SPU_PRINT
//#define SPU_PRINT_ALL
//#define SPU_DEBUG
//#define SPU_PROFILE
//#define SPU_PRINT_BITMPAP

class cBitmap;
class cOsd;

namespace Reel
{

    class reel_video_overlay_event_c;
    class SpuEventQueue;

    class SpuDecoder // public : cSpuDecoder
    {
    public:
        inline SpuDecoder();
        ~SpuDecoder();
        static void SpuPutEvents(reel_video_overlay_event_t *event);
        static reel_video_overlay_event_t *SpuGetEventStruct(size_t paletteSize, size_t rleSize);
        void ProcessSpuEvents();
        bool Sync(reel_video_overlay_event_t *event);     
        void TakeOsd();
        bool HasOsd();
        void Hide(bool clear);
        void SetMenuDomain(bool isInMenuDomain); 

    private:
        void DrawBitmap(cBitmap &bmp);            
        void Redraw();   
        //void Hide(bool clear); 
        void Flush();
        void ShowOverlay(const reel_vo_overlay_t *ovl, bool newOvl);
        void ShowHighlight(const reel_vo_overlay_t *ovl);    
        void HideHighlight();  
        void RletoBmp(cBitmap &bmp, uint w, uint h, const reel_rle_elem_t *rle_data);
        cBitmap *GetNewPartOfBmp(const cBitmap &bmp, int w, int h, int x, int y);
        void SetBmpPalette(cBitmap &bmp, const uint32_t *col, const uint8_t* trans, bool isRgb);
#ifdef SPU_PRINT_BITMAP
        void PrintBmp(const cBitmap &bmp);
#endif
        inline uint32_t yuv2rgb(uint32_t yuv_color);

        static SpuEventQueue eventQueue;
        std::auto_ptr<cOsd> osd_;
        std::auto_ptr<cBitmap> bmp_; 
        std::auto_ptr<cBitmap> hlbmp_; 
        bool osdtaken_;
        bool isInMenuDomain_;
    };

//-----------------------------------------------------------------------------------------------

    class reel_video_overlay_event_c : public  reel_video_overlay_event_t
    {
    public:
        reel_video_overlay_event_c(size_t paletteSize, size_t rleSize);
        ~reel_video_overlay_event_c();
        inline bool operator < (const reel_video_overlay_event_c& event) const;   
    };

//------------------------------------------------------------------------------

    class SpuEventQueue
    {
    public:
        void PutEvent(reel_video_overlay_event_c *event);
        void GetEvent(reel_video_overlay_event_c *&event);
        bool GetEventIfReady(reel_video_overlay_event_c *&event, SpuDecoder *spu);
        bool IsEmpty();
        void Clear();
#ifdef SPU_PRINT
        void PrintEvents();
        void PrintEvent(FILE* file, const reel_video_overlay_event_t *event);
        void PrintOverlay(FILE* file, const reel_vo_overlay_t *ovl);
        void PrintButtons(FILE* file, const reel_vo_buttons_t *buttons);
        void PrintButton(FILE* file, const reel_vo_buttons_t *buttons);
#endif

    private:        
        void SortByVpts();
        std::deque<reel_video_overlay_event_c*> queue_;
        Mutex mutex_;
    };

//-----------------Inline-----------------------------------------------------------------------

    bool reel_video_overlay_event_c::operator < (const reel_video_overlay_event_c& event) const
    {
        return (vpts < event.vpts);
    }

//------------------------------------------------------------------------
    
    inline bool sortfunc(const reel_video_overlay_event_c* left, const reel_video_overlay_event_c* right)
    {
        return (*left < *right);
    }

//------------------------------------------------------------------------ 


    SpuDecoder::SpuDecoder()
    : osdtaken_(false), isInMenuDomain_(false)
    {
    }

    inline  bool SpuDecoder::HasOsd()
    {
       // return !osdtaken_;
       return osdtaken_;
    }

    inline void SpuDecoder::SetMenuDomain(bool isInMenuDomain)
    {
        isInMenuDomain_ = isInMenuDomain;
    }
 
    uint32_t SpuDecoder::yuv2rgb(uint32_t yuv_color)
    {
        int Y, Cb, Cr;
        int Ey, Epb, Epr;
        int Eg, Eb, Er;

        Y = (yuv_color >> 16) & 0xff;
        Cb = (yuv_color) & 0xff;
        Cr = (yuv_color >> 8) & 0xff;

        Ey = (Y - 16);
        Epb = (Cb - 128);
        Epr = (Cr - 128);

        Eg = (298 * Ey - 100 * Epb - 208 * Epr) / 256;
        Eb = (298 * Ey + 516 * Epb) / 256;
        Er = (298 * Ey + 408 * Epr) / 256;

        if (Eg > 255)
            Eg = 255;
        if (Eg < 0)
            Eg = 0;

        if (Eb > 255)
            Eb = 255;
        if (Eb < 0)
            Eb = 0;

        if (Er > 255)
            Er = 255;
        if (Er < 0)
            Er = 0;

        return Eb | (Eg << 8) | (Er << 16);
    }

}
#endif //  SPUDECODE_H
