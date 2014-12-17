/*
 * dpkg Plugin for VDR
 * menu.c: OSD menu
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include "mainmenu.h"

#include <apt-front/init.h>
#include <apt-front/predicate/factory.h>
#include <apt-front/predicate/matchers.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/acquire-item.h>

#include <typeinfo>
#include <vdr/debug.h>
#include <vdr/menu.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>

#define SOURCES_FILE "/etc/apt/sources.list"
#define SECTION_FILTER "reel"
#define OPT_SECTION_FILTER "reel-opt"
#define SEPARATOR_TXT "----------"
#define SOURCE_CHAR_ALLOWED "abcdefghijklmnopqrstuvwxyz1234567890-.:/_-# "
#define BAR_WIDTH 90

#define CMD_VIDEO_AUDIO_OFF "/usr/sbin/hdctrld -v off -V 0"
#define CMD_REBOOT "/bin/touch /var/tmp/reboot;/sbin/reelfpctl -displaymode 0 -clearlcd -showpnm /etc/reel/images/reboot/reboot.pnm"
#define CMD_SHUTDOWN "/sbin/reboot -f"
#define CMD_DPKG_UPDATE "/usr/sbin/dpkg_pi_update"
#define CMD_DPKG_COMMIT "/usr/sbin/dpkg_pi_commit"
#define CMD_DPKG_IDLE   "/usr/sbin/dpkg_pi_idle"

#define UPDATE_ERROR_DELAY_MS 5000

// --- cMainMenu -------------------------------------------------------------

cMainMenu::cMainMenu(int fMode, const char *fTitle):cOsdMenu(fTitle?fTitle:tr("Online Update")), mCache(0) {
	mCommit=0;
	mReboot=0;
	mHideStatus=0;
	mDisplay=0;
	mMode = fMode;
	mResult1="";
	mResult2="";
	aptFront::init();
	/* TB: libapt-front is evil and calls textdomain("libapt-front") - so VDR's textdomain is lost forever */
	textdomain("vdr");
	isyslog("dpkg V%s (Build 16)", VERSION);
	mApt = new cAptThread(this);
	std::string cmd = "dpkg --clear-avail";
	SystemExec(cmd.c_str());
	Update();
} // cMainMenu::cMainMenu

cMainMenu::~cMainMenu() {
	syslog(LOG_INFO, "cMainMenu::~cMainMenu");
	if(mApt) {
		mApt->Stop();
		while (mApt->Active())
			cCondWait::SleepMs(100);
		delete mApt;
	} // if
	if(mCache)
		mCache->close();
} // cMainMenu::~cMainMenu

void cMainMenu::Update() {
	mReelItems.Clear();
	SetCols(20);
	ClearState();
	ClearItems();
	SetStatusText(tr("Updating sources..."), NULL);
	if(mApt) mApt->DoUpdate();
} // cMainMenu::Update

void cMainMenu::Upgrade() {
	mState = DPKG_MODE_NONE;
	SetCols(20);
	ClearState();
	ClearItems();
	SetStatusText(tr("Updating packages..."), NULL);
	mApt->DoCommit();
} // cMainMenu::Upgrade

void cMainMenu::AddPackageItem(cPkgItem *fItem) {
	Add(fItem);
	if((mMode&DPKG_MODE_DESCR_LINE) && fItem->HasShortDescr()) {
		char *lBuff;
		asprintf(&lBuff, "\t    (%s)", fItem->GetShortDescr().c_str());
		cOsdItem *lDescr = new cOsdItem();
		lDescr->SetSelectable(false);
		lDescr->SetText(lBuff, false);
		Add(lDescr);
	} // if
} // cMainMenu::AddPackageItem

void cMainMenu::ShowReelPackages() {
	mState = DPKG_MODE_REEL_PKG;
	SetCols(3, 20);
	ClearState();
	ClearItems();
	Display();
	cPkgItem *lPos = mReelItems.First();
	while (lPos) {
		AddPackageItem(new cPkgItem(lPos->mPkg, mMode));
		lPos = (cPkgItem *)lPos->Next();
	} // while
	Display();
	SetState(mCommit);
} // cMainMenu::ShowReelPackages

void cMainMenu::ShowReelOptPackages() {
	mState = DPKG_MODE_OPT_PKG;
	SetCols(3, 20);
	ClearState();
	ClearItems();
	Display();
	cPkgItem *lPos = mReelOptItems.First();
	while (lPos) {
		AddPackageItem(new cPkgItem(lPos->mPkg, mMode));
		lPos = (cPkgItem *)lPos->Next();
	} // while
	Display();
	SetState(mCommit);
} // cMainMenu::ShowReelOptPackages

void cMainMenu::ShowSources() {
	AddSubMenu(new cSourceMenu(this));
} // cMainMenu::ShowSources

void cMainMenu::ShowReboot() {
	ClearState();
	ClearItems();
	SetStatusText(tr("Rebooting, please wait..."), NULL);

	if(mMode&DPKG_MODE_INSTALL_WIZARD) {
		char tempString[4];
#define eFinished 13 // TODO: get the number of install steps from Install plugin
		snprintf(tempString, 4, "%d", eFinished);
		cPluginManager::GetPlugin("install")->SetupStore("stepdone", eFinished);
		cPluginManager::GetPlugin("install")->SetupParse("stepdone", tempString);
		Setup.Save();
	}

	usleep (300*1000);
	SystemExec(CMD_VIDEO_AUDIO_OFF);
	SystemExec(CMD_REBOOT);
	SystemExec(CMD_SHUTDOWN);
} // cMainMenu::ShowReboot

void cMainMenu::ShowExec(const char *txt, const char *cmd) {
	if(!Interface->Confirm(std::string(std::string(txt)+"?").c_str()))
		return;
	mState = DPKG_MODE_NONE;
	SetCols(0);
	ClearState();
	ClearItems();
	SetStatusText(std::string(std::string(txt)+"...").c_str(), NULL);
	mApt->DoExecute(cmd);
} // cMainMenu::ShowFallbackUpgrade

