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

// BspCommChannel.c

#include "BspCommChannel.h"
#include "ReelBoxMenu.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vdr/tools.h>

#include <bspchannel.h>


#define DEBUG_BSP_COM(format, args ...) printf(format, ## args)
//#define DEBUG_BSP_COM(format, args ...)

#define E(x) if (!(x)) goto errexit;

namespace Reel
{
    namespace Bsp
    {
        BspCommChannel::BspCommChannel()
        :   bsc_osd(0),
            bsc_pes1(0),
            bsc_pes2(0)
        {
        }

        BspCommChannel::~BspCommChannel() NO_THROW
        {
        }

        void BspCommChannel::MicroSleep(int usecs)
        {
            timespec ts;
            ts.tv_sec = usecs / 1000000;
            ts.tv_nsec = (usecs % 1000000) * 1000;
            ::nanosleep(&ts, 0);
        }

        bool BspCommChannel::StartBsp()
        {
            printf("START BSP\n");
            esyslog("INFO [reelbox]: START BSP\n");
            instance_->bspd->bsp_auto_restart=1;

            if (!instance_->bspd->status)
                instance_->bspd->bsp_enable=1;

            while (!instance_->bspd->status)
            {
                MicroSleep(100*1000);
            }
            MicroSleep(100*1000);
            bsp_event=instance_->bspd->event;
            return true;
        }


        void BspCommChannel::StopBsp()
        {
            printf("STOP BSP\n");
            esyslog("INFO [reelbox]: STOP BSP\n");
            bspd->bsp_auto_restart=0;
            bspd->bsp_kill=1;
            MicroSleep(500000);
            return;
        }

        void BspCommChannel::InvalidateAll(void)
        {
            printf("BspCommChannel: INVALIDATE ALL\n");
    //      bsp_channel_invalidate(instance_->bsc_osd,0);
    //      bsp_channel_invalidate(instance_->bsc_pes1,0);
    //      bsp_channel_invalidate(instance_->bsc_cmd,0);
        }

        int BspCommChannel::checkGeneration(void)
        {
            if (instance_->bspd->event!=instance_->bsp_event) {
                DEBUG_BSP_COM("CHECK GENERATION now %i, last %i\n",instance_->bspd->event,instance_->bsp_event);
                InvalidateAll();
                instance_->bsp_event=instance_->bspd->event;
                return 1;
            }
            return 0;
        }

        BspCommChannel *BspCommChannel::instance_ = 0;

        void BspCommChannel::Create() /* atomic */
        {
            if (!instance_)
            {
                instance_ = new BspCommChannel;
                bsp_init(0);
                instance_->bspd = bsp_channel_init();

                if (instance_->bspd->demo_running) {
                    instance_->bspd->demo_stop=1;
                    while(instance_->bspd->demo_running) // Wait for bspd-demo to finish
                    {
                        usleep(1000);
                    }
                    usleep(100*1000);
                }

                instance_->bsc_osd=bsp_channel_open(BSPCH_OSD); // Already created by bspd
                instance_->bsc_pes1=bsp_channel_open(BSPCH_PES1); // Already created by bspd
                instance_->bsc_pes2=bsp_channel_open(BSPCH_PES2); // Already created by bspd

                instance_->bspd->BspPortFocusSync=0;

                instance_->StartBsp();
#if 0
                // Wait until Framebuffer is available
                instance_->bsa_framebuffer=NULL;
                while(!instance_->bsa_framebuffer) {
                    instance_->bsa_framebuffer=bsp_get_area(BSPID_FRAMEBUFFER_RGBA_0);
                    usleep(1000);
                }
#endif
                DEBUG_BSP_COM("init done\n");
            }
        }

        void BspCommChannel::Destroy() NO_THROW
        {
            if (!instance_) return;
            DEBUG_BSP_COM ("\033[0;41  %s  \033[0m\n", __PRETTY_FUNCTION__);
            instance_->StopBsp();
            bsp_channel_close(instance_->bsc_osd);
            bsp_channel_close(instance_->bsc_pes1);
            bsp_channel_close(instance_->bsc_pes2);

            // bsp_free_area(instance_->bsa_framebuffer);
            delete instance_;
            instance_ = 0;
        }

        void BspCommChannel::SetPicture()
        {
            if (!instance_)
                return;

            DEBUG_BSP_COM ("\033[0;41  %s  \033[0m\n", __PRETTY_FUNCTION__);
            bspd_data_t volatile *bspdi=instance_->bspd;
            bspdi->picture.values[0]=RBSetup.brightness;
            bspdi->picture.values[1]=RBSetup.contrast;
            bspdi->picture.values[2]=RBSetup.colour;
            bspdi->picture.values[3]=RBSetup.sharpness;
            bspdi->picture.values[4]=RBSetup.gamma;
            bspdi->picture.values[5]=RBSetup.flicker;
            bspdi->picture.changed++;
        }

        void BspCommChannel::SetVideomode()
        {
            if (!instance_)
                return;

            DEBUG_BSP_COM ("\033[0;41  %s  \033[0m\n", __PRETTY_FUNCTION__);
            bspd_data_t volatile *bspdi=instance_->bspd;

            bspdi->video_mode.main_mode=RBSetup.main_mode;
            bspdi->video_mode.sub_mode=RBSetup.sub_mode;
            bspdi->video_mode.display_type=RBSetup.display_type;
            bspdi->video_mode.aspect=RBSetup.aspect;
            bspdi->video_mode.norm=RBSetup.norm;
            bspdi->video_mode.framerate=RBSetup.framerate;
            bspdi->video_mode.resolution=RBSetup.resolution;
            bspdi->video_mode.flags= ((RBSetup.scart_mode<<BSP_VM_FLAGS_SCART_SHIFT)&BSP_VM_FLAGS_SCART_MASK)|
		    ((RBSetup.deint<<BSP_VM_FLAGS_DEINT_SHIFT)&BSP_VM_FLAGS_DEINT_MASK);
            bspdi->video_mode.changed++;

            // Wait until mode is set
            for (int i=0;i<50;i++)
            {
               if (bspdi->video_mode.changed==bspdi->video_mode.last_changed)
                   break;

                //MicroSleep(100);
                usleep (100 * 1000);
            }
        }
    }
}
