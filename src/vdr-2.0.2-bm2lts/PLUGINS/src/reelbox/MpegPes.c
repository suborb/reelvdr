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

// MpegPes.c
 
#include "MpegPes.h"

#include "AudioDecoderIec60958.h"
#include "AudioDecoderMpeg1.h"
#include "AudioDecoderNull.h"
#include "AudioDecoderPcm.h"

#include <stdio.h> // FIXME

namespace Reel
{
    namespace Mpeg
    {
        EsPacket::EsPacket(Byte const *&pesPacket, UInt &pesPacketLength)
        :   mediaType_(MediaTypeNone)
        {
            Parse(pesPacket, pesPacketLength);

            audioDecoder_ = AudioDecoderNull::Instance();
            if (streamId_ >= StreamIdVideoStream0 && streamId_ <= StreamIdVideoStream15)
            {
                mediaType_ = MediaTypeVideo;
            }
            if (streamId_ >= StreamIdAudioStream0 && streamId_ <= StreamIdAudioStream31)
            {
                mediaType_ = MediaTypeAudio;
                audioDecoder_ = AudioDecoderMpeg1::Instance();
            }
            if (streamId_ == StreamIdMpegPrivateStream1)
            {
                if (subStreamId_ >= SubStreamIdAc3Stream0 && subStreamId_ <= SubStreamIdAc3Stream7)
                {
                    mediaType_ = MediaTypeAudio;
                    audioDecoder_ = AudioDecoderAc3::Instance();
                }
                if (subStreamId_ >= SubStreamIdDtsStream0 && subStreamId_ <= SubStreamIdDtsStream7)
                {
                    mediaType_ = MediaTypeAudio;
                    audioDecoder_ = AudioDecoderDts::Instance();
                }
                if (subStreamId_ >= SubStreamIdPcmStream0 && subStreamId_ <= SubStreamIdPcmStream7)
                {
                    mediaType_ = MediaTypeAudio;
                    audioDecoder_ = AudioDecoderPcm::Instance();
                }
            }
        }

        void EsPacket::Parse(Byte const *&pesPacket, UInt &pesPacketLength)
        {
            Byte const *pesPtr = pesPacket;
            UInt pesLen = pesPacketLength;

            /*Byte const *d = pesPacket;
            ::printf("> %08X\n", (UInt)d);
            for (int n = 0; n < 3; ++n)
            {
                ::printf("  %04X : %02X %02X %02X %02X %02X %02X %02X %02X | ", n * 16,
                            d[0], d[1], d[2], d[3],
                            d[4], d[5], d[6], d[7]);
                ::printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
                            d[8], d[9], d[10], d[11],
                            d[12], d[13], d[14], d[15]);
                d += 16;
            }*/

            hasPts_ = false;

            Byte const *pesPacketEnd = pesPtr + pesLen;

            if (pesLen >= 4 &&
                pesPtr[0] == 0 &&
                pesPtr[1] == 0 &&
                pesPtr[2] == 1)
            {
                /* stream id */
                streamId_ = static_cast<StreamId>(pesPtr[3]);

                /* pes packet length */

                UInt pesLength;
                if (streamId_ == 0xB9)
                {
                    pesLength = implicit_cast<UInt>(-2);
                }
                else 
                {
                    if (pesLen < 8)
                    {
                        REEL_THROW();
                    }
                    if (streamId_ == 0xBA)
                    {
                        // Pack header.
                        if ((pesPtr[4]>>4) == 4)
                        {
                            pesLength = (pesPtr[13] & 7) + 8;
                        }
                        else
                        {
                            pesLength = 6;
                        }
                    }
                    else
                    {
                        pesLength = pesPtr[4] << 8 | pesPtr[5];
                    }
                }

                pesPtr += 6;

                pesPacket = pesPtr + pesLength;
                pesPacketLength -= pesLength + 6;

                if (!(streamId_ >= StreamIdVideoStream0 && streamId_ <= StreamIdVideoStream15) &&
                    !(streamId_ >= StreamIdAudioStream0 && streamId_ <= StreamIdAudioStream31) &&
                    streamId_ != StreamIdMpegPrivateStream1)
                {
                    return;
                }

                /* skip stuffing bytes */
                while (*pesPtr == 0xFF && ++pesPtr != pesPacketEnd)
                {
                    -- pesLength;
                }

                if (pesLength)
                {
                    Byte b = *pesPtr++;
                    -- pesLength;
                    // ::printf("[1] pesLength = %d\n", pesLength);
                    if ((b & 0xC0) == 0x80)
                    {
                        // MPEG-2
                        // ::printf("MPEG-2\n");
                        if (pesLength >= 2)
                        {
                            b = *pesPtr++;
                            Int pesHeaderLength = *pesPtr++;
                            pesLength -= 2;
                            if (b & 0x80)
                            {
                                // pts is present
                                ReadPts(pesPtr, pesLength);
                                pesHeaderLength -= 5;
                            }
                            if (pesHeaderLength >= 0 && pesLength >= implicit_cast<UInt>(pesHeaderLength))
                            {
                                pesPtr += pesHeaderLength;
                                pesLength -= pesHeaderLength;
                                
                                if (streamId_ == StreamIdMpegPrivateStream1)
                                {
                                    if (!pesLength)
                                    {
                                        REEL_THROW();
                                    }
                                    subStreamId_ = static_cast<SubStreamId>(*pesPtr++);
                                    -- pesLength;
                                }
                                data_ = pesPtr;
                                dataLength_ = pesLength;
                                return;
                            }
                        }
                    }
                    else
                    {
                        // MPEG-1
                        // ::printf("MPEG-1\n");
                        if ((b & 0xC0) == 0x40)
                        {
                            if (pesLength < 2)
                            {
                                REEL_THROW();
                            }
                            b = pesPtr[1];
                            pesPtr += 2;
                            pesLength -= 2;
                        }
                        b &= 0xF0;
                        if (b == 0x20 || b == 0x30)
                        {
                            // pts is present
                            ReadPts(-- pesPtr, ++ pesLength);
                            if (b == 0x30)
                            {
                                // skip dts
                                if (pesLength < 5)
                                {
                                    REEL_THROW();
                                }
                                pesPtr += 5;
                                pesLength -= 5;
                            }
                            // ::printf("pts present ###########################################################################\n");
                        }
                        data_ = pesPtr;
                        dataLength_ = pesLength;



                        /*Byte const *d = data_;
                        ::printf(">> %08X\n", (UInt)d);
                        for (int n = 0; n < 3; ++n)
                        {
                            ::printf("  %04X : %02X %02X %02X %02X %02X %02X %02X %02X | ", n * 16,
                                        d[0], d[1], d[2], d[3],
                                        d[4], d[5], d[6], d[7]);
                            ::printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
                                        d[8], d[9], d[10], d[11],
                                        d[12], d[13], d[14], d[15]);
                            d += 16;
                        }*/
            


                        return;
                    }
                }
            }
            REEL_THROW();
        }
    }
}