void cMainMenu::ProcessUpdate() {
	cMutexLock lLock(&mMutex);
	if(mDisplay) {
		mDisplay=0;
		cOsdMenu::Display();
		if(displayMenu)displayMenu->Flush();
	} // if
} // cMainMenu::ProcessUpdate

eOSState cMainMenu::ProcessKey(eKeys Key) {
	ProcessUpdate();
	eOSState state = cOsdMenu::ProcessKey(Key);
	if (state == osUnknown) {

		/* in install wizard 
		 *   handling these keys here again because change in r15709 
		 *   returns osContinue instead of osUnknown 
		 */
		if(mMode&DPKG_MODE_INSTALL_WIZARD)
		{
			/* cAptThread cannot always be cleanly canceled. So,
			** do not skip/go back if apt-thread is running, see issue #795
			*/
			if (!mApt || mApt && mApt->IsIdle()) {
				if (Key == kYellow) return osUser1;
				if (Key == kGreen)  return osUser4;
			}
		} 

		if(mReboot) {
			if(Key == kRed) {
				ShowReboot();
			} else if((Key == kGreen) && mMode&DPKG_MODE_INSTALL_WIZARD) {
				return osUser4;
			} else if((Key == kYellow) && mMode&DPKG_MODE_INSTALL_WIZARD) {
				return osUser1;
			} else if(Key == kOk) {
				if(mMode&DPKG_MODE_INSTALL_WIZARD)
					return osUser1;
			}
			return osContinue; // We do a hard-reboot - don`t allow other things...
		} else {
			state = osContinue;
			switch (Key) {
				case kRed:
					if(mCommit && mApt)
						Upgrade();
					break;
				case kGreen:
					if(!mReboot && mReelItems.Count() && mApt)
						ShowReelPackages();
					break;
				case kYellow:
					if(!mReboot && mReelOptItems.Count() && mApt)
						ShowReelOptPackages();
					break;
				case kBlue:
					if(mMode&DPKG_MODE_SOURCES)
						ShowSources();
					break;
				case k1:
					if(!mReboot)
						ShowExec(tr("Execute aptitude upgrade"), "aptitude update && aptitude dist-upgrade -y -q > /tmp/dpkg.log");
					break;
				case k2:
					if(!mReboot)
						ShowExec(tr("Execute apt upgrade"), "apt-get update && apt-get dist-upgrade -y -q > /tmp/dpkg.log");
					break;
				case k3:
					if(!mReboot)
						ShowExec(tr("Execute base install"), "aptitude install rubuntu-standard -y -f -q > /tmp/dpkg.log");
					break;
				case k4:
					if(!mReboot)
						ShowExec(tr("Execute forced aptitude upgrade"), "aptitude update && aptitude dist-upgrade -y -f -q > /tmp/dpkg.log");
					break;
				case k5:
					if(!mReboot)
						ShowExec(tr("Execute forced apt upgrade"), "apt-get update && apt-get dist-upgrade -y -f -q > /tmp/dpkg.log");
					break;
				case kOk:
					if(mMode&DPKG_MODE_INSTALL_WIZARD)
						if(mApt && mApt->IsIdle())
							return osUser1;
					Display(); // Assume item has changed
					break;
				default:
				    DDD("return %d", (mApt && !mApt->IsIdle()) ? osContinue : osUnknown);
					return (mApt && !mApt->IsIdle()) ? osContinue : osUnknown;
			} // switch
		} // if
	} else if (state == osBack) {
		syslog(LOG_INFO, "state == osBack");
		if(!mApt->Abort())
			state = osContinue; // Don't allow exit while processing
	} else if (state == osUser1) {
		for (int i = 0; i < Count(); i++)
			if(Get(i)->Selectable())
				((cPkgItem *)Get(i))->Show();
		Display(); // Assume item has changed
		SetState(mCommit+1);
	} else if (state == osUser2) {
		for (int i = 0; i < Count(); i++)
			if(Get(i)->Selectable())
				((cPkgItem *)Get(i))->Show();
		Display(); // Assume item has changed
		SetState(mCommit-1);
	} // if
	ProcessUpdate();
	return state;
} // cMainMenu::ProcessKey

void cMainMenu::Display(void) {
	cMutexLock lLock(&mMutex);
	mDisplay=1;
} // cMainMenu::Display

void cMainMenu::Clear(void) {
	cOsdMenu::Clear();
} // cMainMenu::Clear

void cMainMenu::AddItem(const char *fState, const char *fText) {
	if (fState) {
		char *lBuff;
		asprintf(&lBuff, "%s:\t%s", tr(fState), fText);
		cOsdItem *lPos = First();
		while (lPos) {
			if(strstr(lPos->Text(), fText)) {
				lPos->SetText(lBuff, false);
				if(lPos->Selectable())
					SetCurrent(lPos);
				Display();
				return;
			} // while
			lPos = (cOsdItem*)lPos->Next();
		} // while
		AddItem(new cOsdItem(lBuff));
		free(lBuff);
	} else
		AddItem(new cOsdItem(fText));
} // cMainMenu::AddItem

void cMainMenu::AddItem(cOsdItem *fItem) {
	if(!fItem)
		return;
	Add(fItem);
	if(fItem->Selectable())
		SetCurrent((cOsdItem *)Last());
	Display();
} // cMainMenu::AddItem

void cMainMenu::ClearItems() {
	Clear();
	Display();
} // cMainMenu::ClearItems

void cMainMenu::ClearState() {
	SetHelp(NULL);
} // cMainMenu::ClearState

