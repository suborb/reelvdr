/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  display.c  -  Display class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 **/

#include <ctype.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/time.h>

#include <algorithm>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>

#include "display.h"
#include "global.h"
#include "setup.h"
#include "state.h"
#include "strfct.h"

#include <vdr/tools.h>
#include <vdr/menu.h>
#include <vdr/config.h>
#include <vdr/recording.h>

#include "compat.h"

#define MAXLINES_MSG  4
#define MAXLINES_TEXT 16
#define FILENAME_EXTERNAL_TRIGGERED_SYMBOLS "/tmp/graphlcd_symbols"

// tiny:     0..48
#define MINY_T  0
#define MAXY_T  48

// small:   49..61
#define MINY_S  (MAXY_T+1)
#define MAXY_S  61

// medium:  62..127
#define MINY_M  (MAXY_S+1)
#define MAXY_M  127

// large:  128..
#define MINY_L  (MAXY_M+1)
#define MAXY_L  9999



int     FRAME_SPACE_X;
int     FRAME_SPACE_XB;
int     FRAME_SPACE_Y;
int     FRAME_SPACE_YB;
int     TEXT_OFFSET_X;
int     TEXT_OFFSET_Y_TIME;
int     TEXT_OFFSET_Y_CHANNEL;
int     TEXT_OFFSET_Y_TITLE;
int     SYMBOL_SPACE;
int     TIMEBAR_HEIGHT;



cGraphLCDDisplay::cGraphLCDDisplay()
:	active(false),
	bitmap(NULL),
	GraphLCDState(NULL)
{
	cfgDir = "";
	fontDir = "";
	logoDir = "";

  CurrTime = time(NULL);
  LastTime = CurrTime-58;
  CurrTimeval.tv_sec = 0;
  CurrTimeval.tv_usec = 0;
  timerclear(&UpdateAt);
  LastTimeCheckSym = CurrTime;

  State = Normal;
  LastState = Normal;

	menuTop = 0;
	menuCount = 0;
	tabCount = 0;
	for (int i = 0; i < kMaxTabCount; i++)
		tab[i] = 0;
	tabMax[0] = 0;
	tabMax[1] = 66;
	tabMax[2] = 100;
	tabMax[3] = 100;
	tabMax[4] = 100;
	tabMax[5] = 100;
	tabMax[6] = 100;
	tabMax[7] = 100;
	tabMax[8] = 100;
	tabMax[9] = 100;

	showVolume = false;

	logo = NULL;
	logoList = NULL;

	strcpy (szETSymbols, "");

	nCurrentBrightness = -1;
	LastTimeBrightness = 0;
	bBrightnessActive = true;
	
	ShowIdleClock = false;
}

cGraphLCDDisplay::~cGraphLCDDisplay()
{
	active = false;
	Cancel(3);

	delete GraphLCDState;
	delete bitmap;
	delete logoList;
}

int cGraphLCDDisplay::Init(const char * CfgDir, unsigned int displayNumber)
{
        // esyslog(" in %s %s @ %d", __FUNCTION__, __FILE__, __LINE__);
	if (!CfgDir)
		return 2;
	cfgDir = CfgDir;
	fontDir = cfgDir + "/fonts";
	logoDir = cfgDir + "/logos";

	LCD = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
	if (!LCD)
	{
		 esyslog("graphlcd: ERROR: Failed creating display object %s\n", GLCD::Config.driverConfigs[displayNumber].name.c_str());
		return 1;
	}

	logoList = new cGraphLCDLogoList(logoDir.c_str(), cfgDir.c_str());
	if (!logoList)
	{
		esyslog("graphlcd plugin: ERROR out of memory\n");
		return 1;
	}

	std::string fontListFile = cfgDir + "/fonts.conf";
	if (fontList.Load(fontListFile) == false)
	{
		esyslog("graphlcd plugin: ERROR: Could not load %s!\n", fontListFile.c_str());
		return 1;
	}
       // esyslog("graphLCD: %s %s %s",GLCD::Config.driverConfigs[displayNumber].name.c_str(), logoDir.c_str(), fontListFile.c_str());
	Start();
	return 0;
}

