/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  display.h  -  Display class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 **/

/***************************************************************************
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
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#ifndef GRAPHLCD_DISPLAY_H
#define GRAPHLCD_DISPLAY_H

#include <string>
#include <vector>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>

#include "global.h"
#include "layout.h"
#include "logolist.h"
#include "setup.h"
#include "state.h"
#include "widgets.h"

#include <vdr/thread.h>
#include <vdr/player.h>
#include <vdr/config.h>


#define LCDMAXCARDS 4
static const int kMaxTabCount = 10;

enum ThreadState
{
	Normal,
	Replay,
	Menu
};

// Display update Thread
class cGraphLCDDisplay : public cThread
{
public:
	cGraphLCDDisplay();
	~cGraphLCDDisplay();

	int Init(const char * CfgDir, unsigned int DisplayNumber);
	void DeInit() {active=false;};
	
	void SetChannel(int ChannelNumber);
	void SetClear(bool doNotShow);
	void SetOsdTitle();
	void SetOsdItem(const char * Text);
	void SetOsdCurrentItem();
	void Recording(const cDevice * Device , const char * Name);
	void Replaying(bool starting, eReplayMode replayMode);
	//void SetStatusMessage(const char * Msg);
	void SetOsdTextItem(const char * Text, bool Scroll);
	//void SetColorButtons(const char * Red, const char * Green, const char * Yellow, const char * Blue);
	void SetVolume(int Volume, bool Absolute);

	void Update();

protected:
	virtual void Action();

private:
	bool update;
	bool active;
	unsigned int displayNumber;

	cFontList fontList;
	GLCD::cBitmap * bitmap;
	const GLCD::cFont * largeFont;
	const GLCD::cFont * normalFont;
	const GLCD::cFont * smallFont;
	const GLCD::cFont * symbols;
	std::string cfgDir;
	std::string fontDir;
	std::string logoDir;

	ThreadState State;
	ThreadState LastState;

	cMutex mutex;
	cGraphLCDState * GraphLCDState;

	int menuTop;
	int menuCount;
	int tabCount;
	int tab[kMaxTabCount];
	int tabMax[kMaxTabCount];

	std::vector <std::string> textItemLines;
	int textItemTop;
	int textItemVisibleLines;

	bool showVolume;

	time_t CurrTime;
	time_t LastTime;
	time_t LastTimeCheckSym;
	time_t LastTimeModSym;
	struct timeval CurrTimeval;
	struct timeval UpdateAt;

	std::vector<cScroller> scroller;

	cGraphLCDLogoList * logoList;
	cGraphLCDLogo * logo;

	char szETSymbols[32];

	void DisplayChannel();
	void DisplayTime();
	void DisplayLogo();
	void DisplaySymbols();
	void DisplayProgramme();
	void DisplayReplay(tReplayState & replay);
	void DisplayMenu();
	void DisplayMessage();
	void DisplayTextItem();
	void DisplayColorButtons();
	void DisplayVolume();

	void UpdateIn(long usec);
	bool CheckAndUpdateSymbols();
	int WrapText(std::string & text, std::vector <std::string> & lines,
	             const GLCD::cFont * font, int maxTextWidth,
	             int maxLines = 100, bool cutTooLong = true);

	/** Check if replay index bigger as one hour */
	bool IndexIsGreaterAsOneHour(int Index) const;  
	/** Translate replay index to string with minute and second MM:SS */
	const char *IndexToMS(int Index) const;
	/** Compare Scroller with new Textbuffer*/
	bool IsScrollerTextChanged(const std::vector<cScroller> & scroller, const std::vector <std::string> & lines) const;
	/** Returns true if Logo loaded and active*/
	bool IsLogoActive() const;
	/** Returns true if Symbols loaded and active*/
	bool IsSymbolsActive() const;

	/** Set Brightness depends user activity */
	void SetBrightness();
#if VDRVERSNUM < 10716
	uint64 LastTimeBrightness;
#else
	uint64_t LastTimeBrightness;
#endif
	int nCurrentBrightness;
	bool bBrightnessActive;
	bool ShowIdleClock;
};

extern cGraphLCDDisplay Display;

#endif