#define MAX_STATUS_LINES 9
void cMainMenu::SetStatusText(const char *fText, const char *fProgress) {
//isyslog("SetStatusText %s|%s|%d", fText, fProgress, mHideStatus);
	if(mHideStatus) return;
	cMutexLock lLock(&mMutex);
	Clear();
	Add(new cOsdItem(tr("Software update via online connection:"), osUnknown, false));
	Add(new cOsdItem("", osUnknown, false));
	mStatus = fText ? fText : "";
	mStatusProgress = fProgress;
	if(!fProgress) {
		mStatusInfo.Clear();
		mResult1="";
		mResult2="";
	} // if
	Add(new cOsdItem(mStatus, osUnknown, false));
	int lPos=0;
	if(mStatusInfo.Size()) {
		Add(new cOsdItem(SEPARATOR_TXT, osUnknown, false));
		lPos++;
//isyslog("Start at %d|%d|%d", (mStatusInfo.Size() > (MAX_STATUS_LINES-2)) ? (mStatusInfo.Size()-(MAX_STATUS_LINES-2)) : 0, mStatusInfo.Size(), (MAX_STATUS_LINES-2));
		for(int i= (mStatusInfo.Size() > (MAX_STATUS_LINES-2)) ? (mStatusInfo.Size()-(MAX_STATUS_LINES-2)) : 0; i < mStatusInfo.Size(); i++, lPos++ )
			Add(new cOsdItem(mStatusInfo.At(i), osUnknown, false));
		if(mStatusInfo.Size() && !strlen(mResult1)) {
			Last()->SetSelectable(true);
			SetCurrent(Last());
		} // if
		if(lPos < MAX_STATUS_LINES) {
			Add(new cOsdItem(SEPARATOR_TXT, osUnknown, false));
			lPos++;
		} // if
	} // if
	for (int i=lPos; i < MAX_STATUS_LINES; i++)
		Add(new cOsdItem("", osUnknown, false));
	if(fProgress)
		Add(new cOsdItem(fProgress, osUnknown, false));
	else
		Add(new cOsdItem(GetProgress(0,1), osUnknown, false));
	Add(new cOsdItem(mResult1, osUnknown, strlen(mResult1)));
	if(strlen(mResult1)) SetCurrent(Last());
	Add(new cOsdItem(mResult2, osUnknown, strlen(mResult2)));
	if(strlen(mResult2)) SetCurrent(Last());
	Display();
} // cMainMenu::SetStatusText

void cMainMenu::SetStatusLock(const char *fText) {
	mHideStatus=1;
	SetStatus(fText);
} // cMainMenu::SetStatusLock

void cMainMenu::SetStatusUnLock() {
	mHideStatus=0;
	SetStatusText(mStatus, mStatusProgress);
} // cMainMenu::SetStatusUnLock

void cMainMenu::SetState(int fCommit) {
	mCommit      = fCommit;
	int lReel    = mReelItems.Count()    && (mState != DPKG_MODE_REEL_PKG);
	int lOpt     = mReelOptItems.Count() && (mState != DPKG_MODE_OPT_PKG);
	int lSrc     = mMode&DPKG_MODE_SOURCES;
	int lUpgrade = mMode&DPKG_MODE_UPGRADE;
	if(mMode&DPKG_MODE_INSTALL_WIZARD)
		SetHelp(mCommit?lUpgrade?tr("Upgrade"):tr("Install"):NULL, tr("Back"), tr("Skip"), NULL);
	else
		SetHelp(mCommit?lUpgrade?tr("Upgrade"):tr("Install"):NULL, lReel?tr("Packages"):NULL, lOpt?tr("Optional"):NULL, lSrc?tr("Sources"):NULL);
} // cMainMenu::SetState

void cMainMenu::SetReboot() {
	mCommit   = 0;
	mReboot   = 1;
	if (cRecordControls::Active()) {
		SetStatusText(tr("Please restart your ReelBox after the recording."), NULL);
	} else {
		if(mMode&DPKG_MODE_INSTALL_WIZARD)
			SetHelp(tr("Reboot ?"), tr("Back"), tr("Button$Continue"), NULL);
		else
			SetHelp(tr("Reboot ?"), NULL, NULL, NULL);
	}
} // cMainMenu::SetReboot

cString cMainMenu::GetProgress(int pos, int max) {
	int width = BAR_WIDTH;
	int p = pos*width/max;
	if(p>width)
		p=width;
	char buffer[width+3];
	memset(&buffer[1], ' ', width);
	buffer[0] = '[';
	buffer[width+1] = ']';
	buffer[width+2] = 0;
	for (int i = 1; i <= p; i++)
		buffer[i]='|';
	return cString(buffer, false);
} // cMainMenu::GetProgress

/****************************************************************************************/

cAptThread::cAptThread(cMainMenu*fMenu):cThread("dpkg apt thread") {
	mMenu = fMenu;
	mAction = APT_IDLE;
	mCmd = NULL;
	mProgress = new cAptProgress(fMenu);
	mProgress->_ref();
} // cAptThread::cMainMenuCB

cAptThread::~cAptThread(void) {
//	syslog(LOG_INFO, "cAptThread::~cAptThread");
	if(mProgress) {
		mProgress->mMenu = 0;
		mProgress->_unref();
	} // if
} // cAptThread::~cAptThread

void cAptThread::Stop() {
	if(mProgress)mProgress->mContinue=0;
	if(mMenu) mMenu->SetStatusLock(tr("Aborting..."));
	while(mAction==APT_UPDATE)
		cCondWait::SleepMs(100);
	Cancel(3);
} // cAptThread::Stop

int cAptThread::IsIdle() {
	return mAction == APT_IDLE;
} // cAptThread::IsIdle

void cAptThread::DoUpdate() {
	mAction = APT_UPDATE;
	Start();
} // cAptThread::DoUpdate

void cAptThread::DoCommit() {
	mAction = APT_COMMIT;
	Start();
} // cAptThread::DoCommit

void cAptThread::DoExecute(const char *Cmd) {
	mAction = APT_EXEC;
	mCmd    = Cmd;
	Start();
} // cAptThread::DoExecute

