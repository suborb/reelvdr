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

// HdOsd.c

#include "HdOsd.h"
#include "HdTrueColorOsd.h"

#include <string.h>
#include <cstring>
#include <unistd.h>

namespace Reel
{
    const u_int maxOsdWidth  = 720;
    const u_int maxOsdHeight = 576;

    Int const OSD_PACKET_SIZE_BOUND = 0x4000;
    
    static int first_run=1;
    HdOsd *HdOsd::instance_;

#if APIVERSNUM >= 10509 || defined(REELVDR)
    HdOsd::HdOsd(int Left, int Top, uint level)
    :cOsd(Left, Top, level),
#else
    HdOsd::HdOsd(int Left, int Top)
    :cOsd(Left, Top),
#endif
    lPos(Left),
    tPos(Top),
    scaleToVideoPlane_(0)
    {
	    
	    if (first_run) {
		    Clear(0,0,maxOsdWidth,maxOsdHeight);
		    first_run=0;
	    }
	    max_x=min_x=0;
	    max_y=min_y=0;
	    last_numColors=0;
            instance_ = this;
    }
    
    HdOsd::~HdOsd()
    {
            //if (Active())
            //    Clear(min_x, min_y, max_x, max_y);
            instance_ = this;
#if APIVERSNUM >= 10509 || defined(REELVDR)
            SetActive(false);

#else 
            Clear(min_x, min_y, max_x, max_y);
#endif
    }
    
    void HdOsd::Clear(int x, int y, int x1, int y1)
    {
        hdcmd_osd_clear_t buffer;
        buffer.cmd=HDCMD_OSD_CLEAR;
        buffer.x=x;
        buffer.y=y;
        buffer.x1=x1;
        buffer.y1=y1;

        SendOsdCmd(buffer);
    }

#if APIVERSNUM >= 10509 || defined(REELVDR)
    eOsdError HdOsd::SetAreas(const tArea *Areas, int NumAreas) // 1.6
    {
        if (Active()) {
            // clear OSd
            Clear(0,0,maxOsdWidth,maxOsdHeight);
        }
        return cOsd::SetAreas(Areas, NumAreas);
    }
#endif

    void HdOsd::Flush()
    { 
#if APIVERSNUM >= 10509 || defined(REELVDR)
        if (! Active()) return ;
#endif
        bool full = false;

        for (int area = 0; true; ++area)
        {
            cBitmap *pBitmap = GetBitmap(area);
            if (pBitmap == 0)
            {
                break;
            }
            FlushBitmap(*pBitmap, full);
        }
//        ::usleep(40000);
    }
    
