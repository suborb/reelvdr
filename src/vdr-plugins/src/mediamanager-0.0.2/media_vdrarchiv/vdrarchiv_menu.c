/*
 * vdrarchiv_menu.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/menu.h>
#include <vdr/menuitems.h>
#include <vdr/remote.h>
#include <vdr/status.h>

#include "media_vdrarchiv/vdrarchiv_menu.h"
#include "media_vdrarchiv/vdrarchiv_player.h"

#include "mediahandler.h"

//#define DEBUG_I 1
//#define DEBUG_D 1
#include "mediadebug.h"

class cMediaVDRArchivMenuInfo : public cOsdMenu {
  private:
	cMediaVDRArchivRecInfo *info;
	bool withButtons;
	eOSState Play();
	eOSState Rewind();
  public:
	cMediaVDRArchivMenuInfo(cMediaVDRArchivRecInfo *Info, bool WithButtons = false);
	virtual void Display(void);
	virtual eOSState ProcessKey(eKeys Key);
};

cMediaVDRArchivMenuInfo::cMediaVDRArchivMenuInfo(cMediaVDRArchivRecInfo *Info, bool WithButtons)
:cOsdMenu(tr("Recording info"))
{
	info = Info;
	withButtons = WithButtons;
	if(withButtons)
		SetHelp(tr("Button$Play"),tr("Button$Rewind"),NULL,tr("Button$Back"));
}

void cMediaVDRArchivMenuInfo::Display(void)
{
	char *buf = NULL;

	asprintf(&buf, "%s\n\n%s\n\n%s", info->Title() ? info->Title() : "n/a",
						info->ShortText() ? info->ShortText() : "n/a",
						info->Description() ? info->Description() : "n/a");

	cOsdMenu::Display();
	DisplayMenu()->SetText(buf, false);
	cStatus::MsgOsdTextItem(info->Description());
	free(buf);
}

eOSState cMediaVDRArchivMenuInfo::ProcessKey(eKeys Key)
{
	switch(Key) {
		case kUp|k_Repeat:
		case kUp:
		case kDown|k_Repeat:
		case kDown:
		case kLeft|k_Repeat:
		case kLeft:
		case kRight|k_Repeat:
		case kRight:
			DisplayMenu()->Scroll(NORMALKEY(Key)==kUp||NORMALKEY(Key)==kLeft,
								NORMALKEY(Key)==kLeft||NORMALKEY(Key)==kRight);
			cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
			return osContinue;
		default:
			break;
	}

	eOSState state=cOsdMenu::ProcessKey(Key);
	if(state==osUnknown) {
		switch(Key) {
			case kRed:
				if(withButtons)
					Key=kOk; // will play the recording, even if recording commands are defined
			case kGreen:
				if(!withButtons)
					break;
				cRemote::Put(Key,true);
			case kBlue:
			case kOk:
				return osBack;
			default:
				break;
		}
	}
	return state;
}

cMediaVDRArchivMenuItem::cMediaVDRArchivMenuItem(cMediaVDRArchivRec *Recording, int Level)
{
	info = Recording->RecInfo();
	filename = Recording->RecDir();
	title = Recording->TitelByLevel(Level);
	has_info = Recording->HasInfo();
	has_summary = Recording->HasSummary();
	track = Recording->Track();
	is_dir = Recording->IsDirectory(Level);
	SetText(title);

}

cMediaVDRArchivMenuItem::~cMediaVDRArchivMenuItem()
{
}



cMediaVDRArchivMenu::cMediaVDRArchivMenu(cMediaVDRArchivDisc *Disc,
const char *Base, int Level) : cOsdMenu(Base?Base:"")
{
	ejectrequest = false;
	lastreplayed_insubdir = false;
	disc = Disc;
	base = Base ? strdup(Base) : NULL;
	level = Level;
	helpkeys = 0;
	SetCols(8,6);
	Display();
	Set();

	if(lastreplayed_insubdir)
		Open();

	Display();
	SetHelpKeys();
}

cMediaVDRArchivMenu::~cMediaVDRArchivMenu()
{
DBG_D("cMediaVDRArchivMenu::~cMediaVDRArchivMenu(): level %d", level)
	if(base)
		free(base);

	if(ejectrequest)
		cMediaHandler::SetHandlerEjectRequest(true);
}

void cMediaVDRArchivMenu::Set(void)
{
	lastreplayed_insubdir = false;

//DBG_D("cMediaVDRArchivMenu::Set level %d",level)
	Clear();
	if(level == 0)
		SetTitle(tr("VDR-Archives"));

	int current = 0;
	int item_count = -1;
	cMediaVDRArchivRec *rec;
	for(int i = 0; i < disc->Count(); i++) {
		rec = disc->Get(i);
		if(rec) {
			if(rec->Level() >= level) {
				item_count++;
				Add(new cMediaVDRArchivMenuItem(rec, level));
				if(disc->LastReplayed() == rec->Track()) {
					current = item_count;
DBG_D("cMediaVDRArchivMenu::Set track %d",rec->Track())
					if(rec->Level() > level)
						lastreplayed_insubdir = true;
				}
			}
		}
	}
	SetCurrent(Get(current));
}

void cMediaVDRArchivMenu::Open(void)
{
//DBG_D("cMediaVDRArchivMenu::Open 1")
	cMediaVDRArchivMenuItem *item=(cMediaVDRArchivMenuItem*)Get(Current());
	if(item && item->IsDirectory()) {
		const char *t = item->Title();
		char *buffer = NULL;
		if(base) {
			asprintf(&buffer,"%s~%s",base,t);
			t = buffer;
		}
		AddSubMenu(new cMediaVDRArchivMenu(disc, t, level+1));
		if(buffer)
			free(buffer);
	}
//DBG_D("cMediaVDRArchivMenu::Open 2")
}

eOSState cMediaVDRArchivMenu::Play(void)
{
//DBG_D("cMediaVDRArchivMenu::Play")
	cMediaVDRArchivMenuItem *item=(cMediaVDRArchivMenuItem*)Get(Current());
	if(item) {
		if(item->IsDirectory()) {
			Open();
		} else {
//DBG_D("cMediaVDRArchivMenu::Play SetLastReplayed %d", item->Track())
			cMediaVDRArchivRec *rec = disc->Get(item->Track());
			disc->SetLastReplayed(item->Track());
			cMediaVDRArchivControl::SetRecording(rec->RecDir(), rec->Name());
			cControl::Shutdown();
			cControl::Launch(new cMediaVDRArchivControl(disc));
//DBG_D("cMediaVDRArchivMenu::Play return osEnd")
			return osEnd;
		}
	}
//DBG_D("cMediaVDRArchivMenu::Play return osContinue")
	return osContinue;
}

eOSState cMediaVDRArchivMenu::Rewind(void)
{
	if(HasSubMenu()||Count()==0)
		return osContinue;

	cMediaVDRArchivMenuItem *item=(cMediaVDRArchivMenuItem*)Get(Current());
	if(item && !item->IsDirectory()) {
		cDevice::PrimaryDevice()->StopReplay();
		cResumeFile ResumeFile(item->FileName());
		ResumeFile.Delete();
		return Play();
	}
	return osContinue;
}

eOSState cMediaVDRArchivMenu::Info(void)
{
	if(HasSubMenu() || Count()==0)
		return osContinue;

	cMediaVDRArchivMenuItem *item=(cMediaVDRArchivMenuItem*)Get(Current());
	if(item) {
		if(item->IsDirectory() || !item->HasInfo())
			return osContinue;
		return AddSubMenu(new cMediaVDRArchivMenuInfo(item->RecInfo(),true));
	}
 return osContinue;
}

void cMediaVDRArchivMenu::SetHelpKeys(void)
{
	cMediaVDRArchivMenuItem *item=(cMediaVDRArchivMenuItem*)Get(Current());
	if(item) {
		int newhelpkeys = 0;
		if(item->IsDirectory()) {
			newhelpkeys = 1;
		} else {
			newhelpkeys = 2;
			if(item->HasInfo() || item->HasSummary())
				newhelpkeys = 3;
		}
		if(helpkeys != newhelpkeys) {
			bool replays = cMediaHandler::HandlerIsReplaying();
			helpkeys = newhelpkeys;
			switch(helpkeys) {
				case 0: SetHelp(NULL); break;
				case 1:
					SetHelp(tr("Button$Open"), NULL,
						replays ? NULL : tr("Button$Eject"), NULL);
					break;
				case 2:
					SetHelp(tr("Button$Play"), tr("Button$Rewind"),
						replays ? NULL : tr("Button$Eject"), NULL);
					break;
				case 3:
					SetHelp(tr("Button$Play"), tr("Button$Rewind"),
						replays ? NULL : tr("Button$Eject"), tr("Info"));
					break;
				default: SetHelp(NULL); break;
			}
		}
	}
}

eOSState cMediaVDRArchivMenu::ProcessKey(eKeys Key)
{
	eOSState state = cOsdMenu::ProcessKey(Key);
	
	if(!cMediaHandler::HandlerIsActive())
		return osEnd;

	if((Key == kUp) || (Key == kDown))
		SetHelpKeys();

	if((Key != kNone) && (state == osUnknown)) {
//DBG_D("cMediaVDRArchivMenu::ProcessKey(eKeys Key) key %d state %d level %d",Key,state, level)
		switch (Key & ~k_Repeat) {
			case kOk:
			case kRed:
				return Play();
				break;
			case kGreen:
				return Rewind();
				break;
			case kYellow:
				if(!cMediaHandler::HandlerIsReplaying()) {
					ejectrequest = true;
					state = osEnd;
				}
				break;
			case kBlue:
				return Info();
				break;
			default:
				break;
		}
	}

	return state;
}