int cAptThread::Abort() {
	if(IsIdle()) return 1;
	if(mAction!=APT_UPDATE) return 0;
	if(mMenu)mMenu->SetStatusLock(tr("Press OK to abort update."));
	if(!Interface->Confirm(tr("Press OK to abort update."))) {
		if(mMenu)mMenu->SetStatusUnLock();
		return 0;
	} // if
	if(mAction!=APT_UPDATE) {
		if(mMenu)mMenu->SetStatusUnLock();
		return 0;
	} // if
	if(mProgress)mProgress->mContinue=0;
	if(mMenu)
		mMenu->SetStatusLock(tr("Aborting..."));
	while(mAction==APT_UPDATE)
		cCondWait::SleepMs(100);
	return IsIdle();
} // cAptThread::Abort

int hasPackage(aptFront::cache::entity::Package pkg, aptFront::utils::Range< aptFront::cache::entity::Relation > range) {
	while(!range.empty()) {
		try {
			aptFront::utils::Range< aptFront::cache::entity::Package > item = range->targetPackages();
			while(!item.empty()) {
				try {
					if(pkg == *item)
						return 1;
				} catch(...){}
				item = item.next();
			} // while
		} catch (...){}
		range = range.next();
	} // while
	return 0;
} // hasPackage

void markDepend(aptFront::cache::entity::Package pkg, cMainMenu* menu) {
	try {
		aptFront::utils::Range< aptFront::cache::entity::Relation > dep = pkg.candidateVersion().depends();
		syslog(LOG_INFO, "Checking recommends for %s", pkg.name().c_str());
		while (!dep.empty()) {
			try {
				if (dep->type() == aptFront::cache::entity::Relation::Recommends) {
					aptFront::utils::Range< aptFront::cache::entity::Package > item = dep->targetPackages();
					while(!item.empty()) {
						try {
							syslog(LOG_INFO, "Checking for installation (%s recommends %s)", pkg.name().c_str(), item->name().c_str());
							bool lInstall = false;
							if (pkg.isInstalled()) {
								if((pkg.installedVersion() != pkg.candidateVersion()) && item->canInstall() && !hasPackage(*item, pkg.installedVersion().depends()))
									lInstall = true;
							} else if(item->canInstall())
								lInstall = true;
							if(lInstall) {
								(*item).markInstall();
								if(menu) {
									cOsdItem *lInfo = new cOsdItem();
									char *lBuff;
									asprintf(&lBuff, "    i> %s", item->name().c_str());
									lInfo->SetText(lBuff, false);
									lInfo->SetSelectable(false);
									menu->AddItem(lInfo);
								} // if
								markDepend(*item, menu);
							} // if
						} catch(...) {}
						item = item.next();
					} // while
				} // if
			} catch(...) {}
			dep = dep.next();
		} // while
	} catch(...) {}
} // markDepend

