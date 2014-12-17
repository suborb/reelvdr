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

/* VideoStreamParser.cpp */

#include "VideoStreamParser.h"

#include "ReadBuffer.h"

#include <stdio.h>

#include <vector>

enum MpegStartcode
{
    MpegStartcodePictureStart        =  0x00,
    MpegStartcodeUserDataStart       =  0xB2,
    MpegStartcodeSequenceHeader      =  0xB3,
    MpegStartcodeUserExtensionStart  =  0xB5,
    MpegStartcodeSequenceEnd         =  0xB7,
    MpegStartcodeGroupStart          =  0xB8,
    MpegStartcodeNone                = 0x100
};

enum MpegPicType
{
    MpegPicTypeU,     // Unknown
    MpegPicTypeI,     // I-Picture
    MpegPicTypeP,     // P-Picture
    MpegPicTypeB      // B-Picture
};

static Byte const gopHeader_[] = {'\x00', '\x00', '\x01', '\xB8',  // Gop startcode
                                  '\x7F', '\xFF', '\xFF', '\xC0'}; // Timecode, closed gop
static ReadBuffer readBuffer_;

static MpegStartcode  currentStartcode_  = MpegStartcodeNone;
static Byte const    *readPtr_           = nullptr;
static int            readLength_        = 0;
static int            lastTempRef_       = -1;
static bool           saveIFrame_        = false;
static int            stillPicture_      = -1;
static int            consumePos_        = 0;
static int            feedPos_           = 0;

static std::vector<Byte>  iFrame_;

static void ClearIFrame()
{
    stillPicture_ = -1;
    iFrame_.resize(sizeof(gopHeader_));
    std::memcpy(&iFrame_[0], gopHeader_, sizeof(gopHeader_));
}

static void Consume(int size)
{
    consumePos_ += size;

    if (saveIFrame_ && size)
    {
        int iFrameCurrentSize = iFrame_.size();
        iFrame_.resize(iFrameCurrentSize + size);
        Byte *iFramePtr = &iFrame_[iFrameCurrentSize];
        std::memcpy(iFramePtr, readPtr_, size);
    }
    readPtr_ += size;
    readLength_ -= size;
    readBuffer_.Advance(size);
}

static void HandlePictureStart()
{
    readPtr_ = readBuffer_.Read(8, readLength_);
    if (readPtr_)
    {
        int tempRef = (readPtr_[4] << 2) | (readPtr_[5] >> 6);
        int pt = readPtr_[5] & 0x38;

        MpegPicType picType;
        switch (pt)
        {
        case 0x08:
            picType = MpegPicTypeI;
            break; 
        case 0x10:
            picType = MpegPicTypeP;
            break; 
        case 0x18:
            picType = MpegPicTypeB;
            break; 
        default:
            picType = MpegPicTypeU;
        } 

        if (picType == MpegPicTypeI || picType == MpegPicTypeP)
        {
            // If this picture has the same temporal reference as the previous one,
            // it is the second field (interlaced field picture encoding) and we just
            // continue.
            if (tempRef != lastTempRef_)
            {
                // Calculate the number of following Pictures (B-Pictures plus
                // this one).
                int picCount = tempRef - lastTempRef_;
                if (picCount < 0)
                {
                    // temporal references are mod 1024.
                    picCount += 1024;
                }
                // picCount >= 1
    
                ClearIFrame();
                saveIFrame_ = true;
    
                lastTempRef_ = tempRef;
            }
        }
        else
        {
            saveIFrame_ = false;
        }

        Consume(8);
        currentStartcode_ = MpegStartcodeNone;
    }
}

static void HandleSequenceHeader()
{
    saveIFrame_ = false;
    Consume(4);
    currentStartcode_ = MpegStartcodeNone;
}

static void HandleGroupStart()
{
    saveIFrame_ = false;
    Consume(4);
    // Reset temporal reference counting
    lastTempRef_ = -1;
    currentStartcode_ = MpegStartcodeNone;
}

static void HandleSequenceEnd()
{
    saveIFrame_ = false;
    if (iFrame_.size() > sizeof(gopHeader_))
    {
        stillPicture_ = feedPos_ - consumePos_;
    }
    Consume(4);
    currentStartcode_ = MpegStartcodeNone;
}

static void HandleOther()
{
    saveIFrame_ = false;
    Consume(4);
    currentStartcode_ = MpegStartcodeNone;
}

static void HandleStartcode()
{
    switch (currentStartcode_)
    {
    case MpegStartcodePictureStart:
        HandlePictureStart();
        break;
    case MpegStartcodeSequenceHeader:
        HandleSequenceHeader();
        break;
    case MpegStartcodeGroupStart:
        HandleGroupStart();
        break;
    case MpegStartcodeSequenceEnd:
        HandleSequenceEnd();
        break;
    default:
        HandleOther();
    }
}

static int FindNextStartcode(MpegStartcode &nextStartcode)
{
    if (readLength_ >= 4)
    {
        Byte const *p = readPtr_;
        Byte const *e = p + readLength_ - 3;
        while (p != e)
        {
            if (!*p && !p[1] && p[2] == 1)
            {
                int sc = p[3];
                if (sc == 0 ||
                    (sc >= 0xB0 &&
                     sc != MpegStartcodeUserDataStart &&
                     sc != MpegStartcodeUserExtensionStart))
                {
                    nextStartcode = MpegStartcode(sc);
                    return p - readPtr_;
                }
            }
            ++p ;
        }
    }
    nextStartcode = MpegStartcodeNone;
    return 0;
}

int VideoStreamParserFeed(void const *streamData, int length)
{
    readBuffer_.Set(static_cast<Byte const *>(streamData), length);

    feedPos_ += length;

    stillPicture_ = -1;

    // Read at least 4 bytes.
    readPtr_ = readBuffer_.Read(4, readLength_);
    while (readPtr_)
    {
        if (currentStartcode_ != MpegStartcodeNone)
        {
            // This may read more data, but won't consume:
            HandleStartcode();
        }
        else
        {
            // We are currently not handling a startcode.
            // Try to find next start code.
            MpegStartcode nextStartcode;
            int nextStartcodePos = FindNextStartcode(nextStartcode);
            if (nextStartcode != MpegStartcodeNone)
            {
                // Next startcode found.
                // Consume data up to the next startcode.
                if (nextStartcodePos)
                {
                    Consume(nextStartcodePos);
                }
                currentStartcode_ = nextStartcode;
            }
            else
            {
                // No startcode in sight, consume all data.
                if (readLength_ > 3)
                {
                    // The last 3 bytes could begin the next startcode!
                    Consume(readLength_ - 3);
                }
                // Read at least 4 bytes.
                readPtr_ = readBuffer_.Read(4, readLength_);
            }
        }
    }
    readBuffer_.Unset();
    return stillPicture_;
}

void VideoStreamParserGetLastIFrame(void const **data, int *length)
{
    *data = &iFrame_[0];
    *length = iFrame_.size();
}

void VideoStreamParserReset()
{
    readBuffer_.Clear();
    currentStartcode_ = MpegStartcodeNone;
    readPtr_          = nullptr;
    readLength_       = 0;
    lastTempRef_      = -1;

    feedPos_ = 0;
    consumePos_ = 0;

    stillPicture_ = -1;
    ClearIFrame();
    saveIFrame_       = false;
}
