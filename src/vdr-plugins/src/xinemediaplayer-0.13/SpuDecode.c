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

// SpuDecode.c
#include <algorithm> 

#include "vdr/osd.h"
#include "vdr/device.h"
#include "vdr/dvbspu.h"
#ifndef USE_PLAYPES
#include "XineLib.h"
#endif

#include "SpuDecode.h"

#ifdef  SPU_PROFILE
#include "profile.h"
#endif


namespace Reel
{

//--------------SpuDecoder-------------------------------------------------------

    SpuEventQueue SpuDecoder::eventQueue;
    
    SpuDecoder::~SpuDecoder()
    {
        Hide(true);
        eventQueue.Clear();
#ifdef SPU_PRINT_ALL
        eventQueue.PrintEvents();
#endif
    }

    void SpuDecoder::SpuPutEvents(reel_video_overlay_event_t *event)
    {
        eventQueue.PutEvent(static_cast<reel_video_overlay_event_c *>(event)); 
    }  

    reel_video_overlay_event_t *SpuDecoder::SpuGetEventStruct(size_t paletteSize, size_t rleSize)
    {
        return new reel_video_overlay_event_c(paletteSize, rleSize);
    }

    void SpuDecoder::ProcessSpuEvents()
    {
        // XXX warning this  "bool osdtaken_" is not in sync with 
        // void *Control::osdTaker_;
        // this shoud be done 
        if(osdtaken_ && !cOsd::IsOpen())
        { 
            printf (" --------------- set osdtaken_ = false; -------------- \n");
            osdtaken_ = false;
            Redraw();          
        }
        //get next spu event
        reel_video_overlay_event_c *event;

        if(!eventQueue.IsEmpty() && eventQueue.GetEventIfReady(event, this))
        { 
#ifdef SPU_PRINT
#define LOGPATH "/tmp/devel2/temp/SPU_TEST/spu_events", "a+"
            FILE *file = fopen(LOGPATH);
            if(file)
            {
                eventQueue.PrintEvent(file, event);
                fclose(file);
            }
            else
            {
                printf("----cannot open file: "  LOGPATH " ----\n");
            }
#endif

            switch (event->event_type)
            {  
                case OVERLAY_EVENT_NULL:
#ifdef SPU_DEBUG
                    printf("---------SpuDecoder::ProcessSpuEvents:OVERLAY_EVENT_NULL---------\n");
#endif
                    break; 

                case OVERLAY_EVENT_SHOW:
#ifdef SPU_DEBUG 
                    printf("---------SpuDecoder::ProcessSpuEvents:OVERLAY_EVENT_SHOW---------\n");
#endif
                    {
#ifdef SPU_PROFILE
                    printf("\n");
                    cProfile profile("OVERLAY_EVENT_SHOW");
#endif
                    ShowOverlay(event->object.overlay, true);
                    ShowHighlight(event->object.overlay);
                    Flush();
                    }
                    break;
 
                case OVERLAY_EVENT_HIDE:
#ifdef SPU_DEBUG
                    printf("---------SpuDecoder::ProcessSpuEvents:OVERLAY_EVENT_HIDE---------\n");
#endif
                    usleep(1000000);
                    Hide(true);
                    break;

                case OVERLAY_EVENT_MENU_BUTTON:
#ifdef SPU_DEBUG
                    printf("---------SpuDecoder::ProcessSpuEvents:OVERLAY_EVENT_MENU_BUTTON-----\n");
#endif
                    HideHighlight();
                    ShowHighlight(event->object.overlay);
                    Flush();

                    break; 

                case OVERLAY_EVENT_FREE_HANDLE: 
#ifdef SPU_DEBUG
                    printf("---------SpuDecoder::ProcessSpuEvents:OVERLAY_EVENT_FREE_HANDLE-----\n"); 
#endif
                    break;
            }
            delete event;
        }
    } 

