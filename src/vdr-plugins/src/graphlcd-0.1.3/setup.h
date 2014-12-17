/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  setup.h  -  Setup
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

#ifndef _GRAPHLCD_SETUP_H_
#define _GRAPHLCD_SETUP_H_


class cGraphLCDSetup
{
public:
	int PluginActive;
	int ShowDateTime;
	int ShowChannel;
	int ShowLogo;
	int ShowSymbols;
	int ShowETSymbols;
	int ShowProgram;
	int ShowTimebar;
	int ShowMenu;
	int ShowMessages;
	int ShowColorButtons;
	int ShowVolume;
	int ShowNotRecording; // Empty frame around not recording card's empty icons?
	int IdentifyReplayType;
	int ModifyReplayString;
	int ReplayLogo;
	int ScrollMode;
	int ScrollSpeed;
	int ScrollTime;
	int BrightnessActive;
	int BrightnessIdle;
	int BrightnessDelay;
	int ShowIdleClock;
public:
	cGraphLCDSetup(void);
	virtual ~cGraphLCDSetup(void);
	cGraphLCDSetup & operator= (const cGraphLCDSetup & setup);
	void CopyFrom(const cGraphLCDSetup * source);
};

extern cGraphLCDSetup GraphLCDSetup;

#endif
