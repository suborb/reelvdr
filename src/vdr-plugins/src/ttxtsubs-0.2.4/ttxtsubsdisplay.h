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

#include "teletext.h"
#include <vdr/tools.h>
#include <vdr/osd.h>

struct ttxt_data_field;
struct timeval;

class cOsd;
class cFont;

#define MAXTTXTROWS 5

struct SubtitleTextLine
{
    cString text;
    int color;
};

class cTtxtSubsDisplay
{
public:
    cTtxtSubsDisplay(void);
    ~cTtxtSubsDisplay(void);

    void SetPage(int Pageno); // Pageno is 0x000 to 0x799
    void Hide(void);
    void Show(void);
    void TtxtData(const uint8_t *, uint64_t sched_time = 0);

protected:
    void ShowOSD();
    void ClearOSD(void);

private:
    void UpdateSubtitleTextLines();
    void DrawOutlinedText(int x, int y, const char* text, tColor textColor, tColor outlineColor, tColor backgroundColor,
      const cFont* font);

private:
    int _pageState;
    bool _pageChanged;
    int _mag;
    int _no;
    int _doDisplay;
    struct ttxt_page _page;
    cOsd* _osd;
    cMutex _osdLock;
    struct timeval* _lastDataTime;
    const cFont* _osdFont;
    SubtitleTextLine _subTitleTextLines[MAXTTXTROWS];
    int _numberOfSubTitleTextLines;
};
