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

// HdCommChannel.c

#include "HdCommChannel.h"
#include "ReelBoxMenu.h"
#include "MyMutex.h"

#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <stdio.h>
#include <stdlib.h>

#define ALIGN_DOWN(v, a) ((v) & ~((a) - 1))
#define ALIGN_UP(v, a) ALIGN_DOWN((v) + (a) - 1, a)

namespace Reel
{
    namespace HdCommChannel
    {
        ::hd_data_t volatile *hda = NULL;
        Channel chStream1;
        Channel chOsd;

        //static void InitHda();

        static void safe_memcpy(void /* volatile */ *to, void const *from, size_t size)
        {
            REEL_ASSERT(!(reinterpret_cast<size_t>(to) & 3)); // to must be aligned on a 4-byte boundary.

            size_t const buffer_size = 4096;
            Byte buffer[buffer_size];
            Byte *t = static_cast<Byte *>(to);
            Byte const *f = static_cast<Byte const *>(from);

            while (size)
            {
                size_t cpy_size = size;
                if (cpy_size > buffer_size)
                {
                    cpy_size = buffer_size;
                }

                if ((reinterpret_cast<size_t>(f) | cpy_size) & 3)
                {
                    std::memcpy(buffer, f, cpy_size);
                    std::memcpy(t, buffer, (cpy_size + 3) & 0x7FFFFFFC);
                        // cp_size rounded up, will copy up to 3 bytes more than requested.
                }
                else
                {
                    // The simple case: from and cpy_size are aligned. Copy directly.
                    std::memcpy(t, f, cpy_size);
                }
                t += cpy_size;
                f += cpy_size;
                size -= cpy_size;
            }
        }

        Channel::Channel()
        :   ch_(0),
            packetSeqNr_(0)
        {
        }

        Channel::~Channel() NO_THROW
        {
            Close();
        }

        void Channel::Close() NO_THROW
        {
        }

        void Channel::Open(int chNum)
        {
            for(int n=15;n>=0;n--) {
                ch_ = hd_channel_open(chNum);
                if (ch_) 
                    break;                
                syslog(LOG_INFO,"HDE-Channel open %i: waiting to appear (%i) \n",chNum, n);
                sleep(1);
            }
            if (!ch_)
            {
                syslog(LOG_INFO,"Error opening hd channel %d\n, using dummy", chNum);
                fprintf(stderr, "Error opening hd channel %d\n, using dummy", chNum);
                Close();
//              REEL_THROW();
                ch_=NULL;
            }
        }

        void Channel::SendPacket(UInt type, hd_packet_header_t &header, int headerSize,
                                 Byte const *data, int dataSize)
        {
            if (!ch_) return;

	    if (!hda->hdp_running) // Player not running, avoid stall
		    return;

//            static FairMutex mutex;
            mutex_.Lock();

            header.magic = HD_PACKET_MAGIC;
            header.seq_nr = packetSeqNr_;
            header.type = type;
            header.packet_size = headerSize + dataSize;

            packetSeqNr_++;

            void *buffer;

            int ch_packet_size = (header.packet_size + 3) & 0x7FFFFFFC; // Align 4

            int bufferSize = hd_channel_write_start(ch_, &buffer, ch_packet_size, 0);
            if (!bufferSize)
            {
                for (int n = 0; n < 2000000 && !bufferSize; n += 5000)
                {
                    ::usleep(5000);
                    bufferSize = hd_channel_write_start(ch_, &buffer, ch_packet_size, 0);
                }
            }
            if (bufferSize)
            {
                safe_memcpy(buffer, &header, headerSize);

                if (dataSize)
                {
                    safe_memcpy(static_cast<Byte *>(buffer) + headerSize, data, dataSize);
                }
                hd_channel_write_finish(ch_, ch_packet_size);
            }
            mutex_.Unlock();
        }

        void Exit() NO_THROW
        {
            chStream1.Close();

            ::hd_deinit(0);
        }

        void Init()
        {
            InitHda();

            chStream1.Open(HDCH_STREAM1);
            chOsd.Open(HDCH_OSD);
        }

	void SetHWControl(ReelBoxSetup *rb)
	{
		if (hda->hw_control.audiomix!=rb->audiomix) {
			hda->hw_control.audiomix=rb->audiomix;
			hda->hw_control.changed++;
		}
	}

