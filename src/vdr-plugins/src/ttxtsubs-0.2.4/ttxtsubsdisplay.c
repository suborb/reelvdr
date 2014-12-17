/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include <vdr/osd.h>
#include <vdr/osdbase.h>
#include <vdr/thread.h>
#include <vdr/font.h>
#include <vdr/config.h>
#include <vdr/tools.h>

#include "ttxtsubsglobals.h"
#include "ttxtsubsdisplay.h"
#include "utils.h"

// pageState
enum
{
    invalid = 0,
    collecting,
    interimshow,
    finished
};

// --------------------

class cOSDSelfMemory
{
public:
    cOSDSelfMemory(void) : mActive(0) {}
    ~cOSDSelfMemory() {}

    void SetActive(int a)
    {
        cMutexLock lock(&mLock);
        mActive = a;
        if (a)
            mThr = pthread_self();
    }
    int IsMe(void)
    {
        cMutexLock lock(&mLock);
        pthread_t thr = pthread_self();
        if (mActive && pthread_equal(thr, mThr))
            return 1;
        else
            return 0;
    }

private:
    cMutex mLock;
    int mActive;
    pthread_t mThr;
};

class cOSDSelfMemoryLock
{
public:
    cOSDSelfMemoryLock(cOSDSelfMemory *m) : mMem(m)
    {
        mMem->SetActive(1);
    }
    ~cOSDSelfMemoryLock()
    {
        mMem->SetActive(0);
    }

private:
    cOSDSelfMemory *mMem;
};

static cOSDSelfMemory gSelfMem;

// --------------------

cTtxtSubsDisplay::cTtxtSubsDisplay(void)
        :
        _pageState(invalid),
        _pageChanged(true),
        _mag(0),
        _no(0),
        _doDisplay(1),
        _osd(NULL),
        _osdLock(),
        _lastDataTime(NULL)
{
    memset(&_page.data, 0, sizeof(_page.data));
    _lastDataTime = (struct timeval *) calloc(1, sizeof(*_lastDataTime));

    _osdFont = cFont::CreateFont(Setup.FontOsd, globals.mFontSize);
    if (!_osdFont || !_osdFont->Height())
    {
        _osdFont = cFont::GetFont(fontOsd);
    }
}


cTtxtSubsDisplay::~cTtxtSubsDisplay(void)
{
    free(_lastDataTime);
    delete _osd;
    if (_osdFont != cFont::GetFont(fontOsd))
    {
        delete _osdFont;
    }
}


void cTtxtSubsDisplay::SetPage(int Pageno)  // Pageno is 0x000 to 0x799
{
    _mag = (Pageno >> 8) & 0xF;
    _no = Pageno & 0xFF;

    _pageState = invalid;
    ClearOSD();
}


void cTtxtSubsDisplay::Hide(void)
{
    if (gSelfMem.IsMe())
    {
        return;
    }

    cMutexLock lock(&_osdLock);
    _doDisplay = 0;
    ClearOSD();
}


void cTtxtSubsDisplay::Show(void)
{
    if (gSelfMem.IsMe())
    {
        return;
    }

    cMutexLock lock(&_osdLock);
    _doDisplay = 1;
    ShowOSD();
}


