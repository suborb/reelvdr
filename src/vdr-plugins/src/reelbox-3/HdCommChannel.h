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

// HdCommChannel.h

#ifndef HD_COMM_CHANNEL_H_INCLUDED
#define HD_COMM_CHANNEL_H_INCLUDED

#include "Mutex.h"
#include "Reel.h"

#include "BspCommChannel.h"

#include <hdchannel.h>
#include <hdshm_user_structs.h>

namespace Reel
{
    class ReelBoxSetup;

    namespace HdCommChannel
    {

        void InitHda();

        class Channel
        {
            public:
                Channel();
                ~Channel() NO_THROW;

                void Open(int chNum);

                void Close() NO_THROW;

                void SendPacket(UInt type, hd_packet_header_t &header, int headerSize,
                        Byte const *data, int dataSize);

                template<typename T> void SendPacket(UInt type, T &header, 
                         Byte const *data, int dataSize);

            public:
                hd_channel_t *ch_;
            private:
                Mutex mutex_;
                unsigned int packetSeqNr_;
        };

        void Exit() NO_THROW;

        void Init();

        void SetAspect(int HDaspect = -1);

        void SetVideomode(int HDaspect = -1);

	void SetHWControl(ReelBoxSetup *rb);

	void SetVolume(int volume);

	void SetPicture(ReelBoxSetup *rb);
	
        template<typename T>
            inline void Channel::SendPacket(UInt type, T &header,
                    Byte const *data, int dataSize)
            {
                SendPacket(type, header.header, sizeof(T), data, dataSize);
            }

        extern ::hd_data_t volatile *hda;
        extern Channel chStream1;
        extern Channel chOsd;
    }
}

#endif // HD_COMM_CHANNEL_H_INCLUDED