	void SetVolume(int volume)
	{
      	        hda->audio_control.volume=volume;
	        hda->audio_control.changed++;	        
	}
	
	void SetPicture(ReelBoxSetup *rb)
	{
		hda->plane[0].brightness = rb->brightness+72;
		hda->plane[0].contrast = rb->contrast;
		hda->plane[0].gamma = rb->gamma+30;
		hda->plane[0].sharpness = 128;
	        hda->plane[0].changed++;
	}

        void SetAspect(int HDaspect)      // -1 is used on startup + by setup
	{
	    hd_aspect_t hd_aspect = {0} ;

            if (HDaspect == -1)
                HDaspect = RBSetup.HDaspect;

	    switch (RBSetup.HDdisplay_type)
            {
                case 0: /* 4:3 */
                    hd_aspect.w = 4;
                    hd_aspect.h = 3;
                    break;
                case 1: /* 16:9 */
                    hd_aspect.w = 16;
                    hd_aspect.h = 9;
                    break;
            }

            switch (HDaspect)
            {
                case 0: /* WF */
                    hd_aspect.scale=HD_VM_SCALE_F2S;
                    //hd_aspect.overscan=0;
                    hd_aspect.automatic=0;
                    break;
                case 1: /* WS */
                    hd_aspect.scale=HD_VM_SCALE_F2A;
                    //hd_aspect.overscan=0;
                    hd_aspect.automatic=0;
                    break;
                case 2: /* WC */
                    hd_aspect.scale=HD_VM_SCALE_C2F;
                    //hd_aspect.overscan=0;
                    hd_aspect.automatic=0;
                    break;
            }

            if (RBSetup.HDoverscan)
                    hd_aspect.overscan=1;
            else
                    hd_aspect.overscan=0;

	    if (hd_aspect.w) {
                //hda->video_mode.aspect = hd_aspect;
                //printf("INFO [reelbox]: aspect memcopy\n");
                memcpy((void*)&hda->aspect, (void*)&hd_aspect, sizeof(hd_aspect_t)-sizeof(int)); // ommit changed
            }
	    if(RBSetup.HDfb_force)
		hda->aspect.scale |= RBSetup.HDfb_force;
	    else if(RBSetup.HDfb_dbd) {
                hda->plane[2].ws=-1;
		hda->plane[2].osd_scale= HD_VM_SCALE_FB_DBD;
	    } else {
    		hda->plane[2].osd_scale = HD_VM_SCALE_FB_FIT;
                hda->plane[2].ws=0;
                hda->plane[2].hs=0;
            }
            hda->plane[2].changed++;
	    hda->aspect.changed++;
	}

