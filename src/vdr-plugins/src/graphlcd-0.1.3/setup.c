/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  setup.c  -  Setup
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

#include "setup.h"


cGraphLCDSetup GraphLCDSetup;

cGraphLCDSetup::cGraphLCDSetup(void)
:	PluginActive(1),
	ShowDateTime(1),
	ShowChannel(1),
	ShowLogo(1),
	ShowSymbols(1),
	ShowETSymbols(0),
	ShowProgram(1),
	ShowTimebar(1),
	ShowMenu(1),
	ShowMessages(1),
	ShowColorButtons(1),
	ShowVolume(1),
	ShowNotRecording(0),
	IdentifyReplayType(1),
	ModifyReplayString(1),
	ReplayLogo(2),
	ScrollMode(1),
	ScrollSpeed(8),
	ScrollTime(500),
	BrightnessActive(85),
	BrightnessIdle(20),
	BrightnessDelay(5),
	ShowIdleClock(0)
{
}

cGraphLCDSetup::~cGraphLCDSetup(void)
{
}

cGraphLCDSetup & cGraphLCDSetup::operator=(const cGraphLCDSetup & setup)
{
	CopyFrom(&setup);
	return *this;
}

void cGraphLCDSetup::CopyFrom(const cGraphLCDSetup * source)
{
	PluginActive = source->PluginActive;
	ShowDateTime = source->ShowDateTime;
	ShowChannel = source->ShowChannel;
	ShowLogo = source->ShowLogo;
	ShowSymbols = source->ShowSymbols;
	ShowETSymbols = source->ShowETSymbols;
	ShowProgram = source->ShowProgram;
	ShowTimebar = source->ShowTimebar;
	ShowMenu = source->ShowMenu;
	ShowMessages = source->ShowMessages;
	ShowColorButtons = source->ShowColorButtons;
	ShowVolume = source->ShowVolume;
	ShowNotRecording = source->ShowNotRecording;
	IdentifyReplayType = source->IdentifyReplayType;
	ModifyReplayString = source->ModifyReplayString;
	ReplayLogo = source->ReplayLogo;
	ScrollMode = source->ScrollMode;
	ScrollSpeed = source->ScrollSpeed;
	ScrollTime = source->ScrollTime;
	BrightnessActive = source->BrightnessActive;
	BrightnessIdle = source->BrightnessIdle;
	BrightnessDelay = source->BrightnessDelay;
	ShowIdleClock = source->ShowIdleClock;
}