    namespace
    {
        u_int CountBytes(const u_char *&data,
                        u_char *&line,
                        u_char &b,
                        u_int max)
        {
            u_char ind = *data;
            b = ind - *line;
            *line = ind;
            ++data;
            ++line;
            u_int c = 1;
            while (c < max)
            {
                ind = *data;
                u_char b2 = ind - *line;
                if (b2 == b)
                {
                    *line = ind;
                    ++c;
                    ++data;
                    ++line;
                }
                else
                {
                    break;
                }
            }
            return c;
        }
    }

    
    void HdOsd::FlushBitmap(cBitmap &bitmap, bool full)
    {        

	    int x1 = 0;
	    int y1 = 0;
	    int x2 = bitmap.Width() - 1;
	    int y2 = bitmap.Height() - 1;
        
	    if (full || bitmap.Dirty(x1, y1, x2, y2))
	    {
            if (x1 < 0) x1 = 0;
            if (y1 < 0) y1 = 0;
            if (x2 < 0) x2 = 0;
            if (y2 < 0) y2 = 0;

		    u_int x = lPos + bitmap.X0() + x1;
		    u_int y = tPos + bitmap.Y0() + y1;
		    if (x >= maxOsdWidth || y >= maxOsdHeight)
		    {
			    bitmap.Clean();
			    return;
		    }
		    u_int width = x2 - x1 + 1;
		    u_int height = y2 - y1 + 1;
		    if (x + width > maxOsdWidth)
		    {
			    width = maxOsdWidth - x;
		    }
		    if (y + height > maxOsdHeight)
		    {
			    height = maxOsdHeight - y;
		    }
		    int numColors;
		    const tColor *palette = bitmap.Colors(numColors);

		    int h,hh,n;

		    if (numColors!=last_numColors || memcmp(palette,last_palette,sizeof(int)*numColors)) {
			    hdcmd_osd_palette_t *bco;
                            void *buff = malloc( sizeof(hdcmd_osd_palette_t)+numColors*sizeof(int));

                            bco = static_cast<hdcmd_osd_palette_t *>(buff);
			    bco->cmd=HDCMD_OSD_PALETTE;
			    bco->count=numColors;

                            for(n=0;n<numColors;n++)
                                    bco->palette[n]=palette[n];

			    SendOsdCmd(bco, sizeof(hdcmd_osd_palette_t)+numColors*sizeof(int)); 
			    last_numColors=numColors;
			    memcpy(last_palette,palette,sizeof(int)*numColors);
                            free(buff);
		    }		  
#define BLOCK_HEIGHT 16

		    h=height;
	    
		    while(h>0) 
		    {
			    for (u_int yy = y1; yy <y1+height; yy+=BLOCK_HEIGHT) 
			    {

				    hdcmd_osd_draw8_t *bco;

                                    hh=(h>BLOCK_HEIGHT?BLOCK_HEIGHT:h);

                                    void *buff = malloc(sizeof(hdcmd_osd_draw8_t) + hh*width);

                                    bco = static_cast<hdcmd_osd_draw8_t *>(buff);
				    bco->cmd=HDCMD_OSD_DRAW8;
				    bco->x=x;
				    bco->y=y;
				    bco->w=width;
				    bco->h=hh;
				    bco->scale = scaleToVideoPlane_;

				    for(n=0;n<hh;n++)
					    memcpy((char*)(&bco->data)+width*n,bitmap.Data(x1, yy+n),width);

				    SendOsdCmd(bco, sizeof(hdcmd_osd_draw8_t) + hh*width);
				    h=h-hh;
				    y+=BLOCK_HEIGHT;
			            free(buff);
			    }
		    }
            hdcmd_osd_flush bco;

            bco.cmd = HDCMD_OSD_FLUSH;
        
            SendOsdCmd(bco);

  //error_out:	    


	    if (x<min_x)
		    min_x=x;
	    
	    if (y<min_y)
		    min_y=y;
	    
	    if (x+width>max_x)
		    max_x=x+width;
	    
	    if (y+height>max_y)
		    max_y=y+height;


	    bitmap.Clean();
	    }
    }

    void HdOsd::SendOsdCmd(void const *bco, UInt bcoSize, void const *payload, UInt payloadSize)
    {
        void *buffer = (void*)malloc(payloadSize + bcoSize);

        std::memcpy(buffer, bco, bcoSize);
        if (payloadSize)
        {
            std::memcpy(static_cast<Byte *>(buffer) + bcoSize, payload, payloadSize);
        }

        hd_packet_es_data_t packet;
        //printf("sendpacket!\n");
        HdCommChannel::chOsd.SendPacket(HD_PACKET_OSD, packet, (Reel::Byte*)buffer, bcoSize + payloadSize);
        free(buffer);
    }

#if APIVERSNUM >= 10509 || defined(REELVDR)
    void HdOsd::SetActive(bool On)
    {
        if( On != Active())
        {
            cOsd::SetActive(On);

            Clear(min_x, min_y, max_x, max_y);
            if(On) 
                Flush();
        }

    printf("%s On=%i\n", __PRETTY_FUNCTION__, On);
    }
#endif

}