        void SetVideomode(int HDaspect)      // -1 is used on startup + by setup
        {
            //printf("HdCommChannel::SetVideomode(%i)\n", HDaspect);
           
            //printf (" %s \n", __PRETTY_FUNCTION__ );
            int width=0, height=0, interlaced=0, fps=0;
            //hd_aspect_t hd_aspect = {0} ;

	    if (RBSetup.HDdmode == HD_DMODE_DVI) // dvi
		    hda->video_mode.outputd=HD_VM_OUTD_DVI;
	    else {
		    hda->video_mode.outputd=HD_VM_OUTD_HDMI;
	    }

#ifdef RBLITE
	    if (RBSetup.HDdmode == HD_DMODE_HDMI && RBSetup.ac3 == 1)
#else
	    if (RBSetup.ac3 == 1)
#endif
	    {
		    hda->video_mode.outputda=HD_VM_OUTDA_AC3;
	    }
	    else
	    {
		    hda->video_mode.outputda=HD_VM_OUTDA_PCM;
	    }

	    if (RBSetup.HDamode == HD_AMODE_YUV)
		    hda->video_mode.outputa=HD_VM_OUTA_YUV;
	    else if (RBSetup.HDamode == HD_AMODE_YC)
		    hda->video_mode.outputa=HD_VM_OUTA_YC;
	    else if (RBSetup.HDamode == HD_AMODE_RGB)
		    hda->video_mode.outputa=HD_VM_OUTA_RGB;
	    else if (RBSetup.HDamode == HD_AMODE_OFF)
		    hda->video_mode.outputa=HD_VM_OUTA_OFF;
            else
                    hda->video_mode.outputa=HD_VM_OUTA_AUTO;
    
	    if (RBSetup.HDaport == HD_APORT_MDIN)
		    hda->video_mode.outputa|=HD_VM_OUTA_PORT_MDIN;
	    else if (RBSetup.HDaport == HD_APORT_SCART)
		    hda->video_mode.outputa|=HD_VM_OUTA_PORT_SCART;
	    else if (RBSetup.HDaport == HD_APORT_SCART_V0)
		    hda->video_mode.outputa|=HD_VM_OUTA_PORT_SCART_V0;
	    else if (RBSetup.HDaport == HD_APORT_SCART_V1)
		    hda->video_mode.outputa|=HD_VM_OUTA_PORT_SCART_V1;

            switch (RBSetup.HDresolution)
            {
                case HD_VM_RESOLUTION_1080:  //HD 1080
                    width=1920;
                    height=1080;
                    break;
                case HD_VM_RESOLUTION_720:  //HD 720
                    width=1280;
                    height=720;
                    break;
                case HD_VM_RESOLUTION_576:  // SD PAL
                    width=720;
                    height=576;
                    break;
                case HD_VM_RESOLUTION_480:   //SD NTSC
                    width=720;
                    height=480;
                    break;
            }

            switch (RBSetup.HDintProg)
            {
                case 0:
                    interlaced = 0;
                    break;
                case 1:
                    interlaced = 1;
                    break;
            }

            switch (RBSetup.HDnorm)
            {
                case HD_VM_NORM_50HZ:
                    fps = 50;
                    break;
                case HD_VM_NORM_60HZ:
                    fps = 60;
                    break;
            }


            if (width)
            {
                hda->video_mode.width=width;
                hda->video_mode.height=height;
                hda->video_mode.interlace=interlaced;
                hda->video_mode.framerate=fps;
            }
            hda->video_mode.deinterlacer=RBSetup.HDdeint;
            hda->video_mode.auto_format=RBSetup.HDauto_format;
            hda->video_mode.changed++;

	    SetAspect(HDaspect);
        }

        void InitHda()
        {
            ::hdshm_area_t *area;

            if (::hd_init(0) != 0)
            {
                syslog(LOG_ERR, "ERROR: Unable to open hdshm device. Using dummy device.\n");
//                REEL_THROW();
                hda=(::hd_data_t*)malloc(sizeof(::hd_data_t));  // GA: Don't crash if no HD was found
                memset((void*)hda,0,sizeof(::hd_data_t));
                return;
            }

	    // Be tolerant...
	    for(int n=0;n<20;n++) {
		    area = ::hd_get_area(HDID_HDA);
		    if (area)
			    break;
                    syslog(LOG_INFO,"Wait for hdctrld (%i)\n",n);
		    sleep(1);
	    }
	    
	    if (area)
    	        syslog(LOG_INFO,"HDE control area: %p, mapped %p, pyhs %ld, len %x, hdp %i, hdc %i\n",area,
    	            area->mapped, area->physical, area->length,
    	              ((::hd_data_t volatile *)area->mapped)->hdp_running,((::hd_data_t volatile *)area->mapped)->hdc_running);
            else
            { // Create dummy
                syslog(LOG_ERR, "ERROR: Can't get HDE control area (hdctrld not running on HDE?). Using dummy.\n");
                int area_size = ALIGN_UP(sizeof(hd_data_t), 2*4096);
		::hdshm_reset();
		area = hd_create_area(HDID_HDA, NULL, area_size, HDSHM_MEM_HD);
            }

            if (!area)
            {
                syslog(LOG_ERR, "ERROR: Unable to get HDE control area.\n");
                //REEL_THROW();
                hda=(::hd_data_t*)malloc(sizeof(::hd_data_t));  // Don't crash - use dummy
                memset((void*)hda,0,sizeof(::hd_data_t));
                return;
            }

            hda = (::hd_data_t volatile *)area->mapped;
            for(int n=20;n>=0;n--) {
                hda->hdp_enable = 1;
                if (hda->hdp_running)
                    break;
                syslog(LOG_INFO,"Wait for hdplayer (%i)\n",n);
                sleep(1);
            }
        }
    }
}