void cAptThread::Action(void) {
	if(!mMenu)
		return;
	if(!mMenu->mCache) {
		mMenu->mCache = &aptFront::cache::Global::get();
		if(mMenu->mCache)
			mMenu->mCache->open(aptFront::cache::Cache::OpenDefault | aptFront::cache::Cache::OpenReadOnly | aptFront::cache::Cache::OpenDebtags);
		else
			return;
	} // if
	try {
		switch(mAction) {
			case APT_IDLE: break;
			case APT_UPDATE: {
				try {
					SystemExec(CMD_DPKG_UPDATE);
					aptFront::Manager m(mMenu->mCache);
					if(mProgress) mProgress->SetMode(APT_UPDATE);
					m.setProgressCallback(mProgress);
					m.update();
					if(mProgress && mProgress->mFailList.Size()) {
						cCondWait::SleepMs(UPDATE_ERROR_DELAY_MS);
						mMenu->SetStatusText((mMenu->mMode&DPKG_MODE_UPGRADE) ? tr("Failed to update all sources! Searching for updates...")
							: tr("Failed to update all sources! Searching for packages..."), NULL);
					} else {
						mMenu->SetStatusText((mMenu->mMode&DPKG_MODE_UPGRADE) ? tr("Sources up to date. Searching for updates...")
							: tr("Sources up to date. Searching for packages..."), NULL);
					} // if
				} catch(...) {
					esyslog("dpkg: update crashed!");
					mMenu->SetStatusText((mMenu->mMode&DPKG_MODE_UPGRADE) ? tr("Failed to update all sources! Searching for updates...")
						: tr("Failed to update all sources! Searching for packages..."), NULL);
				} // try
				mAction = APT_SEARCH;
				if(mProgress && !mProgress->mContinue)
					break;
				if(mProgress) mProgress->SetMode(APT_SEARCH);
				int lCount = 0;
				int lFailed = 0;
				try {
					mMenu->mReelItems.Clear();
					mMenu->mReelOptItems.Clear();
					aptFront::cache::component::Packages::iterator j;
					int pos=0;
					int count = mMenu->mCache->packages().packageCount();
					for (j = mMenu->mCache->packages().packagesBegin(); j != mMenu->mCache->packages().packagesEnd(); ++ j) {
						if((mMenu->mMode&DPKG_MODE_UPGRADE) && j->isUpgradable()) {
							try {
								char *lBuff;
								asprintf(&lBuff, "%s\t (%s -> %s)", j->name().c_str(), j->installedVersion().versionString().c_str(), j->candidateVersion().versionString().c_str());
								mMenu->mStatusInfo.Append(lBuff);
								(*j).markInstall();
								markDepend(*j, mMenu);
								lCount++;
							} catch (...) {
								lFailed = 1;
								esyslog("markInstall %s crashed!", j->name().c_str());
							} // try
						} // if
						mMenu->SetStatusText(mMenu->mStatus, mMenu->GetProgress(++pos, count));
						try {
							if((j->section() == SECTION_FILTER) && (mMenu->mMode&DPKG_MODE_REEL_PKG))
								mMenu->mReelItems.Add(new cPkgItem(*j));
							else if((j->section() == OPT_SECTION_FILTER) && (mMenu->mMode&DPKG_MODE_OPT_PKG))
								mMenu->mReelOptItems.Add(new cPkgItem(*j));
						} catch(...) {
							lFailed = 1;
						} // try
					} // for
					for (j = mMenu->mCache->packages().packagesBegin(); j != mMenu->mCache->packages().packagesEnd(); ++ j) {
						if((mMenu->mMode&DPKG_MODE_UPGRADE) && (j->pointer().package()->Flags & pkgCache::Flag::Essential) && (j->markedRemove() || !j->isInstalled())) {
							isyslog("ESSENTIAL: Force install of %s (was %s would %s)", j->name().c_str(), j->statusString().c_str(), j->actionString().c_str());
							(*j).markInstall();
							markDepend(*j, mMenu);
							lCount++;
						} // if
					} // for
/*
					for (j = mMenu->mCache->packages().packagesBegin(); j != mMenu->mCache->packages().packagesEnd(); ++ j) {
						if(j->willBreak()) isyslog("%s WILL BREAK!", j->name().c_str());
						if(j->isBroken()) isyslog("%s IS BROKEN!", j->name().c_str());
						if(j->markedInstall()) isyslog("%s installs", j->name().c_str());
					}
*/
					mMenu->mReelItems.Sort();
					mMenu->mReelOptItems.Sort();
				} catch(...) {
					lFailed = 1;
				} // try
				if(lFailed) {
					esyslog("dpkg: markInstall crashed!");
					mMenu->mResult1 = tr("Failed to process all packages!");
					mMenu->SetStatusText(tr("Failed"), mMenu->GetProgress(0, 1));
				} // if
				if(!lFailed && !(mMenu->mMode&DPKG_MODE_AUTO_INSTALL)) {
					if(mMenu->mMode&DPKG_MODE_UPGRADE) {
						if(lCount) {
							mMenu->mResult1 = tr("Now select upgrade to install the new packages.");
							mMenu->SetStatusText(tr("Following packages will be updated:"), mMenu->GetProgress(0, 1));
						} else {
							mMenu->mResult1 = tr("The software is up to date.");
							mMenu->SetStatusText(tr("No updates found."), mMenu->GetProgress(1, 1));
						} // if
						mMenu->SetState(lCount);
					} else if(mMenu->mMode&DPKG_MODE_PKG_JUMP) {
						if (mMenu->mMode&DPKG_MODE_OPT_PKG && mMenu->mMode&DPKG_MODE_OPT_FIRST && mMenu->mReelOptItems.Count()) {
							mMenu->ShowReelOptPackages();
						} else if (mMenu->mMode&DPKG_MODE_REEL_PKG && mMenu->mReelItems.Count()) {
							mMenu->ShowReelPackages();
						} else if (mMenu->mMode&DPKG_MODE_OPT_PKG && mMenu->mReelOptItems.Count()) {
							mMenu->ShowReelOptPackages();
						} else {
							mMenu->SetState(lCount);
						} // if
					} else {
						mMenu->SetState(lCount);
					} // if
				} else {
					mMenu->SetState(lCount);
				} // if
				if(lFailed || !(mMenu->mMode&DPKG_MODE_AUTO_INSTALL))
					break;
				// Don't break if mode is DPKG_MODE_AUTO_INSTALL
				mAction = APT_COMMIT;
				mMenu->SetState(0);
			} // APT_UPDATE
			case APT_COMMIT: {
				SystemExec(CMD_DPKG_COMMIT);
				try {
					cMyManager m(mMenu);
					try {
						if(mProgress) mProgress->SetMode(APT_COMMIT);
						m.setProgressCallback(mProgress);
						mMenu->SetStatusText(tr("Downloading Packages..."), NULL);
						m.download();
						if(mProgress && mProgress->mFailList.Size())
							cCondWait::SleepMs(UPDATE_ERROR_DELAY_MS);
						mMenu->SetStatusText(tr("Processing Packages..."), NULL);
						m.myCommit();
						sync();
						mMenu->mResult1 = tr("All changes successfull!");
						mMenu->mResult2 = tr("Please restart your ReelBox!");
						mMenu->mStatusInfo.Clear();
						mMenu->SetStatusText("", mMenu->GetProgress(1, 1));
						mMenu->SetReboot();
					} catch(...) {
						esyslog("dpkg: commit crashed!");
						mMenu->mResult1 = tr("Please go into deep standby,");
						mMenu->mResult2 = tr("restart the ReelBox and try the update again.");
						mMenu->mStatusInfo.Clear();
						for(int i=0; i<m.mErrorList.Size(); i++) {
							mMenu->mStatusInfo.Append(strdup(m.mErrorList.At(i)));
						}
						mMenu->SetStatusText(tr("Failed to process all packages!"), mMenu->GetProgress(0, 1));
						mMenu->SetReboot();
					} // try
				} catch(...) {}
				break;
			} // // APT_COMMIT
			case APT_EXEC: {
				if(mMenu->mCache) {
					mMenu->mCache->close();
					mMenu->mCache = NULL;
				} // if
				if(mCmd) SystemExec(mCmd);
				mMenu->mResult1 = tr("Please restart your ReelBox!");
				mMenu->mResult2 = "";
				mMenu->mStatusInfo.Clear();
				mMenu->SetStatusText("", mMenu->GetProgress(1, 1));
				mMenu->SetReboot();
				break;
			} // APT_EXEC
		} // switch
	} catch (...) {}
	SystemExec(CMD_DPKG_IDLE);
	mAction = APT_IDLE;
} // cAptThread::Action

/****************************************************************************************/

cMyManager::cMyManager(cMainMenu*fMenu):aptFront::Manager(fMenu->mCache) {
	mStatusRead  = -1;
	mStatusPos   = 0;
	mMenu = fMenu;
} // cMyManager::cMyManager