void cGraphLCDDisplay::Action(void)
{
	int cleared=0;
	if (LCD->Init() != 0)
	{
		esyslog("graphlcd: ERROR: Failed initializing display\n");
		return;
	}

	bitmap = new GLCD::cBitmap(LCD->Width(), LCD->Height());
	if (!bitmap)
	{
		esyslog("graphlcd plugin: ERROR creating drawing bitmap\n");
		return;
	}

	largeFont = fontList.GetFont("Large Font");
	if (largeFont == NULL)
	{
		esyslog("graphlcd plugin: ERROR: No \"Large Font\" specified!\n");
		return;
	}
	normalFont = fontList.GetFont("Normal Font");
	if (normalFont == NULL)
	{
		esyslog("graphlcd plugin: ERROR: No \"Normal Font\" specified!\n");
		return;
	}
	smallFont = fontList.GetFont("Small Font");
	if (smallFont == NULL)
	{
		esyslog("graphlcd plugin: ERROR: No \"Small Font\" specified!\n");
		return;
	}
	symbols = fontList.GetFont("Symbol Font");
	if (symbols == NULL)
	{
		esyslog("graphlcd plugin: ERROR: No \"Symbol Font\" specified!\n");
		return;
	}
	
	if (bitmap->Width() < 240)
	{
		FRAME_SPACE_X  = 0;
		FRAME_SPACE_XB = 1;
		TEXT_OFFSET_X  = 2;
	}
	else
	{
		FRAME_SPACE_X  = 2;
		FRAME_SPACE_XB = 2;
		TEXT_OFFSET_X  = 4;
	}

	if (bitmap->Height() <= MAXY_T)
	{
		// very small display
		FRAME_SPACE_Y         = 0;
		FRAME_SPACE_YB        = 1;
		TEXT_OFFSET_Y_TIME    = 1;
		TEXT_OFFSET_Y_CHANNEL = 1;
		TEXT_OFFSET_Y_TITLE   = 1;
		SYMBOL_SPACE          = 1;
		TIMEBAR_HEIGHT        = 3;
	}
	else if (bitmap->Height() <= MAXY_S)
	{
		// small display
		FRAME_SPACE_Y         = 0;
		FRAME_SPACE_YB        = 1;
		TEXT_OFFSET_Y_TIME    = 1;
		TEXT_OFFSET_Y_CHANNEL = 3;
		TEXT_OFFSET_Y_TITLE   = 1;
		SYMBOL_SPACE          = 1;
		TIMEBAR_HEIGHT        = 3;
	}
	else if (bitmap->Height() <= MAXY_M)
	{
		// medium display
		FRAME_SPACE_Y         = 0;
		FRAME_SPACE_YB        = 1;
		TEXT_OFFSET_Y_TIME    = 1;
		TEXT_OFFSET_Y_CHANNEL = 3;
		TEXT_OFFSET_Y_TITLE   = 1;
		SYMBOL_SPACE          = 1;
		TIMEBAR_HEIGHT        = 6;
	}
	else
	{
		// large display
		FRAME_SPACE_Y         = 2;
		FRAME_SPACE_YB        = 2;
		TEXT_OFFSET_Y_TIME    = 2;
		TEXT_OFFSET_Y_CHANNEL = 5;
		TEXT_OFFSET_Y_TITLE   = 5;
		SYMBOL_SPACE          = 2;
		TIMEBAR_HEIGHT        = 5;
	}

	GraphLCDState = new cGraphLCDState();
	if (!GraphLCDState)
		return;

	dsyslog("graphlcd plugin: Display update thread started (pid=%d)", getpid());

	LCD->Refresh(true);
	active = true;
	update = true;
	while (active)
	{
		if (GraphLCDSetup.PluginActive)
		{
			cleared=0;
			CurrTime = time(NULL);
			if (timerisset(&UpdateAt))
			{
				// timed Update enabled
				if (gettimeofday(&CurrTimeval, NULL) == 0)
				{
					// get current time
					if (CurrTimeval.tv_sec > UpdateAt.tv_sec)
					{
						timerclear(&UpdateAt);
						update = true;
					}
					else if(CurrTimeval.tv_sec == UpdateAt.tv_sec &&
					        CurrTimeval.tv_usec > UpdateAt.tv_usec)
					{
						timerclear(&UpdateAt);
						update = true;
					}
				}
			}
			if (GraphLCDSetup.ShowVolume && !update && showVolume)
			{
				if (TimeMs() - GraphLCDState->GetVolumeState().lastChange > 2000)
				{
					update = true;
					showVolume = false;
				}
			}

			SetBrightness();

			if(ShowIdleClock && (State != Normal)) 
			{
				SystemExec("reelfpctl -displaymode 0");	 
				ShowIdleClock = false;
			}
			switch (State)
			{
				case Normal:
					// check and update external triggered symbols
					if (GraphLCDSetup.ShowETSymbols)
					{
						if (CurrTime != LastTimeCheckSym)
						{
							update |= CheckAndUpdateSymbols();
							LastTimeCheckSym = CurrTime;
						}
					}

					{
						std::vector<cScroller>::iterator it;
						for (it = scroller.begin(); it != scroller.end(); it++)
						{
							if (it->NeedsUpdate())
								update = true;
						}
					}

					// update Display if animated Logo is present, and an update is necessary
					if (!update && IsLogoActive() && logo->Count() > 1 &&
					    (TimeMs() - logo->LastChange() >= logo->Delay()))
					{
						update = true;
					}

					// update Display every minute or due to an update
					if (CurrTime/60 != LastTime/60 || update)
					{
						timerclear(&UpdateAt);
						update = false;
						
						if((GraphLCDSetup.BrightnessIdle == nCurrentBrightness) && 
						   (GraphLCDSetup.BrightnessIdle != GraphLCDSetup.BrightnessActive) &&
						   GraphLCDSetup.ShowIdleClock) 
						{
							if(!ShowIdleClock) SystemExec("reelfpctl -toptext \"\" -displaymode 1");
							else cCondWait::SleepMs(100);
							ShowIdleClock = true;	 
						} 
						else 
						{
							if(ShowIdleClock) SystemExec("reelfpctl -displaymode 0");	 
							ShowIdleClock = false;
							bitmap->Clear();
							DisplayTime();
							DisplayLogo();
							DisplayChannel();
							DisplaySymbols();
							DisplayProgramme();
							DisplayVolume();
							DisplayMessage();
							LCD->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
							LCD->Refresh(false);
						}
						LastTime = CurrTime;
					}
					else
					{
#if VDRVERSNUM < 10314
						usleep(100000);
#else
						cCondWait::SleepMs(100);
#endif
					}
					break;

				case Replay:
					{
						tReplayState replay = GraphLCDState->GetReplayState();
						if (replay.control)
						{
							{
								update = false;
								std::vector<cScroller>::iterator it;
								for (it = scroller.begin(); it != scroller.end(); it++)
								{
									if (it->NeedsUpdate())
										update = true;
								}
							}
							// update Display if animated Logo is present, and an update is necessary
							if (!update && IsLogoActive() && logo->Count() > 1 &&
								TimeMs() - logo->LastChange() >= logo->Delay())
							{
								update = true;
							}

							// update Display every second or due to an update
							if (CurrTime != LastTime || update)
							{
								// but only, if something has changed
#if VDRVERSNUM < 10716
								if (replay.total / FRAMESPERSEC != replay.totalLast / FRAMESPERSEC ||
									replay.current / FRAMESPERSEC != replay.currentLast / FRAMESPERSEC ||
									CurrTime/60 != LastTime/60 ||
									update)
#else
								if (replay.total / DEFAULTFRAMESPERSECOND != replay.totalLast / DEFAULTFRAMESPERSECOND ||
									replay.current / DEFAULTFRAMESPERSECOND != replay.currentLast / DEFAULTFRAMESPERSECOND ||
									CurrTime/60 != LastTime/60 ||
									update)
#endif
								{
									timerclear(&UpdateAt);
									update = false;
									bitmap->Clear();
									DisplayTime();
									DisplayLogo();
									DisplayReplay(replay);
									//DisplaySymbols();
									DisplayVolume();
									DisplayMessage();
									LCD->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
									LCD->Refresh(false);
									LastTime = CurrTime;
								}
								else
								{
#if VDRVERSNUM < 10314
									usleep(100000);
#else
									cCondWait::SleepMs(100);
#endif
								}
							}
							else
							{
#if VDRVERSNUM < 10314
								usleep(100000);
#else
								cCondWait::SleepMs(100);
#endif
							}
						}
						else
						{
//							State = Normal;
// Floh: This line causes lcd switch back to normal-mode (=tuner) while it shows rubbish thing ("question marks").
// I found out, lcd was showing rubbish thing because the state was switching between normal and replay quickly. So the screen was flickering.
// It happens for example if you watch dvd and return to root-menu. I'm not sure if this line really needed.
							Update();
							cCondWait::SleepMs(100);
						}
					}
					break;

				case Menu:
					if (GraphLCDSetup.ShowMenu)
					{
						// update Display every minute or due to an update
						if (CurrTime/60 != LastTime/60 || update)
						{
							timerclear(&UpdateAt);
							update = false;

							bitmap->Clear();
							DisplayTime();
							DisplayMenu();
							DisplayTextItem();
							DisplayVolume();
							DisplayMessage();
							//DisplayColorButtons();
							LCD->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
							LCD->Refresh(false);
							LastTime = CurrTime;
						}
						else
						{
#if VDRVERSNUM < 10314
							usleep(100000);
#else
							cCondWait::SleepMs(100);
#endif
						}
					}
					else
					{
						//GraphLCDState.OsdClear();

						State = LastState;
						// activate delayed Update
#if VDRVERSNUM < 10314
						usleep(100000);
#else
						cCondWait::SleepMs(100);
#endif
					}
					break;

				default:
					break;
			}
		}
		else
		{
#if VDRVERSNUM < 10314
			usleep(100000);
#else
			cCondWait::SleepMs(100);			
#endif
			if (!cleared) {
				bitmap->Clear();
				LCD->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
				LCD->Refresh(false);		
				cleared=1;
			}
		}
	}
	LCD->DeInit();
	dsyslog("graphlcd plugin: Display update thread ended (pid=%d)", getpid());
}

void cGraphLCDDisplay::SetChannel(int ChannelNumber)
{
	if (ChannelNumber == 0)
		return;

	mutex.Lock();
	cChannel * ch = Channels.GetByNumber(ChannelNumber);
	if (GraphLCDSetup.ShowLogo)
	{
		ePicType picType;

		switch (GraphLCDSetup.ShowLogo)
		{
			case 1:   // auto
				if (bitmap->Height() <= MAXY_M)
					picType = ptLogoMedium;
				else
					picType = ptLogoLarge;
				break;
			case 2:   // medium
				picType = ptLogoMedium;
				break;
			case 3:   // large
				picType = ptLogoLarge;
				break;
			default:  // should not happen at the moment !!
				picType = ptLogoSmall;
				break;
		}
#if VDRVERSNUM >= 10300
		char strTmp[64];
		strcpy(strTmp, (const char *) ch->GetChannelID().ToString());
		char * strId = strstr(strTmp, "-") + 1;

        // Cut off RID (because favourite channels have RID=100)
        char *strCutter = strstr(strId, "-");
        for(int i=0; i<3; ++i)
        {
            strCutter = strstr(strCutter, "-");
            if(strCutter)
                ++strCutter;
        }
        if(strCutter)
            *(strCutter-1) = '\0'; //Cut off if there is RID

		logo = logoList->GetLogo(strId, picType);
#else
		char strId[16];
		sprintf(strId, "%d", ch->Sid());
		logo = logoList->GetLogo(strId, picType);
#endif
		if (logo)
			logo->First(TimeMs());
	}
	else
	{
		logo = NULL;
	}
	bBrightnessActive = true;
	Update();
	mutex.Unlock();
}

void cGraphLCDDisplay::SetClear(bool doNotShow)
{
	mutex.Lock();

	textItemLines.clear();
    textItemTop = 0;
	tabCount = 0;
	for (int i = 0; i < kMaxTabCount; i++)
		tab[i] = 0;

	mutex.Unlock();
	
    if (!doNotShow) //avoid flickering of display when playimg mp3
    {
        if (State == Menu)
        {
            State = LastState;
            
            // activate delayed Update
            UpdateIn(100000);
        }
        else
        {
            Update();
        }
    }
}

void cGraphLCDDisplay::SetOsdTitle()
{
	UpdateIn(0);  // stop delayed Update
	mutex.Lock();
	if (State != Menu)
	{
		menuTop = 0;
		LastState = State;
		State = Menu;
	}
	mutex.Unlock();
	// activate delayed Update
	UpdateIn(100000);
}

void cGraphLCDDisplay::SetOsdItem(const char * Text)
{
    //printf("-------------------cGraphLCDDisplay::SetOsdItem, Text = %s-------------\n", Text); 

    /*
    const char *nameptr = Text;
    while(*nameptr)
    {
        printf("%08x : %c\n", *nameptr, *nameptr);
        ++nameptr;
    }*/

	int iAT, t;
	std::string str;
	std::string::size_type pos1, pos2;

	mutex.Lock();

	UpdateIn(0);  // stop delayed Update
	str = Text;
	pos1 = 0;
	pos2 = str.find('\t');
    //printf("-------------------pos2 = %d, str.length() = %d, std::string::npos = %d-------------\n", pos2, str.length(), std::string::npos); 
	iAT = 0;
	while (pos1 < str.length() && pos2 != std::string::npos)
	{
		iAT++;
		t = std::min(normalFont->Width(str.substr(pos1), pos2 - pos1), (tabMax[iAT] * bitmap->Width()) / 100);
		tab[iAT] = std::max(tab[iAT], t);
		tabCount = std::max(tabCount, iAT);
		pos1 = pos2 + 1;
		pos2 = str.find('\t', pos1);
	}
    //iAT = 0;
    //tabCount = 0;
    //pos1 = 0;
    //t = -1;
    //printf("------------pos1 = %d, pos2 = %d, tabCount = %d, iAT = %d, tab[0] = %d, tab[1] = %d, t = %d------------\n", pos1, pos2, tabCount, iAT, tab[0], tab[1], t);
	UpdateIn(1000000);  // start delayed Update
	mutex.Unlock();
}