    bool SpuDecoder::Sync(reel_video_overlay_event_t *event)
    {
#ifdef USE_PLAYPES
    printf("%s called. Not Implemented for PlayPes()\n", __PRETTY_FUNCTION__);
    return false;
#else
        const int64_t MAX_DELAY = 90000 * 60; //1min 
        const int64_t MIN_DELAY = 9000; //0.1 s
        const int64_t MIN_DELAY_CAPTION_HIDE = -5000; //-0.5 s  
        const int64_t MIN_DELAY_CAPTION_SHOW = 5000; //0.5 s 
        //int64_t stc = cDevice::PrimaryDevice()->GetSTC(); 
        int64_t vpts = event->vpts;

        int64_t scr = XineMediaplayer::vxi->GetMetronomTime(event->decoder_instance);
        int minDelay = MIN_DELAY; 
        if(!isInMenuDomain_)
        {
            if (event->event_type == OVERLAY_EVENT_HIDE)
            {
                //show captions for some additional time
                //printf("-------------caption Hide delayed---------------\n");
                minDelay = MIN_DELAY_CAPTION_HIDE;
            }
            else if (event->event_type == OVERLAY_EVENT_SHOW)
            {
                //show captions as soon as possible
                minDelay = MIN_DELAY_CAPTION_SHOW;
            }
        }

        //printf("---------Sync: vpts = %lld, pts = %lld, stc = %lld, scr = %lld---------\n", vpts, event->object.pts, stc, scr);

        if(vpts == 0)   
        //must be handled immediately
        {
            //printf("--------- vpts == 0 ---------\n");
            return true;
        } 
        if(/*stc == -1 || */scr == -1)   
        //invalid stc -> process immediately
        {
             //printf("------- stc = %lld, scr = %lld, decoder_instance = %u------\n", stc, scr, (uint) event->decoder_instance);
            return true;
        }
        if(vpts - scr > MAX_DELAY) 
        //timestamp far in future -> process immediately
        { 
             //printf("------- timestamp far in future -------\n");
            return true;
        }
        if( vpts - scr < minDelay)
        //it's time to process this event
        { 
             //printf("----------- synced ---------\n");
            return true;
        } 
        //printf("------- spu action delayed -------\n");
        return false;  
#endif
    }    

    void SpuDecoder::TakeOsd()
    { 
        printf("--------- %s  Set  osdtaken_ = true; !! ---------\n", __PRETTY_FUNCTION__);

        Hide(false);
        osdtaken_ = true;
    }      

    void SpuDecoder::DrawBitmap(cBitmap &bmp)
    {
#ifdef SPU_DEBUG
        printf("---------SpuDecoder::DrawBitmap---------\n");
#endif
#ifdef SPU_PROFILE
            cProfile profile("DrawBitmap");
#endif
        if(!osd_.get() && !osdtaken_)
        {  
            osd_.reset(cOsdProvider::NewOsd(0, 0));
            //osd_.reset(cOsdProvider::NewTrueColorOsd(0, 0, 0));
            tArea Area = { 0, 0, 720, 576, 8 };
            osd_->SetAreas(&Area, 1); 
        }
        if(osd_.get())
        {
#ifndef USE_PLAYPES
            XineMediaplayer::vxi->SetOsdScaleMode(true);
            osd_->DrawBitmap(bmp.X0(), bmp.Y0(), bmp);
            XineMediaplayer::vxi->SetOsdScaleMode(false);        
#endif
        }
    }  

    void SpuDecoder::Redraw()
    { 
#ifdef SPU_DEBUG
        printf("-----------------SpuDecoder::Redraw()------------------\n");
#endif
        if(bmp_.get())
        {
            DrawBitmap(*bmp_);
        }
        if(hlbmp_.get())
        {
            DrawBitmap(*hlbmp_);
        } 
        if(osd_.get())
        {
            Flush();
        }  
    }

    void SpuDecoder::Hide(bool clear)
    { 
#ifdef SPU_DEBUG
        printf("-----------------SpuDecoder::Hide------------------\n");
#endif
        osd_.reset();
        if(clear)
        {
            bmp_.reset(); 
            hlbmp_.reset();
        }
    }       