void cTtxtSubsDisplay::TtxtData(const uint8_t *Data, uint64_t sched_time)
{
    int mp;
    int mag; // X in ETSI EN 300 706
    int packet; // Y
    struct ttxt_data_field *d;

    // idle time - if we have been sitting on a half finished page for a while - show it
    if (Data == NULL && _pageState == collecting)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        // add second diff to usecs
        tv.tv_usec += 1000000 * (tv.tv_sec - _lastDataTime->tv_sec);

        if ((tv.tv_usec - _lastDataTime->tv_usec) > 500000)
        {
            _pageState = interimshow;
            if (_pageChanged)
            {
                ClearOSD();
                ShowOSD();
                _pageChanged = false;
            }
        }
    }

    if (Data == NULL)
        return;

    d = (struct ttxt_data_field *) Data;

    mp = unham(invtab[d->mag_addr_ham[0]], invtab[d->mag_addr_ham[1]]);
    mag = mp & 0x7;
    packet = (mp >> 3) & 0x1f;

    if (packet == 0)
    {
        int i, no;
        uint8_t fi[8]; // flags field inverted

        for (i = 0; i < 8; i++)
            fi[i] = invtab[d->data[i]];

        if (mag == _mag)  /* XXX: && ! magazine_serial */
        {
            if (_pageState == collecting)
            {
                _pageState = finished;
                if (_pageChanged)
                {
                    ClearOSD();
                    ShowOSD();
                   _pageChanged = false;
                }
            }
            if (_pageState == interimshow)
                _pageState = finished;
        }

        no = unham(fi[0], fi[1]);

        if (mag == _mag && no == _no)
        {

            _page.mag = mag;
            _page.no = no;
            _page.flags = 0;

            if (fi[3] & 0x80)  // Erase Page
            {
                memset(&_page.data, 0, sizeof(_page.data));
                _page.flags |= erasepage;
                _pageChanged = true;
            }
            if (fi[5] & 0x20) // Newsflash
                _page.flags |= newsflash;
            if (fi[5] & 0x80) // Subtitle
                _page.flags |= subtitle;
            if (fi[6] & 0x02) // Suppress Header
                _page.flags |= suppress_header;
            // if(fi[6] & 0x08) // Update Indicator
            // if(fi[6] & 0x20) // Interrupted Sequence
            if (fi[6] & 0x80) // Inhibit Display
                _page.flags |= inhibit_display;
            // if(fi[7] & 0x02) // Magazine Serial

            _page.national_charset = ((fi[7] & 0x80) >> 7) +
                                    ((fi[7] & 0x20) >> 4) + ((fi[7] & 0x08) >> 1);

            if (_pageState != collecting)
            {
                int diff = sched_time - cTimeMs::Now();
                //printf("Got sched_time %llx, diff %d\n", sched_time, diff);
                if (diff > 10) cCondWait::SleepMs(diff);
            }

            _pageState = collecting;
            gettimeofday(_lastDataTime, NULL);

            for (i = 0; i < 32; i++)
                _page.data[0][i] = invtab[d->data[i+8]];
        }
    }
    else if (mag == _page.mag && packet < TTXT_DISPLAYABLE_ROWS &&
             (_pageState == collecting || _pageState == interimshow))
    {
        // mag == _page.mag: The magazines can be sent interleaved
        int i;
        for (i = 0; i < 40; i++)
        {
            unsigned char c = invtab[d->data[i]];
            if (c != _page.data[packet][i])
            {
                _page.data[packet][i] = c;
                _pageChanged = true;
            }
        }

        _pageState = collecting;
        gettimeofday(_lastDataTime, NULL);
    }
}

tColor SubtitleColorMap[8][3] =
{
    // foreground, outline,        background
    {clrBlack,     clrBlack,       clrTransparent},
    {clrRed,       clrBlack,       clrTransparent},
    {clrGreen,     clrBlack,       clrTransparent},
    {clrYellow,    clrBlack,       clrTransparent},
    {clrBlue,      clrTransparent, clrWhite      },
    {clrMagenta,   clrBlack,       clrTransparent},
    {clrCyan,      clrBlack,       clrTransparent},
    {clrWhite,     clrBlack,       clrTransparent}
};

void cTtxtSubsDisplay::UpdateSubtitleTextLines()
{
    // We are making some assumptions here, which may not be correct:
    //
    // - only teletext lines with at least one "box begin" spacing attribute are
    //   used for subtitle texts
    // - a single teletext line results in a single subtitle line, independently of
    //   the number of "box begin" / "box end" spacing attributes
    // - each subtitle line can only have a single color (maybe a color change within
    //   a teletext line should start a new subtitle line)

    _numberOfSubTitleTextLines = 0;
    char textBuffer[81];

    for (int row = 1; row < 24; row++)
    {
        if (!_page.data[row][0]) continue; // Row is empty

        bool withinBox = false;
        int textBufferIndex = 0;
        _subTitleTextLines[_numberOfSubTitleTextLines].color = 7; // default white

        for (int column = 0; column < 40; column++)
        {
            // leave out parity bit!
            uint8_t teletextCharacter = _page.data[row][column] & 0x7f;

            if (teletextCharacter < 0x10) // Process spacing attributes
            {
                if (teletextCharacter == 0x0b) // box begin
                {
                    withinBox = true;

                    // if we already have chars in buffer, add space
                    if (textBufferIndex > 0) textBuffer[textBufferIndex++] = ' ';
                }
                else if (teletextCharacter == 0x0a) // box begin
                {
                    withinBox = false;
                }
                else if (teletextCharacter >= 0x00 && teletextCharacter <= 0x07) // color attributes
                {
                    _subTitleTextLines[_numberOfSubTitleTextLines].color = teletextCharacter;
                }
            }
            else if (withinBox)
            {
                // skip leading spaces
                if (textBufferIndex == 0 && teletextCharacter == 0x20) continue;

                if (teletextCharacter >= 0x20)
                {
                    uint16_t aux = ttxt_laG0_la1_char(0, _page.national_charset, teletextCharacter);
                    if (aux & 0xff00) textBuffer[textBufferIndex++] = (aux & 0xff00) >> 8;
                    textBuffer[textBufferIndex++] = aux & 0x00ff;
                }
            }
        }

        // strip extra spaces
        while (textBufferIndex > 0 && textBuffer[textBufferIndex-1] == ' ') textBufferIndex--;

        if (textBufferIndex > 0)
        {
           textBuffer[textBufferIndex] = 0;
           _subTitleTextLines[_numberOfSubTitleTextLines].text = textBuffer;
           _numberOfSubTitleTextLines++;
           if (_numberOfSubTitleTextLines > MAXTTXTROWS) return;
        }
    }
}