void cGraphLCDDisplay::SetOsdCurrentItem()
{
	UpdateIn(100000); //XXX
}

void cGraphLCDDisplay::Replaying(bool starting, eReplayMode replayMode)
{
	if (starting)
	{
		if (State != Menu)
		{
			State = Replay;
		}
		else
		{
			LastState = Replay;
		}
		if (GraphLCDSetup.ReplayLogo)
		{
			ePicType picType;

			switch (GraphLCDSetup.ReplayLogo)
			{
				case 1:   // auto
					if (bitmap->Height() <= MAXY_M)
						picType = ptLogoMedium;
					else
						picType = ptLogoLarge;
					break;
				case 2:   // medium
					picType = ptLogoMedium;
					break;
				case 3:   // large
					picType = ptLogoLarge;
					break;
				default:  // should not happen at the moment !!
					picType = ptLogoSmall;
					break;
			}
			switch (replayMode)
			{
				default:        
				case eReplayNormal : logo = logoList->GetLogo("REPLAY-VDR", picType);break;
				case eReplayMusic  : logo = logoList->GetLogo("REPLAY-MUSIC", picType);break;
				case eReplayDVD    : logo = logoList->GetLogo("REPLAY-DVD", picType);break;
				case eReplayFile   : logo = logoList->GetLogo("REPLAY-FILE", picType);break;
				case eReplayImage  : logo = logoList->GetLogo("REPLAY-IMAGE", picType);break;
				case eReplayAudioCD: logo = logoList->GetLogo("REPLAY-CD", picType);break;
			}
			if (logo)
				logo->First(TimeMs());
		}
		else
		{
			logo = NULL;
		}
	}
	else
	{
		if (State != Menu)
		{
			State = Normal;
		}
		else
		{
			LastState = Normal;
		}
	}
	bBrightnessActive = true;
	Update();
}

void cGraphLCDDisplay::SetOsdTextItem(const char * Text, bool Scroll)
{
	static const char * lastText = NULL;
	tOsdState osd;

	osd = GraphLCDState->GetOsdState();
	mutex.Lock();
	if (Text)
	{
		if (osd.textItem.length() == 0)
			lastText = NULL;
		int maxTextLen = bitmap->Width() - 2 * FRAME_SPACE_X - 2 * TEXT_OFFSET_X;
		WrapText(osd.textItem, textItemLines, normalFont, maxTextLen);
		textItemLines.push_back("");
		if (lastText != Text)
		{
			lastText = Text;
			textItemTop = 0;
		}
	}
	else
	{
		if (Scroll)
		{
			if (textItemTop > 0)
				textItemTop--;
		}
		else
		{
			if (textItemTop < (int) textItemLines.size() - 2)
				textItemTop++;
		}
	}
	mutex.Unlock();
	UpdateIn(100000);
}

void cGraphLCDDisplay::Update()
{
	update = true;
}

void cGraphLCDDisplay::DisplayTime()
{
	static char buffer[32];
	static char month[5];
	int FrameWidth, TextLen, yPos;
	struct tm tm_r;

	if (GraphLCDSetup.ShowDateTime == 1 ||
	    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
	{
		FrameWidth = std::max(bitmap->Width() - 2 * FRAME_SPACE_X, 1);
		if (State == Normal || State == Replay)
		{
			if (IsLogoActive()) // Logo enabled & available
			{
				FrameWidth = std::max(FrameWidth - FRAME_SPACE_XB - logo->Width() - 2, (unsigned int) 1);
			}
			if (bitmap->Height() <= MAXY_M)
			{
				// tiny, small & medium display
				if (IsSymbolsActive()) // Symbols enabled
				{
					FrameWidth = std::max(FrameWidth - FRAME_SPACE_XB - symbols->TotalWidth(), 1);
				}
			}
		}
		yPos = FRAME_SPACE_Y;

		// draw Rectangle
		bitmap->DrawRoundRectangle(FRAME_SPACE_X, yPos,
				FRAME_SPACE_X + FrameWidth - 1,
				yPos + normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_TIME - 1,
				GLCD::clrBlack, true, (TEXT_OFFSET_Y_TIME >= 2) ? 4 : 1);

		if (CurrTime == 0)
			time(&CurrTime);
		tm * tm = localtime_r(&CurrTime, &tm_r);

		strncpy(month, (char *)(tr("JanFebMarAprMayJunJulAugSepOctNovDec") + 3 * tm->tm_mon), 3);
		month[3] = 0;
		snprintf(buffer, sizeof(buffer), "%s %2d.%s  %d:%02d", (const char *) WeekDayName(tm->tm_wday), tm->tm_mday, month, tm->tm_hour, tm->tm_min);
		TextLen = normalFont->Width(buffer);

		if (TextLen > std::max(FrameWidth - 2 * TEXT_OFFSET_X, 1))
		{
			snprintf(buffer, sizeof(buffer), "%d.%s  %d:%02d", tm->tm_mday, month, tm->tm_hour, tm->tm_min);
			TextLen = normalFont->Width(buffer);
		}

		if (TextLen > std::max(FrameWidth - 2 * TEXT_OFFSET_X, 1))
		{
			snprintf(buffer, sizeof(buffer), "%d.%d. %d:%02d", tm->tm_mday, tm->tm_mon+1, tm->tm_hour, tm->tm_min);
			TextLen = normalFont->Width(buffer);
		}

		if (TextLen > std::max(FrameWidth - 2 * TEXT_OFFSET_X, 1))
		{
			snprintf(buffer, sizeof(buffer), "%d:%02d", tm->tm_hour, tm->tm_min);
			TextLen = normalFont->Width(buffer);
		}

		if (TextLen < std::max(FrameWidth - 2 * TEXT_OFFSET_X, 1))
		{
			bitmap->DrawText(FRAME_SPACE_X + FrameWidth - TextLen - TEXT_OFFSET_X,
					yPos + TEXT_OFFSET_Y_TIME,
					FRAME_SPACE_X + FrameWidth - 1,
					buffer, normalFont, GLCD::clrWhite);
		}
		else
		{
			bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X,
					yPos + TEXT_OFFSET_Y_TIME,
					FRAME_SPACE_X + FrameWidth - 1,
					buffer, normalFont, GLCD::clrWhite);
		}
	}
}

void cGraphLCDDisplay::DisplayChannel()
{
	int FrameWidth, yPos;
	tChannelState channel;

	channel = GraphLCDState->GetChannelState();
	if (GraphLCDSetup.ShowChannel)
	{
		FrameWidth = std::max(bitmap->Width() - 2 * FRAME_SPACE_X, 1);
		if (State == Normal)
		{
			if (IsLogoActive()) // Logo enabled & available
			{
				FrameWidth = std::max(FrameWidth - FRAME_SPACE_XB - logo->Width() - 2, (unsigned int) 1);
			}
			if (bitmap->Height() <= MAXY_M)
			{
				// tiny, small & medium display
				if (IsSymbolsActive()) // Symbols enabled
				{
					FrameWidth = std::max(FrameWidth - FRAME_SPACE_XB - symbols->TotalWidth(), 1);
				}
			}
		}

		if (GraphLCDSetup.ShowDateTime == 1 ||
		    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
		{
			yPos = FRAME_SPACE_Y + normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;

			if (bitmap->Height() >= MINY_L)
			{
				// align bottom border with logo
				if (IsLogoActive()) // Logo enabled & available
				{
					yPos += std::max((unsigned int) 0, FRAME_SPACE_Y + logo->Height() + 2 - yPos -
							(normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_CHANNEL));
				}
			}
		}
		else
		{
			yPos = FRAME_SPACE_Y;
		}

		// draw Rectangle
		bitmap->DrawRoundRectangle(FRAME_SPACE_X, yPos,
				FRAME_SPACE_X + FrameWidth - 1,
				yPos + normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_CHANNEL - 1,
				GLCD::clrBlack, true, (TEXT_OFFSET_Y_CHANNEL >= 4) ? 4 : 1);

		if (channel.strTmp.length() > 0)
		{
			bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X,
					yPos + TEXT_OFFSET_Y_CHANNEL,
					FRAME_SPACE_X + FrameWidth - 1,
					channel.strTmp, normalFont, GLCD::clrWhite);
		}
		else if (channel.str.length() > 0)
		{
			bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X,
					yPos + TEXT_OFFSET_Y_CHANNEL,
					FRAME_SPACE_X + FrameWidth - 1,
					channel.str, normalFont, GLCD::clrWhite);
		}
	}
}

