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

/* ReadBuffer.cpp */

#include "ReadBuffer.h"

ReadBuffer::ReadBuffer()
{
    Clear();
}

void ReadBuffer::AppendToBuffer(int length)
{
    // length must be <= readRangeLength_

    // trim buffer
    int bufferFullness = buffer_.size() - bufferPos_;
    if (bufferFullness)
    {
        // move data to buffer begin
        std::memmove(&buffer_[0], &buffer_[bufferPos_], bufferFullness);
    }
    buffer_.resize(bufferFullness);
    bufferPos_ = 0;

    // increase buffer size
    int newSize = bufferFullness + length;
    buffer_.reserve(newSize); // do not reserve more memory than necessary.
    buffer_.resize(newSize);

    // copy data from readRange_
    std::memcpy(&buffer_[bufferFullness], readRange_, length);

    readRange_ += length;
    readRangeLength_ -= length;
}

void ReadBuffer::Advance(int length)
{
    if (implicit_cast<int>(buffer_.size()) > bufferPos_)
    {
        // length must be <= buffer_.size() - bufferPos_
        bufferPos_ += length;
    }
    else
    {
        // length must be <= readRangeLength_
        readRange_ += length;
        readRangeLength_ -= length;
    }
}

void ReadBuffer::Clear()
{
    std::vector<Byte> emptyVec;
    buffer_.swap(emptyVec);

    readRange_ = nullptr;
    readRangeLength_ = 0;
    readRangeOrigLength_ = 0;
    bufferPos_ = 0;
}

Byte const *ReadBuffer::Read(int min, /*out */ int &length)
{
    // ::printf("ReadRange(%d)\n", min);
    // min must be > 0

    if (!buffer_.size() && readRange_ && readRangeLength_ >= min)
    {
        // most common (fast) case: buffer is empty, readRange_ big enough.
        // ::printf(" from range\n");

        // buffer is empty, but readRange_ is set and is long enough.
        length = readRangeLength_;
        return readRange_;
    }

    int bufferFullness = buffer_.size() - bufferPos_;

    // Try to avoid using the buffer:
    if (bufferFullness &&
        readRange_ &&
        bufferFullness + readRangeLength_ <= readRangeOrigLength_)
    {
        // ::printf(" Avoid buffer\n");
        readRangeLength_ += bufferFullness;
        readRange_ -= bufferFullness;
        buffer_.resize(0);
        bufferPos_ = 0;
        bufferFullness = 0;
    }

    if (bufferFullness >= min)
    {
        // ::printf(" from buffer\n");
        // serve request from buffer.
        length = bufferFullness;
        return &buffer_[bufferPos_];
    }
    else if (bufferFullness)
    {
        // buffer is not empty, but has not enoug data.
        if (bufferFullness + readRangeLength_ < min)
        {
            // not enough total data.
            // ::printf(" not enough total\n");
            return nullptr;
        }
        int appSize = min - bufferFullness + 16;
        if (appSize > readRangeLength_)
        {
            appSize = readRangeLength_;
        }
        AppendToBuffer(appSize);

        // ::printf(" from buffer (app)\n");

        // now buffer has at least min bytes of data
        length = buffer_.size() - bufferPos_;
        return &buffer_[bufferPos_];
    }
    else if (readRange_ && readRangeLength_ >= min)
    {
        // ::printf(" from range\n");

        // buffer is empty, but readRange_ is set and is long enough.
        length = readRangeLength_;
        return readRange_;
    }
    // ::printf(" fail\n");
    return nullptr;
}

void ReadBuffer::Set(Byte const *data, int length)
{
    // ::printf(" Set(%d)\n", length);
    readRange_ = data;
    readRangeLength_ = length;
    readRangeOrigLength_ = length;
}

void ReadBuffer::Unset()
{
    // ::printf(" Unset()\n");

    AppendToBuffer(readRangeLength_);

    readRange_ = nullptr;
    readRangeLength_ = 0;
}
