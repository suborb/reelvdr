/*
 * vdrarchiv_player.c:
 *
 * This is a copy of vdr's cReplayControl removing cRecording stuff which is
 * not compatible with vdrarchiv filehandling.
 * Cutting is disabled.
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/remote.h>
#include <vdr/status.h>

#include "media_vdrarchiv/vdrarchiv_player.h"
#include "mediahandler.h"

//#define DEBUG_I 1
//#define DEBUG_D 1
#include "mediadebug.h"

#define MODETIMEOUT       3 // seconds

char *cMediaVDRArchivControl::fileName = NULL;
char *cMediaVDRArchivControl::title = NULL;

cMediaVDRArchivControl::cMediaVDRArchivControl(cMediaVDRArchivDisc *Disc)
:cDvbPlayerControl(fileName)
{
	disc = Disc;
	kback_key = false;
	displayReplay = NULL;
	visible = modeOnly = shown = displayFrames = false;
	lastCurrent = lastTotal = -1;
	lastPlay = lastForward = false;
	lastSpeed = -1;
	timeoutShow = 0;
	timeSearchActive = false;
	marks.Load(fileName);
	cMediaHandler::SetHandlerReplays(true);
	cStatus::MsgReplaying(this, title, fileName, true);
}

cMediaVDRArchivControl::~cMediaVDRArchivControl()
{
	Hide();
	cStatus::MsgReplaying(this, NULL, fileName, false);
	Stop();
	cMediaHandler::SetHandlerReplays(false);
	if(kback_key)
		cRemote::CallPlugin("mediamanager");
}

void cMediaVDRArchivControl::SetRecording(const char *FileName, const char *Title)
{
	free(fileName);
	free(title);
	fileName = FileName ? strdup(FileName) : NULL;
	title = Title ? strdup(Title) : NULL;
}

void cMediaVDRArchivControl::ShowTimed(int Seconds)
{
	if (modeOnly)
		Hide();
	if (!visible) {
		shown = ShowProgress(true);
		timeoutShow = (shown && Seconds > 0) ? time(NULL) + Seconds : 0;
	}
}

void cMediaVDRArchivControl::Show(void)
{
	ShowTimed();
}

void cMediaVDRArchivControl::Hide(void)
{
	if (visible) {
		delete displayReplay;
		displayReplay = NULL;
		visible = false;
		SetNeedsFastResponse(visible);
		modeOnly = false;
		lastPlay = lastForward = false;
		lastSpeed = -1;
		timeSearchActive = false;
	}
}

void cMediaVDRArchivControl::ShowMode(void)
{
	if (visible || Setup.ShowReplayMode && !cOsd::IsOpen()) {
		bool Play, Forward;
		int Speed;
		if (GetReplayMode(Play, Forward, Speed) &&
				(!visible || Play != lastPlay || Forward != lastForward || Speed != lastSpeed)) {
			bool NormalPlay = (Play && Speed == -1);
			if (!visible) {
				if (NormalPlay)
				return; // no need to do indicate ">" unless there was a different mode displayed before
				visible = modeOnly = true;
				displayReplay = Skins.Current()->DisplayReplay(modeOnly);
			}
			if (modeOnly && !timeoutShow && NormalPlay)
				timeoutShow = time(NULL) + MODETIMEOUT;
			displayReplay->SetMode(Play, Forward, Speed);
			lastPlay = Play;
			lastForward = Forward;
			lastSpeed = Speed;
		}
	}
}

bool cMediaVDRArchivControl::ShowProgress(bool Initial)
{
	int Current, Total;

	if (GetIndex(Current, Total) && Total > 0) {
		if (!visible) {
			displayReplay = Skins.Current()->DisplayReplay(modeOnly);
			displayReplay->SetMarks(&marks);
			visible = true;
			SetNeedsFastResponse(visible);
		}
		if (Initial) {
			if (title)
				displayReplay->SetTitle(title);
			lastCurrent = lastTotal = -1;
		}
		if (Total != lastTotal) {
			displayReplay->SetTotal(IndexToHMSF(Total));
			if (!Initial)
				displayReplay->Flush();
		}
		if (Current != lastCurrent || Total != lastTotal) {
			displayReplay->SetProgress(Current, Total);
			if (!Initial)
				displayReplay->Flush();
			displayReplay->SetCurrent(IndexToHMSF(Current, displayFrames));
			displayReplay->Flush();
			lastCurrent = Current;
		}
		lastTotal = Total;
		ShowMode();
		return true;
	}
	return false;
}

void cMediaVDRArchivControl::TimeSearchDisplay(void)
{
	char buf[64];
	strcpy(buf, tr("Jump: "));
	int len = strlen(buf);
	char h10  = '0' + (timeSearchTime >> 24);
	char h1   = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
	char m10  = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
	char m1   = '0' + (timeSearchTime & 0x000000FF);
	char ch10 = timeSearchPos > 3 ? h10 : '-';
	char ch1  = timeSearchPos > 2 ? h1  : '-';
	char cm10 = timeSearchPos > 1 ? m10 : '-';
	char cm1  = timeSearchPos > 0 ? m1  : '-';
	sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
	displayReplay->SetJump(buf);
}

void cMediaVDRArchivControl::TimeSearchProcess(eKeys Key)
{
	#define STAY_SECONDS_OFF_END 10
	int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
	int Current = (lastCurrent / FRAMESPERSEC);
	int Total = (lastTotal / FRAMESPERSEC);
	switch (Key) {
		case k0 ... k9:
			if (timeSearchPos < 4) {
				timeSearchTime <<= 8;
				timeSearchTime |= Key - k0;
				timeSearchPos++;
				TimeSearchDisplay();
			}
			break;
		case kFastRew:
		case kLeft:
		case kFastFwd:
		case kRight:
			{
				int dir = ((Key == kRight || Key == kFastFwd) ? 1 : -1);
				if (dir > 0)
					Seconds = min(Total - Current - STAY_SECONDS_OFF_END, Seconds);
				SkipSeconds(Seconds * dir);
				timeSearchActive = false;
			}
			break;
		case kPlay:
		case kUp:
		case kPause:
		case kDown:
			Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
			Goto(Seconds * FRAMESPERSEC, Key == kDown || Key == kPause);
			timeSearchActive = false;
			break;
		default:
			timeSearchActive = false;
			break;
	}

	if (!timeSearchActive) {
		if (timeSearchHide)
			Hide();
		else
			displayReplay->SetJump(NULL);
		ShowMode();
	}
}

void cMediaVDRArchivControl::TimeSearch(void)
{
	timeSearchTime = timeSearchPos = 0;
	timeSearchHide = false;
	if (modeOnly)
		Hide();
	if (!visible) {
		Show();
		if (visible)
			timeSearchHide = true;
		else
			return;
	}
	timeoutShow = 0;
	TimeSearchDisplay();
	timeSearchActive = true;
}

void cMediaVDRArchivControl::MarkToggle(void)
{
	int Current, Total;
	if (GetIndex(Current, Total, true)) {
		cMark *m = marks.Get(Current);
		lastCurrent = -1; // triggers redisplay
		if (m)
			marks.Del(m);
		else {
			marks.Add(Current);
			ShowTimed(2);
			bool Play, Forward;
			int Speed;
			if (GetReplayMode(Play, Forward, Speed) && !Play)
				Goto(Current, true);
		}
		marks.Save();
	}
}

void cMediaVDRArchivControl::MarkJump(bool Forward)
{
	if (marks.Count()) {
		int Current, Total;
		if (GetIndex(Current, Total)) {
			cMark *m = Forward ? marks.GetNext(Current) : marks.GetPrev(Current);
			if (m) {
				Goto(m->position, true);
				displayFrames = true;
			}
		}
	}
}

void cMediaVDRArchivControl::MarkMove(bool Forward)
{
	int Current, Total;
	if (GetIndex(Current, Total)) {
		cMark *m = marks.Get(Current);
		if (m) {
			displayFrames = true;
			int p = SkipFrames(Forward ? 1 : -1);
			cMark *m2;
			if (Forward) {
				if ((m2 = marks.Next(m)) != NULL && m2->position <= p)
					return;
			} else {
				if ((m2 = marks.Prev(m)) != NULL && m2->position >= p)
					return;
			}
			Goto(m->position = p, true);
			marks.Save();
		}
	}
}
/*
void cMediaVDRArchivControl::EditCut(void)
{
	if (fileName) {
		Hide();
		if (!cCutter::Active()) {
			if (!marks.Count())
				Skins.Message(mtError, tr("No editing marks defined!"));
			else if (!cCutter::Start(fileName))
				Skins.Message(mtError, tr("Can't start editing process!"));
			else
				Skins.Message(mtInfo, tr("Editing process started"));
		} else
			Skins.Message(mtError, tr("Editing process already active!"));
		ShowMode();
	}
}

void cMediaVDRArchivControl::EditTest(void)
{
	int Current, Total;
	if (GetIndex(Current, Total)) {
		cMark *m = marks.Get(Current);
		if (!m)
			m = marks.GetNext(Current);
		if (m) {
			if ((m->Index() & 0x01) != 0)
				m = marks.Next(m);
			if (m) {
				Goto(m->position - SecondsToFrames(3));
				Play();
			}
		}
	}
}
*/
cOsdObject *cMediaVDRArchivControl::GetInfo(void)
{
	return NULL;
}

