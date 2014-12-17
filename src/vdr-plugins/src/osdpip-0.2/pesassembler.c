/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/config.h>
#if VDRVERSNUM >= 10716
#include <vdr/tools.h>

#include "pesassembler.h"

cPesAssembler::cPesAssembler(void)
{
    data = NULL;
    size = 0;
    Reset();
}

cPesAssembler::~cPesAssembler()
{
    free(data);
}

void
cPesAssembler::Reset(void)
{
    tag = 0xFFFFFFFF;
    length = 0;
}

bool
cPesAssembler::Realloc(int Size)
{
    if (Size > size)
    {
        size = max(Size, 2048);
        data = (uchar *) realloc(data, size);
        if (!data)
        {
            esyslog("ERROR: can't allocate memory for PES assembler");
            length = 0;
            size = 0;
            return false;
        }
    }
    return true;
}

void
cPesAssembler::Put(uchar c)
{
    if (length < 4)
    {
        tag = (tag << 8) | c;
        if ((tag & 0xFFFFFF00) == 0x00000100)
        {
            if (Realloc(4))
            {
                *(uint32_t *) data = htonl(tag);
                length = 4;
            }
        }
        else if (length < 3)
            length++;
    }
    else if (Realloc(length + 1))
        data[length++] = c;
}

void
cPesAssembler::Put(const uchar * Data, int Length)
{
    while (length < 4 && Length > 0)
    {
        Put(*Data++);
        Length--;
    }
    if (Length && Realloc(length + Length))
    {
        memcpy(data + length, Data, Length);
        length += Length;
    }
}

int
cPesAssembler::PacketSize(const uchar * data)
{
    // we need atleast 6 bytes of data here !!!
    switch (data[3])
    {
    default:
    case 0x00 ... 0xB8:          // video stream start codes
    case 0xB9:                 // Program end
    case 0xBC:                 // Programm stream map
    case 0xF0 ... 0xFF:          // reserved
        return 6;

    case 0xBA:                 // Pack header
        if ((data[4] & 0xC0) == 0x40)   // MPEG2
            return 14;
        // to be absolutely correct we would have to add the stuffing bytes
        // as well, but at this point we only may have 6 bytes of data avail-
        // able. So it's up to the higher level to resync...
        //return 14 + (data[13] & 0x07); // add stuffing bytes
        else                    // MPEG1
            return 12;

    case 0xBB:                 // System header
    case 0xBD:                 // Private stream1
    case 0xBE:                 // Padding stream
    case 0xBF:                 // Private stream2 (navigation data)
    case 0xC0 ... 0xCF:          // all the rest (the real packets)
    case 0xD0 ... 0xDF:
    case 0xE0 ... 0xEF:
        return 6 + data[4] * 256 + data[5];
    }
}

#endif