    void SpuDecoder::Flush()
    {
#ifdef SPU_PROFILE
        cProfile profile("Flush");
#endif
        if(osd_.get())
        {
            osd_->Flush();
        }
    }

    void SpuDecoder::ShowOverlay(const reel_vo_overlay_t *ovl, bool newOvl)
    {
#ifdef SPU_DEBUG
        printf("---------SpuDecoder::ShowOverlay---------\n");
#endif
#ifdef SPU_PROFILE
        cProfile profile("ShowOverlay");
#endif
        int x = ovl->x;
        int y = ovl->y;
        int w = ovl->width;
        int h = ovl->height;
#ifdef SPU_DEBUG
        printf("---------x = %d, y =%d, w=%d, h=%d---------\n", x, y, w, h);
#endif   
        if(newOvl && ovl->data_valid)
        {
            bmp_.reset(new cBitmap(w, h, 8, x, y)); 
            RletoBmp(*bmp_, w, h, ovl->rle);
            SetBmpPalette(*bmp_, ovl->color, ovl->trans, ovl->rgb_clut); 

#ifdef SPU_PRINT_BITMAP
            PrintBmp(*bmp_);
#endif
        }
        if(bmp_.get())
        {
            DrawBitmap(*bmp_);
        }
    }

    void SpuDecoder::ShowHighlight(const reel_vo_overlay_t *ovl)
    {
#ifdef SPU_DEBUG
        printf("---------SpuDecoder::ShowHighlight---------\n");
#endif
#ifdef SPU_PROFILE
        cProfile profile("ShowHighlight");
#endif
        int x = ovl->hili_left;
        int y = ovl->hili_top;
        int w = ovl->hili_right - x;
        int h = ovl->hili_bottom - y;
#ifdef SPU_DEBUG  

        printf("---------x = %d, y =%d, w=%d, h=%d---------\n", x, y, w, h);
#endif        
        hlbmp_.reset();
        if(bmp_.get())
        {
            hlbmp_.reset(GetNewPartOfBmp(*bmp_, w, h, x, y));
            if(hlbmp_.get())
            {
                SetBmpPalette(*hlbmp_, ovl->hili_color, ovl->hili_trans, ovl->hili_rgb_clut);

#ifdef SPU_PRINT_BITMAP
                PrintBmp(*hlbmp_);
#endif

                DrawBitmap(*hlbmp_);
            }
        }
    }