bool cGraphLCDDisplay::IsLogoActive() const
{
	if ((State==Normal && GraphLCDSetup.ShowLogo) ||
	    (State==Replay && GraphLCDSetup.IdentifyReplayType && GraphLCDSetup.ReplayLogo))
	{
		return logo != NULL;
	}
	return false;
}

void cGraphLCDDisplay::DisplayLogo()
{
	int x;
	int y;

	if (IsLogoActive())
	{
		if (logo->Count() > 1)
		{
			unsigned long long t = TimeMs();
			if (t - logo->LastChange() >= logo->Delay())
			{
				if (!logo->Next(t))
					logo->First(t);
			}
		}

		x = std::max(bitmap->Width() - FRAME_SPACE_X - logo->Width() - 2, (unsigned int) 0);
		y = FRAME_SPACE_Y;

		bitmap->DrawRoundRectangle(x, y, x + logo->Width() + 1, y + logo->Height() + 1, GLCD::clrBlack, false, 1);
		bitmap->DrawBitmap(x + 1, y + 1, *logo->GetBitmap(), GLCD::clrBlack);
	}
}

bool cGraphLCDDisplay::IsSymbolsActive() const
{
	return GraphLCDSetup.ShowSymbols;
}

void cGraphLCDDisplay::DisplaySymbols()
{
	int yPos = 0;
	int xPos = 0;
	int i;
	tChannelState channel;
	tVolumeState volume;
	tCardState card[MAXDEVICES];

	channel = GraphLCDState->GetChannelState();
	for (i = 0; i < MAXDEVICES; i++)
		card[i] = GraphLCDState->GetCardState(i);
	volume = GraphLCDState->GetVolumeState();

	if (IsSymbolsActive())
	{
		cChannel * ch = Channels.GetByNumber(channel.number);
		if (ch)
		{
			if (bitmap->Height() <= MAXY_M) // medium display
			{
				yPos = FRAME_SPACE_Y;
				xPos = bitmap->Width() - FRAME_SPACE_X - symbols->TotalWidth();

				if (IsLogoActive())
				{
					xPos -= FRAME_SPACE_XB + logo->Width() + 2;
				}

				if (GraphLCDSetup.ShowSymbols == 1) // normal/fixed symbols
				{
					// new layout:
					//    displays rec symbols for every card and
					//    2chan + dolby have their own symbols
					//    user triggered symbols

					int yPos2 = 0;

					// blank or 2chan or mute
					if (volume.value == 0)
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'M', symbols);
						yPos += symbols->Height('S') + SYMBOL_SPACE;
					}
					else if (ch->Apid2())
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'A', symbols);
						yPos += symbols->Height('A') + SYMBOL_SPACE;
					}
					else
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ' ', symbols);
						yPos += symbols->Height(' ') + SYMBOL_SPACE;
					}

					// blank or dolby
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Dpid1() ? 'D' : ' ', symbols);
					yPos += symbols->Height(ch->Dpid1() ? 'D' : ' ') + SYMBOL_SPACE;

					// blank or teletext
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Tpid() ? 'T' : ' ', symbols);
					yPos += symbols->Height(ch->Tpid() ? 'T' : ' ') + SYMBOL_SPACE;

					// blank or crypt
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Ca() ? 'C' : ' ', symbols);
					yPos += symbols->Height(ch->Tpid() ? 'C' : ' ') + SYMBOL_SPACE;

					// show REC symbols at the right border below the logo
					yPos2 = yPos;
					yPos = FRAME_SPACE_Y;
					if (GraphLCDSetup.ShowDateTime == 1 ||
					    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
					{
						yPos += normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
					}
					if (GraphLCDSetup.ShowChannel)
					{
						yPos += normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_CHANNEL + FRAME_SPACE_YB;
					}
					if (IsLogoActive())
					{
						yPos = std::max((unsigned int) yPos, FRAME_SPACE_Y + logo->Height() + 2 + FRAME_SPACE_YB);
					}

					yPos = std::max(yPos, yPos2);
					xPos = bitmap->Width() - FRAME_SPACE_X + SYMBOL_SPACE;
					for (i = LCDMAXCARDS - 1; i >= 0; i--)
					{
						if (card[i].recordingCount > 0)
						{
							xPos -= symbols->Width(49 + i) + SYMBOL_SPACE;
							bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 49 + i, symbols);
						}
					}

					// show external triggered symbols
					if (GraphLCDSetup.ShowETSymbols && strlen(szETSymbols) > 0)
					{
						for (i = strlen(szETSymbols) - 1; i >= 0; i--)
						{
							xPos -= symbols->Width(szETSymbols[i]) + SYMBOL_SPACE;
							bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, szETSymbols[i], symbols);
						}
					}
				}
				else // compressed symbols
				{
					// old layout:
					//    displays only 1 rec symbol and
					//    a combined 2chan + dolby symbol

					// blank or teletext
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Tpid() ? 'T' : ' ', symbols);
					yPos += symbols->Height(ch->Tpid() ? 'T' : ' ') + SYMBOL_SPACE;

					// blank or crypt
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Ca() ? 'C' : ' ', symbols);
					yPos += symbols->Height(ch->Tpid() ? 'C' : ' ') + SYMBOL_SPACE;

					// blank, 2chan, dolby or combined symbol
					if (volume.value == 0)
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'M', symbols);
						yPos += symbols->Height('S') + SYMBOL_SPACE;
					}
					else if (ch->Apid2() && ch->Dpid1())
					{
						// if Apid2 and Dpid1 are set then use combined symbol
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'B', symbols);
						yPos += symbols->Height('B') + SYMBOL_SPACE;
					}
					else if (ch->Apid2())
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'A', symbols);
						yPos += symbols->Height('A') + SYMBOL_SPACE;
					}
					else if (ch->Dpid1())
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'D', symbols);
						yPos += symbols->Height('D') + SYMBOL_SPACE;
					}
					else
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ' ', symbols);
						yPos += symbols->Height(' ') + SYMBOL_SPACE;
					}

					// blank or rec
					if (cRecordControls::Active())
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, '1', symbols);
					}
					else
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ' ', symbols);
					}
				}
			}
			else // large display
			{
				yPos = FRAME_SPACE_Y;
				if (GraphLCDSetup.ShowDateTime == 1 ||
				    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
				{
					yPos += normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
				}
				if (GraphLCDSetup.ShowChannel)
				{
					yPos += normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_CHANNEL + FRAME_SPACE_YB;
				}
				if (IsLogoActive())
				{
					yPos = std::max((unsigned int) yPos, FRAME_SPACE_Y + logo->Height() + 2 + FRAME_SPACE_YB);
				}

				xPos = bitmap->Width() - FRAME_SPACE_X - symbols->Width(' ');

				if (GraphLCDSetup.ShowSymbols == 1) // normal/fixed symbols
				{
					// blank or teletext
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Tpid() ? 'T' : ' ', symbols);
					xPos -= symbols->Width(ch->Tpid() ? 'T' : ' ') + SYMBOL_SPACE;

					// blank or dolby
					bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Dpid1() ? 'D' : ' ', symbols);
					xPos -= symbols->Width(ch->Dpid1() ? 'D' : ' ') + SYMBOL_SPACE;

					if (bitmap->Height() > MAXY_M) // with 128 pixel width only 3 symbols...
					{
						// blank or crypt
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ch->Ca()? 'C' : ' ', symbols);
						xPos -= symbols->Width(ch->Ca() ? 'C' : ' ') + SYMBOL_SPACE;
					}

					// blank or 2chan or mute
					if (volume.value == 0)
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'M', symbols);
						xPos -= symbols->Width('S') + SYMBOL_SPACE;
					}
					else if (ch->Apid2())
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'A', symbols);
						xPos -= symbols->Width('A') + SYMBOL_SPACE;
					}
					else
					{
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ' ', symbols);
						xPos -= symbols->Width(' ') + SYMBOL_SPACE;
					}
				}
				else // compressed symbols
				{
					// crypt
					if (ch->Ca())
					{
						xPos -= symbols->Width('C') + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'C', symbols);
					}

					// teletext
					if (ch->Tpid())
					{
						xPos -= symbols->Width('T') + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'T', symbols);
					}

					// dolby
					if (ch->Dpid1())
					{
						xPos -= symbols->Width('D') + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'D', symbols);
					}

					// 2chan
					if (ch->Apid2())
					{
						xPos -= symbols->Width('A') + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'A', symbols);
					}

					// mute
					if (volume.value == 0)
					{
						xPos -= symbols->Width('S') + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 'M', symbols);
					}
				}

				// show REC symbols at the right border of the 'next line'
				xPos = bitmap->Width() - FRAME_SPACE_X + SYMBOL_SPACE;
				yPos += symbols->TotalHeight() + FRAME_SPACE_YB;
				for (i = cDevice::NumDevices() - 1; i >= 0; i--)
				{
					// Just display present devices
					xPos -= symbols->Width(49 + i) + SYMBOL_SPACE;
					if (card[i].recordingCount > 0)
					{
						// Show a recording Symbol
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, 49 + i, symbols);
					}
					else
					{
						if (GraphLCDSetup.ShowNotRecording == 1)
						{
							// Do we want an empty frame around not recording card's icons?
							// Show an empty frame instead of the recording symbol
							bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, ' ', symbols);
						}
					}
				}

				// show external triggered symbols
				if (GraphLCDSetup.ShowETSymbols && strlen(szETSymbols) > 0)
				{
					for (i = strlen(szETSymbols) - 1; i >= 0; i--)
					{
						xPos -= symbols->Width(szETSymbols[i]) + SYMBOL_SPACE;
						bitmap->DrawCharacter(xPos, yPos, bitmap->Width() - 1, szETSymbols[i], symbols);
					}
				}
			}
		}
	}
}

