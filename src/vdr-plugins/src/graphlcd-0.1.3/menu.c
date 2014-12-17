/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  menu.c  -  setup menu class
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

#include "menu.h"


cGraphLCDMenuSetup::cGraphLCDMenuSetup()
{
	static const char * showDateTimeValues[3];
	showDateTimeValues[0] = tr("no");
	showDateTimeValues[1] = tr("yes");
	showDateTimeValues[2] = tr("not in menu");
	static const char * showSymbolsValues[3];
	showSymbolsValues[0] = tr("no");
	showSymbolsValues[1] = tr("yes");
	showSymbolsValues[2] = tr("compressed");
	static const char * showLogoValues[4];
	showLogoValues[0] = tr("no");
	showLogoValues[1] = tr("auto");
	showLogoValues[2] = tr("medium");
	showLogoValues[3] = tr("large");
	static const char * scrollModeValues[3];
	scrollModeValues[0] = tr("never");
	scrollModeValues[1] = tr("once");
	scrollModeValues[2] = tr("always");

	newGraphLCDSetup = GraphLCDSetup;

	//Add(new cMenuEditBoolItem(tr("Plugin active"), &newGraphLCDSetup.PluginActive));
	Add(new cMenuEditStraItem(tr("Show Date/Time"), &newGraphLCDSetup.ShowDateTime, 3, showDateTimeValues));
	Add(new cMenuEditBoolItem(tr("Show Channel"), &newGraphLCDSetup.ShowChannel));
	Add(new cMenuEditStraItem(tr("Show Logo"), &newGraphLCDSetup.ShowLogo, 4, showLogoValues));
	Add(new cMenuEditStraItem(tr("Show Symbols"), &newGraphLCDSetup.ShowSymbols, 3, showSymbolsValues));
	Add(new cMenuEditBoolItem(tr("Show ET Symbols"), &newGraphLCDSetup.ShowETSymbols));
	Add(new cMenuEditBoolItem(tr("Show Program"), &newGraphLCDSetup.ShowProgram));
	Add(new cMenuEditBoolItem(tr("Show Timebar"), &newGraphLCDSetup.ShowTimebar));
	Add(new cMenuEditBoolItem(tr("Show Menu"), &newGraphLCDSetup.ShowMenu));
	Add(new cMenuEditBoolItem(tr("Show Messages"), &newGraphLCDSetup.ShowMessages));
	//Add(new cMenuEditBoolItem(tr("Show Color Buttons"), &newGraphLCDSetup.ShowColorButtons));
	Add(new cMenuEditBoolItem(tr("Show Volume"), &newGraphLCDSetup.ShowVolume));
	Add(new cMenuEditBoolItem(tr("Show free Tuner"), &newGraphLCDSetup.ShowNotRecording));
	Add(new cMenuEditBoolItem(tr("Identify replay type"), &newGraphLCDSetup.IdentifyReplayType));
	if (newGraphLCDSetup.IdentifyReplayType)
	{
		Add(new cMenuEditBoolItem(tr("Modify replay string"), &newGraphLCDSetup.ModifyReplayString));
	//	Add(new cMenuEditStraItem(tr("Show Logo on Replay"), &newGraphLCDSetup.ReplayLogo, 4, showLogoValues));
	}
	Add(new cMenuEditStraItem(tr("Scroll text lines"), &newGraphLCDSetup.ScrollMode, 3, scrollModeValues));
	Add(new cMenuEditIntItem(tr("Scroll speed"), &newGraphLCDSetup.ScrollSpeed, 1, 10));
	Add(new cMenuEditIntItem(tr("Scroll time interval"), &newGraphLCDSetup.ScrollTime, 100, 2000));
	Add(new cMenuEditIntItem(tr("Brightness on user activity"), &newGraphLCDSetup.BrightnessActive, 0, 100));
	Add(new cMenuEditIntItem(tr("Brightness on user inactivity"), &newGraphLCDSetup.BrightnessIdle, 0, 100));
	Add(new cMenuEditIntItem(tr("Brightness delay [s]"), &newGraphLCDSetup.BrightnessDelay, 0, 600));
	Add(new cMenuEditBoolItem(tr("Show clock on user inactivity"), &newGraphLCDSetup.ShowIdleClock));
}

void cGraphLCDMenuSetup::Store()
{
	SetupStore("PluginActive", GraphLCDSetup.PluginActive  = newGraphLCDSetup.PluginActive);
	SetupStore("ShowDateTime",GraphLCDSetup.ShowDateTime = newGraphLCDSetup.ShowDateTime);
	SetupStore("ShowChannel", GraphLCDSetup.ShowChannel = newGraphLCDSetup.ShowChannel);
	SetupStore("ShowLogo",    GraphLCDSetup.ShowLogo = newGraphLCDSetup.ShowLogo);
	SetupStore("ShowSymbols", GraphLCDSetup.ShowSymbols = newGraphLCDSetup.ShowSymbols);
	SetupStore("ShowETSymbols",GraphLCDSetup.ShowETSymbols = newGraphLCDSetup.ShowETSymbols);
	SetupStore("ShowProgram", GraphLCDSetup.ShowProgram = newGraphLCDSetup.ShowProgram);
	SetupStore("ShowTimebar", GraphLCDSetup.ShowTimebar = newGraphLCDSetup.ShowTimebar);
	SetupStore("ShowMenu",    GraphLCDSetup.ShowMenu = newGraphLCDSetup.ShowMenu);
	SetupStore("ShowMessages",GraphLCDSetup.ShowMessages = newGraphLCDSetup.ShowMessages);
	//SetupStore("ShowColorButtons",GraphLCDSetup.ShowColorButtons = newGraphLCDSetup.ShowColorButtons);
	SetupStore("ShowVolume",  GraphLCDSetup.ShowVolume = newGraphLCDSetup.ShowVolume);
	SetupStore("ShowNotRecording", GraphLCDSetup.ShowNotRecording = newGraphLCDSetup.ShowNotRecording);
	SetupStore("IdentifyReplayType", GraphLCDSetup.IdentifyReplayType = newGraphLCDSetup.IdentifyReplayType);
	SetupStore("ReplayLogo", GraphLCDSetup.ReplayLogo = newGraphLCDSetup.ReplayLogo);
	SetupStore("ModifyReplayString", GraphLCDSetup.ModifyReplayString = newGraphLCDSetup.ModifyReplayString);
	SetupStore("ScrollMode", GraphLCDSetup.ScrollMode = newGraphLCDSetup.ScrollMode);
	SetupStore("ScrollSpeed", GraphLCDSetup.ScrollSpeed = newGraphLCDSetup.ScrollSpeed);
	SetupStore("ScrollTime", GraphLCDSetup.ScrollTime = newGraphLCDSetup.ScrollTime);
	SetupStore("BrightnessActive", GraphLCDSetup.BrightnessActive = newGraphLCDSetup.BrightnessActive);
	SetupStore("BrightnessIdle", GraphLCDSetup.BrightnessIdle = newGraphLCDSetup.BrightnessIdle);
	SetupStore("BrightnessDelay", GraphLCDSetup.BrightnessDelay = newGraphLCDSetup.BrightnessDelay);
	SetupStore("ShowIdleClock", GraphLCDSetup.ShowIdleClock = newGraphLCDSetup.ShowIdleClock);
}
