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

// BspCommChannel.h

#ifndef P__BSP_COMM_CHANNEL_H
#define P__BSP_COMM_CHANNEL_H

#include <vdr/thread.h>
#include <signal.h>

#include "Reel.h"
#include <bspchannel.h>

typedef u_int CommChannelWord;


namespace Reel
{

    namespace Bsp
    {
        enum BspPort
        {
            BspPortPollStream       = 0,
            BspPortFocusSync        = 1,
            BspPortClockTimeIn      = 2,
            BspPortClockTimeOut     = 3,
            BspPortClockTimeCorrect = 4,
            BspPortSTC              = 5,
            BspPortAudioUnderflow   = 6,
            BspPortCount            = 7
        };

        // BspCommChannel handles all communication width the BSP.

        /**
          *  Singleton.
          */
        class BspCommChannel /* final */
        {
        public:
            static void Create(); /* atomic */
                ///< PURPOSE: Create the single instance if it does not exist.

            static void Destroy() NO_THROW;
                ///< PURPOSE: Destroy the single instance if it does exist.

            static BspCommChannel &Instance() NO_THROW;
                ///< PURPOSE: Return the single instance.
                ///<
                ///< REQUIRES: Instance must have been created.

            static void SetPicture();
            static void SetVideomode();

            static void InvalidateAll(void );

            static int BspAlive(void);
            static int checkGeneration(void);

            bspd_data_t volatile *bspd;
            bsp_channel_t * bsc_osd;
            bsp_channel_t * bsc_pes1, *bsc_pes2;
            bspshm_area_t * bsa_framebuffer;

        private:
            BspCommChannel(); // Allow construction only through Create().
            ~BspCommChannel() NO_THROW; // Allow destruction only through Destroy().

            // No assigning or copying
            BspCommChannel(const BspCommChannel &);
            BspCommChannel &operator=(const BspCommChannel &);

            void MicroSleep(int usecs);
            bool StartBsp();
            void StopBsp();

            int bsp_event;

            static BspCommChannel *instance_;
        };


        inline BspCommChannel &BspCommChannel::Instance() NO_THROW
        {
            REEL_ASSERT(instance_);
            return *instance_;
        }

        inline int BspCommChannel::BspAlive(void)
        {
            REEL_ASSERT(instance_);
            if (instance_->bspd)
            {
              return instance_->bspd->status;
            }
            return 0;
        }
    }
}

#endif // P__BSP_COMM_CHANNEL_H