void cGraphLCDDisplay::DisplayProgramme()
{
	struct tm tm_r;
	char buffer[25];
	std::string str;
	bool showTimeBar = false;
	int timeBarWidth = 0;
	int timeBarValue = 0;
	tEventState event;

	event = GraphLCDState->GetEventState();
	if (GraphLCDSetup.ShowProgram)
	{
		strftime(buffer, sizeof(buffer), "%R", localtime_r(&event.presentTime, &tm_r));
		if (event.followingTime && event.followingTime != event.presentTime)
		{
			str = buffer;
			if ((bitmap->Width() >= MINY_L || !IsSymbolsActive()))
			{
				str += " - ";
			}
			else
			{
				str += "-";
			}
			strftime(buffer, sizeof(buffer), "%R", localtime_r(&event.followingTime, &tm_r));
			str += buffer;
			showTimeBar = true;
			timeBarWidth = normalFont->Width(str) - 1;
			timeBarValue = (time(NULL) - event.presentTime) * timeBarWidth / (event.followingTime - event.presentTime);
			if (timeBarValue > timeBarWidth)
				timeBarValue = timeBarWidth;
			if (timeBarValue < 0)
				timeBarValue = 0;
		}
		else
		{
			str = buffer;
		}

		if (!event.presentTime)
		{
			std::vector<cScroller>::iterator it;
			for(it = scroller.begin(); it != scroller.end(); it++)
			{
				it->Reset();
			}
		}

		if (event.presentTime)
		{
			if (scroller.size() < 1 ||
			    event.presentTitle != scroller[0].Text() ||
			    (scroller.size() > 1 && event.presentSubtitle != scroller[1].Text()))
			{
				if (bitmap->Height() <= MAXY_S)
				{
					scroller.resize(1);

					int nTopY = bitmap->Height() - (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight();
					int nMaxX = std::max(1, bitmap->Width() - 1 - (2 * FRAME_SPACE_X));
					// Logo enabled & available, and text with Logo is overlapped
					if (IsLogoActive() && nTopY < (int) logo->Height())
					{
						nMaxX -= FRAME_SPACE_XB;
						nMaxX -= logo->Width();
						nMaxX = std::max(nMaxX-2,1);//Frame around Logo
					}
					// If symbols used, and text with symbols is overlapped
					if (IsSymbolsActive())
					{
						nMaxX -= FRAME_SPACE_XB;
						nMaxX -= symbols->TotalWidth();
						nMaxX = std::max(nMaxX,1);
					}

					scroller[0].Init(FRAME_SPACE_X + TEXT_OFFSET_X,
							nTopY,
							nMaxX,
							largeFont, event.presentTitle);
				}
				else
				{
					scroller.resize(2);

					scroller[0].Init(FRAME_SPACE_X + TEXT_OFFSET_X,
							bitmap->Height() - 2 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight(),
							bitmap->Width() - 1,
							largeFont, event.presentTitle);
					scroller[1].Init(FRAME_SPACE_X + TEXT_OFFSET_X,
							bitmap->Height() - (TEXT_OFFSET_Y_TITLE-1) - normalFont->TotalHeight(),
							bitmap->Width() - 1,
							normalFont, event.presentSubtitle);
				}
			}
			if (bitmap->Height() <= MAXY_S)
			{
				// tiny and small LCDs
				bitmap->DrawText(FRAME_SPACE_X,
						bitmap->Height() - 2 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight(),
						bitmap->Width() - 1,
						str, normalFont);
			}
			else
			{
				// medium and large LCDs
				bitmap->DrawText(FRAME_SPACE_X,
						bitmap->Height() - 3 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - 2 * normalFont->TotalHeight() - (showTimeBar && GraphLCDSetup.ShowTimebar ? TIMEBAR_HEIGHT + 1 : 0),
						bitmap->Width() - 1,
						str, normalFont);
				if (showTimeBar && GraphLCDSetup.ShowTimebar)
				{
					bitmap->DrawRectangle(FRAME_SPACE_X,
							bitmap->Height() - 3 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight() - TIMEBAR_HEIGHT - 1,
							FRAME_SPACE_X + timeBarWidth,
							bitmap->Height() - 3 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight() - 2,
							GLCD::clrBlack, false);
					bitmap->DrawRectangle(FRAME_SPACE_X,
							bitmap->Height() - 3 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight() - TIMEBAR_HEIGHT - 1,
							FRAME_SPACE_X + timeBarValue,
							bitmap->Height() - 3 * (TEXT_OFFSET_Y_TITLE - 1) - largeFont->TotalHeight() - normalFont->TotalHeight() - 2,
							GLCD::clrBlack, true);
				}
			}
			// Draw Programmtext
			{
				std::vector<cScroller>::iterator it;
				for (it = scroller.begin(); it != scroller.end(); it++)
				{
					it->Draw(bitmap);
				}
			}
		}
	}
}

bool cGraphLCDDisplay::IndexIsGreaterAsOneHour(int Index) const
{
#if VDRVERSNUM < 10716
	int h = (Index / FRAMESPERSEC) / 3600;
#else
	int h = (Index / DEFAULTFRAMESPERSECOND) / 3600;
#endif
	return h > 0;
}

const char * cGraphLCDDisplay::IndexToMS(int Index) const
{
	static char buffer[16];
#if VDRVERSNUM < 10716
	int s = (Index / FRAMESPERSEC);
#else
	int s = (Index / DEFAULTFRAMESPERSECOND);
#endif
	int m = s / 60;
	s %= 60;
	snprintf(buffer, sizeof(buffer), "%02d:%02d", m, s);
	return buffer;
}

bool cGraphLCDDisplay::IsScrollerTextChanged(const std::vector<cScroller> & scrollers, const std::vector <std::string> & lines) const
{
  if(lines.size() == 0)
    return true; //Different size found
  if(scrollers.size() == 0)
    return true; //Different size found

  std::vector<cScroller>::const_iterator i = scrollers.begin();
  std::vector<cScroller>::const_iterator e = scrollers.end();
  std::vector<std::string>::const_iterator li = lines.begin();
  std::vector<std::string>::const_iterator le = lines.end();

  for (; e != i && le != li; ++li,++i)
  {
    if (i->Text() != (*li))
      return true; //Different text found
  }  
  return false; //Text seem equal
}

void cGraphLCDDisplay::DisplayReplay(tReplayState & replay)
{
    //printf("-------------------cGraphLCDDisplay::DisplayReplay--------------------------------------------\n");
	int nMaxX, nProgressbarHeight, nTopY;
	int nWidthPreMsg = 0, nWidthCurrent = 0, nWidthTotal = 0,nWidthOffset = 0;
	std::string szPreMsg,szCurrent,szTotal;

	if (bitmap->Height() >= MINY_L)
		nProgressbarHeight = 15;
	else if (bitmap->Height() >= MINY_M)
		nProgressbarHeight = 9;
	else if (bitmap->Height() >= MINY_S)
		nProgressbarHeight = 5;
	else
		nProgressbarHeight = 3;

	if (IsLogoActive())
		nTopY = FRAME_SPACE_Y + logo->Height() + 2;
	else if (GraphLCDSetup.ShowDateTime)
		nTopY = FRAME_SPACE_Y + normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
	else
		nTopY = FRAME_SPACE_Y;
	if (replay.name.length() > 0)
	{
		int lineHeight, maxLines;
		std::vector <std::string> lines;

		nMaxX = std::max(1, bitmap->Width() - (2 * FRAME_SPACE_X) - 2 * TEXT_OFFSET_X);
		lineHeight = FRAME_SPACE_Y + largeFont->TotalHeight();
		maxLines = std::max(0, (bitmap->Height() - normalFont->TotalHeight() - FRAME_SPACE_Y - nProgressbarHeight - 2 - nTopY) / lineHeight);

		if (maxLines == 0)
		{
			if (IsLogoActive())
			{
				// draw replayname next to logo
				nMaxX = std::max((unsigned int) 1, nMaxX - FRAME_SPACE_X - logo->Width() - 2);
				if (GraphLCDSetup.ShowDateTime)
					nTopY = FRAME_SPACE_Y + normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
				else
					nTopY = FRAME_SPACE_Y;
				maxLines = (bitmap->Height() - normalFont->TotalHeight() - FRAME_SPACE_Y - nProgressbarHeight - 2 - nTopY) / lineHeight;
			}
			if (maxLines <= 1)
			{
				// use singleline mode
				lines.push_back(replay.name);
			}
			else
				WrapText(replay.name, lines, largeFont, nMaxX, maxLines, false);
		}
		else if (maxLines == 1) //singleline mode
			lines.push_back(replay.name);
		else
		{
			WrapText(replay.name, lines, largeFont, nMaxX, maxLines, false);
		}

		if (scroller.size() != lines.size() ||
		    IsScrollerTextChanged(scroller,lines)) // if any text is changed
		{
			// Same size for Scroller and Textbuffer
			scroller.resize(lines.size());

			std::vector<cScroller>::iterator i = scroller.begin();
			std::vector<cScroller>::const_iterator e = scroller.end();
			std::vector<std::string>::const_iterator li = lines.begin();
			std::vector<std::string>::const_iterator le = lines.end();

			for (int n = lines.size(); e != i && le != li; ++li,++i,--n)
			{
				nTopY = bitmap->Height() - normalFont->TotalHeight() - FRAME_SPACE_Y - nProgressbarHeight - n * lineHeight - 2;
				i->Init(FRAME_SPACE_X + TEXT_OFFSET_X, nTopY, nMaxX + FRAME_SPACE_X + TEXT_OFFSET_X, largeFont, *li);
			}
		}
	}

    // Draw Replaytext
	{
		std::vector<cScroller>::iterator it;
		for(it = scroller.begin(); it != scroller.end(); it++)
		{
			it->Draw(bitmap);
		}
	}

	// Draw Progressbar with current and total replay time
	nTopY = bitmap->Height() - normalFont->TotalHeight() - FRAME_SPACE_Y - nProgressbarHeight - 2;
	nMaxX = std::max(1, bitmap->Width() - 1 - 2 * FRAME_SPACE_X);
	// Logo enabled & available, and text with Logo is overlapped
	if (IsLogoActive() && nTopY < (int) logo->Height())
	{
		nMaxX -= max(1,FRAME_SPACE_X); // Free line between Logo and progressbar
		nMaxX -= logo->Width();
		nMaxX = std::max(nMaxX - 2, 1); //Frame around Logo
	}

	bitmap->DrawRectangle(FRAME_SPACE_X,
			nTopY,
			FRAME_SPACE_X + nMaxX,
			nTopY + nProgressbarHeight,
			GLCD::clrBlack, false);

	if (1 < replay.total && 1 < replay.current) // Don't show full progressbar for endless streams 
	{
		bitmap->DrawRectangle(FRAME_SPACE_X,
				nTopY,
				FRAME_SPACE_X + (std::min(replay.total, replay.current) * nMaxX / replay.total),
				nTopY + nProgressbarHeight,
				GLCD::clrBlack, true);
	}

	// Draw Strings with current and total replay time
	nTopY = bitmap->Height() - normalFont->TotalHeight() - FRAME_SPACE_Y;
	// use same width like Progressbar
//	if (!IsLogoActive() || nTopY > logo->Height()) 
//	nMaxX = max(1, bitmap->Width() - 1 - (2 * FRAME_SPACE_X));

	switch (replay.mode)
	{
		case eReplayDVD:     szPreMsg = tr("DVD");    break;
		case eReplayMusic:   szPreMsg = tr("Music");  break;
		case eReplayFile:    szPreMsg = tr("File");   break;
		case eReplayImage:   szPreMsg = tr("Image");  break;
		case eReplayAudioCD: szPreMsg = tr("CD");     break;
		default:             szPreMsg = tr("Replay"); break;
	}

	if (bitmap->Width() >= MINY_M)
	{
		szPreMsg += " : ";
		szPreMsg += replay.loopmode;
	}
	else
		szPreMsg += ":";

	if (replay.mode == eReplayImage) // Image-Plugin hasn't Frames per Seconds
	{
		char buffer[8];
		snprintf(buffer, sizeof(buffer), "%d", replay.current); 
		szCurrent = buffer;
		snprintf(buffer, sizeof(buffer), "%d", replay.total); 
		szTotal = buffer;
	}
	else
	{
		if ((replay.total > 1 && IndexIsGreaterAsOneHour(replay.total)) ||
		    IndexIsGreaterAsOneHour(replay.current)) // Check if any index bigger as one hour
		{
			szCurrent = (const char *) IndexToHMSF(replay.current);
			if (replay.total > 1) // Don't draw totaltime for endless streams  
				szTotal = (const char *) IndexToHMSF(replay.total);
		}
		else
		{
			// Show only minutes and seconds on short replays
			szCurrent = (const char *) IndexToMS(replay.current);
			if (replay.total > 1) // Don't draw totaltime for endless streams  
				szTotal = (const char *) IndexToMS(replay.total);
		}
	}
	// Get width of drawable strings
	nWidthPreMsg = normalFont->Width(szPreMsg);
	nWidthCurrent = normalFont->Width(szCurrent);
	if (szTotal.length()) // Don't draw empty string 
		nWidthTotal = normalFont->Width(szTotal);

	// Draw depends on display width, any placeable informations
	if (nWidthTotal && nWidthPreMsg && (nWidthPreMsg + nWidthCurrent + nWidthTotal + 5 < nMaxX))
	{
		// Show prefix and all position
		nWidthOffset = bitmap->DrawText(FRAME_SPACE_X, nTopY, nMaxX, szPreMsg, normalFont);
		bitmap->DrawText(FRAME_SPACE_X + nWidthOffset + 1, nTopY, nMaxX, szCurrent, normalFont);
		bitmap->DrawText(nMaxX - nWidthTotal, nTopY, nMaxX, szTotal, normalFont);
	}
	else if (nWidthTotal && (nWidthCurrent + nWidthTotal + 5 < nMaxX))
	{
		// Show current and total position
		bitmap->DrawText(FRAME_SPACE_X, nTopY, nMaxX, szCurrent, normalFont);
		bitmap->DrawText(nMaxX - nWidthTotal, nTopY, nMaxX, szTotal, normalFont);
	}
	else if (!nWidthTotal && nWidthPreMsg && (nWidthPreMsg + nWidthCurrent + 1 < nMaxX))
	{
		// Show prefix and current position
		nWidthOffset = bitmap->DrawText(FRAME_SPACE_X, nTopY, nMaxX, szPreMsg, normalFont);
		bitmap->DrawText(FRAME_SPACE_X + nWidthOffset + 1, nTopY, nMaxX, szCurrent, normalFont);
	}        
	else
	{
		// Show only current position
		bitmap->DrawText(FRAME_SPACE_X, nTopY, nMaxX, szCurrent, normalFont);
	}
}

void cGraphLCDDisplay::DisplayMenu(void)
{
    //printf("-------------------cGraphLCDDisplay::DisplayMenu--------------------------------------------\n");
	char buffer2[255];
	char * pszTmp1;
	char * pszTmp2;
	int iAT, t;
	int FrameWidth, yPos, iEntryHeight;
	int extra = 0;
	tOsdState osd;

	osd = GraphLCDState->GetOsdState();

	mutex.Lock();

	FrameWidth = std::max(bitmap->Width() - 2 * FRAME_SPACE_X, 1);

	if (GraphLCDSetup.ShowDateTime == 1 ||
	    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
	{
		yPos = FRAME_SPACE_Y + normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
	}
	else
	{
		yPos = FRAME_SPACE_Y;
	}

	// draw Menu Title
	if (osd.title.length() > 0)
	{
		bitmap->DrawRoundRectangle(FRAME_SPACE_X,
				yPos,
				FRAME_SPACE_X + FrameWidth - 1,
				yPos + normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_TIME - 1,
				GLCD::clrBlack, true, TEXT_OFFSET_Y_CHANNEL >= 4 ? 4 : 1);
		bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X,
				yPos + TEXT_OFFSET_Y_TIME,
				FRAME_SPACE_X + FrameWidth - 1,
				osd.title, normalFont, GLCD::clrWhite);
	}

	if (!(textItemLines.size() > 0))
	{
		// draw Menu Entries
		if (normalFont->TotalHeight() == normalFont->LineHeight())
			extra = 1;
		iEntryHeight = normalFont->TotalHeight() + 2 * extra;
		yPos = yPos + normalFont->TotalHeight() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
/*		if (GraphLCDSetup.ShowColorButtons &&
		    (osd.colorButton[0].length() > 0 || osd.colorButton[1].length() > 0 ||
		     osd.colorButton[2].length() > 0 || osd.colorButton[3].length() > 0))
		{
			menuCount = (bitmap->Height() - yPos - smallFont->TotalHeight() - 4 - FRAME_SPACE_Y / 3) / iEntryHeight;
		}
		else
*/		
		{
			menuCount = (bitmap->Height() - yPos) / iEntryHeight;
		}

		if (osd.currentItemIndex < menuTop)
			menuTop = osd.currentItemIndex;
		if (osd.currentItemIndex > menuTop + menuCount - 1)
			menuTop = std::max(0, osd.currentItemIndex + 1 - menuCount);

		bitmap->DrawRectangle(0, yPos, bitmap->Width() - 1, bitmap->Height() - 1, GLCD::clrWhite, true);

		for (int i = menuTop; i < std::min((int) osd.items.size(), menuTop + menuCount); i++)
		{
			if (i == osd.currentItemIndex)
			{
				bitmap->DrawRoundRectangle(FRAME_SPACE_X, yPos + (i - menuTop) * iEntryHeight,
						bitmap->Width() - 1 - FRAME_SPACE_X,
						yPos + (i - menuTop + 1) * iEntryHeight - 1,
						GLCD::clrBlack, true, TEXT_OFFSET_Y_CHANNEL >= 4 ? 3 : 1);
			}
			strncopy(buffer2, osd.items[i].c_str(), sizeof(buffer2));
			pszTmp1 = buffer2;
			pszTmp2 = strchr(pszTmp1, '\t');
			iAT = 0; t = 0;

			while (pszTmp1 && pszTmp2)
			{
				*pszTmp2 = '\0';
				bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X + t,
						yPos + (i - menuTop) * iEntryHeight + extra,
						std::min(FRAME_SPACE_X + TEXT_OFFSET_X + t + tab[iAT + 1], bitmap->Width() - 1 - FRAME_SPACE_X),
						pszTmp1, normalFont, (i == osd.currentItemIndex) ? GLCD::clrWhite : GLCD::clrBlack);
				pszTmp1 = pszTmp2+1;
				pszTmp2 = strchr(pszTmp1, '\t');
				t = t + tab[iAT + 1] + TEXT_OFFSET_X;
				iAT++;
			}

			bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X + t,
					yPos + (i - menuTop) * iEntryHeight + extra,
					bitmap->Width() - 1 - FRAME_SPACE_X,
					pszTmp1, normalFont, (i == osd.currentItemIndex) ? GLCD::clrWhite : GLCD::clrBlack);
		}
	}
	mutex.Unlock();
}