    void SpuDecoder::HideHighlight()
    {
#ifdef SPU_DEBUG
        printf("---------SpuDecoder::HideHighlight---------\n");
#endif       
#ifdef SPU_PROFILE
        cProfile profile("HideHighlight");
#endif
        if (bmp_.get() && hlbmp_.get())
        {
#ifdef SPU_DEBUG
        printf("---------x = %d, y =%d, w=%d, h=%d---------\n", hlbmp_->X0(), hlbmp_->Y0(), hlbmp_->Width(), hlbmp_->Height());
#endif        
            for (int i = 0; i < 4; i++) 
            {
                hlbmp_->SetColor(i, bmp_->Color(i));   
            }
            DrawBitmap(*hlbmp_);
        }
    }

// decolde rle
    void SpuDecoder::RletoBmp(cBitmap &bmp, uint w, uint h, const reel_rle_elem_t *rle)
    {
#ifdef SPU_PROFILE
        cProfile profile("RletoBmp");
#endif
        int pos = 0;
        uint16_t len = 0;
        uint8_t idx = 0;
        for (uint yp = 0; yp < h; ++yp) 
        {
            for (uint xp = 0; xp < w; ++xp) 
            {
                if(len == 0)
                { 
                    len = rle[pos].len; 
                    idx = rle[pos].color;
                    ++pos;
                }
                --len;
#if VDRVERSNUM < 10716
                bmp.SetIndexFast(xp, yp, idx);
#else
                printf("ERROR: SpuDecode.c: SpuDecoder::RletoBmp: Not implemented yet!\n");
#endif
            }
        }
    } 

//Get rectangle out of Bitmap
    cBitmap *SpuDecoder::GetNewPartOfBmp(const cBitmap &bmp, int w, int h, int x, int y)
    {
        if(x + w > bmp.Width() ||  
           y + h > bmp.Height()|| 
           x < 0 ||   
           y < 0 ||
           w < 0 ||
           h < 0)   
        {
            return NULL;
        }
        cBitmap *partBmp = new cBitmap(w, h, 8, x + bmp.X0(), y + bmp.Y0()); 
        for (int yp = 0; yp < h; ++yp) 
        {
            for (int xp = 0; xp < w; ++xp) 
            {
                tIndex idx = *bmp.Data(xp + x, yp + y);
                partBmp->SetIndex(xp, yp, idx);
            }
        } 
        return partBmp;
    }     

// set the palette
    void SpuDecoder::SetBmpPalette(cBitmap &bmp, const uint32_t *col, const uint8_t* trans, bool isRgb)
    {
#ifdef SPU_PROFILE
        cProfile profile("SetBmpPalette");
#endif
        for (int i = 0; i < 4; i++) 
        {
            tColor transVal = (trans[i] * 255) / 15;
            tColor colorVal = transVal << 24;
            if(isRgb)
            {
                colorVal |= col[i];
            }
            else
            {
                colorVal |= yuv2rgb(col[i]);
            }
            //set transparent to black (bsp)
            if((colorVal & 0xFF000000) == 0)
            {
                colorVal = 0;
            } 
            bmp.SetColor(i, colorVal);
        }
    }

#ifdef SPU_PRINT_BITMPAP
    void SpuDecoder::PrintBmp(const cBitmap &bmp)
    {
        FILE *file = fopen("/tmp/devel2/temp/SPU_TEST/spu_bitmap", "w");
        if (file)
        { 
            uint x = bmp.X0();
            uint y = bmp.Y0(); 
            uint w = bmp.Width();
            uint h = bmp.Height();
             for (uint yp = 0; yp < h; ++yp) 
            {
                for (uint xp = 0; xp < w; ++xp) 
                {
                    fprintf(file," %u ", (uint) *bmp.Data(xp, yp));
                }
                fprintf(file, "\n");
            }
            fclose (file);
        } 
        else
        { 
            printf("---------cannot open file /tmp/devel2/temp/bitmap---------\n");
        } 
    }
#endif

//----------------------------------SpuEventQueue--------------------------------------------

    void SpuEventQueue::PutEvent(reel_video_overlay_event_c *event)
    {
        MutexLocker lock(mutex_); 
        queue_.push_back(event);        
        SortByVpts();
    }

    void SpuEventQueue::GetEvent(reel_video_overlay_event_c *&event)
    {
        MutexLocker lock(mutex_); 
        event = *queue_.begin();
        queue_.pop_front();
    }

    bool SpuEventQueue::IsEmpty()
    { 
        MutexLocker lock(mutex_); 
        return queue_.empty();
    }

    void SpuEventQueue::Clear()
    { 
        MutexLocker lock(mutex_); 
        queue_.clear();
    }

    void SpuEventQueue::SortByVpts()
    {
        std::stable_sort(queue_.begin(), queue_.end(), sortfunc);
    }

    bool SpuEventQueue::GetEventIfReady(reel_video_overlay_event_c *&event, SpuDecoder *spu)
    {
        MutexLocker lock(mutex_);
        event = *queue_.begin();  
        if(spu->Sync(event))
        {
            queue_.pop_front();
            return true;  
        }
        return false;    
    }

#ifdef SPU_PRINT
    void SpuEventQueue::PrintEvents()
    { 
        MutexLocker lock(mutex_);
        FILE *file = fopen("/tmp/devel2/temp/SPU_TEST/spu_all_events", "w");
        if(file)
        { 
            for(std::deque<reel_video_overlay_event_c*>::iterator i = queue_.begin(); i != queue_.end(); i++)
            {
                PrintEvent(file, *i);
            }
            fclose(file);
        } 
    }  

