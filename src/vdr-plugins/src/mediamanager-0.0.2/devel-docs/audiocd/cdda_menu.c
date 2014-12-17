#include <stdio.h>

#include <vdr/menu.h>
#include <vdr/menuitems.h>

#include "mediaservice.h"
#include "cdda_menu.h"
#include "cdda_player.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "debug.h"

cMediaCddaMenu::cMediaCddaMenu(cMediaCddaDisc *Disc)
: cOsdMenu("CD Player")
{
	int w = DisplayMenu()->GetTextAreaWidth();
	int mc = 20;
	if(w) {
		mc = w - 3*14;
		mc /= 14;
		mc -= 1;
	}
	ejectrequest = false;
	disc = Disc;
	int current = Current();
	if(disc->LastReplayed() > -1)
		current = disc->LastReplayed();

	Clear();
	SetCols(3,mc);
	if(disc->Count() > 0) {
		int i;
		cMediaCddaTrack *track;
		char buf[128];
DBG_D("cMediaCddaMenu: Disc has %d tracks", disc->Count())
		snprintf(buf, sizeof(buf), "%s: %s",
								disc->Performer(), disc->Title());
		SetTitle(buf);
		for(i = 0; i < disc->Count(); i++) {
			track = disc->Get(i);
			if(track) {
				char *time = disc->TrackTimeHMS(track->TrackSize(), true);
				/*
				snprintf(buf, sizeof(buf), "% 2d\t%s\t %s\t00:%s",
						track->Track(), track->Title(),
						track->Performer(), time);
				*/
				snprintf(buf, sizeof(buf), "%02d\t%s\t%s",
						track->Track(), track->Title(), time);

				Add(new cOsdItem(buf));
				if(time)
					free(time);
			}
		}
		SetCurrent(Get(current));
		SetHelp(tr("Button$Play") ,tr("Button$Rewind") ,
			cMediaService::IsReplaying() ? NULL : tr("Button$Eject"));
	} else {
		Add(new cOsdItem(tr("No tracks available"),osUnknown ,false));
		SetHelp(tr("Button$Close"));
	}

	Display();
}

cMediaCddaMenu::~cMediaCddaMenu()
{
//DBG_D("cMediaCddaMenu::~cMediaCddaMenu()")
	if(ejectrequest)
		cMediaService::EjectDisc();
}

eOSState cMediaCddaMenu::ProcessKey(eKeys Key)
{
	eOSState state = cOsdMenu::ProcessKey(Key);
	
	if(!cMediaService::IsActive())
		return osEnd;

	if(Key != kNone) {
		int current = Current();
		switch (Key & ~k_Repeat) {
			case kOk:
			case kRed:
				cControl::Shutdown();
				cControl::Launch(new cMediaCddaControl(disc, current));
				return osEnd;
				break;
			case kGreen:
				cControl::Shutdown();
				cControl::Launch(new cMediaCddaControl(disc, 0));
				return osEnd;
				break;
			case kYellow:
				if(!cMediaService::IsReplaying()) {
					ejectrequest = true;
					state = osEnd;
				}
				break;
			default:
				break;
		}
	}
	return state;
}