void cGraphLCDDisplay::DisplayMessage()
{
    //printf("-------------------cGraphLCDDisplay::DisplayMessage()--------------------------------------------\n");
	std::vector <std::string> lines;
	int lineCount;
	int maxTextLen, recW, recH;
	int entryHeight;
	tOsdState osd;

	osd = GraphLCDState->GetOsdState();
	if (GraphLCDSetup.ShowMessages && osd.message.length() > 0)
	{
		maxTextLen = bitmap->Width() - 2 * FRAME_SPACE_X - 2 * FRAME_SPACE_XB - 2 * TEXT_OFFSET_X - 10;
		recW = WrapText(osd.message, lines, normalFont, maxTextLen, MAXLINES_MSG);
		lineCount = lines.size();

		// display text
		entryHeight = 2 * (normalFont->TotalHeight() - normalFont->TotalAscent()) + normalFont->TotalAscent();
		recH = lineCount * entryHeight + 2 * TEXT_OFFSET_Y_CHANNEL + 2 * FRAME_SPACE_YB;
		recW = recW + 2 * TEXT_OFFSET_X + 2 * FRAME_SPACE_XB + 2 * FRAME_SPACE_X;
		recW += (recW % 2);

		bitmap->DrawRectangle((bitmap->Width() - recW) / 2,
				(bitmap->Height() - recH) / 2,
				bitmap->Width() - 1 - (bitmap->Width() - recW) / 2,
				bitmap->Height() - 1 - (bitmap->Height() - recH) / 2,
				GLCD::clrWhite, true);
		recH = recH - 2 * FRAME_SPACE_YB;
		recW = recW - 2 * FRAME_SPACE_XB;
		bitmap->DrawRectangle((bitmap->Width() - recW) / 2,
				(bitmap->Height() - recH) / 2,
				bitmap->Width() - 1 - (bitmap->Width() - recW) / 2,
				bitmap->Height() - 1 - (bitmap->Height() - recH) / 2,
				GLCD::clrBlack, false);
		recH = recH - 2 * TEXT_OFFSET_Y_CHANNEL;
		recW = recW - 2 * TEXT_OFFSET_X;
		for (int i = 0; i < lineCount; i++)
		{
			bitmap->DrawText((bitmap->Width() - normalFont->Width(lines[i])) / 2,
					(bitmap->Height() - recH) / 2 + i * entryHeight + (normalFont->TotalHeight() - normalFont->TotalAscent()),
					bitmap->Width() - (bitmap->Width() - recW) / 2,
					lines[i], normalFont);
		}
	}
}