cMyManager::~cMyManager() {
	if (mStatusRead  != -1) close(mStatusRead);
	mStatusRead  = -1;
} // cMyManager::~cMyManager

void cMyManager::Action(void) {
	while(mStatusRead != -1) {
		while (char *lInfo = getInfo()) {
			char *lPackage = NULL;
			char *lSave;
			char *lTok = strtok_r(lInfo, ":", &lSave);
			if (!lTok) continue; // state
			int lIsStatus = !strcmp(lTok, "pmstatus");
			int lIsError  = !strcmp(lTok, "pmerror");
			if(!lIsStatus && !lIsError) continue;
			if(!(lTok = strtok_r(NULL, ":", &lSave))) continue; // package
			asprintf(&lPackage, ": %s", lTok);
			if(!(lTok = strtok_r(NULL, ":", &lSave))) continue; // progress
			int lPos = atoi(lTok);
			if(!(lTok = strtok_r(NULL, ":", &lSave))) continue; // text
			if (mMenu) {
				char *lBuff = NULL;
				asprintf(&lBuff, "%s%s", lIsError ? tr("Error") : "", lIsError ? lPackage : lTok);
				if(lIsError) {
					mErrorList.Append(lBuff);
					mMenu->mStatusInfo.Append(strdup(lBuff));
					const cFont *lFont = mMenu->displayMenu->GetTextAreaFont(false);
					cTextWrapper lTxt (lTok, lFont, mMenu->displayMenu->GetTextAreaWidth()-lFont->Width(' '));
					for(int i=0; i< lTxt.Lines(); i++) {
						lBuff=NULL;
						asprintf(&lBuff, " %s", lTxt.GetLine(i));
						mErrorList.Append(lBuff);
					} // if
				} else {
					mMenu->mStatusInfo.Append(lBuff);
				} // if
				mMenu->SetStatusText(mMenu->mStatus, mMenu->GetProgress(lPos, 100));
			} // if
			if(lPackage) free(lPackage);
		} // while
		cCondWait::SleepMs(100);
	} // while
} // cMyManager::Action

void cMyManager::myCommit() {
	if (!m_pm) {
		esyslog("dpkg: Tried to commit without PM");
		char *lBuff = NULL;
		asprintf(&lBuff, "An other program already uses the packet manager.");
		mErrorList.Append(lBuff);
		throw aptFront::exception::InternalError( "Tried to commit without PM" );
	} // if
	int fds[2];
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fds) < 0) {
		esyslog("dpkg: unable to create status fd");
		fds[0] = -1;
		mStatusRead  = -1;
	} else {
		mStatusRead  = fds[1];
		fcntl(mStatusRead, F_SETFL, O_NONBLOCK);
	} // if
	mStatusPos   = 0;
	Start();
    // Start of orig code...
    // mornfall: is it me or this looks like a race?
	while (1) {
		_system->UnLock(); // unlock for dpkg to take over
		pkgPackageManager::OrderResult Res = m_pm->DoInstall(fds[0]);
		if (Res == pkgPackageManager::Completed) {
			_system->Lock(); // regain lock
			delete m_pm;
			m_pm = 0;
			aptFront::cache::Global::get( m_cache ).reopen(); // reopen cache
			mStatusRead  = -1;
			if (fds[0]  != -1) close(fds[0]);
			if (fds[1]  != -1) close(fds[1]);
			return;
		} else if (Res == pkgPackageManager::Failed) {
			mStatusRead  = -1;
			if (fds[0]  != -1) close(fds[0]);
			if (fds[1]  != -1) close(fds[1]);
			throw aptFront::exception::Error( "Error installing packages" );
		}
		aptFront::exception::checkGlobal( "Error installing packages" );
		getArchives();
	}
} // cMyManager::myCommit

char *cMyManager::getInfo() {
	while(read(mStatusRead, &mStatusBuff[mStatusPos], 1)==1) {
		if(mStatusBuff[mStatusPos] == '\n') {
			mStatusBuff[mStatusPos] = 0;
			mStatusPos = 0;
			return mStatusBuff;
		} // if
		mStatusPos++;
		if(mStatusPos >= sizeof(mStatusBuff))
			mStatusPos = 0;
	} // while
	return 0;
} // cMyManager::getInfo

/****************************************************************************************/

cAptProgress::cAptProgress(cMainMenu*fMenu) {
	mMenu = fMenu;
	mContinue = 1;
} // cAptProgress::cAptProgress

void cAptProgress::IMSHit(pkgAcquire::ItemDesc &Itm) {
	try {
		if(mMode != APT_UPDATE) return;
		char *lTok = strdup(Itm.Description.c_str());
		if(strchr(lTok, ' ')) *strchr(lTok, ' ') = 0;
		int pos = mFailList.Find(lTok);
		if(pos != -1)
			mFailList.Remove(pos);
		if(mDoneList.Find(lTok) == -1)
			mDoneList.Append(lTok);
		else
			free(lTok);
	} catch (...) {}
} // cAptProgress::IMSHit

void cAptProgress::Fetch(pkgAcquire::ItemDesc &Itm) {
	try {
		char *lLast;
		asprintf(&lLast, "%s %s", tr("Load"), Itm.Description.c_str());
		int lFound=0;
		for(int i=0; (i< mProgressList.Size()) && !lFound; i++)
			if(strstr(mProgressList.At(i), Itm.Description.c_str()))
				lFound=1;
		if(!lFound)
			mProgressList.Append(lLast);
		char *lTok = strdup(Itm.Description.c_str());
		if(strchr(lTok, ' ')) *strchr(lTok, ' ') = 0;
		int pos = mFailList.Find(lTok);
		if(pos != -1)
			mFailList.Remove(pos);
		if(mDoneList.Find(lTok) == -1)
			mDoneList.Append(lTok);
		else
			free(lTok);
	} catch (...) {}
} // cAptProgress::Fetch