void cTtxtSubsDisplay::DrawOutlinedText(int x, int y, const char* text, tColor textColor, tColor outlineColor,
  tColor backgroundColor, const cFont* font)
{
    for (int horizontalOffset = -globals.mOutlineWidth; horizontalOffset <= globals.mOutlineWidth; horizontalOffset++)
    {
        for (int verticalOffset = -globals.mOutlineWidth; verticalOffset <= globals.mOutlineWidth; verticalOffset++) 
        {
            if (horizontalOffset || verticalOffset)
            {
                _osd->DrawText(x + horizontalOffset, y + verticalOffset, text, outlineColor, backgroundColor, font);
            }
        }
    }
    _osd->DrawText(x, y, text, textColor, backgroundColor, font);
}

void cTtxtSubsDisplay::ShowOSD(void)
{
    cOSDSelfMemoryLock selfmem(&gSelfMem);
    cMutexLock lock(&_osdLock);

    if (!globals.mRealDoDisplay) return;
    if (!_doDisplay) return;
    if (_pageState != interimshow && _pageState != finished) return;

    UpdateSubtitleTextLines();

    if (_numberOfSubTitleTextLines <= 0) return;

    DELETENULL(_osd);

    int width = cOsd::OsdWidth();
    int textHeight = _numberOfSubTitleTextLines * _osdFont->Height();
    int height = textHeight + 2 * globals.mOutlineWidth + 10;

    _osd = cOsdProvider::NewOsd(cOsd::OsdLeft(), cOsd::OsdTop() + cOsd::OsdHeight() - height, OSD_LEVEL_SUBTITLES + 1);
    tArea Areas[] = { { 0, 0, width - 1, height - 1, 8 } };
    if (Setup.AntiAlias && _osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk)
    {
        _osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    }
    else
    {
        tArea Areas[] = { { 0, 0, width - 1, height - 1, 4 } };
        _osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    }

    _osd->DrawRectangle(0, 0, width - 1, height - 1, clrTransparent);

    if(globals.mOutlineWidth < 2){
       int maxwidth = 0;
       for (int line = 0; line < _numberOfSubTitleTextLines; line++) {
           int linewidth = _osdFont->Width(_subTitleTextLines[line].text);
           if(linewidth > maxwidth)
             maxwidth = linewidth;
       }
       maxwidth+=40;
       _osd->DrawRectangle((width/2)-(maxwidth/2), 0, (width/2)+(maxwidth/2), height - 1, 0x8a000000);
    }

    for(int textLineIndex = 0; textLineIndex < _numberOfSubTitleTextLines; textLineIndex++)
    {
        int lineWidth = _osdFont->Width(_subTitleTextLines[textLineIndex].text) + 2 * globals.mOutlineWidth + 5;
        int lineHeight = (textHeight / _numberOfSubTitleTextLines);
        int x = (width - lineWidth) / 2;
        int y = lineHeight * textLineIndex + globals.mOutlineWidth + 5;
        tColor foregroundColor = SubtitleColorMap[_subTitleTextLines[textLineIndex].color][0];
        tColor outlineColor = SubtitleColorMap[_subTitleTextLines[textLineIndex].color][1];
        tColor backgroundColor = SubtitleColorMap[_subTitleTextLines[textLineIndex].color][2];

        DrawOutlinedText(x, y, _subTitleTextLines[textLineIndex].text, foregroundColor, outlineColor, backgroundColor,
          _osdFont);
    }
    _osd->Flush();
}


void cTtxtSubsDisplay::ClearOSD(void)
{
    cOSDSelfMemoryLock selfmem(&gSelfMem);
    cMutexLock lock(&_osdLock);

    DELETENULL(_osd);
}