void cGraphLCDDisplay::DisplayTextItem()
{ 
    //printf("-------------------cGraphLCDDisplay::DisplayTextItem()--------------------------------------------\n");
	int lineCount;
	int iEntryHeight, iLineAnz;
	int yPos;
	tOsdState osd;

	osd = GraphLCDState->GetOsdState();

	mutex.Lock();
	if (textItemLines.size() > 0)
	{
        //printf("-------------------cGraphLCDDisplay::DisplayTextItem(), if (textItemLines.size() > 0)-------------\n");
		lineCount = textItemLines.size();

		if (GraphLCDSetup.ShowDateTime == 1 ||
		    (GraphLCDSetup.ShowDateTime == 2 && State != Menu))
		{
			yPos = FRAME_SPACE_Y + normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_TIME + FRAME_SPACE_YB;
		}
		else
		{
			yPos = FRAME_SPACE_Y;
		}

		// draw Text
		iEntryHeight = 2 * (normalFont->TotalHeight() - normalFont->TotalAscent()) + normalFont->TotalAscent();
		yPos = yPos + normalFont->TotalAscent() + 2 * TEXT_OFFSET_Y_CHANNEL + FRAME_SPACE_YB;
/*	
		if (GraphLCDSetup.ShowColorButtons &&
		    (osd.colorButton[0].length() > 0 || osd.colorButton[1].length() > 0 ||
		     osd.colorButton[2].length() > 0 || osd.colorButton[3].length() > 0))
		{
			iLineAnz = (bitmap->Height() - yPos - smallFont->TotalHeight() - 4 - FRAME_SPACE_Y / 3) / iEntryHeight;
		}
		else
*/	
		{
			iLineAnz = (bitmap->Height() - yPos) / iEntryHeight;
		}

		int startLine = textItemTop;
		for (int i = 0; i < std::min(lineCount, iLineAnz); i++)
		{ 
		// printf("-----------cGraphLCDDisplay::DisplayTextItem(), textItemLines[i + startLine] = %s---------\n",  textItemLines[i + startLine].c_str());
			if (i + startLine < lineCount)
				bitmap->DrawText(FRAME_SPACE_X + TEXT_OFFSET_X,
						yPos + i * iEntryHeight + (normalFont->TotalHeight() - normalFont->TotalAscent()),
						bitmap->Width() - 1 - FRAME_SPACE_X,
						textItemLines[i + startLine], normalFont);
		}
	}
	mutex.Unlock();
}

void cGraphLCDDisplay::DisplayColorButtons()
{
    //printf("------------------- cGraphLCDDisplay::DisplayColorButtons()--------------------------------------------\n");
	int i, buttonWidth, textLen;
	int extra = 0;
	tOsdState osd;

	osd = GraphLCDState->GetOsdState();

	if (GraphLCDSetup.ShowColorButtons)
	{
		buttonWidth = (bitmap->Width() / 4) - (FRAME_SPACE_X ? 2 * FRAME_SPACE_X : 1);
		if (smallFont->TotalHeight() == smallFont->TotalAscent())
			extra = 1;

		for (i = 0; i < 4; i++)
		{
			if (osd.colorButton[i].length() > 0)
			{
				bitmap->DrawRoundRectangle(i * (bitmap->Width() / 4) + FRAME_SPACE_X,
						bitmap->Height() - smallFont->TotalHeight() - 2 * extra - FRAME_SPACE_Y / 3,
						i * (bitmap->Width() / 4) + FRAME_SPACE_X + buttonWidth - 1,
						bitmap->Height() - 1 - FRAME_SPACE_Y / 3,
						GLCD::clrBlack, true, std::max(1, (smallFont->TotalHeight() + 4) / 5));
				textLen = smallFont->Width(osd.colorButton[i]);
				if (textLen <= buttonWidth - 2)
				{
					bitmap->DrawText(i * (bitmap->Width() / 4) + (bitmap->Width() / 8) - (textLen + 1) / 2,
							bitmap->Height() - smallFont->TotalHeight() - extra - FRAME_SPACE_Y / 3,
							i * (bitmap->Width() / 4) + FRAME_SPACE_X + buttonWidth - 1,
							osd.colorButton[i], smallFont, GLCD::clrWhite);
				}
				else
				{
					bitmap->DrawText(i * (bitmap->Width() / 4) + FRAME_SPACE_X + 1,
							bitmap->Height() - smallFont->TotalHeight() - extra - FRAME_SPACE_Y / 3,
							i * (bitmap->Width() / 4) + FRAME_SPACE_X + buttonWidth - 1,
							osd.colorButton[i], smallFont, GLCD::clrWhite);
				}
			}
		}
	}
}

