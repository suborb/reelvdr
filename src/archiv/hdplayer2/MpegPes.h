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

// MpegPes.h
 
#ifndef MPEG_PES_H_INCLUDED
#define MPEG_PES_H_INCLUDED

#include "AudioDecoder.h"

namespace Reel
{
    namespace Mpeg
    {
        //--------------------------------------------------------------------------------------------------------------

        enum MediaType
        {
            MediaTypeVideo,
            MediaTypeAudio,
            MediaTypeNone
        };

        //--------------------------------------------------------------------------------------------------------------

        enum StreamId
        {
            StreamIdMpegPrivateStream1 = 0xBD,
            StreamIdAudioStream0       = 0xC0,
            StreamIdAudioStream31      = 0xDF,
            StreamIdVideoStream0       = 0xE0,
            StreamIdVideoStream15      = 0xEF
        };

        //--------------------------------------------------------------------------------------------------------------

        enum SubStreamId
        {
            SubStreamIdNone            = 0,
            SubStreamIdAc3Stream0      = 0x80,
            SubStreamIdAc3Stream7      = 0x87,
            SubStreamIdDtsStream0      = 0x88,
            SubStreamIdDtsStream7      = 0x8F,
            SubStreamIdPcmStream0      = 0xA0,
            SubStreamIdPcmStream7      = 0xA7
        };

        //--------------------------------------------------------------------------------------------------------------

        class Player;

        //--------------------------------------------------------------------------------------------------------------

        class EsPacket /* final */
        {
        public:
            EsPacket(Byte const *&pesPacket, UInt &pesPacketLength);
                ///< Parse a PES-Packet and extract the ES-data and metadata.
                ///< pesPacket must point to a series of complete PES-Packets of total length pesPacketLength.
                ///< The parameter are modified to point afert the parsed packet.
                ///< Throws a ReelException if the PES-Packet cannot be parsed.

            EsPacket(Byte const *esData, UInt esDataLength,
                     StreamId streamId, SubStreamId subStreamId,
                     MediaType mediaType, UInt pts=0)
            :   data_(esData),
                mediaType_(mediaType),
                dataLength_(esDataLength),
                pts_(pts),
                streamId_(streamId),
                subStreamId_(subStreamId),
                hasPts_(pts != 0)
            {
            }

            /* Default copy operations and destructor are ok. */

            AudioDecoder *GetAudioDecoder() const NO_THROW;
                ///< Return the audio decoder for audio decoding the es data.

            Byte const *GetData() const NO_THROW;
                ///< Return a pointer to the start of the ES-payload-data.
                ///< The data is backed by the pesPacket data given in the constructor
                ///< (it is not owned by this object).

            UInt GetDataLength() const NO_THROW;
                ///< Return the length of the ES-payload-data in Bytes.

            MediaType GetMediaType() const NO_THROW;
                ///< Return the media type of the ES-payload-data.

            UInt GetPts() const NO_THROW;
                ///< Return the presentation timestamp if present.

            StreamId GetStreamId() const NO_THROW;
                ///< Return the PS stream id.

            SubStreamId GetSubStreamId() const NO_THROW;
                ///< Return the PS substream id if the stream is of type MpegPrivateStream1.

            bool HasPts() const NO_THROW;
                ///< Return true iff a presentation timestamp is attached to this packet.

        private:
            void ReadPts(Byte const *&pesPacket, UInt &pesLength);

            void Parse(Byte const *&pesPacket, UInt &pesPacketLength);

            Byte const    *data_;
            MediaType      mediaType_;
            AudioDecoder  *audioDecoder_;
            UInt           dataLength_;
            UInt           pts_;
            StreamId       streamId_;
            SubStreamId    subStreamId_;
            bool           hasPts_;
        };

        //--------------------------------------------------------------------------------------------------------------

        inline bool EsPacket::HasPts() const NO_THROW
        {
            return hasPts_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline AudioDecoder *EsPacket::GetAudioDecoder() const NO_THROW
        {
            return audioDecoder_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline Byte const *EsPacket::GetData() const NO_THROW
        {
            return data_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline UInt EsPacket::GetDataLength() const NO_THROW
        {
            return dataLength_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline MediaType EsPacket::GetMediaType() const NO_THROW
        {
            return mediaType_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline UInt EsPacket::GetPts() const NO_THROW
        {
            return pts_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline StreamId EsPacket::GetStreamId() const NO_THROW
        {
            return streamId_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline SubStreamId EsPacket::GetSubStreamId() const NO_THROW
        {
            REEL_ASSERT(streamId_ == StreamIdMpegPrivateStream1);
            return subStreamId_;
        }

        //--------------------------------------------------------------------------------------------------------------

        inline void EsPacket::ReadPts(Byte const *&pesPacket, UInt &pesLength)
        {
            if (pesLength < 5)
            {
                REEL_THROW();
            }
            /*Byte const *d = pesPacket;
            ::printf("pts> %02X %02X %02X %02X %02X %02X %02X %02X\n",
                        d[0], d[1], d[2], d[3],
                        d[4], d[5], d[6], d[7]);*/

            // We ignore the highest pts bit.
            pts_ = ((pesPacket[0] & 0x06) << 29) |
                   ( pesPacket[1]         << 22) |
                   ((pesPacket[2] & 0xFE) << 14) |
                   ( pesPacket[3]         <<  7) |
                   ( pesPacket[4]         >>  1);
            hasPts_ = true;
            pesPacket += 5;
            pesLength -= 5;
        }
    }
}

#endif // def MPEG_PES_H_INCLUDED