void cAptProgress::Done(pkgAcquire::ItemDesc &Itm) {
	try {
		char *lLast;
		asprintf(&lLast, "%s %s", tr("OK"), Itm.Description.c_str());
		for(int i=0; i< mProgressList.Size(); i++)
			if(strstr(mProgressList.At(i), Itm.Description.c_str()))
				mProgressList.Remove(i--);
		mProgressList.Append(lLast);
		char *lTok = strdup(Itm.Description.c_str());
		if(strchr(lTok, ' ')) *strchr(lTok, ' ') = 0;
		int pos = mFailList.Find(lTok);
		if(pos != -1)
			mFailList.Remove(pos);
		if(mDoneList.Find(lTok) == -1)
			mDoneList.Append(lTok);
		else
			free(lTok);
	} catch (...) {}
} // cAptProgress::Done

void cAptProgress::Fail(pkgAcquire::ItemDesc &Itm) {
	try {
		if(Itm.Owner && (pkgAcquire::Item::StatError > Itm.Owner->Status)) return;
		char *lLast;
		asprintf(&lLast, "%s %s", tr("Error"), Itm.Description.c_str());
		for(int i=0; i< mProgressList.Size(); i++)
			if(strstr(mProgressList.At(i), Itm.Description.c_str()))
				mProgressList.Remove(i--);
		mProgressList.Append(lLast);
		char *lTok = strdup(Itm.Description.c_str());
		if(strchr(lTok, ' ')) *strchr(lTok, ' ') = 0;
		if(lTok && (mDoneList.Find(lTok) == -1) && (mFailList.Find(lTok) == -1))
			mFailList.Append(lTok);
		else
			free(lTok);
	} catch (...) {}
} // cAptProgress::Fail

void cAptProgress::UpdatePulse(double FetchedBytes, double CurrentCPS, unsigned long CurrentItems) {
	try {
		if(mMode == APT_UPDATE) {
			mMenu->mStatusInfo.Clear();
			if(!mDoneList.Size() && mFailList.Size())
				mMenu->mResult1 = tr("Please check your internet connection.");
			else if(!mDoneList.Size() && mTimeout.TimedOut())
				mMenu->mResult1 = tr("Please check your internet connection.");
			else
				mMenu->mResult1 = "";
			for(int i=0; i < mDoneList.Size(); i++) {
				char *lBuff;
				asprintf(&lBuff, "%s: %s", tr("OK"),mDoneList.At(i));
				mMenu->mStatusInfo.Append(lBuff);
			} // for
			for(int i=0; i < mFailList.Size(); i++) {
				char *lBuff;
				asprintf(&lBuff, "%s: %s", tr("Error"), mFailList.At(i));
				mMenu->mStatusInfo.Append(lBuff);
			} // for
		} else if (mMode == APT_COMMIT) {
			mMenu->mStatusInfo.Clear();
			if(!mDoneList.Size() && mFailList.Size())
				mMenu->mResult1 = tr("Please check your internet connection.");
			else
				mMenu->mResult1 = "";
			for(int i=0; i < mProgressList.Size(); i++) {
				char *lState = NULL;
				char *lServer = NULL;
				char *lSave = NULL;
				char *lBuff = NULL;
				char *lTok = strtok_r((char *)(const char *)mProgressList.At(i), " ", &lSave);
				if (!lTok) return; // State
				if (!strcmp(lTok, tr("IGN"))) goto ignore;
				asprintf(&lState, "%s", lTok);
				if(!(lTok = strtok_r(NULL, " ", &lSave))) goto ignore; // Server
				asprintf(&lServer, "%s", lTok);
				if(!(lTok = strtok_r(NULL, " ", &lSave))) goto ignore; // Path
				if(!(lTok = strtok_r(NULL, " ", &lSave))) goto ignore; // Package
				asprintf(&lBuff, "%s: %s\t %s", lState, lServer, lTok);
				mMenu->mStatusInfo.Append(lBuff);
ignore:
				if(lServer) free(lServer);
				if(lState) free(lState);
			} // for
		} // if
		mMenu->SetStatusText(mMenu->mStatus, mMenu->GetProgress(CurrentItems, TotalItems));
	} catch (...) {}
} // cAptProgress::UpdatePulse

void cAptProgress::SetMode(int fMode) {
	mMode = fMode;
	mDoneList.Clear();
	mFailList.Clear();
	mErrorList.Clear();
	mGood=0;
	mTimeout.Set(INET_TIMEOUT);
} // cAptProgress::SetMode

bool cAptProgress::Pulse(pkgAcquire *Owner) {
	if(!aptFront::ProgressCallback::Pulse(Owner))
		mContinue=0;
	return mContinue;
} // cAptProgress::Pulse

/****************************************************************************************/

cPkgItem::cPkgItem(aptFront::cache::entity::Package fPkg, int fMode) {
	mMode = fMode;
	mPkg  = fPkg;
	Show();
} // cPkgItem::cPkgItem

cPkgItem::~cPkgItem() {
} // cPkgItem::~cPkgItem

int cPkgItem::Compare(const cListObject &ListObject) const {
	return strcmp(mPkg.name().c_str(), ((cPkgItem &)ListObject).mPkg.name().c_str());
} // cPkgItem::Compare

int cPkgItem::HasShortDescr() {
	return ((mMode&DPKG_MODE_DESCR_NAME) || (mMode&DPKG_MODE_DESCR_LINE)) && mPkg.hasVersion();
} // cPkgItem::HasShortDescr

std::string cPkgItem::GetShortDescr() {
	return mPkg.shortDescription();
} // cPkgItem::GetShortDescr