void cGraphLCDDisplay::DisplayVolume()
{
	int RecW, RecH;
	tVolumeState volume;

	volume = GraphLCDState->GetVolumeState();

	if (GraphLCDSetup.ShowVolume)
	{
		if (volume.lastChange > 0)
		{
			if (TimeMs() - volume.lastChange < 2000)
			{
				RecH = (bitmap->Height() / 5) + 2 * FRAME_SPACE_YB + 4 * FRAME_SPACE_YB;
				RecW = bitmap->Width() / 2;
				bitmap->DrawRoundRectangle((bitmap->Width() - RecW) / 2, // draw frame
						(bitmap->Height() - RecH) / 2,
						bitmap->Width() - (bitmap->Width() - RecW) / 2 - 1,
						bitmap->Height() - (bitmap->Height() - RecH) / 2 - 1,
						GLCD::clrWhite, true, 1);
				RecH = RecH - 2 * FRAME_SPACE_YB;
				RecW = RecW - 2 * FRAME_SPACE_XB;
				bitmap->DrawRoundRectangle((bitmap->Width() - RecW) / 2, // draw box
						(bitmap->Height() - RecH) / 2,
						bitmap->Width() - 1 - (bitmap->Width() - RecW) / 2,
						bitmap->Height() - 1 - (bitmap->Height() - RecH) / 2,
						GLCD::clrBlack, false, 1);
				RecH = RecH - 2;
				RecW = RecW - 2;
				if (volume.value > 0)
					bitmap->DrawRectangle((bitmap->Width() - RecW) / 2, // draw bar
							(bitmap->Height() - RecH) / 2,
							(bitmap->Width() - RecW) / 2 + (volume.value * RecW) / 255,
							bitmap->Height() - 1 - (bitmap->Height() - RecH) / 2,
							GLCD::clrBlack, true);
				if (volume.value == 0)
				{
					// display big mute symbol
					bitmap->DrawCharacter(bitmap->Width() / 2 - symbols->Width('5'),
							bitmap->Height() / 2 - symbols->Height('5'),
							bitmap->Width() - 1, '5', symbols);
					bitmap->DrawCharacter(bitmap->Width() / 2,
							bitmap->Height() / 2 - symbols->Height('6'),
							bitmap->Width() - 1, '6', symbols);
					bitmap->DrawCharacter(bitmap->Width() / 2 - symbols->Width('7'),
							bitmap->Height() / 2,
							bitmap->Width() - 1, '7', symbols);
					bitmap->DrawCharacter(bitmap->Width() / 2,
							bitmap->Height() / 2,
							bitmap->Width() - 1, '8', symbols);
				}
				showVolume = true;
			}
		}
	}
}

void cGraphLCDDisplay::UpdateIn(long usec)
{
  if (usec == 0)
  {
    timerclear(&UpdateAt);
  }
  else
  {
    if (gettimeofday(&CurrTimeval, NULL) == 0)
    {
      // get current time
      UpdateAt.tv_sec = CurrTimeval.tv_sec;
      UpdateAt.tv_usec = CurrTimeval.tv_usec + usec;
      while (UpdateAt.tv_usec >= 1000000)
      {
        // take care of an overflow
        UpdateAt.tv_sec++;
        UpdateAt.tv_usec -= 1000000;
      }
    }
  }
}


bool cGraphLCDDisplay::CheckAndUpdateSymbols()
{
  bool                bRet = false;
  static struct stat  filestat;
  FILE*               InFile = NULL;
  static char         szLine[8];

  if(stat(FILENAME_EXTERNAL_TRIGGERED_SYMBOLS, &filestat)==0) {
    if(LastTimeModSym != filestat.st_mtime) {
      InFile = fopen(FILENAME_EXTERNAL_TRIGGERED_SYMBOLS, "r");
      if(InFile) {
        strcpy(szETSymbols, "");
        while(!feof(InFile) && (strlen(szETSymbols)+1<sizeof(szLine))) {
          strcpy(szLine, "");
          fgets(szLine, sizeof(szLine), InFile);
          compactspace(szLine);
          if((strlen(szLine)==2) && (szLine[1]=='1')) {
            strcat(szETSymbols, ".");
            szETSymbols[strlen(szETSymbols)-1] = szLine[0];
          }
        }
        fclose(InFile);
        LastTimeModSym = filestat.st_mtime;
        bRet = true;
      }
    }
  } else {
    if((errno == ENOENT) && (strlen(szETSymbols)>0)) {
      strcpy(szETSymbols, "");
      bRet = true;
    }
  }
  return bRet;
}

int cGraphLCDDisplay::WrapText(std::string & text, std::vector <std::string> & lines, const GLCD::cFont * font, int maxTextWidth, int maxLines, bool cutTooLong)
{
  //printf("------------------- cGraphLCDDisplay::WrapText--------------------------------------------\n");
  int lineCount;
  int textWidth;
  std::string::size_type start;
  std::string::size_type pos;
  std::string::size_type posLast;

  lines.clear();
  lineCount = 0;

  pos = 0;
  start = 0;
  posLast = 0;
  textWidth = 0;
  while (pos < text.length() && lineCount < maxLines)
  {
    if (text[pos] == '\n')
    {
      lines.push_back(trim(text.substr(start, pos - start)));
      start = pos + 1;
      posLast = pos + 1;
      textWidth = 0;
      lineCount++;
    }
    else if (textWidth > maxTextWidth)
    {
      if (posLast > start)
      {
        lines.push_back(trim(text.substr(start, posLast - start)));
        start = posLast + 1;
        posLast = start;
        textWidth = font->Width(text.substr(start, pos - start + 1));
      }
      else
      {
        lines.push_back(trim(text.substr(start, pos - start)));
        start = pos + 1;
        posLast = start;
        textWidth = font->Width(text[pos]);
      }
      lineCount++;
    }
    else if (text[pos] == ' ')
    {
      posLast = pos;
      textWidth += font->Width(text[pos]);
    }
    else
    {
      textWidth += font->Width(text[pos]);
    }
    pos++;
  }

  if (lineCount < maxLines)
  {
    if (pos > start)
    {
      lines.push_back(trim(text.substr(start)));
      lineCount++;
    }
  }
  else if (cutTooLong)
  {
    pos = lines[lineCount - 1].length();
    while (font->Width(lines[lineCount - 1], pos) > maxTextWidth)
    {
      pos = lines[lineCount - 1].rfind(' ', pos - 1);
      if (pos == std::string::npos)
        break;
    }
    if (pos != std::string::npos)
      lines[lineCount - 1].resize(pos);
  }
  else
    return maxTextWidth;

  textWidth = 0;
  for (int i = 0; i < lineCount; i++)
    textWidth = std::max(textWidth, font->Width(lines[i]));
  textWidth = std::min(textWidth, maxTextWidth);

  return textWidth;
}

void cGraphLCDDisplay::SetBrightness() 
{
	mutex.Lock();
	bool bActive = bBrightnessActive
		|| (State == Menu)
		|| (GraphLCDSetup.ShowVolume && showVolume)
		|| (GraphLCDSetup.ShowMessages && GraphLCDState->GetOsdState().message.length() > 0)
		|| (GraphLCDSetup.BrightnessDelay == 900);
	if (bActive)
	{
		LastTimeBrightness = TimeMs();
		bBrightnessActive = false;
	}
	if ((bActive ? GraphLCDSetup.BrightnessActive : GraphLCDSetup.BrightnessIdle) != nCurrentBrightness)
    {
		if (bActive)
		{
			LCD->SetBrightness(GraphLCDSetup.BrightnessActive);
			nCurrentBrightness = GraphLCDSetup.BrightnessActive;
			if(GraphLCDSetup.ShowIdleClock) update = true;
		}
		else
		{
#if VDRVERSNUM < 10716
			if (GraphLCDSetup.BrightnessDelay < 1
				|| ((TimeMs() - LastTimeBrightness) > (uint64) (GraphLCDSetup.BrightnessDelay*1000)))
#else
			if (GraphLCDSetup.BrightnessDelay < 1
				|| ((TimeMs() - LastTimeBrightness) > (uint64_t) (GraphLCDSetup.BrightnessDelay*1000)))
#endif
			{
				LCD->SetBrightness(GraphLCDSetup.BrightnessIdle);
				nCurrentBrightness = GraphLCDSetup.BrightnessIdle;
				if(GraphLCDSetup.ShowIdleClock) update = true;
			}
		}

	}
	mutex.Unlock();
}

cGraphLCDDisplay Display;