eOSState cMediaVDRArchivControl::ProcessKey(eKeys Key)
{
	if (!Active())
		return osEnd;

	if (visible) {
		if (timeoutShow && time(NULL) > timeoutShow) {
			Hide();
			ShowMode();
			timeoutShow = 0;
		} else if (modeOnly)
			ShowMode();
		else
			shown = ShowProgress(!shown) || shown;
	}

	bool DisplayedFrames = displayFrames;
	displayFrames = false;
	if (timeSearchActive && Key != kNone) {
		TimeSearchProcess(Key);
		return osContinue;
	}

	bool DoShowMode = true;
	switch (Key) {
		// Positioning:
		case kPlay:
		case kUp:
			Play();
			break;
		case kPause:
		case kDown:
			Pause();
			break;
		case kFastRew|k_Release:
		case kLeft|k_Release:
			if (Setup.MultiSpeedMode)
				break;
		case kFastRew:
		case kLeft:
			Backward();
			break;
		case kFastFwd|k_Release:
		case kRight|k_Release:
			if (Setup.MultiSpeedMode)
				break;
		case kFastFwd:
		case kRight:
			Forward();
			break;
		case kRed:
			TimeSearch();
			break;
		case kGreen|k_Repeat:
		case kGreen:
			SkipSeconds(-60);
			break;
		case kYellow|k_Repeat:
		case kYellow:
			SkipSeconds( 60);
			break;
		case kStop:
		case kBlue:
			Hide();
			Stop();
			return osEnd;
		default:
			{
				DoShowMode = false;
				switch (Key) {
					// Editing:
					case kMarkToggle:
						MarkToggle();
						break;
					case kMarkJumpBack|k_Repeat:
					case kMarkJumpBack:MarkJump(false);
						break;
					case kMarkJumpForward|k_Repeat:
					case kMarkJumpForward:
						MarkJump(true);
						break;
					case kMarkMoveBack|k_Repeat:
					case kMarkMoveBack:
						MarkMove(false);
						break;
					case kMarkMoveForward|k_Repeat:
					case kMarkMoveForward:
						MarkMove(true);
						break;
/*
					case kEditCut:
						EditCut();
						break;
					case kEditTest:
						EditTest();
						break;
*/
					default:
						{
							displayFrames = DisplayedFrames;
							switch (Key) {
								// Menu control:
								case kOk:
									if (visible && !modeOnly) {
										Hide();
										DoShowMode = true;
									} else
										Show();
									break;
								case kBack:
									kback_key = true;
									return osEnd;
								default:
									return osUnknown;
							}
						}
				}
			}
	}
	if (DoShowMode)
		ShowMode();

	return osContinue;
}