void cPkgItem::Show(void) {
	char *buffer = NULL;
	std::string lText = mPkg.name();
	if(HasShortDescr() && (mMode&DPKG_MODE_DESCR_NAME))
		lText += "\t ("+GetShortDescr()+")";
	if (mPkg.isInstalled()) {
		if (mPkg.markedUpgrade())
			asprintf(&buffer, "u>\t%s",lText.c_str());
		else if(mPkg.markedPurge())
			asprintf(&buffer, "d>\t%s",lText.c_str());
		else if(mPkg.markedInstall())
			asprintf(&buffer, "i>\t%s",lText.c_str());
		else
			asprintf(&buffer, "-i-\t%s",lText.c_str());
	} else {
		if(mPkg.markedInstall())
			asprintf(&buffer, "i>\t%s",lText.c_str());
		else
			asprintf(&buffer, "\t%s",lText.c_str());
	} // if
	SetText(buffer, false);
} // cPkgItem::Show

eOSState cPkgItem::ProcessKey(eKeys Key) {
	if(Key != kOk)
		return osUnknown;
/*
	std::string lAction = "ProcessKey:";
	lAction += mPkg.name();
	lAction += "->Marked: ";
	if(mPkg.markedInstall()) lAction += "Install ";
	if(mPkg.markedUpgrade()) lAction += "Upgrade ";
	if(mPkg.markedNewInstall()) lAction += "NewInstall ";
	if(mPkg.markedReInstall()) lAction += "ReInstall ";
	if(mPkg.markedKeep()) lAction += "Keep ";
	if(mPkg.markedRemove()) lAction += "Remove ";
	if(mPkg.markedPurge()) lAction += "Purge ";
	lAction += "->Is: ";
	if(mPkg.isInstalled()) lAction += "Installed ";
	if(mPkg.isUpgradable()) lAction += "Upgradable ";
	if(mPkg.isBroken()) lAction += "Broken ";
	lAction += "->Can: ";
	if(mPkg.canInstall()) lAction += "Install ";
	if(mPkg.canRemove()) lAction += "Remove ";
	if(mPkg.canKeep()) lAction += "Keep ";
	if(mPkg.canUpgrade()) lAction += "Upgrade ";
	if(mPkg.canReInstall()) lAction += "ReInstall ";
	if(mPkg.canRevertInstall()) lAction += "RevertInstall ";
	isyslog(lAction.c_str());
*/
	if(mPkg.canInstall()) {
		mPkg.markInstall();
		markDepend(mPkg, NULL);
		Show();
		return mPkg.markedKeep() ? osUser2 : osUser1;
	} else if(mPkg.canRemove()) {
		mPkg.markPurge();
		Show();
		return mPkg.markedKeep() ? osUser2 : osUser1;
	} else if(mPkg.canKeep()) {
		mPkg.markKeep();
		Show();
		return osUser2;
	} // if
	return cOsdItem::ProcessKey(Key);
} // cPkgItem::ProcessKey

/****************************************************************************************/

cSourceMenu::cSourceMenu(cMainMenu *fMenu):cOsdMenu(tr("Source list"), 2) { // since there is a ":" and a tab character! Donot set to 0 when there is a Tab Character!
	mMenu = fMenu;
	ReadFile();
} // cSourceMenu::cSourceMenu

cSourceMenu::~cSourceMenu(void) {
} // cSourceMenu::~cSourceMenu

eOSState cSourceMenu::ProcessKey(eKeys Key) {
	eOSState state = cOsdMenu::ProcessKey(Key);
	if (state == osUnknown) {
		switch (Key) {
			case kRed: // Save
				if(!mFailed)
					WriteFile();
				if(mMenu) {
					mMenu->mState = DPKG_MODE_NONE;
					mMenu->Update();
				} // if
				return osBack;
			case kGreen: // Undo
				ReadFile();
				return osContinue;
			case kYellow: // Insert
				Ins(new cSourceItem("deb http://"), true, Get(Current()));
				Display();
				return osContinue;
			case kBlue: // Delete
				if(Current() > -1)
					Del(Current());
				Display();
				return osContinue;
			default:
				return osContinue;
		} // switch
	} else if (state == osContinue) {
		if(Key == kBack || Key == kOk)
			ShowHelp();
	} // switch
	return state;
} // cSourceMenu::ProcessKey

void cSourceMenu::ShowHelp() {
	if(mFailed)
		SetHelp(NULL);
	else
		SetHelp(tr("Save"), tr("Undo"), tr("Insert"), tr("Delete"));
} //cSourceMenu::ShowHelp

void cSourceMenu::ReadFile() {
	Clear();
	FILE *lFile = fopen(SOURCES_FILE, "r");
	if(lFile) {
		cReadLine lReadLine;
		char *s;
		while ((s=lReadLine.Read(lFile)) != NULL)
			Add(new cSourceItem(s));
		fclose(lFile);
		mFailed = 0;
	} else {
		SetStatus(tr("Unable to read from sources.list!"));
		mFailed = 1;
	} // if
	ShowHelp();
	Display();
} //cSourceMenu::ReadFile

void cSourceMenu::WriteFile() {
	FILE *lFile = fopen(SOURCES_FILE, "w");
	if(lFile) {
		for (int i = 0; i < Count(); i++)
			fprintf(lFile, "%s\n", ((cSourceItem *)Get(i))->mLine);
		fclose(lFile);
	} else {
		Skins.Message(mtError, tr("Unable to write to surces.list!"), 3);
	} // if
} // cSourceMenu::WriteFile

/****************************************************************************************/

cSourceItem::cSourceItem(const char *fLine):cMenuEditStrItem("", mLine, SOURCE_CHARS_MAX, SOURCE_CHAR_ALLOWED) {
	memset(mLine, 0, sizeof(mLine));
	strncpy(mLine, fLine, SOURCE_CHARS_MAX);
	SetValue(mLine);
} // cSourceItem::cSourceItem

cSourceItem::~cSourceItem() {
} // cSourceItem::~cSourceItem

eOSState cSourceItem::ProcessKey(eKeys Key) {
	return cMenuEditStrItem::ProcessKey(Key);
} // cSourceItem::ProcessKey