    void SpuEventQueue::PrintEvent(FILE* file, const reel_video_overlay_event_t *event) 
    { 
        printf("------------SpuEventQueue::PrintEvent-----------\n");
        fprintf(file,"\n\n#################new spu event#################\n");
        fprintf(file,"--event_type = %d\n", event->event_type);
        fprintf(file,"--vpts = %lld\n", event->vpts);
        fprintf(file,"--handle = %d\n--object_type = %d\n--pts = %lld\n--buttonN = %d\n", 
            event->object.handle, event->object.object_type, event->object.pts, event->object.buttonN);

        fprintf(file,"---------------------------------------------------\n");

        PrintButtons(file, event->object.button);

        fprintf(file,"---------------------------------------------------\n");

        PrintOverlay(file, event->object.overlay);
    }

    void SpuEventQueue::PrintOverlay(FILE* file, const reel_vo_overlay_t *ovl) 
    { 
        fprintf (file,"video_overlay: \tdata_size = %d\n", ovl->data_size);
        fprintf (file,"video_overlay: \tnum_rle = %d\n", ovl->num_rle);
        fprintf (file,"video_overlay: \trgb_clut = %d\n", ovl->rgb_clut);
        fprintf (file,"video_overlay: \tx = %d y = %d width = %d height = %d\n",
            ovl->x, ovl->y, ovl->width, ovl->height );
        fprintf (file,"video_overlay: \tclut [%x %x %x %x]\n",
            ovl->color[0], ovl->color[1], ovl->color[2], ovl->color[3]);
        fprintf (file,"video_overlay: \ttrans [%d %d %d %d]\n",
            ovl->trans[0], ovl->trans[1], ovl->trans[2], ovl->trans[3]);
        fprintf (file,"video_overlay: \tclip top=%d bottom=%d left=%d right=%d\n",
            ovl->hili_top, ovl->hili_bottom, ovl->hili_left, ovl->hili_right);
        fprintf (file,"video_overlay: \tclip_clut [%x %x %x %x]\n",
            ovl->hili_color[0], ovl->hili_color[1], ovl->hili_color[2], ovl->hili_color[3]);
        fprintf (file,"video_overlay: \thili_trans [%d %d %d %d]\n",
            ovl->hili_trans[0], ovl->hili_trans[1], ovl->hili_trans[2], ovl->hili_trans[3]);
    }

    void SpuEventQueue::PrintButton(FILE* file, const reel_vo_buttons_t *buttons)
    {
        fprintf(file,"--button--hili_top = %d-----\n", buttons->hili_top); 
        fprintf(file,"--button--hili_bottom = %d-----\n", buttons->hili_bottom) ;
        fprintf(file,"--button--hili_left = %d-----\n", buttons->hili_left) ;
        fprintf(file,"--button--hili_right = %d-----\n", buttons->hili_right); 
        fprintf(file,"--button--up = %d-down = %d-left = %d-right = %d\n",
                buttons->up, buttons->down, buttons->left, buttons->right); 
        fprintf(file,"--button--hili_rgb_clut = %d\n", buttons->hili_rgb_clut);

        //xine_event_t      select_event;
        //xine_event_t      active_event;
    }

    void SpuEventQueue::PrintButtons(FILE* file, const reel_vo_buttons_t *buttons)
    {
        uint i = 0;
        for(i = 0; i < 32; i++)
        {
            PrintButton(file, &buttons[i]);
        } 
    }
#endif

//------------------------------reel_video_overlay_event_c---------------------------------------

    reel_video_overlay_event_c::reel_video_overlay_event_c(size_t paletteSize, size_t rleSize)
    { 
        //printf("---------reel_video_overlay_event_c::reel_video_overlay_event_c------------\n");
        object.overlay = new reel_vo_overlay_t;
        object.palette = NULL;
        object.overlay->rle = NULL;
        if(paletteSize > 0)
        {
            object.palette = new uint32_t[paletteSize];
        }
        if(rleSize >0)
        {
            object.overlay->rle = new reel_rle_elem_t[rleSize]; 
        }
    } 

    reel_video_overlay_event_c::~reel_video_overlay_event_c()
    {
        delete[] object.overlay->rle; 
        delete object.overlay;
        delete[] object.palette;
    }

}
